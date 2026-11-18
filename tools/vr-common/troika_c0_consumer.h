#pragma once
#include "troika_mounted_control.h"
#include "hand_authored_mount_profile.h"

// Read-only observation of the SAME completed Native C0 transaction. No
// ordinary hand Frame/lease, pose writer, input edge or engine-call grant.
namespace gears_troika_c0 {
namespace geo=gears_troika_geometry;namespace control=gears_troika_control;namespace hand=gears_weapon_hand;
constexpr std::uint32_t version=1,verified_flags=7;
constexpr std::uint64_t maximum_age_ns=50000000;
struct Request {
    std::uint32_t size{sizeof(Request)},abi_version{version};geo::Source source{};control::Decision control{};
    std::uint64_t maximum_pose_age_ns{maximum_age_ns};
};
struct Frame {
    std::uint32_t size{sizeof(Frame)},abi_version{version};geo::Source source{};control::Decision control{};
    hand::ObjectIdentity asset{};std::array<unsigned char,32> reference_signature{};
    std::uint64_t receipt_sequence{},tick{},compose{},kernel_serial{};
    std::int64_t completed_ns{},observed_qpc{};
    std::uint32_t flags{},solved_mask{},projected_mask{},reserved{};
    double units_per_metre{},model_scale{};
    std::array<gears_vr::Pose,2> grip_world{},wrist_world{},target_world{};
    std::array<std::array<gears_vr::Quaternion,15>,2> fingers{};
    std::array<gears_vr::Quaternion,4> helpers{};
};
inline bool request_valid(Request const& q,std::int64_t now) noexcept {
    geo::Request request{};request.source=q.source;request.query_id=q.control.query_id;
    return q.size==sizeof(q)&&q.abi_version==version&&q.maximum_pose_age_ns&&q.maximum_pose_age_ns<=maximum_age_ns&&
        control::valid(q.control,request,now)&&q.control.both_owned;
}
inline bool pose_matches(gears_vr::Pose const& a,gears_vr::Pose const& b,double position,double angle) noexcept {
    if(!gears_vr::stereo_pose_valid(a)||!gears_vr::stereo_pose_valid(b))return false;
    const double x=a.position.x-b.position.x,y=a.position.y-b.position.y,z=a.position.z-b.position.z;
    const auto norm=[](gears_vr::Quaternion q){return q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;};
    const double dot=std::abs(a.orientation.x*b.orientation.x+a.orientation.y*b.orientation.y+a.orientation.z*b.orientation.z+a.orientation.w*b.orientation.w)/std::sqrt(norm(a.orientation)*norm(b.orientation));
    const double rotation=2*std::acos(std::clamp(dot,0.,1.));
    return std::isfinite(rotation)&&x*x+y*y+z*z<=position*position&&rotation<=angle;
}
inline bool valid(Frame const& f,Request const& q,std::int64_t now) noexcept {
    if(!request_valid(q,now)||f.size!=sizeof(f)||f.abi_version!=version||f.flags!=verified_flags||f.solved_mask!=3||f.projected_mask||f.reserved||
       !hand::identity_valid(f.asset)||f.reference_signature!=gears_hand_authored_mount::marcus115_signature||
       !f.receipt_sequence||!f.tick||!f.compose||!f.kernel_serial||f.completed_ns<=0||f.completed_ns<q.source.frame.published_ns||
       f.completed_ns>now||std::uint64_t(now-f.completed_ns)>q.maximum_pose_age_ns||f.observed_qpc<=0||
       !gears_troika_source::frame_valid(f.source.frame,now,q.maximum_pose_age_ns,f.source.frame.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated)||
       std::memcmp(&f.source,&q.source,sizeof q.source)||std::memcmp(&f.control,&q.control,sizeof q.control)||
       !std::isfinite(f.units_per_metre)||f.units_per_metre<10||f.units_per_metre>500||f.model_scale!=1)return false;
    for(unsigned side=0;side<2;++side){
        if(!gears_vr::stereo_pose_valid(f.grip_world[side])||
           !pose_matches(f.wrist_world[side],f.target_world[side],.002*f.units_per_metre,.001))return false;
        for(auto const& finger:f.fingers[side])if(!gears_vr::stereo_pose_valid({{},finger}))return false;
    }
    for(auto const& helper:f.helpers)if(!gears_vr::stereo_pose_valid({{},helper}))return false;
    const auto ticks=std::uint64_t(f.observed_qpc),frequency=std::uint64_t(f.source.frame.qpc_frequency);
    if(!frequency)return false;const auto seconds=ticks/frequency,remainder=ticks%frequency;
    if(seconds>std::uint64_t(INT64_MAX)/1000000000||remainder>std::uint64_t(INT64_MAX)/1000000000)return false;
    const auto observed=seconds*1000000000+remainder*1000000000/frequency;
    return observed<=std::uint64_t(f.completed_ns)&&observed>=std::uint64_t(f.source.frame.published_ns)&&
        std::uint64_t(f.completed_ns)-observed<=q.maximum_pose_age_ns;
}
using Read=gears_rig_tracking::Reply(__cdecl*)(Request const*,Frame*) noexcept;
constexpr char read_export[]="GearsVrReadTroikaC0ConsumerV1";
static_assert(std::is_standard_layout_v<Frame>&&std::is_trivially_copyable_v<Frame>);
} // namespace gears_troika_c0
