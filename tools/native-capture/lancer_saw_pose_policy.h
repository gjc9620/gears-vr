#pragma once

namespace gears_lancer_saw_pose {
// Original WarPawn.ESpecialMove: SM_None=0, SM_Run2Idle=14,
// SM_Walk2Idle=15. Normal stopping transitions still consume current VR
// hands/eye. This classification grants no fire, grip or animation action.
inline bool ordinary_hands_motion(unsigned special_move) noexcept {
    return special_move==0||special_move==14||special_move==15;
}
// Scope classification only. Callers must authenticate the live actor and
// Native's current held-mount response; these booleans are not permits.
// SM33 is the original stationary ready hold, SM34 is paired execution.
inline bool prepare_hands(unsigned special_move,bool current_lancer,bool hands_enabled) noexcept {
    return ordinary_hands_motion(special_move)||(special_move==33&&current_lancer&&hands_enabled);
}
inline bool commit_hands(unsigned special_move,bool current_lancer,bool hands_enabled,
    bool current_held_mount) noexcept {
    return prepare_hands(special_move,current_lancer,hands_enabled)&&
        (ordinary_hands_motion(special_move)||current_held_mount);
}
} // namespace gears_lancer_saw_pose
