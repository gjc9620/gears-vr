"""Export selected Lancer acquisition geometry, offline.

Does not install, select a live profile, import hand poses or change grab
retention. Output must be reviewed with the rest of the selected candidate.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import math
from pathlib import Path

RIG = "3b3c978cb7b6df3f3be48f60ac33daaf45dd8f30816a18a392fc906bdbc75193"
MODEL = "6704591397071207194"
VOLUME_KEYS = {"shape", "local", "half_extents_metres", "radius_metres",
               "half_length_metres", "snap_distance_metres", "snap_angle_radians",
               "release_distance_metres"}


def number(value):
    if isinstance(value, bool) or not isinstance(value, (float, int)) or not math.isfinite(value):
        raise ValueError("Expected finite number")
    return float(value)


def vector(value, count):
    if not isinstance(value, list) or len(value) != count:
        raise ValueError("Invalid vector length")
    return [number(x) for x in value]


def project_data(project: Path, expected_sha256: str):
    raw = project.read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    if digest != expected_sha256:
        raise ValueError("Project changed or wrong selected SHA256")
    data = json.loads(raw)
    expected = dict(schema=1, kind="quest_weapon_workbench", model="Lancer.glb",
                    model_content_fnv64=MODEL, rig_signature=RIG, units="metres",
                    coordinate_system="right_handed_x_right_y_up_z_back")
    for key, value in expected.items():
        if type(data.get(key)) is not type(value) or data.get(key) != value:
            raise ValueError(f"Unsupported project identity/coordinates: {key}")
    parts = data.get("parts")
    if not isinstance(parts, list) or [p.get("id") for p in parts] != [
        "main_grip", "support", "magazine", "charging", "starter"
    ]:
        raise ValueError("Unexpected or duplicate part identity/order")
    return data, digest


def volume_literal(v):
    if set(v) != VOLUME_KEYS or v["shape"] not in ("sphere", "box", "capsule"):
        raise ValueError("Unsupported volume shape or fields")
    local = v["local"]
    if set(local) != {"position_metres", "rotation_xyzw"}:
        raise ValueError("Unsupported local pose fields")
    p, q = vector(local["position_metres"], 3), vector(local["rotation_xyzw"], 4)
    if abs(sum(x*x for x in q)-1) >= .01:
        raise ValueError("Invalid volume orientation")
    half = vector(v["half_extents_metres"], 3)
    radius, length = number(v["radius_metres"]), number(v["half_length_metres"])
    snap, angle, release = [number(v[k]) for k in
        ("snap_distance_metres", "snap_angle_radians", "release_distance_metres")]
    if min(*half, radius, length, snap, angle, release) < 0 or angle > math.pi:
        raise ValueError("Invalid negative dimensions or angle")
    if (v["shape"] == "box" and min(half) <= 0) or (v["shape"] != "box" and radius <= 0):
        raise ValueError("Empty acquisition volume")
    fmt = lambda xs: ",".join(format(x, ".17g") for x in xs)
    return "{gears_lancer_authored_volume::Shape::%s, {{%s},{%s}}, {%s}, %s}" % (
        v["shape"], fmt(p), fmt(q), fmt(half), fmt([radius, length, snap, angle, release]))


def export(project: Path, expected_sha256: str, settings: Path | None = None,
           expected_settings_sha256: str | None = None, charging_region: bool = False, starter_region: bool = False):
    data, digest = project_data(project, expected_sha256)
    magazine = volume_literal(data["parts"][2]["volume"])
    support = "{}"
    charging = volume_literal(data["parts"][3]["volume"]) if charging_region else "{}"
    starter = volume_literal(data["parts"][4]["volume"]) if starter_region else "{}"
    snap_enabled = False
    settings_report = None
    insertion_override = False
    insertion_enabled = True
    shoulder_enabled = False
    shoulder_adjust = [0., 0., 0.]
    shoulder_scale = 1.
    main_snap_override = False
    main_snap_seconds = 0.
    if settings is not None:
        raw = settings.read_bytes()
        settings_digest = hashlib.sha256(raw).hexdigest()
        if settings_digest != expected_settings_sha256:
            raise ValueError("Settings changed or wrong selected SHA256")
        config = json.loads(raw)
        known = {"schema", "grab_pose", "snap_enabled", "two_hand", "insertion_assist", "haptics",
                 "audio", "show_regions", "sight_guide", "snap_seconds", "preview_fire_interval",
                 "preview_recoil_degrees", "vibration", "audio_gain", "shoulder_adjust_metres",
                 "shoulder_scale", "shooting_feedback_source"}
        if not isinstance(config, dict) or set(config)-known or type(config.get("schema")) is not int or config["schema"] != 1:
            raise ValueError("Unsupported interaction settings schema/fields")
        if type(config.get("snap_enabled")) is not bool:
            raise ValueError("Missing/invalid explicit snap_enabled")
        snap_enabled = config["snap_enabled"]
        if "snap_seconds" in config:
            main_snap_seconds = number(config["snap_seconds"])
            if not 0. <= main_snap_seconds <= .5:
                raise ValueError("Invalid main snap_seconds range")
            main_snap_override = True
        if "insertion_assist" in config:
            if type(config["insertion_assist"]) is not bool:
                raise ValueError("Invalid explicit insertion_assist")
            insertion_override = True
            insertion_enabled = snap_enabled and config["insertion_assist"]
        shoulder_keys = {"shoulder_adjust_metres", "shoulder_scale"}
        if set(config)&shoulder_keys and not shoulder_keys <= set(config):
            raise ValueError("Shoulder offset and scale must be selected together")
        if shoulder_keys <= set(config):
            shoulder_adjust = vector(config["shoulder_adjust_metres"], 3)
            shoulder_scale = number(config["shoulder_scale"])
            if math.hypot(*shoulder_adjust) > .5 or not .5 <= shoulder_scale <= 2.:
                raise ValueError("Invalid shoulder offset/scale")
            shoulder_enabled = True
        support = volume_literal(data["parts"][1]["volume"])
        consumed = {"snap_enabled"} | ({"insertion_assist"} if insertion_override else set()) | (shoulder_keys if shoulder_enabled else set()) | ({"snap_seconds"} if main_snap_override else set())
        settings_report = {"path": str(settings), "sha256": settings_digest,
            "consumed": sorted(consumed), "preserved_not_consumed": sorted(set(config)-{"schema"}-consumed)}
    elif expected_settings_sha256 is not None:
        raise ValueError("Settings hash supplied without settings file")
    support_enabled = settings is not None
    lines = ["#pragma once", '#include "weapon_lancer_authored_volume.h"',
        "// Offline selected acquisition profile, not a full settings/pose import.",
        "namespace gears_lancer_workbench_profile {",
        "inline constexpr bool magazine_acquisition_enabled=true;",
        "inline constexpr bool insertion_assist_override_enabled="+str(insertion_override).lower()+";",
        "inline constexpr bool insertion_assist_enabled="+str(insertion_enabled).lower()+";",
        "inline bool allow_insertion_assist() noexcept {return !insertion_assist_override_enabled||insertion_assist_enabled;}",
        "inline constexpr bool support_acquisition_enabled="+str(support_enabled).lower()+";",
        "inline constexpr bool support_snap_limits_enabled="+str(snap_enabled).lower()+";",
        "inline constexpr bool shoulder_supply_enabled="+str(shoulder_enabled).lower()+";",
        "inline constexpr bool charging_acquisition_enabled="+str(charging_region).lower()+";",
        "inline constexpr bool charging_snap_limits_enabled="+str(charging_region and snap_enabled).lower()+";",
        "inline constexpr bool starter_acquisition_enabled="+str(starter_region).lower()+";",
        "inline constexpr bool starter_snap_limits_enabled="+str(starter_region and snap_enabled).lower()+";",
        "inline constexpr bool main_snap_time_override_enabled="+str(main_snap_override).lower()+";",
        "inline constexpr bool main_snap_enabled="+str(main_snap_override and snap_enabled).lower()+";",
        "inline constexpr double main_snap_seconds="+format(main_snap_seconds,".17g")+";",
        "inline constexpr gears_vr::Vector shoulder_adjust{"+",".join(format(x,".17g") for x in shoulder_adjust)+"};",
        "inline constexpr double shoulder_scale="+format(shoulder_scale,".17g")+";",
        "inline constexpr gears_lancer_authored_volume::Volume magazine_acquisition"+magazine+";",
        "inline constexpr gears_lancer_authored_volume::Volume support_acquisition"+support+";",
        "inline constexpr gears_lancer_authored_volume::Volume charging_acquisition"+charging+";",
        "inline constexpr gears_lancer_authored_volume::Volume starter_acquisition"+starter+";",
        'inline constexpr char project_sha256[]="'+digest+'";',
        'inline constexpr char imported_fields[]="magazine acquisition'+('; support acquisition/passive snap' if support_enabled else '')+('; shoulder supply' if shoulder_enabled else '')+('; E acquisition' if charging_region else '')+('; C acquisition' if starter_region else '')+('; main snap time' if main_snap_override else '')+'";',
        "inline gears_lancer_authored_volume::Volume const* magazine() noexcept {return &magazine_acquisition;}",
        "inline gears_lancer_authored_volume::Volume const* support() noexcept {return support_acquisition_enabled?&support_acquisition:nullptr;}",
        "inline gears_lancer_authored_volume::Volume const* charging() noexcept {return charging_acquisition_enabled?&charging_acquisition:nullptr;}",
        "inline gears_lancer_authored_volume::Volume const* starter() noexcept {return starter_acquisition_enabled?&starter_acquisition:nullptr;}",
        "}"]
    return "\n".join(lines)+"\n", {"project_sha256": digest, "source": str(project),
        "consumer": "installed magazine acquisition"+(" and handguard acquisition/passive snap" if support_enabled else " only")+(" and shoulder supply" if shoulder_enabled else "")+(" and E acquisition" if charging_region else "")+(" and C acquisition" if starter_region else ""),
        "magazine_imported": ["shape", "local", "half_extents_metres", "radius_metres", "half_length_metres"],
        "magazine_preserved_not_consumed": ["snap_distance_metres", "snap_angle_radians", "release_distance_metres"],
        "support_imported": sorted(VOLUME_KEYS) if support_enabled else [],
        "settings": settings_report,
        "charging_imported": ["shape", "local", "half_extents_metres", "radius_metres", "half_length_metres"] if charging_region else [],
        "charging_snap_limits_imported": charging_region and snap_enabled,
        "charging_preserved_not_consumed": ["release_distance_metres"] if charging_region and snap_enabled else (["snap_distance_metres", "snap_angle_radians", "release_distance_metres"] if charging_region else []),
        "starter_imported": ["shape", "local", "half_extents_metres", "radius_metres", "half_length_metres"] if starter_region else [],
        "starter_snap_limits_imported": starter_region and snap_enabled,
        "starter_preserved_not_consumed": ["release_distance_metres"] if starter_region and snap_enabled else (["snap_distance_metres", "snap_angle_radians", "release_distance_metres"] if starter_region else []),
        "insertion_assist_setting_imported": insertion_override,
        "main_snap_time_override_imported": main_snap_override,
        "main_snap_seconds_imported": main_snap_override,
        "main_snap_enabled": main_snap_override and snap_enabled,
        "shoulder_supply_imported": shoulder_enabled,
        "shoulder_layout": "Workbench authored default, supply only; stable current game torso frame, global measured calibration unchanged" if shoulder_enabled else None,
        "not_imported": ["hand poses", "magwell insertion"]+([] if main_snap_override else ["snap_seconds"])+([] if starter_region else ["C moving-part volume"])+([] if charging_region else ["E moving-part volume"])+([] if shoulder_enabled else ["shoulder settings"]),
        "live_profile_selected": False, "game_acceptance": False}



if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", type=Path, required=True)
    parser.add_argument("--expect-sha256", required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--settings", type=Path)
    parser.add_argument("--expect-settings-sha256")
    parser.add_argument("--charging-region", action="store_true")
    parser.add_argument("--starter-region", action="store_true")
    args = parser.parse_args()
    header, report = export(args.project, args.expect_sha256, args.settings, args.expect_settings_sha256, args.charging_region, args.starter_region)
    if args.output.exists() or args.output.with_suffix(".review.json").exists():
        raise ValueError("Refusing to overwrite an existing selected profile")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(header, encoding="utf-8")
    args.output.with_suffix(".review.json").write_text(
        json.dumps(report, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
    print(json.dumps(report, ensure_ascii=False))
