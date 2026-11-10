"""Compile HB parking integration only. Resume is not implemented in this snapshot."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess,sys

ROOT=Path(__file__).resolve().parents[2];AREA=Path(__file__).resolve().parent
BASE=AREA/'reload-cleanup-r1';TARGET=AREA/'reload-parking-r2'


def change(source,old,new):
    assert source.count(old)==1,old[:100]
    return source.replace(old,new)


def main():
    repairing='--repair-existing' in sys.argv
    if TARGET.exists() and (not repairing or (TARGET/'prepared.json').exists()):raise RuntimeError('Preserve this named private snapshot')
    rel=Path('tools/playground/weapon_hammerburst_runtime.inl')
    original=(BASE/'source'/rel).read_text();source=original
    source=change(source,'enum class Stage {idle,acquiring,opening,opened,old_magazine,waiting_ammo,new_magazine,closing,completing,recovering};',
                  'enum class Stage {idle,acquiring,opening,opened,old_magazine,waiting_ammo,new_magazine,closing,completing,recovering,parking,parked,restoring};')
    source=change(source,'Stage stage{};','Stage stage{};\n#include "weapon_hammerburst_reload_park_ledger.inl"\npark_ledger::Plan parked_plan;\nlancerreload_state::Lease parked_native_lease;')
    source=change(source,'void reset(bool preserve_geometry=false) noexcept {',
                  'void reset(bool preserve_geometry=false) noexcept {\n    const bool release_outstanding=stage!=Stage::parked;')
    source=change(source,'    lancerreload_runtime::outstanding.store(false,std::memory_order_release);',
                  '    if(release_outstanding)lancerreload_runtime::outstanding.store(false,std::memory_order_release);\n    parked_plan={};parked_native_lease={};')
    source=change(source,'#include "weapon_hammerburst_latch_history.inl"',
                  '#include "weapon_hammerburst_reload_park.inl"\n#include "weapon_hammerburst_latch_history.inl"')
    source=change(source,'    if(lancerreload_runtime::stop_requested.load(std::memory_order_acquire)||switch_requested){\n        if(stage!=Stage::idle){cleanup(__LINE__);return true;}return false;\n    }',
                  '''    if(stage==Stage::parked&&lancerreload_runtime::stop_requested.load(std::memory_order_acquire)){
        reset();return false; // No live resources; do not touch another owner's services.
    }
    if(lancerreload_runtime::stop_requested.load(std::memory_order_acquire)){
        if(stage!=Stage::idle){cleanup(__LINE__);return true;}return false;
    }
    if(stage==Stage::parking){
        if(lancerreload_runtime::weapon_retired(opening.identity.weapon))cleanup(__LINE__);
        else park_control::finish();
        return true;
    }
    if(stage==Stage::parked){
        if(lancerreload_runtime::weapon_retired(opening.identity.weapon)){reset();return false;}
        return uptr(vrequip::model.held_weapon())==opening.identity.weapon.address;
    }
    if(switch_requested){
        if(stage!=Stage::idle&&(!park_control::enabled()||(!park_control::begin()&&stage!=Stage::parking)))cleanup(__LINE__);
        return stage!=Stage::idle;
    }''')
    # The last tested private baseline predates the shared independent-owner
    # dispatcher. Do not import unrelated dispatcher/other-weapon changes.
    if 'bool pending_work() noexcept {return stage!=Stage::idle;}' in source:
        source=change(source,'bool pending_work() noexcept {return stage!=Stage::idle;}',
                      'bool pending_work() noexcept {return park_control::pending_resources();}')
    lancer=Path('tools/playground/weapon_lancer_reload_runtime.inl');old_lancer=(BASE/'source'/lancer).read_text()
    new_lancer=change(old_lancer,'bool prepare_stow(hand::Frame const& f,bool& changed) noexcept {',
                     'bool prepare_stow(hand::Frame const& f,bool& changed) noexcept {\n    if(hammerburst_runtime::park_control::owns_stow(f))return hammerburst_runtime::park_control::prepare_stow(f,changed);')
    cpp=Path('tools/playground/playground.cpp');old_cpp=(BASE/'source'/cpp).read_text()
    new_cpp=change(old_cpp,'namespace hammerburst_runtime {',
                   'namespace hammerburst_runtime {\nnamespace park_control {\nbool owns_stow(gears_weapon_hand::Frame const&) noexcept;\nbool prepare_stow(gears_weapon_hand::Frame const&,bool&) noexcept;\n}')
    for name in ('playground','vr-common','third-party','native-capture'):
        if not (TARGET/'source/tools'/name).exists():shutil.copytree(BASE/'source/tools'/name,TARGET/'source/tools'/name)
    (TARGET/'source'/rel).write_text(source);(TARGET/'source'/lancer).write_text(new_lancer)
    (TARGET/'source'/cpp).write_text(new_cpp)
    added=['weapon_hammerburst_reload_park_ledger.inl','weapon_hammerburst_reload_park.inl',
           'weapon_hammerburst_reload_park_ledger_test.cpp','weapon_hammerburst_reload_park_test.cpp']
    for name in added:shutil.copy2(ROOT/'tools/playground'/name,TARGET/'source/tools/playground'/name)
    cmake_source=TARGET/'source/tools/playground/CMakeLists.txt'
    cmake_original=(BASE/'source/tools/playground/CMakeLists.txt').read_text()
    cmake_updated=cmake_original+'\n# Private incomplete parking-development build only.\ntarget_compile_definitions(GearsPlayground PRIVATE GEARS_PLAYGROUND_HB_PARK_RESTORE=1)\n'
    cmake_source.write_text(cmake_updated)
    patch=''
    for path,a,b in ((rel,original,source),(lancer,old_lancer,new_lancer),(cpp,old_cpp,new_cpp)):
        patch+=''.join(difflib.unified_diff(a.splitlines(True),b.splitlines(True),fromfile='a/'+path.as_posix(),tofile='b/'+path.as_posix()))
    (ROOT/'docs/hammerburst-reload-parking.patch').write_text(patch)
    flags=json.loads((AREA/'reload-eligibility-r1/prepared.json').read_text())
    configure=list(flags['configure_command']);configure[configure.index('-S')+1]=str(TARGET/'source/tools/playground');configure[configure.index('-B')+1]=str(TARGET/'build')
    build=list(flags['build_command']);build[build.index('--build')+1]=str(TARGET/'build')
    for label,command in (('configure',configure),('build',build)):
        with (TARGET/(label+('-repair' if repairing else '')+'.log')).open('wb') as log:
            subprocess.run(command,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    raw=(AREA/'build-park-control-check.cmd').read_text()
    test=Path('tools/playground/weapon_hammerburst_reload_park_test.cpp')
    raw=raw.replace(str(ROOT/test),str(TARGET/'source'/test))
    assert str(TARGET/'source'/test) in raw
    cmd=TARGET/'check-frozen.cmd';cmd.write_text(raw)
    with (TARGET/'test.log').open('wb') as log:subprocess.run(['cmd','/c',str(cmd)],check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    dll=TARGET/'build/Release/GearsPlayground6DofUnified3.dll'
    proof=dict(base=str(BASE),full_playground_compiled=True,frozen_parking_tests_passed=True,
               restoration_integrated=False,runtime_ready=False,runtime_exercised=False,selected_by_runtime_entry=False,
               parking_development_enabled=True,shared_default_enabled=False,
               dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),prior_build_failed=repairing,
               configure_command=configure,build_command=build)
    (TARGET/'prepared.json').write_text(json.dumps(proof,indent=2))
    print(json.dumps({k:proof[k] for k in ('full_playground_compiled','frozen_parking_tests_passed','restoration_integrated','runtime_ready','dll_sha256')},indent=2))


if __name__=='__main__':main()
