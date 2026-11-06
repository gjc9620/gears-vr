#pragma once

// Metres/radians in the authenticated interaction frame. These describe code
// tests, not UE collision components and not the lifetime of an acquired grasp.
namespace gears_weapon::lancer_contact {
inline constexpr double support_radius=.14;
// Acquisition only. A thicker/longer capsule covers a natural pinch around
// the visible magazine; an established grasp has its own retained lease.
inline constexpr double magazine_radius=.12;
inline constexpr double magazine_axis_length=.16;
inline constexpr double retained_magazine_radius=.12;
inline constexpr double starter_radius=.14;
inline constexpr double snap_near=-.04;
inline constexpr double snap_far=.20;
inline constexpr double snap_radius=.14;
inline constexpr double retain_near=-.04;
inline constexpr double retain_far=.25;
inline constexpr double retain_radius=.18;
inline constexpr double snap_angle=85.*3.14159265358979323846/180.;
inline constexpr double retain_angle=110.*3.14159265358979323846/180.;
}
