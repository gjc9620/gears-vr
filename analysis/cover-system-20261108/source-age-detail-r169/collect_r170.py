"""Collect the new terminal timing run; never treat missing data as success."""
from pathlib import Path
import importlib.util,json,hashlib,shutil
H=Path(__file__).resolve().parent;R=H.parents[2]
root=R/'analysis/instances/cover-1111-r170-cloak-age';out=H/'runtime-r170';out.mkdir(exist_ok=True)
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
v=load(root/'cover-output/run-result.json');assert 'normal_cleanup_verified' in v
s=v.get('terminal_session',{});index=load(root/'launcher-session.json') if (root/'launcher-session.json').exists() else {}
for label,path in [('run-result-original',root/'cover-output/run-result.json'),('first-start-request-original',root/'isolated-start-request.json'),
                   ('isolated-done-original',root/'isolated-done.json'),('launcher-index-original',root/'launcher-session.json')]:
    if path.exists():shutil.copyfile(path,out/(label+'.json'))
(out/'session-original.json').write_text(json.dumps(s,indent=2)+'\n')
native='0de22cab918329d51091f4e640043ffb43ec0f17586a93affab763d333660e17'
spec=importlib.util.spec_from_file_location('r170_timing',H/'inspect_age.py')
reader=importlib.util.module_from_spec(spec);spec.loader.exec_module(reader)
try:
    age=reader.inspect(s,native);timing=age['timing']
    (out/'age-probe.json').write_text(json.dumps(age['source_ages'],indent=2)+'\n')
except Exception as error:timing=dict(interpretation_error=repr(error),query_proven=False)
(out/'timing-probe.json').write_text(json.dumps(timing,indent=2)+'\n')
if 'query' in timing:(out/'path-probe.json').write_text(json.dumps(timing['query'],indent=2)+'\n')
summary={k:v.get(k) for k in ('error','operator_exit','start_exit','normal_cleanup_verified','normal_cleanup_required',
    'projection_frames','projection_errors','remaining_owned','anchor_sources_unchanged','cloak_observations','foreground_observations','forced_termination')}
(out/'closeout.json').write_text(json.dumps(summary,indent=2)+'\n')
print(json.dumps(summary));print(json.dumps(timing.get('rows',timing)))
if (out/'age-probe.json').exists():print((out/'age-probe.json').read_text())
