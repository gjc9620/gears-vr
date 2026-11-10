"""Bounded private repair for the actual3112 stale publication cancellation."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
ROOT=Path(__file__).resolve().parents[2];A=Path(__file__).resolve().parent
BASE=A/'service-loss-r1';TARGET=A/'released-debt-r1'

def replace(text,old,new):
    assert text.count(old)==1,old[:100]
    return text.replace(old,new)

def main():
    assert not TARGET.exists(),'Preserve named frozen candidate'
    for name in ('playground','vr-common','third-party','native-capture'):
        shutil.copytree(BASE/'source/tools'/name,TARGET/'source/tools'/name)
    rel='tools/playground/weapon_hammerburst_burst_event.inl'
    before=(BASE/'source'/rel).read_text();after=before
    after=replace(after,'bool owned_press(hand::Frame const& f) noexcept {',
        '#include "weapon_hammerburst_stale_debt_wait.inl"\nbool owned_press(hand::Frame const& f) noexcept {')
    after=replace(after,'    original_press=common::cohort(f);return original_press.known;',
        '    clear_released_lifetime();original_press=common::cohort(f);return original_press.known;')
    after=replace(after,'gate(16,tx::need_source(*f)))return true;',
        'gate(16,tx::need_source(*f))){remember_released_source(*f);return true;}')
    after=replace(after,'    event={};original_press={};return true;',
        '    event={};original_press={};clear_released_lifetime();return true;')
    after=replace(after,'    after_tick_rejection=0;\n    if(!f)after_tick_rejection=1;',
        '''    after_tick_rejection=0;
    // No dispatch, stale pose reuse or input edge. A new fresh Frame must
    // still pass the original delivery and muzzle checks before another ray.
    if(!f&&vrfire::mode.load(std::memory_order_acquire)==vrfire::running&&
       !requires_release()&&retain_stale_debt())return;
    if(!f)after_tick_rejection=1;''')
    after=replace(after,'service_failed(3,!weaponhand::client.armed())',
        'service_failed(3,!weaponhand::client.armed()&&!retain_stale_debt())')
    (TARGET/'source'/rel).write_text(after)
    helper='tools/playground/weapon_hammerburst_stale_debt_wait.inl';shutil.copy2(ROOT/helper,TARGET/'source'/helper)
    (ROOT/'docs/hammerburst-released-debt-wait.patch').write_text(''.join(difflib.unified_diff(
        before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel)))
    # Fixture seams expose publication loss separately from explicit lifetime
    # revocation. Actual Client identity/epoch behavior remains production code.
    p=TARGET/'source/tools/playground/weapon_hammerburst_burst_cancel_test.cpp';s=p.read_text()
    s=replace(s,'#include "../vr-common/weapon_hand_participant.h"',
        '#include "../vr-common/weapon_hand_participant.h"\n#include "../vr-common/weapon_hand_client.h"')
    s=replace(s,'native_source{true},owned_press{true}',
        'pose_stale{},lifetime_valid{true},native_source{true},owned_press{true}')
    s=replace(s,'    bool armed(){return fixture::model.native_source;}}client;',
        '''    gears_weapon_hand::FrameKey released_key{};
    bool armed(){return fixture::model.native_source;}
    bool capture_existing_action_lifetime_on_gt(gears_weapon_hand::FrameKey const& key,gears_weapon_hand::ExistingActionLifetime& out){
        out={};if(!armed()||!fixture::model.registration)return false;
        released_key=key;out.epoch=1;out.parent.key=key;return true;
    }
    bool existing_action_lifetime_on_gt(gears_weapon_hand::ExistingActionLifetime const& out){
        return out.epoch==1&&fixture::model.lifetime_valid&&fixture::model.registration&&fixture::model.gt;
    }
    gears_weapon_hand::FrameReadObservation withdrawal_on_gt(){
        gears_weapon_hand::FrameReadObservation out{};out.frame=released_key.frame;
        if(fixture::model.pose_stale)out.failure=gears_weapon_hand::FrameReadFailure::stale_before;
        return out;
    }}client;''');p.write_text(s)
    p=TARGET/'source/tools/playground/weapon_hammerburst_burst_event_test.cpp';s=p.read_text()
    block=r'''
    // Actual3112: already accepted physical UP, then pose publication expires.
    // Preserve only the existing debt. There must be no stale-source dispatch.
    {reset_event();auto f=active_frame();actual_first(f);
        model.owned_press=false;model.r2=false;model.bytes[0]=0;f.controllers.hands[1].trigger=0;
        ++f.controllers.hands[1].released[gears_vr::control_trigger];
        assert(adapter::release_completed(&f));
        model.pose_stale=true;model.native_source=false;model.now+=110000000;
        assert(adapter::defer(f.weapon.address,address(hb_timer)));
        const auto serial=adapter::event.serial;const auto captured=adapter::event.captured;
        assert(adapter::service(f.controller.address));adapter::after_tick(nullptr);adapter::deliver(f);
        assert(!bridge::cancel_requested&&adapter::owns_pending());
        assert(event_fixture::invocations==1&&model.ammo_used==1);
        assert(adapter::event.serial==serial&&adapter::event.captured==captured);
        model.now+=10000000;model.native_source=true;
        f.now_ns=f.stereo.received_ns=f.controllers.sync_ns=model.now-10000000;
        f.controllers.received_ns=model.now-9000000;++f.key.frame;++f.stereo.sequence;++f.controllers.sequence;
        adapter::after_tick(&f);assert(adapter::service(f.controller.address));muzzle(f);adapter::deliver(f);
        assert(event_fixture::invocations==2&&model.ammo_used==2&&bridge::completion.count()==2);checks+=8;
    }
    for(unsigned invalid=0;invalid<8;++invalid){
        reset_event();auto f=active_frame();actual_first(f);
        model.owned_press=false;model.r2=false;model.bytes[0]=0;f.controllers.hands[1].trigger=0;
        ++f.controllers.hands[1].released[gears_vr::control_trigger];assert(adapter::release_completed(&f));
        model.pose_stale=true;model.native_source=false;
        if(invalid==0)model.lifetime_valid=false;
        if(invalid==1)model.registration=false;
        if(invalid==2)model.pose_stale=false;
        if(invalid==3)model.owned_press=true;
        if(invalid==4)bridge::cancel_requested=true;
        if(invalid==5)bridge::first_debt=true;
        if(invalid==6)model.hold=false;
        if(invalid==7)adapter::clear_released_lifetime();
        assert(!adapter::retain_stale_debt());assert(event_fixture::invocations==1&&model.ammo_used==1);checks+=2;
    }
    {reset_event();auto f=active_frame();actual_first(f);
        model.owned_press=false;model.r2=false;model.bytes[0]=0;f.controllers.hands[1].trigger=0;
        ++f.controllers.hands[1].released[gears_vr::control_trigger];assert(adapter::release_completed(&f));
        assert(adapter::defer(f.weapon.address,address(hb_timer)));
        const auto captured=adapter::event.captured;
        model.pose_stale=true;model.native_source=false;model.now=captured+250000001;
        assert(adapter::requires_release());assert(adapter::service(f.controller.address));adapter::after_tick(nullptr);
        assert(event_fixture::invocations==1&&model.ammo_used==1);
        assert(!adapter::outstanding()&&lancerrefire::pending_fire.event.empty());checks+=4;
    }
'''
    s=replace(s,'    std::printf("HB actual event adapter:',block+'    std::printf("HB actual event adapter:');p.write_text(s)
    cmake=TARGET/'source/tools/playground/CMakeLists.txt'
    cmake.write_text(cmake.read_text()+'\ntarget_compile_definitions(GearsPlayground PRIVATE GEARS_PLAYGROUND_HB_STALE_DEBT_WAIT=1)\n')
    parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command'])
    configure[configure.index('-S')+1]=str(TARGET/'source/tools/playground');configure[configure.index('-B')+1]=str(TARGET/'build')
    build[build.index('--build')+1]=str(TARGET/'build')
    cmd=A/'build-released-debt-event.cmd';exe=TARGET/'event-test.exe'
    cmd.write_text('@echo off\ncall "E:\\vs\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+
        'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc /DGEARS_PLAYGROUND_HB_STALE_DEBT_WAIT=1 '+
        '"'+str(p)+'" /Fo"'+str(TARGET/'event-test.obj')+'" /Fe"'+str(exe)+'"\nif errorlevel 1 exit /b %errorlevel%\n"'+str(exe)+'"\nexit /b %errorlevel%\n')
    with (TARGET/'event-test.log').open('wb') as log:
        subprocess.run(['cmd','/c',str(cmd)],check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    for label,command in (('configure',configure),('build',build)):
        with (TARGET/(label+'.log')).open('wb') as log:subprocess.run(command,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    dll=TARGET/'build/Release/GearsPlayground6DofUnified3.dll'
    proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,
        dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),
        configure_command=configure,build_command=build,native_rebuilt=False,xr_rebuilt=False,
        scope='Only retain committed original debt after authenticated real UP and exact existing action lifetime, on stale_before withdrawal. No dispatch without fresh source; original100ms/20ms/250ms retained.',
        event_test_log=str(TARGET/'event-test.log'))
    (TARGET/'prepared.json').write_text(json.dumps(proof,indent=2));print(json.dumps(proof,indent=2))

if __name__=='__main__':main()
