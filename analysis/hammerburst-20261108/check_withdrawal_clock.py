from pathlib import Path
import json,subprocess,shutil
A=Path(__file__).resolve().parent;P=A/'withdrawal-clock-r1';BASE=A/'history-rejection-r1';S=P/'source/tools/vr-common'
s=(S/'weapon_hand_client.h').read_text();original=(BASE/'source/tools/vr-common/weapon_hand_client.h').read_text()
s=s.replace('    std::int64_t expired_read_ns{-1},expired_input_age_ns{-1};\n    unsigned expired_reader{}; // Observation only: copy1, source2, snapshot3.\n','')
begin=s.index('    // Keep the FIRST failing read\'s existing clock');end=s.index('    Reply fail_gt(',begin);s=s[:begin]+s[end:]
for i in (1,2,3):s=s.replace(f'withdraw_expired_pose_gt({i},now);','withdraw_expired_pose_gt();')
assert s==original,'Exact policy reversal must match original client'
s=(P/'source/tools/playground/weapon_hammerburst_source_loss_observation.inl').read_text()
s=s.replace(' expired_reader=%u expired_read_ns=%lld expired_input_age_ns=%lld expired_commit_age_ns=%lld expired_since_commit_ns=%lld','')
s=s.replace(',source_withdrawal.expired_reader,source_withdrawal.expired_read_ns,source_withdrawal.expired_input_age_ns,source_withdrawal.commit_check_input_age_ns,source_withdrawal.since_commit_check_ns','')
assert s==(BASE/'source/tools/playground/weapon_hammerburst_source_loss_observation.inl').read_text()
for rel in ('tools/playground/weapon_hammerburst_runtime.inl','tools/playground/weapon_hammerburst_burst_event.inl','tools/playground/weapon_lancer_r2_native.inl','tools/vr-common/controller_relative_motion.h'):
 assert (P/'source'/rel).read_bytes()==(BASE/'source'/rel).read_bytes()
source=(S/'weapon_hand_client_test.cpp').read_text()
source=source.replace('unsigned reads{},source_reads{};','unsigned reads{},source_reads{},clocks{};')
source=source.replace('return static_cast<Host*>(p)->now;','++static_cast<Host*>(p)->clocks;return static_cast<Host*>(p)->now;')
extra=r'''
unsigned expired_clock_cases(){
 unsigned cases{};
 for(bool support:{false,true})for(unsigned reader:{1u,2u,3u,4u}){
  Host h;HandClient c(h.adapter());h.connect(c);
  if(support)h.ready();else {h.step();h.next();assert(h.step()==Grip::single);}
  const auto key=h.frame.key;const auto sync=h.frame.controllers.sync_ns;
  const auto check=h.now;const auto frame=h.frame.key.frame;
  h.now+=100000001;const auto expired=h.now;const auto reads=h.reads,clocks=h.clocks;
  Frame out{};GripGameState g{};GripSnapshot snapshot{};bool ok{};
  {GT gt;
   if(reader==1)ok=c.copy_frame_on_gt(out);
   if(reader==2)ok=c.source_game_on_gt(key,g);
   if(reader==3)ok=c.snapshot_on_gt(snapshot);
   if(reader==4)ok=c.source_current_on_gt(key);
  }
  assert(!ok&&h.reads==reads&&h.clocks==clocks+2);
  FrameReadObservation w{};{GT gt;w=c.withdrawal_on_gt();}
  assert(w.failure==FrameReadFailure::stale_before&&w.frame==frame);
  assert(w.expired_reader==(reader==4?2:reader)&&w.expired_read_ns==expired);
  assert(w.expired_input_age_ns==expired-sync&&w.commit_check_input_age_ns==check-sync);
  assert(w.since_commit_check_ns==100000001&&w.input_age_ns==expired-sync);
  assert(w.expired_input_age_ns==w.commit_check_input_age_ns+w.since_commit_check_ns);
  const auto no_clock=h.clocks;
  {GT gt;assert(!c.copy_frame_on_gt(out));const auto after=c.withdrawal_on_gt();
   assert(after.expired_reader==w.expired_reader&&after.expired_read_ns==expired);}
  assert(h.clocks==no_clock);++cases;
 }
 return cases;
}
'''
assert source.count('int main(){')==1
source=source.replace('int main(){',extra+'\nint main(){\n    std::printf("First stale clock: %u cases, all readers/support modes; no added clock/read, original receipt survives cleared publication\\n",expired_clock_cases());')
target=P/'withdrawal-clock-test.cpp';target.write_text(source);cmd=P/'withdrawal-clock-test.cmd';exe=P/'withdrawal-clock-test.exe'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /D_CRT_SECURE_NO_WARNINGS /EHsc /I"{S}" "{target}" /Fo"{P / "withdrawal-clock-test.obj"}" /Fe"{exe}"\nif errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
with (P/'withdrawal-clock-test.log').open('wb') as log:r=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
print((P/'withdrawal-clock-test.log').read_text(errors='replace'));r.check_returncode()
for name in ('tick-cost-test-result.json','history-observer-test-result.json'):shutil.copy2(BASE/name,P/name)
(P/'withdrawal-clock-test-result.json').write_text(json.dumps(dict(passed=True,cases=8,existing_client_cases_passed=True,exact_policy_reversal=True,added_clocks=0,added_source_reads=0,all_reader_classes=True,game_started=False),indent=2))
