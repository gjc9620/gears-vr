
#include "stereo_hand_age_policy.h"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace gears_vr;
StereoSample sample(){
 StereoSample s{};s.source=StereoSource::MetaOperatorSimulated;s.session=1;s.sequence=1;s.space_epoch=1;
 s.received_ns=1000000000;s.predicted_time=123;s.focused=s.visible=s.should_render=1;s.head_flags=s.view_flags=15;
 s.head={{0,1.7,0},{0,0,0,1}};s.eyes[0].pose={{-.032,1.7,0},{0,0,0,1}};
 s.eyes[1].pose={{.032,1.7,0},{0,0,0,1}};
 for(auto& eye:s.eyes)eye.fov={-.8,.8,.8,-.8};return s;
}
int main(){
 unsigned cases{};
 for(unsigned mutation=0;mutation<16;++mutation){
  StereoGate gate;auto s=sample();assert(gate.update(s,s.received_ns+1,true));
  ++s.sequence;auto now=s.received_ns+100000001;
  if(mutation==1)s.head_flags=s.view_flags=0;
  if(mutation==2)s.focused=s.visible=0;
  if(mutation==3)++s.session;
  if(mutation==4)++s.space_epoch;
  if(mutation==5)s.received_ns=now+1;
  if(mutation==6)s.eyes[0].fov.right=s.eyes[0].fov.left;
  if(mutation==7)s.head.position.x=std::numeric_limits<double>::quiet_NaN();
  if(mutation==8)gate.request_recenter();
  if(mutation==9)s.focused=0; // Visible but no current input focus.
  if(mutation==10)s.sequence=0;
  if(mutation==11)now=s.received_ns+100000000;
  if(mutation==12)now=0;
  if(mutation==13)s.received_ns=0;
  if(mutation==14)s.should_render=0;
  auto before=gate.continuity();auto result=gate.update(s,now,true);auto after=gate.continuity();
  if(mutation==15)++after.epoch;
  const bool kept=gears_stereo_hand_age::preserve(s,now,result,before,after);
  assert(kept==(mutation==0));
  if(kept){assert(!result&&result.status==StereoStatus::stale&&before.stable&&after.stable);}
  ++cases;
 }
 {StereoGate gate;auto s=sample();const auto before=gate.continuity();auto result=gate.update(s,s.received_ns+100000001,true);
  assert(!gears_stereo_hand_age::preserve(s,s.received_ns+100000001,result,before,gate.continuity()));++cases;}
 std::printf("Original StereoGate + logical-age policy: %u boundary/tracking/focus/identity/origin/geometry/replay/recenter cases passed; stale render remains rejected\n",cases);
}
