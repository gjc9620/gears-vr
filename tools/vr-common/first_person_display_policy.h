#pragma once
#include "menu_display_state.h"

namespace gears_first_person_display {
// Missing menu authority cannot authorize exposing an original third-person
// buffer. Explicit live flat-display modes keep their existing routing.
inline bool waiting_fallback(bool continuous, bool enabled, bool body_basis,
                             gears_menu::Mode mode) noexcept {
    return continuous && enabled && body_basis &&
        (mode == gears_menu::Mode::gameplay || mode == gears_menu::Mode::unknown);
}
}
