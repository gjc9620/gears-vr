// Composition only: one current candidate, independent old-owner retirement.
// Class selection is a routing hint from the equipment model, never an action
// grant. Each consumer still authenticates its current source/mesh/mount/input.
namespace lancerreload_runtime {
void tick(uptr pc) noexcept {
    // Per-owner reset() implementations predate independent retirement. Merge
    // at every GT exit, including unheld/unsupported selection, so one finished
    // owner cannot erase another owner's remaining lifecycle obligation.
    // The existing callback counter brackets this whole composition scope.
    struct LifecycleFlush {
        ~LifecycleFlush() noexcept {
            outstanding.store(pending_work()||longshot_runtime::pending_work()||
                hammerburst_runtime::pending_work()||snub_runtime::pending_work()||
                boltok_runtime::pending_work()||gnasher_state::pending_work()||boomshot_runtime::pending_work(),std::memory_order_release);
        }
    } lifecycle_flush;
    const auto selected=uptr(vrequip::model.held_weapon());
    longshot_runtime::service_unselected(pc,selected);
    hammerburst_runtime::service_unselected(pc,selected);
    snub_runtime::service_unselected(pc,selected);
    boltok_runtime::service_unselected(pc,selected);
    service_unselected(pc,selected);
    gnasher_state::service_unselected(pc,selected,stop_requested.load(std::memory_order_acquire));
    boomshot_runtime::service_unselected(pc,selected);
    if(!selected)return;
    if(playground::instance(selected,L"Weap_Boomer_base")){boomshot_runtime::tick(pc);return;}
    if(playground::instance(selected,L"Weap_Bow_base")){torquebow_runtime::tick(pc);return;}
    if(playground::instance(selected,L"Weap_SniperRifle")){longshot_runtime::tick(pc);return;}
    if(playground::instance(selected,L"Weap_LocustAssaultRifle")){hammerburst_runtime::tick(pc);return;}
    if(playground::instance(selected,L"Weap_COGPistol")){snub_runtime::tick(pc);return;}
    if(playground::instance(selected,L"Weap_LocustPistol")){boltok_runtime::tick(pc);return;}
    if(playground::instance(selected,L"Weap_AssaultRifle"))tick_selected(pc);
}
}
