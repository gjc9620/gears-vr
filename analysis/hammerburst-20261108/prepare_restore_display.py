"""Compile actual cosmetic restoration, still excluding fresh-intent resume."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess,sys
ROOT=Path(__file__).resolve().parents[2];AREA=Path(__file__).resolve().parent
BASE=AREA/'restore-rebind-r1';TARGET=AREA/'restore-display-r1'
def change(text,old,new):
    assert text.count(old)==1,old[:100]
    return text.replace(old,new)
def main():
    repairing='--repair-existing' in sys.argv
    if TARGET.exists() and (not repairing or (TARGET/'prepared.json').exists()):raise RuntimeError('Preserve named snapshot')
    for name in ('playground','vr-common','third-party','native-capture'):
        if not (TARGET/'source/tools'/name).exists():shutil.copytree(BASE/'source/tools'/name,TARGET/'source/tools'/name)
    rel='tools/playground/weapon_hammerburst_runtime.inl'
    before=(BASE/'source'/rel).read_text();after=before;root=(ROOT/rel).read_text()
    after=change(after,'bool restore_display_ready{};','bool restore_display_ready{};\nunsigned restore_display_step{};\nbool restoration_failed{};')
    after=change(after,'    restore_display_ready=false;','    restore_display_ready=false;\n    restore_display_step=0;restoration_failed=false;')
    after=change(after,'#include "weapon_hammerburst_restore_rebind.inl"','#include "weapon_hammerburst_restore_rebind.inl"\n#include "weapon_hammerburst_restore_display.inl"')
    def restoring(text):
        start=text.index('    if(stage==Stage::restoring){',text.index('bool tick(uptr pc)'))
        end=text.index('    if(switch_requested){\n        if(stage!=Stage::idle',start)
        return text[start:end]
    after=change(after,restoring(before),restoring(root))
    (TARGET/'source'/rel).write_text(after)
    patch=''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))
    names=['weapon_hammerburst_restore_display.inl','weapon_hammerburst_restore_display_test.cpp',
        'weapon_hammerburst_restore_fixture.inl','weapon_hammerburst_restore_rebind.inl',
        'weapon_hammerburst_restore_rebind_test.cpp','weapon_hammerburst_reload_park.inl',
        'weapon_hammerburst_reload_park_test.cpp']
    for name in names:
        relative='tools/playground/'+name;new=(ROOT/relative).read_text();old=BASE/'source'/relative
        if old.exists():patch+=''.join(difflib.unified_diff(old.read_text().splitlines(True),new.splitlines(True),fromfile='a/'+relative,tofile='b/'+relative))
        shutil.copy2(ROOT/relative,TARGET/'source'/relative)
    (ROOT/'docs/hammerburst-restore-display.patch').write_text(patch)
    for stem in ('restore-display','restore-rebind','restore-admission','park-control'):
        command=(AREA/('build-'+stem+'-check.cmd')).read_text().replace(str(ROOT/'tools/playground'),str(TARGET/'source/tools/playground'))
        path=TARGET/(stem+'-check.cmd');path.write_text(command)
        with (TARGET/(stem+'-test.log')).open('wb') as log:
            subprocess.run(['cmd','/c',str(path)],check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
        print((TARGET/(stem+'-test.log')).read_text(),flush=True)
    flags=json.loads((BASE/'prepared.json').read_text());configure=list(flags['configure_command']);build=list(flags['build_command'])
    configure[configure.index('-S')+1]=str(TARGET/'source/tools/playground');configure[configure.index('-B')+1]=str(TARGET/'build')
    build[build.index('--build')+1]=str(TARGET/'build')
    for label,command in (('configure',configure),('build',build)):
        with (TARGET/(label+('-repair' if repairing else '')+'.log')).open('wb') as log:subprocess.run(command,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    dll=TARGET/'build/Release/GearsPlayground6DofUnified3.dll';changed=[rel]+['tools/playground/'+n for n in names]
    proof=dict(base=str(BASE),full_playground_compiled=True,frozen_display_rebind_admission_parking_tests_passed=True,
        restoration_display_integrated=True,fresh_intent_resume_integrated=False,runtime_ready=False,
        runtime_exercised=False,selected_by_runtime_entry=False,shared_default_enabled=False,
        dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),changed_source_files=changed,
        configure_command=configure,build_command=build,first_build_failed=repairing,
        first_build_failure='Preparation script truncated restoring branch at nested switch; corrected exact outer anchor; failed build log preserved' if repairing else None)
    (TARGET/'prepared.json').write_text(json.dumps(proof,indent=2))
    (TARGET/'source-verification.json').write_text(json.dumps({n:hashlib.sha256((TARGET/'source'/n).read_bytes()).hexdigest() for n in changed},indent=2))
    print(json.dumps({k:proof[k] for k in ('full_playground_compiled','runtime_ready','dll_sha256')}),flush=True)
if __name__=='__main__':main()
