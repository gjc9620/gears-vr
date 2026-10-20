"""Decompress analysis copies of the observed Reloaded 835/76 packages."""
import pathlib
import struct
import sys

sys.path.insert(0, str(pathlib.Path(__file__).parent / "python-libs"))
import lz4.block

source, target = map(pathlib.Path, sys.argv[1:3])
data = source.read_bytes()
assert struct.unpack_from("<IHH", data) == (0x9E2A83C1, 835, 76)
# This layout was verified against UELib's header and both installed packages.
folder_size = struct.unpack_from("<i", data, 12)[0]
assert folder_size == 5
flags_pos = 93
flags, count = struct.unpack_from("<II", data, flags_pos)
assert flags == 32 and 0 < count < 10000
chunks = [struct.unpack_from("<IIII", data, flags_pos + 8 + i * 16) for i in range(count)]
out = bytearray(max(uo + us for uo, us, co, cs in chunks))
out[:chunks[0][0]] = data[:chunks[0][0]]
for uo, us, co, cs in chunks:
    tag, block_size, packed, unpacked = struct.unpack_from("<IIII", data, co)
    assert tag == 0x9E2A83C1 and unpacked == us
    num_blocks = (us + block_size - 1) // block_size
    blocks = [struct.unpack_from("<II", data, co + 16 + i * 8) for i in range(num_blocks)]
    cursor = co + 16 + num_blocks * 8
    decoded = bytearray()
    for csize, usize in blocks:
        block = lz4.block.decompress(data[cursor:cursor+csize], uncompressed_size=usize)
        assert len(block) == usize
        decoded.extend(block)
        cursor += csize
    assert len(decoded) == us and cursor == co + cs
    out[uo:uo+us] = decoded
# Rebuild the uncompressed 105-byte summary, retaining original virtual offsets.
# Never write this analysis copy into the game directory.
struct.pack_into("<I", out, flags_pos, 0)
struct.pack_into("<I", out, flags_pos + 4, 0)
out[flags_pos + 8:flags_pos + 12] = data[flags_pos + 8 + count * 16:flags_pos + 12 + count * 16]
target.parent.mkdir(parents=True, exist_ok=True)
assert source.resolve() != target.resolve()
target.write_bytes(out)
print(f"{source.name}: {count} chunks, {len(data)} -> {len(out)} bytes; all block sizes verified")
