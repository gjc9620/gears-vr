#pragma once
#include "weapon_lancer_authored_volume.h"

// Replaced only by a checked offline project export. No live file I/O or
// mutable shared settings in the game thread. This root default remains off;
// an isolated candidate explicitly selects its SHA-checked owner export.
namespace gears_lancer_workbench_profile {
inline constexpr bool magazine_acquisition_enabled=false;
inline constexpr bool support_acquisition_enabled=false;
inline constexpr bool support_snap_limits_enabled=false;
inline constexpr bool shoulder_supply_enabled=false;
inline constexpr bool charging_acquisition_enabled=false;
inline constexpr bool charging_snap_limits_enabled=false;
inline constexpr bool starter_acquisition_enabled=false;
inline constexpr bool starter_snap_limits_enabled=false;
inline constexpr bool main_snap_time_override_enabled=false;
inline constexpr bool main_snap_enabled=false;
inline constexpr double main_snap_seconds=0.;
inline constexpr bool insertion_assist_override_enabled=false;
inline constexpr bool insertion_assist_enabled=true;
inline bool allow_insertion_assist() noexcept {
    return !insertion_assist_override_enabled||insertion_assist_enabled;
}
inline constexpr gears_vr::Vector shoulder_adjust{};
inline constexpr double shoulder_scale=1.;
inline constexpr gears_lancer_authored_volume::Volume magazine_acquisition{};
inline constexpr gears_lancer_authored_volume::Volume support_acquisition{};
inline constexpr gears_lancer_authored_volume::Volume charging_acquisition{};
inline constexpr gears_lancer_authored_volume::Volume starter_acquisition{};
inline constexpr char project_sha256[]="";
inline constexpr char imported_fields[]="none";
inline gears_lancer_authored_volume::Volume const* magazine() noexcept {
    return magazine_acquisition_enabled?&magazine_acquisition:nullptr;
}
inline gears_lancer_authored_volume::Volume const* support() noexcept {
    return support_acquisition_enabled?&support_acquisition:nullptr;
}
inline gears_lancer_authored_volume::Volume const* charging() noexcept {
    return charging_acquisition_enabled?&charging_acquisition:nullptr;
}
inline gears_lancer_authored_volume::Volume const* starter() noexcept {
    return starter_acquisition_enabled?&starter_acquisition:nullptr;
}
}
