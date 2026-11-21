#include "weapon_torque_bow_arrow_tracking.h"
#include "weapon_torque_bow_arrow_tracking_fixture.h"
using namespace bow_arrow_tracking_fixture;
#define CHECK BOW_ARROW_CHECK

int main(){
 namespace api=gears_torque_bow_arrow_tracking;namespace am=gears_torque_bow_arrow_mount;
 for(unsigned source=0;source<2;++source){
  Fixture f(source);am::Mount m{};
  const auto right=f.rig.world_grip[1];
  std::array<float,16> matrix{.3f,0,0,0,0,.3f,0,0,0,0,.3f,0,12,13,14,1};
  gears_weapon::WeaponMeshFollow fixed;
  CHECK(fixed.bind({right.position,gears_vr::ue_angles(gears_vr::rotation_matrix(right.orientation))},matrix));
  api::RootSample root{};CHECK(api::root_sample(f.hand,f.rig,fixed,now,root));
  CHECK(api::close_vector(root.world.position,{12,13,14},1e-4)&&std::abs(root.scale-.3)<1e-6);
  CHECK(api::close_vector(am::rotate(root.world.orientation,{1,0,0}),{0,1,0},1e-6));
  CHECK(root.input_sync_ns==f.hand.controllers.sync_ns&&root.input_sequence==f.hand.controllers.sequence&&root.scene==f.hand.key.frame);
  CHECK(!api::root_sample(f.hand,f.rig,fixed,now+100000001,root)&&!root.input_sync_ns);
  auto wrong=f.rig;wrong.scene_invocation++;CHECK(!api::root_sample(f.hand,wrong,fixed,now,root));
  wrong=f.rig;wrong.world_grip[1].position.x+=.01;CHECK(!api::root_sample(f.hand,wrong,fixed,now,root));
  auto uneven=matrix;uneven[5]=.303f;gears_weapon::WeaponMeshFollow nonuniform;
  CHECK(nonuniform.bind({right.position,gears_vr::ue_angles(gears_vr::rotation_matrix(right.orientation))},uneven));
  CHECK(api::root_sample(f.hand,f.rig,nonuniform,now,root)&&std::abs(root.scale-.301)<1e-6);
  // Existing WeaponMeshFollow averages its admitted near-uniform bind. The
  // Bow author separately requires the actual arrow scale to match, rather
  // than treating this averaged result as another permitted arrow size.
  am::Mount actual_arrow{};CHECK(am::author_original({true,{},{0,0,0,1}},.3,.3,100,actual_arrow));
  gears_weapon::WeaponMeshFollow unbound;CHECK(!api::root_sample(f.hand,f.rig,unbound,now,root));
  auto moved=f;moved.rig.controllers.hands[1].grip.position.x+=.02;
  ++moved.rig.controllers.sequence;moved.rig.stereo.sequence=moved.rig.controllers.sequence;
  moved.hand.controllers=moved.rig.controllers;moved.hand.stereo=moved.rig.stereo;moved.recompute();
  CHECK(api::root_sample(moved.hand,moved.rig,fixed,now,root));
  CHECK(api::close_vector(am::sub(root.world.position,{12,13,14}),am::sub(moved.rig.world_grip[1].position,right.position),1e-4));
  CHECK(am::author({true,{.01,.02,.03},{0,0,0,1}},{.035,.01,-.005},.3,100,m));
  api::Sample sample{};CHECK(api::sample(f.hand,f.rig,m,now,sample));
  CHECK(sample.input_sync_ns==f.hand.controllers.sync_ns&&sample.input_sequence==f.hand.controllers.sequence&&sample.scene==f.hand.key.frame);
  gears_vr::Pose from_tip{};CHECK(am::source_from_tip(sample.tip_world,.3,from_tip));
  CHECK(api::close_vector(from_tip.position,sample.source_world.position,1e-8));
  CHECK(gears_equipment_rig::detail::near_pose({{},sample.tip_world.orientation},{{},sample.source_world.orientation}));
  const auto source_position=sample.source_world.position,tip_position=sample.tip_world.position;
  // A late consumer retains the original sync, not another full100ms lease.
  CHECK(api::sample(f.hand,f.rig,m,now+90000000,sample));CHECK(sample.input_sync_ns==now-4000000);
  CHECK(!api::sample(f.hand,f.rig,m,now+100000001,sample)&&!sample.input_sync_ns);
  auto changed=m;changed.tip_in_grip_units.position.x+=.001;
  CHECK(!api::sample(f.hand,f.rig,changed,now,sample));
  changed=m;changed.raw_tip_in_grip_m.y+=.001;CHECK(!api::sample(f.hand,f.rig,changed,now,sample));
  changed=m;changed.component_scale=.4;CHECK(!api::sample(f.hand,f.rig,changed,now,sample));
  auto raw=f.rig;raw.scene_invocation++;CHECK(!api::sample(f.hand,raw,m,now,sample));
  raw=f.rig;raw.controllers.sequence++;CHECK(!api::sample(f.hand,raw,m,now,sample));
  raw=f.rig;raw.world_grip[0].position.x+=.01;CHECK(!api::sample(f.hand,raw,m,now,sample));
  raw=f.rig;raw.world_pose_mask&=~rt::left_grip_pose;raw.world_grip[0]={};CHECK(!api::sample(f.hand,raw,m,now,sample));
  // Both original source samples move together through the existing basis.
  f.rig.controllers.hands[0].grip.position.x+=.03;f.rig.controllers.sequence++;
  f.rig.stereo.sequence=f.rig.controllers.sequence;f.hand.stereo=f.rig.stereo;
  f.hand.controllers=f.rig.controllers;f.recompute();CHECK(api::sample(f.hand,f.rig,m,now,sample));
  const auto source_delta=am::sub(sample.source_world.position,source_position),tip_delta=am::sub(sample.tip_world.position,tip_position);
  CHECK(api::close_vector(source_delta,tip_delta,1e-8));
  CHECK(std::hypot(std::hypot(source_delta.x,source_delta.y),source_delta.z)>2.99);
 }
 std::printf("Torque Bow arrow tracking: %u checks passed; actual raw-rig/mount code with synthetic source bytes, no Native/game/fit certification.\n",checks);
}
