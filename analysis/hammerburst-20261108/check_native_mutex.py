from pathlib import Path
import json,subprocess
A=Path(__file__).resolve().parent;P=A/'native-mutex-r1';BASE=A/'native-render-age-r1';S=P/'source/tools/vr-common'
s=(S/'controller_input_bridge.h').read_text()
s=s.replace('\n    DWORD mutex_result{};bool mutex_attempted{},mutex_contended{}; // Receiver-local; never a permit.','')
s=s.replace('static bool lock_now(HANDLE mutex,bool* contended=nullptr,DWORD* observed=nullptr,bool* attempted=nullptr) noexcept {','static bool lock_now(HANDLE mutex,bool* contended=nullptr) noexcept {')
s=s.replace('        if(attempted)*attempted=false;\n','').replace('\n        if(observed)*observed=result;if(attempted)*attempted=true;','')
s=s.replace('        const bool locked=lock_now(mutex_,contended,&local.mutex_result,&local.mutex_attempted);\n        local.mutex_contended=local.mutex_attempted&&local.mutex_result==WAIT_TIMEOUT;\n        if(!locked)return finish(Code::mutex_unavailable);','        if(!lock_now(mutex_,contended))return finish(Code::mutex_unavailable);')
assert s==(BASE/'source/tools/vr-common/controller_input_bridge.h').read_text(),'Exact bridge-policy reversal'
s=(P/'source/tools/native-capture/cover_shared_input_live.inl').read_text()
s=s.replace('\n    unsigned mutex_attempted{},mutex_result{},mutex_contended{};','')
s=s.replace('matching.selected_sync_ns?stamp-matching.selected_sync_ns:0,\n                unsigned(matching.mutex_attempted),matching.mutex_result,unsigned(matching.mutex_contended)};','matching.selected_sync_ns?stamp-matching.selected_sync_ns:0};')
s=s.replace('static_cast<long long>(r.source_age_ns),static_cast<long long>(r.selected_age_ns));\n        fprintf(file,"shared_grip_handoff_mutex_%u=%u,%u,%u ",i,r.mutex_attempted,r.mutex_result,r.mutex_contended);}','static_cast<long long>(r.source_age_ns),static_cast<long long>(r.selected_age_ns));}')
assert s==(BASE/'source/tools/native-capture/cover_shared_input_live.inl').read_text(),'Exact shared-GT reversal'
for rel in ('tools/native-capture/capture.cpp','tools/native-capture/weapon_hand_broker.h','tools/vr-common/controller_input_frame.h','tools/native-capture/cover_input_live.inl'):
 assert (P/'source'/rel).read_bytes()==(BASE/'source'/rel).read_bytes()
source=(S/'controller_input_bridge_test.cpp').read_text()
pre=r'''
#include <windows.h>
#include <thread>
bool inject_wait_failure{};unsigned injected_calls{};
DWORD observed_wait(HANDLE handle,DWORD wait){
 if(inject_wait_failure&&++injected_calls==2)return WAIT_FAILED;
 return ::WaitForSingleObject(handle,wait);
}
#define WaitForSingleObject observed_wait
'''
source=pre+source.replace('#include "controller_input_bridge.h"','#include "controller_input_bridge.h"\n#undef WaitForSingleObject')
extra=r'''
    CHECK(reader.read_matching(stereo,frame.received_ns+100,copy,false,nullptr,&diagnostic));
    CHECK(diagnostic.mutex_attempted&&diagnostic.mutex_result==WAIT_OBJECT_0&&!diagnostic.mutex_contended);
    wchar_t lock_name[128]{};
    swprintf_s(lock_name,LR"(Local\GearsVR.ControllerInputLock.v4.%lu.%llu)",pid,
        static_cast<unsigned long long>(controller_process_start(GetCurrentProcess())));
    HANDLE test_lock=OpenMutexW(MUTEX_ALL_ACCESS,FALSE,lock_name);CHECK(test_lock);
    for(bool abandon:{false,true}){
        HANDLE entered=CreateEventW(nullptr,TRUE,FALSE,nullptr),release=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        CHECK(entered&&release);
        std::thread holder([&](){CHECK(::WaitForSingleObject(test_lock,3000)==WAIT_OBJECT_0);
            SetEvent(entered);if(!abandon){CHECK(::WaitForSingleObject(release,3000)==WAIT_OBJECT_0);CHECK(ReleaseMutex(test_lock));}});
        CHECK(::WaitForSingleObject(entered,3000)==WAIT_OBJECT_0);
        if(abandon)holder.join();
        bool contended{};
        const auto before=copy;
        CHECK(!reader.read_matching(stereo,frame.received_ns+100,copy,false,&contended,&diagnostic));
        CHECK(diagnostic.code==ControllerMatchingRead::Code::mutex_unavailable&&diagnostic.mutex_attempted);
        CHECK(diagnostic.mutex_result==(abandon?WAIT_ABANDONED:WAIT_TIMEOUT));
        CHECK(contended==!abandon&&diagnostic.mutex_contended==!abandon);
        CHECK(!diagnostic.latest_sequence&&!diagnostic.selected_sequence&&std::memcmp(&before,&copy,sizeof copy)==0);
        if(!abandon){SetEvent(release);holder.join();}
        CHECK(reader.read_matching(stereo,frame.received_ns+100,copy,false,nullptr,&diagnostic));
        CHECK(diagnostic.mutex_result==WAIT_OBJECT_0&&!diagnostic.mutex_contended);
        CloseHandle(entered);CloseHandle(release);
    }
    inject_wait_failure=true;injected_calls=0;
    CHECK(!reader.read_matching(stereo,frame.received_ns+100,copy,false,nullptr,&diagnostic));
    inject_wait_failure=false;
    CHECK(injected_calls==2&&diagnostic.mutex_attempted&&diagnostic.mutex_result==WAIT_FAILED&&!diagnostic.mutex_contended);
    CHECK(diagnostic.code==ControllerMatchingRead::Code::mutex_unavailable);
    CHECK(reader.read_matching(stereo,frame.received_ns+100,copy,false,nullptr,&diagnostic));
    ControllerInputBridge unopened;
    CHECK(!unopened.read_matching(stereo,frame.received_ns+100,copy,false,nullptr,&diagnostic));
    CHECK(diagnostic.code==ControllerMatchingRead::Code::not_open&&!diagnostic.mutex_attempted);
    CloseHandle(test_lock);
    std::puts("Original bridge: accepted, real competing-thread TIMEOUT, real ABANDONED, injected WAIT_FAILED and unopened distinguished; no retry, packet/output mutation or changed decision");
'''
# Add before the final unfocused publication, while exact frame10 remains valid.
needle='    auto unfocused=frame;';assert source.count(needle)==1;source=source.replace(needle,extra+needle)
target=P/'mutex-test.cpp';target.write_text(source);cmd=P/'mutex-test.cmd';exe=P/'mutex-test.exe'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /D_CRT_SECURE_NO_WARNINGS /EHsc /I"{S}" "{target}" /Fo"{P / "mutex-test.obj"}" /Fe"{exe}"\nif errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
with (P/'mutex-test.log').open('wb') as log:r=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
print((P/'mutex-test.log').read_text(errors='replace'));r.check_returncode()
(P/'mutex-test-result.json').write_text(json.dumps(dict(passed=True,distinct_outcomes=5,real_timeout=True,real_abandoned=True,injected_failed=True,existing_bridge_cases_passed=True,exact_policy_reversal=True,packet_layout_unchanged=True,extra_waits=0,game_started=False),indent=2))
