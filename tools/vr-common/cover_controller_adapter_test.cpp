#include "cover_controller_adapter.h"
#include <cstdio>

using namespace gears_vr;
static unsigned checks=0;
#define CHECK(condition) do{++checks;if(!(condition)){std::printf("Failed line %d: %s\n",__LINE__,#condition);throw __LINE__;}}while(0)
static bool close(double a,double b){return std::abs(a-b)<1e-9;}
static bool position_is(Vector a,Vector b){return close(a.x,b.x)&&close(a.y,b.y)&&close(a.z,b.z);}
static bool offset_is(CoverResult const& r,double right,double up=0){return close(r.offset.right,right)&&close(r.offset.up,up);}

struct Stream {
    StereoSample stereo;
    ControllerInputFrame frame;
    ControllerEdgeCounter edges; // The imported publisher is the edge oracle.
    CoverControllerReference reference{1,1};
    std::int64_t now=1000000000;
    explicit Stream(float initial=0,StereoSource source=StereoSource::RuntimeTracked){
        stereo.source=source;stereo.session=41;stereo.sequence=1;stereo.space_epoch=2;
        stereo.focused=1;stereo.visible=1;stereo.should_render=1;stereo.view_flags=15;
        stereo.head_flags=source==StereoSource::RuntimeTracked?15:3;
        stereo.head={{0,1.6,0},{}};stereo.eyes[0].pose={{-.032,1.6,0},{}};
        stereo.eyes[1].pose={{.032,1.6,0},{}};
        for(auto& eye:stereo.eyes)eye.fov={-.7,.7,.7,-.7};
        for(auto& hand:frame.hands){
            hand.grip_active=1;hand.grip_flags=15;hand.squeeze_active=1;
        }
        frame.hands[0].grip.position={.25,1.2,-.4};
        frame.hands[1].grip.position={.4,1.1,-.5};
        frame.hands[0].squeeze=initial;publish();
    }
    void publish(){
        stereo.received_ns=now;stereo.predicted_time=now+20000000;
        frame.source=stereo.source;frame.session=stereo.session;frame.sequence=stereo.sequence;
        frame.space_epoch=stereo.space_epoch;frame.focused=stereo.focused;
        frame.received_ns=now;frame.sync_ns=now-1000;frame.predicted_time=stereo.predicted_time;
        edges.update(frame);CHECK(controller_frame_shape_valid(frame));
    }
    void next(float squeeze){now+=10000000;++stereo.sequence;frame.hands[0].squeeze=squeeze;publish();}
    CoverControllerResult read(CoverControllerAdapter& adapter,bool allow_sim=false){
        return adapter.update(&frame,stereo,reference,now,allow_sim);
    }
};

struct Integration {
    Stream stream;
    CoverControllerAdapter adapter;
    CoverInteraction kernel;
    CoverObservation cover;
    CoverOwnership owner;
    CoverControllerResult input;
    CoverResult body;
    Integration(){
        cover.sequence=1;cover.observed_ns=stream.now;cover.epochs={1,1,1,1,1,1,1};
        cover.cover_valid=true;cover.interaction_enabled=true;cover.range={-.6,.5,0,.7};
        cover.edge={17,CoverEdgeKind::left,true,true};
    }
    CoverResult consume(ControllerInputFrame const* frame){
        input=adapter.update(frame,stream.stereo,stream.reference,stream.now);
        body=kernel.update(cover,input.hand,owner,stream.now);
        if(!body.owns_left_hand)owner={}; // Native arbiter must release a relinquished grant.
        return body;
    }
    CoverResult run(){return consume(&stream.frame);}
    CoverResult next(float squeeze){
        stream.next(squeeze);++cover.sequence;cover.observed_ns=stream.now;
        cover.epochs.session=stream.reference.session_epoch;cover.epochs.space=stream.reference.space_epoch;
        return run();
    }
    CoverResult grant(){
        CHECK(body.request_left_hand&&body.request_id);owner={CoverLeftHandOwner::cover,body.request_id};
        auto result=run();CHECK(result.owns_left_hand);return result;
    }
    void start(){
        CHECK(run().phase==CoverPhase::attached&&input.hand.input_active&&!input.hand.lgrip_down);
        CHECK(next(.8f).request_left_hand);grant();
    }
    CoverResult drag(){stream.frame.hands[0].grip.position.x=.45;return next(.5f);}
};

static void exact_matching_and_source_policy(){
    for(unsigned mismatch=0;mismatch<7;++mismatch){
        Stream s;CoverControllerAdapter adapter;auto frame=s.frame;
        if(mismatch==0)++frame.sequence; // Newer hands are never substituted for the selected stereo frame.
        if(mismatch==1)++frame.session;
        if(mismatch==2)++frame.space_epoch;
        if(mismatch==3)++frame.predicted_time;
        if(mismatch==4)frame.source=StereoSource::MetaOperatorSimulated;
        if(mismatch==5)frame.focused=0;
        if(mismatch==6)frame.sync_ns=frame.received_ns+1;
        auto r=adapter.update(&frame,s.stereo,s.reference,s.now);
        CHECK(!r.hand.input_active&&!r.hand.position_valid&&position_is(r.hand.left_position_metres,{}));
        CHECK(r.status==CoverControllerStatus::mismatch);
    }
    for(bool simulated:{false,true})for(bool allow:{false,true}){
        Stream s(0,simulated?StereoSource::MetaOperatorSimulated:StereoSource::RuntimeTracked);
        CoverControllerAdapter adapter;auto r=s.read(adapter,allow);
        CHECK(r.hand.input_active==(simulated==allow));
        if(r.hand.input_active)CHECK(position_is(r.hand.left_position_metres,s.frame.hands[0].grip.position));
    }
    Stream s;CoverControllerAdapter adapter;auto r=s.read(adapter);
    CHECK(r.status==CoverControllerStatus::baseline&&r.hand.input_active&&!r.hand.lgrip_down);
    CHECK(r.hand.session_epoch==1&&r.hand.space_epoch==1&&r.hand.sequence==s.stereo.sequence);
}

static void cumulative_squeeze_and_initial_neutral(){
    Stream s;CoverControllerAdapter adapter;CHECK(s.read(adapter).hand.input_active);
    s.next(.8f);auto press=s.read(adapter);CHECK(press.hand.input_active&&press.hand.lgrip_down);
    CHECK(s.frame.hands[0].pressed[control_squeeze]==1&&s.frame.hands[0].released[control_squeeze]==0);
    for(float value:{.5f,.36f}){s.next(value);auto held=s.read(adapter);CHECK(held.hand.input_active&&held.hand.lgrip_down);}
    s.next(.35f);auto released=s.read(adapter);CHECK(released.hand.input_active&&!released.hand.lgrip_down);
    CHECK(s.frame.hands[0].released[control_squeeze]==1);
    s.next(.5f);CHECK(!s.read(adapter).hand.lgrip_down); // Same analog band, opposite state after release.
    s.next(.65f);CHECK(s.read(adapter).hand.lgrip_down);

    for(float initial:{.8f,.5f}){
        Stream held(initial);CoverControllerAdapter gate;CHECK(!held.read(gate).hand.input_active);
        held.next(.5f);CHECK(!held.read(gate).hand.input_active);
        if(initial==.5f){
            held.next(0);CHECK(!held.read(gate).hand.input_active); // Band baseline has no release edge yet.
            held.next(.8f);CHECK(!held.read(gate).hand.input_active);
        }
        held.next(0);auto neutral=held.read(gate);CHECK(neutral.hand.input_active&&!neutral.hand.lgrip_down);
        held.next(.8f);CHECK(held.read(gate).hand.lgrip_down);
    }
    // A single skipped press may be observed while still held, but the CURRENT
    // grip is used, never the historical squeeze-press pose carried by the wire.
    Stream skipped;CoverControllerAdapter gate;skipped.read(gate);
    skipped.frame.hands[0].grip.position.x=.3;skipped.next(.8f);
    skipped.frame.hands[0].grip.position.x=.7;skipped.next(.5f);
    CHECK(close(skipped.frame.hands[0].last_squeeze_press_grip.position.x,.3));
    auto current=skipped.read(gate);CHECK(current.hand.lgrip_down&&close(current.hand.left_position_metres.x,.7));
}

static void request_grant_release_and_repeat(){
    Integration f;f.start();auto repeated=f.run();
    CHECK(repeated.owns_left_hand&&offset_is(repeated,0));
    CHECK(f.drag().owns_left_hand&&offset_is(f.body,-.2));
    auto sample=f.input.hand;
    for(unsigned i=0;i<100;++i){
        CHECK(offset_is(f.run(),-.2)&&f.body.owns_left_hand);
        CHECK(f.input.hand.sequence==sample.sequence&&f.input.hand.observed_ns==sample.observed_ns&&
              f.input.hand.lgrip_down==sample.lgrip_down&&position_is(f.input.hand.left_position_metres,sample.left_position_metres));
    }
    CHECK(offset_is(f.next(0),-.2)&&!f.body.owns_left_hand);
    f.stream.frame.hands[0].grip.position.x=-2;CHECK(offset_is(f.next(0),-.2));
    CHECK(f.next(.8f).request_left_hand);CHECK(offset_is(f.grant(),-.2));
    f.stream.frame.hands[0].grip.position.x=-2.1;CHECK(offset_is(f.next(.5f),-.1));
}

static void continuity_boundaries(){
    for(unsigned boundary=0;boundary<4;++boundary){
        Integration f;f.start();f.drag();auto generation=f.input.continuity_generation;
        if(boundary==0){++f.stream.frame.profile_generation;f.stream.edges.profile_changed();}
        if(boundary==1)f.stream.edges.profile_changed();
        if(boundary==2){f.stream.stereo.session=7;++f.stream.reference.session_epoch;}
        if(boundary==3){++f.stream.stereo.space_epoch;++f.stream.reference.space_epoch;}
        auto interrupted=f.next(.8f); // New publisher epoch starts with a clearly held baseline.
        CHECK(!f.input.hand.input_active&&!interrupted.owns_left_hand&&offset_is(interrupted,-.2));
        CHECK(f.input.continuity_generation>generation);
        CHECK(!f.next(.5f).owns_left_hand&&!f.input.hand.input_active);
        f.next(0);CHECK(f.input.hand.input_active&&!f.input.hand.lgrip_down&&offset_is(f.body,-.2));
        CHECK(f.next(.8f).request_left_hand);CHECK(offset_is(f.grant(),-.2));
    }
    Stream bad;CoverControllerAdapter adapter;bad.read(adapter);
    bad.stereo.session=7;bad.next(0); // Caller failed to remap the non-monotonic raw session identity.
    CHECK(bad.read(adapter).status==CoverControllerStatus::replayed);
    Stream bad_space;CoverControllerAdapter space_adapter;bad_space.read(space_adapter);
    ++bad_space.stereo.space_epoch;bad_space.next(0);
    CHECK(bad_space.read(space_adapter).status==CoverControllerStatus::replayed);
    Integration neutral;neutral.run();++neutral.stream.frame.profile_generation;neutral.stream.edges.profile_changed();
    neutral.next(0);CHECK(!neutral.input.hand.input_active); // Existing kernel must see the boundary first.
    neutral.next(0);CHECK(neutral.input.hand.input_active&&!neutral.input.hand.lgrip_down);
    CHECK(neutral.next(.8f).request_left_hand);neutral.grant();
}

static void loss_and_freshness_preserve_held_offset(){
    for(unsigned loss=0;loss<8;++loss){
        Integration f;f.start();f.drag();auto generation=f.input.continuity_generation;
        if(loss==0){
            CHECK(!f.consume(nullptr).owns_left_hand&&offset_is(f.body,-.2));
            CHECK(position_is(f.input.hand.left_position_metres,{}));
        }else{
            if(loss==1){f.stream.stereo.focused=0;f.stream.next(.5f);}
            if(loss==2){f.stream.frame.hands[0].grip_flags=3;f.stream.next(.5f);}
            if(loss==3){f.stream.frame.hands[0].squeeze_active=0;f.stream.next(0);}
            if(loss==4){f.stream.stereo.head_flags=3;f.stream.next(.5f);}
            if(loss==5){f.stream.next(.5f);f.stream.frame.sync_ns=f.stream.now-100000001;}
            if(loss==6){f.stream.next(.5f);f.stream.frame.sequence+=1;}
            if(loss==7){f.stream.stereo.head.orientation={0,0,0,0};f.stream.next(.5f);}
            ++f.cover.sequence;f.cover.observed_ns=f.stream.now;
            CHECK(!f.run().owns_left_hand&&offset_is(f.body,-.2));
        }
        CHECK(!f.input.hand.input_active&&f.input.continuity_generation>generation);
        f.stream.stereo.focused=1;f.stream.stereo.head_flags=15;f.stream.stereo.head.orientation={};
        f.stream.frame.hands[0].grip_flags=15;f.stream.frame.hands[0].squeeze_active=1;
        CHECK(!f.next(.8f).owns_left_hand); // Recovery baseline is always inactive.
        CHECK(!f.next(.5f).owns_left_hand);f.next(0);
        CHECK(f.input.hand.input_active&&!f.input.hand.lgrip_down);
        CHECK(f.next(.8f).request_left_hand);f.grant();
    }
    Stream s;CoverControllerAdapter adapter;s.read(adapter);s.now+=100000001;
    CHECK(!s.read(adapter).hand.input_active);
    Stream stereo_old;CoverControllerAdapter other;stereo_old.stereo.received_ns-=100000001;
    CHECK(stereo_old.read(other).status==CoverControllerStatus::stale);
    Stream future;CoverControllerAdapter gate;++future.frame.received_ns;
    CHECK(!future.read(gate).hand.input_active);
    for(unsigned clock=0;clock<4;++clock){
        Stream backwards;CoverControllerAdapter reader;backwards.read(reader);auto old=backwards.frame;
        backwards.next(0);
        if(clock==0){backwards.frame.received_ns=old.received_ns-1;backwards.frame.sync_ns=old.sync_ns-1;}
        if(clock==1)backwards.frame.sync_ns=old.sync_ns-1;
        if(clock==2)backwards.stereo.received_ns=old.received_ns-1;
        if(clock==3){backwards.frame.predicted_time=old.predicted_time-1;backwards.stereo.predicted_time=old.predicted_time-1;}
        auto rejected=backwards.read(reader);
        CHECK(rejected.status==CoverControllerStatus::replayed&&!rejected.hand.input_active);
    }
}

static void ambiguous_edges_and_replay(){
    Integration f;f.run();f.stream.next(.8f);f.stream.next(0);
    ++f.cover.sequence;f.cover.observed_ns=f.stream.now;f.run();
    CHECK(f.input.status==CoverControllerStatus::ambiguous&&!f.body.owns_left_hand&&!f.body.request_left_hand);
    CHECK(!f.input.hand.input_active);f.next(0);
    CHECK(f.input.hand.input_active&&!f.input.hand.lgrip_down);
    CHECK(f.next(.8f).request_left_hand);f.grant();f.drag();
    f.stream.next(0);f.stream.next(.8f); // Release then re-press skipped while previously gripping.
    ++f.cover.sequence;f.cover.observed_ns=f.stream.now;
    CHECK(!f.run().owns_left_hand&&offset_is(f.body,-.2));
    CHECK(f.input.status==CoverControllerStatus::ambiguous);f.next(.5f);
    CHECK(!f.input.hand.input_active);f.next(0);CHECK(f.input.hand.input_active&&!f.input.hand.lgrip_down);

    Stream regress;CoverControllerAdapter adapter;regress.read(adapter);regress.next(.8f);regress.read(adapter);
    regress.next(.5f);regress.frame.hands[0].pressed[control_squeeze]=0;
    CHECK(regress.read(adapter).status==CoverControllerStatus::ambiguous);
    Stream many;CoverControllerAdapter missing_edges;many.read(missing_edges);
    for(unsigned i=0;i<2;++i){many.next(.8f);many.next(0);}
    CHECK(many.read(missing_edges).status==CoverControllerStatus::ambiguous);
    Integration replay;replay.start();replay.drag();
    replay.stream.frame.hands[0].grip.position.x=.8; // Changed payload under the selected sequence.
    CHECK(!replay.run().owns_left_hand&&offset_is(replay.body,-.2));
    CHECK(replay.input.status==CoverControllerStatus::replayed);
    replay.stream.frame.hands[0].grip.position.x=.45;
    CHECK(!replay.run().owns_left_hand); // Restoring the old payload cannot resurrect ownership.
}

static void head_motion_is_not_applied_to_hands_or_twice_to_body(){
    Integration f;f.start();f.drag();const Vector raw=f.stream.frame.hands[0].grip.position;
    const Pose neutral=f.stream.stereo.head;
    const Viewpoint anchor{{10,20+100*f.body.offset.right,30},{}};
    for(Vector displacement:{Vector{1.1,.8,-.4},Vector{-.7,-.5,.3},Vector{}}){
        f.stream.stereo.head.position={neutral.position.x+displacement.x,
            neutral.position.y+displacement.y,neutral.position.z+displacement.z};
        f.stream.stereo.head.orientation={0,std::sin(.7),0,std::cos(.7)};
        const auto rotation=rotation_matrix(f.stream.stereo.head.orientation);
        for(unsigned eye=0;eye<2;++eye){
            auto delta=transform(rotation,{eye==0?-.032:.032,0,0});
            f.stream.stereo.eyes[eye].pose={
                {f.stream.stereo.head.position.x+delta.x,f.stream.stereo.head.position.y+delta.y,
                 f.stream.stereo.head.position.z+delta.z},f.stream.stereo.head.orientation};
        }
        CHECK(f.next(.5f).owns_left_hand&&offset_is(f.body,-.2));
        CHECK(position_is(f.input.hand.left_position_metres,raw));
        auto view=relative_view(anchor,neutral,f.stream.stereo.head);
        CHECK(close(view.position.x,10-100*displacement.z));
        CHECK(close(view.position.y,100*displacement.x));
        CHECK(close(view.position.z,30+100*displacement.y));
    }
}

int main(){
    try{
        exact_matching_and_source_policy();cumulative_squeeze_and_initial_neutral();request_grant_release_and_repeat();
        continuity_boundaries();loss_and_freshness_preserve_held_offset();ambiguous_edges_and_replay();
        head_motion_is_not_applied_to_hands_or_twice_to_body();
    }catch(int){return 1;}
    std::printf("Cover controller adapter CPU PASS: 7 groups, 5 continuity boundary cases, 8 loss/recovery cases, 7 frame-key mismatches, 4 source-policy cases, 4 clock regressions; %u checks. Shared squeeze counters/hysteresis, neutral/held baselines, same-frame ownership, skipped-edge rearm and raw-hand/separate-HMD composition.\n",checks);
    return 0;
}
