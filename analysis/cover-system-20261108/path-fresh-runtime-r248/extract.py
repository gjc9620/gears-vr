from pathlib import Path
import json,re,shutil,hashlib
h=Path(__file__).resolve().parent;r=h.parents[2];out=h/'runtime-r248';out.mkdir(exist_ok=True)
root=r/'analysis/instances/cover-1112-r248-cloak-path-fresh';run=json.loads((root/'cover-output/run-result.json').read_text());session=run['terminal_session']
assert run['normal_cleanup_verified'] and not run['remaining_owned']
for source,name in [(root/'cover-output/run-result.json','run-result-original.json'),(root/'launcher-session.json','launcher-index-original.json'),(root/'isolated-done.json','isolated-done-original.json'),(Path(session['session_report']).with_suffix('.session-status.txt'),'native-session-status-original.txt')]:shutil.copyfile(source,out/name)
raw=out/'native-session-status-original.txt';values=dict(re.findall(r'(\w+)=([^\s]+)',raw.read_text()));sphere={k:v for k,v in values.items() if k.startswith('cover_path_probe_')}
report={k:run.get(k) for k in ['operator_exit','normal_cleanup_verified','remaining_owned','projection_frames','projection_errors','anchor_sources_unchanged','cover_reader_sources_unchanged','cloak_observations','foreground_observations']}
report.update(sphere=sphere,source_sha256=hashlib.sha256(raw.read_bytes()).hexdigest(),elapsed_ms=(int(sphere['cover_path_probe_end_ns'])-int(sphere['cover_path_probe_begin_ns']))/1e6,
 original_started=int(sphere['cover_path_probe_original_started_after'])-int(sphere['cover_path_probe_original_started_before']),original_returned=int(sphere['cover_path_probe_original_returned_after'])-int(sphere['cover_path_probe_original_returned_before']),
 raw_query_success=sphere['cover_path_probe_code']=='0',movement_permission=False,gameplay_accepted=False,headset_accepted=False)
(out/'closeout.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ['sphere','cloak_observations','foreground_observations']}));print(json.dumps(sphere))


