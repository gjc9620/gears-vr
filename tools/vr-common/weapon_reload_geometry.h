#pragma once
#include "weapon_pose_solver.h"
#include "weapon_reload_model.h"
#include <algorithm>
#include <cmath>

namespace gears_weapon {
enum class GuidePart : std::uint8_t { none,feed_port,bolt,lever_handle,left_shoulder,magazine_closure };

inline GuidePart reload_guide(ReloadModel const& reload) noexcept {
    switch(reload.stage()){
        case ReloadStage::feed_port:
        case ReloadStage::holding_ammo:return GuidePart::feed_port;
        case ReloadStage::mechanism:
        case ReloadStage::complete:
        case ReloadStage::idle:
            switch(reload.pending_action()){
                case MechanismAction::bolt:return GuidePart::bolt;
                case MechanismAction::lever_down_return:return GuidePart::lever_handle;
                case MechanismAction::magazine_close:return GuidePart::magazine_closure;
                default:return GuidePart::none;
            }
    }
    return GuidePart::none;
}

// Verified per weapon and item. The axis points OUT of the gun, so insertion
// moves from positive depth at the port to zero. The source mesh's raw units
// must be converted before these metre-scale limits are used.
struct FeedPortMount {
    Pose port_in_weapon{};
    Pose item_in_left_grip{};
    Vector outward_axis_in_weapon{0,0,1};
    double approach_depth_metres{.045};
    double seated_depth_metres{.006};
    double lateral_tolerance_metres{.035};
    double maximum_orientation_error_radians{.4};
    double maximum_relative_speed_metres_per_second{3.};
    std::int64_t maximum_sample_gap_ns{100000000};
    bool valid() const noexcept {
        const auto axis=length(outward_axis_in_weapon);
        return gears_vr::stereo_pose_valid(port_in_weapon)&&
            gears_vr::stereo_pose_valid(item_in_left_grip)&&
            gears_vr::finite(outward_axis_in_weapon)&&std::isfinite(axis)&&axis>.99&&axis<1.01&&
            std::isfinite(approach_depth_metres)&&approach_depth_metres>.02&&approach_depth_metres<.2&&
            std::isfinite(seated_depth_metres)&&seated_depth_metres>=0&&
            seated_depth_metres<approach_depth_metres-.015&&
            std::isfinite(lateral_tolerance_metres)&&lateral_tolerance_metres>.0005&&
            lateral_tolerance_metres<.1&&
            std::isfinite(maximum_orientation_error_radians)&&
            maximum_orientation_error_radians>.05&&maximum_orientation_error_radians<1.0&&
            std::isfinite(maximum_relative_speed_metres_per_second)&&
            maximum_relative_speed_metres_per_second>.1&&
            maximum_relative_speed_metres_per_second<10&&
            maximum_sample_gap_ns>0&&maximum_sample_gap_ns<=250000000;
    }
};
struct FeedSample {
    std::uint64_t weapon{},item{},world{},space_epoch{},sequence{};
    std::int64_t monotonic_ns{};
    Pose weapon_pose{},left_grip{};
    bool tracked{},item_held{};
};
// Geometry-only detector input for an already-authenticated producer path.
// It contains no Frame, Pose, permission, or renewed source timestamp.
struct FeedGeometrySample {
    std::uint64_t weapon{},item{},world{},space_epoch{},sequence{};
    std::int64_t monotonic_ns{};
    bool tracked{},item_held{};
    double depth_metres{},lateral_metres{},orientation_dot{};
};
enum class FeedResult : std::uint8_t { rejected,approaching,inserted };

inline bool feed_sample_from_hands(gears_vr::ControllerInputFrame const& hands,
                                   gears_vr::StereoSample const& stereo,
                                   WeaponPoseSolution const& gun,std::uint64_t weapon,
                                   std::uint64_t item,std::uint64_t world,
                                   std::int64_t now_ns,bool item_held,
                                   FeedSample& out,bool allow_sim=false) noexcept {
    if(!weapon||!item||!world||!item_held||
       !gears_vr::controller_frame_matches(hands,stereo,now_ns,allow_sim)||
       !gears_vr::controller_pose_usable(hands.hands[0].grip,
           hands.hands[0].grip_flags,hands.hands[0].grip_active)||
       gun.session!=hands.session||gun.sequence!=hands.sequence||
       gun.space_epoch!=hands.space_epoch||gun.predicted_time!=hands.predicted_time||
       !gears_vr::stereo_pose_valid(gun.gun))return false;
    out={weapon,item,world,hands.space_epoch,hands.sequence,now_ns,
         gun.gun,hands.hands[0].grip,true,true};
    return true;
}

class FeedInsertionStroke {
    std::uint64_t weapon_{},item_{},world_{},space_epoch_{},last_sequence_{};
    std::int64_t last_ns_{};
    double last_depth_{};
    bool approaching_{};
    FeedResult observe_values(std::uint64_t weapon,std::uint64_t item,std::uint64_t world,
        std::uint64_t space_epoch,std::uint64_t sequence,std::int64_t monotonic_ns,
        bool tracked,bool item_held,double depth,double lateral,double orientation_dot,
        FeedPortMount const& mount) noexcept {
        if(!weapon_||!mount.valid()||weapon!=weapon_||item!=item_||world!=world_||
           space_epoch!=space_epoch_||!tracked||!item_held||monotonic_ns<=0){
            reset();return FeedResult::rejected;
        }
        if(last_sequence_&&(sequence<=last_sequence_||monotonic_ns<=last_ns_||
            monotonic_ns-last_ns_>mount.maximum_sample_gap_ns)){
            reset();return FeedResult::rejected;
        }
        const bool aligned=std::isfinite(orientation_dot)&&lateral<=mount.lateral_tolerance_metres&&
            std::abs(orientation_dot)>=std::cos(mount.maximum_orientation_error_radians*.5);
        if(!std::isfinite(depth)||!std::isfinite(lateral)||!aligned){
            approaching_=false;last_sequence_=sequence;last_ns_=monotonic_ns;
            return FeedResult::rejected;
        }
        if(last_sequence_&&std::abs(depth-last_depth_)>
            mount.maximum_relative_speed_metres_per_second*
                (double(monotonic_ns-last_ns_)/1e9)){
            approaching_=false;last_sequence_=sequence;last_ns_=monotonic_ns;
            last_depth_=depth;return FeedResult::rejected;
        }
        last_sequence_=sequence;last_ns_=monotonic_ns;last_depth_=depth;
        if(!approaching_){
            if(depth>=mount.approach_depth_metres&&depth<.25){
                approaching_=true;return FeedResult::approaching;
            }
            return FeedResult::rejected;
        }
        if(depth>mount.approach_depth_metres+.03){approaching_=false;return FeedResult::rejected;}
        if(depth<=mount.seated_depth_metres){reset();return FeedResult::inserted;}
        return FeedResult::approaching;
    }
public:
    void reset() noexcept {*this=FeedInsertionStroke{};}
    bool arm(std::uint64_t weapon,std::uint64_t item,std::uint64_t world,
             std::uint64_t space_epoch) noexcept {
        reset();if(!weapon||!item||!world||!space_epoch)return false;
        weapon_=weapon;item_=item;world_=world;space_epoch_=space_epoch;return true;
    }
    FeedResult observe(FeedSample const& sample,FeedPortMount const& mount) noexcept {
        if(!weapon_||!mount.valid()||sample.weapon!=weapon_||sample.item!=item_||
           sample.world!=world_||sample.space_epoch!=space_epoch_||
           !sample.tracked||!sample.item_held||sample.monotonic_ns<=0||
           !gears_vr::stereo_pose_valid(sample.weapon_pose)||
           !gears_vr::stereo_pose_valid(sample.left_grip)){
            reset();return FeedResult::rejected;
        }
        const auto item_orientation=gears_vr::normalized(gears_vr::multiply(
            sample.left_grip.orientation,mount.item_in_left_grip.orientation));
        const auto item_position=sum(sample.left_grip.position,
            rotate(sample.left_grip.orientation,mount.item_in_left_grip.position));
        const auto port_orientation=gears_vr::normalized(gears_vr::multiply(
            sample.weapon_pose.orientation,mount.port_in_weapon.orientation));
        const auto port_position=sum(sample.weapon_pose.position,
            rotate(sample.weapon_pose.orientation,mount.port_in_weapon.position));
        const auto axis=rotate(sample.weapon_pose.orientation,mount.outward_axis_in_weapon);
        const auto offset=difference(item_position,port_position);
        const auto depth=dot(offset,axis);
        const auto lateral=length(difference(offset,scaled(axis,depth)));
        const auto orientation_dot=std::abs(item_orientation.x*port_orientation.x+
            item_orientation.y*port_orientation.y+item_orientation.z*port_orientation.z+
            item_orientation.w*port_orientation.w);
        return observe_values(sample.weapon,sample.item,sample.world,sample.space_epoch,
            sample.sequence,sample.monotonic_ns,sample.tracked,sample.item_held,
            depth,lateral,orientation_dot,mount);
    }
    FeedResult observe_geometry(FeedGeometrySample const& sample,FeedPortMount const& mount) noexcept {
        return observe_values(sample.weapon,sample.item,sample.world,sample.space_epoch,
            sample.sequence,sample.monotonic_ns,sample.tracked,sample.item_held,
            sample.depth_metres,sample.lateral_metres,sample.orientation_dot,mount);
    }
};
}
