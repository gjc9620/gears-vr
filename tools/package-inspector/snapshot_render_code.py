"""Read verified ordinary image sections for an isolated Ghidra data set.

No process writes or injection. The output is analysis data, not a runnable PE.
Section bytes are read twice to reject changes during collection. Mutable data
sections are deliberately excluded; heap objects and live resources are absent.
"""
import argparse
import datetime
import hashlib
import json
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).parent / 'python-libs'))
import pefile
from resolve_camera import Resolver

ROOT = pathlib.Path(__file__).resolve().parents[2]
ENTRIES = (0x15CF08, 0x3A148C, 0x116250, 0x15A7F4, 0x15A80C,
           0x11908C, 0x1FC060, 0x2B0A98, 0xD82F58, 0x1AF42C)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid', type=int, required=True)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to((ROOT / 'analysis/ghidra').resolve()):
        parser.error('Output must be a new directory under analysis/ghidra')
    output.mkdir(parents=True, exist_ok=False)
    profile = json.loads(pathlib.Path(__file__).with_name('reloaded-profile.json').read_text())
    resolver = Resolver(args.pid, profile)
    try:
        resolver.prepare()
        pe = pefile.PE(resolver.path)
        sections = []
        for section in pe.sections:
            name = section.Name.rstrip(b'\0').decode('ascii')
            # Restrict to ordinary code and read-only image data. No protector.
            if (name in ('.themida', '.boot') or section.VirtualAddress >= 0x200F000
                    or section.Characteristics & 0x80000000):
                continue
            size, rva = section.Misc_VirtualSize, section.VirtualAddress
            if size <= 0 or size > 64 * 1024 * 1024:
                raise RuntimeError('Unexpected section size')
            chunks = []
            for offset in range(0, size, 65536):
                count = min(65536, size - offset)
                address = resolver.base + rva + offset
                raw = resolver.read(address, count)
                if raw != resolver.read(address, count):
                    raise RuntimeError(f'Image bytes changed at {address:#x}')
                chunks.append(raw)
            data = b''.join(chunks)
            filename = f'section_{rva:x}.bin'
            (output / filename).write_bytes(data)
            sections.append(dict(name=name, file=filename, rva=hex(rva),
                                 address=hex(resolver.base+rva), size=size,
                                 executable=bool(section.Characteristics & 0x20000000),
                                 sha256=hashlib.sha256(data).hexdigest()))
        if not sections or not any(s['executable'] for s in sections):
            raise RuntimeError('No ordinary executable sections were captured')
        comparisons = []
        for rva in ENTRIES:
            live = resolver.read(resolver.base+rva, 32)
            disk = pe.get_data(rva, 32)
            comparisons.append(dict(rva=hex(rva), address=hex(resolver.base+rva),
                                    runtime_hex=live.hex(), disk_hex=disk.hex(),
                                    equal=live == disk))
        report = dict(pid=args.pid, executable=resolver.path,
                      executable_sha256=profile['sha256'], base=hex(resolver.base),
                      captured_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                      sections=sections, entries=comparisons,
                      caveat='Read-only snapshot; not a runnable executable or evidence of GPU execution.')
        (output / 'manifest.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
        (output / 'sections.tsv').write_text('\n'.join(
            f"{s['file']}\t{s['address']}\t{int(s['executable'])}" for s in sections), encoding='ascii')
        (output / 'entries.txt').write_text('\n'.join(e['address'] for e in comparisons), encoding='ascii')
        print(json.dumps(report, indent=2))
    finally:
        resolver.p.close()


if __name__ == '__main__':
    main()
