"""Record actual finite drain success and replacement seating failure."""
from pathlib import Path
import hashlib,json,re,shutil
ROOT=Path(__file__).resolve().parents[2];A=Path(__file__).resolve().parent
P=ROOT/'analysis/instances/hb-cloak-1110-233210';ACTION=P/'actions/menu-firstperson-45736-233241'
def read(path):return json.loads(path.read_text(encoding='utf-8-sig'))
def main(instance_name='hb-cloak-1110-233210',game_pid=45736,action_name='menu-firstperson-45736-233241',
         candidate_name='released-debt-r1',context_prefix='hammerburst_reload_transition from=5 to=6',interpretation=None):
    P=ROOT/'analysis/instances'/instance_name;ACTION=P/'actions'/action_name
    result=read(A/instance_name/'result.json');state=read(P/'launcher-session.json')
    assert result['game_identity']['pid']==game_pid
    session_path=Path(state['session_report']);session=read(session_path)
    assert result['cleanup']['owned_game_absent'] and not result['cleanup']['owned_children']['remaining']
    pg=ROOT/'tools/playground'/('build-'+instance_name)/f'playground-{game_pid}.log'
    raw=pg.read_bytes();lines=raw.decode('utf-16' if raw.startswith(b'\xff\xfe') else 'utf-8',errors='replace').splitlines()
    def rows(prefix):return [dict(re.findall(r'(\w+)=([^\s]+)',l)) for l in lines if l.startswith(prefix+' ')]
    empty=read(ACTION/'hammerburst-empty-reload.json')
    replacement=ACTION/'ordinary_empty_replacement/hammerburst-replacement.json'
    at=next(i for i,l in enumerate(lines) if l.startswith(context_prefix))
    archive=A/f'runtime-{game_pid}-text';archive.mkdir(exist_ok=True)
    sources=[pg,ACTION/'result.json',ACTION/'equipment-chain.json',ACTION/'hammerburst-core.json',
             ACTION/'hammerburst-empty-reload.json',replacement,session_path,P/'isolated-done.json',Path(state['test_audio_guard']['log'])]
    copied=[]
    for source in sources:
        target=archive/source.name;shutil.copy2(source,target);digest=hashlib.sha256(source.read_bytes()).hexdigest()
        assert hashlib.sha256(target.read_bytes()).hexdigest()==digest
        copied.append(dict(source=str(source),archive=str(target),sha256=digest))
    shots=rows('native_lancer_shot_trajectory');media=read(ACTION/'recording.json')
    proof=dict(game=result['game_identity'],candidate=read(A/candidate_name/'prepared.json')['dll_sha256'],
        operator_exit=result['operator_exit'],cleanup=result['cleanup'],stop_exit=result['stop_exit'],
        continuous_passed=session['passed'],native_cleanup_verified=session['native_cleanup_verified'],xr_exit_code=session['xr_exit_code'],
        playground_cleanup=state['playground_cleanup'],audio=state['test_audio_guard'],
        drain_complete=empty['drain_loaded_only']['complete'],drain_bursts=empty['drain_loaded_only']['genuine_press_release_bursts'],
        drain_before=empty['drain_loaded_only']['before'],drain_after=empty['drain_loaded_only']['after'],
        empty_checks=empty['checks'],error=empty['error'],replacement=read(replacement),
        actual_trajectory_count=len(shots),trajectory_consistent=all(r.get('trace_valid')=='1' and r.get('trace_source_consistent')=='1' and r.get('write_source_consistent')=='1' for r in shots),
        stale_debt_wait_rows=rows('hammerburst_stale_debt_wait'),service_loss_rows=rows('hammerburst_service_loss'),
        supply_handoff_rows=rows('hammerburst_supply_frame'),
        failure_context=lines[max(0,at-22):at+19],transitions=rows('hammerburst_reload_transition'),
        foreground_samples=result['focus_samples'],cloak_samples=result['cloak_samples'],
        foreground_violations=result['foreground_violations'],cloak_violations=result['cloak_violations'],
        media=dict(frames=len(media['frames']),errors=media['errors'],encoded=(ACTION/'stereo-demo.mp4').exists(),
                   review=str(A/f'runtime-{game_pid}-media-review/prepared.json'),full_fit_accepted=False),
        verified_archive=copied,gameplay_accepted=False,physical_headset_accepted=False,
        interpretation=interpretation or 'Current candidate completed core6 and12 drain bursts72->0/R78 plus empty-trigger veto. Replacement acquired to stage6; same Frame710 read-consumer withdrawal reason7 difference5 (request_ammo and incumbent), then Frame712 expiry. No seating to7 or return-fire. Retained stale-debt wait branch was not observed; no causal repair claim.')
    (A/f'runtime-{game_pid}-first-failure-analysis.json').write_text(json.dumps(proof,indent=2),encoding='utf-8')
    print(json.dumps({k:proof[k] for k in ('drain_complete','drain_bursts','actual_trajectory_count','trajectory_consistent','error','stale_debt_wait_rows','cleanup')}))
if __name__=='__main__':main()
