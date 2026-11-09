#include "cover_interaction.h"
#include <array>
#include <cstdio>
#include <limits>

using namespace gears_vr;
#define CHECK(condition) do{if(!(condition)){std::printf("Failed line %d: %s\n",__LINE__,#condition);throw __LINE__;}}while(0)
static bool close(double a,double b){return std::abs(a-b)<1e-9;}
static bool offset_is(CoverResult const& r,double right,double up){return close(r.offset.right,right)&&close(r.offset.up,up);}
static Vector plus(Vector a,Vector b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
static Vector times(Vector a,double b){return {a.x*b,a.y*b,a.z*b};}

struct Fixture {
    CoverInteraction kernel;
    CoverObservation cover;
    CoverHandSample hand;
    CoverOwnership owner;
    std::int64_t now=1000000000;
    explicit Fixture(CoverEdgeKind kind=CoverEdgeKind::left,double gain=1):kernel(gain){
        cover.sequence=1;cover.observed_ns=now;cover.epochs={1,1,1,1,1,1,1};
        cover.cover_valid=true;cover.interaction_enabled=true;
        cover.range={-.6,.5,0,.7};cover.edge={17,kind,true,true};
        hand.sequence=1;hand.observed_ns=now;hand.session_epoch=1;hand.space_epoch=1;
        hand.input_active=true;hand.position_valid=true;hand.position_tracked=true;
    }
    CoverResult run(){return kernel.update(cover,hand,owner,now);}
    void advance(){now+=10000000;++cover.sequence;++hand.sequence;cover.observed_ns=now;hand.observed_ns=now;}
    CoverResult tick(){advance();return run();}
    CoverResult release(){hand.lgrip_down=false;owner={};return tick();}
    CoverResult press(){
        hand.lgrip_down=true;auto request=tick();
        CHECK(request.phase==CoverPhase::requesting&&request.request_left_hand&&!request.owns_left_hand);
        CHECK(request.request_id!=0&&request.edge_id==cover.edge.id);
        owner={CoverLeftHandOwner::cover,request.request_id};
        auto granted=run();CHECK(granted.phase==CoverPhase::grabbing&&granted.owns_left_hand);
        CHECK(!granted.request_left_hand&&granted.request_id==request.request_id);
        return granted;
    }
    CoverResult start(){CHECK(run().phase==CoverPhase::attached);return press();}
    CoverResult move(Vector p){hand.left_position_metres=p;return tick();}
};

static void directions_and_release(){
    Fixture left;CHECK(offset_is(left.start(),0,0));
    CHECK(offset_is(left.move({.2,.3,.4}),-.2,0)); // Left hand right -> body left; off-axis motion ignored.
    CHECK(offset_is(left.move({.08,-.5,-.7}),-.08,0));
    CHECK(offset_is(left.release(),-.08,0));
    CHECK(offset_is(left.move({-5,7,3}),-.08,0));
    CHECK(left.run().phase==CoverPhase::held&&!left.run().owns_left_hand);
    CHECK(offset_is(left.press(),-.08,0)); // New anchor uses the current real hand and held body position.
    CHECK(offset_is(left.move({-5.05,7,3}),-.03,0));
    CHECK(offset_is(left.move({-5.08,7,3}),0,0));
    CHECK(left.release().phase==CoverPhase::attached);

    Fixture right(CoverEdgeKind::right);right.start();
    CHECK(offset_is(right.move({-.25,0,0}),.25,0));
    CHECK(offset_is(right.move({-.1,0,0}),.1,0));
    Fixture top(CoverEdgeKind::top);top.start();
    CHECK(offset_is(top.move({3,-.3,4}),0,.3));
    CHECK(offset_is(top.move({-7,-.12,-5}),0,.12));
    CHECK(offset_is(top.move({0,.2,0}),0,0));
    Fixture gain(CoverEdgeKind::left,2);gain.start();
    CHECK(offset_is(gain.move({.125,0,0}),-.25,0));
}

static void frozen_rotated_axes_and_no_drift(){
    // Independent expected directions for 0, +90 and 180 degree cover headings.
    for(Vector axis:{Vector{1,0,0},Vector{0,0,-1},Vector{-1,0,0}}){
        Fixture f;f.cover.right_axis_tracking=axis;f.hand.left_position_metres={2,3,4};f.start();
        CHECK(offset_is(f.move(plus({2,3,4},times(axis,.17))),-.17,0));
        CHECK(offset_is(f.move(plus({2,3,4},times(axis,.07))),-.07,0));
    }
    Fixture tilted(CoverEdgeKind::top);tilted.cover.right_axis_tracking={0,1,0};
    tilted.cover.up_axis_tracking={0,0,1};tilted.start();
    CHECK(offset_is(tilted.move({0,0,-.21}),0,.21));

    Fixture f;f.start();CHECK(offset_is(f.move({.23,0,0}),-.23,0));
    f.cover.right_axis_tracking={0,0,1}; // Later input axes cannot rotate the acquired grip.
    CHECK(offset_is(f.move({.31,0,0}),-.31,0));
    f.cover.edge.within_acquisition_reach=false; // Reach is an acquisition condition, not an elastic tether.
    for(unsigned i=0;i<10000;++i)CHECK(offset_is(f.tick(),-.31,0));
    for(unsigned i=0;i<10000;++i){++f.now;CHECK(offset_is(f.run(),-.31,0));}
    CHECK(f.run().owns_left_hand); // Fresh exact repeats are idempotent; there is no velocity integration.
}

static void acquisition_reach_does_not_tether_owned_grip(){
    for(CoverEdgeKind kind:{CoverEdgeKind::left,CoverEdgeKind::right,CoverEdgeKind::top}){
        Fixture f(kind);f.start();
        f.cover.edge.within_acquisition_reach=false;
        const Vector pull=kind==CoverEdgeKind::left?Vector{.2,0,0}:
            kind==CoverEdgeKind::right?Vector{-.2,0,0}:Vector{0,-.2,0};
        const double right=kind==CoverEdgeKind::left?-.2:kind==CoverEdgeKind::right?.2:0;
        const double up=kind==CoverEdgeKind::top?.2:0;
        auto away=f.move(pull);
        CHECK(away.owns_left_hand&&away.status==CoverStatus::okay&&offset_is(away,right,up));
        auto retract=f.move(times(pull,.5));
        CHECK(retract.owns_left_hand&&offset_is(retract,right*.5,up*.5));
        f.cover.edge.allowed=false; // Actual loss of edge permission still ends the grip.
        auto lost=f.tick();
        CHECK(lost.status==CoverStatus::edge_unavailable&&!lost.owns_left_hand&&
              offset_is(lost,right*.5,up*.5));
        f.cover.edge.allowed=true;
        CHECK(f.tick().status==CoverStatus::needs_release);
        f.release();f.hand.lgrip_down=true;
        auto outside=f.tick(); // A NEW grip must satisfy acquisition reach again.
        CHECK(outside.status==CoverStatus::edge_unavailable&&!outside.request_left_hand&&!outside.owns_left_hand);
    }
}

static void clipping_reverse_and_held_range_loss(){
    for(CoverEdgeKind kind:{CoverEdgeKind::left,CoverEdgeKind::right,CoverEdgeKind::top}){
        Fixture f(kind);f.start();
        const Vector pull=kind==CoverEdgeKind::left?Vector{2,0,0}:
            kind==CoverEdgeKind::right?Vector{-2,0,0}:Vector{0,-2,0};
        auto clipped=f.move(pull);
        const double right=kind==CoverEdgeKind::left?-.6:kind==CoverEdgeKind::right?.5:0;
        const double up=kind==CoverEdgeKind::top?.7:0;
        CHECK(offset_is(clipped,right,up)&&clipped.owns_left_hand&&clipped.offset_certified&&clipped.status==CoverStatus::clipped);
        // The reverse starts at the actual limited position, not at the
        // original press's out-of-range request. A centimetre must respond.
        const auto reverse=plus(pull,times(pull,-.005));
        const auto moved=f.move(reverse);
        CHECK(offset_is(moved,right+(kind==CoverEdgeKind::left?.01:kind==CoverEdgeKind::right?-.01:0),
                              up-(kind==CoverEdgeKind::top?.01:0)));
        for(unsigned i=0;i<10;++i)CHECK(offset_is(f.tick(),moved.offset.right,moved.offset.up));
        f.release();
        const auto held=moved.offset;
        if(kind==CoverEdgeKind::left)f.cover.range.min_right=-.2;
        if(kind==CoverEdgeKind::right)f.cover.range.max_right=.2;
        if(kind==CoverEdgeKind::top)f.cover.range.max_up=.2;
        auto invalid=f.tick();
        CHECK(!invalid.offset_certified&&!invalid.owns_left_hand&&offset_is(invalid,held.right,held.up)&&invalid.status==CoverStatus::range_invalidated);
        f.hand.lgrip_down=true;
        CHECK(!f.tick().request_left_hand&&!f.run().offset_certified);
        f.cover.range={-.6,.5,0,.7};
        auto recovered=f.tick();
        CHECK(recovered.offset_certified&&!recovered.owns_left_hand&&!recovered.request_left_hand&&
              offset_is(recovered,held.right,held.up));
        f.release();CHECK(offset_is(f.press(),held.right,held.up));
    }
    Fixture grabbing;grabbing.start();grabbing.move({.3,0,0});
    grabbing.cover.range.min_right=-.2;auto lost=grabbing.tick();
    CHECK(!lost.owns_left_hand&&!lost.offset_certified&&offset_is(lost,-.3,0));
    grabbing.cover.range.min_right=-.6;CHECK(!grabbing.tick().owns_left_hand);
    grabbing.release();CHECK(offset_is(grabbing.press(),-.3,0));
    Fixture gain(CoverEdgeKind::left,2);gain.cover.right_axis_tracking={0,0,-1};gain.start();
    CHECK(offset_is(gain.move({0,0,-2}),-.6,0));
    CHECK(offset_is(gain.move({0,0,-1.99}),-.58,0));
}

static void common_bounds_and_axis_changes(){
    Fixture f;f.start();CHECK(offset_is(f.move({.55,0,0}),-.55,0));f.release();
    for(unsigned i=0;i<20;++i){
        CHECK(offset_is(f.move({0,0,0}),i==0?-.55:-.6,0));
        f.press();CHECK(offset_is(f.move({.5,0,0}),-.6,0));f.release();
    }
    f.move({0,0,0});CHECK(offset_is(f.press(),-.6,0));
    CHECK(offset_is(f.move({-.25,0,0}),-.35,0)); // Reverse motion retracts from a regrab at the limit.
    f.release();f.cover.edge={18,CoverEdgeKind::top,true,true};f.move({0,0,0});
    CHECK(offset_is(f.press(),-.35,0));CHECK(offset_is(f.move({0,-1,0}),-.35,.7));
    f.release();f.move({0,0,0});f.press();CHECK(offset_is(f.move({0,-1,0}),-.35,.7));
    f.release();f.cover.range={-.2,.15,0,.4};
    auto lost=f.tick();CHECK(!lost.offset_certified&&offset_is(lost,-.35,.7));
    f.cover.range={-.6,.5,0,.7};CHECK(offset_is(f.tick(),-.35,.7));
    f.cover.edge={19,CoverEdgeKind::right,true,true};f.move({0,0,0});f.press();
    CHECK(offset_is(f.move({-2,0,0}),.5,.7)); // Both retained axes remain within the common rectangle.
}

static void ownership_and_edge_lock(){
    Fixture boot;boot.hand.lgrip_down=true;
    CHECK(boot.run().status==CoverStatus::needs_release);
    CHECK(boot.tick().status==CoverStatus::needs_release);boot.release();boot.press();

    Fixture conflict;conflict.run();conflict.owner={CoverLeftHandOwner::other,0};
    conflict.hand.lgrip_down=true;auto blocked=conflict.tick();
    CHECK(blocked.status==CoverStatus::ownership_conflict&&!blocked.request_left_hand&&!blocked.owns_left_hand);
    conflict.owner={};CHECK(conflict.tick().status==CoverStatus::needs_release);conflict.release();conflict.press();
    CHECK(offset_is(conflict.move({.2,0,0}),-.2,0));
    auto old_request=conflict.owner.request_id;conflict.owner={CoverLeftHandOwner::other,0};
    auto lost=conflict.tick();CHECK(lost.status==CoverStatus::ownership_conflict&&offset_is(lost,-.2,0));
    conflict.owner={CoverLeftHandOwner::cover,old_request};
    CHECK(!conflict.tick().owns_left_hand);conflict.release();
    CHECK(conflict.press().request_id>old_request);

    Fixture pending;pending.run();pending.hand.lgrip_down=true;auto request=pending.tick();
    CHECK(request.request_left_hand&&!request.owns_left_hand&&offset_is(request,0,0));
    pending.owner={CoverLeftHandOwner::free,request.request_id};
    CHECK(pending.run().request_left_hand&&!pending.run().owns_left_hand);
    CHECK(offset_is(pending.move({.4,0,0}),0,0)); // Waiting for a grant cannot move the body.
    pending.owner={CoverLeftHandOwner::cover,request.request_id};
    CHECK(offset_is(pending.run(),0,0)&&pending.run().owns_left_hand); // Delayed grant has no jump.
    CHECK(offset_is(pending.move({.5,0,0}),-.1,0));
    pending.cover.edge={99,CoverEdgeKind::right,true,true};auto switched=pending.tick();
    CHECK(switched.status==CoverStatus::edge_unavailable&&!switched.owns_left_hand&&offset_is(switched,-.1,0));
    CHECK(!pending.tick().request_left_hand);pending.release();CHECK(pending.press().edge_id==99);

    Fixture wrong;wrong.run();wrong.hand.lgrip_down=true;auto wanted=wrong.tick();
    wrong.owner={CoverLeftHandOwner::cover,wanted.request_id+1};
    CHECK(wrong.run().status==CoverStatus::ownership_conflict);wrong.owner={CoverLeftHandOwner::cover,wanted.request_id};
    CHECK(!wrong.tick().owns_left_hand);
    for(unsigned failure=0;failure<4;++failure){
        Fixture f;f.run();
        if(failure==0)f.cover.edge.id=0;
        if(failure==1)f.cover.edge.kind=CoverEdgeKind::none;
        if(failure==2)f.cover.edge.allowed=false;
        if(failure==3)f.cover.edge.within_acquisition_reach=false;
        f.hand.lgrip_down=true;auto denied=f.tick();
        CHECK(denied.status==CoverStatus::edge_unavailable&&!denied.request_left_hand);
        f.cover.edge={17,CoverEdgeKind::left,true,true};CHECK(!f.tick().request_left_hand);
        f.release();f.press();f.cover.edge.allowed=false;
        CHECK(f.tick().status==CoverStatus::edge_unavailable);
    }
}

static void freshness_replay_and_clocks(){
    Fixture f;f.start();CHECK(offset_is(f.move({.2,0,0}),-.2,0));
    f.now+=100000000;CHECK(f.run().owns_left_hand); // Age boundary is inclusive.
    ++f.now;auto stale=f.run();CHECK(stale.status==CoverStatus::stale&&!stale.offset_certified&&offset_is(stale,-.2,0));
    CHECK(!f.tick().owns_left_hand);f.release();f.press();

    for(unsigned failure=0;failure<8;++failure){
        Fixture s;s.start();s.move({.2,0,0});s.advance();
        if(failure==0)s.hand.observed_ns=s.now-100000001;
        if(failure==1)s.hand.observed_ns=s.now+1;
        if(failure==2)s.hand.observed_ns=0;
        if(failure==3)s.hand.sequence-=2;
        if(failure==4){s.hand.observed_ns=s.now-20000000;}
        if(failure==5)s.hand.sequence=0;
        if(failure==6)s.cover.observed_ns=s.now+1;
        if(failure==7)s.cover.observed_ns=0;
        auto r=s.run();CHECK((r.status==CoverStatus::stale||r.status==CoverStatus::replayed)&&!r.owns_left_hand);
        CHECK(offset_is(r,-.2,0));CHECK(!s.tick().owns_left_hand);
    }
    // Same sequence with changed hand position, grip state or cover permissions is a replay.
    for(unsigned payload=0;payload<3;++payload){
        Fixture r;r.start();r.move({.2,0,0});
        if(payload==0)r.hand.left_position_metres.x=.4;
        if(payload==1)r.hand.lgrip_down=false;
        if(payload==2)r.cover.edge.allowed=false;
        auto result=r.run();CHECK(result.status==CoverStatus::replayed&&!result.owns_left_hand&&offset_is(result,-.2,0));
        if(payload==0)r.hand.left_position_metres.x=.2;
        if(payload==1)r.hand.lgrip_down=true;
        if(payload==2)r.cover.edge.allowed=true;
        CHECK(r.run().status==CoverStatus::needs_release); // Restoring payload cannot resurrect an interrupted grant.
    }
    Fixture time;time.start();auto previous=time.now;--time.now;
    CHECK(time.run().status==CoverStatus::stale);time.now=previous;
    CHECK(time.run().status==CoverStatus::needs_release);
    time.now=(std::numeric_limits<std::int64_t>::max)();
    CHECK(time.run().status==CoverStatus::stale); // No signed subtraction overflow.
    Fixture negative;negative.now=(std::numeric_limits<std::int64_t>::min)();
    CHECK(negative.run().status==CoverStatus::stale);
}

static void tracking_interruptions_and_epochs(){
    for(unsigned flag=0;flag<4;++flag){
        Fixture f;f.start();f.move({.22,0,0});
        if(flag==0)f.hand.position_valid=false;
        if(flag==1)f.hand.position_tracked=false;
        if(flag==2)f.hand.input_active=false;
        if(flag==3)f.cover.interaction_enabled=false;
        auto stopped=f.tick();CHECK(stopped.status==CoverStatus::inactive&&stopped.offset_certified&&offset_is(stopped,-.22,0));
        CHECK(!stopped.owns_left_hand&&!stopped.request_left_hand);
        f.hand.position_valid=true;f.hand.position_tracked=true;f.hand.input_active=true;f.cover.interaction_enabled=true;
        CHECK(f.tick().status==CoverStatus::needs_release);f.release();CHECK(offset_is(f.press(),-.22,0));
        CHECK(offset_is(f.move({.12,0,0}),-.12,0));
    }
    const std::array<std::uint64_t CoverEpochs::*,7> members{
        &CoverEpochs::world,&CoverEpochs::pawn,&CoverEpochs::cover,&CoverEpochs::slot,
        &CoverEpochs::session,&CoverEpochs::space,&CoverEpochs::reference};
    for(unsigned i=0;i<members.size();++i){
        Fixture f;f.start();f.move({.2,0,0});auto previous_request=f.owner.request_id;
        ++(f.cover.epochs.*members[i]);f.hand.session_epoch=f.cover.epochs.session;f.hand.space_epoch=f.cover.epochs.space;
        if(i==4||i==5)f.hand.sequence=0; // A new source epoch may restart its sequence.
        auto changed=f.tick();CHECK(changed.status==CoverStatus::epoch_changed&&!changed.owns_left_hand);
        CHECK(offset_is(changed,i<3?0:-.2,0));CHECK(f.tick().status==CoverStatus::needs_release);
        f.release();auto regrab=f.press();CHECK(regrab.request_id>previous_request&&offset_is(regrab,i<3?0:-.2,0));
        --(f.cover.epochs.*members[i]);auto old=f.tick();
        CHECK(old.status==CoverStatus::replayed&&!old.owns_left_hand);
    }
    Fixture mismatch;mismatch.start();mismatch.move({.2,0,0});++mismatch.hand.space_epoch;
    auto wrong_space=mismatch.tick();CHECK(wrong_space.status==CoverStatus::epoch_changed&&offset_is(wrong_space,-.2,0));
    mismatch.hand.space_epoch=mismatch.cover.epochs.space;CHECK(!mismatch.tick().owns_left_hand);

    Fixture exit;exit.start();exit.move({.2,0,0});exit.cover.cover_valid=false;
    auto outside=exit.tick();CHECK(outside.phase==CoverPhase::outside&&!outside.offset_certified&&offset_is(outside,0,0));
    exit.cover.cover_valid=true;CHECK(exit.tick().status==CoverStatus::needs_release);
    exit.release();CHECK(offset_is(exit.press(),0,0));
}

static void invalid_numbers_and_ranges(){
    const double nan=(std::numeric_limits<double>::quiet_NaN)();
    const double inf=(std::numeric_limits<double>::infinity)();
    for(unsigned failure=0;failure<12;++failure){
        Fixture f;f.start();f.move({.2,0,0});
        if(failure==0)f.hand.left_position_metres.x=nan;
        if(failure==1)f.hand.left_position_metres.y=inf;
        if(failure==2)f.cover.range.min_right=nan;
        if(failure==3)f.cover.range.max_up=inf;
        if(failure==4)f.cover.range.min_right=.1;
        if(failure==5)f.cover.range.max_right=-.1;
        if(failure==6)f.cover.right_axis_tracking={};
        if(failure==7)f.cover.right_axis_tracking={2,0,0};
        if(failure==8)f.cover.up_axis_tracking=f.cover.right_axis_tracking;
        if(failure==9)f.cover.right_axis_tracking.x=nan;
        if(failure==10)f.cover.epochs.slot=0;
        if(failure==11)f.cover.range.min_up=.1;
        auto rejected=f.tick();CHECK(rejected.status==CoverStatus::invalid&&!rejected.owns_left_hand&&offset_is(rejected,-.2,0));
        CHECK(std::isfinite(rejected.offset.right)&&std::isfinite(rejected.offset.up));
    }
    Fixture overflow;overflow.hand.left_position_metres.x=-(std::numeric_limits<double>::max)();overflow.start();
    auto invalid=overflow.move({(std::numeric_limits<double>::max)(),0,0});
    CHECK(invalid.status==CoverStatus::invalid&&offset_is(invalid,0,0));
    for(unsigned config=0;config<4;++config){
        bool threw=false;
        try{[[maybe_unused]] CoverInteraction bad(config==0?0:config==1?-1:config==2?nan:1,config==3?0:1);}
        catch(std::invalid_argument const&){threw=true;}
        CHECK(threw);
    }
}

static void physical_head_motion_stays_separate(){
    Fixture f;f.start();f.move({.2,0,0});f.release();f.cover.edge={18,CoverEdgeKind::top,true,true};
    f.move({0,0,0});f.press();f.move({0,-.3,0});auto held=f.release();
    CHECK(offset_is(held,-.2,.3));
    // Composition example using existing math, NOT a claim of native integration:
    // the virtual offset is bounded, while genuine head movement remains intact.
    Viewpoint cover_eye{{10,20+held.offset.right*100,30+held.offset.up*100},{}};
    for(Vector physical:{Vector{},Vector{1.4,1.8,-.6},Vector{-1.1,-1.3,.8}}){
        Pose head{physical,{}};auto view=relative_view(cover_eye,{},head);
        CHECK(close(view.position.x,10-physical.z*100));
        CHECK(close(view.position.y,physical.x*100));
        CHECK(close(view.position.z,60+physical.y*100));
        CHECK(offset_is(f.tick(),-.2,.3));
    }
    Pose pure_turn{{},{0,std::sin(.7),0,std::cos(.7)}};
    auto rotated=relative_view(cover_eye,{},pure_turn);
    CHECK(close(rotated.position.x,cover_eye.position.x)&&close(rotated.position.y,cover_eye.position.y)&&
          close(rotated.position.z,cover_eye.position.z));
    CHECK(offset_is(f.run(),-.2,.3));
}

int main(){
    try{
        clipping_reverse_and_held_range_loss();
        directions_and_release();frozen_rotated_axes_and_no_drift();acquisition_reach_does_not_tether_owned_grip();
        common_bounds_and_axis_changes();
        ownership_and_edge_lock();freshness_replay_and_clocks();tracking_interruptions_and_epochs();
        invalid_numbers_and_ranges();physical_head_motion_stays_separate();
    }catch(int){return 1;}
    std::puts("Cover interaction CPU: directions, frozen axes, 20000-frame drift/repeat, acquisition reach versus held grip, held/regrab total bounds, ownership/edge lock, freshness/replay/clocks, tracking/epochs, finite/overflow and separate physical HMD composition PASS");
    return 0;
}
