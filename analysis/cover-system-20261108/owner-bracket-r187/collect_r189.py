"""Collect the new terminal timing run; never treat missing data as success."""
from pathlib import Path
import importlib.util,json,hashlib,shutil
H=Path(__file__).resolve().parent;R=H.parents[2]
root=R/'analysis/instances/cover-1111-r189-cloak-bracket';out=H/'runtime-r189';out.mkdir(exist_ok=True)
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
v=load(root/'cover-output/run-result.json');assert 'normal_cleanup_verified' in v
s=v.get('terminal_session',{});index=load(root/'launcher-session.json') if (root/'launcher-session.json').exists() else {}
for label,path in [('run-result-original',root/'cover-output/run-result.json'),('first-start-request-original',root/'isolated-start-request.json'),
                   ('isolated-done-original',root/'isolated-done.json'),('launcher-index-original',root/'launcher-session.json')]:
    if path.exists():shutil.copyfile(path,out/(label+'.json'))
(out/'session-original.json').write_text(json.dumps(s,indent=2)+'\n')
native='e09b3e0ff8ea8da3c6998022254932835763540052a2eb0bc2201a688c64f6af'
spec=importlib.util.spec_from_file_location('r189_timing',H.parent/'actor-backlink-r173/inspect_links.py')
reader=importlib.util.module_from_spec(spec);spec.loader.exec_module(reader)
try:
    if 'cover_path_probe_timing_8' not in s.get('native_status',{}):raise ValueError('r187 owner_bracket timing bin missing; not valid new-candidate evidence')
    age=reader.inspect(s,native);timing=age['timing']
    assert len(timing['rows'])==9 and timing['rows'][8]['stage']=='owner_bracket'
    (out/'age-probe.json').write_text(json.dumps(age['source_ages'],indent=2)+'\n')
    (out/'owner-links.json').write_text(json.dumps(age['owner_links'],indent=2)+'\n')
except Exception as error:timing=dict(interpretation_error=repr(error),query_proven=False)
(out/'timing-probe.json').write_text(json.dumps(timing,indent=2)+'\n')
if 'query' in timing:(out/'path-probe.json').write_text(json.dumps(timing['query'],indent=2)+'\n')
summary={k:v.get(k) for k in ('error','operator_exit','start_exit','normal_cleanup_verified','normal_cleanup_required',
    'projection_frames','projection_errors','remaining_owned','anchor_sources_unchanged','cloak_observations','foreground_observations','forced_termination')}
(out/'closeout.json').write_text(json.dumps(summary,indent=2)+'\n')
print(json.dumps(summary));print(json.dumps(timing.get('rows',timing)))
if (out/'age-probe.json').exists():print((out/'age-probe.json').read_text())



