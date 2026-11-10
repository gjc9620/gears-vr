"""Preserve the actual failed recording; no manufactured action/video pass."""
from pathlib import Path
import hashlib,json,shutil,re
A=Path(__file__).resolve().parent;ROOT=A.parents[1];NAME='hb-cloak-1111-042322';PID=21288
I=ROOT/'analysis/instances'/NAME;ACTION=I/'actions/menu-firstperson-21288-042751'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
result=read(A/NAME/'result.json');state=read(I/'launcher-session.json');session=read(Path(state['session_report']));media=read(ACTION/'recording.json')
assert result['game_identity']['pid']==PID and result['operator_exit']==1 and result['stop_exit']==0
assert result['cleanup']['owned_game_absent'] and not result['cleanup']['owned_children']['remaining'] and not result['cleanup']['forced_termination']
assert session['native_cleanup_verified'] and session['xr_exit_code']==0 and len(media['frames'])==2 and len(media['errors'])==1
pg=ROOT/'tools/playground'/('build-'+NAME)/f'playground-{PID}.log';b=pg.read_bytes();lines=b.decode('utf-16' if b[:2]==b'\xff\xfe' else 'utf8',errors='replace').splitlines()
def rows(prefix):return [dict(re.findall(r'(\w+)=([^\s]+)',l)) for l in lines if l.startswith(prefix+' ')]
assert not rows('native_lancer_shot_trajectory') and not rows('hammerburst_press_timing') and not (ACTION/'hammerburst-core.json').exists()
sources=[pg,ACTION/'result.json',ACTION/'recording.json',I/'launcher-session.json',Path(state['session_report']),I/'isolated-done.json',Path(state['test_audio_guard']['log']),I/'xr-input/LocalAppData/OpenXR-Simulator/screenshot_status.json']
archive=A/'runtime-21288-text';archive.mkdir(exist_ok=True);copies=[]
for source in sources:
 target=archive/source.name;shutil.copy2(source,target);digest=hashlib.sha256(source.read_bytes()).hexdigest();assert hashlib.sha256(target.read_bytes()).hexdigest()==digest
 copies.append(dict(source=str(source),archive=str(target),sha256=digest))
images=[dict(path=str(ACTION/f['file']),sha256=hashlib.sha256((ACTION/f['file']).read_bytes()).hexdigest()) for f in media['frames']]
assert images[0]['sha256']==images[1]['sha256']
proof=dict(game=result['game_identity'],candidate=read(A/'endpoint-baseline-r1/prepared.json')['dll_sha256'],
 actual_trajectory_count=0,acquisition_attempted=False,core_not_reached=True,press_timing_not_reached=True,release_handoff_not_reached=True,
 first_failure='Private recorder captured two identical startup wait cards then no complete fresh screenshot within unchanged5s. action_live stopped before equipment/core inputs. No first-world/action visual evidence; do not call weapon/driver passed or failed.',
 startup_evidence=session['startup_timings'],media=dict(frames=2,errors=media['errors'],images=images,personally_reviewed_both=True,findings='Both actual saved images are identical STARTING VR waiting cards, not actual gameplay. No overview or all-angle fitting acceptance.',encoded=False,encoding_not_attempted='Failed recorder did not produce frames.txt; retain original errors and images, no passing video fabricated.'),
 cleanup=result['cleanup'],stop_exit=result['stop_exit'],operator_exit=result['operator_exit'],native_cleanup_verified=session['native_cleanup_verified'],xr_exit_code=session['xr_exit_code'],playground_cleanup=state['playground_cleanup'],audio=state['test_audio_guard'],foreground_samples=result['focus_samples'],cloak_samples=result['cloak_samples'],foreground_violations=result['foreground_violations'],cloak_violations=result['cloak_violations'],verified_archive=copies,
 repair='Before starting capture or any weapon action, wait<=15s for existing same-instance/session/game/XR identity-bound first_stereo_submission actual successful xrEndFrame receipt. This is startup evidence only; recorder request5s timeout, source100ms/reserve20ms/event250ms and all action checks unchanged. No writer/retry/input added.',gameplay_accepted=False,physical_headset_accepted=False)
p=A/'runtime-21288-first-failure-analysis.json';p.write_text(json.dumps(proof,indent=2));digest=hashlib.sha256(p.read_bytes()).hexdigest()
p=A/'full-requirements-audit-20261110.json';audit=read(p);audit.update(latest_runtime_failure='runtime-21288-first-failure-analysis.json',candidate_runtime_status='endpoint-baseline-r1 actual21288 stopped on recording error before equipment/core actions. Two personally reviewed images only startup cards; no acquisition/press/handoff exercised. Normal cleanup.',immediate_next='Save identity-bound first-real-projection startup gate/13 helper tests+8 syntax checks, then one fresh instance with unchanged compiled endpoint-baseline-r1. Keep per-frame5s timeout and stop failed recording before actions. Full13 requirements retained.')
for r in audit['requirements']:
 if r['id']=='delivery_headset':r['evidence'].append(dict(path=str(A/'runtime-21288-first-failure-analysis.json'),sha256=digest))
assert len(audit['requirements'])==13 and not audit['completion_proven'];p.write_text(json.dumps(audit,indent=2,ensure_ascii=False))
print(json.dumps(dict(game=PID,images=2,images_are_identical=True,actual_shots=0,core_reached=False,cleanup_normal=True)))
