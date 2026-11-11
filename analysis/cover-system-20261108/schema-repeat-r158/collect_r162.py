"""Preserve terminal r162 facts without converting absent evidence to passes."""
from pathlib import Path
import importlib.util, json, hashlib
H=Path(__file__).resolve().parent
R=H.parents[2]
root=R/'analysis/instances/cover-1111-r162-cloak-schema'
out=H/'runtime-r162'
out.mkdir(exist_ok=True)
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
result=load(root/'cover-output/run-result.json')
assert 'normal_cleanup_verified' in result, 'Runner is not terminal'
for label,path in [('run-result-original',root/'cover-output/run-result.json'),
                   ('first-start-request-original',root/'isolated-start-request.json')]:
    (out/(label+'.json')).write_bytes(path.read_bytes())
session=result.get('terminal_session',{})
(out/'session-original.json').write_text(json.dumps(session,indent=2)+'\n')
spec=importlib.util.spec_from_file_location('r162_cause',H.parent/'query-age-cause-r149/inspect_cause.py')
reader=importlib.util.module_from_spec(spec);spec.loader.exec_module(reader)
try:
    query=reader.inspect(session,'58c2fe1d9fa2a108010f33008278060f9ad3a2ccde53d3d87f3e27c829680c7f')
except Exception as error:
    query={'interpretation_error':repr(error),'query_proven':False}
(out/'path-probe.json').write_text(json.dumps(query,indent=2)+'\n')
facts={k:result.get(k) for k in ('error','operator_exit','start_exit','normal_cleanup_required',
    'normal_cleanup_verified','remaining_owned','projection_frames','projection_errors',
    'anchor_sources_unchanged','cloak_observations','foreground_observations','forced_termination')}
facts.update(gameplay_accepted=False,headset_accepted=False,
    original_result_sha256=hashlib.sha256((root/'cover-output/run-result.json').read_bytes()).hexdigest())
(out/'closeout.json').write_text(json.dumps(facts,indent=2)+'\n')
print(json.dumps(facts))
