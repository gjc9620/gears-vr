"""Read matching x64 exception-table ranges for captured return addresses."""
import argparse
import json
import pathlib
import struct
import sys
sys.path.insert(0, str(pathlib.Path(__file__).parent / 'python-libs'))
import pefile
from resolve_camera import Resolver


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid', type=int, required=True)
    parser.add_argument('--return-rva', action='append', required=True)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    args = parser.parse_args()
    profile = json.loads(pathlib.Path(__file__).with_name('reloaded-profile.json').read_text())
    resolver = Resolver(args.pid, profile)
    try:
        resolver.resolve()  # Existing executable hash and live object gates.
        pe = pefile.PE(resolver.path)
        try:
            directory = pe.OPTIONAL_HEADER.DATA_DIRECTORY[3]
            size, rva = directory.Size, directory.VirtualAddress
            if not size or size > 8 * 1024 * 1024 or size % 12 or rva + size > pe.OPTIONAL_HEADER.SizeOfImage:
                raise RuntimeError('Exception directory outside bounded PE range')
            data = resolver.read(resolver.base + rva, size)
            ranges = list(struct.iter_unpack('<III', data))
            targets = []
            for value in args.return_rva:
                address = int(value, 0)
                # Return PC points past the call; use the last call byte for containment.
                matches = [dict(begin=hex(lo), end=hex(hi), unwind=hex(unwind))
                           for lo, hi, unwind in ranges if lo <= address - 1 < hi]
                targets.append(dict(return_rva=hex(address), ranges=matches))
            result = dict(pid=args.pid, directory_rva=hex(rva), size=size, entries=len(ranges),
                          targets=targets, limits='Runtime exception ranges may describe funclets. Not a function signature or semantic identification.')
            args.output.write_text(json.dumps(result, indent=2) + '\n')
            print(json.dumps(result))
        finally:
            pe.close()
    finally:
        resolver.p.close()


if __name__ == '__main__':
    main()
