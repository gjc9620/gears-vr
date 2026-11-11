"""Actual adapter regressions; queue/engine/certificate/source remain fakes."""
from pathlib import Path
import hashlib,json,subprocess
A=Path(__file__).resolve().parent;P=A/'released-pose-gap-r1'
assert (P/'source/tools/playground/weapon_lancer_r2_native.inl').read_bytes()==(A/'extraction-observe-r1/source/tools/playground/weapon_lancer_r2_native.inl').read_bytes()
r2=(P/'source/tools/playground/weapon_lancer_r2_native.inl').read_text()
assert r2.index('const auto result=input.advance(')<r2.index('hammerburst_burst_event::after_release(frame)')<r2.index('hammerburst_burst_event::after_tick(current?&frame:nullptr)')
pg=P/'source/tools/playground';source=(pg/'weapon_hammerburst_burst_event_test.cpp').read_text()
anchor='    std::printf("HB actual event adapter:';assert source.count(anchor)==1
case=r'''
    for(unsigned mutation=0;mutation<20;++mutation){
        reset_event();auto f=active_frame();actual_first(f);
        model.now+=60000000;f.now_ns=f.stereo.received_ns=f.controllers.sync_ns=model.now-10000000;
        f.controllers.received_ns=model.now-9000000;++f.key.frame;++f.stereo.sequence;++f.controllers.sequence;
        assert(adapter::defer(f.weapon.address,address(hb_timer)));
        assert(adapter::service(f.controller.address)&&adapter::validated);
        const auto serial=adapter::event.serial;const auto captured=adapter::event.captured;
        model.owned_press=false;model.r2=false;model.bytes[0]=0;f.controllers.hands[1].trigger=0;
        ++f.controllers.hands[1].released[gears_vr::control_trigger];
        assert(adapter::release_completed(&f)&&!adapter::validated);
        assert(adapter::event.serial==serial&&adapter::event.captured==captured);
        if(mutation==1)++f.controllers.hands[1].pressed[gears_vr::control_trigger];
        if(mutation==2)++f.controllers.hands[1].released[gears_vr::control_trigger];
        if(mutation==3)bridge::revoke();
        if(mutation==4)++f.reference_generation;
        if(mutation==5)++f.controllers.sync_ns;
        if(mutation==6)++f.key.frame;
        if(mutation==7)model.owned_press=true;
        if(mutation==8)++adapter::event.serial;
        if(mutation==9)++adapter::event.captured;
        if(mutation==10)event_fixture::identity=false;
        if(mutation==11)event_fixture::ready=false;
        if(mutation==12)model.now=captured+250000001;
        if(mutation==13)adapter::clear_release_handoff();
        if(mutation==14)f.controllers.hands[1].trigger=1;
        if(mutation==15)vrfire::mode=vrfire::stopping;
        if(mutation==16)f.controllers.hands[1].grip_flags=0;
        if(mutation==17)++weaponhand::held_binding.generation;
        if(mutation==18)model.registration=false;
        if(mutation==19)model.state={};
        adapter::after_release(f);muzzle(f,true);adapter::deliver(f);
        if(mutation==0){
            assert(event_fixture::invocations==2&&model.burst_count==2&&model.ammo_used==2);
            adapter::after_release(f);adapter::deliver(f);assert(event_fixture::invocations==2);
        }else assert(event_fixture::invocations==1&&model.burst_count==1&&model.ammo_used==1);
        assert(adapter::released_pending.pending.serial==0);checks+=6;
    }
    {reset_event();auto f=active_frame();captured_first(f);assert(adapter::service(f.controller.address));
        // A pending FIRST has no release handoff; authentic UP cancels it.
        model.owned_press=false;model.r2=false;model.bytes[0]=0;f.controllers.hands[1].trigger=0;
        ++f.controllers.hands[1].released[gears_vr::control_trigger];
        assert(adapter::release_completed(&f));adapter::after_release(f);muzzle(f,true);adapter::deliver(f);
        assert(!event_fixture::invocations&&!model.ammo_used);checks+=2;
    }
'''
source=source.replace(anchor,case+anchor);target=pg/'hammerburst_release_handoff_regression_test.cpp';target.write_text(source)
cmd=P/'release-handoff-test.cmd';exe=P/'release-handoff-test.exe'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /DGEARS_PLAYGROUND_HB_STALE_DEBT_WAIT=1 /EHsc "{target}" /Fo"{P / "release-handoff-test.obj"}" /Fe"{exe}"\nif errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
with (P/'release-handoff-test.log').open('wb') as log:result=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
print((P/'release-handoff-test.log').read_text(errors='replace'));result.check_returncode()
(P/'release-handoff-test-result.json').write_text(json.dumps(dict(passed=True,added_cases=21,added_checks=122,total_checks=457,actual_adapter=True,game_execution=False,source_sha256=hashlib.sha256(target.read_bytes()).hexdigest(),limits='Common queue, full retail certificate, source and engine are bounded fakes. Actual Input/delivery production blocks preserved; no game/source/headset acceptance.'),indent=2))
