// Included inside handstream after related(). Engine-capable legacy path;
// NoAuth callers must use the separate pure handoff helper.
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
// Pure name classification only. It grants no object association or mutation.
bool component_action_name(const wchar_t* label) noexcept {
    constexpr const wchar_t* actions[]={
        L"ServerUse",L"GrabPickup",L"ServerDictateSpecialMove",L"DoSpecialMove",
        L"EndSpecialMove",L"StartSpecialMove",L"StopSpecialMove",L"BodyStanceAnimEndNotification",
        L"OnAnimEnd",L"PlayBodyStance",L"PlayBodyStanceByDuration",L"SetBodyStance",
        L"StopBodyStance",L"StopAllBodyStances",L"StartMeleeAttack",L"StopMeleeAttack",
        L"ForceStopMeleeAttack",L"ForciblyStopMeleeAttack",L"ClientStopMeleeAttack",L"MeleeAttackStarted",
        L"MeleeAttackEnded",L"BeginState",L"EndState",L"SetWeaponTemporarilyHolstered",
        L"HolsterWeaponTemporarily",L"SetWeaponFromSlot",L"SetCurrentWeapon",L"ChangedWeapon",
        L"ClientWeaponSet",L"AttachWeaponToHand",L"DetachWeapon",L"AttachWeaponToSlot",
        L"DetachWeaponFromSlot",L"AttachComponent",L"AttachComponentToSocket",L"DetachComponent",
        L"DetachComponentFromSocket",L"ForceReload",L"Died",L"Destroyed",
        L"SetAbsolute",L"SetTranslation",L"SetRotation",L"PlayCustomAnim",
        L"PlayCustomAnimByDuration",L"StopCustomAnim"
    };
    for(const auto item:actions)if(!wcscmp(item,label))return true;
    return false;
}
#endif
enum class BeforeActionOutcome {ignored,clean,unresolved};
struct ActionAttempt {bool recognized{},restore_known{},restore_ok{};};
// The old void entry remains below. Report only after this function's local
// Action guard has finished its possible final drain, never in its return
// expression while the destructor is still pending.
void before_action_impl(uptr object,uptr function,ActionAttempt& attempt
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    ,bool owned_invoke
#endif
){
    // Reject other threads before touching any non-atomic game-thread state.
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    if(GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return;
    if((!active_visible.load(std::memory_order_acquire)&&!journals_dirty())||
       !trial.anchored||!registered(function))return;
#else
    if(!active_visible.load(std::memory_order_acquire)||
       GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return;
    if(trial.releasing||!trial.anchored||!registered(function)||!related(object))return;
#endif
    wchar_t label[96]{};
    if(!name(function,label,96)){
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        // Unknown names retain the original related-object fallback handling.
        (void)related(object);
#endif
        return;
    }
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    if(!component_action_name(label)||!related(object))return;
    attempt.recognized=true;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    component_continuous::Action continuous_action;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    component_continuous::suspend_interval(label,false);
#else
    component_continuous::request_terminal(7,label);
#endif
#endif
#else
    constexpr const wchar_t* actions[]={L"ServerUse",L"GrabPickup",L"ServerDictateSpecialMove",L"DoSpecialMove",
        L"EndSpecialMove",L"StartSpecialMove",L"StopSpecialMove",L"BodyStanceAnimEndNotification",L"OnAnimEnd",
        L"PlayBodyStance",L"PlayBodyStanceByDuration",L"SetBodyStance",L"StopBodyStance",L"StopAllBodyStances",
        L"StartMeleeAttack",L"StopMeleeAttack",L"ForceStopMeleeAttack",L"ForciblyStopMeleeAttack",
        L"ClientStopMeleeAttack",L"MeleeAttackStarted",L"MeleeAttackEnded",L"BeginState",L"EndState",
        L"SetWeaponTemporarilyHolstered",L"HolsterWeaponTemporarily",
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
        L"SetWeaponFromSlot",L"SetCurrentWeapon",L"ChangedWeapon",L"ClientWeaponSet",
        L"AttachWeaponToHand",L"DetachWeapon",L"AttachWeaponToSlot",L"DetachWeaponFromSlot",
        L"AttachComponent",L"AttachComponentToSocket",L"DetachComponent",L"DetachComponentFromSocket",
        L"ForceReload",L"Died",L"Destroyed",
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        L"SetAbsolute",L"SetTranslation",L"SetRotation",
#endif
#endif
        L"PlayCustomAnim",L"PlayCustomAnimByDuration",L"StopCustomAnim"};
    bool watched=false;for(const auto item:actions)if(!wcscmp(item,label)){watched=true;break;}
    if(!watched)return;
    attempt.recognized=true;
#endif
    if(trial.phase==3&&(!wcscmp(label,L"PlayCustomAnim")||!wcscmp(label,L"PlayCustomAnimByDuration")))
        trial.slot_play_seen=true;
#if defined(GEARS_PLAYGROUND_HAND_IK_XR_STREAM_TRIAL)
    if(!wcscmp(label,L"StartMeleeAttack"))trial.melee_seen=true;
    if(!wcscmp(label,L"PlayCustomAnim")||!wcscmp(label,L"PlayCustomAnimByDuration"))trial.slot_play_seen=true;
#endif
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    const bool dirty=trial.lease.dirty();
    const bool restored=yield_for_equipment(label);
    attempt.restore_known=true;attempt.restore_ok=restored;
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    if(!restored&&!owned_invoke){
        // Original PE/CallFunction must keep consuming/forwarding its ABI.
        // Relinquish before it runs; late ABA cannot resurrect rollback rights.
        hand_writes_relinquished=true;component_relinquish();
        if(!trial.ended)end(8,L"external_action_with_unclean_journals");
        publish_journals();
    }
#endif
    if(log_file&&diagnostic_event()){fwprintf(log_file,L"hand_ik_stream_trial_action function=%ls epoch=%u "
        L"dirty_before=%u dirty_after=%u restored=%u equipment_phase=%u\n",label,trial.epoch,
        unsigned(dirty),unsigned(trial.lease.dirty()),unsigned(restored),trial.xr_equipment_phase);fflush(log_file);}
#else
    ++trial.epoch;trial.stability.reset();
    const bool dirty=trial.lease.dirty();
    if(dirty){
        ++trial.action_yields;
        const bool restored=restore(L"action_boundary");
        attempt.restore_known=true;attempt.restore_ok=restored;
        if(log_file){fwprintf(log_file,L"hand_ik_stream_trial_action function=%ls epoch=%u "
            L"dirty_before=1 dirty_after=%u restored=%u\n",label,trial.epoch,
            unsigned(trial.lease.dirty()),unsigned(restored));fflush(log_file);}
        if(!restored)return;
#if !defined(GEARS_PLAYGROUND_HAND_IK_XR_STREAM_TRIAL)
        if(!trial.immediate_pending)end(7,L"unexpected_action_yield");
#endif
    }
#if defined(GEARS_PLAYGROUND_HAND_IK_XR_STREAM_TRIAL)
    // Block the old plan synchronously, even when the observer log is full.
    gears_hand_stream::Context blocked{};
    blocked.action_epoch=trial.epoch;blocked.observed_ms=GetTickCount64();blocked.clean=!trial.lease.dirty();
    observe_xr_policy(blocked,nullptr,blocked.observed_ms);
    for(auto& binding:trial.xr_binding)binding.reset();
    trial.xr_ready=false;trial.phase=1;status=1;
#endif
#endif
}
BeforeActionOutcome before_action_checked(uptr object,uptr function
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    ,bool owned_invoke=false
#endif
){
    ActionAttempt attempt{};
    before_action_impl(object,function,attempt
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        ,owned_invoke
#endif
    );
    if(!attempt.recognized)return BeforeActionOutcome::ignored;
    const bool clean=(!attempt.restore_known||attempt.restore_ok)&&!trial.lease.dirty()&&!trial.lease.faulted()
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        &&!component_dirty()&&!component_faulted()&&!hand_writes_relinquished
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
        &&!component_continuous::state.cleanup_failed
#endif
        ;
    return clean?BeforeActionOutcome::clean:BeforeActionOutcome::unresolved;
}
void before_action(uptr object,uptr function
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    ,bool owned_invoke
#endif
){
    (void)before_action_checked(object,function
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        ,owned_invoke
#endif
    );
}
