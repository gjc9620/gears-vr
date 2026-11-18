#pragma once
#include "weapon_gnasher_mechanics.h"

namespace gears_weapon {
struct GnasherClosedLeverReference {
    std::uint64_t calibration_id{};Pose in_main{};
    double pivot_tolerance_metres{.002},rotation_tolerance_radians{.01};
    bool valid() const noexcept {
        return calibration_id&&gears_vr::stereo_pose_valid(in_main)&&
            std::isfinite(pivot_tolerance_metres)&&pivot_tolerance_metres>0&&pivot_tolerance_metres<=.01&&
            std::isfinite(rotation_tolerance_radians)&&rotation_tolerance_radians>0&&rotation_tolerance_radians<=.08;
    }
};
// Derived from CURRENT original Main/Lever bones, never from the posed hands
// or requested animation time. The adapter certifies component/asset/node and
// binds this numeric record to its current full Native Frame and reference.
struct GnasherCycleReadback {
    GnasherIdentity identity{};
    std::uint64_t request_token{},calibration_id{},input_sequence{},original_observation_sequence{};
    std::int64_t observed_ns{};
    Pose lever_in_main{};
    bool original_bones_verified{},original_ready{};
};
class GnasherCycleReadbackGate {
    GnasherCycleRequest request_{};
    GnasherClosedLeverReference closed_{};
    std::uint64_t last_observation_{};
    bool accepted_{};
    bool source_current(GnasherCycleReadback const& value,std::int64_t now_ns) const noexcept {
        return request_.token&&!accepted_&&value.identity==request_.identity&&
            value.request_token==request_.token&&value.calibration_id==closed_.calibration_id&&
            value.input_sequence>=request_.input_sequence&&value.original_observation_sequence&&
            value.original_observation_sequence>last_observation_&&
            value.observed_ns>=request_.physical_completion_ns&&now_ns>=value.observed_ns&&
            now_ns-value.observed_ns<=100000000&&value.original_bones_verified&&
            gears_vr::stereo_pose_valid(value.lever_in_main);
    }
    bool at_closed_pose(GnasherCycleReadback const& value) const noexcept {
        const auto displacement=difference(value.lever_in_main.position,closed_.in_main.position);
        auto const& a=value.lever_in_main.orientation;auto const& b=closed_.in_main.orientation;
        const double norm=std::sqrt((a.x*a.x+a.y*a.y+a.z*a.z+a.w*a.w)*(b.x*b.x+b.y*b.y+b.z*b.z+b.w*b.w));
        const double angle=2*std::acos((std::clamp)(std::abs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w)/norm,0.,1.));
        return length(displacement)<=closed_.pivot_tolerance_metres&&angle<=closed_.rotation_tolerance_radians;
    }
public:
    bool begin(GnasherCycleRequest const& request,GnasherClosedLeverReference const& closed,
               std::int64_t now_ns) noexcept {
        if(request_.token||!closed.valid()||!request.identity.valid()||!request.token||
           !request.input_sequence||request.physical_completion_ns<=0||
           now_ns<request.physical_completion_ns||now_ns-request.physical_completion_ns>100000000||
           request.action!=MechanismAction::lever_down_return)return false;
        request_=request;closed_=closed;return true;
    }
    // Current original geometry only. This may permit the owned non-refilling
    // Reloading exit, but never emits a cycle ACK or substitutes original ready.
    bool endpoint_current(GnasherCycleReadback const& value,std::int64_t now_ns) const noexcept {
        return source_current(value,now_ns)&&at_closed_pose(value);
    }
    bool observe(GnasherCycleReadback const& value,std::int64_t now_ns,
                 GnasherCycleReceipt& receipt) noexcept {
        receipt={};
        if(!source_current(value,now_ns))return false;
        last_observation_=value.original_observation_sequence;
        if(!value.original_ready||!at_closed_pose(value))return false;
        accepted_=true;receipt={request_.identity,request_.token,value.original_observation_sequence,true};return true;
    }
    bool pending() const noexcept{return request_.token&&!accepted_;}
};
}
