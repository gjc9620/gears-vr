#include "weapon_torque_bow_feed.h"
#include <cassert>
#include <cstdio>
using namespace gears_weapon;
void original_front_loading(){
 namespace a=gears_torque_bow_arrow_mount;
 for(auto parameters:{std::array<double,2>{.3,30},std::array<double,2>{.3,100},std::array<double,2>{1,100},std::array<double,2>{2,200}}){
  const auto scale=parameters[0],units=parameters[1];a::Mount arrow{};
  assert(a::author_original({true,{.01,-.02,.03},{0,0,0,1}},scale,scale,units,arrow));
  TorqueBowFeedCalibration plan{};assert(author_torque_bow_feed(71,scale,arrow,plan));
  const auto travel=plan.mount.approach_depth_metres-plan.mount.seated_depth_metres;
  assert(std::abs(travel-19.039143167676684*scale/units)<1e-10);
  TorqueBowArrowFeed feed;assert(feed.arm(1,2,3,4,plan));
  const Pose root{{.3,.7,.2},{0,0,std::sin(.3),std::cos(.3)}};
  const auto grip=[&](double depth){
   const auto orientation=gears_vr::normalized(gears_vr::multiply(gears_vr::multiply(root.orientation,plan.mount.port_in_weapon.orientation),gears_vr::inverse(plan.mount.item_in_left_grip.orientation)));
   const auto target=sum(root.position,rotate(root.orientation,sum(plan.mount.port_in_weapon.position,{depth,0,0})));
   return Pose{difference(target,rotate(orientation,plan.mount.item_in_left_grip.position)),orientation};
  };
  FeedSample sample{1,2,3,4,1,1000000000,root,grip(0),true,true};
  assert(feed.observe_raw(sample,plan)==FeedResult::rejected); // touching the final head point has no travel
  sample.sequence++;sample.monotonic_ns+=100000000;sample.left_grip=grip(plan.mount.approach_depth_metres+.001);
  assert(feed.observe_raw(sample,plan)==FeedResult::approaching);
  sample.sequence++;sample.monotonic_ns+=50000000;sample.left_grip=grip(plan.mount.approach_depth_metres*.5);
  assert(feed.observe_raw(sample,plan)==FeedResult::approaching);
  sample.sequence++;sample.monotonic_ns+=50000000;sample.left_grip=grip(plan.mount.seated_depth_metres*.5);
  assert(feed.observe_raw(sample,plan)==FeedResult::inserted);
  // An altered mount cannot counterfeit the original head/tail coordinates.
  auto corrupt=arrow;corrupt.tip_in_grip_units.position.x+=1;
  TorqueBowFeedCalibration rejected{};assert(!author_torque_bow_feed(71,scale,corrupt,rejected));
  assert(!author_torque_bow_feed(71,scale*1.1,arrow,rejected)); // preserve actual relative asset scale, never resize
 }
}
int main(){
 original_front_loading();
 TorqueBowFeedCalibration c;c.instance_serial=1;c.bow_asset[0]=c.arrow_asset[0]=1;
 c.original_arrow_verified=c.original_slot_verified=true;c.mount.outward_axis_in_weapon={0,1,0};
 assert(c.valid());TorqueBowArrowFeed feed;assert(feed.arm(1,2,3,4,c));
 FeedSample raw{1,2,3,4,1,1000000000,{},{{.01,.005,0},{}},true,true};
 TorqueBowFeedSnap snap{};assert(torque_bow_feed_snap(raw,c,false,false,snap));
 assert(snap.item.position.y==raw.left_grip.position.y); // assistance cannot move toward seat
 assert(feed.observe_raw(raw,c)==FeedResult::rejected); // seat proximity alone is never insertion
 raw.sequence=2;raw.monotonic_ns+=20000000;raw.left_grip.position={.01,.05,0};
 assert(feed.observe_raw(raw,c)==FeedResult::approaching);
 raw.sequence=3;raw.monotonic_ns+=20000000;raw.left_grip.position={.01,.03,0};
 assert(feed.observe_raw(raw,c)==FeedResult::approaching);
 raw.sequence=4;raw.monotonic_ns+=20000000;raw.left_grip.position={.01,.005,0};
 assert(feed.observe_raw(raw,c)==FeedResult::inserted);assert(feed.observe_raw(raw,c)==FeedResult::rejected);
 assert(!torque_bow_feed_snap(raw,c,false,true,snap));
 raw.left_grip.position.x=.03;assert(!torque_bow_feed_snap(raw,c,false,false,snap));
 assert(torque_bow_feed_snap(raw,c,true,false,snap)); // explicit acquisition/retention hysteresis
 raw.left_grip.position.x=.04;assert(!torque_bow_feed_snap(raw,c,true,false,snap));
 raw.left_grip.position={0,.05,0};raw.left_grip.orientation={0,0,1,0};
 assert(!torque_bow_feed_snap(raw,c,false,false,snap));
 assert(feed.arm(1,2,3,4,c));raw.left_grip.orientation={};raw.sequence=10;raw.monotonic_ns+=100000000;
 assert(feed.observe_raw(raw,c)==FeedResult::approaching);
 raw.sequence=11;raw.monotonic_ns+=100000001;raw.left_grip.position.y=.005;
 assert(feed.observe_raw(raw,c)==FeedResult::rejected); // stale chain cannot finish after recovery
 c.instance_serial=2;assert(feed.observe_raw(raw,c)==FeedResult::rejected);
 c.original_slot_verified=false;assert(!feed.arm(1,2,3,4,c));
 std::puts("Torque Bow raw feed/snap separation regression passed; synthetic geometry only.");
}
