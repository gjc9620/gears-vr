#pragma once
#include <cstddef>

namespace gears_recording_capacity {
// Frame association and RS/OM binding state cover the same simultaneous lists.
// This is a fixed admission bound, never an eviction policy.
inline constexpr std::size_t command_lists=1024;
}
