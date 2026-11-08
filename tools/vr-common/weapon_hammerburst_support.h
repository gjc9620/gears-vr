#pragma once
#include "weapon_support_socket_mode.h"

namespace gears_weapon_hammerburst_support {
// HB adapter keeps its call sites separate from the weapon-neutral policy.
// HB foregrip geometry and runtime ownership remain in their own modules.
using Mode=gears_weapon_support_socket::Mode;
inline Mode mode(gears_weapon_hand::Frame const& f,
                 gears_weapon_hand::GripGameState const& game,
                 gears_weapon_hand::Decision const& d,std::int64_t now) noexcept {
    return gears_weapon_support_socket::mode(f,game,d,now);
}
}
