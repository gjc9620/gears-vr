"""Preserve pre-Native input-start failure; not a weapon or cleanup pass."""
from pathlib import Path
import hashlib,json,shutil
ROOT=Path(__file__).resolve().parents[2];A=Path(__file__).resolve().parent
P=ROOT/'analysis/instances/hb-cloak-1110-232025'
def read(path):return json.loads(path.read_text(encoding='utf-8-sig'))
def main():
    wrapper=A/'hb-cloak-1110-232025/result.json';outcome=read(wrapper)
    state=read(P/'launcher-session.json');session_path=Path(state['session_report']);session=read(session_path)
    assert outcome['game_identity']['pid']==49732 and not outcome['operator_started']
    assert not session['native_started'] and outcome['cleanup']['owned_game_absent']
    trace=session['exception_traceback'];assert 'FrozenThreads' in trace and 'WinError 87' in trace
    sources=[wrapper,session_path,P/'launcher-session.json',P/'isolated-done.json',P/'manifest.json',
             Path(state['test_audio_guard']['log'])]
    archive=A/'runtime-49732-startup-text';archive.mkdir(exist_ok=True);proofs=[]
    for source in sources:
        target=archive/source.name;shutil.copy2(source,target);digest=hashlib.sha256(source.read_bytes()).hexdigest()
        assert hashlib.sha256(target.read_bytes()).hexdigest()==digest
        proofs.append(dict(source=str(source),archive=str(target),sha256=digest))
    result=dict(game=outcome['game_identity'],candidate=read(A/'released-debt-r1/prepared.json')['dll_sha256'],
        operator_started=False,native_started=False,weapon_actions_exercised=False,
        error=session['error'],exception_traceback=trace,cleanup=outcome['cleanup'],
        stop_exit=outcome['stop_exit'],playground_cleanup=state['playground_cleanup'],prime_cleanup=state['prime_cleanup'],
        xr_exit_code=session['xr_exit_code'],native_cleanup_verified=session.get('native_cleanup_verified',False),
        foreground_samples=outcome['focus_samples'],cloak_samples=outcome['cloak_samples'],
        foreground_violations=outcome['foreground_violations'],cloak_violations=outcome['cloak_violations'],
        audio=state['test_audio_guard'],verified_archive=proofs,
        background_input_file_observed_after_run_sha256=hashlib.sha256((ROOT/'tools/game-io/background_input.py').read_bytes()).hexdigest(),
        cause='OpenThread failed after thread enumeration with WinError87. Thread-exit race is a hypothesis; no failed TID or closing snapshot was recorded, so it is not proven.',
        next='Coordinate bounded shared startup-tool repair/evidence; do not rerun unchanged weapon test or weaken thread/code/input guards.',
        gameplay_accepted=False,physical_headset_accepted=False)
    (A/'runtime-49732-startup-failure.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    print(json.dumps({k:result[k] for k in ('operator_started','native_started','error','cleanup','stop_exit','xr_exit_code')}))
if __name__=='__main__':main()
