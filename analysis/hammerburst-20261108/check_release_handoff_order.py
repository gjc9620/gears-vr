"""Reproduce service-before-UP validation loss in the actual frozen adapter.

This does not assert that game22896 had its refire pending at Frame207; its
current log lacks that exact event-capture witness. Engine/common queue fake.
"""
from pathlib import Path
import hashlib,json,subprocess
A=Path(__file__).resolve().parent;P=A/'first-priority-r1';PG=P/'source/tools/playground'
source=(PG/'weapon_hammerburst_burst_event_test.cpp').read_text()
anchor='    std::printf("HB actual event adapter:'
assert source.count(anchor)==1
case=r'''
    // Real PlayerTick ordering: service validates a captured refire BEFORE
    // Input::advance invokes successful real-UP release_completed.
    for(bool expire:{false,true}){
        reset_event();auto f=active_frame();actual_first(f);
        model.now+=60000000;f.now_ns=f.stereo.received_ns=f.controllers.sync_ns=model.now-10000000;
        f.controllers.received_ns=model.now-9000000;++f.key.frame;++f.stereo.sequence;++f.controllers.sequence;
        assert(adapter::defer(f.weapon.address,address(hb_timer)));
        const auto serial=adapter::event.serial,captured=std::uint64_t(adapter::event.captured);
        assert(adapter::service(f.controller.address)&&adapter::validated);
        model.owned_press=false;model.r2=false;model.bytes[0]=0;f.controllers.hands[1].trigger=0;
        ++f.controllers.hands[1].released[gears_vr::control_trigger];
        assert(adapter::release_completed(&f)&&!adapter::validated);
        assert(adapter::event.serial==serial&&std::uint64_t(adapter::event.captured)==captured);
        adapter::after_tick(&f);muzzle(f,true);adapter::deliver(f);
        assert(event_fixture::invocations==1&&model.burst_count==1&&adapter::delivery_stage==1);
        if(expire){
            model.now=std::int64_t(captured)+250000001;
            assert(adapter::service(f.controller.address));
            assert(event_fixture::invocations==1&&adapter::event.serial==0);
        }else{
            assert(adapter::service(f.controller.address)&&adapter::validated);
            adapter::deliver(f);adapter::deliver(f);
            assert(event_fixture::invocations==2&&model.burst_count==2&&model.ammo_used==2);
        }
        checks+=7;
    }
'''
source=source.replace(anchor,case+anchor);target=PG/'hammerburst_release_handoff_order_test.cpp';target.write_text(source)
cmd=P/'release-handoff-test.cmd';exe=P/'release-handoff-test.exe'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /DGEARS_PLAYGROUND_HB_STALE_DEBT_WAIT=1 /EHsc "{target}" /Fo"{P / "release-handoff-test.obj"}" /Fe"{exe}"\nif errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
with (P/'release-handoff-test.log').open('wb') as log:
 result=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
print((P/'release-handoff-test.log').read_text(errors='replace'));result.check_returncode()
(P/'release-handoff-test-result.json').write_text(json.dumps(dict(passed=True,added_checks=14,actual_adapter=True,game_execution=False,source_sha256=hashlib.sha256(target.read_bytes()).hexdigest(),finding='Service-validated existing refire is deliberately invalidated by real-UP release_completed. Original after_tick/deliver do not revalidate it that Frame; a new service succeeds with same event before expiry, expires without shot after250ms.',limitation='Full engine/common queue/certificate/source are bounded fakes. Game22896 log does not prove event was pending at207, so not a proven sole cause or runtime repair.'),indent=2))
