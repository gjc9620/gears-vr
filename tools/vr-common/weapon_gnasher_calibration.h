#pragma once
#include "weapon_gnasher_animation_tracks.h"
namespace gears_weapon_gnasher_calibration {
// FNV of the independently decoded ORIGINAL SG_Chamber offsets/bytes/frame
// count/length, in the same order as the live reader. This identifies data,
// not the original engine's interpolation law.
inline constexpr std::uint64_t original_curve_content=0x29275018118bc74aull;
inline constexpr char original_sequence_sha256[]="c5811ff7a8852240b0c0d4ad79f703d7b5b384d199dd9ed32cc91739e39f5c73";
struct OriginalTiming {
    std::uint64_t curve_content{},evidence{};
    gears_weapon::gnasher_tracks::Timing law{gears_weapon::gnasher_tracks::Timing::uncertified};
    constexpr bool valid() const noexcept {
        return curve_content==original_curve_content&&evidence&&
            (law==gears_weapon::gnasher_tracks::Timing::keys||law==gears_weapon::gnasher_tracks::Timing::keys_minus_one);
    }
};
// Original game50672: complete35 strict Main/Lever/Main brackets, verified
// node positions, normal consumer drain and parent cleanup, paired review.
// analysis/gnasher-development-20261109/original-timing-50672-uninstalled.json
// Evidence SHA256:309f5d699f968aad414d98f6782da11d0bfd97b7b20d2f634dd6192f46dedfb3
// This certifies the stored original curve's timing only, not controller fit.
inline constexpr OriginalTiming current{original_curve_content,3503621743347927725ull,gears_weapon::gnasher_tracks::Timing::keys_minus_one};
static_assert(current.valid()||(current.evidence==0&&current.law==gears_weapon::gnasher_tracks::Timing::uncertified),
    "A pending timing record must remain uncertified");
}
