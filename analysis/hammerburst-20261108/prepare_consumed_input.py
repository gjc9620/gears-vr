"""Add passive bounded input-consumption receipts to the compiled HB chain."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
ROOT=Path(__file__).resolve().parents[2];AREA=Path(__file__).resolve().parent
BASE=AREA/'restore-resume-r3';TARGET=AREA/'consumed-input-r2'
def main():
    if TARGET.exists():raise RuntimeError('Preserve named candidate')
    for name in ('playground','vr-common','third-party','native-capture'):
        shutil.copytree(BASE/'source/tools'/name,TARGET/'source/tools'/name)
    relative='tools/playground/weapon_lancer_r2_native.inl'
    before=(BASE/'source'/relative).read_text();anchor='    const auto diagnostic_finished=weaponhand::clock_ns(nullptr);'
    assert before.count(anchor)==1
    after=before.replace(anchor,anchor+'\n#include "weapon_hammerburst_input_consumed_observation.inl"')
    (TARGET/'source'/relative).write_text(after)
    helper='tools/playground/weapon_hammerburst_input_consumed_observation.inl'
    shutil.copy2(ROOT/helper,TARGET/'source'/helper)
    (ROOT/'docs/hammerburst-input-consumed-observation.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+relative,tofile='b/'+relative)))
    flags=json.loads((BASE/'prepared.json').read_text());configure=list(flags['configure_command']);build=list(flags['build_command'])
    configure[configure.index('-S')+1]=str(TARGET/'source/tools/playground');configure[configure.index('-B')+1]=str(TARGET/'build')
    build[build.index('--build')+1]=str(TARGET/'build')
    for label,command in (('configure',configure),('build',build)):
        with (TARGET/(label+'.log')).open('wb') as log:subprocess.run(command,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    dll=TARGET/'build/Release/GearsPlayground6DofUnified3.dll'
    proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,runtime_ready_for_owned_validation=True,
        observation_only=True,source_clocks_renewed=False,input_deadlines_changed=False,gameplay_accepted=False,
        observation_after_input_advance=True,baseline_interval_ms=250,maximum_rows=192,native_rebuilt=False,xr_rebuilt=False,
        dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build)
    (TARGET/'prepared.json').write_text(json.dumps(proof,indent=2))
    (TARGET/'source-verification.json').write_text(json.dumps({n:hashlib.sha256((TARGET/'source'/n).read_bytes()).hexdigest() for n in (relative,helper)},indent=2))
    print(json.dumps({k:proof[k] for k in ('full_playground_compiled','dll_sha256','gameplay_accepted')}),flush=True)
if __name__=='__main__':main()
