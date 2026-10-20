"""Read-only vtable candidate scan; field-offset hits are leads, not identities."""
import argparse
import json
import pathlib
import struct
import sys
sys.path.insert(0, str(pathlib.Path(__file__).parent / 'python-libs'))
import capstone as cs
from capstone.x86 import X86_OP_IMM, X86_OP_MEM
import pefile
from resolve_camera import Resolver


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--pid', type=int, required=True)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    parser.add_argument('--slots', type=int, default=256, choices=(256, 512, 1024))
    parser.add_argument('--entry-rva', action='append', default=[], help='Also decode an explicitly selected ordinary-code RVA')
    parser.add_argument('--max-instructions', type=int, default=1600, choices=range(100,10001), help='Bound per entry, including explicitly selected entries')
    parser.add_argument('--skip-vtables', action='store_true', help='Decode only explicitly selected entries')
    args = parser.parse_args()
    if args.skip_vtables and not args.entry_rva: parser.error('Explicit entries are required when skipping vtables')
    profile = json.loads(pathlib.Path(__file__).with_name('reloaded-profile.json').read_text())
    resolver = Resolver(args.pid, profile)
    try:
        camera = resolver.resolve()
        pe = pefile.PE(resolver.path)
        ranges = [(resolver.base + s.VirtualAddress, resolver.base + s.VirtualAddress + s.Misc_VirtualSize)
                  for s in pe.sections if s.Characteristics & 0x20000000
                  and s.Name.rstrip(b'\0') not in (b'.themida', b'.boot')]
        def code(address):
            return any(lo <= address < hi for lo, hi in ranges)
        pages = {}
        def read_code(address):
            page = address & ~4095
            if page not in pages:
                pages[page] = resolver.p.read(page, 8192)
            return pages[page][address - page:address - page + 16]
        decoder = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_64)
        decoder.detail = True
        offsets = {int(v, 16) for k, v in camera['offsets'].items()
                   if k in ('CameraCache', 'PlayerCamera', 'Player', 'PCOwner')}
        cache_pov = int(camera['offsets']['CameraCache'], 16) + int(camera['offsets']['POV'], 16)
        offsets.update(cache_pov + int(camera['offsets'][key], 16) for key in ('Location', 'Rotation', 'FOV'))
        graphs = {}
        def graph(entry, limit=None):
            if limit is None: limit=args.max_instructions
            if entry in graphs:
                return graphs[entry]
            seen = {}
            pending = [entry]
            hits = []
            calls = []
            blocked = []
            while pending and len(seen) < limit:
                address = pending.pop()
                while address not in seen and len(seen) < limit:
                    if not code(address):
                        blocked.append(hex(address)); break
                    insn = next(decoder.disasm(read_code(address), address, count=1), None)
                    if insn is None:
                        blocked.append(hex(address)); break
                    record = dict(rva=hex(address-resolver.base), mnemonic=insn.mnemonic, operands=insn.op_str)
                    seen[address] = record
                    if any(op.type == X86_OP_MEM and op.mem.disp in offsets for op in insn.operands):
                        hits.append(record)
                    if insn.group(cs.CS_GRP_CALL):
                        target = insn.operands[0].imm if insn.operands[0].type == X86_OP_IMM else None
                        calls.append(dict(rva=record['rva'], target_rva=hex(target-resolver.base) if target else None,
                                          operands=insn.op_str))
                    if insn.group(cs.CS_GRP_RET) or insn.mnemonic in ('int3', 'ud2'):
                        break
                    if insn.group(cs.CS_GRP_JUMP):
                        op = insn.operands[0]
                        if op.type == X86_OP_IMM and code(op.imm):
                            pending.append(op.imm)
                        elif insn.mnemonic == 'jmp':
                            blocked.append(insn.op_str)
                        if insn.mnemonic == 'jmp':
                            break
                    address += insn.size
            result = dict(entry_rva=hex(entry-resolver.base), instruction_count=len(seen),
                          limit_reached=bool(pending) or len(seen)>=limit, hits=hits, calls=calls,
                          blocked=blocked, instructions=[seen[a] for a in sorted(seen)])
            graphs[entry] = result
            return result
        tables = []
        candidates = []
        slot_records = []
        # Bounded window; adjacent vtables may occur before the window ends.
        # Slot membership beyond the actual class table is deliberately unproven.
        for kind, key in ([] if args.skip_vtables else [('camera', 'camera_address'), ('controller', 'controller_address'), ('local_player', 'local_player_address')]):
            obj = int(camera[key], 16)
            table = resolver.u64(obj)
            slots = struct.unpack('<'+'Q'*args.slots, resolver.read(table, args.slots*8))
            tables.append(dict(kind=kind, object_address=hex(obj), table_rva=hex(table-resolver.base), scanned_slots=args.slots))
            for slot, entry in enumerate(slots):
                if not code(entry):
                    continue
                result = graph(entry)
                slot_records.append(dict(kind=kind, slot_offset=hex(slot*8), entry_rva=result['entry_rva'],
                                         instruction_count=result['instruction_count'], limit_reached=result['limit_reached']))
                if result['hits']:
                    candidates.append(dict(kind=kind, slot_offset=hex(slot*8), **result))
        explicit = []
        for value in args.entry_rva:
            entry = resolver.base + int(value, 0)
            if not code(entry):
                raise RuntimeError('Explicit entry outside ordinary executable sections')
            explicit.append(graph(entry))
        result = dict(pid=args.pid, camera=camera, max_instructions=args.max_instructions, skipped_vtables=args.skip_vtables, tables=tables, slots=slot_records, explicit_entries=explicit, scanned_unique_entries=len(graphs),
                      entry_summaries=[{k:v for k,v in g.items() if k not in ('instructions','calls','hits')} for g in graphs.values()],
                      searched_displacements=[hex(v) for v in sorted(offsets)], candidates=candidates,
                      caveat='Offset matches and fixed vtable windows are heuristic leads; no runtime invocation or register provenance proved.')
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(result, indent=2), encoding='utf-8')
        print('Unique entries:', len(graphs), 'candidate slots:', len(candidates))
        for item in explicit:
            print('Explicit entry',item['entry_rva'],'instructions',item['instruction_count'],'limit',item['limit_reached'])
    finally:
        resolver.p.close()


if __name__ == '__main__':
    main()
