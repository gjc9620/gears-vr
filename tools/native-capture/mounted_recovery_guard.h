#pragma once
#include "crosshair_assignment_config_loader.h"

namespace gears_crosshair_assignment {
// Mounted suspension still needs a live camera guard. Preserve that guard's
// camera-only rollover diagnostic so world handoff can retry the same bundle.
template<class Read> bool mounted_recovery_fresh(OwnedConfig const& candidate,
    gears_anchor::Config const& anchor,gears_ui::Config const& ui,
    unsigned pid,std::uintptr_t base,Read read,gears_anchor::GuardDiagnostic* diagnostic=nullptr){
    if(diagnostic)*diagnostic={};
    if(!candidate.mounted_suspended||!anchor.mounted_enabled||!ui.mounted||
       anchor.role_controller>=anchor.identity_count||anchor.role_pawn>=anchor.identity_count||
       anchor.mounted_role_driver>=anchor.identity_count||anchor.role_camera>=anchor.identity_count||
       candidate.pid!=pid||candidate.identities.base!=base||
       candidate.suspended_controller!=anchor.identities[anchor.role_controller].address||
       candidate.suspended_controlled!=anchor.identities[anchor.role_pawn].address||
       candidate.suspended_driver!=anchor.identities[anchor.mounted_role_driver].address||
       candidate.suspended_camera!=anchor.identities[anchor.role_camera].address)return false;
    gears_anchor::Anchor live{};std::uint32_t stamp{};
    return gears_anchor::guard(anchor,read,live,stamp,diagnostic);
}
}
