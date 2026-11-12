#pragma once
namespace gears_cover_pose_role {
// Selected only during successful CoverInput configuration with a live/top
// legacy reader AND both Native C0/Goal requested. No live role switching.
// The legacy target publisher is sealed when this flag is set; it supplies
// metadata only. A failed later C0 configuration still aborts startup.
inline bool c0_exclusive{};
inline bool no_competing_writer(bool legacy_enabled,bool input_enabled,bool engine_restore)noexcept{
    return !engine_restore&&((!legacy_enabled&&!input_enabled)||
        (c0_exclusive&&legacy_enabled&&input_enabled));
}
}
