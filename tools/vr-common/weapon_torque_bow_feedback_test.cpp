#include "weapon_torque_bow_feedback.h"
#include "../playground/weapon_torque_bow_arrow_tracking_fixture.h"
#define CHECK BOW_ARROW_CHECK
namespace api=gears_torque_bow_feedback;namespace vr=gears_vr;
struct F:bow_arrow_tracking_fixture::Fixture {
 api::Intent intent;std::int64_t time=bow_arrow_tracking_fixture::now;
 explicit F(unsigned source=0):Fixture(source){hand.controllers.hands[0].squeeze=0;}
 void next(bool down,unsigned pressed=0,unsigned released=0){
  time+=20000000;++hand.key.frame;++hand.controllers.sequence;hand.stereo.sequence=hand.controllers.sequence;
  hand.now_ns=time-1000000;hand.stereo.received_ns=time-2000000;hand.controllers.received_ns=time-3000000;hand.controllers.sync_ns=time-4000000;
  auto&left=hand.controllers.hands[0];left.squeeze=down?1.f:0.f;hand.left_grip_down=down;
  left.pressed[vr::control_squeeze]+=pressed;left.released[vr::control_squeeze]+=released;
  if(pressed){hand.press_token+=pressed;left.last_squeeze_press_sequence=hand.controllers.sequence;left.last_squeeze_press_grip=left.grip;left.last_squeeze_press_flags=15;}
 }
 bool run(bool eligible=true,std::uint64_t hold=1){return intent.advance(hand,hold,time,eligible);}
};
int main(){
 for(unsigned source=0;source<2;++source){
  F f(source);CHECK(!f.run());f.next(true,1);CHECK(f.run());CHECK(!f.run());
  f.next(true);CHECK(!f.run());f.intent.lost();f.next(true);CHECK(!f.run());f.next(true);CHECK(!f.run());
  f.next(false,0,1);CHECK(!f.run());f.next(true,1);CHECK(f.run());
  f.next(true,1,1);CHECK(f.run()); // One real release/press, original counter+press-pose proof retained.
  f.next(false,0,1);CHECK(!f.run());f.next(true,1);CHECK(!f.run(false));CHECK(!f.run(true)); // No delayed cue when moving into a shoulder later.
  f.next(false,0,1);f.run();f.next(true,1);f.hand.controllers.hands[0].last_squeeze_press_sequence--;
  CHECK(!f.run());f.next(true);CHECK(!f.run());
  f.next(false,0,1);f.run();f.next(true,2,1);CHECK(!f.run()); // Ambiguous burst is never a repeated cue.
  f.next(false,0,1);f.run();f.next(true,1);f.hand.interaction_control=unsigned(vr::LeftInteractionControl::pinch);
  CHECK(!f.run());f.hand.interaction_control=0;CHECK(!f.run());
  f.next(false,0,1);f.run();f.next(true,1);f.hand.left_tracked=0;CHECK(!f.run());f.hand.left_tracked=1;CHECK(!f.run());
  f.next(false,0,1);f.run();f.next(true,1);++f.hand.controllers.input_epoch;CHECK(!f.run());
  f.next(false,0,1);f.run();f.next(true,1);CHECK(!f.run(true,2)); // New hold is a new baseline, not an action.
  F cold(source);cold.next(true,1);CHECK(!cold.run());cold.next(true);CHECK(!cold.run());
  F stale(source);stale.run();stale.next(true,1);stale.time+=100000001;CHECK(!stale.run());
  F replay(source);const auto old_up=replay.hand;CHECK(!replay.run());replay.next(true,1);CHECK(replay.run());
  CHECK(!replay.intent.advance(old_up,1,replay.time,true));CHECK(!replay.run()); // Old packet cannot rewind a consumed press.
 }
 std::printf("Torque Bow no-ammo intent: %u checks passed; actual input/cohort parser, synthetic frames, no original cue/game certification.\n",bow_arrow_tracking_fixture::checks);
}
