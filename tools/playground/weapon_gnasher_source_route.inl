// Read-only geometry projection from the exact committed GN route. The caller
// still authenticates current profile/cohort/freshness. Never proposes, grants,
// restores a candidate/incumbent, or changes a source clock.
bool project_source_route(hand::Frame const& f,hand::Frame const& published,
    hand::GripGameState const& before,hand::GripGameState const& routed,
    hand::Frame const& decision_sample,hand::Decision const& decision,
    bool committed,bool current,hand::GripGameState& fresh_game,unsigned* route_failure=nullptr,
    bool certified_asset=false) noexcept {
    if(route_failure)*route_failure=0;
    const auto reject_route=[&](unsigned reason) noexcept {if(route_failure)*route_failure=reason;return false;};
    if(!committed||!current)return reject_route(1);
    if(!hand::same_frame(f,published))return reject_route(2);
    if(!hand::same_frame(f,decision_sample))return reject_route(3);
    if(!hand::same(f.key,decision.key))return reject_route(4);
    if(!before.known||!before.main_hand_held||before.busy||before.animation_owns_hands)return reject_route(5);
    // Generic support geometry deliberately excludes Gnasher. Its absence is
    // not an asset certificate: require the caller's fresh GN route_asset read.
    // If a base mount exists, preserve its identity/validity check as well.
    if(!certified_asset||(before.mount_known&&
       (!before.mount.valid()||!hand::same(before.mount_asset,routed.mount_asset))))return reject_route(6);
    if(before.kind!=hand::WeaponKind::firearm||!routed.known||!routed.main_hand_held||
       routed.busy||routed.animation_owns_hands||routed.kind!=hand::WeaponKind::firearm)return reject_route(7);
    if(!routed.mount_known||!routed.mount.valid()||!hand::identity_valid(routed.mount_asset)||!routed.mount_revision)return reject_route(8);
    if(!hand::same_game(fresh_game,before))return reject_route(9);
    if(!(fresh_game.support_box==before.support_box))return reject_route(10);
    // Preserve all freshly read flags, candidates and incumbents. Only the
    // previously committed representation of this same mount is selected.
    fresh_game.mount=routed.mount;fresh_game.mount_known=routed.mount_known;
    fresh_game.mount_asset=routed.mount_asset;fresh_game.mount_revision=routed.mount_revision;
    fresh_game.acquire_metres=routed.acquire_metres;fresh_game.release_metres=routed.release_metres;
    fresh_game.support_box=routed.support_box;
    return true;
}


