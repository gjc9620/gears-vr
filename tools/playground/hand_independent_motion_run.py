"""UNSHIPPED hand-only left/right 4cm translation-and-return runner.

Default execution is an intentionally rejected preflight. No project/runtime/SDK
helper is imported, prepared or launched until the exact reviewed typed RigV1,
PG/Native/XR, current renderer/config and runtime binding has passed. Source 0
means RuntimeTracked (including an explicitly selected simulator runtime), never
MetaOperator source 1 or physical-headset acceptance. Sealed AX is read-only.
"""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import math
import os
import re
import struct
import sys
import time
import traceback
from types import SimpleNamespace

WORK = Path(__file__).resolve().parents[2]
SCOPE = "independent_hand_only_translation_return_4cm"
EVIDENCE = WORK / "analysis/hand-independent-motion-20261025bp"
AX_PATH = WORK / "analysis/hand-mesh-runtime-1024ax/run.py"
AX_SHA = "67591232856e5477c250d89b60a6cafa8fa8ffd81d5a37f77cc19038916e9011"
MATH_PATH = WORK / "analysis/hand-equipment-1023h/audit.py"
MATH_SHA = "316a5b745eaff2f3abebdc69eea59d9637bc624bc5493bc4524823c046143ac3"
AX_AUDIT_PATH = WORK / "analysis/hand-mesh-runtime-1024ax/audit.py"
AX_AUDIT_SHA = "6bbfcd395eaf2ab699f6617b857afb5e15acf2191f181f101f31e2dd5b5fa77b"
BRIDGE_PATH = WORK / "tools/playground/hand_independent_motion_bridge.py"
BRIDGE_SHA = "2872bbe56c96f95fc6f617a4f772c4f16a66140a6ee2525f802882ca7c00f039"
RIG_LAYOUT_PATH = WORK / "analysis/hand-rig-policy-20261025br/pg-cpu-attempt1/layout.json"
RIG_LAYOUT_SHA = "a2f5d5e13230caa5cfe7fd70e3b6dd79a456ec7e90a5bce47050b323dae2c9e7"
RIG_HEADER_PATH = WORK / "tools/vr-common/rig_tracking_frame.h"
RIG_HEADER_SHA = "58cbb26f4c9745138040478070c9e5ac50976ee6c1feed094524ce87cdc976b0"
CONNECT_HEADER_PATH = WORK / "tools/playground/hand_rig_connect_request.h"
CONNECT_HEADER_SHA = "b9742be987c68cb8cf653cdbfa921c5b0275f89b3d1489b244770fa2d54bd66b"
DEFAULT_NATIVE_BASE = "4752801af933d3a4773020052716ff5af093baf7a4cb07c8c9a164ea1ad1ae14"
DEFAULT_XR_BASE = "37f463121e4b90d70c76a77417e9e9736aed4da925bb88c51ce8d08345e3ea4d"
TEST_ENV = {"GEARSVR_TEST_WINDOWS_BEHIND": "1", "GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY": "1", "GEARSVR_TEST_MUTE_GAME": "1"}
BUDGET_SECONDS, CLEANUP_RESERVE = 15.0, 3.0
SOURCE_AGE_NS, POSITION_LIMIT, ROTATION_LIMIT_DEG = 100_000_000, 0.1, 0.1
TRANSLATION_METRES = 0.04
RUNTIME_NAMES = {"runtime_probe", "resolve_camera", "start_capture", "run_stereo_probe", "run_stereo_continuous", "runtime"}
RIG_COHORT = ("game_pid", "game_start", "gt", "native_cookie", "native_activation", "world_generation", "binding_generation",
              "xr_pid", "xr_start", "source", "raw_session", "space_epoch", "input_epoch", "profile_generation", "producer_stream",
              "tracking_left", "tracking_right", "native_reference_epoch", "reference_flags", "reference_generation",
              "observed_stream", "observed_reference", "observed_head", "observed_left", "observed_right", "units_per_metre", "view_policy")
REQUIRED_ARTIFACTS = {"pg", "native", "xr", "loader", "source_manifest", "release_manifest", "source_config", "sim_runtime_manifest", "sim_runtime_dll", "audio_guard"}
READ_ONLY_FUNCTIONS = frozenset({"GetWeaponInSlot", "GetStateName", "ShouldLeftHandIKBeOn", "IsComponentAttached", "CanSwitchWeapons", "DoingAnimationTransition", "IsReloading"})
EXECUTION_GATES = ("sole_native_cf_owner", "cf_handoff_ready", "normal_frame_stage_ready")


class BindingError(ValueError):
    pass


def need(value, message):
    if not value:
        raise BindingError(message)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load(path):
    return json.loads(Path(path).read_text(encoding="utf-8-sig"))


def save(path, value, *, exclusive=False):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("x" if exclusive else "w", encoding="utf-8") as stream:
        json.dump(value, stream, indent=2, allow_nan=False)
        stream.write("\n")


def evidence_path(path):
    path = Path(path).resolve()
    need(path.is_relative_to(EVIDENCE.resolve()) and any(p.startswith("runner-") for p in path.relative_to(EVIDENCE.resolve()).parts), "Evidence must stay in this task's runner-* directory")
    return path


def pin(row):
    need(isinstance(row, dict) and isinstance(row.get("sha256"), str) and re.fullmatch("[0-9a-f]{64}", row["sha256"]), "UNBOUND exact file SHA256")
    path = Path(row.get("path", "UNBOUND"))
    need(path.is_absolute() and path.is_file() and digest(path) == row["sha256"], "Missing/changed bound file: " + str(path))
    return path.resolve()


def binding_template():
    missing = lambda: {"path": "UNBOUND", "sha256": "UNBOUND"}
    return {"schema": 1, "scope": SCOPE, "authorized": False, "producer_ready": False, **{name: False for name in EXECUTION_GATES},
            "runner_sha256": "UNBOUND", "run_directory": str(EVIDENCE / "runner-UNBOUND"),
            "renderer_lineage": {"release": "0.0.4-development", "preserve_default": True,
                "native_base_sha256": DEFAULT_NATIVE_BASE, "xr_base_sha256": DEFAULT_XR_BASE},
            "source_profile": {"wire_source": 0, "name": "RuntimeTracked", "simulator_runtime": True, "physical_tracking_acceptance": False},
            "artifacts": {name: missing() for name in sorted(REQUIRED_ARTIFACTS)},
            "source_files": [], "runtime_helpers": [], "extra_native_artifacts": [],
            "helpers": {name: missing() for name in ("base", "discovery", "mesh_preflight", "freshness", "work_profile", "instance", "idle_gui", "retained_bridge", "rig_layout")},
            "early_pg_build_relative": "UNBOUND", "game_path": "UNBOUND", "retail_root": "UNBOUND",
            "rig": {"start_export": "UNBOUND", "status_export": "UNBOUND", "stop_export": "UNBOUND",
                "start_payload": "RigV1Start304", "frame_contract": "RigV1", "hand_only_first": True, "terminal_phase": 6,
                "connect_export": "PlaygroundHandRigConnect", "client_status_export": "PlaygroundWeaponHandStatus",
                "connect_payload": "RigConnect24", "native_world_generation_field": "UNBOUND"},
            "initial_controller_poses": {str(hand): {name: None for name in ("posX", "posY", "posZ", "yaw", "pitch", "trigger", "buttonA")} for hand in range(2)},
            "native_session_options": {"render_scale": None, "stereo_units_per_metre": None},
            "limits": {"budget_seconds": BUDGET_SECONDS, "cleanup_reserve_seconds": CLEANUP_RESERVE, "source_age_ns": SOURCE_AGE_NS,
                       "position_raw_units": POSITION_LIMIT, "rotation_degrees": ROTATION_LIMIT_DEG, "translation_metres": TRANSLATION_METRES},
            "test_environment": {**TEST_ENV, "GEARSVR_TEST_HAND_INDEPENDENT_MOTION": "1", "GEARSVR_TEST_HAND_MOTION_SOURCE": "0"}, "physical_background_input": False}


def require_bindings(path):
    # Keep this ahead of any project import, inventory, copy, prepare or Win32/SDK.
    bound = load(path)
    need(bound.get("schema") == 1 and bound.get("scope") == SCOPE and bound.get("authorized") is True, "UNBOUND/unauthorized hand-only profile; no imports, prepare or run")
    need(bound.get("producer_ready") is True, "Native typed RigV1 producer is not ready; preflight only")
    rig_contract(bound.get("rig", {}))
    need(bound.get("runner_sha256") == digest(__file__), "Runner bytes are not exactly bound")
    template = binding_template()
    need(bound.get("limits") == template["limits"] and bound.get("physical_background_input") is False, "Original 15s/100ms/0.1/background limits differ")
    need(bound.get("renderer_lineage") == template["renderer_lineage"], "Latest Support3/menu/.4 renderer lineage is not preserved")
    profile = bound.get("source_profile", {})
    names = {0: "RuntimeTracked", 1: "MetaOperator"}
    need(type(profile.get("wire_source")) is int and names.get(profile["wire_source"]) == profile.get("name"), "Source 0 RuntimeTracked and source 1 MetaOperator are distinct")
    need(type(profile.get("simulator_runtime")) is bool and profile.get("physical_tracking_acceptance") is False, "Explicit simulator/MetaOperator provenance required; no physical acceptance")
    need(bound.get("test_environment") == {**TEST_ENV, "GEARSVR_TEST_HAND_INDEPENDENT_MOTION": "1", "GEARSVR_TEST_HAND_MOTION_SOURCE": str(profile["wire_source"])}, "Exact independent-motion environment/source character required")
    need(set(bound.get("artifacts", {})) == REQUIRED_ARTIFACTS, "Exact PG/Native/XR/loader/current manifest/config/runtime set required")
    need(all(bound.get(name) is True for name in EXECUTION_GATES),
         "UNBOUND execution readiness: sole_native_cf_owner/cf_handoff_ready/normal_frame_stage_ready are required; compilation and Rig data are not setter permission")
    # Read only the pinned source manifest first. The current compile-only PG
    # profiles have not resolved CF ownership/handoff and Native normal-frame
    # permission, so their successful builds cannot enter artifact preparation.
    source = load(pin(bound["artifacts"]["source_manifest"]))
    need(all(source.get(name) is True for name in EXECUTION_GATES),
         "Actual source manifest does not prove sole_native_cf_owner/cf_handoff_ready/normal_frame_stage_ready; no artifact imports/prepare")
    for item in bound["artifacts"].values():
        pin(item)
    need(source.get("hand_independent_motion_trial") is True and source.get("hand_only_first") is True and source.get("hand_rig_tracking_trial") is True,
         "Actual selected build manifest lacks the typed hand-only RigV1 profile")
    need(type(source.get("hand_wire_source")) is int and type(source.get("hand_motion_expected_source")) is int and
         source["hand_wire_source"] == source["hand_motion_expected_source"] == profile["wire_source"] and
         source.get("renderer_lineage") == bound["renderer_lineage"], "Actual compile-bound source/RigV1/renderer mismatch")
    for name in ("pg", "native", "xr", "loader"):
        need(source.get(name + "_sha256") == bound["artifacts"][name]["sha256"], "Selected artifact/build manifest mismatch: " + name)
    need(bound.get("source_files") and source.get("source_files") == bound["source_files"], "Exact live sources must be bound; no sealed AX source fallback")
    for row in bound["source_files"] + bound.get("extra_native_artifacts", []):
        pin(row)
    for path, expected in ((RIG_HEADER_PATH, RIG_HEADER_SHA), (CONNECT_HEADER_PATH, CONNECT_HEADER_SHA)):
        need(any(Path(row["path"]).resolve() == path.resolve() and row["sha256"] == expected for row in bound["source_files"]),
             "Actual typed Rig/Connect header must be in the exact source manifest: " + path.name)
    helpers = bound.get("helpers", {})
    need(set(helpers) == set(template["helpers"]), "Complete explicit helper origins required")
    for row in helpers.values():
        pin(row)
    need(pin(helpers["retained_bridge"]) == BRIDGE_PATH.resolve() and helpers["retained_bridge"]["sha256"] == BRIDGE_SHA,
         "Exact reviewed retained-export/Submit adapter required; ordinary Bridge is unsafe")
    need(pin(helpers["rig_layout"]) == RIG_LAYOUT_PATH.resolve() and helpers["rig_layout"]["sha256"] == RIG_LAYOUT_SHA,
         "Actual compiled typed RigV1 layout receipt must be exactly bound")
    need({row.get("name") for row in bound.get("runtime_helpers", [])} == RUNTIME_NAMES and len(bound["runtime_helpers"]) == len(RUNTIME_NAMES), "Exact current runtime helper set required")
    for row in bound["runtime_helpers"]:
        need(pin(row).is_relative_to(WORK), "Runtime helper must be in this worktree")
    need(digest(AX_PATH) == AX_SHA and digest(MATH_PATH) == MATH_SHA and digest(AX_AUDIT_PATH) == AX_AUDIT_SHA, "Sealed reusable helper changed")
    run_dir = evidence_path(bound["run_directory"])
    need("UNBOUND" not in str(run_dir), "Unbound run directory")
    early = Path(bound.get("early_pg_build_relative", "UNBOUND"))
    need(not early.is_absolute() and ".." not in early.parts and early.parts[:2] == ("tools", "playground") and "UNBOUND" not in early.parts, "Exact staged PG build required")
    need(pin(bound["artifacts"]["pg"]).parent == (WORK / early).resolve(), "PG must be the actually staged early build")
    rig = bound.get("rig", {})
    rig_contract(rig)
    for key in ("start_export", "status_export", "stop_export"):
        need(isinstance(rig.get(key), str) and re.fullmatch("Playground[A-Za-z0-9]+", rig[key]) and "UNBOUND" not in rig[key], "Exact Rig export required: " + key)
    need(source.get("rig") == rig, "Exact selected Rig export/payload contract differs")
    need(re.fullmatch("[a-z][a-z0-9_]+", rig.get("native_world_generation_field", "")), "Actual Native status world-generation field required")
    config = load(bound["artifacts"]["source_config"]["path"])
    opts = bound.get("native_session_options", {})
    for key in ("render_scale", "stereo_units_per_metre"):
        value = opts.get(key)
        need(type(value) in (int, float) and math.isfinite(value) and value > 0 and config.get(key) == value, "Current selected configuration value required: " + key)
    need(opts["render_scale"] == 0.4, "Autonomous private background render_scale must be exactly 0.4")
    need(10 <= opts["stereo_units_per_metre"] <= 500, "World unit scale outside original range")
    need(config.get("initial_controller_poses") == bound.get("initial_controller_poses"), "Explicit private simulator initial poses must match selected config")
    command_plan(bound)
    need(Path(bound.get("game_path", "")).is_file() and Path(bound.get("retail_root", "")).is_dir(), "Exact purchased game/private runtime roots required")
    return bound


def rig_contract(rig):
    need(rig.get("hand_only_first") is True and rig.get("start_payload") == "RigV1Start304" and
         rig.get("frame_contract") == "RigV1" and rig.get("terminal_phase") == 6,
         "Typed RigV1Start304 required; legacy GHW1/certificate recipes are rejected")
    need(rig.get("connect_export") == "PlaygroundHandRigConnect" and
         rig.get("client_status_export") == "PlaygroundWeaponHandStatus" and rig.get("connect_payload") == "RigConnect24",
         "Exact owned RigConnect24/status export contract required")


def command_plan(bound):
    poses = bound.get("initial_controller_poses", {})
    keys = {"posX", "posY", "posZ", "yaw", "pitch", "trigger", "buttonA"}
    for hand in ("0", "1"):
        need(set(poses.get(hand, {})) == keys, "Complete initial controller pose required")
        need(all(type(value) in (int, float) and math.isfinite(value) for value in poses[hand].values()), "Nonfinite/unknown initial pose")
        need(poses[hand]["trigger"] == 0 and poses[hand]["buttonA"] == 0, "Physical/weapon input must remain off")
    result = []
    for hand in range(2):
        baseline = dict(poses[str(hand)], hand=hand)
        moved = dict(baseline, posX=baseline["posX"] + TRANSLATION_METRES)
        for label, data in (("translate", moved), ("return", baseline)):
            result.append({"ordinal": len(result), "label": ("left" if hand == 0 else "right") + "_" + label, "hand": hand, "data": data})
    return result


def import_pinned(name, path, sha):
    path = Path(path).resolve()
    need(digest(path) == sha, "Changed helper before import")
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    need(Path(module.__file__).resolve() == path and digest(path) == sha, "Changed helper origin/bytes during import")
    return module


def reusable_helpers(bound, out, *, runtime=False):
    ax = import_pinned("independent_ax_helpers", AX_PATH, AX_SHA)
    h = import_pinned("independent_hand_geometry", MATH_PATH, MATH_SHA)
    audit = import_pinned("independent_typed_tail_helpers", AX_AUDIT_PATH, AX_AUDIT_SHA)
    # AX source stays unchanged; imported helper-local output/dependency globals
    # point only at this new run. Never call AX prepare/run/full-scope audit.
    ax.OUT, ax.OWNED, ax.TAG, ax.WORK = out, out, out.name, WORK
    ax.PG, ax.ART = out / "playground", out / "artifacts"
    ax.DLL = ax.CANDIDATE = Path(bound["artifacts"]["pg"]["path"])
    ax.CANDIDATE_SHA = bound["artifacts"]["pg"]["sha256"]
    ax.EARLY_PG_BUILD_REL, ax.LIVE_PG = Path(bound["early_pg_build_relative"]), ax.DLL.parent
    ax.SAVED_DLL = ax.PG / ax.DLL.name
    ax.RETAIL = Path(bound["retail_root"])
    ax.GAME_EXE = Path(bound["game_path"])
    ax.IDLE_GUI_EXE, ax.IDLE_GUI_SHA = Path(bound["helpers"]["idle_gui"]["path"]), bound["helpers"]["idle_gui"]["sha256"]
    ax.WORK_RUNTIME_PINS = tuple((row["name"], str(Path(row["path"]).relative_to(WORK)), row["sha256"]) for row in bound["runtime_helpers"])
    for field, name in (("BASE", "base"), ("BUTTONS", "discovery"), ("MESH_PREFLIGHT", "mesh_preflight"), ("FRESHNESS", "freshness"), ("WORK_PROFILE", "work_profile")):
        setattr(ax, field, Path(bound["helpers"][name]["path"]))
        setattr(ax, field + "_SHA", bound["helpers"][name]["sha256"])
    ax.H_MATH, ax.H_MATH_SHA = MATH_PATH, MATH_SHA
    ax.ALLOW = {bound["rig"][key] for key in ("start_export", "status_export", "stop_export", "connect_export", "client_status_export")} | {"PlaygroundInitialize", "PlaygroundEnable", "PlaygroundTestEnter", "PlaygroundVrFireStatus", "PlaygroundVrAimStatus", "PlaygroundEquipmentStowStatus", "PlaygroundEquipmentSwitchStatus", "PlaygroundShutdown"}
    ax.AUDIO, ax.AUDIO_SHA = Path(bound["artifacts"]["audio_guard"]["path"]), bound["artifacts"]["audio_guard"]["sha256"]
    bridge_adapter = import_pinned("independent_retained_bridge", BRIDGE_PATH, BRIDGE_SHA) if runtime else None
    rt = ax.runtime_modules() if runtime else None
    return SimpleNamespace(ax=ax, h=h, audit=audit, rt=rt, bridge_adapter=bridge_adapter,
                           native_modules=sys.modules["start_capture"].modules if runtime else None)


def checked_reservation_birth(row, expected_image):
    """Validate the one launch receipt, including its immutable game identity."""
    need(isinstance(row, dict) and row.get("schema") == 1 and row.get("kind") == "reservation_birth", "Unknown reservation birth receipt")
    need(type(row.get("injection_attempted")) is bool and type(row.get("prime_attempted")) is bool, "Malformed reservation progress flags")
    born = row.get("game_identity")
    need(isinstance(born, dict) and type(born.get("pid")) is int and born["pid"] > 0 and
         type(born.get("creation_time")) is int and born["creation_time"] > 0 and isinstance(born.get("image"), str) and
         str(Path(born["image"]).resolve()).casefold() == str(Path(expected_image).resolve()).casefold(), "Reservation birth is not the exact selected game")
    return dict(born)


def cleanup_observation_error(receipt, field):
    receipt[field + "_count"] = receipt.get(field + "_count", 0) + 1
    rows = receipt.setdefault(field, [])
    rows.append(traceback.format_exc())
    del rows[:-8]


def reservation_cleanup_step(launch, read_birth, adopt, identity, receipt):
    """A timeout/worker failure cannot prove that its one game was never born.

    Return ready only after the original worker has exited AND an authenticated
    identity is journaled. The caller keeps its owner alive for any other state.
    This step starts nothing and never resolves an unknown identity by name.
    """
    if launch is None:
        return True
    receipt["reservation_cleanup_pending"] = True
    try:
        row = read_birth()
        if row is not None:
            adopt(row)
        code = launch.poll()
        if code is None:
            return False
        receipt["reservation_worker_exit"] = code
        # Re-read after exit so a birth committed immediately before worker exit
        # is adopted in this same cleanup pass, before exact-game cleanup.
        row = read_birth()
        if row is not None:
            adopt(row)
        if identity() is None:
            receipt["reservation_identity_unresolved"] = True
            return False
        receipt["reservation_cleanup_pending"] = False
        receipt["reservation_identity_unresolved"] = False
        return True
    except BaseException:
        cleanup_observation_error(receipt, "reservation_cleanup_observation_errors")
        return False


def settle_reservation_before_exact_cleanup(launch, read_birth, adopt, identity, receipt, *, persist, sleep=time.sleep):
    # No new worker, bounded wait or KeyboardInterrupt can abandon a possible
    # late owned child. Individual observations remain finite; the owner stays.
    while not reservation_cleanup_step(launch, read_birth, adopt, identity, receipt):
        try:
            persist()
        except BaseException:
            cleanup_observation_error(receipt, "reservation_cleanup_persist_errors")
        try:
            sleep(.1)
        except BaseException:
            cleanup_observation_error(receipt, "reservation_cleanup_wait_errors")


def loaded_native_base(bridge, list_modules, selected_native, identity):
    selected_native = Path(selected_native).resolve()
    need(selected_native.name.casefold() == "gearscapture.dll" and bridge.identity == identity and bridge.handle,
         "Exact selected Native/game handle identity required")
    handle = bridge.handle
    bridge.verify()
    rows = list_modules(handle)  # The exact already-owned game handle, never an inventory PID or LoadLibrary.
    bridge.verify()
    need(bridge.handle == handle and bridge.identity == identity and isinstance(rows, dict), "Game handle/identity changed during Native module read")
    matches = []
    for value in rows.values():
        need(isinstance(value, (tuple, list)) and len(value) == 2, "Malformed loaded module observation")
        address, path = value
        if str(Path(path).resolve()).casefold() == str(selected_native).casefold():
            need(type(address) is int and 0 < address < 1 << 64 and not address & 7, "Invalid actual loaded Native module base")
            matches.append(address)
    need(len(matches) == 1, "Exact selected/native/GearsCapture.dll is not uniquely already loaded")
    return matches[0]


def connect_typed_rig(bridge, rig, identity, selected_native, native_sha, list_modules, live_native,
                      expected_activation, deadline, receipt, *, clock=time.monotonic):
    rig_contract(rig)
    need(not receipt.get("rig_connect_attempted"), "Rig Connect is one-shot; no replay")
    need(type(expected_activation) is int and 0 < expected_activation < 1 << 64, "Exact positive Native incarnation required")
    need(clock() < deadline, "Original startup deadline exhausted before Rig Connect")
    bridge.set_deadline(deadline)
    before = live_native()
    need(before and positive(before, "started_qpc") == expected_activation, "Native incarnation changed before Rig Connect")
    selected_native = Path(selected_native).resolve()
    need(selected_native.is_file() and digest(selected_native) == native_sha, "Selected private Native bytes differ before Connect")
    address = loaded_native_base(bridge, list_modules, selected_native, identity)
    payload = struct.pack("<IIQQ", 1, 24, address, expected_activation)
    need(clock() < deadline, "Original startup deadline exhausted during Native module observation")
    receipt["rig_connect"] = {"payload": "RigConnect24", "payload_hex": payload.hex(), "native_module": address,
                              "native_path": str(selected_native), "native_sha256": native_sha,
                              "native_started_qpc": expected_activation, "startup_deadline_monotonic": deadline,
                              "native_before": before, "data_registration_only": True, "grip_or_engine_grant": False}
    receipt["rig_connect_attempted"] = True  # Dispatch obligation exists even if owned_export raises.
    value = bridge.owned_export(rig["connect_export"], payload)
    receipt["rig_connect"]["reply"] = value
    need(type(value) is int and value == 0, "Typed Rig Connect was not accepted")
    status_value = bridge.export(rig["client_status_export"])
    receipt["rig_connect"]["client_status"] = status_value
    need(type(status_value) is int and 0 <= status_value <= 0xffffffff and status_value & 0xff == 2,
         "Typed Rig client is not active; armed is not a required/grip grant")
    after = live_native()
    need(after and positive(after, "started_qpc") == expected_activation, "Native incarnation changed across Rig Connect")
    need(clock() < deadline, "Original startup deadline exhausted by Rig Connect")
    receipt["rig_connect"]["native_after"] = after
    return address


def owned_xr_identity(report, control, selected_xr):
    value = report.get("xr_identity")
    need(isinstance(value, dict) and type(value.get("pid")) is int and value["pid"] > 0 and
         type(value.get("creation_time")) is int and value["creation_time"] > 0 and isinstance(value.get("image"), str) and
         str(Path(value["image"]).resolve()).casefold() == str(Path(selected_xr).resolve()).casefold(), "Exact selected owned XR report identity is missing/different")
    need(control.identity(value["pid"]) == value, "Owned XR process identity changed")
    return dict(value)


def start_object_identities(headers):
    need(len(headers) == 6 and all(type(address) is int and len(raw) == 0x58 for address, raw in headers), "Complete actual Start object headers required")
    result = []
    for index in (0, 2, 3, 4, 5):  # Same five object roles as the typed Rig Frame; level is Start-only.
        address, raw = headers[index]
        result.append(struct.pack("<QQQ4I", address, struct.unpack_from("<Q", raw, 0x40)[0],
            struct.unpack_from("<Q", raw, 0x50)[0], struct.unpack_from("<I", raw, 8)[0],
            *struct.unpack_from("<II", raw, 0x48), 1).hex())
    return tuple(result)


def fields(text):
    result = {}
    for token in text.split()[1:]:
        key, sep, value = token.partition("=")
        need(sep and key not in result, "Malformed/duplicate source field")
        result[key] = value
    return result


def positive(row, key):
    value = row.get(key)
    need(isinstance(value, (str, int)) and re.fullmatch(r"\d+", str(value)) and int(value) > 0, "Unknown/nonpositive " + key)
    return int(value)


def verified_rig_layout():
    # This is a real MSVC sizeof/offsetof receipt over the exact current typed
    # header. No runtime/AX import or guessed packed-layout fallback exists.
    need(digest(RIG_LAYOUT_PATH) == RIG_LAYOUT_SHA and digest(RIG_HEADER_PATH) == RIG_HEADER_SHA,
         "Typed RigV1 layout/header changed; no decoding fallback")
    value = load(RIG_LAYOUT_PATH)
    need(value["frame_bytes"] == 1824 and value["frame_alignment"] == value["pointer_bytes"] == 8,
         "Actual compiled RigV1 recipe differs")
    return value


def decode_rig_receipt(row, pose, bound, layout):
    need(row.get("schema") == "1" and row.get("continuity") == "RigV1" and row.get("stage") == "before_write",
         "Actual before-write typed RigV1 sidecar required")
    count = positive(row, "bytes")
    need(count == layout["frame_bytes"] and re.fullmatch(r"[0-9a-f]{" + str(2 * count) + r"}", row.get("frame_hex", "")),
         "Complete exact-size RigV1 Frame bytes required")
    raw = bytes.fromhex(row["frame_hex"])
    get = lambda fmt, offset: struct.unpack_from(fmt, raw, offset)[0]
    field = lambda name, fmt="<Q": get(fmt, layout["frame_" + name])
    size, abi, status_value, flags = struct.unpack_from("<IIII", raw)
    need(size == count and abi == 1 and status_value == 1 and flags & 3 == 3 and not flags & ~7,
         "RigV1 Frame header/status/producer continuity is not ready")
    game_pid, gt, game_start, activation, cookie = struct.unpack_from("<IIQQQ", raw, layout["frame_native"])
    rig = {"game_pid": game_pid, "gt": gt, "game_start": game_start, "native_activation": activation, "native_cookie": cookie,
           "publication": field("publication_sequence"), "scene": field("scene_invocation"),
           "world_generation": field("world_generation"), "binding_generation": field("binding_generation"),
           "reference_generation": field("reference_generation"), "published_ns": field("published_ns"),
           "scene_qpc": field("scene_qpc", "<q"), "qpc_frequency": field("qpc_frequency", "<q"),
           "xr_pid": field("controller_publisher_pid", "<I"), "xr_start": field("controller_publisher_start"),
           "units_per_metre": field("units_per_metre", "<d"), "view_policy": field("view_policy", "<I")}
    for key, value in zip(("observed_stream", "observed_reference", "observed_head", "observed_left", "observed_right"),
                          struct.unpack_from("<5Q", raw, layout["frame_observed"])):
        rig[key] = value
    for key, value in zip(("producer_stream", "tracking_left", "tracking_right"),
                          struct.unpack_from("<3Q", raw, layout["frame_producer"])):
        rig[key] = value
    rig["native_reference_epoch"], rig["reference_flags"], reference_reserved = struct.unpack_from("<QII", raw, layout["frame_native_reference"])
    need(rig["reference_flags"] == 1 and reference_reserved == 0 and field("reserved", "<I") == 0 and
         field("world_pose_mask", "<I") == 7, "RigV1 reference/both world grips are not complete")
    stereo, controllers = layout["frame_stereo"], layout["frame_controllers"]
    for key, member, fmt in (("source", "source", "<I"), ("raw_session", "session", "<Q"),
                             ("raw_sequence", "sequence", "<Q"), ("stereo_ns", "received_ns", "<q"), ("space_epoch", "space_epoch", "<Q")):
        rig[key] = get(fmt, stereo + layout["stereo_" + member])
    for member in ("source", "session", "sequence", "space_epoch"):
        target = {"session": "raw_session", "sequence": "raw_sequence"}.get(member, member)
        need(get("<I" if member == "source" else "<Q", controllers + layout["controllers_" + member]) == rig[target],
             "RigV1 controller/stereo frame pairing differs")
    for key, fmt in (("input_epoch", "<Q"), ("profile_generation", "<Q"), ("input_ns", "<q"), ("sync_ns", "<q")):
        member = "received_ns" if key == "input_ns" else key
        rig[key] = get(fmt, controllers + layout["controllers_" + member])
    # Text fields are observations of this same complete Frame; disagreement
    # rejects the record instead of preferring a convenient representation.
    for key in ("publication", "scene", "native_cookie", "native_activation", "game_pid", "game_start", "gt", "world_generation",
                "binding_generation", "xr_pid", "xr_start", "raw_session", "raw_sequence", "producer_stream", "tracking_left", "tracking_right",
                "native_reference_epoch", "reference_generation", "observed_stream", "observed_reference", "observed_head", "observed_left",
                "observed_right", "stereo_ns", "input_ns", "published_ns"):
        need(positive(row, key) == rig[key], "RigV1 sidecar/Frame bytes differ: " + key)
    need(int(row["source"]) == rig["source"] == bound["source_profile"]["wire_source"] and int(row["reference_flags"]) == rig["reference_flags"],
         "RigV1 actual source/reference differs")
    need(row.get("current_tick_equivalence") == row.get("engine_permission") == "0" and row.get("handworld_adapter_only") == "1",
         "Prepared Rig receipt must not claim current-Tick equivalence or engine permission")
    for key in RIG_COHORT:
        if key not in ("source", "units_per_metre", "view_policy"):
            need(rig[key] > 0, "Unknown typed RigV1 cohort field: " + key)
    game, xr = bound.get("_game_identity", {}), bound.get("_xr_identity", {})
    need((rig["game_pid"], rig["game_start"]) == (game.get("pid"), game.get("creation_time")) and
         rig["native_activation"] == bound.get("_native_started_qpc") and rig["world_generation"] == bound.get("_native_world_generation"),
         "Typed Rig receipt belongs to another game/Native incarnation/world")
    need((rig["xr_pid"], rig["xr_start"]) == (xr.get("pid"), xr.get("creation_time")), "Typed Rig controller publisher differs from the owned XR child")
    units = rig["units_per_metre"]
    need(math.isfinite(units) and 10 <= units <= 500 and units == bound["native_session_options"]["stereo_units_per_metre"] and
         math.isclose(float(pose["units_per_metre"]), units, rel_tol=1e-8, abs_tol=1e-8), "Actual typed Rig world scale differs")
    for key, typed in (("publication", "publication"), ("serial", "publication"), ("scene", "scene"), ("sequence", "raw_sequence"),
                       ("session", "raw_session"), ("space_epoch", "space_epoch"), ("input_epoch", "input_epoch"), ("world_generation", "world_generation")):
        need(positive(pose, key) == rig[typed], "Pose geometry adapter/Rig Frame pairing differs: " + key)
    need(pose.get("created_ms_is_local_receipt") == pose.get("handworld_adapter_only") == "1" and pose.get("current_tick_equivalence") == "0",
         "Rig pose local receipt/source semantics differ")
    observed = positive(row, "receipt_ns")
    need(observed == positive(pose, "receipt_ns") and positive(row, "receipt_ms") == positive(pose, "created_ms"), "Rig local observation differs")
    need(0 < rig["sync_ns"] <= rig["input_ns"] <= rig["published_ns"] <= observed and rig["scene_qpc"] > 0 and
         rig["qpc_frequency"] == bound.get("_qpc_frequency") and rig["qpc_frequency"] > 0,
         "Actual Rig publication/clock receipt is invalid")
    for key, label in (("stereo_ns", "stereo_age_ns"), ("input_ns", "input_age_ns")):
        need(0 < rig[key] <= rig["published_ns"] and 0 <= observed - rig[key] <= SOURCE_AGE_NS and
             int(pose[label]) == observed - rig[key], "Original 100ms actual Rig source age differs/exceeded")
    need(observed - rig["published_ns"] <= SOURCE_AGE_NS, "Actual Rig publication is stale")
    identities = tuple(raw[layout["frame_" + name]:layout["frame_" + name] + layout["object_bytes"]].hex()
                       for name in ("world", "controller", "pawn", "mesh", "weapon"))
    need(identities == bound.get("_rig_object_identity_hex"), "Rig complete object identities differ from the actual Start snapshot")
    cover = raw[layout["frame_cover_link"]:layout["frame_cover_link"] + layout["object_bytes"]].hex(), field("cover_slot", "<i"), field("cover_type", "<I")
    reference = raw[layout["frame_reference"]:layout["frame_reference"] + layout["pose_bytes"]].hex()
    grips = [struct.unpack_from("<7d", raw, layout["frame_world_grip"] + hand * layout["pose_bytes"]) for hand in range(2)]
    need(all(math.isfinite(x) for grip in grips for x in grip), "Nonfinite actual Rig world-grip data")
    for hand in range(2):
        need(math.dist(grips[hand][:3], pose["grip" + str(hand) + "_pose"][:3]) <= POSITION_LIMIT,
             "Logged geometry adapter position differs from its actual Rig world grip")
    rig.update(world_grips=grips, object_identity_hex=identities, cover_key=cover, reference_hex=reference,
               receipt_ns=observed, receipt_ms=positive(row, "receipt_ms"), sidecar=row, frame_hex=row["frame_hex"],
               cohort=tuple(rig[key] for key in RIG_COHORT) + identities + cover + (reference,))
    return rig


def sample_id(item):
    pose, rig = item["pose"], item["rig"]
    return {"lease": item["lease"], "sequence": item["sequence"], "serial": positive(pose, "serial"),
            "publication": rig["publication"], "scene": rig["scene"], "published_ns": rig["published_ns"],
            "input_ns": rig["input_ns"], "stereo_ns": rig["stereo_ns"],
            "line": item["written"]["line"], "pose_line": pose["line"], "before_line": item["before"]["line"],
            "pre_line": item["pre"]["line"], "restore_line": item["restore"]["line"],
            "action_epoch": positive(pose, "action_epoch")}


def completed_hand_only(text, h, bound, audit, *, after_line=0):
    # Reuse sealed parsing/rigid math only, not AX's phase2/joint/whole-run gates.
    _, samples, poses, restores, written = h.parse(text.splitlines())
    lines, layout = text.splitlines(), verified_rig_layout()
    result = []
    for lease, (key, w) in sorted(written.items()):
        group, pose, restore = samples[key], poses.get(key), restores.get(lease)
        if not pose or not restore or not all(stage in group for stage in ("before", "written", "pre_restore")):
            continue
        b, p = group["before"], group["pre_restore"]
        if b["line"] <= after_line:
            continue
        need(b["line"] < pose["line"] < w["line"] < p["line"] < restore["line"], "Actual hand sample/restore order differs")
        phase = int(pose["equipment_phase"])
        need(phase in (1, 2) and pose.get("hand_only") == "1" and pose.get("continuity") == "RigV1", "Only actual typed RigV1 phase1/2 hand-only leases qualify")
        if pose["line"] >= len(lines) or not lines[pose["line"]].startswith("hand_rig_receipt "):
            continue  # Missing/interleaved sidecar remains pending, never nearest-neighbour paired.
        sidecar = {**fields(lines[pose["line"]]), "line": pose["line"] + 1}
        if sidecar.get("stage") != "before_write":
            continue
        rig = decode_rig_receipt(sidecar, pose, bound, layout)
        need(sidecar["line"] < w["line"], "Rig before-write sidecar is not before the written sample")
        need(pose.get("tracked_mask") == pose.get("planned_mask") == w.get("trial_apply_mask") == p.get("trial_apply_mask") == "3", "Both actual hands must participate")
        need(b.get("trial_apply_mask") == "0" and all(row.get("action_epoch") == pose.get("action_epoch") for row in (b, w, p)), "Hand lease epoch/clean-before sample differs")
        need(all(row.get("equipment_phase") == str(phase) and row.get("mesh_tick_known") == "1" for row in (b, w, p)), "Unknown/cross-phase actual bone evidence")
        need(int(p["mesh_tick"]) > int(w["mesh_tick"]), "No later mesh Tick/pre-restore evidence")
        need(restore.get("dirty_before") == restore.get("restored") == "1" and restore.get("dirty_after") == "0", "Actual exact-control restore receipt is missing/unclean")
        need(0 <= int(w["ms"]) - int(pose["created_ms"]) <= 100, "Original 100ms pose limit exceeded")
        qpc, frequency = positive(w, "qpc"), rig["qpc_frequency"]
        written_ns = (qpc // frequency) * 1_000_000_000 + (qpc % frequency) * 1_000_000_000 // frequency
        need(written_ns >= rig["receipt_ns"] and all(0 <= written_ns - rig[key] <= SOURCE_AGE_NS for key in ("stereo_ns", "input_ns")),
             "Original 100ms actual source age exceeded by the written sample")
        for channel in range(4):
            target = "target" + str(channel)
            need(b[target] == w[target] == p[target], "Logged writer targets changed within lease")
        residuals = {}
        for name, channel in (("left", 2), ("right", 0)):
            target = struct.unpack("<3f", h.hexbytes(w["target" + str(channel)], 12))
            rotation = struct.unpack("<3i", h.hexbytes(w["target" + str(channel + 1)], 12))
            position_error = math.dist(target, p[name + "_matrix"][12:15])
            rotation_error = h.angle(h.axes(p[name + "_matrix"]), h.rotator_axes(rotation))
            need(position_error <= POSITION_LIMIT and rotation_error <= ROTATION_LIMIT_DEG, "Actual later-Tick bone residual exceeds original 0.1 limits")
            residuals[name] = {"position_raw_units": position_error, "rotation_degrees": rotation_error}
        result.append({"lease": lease, "sequence": key[1], "pose": pose, "rig": rig, "before": b, "written": w, "pre": p,
                       "restore": restore, "restored_world_bones": {"proved": False, "reason": "No same-lease post-restore bone getter/sample exists"},
                       "cohort": rig["cohort"] + (pose["action_epoch"], phase), "residuals": residuals,
                       "after_written_source_age_ns": {name: written_ns - rig[name] for name in ("stereo_ns", "input_ns")}})
    if result:
        # The immediate matrix need not have reevaluated. Validate the actual
        # typed tail bytes against baseline/targets, then the later-Tick bones.
        prefix = "\n".join(text.splitlines()[:max(item["restore"]["line"] for item in result)]) + "\n"
        need(bound.get("_tail_baseline"), "Original typed tail metadata/bytes are unavailable")
        audit.tail_samples(prefix, bound["_tail_baseline"])
        for item in result:
            item["typed_tail_rows"] = [{"line": line, **fields(raw)} for line, raw in enumerate(prefix.splitlines(), 1)
                if raw.startswith("hand_tail_translation_sample ") and int(fields(raw)["lease"]) == item["lease"]]
    return result


def binding_group(text, h, bound, cursor, audit):
    complete = completed_hand_only(text, h, bound, audit, after_line=cursor)
    by_key = {(x["lease"], x["sequence"]): x for x in complete}
    binding = None
    group = []
    for line, raw in enumerate(text.splitlines(), 1):
        if line <= cursor or not raw.startswith("hand_ik_xr_stream_pose "):
            continue
        pose = fields(raw)
        if pose.get("hand_only") != "1" or pose.get("equipment_phase") not in ("1", "2"):
            continue
        need(pose.get("continuity") == "RigV1", "Legacy GHW1 pose cannot satisfy typed RigV1 selection")
        key = (positive(pose, "lease"), positive(pose, "sequence"))
        if int(pose["bind_mask"]):
            binding, group = by_key.get(key), []
            continue
        item = by_key.get(key)
        if binding and item and item["cohort"] == binding["cohort"]:
            group.append(item)
        elif item:
            binding, group = None, []
    return binding, group


def pure_translation(previous, current, hand, h, *, baseline=None, returning=False):
    need(previous["cohort"] == current["cohort"], "Hand binding/source cohort changed")
    other = 1 - hand
    channel, other_channel = (2, 0) if hand == 0 else (0, 2)
    a, b = (row["rig"]["world_grips"][hand] for row in (previous, current))
    need(a[3:] == b[3:], "Pure translation changed grip rotation")
    for c in (channel + 1, other_channel, other_channel + 1):
        need(previous["written"]["target" + str(c)] == current["written"]["target" + str(c)], "Rotation/other-hand target changed during independent translation")
    other_control = 3 if other == 1 else 7
    tail = lambda item: next(row for row in item["typed_tail_rows"] if row["phase"] == "written" and int(row["control"]) == other_control)
    need(all(tail(previous)[name] == tail(current)[name] for name in ("translation", "space", "space_bone", "flags", "raw_flags")), "Actual other-hand typed tail parameters changed")
    need(previous["pose"]["grip" + str(other) + "_pose"] == current["pose"]["grip" + str(other) + "_pose"], "Other controller pose changed")
    need(previous["rig"]["world_grips"][other] == current["rig"]["world_grips"][other], "Actual other Rig world grip changed")
    units = current["rig"]["units_per_metre"]
    displacement = [b[i] - a[i] for i in range(3)]
    need(abs(math.sqrt(sum(x*x for x in displacement)) - TRANSLATION_METRES * units) <= POSITION_LIMIT, "Actual source translation is not 4cm")
    ta, tb = (struct.unpack("<3f", h.hexbytes(row["written"]["target" + str(channel)], 12)) for row in (previous, current))
    need(math.dist([tb[i] - ta[i] for i in range(3)], displacement) <= POSITION_LIMIT, "Actual parameter translation does not follow the same hand source")
    if returning:
        need(baseline is not None, "Return has no actual baseline")
        base = baseline["rig"]["world_grips"][hand]
        base_target = struct.unpack("<3f", h.hexbytes(baseline["written"]["target" + str(channel)], 12))
        need(math.dist(base[:3], b[:3]) <= POSITION_LIMIT and base[3:] == b[3:] and math.dist(base_target, tb) <= POSITION_LIMIT, "Actual hand source/parameters did not return to baseline")
    return {"hand": hand, "pure_translation": True, "returned": returning, "source_delta_world": displacement,
            "source_delta_metres": math.sqrt(sum(x*x for x in displacement)) / units, "other_hand_unchanged": True}


class Budget:
    def __init__(self, clock=time.monotonic):
        self.clock, self.began = clock, clock()
        self.deadline = self.began + BUDGET_SECONDS

    def remaining(self):
        return self.deadline - self.clock()

    def check(self, reserve=CLEANUP_RESERVE):
        need(self.remaining() > reserve, "Original 15-second budget exhausted/reserved for cleanup; no extension")


def status(io, budget, receipt):
    budget.check()
    io.live_native()
    raw = io.status()
    need(type(raw) is int and 0 <= raw <= 0xffffffff and (raw & 0xffff) in (0, 1, 2, 3, 4) and (raw >> 16) <= 256, "Invalid/terminal hand status")
    receipt["last_hand_status"] = {"raw": raw, "phase": raw & 0xffff, "leases": raw >> 16}
    budget.check()


def wait_response(io, budget, receipt, helpers, bound, cursor, *, binding=None, command=None, previous=None, baseline=None, hand=None, returning=False):
    while True:
        status(io, budget, receipt)
        actual, group = binding_group(io.log_text(), helpers.h, bound, cursor, helpers.audit)
        if actual is not None and group:
            need(binding is None or sample_id(actual) == binding, "Hand-only binding changed; no command replay")
            for item in group:
                identity = sample_id(item)
                if command:
                    if min(identity[name] for name in ("before_line", "pose_line", "line", "pre_line", "restore_line")) <= command["ack_line"]:
                        continue
                    if any(identity[name] <= command["barrier"][name] for name in ("lease", "sequence", "serial", "scene", "published_ns", "input_ns", "stereo_ns")):
                        continue
                    if any(identity[name] <= command["ack_qpc_boundary"]["ns"] for name in ("input_ns", "stereo_ns")):
                        continue
                    first, second = (row["pose"]["grip" + str(hand) + "_pose"] for row in (previous, item))
                    if abs(math.dist(first[:3], second[:3]) - TRANSLATION_METRES * float(item["pose"]["units_per_metre"])) > POSITION_LIMIT:
                        continue  # Wait for a response; never resend the command.
                    pure_translation(previous, item, hand, helpers.h, baseline=baseline, returning=returning)
                budget.check()
                return actual, item
        time.sleep(.02)


def command_hand(io, budget, receipt, helpers, bound, cursor, binding, planned):
    status(io, budget, receipt)
    actual, group = binding_group(io.log_text(), helpers.h, bound, cursor, helpers.audit)
    need(actual is not None and group and sample_id(actual) == binding, "Complete actual hand-only binding unavailable before command")
    command = Path(io.command_path)
    ack = command.with_name("command_ack.json")
    need(not command.exists(), "Command already pending; exclusive file cannot be overwritten/retried")
    prior = ack.stat().st_mtime_ns if ack.exists() else 0
    sent = time.monotonic()
    deadline = min(sent + 1.0, budget.deadline - CLEANUP_RESERVE)
    row = {**planned, "data": dict(planned["data"]), "binding": binding, "attempted": True, "consumed": False,
           "previous_ack_mtime_ns": prior, "before_line": len(io.log_text().splitlines()), "sent_monotonic": sent, "deadline_monotonic": deadline}
    receipt.setdefault("controller_commands", []).append(row)
    with command.open("x", encoding="utf-8") as stream:
        json.dump(row["data"], stream, allow_nan=False)
    while command.exists() or not ack.exists() or ack.stat().st_mtime_ns <= prior:
        status(io, budget, receipt)
        need(time.monotonic() < deadline, "No fresh exclusive-file ACK within original budget; no retry")
        time.sleep(.01)
    status(io, budget, receipt)
    acknowledged = time.monotonic()
    need(acknowledged < deadline, "ACK arrived after original command deadline")
    value = load(ack)
    need(value.get("command") == "controller_pose" and value.get("success") is True, "ACK rejected/different command")
    fence = helpers.ax.qpc_boundary()
    need(fence.get("algorithm") == "QPC_divmod_integer_ns" and fence["ns"] == (fence["qpc"] // fence["frequency"]) * 1_000_000_000 + (fence["qpc"] % fence["frequency"]) * 1_000_000_000 // fence["frequency"], "Unknown QPC/source-clock barrier")
    text = io.log_text()
    actual, group = binding_group(text, helpers.h, bound, cursor, helpers.audit)
    need(actual is not None and group and sample_id(actual) == binding, "Binding changed across ACK; no replay")
    row.update(ack=value, ack_mtime_ns=ack.stat().st_mtime_ns, consumed=True, ack_monotonic=acknowledged,
               remaining_after_ack=budget.remaining(), ack_qpc_boundary=fence, ack_line=len(text.splitlines()), barrier=sample_id(group[-1]))
    return row


def run_motion(io, budget, receipt, helpers, bound, cursor):
    need("independent_selection" not in receipt, "Motion flow already entered; no replay")
    actual, baseline = wait_response(io, budget, receipt, helpers, bound, cursor)
    binding = sample_id(actual)
    receipt["independent_selection"] = {"binding": binding, "initial": sample_id(baseline), "responses": []}
    previous = baseline
    for planned in command_plan(bound):
        command = command_hand(io, budget, receipt, helpers, bound, cursor, binding, planned)
        _, response = wait_response(io, budget, receipt, helpers, bound, cursor, binding=binding, command=command,
                                    previous=previous, baseline=baseline, hand=planned["hand"], returning=planned["label"].endswith("return"))
        movement = pure_translation(previous, response, planned["hand"], helpers.h, baseline=baseline, returning=planned["label"].endswith("return"))
        command["response"] = sample_id(response)
        receipt["independent_selection"]["responses"].append({"ordinal": planned["ordinal"], "identity": sample_id(response), "motion": movement,
            "actual_evidence": {name: response[name] for name in ("before", "pose", "written", "pre", "restore", "typed_tail_rows", "restored_world_bones", "residuals")}})
        previous = response
    receipt["control_restore_receipts"] = [entry["actual_evidence"]["restore"] for entry in receipt["independent_selection"]["responses"]]
    receipt["independent_four_commands_complete"] = True
    return receipt["independent_selection"]


def prepare(bound, binding_path):
    """Future exact-bound staging only; current UNBOUND preflight cannot enter."""
    out = evidence_path(bound["run_directory"])
    need(not (out / "run-started.json").exists() and not (out / "receipt.json").exists(), "One-shot evidence exists; no retry/reuse")
    out.mkdir(parents=True, exist_ok=True)
    helpers = reusable_helpers(bound, out)
    ax = helpers.ax
    artifact = out / "artifacts"
    copies = []
    names = {"native": "native/GearsCapture.dll", "xr": "xr/OpenXRPoseProbe.exe", "loader": "xr/openxr_loader.dll", "sim_runtime_dll": "simulator/openxr_simulator.dll"}
    for name, relative in names.items():
        row = bound["artifacts"][name]
        copies.append(ax.copy_checked(row["path"], artifact / relative, row["sha256"]))
    for row in bound.get("extra_native_artifacts", []):
        relative = Path(row["relative"])
        need(not relative.is_absolute() and ".." not in relative.parts, "Invalid exact native dependency stage path")
        copies.append(ax.copy_checked(row["path"], artifact / "native" / relative, row["sha256"]))
    # A private manifest references the exact selected runtime DLL; no registry
    # edit, installed-runtime substitution or old AX renderer/config fallback.
    private_runtime = artifact / "simulator/openxr_simulator.json"
    save(private_runtime, {"file_format_version": "1.0.0", "runtime": {"library_path": str(artifact / "simulator/openxr_simulator.dll")}})
    selected = load(bound["artifacts"]["release_manifest"]["path"])
    manifest = dict(selected)
    manifest.update(game_path=bound["game_path"], native_build=str(artifact / "native"), native_dll="GearsCapture.dll",
                    native_sha256=bound["artifacts"]["native"]["sha256"], xr_build=str(artifact / "xr"), xr_sha256=bound["artifacts"]["xr"]["sha256"],
                    playground_build=bound["early_pg_build_relative"], playground_dll=Path(bound["artifacts"]["pg"]["path"]).name,
                    playground_sha256=bound["artifacts"]["pg"]["sha256"])
    save(out / "private-manifest.json", manifest)
    dependencies = {str(Path(row["path"]).resolve()): row["sha256"] for row in [*bound["source_files"], *bound["helpers"].values(), *bound["runtime_helpers"], *bound["artifacts"].values()]}
    dependencies.update({str(AX_PATH): AX_SHA, str(MATH_PATH): MATH_SHA, str(AX_AUDIT_PATH): AX_AUDIT_SHA, str(Path(__file__).resolve()): digest(__file__)})
    result = {"schema": 1, "scope": SCOPE, "runner_sha256": digest(__file__), "binding_sha256": digest(binding_path),
              "dependencies": dependencies, "artifact_copies": copies, "runtime_manifest_sha256": digest(private_runtime),
              "private_manifest_sha256": digest(out / "private-manifest.json"), "limits": bound["limits"], "commands": command_plan(bound),
              "game_started": False, "sdk_session_started": False, "gun_joint_prerequisites": False, "stow_return_attempted": False,
              "final_world_bone_restore": {"proved": False, "reason": "No same-lease/final actual restored-world-bone getter contract"}}
    save(out / "prep.json", result)
    return result


def run(bound, binding_path, prepared_sha):
    # require_bindings has already run before this function can import helpers.
    out = evidence_path(bound["run_directory"])
    need(prepared_sha and digest(out / "prep.json") == prepared_sha, "Exact separately reviewed preparation SHA required")
    prep = load(out / "prep.json")
    need(prep["runner_sha256"] == digest(__file__) and prep["binding_sha256"] == digest(binding_path), "Runner/binding changed after preparation")
    for path, sha in prep["dependencies"].items():
        need(digest(path) == sha, "Prepared exact dependency changed")
    for row in prep["artifact_copies"]:
        need(digest(row["path"]) == row["sha256"], "Prepared private artifact changed")
    need(digest(out / "artifacts/simulator/openxr_simulator.json") == prep["runtime_manifest_sha256"] and
         digest(out / "private-manifest.json") == prep["private_manifest_sha256"], "Prepared private manifest changed")
    need(not (out / "run-started.json").exists() and not (out / "receipt.json").exists(), "One-shot run already entered")
    helpers = reusable_helpers(bound, out, runtime=True)
    ax, rt = helpers.ax, helpers.rt
    import ctypes
    import subprocess
    import threading
    idle = ax.inspect_idle()
    need(idle["idle"], "Another game/runtime worker is active; no launch/intervention")
    control = rt.WindowsControl()
    owner = control.identity(os.getpid())
    need(owner, "Exact runner process identity unavailable")
    save(out / "run-started.json", {"owner": owner, "prep_sha256": prepared_sha, "scope": SCOPE, "one_shot": True}, exclusive=True)
    r = {"schema": 1, "scope": SCOPE, "passed": False, "whole_session_passed": False,
         "full_stow_return_acceptance": "not_attempted", "large_reach_scope_acceptance": "not_attempted",
         "gun_joint_prerequisites": False, "source_profile": bound["source_profile"], "binding_sha256": digest(binding_path),
         "runner_sha256": digest(__file__), "limits": bound["limits"], "cleanup_order": [], "owner": owner, "game_started": False,
         "physical_background_input": False, "final_world_bone_restore": prep["final_world_bone_restore"],
         "immediate_written_matrix_requires_reevaluation": False, "immediate_written_proof": "typed control/tail bytes; actual motion uses later-Tick pre_restore matrices",
         "write_instant_source_ns_age_proved_by_host": False}
    identity = guard = bridge = readbacks = launch = thread = game_handle = baseline = budget = log = native = birth = None
    original_xr_environment = None
    old_environment = {key: os.environ.get(key) for key in (*bound["test_environment"], "XR_RUNTIME_JSON", "PYTHONPATH")}
    cancel = threading.Event()
    session_paths, session_result = [], {}
    trial_start_attempted = normal_close = False
    rig = bound["rig"]
    aliases = {"PlaygroundHandIkStreamTrialStop": rig["stop_export"], "PlaygroundHandIkStreamTrialStatus": rig["status_export"]}

    def persist():
        save(out / "receipt.json", r)

    def export(b, name, arg=None):
        return ax.export(b, aliases.get(name, name), arg)

    def adopt_birth(row, *, late=False):
        nonlocal identity, guard, game_handle, log
        born = checked_reservation_birth(row, bound["game_path"])
        if identity is not None:
            need(identity == born, "Reservation changed its already journaled identity")
            return
        current = control.identity(born["pid"])
        if current != born:
            need(current is None and control.game_absent(born["pid"]), "Exact born game identity changed/unknown")
            identity = born
            r.update(game_identity=identity, game_started=True, birth_at_guard_start=row, late_birth_already_absent=True)
            return
        # Journal the exact identity before handle/Guard operations can fail.
        # An OpenProcess/Guard exception must still reach exact-game cleanup.
        identity = born
        r.update(game_identity=identity, game_started=True, birth_at_guard_start=row)
        if late:
            r["late_birth_adopted_during_cleanup"] = True
        game_handle = control.k.OpenProcess(0x1000, False, identity["pid"])
        need(game_handle and control.identity(identity["pid"]) == identity, "Exact retained process handle unavailable")
        guard = rt.Guard(identity, owner, out / "audio.jsonl", helper=Path(bound["artifacts"]["audio_guard"]["path"]))
        log = Path(bound["artifacts"]["pg"]["path"]).parent / f'playground-{identity["pid"]}.log'

    def read_birth():
        return load(birth) if birth is not None and birth.exists() else None

    try:
        instance_row = bound["helpers"]["instance"]
        instance = import_pinned("independent_private_instance", instance_row["path"], instance_row["sha256"])
        selected, proof = instance.prepare(out.name, out / "private-manifest.json", Path(bound["artifacts"]["source_config"]["path"]), background_test=True)
        r["private_root"], r["prelaunch_ini"] = str(selected), ax.snapshot_ini(selected)
        r["test_render_policy"] = proof["test_render_policy"]
        env = {**bound["test_environment"], "GEARSVR_INSTANCE_ID": out.name, "GEARSVR_INSTANCE_ROOT": str(selected),
               "GEARSVR_INSTANCE_ORIGINAL_LOCALAPPDATA": proof["original_localappdata"], "GEARSVR_INSTANCE_GAME_LOGS": proof["game_logs"],
               "GEARSVR_QUIET_STARTUP_BUILD": proof["quiet_build"], "XR_RUNTIME_JSON": str(out / "artifacts/simulator/openxr_simulator.json")}
        old_environment.update({key: os.environ.get(key) for key in env if key not in old_environment})
        os.environ.update(env)
        os.environ["PYTHONPATH"] = str(Path(bound["retail_root"]) / "tools/package-inspector/python-libs") + os.pathsep + os.environ.get("PYTHONPATH", "")
        rt.verify_behind(Path(proof["quiet_build"]))
        need(ax.inspect_idle()["idle"], "Game/runtime worker arrived before the only spawn")
        birth, native = out / "reservation-birth.json", selected / "native"
        need(not birth.exists(), "One-shot reservation birth already exists; no spawn")
        pg = Path(bound["artifacts"]["pg"]["path"])
        command = [sys.executable, str(WORK / "tools/native-capture/launch_reserve.py"), "--game-path", proof["game_path"],
                   "--build-dir", str(native), "--dll-name", "GearsCapture.dll", "--prime-targets", "--graphics-startup-lock", "--graphics-menu-lock",
                   "--check-mask-control", "--check-frame-link", "--prime-owner-pid", str(owner["pid"]), "--prime-owner-creation-time", str(owner["creation_time"]),
                   "--launch-receipt", str(birth), "--playground-build", bound["early_pg_build_relative"], "--playground-dll", pg.name,
                   "--playground-sha256", bound["artifacts"]["pg"]["sha256"]]
        with (out / "reserve.stdout.txt").open("xb") as stdout, (out / "reserve.stderr.txt").open("xb") as stderr:
            launch = subprocess.Popen(command, stdout=stdout, stderr=stderr, creationflags=subprocess.CREATE_NO_WINDOW)
        until = time.monotonic() + 120
        while time.monotonic() < until:
            born = read_birth()
            if born and born.get("game_identity"):
                adopt_birth(born)
                break
            need(launch.poll() is None, "Reservation exited before birth; no retry")
            time.sleep(.025)
        need(identity is not None, "Exact owned game birth timed out")
        until = time.monotonic() + 100
        while launch.poll() is None and time.monotonic() < until:
            if rt.base.prior.decline_owned_private_settings_prompt(identity["pid"]):
                r["private_settings_restore_declined"] = True
            time.sleep(.08)
        need(launch.poll() == 0, "Reservation failed/timed out; no relaunch")
        r["playground_early_reservation"] = ax.retain_early_reservation(identity, r)
        save(out / "owned.json", {"game": identity, "owner": owner, "private_root": str(selected), "background_input_requested": False})
        def registry_ready():
            need(control.identity(identity["pid"]) == identity, "Exact owned process changed before attach")
            if rt.base.prior.decline_owned_private_settings_prompt(identity["pid"]):
                r["private_settings_restore_declined"] = True
            try:
                return rt.base.prior.verify_preload_call_function_entry(identity, WORK, r)
            except RuntimeError as error:
                if str(error) not in ("Invalid name array header", "Invalid object array header"):
                    raise
                return False
        rt.base.prior.wait_for(registry_ready, 90, "Original registry/entry unavailable before attach")
        # The early-reserved exact module is already present. attach_only never
        # injects/initializes via ordinary Bridge.export or remote_call.
        bridge = helpers.bridge_adapter.attach_only(rt.Bridge, identity, pg,
                    expected_sha256=bound["artifacts"]["pg"]["sha256"], allowed_exports=ax.ALLOW,
                    allowed_functions=READ_ONLY_FUNCTIONS,
                    on_retire=lambda value: r.setdefault("retained_remote_retirements", []).append(value))
        # Ordinary base.attach hid Initialize in its constructor. Submit it once
        # through the retained adapter instead; an unknown result is not retried.
        r["playground_initialize"] = export(bridge, "PlaygroundInitialize")
        need(r["playground_initialize"] == 0, "Exact reserved PG Initialize rejected")
        def enable():
            if rt.base.prior.decline_owned_private_settings_prompt(identity["pid"]):
                r["private_settings_restore_declined"] = True
            return export(bridge, "PlaygroundEnable") == 0
        rt.base.prior.wait_for(enable, 60, "Owned PG enable unavailable")
        need(export(bridge, "PlaygroundTestEnter") == 0, "Arena rejected")
        def arena_ready():
            value = ax.maybe(pg.parent / f'playground-status-{identity["pid"]}.json')
            need(not value or not value.get("failures"), "Arena reported failure")
            return value if value and value.get("ready") else False
        arena = rt.base.prior.wait_for(arena_ready, 120, "Owned arena unavailable")
        need(not export(bridge, "PlaygroundVrFireStatus") and not export(bridge, "PlaygroundVrAimStatus") and
             not export(bridge, "PlaygroundEquipmentStowStatus") & 0x10000, "R2/L2/stow must be clean/off")
        # No GiveTo/Lancer preparation, no component baseline/bind, no tap11/10,
        # no joint interval. Discover the current hand rig and retain its bytes.
        readbacks = rt.discovery.discover_readbacks(bridge, arena, r, ax.make_readbacks(rt, prep["dependencies"]))
        baseline = rt.base.phase(readbacks, r, "baseline", True, False)
        baseline["controls"] = readbacks.controls_full()
        baseline["control_identity_headers"] = readbacks.last_control_headers
        r["baseline"] = baseline
        need(ax.baseline_strengths_exact(baseline["controls"]), "Exact original/tail eight-control baseline unavailable")
        tail_baseline = {index: helpers.audit.tail_observation(baseline["controls"][index], baseline["control_identity_headers"][index], index) for index in (1, 3, 5, 7)}
        sim_data = out / "sim-appdata"
        (sim_data / "OpenXR-Simulator").mkdir(parents=True, exist_ok=True)
        original_xr_environment = rt.continuous.xr_environment
        rt.continuous.xr_environment = lambda parent: {**original_xr_environment(parent), "LOCALAPPDATA": str(sim_data), **bound["test_environment"]}
        opts = dict(bound["native_session_options"])
        args = SimpleNamespace(**{**opts, "pid": identity["pid"], "build_dir": native, "dll_name": "GearsCapture.dll",
                    "xr_build_dir": out / "artifacts/xr", "startup_timeout": 45., "cleanup_timeout": 35.,
                    "anchor_observer": True, "ui_observer": True, "crosshair_assignment_observer": True, "controller_input_observer": True,
                    "cinematic_camera": True, "first_person_preview": True, "weapon_calibration_path": out / "unused-armspan.json"})
        def session():
            try:
                session_result["return"] = rt.continuous.run_session(args, control=control, on_report=lambda p, *unused: session_paths.append(p), cancel_requested=cancel.is_set)
            except BaseException:
                session_result["error"] = traceback.format_exc()
        thread = threading.Thread(target=session, name="independent-owned-native-xr")
        thread.start()
        startup_deadline = time.monotonic() + 100
        native_started = xr_identity = None
        k = control.k
        k.GetTickCount64.restype = ctypes.c_ulonglonglong
        frequency = ctypes.c_longlong()
        need(k.QueryPerformanceFrequency(ctypes.byref(frequency)) and frequency.value > 0, "QPF unavailable")
        def live_native():
            nonlocal native_started, xr_identity
            need(thread.is_alive(), "Owned Native/XR session ended")
            report = ax.maybe(session_paths[0]) if session_paths else None
            if not report or report.get("phase") != "running":
                need(native_started is None, "Authenticated running report disappeared")
                return False
            actual = ax.trusted_native_started(report, identity, owner)
            need(native_started is None or native_started == actual, "Native incarnation changed")
            native_started = actual
            current_xr = owned_xr_identity(report, control, out / "artifacts/xr/OpenXRPoseProbe.exe")
            need(xr_identity is None or xr_identity == current_xr, "Owned XR incarnation changed")
            xr_identity = current_xr
            def qpc():
                ticks = ctypes.c_longlong()
                need(k.QueryPerformanceCounter(ctypes.byref(ticks)), "QPC unavailable")
                return ticks.value, frequency.value
            def read_status():
                need(thread.is_alive() and control.identity(identity["pid"]) == identity, "Exact session/game changed during freshness read")
                current = ax.maybe(session_paths[0])
                need(current and ax.trusted_native_started(current, identity, owner) == native_started, "Native report changed during read")
                need(not any(x.get("event") in ("guard_error", "guard_cleanup") for x in rt.audio_records(out / "audio.jsonl")), "Audio guard ended before game exit")
                with rt.continuous.open_session_status(native / "session-status.txt") as stream:
                    return stream.buffer.read()
            state = ax.record_native_freshness(rt.freshness, r, read=read_status, qpc=qpc, clock=time.monotonic, sleep=time.sleep,
                    cancel=cancel.is_set, remaining=lambda: ax.freshness_remaining(budget, startup_deadline), identity=identity, owner=owner, expected_started=native_started)
            need(control.identity(xr_identity["pid"]) == xr_identity, "Owned XR exited/changed during Native freshness read")
            return state if int(state.get("activation_ready_qpc", 0)) > 0 else False
        r["native_baseline"] = rt.base.prior.wait_for(live_native, max(.001, startup_deadline - time.monotonic()), "No authenticated fresh Native session")
        connect_typed_rig(bridge, rig, identity, native / "GearsCapture.dll", bound["artifacts"]["native"]["sha256"],
                          helpers.native_modules, live_native, native_started, startup_deadline, r)
        need(control.signal(f'Local\\GearsVR.FirstPersonToggle.{identity["pid"]}'), "Selected FPP toggle absent")
        tick = readbacks.scalar(readbacks.mesh, "TickTag", "IntProperty", "<i")
        until = time.monotonic() + 3
        while readbacks.scalar(readbacks.mesh, "TickTag", "IntProperty", "<i") == tick:
            live_native(); need(time.monotonic() < until, "No natural mesh Tick after FPP request"); time.sleep(.02)
        before = live_native()
        headers = []
        for obj in (readbacks.world, bridge.r.u64(readbacks.world + 0x40), readbacks.pc, readbacks.pawn, readbacks.mesh, readbacks.weapon):
            raw = bridge.r.read(obj, 0x58)
            need(bridge.r.read(obj, 0x58) == raw, "Full original object identity changed")
            headers.append((obj, raw))
        world_time = readbacks.scalar(readbacks.world, "TimeSeconds", "FloatProperty", "<f")
        after = live_native()
        need(before and after and before["started_qpc"] == after["started_qpc"], "Start native incarnation bracket changed")
        cursor = len(ax.lines(log))
        generation = positive(after, rig["native_world_generation_field"])
        raw_start = bytearray(ax.pack_start(identity, after, headers, world_time, int(k.GetTickCount64())))
        need(len(raw_start) == 304, "Actual Start304 helper contract changed")
        struct.pack_into("<Q", raw_start, 24, generation)  # Actual fresh status, not AX's fixed generation1.
        budget = Budget()
        bridge.set_deadline(budget.deadline)
        r["trial_start_clock"] = {"budget_began_monotonic": budget.began, "deadline_monotonic": budget.deadline}
        # Adapter journals owned bytes + the original remote thread before
        # dispatch. A timeout retains both; no Start replay/GT-based free.
        trial_start_attempted = r["trial_start_attempted"] = True
        r["trial_start"] = bridge.owned_export(rig["start_export"], bytes(raw_start))
        need(r["trial_start"] == 0, "Actual independent hand-only Start rejected")
        io = SimpleNamespace(live_native=live_native, status=lambda: export(bridge, rig["status_export"]),
                             log_text=lambda: ax.complete_log(log), command_path=sim_data / "OpenXR-Simulator/controller_pose_command.json")
        live_bound = {**bound, "_native_started_qpc": native_started, "_native_world_generation": generation, "_tail_baseline": tail_baseline,
                      "_game_identity": dict(identity), "_xr_identity": dict(xr_identity), "_rig_object_identity_hex": start_object_identities(headers),
                      "_qpc_frequency": frequency.value}
        r["native_source_binding"] = {"native_started_qpc": native_started, "world_generation": generation,
                                      "status_field": rig["native_world_generation_field"]}
        run_motion(io, budget, r, helpers, live_bound, cursor)
        budget.check()
        normal_close = True
        ax.cont_stop_and_shutdown(bridge, r, do_export=export, log_cursor=lambda: len(ax.lines(log)))
    except BaseException:
        r["error"] = traceback.format_exc()
    finally:
        def remote_pending():
            if bridge is None:
                return False
            try:
                bridge.poll_pending()
                return bridge.has_pending
            except BaseException:
                cleanup_observation_error(r, "retained_remote_observation_errors")
                return True

        def native_cleanup():
            r["cleanup_order"].append("native_cancel_and_drain"); cancel.set()
            if thread:
                thread.join(timeout=100); r["session_thread_joined"] = not thread.is_alive()
            r["session_result"] = session_result
            if session_paths:
                r["native_session_state"] = ax.maybe(session_paths[0])
            r["cleanup_order"].append("xr_exit")
            if thread and not thread.is_alive() and native is not None:
                try:
                    r["native_capture_after_stop"] = ax.capture_contract(ax.lines(native / "capture-status.txt"), True)
                except BaseException:
                    r["native_capture_error"] = traceback.format_exc()

        def prior_cleanup():
            nonlocal bridge
            try:
                if bridge:
                    # Clearing the deadline never restarts a pending request.
                    # It allows only ordinary cleanup once the original thread
                    # AND (for Submit) its GT result have actually retired.
                    bridge.set_deadline(None)
                    if remote_pending():
                        r["retained_remote_cleanup_pending"] = True
                        r["playground_cleanup_deferred_until_exact_game_exit"] = True
                        return
                    try:
                        if not normal_close:
                            ax.failure_stop_and_shutdown(bridge, r, trial_start_attempted, do_export=export)
                        if remote_pending():
                            r["retained_remote_cleanup_pending"] = True
                            r["playground_cleanup_deferred_until_exact_game_exit"] = True
                            return
                        r["cleanup_order"].append("playground_direct_restore_and_hooks")
                        if readbacks:
                            try:
                                r["after_shutdown_direct"] = readbacks.final_direct("after_shutdown_direct")
                                r["restored_controls8_exact"] = bool(baseline and ax.full_restore_matches(baseline, r["after_shutdown_direct"]))
                                r["restored_attachment44_exact"] = bool(baseline and rt.base.hand_pose(baseline) == rt.base.hand_pose(r["after_shutdown_direct"]))
                            except BaseException:
                                r["shutdown_direct_restore_error"] = traceback.format_exc()
                        for name, verify in (("call_function", rt.base.verify_call_function_entry), ("process_event", rt.base.verify_process_event_entry)):
                            try:
                                bridge.verify(); verify(bridge.r, r, "after_shutdown"); bridge.verify()
                            except BaseException:
                                r[name + "_restore_error"] = traceback.format_exc()
                    finally:
                        if not remote_pending():
                            try:
                                if bridge.close():
                                    bridge = None
                            except BaseException:
                                cleanup_observation_error(r, "retained_bridge_close_errors")
            finally:
                if bridge is not None:
                    # No module/session teardown while an export or Submit may
                    # still execute in PG/Native. Exact owned game exit is the
                    # fallback; retaining this journal is not a clean receipt.
                    r["native_cleanup_deferred_until_exact_game_exit"] = True
                else:
                    native_cleanup()
        def exact_cleanup():
            nonlocal bridge
            r["cleanup_order"].append("reservation_settle_before_exact_game")
            settle_reservation_before_exact_cleanup(launch, read_birth, lambda row: adopt_birth(row, late=True),
                    lambda: identity, r, persist=persist)
            if identity:
                ax.finish_game_then_guard(control, identity, guard, r, save=lambda value: save(out / "receipt.json", value), retained_handle=game_handle)
            if bridge is not None:
                # The adapter owns a handle bound to the original process
                # incarnation. Only its original-thread/process observation can
                # release the retained request, never r['game_absent'] or GT.
                while bridge is not None:
                    try:
                        bridge.poll_pending()
                        if bridge.close():
                            bridge = None
                            r["retained_remote_cleanup_pending"] = False
                            break
                    except BaseException:
                        cleanup_observation_error(r, "retained_bridge_post_game_close_errors")
                    try:
                        persist(); time.sleep(.1)
                    except BaseException:
                        cleanup_observation_error(r, "retained_bridge_post_game_wait_errors")
            if r.get("native_cleanup_deferred_until_exact_game_exit"):
                native_cleanup()
        ax.cleanup_before_exact_game(prior_cleanup, exact_cleanup, r)
        if game_handle and r.get("game_absent"):
            control.k.CloseHandle(game_handle)
        if original_xr_environment and (not thread or not thread.is_alive()):
            rt.continuous.xr_environment = original_xr_environment
        if not thread or not thread.is_alive():
            for key, previous in old_environment.items():
                if previous is None:
                    os.environ.pop(key, None)
                else:
                    os.environ[key] = previous
        else:
            r["environment_restore_pending"] = True
        if log:
            try:
                ax.retain_playground_log(log, r)
            except BaseException:
                r["log_copy_error"] = traceback.format_exc()
        native_state = r.get("native_session_state") or {}
        r["native_image_session_outcome"] = {"passed": native_state.get("passed"), "separate_scope": True}
        r["cleanup_passed"] = bool(r.get("hand_stop") == 0 and r.get("shutdown") == 0 and r.get("cont_terminal", {}).get("phase") == 6 and
            r.get("restored_controls8_exact") is True and r.get("restored_attachment44_exact") is True and
            r.get("call_function_entry", {}).get("after_shutdown", {}).get("matched") is True and r.get("process_event_entry", {}).get("after_shutdown", {}).get("matched") is True and
            r.get("session_thread_joined") is True and native_state.get("native_cleanup_verified") is True and native_state.get("entry_restore_verified") is True and
            native_state.get("xr_exit_code") == 0 and r.get("game_absent") is True and r.get("audio_restored", {}).get("restored") is True)
        r["passed"] = bool(r.get("independent_four_commands_complete") and r["cleanup_passed"] and not r.get("error") and
                           not any(value for key, value in r.items() if key.endswith("_error")))
        persist()
    return 0 if r["passed"] else 1


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binding", type=Path)
    parser.add_argument("--preflight", action="store_true")
    parser.add_argument("--prepare", action="store_true")
    parser.add_argument("--run", action="store_true")
    parser.add_argument("--prepared-sha256")
    parser.add_argument("--report", type=Path)
    parser.add_argument("--write-unbound-template", type=Path)
    args = parser.parse_args(argv)
    if args.write_unbound_template:
        save(evidence_path(args.write_unbound_template), binding_template(), exclusive=True)
        return 0
    report = {"schema": 1, "scope": SCOPE, "passed": False, "profile_bound": False, "project_helpers_imported": False,
              "prepare_entered": False, "game_started": False, "sdk_session_started": False, "runtime_started": False,
              "intentional_unbound": args.binding is None, "runner_sha256": digest(__file__)}
    try:
        need(sum((args.preflight, args.prepare, args.run)) <= 1, "Choose one operation")
        if args.binding is None:
            raise BindingError("UNBOUND typed Native RigV1/PG/XR/runtime profile; no imports, prepare or run")
        bound = require_bindings(args.binding)
        report["profile_bound"] = True
        if args.run:
            return run(bound, args.binding, args.prepared_sha256)
        if args.prepare:
            report["prepare_entered"] = True
            value = prepare(bound, args.binding)
            report.update(passed=True, prepared_sha256=digest(Path(bound["run_directory"]) / "prep.json"), project_helpers_imported=True, preparation=value)
        else:
            report.update(passed=True, command_plan=command_plan(bound), source_profile=bound["source_profile"])
    except (BindingError, FileNotFoundError, KeyError, TypeError, ValueError) as error:
        report.update(error={"type": type(error).__name__, "message": str(error)}, preflight_rejected=True)
    if args.report:
        save(evidence_path(args.report), report)
    print(json.dumps(report, indent=2, allow_nan=False))
    return 0 if report["passed"] else 2


if __name__ == "__main__":
    sys.exit(main())
