"""Preserve startup receipt failure separately from unperformed weapon tests."""
from pathlib import Path
import hashlib,json,shutil
A=Path(__file__).resolve().parent;ROOT=A.parents[1];NAME='hb-cloak-1111-045812';I=ROOT/'analysis/instances'/NAME
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
result=read(A/NAME/'result.json');state=read(I/'launcher-session.json');done=read(I/'isolated-done.json')
assert result['birth_game_identity']['pid']==21596 and not result['operator_started']
assert result['cleanup']['launcher_exit']==1 and result['stop_exit']==1 and result['cleanup']['owned_game_absent']
assert not result['cleanup']['owned_children']['remaining'] and not result['cleanup']['forced_termination']
assert not state.get('session_report') and 'WinError 5' in result['error']
sources=[A/NAME/'result.json',I/'launcher-session.json',I/'launcher-session.reserve.log',I/'isolated-done.json']
sources+=list(I.glob('launcher-session.reserve.log.*.birth.json'))+list(I.glob('launcher-session.reserve.log.*.birth.json.tmp'))
sources+=list(I.glob('launcher-session.reserve.log.*.startup.jsonl'))+list(I.glob('launcher-session.reserve.log.*.startup.jsonl.first-failure.json'))
archive=A/'runtime-21596-text';archive.mkdir(exist_ok=True);copies=[]
for source in sources:
 target=archive/source.name;shutil.copy2(source,target);digest=hashlib.sha256(source.read_bytes()).hexdigest();assert hashlib.sha256(target.read_bytes()).hexdigest()==digest
 copies.append(dict(source=str(source),archive=str(target),sha256=digest))
birth=read(next(I.glob('*.birth.json')));pending=read(next(I.glob('*.birth.json.tmp')))
assert birth['game_identity']==result['birth_game_identity']==pending['game_identity']
proof=dict(game=result['birth_game_identity'],candidate=read(A/'endpoint-baseline-r1/prepared.json')['dll_sha256'],operator_started=False,actual_trajectory_count=0,
 first_failure='mark_launch_stage injection_attempted: same birth.json.tmp ->birth.json atomic replace WinError5. Reservation stopped; no startup retry. Earlier PG reservation may already have run; injection state remains unknown, not safe to retry.',
 error=result['error'],birth_receipt=birth,pending_payload=pending,
 note='Both final observed birth/pending have injection_attempted true: stage is marked before optional PG reservation and again before capture DLL load. Do not infer no prior injection from this failed second publication.',
 cleanup=result['cleanup'],launcher_exit=1,stop_exit=1,normal_full_runtime_cleanup_pass=False,owned_game_absent=True,native_xr_gameplay_evidence='No continuous session report/operator; no Native/XR/action acceptance. Preserve unknown-injection cleanup classification.',
 media=dict(frames=0,recording_started=False,encoded=False),foreground_samples=result['focus_samples'],foreground_violations=result['foreground_violations'],cloak_samples=result['cloak_samples'],cloak_violations=result['cloak_violations'],verified_archive=copies,
 shared_fix='Existing coordinator owns shared launch_reserve bounded same-payload atomic replacement.6 attempts,5/32/33 only,25ms between, original full receipt equality before later attempt; no reserve/inject/prime/game retry.12 offline tests, no live repair proof.',
 own_reader_experiment='Actual Windows temp-file probe4 tests:2pass/2error. SHARE_DELETE alone did not permit replacement with held reader. Unwired from runner; failed source/evidence retained, not shipped as fix.',
 gameplay_accepted=False,physical_headset_accepted=False)
p=A/'runtime-21596-first-failure-analysis.json';p.write_text(json.dumps(proof,indent=2));digest=hashlib.sha256(p.read_bytes()).hexdigest()
p=A/'full-requirements-audit-20261110.json';audit=read(p);audit.update(latest_runtime_failure='runtime-21596-first-failure-analysis.json',candidate_runtime_status='endpoint-baseline-r1 actual21596 startup failure publishing birth stage WinError5; operator not started. launcher1/stop1, owned game gone/no force. No stricter first-fire setup/handoff/weapon action tested.',immediate_next='Verify coordinator shared source hashes/bounded same-payload patch, preserve failed share-delete experiment unwired, save then one fresh instance. Never reuse045812/72902. Retain9 first-fire fixture results; no live readiness proof yet. All13 requirements retained.')
for r in audit['requirements']:
 if r['id']=='delivery_headset':r['evidence'].append(dict(path=str(A/'runtime-21596-first-failure-analysis.json'),sha256=digest))
assert len(audit['requirements'])==13 and not audit['completion_proven'];p.write_text(json.dumps(audit,indent=2,ensure_ascii=False))
p=A/'active-runtime-wait.json';w=read(p);w.update(terminal=True,game_pid=21596,runner_last_poll='72902 terminal exit1; startup failure, launcher1/stop1, game absent verified',next=audit['immediate_next']);p.write_text(json.dumps(w,indent=2))
print(json.dumps(dict(game=21596,operator_started=False,launcher_exit=1,stop_exit=1,game_absent=True,archived_sources=len(copies))))
