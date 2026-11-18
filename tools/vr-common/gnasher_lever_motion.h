#pragma once
#include "gnasher_lever.h"
#include <array>

namespace gears_weapon {
// Original producer motion DATA, not current hand poses or action permission.
struct GnasherLeverMotion {
    std::array<LeverPoseSample,9> points{};
    unsigned count{};
};
enum class GnasherMotionResult {rejected,observed,completed};
inline GnasherMotionResult observe_gnasher_motion(GnasherLever& lever,
    LeverTravel const& travel,GnasherLeverMotion const& motion) noexcept {
    if(!travel.valid()||travel.maximum_sample_gap_ns!=100000000||
       motion.count<2||motion.count>motion.points.size()||
       !lever.motion_anchor(motion.points[0],.0002))return GnasherMotionResult::rejected;
    auto const& first=motion.points[0];auto const& tail=motion.points[motion.count-1];
    if(first.monotonic_ns<=0||tail.monotonic_ns<=first.monotonic_ns||
       tail.monotonic_ns-first.monotonic_ns<=travel.maximum_sample_gap_ns||
       tail.monotonic_ns-first.monotonic_ns>200000000)return GnasherMotionResult::rejected;
    // Validate the ENTIRE span before changing even the motion-only state.
    for(unsigned i=1;i<motion.count;++i){
        auto const& a=motion.points[i-1];auto const& b=motion.points[i];
        if(b.weapon!=first.weapon||b.world!=first.world||b.space_epoch!=first.space_epoch||
           !b.tracked||!b.left_front_held||!b.right_handle_held||
           a.sequence==UINT64_MAX||b.sequence!=a.sequence+1||b.monotonic_ns<=a.monotonic_ns||
           b.monotonic_ns-a.monotonic_ns>travel.maximum_sample_gap_ns||
           !std::isfinite(b.lever_angle_radians)||b.lever_angle_radians<0||
           b.lever_angle_radians>travel.maximum_angle_radians||
           std::abs(b.lever_angle_radians-a.lever_angle_radians)>
               travel.maximum_speed_radians_per_second*(double(b.monotonic_ns-a.monotonic_ns)/1e9))
            return GnasherMotionResult::rejected;
    }
    auto trial=lever;bool completed=false;
    for(unsigned i=1;i<motion.count;++i){
        if(completed){
            // A completed return may dwell at home. A second movement in the
            // same unconsumed span cannot be hidden behind that completion.
            if(motion.points[i].lever_angle_radians>travel.home_max_radians)
                return GnasherMotionResult::rejected;
        }else completed=trial.observe(motion.points[i],travel);
    }
    lever=trial;
    return completed?GnasherMotionResult::completed:GnasherMotionResult::observed;
}
}
