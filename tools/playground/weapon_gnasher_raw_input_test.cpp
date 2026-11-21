#include "weapon_gnasher_raw_input.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#ifdef NDEBUG
#error GnasherRawInputTest requires active checks
#endif
namespace {
namespace wh=gears_weapon_hand;namespace rt=gears_rig_tracking;namespace er=gears_equipment_rig;
constexpr std::int64_t now=2000000000;
unsigned checks{};
#define CHECK(x) do {++checks;if(!(x)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);std::abort();}} while(false)
wh::ObjectIdentity object(std::uint64_t address,std::uint32_t index){
    return {address,0x8000+address,0x9000,index,index+10,index+2,1};
}
// Owned synthetic fixture derived from native_rig_tracking_frame_test.cpp and
// weapon_hand_client_test.cpp. No Native, bridge, game memory or runtime input.
struct Fixture {
    wh::Frame hand{};rt::Frame rig{};unsigned source{};
    explicit Fixture(unsigned selected=0):source(selected){
        auto& f=rig;f.status=rt::Status::ready;f.flags=rt::controllers_matched|rt::producer_continuity_available;
        f.native={21,31,41,51,61};f.publication_sequence=71;f.scene_invocation=81;f.world_generation=91;
        f.binding_generation=101;f.reference_generation=111;f.published_ns=now-1000000;
        f.scene_qpc=121;f.qpc_frequency=10000000;f.observed={131,141,151,{161,171}};
        f.native_reference={181,rt::reference_stable,0};f.producer={221,{231,241}};
        f.controller_publisher_pid=251;f.controller_publisher_start=261;f.view_policy=rt::first_person;
        f.world=object(0x1000,1);f.controller=object(0x1100,2);f.pawn=object(0x1200,3);
        f.mesh=object(0x1300,4);f.weapon=object(0x1400,5);f.units_per_metre=100;
        f.source_view={1,0,0,0,0,1,0,0,0,0,1,0,1,2,3,1};
        f.base_view=f.source_view;f.base_view[12]=4;f.base_view[13]=5;f.base_view[14]=6;
        f.reference={{.1,.2,.3},{0,.6,0,.8}};
        auto& s=f.stereo;s.source=static_cast<gears_vr::StereoSource>(source);s.session=191;s.sequence=201;s.space_epoch=211;
        s.received_ns=now-2000000;s.predicted_time=100000;s.head_flags=s.view_flags=15;s.focused=s.visible=s.should_render=1;
        s.head={{.2,.3,-.4},{0,0,.6,.8}};
        for(unsigned i=0;i<2;++i){s.eyes[i].pose={};s.eyes[i].pose.position.x=i?.032:-.032;s.eyes[i].fov={-.7,.7,.6,-.6};}
        auto& c=f.controllers;c.source=s.source;c.session=s.session;c.sequence=s.sequence;c.space_epoch=s.space_epoch;
        c.profile_generation=271;c.input_epoch=281;c.predicted_time=s.predicted_time;
        c.received_ns=now-3000000;c.sync_ns=now-4000000;c.focused=1;
        for(unsigned i=0;i<2;++i){auto& h=c.hands[i];h.grip_active=h.aim_active=1;h.grip_flags=h.aim_flags=15;
            h.grip={{.1+double(i)*.2,.4,-.5},{.6,0,0,.8}};h.aim={{-.1,.5,.2},{0,.6,0,.8}};
            h.trigger_active=h.squeeze_active=h.stick_active=1;h.trigger=.3f;h.squeeze=.4f;h.stick_x=-.2f;h.stick_y=.1f;
            h.last_squeeze_press_grip={{1,2,3},{0,0,.6,.8}};}
        f.world_pose_mask=rt::head_pose|rt::left_grip_pose|rt::right_grip_pose;recompute();
        hand.flags=wh::scene_owned|wh::input_matched|wh::gameplay_interactable|(source?wh::allow_sim:0u);
        hand.key={41,51,61,1,f.scene_invocation,91,21,31};hand.now_ns=now-1000000;
        hand.world=f.world;hand.controller=f.controller;hand.pawn=f.pawn;hand.mesh=f.mesh;hand.weapon=f.weapon;
        hand.reference_generation=f.reference_generation;hand.reference=f.reference;hand.stereo=f.stereo;hand.controllers=f.controllers;
        hand.stereo_status=unsigned(gears_vr::StereoStatus::accepted);hand.left_grip_known=hand.left_tracked=1;
        CHECK(wh::frame_active(hand,now));CHECK(rt::frame_valid(rig,now,rt::maximum_age_ns,source==1));
    }
    void recompute(){
        CHECK(gears_vr::rig_tracking::pose(rig.base_view,rig.reference,rig.stereo.head,rig.units_per_metre,rig.world_head));
        for(unsigned i=0;i<2;++i)CHECK(gears_vr::rig_tracking::pose(rig.base_view,rig.reference,rig.controllers.hands[i].grip,rig.units_per_metre,rig.world_grip[i]));
    }
    er::Status status(std::int64_t stamp=now) const {return er::match(hand,rig,stamp,source);}
};

wh::GripSnapshot ownership(Fixture const& f){
    wh::GripSnapshot g{};g.grip=wh::Grip::two;g.key=f.hand.key;
    g.world=f.hand.world;g.controller=f.hand.controller;g.pawn=f.hand.pawn;g.mesh=f.hand.mesh;g.weapon=f.hand.weapon;
    g.input_session=f.hand.stereo.session;g.input_sequence=f.hand.controllers.sequence;g.space_epoch=f.hand.stereo.space_epoch;
    g.profile_generation=f.hand.controllers.profile_generation;g.input_epoch=f.hand.controllers.input_epoch;
    g.reference_generation=f.hand.reference_generation;g.committed_ns=now-1000000;
    g.stereo_received_ns=f.hand.stereo.received_ns;g.input_received_ns=f.hand.controllers.received_ns;
    g.press_token=f.hand.press_token;g.left_hand.kind=wh::LeaseKind::support;g.left_hand.id=f.hand.press_token;g.left_hand.lifecycle=f.hand.key.lifecycle;return g;
}
bool convert(Fixture const& f,wh::GripSnapshot const* g,gears_weapon::GnasherMechanicalSample& out,
             std::uint64_t round=0,std::int64_t stamp=now){
    wh::GripGameState game{};game.known=game.main_hand_held=true;
    return gears_gnasher_raw::sample(f.hand,f.rig,game,g,stamp,1234,round,false,true,out);
}
}
int main(){
    {
        using namespace gears_weapon;
        Fixture f;GnasherMechanicalInteraction model;
        CHECK(gears_gnasher_raw::ownership_required(f.hand,model));
        const GnasherIdentity id{1,2,3,4,5,6,7,8};
        GnasherMechanicalConfig config{};
        config.calibration_id=1;config.lever_calibrated=config.feed_calibrated=true;
        config.capacity=8;config.game_ticks_per_second=1000;
        config.lever.left_front_grip_in_main.position={0,0,.08};
        config.lever.right_handle_home_in_main={0,-.2,0};
        config.lever.hinge_axis_in_main={1,0,0};
        CHECK(model.bind(id,config,{id,1,2,10}));
        CHECK(!gears_gnasher_raw::ownership_required(f.hand,model));
        f.hand.left_grip_down=1;CHECK(gears_gnasher_raw::ownership_required(f.hand,model));
        f.hand.left_grip_down=0;f.hand.left_grip_known=0;
        CHECK(gears_gnasher_raw::ownership_required(f.hand,model));
        f.hand.left_grip_known=1;
        GnasherMechanicalSample baseline{};CHECK(convert(f,nullptr,baseline));baseline.identity=id;
        CHECK(model.observe(baseline).event==GnasherMechanicalEvent::none);
        auto loading=model;
        CHECK(loading.begin_reload(id,baseline.monotonic_ns,baseline.game_tick).event==GnasherMechanicalEvent::reload_started);
        CHECK(gears_gnasher_raw::ownership_required(f.hand,loading));
        CHECK(model.suspend_hold());CHECK(gears_gnasher_raw::ownership_required(f.hand,model));
        GnasherMechanicalInteraction empty;CHECK(empty.bind(id,config,{id,1,0,10}));
        CHECK(gears_gnasher_raw::ownership_required(f.hand,empty));
        GnasherMechanicalInteraction debt;
        GnasherShotReceipt receipt{};
        receipt.identity=id;receipt.original_event_sequence=1;
        receipt.monotonic_ns=now;receipt.game_tick=1000;
        receipt.observed={id,2,1,10};
        CHECK(debt.bind_after_original_shot(config,receipt));
        CHECK(gears_gnasher_raw::ownership_required(f.hand,debt));
    }
    for(unsigned source=0;source<2;++source){
        Fixture f(source);f.hand.press_token=1;f.hand.left_grip_down=1;auto g=ownership(f);
        gears_weapon::GnasherMechanicalSample out{};
        CHECK(convert(f,&g,out));CHECK(out.left_front_held&&out.right_handle_held);
        CHECK(out.monotonic_ns==f.hand.controllers.sync_ns&&out.sequence==f.hand.controllers.sequence);
        CHECK(out.original_ready&&!out.original_animation_active&&out.game_tick==1234);
        CHECK(out.reference_generation==f.hand.reference_generation);
        CHECK(out.identity.world==f.hand.world.address&&out.identity.pawn==f.hand.pawn.address&&out.identity.weapon==f.hand.weapon.address);
        CHECK(out.identity.world_generation==f.hand.key.world_generation&&out.identity.session==f.hand.stereo.session);
        CHECK(out.identity.space_epoch==f.hand.stereo.space_epoch&&out.identity.input_epoch==f.hand.controllers.input_epoch);
        CHECK(out.identity.profile_generation==f.hand.controllers.profile_generation);
        for(unsigned i=0;i<2;++i){auto const& pose=i?out.right_grip:out.left_grip;
            CHECK(std::abs(pose.position.x-f.rig.world_grip[i].position.x/100)<1e-12);
            CHECK(std::abs(pose.position.y-f.rig.world_grip[i].position.y/100)<1e-12);
            CHECK(std::abs(pose.position.z-f.rig.world_grip[i].position.z/100)<1e-12);
            CHECK(gears_vr::stereo_same_pose({{},pose.orientation},{{},f.rig.world_grip[i].orientation}));}
        CHECK(convert(f,nullptr,out));CHECK(!out.left_front_held);
        CHECK(convert(f,nullptr,out,42));CHECK(out.left_round_serial==42&&!out.left_front_held);
        CHECK(!convert(f,&g,out,42));CHECK(!out.identity.valid());
        CHECK(convert(f,&g,out,0,now+90000000));CHECK(out.monotonic_ns==f.hand.controllers.sync_ns);
        CHECK(!convert(f,&g,out,0,now+100000001));CHECK(!out.identity.valid());
        for(unsigned field=0;field<11;++field){auto wrong=g;
            if(field==0)++wrong.key.frame;if(field==1)++wrong.world.registry_index;if(field==2)++wrong.pawn.registry_index;
            if(field==3)++wrong.controller.registry_index;if(field==4)++wrong.mesh.registry_index;if(field==5)++wrong.weapon.registry_index;
            if(field==6)++wrong.input_session;if(field==7)++wrong.input_sequence;if(field==8)++wrong.space_epoch;
            if(field==9)++wrong.input_epoch;if(field==10)++wrong.profile_generation;
            CHECK(!convert(f,&wrong,out));CHECK(!out.identity.valid());}
        {auto wrong=g;++wrong.reference_generation;CHECK(!convert(f,&wrong,out));}
        {auto wrong=g;wrong.committed_ns=now+1;CHECK(!convert(f,&wrong,out));}
        {auto wrong=g;wrong.left_hand.kind=wh::LeaseKind::ammo;CHECK(convert(f,&wrong,out));CHECK(!out.left_front_held);}
        {auto wrong=g;wrong.left_hand.id=0;CHECK(!convert(f,&wrong,out));}
        {auto wrong=g;wrong.grip=wh::Grip::single;CHECK(convert(f,&wrong,out));CHECK(!out.left_front_held);}
        {auto wrong=g;++wrong.left_hand.lifecycle;CHECK(!convert(f,&wrong,out));}
        {auto wrong=g;++wrong.left_hand.id;CHECK(convert(f,&wrong,out));CHECK(!out.left_front_held);}
        {auto wrong=g;++wrong.press_token;CHECK(!convert(f,&wrong,out));}
        f.hand.left_grip_down=0;CHECK(convert(f,&g,out));CHECK(!out.left_front_held);
    }
    for(unsigned field=0;field<8;++field){Fixture f;gears_weapon::GnasherMechanicalSample out{};
        if(field==0)++f.hand.key.frame;if(field==1)++f.hand.controllers.input_epoch;
        if(field==2)++f.hand.controllers.profile_generation;if(field==3)f.hand.recentered=1;
        if(field==4)f.hand.heading_deferred=1;if(field==5)f.rig.world_pose_mask&=~rt::left_grip_pose;
        if(field==6)f.rig.world_grip[1].position.x+=.05;if(field==7)++f.rig.native.registration_cookie;
        CHECK(!convert(f,nullptr,out));CHECK(!out.identity.valid());}
    Fixture f;wh::GripGameState game{};gears_weapon::GnasherMechanicalSample out{};
    CHECK(!gears_gnasher_raw::sample(f.hand,f.rig,game,nullptr,now,0,0,false,true,out));
    game.known=true;CHECK(!gears_gnasher_raw::sample(f.hand,f.rig,game,nullptr,now,0,0,false,true,out));
    game.main_hand_held=true;CHECK(!gears_gnasher_raw::sample(f.hand,f.rig,game,nullptr,now,-1,0,false,true,out));
    std::printf("GnasherRawInputTest: %u checks passed (synthetic raw/rig data only)\n",checks);
}
