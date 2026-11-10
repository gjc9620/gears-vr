"""Source-only discovery regression and baseline comparison; no process opened."""
import hashlib
import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[3]
OUT = pathlib.Path(__file__).resolve().parent
NATIVE = ROOT / 'tools/native-capture'
INSPECTOR = ROOT / 'tools/package-inspector'

def run(label, code):
    result = subprocess.run([sys.executable, '-E', '-s', '-B', '-X', 'utf8', '-c',
        'import sys;sys.path[:0]=' + repr([str(NATIVE), str(INSPECTOR)]) + '\n' + code],
        cwd=ROOT, text=True, encoding='utf-8', capture_output=True)
    (OUT / (label + '.log')).write_text(result.stdout + result.stderr, encoding='utf-8')
    return dict(exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr)

baseline = run('baseline-anchor', '''import subprocess,unittest,cinematic_anchor
source=subprocess.check_output(['git','show','HEAD:tools/native-capture/cinematic_anchor.py'],text=True,encoding='utf-8')
exec(compile(source,'HEAD:cinematic_anchor.py','exec'),cinematic_anchor.__dict__)
suite=unittest.defaultTestLoader.loadTestsFromName('test_anchor_config')
result=unittest.TextTestRunner(verbosity=1).run(suite)
sys.exit(not result.wasSuccessful())
''')
current = run('current-anchor', '''import unittest
suite=unittest.defaultTestLoader.loadTestsFromName('test_anchor_config')
result=unittest.TextTestRunner(verbosity=1).run(suite)
sys.exit(not result.wasSuccessful())
''')
focused = run('focused', '''import unittest
suite=unittest.defaultTestLoader.loadTestsFromNames([
 'test_cinematic_anchor','test_resolve_camera_class_hints','test_resolve_camera_discovery',
 'test_sample_player_anchor'])
result=unittest.TextTestRunner(verbosity=1).run(suite)
sys.exit(not result.wasSuccessful())
''')
counts = run('mapped-read-counts', '''import json
from test_resolve_camera_class_hints import fixture
r,_,calls,pointers,_=fixture()
legacy=[(a,r.identity(a)) for a in pointers if a and r.identity(a)['name']=='Canvas']
old=len(calls)
r,_,calls,pointers,_=fixture()
batched=[(a,h) for a,h in r.discovery_class_candidates(pointers) if h['name']=='Canvas']
assert legacy==batched
print(json.dumps(dict(objects=len(pointers),legacy_reads=old,class_hint_reads=len(calls),same_candidates=True)))
''')
receipt = dict(scope='Synthetic memory and source only; no game/XR/Native activation',
    baseline_anchor=baseline,current_anchor=current,focused=focused,mapped_read_counts=counts,
    files={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (
        NATIVE/'cinematic_anchor.py',NATIVE/'test_cinematic_anchor.py',INSPECTOR/'resolve_camera.py')},
    runtime_r140_cause='Unproven: no per-substep latency witness',
    runtime_r139_optimization='Still unexecuted in game; r140 rejected before activation')
(OUT/'verification.json').write_text(json.dumps(receipt,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({key:receipt[key]['exit_code'] for key in ('baseline_anchor','current_anchor','focused','mapped_read_counts')}))
print(counts['stdout'])
print(focused['stderr'])
assert focused['exit_code']==counts['exit_code']==0
assert baseline['exit_code']==current['exit_code']==1
for name in ('test_guard_attempts_exhaustion_not_retried_by_sample_layer',
             'test_sample_after_guard_exhaustion_keeps_actual_partial_sample'):
    assert name in baseline['stderr'] and name in current['stderr']
