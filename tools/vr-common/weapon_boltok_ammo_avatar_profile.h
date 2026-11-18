// weapon_boltok_ammo_avatar_profile.h
#pragma once

#include "weapon_boltok_owner_profiles.h"

// Derived avatar thumb-only profile from the original whole-cylinder capture.
// This is not a new capture. Wrist calibration, all non-thumb quaternions,
// bone translations and tracking data remain the original owner profile.
// No Native writer registration or runtime finger fitting is implied.
namespace gears_boltok_ammo_avatar {
namespace owner=gears_boltok_owner_profiles;

inline constexpr std::uint64_t source_profile_id=owner::ammo.profile_id;
inline constexpr std::uint64_t profile_id=0x424f4c544f4b0032ULL;
inline constexpr char capture_id[]="1794500066511-3";
inline constexpr bool derived_avatar_thumb=true;
inline constexpr char fit_candidate_sha256[]="38ddabbf59efee717788b8f25fe7e6b912d8295916fb002f9c9218ca4ae1b0f6";

// Main supplies the independently reviewed avatar thumb result here.
inline constexpr gears_vr::Quaternion THUMB_Q12{0.91631499523047666,-0.39739074589925327,0.024297741530662692,-0.043093437388026526};
inline constexpr gears_vr::Quaternion THUMB_Q13{0.28019741057423098,0.00034216816606955774,0.00076647827765903565,-0.95994203290588953};
inline constexpr gears_vr::Quaternion THUMB_Q14{0.23892701521861986,0.26338288210356381,0.020289229837427879,-0.93441515717915535};

inline constexpr auto make_closed() noexcept {
    auto q=owner::ammo.fingers_native;
    q[12]=THUMB_Q12;
    q[13]=THUMB_Q13;
    q[14]=THUMB_Q14;
    return q;
}

inline constexpr auto closed=make_closed();
} // namespace gears_boltok_ammo_avatar
