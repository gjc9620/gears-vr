from pathlib import Path
import hashlib,json
A=Path(__file__).resolve().parent;ROOT=A.parents[1]
def main(name,tool_session):
 p=A/name;result=json.loads((p/'result.json').read_text());log=(p/'start.log').read_text(errors='replace')
 assert not result['game_started'] and not result['operator_started'] and 'Two automatic instances already own' in log
 instance=ROOT/'analysis/instances'/name
 assert not any((instance/n).exists() for n in ('isolated-worker.json','launcher-session.json','isolated-done.json'))
 proof=dict(name=name,terminal_tool_session=tool_session,terminal_tool_exit=1,game_created=False,operator_created=False,
  resource_start_rejected=True,cause='Canonical quota recheck rejected after advisory wait observed one active instance; another worker registered before this start transaction. No gameplay or authority change.',
  resource_admission_wait=result['resource_admission_wait'],result_sha256=hashlib.sha256((p/'result.json').read_bytes()).hexdigest(),
  start_log_sha256=hashlib.sha256((p/'start.log').read_bytes()).hexdigest(),old_name_never_restart=True)
 target=A/('admission-rejected-'+name.split('-')[-1]+'.json');target.write_text(json.dumps(proof,indent=2));print(json.dumps(proof))
if __name__=='__main__':main('hb-cloak-1111-023618',41155)
