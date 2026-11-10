"""Read-only hash and bounded changed-hunk verification; no repeated game/test."""
from pathlib import Path
import hashlib,json,ast
A=Path(__file__).resolve().parent;ROOT=A.parents[1]
proof_path=ROOT/'analysis/coordinator-multi-20261110/birth-stage-publication-20261111.json';proof=json.loads(proof_path.read_text())
assert proof['tests_passed']==12 and not proof['live_recheck'] and not proof['new_game_started']
for rel,digest in proof['source_sha256'].items():assert hashlib.sha256((ROOT/rel).read_bytes()).hexdigest()==digest,rel
t=(ROOT/'tools/native-capture/launch_reserve.py').read_text();tree=ast.parse(t);fn=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='mark_launch_stage')
body=ast.get_source_segment(t,fn)
assert 'range(6)' in body and 'not in (5,32,33)' in body and 'time.sleep(.025)' in body
assert body.count('temporary.write_text(')==1 and body.count('temporary.replace(path)')==1
assert "json.loads(path.read_text(encoding='utf-8'))!=previous" in body
assert 'run_hidden(' not in body and 'remote_call(' not in body
runner=(A/'run_isolated_empty.py').read_text();assert 'from shared_receipt_reader' not in runner and 'read_shared_json(birth)' not in runner
result=dict(shared_proof_sha256=hashlib.sha256(proof_path.read_bytes()).hexdigest(),source_sha256=proof['source_sha256'],read_only_review_passed=True,repeated_tests=False,coordinator_offline_tests=12,game_execution=False,own_failed_reader_unwired=True,gameplay_accepted=False)
(A/'birth-publication-integration-check.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
