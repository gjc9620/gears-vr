#pragma once
#include "weapon_equipment_observation.h"
// Read-only incarnation observation for an ALREADY owned original input.
// It is not an action/pose lease, restoration grant, current Frame or journal
// ownership proof. The caller separately retains its original serial/objects.
namespace gears_weapon_original_input_cleanup_context {
inline constexpr char export_name[]="GearsCaptureOriginalInputCleanupContextV1";
struct Request {
    std::uint32_t size{sizeof(Request)},version{1};
    gears_weapon_hand::EquipmentIdentity parent{};
    std::uint32_t reserved[2]{};
};
enum class Result:std::uint32_t {unavailable,rejected,same_incarnation};
#if defined(_WIN32)
using Query=std::uint32_t(__stdcall*)(Request const*) noexcept;
#else
using Query=std::uint32_t(*)(Request const*) noexcept;
#endif
// Native must independently observe the same registered game/Native, GT and
// world/actor incarnation, including world_generation. Pose/input/reference
// expiry may not manufacture a changed world, and a cached pointer tuple may
// not conceal travel/ABA. Unknown context stays unavailable. Terminal/drain,
// registration replacement and an unsafe inherited scope must be rejected.
// No engine calls, re-admission, tracking publication, callback or writer.
static_assert(std::is_trivially_copyable_v<Request>);
}
