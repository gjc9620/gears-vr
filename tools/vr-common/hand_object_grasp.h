#pragma once
#include "weapon_hand_participant.h"
#include "hand_wrist_calibration.h"
#include "hand_finger_pose.h"
#include <array>

// Cosmetic target only. It cannot grant contact, move a controller, advance a
// mechanism, supply ammunition or replace the unmodified tracked palm.
namespace gears_hand_object_grasp {
namespace vr=gears_vr;namespace hand=gears_weapon_hand;
enum class Kind:unsigned {none,magazine,loading,starter,support};
enum class Preset:unsigned {automatic,precision,cylinder,support_surface,authored};
inline bool valid(Preset preset) noexcept {return unsigned(preset)<=unsigned(Preset::authored);}
inline bool valid_for(Kind kind,Preset preset) noexcept {
    return valid(preset)&&(preset!=Preset::support_surface||kind==Kind::support);
}
inline gears_hand_fingers::Shape finger_shape(Kind kind,Preset preset) noexcept {
    if(kind==Kind::none||unsigned(kind)>unsigned(Kind::support)||!valid(preset))return gears_hand_fingers::Shape::free;
    if(preset==Preset::authored)return gears_hand_fingers::Shape::authored;
    if(preset==Preset::support_surface)return kind==Kind::support?
        gears_hand_fingers::Shape::authored:gears_hand_fingers::Shape::free;
    if(preset==Preset::precision)return gears_hand_fingers::Shape::loading;
    if(preset==Preset::cylinder)return gears_hand_fingers::Shape::starter;
    return static_cast<gears_hand_fingers::Shape>(kind);
}
// Only an acquisition preference at a REAL released part. No lease, pose,
// travel or action survives here. Reflected busy flags and neutral fitting
// publication can disappear independently of the user's stationary hand.
struct ReleasedSocket {
    hand::Frame source{};vr::Vector anchor{};std::uint64_t hold{};bool known{};
    void clear() noexcept {known=false;hold=0;}
    bool bind(hand::Frame const& f,std::uint64_t owner_hold) noexcept {
        clear();const auto& input=f.controllers.hands[0];
        if(!owner_hold||!f.left_tracked||!f.left_grip_known||input.squeeze_active!=1||
           hand::control_held(input,vr::control_squeeze)||
           !vr::controller_pose_usable(input.grip,input.grip_flags,input.grip_active))return false;
        source=f;anchor=input.grip.position;hold=owner_hold;known=true;return true;
    }
    bool retained(hand::Frame const& f,std::uint64_t owner_hold) noexcept {
        const auto& a=source;const auto& input=f.controllers.hands[0];
        const auto p=input.grip.position;
        const bool same=known&&hold==owner_hold&&f.left_tracked&&f.left_grip_known&&input.squeeze_active==1&&
            vr::controller_pose_usable(input.grip,input.grip_flags,input.grip_active)&&
            a.key.game_pid==f.key.game_pid&&a.key.game_start==f.key.game_start&&
            a.key.native_started_qpc==f.key.native_started_qpc&&a.key.registration_cookie==f.key.registration_cookie&&
            a.key.lifecycle==f.key.lifecycle&&a.key.world_generation==f.key.world_generation&&
            hand::same(a.world,f.world)&&hand::same(a.pawn,f.pawn)&&hand::same(a.weapon,f.weapon)&&
            a.stereo.session==f.stereo.session&&a.stereo.space_epoch==f.stereo.space_epoch&&
            a.controllers.input_epoch==f.controllers.input_epoch&&a.controllers.profile_generation==f.controllers.profile_generation&&
            std::hypot(std::hypot(p.x-anchor.x,p.y-anchor.y),p.z-anchor.z)<=.03;
        if(!same)clear();return same;
    }
};
// Neutral proximity has no action owner. Retain its cosmetic socket across
// small boundary jitter, but allow a deliberate approach to the other nearby
// part. An unconditional22cm incumbent kept handguard fitting when reaching C.
inline Kind proximity_socket(Kind incumbent,double support,double starter,
    double support_radius,double starter_radius,bool stationary_starter=false) noexcept {
    for(double v:{support,starter,support_radius,starter_radius})if(!std::isfinite(v)||v<0)return Kind::none;
    const auto nearest=support<=support_radius&&support<=starter?Kind::support:
        starter<=starter_radius?Kind::starter:Kind::none;
    const double old=incumbent==Kind::support?support:starter;
    if((incumbent==Kind::support||incumbent==Kind::starter)&&old<=.22){
        //70956: unfolding/ready animation changes the rendered distances even
        //when the physical hand has not moved. Preserve that C hover only;
        //the caller still validates current geometry, identity and actual
        //physical displacement. No action/grasp or wider contact is supplied.
        if(incumbent==Kind::starter&&stationary_starter)return incumbent;
        const double next=nearest==Kind::support?support:starter;
        if(nearest!=Kind::none&&nearest!=incumbent&&next+.03<old)return nearest;
        return incumbent;
    }
    return nearest;
}
// Exact optional authoring regions provide their own enter/retain predicates.
// Selection still uses distance and the same3cm switch hysteresis, no lease.
struct ProximityContact {double distance{};bool enter{},retain{};};
inline Kind proximity_socket(Kind incumbent,ProximityContact support,ProximityContact starter,
    bool stationary_starter=false) noexcept {
    for(const auto c:{support,starter})if(!std::isfinite(c.distance)||c.distance<0)return Kind::none;
    const auto nearest=support.enter&&(!starter.enter||support.distance<=starter.distance)?Kind::support:
        starter.enter?Kind::starter:Kind::none;
    const bool retained=incumbent==Kind::support?support.retain:incumbent==Kind::starter&&starter.retain;
    if(retained){
        if(incumbent==Kind::starter&&stationary_starter)return incumbent;
        const double old=incumbent==Kind::support?support.distance:starter.distance;
        const double next=nearest==Kind::support?support.distance:starter.distance;
        if(nearest!=Kind::none&&nearest!=incumbent&&next+.03<old)return nearest;
        return incumbent;
    }
    return nearest;
}

// Refresh an existing C hover anchor only at a currently observed contact
// centre, not at every point inside the broad entry/retention volumes.
inline bool recenter_starter_anchor(double closed_distance,double visible_distance) noexcept {
    return std::isfinite(closed_distance)&&closed_distance>=0&&
        std::isfinite(visible_distance)&&visible_distance>=0&&
        (closed_distance<=.03||visible_distance<=.03);
}
// Keep the selected cosmetic handguard through its support lease, and a
// selected C while the real Grip still awaits its first actual part grant.
// No pose/contact/action is saved; an occupied unrelated lease clears C.
// the next neutral frame still resolves current identity and real distances.
inline Kind proximity_control_handoff(Kind incumbent,hand::LeaseKind holder,bool reload_idle) noexcept {
    if(reload_idle&&incumbent==Kind::starter&&holder==hand::LeaseKind::none)return incumbent;
    return reload_idle&&incumbent==Kind::support&&
        (holder==hand::LeaseKind::none||holder==hand::LeaseKind::support)?Kind::support:Kind::none;
}
struct Target {
    Kind kind{};vr::Pose grip{};double weight{};bool passive_socket{};Preset preset{};
    // Internal cosmetic data, never part of the existing V2/V3 input wires.
    // Same sole C0 writer; no item/action or tracking authority.
    std::array<vr::Quaternion,15> authored_pose{};bool has_authored_pose{};
    bool passive_relative_socket{}; // Static main-Grip geometry; current C0 resolves it.
};
inline bool authored_valid(Target const& target) noexcept {
    if(target.preset!=Preset::support_surface&&target.preset!=Preset::authored)return true;
    if(!target.has_authored_pose)return false;
    for(auto q:target.authored_pose)if(!vr::stereo_pose_valid({{},q}))return false;
    return true;
}
// support carries a fixed foregrip-in-right-grip pose in metres; Native
// evaluates it from BOTH current physical hands in the one C0 solve.
// A proximity request contains a WORLD socket and grants skin fitting only.
// Ordinary support still contains its relative two-hand geometry and requires
// an actual support lease. Both binaries must agree on this V2 layout.
struct Request {unsigned size{sizeof(Request)},version{2};hand::Frame source{};Kind kind{};vr::Pose grip{};unsigned proximity{};};
inline constexpr char publish_export[]="GearsVrPublishLancerGraspV2";
#if defined(_WIN32)
using Publish=bool(__cdecl*)(Request const*) noexcept;
#endif
inline vr::Quaternion blend(vr::Quaternion a,vr::Quaternion b,double t) noexcept {
    if(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w<0)b={-b.x,-b.y,-b.z,-b.w};
    return vr::normalized({a.x*(1-t)+b.x*t,a.y*(1-t)+b.y*t,a.z*(1-t)+b.z*t,a.w*(1-t)+b.w*t});
}
inline bool apply(Target const& target,vr::Pose const& physical,
    gears_hand_wrist::Calibration const& calibration,double units,vr::Pose& visual) noexcept {
    if(target.kind==Kind::none||unsigned(target.kind)>unsigned(Kind::support)||!valid_for(target.kind,target.preset)||!authored_valid(target)||
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
inline bool keep_relative_fit(bool current,Kind part,Kind fitted,Preset preset,
    std::uint64_t token,std::uint64_t fitted_token,std::uint64_t profile,std::uint64_t fitted_profile) noexcept {
    return current&&part!=Kind::none&&part!=Kind::support&&part==fitted&&
        preset==Preset::authored&&token&&token==fitted_token&&profile&&profile==fitted_profile;
}
struct BlendState {
    Kind kind{};double weight{};std::int64_t last_ns{};
    bool position_fitted{};
    vr::Quaternion wrist_in_grip{};vr::Vector wrist_metres{};
    Preset preset{};
    std::array<vr::Quaternion,15> authored_pose{};
    bool update(Target const& target,vr::Pose const& physical,gears_hand_wrist::Calibration const& calibration,
        double units,std::int64_t now,vr::Pose& visual,bool current_same_grasp=false) noexcept {
        if(now<=0||!vr::stereo_pose_valid(physical)||!calibration.known||!valid_for(target.kind,target.preset)||!authored_valid(target))return false;
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
            if(kind!=target.kind||preset!=target.preset){weight=0;kind=target.kind;preset=target.preset;}
            if(preset==Preset::support_surface||preset==Preset::authored)authored_pose=target.authored_pose;
            wrist_in_grip=vr::multiply(vr::inverse(physical.orientation),desired.orientation);
            const auto d=vr::Vector{(desired.position.x-physical.position.x)/units,
                (desired.position.y-physical.position.y)/units,(desired.position.z-physical.position.z)/units};
            wrist_metres=vr::transform(vr::transpose(vr::rotation_matrix(physical.orientation)),d);
            weight=std::clamp(weight+dt/.12,0.,1.);
        }else if(!current_same_grasp||target.kind!=Kind::none)
            weight=std::clamp(weight-dt/.12,0.,1.);
        // A separately current SAME part/press/profile may retain only this
        // already fitted relative cosmetic offset on the NEW physical hand.
        // No cached world socket, tracking, contact or travel is supplied.
        // Release/unknown/currentness failure uses the ordinary decay above.
        if(weight<=0){kind=Kind::none;preset=Preset::automatic;authored_pose={};return false;}
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
