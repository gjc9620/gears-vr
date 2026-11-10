"""Keep failure witness capacity through the full ordinary reload chain."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
ROOT=Path(__file__).resolve().parents[2];AREA=Path(__file__).resolve().parent
BASE=AREA/'source-loss-r1';TARGET=AREA/'source-loss-r2'
def main():
    if TARGET.exists():raise RuntimeError('Preserve named snapshot')
    for name in ('playground','vr-common','third-party','native-capture'):
        shutil.copytree(BASE/'source/tools'/name,TARGET/'source/tools'/name)
    rel='tools/playground/weapon_lancer_r2_native.inl';before=(BASE/'source'/rel).read_text();after=before
    anchor='    const bool hb_pending_before=hammerburst_burst_event::cleanup_pending();';assert after.count(anchor)==1
    after=after.replace(anchor,anchor+'\n    const bool hb_bound_before=nativelancerfollow::bound_is_hammerburst;')
    old='    if(hb_pending_before&&(!have_source||!current))source_withdrawal=weaponhand::client.withdrawal_on_gt();';assert after.count(old)==1
    after=after.replace(old,'    if((hb_pending_before||hb_bound_before)&&(!have_source||!current))source_withdrawal=weaponhand::client.withdrawal_on_gt();')
    (TARGET/'source'/rel).write_text(after)
    patch=''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))
    names=['weapon_hammerburst_source_loss_observation.inl','weapon_hammerburst_input_consumed_observation.inl']
    for name in names:
        relative='tools/playground/'+name;old=(BASE/'source'/relative).read_text();new=(ROOT/relative).read_text()
        patch+=''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='a/'+relative,tofile='b/'+relative))
        shutil.copy2(ROOT/relative,TARGET/'source'/relative)
    (ROOT/'docs/hammerburst-source-loss-full-observation.patch').write_text(patch)
    flags=json.loads((BASE/'prepared.json').read_text());configure=list(flags['configure_command']);build=list(flags['build_command'])
    configure[configure.index('-S')+1]=str(TARGET/'source/tools/playground');configure[configure.index('-B')+1]=str(TARGET/'build')
    build[build.index('--build')+1]=str(TARGET/'build')
    for label,command in (('configure',configure),('build',build)):
        with (TARGET/(label+'.log')).open('wb') as log:subprocess.run(command,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    dll=TARGET/'build/Release/GearsPlayground6DofUnified3.dll'
    proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,
        observation_only=True,clock_reads_added=False,frame_read_retries_added=False,source_or_release_policy_changed=False,
        pending_failure_rows=12,idle_failure_rows=6,consumed_receipt_rows=768,native_rebuilt=False,xr_rebuilt=False,
        dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build)
    (TARGET/'prepared.json').write_text(json.dumps(proof,indent=2));changed=[rel]+['tools/playground/'+n for n in names]
    (TARGET/'source-verification.json').write_text(json.dumps({n:hashlib.sha256((TARGET/'source'/n).read_bytes()).hexdigest() for n in changed},indent=2))
    print(json.dumps({k:proof[k] for k in ('full_playground_compiled','dll_sha256','gameplay_accepted')}),flush=True)
if __name__=='__main__':main()
