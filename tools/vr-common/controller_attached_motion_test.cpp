#include "controller_relative_motion.h"
#include "weapon_longshot_chamber_snap.h"
#include "weapon_longshot_grasp_pose.h"
#include "hand_authored_mount_profile.h"
#include "rig_tracking_frame.h"
#include <cassert>
#include <cstdio>
#include <limits>
namespace c=gears_hand_continuity;namespace r=gears_controller_relative_motion;
using V=gears_vr::Vector;using Q=gears_vr::Quaternion;
c::TrackedInput frame(unsigned sequence){
 c::TrackedInput x{};x.stream_epoch=x.tracking_epoch[0]=x.tracking_epoch[1]=1;
 auto& p=x.input;p.source=gears_vr::StereoSource::RuntimeTracked;p.session=1;p.sequence=sequence;
 p.space_epoch=p.input_epoch=p.profile_generation=1;p.sync_ns=1000000000+std::int64_t(sequence-1)*22000000;
 p.received_ns=p.sync_ns+1000;p.predicted_time=p.sync_ns+10000000;p.focused=1;
 for(auto& h:p.hands){h.grip_active=h.aim_active=h.trigger_active=h.squeeze_active=1;h.grip_flags=h.aim_flags=15;}
 p.hands[0].squeeze=1;p.hands[0].pressed[gears_vr::control_squeeze]=1;
 return x;
}
double distance(V a,V b){return std::hypot(std::hypot(a.x-b.x,a.y-b.y),a.z-b.z);}
int main(){
 unsigned cases{};const V tip{.1,.03,-.02};
 {auto x=frame(1);V a{},b{};assert(r::project(x,{},1,a)&&r::project_attached(x,{},1,{},b)&&distance(a,b)==0);++cases;}
 {auto x=frame(1);V out{};assert(r::project_attached(x,{},1,tip,out)&&distance(out,tip)<1e-12);++cases;}
 {auto x=frame(1);V out{};assert(r::project_attached(x,{},2,tip,out)&&distance(out,{.05,.015,-.01})<1e-12);++cases;}
 {auto x=frame(1);x.input.hands[0].grip.orientation={0,std::sin(.3),0,std::cos(.3)};
  V out{};const auto matrix=gears_vr::xr_to_ue(gears_vr::rotation_matrix(x.input.hands[0].grip.orientation));
  assert(r::project_attached(x,{},1,tip,out)&&distance(out,gears_vr::transform(matrix,tip))<1e-12);++cases;}
 {auto x=frame(1),rotated=x;const Q q{std::sin(.2),0,0,std::cos(.2)};
  x.input.hands[0].grip.position={.1,.2,.3};x.input.hands[1].grip.position={-.2,.1,.15};rotated=x;
  for(auto& h:rotated.input.hands){h.grip.position=gears_vr::transform(gears_vr::rotation_matrix(q),h.grip.position);
   h.grip.position.x+=20;h.grip.position.y-=8;h.grip.orientation=q;}
  V a{},b{};assert(r::project_attached(x,{},1,tip,a)&&r::project_attached(rotated,{},1,tip,b)&&distance(a,b)<1e-12);++cases;}
 const auto previous=frame(1);auto current=frame(6);c::MotionHistory history{};history.count=5;
 for(unsigned i=0;i<5;++i){history.frames[i]=frame(i+2);const double angle=.1*(i+1);
  history.frames[i].input.hands[0].grip.orientation={0,std::sin(angle/2),0,std::cos(angle/2)};}
 current=history.frames[4];
 {r::Segment result{};assert(r::build_attached(previous,current,history,gears_vr::control_squeeze,{},1,tip,result));
  assert(result.count==6);for(unsigned i=0;i<6;++i){V expected{};const auto& input=i?history.frames[i-1]:previous;
   assert(r::project_attached(input,{},1,tip,expected)&&distance(expected,result.points[i].position)<1e-12);
   assert(result.points[i].sync_ns==input.input.sync_ns);}assert(distance(result.points[0].position,result.points[5].position)>.04);++cases;}
 for(unsigned fail=0;fail<5;++fail){auto h=history;auto b=current;V point=tip;
  if(fail==0)++h.frames[2].input.sequence;
  if(fail==1)++h.frames[2].tracking_epoch[0];
  if(fail==2)h.frames[2].input.hands[0].released[gears_vr::control_squeeze]=1;
  if(fail==3)point.x=std::numeric_limits<double>::quiet_NaN();
  if(fail==4)++b.input.sequence;
  r::Segment result{};result.count=99;assert(!r::build_attached(previous,b,h,gears_vr::control_squeeze,{},1,point,result)&&result.count==99);++cases;
 }
 for(double units:{50.,100.,200.}){
  namespace snap=gears_weapon_longshot_chamber_snap;namespace grasp=gears_weapon_longshot_grasp;
  gears_hand_authored_mount::Mount anatomy{};gears_vr::Pose mount{};
  assert(gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,gears_hand_authored_mount::Side::left,1,units,anatomy));
  assert(grasp::cartridge_mount(anatomy.calibration.grip_to_wrist_metres,anatomy.calibration.grip_to_wrist_rotation,units,mount));
  V mounted_tip{};assert(snap::tip_in_grip_m(mount,units,mounted_tip));
  auto raw=frame(1);raw.input.hands[0].grip.position={.2,1.4,-.4};raw.input.hands[1].grip.position={.1,1.5,-.5};
  raw.input.hands[0].grip.orientation={std::sin(.15),0,0,std::cos(.15)};
  gears_vr::StereoMatrix view{};view[0]=view[5]=view[10]=view[15]=1;
  gears_vr::Pose left{},right{};assert(gears_vr::rig_tracking::pose(view,{},raw.input.hands[0].grip,units,left));
  assert(gears_vr::rig_tracking::pose(view,{},raw.input.hands[1].grip,units,right));
  const auto tip_world=snap::add(left.position,gears_vr::transform(gears_vr::rotation_matrix(left.orientation),snap::mul(mounted_tip,units)));
  const auto expected=snap::mul(gears_vr::transform(gears_vr::rotation_matrix(gears_vr::inverse(right.orientation)),snap::sub(tip_world,right.position)),1/units);
  V projected{};assert(r::project_attached(raw,{},1,mounted_tip,projected)&&distance(expected,projected)<1e-12);++cases;
 }
 {V output{99,0,0};assert(!gears_weapon_longshot_chamber_snap::tip_in_grip_m({},0,output)&&output.x==99);++cases;}
 {V output{99,0,0};gears_vr::Pose mount{};mount.orientation.w=0;
  assert(!gears_weapon_longshot_chamber_snap::tip_in_grip_m(mount,100,output)&&output.x==99);++cases;}
 std::printf("Controller attached motion:%u cases; real left rotation contributes cartridge-tip travel, source/epoch/release guards and atomic output retained; geometry only\n",cases);
}
