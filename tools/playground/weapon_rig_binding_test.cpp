#include "weapon_rig_binding.h"
#include <cstdio>
#include <cstdlib>
#include <limits>

#ifdef NDEBUG
#error WeaponRigBindingTest requires active checks
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
template<class Change>void rejected(Change change,er::Status expected){Fixture f;change(f);CHECK(f.status()==expected);
    gears_hand_continuity::TrackedInput input{};input.input.sequence=99;
    gears_hand_continuity::Publisher publisher{99,99};
    CHECK(!er::continuity(f.hand,f.rig,now,f.source,input,publisher));
    CHECK(!input.input.sequence&&!publisher.pid&&!publisher.start);}
static_assert(noexcept(er::match(wh::Frame{},rt::Frame{},now,0)));
}
int main(){
    for(unsigned source=0;source<2;++source){Fixture f(source);
        gears_hand_continuity::TrackedInput input{};gears_hand_continuity::Publisher publisher{};
        // The selected row is94ms old: a90Hz eight-row transport ring may
        // have overwritten it, while Native's SAME Scene remains valid.
        CHECK(er::continuity(f.hand,f.rig,now+90000000,source,input,publisher));
        CHECK(gears_hand_continuity::same_input(input.input,f.hand.controllers));
        CHECK(input.input.sync_ns==now-4000000&&input.stream_epoch==f.rig.producer.stream);
        CHECK(publisher.pid==f.rig.controller_publisher_pid&&publisher.start==f.rig.controller_publisher_start);
        CHECK(!er::continuity(f.hand,f.rig,now+100000001,source,input,publisher));
        CHECK(!input.input.sequence&&!publisher.pid&&!publisher.start);
    }
    for(unsigned source=0;source<2;++source){Fixture f(source);CHECK(f.status()==er::Status::matched);
        CHECK(er::match(f.hand,f.rig,now,1-source)==er::Status::source_mismatch);
        CHECK(er::match(f.hand,f.rig,now,2)==er::Status::invalid_source);
        CHECK(f.status(now+rt::maximum_age_ns)==er::Status::inactive_hand);}
    for(unsigned field=0;field<5;++field)rejected([field](auto& f){auto& n=f.rig.native;
        if(field==0)++n.game_pid;if(field==1)++n.game_thread;if(field==2)++n.game_start;
        if(field==3)++n.native_started_qpc;if(field==4)++n.registration_cookie;},er::Status::native_mismatch);
    rejected([](auto& f){++f.hand.key.frame;},er::Status::scene_mismatch);
    rejected([](auto& f){++f.rig.scene_invocation;},er::Status::scene_mismatch);
    // Same original input and timestamps can be reused by separate Scenes.
    // Neither the public publication serial nor a C0 counter can substitute.
    rejected([](auto& f){f.hand.key.frame=f.rig.publication_sequence;},er::Status::scene_mismatch);
    {Fixture f;f.rig.publication_sequence+=10;CHECK(f.status()==er::Status::matched);}
    for(unsigned object=0;object<5;++object)for(unsigned field=0;field<7;++field)
        rejected([object,field](auto& f){wh::ObjectIdentity* objects[]={&f.rig.world,&f.rig.controller,&f.rig.pawn,&f.rig.mesh,&f.rig.weapon};
            auto& o=*objects[object];if(field==0)o.address+=8;if(field==1)o.outer+=8;if(field==2)o.klass+=8;
            if(field==3)++o.registry_index;if(field==4)++o.name_index;if(field==5)++o.name_number;
            if(field==6){o={};} // Empty weapon is shape-valid; other objects are invalid.
        },field==6&&object!=4?er::Status::invalid_rig:er::Status::objects_mismatch);
    rejected([](auto& f){++f.rig.world_generation;},er::Status::objects_mismatch);
    rejected([](auto& f){++f.rig.reference_generation;},er::Status::reference_mismatch);
    rejected([](auto& f){f.rig.reference.position.x+=.01;},er::Status::reference_mismatch);
    rejected([](auto& f){f.hand.recentered=1;},er::Status::reference_transition);
    rejected([](auto& f){f.hand.heading_deferred=1;},er::Status::reference_transition);
    rejected([](auto& f){f.rig.stereo.eyes[0].fov.left+=.01;},er::Status::stereo_mismatch);
    rejected([](auto& f){f.rig.stereo.head.position.x+=.01;},er::Status::stereo_mismatch);
    rejected([](auto& f){f.rig.controllers.hands[1].trigger+=.01f;},er::Status::controllers_mismatch);
    rejected([](auto& f){++f.rig.controllers.hands[0].pressed[gears_vr::control_x];},er::Status::controllers_mismatch);
    rejected([](auto& f){f.rig.controllers.hands[1].last_squeeze_press_grip.position.x+=.01;},er::Status::controllers_mismatch);
    rejected([](auto& f){f.rig.controllers.received_ns-=1;},er::Status::controllers_mismatch);
    rejected([](auto& f){f.rig.flags&=~rt::producer_continuity_available;f.rig.producer={};},er::Status::missing_continuity);
    rejected([](auto& f){f.rig.flags&=~rt::controllers_matched;},er::Status::missing_continuity);
    rejected([](auto& f){f.rig.producer.stream=0;},er::Status::invalid_rig);
    rejected([](auto& f){f.hand.now_ns=now-100000001;},er::Status::inactive_hand);
    rejected([](auto& f){f.hand.now_ns=now+1;},er::Status::inactive_hand);
    rejected([](auto& f){f.rig.published_ns=now-100000001;},er::Status::invalid_rig);
    rejected([](auto& f){f.rig.world_pose_mask&=~rt::right_grip_pose;},er::Status::missing_right_grip);
    rejected([](auto& f){f.rig.controllers.hands[1].grip_active=0;f.hand.controllers=f.rig.controllers;},er::Status::invalid_rig);
    for(unsigned hand=0;hand<2;++hand){
        rejected([hand](auto& f){f.rig.world_grip[hand].position.x+=.01;},er::Status::inconsistent_world_pose);
        rejected([hand](auto& f){f.rig.world_grip[hand].orientation={0,0,0,1};},er::Status::inconsistent_world_pose);}
    rejected([](auto& f){f.rig.base_view=f.rig.source_view;},er::Status::inconsistent_world_pose);
    rejected([](auto& f){f.rig.units_per_metre=101;},er::Status::inconsistent_world_pose);
    {Fixture f;f.rig.world_pose_mask&=~rt::left_grip_pose;f.rig.world_grip[0]={};CHECK(f.status()==er::Status::matched);}
    {Fixture f;f.rig.world_grip[1].position.x+=5e-6;auto& q=f.rig.world_grip[1].orientation;
        q={-q.x,-q.y,-q.z,-q.w};CHECK(f.status()==er::Status::matched);}
    {Fixture f;f.rig.base_view[12]+=20;f.recompute();CHECK(f.status()==er::Status::matched);}
    std::printf("WeaponRigBindingTest: %u checks passed (synthetic CPU data only)\n",checks);return 0;
}
