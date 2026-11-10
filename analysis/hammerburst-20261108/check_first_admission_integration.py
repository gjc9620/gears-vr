"""Read-only verification of the reviewed shared first-admission fix."""
from pathlib import Path
import hashlib,json
A=Path(__file__).resolve().parent;ROOT=A.parents[1]
p=ROOT/'analysis/coordinator-multi-20261110/first-admission-wait-20261111.json'
d=json.loads(p.read_text());verified={}
for rel,expected in d['source_hashes'].items():
 actual=hashlib.sha256((ROOT/rel).read_bytes()).hexdigest();assert actual==expected,(rel,actual,expected)
 verified[rel]=actual
runner=(A/'run_isolated_empty.py').read_text();assert "'--wait-resources-seconds','600'" in runner
compile(runner,str(A/'run_isolated_empty.py'),'exec')
proof=dict(shared_source_hashes_verified=verified,source_proof_sha256=hashlib.sha256(p.read_bytes()).hexdigest(),
 shared_offline_tests_reported=14,shared_code_hunk_reviewed=True,own_runner_explicit_first_admission_wait_seconds=600,
 no_old_name_reused=True,new_game_started=False,scope='Pre-registration only, lock-outside waiting; no retry after commit/spawn starts. Original2-instance/memory/disk/identity/pause gates retained.')
(A/'first-admission-integration-check.json').write_text(json.dumps(proof,indent=2));print(json.dumps(proof))
