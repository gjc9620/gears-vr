#pragma once
#include "weapon_rig_binding.h"
#include <cstdio>
#include <cstdlib>
namespace bow_arrow_tracking_fixture {
namespace wh=gears_weapon_hand;namespace rt=gears_rig_tracking;namespace er=gears_equipment_rig;
constexpr std::int64_t now=2000000000;
unsigned checks{};
#define BOW_ARROW_CHECK(x) do {++::bow_arrow_tracking_fixture::checks;if(!(x)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);std::abort();}} while(false)
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
        BOW_ARROW_CHECK(wh::frame_active(hand,now));BOW_ARROW_CHECK(rt::frame_valid(rig,now,rt::maximum_age_ns,source==1));
    }
    void recompute(){
        BOW_ARROW_CHECK(gears_vr::rig_tracking::pose(rig.base_view,rig.reference,rig.stereo.head,rig.units_per_metre,rig.world_head));
        for(unsigned i=0;i<2;++i)BOW_ARROW_CHECK(gears_vr::rig_tracking::pose(rig.base_view,rig.reference,rig.controllers.hands[i].grip,rig.units_per_metre,rig.world_grip[i]));
    }
    er::Status status(std::int64_t stamp=now) const {return er::match(hand,rig,stamp,source);}
};
}
