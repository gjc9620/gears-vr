# Derived r80 observation; original __file__ binds original static inputs.
# Actual derived bytes and independent parent are bound by r96 ready/plan.
from pathlib import Path
import sys
__file__ = 'C:\\Users\\11\\.codex\\worktrees\\6dof-single\\gearsVr\\analysis\\solo-m1-20261028\\run_cover_eye_r80.py'
OUT_RUN = Path('C:\\Users\\11\\.codex\\worktrees\\6dof-single\\gearsVr\\analysis\\cover-system-20261108\\view-alignment-r97')
sys.path.insert(0, str(OUT_RUN))
sys.path.insert(0, 'C:\\Users\\11\\.codex\\worktrees\\6dof-single\\gearsVr\\tools\\game-playtest')
sys.path.insert(0, 'C:\\Users\\11\\.codex\\worktrees\\6dof-single\\gearsVr\\analysis\\solo-m1-20261028')
"""One frozen Cover attempt; --prepare-only never stages or consumes admission."""
import os
import sys

# This deliberately precedes argument, ticket, source and dependency validation.
# The parent is created offline. Never unlink/recreate a consumed attempt.
_CLAIM = 'C:\\Users\\11\\.codex\\worktrees\\6dof-single\\gearsVr\\analysis\\cover-system-20261108\\view-alignment-r97\\runtime\\claim.json'
_PREPARE = sys.argv[1:] == ['--prepare-only']
if not _PREPARE:
    _fd = os.open(_CLAIM, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o600)
    try:
        os.write(_fd, ('{"state":"consumed_unverified","pid":%d}\n' % os.getpid()).encode('ascii'))
        os.fsync(_fd)
    finally:
        os.close(_fd)
# END_EXCLUSIVE_BOOTSTRAP

if not (sys.flags.optimize == 0 and sys.flags.dont_write_bytecode and sys.flags.ignore_environment and sys.flags.no_user_site and sys.flags.utf8_mode and not sys.flags.isolated):
    raise RuntimeError('Exact -E -s -B -X utf8 without -O/-I required')

from pathlib import Path
import ast
import hashlib
import importlib
import json
import subprocess
import time

R = Path('C:\\Users\\11\\.codex\\worktrees\\6dof-single\\gearsVr')
O = R / 'analysis/solo-m1-20261028'
TARGET = O / 'equipment-controls-prepared-20261110-cover-eye-r80'
PLAN = O / 'cover-eye-r80-plan.json'
READY = TARGET / 'runtime-ready.json'
TICKET = Path('C:\\Users\\11\\.codex\\worktrees\\6dof-single\\gearsVr\\analysis\\cover-system-20261108\\view-alignment-r80\\final-window-ticket.json')
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()
assert Path.cwd().resolve() == R
assert sys.flags.optimize == 0 and sys.flags.dont_write_bytecode
assert sys.flags.ignore_environment and sys.flags.no_user_site and sys.flags.utf8_mode
assert _PREPARE or sys.argv[1:] == ['--run']
plan = json.loads(PLAN.read_text(encoding='utf-8'))
ready = json.loads(READY.read_text(encoding='utf-8'))
assert ready['plan_sha256'] == sha(PLAN)
assert ready['entrypoint_sha256'] == sha(__file__)
assert ready['prepared_sha256'] == plan['prepared_sha256'] == sha(TARGET / 'prepared.json')
assert Path(sys.executable).resolve() == Path(plan['invocation']['interpreter']).resolve()
assert sha(sys.executable) == plan['invocation']['interpreter_sha256']

deps_path = TARGET / 'runtime/dependencies.json'
assert sha(deps_path) == plan['dependencies']['sha256']
deps = json.loads(deps_path.read_text(encoding='utf-8'))
for path, digest in deps['files'].items():
    assert sha(path) == digest, ('Dependency changed', path)
for name, expected in deps['module_origins'].items():
    module = importlib.import_module(name)
    assert Path(module.__spec__.origin).resolve() == Path(expected['path']).resolve(), ('Wrong module origin', name)
    assert sha(module.__spec__.origin) == expected['sha256']
import PIL
import numpy
assert PIL.__version__ == deps['versions']['Pillow'] and numpy.__version__ == deps['versions']['numpy']

prepared = json.loads((TARGET/'prepared.json').read_text(encoding='utf-8'))
assert Path(prepared['source_archive']).resolve() == TARGET
assert prepared['runtime_driver_sha256'] == plan['runtime_driver_sha256']
for relative, digest in plan['runtime_driver_sha256'].items():
    current = (TARGET/'driver-source'/relative if relative == 'analysis/solo-m1-20261028/run_v63_startup_repro.py' else R/relative)
    archived = TARGET / 'driver-source' / relative
    assert sha(current) == sha(archived) == digest, ('Driver changed', relative)
    ast.parse(current.read_text(encoding='utf-8-sig'), filename=relative)
for relative, digest in prepared['source_sha256'].items():
    assert sha(Path(prepared['source_snapshot'])/relative) == digest
for key in ('native', 'playground', 'sim_xr', 'eye_inspector'):
    value = prepared['builds'][key]
    assert sha(value['saved']) == value['sha256'] == plan['build_sha256'][key]
for key in ('selected_game', 'selected_physx'):
    assert sha(plan[key]['path']) == plan[key]['sha256']
assert plan['operator_checks'] == ['--cover-eye-observation']

receipt = dict(prepared=TARGET.name, staged=False, started=False, physical_headset_accepted=False)
def register_capture_identity(game, stamp):
    assert game['pid'] > 0 and game['creation_time'] > 0
    assert Path(game['image']).resolve() == Path(plan['selected_game']['path']).resolve()
    assert len(stamp) == 15 and stamp[8] == '-' and stamp.replace('-', '').isdigit()
    if 'capture_identity' in receipt:
        assert receipt['capture_identity'] == game
        return
    folder = O / f'menu-firstperson-{game["pid"]}-interrupted-cover-{stamp}'
    folder.mkdir(exist_ok=False)
    with (folder/'result.json').open('x', encoding='utf-8') as stream:
        json.dump(dict(game=game, source=1, passed=False, checks={}, retention_identity_only=True,
            state='started_unverified; cover result and normal cleanup required',
            physical_headset_accepted=False, native_normal_cleanup_verified=False), stream, indent=2)
    receipt['capture_identity'] = dict(game)
    receipt['capture_identity_receipt'] = str(folder/'result.json')
    receipt_path.write_text(json.dumps(receipt, indent=2), encoding='utf-8')

s = (TARGET/'driver-source/analysis/solo-m1-20261028/run_v63_startup_repro.py').read_text(encoding='utf-8')
needle = "str(o/'run_menu_firstperson_check.py')"
assert s.count(needle) == 1
s = s.replace(needle, "'C:\\\\Users\\\\11\\\\.codex\\\\worktrees\\\\6dof-single\\\\gearsVr\\\\analysis\\\\cover-system-20261108\\\\view-alignment-r97\\\\run_cover_eye_approach_operator.py'")
s = s.replace("equipment_flags=['--equipment-chain','--lancer-physical-repair']", "equipment_flags=['--equipment-chain']")
needle = '                    operator=subprocess.Popen('
assert s.count(needle) == 1
s = s.replace(needle, '                    checks=' + repr(plan['operator_checks']) + '\n' + needle)
needle = '                    if paired_capture is None and not performance_only:'
assert s.count(needle) == 1
s = s.replace(needle, '                    _cover_register_capture_identity(game,stamp)\n' + needle)
basic = (O/'run_basic.py').read_text(encoding='utf-8')
assert basic.count('deadline=time.monotonic()+3600') == 1
basic = basic.replace('deadline=time.monotonic()+3600', 'deadline=time.monotonic()+180')
compile(basic, str(O/'run_basic.py'), 'exec')
needle = "runpy.run_path(str(o/'run_basic.py'),run_name='__main__')"
assert s.count(needle) == 1
s = s.replace(needle, 'exec(compile(' + repr(basic) + ', ' + repr(str(O/'run_basic.py')) + ", 'exec'), {'__file__': " + repr(str(O/'run_basic.py')) + ", '__name__': '__main__'})")
assert s.count('from retain_test_media import idle') == 1
s = s.replace('from retain_test_media import idle', 'from cover_observation_runtime import idle')
compile(s, str(O/'run_v63_startup_repro.py'), 'exec')
if _PREPARE:
    assert not Path(_CLAIM).exists()
    print(json.dumps(dict(prepared=TARGET.name, drivers=len(plan['runtime_driver_sha256']),
        dependency_files=len(deps['files']), origins=len(deps['module_origins']),
        staged=False, started=False, claim_consumed=False)))
    raise SystemExit(0)

# No mutation of shared state or child creation occurs above this line.
from cover_observation_runtime import idle
assert idle(), 'Another owner has a live game/XR/recorder; consumed attempt cannot start'
trace_source = R/'analysis/instances/solo-m1-1028d/native/authored-eye-geometry.bin'
assert not trace_source.exists(), 'Prior eye evidence must be archived by its owner; never overwrite it'

for key in tuple(os.environ):
    if key.startswith(('GEARSVR_', 'GEARS_', 'XR_')) or key.upper().startswith('PYTHON'):
        del os.environ[key]
os.environ.update(plan['controlled_outer_environment'])
receipt_path = OUT_RUN/'runtime/attempt-result.json'
assert not receipt_path.exists()
receipt.update(claim=_CLAIM, entry_policy='independent actual-start occupancy; no predecessor or FINAL',
               independent_entry=str(OUT_RUN/'run_cover_eye_independent.py'))
receipt_path.write_text(json.dumps(receipt, indent=2), encoding='utf-8')
with (OUT_RUN/'runtime/stage.log').open('xb') as stage_log:
    stage = subprocess.run([sys.executable, '-E', '-s', '-B', '-X', 'utf8',
        str(OUT_RUN/'stage.py'), TARGET.name], cwd=R, stdout=stage_log,
        stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW|subprocess.BELOW_NORMAL_PRIORITY_CLASS)
receipt['stage_exit_code'] = stage.returncode
receipt_path.write_text(json.dumps(receipt, indent=2), encoding='utf-8')
assert stage.returncode == 0, 'Partial stage requires baseline restoration; never retry this attempt'
receipt['staged'] = True
config = R/'analysis/instances/solo-m1-1028d/config.json'
before = config.read_bytes()
receipt['config_before_hex'] = before.hex()
receipt['restored'] = False
try:
    assert idle(), 'Another owner appeared after stage; no start'
    value = json.loads(before.decode('utf-8-sig'))
    value['game_path'] = plan['selected_game']['path']
    config.write_text(json.dumps(value, indent=2), encoding='utf-8')
    receipt['started'] = True
    receipt_path.write_text(json.dumps(receipt, indent=2), encoding='utf-8')
    exec(compile(s, str(O/'run_v63_startup_repro.py'), 'exec'),
         {'__file__': str(O/'run_v63_startup_repro.py'), '__name__': '__main__',
          '_cover_register_capture_identity': register_capture_identity})
finally:
    # Stop/restore is not conditional on successful geometry decoding. Preserve
    # raw evidence now; the independent parent restores the full baseline before
    # any optional numeric or image review.
    if idle():
        config.write_bytes(before)
        receipt['restored'] = config.read_bytes() == before
        if trace_source.exists():
            raw = trace_source.read_bytes()
            saved = OUT_RUN/'runtime/authored-eye-geometry.bin'
            with saved.open('xb') as stream:
                stream.write(raw)
                stream.flush()
                os.fsync(stream.fileno())
            assert sha(saved) == sha(trace_source)
            receipt['eye_geometry'] = dict(path=str(saved), sha256=sha(saved), bytes=len(raw),
                decoded=False, visual_accepted=False, physical_headset_accepted=False)
            receipt_path.write_text(json.dumps(receipt, indent=2), encoding='utf-8')
            trace_source.unlink()
    receipt['runtime_drivers_still_match'] = all(sha(TARGET/'driver-source'/name if name == 'analysis/solo-m1-20261028/run_v63_startup_repro.py' else R/name) == digest for name,digest in plan['runtime_driver_sha256'].items())
    receipt['complete_baseline_restoration_required'] = True
    receipt_path.write_text(json.dumps(receipt, indent=2), encoding='utf-8')
