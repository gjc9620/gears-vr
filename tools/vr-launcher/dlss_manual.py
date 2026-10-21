"""Prepare a separate, hash-checked DLSS manual headset test launcher."""
import argparse
import ctypes
from ctypes import wintypes
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import uuid

import launcher

ROOT=Path(__file__).resolve().parents[2]
TEMPLATE=ROOT/'tools/vr-launcher/manifest-dlss-manual1.json'

def digest(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def launch_environment(original,manifest):
    env={k:v for k,v in original.items()if not k.upper().startswith('GEARSVR_TEST_')}
    env.pop('GEARSVR_QUIET_STARTUP_BUILD',None)
    env.pop('XR_RUNTIME_JSON',None)
    env.update(GEARSVR_MANIFEST=str(manifest),GEARSVR_TEST_NGX_MANUAL_OWNED='1',
        GEARSVR_TEST_NGX_INPUT_UNION_MASK='1',GEARSVR_TEST_NGX_FINAL_OUTPUT_JOIN='1',
        GEARSVR_TEST_NGX_OWNED_SCENE='1')
    return env

def verify_artifacts(expected):
    if not expected:raise RuntimeError('Missing startup artifact receipt')
    for path,value in expected.items():
        if digest(path)!=value:raise RuntimeError('DLSS test artifact SHA256 mismatch: '+str(path))

def prepare_run(root,manifest,expected,run_id):
    """Each GUI gets a private copy; no prior event log is removed or reused."""
    verify_artifacts(expected)
    folder=root/'analysis/native-dlss-manual'/run_id
    folder.mkdir(parents=True,exist_ok=False)
    hashes={}
    for path,value in expected.items():
        destination=folder/Path(path).name
        if destination.name in hashes:raise RuntimeError('Duplicate startup artifact name')
        shutil.copy2(path,destination)
        if digest(destination)!=value:raise RuntimeError('Copied DLSS test artifact differs')
        hashes[destination.name]=value
    chosen=dict(manifest,native_build=folder.relative_to(root).as_posix())
    target=folder/'manifest.json';target.write_text(json.dumps(chosen,indent=2),encoding='utf-8')
    (folder/'launch-receipt.json').write_text(json.dumps(dict(manifest=str(target),artifacts=hashes,
        mode='user-operated VDXR headset test',game_launched_by_wrapper=False),indent=2),encoding='utf-8')
    return target

def verify_candidate(root=ROOT):
    manifest=json.loads((root/'tools/vr-launcher/manifest-dlss-manual1.json').read_text())
    if any(manifest.get(k) is not False for k in ('graphics_startup_lock','graphics_menu_lock','experimental_preserve_upscaler')):
        raise RuntimeError('Manual DLSS test requires unlocked menus and ordinary SR-off startup')
    config=json.loads((root/'tools/vr-launcher/config.json').read_text())
    game,_=launcher.settings(config,manifest,root)
    launcher.verify_files(manifest,game,root)
    proof=json.loads((root/manifest['manual_dlss_test_receipt']).read_text())
    if proof.get('passed')is not True or proof.get('manual_owned_test')is not True or proof.get('sha256')!=manifest['native_sha256']:
        raise RuntimeError('Manual DLSS build tests are not accepted')
    expected=proof['startup_artifacts'];verify_artifacts(expected)
    sys.path.insert(0,str(root/'tools/native-capture'))
    from startup_selftests import artifacts,continuous_suite
    dll=launcher.rooted(manifest['native_build'],root)/manifest['native_dll']
    if artifacts(dll,continuous_suite())!=expected:raise RuntimeError('Startup receipt suite or DLL path differs')
    gui=root/manifest['manual_dlss_gui']
    if not gui.is_file()or digest(gui)!=manifest['manual_dlss_gui_sha256']:
        raise RuntimeError('Manual-test GUI launcher is missing or has changed')
    runtime=Path(os.environ.get('ProgramFiles','C:/Program Files'))/'Virtual Desktop Streamer/OpenXR/virtualdesktop-openxr.json'
    spec=json.loads(runtime.read_text())
    library=(runtime.parent/spec['runtime']['library_path']).resolve()
    if not library.is_file():raise RuntimeError('VDXR runtime library is missing')
    return manifest,expected,gui,runtime

def require_no_session():
    backend=launcher.Backend()
    if backend.game_pids():raise RuntimeError('Exit the current game normally before opening the DLSS test launcher.')
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    kernel.OpenMutexW.argtypes=[wintypes.DWORD,wintypes.BOOL,wintypes.LPCWSTR]
    kernel.OpenMutexW.restype=wintypes.HANDLE
    kernel.CloseHandle.argtypes=[wintypes.HANDLE]
    handle=kernel.OpenMutexW(0x00100000,False,'Local\\GearsVR.RepositoryLauncher.Owner')
    if handle:
        kernel.CloseHandle(handle)
        raise RuntimeError('Wait for the existing VR launcher session to finish.')
    if ctypes.get_last_error()!=2:raise ctypes.WinError(ctypes.get_last_error())

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check',action='store_true',help='Verify files and startup receipts without opening the GUI or starting a game')
    args=parser.parse_args()
    manifest,expected,gui,runtime=verify_candidate()
    if args.check:
        print(json.dumps(dict(passed=True,dll=manifest['native_dll'],sha256=manifest['native_sha256'],
            runtime=str(runtime),startup_artifacts=len(expected),dlss_menu_unlocked=True,game_launched=False)))
        return 0
    require_no_session()
    chosen=prepare_run(ROOT,manifest,expected,uuid.uuid4().hex)
    env=launch_environment(os.environ,chosen)
    # The GUI defaults to Quest / Virtual Desktop and starts only on the user's
    # Start VR click, with the existing --foreground policy and normal audio.
    return subprocess.run([str(gui)],cwd=ROOT,env=env).returncode

if __name__=='__main__':
    try:raise SystemExit(main())
    except Exception as error:
        print('DLSS test launcher: '+str(error),file=sys.stderr)
        raise SystemExit(1)
