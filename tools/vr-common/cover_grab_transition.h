#pragma once
#include "cover_grab_pose.h"
#include "hand_finger_pose.h"

// Cosmetic state only, scoped to this C0 consumer's current cover/reference.
// No world-space contact, input ticket, lease or body offset is retained.
namespace gears_cover_grab_transition {
namespace grab=gears_cover_grab_pose;namespace vr=gears_vr;
using Joints=std::array<vr::Quaternion,15>;
enum class Code {prepared,inactive,invalid};
struct State {
    grab::Key key{};
    vr::Vector relative_metres{};
    vr::Quaternion relative_rotation{};
    gears_hand_fingers::PoseBlend fingers{};
    std::int64_t last_ns{};
    double weight{};
    bool known{};
};
inline vr::Quaternion blend(vr::Quaternion a,vr::Quaternion b,double t) noexcept {
    if(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w<0)b={-b.x,-b.y,-b.z,-b.w};
    return vr::normalized({a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t,a.w+(b.w-a.w)*t});
}
// Physical fingers are THIS fresh unfitted C0's local rotations, not last
// frame's fitted output. All raw wrist/rotation motion remains immediate.
// Failure clears both cosmetic state and proposal. The caller may commit
// the next state only after its complete fixed-length C0 solve succeeds.
inline Code prepare(grab::Frame const& f,grab::Contact const& contact,grab::Profile const& profile,
    Joints const& physical_fingers,State& state,grab::Plan& out) noexcept {
    out={};
    const auto reject=[&](){state={};out={};return Code::invalid;};
    if(!grab::valid(f.expected)||!grab::same(f.expected.epochs,f.cover.epochs)||
       f.hand.session_epoch!=f.expected.epochs.session||f.hand.space_epoch!=f.expected.epochs.space||
       !grab::fresh(f.hand.observed_ns,f.now_ns)||!grab::fresh(f.cover.observed_ns,f.now_ns)||
       !f.cover.sequence||!f.hand.sequence||!f.hand.input_active||!f.hand.position_valid||!f.hand.position_tracked||
       !f.cover.cover_valid||!f.cover.interaction_enabled||!f.interaction.offset_certified||
       !vr::stereo_pose_valid(f.physical_grip_world))return reject();
    for(auto q:physical_fingers)if(!vr::stereo_pose_valid({{},q}))return reject();
    if(f.ownership.owner==vr::CoverLeftHandOwner::other){state={};return Code::inactive;}
    if(state.known&&!grab::same(state.key,f.expected))state={};
    if(f.now_ns<=0||(state.last_ns&&f.now_ns<state.last_ns))return reject();
    // No pose resumes after a pause/outage. A new valid contact starts from
    // the current physical wrist, never from a stale fitted world point.
    if(state.last_ns&&f.now_ns-state.last_ns>100000000)state={};
    grab::mount::Mount mount{};vr::Pose physical{};
    if(!grab::mount::marcus115(f.actual_geometry_signature,grab::mount::Side::left,
       f.model_scale,f.units_per_metre,mount)||
       gears_hand_wrist::native_wrist_target(f.physical_grip_world,mount.calibration,
       f.units_per_metre,physical)!=gears_hand_wrist::Status::okay)return reject();
    grab::Plan desired{};const auto result=grab::prepare(f,contact,profile,desired);
    const bool releasing=(f.mode==grab::Mode::release||f.mode==grab::Mode::none||!f.hand.lgrip_down)&&
        f.mode!=grab::Mode::approach;
    // Only an actual free/released context may blend out. Rejected current
    // geometry/ownership cannot keep driving the hand as an admitted contact.
    if(result!=grab::Code::prepared&&!releasing)return reject();
    if(releasing&&(f.hand.lgrip_down||f.ownership.owner!=vr::CoverLeftHandOwner::free||
       f.interaction.owns_left_hand||f.interaction.request_left_hand))return reject();
    State next=state;
    const double dt=next.last_ns?std::clamp(double(f.now_ns-next.last_ns)/1e9,0.,.05):0.;
    next.last_ns=f.now_ns;
    if(result==grab::Code::prepared){
        next.key=f.expected;next.known=true;
        const auto d=vr::Vector{(desired.wrist_world.position.x-physical.position.x)/f.units_per_metre,
            (desired.wrist_world.position.y-physical.position.y)/f.units_per_metre,
            (desired.wrist_world.position.z-physical.position.z)/f.units_per_metre};
        next.relative_metres=vr::transform(vr::transpose(vr::rotation_matrix(physical.orientation)),d);
        next.relative_rotation=vr::normalized(vr::multiply(vr::inverse(physical.orientation),desired.wrist_world.orientation));
        next.weight=std::clamp(next.weight+dt/.12,0.,1.);
    }else next.weight=std::clamp(next.weight-dt/.12,0.,1.);
    if(!next.known){
        // Keep time for the next acquisition tick, but discard all skin data.
        const auto stamp=next.last_ns;state={};state.last_ns=stamp;return Code::inactive;
    }
    const auto d=vr::transform(vr::rotation_matrix(physical.orientation),
        {next.relative_metres.x*f.units_per_metre*next.weight,
         next.relative_metres.y*f.units_per_metre*next.weight,next.relative_metres.z*f.units_per_metre*next.weight});
    out.key=f.expected;out.mode=result==grab::Code::prepared?f.mode:grab::Mode::release;
    out.wrist_world={{physical.position.x+d.x,physical.position.y+d.y,physical.position.z+d.z},
        vr::normalized(vr::multiply(physical.orientation,blend({},next.relative_rotation,next.weight)))};
    if(!vr::stereo_pose_valid(out.wrist_world)||!next.fingers.begin(f.now_ns))return reject();
    bool fingers_settled=true;
    for(unsigned n=0;n<15;++n){
        const auto q=result==grab::Code::prepared?desired.joints[n]:physical_fingers[n];
        if(!next.fingers.joint(n,physical_fingers[n],q,out.joints[n]))return reject();
        // Original float local quaternions are valid within the rig's norm
        // tolerance. Compare rotations, not their slightly non-unit lengths,
        // otherwise release can remain active after the fingers have settled.
        const auto a=vr::normalized(out.joints[n]),b=vr::normalized(physical_fingers[n]);
        const double dot=std::abs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w);
        fingers_settled=fingers_settled&&dot>=1.-1e-12;
    }
    if(releasing&&next.weight<=0&&fingers_settled){state={};out={};return Code::inactive;}
    out.active=true;state=next;return Code::prepared;
}
} // namespace gears_cover_grab_transition
