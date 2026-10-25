"""Pure diagnostics for saved passive cover-call receipts; never reads a process.

Recorded pointers and native acceptance labels are evidence, not permissions or
authentication. Parameter references are checked against saved script bytes;
raw CPF flags and Result-buffer/return-register semantics remain uninterpreted.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

import cover_call_wire as wire

LIMITS = ('Saved passive receipts only; no live identity, atomic cohort, native out-parameter ABI, '
          'body motion, collision, aim, GPU or headset behavior is proven. Recorded script windows '
          'do not cover every parameter byte; the complete parameter body comes from the saved '
          'source-checked script. No pointer is dereferenced and no Result buffer is read.')
FLAGS = dict(activation_allowed=False, mutation_authorized=False, return_buffer_semantics_unproven=True)
MAX_ROWS, MAX_TEXT = 512, 16 * 1024 * 1024
ARGUMENTS = ('DeltaTime', 'PawnCT', 'PawnCA', 'PawnCD', 'bNewLeftSided')
SHAPES = {'DeltaTime': ('FloatProperty', 4), 'PawnCT': ('ByteProperty', 1),
          'PawnCA': ('ByteProperty', 1), 'PawnCD': ('ByteProperty', 1), 'bNewLeftSided': ('IntProperty', 4)}
VALUES = ('ct', 'ca', 'cd')
CONTEXT = ('invocation', 'arm', 'world', 'thread', 'caller', 'locals_diagnostic', 'caller_role', 'parent')
DECIMAL = ('index', 'invocation', 'parent', 'world', 'arm', 'role', 'caller_role', 'thread', 'begin', 'end',
           'returned', 'same_storage', 'code_before', 'code_after', 'mutation_authorized',
           'ct_before', 'ca_before', 'cd_before', 'ct_after', 'ca_after', 'cd_after')
POINTERS = ('receiver', 'caller', 'callee', 'result_address', 'opaque_rax', 'locals_diagnostic')


class DecodeRejected(ValueError):
    pass


def _require(condition, reason):
    if not condition:
        raise DecodeRejected(reason)


def _uint(value, label, maximum=(1 << 64) - 1):
    _require(type(value) is int and 0 <= value <= maximum, 'Invalid unsigned integer: ' + label)
    return value


def _decimal(value, label):
    _require(isinstance(value, str) and re.fullmatch(r'[0-9]{1,20}', value), 'Invalid decimal: ' + label)
    return _uint(int(value), label)


def _pointer(value, label):
    _require(isinstance(value, str) and re.fullmatch(r'(?:0[xX])?[0-9a-fA-F]{1,16}', value),
             'Invalid diagnostic pointer: ' + label)
    return _uint(int(value, 16), label)


def _hex(value, maximum, label):
    _require(isinstance(value, str) and len(value) % 2 == 0 and len(value) <= maximum * 2
             and re.fullmatch(r'[0-9a-fA-F]*', value), 'Invalid bounded hex: ' + label)
    return bytes.fromhex(value)


def _config(supplied):
    _require(type(supplied) is dict, 'Config must be an object')
    cfg = supplied.get('config', supplied)
    wire.encode(cfg)  # Existing pure shape/source/script validator; no I/O or authority.
    digest = hashlib.sha256(json.dumps(cfg, sort_keys=True, separators=(',', ':'), allow_nan=False).encode()).hexdigest()
    if 'config' in supplied:
        provenance = supplied.get('provenance', {})
        _require(type(provenance) is dict and provenance.get('config_sha256') == digest,
                 'Wrapped config provenance hash differs')
    sources = {(item['address'], item['size']): bytes.fromhex(item['bytes']) for item in cfg['source_checks']}
    snapshots = cfg['registry_snapshots']
    closing = cfg.get('registry_closing_snapshots', snapshots)
    for name in ('objects', 'names'):
        initial = _hex(snapshots[name], 16, 'initial registry header')
        final = _hex(closing[name], 16, 'closing registry header')
        _require(len(initial) == len(final) == 16 and sources.get((cfg[name + '_header'], 16)) == initial,
                 'Registry snapshot/source binding differs')
        data, count, capacity = struct.unpack('<Qii', initial)
        end_data, end_count, end_capacity = struct.unpack('<Qii', final)
        _require(data and data % 8 == 0 and 1000 < count <= end_count <= capacity < 4000000
                 and data <= (1 << 64) - 1 - capacity * 8 and data == end_data and capacity == end_capacity,
                 'Invalid/replaced/shrinking registry cohort')
    callers = {}
    for role in (0, 2):
        function = cfg['functions'][role]
        fields = {item['name']: item for item in function['fields']}
        spans, addresses = [], {}
        for name, (kind, width) in SHAPES.items():
            field = fields.get(name)
            _require(field is not None and field['type'] == kind and field['width'] == width
                     and field['dimension'] == 1 and field['storage_width'] == width,
                     'PlayerMove argument shape differs: ' + name)
            offset = field['offset']
            _require(offset + width <= min(128, function['locals_size']), 'Caller argument exceeds captured prefix: ' + name)
            _require(all(offset + width <= a or offset >= b for a, b in spans), 'Caller argument storage overlaps')
            spans.append((offset, offset + width))
            address = cfg['identities'][field['identity']]['address']
            for where, raw in ((address + 0x70, struct.pack('<II', 1, width)),
                               (address + 0x78, bytes.fromhex(field['flags_raw'])),
                               (address + 0x94, struct.pack('<I', offset))):
                _require(sources.get((where, len(raw))) == raw, 'Caller argument source binding differs: ' + name)
            addresses[name] = address
        callers[role] = dict(function=function, fields=fields, addresses=addresses,
                             script=bytes.fromhex(function['script']['bytes']))
    controller = cfg['identities'][cfg['roles']['controller']]['address']
    return cfg, digest, controller, callers


def _tokens(line):
    parts = line.split()
    _require(len(parts) <= 64, 'Receipt token bound exceeded')
    result = {}
    for part in parts[1:]:
        _require('=' in part, 'Malformed receipt token')
        name, value = part.split('=', 1)
        _require(name and name not in result, 'Duplicate receipt key: ' + name)
        result[name] = value
    return result


def _parse(status):
    _require(type(status) is str and len(status) <= MAX_TEXT, 'Status text exceeds bound or has wrong type')
    rows, endpoints, summary = [], {}, None
    seen = set()
    for line in status.splitlines():
        kind = line.split(' ', 1)[0]
        if kind not in ('cover_call_row', 'cover_call_bytes', 'cover_calls', 'cover_call_config'):
            continue
        _require(len(line) <= 4096, 'Receipt line bound exceeded')
        item = _tokens(line)
        if kind == 'cover_calls':
            _require(summary is None, 'Duplicate cover_calls summary')
            _require(item.get('compiled') == item.get('enabled') == item.get('observation_only') == '1'
                     and item.get('mutation_authorized') == '0', 'Observer summary is not passive/enabled')
            summary = item
        elif kind == 'cover_call_config':
            _require(item.get('accepted') == item.get('observation_only') == '1'
                     and item.get('mutation_authorized') == '0', 'Observer config was not accepted/passive')
        elif kind == 'cover_call_row':
            index = _decimal(item.get('index'), 'row index')
            _require(index not in seen and len(rows) < MAX_ROWS, 'Duplicate row index or row bound exceeded')
            seen.add(index)
            rows.append(item)
        else:
            index = _decimal(item.get('index'), 'bytes index')
            endpoint = item.get('endpoint')
            _require(endpoint in ('entry', 'return') and (index, endpoint) not in endpoints
                     and len(endpoints) < MAX_ROWS * 2, 'Duplicate/invalid endpoint or endpoint bound exceeded')
            endpoints[index, endpoint] = item
    _require([_decimal(row['index'], 'index') for row in rows] == list(range(len(rows))),
             'Row indices are not one contiguous ordered capture')
    _require(set(endpoints) == {(i, endpoint) for i in seen for endpoint in ('entry', 'return')},
             'Missing/orphan endpoint receipts')
    return rows, endpoints, summary


def _row(item, endpoints, cfg, controller, callers):
    row = {key: _decimal(item.get(key), key) for key in DECIMAL}
    row.update({key: _pointer(item.get(key), key) for key in POINTERS})
    _require(item.get('before') == item.get('after') == 'accepted' and row['returned'] == row['same_storage'] == 1
             and row['mutation_authorized'] == 0, 'Row endpoints were not accepted/returned/same-storage/passive')
    _require(row['invocation'] > row['parent'] and row['arm'] > 0 and 0 < row['thread'] <= 0xffffffff
             and 0 < row['begin'] <= row['end'] <= (1 << 63) - 1, 'Invalid invocation/arm/thread/timing')
    _require(row['receiver'] == controller, 'Row receiver differs from configured controller')
    role, caller_role = row['role'], row['caller_role']
    _require(caller_role in callers and role in (caller_role + 1, 4, 5), 'Wrong updater/caller role')
    target = cfg['functions'][role]
    _require(row['callee'] == cfg['identities'][target['identity']]['address'], 'Row callee is not the exact configured function')
    caller = callers[caller_role]
    size = caller['function']['locals_size']
    _require(0 < row['caller'] <= (1 << 64) - 1 - 0x40 and row['caller'] % 8 == 0
             and 0 < row['locals_diagnostic'] <= (1 << 64) - 1 - size and row['locals_diagnostic'] % 8 == 0,
             'Caller/locals diagnostic pointer range or alignment differs')
    raw_values, prefixes, windows = {}, {}, {}
    for endpoint, suffix in (('entry', 'before'), ('return', 'after')):
        code = row['code_' + suffix]
        _require(code < len(caller['script']), 'Caller code is outside saved script')
        body = endpoints[row['index'], endpoint]
        offset = _decimal(body.get('code_window_offset'), 'window offset')
        raw = _hex(body.get('script'), 32, 'script window')
        window_size = min(32, len(caller['script']))
        expected_offset = min(max(0, code - 16), len(caller['script']) - window_size)
        _require(len(raw) == window_size and offset == expected_offset
                 and raw == caller['script'][offset:offset + window_size], 'Recorded script window differs from saved bytes/range')
        prefix = _hex(body.get('locals_prefix'), 128, 'locals prefix')
        _require(len(prefix) == min(128, size), 'Recorded locals prefix size differs')
        values = {short: prefix[caller['fields'][name]['offset']] for short, name in
                  zip(VALUES, ('PawnCT', 'PawnCA', 'PawnCD'))}
        _require(all(row[name + '_' + suffix] == values[name] for name in VALUES), 'Raw prefix disagrees with row values')
        raw_values[endpoint], prefixes[endpoint] = values, prefix.hex()
        windows[endpoint] = dict(offset=offset, bytes=raw.hex())
    arguments = ARGUMENTS if role in (1, 3) else ('PawnCA',) if role == 4 else ('PawnCD',)
    expected = b''.join(b'\x00' + struct.pack('<Q', caller['addresses'][name]) for name in arguments) + b'\x16'
    before, after = row['code_before'], row['code_after']
    _require(after - before == len(expected) and caller['script'][before:after] == expected,
             'Call parameter body is not the exact reflected EX_LocalVariable sequence and EndParm')
    return dict(row, function=target['key'], values=raw_values, locals_prefix=prefixes, script_windows=windows,
                argument_names=list(arguments), argument_properties={name: hex(caller['addresses'][name]) for name in arguments},
                **FLAGS)


def analyze(status_text, config):
    """Return JSON-safe diagnostics. Malformed or incomplete evidence never enables activation."""
    result = dict(schema=1, kind='cover_call_diagnostic', accepted=False, groups=[], errors=[],
                  input_rows=0, input_endpoints=0, accepted_groups=0, rejected_groups=0,
                  out_storage_change_groups=0, continuity_only_groups=0, rejected_rows=0,
                  result_buffer_read=False, native_out_parameter_abi_proven=False,
                  raw_property_flags_interpreted=False, body_motion_proven=False, gpu_behavior_proven=False,
                  limits=LIMITS, **FLAGS)
    try:
        cfg, digest, controller, callers = _config(config)
        result.update(config_sha256=digest, config_pid=cfg['pid'], controller=hex(controller))
        rows, endpoints, summary = _parse(status_text)
        result.update(status_sha256=hashlib.sha256(status_text.encode()).hexdigest(), observer_summary=summary,
                      input_rows=len(rows), input_endpoints=len(endpoints))
    except (ValueError, KeyError, TypeError, OverflowError, struct.error) as error:
        result['errors'].append(dict(stage='config_or_receipt_shape', reason=str(error), **FLAGS))
        return result
    validated, invalid, invocations = {}, {}, {}
    for raw in rows:
        index, invocation = int(raw['index']), raw.get('invocation', '<missing>')
        invocations.setdefault(invocation, []).append(index)
        try:
            validated[index] = _row(raw, endpoints, cfg, controller, callers)
        except (ValueError, KeyError, TypeError, OverflowError, struct.error) as error:
            invalid[index] = str(error)
    for invocation, indices in invocations.items():
        group = dict(invocation=int(invocation) if re.fullmatch(r'[0-9]{1,20}', invocation) else invocation,
                     row_indices=indices, accepted=False,
                     out_storage_change_observed=False, **FLAGS)
        try:
            _require(not any(index in invalid for index in indices),
                     'Rejected row(s): ' + '; '.join(str(index) + ': ' + invalid[index] for index in indices if index in invalid))
            selected = [validated[index] for index in indices]
            _require(len(selected) == 3, 'Expected exactly updater, action setter and direction setter in this invocation')
            first, action, direction = selected
            _require(all(tuple(row[key] for key in CONTEXT) == tuple(first[key] for key in CONTEXT) for row in selected),
                     'Invocation arm/world/thread/caller/locals/caller-role/parent context changed')
            _require([row['role'] for row in selected] == [first['caller_role'] + 1, 4, 5], 'Call order differs')
            _require(all(a['end'] <= b['begin'] and a['code_after'] <= b['code_before']
                         for a, b in zip(selected, selected[1:])), 'Call timing/code order overlaps or goes backwards')
            after = first['values']['return']
            _require(all(row['values'][endpoint] == after for row in (action, direction) for endpoint in ('entry', 'return')),
                     'Subsequent setters did not preserve updater CT/CA/CD in the same caller storage')
            changed = [key for key in ('ca', 'cd') if first['values']['entry'][key] != after[key]]
            group.update(accepted=True, context={key: first[key] for key in CONTEXT}, rows=selected,
                         caller_function=callers[first['caller_role']]['function']['key'], changed_fields=changed,
                         updater_before=first['values']['entry'], updater_after=after,
                         out_storage_change_observed=bool(changed), argument_local_references_verified=True,
                         conclusion='out_storage_change_observed' if changed else 'call_and_storage_continuity_only')
        except (ValueError, KeyError, TypeError) as error:
            group['reason'] = str(error)
        result['groups'].append(group)
    accepted = [group for group in result['groups'] if group['accepted']]
    result.update(accepted=bool(accepted), accepted_groups=len(accepted), rejected_groups=len(invocations) - len(accepted),
                  out_storage_change_groups=sum(group['out_storage_change_observed'] for group in accepted),
                  continuity_only_groups=sum(not group['out_storage_change_observed'] for group in accepted),
                  rejected_rows=len(invalid))
    if not rows:
        result['errors'].append(dict(stage='receipts', reason='No cover-call rows were recorded', **FLAGS))
    return result


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--status', type=Path, required=True)
    parser.add_argument('--config', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args(argv)
    if args.output.resolve() in (args.status.resolve(), args.config.resolve()):
        parser.error('Output must differ from both evidence inputs')
    result = analyze(args.status.read_text(encoding='utf-8'), json.loads(args.config.read_text(encoding='utf-8')))
    args.output.write_text(json.dumps(result, indent=2, allow_nan=False) + '\n', encoding='utf-8')
    return 0 if result['accepted'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
