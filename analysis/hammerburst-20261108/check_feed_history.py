"""Real feed kernel and exact runtime history consumer with local context fixtures."""
from pathlib import Path
import hashlib,json,shutil,subprocess
A=Path(__file__).resolve().parent;ROOT=A.parents[1];P=A/'feed-history-r1';COMMON=P/'source/tools/vr-common'
def extract(text,signature):
 start=text.index(signature);brace=text.index('{',start);depth=1;i=brace+1
 while depth:
  depth+=(text[i]=='{')-(text[i]=='}');i+=1
 return text[start:i]
runtime=(P/'source/tools/playground/weapon_hammerburst_feed_history.inl').read_text()
latch=(P/'source/tools/playground/weapon_hammerburst_latch_history.inl').read_text()
source=r'''
#include "weapon_hammerburst_feed_history.h"
#include "controller_relative_motion.h"
#include "weapon_hand_participant.h"
#include "rig_tracking_math.h"
#include <cassert>
#include <cstdio>
using P=gears_vr::Pose;using V=gears_vr::Vector;using Q=gears_vr::Quaternion;
namespace hand=gears_weapon_hand;namespace input=gears_hand_continuity;
enum class Stage{new_magazine,old_magazine};Stage stage=Stage::new_magazine;
std::uint64_t feed_item{},feed_sequence{};std::int64_t feed_ns{};
gears_weapon::FeedInsertionStroke insertion;gears_weapon::FeedPortMount feed;
namespace latch_history {
struct Context{hand::Frame frame{};input::TrackedInput raw{};input::Publisher publisher{};
 P mount{};double size{1},units{100};bool valid{true};};
bool close(V a,V b,double e){return gears_weapon::length(gears_weapon::difference(a,b))<=e;}
'''+extract(latch,'bool same_context(')+r'''
}
namespace feed_history {
namespace relative=gears_controller_relative_motion;using Context=latch_history::Context;
Context previous{},sampled{};input::MotionHistory history{};gears_weapon::FeedSample prior{};
unsigned gate{},point_count{};
'''+extract(runtime,'bool project_pose(')+'\n'+extract(runtime,'bool advance(')+r'''
}
input::TrackedInput raw(unsigned n){
 input::TrackedInput v{};v.stream_epoch=v.tracking_epoch[0]=v.tracking_epoch[1]=1;
 auto& f=v.input;f.source=gears_vr::StereoSource::RuntimeTracked;f.session=f.space_epoch=f.input_epoch=f.profile_generation=1;
 f.sequence=n;f.sync_ns=1000000000+std::int64_t(n-1)*22000000;f.received_ns=f.sync_ns+1000;f.predicted_time=f.sync_ns+10000000;f.focused=1;
 for(auto& h:f.hands){h.grip_active=h.aim_active=h.trigger_active=h.squeeze_active=1;h.grip_flags=h.aim_flags=15;}
 f.hands[0].squeeze=1;f.hands[0].pressed[gears_vr::control_squeeze]=1;f.hands[0].grip.position.y=.06-.0112*(n-1);
 assert(input::tracked_input_shape_valid(v));return v;
}
int main(){unsigned cases{};
 for(unsigned failure=0;failure<22;++failure){
  using namespace feed_history;previous={};sampled={};history={};gate=point_count=0;insertion={};feed={};
  previous.raw=raw(1);sampled.raw=raw(6);previous.publisher=sampled.publisher={1,2};
  previous.frame.key.lifecycle=sampled.frame.key.lifecycle=1;
  previous.frame.press_token=sampled.frame.press_token=1;
  previous.frame.interaction_control=sampled.frame.interaction_control=unsigned(gears_vr::LeftInteractionControl::grip);
  history.count=5;for(unsigned i=0;i<5;++i)history.frames[i]=raw(i+2);
  P left{};assert(project_pose(previous.raw,previous,left));assert(std::abs(left.position.z-.06)<1e-12);
  prior={1,2,3,4,1,previous.raw.input.sync_ns,{},left,true,true};
  auto current=prior;current.sequence=6;current.monotonic_ns=sampled.raw.input.sync_ns;
  assert(project_pose(sampled.raw,sampled,current.left_grip));
  feed_item=2;feed_sequence=1;feed_ns=prior.monotonic_ns;stage=Stage::new_magazine;
  assert(insertion.arm(1,2,3,4));assert(insertion.observe(prior,feed)==gears_weapon::FeedResult::approaching);
  switch(failure){
   case 1:++sampled.frame.press_token;break;
   case 2:++sampled.frame.key.lifecycle;break;
   case 3:++sampled.frame.key.game_start;break;
   case 4:++sampled.frame.key.native_started_qpc;break;
   case 5:++sampled.frame.key.registration_cookie;break;
   case 6:++sampled.frame.key.world_generation;break;
   case 7:++sampled.frame.weapon.address;break;
   case 8:++sampled.frame.reference_generation;break;
   case 9:++sampled.frame.flags;break;
   case 10:++sampled.publisher.pid;break;
   case 11:sampled.mount.position.x=.01;break;
   case 12:sampled.frame.interaction_control=99;break;
   case 13:history.frames[2].input.hands[0].squeeze=0;break;
   case 14:++history.frames[2].tracking_epoch[0];break;
   case 15:++history.frames[2].tracking_epoch[1];break;
   case 16:++history.frames[2].input.input_epoch;break;
   case 17:history.frames[1]=history.frames[2];break;
   case 18:current.left_grip.position.x=.01;break;
   case 19:stage=Stage::old_magazine;break;
   case 20:++feed_item;break;
   case 21:++feed_ns;break;
  }
  gears_weapon::FeedResult result=gears_weapon::FeedResult::rejected;
  const bool used=advance(sampled.frame,current,result);
  if(!failure){assert(used&&result==gears_weapon::FeedResult::inserted&&point_count==6);}
  else {assert(!used);auto direct=prior;++direct.sequence;direct.monotonic_ns+=20000000;direct.left_grip.position.z=.004;
   assert(insertion.observe(direct,feed)==gears_weapon::FeedResult::inserted);}
  ++cases;
 }
 std::printf("HB exact runtime feed history: %u local fixture cases; real raw continuity/projector/kernel, no game\n",cases);
}
'''
target=P/'source/tools/playground/hammerburst_feed_history_consumer_test.cpp';target.write_text(source)
tests=[(ROOT/'tools/vr-common/weapon_hammerburst_feed_history_test.cpp','kernel'),(target,'consumer')]
results=[]
for path,label in tests:
 if label=='kernel':
  dest=COMMON/path.name;shutil.copy2(path,dest);path=dest
 cmd=P/f'feed-history-{label}.cmd';exe=P/f'feed-history-{label}.exe'
 cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+
  f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc /I"{COMMON}" "{path}" /Fo"{P / (label+".obj")}" /Fe"{exe}"\n'+
  f'if errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
 with (P/f'{label}-test.log').open('wb') as log:
  result=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 print((P/f'{label}-test.log').read_text(errors='replace'));result.check_returncode()
 results.append(dict(label=label,source_sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
(P/'feed-history-test-result.json').write_text(json.dumps(dict(passed=True,kernel_scenarios=21,consumer_scenarios=22,
 exact_runtime_functions=True,real_kernel_and_motion_chain=True,local_fixtures=True,game_execution=False,results=results),indent=2))
