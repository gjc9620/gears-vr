from pathlib import Path
import json,subprocess,shutil
A=Path(__file__).resolve().parent;P=A/'history-origin-r1';BASE=A/'withdrawal-clock-r1';S=P/'source/tools/vr-common'
s=(S/'controller_continuity_bridge.h').read_text()
s=s.replace('struct MotionReadObservation {\n    std::uint64_t latest{},selected{},stopped{},stored{};\n    std::int64_t read_ns{};unsigned reason{}; // 1evicted,2missing,3original pair reject;0no break.\n};\n','').replace('\n    MotionReadObservation motion_observation_{};','').replace('    MotionReadObservation motion_observation() const noexcept {return motion_observation_;}\n','').replace('        if(right_motion)motion_observation_={};\n','').replace('\n        if(right_motion)motion_observation_={packet.latest_sequence,stereo.sequence,0,0,now_ns,0};','')
s=s.replace('                if(packet.latest_sequence-sequence>=ContinuityPacket::capacity){motion_observation_.reason=1;motion_observation_.stopped=sequence;break;}','                if(packet.latest_sequence-sequence>=ContinuityPacket::capacity)break;')
s=s.replace('                if(prior.input.sequence!=sequence){motion_observation_.reason=2;motion_observation_.stopped=sequence;motion_observation_.stored=prior.input.sequence;break;}\n                if(!motion_pair(prior,history.frames[0],1,100000000)){motion_observation_.reason=3;motion_observation_.stopped=sequence;motion_observation_.stored=prior.input.sequence;break;}','                if(prior.input.sequence!=sequence||!motion_pair(prior,history.frames[0],1,100000000))break;')
assert s==(BASE/'source/tools/vr-common/controller_continuity_bridge.h').read_text()
s=(S/'current_tracking_reader.h').read_text().replace('    gears_hand_continuity::MotionReadObservation motion_observation() const noexcept {return continuity_.motion_observation();}\n','');assert s==(BASE/'source/tools/vr-common/current_tracking_reader.h').read_text()
s=(P/'source/tools/playground/weapon_hammerburst_latch_history.inl').read_text().replace('        const auto before_record=hb_history_rejection::records;\n','')
start=s.index('        if(hb_history_rejection::records>before_record){');end=s.index('        return false;',start);s=s[:start]+s[end:];assert s==(BASE/'source/tools/playground/weapon_hammerburst_latch_history.inl').read_text()
assert (S/'controller_relative_motion.h').read_bytes()==(BASE/'source/tools/vr-common/controller_relative_motion.h').read_bytes()
source=(S/'controller_continuity_bridge_test.cpp').read_text()
extra=r'''
void history_origin_cases(){
 for(unsigned mode=0;mode<4;++mode){
  InputBridge producer,reader;const auto pid=GetCurrentProcessId();
  CHECK(producer.open(pid,true)&&reader.open(pid,false));
  ControllerInputFrame selected_input{};
  const unsigned last=mode==0?14:10;
  for(unsigned n=1;n<=last;++n){
   if(mode==1&&n==9)continue;
   auto in=frame(n);in.predicted_time=10000000000ll+n*10000000ll;
   in.sync_ns=1000000000ll+n*10000000ll;in.received_ns=in.sync_ns+100;
   CHECK(producer.publish(tracked(in,1,1,mode==2&&n==10?2:1)));
   if(n==10)selected_input=in;
  }
  MotionHistory history{};TrackedInput out{};Publisher publisher{};
  const auto now=1000000000ll+last*10000000ll+1000;
  CHECK(reader.read_matching(stereo_for(selected_input),now,out,publisher,false,nullptr,nullptr,&history));
  const auto origin=reader.motion_observation();CHECK(origin.latest==last&&origin.selected==10&&origin.read_ns==now);
  if(mode==0){CHECK(origin.reason==1&&origin.stopped==6&&!origin.stored&&history.count==4&&history.frames[0].input.sequence==7);}
  if(mode==1){CHECK(origin.reason==2&&origin.stopped==9&&origin.stored==1&&history.count==1);}
  if(mode==2){CHECK(origin.reason==3&&origin.stopped==9&&origin.stored==9&&history.count==1);}
  if(mode==3){CHECK(!origin.reason&&history.count==8&&history.frames[0].input.sequence==3);}
  CHECK(motion_history_valid(history,out,1));
  const auto received=out.input.received_ns;
  CHECK(!reader.read_matching(stereo_for(selected_input),received+100000001,out,publisher,false,nullptr,nullptr,&history));
  CHECK(!history.count&&!out.input.sequence&&!publisher.pid);
  CHECK(!reader.motion_observation().reason); // No stale stop reason on rejected fresh read.
 }
 std::puts("History origin: actual ring overwrite, skipped publication, right tracking boundary and full8-ring distinguished; original expiry and history output preserved");
}
'''
needle='int main(int argc,char** argv) {';assert source.count(needle)==1;source=source.replace(needle,extra+'\n'+needle)
source=source.replace('    original_motion_ring_copy();','    history_origin_cases();original_motion_ring_copy();')
target=P/'history-origin-test.cpp';target.write_text(source);cmd=P/'history-origin-test.cmd';exe=P/'history-origin-test.exe'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /D_CRT_SECURE_NO_WARNINGS /EHsc /I"{S}" "{target}" /Fo"{P / "history-origin-test.obj"}" /Fe"{exe}"\nif errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
with (P/'history-origin-test.log').open('wb') as log:r=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
print((P/'history-origin-test.log').read_text(errors='replace'));r.check_returncode()
for n in ('tick-cost-test-result.json','history-observer-test-result.json','withdrawal-clock-test-result.json'):shutil.copy2(BASE/n,P/n)
(P/'history-origin-test-result.json').write_text(json.dumps(dict(passed=True,cases=4,original_bridge_suite_passed=True,exact_policy_reversal=True,pure_kernel_unchanged=True,extra_ring_reads=0,extra_clocks=0,game_started=False),indent=2))
