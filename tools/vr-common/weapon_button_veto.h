#pragma once
#include "weapon_hand_participant.h"
#include <type_traits>
// Cosmetic/action-routing veto only. The caller still owns its weapon-specific
// fresh R3 semantics. This never publishes input, tracking or a hand grant.
namespace gears_weapon_button_veto {
inline constexpr char export_name[]="GearsCaptureWeaponButtonVetoV1";
struct Request {
 std::uint32_t size{sizeof(Request)},version{1};
 gears_weapon_hand::Frame source{};
 std::uint32_t armed{},reserved{};
};
enum class Result:std::uint32_t {rejected,applied};
#if defined(_WIN32)
using Update=std::uint32_t(__stdcall*)(Request const*) noexcept;
#else
using Update=std::uint32_t(*)(Request const*) noexcept;
#endif
static_assert(std::is_trivially_copyable_v<Request>);
}
