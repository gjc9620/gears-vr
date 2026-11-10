"""Existing client read-failure witness for the actual partial-burst loss."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
ROOT=Path(__file__).resolve().parents[2];AREA=Path(__file__).resolve().parent
BASE=AREA/'restore-receipts-r1';TARGET=AREA/'source-loss-r1'
def change(text,old,new):
    assert text.count(old)==1,old[:80]
    return text.replace(old,new)
def main():
    if TARGET.exists():raise RuntimeError('Preserve named candidate')
    for name in ('playground','vr-common','third-party','native-capture'):
        shutil.copytree(BASE/'source/tools'/name,TARGET/'source/tools'/name)
    rel='tools/playground/weapon_lancer_r2_native.inl';before=(BASE/'source'/rel).read_text();after=before
    after=change(after,'    const bool have_source=state==running&&weaponhand::client.copy_frame_on_gt(frame,copied_game,copied_decision);',
        '    wh::FrameReadObservation copy_observation{};\n    const bool hb_pending_before=hammerburst_burst_event::cleanup_pending();\n    const bool r2_owned_before=input.owns_press();\n    const bool have_source=state==running&&weaponhand::client.copy_frame_on_gt(frame,copied_game,copied_decision,&copy_observation);')
    anchor='    if(!current&&input.has_actors()&&input.actors().controller.address==pc)read_game(input.actors(),nullptr,game);'
    after=change(after,anchor,anchor+'\n    wh::FrameReadObservation source_withdrawal{};\n    if(hb_pending_before&&(!have_source||!current))source_withdrawal=weaponhand::client.withdrawal_on_gt();')
    after=change(after,'#include "weapon_hammerburst_input_consumed_observation.inl"',
        '#include "weapon_hammerburst_input_consumed_observation.inl"\n#include "weapon_hammerburst_source_loss_observation.inl"')
    (TARGET/'source'/rel).write_text(after)
    helper='tools/playground/weapon_hammerburst_source_loss_observation.inl';shutil.copy2(ROOT/helper,TARGET/'source'/helper)
    (ROOT/'docs/hammerburst-source-loss-observation.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel)))
    flags=json.loads((BASE/'prepared.json').read_text());configure=list(flags['configure_command']);build=list(flags['build_command'])
    configure[configure.index('-S')+1]=str(TARGET/'source/tools/playground');configure[configure.index('-B')+1]=str(TARGET/'build')
    build[build.index('--build')+1]=str(TARGET/'build')
    for label,command in (('configure',configure),('build',build)):
        with (TARGET/(label+'.log')).open('wb') as log:subprocess.run(command,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    dll=TARGET/'build/Release/GearsPlayground6DofUnified3.dll'
    proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,
        observation_only=True,existing_copy_observation=True,existing_withdrawal_snapshot=True,
        new_clock_reads=False,read_retries_added=False,source_or_release_policy_changed=False,maximum_rows=12,
        native_rebuilt=False,xr_rebuilt=False,dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),
        configure_command=configure,build_command=build)
    (TARGET/'prepared.json').write_text(json.dumps(proof,indent=2))
    (TARGET/'source-verification.json').write_text(json.dumps({n:hashlib.sha256((TARGET/'source'/n).read_bytes()).hexdigest() for n in (rel,helper)},indent=2))
    print(json.dumps({k:proof[k] for k in ('full_playground_compiled','dll_sha256','gameplay_accepted')}),flush=True)
if __name__=='__main__':main()
