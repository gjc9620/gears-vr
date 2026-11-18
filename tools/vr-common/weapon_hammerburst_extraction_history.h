#pragma once
#include "weapon_hammerburst_magazine.h"
#include "weapon_hammerburst_feed_history.h"

namespace gears_hammerburst_extraction_history {
using namespace gears_weapon;
namespace sample=gears_hammerburst_feed_history;
// Caller has separately authenticated the exact current grant, producer,
// fixed mapping and EVERY original sample. This pure helper grants no action.
namespace detail {
inline bool advance(HammerburstMagazineExtraction& stroke,FeedSample const& previous,
    FeedSample const& current,FeedSample const* points,unsigned capacity,unsigned count,FeedPortMount const& mount) noexcept {
    const auto* accepted=stroke.previous_sample();
    if(!accepted||!sample::same_sample(*accepted,previous)||!mount.valid()||!points||count<3||count>capacity||capacity>gears_hand_continuity::motion_history_capacity+1||
       !sample::same_sample(points[0],previous)||!sample::same_sample(points[count-1],current))return false;
    for(unsigned i=0;i<count;++i){const auto& s=points[i];
        if(!s.weapon||!s.item||!s.world||!s.space_epoch||!s.sequence||s.monotonic_ns<=0||!s.tracked||!s.item_held||
           s.weapon!=current.weapon||s.item!=current.item||s.world!=current.world||s.space_epoch!=current.space_epoch||
           !sample::close_pose(s.weapon_pose,current.weapon_pose)||!gears_vr::stereo_pose_valid(s.left_grip))return false;
        if(i){const auto& p=points[i-1];
            if(p.sequence==UINT64_MAX||s.sequence!=p.sequence+1||s.monotonic_ns<=p.monotonic_ns||
               s.monotonic_ns-p.monotonic_ns>mount.maximum_sample_gap_ns)return false;
        }
    }
    // Original geometry, speed, alignment, start and completion rules run on
    // each real point. Failure leaves the original witness untouched, so the
    // caller's original direct observation still rejects a missing interval.
    auto trial=stroke;
    for(unsigned i=1;i<count;++i)if(!trial.observe(points[i],mount))return false;
    stroke=trial;return true;
}
}
inline bool advance(HammerburstMagazineExtraction& stroke,FeedSample const& previous,
    FeedSample const& current,FeedSample const* points,unsigned count,FeedPortMount const& mount) noexcept {
    return detail::advance(stroke,previous,current,points,9,count,mount);
}
template<std::size_t N>
inline bool advance(HammerburstMagazineExtraction& stroke,FeedSample const& previous,
    FeedSample const& current,std::array<FeedSample,N> const& points,unsigned count,FeedPortMount const& mount) noexcept {
    static_assert(N<=gears_hand_continuity::motion_history_capacity+1,"Actual producer bound");
    return detail::advance(stroke,previous,current,points.data(),unsigned(N),count,mount);
}
}
