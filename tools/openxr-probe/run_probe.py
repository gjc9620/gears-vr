"""Bounded subprocess probe; optional runtime override is process-local only."""
import argparse
import datetime
import json
import os
import pathlib
import subprocess
ROOT = pathlib.Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--runtime', type=pathlib.Path)
args = parser.parse_args()
env = os.environ.copy()
if args.runtime:
    if not args.runtime.is_file(): raise RuntimeError('Runtime manifest not found')
    env['XR_RUNTIME_JSON'] = str(args.runtime.resolve())
folder = ROOT / 'analysis/openxr'
folder.mkdir(parents=True, exist_ok=True)
try:
    result = subprocess.run([str(pathlib.Path(__file__).parent / 'build/OpenXRProbe.exe')],
                            env=env, capture_output=True, text=True, timeout=25)
    report = {'exit_code': result.returncode, 'runtime_override': env.get('XR_RUNTIME_JSON'),
              'probe': json.loads(result.stdout), 'stderr': result.stderr[-4000:]}
except subprocess.TimeoutExpired:
    report = {'error': 'Probe timed out after 25 seconds; child terminated',
              'runtime_override': env.get('XR_RUNTIME_JSON')}
name = datetime.datetime.now().strftime('%Y%m%d-%H%M%S') + '.json'
(folder / name).write_text(json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps(report, indent=2))
