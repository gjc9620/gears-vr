"""Record prelaunch failure and live unknown cleanup without inventing a game."""
from pathlib import Path
import hashlib,json,sys,time
A=Path(__file__).resolve().parent;ROOT=A.parents[1];NAME='hb-cloak-1111-052600';I=ROOT/'analysis/instances'/NAME
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
result=read(A/NAME/'result.json');state=read(I/'launcher-session.json');worker=read(I/'isolated-worker.json')
assert not result['game_started'] and not result['operator_started'] and result['cleanup_unresolved'] and result['stop_exit']==1
assert state.get('game_identity') is None and not list(I.glob('*.birth.json')) and not state.get('session_report')
log=(I/'launcher-session.reserve.log').read_text(errors='replace')
assert 'GEARSVR_PYTHON is not a usable Python 3 executable; no game startup actions were taken.' in log
sys.path.insert(0,str(ROOT/'tools/native-capture'))
from run_stereo_continuous import WindowsControl
actual_worker=WindowsControl().identity(worker['worker_pid'])
assert actual_worker==worker['worker_identity']
archive=A/'prelaunch-052600-text';archive.mkdir(exist_ok=True);copies=[]
sources=[A/NAME/'result.json',I/'launcher-session.json',I/'launcher-session.reserve.log',I/'isolated-worker.json',I/'isolated-cleanup-pending.json']
for source in sources:
 data=source.read_bytes();target=archive/source.name;target.write_bytes(data);digest=hashlib.sha256(data).hexdigest();assert hashlib.sha256(target.read_bytes()).hexdigest()==digest
 copies.append(dict(source=str(source),archive=str(target),sha256_of_observed_bytes=digest,mutable_snapshot=source.name=='isolated-cleanup-pending.json'))
ps1=ROOT/'tools/debug-launch/Start-DebugGame.ps1'
proof=dict(instance=NAME,observed_ns=time.perf_counter_ns(),wrapper_session=34409,wrapper_terminal_exit=1,game_identity=None,game_started_recorded=False,operator_started=False,
 first_failure='Start-DebugGame.ps1 Find-GearsVrPython guard refused explicit interpreter before game_install/instance_policy/game creation; original probe failure cause not logged.',
 guard_log_exact='GEARSVR_PYTHON is not a usable Python 3 executable; no game startup actions were taken.',
 ps1_sha256_at_observation=hashlib.sha256(ps1.read_bytes()).hexdigest(),source_hash_is_not_launch_time_attestation=True,
 source_order='Observed guard before $python assignment/PythonCheckOnly exit and all game startup commands; compatible with captured error at line48. No fabricated birth/game identity.',
 current_check_only_probe=dict(exit_code=0,elapsed_seconds=.4604311,check_only=True,python='C:/Users/11/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe',game_started=False,scope='Single explicit -PythonCheckOnly check passed now; not original failure cause proof or repaired startup proof.'),
 cleanup=dict(stop_exit=1,unresolved=True,done_receipt_exists=(I/'isolated-done.json').exists(),exact_worker_live=actual_worker,owned_game_absent_verified=False,forced_termination=False,forged_birth=False),
 verified_observed_text=copies,actions_not_reached=['acquisition','first_fire','drain78','empty_veto','empty_reload','return_fire','pause'],media=dict(recording_started=False,frames=0,encoded=False),
 coordination='Existing coordinator notified of exact prelaunch guard and permanent known.game None finally loop. No new helpers/chats/runtime, no shared edits by HB, no worker kill, no release of unknown resource reservation.',gameplay_accepted=False,full_goal_complete=False)
p=A/'prelaunch-052600-result.json';p.write_text(json.dumps(proof,indent=2));digest=hashlib.sha256(p.read_bytes()).hexdigest()
p=A/'full-requirements-audit-20261110.json';audit=read(p)
audit.update(latest_runtime_failure='prelaunch-052600-result.json',candidate_runtime_status='Latest successful focused22992 remains. Full empty request052600 refused Python guard before game/operator; wrapper34409 terminal1, worker53716 exact lifetime still live in no_birth cleanup unknown. No empty action evidence.',immediate_next='No new runtime while own53716 cleanup unresolved. Existing coordinator owns shared prelaunch diagnostic/empty-worker normal terminal path. Preserve no_birth unknown, do not kill or fabricategame; original PythonCheckOnly now passes but cause unproven. Continue actual empty validation once normal owned terminal established. All13 requirements retained.')
for req in audit['requirements']:
 if req['id'] in ('empty_reload','delivery_headset'):req['evidence'].append(dict(path=str(A/'prelaunch-052600-result.json'),sha256=digest))
assert len(audit['requirements'])==13 and not audit['completion_proven'];p.write_text(json.dumps(audit,indent=2,ensure_ascii=False))
p=A/'active-runtime-wait.json';w=read(p);w.update(terminal=True,wrapper_terminal=True,worker_terminal=False,worker_identity=actual_worker,runner_last_poll='34409 terminal1; separate exact53716 worker still live no_birth cleanup pending',next=audit['immediate_next']);p.write_text(json.dumps(w,indent=2))
print(json.dumps(dict(instance=NAME,wrapper_terminal=True,worker_still_live=actual_worker,game_identity=None,cleanup_resolved=False,empty_test_executed=False)))
