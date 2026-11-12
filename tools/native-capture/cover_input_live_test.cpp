// Composition test: real controller-v3 mapping in THIS test process, production
// CoverInputOwner/consumer, synthetic certified scene/camera/target endpoints.
// No game, input injection, engine calls or hooks are used.
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
#include "../vr-common/cover_input_owner.h"
bool simulator_operator=false;double stereo_units_per_metre=100;std::mutex stereo_mutex;
long long now(){LARGE_INTEGER value{};QueryPerformanceCounter(&value);return value.QuadPart;}
namespace cover_body_live {
enum class PoseProfile {top,side};PoseProfile pose_profile=PoseProfile::top;std::mutex mutex;
bool live=true,accept_target=true,publication_lock_free=true;unsigned published{};
struct LiveTarget {gears_anchor::Identity pawn,mesh,world;uint64_t world_generation{},frame{};int64_t timestamp_qpc{};unsigned thread{};float u{};};
LiveTarget latest{};bool live_source(){return live;}
bool publish_target(LiveTarget const& target){publication_lock_free=mutex.try_lock();if(!publication_lock_free)return false;mutex.unlock();if(!accept_target)return false;++published;latest=target;return true;}
}
namespace cover_neutral_eye_live {
struct EyeSample {
    gears_anchor::Identity pawn,mesh,link;uintptr_t current_world{};uint64_t world_generation{};
    int slot{};uint64_t calibrated_ms{},scene_invocation{};unsigned char special_move{},native_ct{};
    gears_vr::Vector neutral_eye_world;
};
struct Context {struct {gears_anchor::Identity pawn;} binding;gears_anchor::Identity mesh,world;uint64_t generation{};unsigned sm{};};
struct OwnContextFailure {};
EyeSample eye;Context current;bool available=true,proof_valid=true,throw_context=false;unsigned samples{};
bool sample(gears_anchor::Receipt const* scene,EyeSample& out){++samples;if(!scene||!available)return false;out=eye;out.scene_invocation=scene->invocation;return true;}
bool body_ready(){return proof_valid;}bool proof_current(){return proof_valid;}
bool context(gears_anchor::Receipt const&,Context& out){if(throw_context)throw OwnContextFailure{};out=current;return true;}
}
namespace cover_camera_live {
namespace neutral=cover_neutral_eye_live;
enum class Reason {applied,neutral_provider};
struct {Reason last=Reason::neutral_provider;uint64_t latest_invocation{};} diagnostics;
bool enabled=true;struct {gears_vr::Vector forward{1,0,0},right{0,1,0};} facing;
bool matches(neutral::Context const& c,neutral::EyeSample const& eye){return gears_anchor::equal_identity(c.binding.pawn,eye.pawn)&&
    gears_anchor::equal_identity(c.mesh,eye.mesh)&&c.world.address==eye.current_world&&c.generation==eye.world_generation;}
template<class Sample>bool prepare_with(gears_anchor::Receipt const* scene,gears_vr::StereoMatrix& view,Sample sample){
    neutral::EyeSample eye{};gears_vr::CoverViewResult result{};
    if(!enabled||!sample(scene,eye)||!gears_vr::prepare_cover_view({eye.neutral_eye_world,facing.forward,facing.right},{},100,result)){diagnostics.last=Reason::neutral_provider;return false;}
    view=result.base_view;diagnostics.last=Reason::applied;diagnostics.latest_invocation=scene->invocation;return true;
}
}
#include "cover_input_live.inl"
namespace live=cover_input_live;namespace neutral=cover_neutral_eye_live;namespace body=cover_body_live;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
bool close_enough(double a,double b){return std::abs(a-b)<1e-6;}
gears_vr::StereoResult stereo;gears_vr::ControllerInputFrame hands;gears_vr::ControllerEdgeCounter edges;
gears_anchor::Receipt scene;gears_vr::StereoMatrix view;gears_vr::ControllerInputBridge producer;
void next(double up,float squeeze=0){
    ++scene.invocation;auto& sample=stereo.sample;++sample.sequence;sample.received_ns=gears_vr::monotonic_ns();sample.predicted_time=sample.received_ns+20000000;
    sample.head.position.y=1.6+up;sample.eyes[0].pose.position={-.032,1.6+up,0};sample.eyes[1].pose.position={.032,1.6+up,0};
    hands.source=sample.source;hands.session=sample.session;hands.sequence=sample.sequence;hands.space_epoch=sample.space_epoch;
    hands.received_ns=hands.sync_ns=sample.received_ns;hands.predicted_time=sample.predicted_time;hands.hands[0].squeeze=squeeze;
    edges.update(hands);C(producer.publish(hands));
}
int main(){
    live::enabled=true;live::height_curve={{{0,0},{.5,.3},{1,.7}}};
    neutral::eye.pawn.address=100;neutral::eye.mesh.address=200;neutral::eye.link.address=300;
    neutral::eye.current_world=400;neutral::eye.world_generation=1;neutral::eye.calibrated_ms=5;neutral::eye.neutral_eye_world={1000,2000,-40};
    neutral::current.binding.pawn=neutral::eye.pawn;neutral::current.mesh=neutral::eye.mesh;
    neutral::current.world.address=400;neutral::current.generation=1;
    scene.thread=GetCurrentThreadId();stereo.status=gears_vr::StereoStatus::accepted;stereo.reference_generation=1;
    stereo.reference={{0,1.6,0},{}};auto& sample=stereo.sample;sample.session=41;sample.space_epoch=1;
    sample.head_flags=sample.view_flags=15;sample.focused=sample.visible=sample.should_render=1;sample.head.orientation={0,0,0,1};
    for(auto& eye:sample.eyes){eye.fov={-.7,.7,.7,-.7};eye.pose.orientation={0,0,0,1};}
    hands.focused=1;for(auto& hand:hands.hands){hand.grip_active=hand.squeeze_active=1;hand.grip_flags=15;hand.grip={{-.2,1.3,-.4},{0,0,0,1}};}
    C(producer.open(GetCurrentProcessId(),true));
    next(0);C(live::prepare(&scene,view,stereo));C(neutral::samples==1&&live::diagnostics.matched==1&&body::published==1&&body::latest.u==0);
    C(live::camera_frame_aligned());++cover_camera_live::diagnostics.latest_invocation;C(!live::camera_frame_aligned());--cover_camera_live::diagnostics.latest_invocation;
    C(body::latest.pawn.address==100&&body::latest.mesh.address==200&&body::latest.world.address==400&&body::latest.frame==scene.invocation&&body::latest.thread==GetCurrentThreadId());
    const auto old=body::published;const auto retry_rejected=live::diagnostics.rejected;C(live::prepare(&scene,view,stereo));C(body::published==old&&!live::interrupted&&live::diagnostics.rejected==retry_rejected); // Retry preserves continuity/TTL.
    next(.3);C(live::prepare(&scene,view,stereo));C(close_enough(body::latest.u,.5));C(live::diagnostics.latest.controller_status!=gears_vr::CoverControllerStatus::unavailable&&body::publication_lock_free);
    C(close_enough(gears_vr::stereo_camera_position(view).z,-40)); // Physical motion remains outside the base.
    C(live::diagnostics.hand_world_valid&&close_enough(live::diagnostics.hand_world.x,1040)&&close_enough(live::diagnostics.hand_world.y,1980)&&close_enough(live::diagnostics.hand_world.z,-70));
    next(.3);sample.head.orientation={std::sin(.5),0,0,std::cos(.5)};
    C(live::prepare(&scene,view,stereo));C(close_enough(body::latest.u,.5)); // Rotation alone does not change body.
    next(.3,.8f);C(live::prepare(&scene,view,stereo));C(!live::diagnostics.latest.interaction.owns_left_hand); // No certified edge, no grab.
    next(.7);C(live::prepare(&scene,view,stereo));C(close_enough(body::latest.u,1));
    next(-.3);C(live::prepare(&scene,view,stereo));C(body::latest.u==0);
    // A newer mismatched controller record cannot replace the frozen ticket.
    next(.2);++sample.sequence;C(live::prepare(&scene,view,stereo));C(live::diagnostics.missing==1&&close_enough(body::latest.u,1./3));
    const auto before_failure=body::published;
    next(.2);neutral::proof_valid=false;C(live::prepare(&scene,view,stereo));C(body::published==before_failure);
    neutral::proof_valid=true;next(.2);neutral::current.world.address=999;C(live::prepare(&scene,view,stereo));C(body::published==before_failure);
    neutral::current.world.address=400;next(.2);neutral::current.sm=12;C(live::prepare(&scene,view,stereo));C(body::published==before_failure);
    neutral::current.sm=0;next(.2);neutral::available=false;C(!live::prepare(&scene,view,stereo));C(body::published==before_failure&&!live::camera_frame_aligned());
    neutral::available=true;next(.2);C(live::prepare(&scene,view,stereo));C(body::published==before_failure+1);
    // Recenter advances reference generation without inventing a cover change.
    const auto cover_epoch=live::epochs.cover;next(.2);++stereo.reference_generation;
    C(live::prepare(&scene,view,stereo));C(live::epochs.cover==cover_epoch&&live::epochs.reference==2);
    next(.2);++neutral::eye.calibrated_ms;C(live::prepare(&scene,view,stereo));C(live::epochs.cover==cover_epoch+1);
    next(.2);body::accept_target=false;const auto rejected=live::diagnostics.target_rejected;
    C(live::prepare(&scene,view,stereo));C(live::diagnostics.target_rejected==rejected+1);
    body::accept_target=true;next(.2);neutral::throw_context=true;const auto before_throw=body::published;const auto own_rejected=live::diagnostics.rejected;
    C(live::prepare(&scene,view,stereo));C(body::published==before_throw&&live::interrupted&&live::diagnostics.rejected==own_rejected+1);
    const bool released=body::mutex.try_lock();C(released);if(released)body::mutex.unlock();neutral::throw_context=false;
    next(.2);C(live::prepare(&scene,view,stereo));C(body::published==before_throw+1&&body::publication_lock_free);
    const auto before_c0=body::published;live::full_body_cover_enabled=true;
    next(.2);C(live::prepare(&scene,view,stereo));C(body::published==before_c0);
    live::full_body_cover_enabled=false;next(.2);C(live::prepare(&scene,view,stereo));C(body::published==before_c0+1);
    live::close();producer.close();C(!live::bridge_open);
    std::printf("cover_input_live: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
