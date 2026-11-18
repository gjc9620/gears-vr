#pragma once
#include "hand_object_grasp.h"
#include <type_traits>

// Optional cosmetic extension. Legacy Request/version2/export/layout remains
// unchanged; old callers always select automatic. No input/action permission.
namespace gears_hand_part_grasp {
using Preset=gears_hand_object_grasp::Preset;
struct Request {
    unsigned size{sizeof(Request)},version{3};
    gears_hand_object_grasp::Request pose{};
    Preset preset{Preset::precision};unsigned reserved{};
};
inline constexpr char publish_export[]="GearsVrPublishPartGraspV3";
#if defined(_WIN32)
using Publish=bool(__cdecl*)(Request const*) noexcept;
#endif
static_assert(std::is_trivially_copyable_v<Request>);
}
