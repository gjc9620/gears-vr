#define TROIKA_SCENE_BINDING_FIXTURE_ONLY
#include "troika_scene_binding_test.cpp"
#include "../vr-common/troika_mounted_source.h"
#include "../vr-common/troika_entry_basis.h"
#include "../vr-common/troika_mounted_geometry.h"
#include "../vr-common/troika_mounted_control.h"
#include "../vr-common/cover_input_owner.h"
#include "native_action_context.h"
gears_native_context::StereoMutex stereo_mutex;
#include "../vr-common/controller_continuity_frame.h"
#define GEARS_TROIKA_MOUNTED_SOURCE 1
bool simulator_operator=false;
int64_t qpc_frequency=1000000000,current_ns=2000000000;
namespace gears_weapon_hand::weapon_broker_win32 {
bool copy(void* out,void const* from,size_t bytes){if(!out||!from)return false;std::memcpy(out,from,bytes);return true;}
}
namespace weapon_hand_runtime {
struct Session {uint32_t game_pid=GetCurrentProcessId();uint64_t game_start=31,native_started_qpc=41;};
struct Broker {uint64_t value{51};uint64_t cookie(){return value;}} broker;
unsigned geometry_queries{};
struct GeometryQuery {enum class Code {unavailable,ready};Code code{Code::unavailable};};
GeometryQuery transact_troika_control(gears_troika_geometry::Source const& source,gears_troika_control::Decision&,gears_troika_geometry::Response& output){
    ++geometry_queries;output={};output.query_id=1;output.publication_sequence=source.frame.publication_sequence;
    output.source_ticket=source.frame.source_ticket;output.entry_binding=source.entry_binding;output.entry_token=source.entry_token;
    return {GeometryQuery::Code::ready};
}
bool open=true;
Broker* current(){return &broker;}Session session(){return {};}
namespace detail {unsigned calls{};struct Call {Call(){++calls;}~Call(){--calls;}};}
}
namespace rig_tracking_live {
bool enabled=true;uint64_t reference_revision=2;
gears_rig_tracking::Frame original{};
gears_rig_tracking::Reply read(gears_rig_tracking::ReadRequest const* request,gears_rig_tracking::Frame* output) noexcept {
    if(!request||!output||!gears_rig_tracking::request_valid(*request))return gears_rig_tracking::Reply::invalid;
    if(request->registration_cookie!=original.native.registration_cookie||request->native_started_qpc!=original.native.native_started_qpc)return gears_rig_tracking::Reply::wrong_cookie;
    if(request->minimum_publication_sequence>original.publication_sequence)return gears_rig_tracking::Reply::unavailable;
    *output=original;return gears_rig_tracking::Reply::accepted;
}
int64_t clock_ns(){return current_ns;}
bool reference_snapshot(gears_rig_tracking::NativeReference& out,uint64_t& revision){out={61,gears_rig_tracking::reference_stable,0};revision=reference_revision;return true;}
bool session_current(weapon_hand_runtime::Broker* b,weapon_hand_runtime::Session const&,uint64_t cookie){return weapon_hand_runtime::open&&b==&weapon_hand_runtime::broker&&b->cookie()==cookie&&cookie;}
gears_weapon_hand::ObjectIdentity object(gears_anchor::Identity const& id){return {id.address,id.outer,id.cls,id.index,id.name_index,id.number,1};}
}
namespace cover_input_live {
gears_vr::CoverInputOwner participant;gears_vr::LeftHandInteractionOwner left_hand_owner;gears_vr::CoverEpochs epochs{};
bool increment(uint64_t& value){if(value==UINT64_MAX)return false;++value;return true;}
gears_vr::ControllerInputFrame input{};unsigned selections{};bool matched=true;
struct Controllers {bool publisher_identity(uint64_t& pid,uint64_t& start){pid=71;start=81;return true;}} controllers;
bool matching_input(gears_vr::StereoResult const&,int64_t,gears_vr::ControllerInputFrame& out){++selections;out=input;return matched;}
}
namespace cover_shared_input_live {
bool companion=true;
struct Diagnostics {uint64_t lifecycle{1};} diagnostics;
template<class Selection>void select_continuity(Selection& selected,int64_t){
    if(companion&&selected.available){selected.continuity_known=true;selected.continuity.stream_epoch=91;
        selected.continuity.tracking_epoch[0]=101;selected.continuity.tracking_epoch[1]=111;}
}
}
namespace troika_control_live {void interrupt_locked() noexcept;}
namespace vr_gamepad_live {
bool look_ready=true;
bool arm_mounted_look() noexcept {return look_ready;}
bool mounted_look_ready() noexcept {return look_ready;}
}
#include "troika_scene_tracking.inl"
#if defined(GEARS_TROIKA_VIEW_LIVE_TEST)
double stereo_units_per_metre=100.;
#include "troika_view_live.inl"
int run_view_cases(gears_vr::StereoResult&);
#endif
#if defined(GEARS_TROIKA_CONTROL_LIVE_TEST)
#include "troika_control_live.inl"
void run_control_owner_cases(gears_vr::StereoResult&);
#else
namespace troika_control_live {void interrupt_locked() noexcept {}}
#endif
int main(){
    using namespace gears_vr;namespace tracking=troika_scene_tracking;namespace api=gears_troika_source;
    Fixture f;fixture=&f;CHECK(f.load(L"troika"));anchor_config=f.anchor;uint32_t camera_stamp{};
    CHECK(gears_anchor::guard(anchor_config,safe_copy,fixture_scene.anchor,camera_stamp));
    fixture_scene.invocation=17;fixture_scene.thread=GetCurrentThreadId();fixture_scene.scene_qpc=current_ns-2000000;fixture_scene.family_qpc=fixture_scene.scene_qpc+1;fixture_scene.renderer_qpc=fixture_scene.scene_qpc+2;
    StereoResult stereo{};stereo.status=StereoStatus::accepted;auto& s=stereo.sample;s.session=121;s.sequence=131;s.space_epoch=141;s.received_ns=current_ns-1000000;
    s.predicted_time=100000;s.head_flags=s.view_flags=15;s.focused=s.visible=s.should_render=1;s.head={};
    for(unsigned i=0;i<2;++i){s.eyes[i].pose={};s.eyes[i].pose.position.x=i?.032:-.032;s.eyes[i].fov={-.7,.7,.6,-.6};}
    stereo.reference_generation=151;
    auto& input=cover_input_live::input;input.source=s.source;input.session=s.session;input.sequence=s.sequence;input.space_epoch=s.space_epoch;
    input.profile_generation=161;input.input_epoch=171;input.predicted_time=s.predicted_time;input.focused=1;
    input.received_ns=current_ns-2000000;input.sync_ns=current_ns-3000000;
    for(auto& h:input.hands){h.grip=h.aim={};h.grip_active=h.aim_active=1;h.grip_flags=h.aim_flags=15;}
    // Mock the authentic ordinary rig cache from just before original entry.
    // The real Native implementation supplies its own rig::read guards.
    t::Binding actor{};CHECK(t::sample(&fixture_scene,actor));
    auto& entry=rig_tracking_live::original;entry.status=gears_rig_tracking::Status::ready;
    entry.flags=gears_rig_tracking::controllers_matched|gears_rig_tracking::producer_continuity_available;
    const auto session=weapon_hand_runtime::session();entry.native={session.game_pid,GetCurrentThreadId(),session.game_start,session.native_started_qpc,51};
    entry.world=rig_tracking_live::object(actor.world);entry.controller=rig_tracking_live::object(actor.controller);
    entry.pawn=rig_tracking_live::object(actor.driver);entry.mesh=rig_tracking_live::object(actor.driver_mesh);
    entry.publication_sequence=501;entry.scene_invocation=16;entry.world_generation=1;entry.binding_generation=1;
    entry.reference_generation=151;entry.published_ns=current_ns-1000000;entry.scene_qpc=current_ns-2000000;entry.qpc_frequency=qpc_frequency;
    entry.observed={1,1,1,{1,1}};entry.producer={91,{101,111}};entry.native_reference={61,gears_rig_tracking::reference_stable,0};
    entry.controller_publisher_pid=71;entry.controller_publisher_start=81;entry.units_per_metre=100;entry.view_policy=gears_rig_tracking::first_person;
    entry.base_view={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};entry.source_view=entry.base_view;entry.reference=stereo.reference;
    entry.stereo=s;entry.controllers=input;entry.world_pose_mask=7;
    CHECK(rig_tracking::pose(entry.base_view,entry.reference,s.head,100,entry.world_head));
    for(unsigned i=0;i<2;++i)CHECK(rig_tracking::pose(entry.base_view,entry.reference,input.hands[i].grip,100,entry.world_grip[i]));
    CHECK(gears_rig_tracking::frame_valid(entry,current_ns,100000000));
    gears_troika_basis::Request basis{};basis.owner={entry.native,entry.world,entry.controller,entry.pawn,
        rig_tracking_live::object(actor.turret),rig_tracking_live::object(actor.weapon),201,201};basis.entry_publication=entry.publication_sequence;
    CHECK(tracking::retain_basis(&basis)==gears_rig_tracking::Reply::accepted);
    CHECK(tracking::retain_basis(&basis)==gears_rig_tracking::Reply::accepted); // No new calibration/time on retry.
    auto missing=basis;++missing.entry_publication;CHECK(tracking::retain_basis(&missing)==gears_rig_tracking::Reply::unavailable);
    vr_gamepad_live::look_ready=false;CHECK(!tracking::prepare(&fixture_scene,stereo));
    CHECK(cover_input_live::selections==0&&!tracking::active); // Armed alone cannot authorize controller ownership.
    vr_gamepad_live::look_ready=true;
    CHECK(tracking::prepare(&fixture_scene,stereo));CHECK(cover_input_live::selections==1);
    const auto first=tracking::value;
    gears_weapon::TroikaMountedMapping native_mapping{};CHECK(tracking::mapped_for(first,native_mapping));CHECK(tracking::control_mapping_ready());
    CHECK(native_mapping.hands.input_sync_ns==input.sync_ns&&native_mapping.hands.tracked_mask==3);
    gears_troika_geometry::Source geometry_source{};geometry_source.frame=first;geometry_source.entry_binding=basis.owner.binding;geometry_source.entry_token=basis.owner.token;
    CHECK(tracking::geometry_current(geometry_source));
    auto wrong_geometry=geometry_source;++wrong_geometry.entry_token;CHECK(!tracking::geometry_current(wrong_geometry));
    wrong_geometry=geometry_source;++wrong_geometry.frame.driver.registry_index;CHECK(!tracking::geometry_current(wrong_geometry));
    CHECK(!tracking::geometry_ready&&weapon_hand_runtime::geometry_queries==0); // Raw selection never invokes PG under stereo lock.
    auto wrong_scene=fixture_scene;++wrong_scene.invocation;tracking::query_geometry(&wrong_scene);CHECK(weapon_hand_runtime::geometry_queries==0);
    tracking::query_geometry(&fixture_scene);CHECK(weapon_hand_runtime::geometry_queries==1&&tracking::geometry_ready);
    tracking::query_geometry(&fixture_scene);CHECK(weapon_hand_runtime::geometry_queries==1&&tracking::geometry_ready);
    CHECK(tracking::geometry_value.source_ticket==first.source_ticket&&tracking::geometry_value.publication_sequence==first.publication_sequence);
#if defined(GEARS_TROIKA_VIEW_LIVE_TEST)
    return run_view_cases(stereo);
#elif defined(GEARS_TROIKA_CONTROL_LIVE_TEST)
    run_control_owner_cases(stereo);return 0;
#else
    auto wrong_driver=first;++wrong_driver.driver.registry_index;CHECK(!tracking::mapped_for(wrong_driver,native_mapping));
    auto stale_cancel=basis;++stale_cancel.owner.token;CHECK(tracking::cancel_basis(&stale_cancel)==gears_rig_tracking::Reply::unavailable);
    api::Request request{};request.native=first.native;request.world=first.world;request.controller=first.controller;request.turret=first.turret;request.driver=first.driver;
    api::Frame read{};const auto before_reads=memory_reads;
    CHECK(tracking::read(&request,&read)==gears_rig_tracking::Reply::accepted);
    CHECK(memory_reads==before_reads&&weapon_hand_runtime::detail::calls==0); // Reader queries no actor/bridge.
    CHECK(read.controllers.sync_ns==input.sync_ns&&read.stereo.received_ns==s.received_ns);
    CHECK(read.control_policy==api::mounted_look_suppressed&&read.abi_version==2);
    vr_gamepad_live::look_ready=false;
    CHECK(tracking::read(&request,&read)==gears_rig_tracking::Reply::unavailable);
    CHECK(!tracking::mapped_for(first,native_mapping));
    vr_gamepad_live::look_ready=true;
    CHECK(tracking::prepare(&fixture_scene,stereo));CHECK(cover_input_live::selections==1);
    CHECK(tracking::value.published_ns==first.published_ns&&tracking::value.publication_sequence==first.publication_sequence);
    auto bad=request;++bad.turret.registry_index;
    CHECK(tracking::read(&bad,&read)==gears_rig_tracking::Reply::unavailable);CHECK(!read.publication_sequence);
    bad=request;++bad.native.registration_cookie;CHECK(tracking::read(&bad,&read)==gears_rig_tracking::Reply::wrong_cookie);
    current_ns+=100000001;CHECK(!tracking::prepare(&fixture_scene,stereo));CHECK(cover_input_live::selections==1);
    CHECK(tracking::read(&request,&read)==gears_rig_tracking::Reply::unavailable);current_ns-=100000001;
    ++fixture_scene.invocation;cover_input_live::matched=false;CHECK(!tracking::prepare(&fixture_scene,stereo));CHECK(cover_input_live::selections==2);
    cover_input_live::matched=true;CHECK(!tracking::prepare(&fixture_scene,stereo));CHECK(cover_input_live::selections==2);
    ++fixture_scene.invocation;cover_shared_input_live::companion=false;CHECK(!tracking::prepare(&fixture_scene,stereo));CHECK(cover_input_live::selections==3);
    ++fixture_scene.invocation;cover_shared_input_live::companion=true;input.hands[0].grip_active=0;input.hands[0].grip_flags=0;
    CHECK(tracking::prepare(&fixture_scene,stereo));CHECK(cover_input_live::selections==4);
    CHECK(tracking::read(&request,&read)==gears_rig_tracking::Reply::accepted);CHECK(!read.controllers.hands[0].grip_active);
    rig_tracking_live::reference_revision+=2;CHECK(tracking::read(&request,&read)==gears_rig_tracking::Reply::unavailable);
    CHECK(!tracking::prepare(&fixture_scene,stereo));CHECK(cover_input_live::selections==4);
    ++fixture_scene.invocation;CHECK(tracking::prepare(&fixture_scene,stereo));
    CHECK(tracking::mapped_for(tracking::value,native_mapping));
    CHECK(tracking::cancel_basis(&basis)==gears_rig_tracking::Reply::accepted);CHECK(!tracking::mapped_for(tracking::value,native_mapping));CHECK(!tracking::control_mapping_ready());
    CHECK(tracking::retain_basis(&basis)==gears_rig_tracking::Reply::unavailable); // Retired token cannot replay.
    auto replacement=basis;++replacement.owner.binding;++replacement.owner.token;
    CHECK(tracking::retain_basis(&replacement)==gears_rig_tracking::Reply::accepted);
    CHECK(tracking::cancel_basis(&basis)==gears_rig_tracking::Reply::unavailable);CHECK(tracking::basis_retained);
    tracking::clear();
    CHECK(tracking::read(&request,&read)==gears_rig_tracking::Reply::unavailable);
    CHECK(!tracking::prepare(&fixture_scene,stereo));CHECK(cover_input_live::selections==5);
    tracking::stop();CHECK(tracking::read(&request,&read)==gears_rig_tracking::Reply::stopping);
    CHECK(!tracking::geometry_ready);CHECK(!tracking::basis_retained&&!tracking::mapping_ready);CHECK(tracking::cancel_basis(&replacement)==gears_rig_tracking::Reply::stopping);
    CHECK(!tracking::prepare(&fixture_scene,stereo));CHECK(cover_input_live::selections==5);
    std::printf("Troika mounted selected-source lifecycle: %u checks passed; synthetic input/session only.\n",checks);
    return 0;
#endif
}
