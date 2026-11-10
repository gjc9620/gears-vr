from pathlib import Path
import hashlib,json
A=Path(__file__).resolve().parent;ROOT=A.parents[1];NAME='hb-cloak-1111-023254'
p=A/NAME;result=json.loads((p/'result.json').read_text());log=(p/'start.log').read_text(errors='replace')
assert not result['game_started'] and not result['operator_started']
assert 'Two automatic instances already own' in log
instance=ROOT/'analysis/instances'/NAME
assert not any((instance/n).exists() for n in ('isolated-worker.json','launcher-session.json','isolated-done.json'))
proof=dict(name=NAME,terminal_tool_exit=1,game_created=False,operator_created=False,resource_start_rejected=True,
 cause='Canonical quota recheck rejected after advisory wait saw only Lancer; Boltok admitted before this start transaction. No gameplay run or permission weakening.',
 result_sha256=hashlib.sha256((p/'result.json').read_bytes()).hexdigest(),start_log_sha256=hashlib.sha256((p/'start.log').read_bytes()).hexdigest(),
 old_name_never_restart=True,next='Fresh unique preparation, bounded resource wait, first start only; same unexercised inventory timing candidate.')
(A/'admission-rejected-023254.json').write_text(json.dumps(proof,indent=2));print(json.dumps(proof))
