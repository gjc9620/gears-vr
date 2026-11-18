#pragma once
#include "weapon_hand_participant.h"
#include <type_traits>
namespace gears_weapon_longshot_scope_view {
inline constexpr char export_name[]="GearsCaptureLongshotScopeViewV2";
// Display selection only. The PG owner supplies the original getter's FOV,
// after near-eye dwell; Native separately checks current original objects,
// mesh matrix, eye relief, source freshness and the final stereo matrices.
struct Request {
    std::uint32_t size{sizeof(Request)},version{2};
    gears_weapon_hand::Frame source{};
    gears_weapon_hand::ObjectIdentity component{},asset{};
    // Exact source Frame's existing held-model plan, in engine world units.
    // No tracking time is attached or renewed by this display-only packet.
    gears_vr::Pose model_world{};
    double model_scale{};
    double original_targeting_fov_degrees{};
    std::uint32_t active{},eye{};
};
enum class Result:std::uint32_t {rejected,applied};
#if defined(_WIN32)
using Update=std::uint32_t(__stdcall*)(Request const*) noexcept;
#else
using Update=std::uint32_t(*)(Request const*) noexcept;
#endif
static_assert(std::is_trivially_copyable_v<Request>);
}
