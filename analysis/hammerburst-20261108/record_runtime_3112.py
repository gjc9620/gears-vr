"""Preserve this terminal private run and its first causal service receipt."""
from pathlib import Path
import hashlib,json,re,shutil

ROOT=Path(__file__).resolve().parents[2]
A=Path(__file__).resolve().parent
INSTANCE=ROOT/'analysis/instances/hb-cloak-1110-225401'
ACTION=INSTANCE/'actions/menu-firstperson-3112-225439'

def read(path):return json.loads(path.read_text(encoding='utf-8-sig'))
def main():
    result=read(A/'hb-cloak-1110-225401/result.json')
    assert result['cleanup']['owned_game_absent'] and not result['cleanup']['owned_children']['remaining']
    state=read(INSTANCE/'launcher-session.json');session=read(Path(state['session_report']))
    pg=ROOT/'tools/playground/build-hb-cloak-1110-225401/playground-3112.log'
    raw=pg.read_bytes();lines=raw.decode('utf-16' if raw.startswith(b'\xff\xfe') else 'utf-8',errors='replace').splitlines()
    def rows(prefix):
        return [dict(re.findall(r'(\w+)=([^\s]+)',line)) for line in lines if line.startswith(prefix+' ')]
    loss=rows('hammerburst_service_loss');assert len(loss)==1 and loss[0]['gate']=='3'
    first=next(i for i,l in enumerate(lines) if l.startswith('hammerburst_service_loss '))
    drain=read(ACTION/'hammerburst-empty-reload.json')['drain_loaded_only']
    shots=rows('native_lancer_shot_trajectory')
    media=read(ACTION/'recording.json')
    archive=A/'runtime-3112-text';archive.mkdir(exist_ok=True)
    source_files=[pg,ACTION/'result.json',ACTION/'equipment-chain.json',ACTION/'hammerburst-core.json',
                  ACTION/'hammerburst-empty-reload.json',Path(state['session_report']),INSTANCE/'isolated-done.json']
    copied=[]
    for source in source_files:
        target=archive/source.name;shutil.copy2(source,target)
        digest=hashlib.sha256(source.read_bytes()).hexdigest();assert hashlib.sha256(target.read_bytes()).hexdigest()==digest
        copied.append(dict(source=str(source),archive=str(target),sha256=digest))
    proof=dict(game=result['game_identity'],candidate=read(A/'service-loss-r1/prepared.json')['dll_sha256'],
        operator_exit=result['operator_exit'],cleanup=result['cleanup'],
        continuous_passed=session.get('passed'),native_cleanup_verified=session.get('native_cleanup_verified'),
        xr_exit_code=session.get('xr_exit_code'),service_loss=loss,
        first_service_loss_context=lines[max(0,first-12):first+5],
        final_drain_typed=drain.get('last_observed_counts'),burst_observations=drain.get('burst_observations'),
        drain_complete=drain.get('complete'),actual_trajectory_count=len(shots),
        trajectory_consistency=all(r.get('trace_valid')=='1' and r.get('trace_source_consistent')=='1' and r.get('write_source_consistent')=='1' for r in shots),
        foreground_samples=result['focus_samples'],cloak_samples=result['cloak_samples'],
        foreground_violations=result['foreground_violations'],cloak_violations=result['cloak_violations'],
        media=dict(frames=len(media['frames']),errors=media['errors'],encoded=(ACTION/'stereo-demo.mp4').exists(),
                   full_fit_accepted=False,review=str(A/'runtime-3112-media-review/prepared.json')),
        verified_archive=copied,
        interpretation='First service cancellation gate is unpublished pose (armed=false), with registration_alive=1 and latest stale_before withdrawal age102.1307ms. This proves the local service trigger; it does not authorize dispatch from expired input or prove all previous partial bursts share this cause.',
        gameplay_accepted=False,physical_headset_accepted=False)
    (A/'runtime-3112-first-failure-analysis.json').write_text(json.dumps(proof,indent=2),encoding='utf-8')
    print(json.dumps({k:proof[k] for k in ('service_loss','actual_trajectory_count','trajectory_consistency','final_drain_typed','cleanup')}))

if __name__=='__main__':main()
