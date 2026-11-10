"""Read-only review of the coordinator's new typed prelaunch protocol."""
from pathlib import Path
import ast,hashlib,json,sys
A=Path(__file__).resolve().parent;ROOT=A.parents[1]
p=ROOT/'analysis/coordinator-multi-20261110/python-prelaunch-rejection-20261111.json';proof=json.loads(p.read_text())
assert not proof['game_started'] and proof['checks']==dict(python_discovery_readonly=5,launcher_offline=81,runtime_receipts_offline=15,python_parse=True)
for rel,digest in proof['source_sha256'].items():assert hashlib.sha256((ROOT/rel).read_bytes()).hexdigest()==digest,rel
receipts=(ROOT/'tools/multi-instance/runtime_receipts.py').read_text();tree=ast.parse(receipts)
fn=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='confirmed_prelaunch_rejection');body=ast.get_source_segment(receipts,fn)
for token in ("index.get('owner_identity')!=owner","cleanup.get('no_game_started') is not True", "proof.get('stage')!='python_discovery'", "('game_creation_attempted','injection_attempted','prime_attempted')",'not receipt.exists()',"not any(selected.glob('*.birth.json'))"):
 assert token in body,token
worker=(ROOT/'tools/multi-instance/isolated_sim.py').read_text();assert 'wait_launcher(child,control,child_identity)' in worker
assert 'known.game is None and confirmed_prelaunch_rejection(known.index,child_identity,selected)' in worker
assert "'owned_game_absent':absent" in worker and "'no_game_started':no_game" in worker
runner=(A/'run_isolated_empty.py').read_text();assert "result['cleanup']=json.loads((selected/'isolated-done.json').read_text())" in runner
assert "result.get('operator_exit')==0" in runner and ".get('owned_game_absent')" in runner
# No compatibility edit is necessary: done is retained verbatim, including
# no_game_started and absent:null; failed prelaunch can never count as pass.
sys.path.insert(0,str(ROOT/'tools/native-capture'));from run_stereo_continuous import WindowsControl
record=json.loads((ROOT/'analysis/instances/hb-cloak-1111-052600/isolated-worker.json').read_text())
identity=WindowsControl().identity(record['worker_pid']);assert identity==record['worker_identity']
result=dict(shared_proof_sha256=hashlib.sha256(p.read_bytes()).hexdigest(),source_sha256=proof['source_sha256'],read_only_changed_protocol_review=True,shared_tests_reused=proof['checks'],repeated_tests=False,new_game_started=False,own_wrapper_compatible_without_change=True,
 current_old_worker=identity,old_worker_normal_cleanup_resolved=False,old_worker_hotpatched=False,retroactive_sidecar_created=False,
 limitation='Future workers only. Actual old worker53716/old139 still occupy both slots. No new runtime, no forced kill, no fake birth/absence or weakening quota. Full objective remains active/incomplete.')
(A/'prelaunch-rejection-integration-check.json').write_text(json.dumps(result,indent=2));print(json.dumps({k:result[k] for k in ('read_only_changed_protocol_review','own_wrapper_compatible_without_change','old_worker_normal_cleanup_resolved','current_old_worker')}))
