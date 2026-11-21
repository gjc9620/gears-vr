#include "weapon_torque_bow_arrow_presentation_fixture.h"
#define CHECK BOW_ARROW_CHECK
using namespace bow_arrow_presentation_fixture;
int main(){
 for(unsigned source=0;source<2;++source){
  Fixture f(source);p::Result result{};CHECK(f.place(.03,.015,.08)&&f.make(false,result)&&result.feed_assisted);
  CHECK(result.raw.input_sequence==f.hand.controllers.sequence&&result.raw.input_sync_ns==f.hand.controllers.sync_ns);
  CHECK(result.grip_correction_metres<=.04&&result.grip_correction_metres>0);
  p::tracking::Sample raw{};CHECK(p::tracking::sample(f.hand,f.rig,f.arrow,bow_arrow_tracking_fixture::now,raw));
  CHECK(p::same_pose(result.raw.tip_world,raw.tip_world,1e-10)&&p::same_pose(result.raw.source_world,raw.source_world,1e-10));
  // The same rigid mount reconstructs both the submitted palm and the visible
  // arrow. No output pointer/matrix or aligned result enters the raw stroke.
  const auto source_from_grip=vr::Pose{am::add(result.grip_world.position,am::rotate(result.grip_world.orientation,f.arrow.source_in_grip_units.position)),
   vr::normalized(vr::multiply(result.grip_world.orientation,f.arrow.source_in_grip_units.orientation))};
  CHECK(p::same_pose(source_from_grip,result.source_world,1e-8));
  p::tracking::RootSample root{};CHECK(p::tracking::root_sample(f.hand,f.rig,f.fixed,bow_arrow_tracking_fixture::now,root));
  const auto axis=am::rotate(root.world.orientation,f.feed.mount.outward_axis_in_weapon);
  CHECK(std::abs(w::dot(am::sub(result.tip_world.position,raw.tip_world.position),axis))<1e-6); // no axial advance
  auto weapon=root.world,left=f.rig.world_grip[0];weapon.position=am::mul(weapon.position,.01);left.position=am::mul(left.position,.01);
  w::TorqueBowArrowFeed mechanics;CHECK(mechanics.arm(f.hand.weapon.address,11,f.hand.world.address,f.hand.controllers.space_epoch,f.feed));
  const w::FeedSample physical{f.hand.weapon.address,11,f.hand.world.address,f.hand.controllers.space_epoch,
   f.hand.controllers.sequence,f.hand.controllers.sync_ns,weapon,left,true,true};
  CHECK(mechanics.observe_raw(physical,f.feed)==w::FeedResult::rejected); // original15mm miss, even though display aligned
  CHECK(f.make(false,result,bow_arrow_tracking_fixture::now+90000000)&&result.raw.input_sync_ns==f.hand.controllers.sync_ns);
  CHECK(!f.make(false,result,bow_arrow_tracking_fixture::now+100000001)&&!result.raw.input_sequence);
  CHECK(f.place(.03,.030)&&f.make(false,result)&&!result.feed_assisted);
  CHECK(f.make(true,result)&&result.feed_assisted&&result.grip_correction_metres<=.05);
  CHECK(f.place(.03,.040)&&f.make(true,result)&&!result.feed_assisted);
  CHECK(f.place(.03,.001,.35)&&f.make(false,result)&&!result.feed_assisted);
  CHECK(f.make(true,result)&&result.feed_assisted);
  // Authoring mismatch never silently changes the displayed original model.
  const auto saved=f.feed;f.feed.mount.item_in_left_grip.position.x+=.001;CHECK(!f.make(false,result));f.feed=saved;
  f.feed.bow_asset[17]^=1;CHECK(!f.make(false,result));f.feed=saved;
  f.feed.mount.approach_depth_metres*=.5;CHECK(!f.make(false,result));f.feed=saved;
  auto bad=f.rig;bad.scene_invocation++;
  CHECK(!p::make(f.hand,bad,f.arrow,{f.feed,f.fixed,11},bow_arrow_tracking_fixture::now,false,result));
  CHECK(!p::make(f.hand,f.rig,f.arrow,{f.feed,f.fixed,0},bow_arrow_tracking_fixture::now,false,result));
 }
 // At the normal30units/m scale a modest arrow-head rotation can pivot the
 // palm too far. The snap target itself is admissible; presentation refuses
 // the excessive palm pull and keeps the original grip/source unchanged.
 Fixture long_arrow(0,30);p::Result limited{};CHECK(long_arrow.place(.10,.001,.35));
 CHECK(long_arrow.make(true,limited)&&!limited.feed_assisted);
 CHECK(p::same_pose(limited.grip_world,long_arrow.rig.world_grip[0],1e-10));
 p::tracking::RootSample root{};CHECK(p::tracking::root_sample(long_arrow.hand,long_arrow.rig,long_arrow.fixed,bow_arrow_tracking_fixture::now,root));
 auto gun=root.world,left=long_arrow.rig.world_grip[0];gun.position=am::mul(gun.position,1/30.);left.position=am::mul(left.position,1/30.);
 w::FeedSample physical{long_arrow.hand.weapon.address,11,long_arrow.hand.world.address,long_arrow.hand.controllers.space_epoch,
  long_arrow.hand.controllers.sequence,long_arrow.hand.controllers.sync_ns,gun,left,true,true};w::TorqueBowFeedSnap eligible{};
 CHECK(w::torque_bow_feed_snap(physical,long_arrow.feed,true,false,eligible));
 // Whole physically coherent input stroke, with a15mm lateral miss. The
 // displayed guide reaches its seat while authenticated RAW mechanics never
 // produce insertion. Each sample has its own original sequence/sync/scene.
 Fixture stroke;CHECK(stroke.place(.03,.015));w::TorqueBowArrowFeed raw_mechanics;
 CHECK(raw_mechanics.arm(stroke.hand.weapon.address,11,stroke.hand.world.address,stroke.hand.controllers.space_epoch,stroke.feed));
 p::tracking::RootSample anchored{};CHECK(p::tracking::root_sample(stroke.hand,stroke.rig,stroke.fixed,bow_arrow_tracking_fixture::now,anchored));
 const auto axis=am::rotate(anchored.world.orientation,{1,0,0});double previous=.03;unsigned step{};
 for(double depth:{stroke.feed.mount.approach_depth_metres+.001,.04,.02,.001}){
  const auto stamp=bow_arrow_tracking_fixture::now+std::int64_t(++step)*20000000;
  stroke.move_left_world(am::mul(axis,(depth-previous)*stroke.rig.units_per_metre),stamp);previous=depth;
  p::Result shown{};CHECK(stroke.make(true,shown,stamp)&&shown.feed_assisted);
  auto weapon=anchored.world,raw_left=stroke.rig.world_grip[0];weapon.position=am::mul(weapon.position,.01);raw_left.position=am::mul(raw_left.position,.01);
  const w::FeedSample sample{stroke.hand.weapon.address,11,stroke.hand.world.address,stroke.hand.controllers.space_epoch,
   stroke.hand.controllers.sequence,stroke.hand.controllers.sync_ns,weapon,raw_left,true,true};
  CHECK(raw_mechanics.observe_raw(sample,stroke.feed)==w::FeedResult::rejected);
  CHECK(shown.raw.input_sequence==sample.sequence&&shown.raw.input_sync_ns==sample.monotonic_ns);
  CHECK(std::abs(shown.raw_depth-depth)<1e-6&&std::abs(shown.raw_lateral-.015)<1e-6);
 }
 std::printf("Torque Bow arrow/palm presentation: %u checks passed; actual raw tracking/groove/mount, synthetic source, no draw/skin/headset certification.\n",bow_arrow_tracking_fixture::checks);
}
