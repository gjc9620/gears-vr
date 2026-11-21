#define GEARS_TROIKA_CONTROL_LIVE_TEST
#include "troika_scene_tracking_test.cpp"
#include <cstdlib>
#undef CHECK
#define CHECK(x) do{++checks;if(!(x)){std::printf("FAIL %u: %s\n",__LINE__,#x);std::exit(1);}}while(0)
void run_control_owner_cases(gears_vr::StereoResult& stereo){
    namespace live=troika_control_live;namespace geo=gears_troika_geometry;namespace ctrl=gears_troika_control;
    namespace track=troika_scene_tracking;namespace hand=gears_weapon_hand;namespace vr=gears_vr;
    auto& input=cover_input_live::input;auto& left=input.hands[0];left.squeeze_active=1;left.trigger_active=1;
    auto& right=input.hands[1];right.squeeze_active=right.trigger_active=1;
    std::uint64_t query{};bool was_down{},right_down{},trigger_down{},was_right{},was_trigger{};
    unsigned right_contacts=2,right_press_contacts=2,assigned_left=0;int original_left_contact=-1;
    geo::Request last_request{};ctrl::Decision last_decision{};
    const auto step=[&](bool down,unsigned contacts,bool accept=true,bool right_tracked=true){
        current_ns+=10000000;++fixture_scene.invocation;
        fixture_scene.scene_qpc=current_ns-2000000;fixture_scene.family_qpc=fixture_scene.scene_qpc+1;fixture_scene.renderer_qpc=fixture_scene.scene_qpc+2;
        ++stereo.sample.sequence;stereo.sample.predicted_time+=10000000;stereo.sample.received_ns=current_ns-1000000;
        input.sequence=stereo.sample.sequence;input.predicted_time=stereo.sample.predicted_time;input.received_ns=current_ns-2000000;input.sync_ns=current_ns-3000000;
        left.squeeze=down?1.f:0.f;
        if(down!=was_down){
            if(down){++left.pressed[vr::control_squeeze];left.last_squeeze_press_sequence=input.sequence;
                left.last_squeeze_press_flags=15;left.last_squeeze_press_grip=left.grip;}
            else ++left.released[vr::control_squeeze];
        }
        was_down=down;input.hands[1].grip_active=right_tracked?1u:0u;input.hands[1].grip_flags=right_tracked?15u:0u;
        right.squeeze=right_down?1.f:0.f;right.trigger=trigger_down&&right.trigger_active?1.f:0.f;
        if(right_down!=was_right){if(right_down){++right.pressed[vr::control_squeeze];right.last_squeeze_press_sequence=input.sequence;
            right.last_squeeze_press_flags=15;right.last_squeeze_press_grip=right.grip;}else ++right.released[vr::control_squeeze];}
        if(trigger_down!=was_trigger){if(trigger_down)++right.pressed[vr::control_trigger];else ++right.released[vr::control_trigger];}
        was_right=right_down;was_trigger=trigger_down;
        const bool prepared=track::prepare(&fixture_scene,stereo);
        if(!prepared){
            std::printf("source rejected query=%llu trigger_active=%u value=%f input_shape=%u frame_match=%u\n",query+1,right.trigger_active,
                double(right.trigger),unsigned(vr::controller_hand_shape_valid(right,1)),unsigned(vr::controller_frame_matches(input,stereo.sample,current_ns)));
        }
        CHECK(prepared);
        geo::Request q{};q.query_id=++query;q.source.frame=track::value;
        q.source.entry_binding=track::retained_basis.owner.binding;q.source.entry_token=track::retained_basis.owner.token;
        geo::Response g{};g.query_id=q.query_id;g.publication_sequence=q.source.frame.publication_sequence;g.source_ticket=q.source.frame.source_ticket;
        g.entry_binding=q.source.entry_binding;g.entry_token=q.source.entry_token;g.component=q.source.frame.turret_mesh;
        g.asset={0x7710,0,0x9010,17,18,0,1};g.tracked_mask=right_tracked?3u:1u;g.contacts[0]=contacts;
        g.hand_handle[0]=assigned_left;g.hand_handle[1]=1-assigned_left;
        for(unsigned handle=0;handle<2;++handle)if(contacts&(1u<<handle))g.distances[0][handle]=.01;
        g.contacts[1]=right_tracked?right_contacts:0;
        for(unsigned h=0;h<2;++h)if(g.contacts[1]&(1u<<h))g.distances[1][h]=.01;
        for(unsigned side=0;side<2;++side)if(input.hands[side].last_squeeze_press_sequence){
            g.press_tracked_mask|=1u<<side;
            g.press_contacts[side]=side?right_press_contacts:original_left_contact<0?contacts:unsigned(original_left_contact);
            for(unsigned h=0;h<2;++h)if(g.press_contacts[side]&(1u<<h))g.press_distances[side][h]=.01;
        }
        CHECK(geo::valid(g,q));ctrl::Decision d{};
        CHECK(live::decide(q,g,d));CHECK(ctrl::valid(d,q,current_ns));
        ctrl::Decision premature{};CHECK(!live::read_for(q.source,premature)); // Native complete is mandatory.
        CHECK(live::complete(q,d,accept)==accept);
        ctrl::Decision read{};CHECK(live::read_for(q.source,read)==accept);
        if(accept)CHECK(read.query_id==q.query_id&&read.lease.kind==d.lease.kind);
        last_request=q;last_decision=d;return d;
    };
    auto d=step(false,1);CHECK(d.lease.kind==hand::LeaseKind::none);
    d=step(false,1);CHECK(d.lease.kind==hand::LeaseKind::none);
    left.trigger=1;++left.pressed[vr::control_trigger];d=step(false,1);CHECK(d.lease.kind==hand::LeaseKind::none);
    left.trigger=0;++left.released[vr::control_trigger];d=step(false,1);CHECK(d.lease.kind==hand::LeaseKind::none); // Index is never LGrip here.
    d=step(true,1);CHECK(d.lease.kind==hand::LeaseKind::support&&d.new_grant);
    const auto lease=d.lease;const auto token=d.press_token;
    geo::Source hint{};CHECK(live::driver_hint(last_request.source.frame.driver_mesh.address,hint));
    CHECK(!std::memcmp(&hint,&last_request.source,sizeof hint));
    CHECK(!live::driver_hint(last_request.source.frame.turret_mesh.address,hint)&&!hint.frame.publication_sequence);
    ctrl::Decision exported{};CHECK(live::read(&last_request.source,&exported)==gears_rig_tracking::Reply::accepted);
    CHECK(hand::same(exported.lease,lease)&&weapon_hand_runtime::detail::calls==0);
    auto forged=last_request.source;++forged.entry_token;
    CHECK(live::read(&forged,&exported)==gears_rig_tracking::Reply::unavailable&&!exported.query_id);
    CHECK(live::read(nullptr,&exported)==gears_rig_tracking::Reply::invalid&&!exported.query_id);
    CHECK(live::read(&last_request.source,nullptr)==gears_rig_tracking::Reply::invalid);
    for(unsigned i=0;i<8;++i){d=step(true,0);CHECK(hand::same(d.lease,lease)&&d.press_token==token&&!d.new_grant);}
    // No contact on the next sample is not a new acquisition: the same true
    // owner holds its handle while raw palms measure mechanical motion.
    auto retained_request=last_request;auto retained_decision=last_decision;
    d=step(false,1);CHECK(d.lease.kind==hand::LeaseKind::none);
    d=step(true,0);CHECK(d.lease.kind==hand::LeaseKind::none);
    d=step(true,1);CHECK(d.lease.kind==hand::LeaseKind::none); // Held return cannot acquire.
    step(false,1);d=step(true,2);CHECK(d.lease.kind==hand::LeaseKind::none); // Wrong shaft.
    step(false,1);d=step(true,3);CHECK(d.lease.kind==hand::LeaseKind::none); // Ambiguous shafts.
    step(false,1);d=step(true,1,false);CHECK(d.lease.kind==hand::LeaseKind::support);
    d=step(true,1);CHECK(d.lease.kind==hand::LeaseKind::none); // Failed PG commit must not retain.
    step(false,1);d=step(true,1);CHECK(d.lease.kind==hand::LeaseKind::support);
    ++input.input_epoch;
    d=step(true,1);CHECK(d.lease.kind==hand::LeaseKind::none);
    d=step(true,1);CHECK(d.lease.kind==hand::LeaseKind::none);
    step(false,1);step(false,1);d=step(true,1);CHECK(d.lease.kind==hand::LeaseKind::support);
    d=step(true,1,true,false);CHECK(d.lease.kind==hand::LeaseKind::none); // Either hand's tracking loss.
    d=step(true,1);CHECK(d.lease.kind==hand::LeaseKind::none);
    step(false,1);step(false,1);d=step(true,1);CHECK(d.lease.kind==hand::LeaseKind::support);
    CHECK(!live::complete(retained_request,retained_decision,false));
    ctrl::Decision current{};CHECK(live::read_for(last_request.source,current)&&current.lease.kind==hand::LeaseKind::support);
    auto wrong=last_request.source;++wrong.frame.driver.registry_index;CHECK(!live::read_for(wrong,current));
    // Current contact cannot erase the actual outside press position.
    step(false,1);original_left_contact=0;d=step(true,1);CHECK(d.lease.kind==hand::LeaseKind::none);
    original_left_contact=-1;d=step(true,1);CHECK(d.lease.kind==hand::LeaseKind::none);
    step(false,1);d=step(true,1);CHECK(d.lease.kind==hand::LeaseKind::support);
    current_ns+=100000001;d=step(true,1);CHECK(d.lease.kind==hand::LeaseKind::none); // Inter-source gap is a boundary.
    step(false,1);step(false,1);d=step(true,1);CHECK(d.lease.kind==hand::LeaseKind::support);
    right_down=true;d=step(true,1);CHECK(d.both_owned&&d.right_owned&&!d.fire_requested);
    trigger_down=true;d=step(true,1);CHECK(d.fire_requested&&d.fire_edge_sequence==input.sequence);
    auto forged_control=d;++forged_control.right_press_sequence;CHECK(!ctrl::valid(forged_control,last_request,current_ns));
    forged_control=d;forged_control.both_owned=0;CHECK(!ctrl::valid(forged_control,last_request,current_ns));
    forged_control=d;forged_control.fire_window_begin_ns=input.sync_ns+1;CHECK(!ctrl::valid(forged_control,last_request,current_ns));
    forged_control=d;forged_control.fire_edge_sequence=input.sequence+1;CHECK(!ctrl::valid(forged_control,last_request,current_ns));
    const auto fire_sequence=d.fire_edge_sequence;const auto fire_begin=d.fire_window_begin_ns;
    for(unsigned i=0;i<12;++i){d=step(true,0);CHECK(d.fire_requested&&d.fire_edge_sequence==fire_sequence&&d.fire_window_begin_ns==fire_begin);}
    d=step(false,1);CHECK(!d.both_owned&&!d.fire_requested&&d.right_owned);
    d=step(true,1);CHECK(d.both_owned&&!d.fire_requested); // R2 held through regrab cannot fire.
    trigger_down=false;step(true,1);trigger_down=true;d=step(true,1);CHECK(d.fire_requested&&d.fire_edge_sequence!=fire_sequence);
    right_down=false;d=step(true,1);CHECK(!d.right_owned&&!d.fire_requested);
    right_down=true;right_press_contacts=0;d=step(true,1);CHECK(!d.right_owned&&!d.fire_requested);
    right_press_contacts=2;d=step(true,1);CHECK(!d.right_owned); // Outside press held return.
    right_down=false;step(true,1);right_down=true;d=step(true,1);CHECK(d.both_owned&&!d.fire_requested);
    trigger_down=false;step(true,1);trigger_down=true;d=step(true,1);CHECK(d.fire_requested);
    right.trigger_active=0;d=step(true,1);CHECK(!d.trigger_known&&!d.fire_requested);
    right.trigger_active=1;d=step(true,1);CHECK(!d.fire_requested);
    trigger_down=false;step(true,1);trigger_down=true;d=step(true,1);CHECK(d.fire_requested);
    right.pressed[vr::control_trigger]+=2;right.released[vr::control_trigger]+=2;
    d=step(true,1);CHECK(!d.trigger_known&&!d.fire_requested); // Skipped counter edges are not replayed.
    trigger_down=false;step(true,1);trigger_down=true;d=step(true,1);CHECK(d.fire_requested);
    d=step(true,1,false);CHECK(d.fire_requested); // Candidate does not survive rejected PG commit.
    d=step(true,1);CHECK(!d.right_owned&&!d.fire_requested&&d.lease.kind==hand::LeaseKind::none);
    right_down=false;trigger_down=false;step(false,2);step(false,2);
    d=step(true,2);CHECK(d.lease.kind==hand::LeaseKind::none); // Old assignment still rejects physical handle_R.
    assigned_left=1;d=step(true,2);CHECK(d.lease.kind==hand::LeaseKind::none); // Assignment change is a release boundary.
    step(false,2);d=step(true,2);CHECK(d.lease.kind==hand::LeaseKind::support);
    right_contacts=right_press_contacts=1;right_down=true;d=step(true,2);CHECK(d.both_owned&&!d.fire_requested);
    trigger_down=true;d=step(true,2);CHECK(d.fire_requested);
    assigned_left=0;d=step(true,1);CHECK(!d.fire_requested&&!d.right_owned&&d.lease.kind==hand::LeaseKind::none);
    current_ns+=100000001;CHECK(!live::read_for(last_request.source,current));current_ns-=100000001;
    live::stop();CHECK(!live::read_for(last_request.source,current));
    CHECK(!live::driver_hint(last_request.source.frame.driver_mesh.address,hint)&&!hint.frame.publication_sequence);
    CHECK(live::read(&last_request.source,&exported)==gears_rig_tracking::Reply::stopping&&!exported.query_id);
    std::printf("Troika real shared participant/left-owner mounted lifecycle: %u checks passed; synthetic engine/input only.\n",checks);
}
