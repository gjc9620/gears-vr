#pragma once
#include "controller_input_frame.h"
#include <algorithm>
#include <cstdint>
#include <optional>

namespace gears_weapon {
using gears_vr::Vector;
using gears_vr::Pose;
enum class BodySlot : std::uint8_t { none, shoulder_ammo, weapon_2, weapon_1, pistol, grenade };

// WarPC.WeaponSlotFromDpadDirection and WarPawn.SetSlotAttachment in the
// verified retail scripts identify 0=holster, 1=belt, 2=left shoulder and
// 3=right shoulder. This is inventory identity, not a rendered hand pose.
inline std::optional<std::uint8_t> original_inventory_slot(BodySlot slot) noexcept {
    switch(slot){
        case BodySlot::pistol:return std::uint8_t{0};
        case BodySlot::grenade:return std::uint8_t{1};
        case BodySlot::weapon_2:return std::uint8_t{2};
        case BodySlot::weapon_1:return std::uint8_t{3};
        default:return std::nullopt;
    }
}

struct ArmspanCalibration {
    std::uint32_t version{1};
    double armspan_metres{};
    double hands_below_head_metres{};
    bool valid() const noexcept {
        return version==1&&std::isfinite(armspan_metres)&&armspan_metres>=.9&&armspan_metres<=2.4&&
            std::isfinite(hands_below_head_metres)&&hands_below_head_metres>=-.35&&hands_below_head_metres<=.1;
    }
};

// Authored interaction layout, NOT a measured human armspan. Used only for
// slot hit regions until an optional real calibration replaces it. Never
// persist this as calibration or use it to scale the avatar's bones.
inline ArmspanCalibration default_body_slot_layout() noexcept {return {1,1.7,-.15};}

// A two-second, uninterrupted, tracked T-pose grip hold. After completion the
// caller atomically saves the completed value.
class ArmspanCalibrator {
    std::uint64_t session_{},epoch_{},profile_{},input_epoch_{},last_sequence_{};
    std::int64_t started_ns_{},last_ns_{};
    gears_vr::StereoSource last_source_{};
    std::int64_t last_predicted_time_{},last_received_ns_{},last_sync_ns_{},last_stereo_received_ns_{};
    Pose last_head_{},last_grips_[2]{};
    std::uint64_t last_grip_flags_[2]{};
    float last_squeeze_[2]{};
    unsigned samples_{};
    double span_sum_{},hand_y_sum_{};
    Vector first_left_{},first_right_{};
    std::optional<ArmspanCalibration> result_;
    void clear_window() noexcept {
        session_=epoch_=profile_=input_epoch_=last_sequence_=0;
        started_ns_=last_ns_=0;samples_=0;span_sum_=hand_y_sum_=0;
    }
    bool same_measurement(gears_vr::ControllerInputFrame const& hands,
                          gears_vr::StereoSample const& stereo) const noexcept {
        if(hands.source!=last_source_||hands.predicted_time!=last_predicted_time_||
           hands.received_ns!=last_received_ns_||hands.sync_ns!=last_sync_ns_||
           stereo.received_ns!=last_stereo_received_ns_||
           !gears_vr::stereo_same_pose(stereo.head,last_head_))return false;
        for(unsigned i=0;i<2;++i){
            if(!gears_vr::stereo_same_pose(hands.hands[i].grip,last_grips_[i])||
               hands.hands[i].grip_flags!=last_grip_flags_[i]||
               hands.hands[i].squeeze!=last_squeeze_[i])return false;
        }
        return true;
    }
public:
    void reset() noexcept {clear_window();result_.reset();}
    std::optional<ArmspanCalibration> result() const noexcept {return result_;}
    double progress() const noexcept {
        if(!samples_)return 0;
        return (std::min)(1.,double(last_ns_-started_ns_)/2000000000.);
    }
    bool observe(gears_vr::ControllerInputFrame const& hands,
                 gears_vr::StereoSample const& stereo,std::int64_t now_ns) noexcept {
        if(result_)return true;
        if(!gears_vr::controller_frame_matches(hands,stereo,now_ns)||
           !gears_vr::controller_pose_usable(hands.hands[0].grip,hands.hands[0].grip_flags,hands.hands[0].grip_active)||
           !gears_vr::controller_pose_usable(hands.hands[1].grip,hands.hands[1].grip_flags,hands.hands[1].grip_active)||
           !hands.hands[0].squeeze_active||!hands.hands[1].squeeze_active||
           hands.hands[0].squeeze<.8f||hands.hands[1].squeeze<.8f||
           !gears_vr::stereo_pose_valid(stereo.head)){
            clear_window();return false;
        }
        Pose heading{};
        if(!gears_vr::stereo_heading_reference(stereo.head,heading)){
            clear_window();return false;
        }
        const auto inverse=gears_vr::transpose(gears_vr::rotation_matrix(heading.orientation));
        const auto relative=[&](Vector position){
            return gears_vr::transform(inverse,{position.x-stereo.head.position.x,
                position.y-stereo.head.position.y,position.z-stereo.head.position.z});
        };
        const Vector left=relative(hands.hands[0].grip.position),right=relative(hands.hands[1].grip.position);
        const double span=gears_vr::stereo_distance(left,right);
        const double hand_y=(left.y+right.y)*.5;
        if(left.x>-.35||right.x<.35||span<.9||span>2.4||
           std::abs(left.y-right.y)>.15||hand_y<-.35||hand_y>.1||
           std::abs(left.z)>.45||std::abs(right.z)>.45){clear_window();return false;}
        if(samples_){
            const bool same_key=hands.session==session_&&hands.space_epoch==epoch_&&
                hands.profile_generation==profile_&&hands.input_epoch==input_epoch_;
            if(same_key&&hands.sequence<=last_sequence_){
                // Rendering an immutable input ticket again contributes neither
                // a sample nor elapsed hold time. All live guards ran above.
                if(hands.sequence==last_sequence_&&same_measurement(hands,stereo))return false;
                clear_window();return false;
            }
            if(!same_key||now_ns<=last_ns_||now_ns-last_ns_>100000000||
               gears_vr::stereo_distance(left,first_left_)>.08||
               gears_vr::stereo_distance(right,first_right_)>.08)clear_window();
        }
        if(!samples_){session_=hands.session;epoch_=hands.space_epoch;
            profile_=hands.profile_generation;input_epoch_=hands.input_epoch;
            started_ns_=now_ns;first_left_=left;first_right_=right;}
        last_sequence_=hands.sequence;last_ns_=now_ns;++samples_;
        last_source_=hands.source;last_predicted_time_=hands.predicted_time;
        last_received_ns_=hands.received_ns;last_sync_ns_=hands.sync_ns;
        last_stereo_received_ns_=stereo.received_ns;last_head_=stereo.head;
        for(unsigned i=0;i<2;++i){
            last_grips_[i]=hands.hands[i].grip;last_grip_flags_[i]=hands.hands[i].grip_flags;
            last_squeeze_[i]=hands.hands[i].squeeze;
        }
        span_sum_+=span;hand_y_sum_+=hand_y;
        if(samples_<60||now_ns-started_ns_<2000000000)return false;
        ArmspanCalibration value{1,span_sum_/samples_,hand_y_sum_/samples_};
        if(!value.valid()){clear_window();return false;}
        result_=value;return true;
    }
};

// body is a stable torso frame in XR LOCAL coordinates supplied by the game
// rig. It must not be overwritten each frame with the latest head yaw.
inline bool shoulder_ammo_center(ArmspanCalibration const& calibration,Pose const& body,Vector& out) noexcept {
    if(!calibration.valid()||!gears_vr::stereo_pose_valid(body))return false;
    const Vector local{-calibration.armspan_metres*.135,calibration.hands_below_head_metres+.035,.05};
    const auto offset=gears_vr::transform(gears_vr::rotation_matrix(body.orientation),local);
    out={body.position.x+offset.x,body.position.y+offset.y,body.position.z+offset.z};return gears_vr::finite(out);
}
inline BodySlot body_slot(ArmspanCalibration const& calibration,Pose const& body,
                          Vector grip,unsigned hand) noexcept {
    if(!calibration.valid()||!gears_vr::stereo_pose_valid(body)||!gears_vr::finite(grip)||hand>1)
        return BodySlot::none;
    const double lateral=calibration.armspan_metres*.135;
    const double shoulder_y=calibration.hands_below_head_metres+.035;
    const double hip_y=shoulder_y-.55;
    // Owner-requested forgiving reach. Broaden height/front-back most, with
    // a narrower side radius and a small centre seam between left/right.
    // Shoulder/pocket vertical centres remain55cm apart: even these wider
    // regions have a gap, rather than silently selecting a neighbouring gun.
    constexpr double shoulder_reach=.24,pocket_reach=.22,centre_seam=.02;
    const double side_reach=(std::min)(.22,(std::max)(.16,lateral-.025));
    struct Candidate {BodySlot slot;Vector local;double radius;};
    const Candidate left[]={{BodySlot::shoulder_ammo,{-lateral,shoulder_y,.05},shoulder_reach}};
    const Candidate right[]={{BodySlot::weapon_2,{-lateral,shoulder_y,.05},shoulder_reach},
                             {BodySlot::weapon_1,{lateral,shoulder_y,.05},shoulder_reach},
                             {BodySlot::pistol,{-lateral,hip_y,.02},pocket_reach},
                             {BodySlot::grenade,{lateral,hip_y,.02},pocket_reach}};
    const Candidate* choices=hand?right:left;
    const unsigned count=hand?4:1;
    BodySlot best=BodySlot::none;double best_distance=1e9;
    const auto inverse=gears_vr::transpose(gears_vr::rotation_matrix(body.orientation));
    const auto local=gears_vr::transform(inverse,{grip.x-body.position.x,grip.y-body.position.y,grip.z-body.position.z});
    if(std::abs(local.x)<centre_seam)return BodySlot::none;
    for(unsigned i=0;i<count;++i){
        const auto& c=choices[i];if((local.x<0)!=(c.local.x<0))continue;
        const double x=(local.x-c.local.x)/(std::min)(side_reach,c.radius);
        const double y=(local.y-c.local.y)/c.radius,z=(local.z-c.local.z)/c.radius;
        const double distance=x*x+y*y+z*z;
        if(distance<=1&&distance<best_distance){best=c.slot;best_distance=distance;}
    }
    return best;
}
}
