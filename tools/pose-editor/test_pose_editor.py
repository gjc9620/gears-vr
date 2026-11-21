import copy
import json
from pathlib import Path
import struct
import tempfile
import unittest
import pose_editor as p


class ProtocolTest(unittest.TestCase):
    def test_wire_matches_native_layout(self):
        profile = p.baseline()
        profile["palm_metres"] = [.012, -.003, .001]
        profile["finger_degrees"][4] = [0., 0., 25.]
        wire = p.encode(profile, 123, True)
        self.assertEqual(len(wire), 600)
        self.assertEqual(wire[:8], b"GVRPOSE2")
        self.assertEqual(struct.unpack_from("<QII", wire, 8), (123, 1, 768))
        self.assertEqual(wire[24:56].hex(), p.SIGNATURE)
        self.assertEqual(struct.unpack_from("<3d", wire, 56), (.012, -.003, .001))
        self.assertEqual(struct.unpack_from("<Q", wire, 592)[0], p.checksum(wire[:592]))

    def test_all_five_parts_and_display_flags(self):
        for index, part in enumerate(p.PARTS):
            wire = p.encode(p.baseline(part), 200+index, True, volumes=False, poses=True)
            self.assertEqual(struct.unpack_from("<I", wire, 20)[0], index | 512)
            self.assertEqual(p.baseline(part)["side"], "right" if index==0 else "left")

    def test_invalid_profiles(self):
        mutations = [("geometry", "0"*64), ("side", "left"), ("space", "world"),
                     ("palm_metres", [.04, .04, 0]), ("palm_metres", [float("nan"), 0, 0]),
                     ("palm_degrees", [45, 45, 0]), ("finger_degrees", [[0, 0, 0]]*14)]
        for key, value in mutations:
            with self.subTest(key=key, value=value):
                profile = p.baseline()
                profile[key] = value
                with self.assertRaises(ValueError):
                    p.encode(profile, 1, True)

    def test_baseline_and_save_roundtrip(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/"pose.json"
            profile = p.baseline()
            profile["finger_degrees"][12] = [5, -10, 12]
            p.save_profile(path, profile)
            self.assertEqual(json.loads(path.read_text("utf-8")), profile)
            binary = Path(directory)/"preview.bin"
            p.atomic_write(binary, p.encode(profile, 10, True))
            p.atomic_write(binary, p.encode(p.baseline(), 11, False))
            self.assertEqual(struct.unpack_from("<QI", binary.read_bytes(), 8), (11, 0))
            self.assertEqual(sorted(x.name for x in Path(directory).iterdir()), ["pose.json", "preview.bin"])


if __name__ == "__main__":
    unittest.main()
