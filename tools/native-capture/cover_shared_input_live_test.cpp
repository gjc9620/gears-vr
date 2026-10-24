// Composition test: production shared consumer, CoverInputOwner, arbiter,
// logical decoder, Broker and exact controller-v3 mapping in THIS process.
// Native binding/camera/body/runtime endpoints are synthetic and read-only.
// No game, input injection, real module unpin, geometry claim or engine call.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define GEARS_COVER_BODY_PROTOTYPE 1
#include <windows.h>
#include <array>
#include <mutex>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include "anchor_observer.h"
#include "../vr-common/pose_bridge.h"
#include "../vr-common/controller_input_bridge.h"
#include "../vr-common/controller_continuity_bridge.h"
#include "../vr-common/cover_input_owner.h"
#include "../vr-common/weapon_hand_owner_adapter.h" // MUST be global, as in capture.
#include "weapon_hand_broker.h"
namespace {
namespace w=gears_weapon_hand;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
bool simulator_operator=false;double stereo_units_per_metre=100;std::mutex stereo_mutex;
long long now(){LARGE_INTEGER value{};QueryPerformanceCounter(&value);return value.QuadPart;}
bool close_enough(double a,double b){return std::abs(a-b)<1e-7;}
namespace cover_body_live {
enum class PoseProfile {top,side};PoseProfile pose_profile=PoseProfile::top;std::mutex mutex;
bool accept_target=true,publication_lock_free=true;unsigned published{};
struct LiveTarget {gears_anchor::Identity pawn{},mesh{},world{};uint64_t world_generation{},frame{};long long timestamp_qpc{};unsigned thread{};float u{};};
LiveTarget latest{};bool live_source(){return true;}
bool publish_target(LiveTarget const& target){publication_lock_free=mutex.try_lock();if(!publication_lock_free)return false;mutex.unlock();
    if(!accept_target)return false;++published;latest=target;return true;}
}
namespace cover_neutral_eye_live {
struct EyeSample {
    gears_anchor::Identity pawn{},mesh{},link{};uintptr_t current_world{};uint64_t world_generation{};
    int slot=-1;unsigned char native_ct{},native_ca{},special_move{},last_cover_action{};
    uint32_t transition_word{},transition_mask{};bool transition{};
    gears_vr::Vector pawn_location{},neutral_eye_world{},neutral_offset{};
    uint64_t head_name{};int head_index=-1;uint64_t calibrated_ms{},scene_invocation{};
    bool mutation_authorized=false,view_activation_authorized=false;
};
struct Context {struct {gears_anchor::Identity pawn;} binding;gears_anchor::Identity mesh,world;uint64_t generation{};unsigned sm{};};
EyeSample eye{};Context current{};bool available=true,proof_valid=true;unsigned samples{};
bool sample(gears_anchor::Receipt const* scene,EyeSample& out){++samples;if(!scene||!available)return false;out=eye;out.scene_invocation=scene->invocation;return true;}
bool body_ready(){return proof_valid;}bool proof_current(){return proof_valid;}
bool context(gears_anchor::Receipt const&,Context& out){out=current;return true;}
}
namespace cover_camera_live {
namespace neutral=cover_neutral_eye_live;
enum class Reason {applied,neutral_provider};
struct {Reason last=Reason::neutral_provider;uint64_t latest_invocation{};} diagnostics;
bool enabled=true;struct {gears_vr::Vector forward{1,0,0},right{0,1,0};} facing;
bool same_key(neutral::EyeSample const& a,neutral::EyeSample const& b){return
    gears_anchor::equal_identity(a.pawn,b.pawn)&&gears_anchor::equal_identity(a.mesh,b.mesh)&&gears_anchor::equal_identity(a.link,b.link)&&
    a.current_world==b.current_world&&a.world_generation==b.world_generation&&a.slot==b.slot&&a.calibrated_ms==b.calibrated_ms&&
    a.head_name==b.head_name&&a.head_index==b.head_index&&a.neutral_offset.x==b.neutral_offset.x&&
    a.neutral_offset.y==b.neutral_offset.y&&a.neutral_offset.z==b.neutral_offset.z;}
bool matches(neutral::Context const& c,neutral::EyeSample const& eye){return gears_anchor::equal_identity(c.binding.pawn,eye.pawn)&&
    gears_anchor::equal_identity(c.mesh,eye.mesh)&&c.world.address==eye.current_world&&c.generation==eye.world_generation;}
template<class Sample>bool prepare_with(gears_anchor::Receipt const* scene,gears_vr::StereoMatrix& view,Sample sample){
    neutral::EyeSample eye{};gears_vr::CoverViewResult result{};
    if(!enabled||!sample(scene,eye)||!gears_vr::prepare_cover_view({eye.neutral_eye_world,facing.forward,facing.right},{},100,result)){
        diagnostics.last=Reason::neutral_provider;return false;}
    view=result.base_view;diagnostics.last=Reason::applied;diagnostics.latest_invocation=scene->invocation;return true;
}
bool prepare(gears_anchor::Receipt const* scene,gears_vr::StereoMatrix& view){return prepare_with(scene,view,cover_neutral_eye_live::sample);}
}
#include "cover_input_live.inl"
namespace weapon_hand_scene_binding {
struct Binding {
    gears_anchor::Identity controller{},pawn{},mesh{},world{},weapon{};
    uint64_t world_generation{},scene_invocation{};unsigned char special_move{};
    bool weapon_known=false,context_valid=false,shared_actor_world=false;
    bool actor_or_mesh_fields_written=false,atomic_snapshot_verified=false;
};
enum class Reason {ready,body_unavailable};Reason last_reason=Reason::ready;
const char* reason_name(Reason reason){return reason==Reason::ready?"ready":"body_unavailable";}
Binding actor{};bool available=true;unsigned sample_calls{};void (*sample_hook)(unsigned){};
bool same_key(Binding const& a,Binding const& b){return a.context_valid&&b.context_valid&&a.weapon_known&&b.weapon_known&&
    a.world_generation&&a.world_generation==b.world_generation&&gears_anchor::equal_identity(a.controller,b.controller)&&
    gears_anchor::equal_identity(a.pawn,b.pawn)&&gears_anchor::equal_identity(a.mesh,b.mesh)&&
    gears_anchor::equal_identity(a.world,b.world)&&gears_anchor::equal_identity(a.weapon,b.weapon);}
bool sample(gears_anchor::Receipt const* scene,Binding& out){++sample_calls;if(sample_hook)sample_hook(sample_calls);if(!scene||!available){last_reason=Reason::body_unavailable;return false;}
    out=actor;out.scene_invocation=scene->invocation;last_reason=Reason::ready;return true;}
}
constexpr unsigned worker_thread=0xfffffff0u;thread_local bool as_worker=false;
struct PG {
    unsigned pins{},unpins{},prepares{},commits{},invalidations{},violations{},grants{};
    bool unpin_ok=true,candidates=true,revoke_prepare=false,revoke_commit=false;
    w::Reply prepare_reply=w::Reply::accepted,commit_reply=w::Reply::accepted;
    w::Frame last_frame{};w::Proposal last_proposal{};w::Decision last_decision{};w::Lease incumbent{};
    static bool pin(void*,w::Registration const&,w::ModulePin& value) noexcept;
    static bool unpin(void*,w::ModulePin) noexcept;
    static std::uint32_t thread(void*) noexcept {return as_worker?worker_thread:GetCurrentThreadId();}
    static std::int64_t clock(void*) noexcept {return gears_vr::monotonic_ns();}
    static std::uint32_t prepare(void*,w::Frame const*,w::Proposal*) noexcept;
    static std::uint32_t commit(void*,w::Frame const*,w::Proposal const*,w::Decision const*) noexcept;
    static void invalidate(void*,w::Invalidation const*) noexcept;
    static bool call_module(void*,gears_weapon_call::Registration const&,w::ModulePin) noexcept {return true;}
    static bool action_module(void*,gears_weapon_action::Registration const&,w::ModulePin) noexcept {return true;}
    static void call_begin(void*,gears_weapon_call::Invocation const*,gears_weapon_call::State*) noexcept {}
    static gears_weapon_call::End call_end(void*,gears_weapon_call::Invocation const*,gears_weapon_call::State*,uint64_t) noexcept {return gears_weapon_call::End::closed;}
    static gears_weapon_call::End call_unwind(void*,gears_weapon_call::Invocation const*,gears_weapon_call::State*,gears_weapon_call::UnwindReason) noexcept {return gears_weapon_call::End::closed;}
    static gears_weapon_action::BeginReply action_begin(void*,gears_weapon_action::Invocation const*,gears_weapon_action::State*) noexcept {return gears_weapon_action::BeginReply::unrelated;}
    static gears_weapon_action::End action_end(void*,gears_weapon_action::Invocation const*,gears_weapon_action::State*,uint64_t) noexcept {return gears_weapon_action::End::closed;}
    static gears_weapon_action::End action_unwind(void*,gears_weapon_action::Invocation const*,gears_weapon_action::State*,gears_weapon_action::UnwindReason) noexcept {return gears_weapon_action::End::closed;}
} pg;
w::BrokerBackend backend(){
    w::BrokerBackend b{nullptr,PG::pin,PG::unpin,PG::clock,PG::thread,w::weapon_broker_win32::prepare,w::weapon_broker_win32::commit,w::weapon_broker_win32::invalidate};
    b.call_module=PG::call_module;b.call_begin=w::weapon_broker_win32::call_begin;b.call_returned=w::weapon_broker_win32::call_returned;b.call_unwind=w::weapon_broker_win32::call_unwind;
    b.action_module=PG::action_module;b.action_begin=w::weapon_broker_win32::action_begin;b.action_returned=w::weapon_broker_win32::action_returned;b.action_unwind=w::weapon_broker_win32::action_unwind;return b;
}
namespace weapon_hand_runtime {
bool enabled=true;
const w::BrokerSession binding{GetCurrentProcessId(),worker_thread,400,500};
w::Broker broker(binding,backend());
w::Broker* current(){return &broker;}w::BrokerSession session(){return binding;}
void request_stop() noexcept {try{broker.invalidate(w::InvalidateReason::native_stopping);}catch(...){}}
}
bool PG::pin(void*,w::Registration const&,w::ModulePin& value) noexcept {++pg.pins;value={0x9000,0x9000};return true;}
bool PG::unpin(void*,w::ModulePin value) noexcept {++pg.unpins;if(!as_worker||value.handle!=0x9000||weapon_hand_runtime::broker.pending_callbacks()||weapon_hand_runtime::broker.pending_transactions())++pg.violations;return pg.unpin_ok;}
void revoke(w::Frame const& frame){
    gears_weapon_boundary::Request r{};r.source=gears_weapon_boundary::Source::process_event;r.flags=gears_weapon_boundary::related;
    r.hand_cookie=frame.key.registration_cookie;r.native_started_qpc=frame.key.native_started_qpc;
    r.context=reinterpret_cast<uintptr_t>(&pg);r.object=frame.pawn.address;r.function=0x7000;r.thread_id=GetCurrentThreadId();
    const auto before=weapon_hand_runtime::broker.action_generation();uint64_t generation{};
    C(weapon_hand_runtime::broker.revoke_action(r,generation)==w::Reply::accepted&&generation>before);
    C(!weapon_hand_runtime::broker.authorized());
}
std::uint32_t PG::prepare(void*,w::Frame const* frame,w::Proposal* proposal) noexcept {
    ++pg.prepares;pg.last_frame=*frame;*proposal={};proposal->key=frame->key;proposal->nonce=frame->key.frame*17+1;
    proposal->support=pg.candidates?1u:0u;if(pg.incumbent.lifecycle==frame->key.lifecycle)proposal->incumbents[0]=pg.incumbent;
    pg.last_proposal=*proposal;if(pg.revoke_prepare)revoke(*frame);return unsigned(pg.prepare_reply);
}
std::uint32_t PG::commit(void*,w::Frame const* frame,w::Proposal const* proposal,w::Decision const* decision) noexcept {
    ++pg.commits;pg.last_frame=*frame;pg.last_proposal=*proposal;pg.last_decision=*decision;
    if(pg.commit_reply==w::Reply::accepted){pg.incumbent=decision->lease;if(decision->new_grant)++pg.grants;}
    if(pg.revoke_commit)revoke(*frame);return unsigned(pg.commit_reply);
}
void PG::invalidate(void*,w::Invalidation const*) noexcept {++pg.invalidations;pg.incumbent={};}
#include "cover_shared_input_live.inl"
namespace shared=cover_shared_input_live;namespace input=cover_input_live;namespace native=weapon_hand_scene_binding;
namespace neutral=cover_neutral_eye_live;namespace body=cover_body_live;namespace runtime=weapon_hand_runtime;
gears_vr::StereoResult stereo{};gears_vr::ControllerInputFrame hands{};gears_vr::ControllerEdgeCounter edges;
gears_anchor::Receipt scene{};gears_vr::StereoMatrix view{};gears_vr::ControllerInputBridge producer;
uint64_t cookie{};
gears_anchor::Identity id(uintptr_t address,unsigned index,uintptr_t outer=0){gears_anchor::Identity value{};value.address=address;value.cls=0x9000;value.index=index;value.name_index=index+10;value.outer=outer;return value;}
void sync_neutral(){
    neutral::eye.pawn=native::actor.pawn;neutral::eye.mesh=native::actor.mesh;neutral::eye.current_world=native::actor.world.address;
    neutral::eye.world_generation=native::actor.world_generation;neutral::current.binding.pawn=neutral::eye.pawn;neutral::current.mesh=neutral::eye.mesh;
    neutral::current.world=native::actor.world;neutral::current.generation=native::actor.world_generation;
}
void setup(bool cover,bool simulated=false){
    input::enabled=true;input::height_curve={{{0,0},{.5,.3},{1,.7}}};simulator_operator=simulated;
    native::actor.controller=id(0x2000,2,0x1000);native::actor.pawn=id(0x3000,3,0x1000);native::actor.mesh=id(0x4000,4,0x3000);
    native::actor.world=id(0x1000,1);native::actor.weapon=id(0x5000,5,0x3000);native::actor.world_generation=1;
    native::actor.weapon_known=native::actor.context_valid=native::actor.shared_actor_world=true;
    neutral::available=cover;neutral::eye.link=id(0x6000,6);neutral::eye.slot=1;neutral::eye.calibrated_ms=10;
    neutral::eye.native_ct=2;neutral::eye.head_name=55;neutral::eye.head_index=55;neutral::eye.neutral_offset={0,0,76};
    neutral::eye.pawn_location={1000,2000,-116};neutral::eye.neutral_eye_world={1000,2000,-40};sync_neutral();
    scene.thread=GetCurrentThreadId();stereo.status=gears_vr::StereoStatus::accepted;stereo.reference_generation=1;stereo.reference={{0,1.6,0},{}};
    auto& sample=stereo.sample;sample.session=41;sample.space_epoch=1;sample.source=simulated?gears_vr::StereoSource::MetaOperatorSimulated:gears_vr::StereoSource::RuntimeTracked;
    sample.head_flags=simulated?3:15;sample.view_flags=15;sample.focused=sample.visible=sample.should_render=1;sample.head.orientation={0,0,0,1};
    for(auto& eye:sample.eyes){eye.fov={-.7,.7,.7,-.7};eye.pose.orientation={0,0,0,1};}
    hands.focused=1;for(auto& hand:hands.hands){hand.grip_active=hand.squeeze_active=1;hand.grip_flags=15;hand.grip={{-.2,1.3,-.4},{0,0,0,1}};}
    C(producer.open(GetCurrentProcessId(),true));
}
void attach(){w::Registration r{};r.context=&pg;r.prepare=PG::prepare;r.commit=PG::commit;r.invalidate=PG::invalidate;w::RegistrationResult result{};
    C(runtime::broker.register_participant(&r,&result)==w::Reply::accepted);cookie=result.cookie;C(cookie&&result.native_started_qpc==500);}
void next(double up=0,float squeeze=0){
    ++scene.invocation;auto& sample=stereo.sample;++sample.sequence;sample.received_ns=gears_vr::monotonic_ns();sample.predicted_time=sample.received_ns+20000000;
    sample.head.position={0,1.6+up,0};sample.eyes[0].pose.position={-.032,1.6+up,0};sample.eyes[1].pose.position={.032,1.6+up,0};
    hands.source=sample.source;hands.session=sample.session;hands.sequence=sample.sequence;hands.space_epoch=sample.space_epoch;
    hands.received_ns=hands.sync_ns=sample.received_ns;hands.predicted_time=sample.predicted_time;hands.hands[0].squeeze=squeeze;
    edges.update(hands);C(producer.publish(hands));
}
bool run(){std::lock_guard<std::mutex> lock(stereo_mutex);return shared::prepare(&scene,view,stereo);}
bool step(double up=0,float squeeze=0){next(up,squeeze);return run();}
void release(){for(unsigned n=0;n<3;++n)step();C(pg.last_frame.left_grip_known&&pg.last_frame.left_tracked&&!pg.last_frame.left_grip_down);}
void finish(){
    as_worker=true;runtime::broker.invalidate(w::InvalidateReason::native_stopping);C(!runtime::broker.pending_transactions());
    C(runtime::broker.collect_on_owner_worker()==w::Reply::quiescent);as_worker=false;input::close();producer.close();C(pg.violations==0);
}
void outside(bool simulated=false){
    setup(false,simulated);attach();release();C(pg.prepares==3&&pg.commits==3&&body::published==0);
    C(!step(0,.8f));C(pg.last_decision.new_grant==1&&pg.last_decision.lease.kind==w::LeaseKind::support);
    const auto token=pg.last_decision.press_token;C(token&&token==pg.last_frame.press_token&&pg.last_frame.left_grip_down&&pg.grants==1);
    C(!step(0,.8f));C(pg.last_decision.status==w::DecisionStatus::retained&&pg.last_decision.press_token==token);
    const auto prepares=pg.prepares,commits=pg.commits;const auto retries=shared::diagnostics.retries;C(!run());
    C(pg.prepares==prepares&&pg.commits==commits&&shared::diagnostics.retries==retries+1);
    step();C(pg.last_decision.status==w::DecisionStatus::released);step(0,.8f);
    C(pg.last_decision.new_grant==1&&pg.last_decision.press_token==token+1&&pg.grants==2&&body::published==0);
    C((pg.last_frame.flags&w::allow_sim)==(simulated?w::allow_sim:0u));finish();
}
void cover(bool simulated=false){
    setup(true,simulated);attach();release();C(step(.3));C(body::published>0&&close_enough(body::latest.u,.5));
    C(close_enough(gears_vr::stereo_camera_position(view).z,-40)&&body::publication_lock_free);
    pg.candidates=false;C(step(.3,.8f));C(!input::diagnostics.latest.interaction.owns_left_hand&&pg.last_decision.lease.kind!=w::LeaseKind::cover);
    C(close_enough(body::latest.u,.5));finish();
}
void rejection(bool prepare_failure){
    setup(true);attach();release();const auto before=body::published;
    if(prepare_failure)pg.prepare_reply=w::Reply::unavailable;else pg.commit_reply=w::Reply::busy;
    step(.3);const auto at_first=shared::diagnostics.lifecycle;step(.3);const auto at_second=shared::diagnostics.lifecycle;step(.3);
    // PG rejection gates the hand transaction, not independent physical HMD.
    // Production geometry is zero here, so no grip delta can be proposed.
    C(body::published==before+3&&shared::diagnostics.aborted>=3);
    C(!input::diagnostics.latest.accepted&&input::diagnostics.latest.view_valid&&input::diagnostics.latest.body_weight_valid);
    C(!input::diagnostics.latest.interaction.owns_left_hand&&input::diagnostics.latest.left_hand.lease.kind==gears_vr::LeftHandInteraction::none);
    C(close_enough(body::latest.u,.5)&&body::latest.frame==scene.invocation&&close_enough(gears_vr::stereo_camera_position(view).z,-40));
    if(prepare_failure)C(shared::diagnostics.lifecycle==at_second&&at_second<=at_first+1);
    else C(shared::diagnostics.lifecycle>=at_second&&at_second>=at_first);
    C(!runtime::broker.draining()&&runtime::broker.availability()==w::Availability::unavailable);
    pg.prepare_reply=pg.commit_reply=w::Reply::accepted;release();const auto recovered=pg.grants;
    C(step(.3,.8f));C(pg.last_decision.new_grant==1&&pg.grants==recovered+1&&runtime::broker.availability()==w::Availability::ready);
    C(body::published>before);finish();
}
void barrier(const char* kind){
    setup(false);attach();release();step(0,.8f);C(pg.last_decision.new_grant==1);const auto token=pg.last_decision.press_token;const auto grants=pg.grants;
    const auto lifecycle=shared::diagnostics.lifecycle;
    if(!std::strcmp(kind,"weapon")){native::actor.weapon=id(0x5080,15,0x3000);}
    else if(!std::strcmp(kind,"world")){native::actor.world=id(0x1080,11);++native::actor.world_generation;}
    else if(!std::strcmp(kind,"pawn")){native::actor.pawn=id(0x3080,13,0x1000);native::actor.mesh=id(0x4080,14,0x3080);}
    else if(!std::strcmp(kind,"reference")){++stereo.reference_generation;stereo.reference.position.x+=.1;}
    else if(!std::strcmp(kind,"recenter")){++stereo.reference_generation;stereo.recentered=true;}
    else if(!std::strcmp(kind,"revision")){runtime::broker.invalidate(w::InvalidateReason::context_lost);}
    else if(!std::strcmp(kind,"registration")){as_worker=true;C(runtime::broker.unregister(cookie)==w::Reply::quiescent);
        C(runtime::broker.collect_on_owner_worker()==w::Reply::quiescent);as_worker=false;attach();}
    sync_neutral();step(0,.8f);C(shared::diagnostics.lifecycle>lifecycle&&pg.grants==grants&&!pg.last_decision.new_grant);
    stereo.recentered=false;step(0,.8f);C(pg.grants==grants&&!pg.last_decision.new_grant);
    release();step(0,.8f);C(pg.last_decision.new_grant==1&&pg.last_decision.press_token>token&&pg.grants==grants+1);finish();
}
void retry(const char* kind){
    setup(true);attach();release();step(.3);const auto target=body::latest;const auto published=body::published;
    const auto prepares=pg.prepares,commits=pg.commits;const auto rejected=shared::diagnostics.rejected,retries=shared::diagnostics.retries;
    bool exact=!std::strcmp(kind,"exact");
    if(!std::strcmp(kind,"mutated"))stereo.sample.head.position.x+=.01;
    else if(!std::strcmp(kind,"stale"))stereo.sample.received_ns-=100000001;
    else if(!std::strcmp(kind,"neutral-loss"))neutral::available=false;
    else if(!std::strcmp(kind,"link"))neutral::eye.link=id(0x6080,16);
    else if(!std::strcmp(kind,"slot"))++neutral::eye.slot;
    else if(!std::strcmp(kind,"calibration"))++neutral::eye.calibrated_ms;
    else if(!std::strcmp(kind,"action"))++neutral::eye.native_ca;
    else if(!std::strcmp(kind,"transition"))neutral::eye.transition=true;
    else if(!std::strcmp(kind,"location"))neutral::eye.pawn_location.x+=1;
    else if(!std::strcmp(kind,"actor-sm"))native::actor.special_move=12;
    const bool result=run();C(pg.prepares==prepares&&pg.commits==commits&&body::published==published);
    C(body::latest.frame==target.frame&&body::latest.timestamp_qpc==target.timestamp_qpc&&body::latest.u==target.u);
    if(exact){C(result&&shared::diagnostics.retries==retries+1&&shared::diagnostics.rejected==rejected);}
    else {C(shared::diagnostics.rejected==rejected+1&&shared::diagnostics.retries==retries&&shared::interrupted);}
    if(!std::strcmp(kind,"neutral-loss"))C(!result&&!shared::cached_owned);
    finish();
}
void absence(bool blocked){
    setup(true);if(blocked){attach();C(runtime::broker.unregister(cookie)==w::Reply::quiescent);}
    for(unsigned n=0;n<3;++n)step(.3);
    if(blocked){C(runtime::broker.availability()==w::Availability::unavailable&&body::published>0&&pg.prepares==0&&shared::diagnostics.blocked==3);
        C(!input::diagnostics.latest.accepted&&input::diagnostics.latest.view_valid&&!input::diagnostics.latest.interaction.owns_left_hand);}
    else {C(runtime::broker.availability()==w::Availability::absent_quiescent&&body::published>0&&pg.prepares==0&&shared::diagnostics.absent==3);
        step(.3,.8f);C(!input::diagnostics.latest.interaction.owns_left_hand);}
    finish();
}
void pending_gap(bool stopping){
    setup(false);attach();release();auto frame=pg.last_frame;frame.key.frame+=100;frame.now_ns=gears_vr::monotonic_ns();w::Proposal p{};
    C(runtime::broker.prepare(frame,p)==w::Reply::accepted);
    {
        shared::Pending pending{};pending.broker=&runtime::broker;pending.key=frame.key;pending.nonce=p.nonce;pending.armed=true;
        as_worker=true;if(stopping)runtime::broker.invalidate(w::InvalidateReason::native_stopping);
        else C(runtime::broker.unregister(cookie)==w::Reply::draining);
        C(runtime::broker.pending_transactions()==1&&runtime::broker.pending_callbacks()==0&&runtime::broker.pin_retained());
        C(runtime::broker.collect_on_owner_worker()==w::Reply::draining&&pg.unpins==0);as_worker=false;
    }
    C(runtime::broker.pending_transactions()==0&&runtime::broker.quiescent()&&runtime::broker.pin_retained());finish();
    C(pg.unpins==1);
}
void source_disallowed(){setup(true,true);simulator_operator=false;attach();step(.3);C(pg.prepares==0&&pg.commits==0&&body::published==0&&input::diagnostics.missing==1);finish();}

// A separate common-boundary probe on the SAME participant/arbiter and real
// mapping. The top edge is synthetic test evidence; production shared geometry
// remains zero. This makes an actual pending delta/committed held offset that
// production's intentionally disabled grip geometry cannot yet generate.
gears_vr::CoverInputOwnerResult common_step(double up,float squeeze,bool commit,
        gears_vr::CoverWeaponState state=gears_vr::CoverWeaponState::absent_quiescent){
    next(up,squeeze);const auto stamp=gears_vr::monotonic_ns();gears_vr::ControllerInputFrame selected{};
    C(input::matching_input(stereo,stamp,selected));gears_vr::CoverTrackingBasis axes{};C(gears_vr::cover_tracking_basis(stereo.reference,axes));
    gears_vr::CoverInputOwnerFrame f{};f.lifecycle=1;f.frame=scene.invocation;
    f.cover.sequence=f.frame;f.cover.observed_ns=stamp;f.cover.epochs={1,1,1,1,1,1,1};
    f.cover.cover_valid=f.cover.interaction_enabled=true;f.cover.range={-.6,.5,0,.7};
    f.cover.edge={17,gears_vr::CoverEdgeKind::top,true,true};f.cover.right_axis_tracking=axes.right;f.cover.up_axis_tracking=axes.up;
    f.anchor={neutral::eye.neutral_eye_world,cover_camera_live::facing.forward,cover_camera_live::facing.right};
    f.stereo=stereo;f.controllers=&selected;f.height_curve=input::height_curve.data();f.height_key_count=unsigned(input::height_curve.size());
    f.units_per_metre=100;f.now_ns=stamp;f.owner_context_valid=true;
    const auto decoded=input::participant.begin(f,input::left_hand_owner);C(decoded.accepted);
    gears_vr::CoverWeaponContribution weapon{};weapon.state=state;
    const auto candidate=input::participant.update_with_decoded(decoded.ticket,weapon,input::left_hand_owner);
    if(commit)C(candidate.accepted);
    return input::participant.complete(decoded.ticket,commit,input::left_hand_owner);
}
void held_fallback(){
    setup(true);auto result=common_step(0,0,true);C(result.view_valid);
    result=common_step(0,.8f,true);C(result.interaction.owns_left_hand&&result.left_hand.new_grant);
    hands.hands[0].grip.position.y-=.2;
    result=common_step(0,.8f,false); // Pending .2 MUST NOT become committed.
    C(!result.accepted&&result.view_valid&&result.body_weight_valid&&!result.interaction.owns_left_hand);
    C(result.left_hand.lease.kind==gears_vr::LeftHandInteraction::none&&close_enough(result.interaction.offset.up,0));
    C(close_enough(result.requested_up_metres,0)&&close_enough(result.view.base_eye_world.z,-40));
    common_step(0,0,true);result=common_step(0,.8f,true);C(result.interaction.owns_left_hand);
    hands.hands[0].grip.position.y-=.2;result=common_step(0,.8f,true);C(close_enough(result.interaction.offset.up,.2));
    result=common_step(0,0,true);C(!result.interaction.owns_left_hand&&close_enough(result.interaction.offset.up,.2));
    hands.hands[0].grip.position.y-=.3;
    // Action revocation drops the hand lease, while the separately committed
    // virtual displacement remains available for fresh Scene certification.
    input::participant.invalidate(input::left_hand_owner);
    result=common_step(.3,0,false,gears_vr::CoverWeaponState::blocked);
    C(!result.accepted&&result.view_valid&&result.body_weight_valid&&!result.interaction.owns_left_hand);
    C(close_enough(result.interaction.offset.up,.2)&&close_enough(result.physical.up,.3));
    C(close_enough(result.view.base_eye_world.z,-20)&&close_enough(result.requested_up_metres,.5)&&close_enough(result.requested_up_weight,.75));
    C(input::publish(&scene,neutral::eye,float(result.requested_up_weight))&&close_enough(body::latest.u,.75));finish();
}
void action_cache(bool external,bool revise=false){
    setup(true);if(external)C(runtime::broker.configure_engine_restore(true));attach();release();step(.3,.8f);
    if(external){
        w::RegistrationResult result{};gears_weapon_call::Registration c{};c.hand_cookie=cookie;c.native_started_qpc=500;c.context=&pg;
        c.begin=PG::call_begin;c.returned=PG::call_end;c.unwind=PG::call_unwind;
        C(runtime::broker.attach_call_participant(&c,&result,true)==w::Reply::accepted);
        gears_weapon_action::Registration a{};a.hand_cookie=cookie;a.native_started_qpc=500;a.context=&pg;
        a.begin=PG::action_begin;a.returned=PG::action_end;a.unwind=PG::action_unwind;
        C(runtime::broker.attach_action_gate(&a,&result,true)==w::Reply::accepted);
    }
    C(runtime::broker.authorized());const auto grants=pg.grants;const auto prepares=pg.prepares,commits=pg.commits;
    const auto old_generation=runtime::broker.action_generation();revoke(pg.last_frame);
    if(revise)runtime::broker.invalidate(w::InvalidateReason::context_lost);
    gears_weapon_boundary::State state{};gears_weapon_boundary::Receipt receipt{};
    if(external){
        gears_weapon_boundary::Request r{};r.source=gears_weapon_boundary::Source::owned_bridge;r.flags=gears_weapon_boundary::related;
        r.hand_cookie=cookie;r.native_started_qpc=500;r.context=reinterpret_cast<uintptr_t>(&pg);r.object=0x3000;r.function=0x7000;r.thread_id=GetCurrentThreadId();
        C(runtime::broker.open_boundary(r,state,receipt,reinterpret_cast<uintptr_t>(&state))==w::Reply::accepted);
        C(runtime::broker.action_barrier());
    }
    C(run());C(pg.prepares==prepares&&pg.commits==commits&&pg.grants==grants);
    C(shared::previous.action_generation>old_generation&&shared::previous.action_blocked);
    C(!input::diagnostics.latest.accepted&&input::diagnostics.latest.view_valid&&input::diagnostics.latest.body_weight_valid);
    C(!input::diagnostics.latest.interaction.owns_left_hand&&close_enough(body::latest.u,.5));
    const auto published=body::published;const auto retries=shared::diagnostics.retries;
    C(run());C(body::published==published&&shared::diagnostics.retries==retries+1&&pg.prepares==prepares);
    if(external){
        C(step(.35,.8f));C(pg.prepares==prepares&&pg.commits==commits&&pg.grants==grants);
        C(!input::diagnostics.latest.accepted&&input::diagnostics.latest.view_valid&&close_enough(body::latest.u,.5625));
        gears_weapon_boundary::Finish f{};f.flags=gears_weapon_boundary::stage_closed|gears_weapon_boundary::handback_complete;
        C(runtime::broker.close_boundary(state,receipt,f,reinterpret_cast<uintptr_t>(&state))==gears_weapon_boundary::End::closed);
        C(!runtime::broker.action_barrier());const auto before=pg.prepares;C(run());C(pg.prepares==before&&!input::diagnostics.latest.accepted);
    }
    step(.3,.8f);C(pg.grants==grants&&!pg.last_decision.new_grant);release();step(.3,.8f);
    C(pg.grants==grants+1&&pg.last_decision.new_grant&&runtime::broker.authorized());finish();
}
void action_race(bool at_commit){
    setup(true);attach();release();if(at_commit)pg.revoke_commit=true;else pg.revoke_prepare=true;
    C(step(.3,.8f));C(!runtime::broker.authorized()&&!input::diagnostics.latest.accepted);
    C(input::diagnostics.latest.view_valid&&input::diagnostics.latest.body_weight_valid&&close_enough(body::latest.u,.5));
    C(!input::diagnostics.latest.interaction.owns_left_hand&&runtime::broker.pending_transactions()==0);
    pg.revoke_prepare=pg.revoke_commit=false;release();const auto before=pg.grants;step(.3,.8f);
    C(pg.grants==before+1&&runtime::broker.authorized());finish();
}
void tracking_bundle(){
    setup(false);attach();release();shared::TrackingBundle selected{};
    C(shared::tracking_for(&scene,stereo,selected)&&selected.matched&&selected.publisher_pid==GetCurrentProcessId());
    const auto ticket=selected.ticket;const auto stamp=selected.hands.received_ns;const auto prepares=pg.prepares,commits=pg.commits;
    input::controllers.close();input::bridge_open=false;C(!run());shared::TrackingBundle again{};
    C(shared::tracking_for(&scene,stereo,again)&&again.ticket==ticket&&again.hands.received_ns==stamp&&!input::bridge_open);
    C(pg.prepares==prepares&&pg.commits==commits);stereo.sample.head.position.x+=.1;C(!run());
    C(!shared::tracking_for(&scene,stereo,again)&&!shared::tracking_ticket_current(ticket));finish();
}
} // namespace
int main(int argc,char** argv){
    const char* mode=argc>1?argv[1]:"outside";
    if(!std::strcmp(mode,"tracking"))tracking_bundle();else if(!std::strcmp(mode,"outside"))outside();else if(!std::strcmp(mode,"cover"))cover();
    else if(!std::strcmp(mode,"prepare-rejected"))rejection(true);else if(!std::strcmp(mode,"commit-rejected"))rejection(false);
    else if(!std::strcmp(mode,"barrier"))barrier(argc>2?argv[2]:"weapon");
    else if(!std::strcmp(mode,"retry"))retry(argc>2?argv[2]:"exact");
    else if(!std::strcmp(mode,"absent"))absence(false);else if(!std::strcmp(mode,"blocked"))absence(true);
    else if(!std::strcmp(mode,"gap-stop"))pending_gap(true);else if(!std::strcmp(mode,"gap-unregister"))pending_gap(false);
    else if(!std::strcmp(mode,"source1-outside"))outside(true);else if(!std::strcmp(mode,"source1-cover"))cover(true);
    else if(!std::strcmp(mode,"source1-disallowed"))source_disallowed();else if(!std::strcmp(mode,"held-fallback"))held_fallback();
    else if(!std::strcmp(mode,"action-revoke"))action_cache(false);else if(!std::strcmp(mode,"action-external"))action_cache(true);
    else if(!std::strcmp(mode,"action-revoke-revision"))action_cache(false,true);
    else if(!std::strcmp(mode,"action-prepare-race"))action_race(false);else if(!std::strcmp(mode,"action-commit-race"))action_race(true);else C(false);
    std::printf("cover_shared_input_live %s: %u checks, %u failures\n",mode,checks,failures);return failures?1:0;
}
