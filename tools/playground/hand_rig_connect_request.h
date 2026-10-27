#pragma once
#include <cstdint>
#include <type_traits>

namespace gears_hand_rig_trial {
// Owned private export payload. The Native image is already loaded by the
// selected capture session; this request neither loads it nor starts hands.
struct Connect {
    std::uint32_t schema{1},size{sizeof(Connect)};
    std::uint64_t native_module{},native_started_qpc{};
};
inline bool shape(Connect const& value) noexcept {
    return value.schema==1&&value.size==sizeof(Connect)&&value.native_module&&
        !(value.native_module&7)&&value.native_started_qpc;
}
static_assert(sizeof(Connect)==24&&std::is_standard_layout_v<Connect>&&std::is_trivially_copyable_v<Connect>);
} // namespace gears_hand_rig_trial
