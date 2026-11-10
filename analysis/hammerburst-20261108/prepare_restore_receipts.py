"""Instrument existing restore transitions, without changing their decisions."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
ROOT=Path(__file__).resolve().parents[2];AREA=Path(__file__).resolve().parent
BASE=AREA/'consumed-input-r2';TARGET=AREA/'restore-receipts-r1'
def main():
    if TARGET.exists():raise RuntimeError('Preserve named private snapshot')
    for name in ('playground','vr-common','third-party','native-capture'):
        shutil.copytree(BASE/'source/tools'/name,TARGET/'source/tools'/name)
    rel='tools/playground/weapon_hammerburst_runtime.inl';before=(BASE/'source'/rel).read_text()
    anchor='unsigned geometry_failure{},last_geometry_failure{~0u};';assert before.count(anchor)==1
    after=before.replace(anchor,anchor+'\n#include "weapon_hammerburst_restore_log.inl"')
    (TARGET/'source'/rel).write_text(after)
    patch=''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))
    names=['weapon_hammerburst_restore_log.inl','weapon_hammerburst_reload_park.inl',
        'weapon_hammerburst_restore_rebind.inl','weapon_hammerburst_restore_display.inl',
        'weapon_hammerburst_restore_resume.inl','weapon_hammerburst_restore_fixture.inl',
        'weapon_hammerburst_reload_park_test.cpp']
    for name in names:
        relative='tools/playground/'+name;new=(ROOT/relative).read_text();old=BASE/'source'/relative
        if old.exists():patch+=''.join(difflib.unified_diff(old.read_text().splitlines(True),new.splitlines(True),fromfile='a/'+relative,tofile='b/'+relative))
        shutil.copy2(ROOT/relative,TARGET/'source'/relative)
    (ROOT/'docs/hammerburst-restore-receipts.patch').write_text(patch)
    for stem in ('restore-resume','park-control'):
        command=(AREA/('build-'+stem+'-check.cmd')).read_text().replace(str(ROOT/'tools/playground'),str(TARGET/'source/tools/playground'))
        path=TARGET/(stem+'-check.cmd');path.write_text(command)
        with (TARGET/(stem+'-test.log')).open('wb') as log:subprocess.run(['cmd','/c',str(path)],check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    flags=json.loads((BASE/'prepared.json').read_text());configure=list(flags['configure_command']);build=list(flags['build_command'])
    configure[configure.index('-S')+1]=str(TARGET/'source/tools/playground');configure[configure.index('-B')+1]=str(TARGET/'build')
    build[build.index('--build')+1]=str(TARGET/'build')
    for label,command in (('configure',configure),('build',build)):
        with (TARGET/(label+'.log')).open('wb') as log:subprocess.run(command,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    dll=TARGET/'build/Release/GearsPlayground6DofUnified3.dll';changed=[rel]+['tools/playground/'+n for n in names]
    proof=dict(base=str(BASE),full_playground_compiled=True,frozen_resume_parking_tests_passed=True,
        runtime_ready_for_owned_validation=True,runtime_exercised=False,gameplay_accepted=False,
        observer_only=True,new_clock_reads=False,maximum_transition_rows=64,native_rebuilt=False,xr_rebuilt=False,
        dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),changed_source_files=changed,
        configure_command=configure,build_command=build)
    (TARGET/'prepared.json').write_text(json.dumps(proof,indent=2))
    (TARGET/'source-verification.json').write_text(json.dumps({n:hashlib.sha256((TARGET/'source'/n).read_bytes()).hexdigest() for n in changed},indent=2))
    print(json.dumps({k:proof[k] for k in ('full_playground_compiled','dll_sha256','gameplay_accepted')}),flush=True)
if __name__=='__main__':main()
