"""Decode bounded read-only logical-group evidence, never authorize replay.

--ancestor is an actually resolved runtime UFunction address; there is no
name/address heuristic. A result is copied buffered geometry, not GPU proof.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct

PREFIX = "ui_logical_group_"


def pointer(value: str) -> int:
    # Native printf %p is hexadecimal, even if it contains only decimal digits.
    return int(value, 16)


def integer(fields: dict, key: str) -> int:
    return int(fields[key])


def project(transform: str, x: float, y: float) -> tuple[float, float]:
    raw = bytes.fromhex(transform)
    if len(raw) != 80:
        raise ValueError("Invalid copied transform length")
    m = struct.unpack("<16f", raw[:64])
    if not all(map(math.isfinite, (*m, x, y))):
        raise ValueError("Nonfinite geometry")
    w = x * m[3] + y * m[7] + m[15]
    if not math.isfinite(w) or w <= 0:
        raise ValueError("Unsupported projected W")
    p = (((x * m[0] + y * m[4] + m[12]) / w + 1) / 2,
         (1 - (x * m[1] + y * m[5] + m[13]) / w) / 2)
    if not all(map(math.isfinite, p)):
        raise ValueError("Nonfinite projected geometry")
    return p


def decode(data: bytes, ancestor: int | None = None, invocation: int | None = None) -> dict:
    if len(data) > 64 * 1024 * 1024:
        raise ValueError("Status exceeds bounded decoder input")
    rows = {}
    for raw_line in data.splitlines():
        if not raw_line.startswith(PREFIX.encode("ascii")):
            continue
        line = raw_line.decode("ascii", errors="strict")
        tokens = line.split()
        tag = tokens[0][len(PREFIX):]
        if tag not in {"row", "call", "frame", "member", "simple", "vertex", "material", "node", "child", "children", "call_child"}:
            continue
        fields = dict(token.split("=", 1) for token in tokens[1:])
        key = (integer(fields, "kind"), integer(fields, "row"))
        if key[0] not in (1, 2) or not 0 <= key[1] < 8:
            raise ValueError("Unbounded row identity")
        row = rows.setdefault(key, {"header": None, "calls": {}, "simple": {}, "material": {}, "nodes": {}, "children": {}, "child_overflow": 0})
        if tag == "row":
            if row["header"] is not None:
                raise ValueError("Duplicate root row")
            row["header"] = fields
        elif tag == "children":
            row["child_overflow"] = integer(fields, "overflow")
        elif tag == "child":
            index = integer(fields, "id")
            if index in row["children"] or len(row["children"]) >= 128:
                raise ValueError("Duplicate or excessive child invocation")
            row["children"][index] = fields
        elif tag == "node":
            index = integer(fields, "index")
            if not 0 <= index < 32 or index in row["nodes"]:
                raise ValueError("Invalid or duplicate node metadata")
            row["nodes"][index] = fields
        elif tag in {"call", "frame", "member", "call_child"}:
            index = integer(fields, "call")
            if not 0 <= index < 64:
                raise ValueError("Unbounded call index")
            call = row["calls"].setdefault(index, {"header": None, "frames": {}, "members": {}, "logical_ids": {}})
            if tag == "call":
                if call["header"] is not None:
                    raise ValueError("Duplicate call row")
                call["header"] = fields
            else:
                bucket = {"frame": "frames", "member": "members", "call_child": "logical_ids"}[tag]
                index = integer(fields, "member" if tag == "member" else "depth")
                if not 0 <= index < (32 if tag == "member" else 12) or index in call[bucket]:
                    raise ValueError("Invalid or duplicate call detail")
                call[bucket][index] = fields
        else:
            index = integer(fields, "range")
            if not 0 <= index < 128:
                raise ValueError("Unbounded geometry range")
            if tag == "material":
                if index in row["material"]:
                    raise ValueError("Duplicate material")
                row["material"][index] = fields
            else:
                part = row["simple"].setdefault(index, {"header": None, "vertices": {}})
                if tag == "simple":
                    if part["header"] is not None:
                        raise ValueError("Duplicate simple range")
                    part["header"] = fields
                else:
                    v = integer(fields, "vertex")
                    if not 0 <= v < 1024 or v in part["vertices"]:
                        raise ValueError("Invalid or duplicate vertex")
                    part["vertices"][v] = fields["bytes44"]
    results = []
    for (kind, index), row in sorted(rows.items()):
        h = row["header"]
        if h is None:
            raise ValueError("Missing root header")
        result = {"kind": kind, "row": index, "token": h["token"],
                  "root_scope": "authenticated_postrender" if kind == 1 else "structural_pathchoice",
                  "root_delta_ok": h["delta_ok"] == "1", "call_count": integer(h, "calls"),
                  "call_overflow": integer(h, "call_overflow"), "simple_ranges": integer(h, "simple_ranges"),
                  "unclassified_canvas": int(h.get("unclassified_canvas", "0")),
                  "material_ranges": integer(h, "materials"), "ancestor": hex(ancestor) if ancestor else None,
                  "invocation": invocation, "unique_invocation_proven": False,
                  "semantic_pickup_proven": False, "gpu_clip_proven": False, "replay_authorized": False}
        result["observed_functions"] = [
            {"node": hex(pointer(n["node"])), "object_index": integer(n, "object_index"),
             "name_index": integer(n, "name_index"), "number": integer(n, "number"),
             **{key: bytes.fromhex(n[key + "_utf16"]).decode("utf-16-be") for key in ("name", "class", "outer")}}
            for n in row["nodes"].values()]
        result["observed_invocations"] = [{"id": integer(c, "id"), "callee": hex(pointer(c["callee"])),
            "name": bytes.fromhex(c["name_utf16"]).decode("utf-16-be"), "returned": c["returned"] == "1",
            "body_calls": integer(c, "body_calls")} for c in row["children"].values()]
        if ancestor is None and invocation is None:
            result["bounds"] = {k: float(h[k]) for k in ("left", "top", "right", "bottom")} if h["bounds_ok"] == "1" else None
            results.append(result)
            continue
        reasons = []
        child = row["children"].get(invocation) if invocation is not None else None
        if invocation is not None:
            if child is None or child["returned"] != "1" or child.get("current_after") != "1" or integer(child, "exit") < integer(child, "enter"):
                reasons.append("unique_invocation_incomplete")
            if row["child_overflow"]:
                reasons.append("child_invocation_overflow")
        if not all(h[k] == "1" for k in ("before_ok", "returned", "root_current", "delta_ok")):
            reasons.append("root_incomplete")
        if integer(h, "simple_executes") or integer(h, "material_executes"):
            reasons.append("synchronous_execute")
        if result["call_overflow"] or set(row["calls"]) != set(range(result["call_count"])):
            reasons.append("call_inventory_incomplete")
        if result["unclassified_canvas"]:
            reasons.append("unclassified_canvas")
        selected, refs = [], {}
        for call_index, call in sorted(row["calls"].items()):
            c = call["header"]
            if c is None or c["ancestry_complete"] != "1" or set(call["frames"]) != set(range(integer(c, "frames"))):
                reasons.append("ancestry_incomplete")
                continue
            matched = (any(integer(v, "id") == invocation for v in call["logical_ids"].values())
                       if invocation is not None else any(pointer(f["node"]) == ancestor for f in call["frames"].values()))
            if not matched:
                continue
            if invocation is not None and (child is None or not any(
                    all(pointer(f[k]) == pointer(child[v]) for k, v in (("node", "callee"), ("object", "self"), ("previous", "caller"))) and
                    0 <= pointer(f["code"]) - pointer(child["script"]) < integer(child, "script_size")
                    for f in call["frames"].values())):
                reasons.append("invocation_body_ancestry_mismatch")
            selected.append(call_index)
            if c["append_complete"] != "1" or c["mapped_to_root_delta"] != "1" or set(call["members"]) != set(range(integer(c, "members"))):
                reasons.append("selected_call_members_incomplete")
                continue
            for member in call["members"].values():
                refs[(member["material"], pointer(member["item"]), pointer(member["payload"]),
                      integer(member, "key"), integer(member, "begin"), integer(member, "end"))] = member
        points = []
        try:
            for ref in refs.values():
                if pointer(ref["fcanvas"]) != pointer(h["fcanvas"]):
                    raise ValueError("Member canvas mismatch")
                match = lambda p: (pointer(p["item"]) == pointer(ref["item"]) and
                                   pointer(p["payload"]) == pointer(ref["payload"]) and p["key"] == ref["key"])
                if ref["material"] == "1":
                    matches = [p for p in row["material"].values() if match(p)]
                    if len(matches) != 1:
                        raise ValueError("Missing or aliased material")
                    p = matches[0]
                    raw = bytes.fromhex(p["record36"])
                    if len(raw) != 36:
                        raise ValueError("Invalid material record")
                    x, y, w, ht, *_ = struct.unpack("<8f", raw[:32])
                    points.extend(project(p["transform80"], px, py) for px in (x, x + w) for py in (y, y + ht))
                else:
                    matches = [p for p in row["simple"].values() if p["header"] and match(p["header"])]
                    if len(matches) != 1:
                        raise ValueError("Missing or aliased simple range")
                    p = matches[0]
                    b, e = integer(ref, "begin"), integer(ref, "end")
                    start, end = integer(p["header"], "begin"), integer(p["header"], "end")
                    if not start <= b < e <= end or (e - b) % 4:
                        raise ValueError("Invalid member interval")
                    for v in range(b - start, e - start):
                        raw = bytes.fromhex(p["vertices"][v])
                        if len(raw) != 44:
                            raise ValueError("Invalid vertex length")
                        x, y, z, w = struct.unpack("<4f", raw[:16])
                        if z != 0 or w != 1:
                            raise ValueError("Unsupported vertex plane")
                        points.append(project(p["header"]["transform80"], x, y))
        except (ValueError, KeyError, struct.error) as error:
            reasons.append(str(error))
        if not selected or not points:
            reasons.append("ancestor_has_no_complete_geometry")
        if child is not None and len(selected) != integer(child, "body_calls"):
            reasons.append("invocation_body_call_count_mismatch")
        result.update(selected_calls=selected, members=len(refs), rejection_reasons=sorted(set(reasons)),
                      copied_group_candidate=not reasons, bounds=None)
        result["unique_invocation_proven"] = invocation is not None and not reasons
        if not reasons:
            result["bounds"] = {"left": min(x for x, _ in points), "top": min(y for _, y in points),
                                "right": max(x for x, _ in points), "bottom": max(y for _, y in points)}
        results.append(result)
    return {"schema": 1, "source_sha256": hashlib.sha256(data).hexdigest(),
            "read_only": True, "flush_coverage_proven": False, "rows": results}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("status", type=Path)
    choice = parser.add_mutually_exclusive_group()
    choice.add_argument("--ancestor", type=lambda text: int(text, 0))
    choice.add_argument("--invocation", type=int)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    report = decode(args.status.read_bytes(), args.ancestor, args.invocation)
    report["source_path"] = str(args.status.resolve())
    encoded = json.dumps(report, ensure_ascii=False, indent=2, allow_nan=False) + "\n"
    if args.output:
        args.output.write_text(encoded, encoding="utf-8")
    else:
        print(encoded, end="")


if __name__ == "__main__":
    main()
