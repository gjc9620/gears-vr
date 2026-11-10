"""Actual deferred first press expired before any original shot; no feed acceptance."""
from pathlib import Path
import hashlib,json,re,shutil
import review_runtime_11644 as review
A=Path(__file__).resolve().parent;ROOT=A.parents[1];NAME='hb-cloak-1111-015245'
P=ROOT/'analysis/instances'/NAME;ACTION=P/'actions/menu-firstperson-56348-015325'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
result=read(A/NAME/'result.json');state=read(P/'launcher-session.json');session=read(Path(state['session_report']))
assert result['cleanup']['owned_game_absent'] and not result['cleanup']['owned_children']['remaining']
pg=ROOT/'tools/playground'/('build-'+NAME)/'playground-56348.log';b=pg.read_bytes()
lines=b.decode('utf-16' if b[:2]==b'\xff\xfe' else 'utf8',errors='replace').splitlines()
def rows(prefix):return [dict(re.findall(r'(\w+)=([^\s]+)',l)) for l in lines if l.startswith(prefix+' ')]
i=next(i for i,l in enumerate(lines) if l.startswith('hammerburst_input_consumed frame=193 '))
archive=A/'runtime-56348-text';archive.mkdir(exist_ok=True);copies=[]
for p in (pg,ACTION/'result.json',ACTION/'equipment-chain.json',ACTION/'hammerburst-core.json',Path(state['session_report']),P/'isolated-done.json',Path(state['test_audio_guard']['log'])):
 target=archive/p.name;shutil.copy2(p,target);digest=hashlib.sha256(p.read_bytes()).hexdigest()
 assert hashlib.sha256(target.read_bytes()).hexdigest()==digest
 copies.append(dict(source=str(p),archive=str(target),sha256=digest))
media=read(ACTION/'recording.json');shots=rows('native_lancer_shot_trajectory')
proof=dict(game=result['game_identity'],candidate=read(A/'feed-history-r1/prepared.json')['dll_sha256'],test_mode=result['test_mode'],
 operator_exit=result['operator_exit'],stop_exit=result['stop_exit'],cleanup=result['cleanup'],
 continuous_passed=session['passed'],native_cleanup_verified=session['native_cleanup_verified'],xr_exit_code=session['xr_exit_code'],
 playground_cleanup=state['playground_cleanup'],audio=state['test_audio_guard'],core=read(ACTION/'hammerburst-core.json'),
 actual_trajectory_count=len(shots),execute_not_ready_rows=rows('native_lancer_execute_not_ready'),
 feed_history_rows=rows('hammerburst_feed_history'),transitions=rows('hammerburst_reload_transition'),failure_context=lines[max(0,i-8):i+22],
 foreground_samples=result['focus_samples'],cloak_samples=result['cloak_samples'],foreground_violations=result['foreground_violations'],cloak_violations=result['cloak_violations'],
 media=dict(frames=len(media['frames']),errors=media['errors'],review=str(A/'runtime-56348-media-review/prepared.json'),full_fit_accepted=False),verified_archive=copies,
 interpretation='Actual first core press: Frame193 status10 is deferred, not a hard execution failure. Frame194 pre-dispatch evidence valid/same/current/live but reserved_frame_current0/reserved_live0 at83.8485ms source age, so original ButtonPress not called. Frame195 status8 blocked, later held input accepted without original shot; finally real release, zero trajectories. Original intent lifetime expired; full timing cause before dispatch unproven. No reload/history branch reached; cannot accept or reject feed repair from this run. Preserve20ms reserve/100ms source/original intent deadline.',
 gameplay_accepted=False,physical_headset_accepted=False)
(A/'runtime-56348-first-failure-analysis.json').write_text(json.dumps(proof,indent=2))
review.SOURCE=ACTION;review.OUTPUT=A/'runtime-56348-media-review';review.main(len(media['frames']),56348)
print(json.dumps(dict(frames=len(media['frames']),shots=len(shots),execute_not_ready=proof['execute_not_ready_rows'],cleanup=result['cleanup'])))
