#pragma once

namespace gears_lancer_saw_pose {
// Scope classification only. Callers must authenticate the live actor and
// Native's current held-mount response; these booleans are not permits.
// SM33 is the original stationary ready hold, SM34 is paired execution.
inline bool prepare_hands(unsigned special_move,bool current_lancer,bool hands_enabled) noexcept {
    return special_move==0||(special_move==33&&current_lancer&&hands_enabled);
}
inline bool commit_hands(unsigned special_move,bool current_lancer,bool hands_enabled,
    bool current_held_mount) noexcept {
    return prepare_hands(special_move,current_lancer,hands_enabled)&&
        (special_move==0||current_held_mount);
}
} // namespace gears_lancer_saw_pose
