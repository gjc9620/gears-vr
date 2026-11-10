"""Preserve the focused live insertion failure, including actual kernel outputs."""
from pathlib import Path
import hashlib,json,re,shutil
import review_runtime_11644 as review
A=Path(__file__).resolve().parent;ROOT=A.parents[1];NAME='hb-cloak-1111-013604'
P=ROOT/'analysis/instances'/NAME;ACTION=P/'actions/menu-firstperson-53560-013858'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
result=read(A/NAME/'result.json');state=read(P/'launcher-session.json');session=read(Path(state['session_report']))
assert result['cleanup']['owned_game_absent'] and not result['cleanup']['owned_children']['remaining']
pg=ROOT/'tools/playground'/('build-'+NAME)/'playground-53560.log';b=pg.read_bytes()
lines=b.decode('utf-16' if b[:2]==b'\xff\xfe' else 'utf8',errors='replace').splitlines()
def rows(prefix):return [dict(re.findall(r'(\w+)=([^\s]+)',l)) for l in lines if l.startswith(prefix+' ')]
archive=A/'runtime-53560-text';archive.mkdir(exist_ok=True);copies=[]
for p in (pg,ACTION/'result.json',ACTION/'equipment-chain.json',ACTION/'hammerburst-core.json',ACTION/'hammerburst-replacement.json',Path(state['session_report']),P/'isolated-done.json',Path(state['test_audio_guard']['log'])):
 target=archive/p.name;shutil.copy2(p,target);digest=hashlib.sha256(p.read_bytes()).hexdigest()
 assert hashlib.sha256(target.read_bytes()).hexdigest()==digest
 copies.append(dict(source=str(p),archive=str(target),sha256=digest))
media=read(ACTION/'recording.json');shots=rows('native_lancer_shot_trajectory');outcomes=rows('hammerburst_feed_outcome')
first=next(r for r in outcomes if r['rearmed']=='1')
assert first['frame']=='787' and first['gap_ns']=='110739000'
assert float(first['depth'])<.045 and float(first['depth'])>.006
assert any(r['result']=='1' and int(r['frame'])<787 for r in outcomes)
assert not any(r['result']=='2' for r in outcomes)
endpoint=next(r for r in outcomes if r['frame']=='851')
proof=dict(game=result['game_identity'],candidate=read(A/'feed-observe-r1/prepared.json')['dll_sha256'],test_mode=result['test_mode'],
 operator_exit=result['operator_exit'],stop_exit=result['stop_exit'],cleanup=result['cleanup'],
 continuous_passed=session['passed'],native_cleanup_verified=session['native_cleanup_verified'],xr_exit_code=session['xr_exit_code'],
 playground_cleanup=state['playground_cleanup'],audio=state['test_audio_guard'],replacement=read(ACTION/'hammerburst-replacement.json'),
 actual_trajectory_count=len(shots),trajectory_consistent=all(all(r.get(k)=='1' for k in ('trace_valid','trace_source_consistent','write_source_consistent')) for r in shots),
 feed_outcome_rows=outcomes,feed_move_rejected_rows=rows('hammerburst_feed_move_rejected'),feed_rejected_rows=rows('hammerburst_feed_rejected'),
 first_observed_rearm=first,endpoint=endpoint,latch_handoff_rows=rows('hammerburst_latch_frame'),supply_handoff_rows=rows('hammerburst_supply_frame'),
 service_loss_rows=rows('hammerburst_service_loss'),stale_wait_rows=rows('hammerburst_stale_debt_wait'),
 support_withdrawal_rows=rows('native_equipment_support_withdrawal'),identity_lost_rows=rows('hammerburst_identity_lost'),transitions=rows('hammerburst_reload_transition'),
 foreground_samples=result['focus_samples'],cloak_samples=result['cloak_samples'],foreground_violations=result['foreground_violations'],cloak_violations=result['cloak_violations'],
 media=dict(frames=len(media['frames']),errors=media['errors'],review=str(A/'runtime-53560-media-review/prepared.json'),full_fit_accepted=False),verified_archive=copies,
 interpretation='Focused loaded replacement only. Core6 passed, actual72/R78 original reload, latch/extraction/new supply reached6. Kernel approaching before Frame787; actual source gap110.739ms rearms below45mm approach at41.923mm, loses stroke. Later endpoint depth.011326mm/lateral.005797mm/orientation_dot1 still rejected. No move/early/identity/support rejection recorded. No seating/closure/return. Do not widen100ms or synthesize samples; next inspect authenticated original producer motion history with exact current authority. Empty full flow and intermittent event expiry remain open.',
 gameplay_accepted=False,physical_headset_accepted=False)
(A/'runtime-53560-first-failure-analysis.json').write_text(json.dumps(proof,indent=2))
review.SOURCE=ACTION;review.OUTPUT=A/'runtime-53560-media-review';review.main(len(media['frames']),53560)
print(json.dumps(dict(frames=len(media['frames']),shots=len(shots),first_rearm=first,endpoint=endpoint,cleanup=result['cleanup'])))
