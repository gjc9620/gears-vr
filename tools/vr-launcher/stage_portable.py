"""Build a new, allowlisted offline candidate; never launch the game or overwrite a stage."""
import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import shutil
import stat
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
PROJECT_MODULES = {
    'vr-launcher': 'launcher menu_navigation stage_portable portable_preflight diagnostics_bundle game_audio_guard',
    'debug-launch': 'game_install background_launch quiet_launch configure_startup graphics_policy tiny_output_recovery instance_policy',
    'game-io': 'background_input game_io menu_state menu_control movie_state scene_owner_snapshot',
    'package-inspector': 'runtime_probe resolve_camera inspect_ui_state sample_player_anchor',
    'native-capture': ('anchor_config cinematic_anchor mounted_context mounted_read_pass crosshair_assignment_config hook_entry_attestation camera_closer_options '
        'launch_reserve menu_display_monitor prime_owner run_stereo_continuous run_stereo_probe startup_selftests '
        'session_telemetry start_capture ui_observer_config verify_stopped world_rebind world_rebind_monitor world_rebind_runtime '
        'path_choice_center_monitor path_choice_layout path_choice_trial_config '
        'path_choice_position_monitor path_choice_position_collect path_choice_position_config path_choice_group_fit '
        'path_choice_close_config path_choice_owned_sidecar'),
}
NATIVE_TESTS = ('CaptureSelfTest.exe', 'GraphicsMenuLockTest.exe', 'FrameLinkTest.exe',
                'GpuViewportStatsTest.exe', 'SplitSourceViewsTest.exe', 'SubmittedViewLifetimeTest.exe',
                'SourceViewShadowTest.exe','SourceViewArgsTest.exe','SourceEyeArgsTest.exe')
AUDIO_HELPER = 'tools/game-audio/GearsTestAudioGuard.exe'
AUDIO_PROVENANCE = 'tools/game-audio/audio-helper-provenance.json'
QUIET_BUILD = 'tools/quiet-startup/build-quiet11'
QUIET_HASHES = {
    'GearsEarlyLaunch.exe': '4eb27120bb8d1c5abf0d282a5280f65bf69d6f798edaa003e34673537bec0bf8',
    'GearsQuietStartup.dll': '58b81c8340d14c7083444e6031f050fc4d1d50a4481301fee76c8269f875ea5b',
}
PYTHON_FILES = ('python.exe', 'python3.dll', 'python312.dll', 'vcruntime140.dll', 'vcruntime140_1.dll', 'LICENSE.txt')
LIB_EXCLUDE = {'site-packages', '__pycache__', 'test', 'tests', 'idlelib', 'tkinter', 'turtledemo', 'ensurepip'}
LICENSES = {
    'tools/third-party/minhook/LICENSE.txt': 'licenses/MinHook.txt',
    'tools/third-party/openxr-loader/share/doc/openxr/LICENSE': 'licenses/OpenXR-Loader.txt',
    'tools/package-inspector/python-libs/pefile-2024.8.26.dist-info/LICENSE': 'licenses/pefile.txt',
}


def digest(path):
    value = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''): value.update(block)
    return value.hexdigest()


def no_links(path):
    path = Path(os.path.abspath(path))
    for item in (path, *path.parents):
        if not item.exists() and not item.is_symlink(): continue
        info = item.lstat()
        if stat.S_ISLNK(info.st_mode) or getattr(info, 'st_file_attributes', 0) & 0x400:
            raise ValueError('Symlink/reparse point refused: ' + str(item))
    return path


def contained(root, relative):
    relative = Path(relative)
    if relative.is_absolute() or '..' in relative.parts:
        raise ValueError('Expected a contained relative path: ' + str(relative))
    path = no_links(Path(root) / relative)
    if not path.resolve().is_relative_to(Path(root).resolve()):
        raise ValueError('Path escapes source root')
    return path


def payload_paths(manifest):
    native, xr = Path(manifest['native_build']), Path(manifest['xr_build'])
    for path in (native, xr):
        if path.is_absolute() or '..' in path.parts or path.parts[:1] != ('tools',):
            raise ValueError('Build must remain inside tools: ' + str(path))
    dll = manifest['native_dll']
    if Path(dll).name != dll or not dll.lower().endswith('.dll'): raise ValueError('Invalid DLL basename')
    result = ['GearsVR Launcher.exe', 'tools/debug-launch/Start-DebugGame.ps1',
              'tools/package-inspector/reloaded-profile.json']
    result += [f'tools/{folder}/{name}.py' for folder, names in PROJECT_MODULES.items() for name in names.split()]
    result += [(native / name).as_posix() for name in (dll, *NATIVE_TESTS)]
    result += [(xr / name).as_posix() for name in ('OpenXRPoseProbe.exe', 'openxr_loader.dll')]
    result += [f'{QUIET_BUILD}/{name}' for name in QUIET_HASHES]
    result += [AUDIO_HELPER, AUDIO_PROVENANCE]
    result += ['tools/package-inspector/python-libs/pefile.py', 'tools/package-inspector/python-libs/peutils.py']
    result += [f'tools/package-inspector/python-libs/ordlookup/{name}.py' for name in ('__init__', 'oleaut32', 'ws2_32', 'wsock32')]
    return result


def python_sources(python_root):
    result = list(PYTHON_FILES)
    for folder in ('Lib', 'DLLs'):
        for directory, dirs, files in os.walk(contained(python_root, folder), followlinks=False):
            # Reject reparse entries even if their name would otherwise be excluded.
            for name in (*dirs, *files): no_links(Path(directory) / name)
            dirs[:] = sorted(name for name in dirs if name.casefold() not in LIB_EXCLUDE)
            for name in sorted(files):
                relative = (Path(directory) / name).relative_to(python_root)
                if folder == 'Lib' and relative.suffix.lower() != '.py': continue
                if folder == 'DLLs' and (relative.suffix.lower() not in ('.dll', '.pyd') or
                        name.lower().startswith(('_tkinter', 'tcl', 'tk'))): continue
                result.append(relative.as_posix())
    return sorted(set(result))


def stage(root, python_root, destination, *, manifest_path=None, launcher_path=None, release_version=None):
    root, python_root = no_links(root).resolve(), no_links(python_root).resolve()
    destination = no_links(destination)
    if destination.exists(): raise FileExistsError('A stage must be a new directory: ' + str(destination))
    for source in (root, python_root):
        if source == destination.resolve() or source.is_relative_to(destination.resolve()):
            raise ValueError('Destination cannot be a source root or its ancestor')
    selected_manifest = (contained(root, 'tools/vr-launcher/manifest.json') if manifest_path is None
                         else no_links(manifest_path))
    manifest = json.loads(selected_manifest.read_text())
    release_documents = {}
    if release_version is not None:
        if release_version != '0.0.1': raise ValueError('Unsupported release version')
        for name in ('README.txt', 'RELEASE_NOTES.txt'):
            release_documents[name] = contained(root, 'docs/releases/' + release_version + '/' + name).read_text(encoding='utf-8')
    candidate_launcher = no_links(launcher_path) if launcher_path is not None else None
    if candidate_launcher is not None and candidate_launcher.suffix.lower() != '.exe':
        raise ValueError('Expected a compiled candidate launcher EXE')
    copies = [(candidate_launcher if path == 'GearsVR Launcher.exe' and candidate_launcher is not None else contained(root, path),
               path, 'candidate-gui:' + candidate_launcher.name if path == 'GearsVR Launcher.exe' and candidate_launcher is not None else 'repository:' + path,
               'project-code-or-built-artifact') for path in payload_paths(manifest)]
    copies += [(contained(root, source), target, 'repository:' + source, 'third-party-license')
               for source, target in LICENSES.items()]
    copies += [(contained(python_root, path), 'runtime/python/' + path, 'python-root:' + path,
                'CPython-runtime-and-bundled-VC-runtime') for path in python_sources(python_root)]
    for source, _, _, _ in copies:
        if not source.is_file(): raise FileNotFoundError(source)
    for key, relative in (('native_sha256', Path(manifest['native_build']) / manifest['native_dll']),
                          ('xr_sha256', Path(manifest['xr_build']) / 'OpenXRPoseProbe.exe')):
        if digest(contained(root, relative)) != manifest[key]: raise ValueError('Manifest artifact hash mismatch: ' + str(relative))
    for name, expected in QUIET_HASHES.items():
        if digest(contained(root, f'{QUIET_BUILD}/{name}')) != expected: raise ValueError('Quiet11 hash mismatch')
    from game_audio_guard import verify_manual_helper
    verify_manual_helper(contained(root, AUDIO_HELPER))
    destination.mkdir(parents=True)
    inventory = []
    def record(target, source, category):
        inventory.append(dict(path=target.relative_to(destination).as_posix(), size=target.stat().st_size,
                              sha256=digest(target), source=source, license_category=category))
    for source, relative, provenance, category in copies:
        target = contained(destination, relative); target.parent.mkdir(parents=True, exist_ok=True)
        expected = digest(source); shutil.copyfile(source, target)
        if digest(source) != expected or digest(target) != expected: raise RuntimeError('Source changed during copy: ' + str(source))
        record(target, provenance, category)
    manifest['game_path'] = 'SELECT_GAME_EXECUTABLE'
    alias = ('tools/vr-launcher/' + selected_manifest.name
             if re.fullmatch(r'manifest-[A-Za-z0-9_-]+\.json', selected_manifest.name) else None)
    config = {'game_path': '', 'stereo_units_per_metre': 100, 'launcher_language': 'en'}
    if manifest.get('runtime_render_size') is True: config['render_scale'] = 1.0
    generated = {
        'tools/vr-launcher/manifest.json': manifest,
        'tools/vr-launcher/config.json': config,
        'portable-candidate.json': {'schema': 1, 'kind': 'offline-staging-candidate', 'release_ready': False,
            'candidate_manifest': alias, 'source_manifest_name': selected_manifest.name,
            'source_manifest_sha256': digest(selected_manifest),
            'game_selection_required': True, 'simulator_bundled': False,
            'runtime_choice': 'The GUI defaults to System OpenXR runtime; Quest 3 / Virtual Desktop (VDXR) is also selectable. Simulator files are not bundled.',
            'limits': ['No game/headset acceptance performed', 'Redistribution/license review remains required',
                      'GUI developer Python fallbacks still exist outside strict preflight',
                      'Optional game_io capture/controller diagnostics are not bundled dependencies']},
    }
    if release_version is not None:
        metadata = generated['portable-candidate.json']
        metadata.update(kind='public-test-release', version=release_version,
            user_reported_acceptance=['Act 1 completed', 'KRVR + Apple Vision Pro playable',
                                      'Virtual Desktop + Quest 3 playable', 'Reticle issue resolved'],
            limits=['Later chapters not fully verified', 'Mounted machine-gun transitions may pause for about 2 seconds',
                    'DLSS / DLAA disabled in the normal launcher',
                    'Offline dependency preflight alone does not certify game/headset behavior'])
        # Public-test acceptance is recorded separately from the offline stager.
        metadata.pop('release_ready', None)
    if alias is not None: generated[alias] = manifest
    for relative, value in generated.items():
        target = contained(destination, relative); target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')
        record(target, 'generated:' + relative, 'project-generated-config')
    readme = contained(destination, 'README.txt')
    instructions = '''GearsVR — 开发候选 / Development candidate

中文
1. 解压整个文件夹，不要只移动启动器 EXE。
2. 关闭已运行的《战争机器》，打开 GearsVR Launcher.exe，选择游戏的 GOWDE-Steam.exe。
3. Quest 3 / Virtual Desktop 用户先连接 VD，选择 Quest 3 / Virtual Desktop (VDXR)，再点击启动 VR。
   其他头显可选择系统 OpenXR Runtime；此包不含模拟器。
4. 分辨率在游戏内设置；DLSS / DLAA 保持关闭。手柄在游戏前台正常响应，默认不响应后台输入。
   VR 场景暂时关闭景深模糊。
5. 界面右上角可切换中文 / English。停止时点击“停止 VR”，等待结束后再退出游戏。
   若有“镜头拉近”选项，可设0–0.5米，下次启动生效；0为原距离。建议先从0.3米试起。

这是待验证的开发候选，不是正式发布版。不包含游戏，请使用受支持的游戏版本。

English
1. Extract the whole folder; keep the launcher EXE with the other files.
2. Close any running Gears game. Open GearsVR Launcher.exe and select the game's GOWDE-Steam.exe.
3. For Quest 3 / Virtual Desktop, connect VD first, select Quest 3 / Virtual Desktop (VDXR), then Start VR.
   Other headsets can use System OpenXR Runtime. Simulator files are not included.
4. Set resolution in the game. DLSS / DLAA remain disabled. Controller input works with the game in the foreground;
   background controller input is disabled by default.
   Depth-of-field blur is currently disabled in VR scenes.
5. Choose 中文 / English at the upper right. Click Stop VR and wait for it to finish before exiting the game.
   If Camera closer is available, choose 0–0.5 metres for the next launch; 0 keeps the original distance. Try 0.3 first.

Feedback / 问题反馈:
Click Export logs in the launcher, save the ZIP, and attach it with your issue report.
Use it while the game is running or after exiting. It does not upload anything.
Includes recent session state, sampled performance rates, graphics settings and CPU/GPU/driver/Windows details.
Missing rates are unavailable, not zero FPS; XR loop rate is not headset display FPS.
启动器点击“导出日志”，保存 ZIP 后随问题描述发送。运行中和退出后均可导出，不自动上传。
不包含游戏文件、存档、截图和逆向资料；账号和个人路径会过滤。

This is an unvalidated development candidate, not a public release. Game files are not included; a supported game build is required.
'''
    if manifest.get('runtime_render_size') is True:
        instructions = instructions.replace('分辨率在游戏内设置；',
            'VR 每眼分辨率使用 Runtime 推荐值 × 启动器渲染倍率，默认100%；“检查眼图尺寸”可预先查看。\n   游戏分辨率只影响桌面镜像；倍率改动在下次启动生效。')
        instructions = instructions.replace('Set resolution in the game.',
            'VR eye size uses the runtime recommendation × launcher render scale (100% by default).\n   Use Check eye size before launch. Game resolution controls the desktop mirror; scale changes apply next launch.')
    readme.write_text(release_documents.get('README.txt', instructions), encoding='utf-8')
    record(readme, 'generated:README.txt', 'project-user-instructions')
    for relative, content in release_documents.items():
        if relative == 'README.txt': continue
        target = contained(destination, relative)
        target.write_text(content, encoding='utf-8')
        record(target, 'repository:docs/releases/' + release_version + '/' + relative, 'project-user-instructions')
    for relative in ('analysis/runtime', 'analysis/vr-launcher-ui'): contained(destination, relative).mkdir(parents=True, exist_ok=True)
    # A file cannot contain its own digest. All payload files are listed; the
    # inventory's own hash belongs in the separate preflight evidence receipt.
    report = dict(schema=1, files=sorted(inventory, key=lambda row: row['path']),
                  inventory_self_excluded=True, bytes=sum(row['size'] for row in inventory))
    (destination / 'inventory.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    return report


def run_preflight(destination, evidence):
    destination = Path(destination).resolve(); evidence = Path(evidence).resolve()
    env = dict(os.environ)
    for key in ('PYTHONHOME', 'PYTHONPATH', 'GEARSVR_PYTHON', 'GEARSVR_QUIET_STARTUP_BUILD'): env.pop(key, None)
    command = [str(destination / 'runtime/python/python.exe'), '-I', '-B',
               str(destination / 'tools/vr-launcher/portable_preflight.py'), '--output', str(evidence)]
    result = subprocess.run(command, cwd=destination, env=env, capture_output=True, text=True,
                            creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0), timeout=90)
    if result.returncode: raise RuntimeError('Isolated preflight failed: ' + result.stderr[-2500:])
    return json.loads(result.stdout)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--python-root', type=Path, required=True)
    parser.add_argument('--destination', type=Path, required=True)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--manifest', type=Path, help='Stage an explicit candidate without changing the local default')
    parser.add_argument('--launcher', type=Path, help='Use an independently compiled GUI without replacing the player EXE')
    parser.add_argument('--release-version', choices=('0.0.1',), help='Include versioned public-test instructions')
    parser.add_argument('--preflight-output', type=Path, required=True)
    args = parser.parse_args()
    report = stage(args.root, args.python_root, args.destination, manifest_path=args.manifest, launcher_path=args.launcher,
                   release_version=args.release_version)
    preflight = run_preflight(args.destination, args.preflight_output)
    print(json.dumps({'destination': str(args.destination.resolve()), 'files': len(report['files']),
                      'payload_bytes': report['bytes'], 'preflight': preflight}))


if __name__ == '__main__': main()
