"""Compile exact cosmetic restore admission, without runtime activation."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess

ROOT=Path(__file__).resolve().parents[2];AREA=Path(__file__).resolve().parent
BASE=AREA/'reload-parking-r2';TARGET=AREA/'restore-admission-r3'


def change(source,old,new,count=1):
    assert source.count(old)==count,old[:100]
    return source.replace(old,new)


def main():
    if TARGET.exists():raise RuntimeError('Preserve this named source/build snapshot')
    changes={}
    rel='tools/playground/weapon_hammerburst_runtime.inl';old=(BASE/'source'/rel).read_text();new=old
    new=change(new,'lancerreload_state::Lease parked_native_lease;',
               'lancerreload_state::Lease parked_native_lease;\nnamespace restoration_admission {void cancel() noexcept;}')
    new=change(new,'void reset(bool preserve_geometry=false) noexcept {',
               'void reset(bool preserve_geometry=false) noexcept {\n    restoration_admission::cancel();')
    new=change(new,'#include "weapon_hammerburst_reload_park.inl"',
               '#include "weapon_hammerburst_reload_park.inl"\n#include "weapon_hammerburst_restore_admission.inl"')
    changes[rel]=(old,new)
    rel='tools/playground/playground.cpp';old=(BASE/'source'/rel).read_text()
    new=change(old,'namespace hammerburst_runtime {',
               'namespace hammerburst_runtime {\nbool restoration_owned(gears_weapon_hand::Frame const&) noexcept;\nbool restoration_seek_owned(gears_weapon_hand::Frame const&,std::uint64_t,float) noexcept;')
    changes[rel]=(old,new)
    rel='tools/playground/weapon_hammerburst_visual_clip.inl';old=(BASE/'source'/rel).read_text()
    new=change(old,'!lancerreload_state::item_owned(f)||!nativeequipment::current(nullptr,f)||!prepare_reload_name(f)',
               '(!lancerreload_state::item_owned(f)&&!hammerburst_runtime::restoration_owned(f))||!nativeequipment::current(nullptr,f)||!prepare_reload_name(f)')
    changes[rel]=(old,new)
    rel='tools/playground/weapon_hammerburst_animation.inl';old=(BASE/'source'/rel).read_text()
    new=change(old,'!lancerreload_state::item_owned(source))return Acquire::rejected;',
               '(!lancerreload_state::item_owned(source)&&!hammerburst_runtime::restoration_owned(source)))return Acquire::rejected;',2)
    new=change(new,'!source.left_grip_known||source.left_grip_down||time<ownership.last.time):!lancerreload_state::item_owned(source))||',
               '!source.left_grip_known||source.left_grip_down||time<ownership.last.time):\n            (!lancerreload_state::item_owned(source)&&!hammerburst_runtime::restoration_seek_owned(source,serial,time)))||')
    changes[rel]=(old,new)
    rel='tools/playground/weapon_lancer_magazine_visual.inl';old=(BASE/'source'/rel).read_text()
    new=change(old,'else if(!lancerreload_state::magazine_owned(source))return fail(13);',
               'else if(!lancerreload_state::magazine_owned(source)&&!(hb&&hammerburst_runtime::restoration_owned(source)))return fail(13);')
    changes[rel]=(old,new)
    for name in ('playground','vr-common','third-party','native-capture'):
        shutil.copytree(BASE/'source/tools'/name,TARGET/'source/tools'/name)
    patch=''
    for relative,(before,after) in changes.items():
        (TARGET/'source'/relative).write_text(after)
        patch+=''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+relative,tofile='b/'+relative))
    (ROOT/'docs/hammerburst-restore-admission.patch').write_text(patch)
    for name in ('weapon_hammerburst_restore_admission.inl','weapon_hammerburst_restore_admission_test.cpp'):
        shutil.copy2(ROOT/'tools/playground'/name,TARGET/'source/tools/playground'/name)
    flags=json.loads((BASE/'prepared.json').read_text());configure=list(flags['configure_command']);build=list(flags['build_command'])
    configure[configure.index('-S')+1]=str(TARGET/'source/tools/playground');configure[configure.index('-B')+1]=str(TARGET/'build')
    build[build.index('--build')+1]=str(TARGET/'build')
    for label,command in (('configure',configure),('build',build)):
        with (TARGET/(label+'.log')).open('wb') as log:subprocess.run(command,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    command=(AREA/'build-restore-admission-check.cmd').read_text()
    test='tools/playground/weapon_hammerburst_restore_admission_test.cpp'
    assert str(ROOT/test) in command
    command=command.replace(str(ROOT/test),str(TARGET/'source'/test))
    cmd=TARGET/'check-frozen.cmd';cmd.write_text(command)
    with (TARGET/'test.log').open('wb') as log:subprocess.run(['cmd','/c',str(cmd)],check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    changed=[]
    for name in ('playground','vr-common','third-party','native-capture'):
        for file in (TARGET/'source/tools'/name).rglob('*'):
            if not file.is_file():continue
            relative=file.relative_to(TARGET/'source');prior=BASE/'source'/relative
            if not prior.exists() or file.read_bytes()!=prior.read_bytes():changed.append(relative.as_posix())
    assert len(changed)==7,changed
    dll=TARGET/'build/Release/GearsPlayground6DofUnified3.dll'
    proof=dict(base=str(BASE),changed_source_files=changed,full_playground_compiled=True,frozen_admission_tests_passed=True,
               restoration_dispatch_integrated=False,runtime_ready=False,runtime_exercised=False,shared_default_enabled=False,
               dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build)
    (TARGET/'prepared.json').write_text(json.dumps(proof,indent=2))
    print(json.dumps({k:proof[k] for k in ('full_playground_compiled','frozen_admission_tests_passed','runtime_ready','dll_sha256')},indent=2))


if __name__=='__main__':main()
