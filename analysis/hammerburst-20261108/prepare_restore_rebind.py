"""Freeze actual parked-to-restoring rebind; continuation remains incomplete."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess,sys
ROOT=Path(__file__).resolve().parents[2];AREA=Path(__file__).resolve().parent
BASE=AREA/'restore-admission-r3';TARGET=AREA/'restore-rebind-r1'

def change(text,old,new):
    assert text.count(old)==1,old[:80]
    return text.replace(old,new)

def main():
    repairing='--repair-existing' in sys.argv
    if TARGET.exists() and (not repairing or (TARGET/'prepared.json').exists()):raise RuntimeError('Preserve named snapshot')
    for name in ('playground','vr-common','third-party','native-capture'):
        if not (TARGET/'source/tools'/name).exists():shutil.copytree(BASE/'source/tools'/name,TARGET/'source/tools'/name)
    rel='tools/playground/weapon_hammerburst_runtime.inl'
    before=(BASE/'source'/rel).read_text();after=before
    after=change(after,'bool released{},switch_requested{},native_exited{};',
                 'bool released{},switch_requested{},native_exited{};\nbool restore_display_ready{};')
    root=(ROOT/rel).read_text();start=root.index('bool fresh_restore_main(');end=root.index('// All relative measurements',start)
    after=change(after,'// All relative measurements',root[start:end]+'// All relative measurements')
    after=change(after,'    restoration_admission::cancel();','    restoration_admission::cancel();\n    restore_display_ready=false;')
    after=change(after,'#include "weapon_hammerburst_restore_admission.inl"',
                 '#include "weapon_hammerburst_restore_admission.inl"\n#include "weapon_hammerburst_restore_rebind.inl"')
    start=root.index('    if(stage==Stage::parked){\n',root.index('bool tick(uptr pc)'))
    end=root.index('    if(switch_requested){',start)
    old='''    if(stage==Stage::parked){
        if(lancerreload_runtime::weapon_retired(opening.identity.weapon)){reset();return false;}
        return uptr(vrequip::model.held_weapon())==opening.identity.weapon.address;
    }
'''
    after=change(after,old,root[start:end])
    (TARGET/'source'/rel).write_text(after)
    patch=''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))
    (ROOT/'docs/hammerburst-restore-rebind.patch').write_text(patch)
    added=['weapon_hammerburst_restore_rebind.inl','weapon_hammerburst_restore_rebind_test.cpp']
    for name in added:shutil.copy2(ROOT/'tools/playground'/name,TARGET/'source/tools/playground'/name)
    # Carry the independently verified established-hold repair into this dev snapshot.
    shutil.copy2(ROOT/'tools/playground/weapon_equipment_runtime.h',TARGET/'source/tools/playground/weapon_equipment_runtime.h')
    tests=[]
    for stem in ('restore-rebind','restore-admission','park-control'):
        cmd=(AREA/('build-'+stem+'-check.cmd')).read_text().replace(str(ROOT/'tools/playground'),str(TARGET/'source/tools/playground'))
        path=TARGET/(stem+'-check.cmd');path.write_text(cmd)
        with (TARGET/(stem+'-test.log')).open('wb') as log:
            subprocess.run(['cmd','/c',str(path)],check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
        tests.append((TARGET/(stem+'-test.log')).read_text())
    flags=json.loads((BASE/'prepared.json').read_text());configure=list(flags['configure_command']);build=list(flags['build_command'])
    configure[configure.index('-S')+1]=str(TARGET/'source/tools/playground');configure[configure.index('-B')+1]=str(TARGET/'build')
    build[build.index('--build')+1]=str(TARGET/'build')
    for label,command in (('configure',configure),('build',build)):
        with (TARGET/(label+('-repair' if repairing else '')+'.log')).open('wb') as log:subprocess.run(command,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    dll=TARGET/'build/Release/GearsPlayground6DofUnified3.dll'
    changed=[rel,'tools/playground/weapon_equipment_runtime.h']+['tools/playground/'+n for n in added]
    proof=dict(base=str(BASE),full_playground_compiled=True,frozen_rebind_admission_parking_tests_passed=True,
        restoration_rebind_integrated=True,restoration_display_integrated=False,fresh_intent_resume_integrated=False,
        runtime_ready=False,runtime_exercised=False,selected_by_runtime_entry=False,shared_default_enabled=False,
        dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),changed_source_files=changed,
        configure_command=configure,build_command=build,first_build_failed=repairing,
        first_build_failure='C4459 local observed shadowed existing global; renamed local, retained /WX' if repairing else None)
    (TARGET/'prepared.json').write_text(json.dumps(proof,indent=2))
    (TARGET/'source-verification.json').write_text(json.dumps({n:hashlib.sha256((TARGET/'source'/n).read_bytes()).hexdigest() for n in changed},indent=2))
    print(''.join(tests));print(json.dumps({k:proof[k] for k in ('full_playground_compiled','runtime_ready','dll_sha256')}))

if __name__=='__main__':main()
