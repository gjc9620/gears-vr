#include "original_l2_aim.h"
#include "weapon_hand_participant.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace gears_vr;
using namespace gears_weapon;

namespace {
struct Trial {
    StereoSample stereo{};ControllerInputFrame frame{};
    OriginalL2Context context{11,12,13,true,false,false,true};
    OriginalL2Aim aim;std::vector<OriginalL2Action> calls;bool succeeds{true};
    explicit Trial(bool start_baseline=true){
        stereo.session=frame.session=7;stereo.sequence=frame.sequence=1;
        stereo.space_epoch=frame.space_epoch=3;stereo.predicted_time=frame.predicted_time=1000;
        stereo.focused=frame.focused=1;frame.sync_ns=999999900;frame.received_ns=1000000000;
        frame.hands[0].grip_active=1;frame.hands[0].grip_flags=15;
        frame.hands[0].grip.orientation.w=1;
        if(start_baseline)assert(advance()==OriginalL2Status::baseline);
    }
    bool execute(OriginalL2Action action,std::uint64_t pc){
        assert(pc==12);calls.push_back(action);return succeeds;
    }
    OriginalL2Status advance(){
        return aim.advance(frame,stereo,context,1000000100,
            [&](OriginalL2Action action,std::uint64_t pc){return execute(action,pc);});
    }
    OriginalL2Status yield(){
        return aim.yield(context,
            [&](OriginalL2Action action,std::uint64_t pc){return execute(action,pc);});
    }
    void next(){++frame.sequence;++stereo.sequence;}
    void press(){
        next();++frame.hands[0].pressed[control_trigger];
        assert(advance()==OriginalL2Status::pressed&&aim.owns_press());context.game_target_key=true;
    }
};
void part_grant_reserves_first_press_before_reload_becomes_busy(){
    namespace h=gears_weapon_hand;
    for(auto kind:{h::LeaseKind::magazine,h::LeaseKind::ammo,h::LeaseKind::cover,h::LeaseKind::other,h::LeaseKind::support}){
        Trial t;h::Frame f{};f.key.lifecycle=9;f.key.frame=10;
        if(kind==h::LeaseKind::support)f.interaction_control=unsigned(LeftInteractionControl::pinch);
        f.press_token=4;f.left_grip_known=f.left_grip_down=1;
        h::Decision d{};d.key=f.key;d.press_token=4;d.lease={kind,0,9,4};
        d.status=h::DecisionStatus::granted;d.release_required=0;
        assert(h::part_reserves_secondary_trigger(f,d));
        t.next();++t.frame.hands[0].pressed[control_trigger];
        t.context.allow_press=!h::part_reserves_secondary_trigger(f,d);
        assert(t.advance()==OriginalL2Status::blocked&&t.calls.empty()&&!t.aim.owns_press());
        d.status=h::DecisionStatus::retained;++f.key.frame;d.key=f.key;
        assert(h::part_reserves_secondary_trigger(f,d));
        t.next();assert(t.advance()==OriginalL2Status::accepted&&t.calls.empty());
        // Losing the part does not replay the consumed trigger edge as aim.
        f.left_grip_down=0;t.context.allow_press=!h::part_reserves_secondary_trigger(f,d);
        t.next();assert(t.advance()==OriginalL2Status::accepted&&t.calls.empty());
        t.next();++t.frame.hands[0].released[control_trigger];assert(t.advance()==OriginalL2Status::accepted);
        t.press();assert(t.calls.size()==1);
        f.left_grip_down=1;++d.key.frame;assert(!h::part_reserves_secondary_trigger(f,d));
        d.key=f.key;d.lease.kind=h::LeaseKind::support;
        f.interaction_control=unsigned(LeftInteractionControl::grip);
        assert(!h::part_reserves_secondary_trigger(f,d));
        f.interaction_control=unsigned(LeftInteractionControl::pinch);
        assert(h::part_reserves_secondary_trigger(f,d));
    }
}
void cold_yield_still_requires_baseline_release_and_press(){
    Trial t(false);
    assert(t.yield()==OriginalL2Status::accepted&&t.calls.empty()&&!t.aim.owns_press());
    assert(t.yield()==OriginalL2Status::accepted&&t.calls.empty());
    t.frame.hands[0].pressed[control_trigger]=1;
    t.frame.hands[0].trigger_active=1;t.frame.hands[0].trigger=1;
    assert(t.advance()==OriginalL2Status::baseline&&t.calls.empty());
    assert(t.advance()==OriginalL2Status::rejected&&t.calls.empty());
    t.next();assert(t.advance()==OriginalL2Status::blocked&&t.calls.empty());
    t.next();++t.frame.hands[0].released[control_trigger];
    assert(t.advance()==OriginalL2Status::accepted&&t.calls.empty());
    t.press();assert(t.calls.size()==1);

    Trial known;known.frame.hands[0].grip_flags=0;
    assert(known.advance()==OriginalL2Status::rejected);
    ++known.context.weapon;
    assert(known.yield()==OriginalL2Status::abort_required&&known.calls.empty());
    --known.context.weapon;
    assert(known.yield()==OriginalL2Status::abort_required);

    Trial toggle(false);toggle.context.toggle_targeting=true;
    assert(toggle.yield()==OriginalL2Status::abort_required&&toggle.calls.empty());
    toggle.context.toggle_targeting=false;
    assert(toggle.yield()==OriginalL2Status::abort_required);
}
void baseline_release_proof_requires_active_low_trigger_and_game_key_up(){
    Trial cold(false);assert(cold.yield()==OriginalL2Status::accepted);
    cold.frame.hands[0].trigger_active=1;cold.frame.hands[0].trigger=0;
    assert(cold.advance()==OriginalL2Status::baseline&&cold.calls.empty());
    cold.press();assert(cold.calls.size()==1);
    Trial up;up.press();assert(up.yield()==OriginalL2Status::released);
    up.context.game_target_key=false;up.frame.hands[0].trigger_active=1;
    up.frame.hands[0].trigger=.35f;up.next();
    ++up.frame.hands[0].pressed[control_trigger];up.frame.hands[0].released[control_trigger]+=2;
    assert(up.advance()==OriginalL2Status::baseline&&up.calls.size()==2);
    up.frame.hands[0].trigger=1;up.press();assert(up.calls.size()==3);
    Trial foreign;foreign.context.game_target_key=true;foreign.next();
    ++foreign.frame.hands[0].pressed[control_trigger];
    assert(foreign.advance()==OriginalL2Status::suppressed);
    assert(foreign.yield()==OriginalL2Status::suppressed&&foreign.calls.empty());
    foreign.context.game_target_key=false;foreign.frame.hands[0].trigger_active=1;
    foreign.frame.hands[0].trigger=0;foreign.next();++foreign.frame.hands[0].released[control_trigger];
    assert(foreign.advance()==OriginalL2Status::baseline&&foreign.calls.empty());
    foreign.press();assert(foreign.calls.size()==1);
    for(unsigned state=0;state<4;++state){
        Trial held;held.press();assert(held.yield()==OriginalL2Status::released);
        held.context.game_target_key=state==3;
        auto& hand=held.frame.hands[0];hand.trigger_active=state==2?0:1;
        hand.trigger=state==0?1.f:state==1?.5f:0.f;
        held.next();assert(held.advance()==OriginalL2Status::baseline);
        held.next();assert(held.advance()==OriginalL2Status::blocked&&held.calls.size()==2);
        held.next();++hand.pressed[control_trigger];++hand.released[control_trigger];
        assert(held.advance()==OriginalL2Status::blocked&&!held.aim.owns_press()&&held.calls.size()==2);
    }
}
void synchronous_yield_seals_held_and_missed_edges(){
    Trial t;t.press();
    assert(t.yield()==OriginalL2Status::released&&!t.aim.owns_press()&&t.calls.size()==2);
    assert(t.yield()==OriginalL2Status::suppressed&&t.calls.size()==2);
    t.context.game_target_key=false;
    assert(t.yield()==OriginalL2Status::accepted&&t.calls.size()==2);
    assert(t.advance()==OriginalL2Status::rejected&&t.calls.size()==2);
    // A release/press while the weapon was away is sealed by the first frame.
    t.next();++t.frame.hands[0].pressed[control_trigger];++t.frame.hands[0].released[control_trigger];
    assert(t.advance()==OriginalL2Status::baseline&&t.calls.size()==2);
    t.next();assert(t.advance()==OriginalL2Status::blocked&&!t.aim.owns_press());
    // A combined pair after that baseline is also not guessed/replayed.
    t.next();++t.frame.hands[0].pressed[control_trigger];++t.frame.hands[0].released[control_trigger];
    assert(t.advance()==OriginalL2Status::blocked&&t.calls.size()==2);
    t.next();++t.frame.hands[0].released[control_trigger];
    assert(t.advance()==OriginalL2Status::accepted);t.press();assert(t.calls.size()==3);
}
void allow_press_false_releases_without_a_new_frame(){
    Trial t;t.press();t.context.allow_press=false;
    assert(t.advance()==OriginalL2Status::released&&!t.aim.owns_press()&&t.calls.size()==2);
    assert(t.advance()==OriginalL2Status::rejected&&t.calls.size()==2);
    t.frame.hands[0].grip_flags=0;
    assert(t.advance()==OriginalL2Status::rejected&&t.calls.size()==2);
    t.context.allow_press=true;t.context.game_target_key=false;t.frame.hands[0].grip_flags=15;
    ++t.frame.input_epoch;t.frame.hands[0].pressed[control_trigger]=0;
    t.frame.hands[0].released[control_trigger]=0;t.next();
    assert(t.advance()==OriginalL2Status::baseline);
    t.next();++t.frame.hands[0].pressed[control_trigger];
    assert(t.advance()==OriginalL2Status::blocked&&t.calls.size()==2);
    t.next();++t.frame.hands[0].released[control_trigger];
    assert(t.advance()==OriginalL2Status::accepted);t.press();assert(t.calls.size()==3);
}
void identity_toggle_and_execution_failure_keep_owned_sticky(){
    for(unsigned field=0;field<8;++field){
        Trial t;t.press();const auto certified=t.context;
        switch(field){
        case 0:++t.context.world;break;case 1:++t.context.controller;break;
        case 2:++t.context.weapon;break;case 3:t.context.ordinary_player=false;break;
        case 4:t.context.world=0;break;case 5:t.context.controller=0;break;
        case 6:t.context.weapon=0;break;case 7:t.context.toggle_targeting=true;break;
        }
        assert(t.yield()==OriginalL2Status::abort_required&&t.aim.owns_press()&&t.calls.size()==1);
        t.context=certified;
        assert(t.yield()==OriginalL2Status::abort_required&&t.advance()==OriginalL2Status::abort_required);
        assert(t.aim.owns_press()&&t.calls.size()==1);
    }
    Trial failed;failed.press();failed.succeeds=false;
    assert(failed.yield()==OriginalL2Status::abort_required&&failed.aim.owns_press()&&failed.calls.size()==2);
    failed.succeeds=true;
    assert(failed.yield()==OriginalL2Status::abort_required&&failed.advance()==OriginalL2Status::abort_required);
    assert(failed.aim.owns_press()&&failed.calls.size()==2);
    Trial toggle;toggle.press();toggle.context.allow_press=false;toggle.context.toggle_targeting=true;
    assert(toggle.advance()==OriginalL2Status::abort_required&&toggle.aim.owns_press()&&toggle.calls.size()==1);
}
void foreign_and_suppressed_keys_are_not_released(){
    Trial foreign;foreign.context.game_target_key=true;
    assert(foreign.yield()==OriginalL2Status::suppressed&&foreign.calls.empty());
    assert(foreign.yield()==OriginalL2Status::suppressed&&!foreign.aim.owns_press());
    Trial shared;shared.context.game_target_key=true;shared.next();
    ++shared.frame.hands[0].pressed[control_trigger];
    assert(shared.advance()==OriginalL2Status::suppressed);
    assert(shared.yield()==OriginalL2Status::suppressed&&shared.calls.empty());
    Trial toggle;toggle.context.toggle_targeting=true;
    assert(toggle.yield()==OriginalL2Status::abort_required&&toggle.calls.empty());
    Trial already_up;already_up.press();already_up.context.game_target_key=false;
    assert(already_up.yield()==OriginalL2Status::recovered&&!already_up.aim.owns_press());
    assert(already_up.calls.size()==1);
}
void stowed_release_press_is_consumed_without_rearm(){
    Trial t;t.press();t.context.allow_press=false;
    assert(t.advance()==OriginalL2Status::released);t.context.game_target_key=false;
    t.next();++t.frame.hands[0].released[control_trigger];
    assert(t.advance()==OriginalL2Status::blocked);
    t.next();++t.frame.hands[0].pressed[control_trigger];
    assert(t.advance()==OriginalL2Status::blocked&&t.calls.size()==2);
    t.context.allow_press=true;t.next();assert(t.advance()==OriginalL2Status::baseline);
    t.next();assert(t.advance()==OriginalL2Status::blocked&&!t.aim.owns_press());
    t.next();++t.frame.hands[0].released[control_trigger];
    assert(t.advance()==OriginalL2Status::accepted);t.press();assert(t.calls.size()==3);
}
} // namespace

int main(){
    StereoSample stereo{};stereo.session=7;stereo.sequence=1;
    stereo.space_epoch=3;stereo.predicted_time=1000;stereo.focused=1;
    ControllerInputFrame frame{};frame.session=7;frame.sequence=1;
    frame.space_epoch=3;frame.predicted_time=1000;frame.focused=1;
    frame.sync_ns=999999900;frame.received_ns=1000000000;
    frame.hands[0].grip_active=1;frame.hands[0].grip_flags=15;
    frame.hands[0].grip.orientation.w=1;
    OriginalL2Context context{11,12,13,true,false,false,true};
    OriginalL2Aim aim;std::vector<OriginalL2Action> calls;
    auto execute=[&](OriginalL2Action action,std::uint64_t pc){
        assert(pc==12);calls.push_back(action);return true;};
    auto advance=[&]{return aim.advance(frame,stereo,context,1000000100,execute);};
    auto next=[&]{++frame.sequence;++stereo.sequence;};
    assert(advance()==OriginalL2Status::baseline);
    next();++frame.hands[0].pressed[control_trigger];
    assert(advance()==OriginalL2Status::pressed&&aim.owns_press());
    context.game_target_key=true;
    next();assert(advance()==OriginalL2Status::accepted&&calls.size()==1);
    next();++frame.hands[0].released[control_trigger];
    assert(advance()==OriginalL2Status::released&&!aim.owns_press());
    context.game_target_key=false;
    assert(calls.size()==2&&calls[0]==OriginalL2Action::press&&
           calls[1]==OriginalL2Action::release);
    next();++frame.hands[0].pressed[control_trigger];
    ++frame.hands[0].released[control_trigger];
    assert(advance()==OriginalL2Status::tapped&&calls.size()==4&&!aim.owns_press());
    context.game_target_key=true;
    next();++frame.hands[0].pressed[control_trigger];
    assert(advance()==OriginalL2Status::suppressed&&calls.size()==4);
    context.game_target_key=false;
    next();++frame.hands[0].released[control_trigger];
    assert(advance()==OriginalL2Status::suppressed&&calls.size()==4);
    context.toggle_targeting=true;
    next();++frame.hands[0].pressed[control_trigger];
    assert(advance()==OriginalL2Status::blocked&&calls.size()==4);
    next();++frame.hands[0].released[control_trigger];
    assert(advance()==OriginalL2Status::accepted&&calls.size()==4);
    context.toggle_targeting=false;
    next();++frame.hands[0].pressed[control_trigger];
    assert(advance()==OriginalL2Status::pressed&&aim.owns_press());
    context.game_target_key=true;
    frame.received_ns=800000000;
    assert(advance()==OriginalL2Status::recovered&&!aim.owns_press());
    context.game_target_key=false;frame.received_ns=1000000000;
    ++frame.input_epoch;next();
    frame.hands[0].pressed[control_trigger]=frame.hands[0].released[control_trigger]=0;
    assert(advance()==OriginalL2Status::baseline);
    next();++frame.hands[0].pressed[control_trigger];
    assert(advance()==OriginalL2Status::pressed&&aim.owns_press());
    context.game_target_key=true;context.weapon=99;
    assert(advance()==OriginalL2Status::abort_required&&aim.owns_press());
    assert(advance()==OriginalL2Status::abort_required);
    part_grant_reserves_first_press_before_reload_becomes_busy();
    cold_yield_still_requires_baseline_release_and_press();
    baseline_release_proof_requires_active_low_trigger_and_game_key_up();
    synchronous_yield_seals_held_and_missed_edges();allow_press_false_releases_without_a_new_frame();
    stowed_release_press_is_consumed_without_rearm();
    identity_toggle_and_execution_failure_keep_owned_sticky();foreign_and_suppressed_keys_are_not_released();
    std::puts("original L2 hold aim, safe synchronous yield, sticky abort and post-yield release/press rearm passed");
}
