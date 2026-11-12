from pathlib import Path
import json,re,shutil,hashlib
h=Path(__file__).resolve().parent;r=h.parents[2];out=h/'runtime-r254';out.mkdir(exist_ok=True)
root=r/'analysis/instances/cover-1112-r254-cloak-proof-pass';run=json.loads((root/'cover-output/run-result.json').read_text());session=run['terminal_session']
assert run['normal_cleanup_verified'] and not run['remaining_owned']
for source,name in [(root/'cover-output/run-result.json','run-result-original.json'),(root/'launcher-session.json','launcher-index-original.json'),(root/'isolated-done.json','isolated-done-original.json'),(Path(session['session_report']).with_suffix('.session-status.txt'),'native-session-status-original.txt')]:shutil.copyfile(source,out/name)
raw=out/'native-session-status-original.txt';values=dict(re.findall(r'(\w+)=([^\s]+)',raw.read_text()));probe={k:v for k,v in values.items() if k.startswith('cover_path_probe_')}
report={k:run.get(k) for k in ['operator_exit','normal_cleanup_verified','remaining_owned','projection_frames','projection_errors','anchor_sources_unchanged','cover_reader_sources_unchanged','cloak_observations','foreground_observations']}
report.update(probe=probe,source_sha256=hashlib.sha256(raw.read_bytes()).hexdigest(),elapsed_ms=(int(probe['cover_path_probe_end_ns'])-int(probe['cover_path_probe_begin_ns']))/1e6,
 original_started=int(probe['cover_path_probe_original_started_after'])-int(probe['cover_path_probe_original_started_before']),original_returned=int(probe['cover_path_probe_original_returned_after'])-int(probe['cover_path_probe_original_returned_before']),
 raw_query_success=probe['cover_path_probe_code']=='0',movement_permission=False,gameplay_accepted=False,headset_accepted=False)
budget_line=next(x for x in raw.read_text().splitlines() if 'cover_path_budget_checks=' in x)
budget_line=budget_line[budget_line.index('cover_path_budget_checks='):]
report['budget']=dict(re.findall(r'(\w+)=([^\s]+)',budget_line))
(out/'closeout.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ['probe','cloak_observations','foreground_observations']}));print(json.dumps({k:v for k,v in probe.items() if k in ('cover_path_probe_attempted','cover_path_probe_code','cover_path_probe_owner_guard','cover_path_probe_session_guard','cover_path_probe_engine_stage','cover_path_probe_session_stage','cover_path_probe_source_age_cause','cover_path_probe_full_rechecks','cover_path_probe_borrowed_read_guards','cover_path_probe_component_proofs','cover_path_probe_segments','cover_path_probe_hits','cover_path_probe_endpoint_shapes','cover_path_probe_self_hits','cover_path_probe_endpoint_self')}))






