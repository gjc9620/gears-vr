"""Record actual first pending-event expiry; do not classify it as seating evidence."""
from pathlib import Path
import hashlib,json,re,shutil
import review_runtime_11644 as review
A=Path(__file__).resolve().parent;ROOT=A.parents[1];NAME='hb-cloak-1111-013055'
P=ROOT/'analysis/instances'/NAME;ACTION=P/'actions/menu-firstperson-20672-013122'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
result=read(A/NAME/'result.json');state=read(P/'launcher-session.json');session=read(Path(state['session_report']))
assert result['cleanup']['owned_game_absent'] and not result['cleanup']['owned_children']['remaining']
pg=ROOT/'tools/playground'/('build-'+NAME)/'playground-20672.log';b=pg.read_bytes()
lines=b.decode('utf-16' if b[:2]==b'\xff\xfe' else 'utf8',errors='replace').splitlines()
def rows(prefix):return [dict(re.findall(r'(\w+)=([^\s]+)',l)) for l in lines if l.startswith(prefix+' ')]
i=next(i for i,l in enumerate(lines) if l.startswith('hammerburst_service_loss'))
archive=A/'runtime-20672-text';archive.mkdir(exist_ok=True);copies=[]
for p in (pg,ACTION/'result.json',ACTION/'equipment-chain.json',ACTION/'hammerburst-core.json',ACTION/'hammerburst-empty-reload.json',Path(state['session_report']),P/'isolated-done.json',Path(state['test_audio_guard']['log'])):
 target=archive/p.name;shutil.copy2(p,target);digest=hashlib.sha256(p.read_bytes()).hexdigest()
 assert hashlib.sha256(target.read_bytes()).hexdigest()==digest
 copies.append(dict(source=str(p),archive=str(target),sha256=digest))
media=read(ACTION/'recording.json');shots=rows('native_lancer_shot_trajectory')
proof=dict(game=result['game_identity'],candidate=read(A/'feed-observe-r1/prepared.json')['dll_sha256'],
 operator_exit=result['operator_exit'],stop_exit=result['stop_exit'],cleanup=result['cleanup'],
 continuous_passed=session['passed'],native_cleanup_verified=session['native_cleanup_verified'],xr_exit_code=session['xr_exit_code'],
 playground_cleanup=state['playground_cleanup'],audio=state['test_audio_guard'],empty_reload=read(ACTION/'hammerburst-empty-reload.json'),
 actual_trajectory_count=len(shots),trajectory_consistent=all(all(r.get(k)=='1' for k in ('trace_valid','trace_source_consistent','write_source_consistent')) for r in shots),
 service_loss_rows=rows('hammerburst_service_loss'),stale_wait_rows=rows('hammerburst_stale_debt_wait'),
 feed_outcome_rows=rows('hammerburst_feed_outcome'),transitions=rows('hammerburst_reload_transition'),failure_context=lines[max(0,i-20):i+15],
 foreground_samples=result['focus_samples'],cloak_samples=result['cloak_samples'],foreground_violations=result['foreground_violations'],cloak_violations=result['cloak_violations'],
 media=dict(frames=len(media['frames']),errors=media['errors'],review=str(A/'runtime-20672-media-review/prepared.json'),full_fit_accepted=False),verified_archive=copies,
 interpretation='Actual core6 plus five drain6 passed; sixth drain produced1 actual ray before service gate4 original pending event release_required. Latest withdrawal source stale_before101.0706ms is contemporaneous, not proof of the complete pending-event delay cause. Total37 consistent rays. No reload/insertion reached, no feed observer exercise; no retry. Original250ms event deadline retained. All cleanup normal.',
 gameplay_accepted=False,physical_headset_accepted=False)
(A/'runtime-20672-first-failure-analysis.json').write_text(json.dumps(proof,indent=2))
review.SOURCE=ACTION;review.OUTPUT=A/'runtime-20672-media-review';review.main(len(media['frames']),20672)
print(json.dumps({k:proof[k] for k in ('actual_trajectory_count','trajectory_consistent','service_loss_rows','cleanup')}))
