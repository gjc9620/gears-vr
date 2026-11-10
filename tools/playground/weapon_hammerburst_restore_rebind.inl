// Fresh re-hold admission for a parked private transaction. All reads prepare
// copies first; no original reload, animation, hand lease or ammo write here.
namespace restore_control {
bool rebind(hand::Frame const& source) noexcept {
    if(!park_control::enabled()||stage!=Stage::parked||!parked_plan.valid||
       !fresh_restore_main(source)||!eligible(source)||
       !(parked_plan.transaction.inventory().identity==opening.identity)||
       !inv::same_instance_across_hold(opening.identity,inv::identity(source))||
       (serial&&(parked_native_lease.serial!=serial||!(parked_native_lease.identity==opening.identity)))||
       lancerreload_state::lease.acquired||anim::ownership.active||visual::active)return false;
    const auto previous_geometry=geometry;
    const auto& next_key=nativelancerfollow::bound_key;
    if(!previous_geometry.known||!hand::same(previous_geometry.key.weapon,next_key.weapon)||
       !hand::same(previous_geometry.key.component,next_key.component)||
       !hand::same(previous_geometry.key.asset,next_key.asset))return false;
    auto prepared_observer=observer;
    inv::Inventory rebound_inventory{};park_ledger::Plan prepared_plan{};
    if(!prepared_observer.rebind_hold(source,opening.identity,rebound_inventory)||
       !park_ledger::rebind(parked_plan,rebound_inventory,old_item,prepared_plan))return false;
    // discover authenticates this closed original mesh in the new mount.
    // A failed/freshness-expired preparation cannot replace saved geometry.
    if(!discover(source)||!geometry_current(source)||!fresh_restore_main(source)||
       !hand::same(previous_geometry.key.weapon,geometry.key.weapon)||
       !hand::same(previous_geometry.key.component,geometry.key.component)||
       !hand::same(previous_geometry.key.asset,geometry.key.asset)||
       !nativeequipment::current(nullptr,source)){
        geometry=previous_geometry;return false;
    }
    geometry.latch.open_angle=previous_geometry.latch.open_angle;
    observer=prepared_observer;parked_plan=prepared_plan;transaction=prepared_plan.transaction;
    opening.identity=rebound_inventory.identity; // Preserve original pre-insert counts/reward basis.
    if(parked_native_lease.serial)parked_native_lease.identity=rebound_inventory.identity;
    held={};stroke.reset();insertion.reset();extraction.reset();
    released=false;restore_display_ready=false;stage=Stage::restoring;
    return true;
}
}
