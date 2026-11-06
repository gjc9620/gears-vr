#pragma once
#include "weapon_hand_participant.h"
#include "hand_wrist_calibration.h"

// Cosmetic target only. It cannot grant contact, move a controller, advance a
// mechanism, supply ammunition or replace the unmodified tracked palm.
namespace gears_hand_object_grasp {
namespace vr=gears_vr;namespace hand=gears_weapon_hand;
enum class Kind:unsigned {none,magazine,loading,starter,support};
struct Target {Kind kind{};vr::Pose grip{};double weight{};};
// support carries a fixed foregrip-in-right-grip pose in metres; Native
// evaluates it from BOTH current physical hands in the one C0 solve.
struct Request {unsigned size{sizeof(Request)},version{1};hand::Frame source{};Kind kind{};vr::Pose grip{};};
inline constexpr char publish_export[]="GearsVrPublishLancerGraspV1";
#if defined(_WIN32)
using Publish=bool(__cdecl*)(Request const*) noexcept;
#endif
inline vr::Quaternion blend(vr::Quaternion a,vr::Quaternion b,double t) noexcept {
    if(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w<0)b={-b.x,-b.y,-b.z,-b.w};
    return vr::normalized({a.x*(1-t)+b.x*t,a.y*(1-t)+b.y*t,a.z*(1-t)+b.z*t,a.w*(1-t)+b.w*t});
}
inline bool apply(Target const& target,vr::Pose const& physical,
    gears_hand_wrist::Calibration const& calibration,double units,vr::Pose& visual) noexcept {
    if(target.kind==Kind::none||unsigned(target.kind)>unsigned(Kind::support)||
       !vr::stereo_pose_valid(target.grip)||!vr::stereo_pose_valid(physical)||
       !std::isfinite(units)||units<10||units>500||!std::isfinite(target.weight)||target.weight<=0||target.weight>1)return false;
    const auto d=vr::Vector{target.grip.position.x-physical.position.x,target.grip.position.y-physical.position.y,target.grip.position.z-physical.position.z};
    // A visual attachment may correct a bounded offset after real contact;
    // it cannot pull a distant hand into contact or hide fixed-arm reach loss.
    const double distance=std::hypot(std::hypot(d.x,d.y),d.z);
    // Part poses share a forgiving bound after acquisition. The previous
    // 12cm skin cutoff discarded valid magazine/E/C attachments during
    // normal extraction or constrained travel.
    // This is bounded skin assistance only; the raw contact/stroke, lease,
    // tracking and fixed-length arm admission remain independent.
    const double maximum=target.kind==Kind::support?.25:.22;
    const double correction=target.kind==Kind::support?.22:.16;
    if(!std::isfinite(distance)||distance>maximum*units)return false;
    const double t=target.weight,position_t=t*(distance>correction*units?correction*units/distance:1.);
    const vr::Pose grip{{physical.position.x+d.x*position_t,physical.position.y+d.y*position_t,
        physical.position.z+d.z*position_t},blend(physical.orientation,target.grip.orientation,t)};
    return gears_hand_wrist::native_wrist_target(grip,calibration,units,visual)==gears_hand_wrist::Status::okay;
}
struct BlendState {
    Kind kind{};double weight{};std::int64_t last_ns{};
    bool position_fitted{};
    vr::Quaternion wrist_in_grip{};vr::Vector wrist_metres{};
    bool update(Target const& target,vr::Pose const& physical,gears_hand_wrist::Calibration const& calibration,
        double units,std::int64_t now,vr::Pose& visual) noexcept {
        if(now<=0||!vr::stereo_pose_valid(physical)||!calibration.known)return false;
        const double dt=last_ns>0&&now>=last_ns?std::clamp(double(now-last_ns)/1e9,0.,.05):0.;last_ns=now;
        vr::Pose desired{};position_fitted=apply(target,physical,calibration,units,desired);
        bool active=position_fitted;
        if(!active&&target.kind!=Kind::none&&unsigned(target.kind)<=unsigned(Kind::support)&&
           vr::stereo_pose_valid(target.grip)&&std::isfinite(target.weight)&&target.weight>0&&target.weight<=1&&
           std::isfinite(units)&&units>=10&&units<=500){
            // A current Native-admitted object grasp still owns its hand
            // pose when positional assistance reaches its limit. Keep the
            // authored orientation on THIS physical palm, not a distant old
            // object point. Finger/pose retention grants no contact or stroke.
            active=gears_hand_wrist::native_wrist_target({physical.position,target.grip.orientation},
                calibration,units,desired)==gears_hand_wrist::Status::okay;
        }
        if(active){
            if(kind!=target.kind){weight=0;kind=target.kind;}
            wrist_in_grip=vr::multiply(vr::inverse(physical.orientation),desired.orientation);
            const auto d=vr::Vector{(desired.position.x-physical.position.x)/units,
                (desired.position.y-physical.position.y)/units,(desired.position.z-physical.position.z)/units};
            wrist_metres=vr::transform(vr::transpose(vr::rotation_matrix(physical.orientation)),d);
            weight=std::clamp(weight+dt/.12,0.,1.);
        }else weight=std::clamp(weight-dt/.12,0.,1.);
        if(weight<=0){kind=Kind::none;return false;}
        // Release uses only a decaying relative skin offset on THIS fresh
        // physical hand. No old world contact/pose is replayed as tracking.
        const auto a=calibration.grip_to_wrist_metres;const double w=weight;
        const auto d=vr::transform(vr::rotation_matrix(physical.orientation),vr::Vector{
            (a.x*(1-w)+wrist_metres.x*w)*units,(a.y*(1-w)+wrist_metres.y*w)*units,(a.z*(1-w)+wrist_metres.z*w)*units});
        visual={{physical.position.x+d.x,physical.position.y+d.y,physical.position.z+d.z},
            vr::normalized(vr::multiply(physical.orientation,blend(calibration.grip_to_wrist_rotation,wrist_in_grip,w)))};
        return vr::stereo_pose_valid(visual);
    }
};
}
