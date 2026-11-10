"""Record one terminal owned loaded-replacement attempt without inferring acceptance."""
from pathlib import Path
import hashlib,json,re,shutil
import review_runtime_11644 as review
A=Path(__file__).resolve().parent;ROOT=A.parents[1]
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def main(name,pid,action,candidate,interpretation):
 p=ROOT/'analysis/instances'/name;action=p/'actions'/action
 result=read(A/name/'result.json');state=read(p/'launcher-session.json');session=read(Path(state['session_report']))
 assert result['game_identity']['pid']==pid and result['cleanup']['owned_game_absent'] and not result['cleanup']['owned_children']['remaining']
 pg=ROOT/'tools/playground'/('build-'+name)/f'playground-{pid}.log';b=pg.read_bytes()
 lines=b.decode('utf-16' if b[:2]==b'\xff\xfe' else 'utf8',errors='replace').splitlines()
 def rows(prefix):return [dict(re.findall(r'(\w+)=([^\s]+)',l)) for l in lines if l.startswith(prefix+' ')]
 archive=A/f'runtime-{pid}-text';archive.mkdir(exist_ok=True);copies=[]
 for source in (pg,action/'result.json',action/'equipment-chain.json',action/'hammerburst-core.json',action/'hammerburst-replacement.json',Path(state['session_report']),p/'isolated-done.json',Path(state['test_audio_guard']['log'])):
  target=archive/source.name;shutil.copy2(source,target);digest=hashlib.sha256(source.read_bytes()).hexdigest()
  assert hashlib.sha256(target.read_bytes()).hexdigest()==digest
  copies.append(dict(source=str(source),archive=str(target),sha256=digest))
 media=read(action/'recording.json');shots=rows('native_lancer_shot_trajectory')
 proof=dict(game=result['game_identity'],candidate=read(A/candidate/'prepared.json')['dll_sha256'],test_mode=result['test_mode'],
  operator_exit=result['operator_exit'],stop_exit=result['stop_exit'],cleanup=result['cleanup'],
  continuous_passed=session['passed'],native_cleanup_verified=session['native_cleanup_verified'],xr_exit_code=session['xr_exit_code'],
  playground_cleanup=state['playground_cleanup'],audio=state['test_audio_guard'],core=read(action/'hammerburst-core.json'),replacement=read(action/'hammerburst-replacement.json'),
  actual_trajectory_count=len(shots),trajectory_consistent=all(all(r.get(k)=='1' for k in ('trace_valid','trace_source_consistent','write_source_consistent')) for r in shots),
  foreground_samples=result['focus_samples'],cloak_samples=result['cloak_samples'],foreground_violations=result['foreground_violations'],cloak_violations=result['cloak_violations'],
  media=dict(frames=len(media['frames']),errors=media['errors'],encoded=False,review=str(A/f'runtime-{pid}-media-review/prepared.json'),full_fit_accepted=False),
  verified_archive=copies,interpretation=interpretation,gameplay_accepted=False,physical_headset_accepted=False)
 for prefix in ('hammerburst_press_timing','hammerburst_reload_transition','hammerburst_latch_frame','hammerburst_supply_frame','hammerburst_identity_lost',
  'native_equipment_support_withdrawal','hammerburst_feed_history','hammerburst_feed_outcome','hammerburst_feed_rejected','hammerburst_feed_move_rejected',
  'hammerburst_service_loss','hammerburst_stale_debt_wait','hammerburst_reload_complete','hammerburst_early_reload'):
  proof[prefix+'_rows']=rows(prefix)
 (A/f'runtime-{pid}-first-failure-analysis.json').write_text(json.dumps(proof,indent=2))
 review.SOURCE=action;review.OUTPUT=A/f'runtime-{pid}-media-review';review.main(len(media['frames']),pid)
 print(json.dumps(dict(game=pid,frames=len(media['frames']),shots=len(shots),error=proof['replacement'].get('error'),timing=rows('hammerburst_press_timing'),cleanup=result['cleanup'])))
if __name__=='__main__':
 main('hb-cloak-1111-020257',53152,'menu-firstperson-53152-020413','press-timing-r1',
  'Core6 succeeded; entry source age60.337ms, entry checks.7143ms/asset2.7674ms/contract3.0047ms/ButtonName.1751ms/finalreserve.003ms/owned+dispatch.0745ms. No causal first-press repair claim. Initial latch acquired Native magazine grant Frame280, but source stale101.0049ms before HB open; later retained/revoked snapshots stale, no opening transition or feed history. Existing backend source calls fire then follow tick/apply then reload, so place HB-only reload before follow visual work while retaining exact once/current checks. Full flow remains failed.')
