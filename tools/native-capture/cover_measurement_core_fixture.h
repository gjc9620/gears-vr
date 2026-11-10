#pragma once
// Test-only fixture extracted from the existing Native Core validator test.
// It carries no ABI-size assertions or test entry.
#include <array>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <utility>
namespace nr=gears_native_rig_tracking;namespace rig=gears_rig_tracking;
namespace {
unsigned checks{},groups{},differential_frames{},differential_requests{},byte_exact_projections{};
#define CHECK(x) do {++checks;if(!(x)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);std::abort();}} while(false)
constexpr std::int64_t now=2000000000;
constexpr std::uint64_t age=100000000;
template<class T,class=void>struct has_cookie:std::false_type {};
template<class T>struct has_cookie<T,std::void_t<decltype(std::declval<T>().registration_cookie)>>:std::true_type {};
static_assert(!has_cookie<nr::NativeSessionKey>::value&&!std::is_same_v<nr::CoreSnapshot,rig::Frame>);
static_assert(!std::is_convertible_v<nr::CoreSnapshot,rig::Frame>);
static_assert(noexcept(nr::core_snapshot_valid(nr::CoreSnapshot{},now,age)));
static_assert(noexcept(nr::project_for_authenticated_registration(nr::CoreSnapshot{},rig::NativeKey{},now,age,std::declval<rig::Frame&>())));

gears_weapon_hand::ObjectIdentity object(std::uint64_t address,std::uint32_t index){
    return {address,0x8000+address,0x9000,index,index+10,index+2,1};
}
rig::Frame frame(unsigned source=0,bool hands=true){
    rig::Frame f{};f.status=rig::Status::ready;f.flags=rig::cover_key_known;
    f.native={21,31,41,51,61};f.publication_sequence=71;f.scene_invocation=81;f.world_generation=91;
    f.binding_generation=101;f.reference_generation=111;f.published_ns=now-1000000;
    f.scene_qpc=121;f.qpc_frequency=10000000;f.observed={131,141,151,{161,171}};
    f.native_reference={181,rig::reference_stable,0};f.view_policy=15;
    f.world=object(0x1000,1);f.controller=object(0x1100,2);f.pawn=object(0x1200,3);
    f.mesh=object(0x1300,4);f.weapon=object(0x1400,5);f.cover_link=object(0x1500,6);f.cover_slot=3;f.cover_type=2;
    f.units_per_metre=100;f.source_view={1,0,0,0,0,1,0,0,0,0,1,0,1,2,3,1};
    f.base_view=f.source_view;f.base_view[12]=4;f.base_view[13]=5;f.base_view[14]=6;
    f.reference.position={.1,.2,.3};f.reference.orientation={0,.6,0,.8};
    auto& s=f.stereo;s.source=static_cast<gears_vr::StereoSource>(source);s.session=191;s.sequence=201;s.space_epoch=211;
    s.received_ns=now-2000000;s.predicted_time=100000;s.head_flags=s.view_flags=15;s.focused=s.visible=s.should_render=1;
    s.head={{.2,.3,-.4},{0,0,.6,.8}};
    for(unsigned i=0;i<2;++i){s.eyes[i].pose={};s.eyes[i].pose.position.x=i?.032:-.032;s.eyes[i].fov={-.7,.7,.6,-.6};}
    f.world_pose_mask=rig::head_pose;
    CHECK(gears_vr::rig_tracking::pose(f.base_view,f.reference,s.head,f.units_per_metre,f.world_head));
    if(hands){
        f.flags|=rig::controllers_matched|rig::producer_continuity_available;f.producer={221,{231,241}};
        f.controller_publisher_pid=251;f.controller_publisher_start=261;
        auto& input=f.controllers;input.source=s.source;input.session=s.session;input.sequence=s.sequence;input.space_epoch=s.space_epoch;
        input.profile_generation=271;input.input_epoch=281;input.predicted_time=s.predicted_time;
        input.received_ns=now-3000000;input.sync_ns=now-4000000;input.focused=1;
        for(unsigned i=0;i<2;++i){auto& h=input.hands[i];h.grip_active=h.aim_active=1;h.grip_flags=h.aim_flags=15;
            h.grip={{.1+double(i)*.2,.4,-.5},{.6,0,0,.8}};h.aim={{-.1,.5,.2},{0,.6,0,.8}};
            h.trigger_active=h.squeeze_active=h.stick_active=1;h.trigger=.3f;h.squeeze=.4f;h.stick_x=-.2f;h.stick_y=.1f;
            h.last_squeeze_press_grip={{1,2,3},{0,0,.6,.8}};
            CHECK(gears_vr::rig_tracking::pose(f.base_view,f.reference,h.grip,f.units_per_metre,f.world_grip[i]));}
        f.world_pose_mask|=rig::left_grip_pose|rig::right_grip_pose;
    }
    CHECK(rig::frame_valid(f,now,age,source==1));return f;
}
nr::CoreSnapshot core(rig::Frame const& f){
    nr::CoreSnapshot c{};rig::detail::copy_payload(f,c);c.source_ticket=291;c.native_reference_revision=302;return c;
}
template<class T>std::array<unsigned char,sizeof(T)> bytes(T const& value){
    std::array<unsigned char,sizeof(T)> out{};std::memcpy(out.data(),&value,sizeof(value));return out;
}
bool compare_frame(rig::Frame const& f,std::int64_t stamp=now,std::uint64_t maximum=age,bool sim=false){
    const bool result=rig::frame_valid(f,stamp,maximum,sim);
#if defined(GEARS_NATIVE_RIG_FROZEN_V1) && GEARS_NATIVE_RIG_FROZEN_V1
    frozen_gears_rig_tracking::Frame old{};static_assert(sizeof(old)==sizeof(f));std::memcpy(&old,&f,sizeof(old));
    CHECK(result==frozen_gears_rig_tracking::frame_valid(old,stamp,maximum,sim));++differential_frames;
#endif
    return result;
}
bool compare_request(rig::ReadRequest const& request){
    const bool result=rig::request_valid(request);
#if defined(GEARS_NATIVE_RIG_FROZEN_V1) && GEARS_NATIVE_RIG_FROZEN_V1
    frozen_gears_rig_tracking::ReadRequest old{};static_assert(sizeof(old)==sizeof(request));std::memcpy(&old,&request,sizeof(old));
    CHECK(result==frozen_gears_rig_tracking::request_valid(old));++differential_requests;
#endif
    return result;
}
void projection_failure(nr::CoreSnapshot const& c,rig::NativeKey key,std::int64_t stamp=now,std::uint64_t maximum=age,bool sim=false){
    rig::Frame out=frame();const auto before=bytes(out);
    CHECK(!nr::project_for_authenticated_registration(c,key,stamp,maximum,out,sim));CHECK(bytes(out)==before);
}
template<class Change>void payload_rejection(Change change){
    auto f=frame();auto c=core(f);change(f);change(c);
    CHECK(!compare_frame(f));CHECK(!nr::core_snapshot_valid(c,now,age));projection_failure(c,f.native);
}

}
