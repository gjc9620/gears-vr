"""Exercise the exact HB reset helper and real Client/Native-owner fixtures."""
from pathlib import Path
import hashlib,json,subprocess
A=Path(__file__).resolve().parent;P=A/'pose-gap-incumbent-r1'
source=(P/'source/tools/playground/weapon_hammerburst_runtime.inl').read_text()
start=source.index('void unavailable_frame(');end=source.index('\nbool tick(uptr pc)',start)
helper=source[start:end]
assert 'copy_frame_on_gt(f,game,decision,&frame_read)' in source
assert 'if(stage!=Stage::idle){unavailable_frame(frame_read);' in source
suite=(P/'source/tools/vr-common/hammerburst_supply_client_test.cpp').read_text()
last=suite.rfind('}');suite=suite[:last]+'return 0;\n'+suite[last:]
(P/'preserved-supply-suite.inl').write_text(suite)
fixture=r'''
#define main existing_supply_suite
#include "preserved-supply-suite.inl"
#undef main
namespace policy {
namespace hand=gears_weapon_hand;
enum class Stage {idle,opening,old_magazine,waiting_ammo,new_magazine,closing,recovering};
FILE* log_file{};Stage stage{};hand::Lease held{};std::uint64_t new_item{};bool released{};
struct Opening {struct Identity {std::uint64_t lifecycle{};} identity;} opening;
struct Counter {unsigned resets{};void reset(){++resets;}} stroke,insertion,extraction;
@HELPER@
void reset(){stage=Stage::new_magazine;new_item=7;held={hand::LeaseKind::ammo,0,1,7};opening.identity.lifecycle=1;released=true;stroke={};insertion={};extraction={};}
}
int main(){
 existing_supply_suite();unsigned cases{};
 using namespace policy;
 for(unsigned reason=0;reason<=unsigned(FrameReadFailure::implicit_clear);++reason){
  reset();FrameReadObservation f{};f.failure=FrameReadFailure(reason);unavailable_frame(f);
  bool keep=f.failure==FrameReadFailure::publication||f.failure==FrameReadFailure::stale_before;
  assert((held.kind==LeaseKind::ammo)==keep);assert(insertion.resets==(keep?0u:1u));++cases;
 }
 for(unsigned change=0;change<7;++change){
  reset();if(change==0)stage=Stage::idle;if(change==1)stage=Stage::old_magazine;
  if(change==2)held.kind=LeaseKind::magazine;if(change==3)new_item=0;
  if(change==4)++held.id;if(change==5)++held.lifecycle;if(change==6)held={};
  FrameReadObservation f{};f.failure=FrameReadFailure::stale_before;unavailable_frame(f);
  assert(held.kind==LeaseKind::none&&insertion.resets==1);++cases;
 }
 for(bool repaired:{false,true}){
  Host h;HandClient c(h.adapter());h.connect(c);h.frame.controllers.hands[0].grip.position.x=2.;
  h.game.request_ammo=true;h.step();h.next();h.step();h.next(true);h.step();
  Frame out{};GripGameState game{};Decision d{};
  {GT gt;assert(c.copy_frame_on_gt(out,game,d));}
  assert(d.new_grant&&d.lease.kind==LeaseKind::ammo);
  h.hb_lease=d.lease;h.hb_publication.remember(h.frame,d);h.hb_supply_held=true;
  reset();held=d.lease;new_item=d.lease.id;opening.identity.lifecycle=d.lease.lifecycle;
  h.now+=100000001;FrameReadObservation failure{};
  {GT gt;assert(!c.copy_frame_on_gt(out,game,d,&failure));}
  assert(failure.failure==FrameReadFailure::stale_before&&out.key.frame==0);
  if(repaired)unavailable_frame(failure);else held={}; // Actual old reset block.
  h.hb_supply_held=held.kind!=LeaseKind::none;
  h.frame.now_ns=h.now;h.frame.stereo.received_ns=h.now;h.next(true);h.step();
  {GT gt;assert(c.copy_frame_on_gt(out,game,d));}
  if(repaired){
   assert(d.status==DecisionStatus::retained&&!d.new_grant&&same(d.lease,held));
   h.next(false);h.step();{GT gt;assert(c.copy_frame_on_gt(out,game,d));}
   assert(d.status==DecisionStatus::released&&d.lease.kind==LeaseKind::none);
  }else assert(d.status==DecisionStatus::lease_lost&&d.lease.kind==LeaseKind::none);
  ++cases;
 }
 std::printf("HB exact unavailable helper + actual Client/owner: %u additional cases; old reset reproduces lease_lost, new retains only fresh successor, real UP releases\n",cases);
}
'''.replace('@HELPER@',helper)
(P/'pose-gap-test.cpp').write_text(fixture)
cmd=P/'pose-gap-test.cmd';exe=P/'pose-gap-test.exe'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc /I"{P / "source/tools/vr-common"}" "{P / "pose-gap-test.cpp"}" /Fo"{P / "pose-gap-test.obj"}" /Fe"{exe}"\nif errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
with (P/'pose-gap-test.log').open('wb') as log:subprocess.run(['cmd','/c',str(cmd)],check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
result=dict(actual_helper_sha256=hashlib.sha256(helper.encode()).hexdigest(),fixture_sha256=hashlib.sha256(fixture.encode()).hexdigest(),exit_code=0,game_started=False,scope='Exact reset helper plus existing real HandClient/owner fixture; not runtime or full geometry evidence',transcript=(P/'pose-gap-test.log').read_text(errors='replace'))
(P/'pose-gap-test-result.json').write_text(json.dumps(result,indent=2));print(result['transcript'])
