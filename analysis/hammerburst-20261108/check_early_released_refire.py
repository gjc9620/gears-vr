from pathlib import Path
import json,subprocess,shutil
from prepare_early_released_refire import HELPER
A=Path(__file__).resolve().parent;P=A/'early-released-refire-r1';BASE=A/'history-origin-r1';S=P/'source/tools/playground'
s=(S/'weapon_hammerburst_burst_event.inl').read_text();assert s.count(HELPER)==1 and s.replace(HELPER,'')==(BASE/'source/tools/playground/weapon_hammerburst_burst_event.inl').read_text()
s=(S/'weapon_lancer_r2_native.inl').read_text();begin=s.index('    if(!unified_weapon_fire::ready_for(frame,20000000)){');end=s.index('\n}\n#endif\nvoid tick(uptr pc)',begin)
old='''    if(unified_weapon_fire::ready_for(frame,20000000))return;
    nativelancerfollow::tick(frame,nativelancerfollow::Phase::prepare_only);
    if(!unified_weapon_fire::prepare_current(frame,game))unified_weapon_fire::clear_source();'''
assert s[:begin]+old+s[end:]==(BASE/'source/tools/playground/weapon_lancer_r2_native.inl').read_text()
s=(S/'weapon_hammerburst_burst_runtime.h').read_text().replace('void pre_original_released_refire(gears_weapon_hand::Frame const&) noexcept;\n','');assert s==(BASE/'source/tools/playground/weapon_hammerburst_burst_runtime.h').read_text()
s=(S/'weapon_lancer_r2_native.inl').read_text()
for rel in ('tools/playground/playground.cpp','tools/playground/weapon_hammerburst_burst_runtime.inl','tools/vr-common/weapon_hammerburst_burst.h','tools/playground/weapon_equipment_backend.inl'):
 assert (P/'source'/rel).read_bytes()==(BASE/'source'/rel).read_bytes()
assert s[begin:end].index('prepare_current(frame,game)')<s[begin:end].index('pre_original_released_refire(frame)')
source=(S/'weapon_hammerburst_burst_event_test.cpp').read_text();anchor='    std::printf("HB actual event adapter:';assert source.count(anchor)==1
case=r'''
    for(unsigned mutation=0;mutation<22;++mutation){
        reset_event();auto f=active_frame();actual_first(f);
        model.now+=60000000;f.now_ns=f.stereo.received_ns=f.controllers.sync_ns=model.now-10000000;
        f.controllers.received_ns=model.now-9000000;++f.key.frame;++f.stereo.sequence;++f.controllers.sequence;
        assert(adapter::defer(f.weapon.address,address(hb_timer)));
        model.owned_press=false;model.r2=false;model.bytes[0]=0;f.controllers.hands[1].trigger=0;
        ++f.controllers.hands[1].released[gears_vr::control_trigger];
        assert(adapter::release_completed(&f)&&adapter::released_lifetime.epoch);
        const auto serial=adapter::event.serial;const auto captured=adapter::event.captured;
        model.now+=180000000;f.now_ns=f.stereo.received_ns=f.controllers.sync_ns=model.now-10000000;
        f.controllers.received_ns=model.now-9000000;++f.key.frame;++f.stereo.sequence;++f.controllers.sequence;
        muzzle(f,true);
        if(mutation==1)model.owned_press=true;
        if(mutation==2)adapter::clear_released_lifetime();
        if(mutation==3)bridge::first_debt=true;
        if(mutation==4)bridge::cancel_requested=true;
        if(mutation==5)bridge::firing_debt=false;
        if(mutation==6)bridge::completion={};
        if(mutation==7)adapter::event.kind=lancerrefire::policy::EventKind::first;
        if(mutation==8)f.controllers.hands[1].trigger_active=0;
        if(mutation==9)f.controllers.hands[1].trigger=1;
        if(mutation==10)event_fixture::identity=false;
        if(mutation==11)model.native_source=false;
        if(mutation==12)f.controllers.hands[1].grip_flags=0;
        if(mutation==13)++weaponhand::held_binding.generation;
        if(mutation==14)model.registration=false;
        if(mutation==15)vrfire::mode=vrfire::stopping;
        if(mutation==16)model.now=captured+250000001;
        if(mutation==17)model.now+=100000001;
        if(mutation==18)event_fixture::ready=false;
        if(mutation==19)++f.reference_generation;
        if(mutation==20)++f.controllers.hands[1].pressed[gears_vr::control_trigger];
        if(mutation==21)++f.controllers.hands[1].released[gears_vr::control_trigger];
        if(mutation==12||mutation>=19)muzzle(f,true);
        adapter::pre_original_released_refire(f);
        if(mutation==0){
            assert(event_fixture::invocations==2&&model.burst_count==2&&model.ammo_used==2);
            assert(!adapter::event.serial&&lancerrefire::pending_fire.event.empty());
            adapter::pre_original_released_refire(f);adapter::service(f.controller.address);adapter::deliver(f);
            assert(event_fixture::invocations==2&&model.ammo_used==2);
        }else assert(event_fixture::invocations==1&&model.ammo_used==1);
        if(mutation==1||mutation==2||mutation==3||mutation==5||mutation==8||mutation==9){
            assert(adapter::event.serial==serial&&adapter::event.captured==captured);}
        checks+=4;
    }
    // Actual old/new scheduling comparison: same captured event, fresh early
    // source at180ms; later work reaches310ms. No renewed event or timestamp.
    for(bool early:{false,true}){
        reset_event();auto f=active_frame();actual_first(f);
        model.now+=60000000;f.now_ns=f.stereo.received_ns=f.controllers.sync_ns=model.now-10000000;
        f.controllers.received_ns=model.now-9000000;++f.key.frame;++f.stereo.sequence;++f.controllers.sequence;
        assert(adapter::defer(f.weapon.address,address(hb_timer)));
        const auto captured=adapter::event.captured;
        model.owned_press=false;model.r2=false;model.bytes[0]=0;f.controllers.hands[1].trigger=0;
        ++f.controllers.hands[1].released[gears_vr::control_trigger];assert(adapter::release_completed(&f));
        model.now=captured+180000000;f.now_ns=f.stereo.received_ns=f.controllers.sync_ns=model.now-10000000;
        f.controllers.received_ns=model.now-9000000;++f.key.frame;++f.stereo.sequence;++f.controllers.sequence;muzzle(f,true);
        if(early)adapter::pre_original_released_refire(f);
        model.now=captured+310000000;adapter::service(f.controller.address);adapter::deliver(f);
        assert(event_fixture::invocations==(early?2u:1u)&&model.ammo_used==(early?2u:1u));checks+=2;
    }
'''
source=source.replace(anchor,case+anchor);target=S/'hammerburst_early_released_refire_test.cpp';target.write_text(source);cmd=P/'early-refire-test.cmd';exe=P/'early-refire-test.exe'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /DGEARS_PLAYGROUND_HB_STALE_DEBT_WAIT=1 /EHsc "{target}" /Fo"{P / "early-refire-test.obj"}" /Fe"{exe}"\nif errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
with (P/'early-refire-test.log').open('wb') as log:r=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
print((P/'early-refire-test.log').read_text(errors='replace'));r.check_returncode()
for n in ('tick-cost-test-result.json','history-observer-test-result.json','withdrawal-clock-test-result.json','history-origin-test-result.json'):shutil.copy2(BASE/n,P/n)
(P/'early-refire-test-result.json').write_text(json.dumps(dict(passed=True,added_cases=24,added_checks=92,actual_event_adapter=True,exact_original_service_delivery_and_claim=True,original_policy_unchanged=True,new_input_or_event=False,clock_renewal=False,game_started=False,limits='Actual adapter/finite ledger with bounded engine/source/certificate fakes, scheduling comparison synthetic. Does not prove game execution or6-shot full-flow completion.'),indent=2))
