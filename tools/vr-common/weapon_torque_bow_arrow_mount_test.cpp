#include "weapon_torque_bow_arrow_mount.h"
#include <cassert>
#include <cstdio>
namespace a=gears_torque_bow_arrow_mount;using V=a::V;using Q=a::Q;
double distance(V x,V y){return std::hypot(std::hypot(x.x-y.x,x.y-y.y),x.z-y.z);}
int main(){
 for(double units:{50.,100.,200.})for(double scale:{.3,1.,2.}){
  gears_hand_wrist::Calibration wrist{true,{-.035,.012,-.004},{0,std::sin(.16),0,std::cos(.16)}};
  V pad{.0975,-.046,-.021};a::Mount m{};assert(a::author(wrist,pad,scale,units,m));
  auto contact=a::add(m.source_in_grip_units.position,a::rotate(m.source_in_grip_units.orientation,a::mul(a::pad_contact_raw,scale)));
  auto expected=a::mul(a::add(wrist.grip_to_wrist_metres,a::rotate(wrist.grip_to_wrist_rotation,pad)),units);
  assert(distance(contact,expected)<1e-10);
  auto tip=a::add(m.source_in_grip_units.position,a::rotate(m.source_in_grip_units.orientation,a::mul(a::tip_raw,scale)));
  assert(distance(tip,m.tip_in_grip_units.position)<1e-10);
  assert(distance(a::mul(m.raw_tip_in_grip_m,units),tip)<1e-10);
  a::P back{};assert(a::source_from_tip(m.tip_in_grip_units,scale,back));
  assert(distance(back.position,m.source_in_grip_units.position)<1e-10);
  auto axis=a::rotate(a::arrow_in_wrist,a::forward_raw);
  assert(distance(axis,{.512111872899502,.258974612484765,-.81894600408714})<2e-6);
  assert(distance(a::rotate(a::arrow_in_wrist,{0,1,0}),a::mul(axis,-1))<1e-12); // source +Y is the forked tail
  a::Mount original{};assert(a::author_original(wrist,scale,scale,units,original));
  const auto source_m=a::mul(a::original_source_in_wrist_raw.position,scale/units);
  const auto expected_source=a::mul(a::add(wrist.grip_to_wrist_metres,a::rotate(wrist.grip_to_wrist_rotation,source_m)),units);
  assert(distance(original.source_in_grip_units.position,expected_source)<1e-10);
 }
 a::Mount out{};assert(!a::author({}, {},1,100,out));
 gears_hand_wrist::Calibration invalid{true,{},{0,0,0,0}};assert(!a::author(invalid,{},1,100,out));
 std::puts("Torque Bow original arrow mount/contact/tip coherence passed; mathematical geometry only.");
}
