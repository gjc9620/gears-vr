#define NOMINMAX
#include "weapon_torque_bow_reload_fixture.h"
#include "weapon_torque_bow_feed.h"
namespace {
using Feed=w::TorqueBowArrowFeed;using Calibration=w::TorqueBowFeedCalibration;using RF=DriverFixture<ReloadBackend>;
struct Chain {
 RF f;Calibration calibration{};Feed feed;gears_torque_bow_arrow_mount::Mount arrow{};
 w::Pose root{{.3,.7,.2},{0,0,std::sin(.3),std::cos(.3)}};std::uint64_t begin{};
 Chain(){f.backend.actual.loaded=0;assert(gears_torque_bow_arrow_mount::author_original({true,{.01,-.02,.03},{0,0,0,1}},.3,.3,100,arrow));assert(w::author_torque_bow_feed(1,.3,arrow,calibration));
  f.backend.component=calibration.instance_serial;f.run();f.next();f.run();h::Decision decision{};decision.new_grant=true;
  assert(f.driver.begin_reload(f.backend,f.frame,f.game,decision)==d::Status::owned);arm();}
 void arm(){begin=0;assert(feed.arm(f.frame.weapon.address,f.backend.arrow_id,f.frame.world.address,f.frame.controllers.space_epoch,calibration));}
 w::Pose grip(double depth)const {
  const auto& m=calibration.mount;const auto port=w::sum(root.position,w::rotate(root.orientation,m.port_in_weapon.position));
  const auto q=v::normalized(v::multiply(v::multiply(root.orientation,m.port_in_weapon.orientation),v::inverse(m.item_in_left_grip.orientation)));
  return {w::difference(w::sum(port,w::scaled(w::rotate(root.orientation,m.outward_axis_in_weapon),depth)),w::rotate(q,m.item_in_left_grip.position)),q};}
 w::FeedResult at(double depth,std::int64_t dt=50000000){f.next(dt);const auto left=grip(depth);f.frame.controllers.hands[0].grip=left;f.run();
  const w::FeedSample sample{f.frame.weapon.address,f.backend.arrow_id,f.frame.world.address,f.frame.controllers.space_epoch,f.frame.controllers.sequence,f.frame.controllers.sync_ns,root,left,true,true};
  const auto r=feed.observe_raw(sample,calibration);if(r==w::FeedResult::approaching&&!begin)begin=f.frame.controllers.sequence;return r;}
 bool submit(std::uint64_t part=0){d::ReloadMotion m{};m.instance_config_serial=calibration.instance_serial;m.original_reload_serial=f.backend.serial;m.component=part?part:calibration.instance_serial;
  m.arrow_serial=f.backend.arrow_id;m.begin_input_sequence=begin;m.geometry_verified=m.aligned=m.continuous=m.complete=m.tracked_left_hand=true;
  return f.driver.complete_reload_motion(f.backend,f.frame,f.game,m);}
};
void original_feed_to_driver(){Chain c;const auto distance=c.calibration.mount.approach_depth_metres-c.calibration.mount.seated_depth_metres;
 assert(std::abs(distance-19.039143167676684*.3/100)<1e-10);
 assert(c.at(c.calibration.mount.approach_depth_metres)==w::FeedResult::approaching);
 assert(c.at(c.calibration.mount.approach_depth_metres*.5)==w::FeedResult::approaching);
 assert(c.at(c.calibration.mount.seated_depth_metres*.5)==w::FeedResult::inserted);assert(!c.submit(201));assert(c.submit());
 assert(c.f.backend.completions==1&&c.f.driver.inventory().loaded==1&&c.f.driver.inventory().reserve==4);assert(!c.submit());
}
void release_and_fresh_regrasp(){Chain c;assert(c.at(c.calibration.mount.approach_depth_metres)==w::FeedResult::approaching);assert(c.at(c.calibration.mount.approach_depth_metres*.5)==w::FeedResult::approaching);
 const auto old=c.f.backend.arrow_id;c.f.next();c.f.run();assert(c.f.driver.release_reload_arrow(c.f.backend,c.f.frame));assert(c.f.driver.interaction().held_arrow_serial()==0&&c.f.driver.inventory().reserve==5);
 c.f.next();c.f.run();h::Decision decision{};decision.new_grant=true;assert(c.f.driver.regrab_reload_arrow(c.f.backend,c.f.frame,decision));assert(c.f.backend.arrow_id>old);
 // Old raw stroke is tied to old arrow; the new arrow cannot use its remaining half.
 assert(c.at(c.calibration.mount.seated_depth_metres*.5)==w::FeedResult::rejected);assert(!c.f.backend.completions&&c.f.driver.inventory().loaded==0);
 c.arm();assert(c.at(c.calibration.mount.seated_depth_metres*.5)==w::FeedResult::rejected);c.arm();
 assert(c.at(c.calibration.mount.approach_depth_metres)==w::FeedResult::approaching);assert(c.at(c.calibration.mount.approach_depth_metres*.5)==w::FeedResult::approaching);assert(c.at(c.calibration.mount.seated_depth_metres*.5)==w::FeedResult::inserted);assert(c.submit());assert(c.f.backend.starts==1&&c.f.backend.completions==1&&c.f.driver.inventory().reserve==4);
}
void gap_cannot_finish_old_stroke(){Chain c;assert(c.at(c.calibration.mount.approach_depth_metres)==w::FeedResult::approaching);
 assert(c.at(c.calibration.mount.seated_depth_metres*.5,100000001)==w::FeedResult::rejected);assert(!c.f.backend.completions&&c.f.driver.inventory().loaded==0&&c.f.driver.inventory().reserve==5);assert(!c.submit());
}
}
int main(){
#ifdef _WIN32
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
 original_feed_to_driver();release_and_fresh_regrasp();gap_cannot_finish_old_stroke();
 std::puts("Production original Bow calibration/raw feed -> driver reload: full original stroke commits once; released/regrasped new arrow cannot reuse old half-stroke; seat-only and >100ms gap refuse. Injected game backend and synthetic raw poses only.");
}
