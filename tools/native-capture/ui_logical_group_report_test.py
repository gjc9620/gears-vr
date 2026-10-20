import struct
import unittest
from ui_logical_group_report import decode


def fixture():
    transform = (struct.pack("<16f", 2, 0, 0, 0, 0, -2, 0, 0, 0, 0, 1, 0, -1, 1, 0, 1) + bytes(16)).hex()
    lines = [
        "ui_logical_group_row kind=1 row=0 token=123 fcanvas=0000000000001000 before_ok=1 returned=1 root_current=1 delta_ok=1 bounds_ok=1 calls=1 call_overflow=0 simple_executes=0 material_executes=0 simple_ranges=1 materials=0 left=.2 top=.1 right=.8 bottom=.3",
        "ui_logical_group_call kind=1 row=0 call=0 ancestry_complete=1 frames=2 append_complete=1 members=1 mapped_to_root_delta=1",
        "ui_logical_group_frame kind=1 row=0 call=0 depth=0 node=0000000000000500",
        "ui_logical_group_frame kind=1 row=0 call=0 depth=1 node=0000000000001000",
        "ui_logical_group_member kind=1 row=0 call=0 member=0 material=0 fcanvas=0000000000001000 key=7 item=0000000000004000 payload=0000000000005000 begin=0 end=4",
        "ui_logical_group_simple kind=1 row=0 range=0 key=7 item=0000000000004000 payload=0000000000005000 begin=0 end=4 vertices=4 transform80=" + transform,
    ]
    names = {k: v.encode("utf-16-be").hex() for k, v in {"name": "DrawActionInfo", "class": "Function", "outer": "WarHUD"}.items()}
    lines.append("ui_logical_group_node kind=1 row=0 index=0 node=0000000000001000 object_index=5 name_index=7 number=0 " +
                 " ".join(f"{k}_utf16={v}" for k, v in names.items()))
    for i, (x, y) in enumerate(((.2, .1), (.8, .1), (.2, .3), (.8, .3))):
        v = (struct.pack("<4f", x, y, 0, 1) + bytes(28)).hex()
        lines.append(f"ui_logical_group_vertex kind=1 row=0 range=0 vertex={i} bytes44={v}")
    return ("\n".join(lines) + "\n").encode()


class ReportTest(unittest.TestCase):
    def test_bounds_and_hex_pointer(self):
        row = decode(fixture(), 0x1000)["rows"][0]
        self.assertTrue(row["copied_group_candidate"])
        self.assertEqual(row["selected_calls"], [0])
        self.assertAlmostEqual(row["bounds"]["top"], .1, places=6)
        self.assertAlmostEqual(row["bounds"]["bottom"], .3, places=6)
        self.assertFalse(row["replay_authorized"])
        self.assertEqual(row["observed_functions"][0]["name"], "DrawActionInfo")
        self.assertEqual(row["observed_functions"][0]["class"], "Function")
        self.assertFalse(decode(fixture(), 1000)["rows"][0]["copied_group_candidate"])

    def test_missing_geometry_never_partial(self):
        data = b"\n".join(fixture().splitlines()[:-1]) + b"\n"
        row = decode(data, 0x1000)["rows"][0]
        self.assertFalse(row["copied_group_candidate"])
        self.assertIsNone(row["bounds"])

    def test_incomplete_proofs(self):
        for old, new in ((b"delta_ok=1", b"delta_ok=0"), (b"ancestry_complete=1", b"ancestry_complete=0"),
                         (b"append_complete=1", b"append_complete=0"), (b"call_overflow=0", b"call_overflow=1"),
                         (b"simple_executes=0", b"simple_executes=1"), (b"mapped_to_root_delta=1", b"mapped_to_root_delta=0")):
            with self.subTest(old=old):
                self.assertFalse(decode(fixture().replace(old, new), 0x1000)["rows"][0]["copied_group_candidate"])
        self.assertFalse(decode(fixture().replace(b"call_overflow=0", b"call_overflow=0 unclassified_canvas=1"), 0x1000)["rows"][0]["copied_group_candidate"])

    def test_bad_member_interval(self):
        data = fixture().replace(b"member=0 material=0", b"member=0 material=0").replace(b"begin=0 end=4\n", b"begin=0 end=8\n")
        self.assertFalse(decode(data, 0x1000)["rows"][0]["copied_group_candidate"])

    def test_duplicate_detail_rejected(self):
        data = fixture()
        data += next(line for line in data.splitlines() if b"depth=0" in line) + b"\n"
        with self.assertRaisesRegex(ValueError, "duplicate"):
            decode(data, 0x1000)

    def test_unknown_root_never_inferred(self):
        report = decode(b"unrelated_locale_text=\xff\x81\n" + fixture())
        self.assertEqual(len(report["source_sha256"]), 64)
        self.assertIsNone(report["rows"][0]["ancestor"])
        self.assertNotIn("copied_group_candidate", report["rows"][0])

    def test_unique_invocation_requires_live_boundary_and_body(self):
        data = fixture().replace(b"depth=1 node=0000000000001000", b"depth=1 node=0000000000001000 object=0000000000004444 previous=0000000000007000 code=0000000000090001")
        data += b"ui_logical_group_child kind=1 row=0 index=0 id=7 parent=0 self=0000000000004444 callee=0000000000001000 caller=0000000000007000 script=0000000000090000 script_size=8 enter=10 exit=20 returned=1 current_after=1 body_calls=1 name_utf16=0044007200610077\n"
        data += b"ui_logical_group_call_child kind=1 row=0 call=0 depth=0 id=7\n"
        row = decode(data, invocation=7)["rows"][0]
        self.assertTrue(row["unique_invocation_proven"])
        self.assertEqual(row["selected_calls"], [0])
        self.assertFalse(decode(data, invocation=8)["rows"][0]["unique_invocation_proven"])
        for old, new in ((b"current_after=1", b"current_after=0"), (b"body_calls=1", b"body_calls=2"),
                         (b"script_size=8", b"script_size=1")):
            self.assertFalse(decode(data.replace(old, new), invocation=7)["rows"][0]["unique_invocation_proven"])


if __name__ == "__main__":
    unittest.main()
