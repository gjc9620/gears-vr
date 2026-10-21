"""Run inside IDAPython on a private database copy. Never saves the IDB.

Native batch decompilation is followed by cached per-function extraction for
independent files and precise failures. No function bodies cross MCP transport.
"""
import hashlib
import json
import os
from pathlib import Path
import time


def digest(path):
    with open(path, 'rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def atomic(path, value):
    path = Path(path)
    temp = path.with_suffix(path.suffix + '.tmp')
    temp.write_text(value, encoding='utf-8')
    os.replace(temp, path)


def export(config_path, max_new=25):
    import idaapi
    import ida_bytes
    import ida_funcs
    import ida_hexrays
    import ida_nalt
    import ida_xref
    import idautils
    import idc

    config = json.loads(Path(config_path).read_text(encoding='utf-8-sig'))
    root = Path(config['output'])
    root.mkdir(parents=True, exist_ok=True)
    lock = root / 'export.lock'
    # Exclusive owner; stale lock needs inspection, never automatic takeover.
    with lock.open('x') as stream:
        stream.write(str(os.getpid()))
    started = time.monotonic()
    try:
        artifact = config['artifact']
        if digest(artifact['path']) != artifact['sha256']:
            raise ValueError('snapshot hash mismatch')
        if digest(ida_nalt.get_input_file_path()) != artifact['sha256']:
            raise ValueError('IDA input differs from selected snapshot')
        if not ida_hexrays.init_hexrays_plugin():
            raise RuntimeError('Hex-Rays unavailable')
        base = int(artifact['module_base'], 0)
        load_address = int(config['load_address'], 0)
        source = Path(artifact['path']).read_bytes()
        all_functions = list(idautils.Functions())
        profile = {'config': config, 'exporter_sha256': digest(__file__),
                   'ida': idaapi.get_kernel_version(),
                   'hexrays': ida_hexrays.get_hexrays_version()}
        profile_hash = hashlib.sha256(json.dumps(profile, sort_keys=True).encode()).hexdigest()
        profile_path = root / 'profile.json'
        if profile_path.exists():
            old = json.loads(profile_path.read_text())
            if old['hash'] != profile_hash:
                raise ValueError('changed profile: use a new output directory')
        else:
            atomic(profile_path, json.dumps({'hash': profile_hash, **profile}, indent=2))
        selection_path = root / 'selection.json'
        if selection_path.exists():
            selected = json.loads(selection_path.read_text())
        else:
            excluded = {base + int(x, 0) for x in config.get('exclude_rvas', [])}
            eligible = [ea for ea in all_functions if ea not in excluded and not
                        ida_funcs.get_func(ea).flags & (ida_funcs.FUNC_LIB | ida_funcs.FUNC_THUNK)]
            seeds = [int(x, 0) + base for x in config.get('seed_rvas', [])]
            selected = list(dict.fromkeys(ea for ea in seeds if ea in eligible))
            # Deterministic spread across address space, not just startup code.
            count = min(config['limit'], len(eligible))
            for i in range(count):
                ea = eligible[i * len(eligible) // count]
                if ea not in selected:
                    selected.append(ea)
                if len(selected) == count:
                    break
            atomic(selection_path, json.dumps(selected))
        for directory in ('records', 'pseudocode', 'batches'):
            (root / directory).mkdir(exist_ok=True)
        pending = [ea for ea in selected if not (root / 'records' / f'{ea:x}.json').exists()][:max_new]
        batch_ok = None
        batch_error = None
        batch_start = time.monotonic()
        if pending:  # Empty list means ALL FUNCTIONS in Hex-Rays; never pass it.
            try:
                batch_ok = ida_hexrays.decompile_many(
                    str(root / 'batches' / f'{pending[0]:x}.c'), pending, ida_hexrays.VDRUN_SILENT)
            except Exception as exc:
                batch_error = repr(exc)
        batch_seconds = time.monotonic() - batch_start
        for ea in pending:
            record = {'type': 'function', 'va': hex(ea), 'rva': hex(ea-base),
                      'name': ida_funcs.get_func_name(ea), 'status': 'failed',
                      'error': None, 'pseudocode_path': None, 'xrefs': [], 'calls': [],
                      'strings': [], 'metadata': {'profile': profile_hash}}
            try:
                chunks = list(idautils.Chunks(ea))
                byte_parts = []
                for start, end in chunks:
                    data = ida_bytes.get_bytes(start, end-start)
                    offset = start-load_address
                    if data is None or offset < 0 or data != source[offset:offset+end-start]:
                        raise ValueError(f'IDA bytes do not match snapshot at {start:x}')
                    byte_parts.append(data)
                record['bytes_sha256'] = hashlib.sha256(b''.join(byte_parts)).hexdigest()
                record['metadata']['chunks'] = [[hex(a), hex(b)] for a,b in chunks]
                record['metadata']['snapshot_bytes_verified'] = True
                refs = set()
                string_addresses = set()
                for address in idautils.FuncItems(ea):
                    for x in idautils.XrefsFrom(address, 0):
                        if x.type == ida_xref.fl_F:
                            continue
                        refs.add((x.frm, x.to, x.type, bool(x.iscode)))
                        if x.type in (ida_xref.fl_CN, ida_xref.fl_CF):
                            record['calls'].append({'from': hex(x.frm), 'to': hex(x.to), 'type': 'call'})
                        if not x.iscode:
                            string_addresses.add(x.to)
                for x in idautils.XrefsTo(ea, 0):
                    if x.type != ida_xref.fl_F:
                        refs.add((x.frm, x.to, x.type, bool(x.iscode)))
                record['xrefs'] = [{'from':hex(a),'to':hex(b),'type':str(t),'iscode':c}
                                   for a,b,t,c in sorted(refs)]
                for address in sorted(string_addresses):
                    string_type = idc.get_str_type(address)
                    if string_type is not None and string_type >= 0:
                        raw = ida_bytes.get_strlit_contents(address, -1, string_type)
                        if raw:
                            record['strings'].append({'address':hex(address),'value':raw.decode('utf-8', errors='replace')})
                failure = ida_hexrays.hexrays_failure_t()
                cfunc = ida_hexrays.decompile(ea, failure)
                if cfunc is None:
                    raise RuntimeError(str(failure))
                text = str(cfunc)
                path = root / 'pseudocode' / f'{ea:x}.c'
                atomic(path, text)
                record.update(status='success', pseudocode_path=str(path.resolve()),
                              pseudocode_sha256=digest(path))
            except Exception as exc:
                record['error'] = f'{type(exc).__name__}: {exc}'
            atomic(root / 'records' / f'{ea:x}.json', json.dumps(record, ensure_ascii=False))
        header = {'type': 'artifact', **artifact, 'total_functions': len(all_functions),
                  'metadata': {'profile': profile_hash, 'scope':'one runtime code section; IDA-defined functions',
                               'indirect_calls':'unresolved', 'strings':'referenced IDA-defined strings only; data sections absent'}}
        records = [json.loads((root/'records'/f'{ea:x}.json').read_text(encoding='utf-8'))
                   for ea in selected if (root/'records'/f'{ea:x}.json').exists()]
        atomic(root/'functions.jsonl', '\n'.join(json.dumps(x, ensure_ascii=False) for x in [header]+records)+'\n')
        result = {'selected':len(selected),'complete':len(records),
                  'success':sum(x['status']=='success' for x in records),
                  'failed':sum(x['status']=='failed' for x in records), 'new':len(pending),
                  'batch_ok':batch_ok,'batch_error':batch_error,'batch_seconds':batch_seconds,
                  'elapsed_seconds':time.monotonic()-started,'ai_tokens':None,
                  'ai_deeply_analyzed_functions':0}
        with (root/'runs.jsonl').open('a', encoding='utf-8') as stream:
            stream.write(json.dumps(result)+'\n')
        atomic(root/'summary.json', json.dumps(result, indent=2))
        return result
    finally:
        lock.unlink()
