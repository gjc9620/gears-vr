"""Clone an inventoried local candidate; replace only explicitly built editor modules."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def package(base, destination, native, playground, launcher, simulator, xr=None):
    base, destination = base.resolve(), destination.resolve()
    if destination.exists():
        raise ValueError("Choose a fresh destination; existing candidates are preserved")
    original = json.loads((base/"inventory.json").read_text("utf-8-sig"))
    destination.mkdir(parents=True)
    provenance = {}
    for row in original["files"]:
        source = (base/row["path"]).resolve()
        target = (destination/row["path"]).resolve()
        if not source.is_relative_to(base) or not target.is_relative_to(destination):
            raise ValueError("Inventory path escaped its candidate")
        # Config may have legitimately changed since its original inventory.
        # Record current exact source bytes instead of attributing the old SHA.
        expected = sha(source)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
        if sha(source) != expected or sha(target) != expected:
            raise RuntimeError("Source changed while copying: "+str(source))
        provenance[row["path"]] = "cloned-current:"+str(source)
    shutil.copy2(launcher, destination/"GearsVR Launcher.exe")
    provenance["GearsVR Launcher.exe"] = "editor-launcher-fix:"+str(launcher)
    manifests = [destination/"tools/vr-launcher/manifest.json",
                 destination/"tools/vr-launcher/manifest-6dof-unified.json"]
    selected = json.loads(manifests[-1].read_text("utf-8-sig"))
    for source, build_key, dll_key, hash_key in ((native,"native_build","native_dll","native_sha256"),
                                                (playground,"playground_build","playground_dll","playground_sha256")):
        target = destination/selected[build_key]/selected[dll_key]
        if not target.resolve().is_relative_to(destination):
            raise ValueError("Runtime module target escaped package")
        shutil.copy2(source, target)
        selected[hash_key] = sha(source)
        provenance[target.relative_to(destination).as_posix()] = "editor-production:"+str(source)
    if xr is not None:
        target = destination/selected['xr_build']/'OpenXRPoseProbe.exe'
        shutil.copy2(xr, target)
        selected['xr_sha256'] = sha(xr)
        provenance[target.relative_to(destination).as_posix()] = 'integrated-current-xr:'+str(xr)
    # Match current compiled modules with their current lifecycle/input helpers.
    source_root = Path(__file__).resolve().parents[2]
    for folder in ('vr-launcher', 'native-capture', 'openxr-probe', 'playground',
                   'game-io', 'game-audio', 'debug-launch', 'package-inspector', 'weapon-models'):
        for source in (source_root/'tools'/folder).glob('*'):
            if source.is_file() and source.suffix in ('.py', '.ps1') and not source.name.startswith('test_'):
                target = destination/'tools'/folder/source.name
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(source, target)
                provenance[target.relative_to(destination).as_posix()] = 'integrated-current-helper:'+str(source)
    selected.update(candidate_id=destination.name,
                    candidate_validation="Live editor development candidate; same-game visual validation recorded separately")
    for target in manifests:
        target.write_text(json.dumps(selected, ensure_ascii=False, indent=2), "utf-8")
    config_path = destination/"tools/vr-launcher/config.json"
    config = json.loads(config_path.read_text("utf-8-sig"))
    config["game_path_history"] = [path for path in config.get("game_path_history", []) if "p2p" not in path.lower()]
    config["game_settings_paths"] = {path:settings for path,settings in config.get("game_settings_paths", {}).items() if "p2p" not in path.lower()}
    config.update(game_path="J:/SteamLibrary/steamapps/common/Gears of War Reloaded/Binaries_x64/GOWDE-Steam.exe",
                  render_scale=.3, lancer_interaction_ranges=False)
    config_path.write_text(json.dumps(config, ensure_ascii=False, indent=2), "utf-8")
    editor = destination/"editor"
    editor.mkdir(exist_ok=True)
    for name in ("pose_editor.py", "launch_live.py", "desktop_controls.py"):
        shutil.copy2(Path(__file__).with_name(name), editor/name)
    shutil.copy2(Path(__file__).with_name("Open Live Editor.cmd"), destination/"Open Live Editor.cmd")
    shutil.copy2(Path(__file__).with_name("Open Desktop Editor.cmd"), destination/"Open Desktop Editor.cmd")
    (destination/"LIVE-EDITOR.txt").write_text(
        "打开 Open Live Editor.cmd，会同时打开正常游戏启动器和实时编辑器。\n"
        "在启动器开始游戏，拿起 Lancer 后，用编辑器选择主握把／护木／弹匣／E／C。\n"
        "Grab Volume 显示实际当前吸附判定范围；Grab Pose 显示所选姿势的黄色骨架。\n"
        "拖动位置、旋转和关节滑块后，观察游戏中的变化。手模只在当前合法抓握时应用。\n"
        "主握把挂载调整会影响枪体和枪口；其他部件调整作用于现有手部皮肤。\n"
        "保存姿势可重复载入。恢复基线或关闭编辑器会发送全部恢复指令。\n"
        "本候选与公开 v71 分开；游戏内记录和头显验收状态见开发记录。\n", "utf-8")
    for name in ("portable-candidate.json", "candidate-state.json"):
        target = destination/name
        if target.exists():
            value = json.loads(target.read_text("utf-8-sig"))
            value.update(release_ready=False, candidate_id=destination.name)
            if name=="portable-candidate.json":
                value["limits"] = ["Live Grab editor development build; headset acceptance pending."]
            target.write_text(json.dumps(value, ensure_ascii=False, indent=2), "utf-8")
    # Bundle the existing local simulator, preserving relocation through a relative DLL path.
    sim = json.loads(simulator.read_text("utf-8-sig"))
    library = Path(sim["runtime"]["library_path"])
    if not library.is_absolute():
        library = simulator.parent/library
    if not library.is_file():
        raise FileNotFoundError("Simulator library missing: "+str(library))
    sim_dir = destination/"analysis/runtime/simulator-quiet2"
    sim_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(library, sim_dir/"openxr_simulator.dll")
    sim["runtime"]["library_path"] = "./openxr_simulator.dll"
    (sim_dir/"openxr_simulator.json").write_text(json.dumps(sim,indent=2),"utf-8")
    rows = []
    for file in sorted(destination.rglob("*")):
        if file.is_file() and file.name!="inventory.json":
            relative = file.relative_to(destination).as_posix()
            rows.append(dict(path=relative, size=file.stat().st_size, sha256=sha(file),
                             source=provenance.get(relative,"editor-generated-or-updated"),
                             license_category="local-development-candidate"))
    (destination/"inventory.json").write_text(json.dumps(dict(schema=1,inventory_self_excluded=True,bytes=sum(row["size"] for row in rows),files=rows),indent=2), "utf-8")
    return dict(destination=str(destination),files=len(rows),game_started=False,native_sha256=sha(native),playground_sha256=sha(playground))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("base", "destination", "native", "playground", "launcher"):
        parser.add_argument("--"+name,type=Path,required=True)
    parser.add_argument("--simulator",type=Path,default=Path("J:/gearsVr/analysis/runtime/simulator-quiet2/openxr_simulator.json"))
    parser.add_argument("--xr", type=Path)
    args = parser.parse_args()
    print(json.dumps(package(args.base,args.destination,args.native,args.playground,args.launcher,args.simulator,args.xr),ensure_ascii=False))
