"""Connect existing released-debt WAIT to the source-loss release callback."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
A=Path(__file__).resolve().parent;R=A.parents[1];BASE=A/'extraction-observe-r1';P=A/'released-pose-gap-r1'
TEST=r'''
    // Real UP already passed all16 gates; source-loss cleanup must reach the
    // EXISTING debt-only WAIT, not cancel before after_tick can use it.
    {reset_event();auto f=active_frame();actual_first(f);
        model.owned_press=false;model.r2=false;model.bytes[0]=0;f.controllers.hands[1].trigger=0;
        ++f.controllers.hands[1].released[gears_vr::control_trigger];assert(adapter::release_completed(&f));
        assert(adapter::defer(f.weapon.address,address(hb_timer)));
        const auto serial=adapter::event.serial;const auto captured=adapter::event.captured;
        model.pose_stale=true;model.native_source=false;model.now+=110000000;
        const bool released=adapter::release_completed(nullptr);
        if(bridge::cancel_requested||!adapter::owns_pending()){
            std::puts("REPRO: source-loss release canceled previously released finite debt before existing WAIT");return 17;
        }
        assert(released&&adapter::event.serial==serial&&adapter::event.captured==captured);
        adapter::after_tick(nullptr);adapter::deliver(f);
        assert(event_fixture::invocations==1&&model.ammo_used==1);
        model.now+=10000000;model.native_source=true;
        f.now_ns=f.stereo.received_ns=f.controllers.sync_ns=model.now-10000000;
        f.controllers.received_ns=model.now-9000000;++f.key.frame;++f.stereo.sequence;++f.controllers.sequence;
        adapter::after_tick(&f);assert(adapter::service(f.controller.address));muzzle(f);adapter::deliver(f);
        assert(event_fixture::invocations==2&&model.ammo_used==2&&bridge::completion.count()==2);checks+=7;
    }
    for(unsigned invalid=0;invalid<10;++invalid){
        reset_event();auto f=active_frame();actual_first(f);
        model.owned_press=false;model.r2=false;model.bytes[0]=0;f.controllers.hands[1].trigger=0;
        ++f.controllers.hands[1].released[gears_vr::control_trigger];assert(adapter::release_completed(&f));
        assert(adapter::defer(f.weapon.address,address(hb_timer)));
        model.pose_stale=true;model.native_source=false;
        if(invalid==0)model.lifetime_valid=false;if(invalid==1)model.registration=false;
        if(invalid==2)model.pose_stale=false;if(invalid==3)model.owned_press=true;
        if(invalid==4)bridge::cancel_requested=true;if(invalid==5)bridge::first_debt=true;
        if(invalid==6)model.hold=false;if(invalid==7)adapter::clear_released_lifetime();
        if(invalid==8)vrfire::mode.store(vrfire::stopping);
        if(invalid==9)model.now=adapter::event.captured+250000001;
        adapter::release_completed(nullptr);
        assert((bridge::cancel_requested||!adapter::outstanding())&&event_fixture::invocations==1&&model.ammo_used==1);checks+=2;
    }
'''
def run_test(label,expected=0):
 cmd=P/'event-test.cmd';exe=P/'event-test.exe';src=P/'source/tools/playground/weapon_hammerburst_burst_event_test.cpp'
 cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc /DGEARS_PLAYGROUND_HB_STALE_DEBT_WAIT=1 "{src}" /Fo"{P / "event-test.obj"}" /Fe"{exe}"\nif errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
 with (P/(label+'.log')).open('wb') as log:r=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 assert r.returncode==expected,(label,r.returncode)
def main():
 assert not P.exists(),'Preserve candidate'
 for n in ('playground','vr-common','native-capture','third-party'):shutil.copytree(BASE/'source/tools'/n,P/'source/tools'/n)
 test='tools/playground/weapon_hammerburst_burst_event_test.cpp';t=(P/'source'/test).read_text();needle='    std::printf("HB actual event adapter:';assert t.count(needle)==1;t=t.replace(needle,TEST+needle);(P/'source'/test).write_text(t)
 finish()
def finish():
 test='tools/playground/weapon_hammerburst_burst_event_test.cpp'
 run_test('original-repro',17)
 rel='tools/playground/weapon_hammerburst_burst_event.inl';before=(BASE/'source'/rel).read_text();after=before
 needle='''    validated=false;common::press.known=false;common::certified=false;common::pending_validated=false;
    // Only a current real UP'''
 replacement='''    validated=false;common::press.known=false;common::certified=false;common::pending_validated=false;
    // A previous real UP already captured the original lifetime witness.
    // Route a later pose-expiry cleanup through the SAME debt-only WAIT used
    // by service/after_tick. No source, event, deadline or dispatch is created.
    if(!f&&vrfire::mode.load(std::memory_order_acquire)==vrfire::running&&
       !requires_release()&&retain_stale_debt()){
        static unsigned wait_rows{};
        if(log_file&&wait_rows<8){++wait_rows;
            fwprintf(log_file,L"hammerburst_release_debt_wait count=%u event=%llu captured_ns=%lld no_dispatch=1\\n",
                tx::completion.count(),event.serial,event.captured);fflush(log_file);}
        return true;
    }
    // Only a current real UP'''
 assert after.count(needle)==1;after=after.replace(needle,replacement);(P/'source'/rel).write_text(after)
 run_test('fixed-event')
 (R/'docs/hammerburst-released-pose-gap.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel)))
 parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command'])
 configure[configure.index('-S')+1]=str(P/'source/tools/playground');configure[configure.index('-B')+1]=str(P/'build');build[build.index('--build')+1]=str(P/'build')
 for label,cmd in (('configure',configure),('build',build)):
  with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 dll=P/'build/Release/GearsPlayground6DofUnified3.dll'
 proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,native_rebuilt=False,xr_rebuilt=False,baseline_repro_exit=17,additional_event_checks=27,selected_for_runtime=False,actual_failure_repair_proven=False,scope='Only previously real-UP certified finite debt may use existing stale_before lifetime WAIT during later null-source release cleanup. Running mode, original expiry and all lifetime checks required. No stale dispatch, edge replay, deadline renewal or input/identity relaxation; source/20ms reserve still required for any later shot.')
 (P/'prepared.json').write_text(json.dumps(proof,indent=2));(P/'source-verification.json').write_text(json.dumps({n:hashlib.sha256((P/'source'/n).read_bytes()).hexdigest() for n in (rel,test)},indent=2));print(json.dumps({k:proof[k] for k in ('dll_sha256','baseline_repro_exit','additional_event_checks')}))
if __name__=='__main__':main()
