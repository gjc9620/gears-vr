#include "weapon_boomshot_feedback.h"
#define main previous_feedback_rig_cases
#include "../playground/weapon_rig_binding_test.cpp"
#undef main
namespace p=gears_boomshot_feedback;namespace vr=gears_vr;
struct FeedbackFixture:Fixture {
    p::Intent intent;std::int64_t stamp=now;
    explicit FeedbackFixture(unsigned source):Fixture(source){hand.controllers.hands[0].squeeze=0;hand.press_token=1;}
    void next(bool down,unsigned presses=0,unsigned releases=0){
        stamp+=20000000;++hand.key.frame;++hand.controllers.sequence;hand.stereo.sequence=hand.controllers.sequence;
        hand.now_ns=stamp-1000000;hand.stereo.received_ns=stamp-2000000;hand.controllers.received_ns=stamp-3000000;hand.controllers.sync_ns=stamp-4000000;
        auto& left=hand.controllers.hands[0];left.squeeze=down?1.f:0.f;hand.left_grip_down=down;
        left.pressed[vr::control_squeeze]+=presses;left.released[vr::control_squeeze]+=releases;
        if(presses){hand.press_token+=presses;left.last_squeeze_press_sequence=hand.controllers.sequence;left.last_squeeze_press_grip=left.grip;left.last_squeeze_press_flags=15;}
    }
    bool run(bool eligible=true,std::uint64_t hold=10){const auto copy=hand;const auto result=intent.advance(hand,hold,stamp,eligible);CHECK(wh::same_frame(copy,hand));return result;}
};
int main(){
    for(unsigned source=0;source<2;++source){
        FeedbackFixture f(source);CHECK(!f.run());f.next(true,1);CHECK(f.run());CHECK(!f.run());f.next(true);CHECK(!f.run());
        f.intent.lost();f.next(true);CHECK(!f.run());f.next(true);CHECK(!f.run());
        f.next(false,0,1);CHECK(!f.run());f.next(true,1);CHECK(f.run());f.next(true,1,1);CHECK(f.run());
        f.next(false,0,1);f.run();f.next(true,1);CHECK(!f.run(false));CHECK(!f.run());f.next(true);CHECK(!f.run());
        f.next(false,0,1);f.run();f.next(true,1);--f.hand.controllers.hands[0].last_squeeze_press_sequence;CHECK(!f.run());
        f.next(false,0,1);f.run();f.next(true,2,1);CHECK(!f.run());
        f.next(false,0,1);f.run();f.next(true,1);--f.hand.press_token;CHECK(!f.run());
        f.next(false,0,1);f.run();f.next(true,1);f.hand.interaction_control=unsigned(vr::LeftInteractionControl::pinch);CHECK(!f.run());
        f.hand.interaction_control=0;CHECK(!f.run());f.next(false,0,1);f.run();f.next(true,1);f.hand.left_tracked=0;CHECK(!f.run());
        f.hand.left_tracked=1;CHECK(!f.run());f.next(false,0,1);f.run();f.next(true,1);++f.hand.controllers.input_epoch;CHECK(!f.run());
        f.next(false,0,1);f.run();f.next(true,1);CHECK(!f.run(true,11));
        FeedbackFixture held(source);held.next(true,1);CHECK(!held.run());held.next(true);CHECK(!held.run());
        FeedbackFixture stale(source);stale.run();stale.next(true,1);stale.stamp+=100000001;CHECK(!stale.run());
        FeedbackFixture replay(source);const auto up=replay.hand;replay.run();replay.next(true,1);CHECK(replay.run());
        CHECK(!replay.intent.advance(up,10,replay.stamp,true));CHECK(!replay.run());
        FeedbackFixture mismatch(source);mismatch.run();mismatch.next(true,1);mismatch.hand.left_grip_down=0;CHECK(!mismatch.run());
    }
    std::printf("Boomshot once-only physical feedback intent: %u checks passed; real Frame parser, original counters/token/press-pose, baseline/release/repress, refusal consumption, lost/high-water/replay/epoch/freshness; synthetic input, no Engine/audio proof\n",checks);
}
