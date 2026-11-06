// Include after the phase1 definitions, at the PG namespace scope. This is
// classification and NoAuth fencing only, never an engine-capable handback.
// Seen is invocation-local; its addresses are objects/functions, not fields.
namespace weaponaction_handoff {
struct Seen {
    uptr object{},function{};std::uint32_t thread{};
    wchar_t function_name[96]{}; // Same existing metadata read, observation only.
    bool gt{},name_known{},recognized{},unknown_name{};
    bool hands{},stow{},hands_required{},stow_required{},required{};
    bool hands_restore_echo{},stow_restore_echo{},owned_restore_echo{};
    bool hands_dirty{},component_dirty{},stow_dirty{},pending{};
    bool hands_restoration_busy{},pending_state_boundary{};
    // The old non-phase1 hand Backend has no pure in-flight write fence.
    bool hands_forward_fence_supported{};
};
static_assert(std::is_trivially_copyable<Seen>::value,"Action observations retain no ownership or VM-local fields");

#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
inline bool hands_name(wchar_t const* label) noexcept {
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    return handstream::component_action_name(label);
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
#endif
        L"PlayCustomAnim",L"PlayCustomAnimByDuration",L"StopCustomAnim"};
    for(const auto action:actions)if(!wcscmp(label,action))return true;
    return false;
#endif
}
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
inline bool stow_name(wchar_t const* label,bool has_pending) noexcept {
    constexpr const wchar_t* actions[]={L"SetWeaponTemporarilyHolstered",L"HolsterWeaponTemporarily",
        L"SetWeaponFromSlot",L"SetCurrentWeapon",L"ChangedWeapon",L"ClientWeaponSet",
        L"AttachWeaponToHand",L"DetachWeapon",L"AttachWeaponToSlot",L"DetachWeaponFromSlot",
        L"AttachComponent",L"AttachComponentToSocket",L"DetachComponent",L"DetachComponentFromSocket",
        L"ServerUse",L"GrabPickup",L"ServerDictateSpecialMove",L"DoSpecialMove",L"EndSpecialMove",
        L"StartSpecialMove",L"StopSpecialMove",L"StartMeleeAttack",L"StopMeleeAttack",
        L"PlayBodyStance",L"PlayBodyStanceByDuration",L"SetBodyStance",L"StopAllBodyStances",
        L"BeginState",L"EndState",L"ForceReload",L"Died",L"Destroyed"};
    for(const auto action:actions)if(!wcscmp(label,action))return true;
    return has_pending&&(!wcscmp(label,L"ButtonPress")||!wcscmp(label,L"ButtonRelease"));
}
#endif

inline Seen inspect(uptr object,uptr function) {
    Seen seen{};seen.object=object;seen.function=function;seen.thread=GetCurrentThreadId();
    const auto gt=game_thread.load(std::memory_order_acquire);
    if(!gt||seen.thread!=gt)return seen;
    seen.gt=true;
    wchar_t label[96]{};seen.name_known=registered(function)&&name(function,label,96);
    if(seen.name_known)for(unsigned i=0;i<96;++i)seen.function_name[i]=label[i];
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
    seen.hands_dirty=handstream::trial.lease.dirty();
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    seen.component_dirty=handstream::component_dirty();seen.hands_forward_fence_supported=true;
    const bool hand_live=handstream::active_visible.load(std::memory_order_acquire)||seen.hands_dirty||seen.component_dirty;
    seen.hands_restoration_busy=handstream::trial.releasing||handstream::dual_draining;
#else
    const bool hand_live=handstream::active_visible.load(std::memory_order_acquire);
    seen.hands_restoration_busy=handstream::trial.releasing;
#endif
    const bool hand_watched=seen.name_known&&hands_name(label);
    seen.recognized=hand_watched;
    if(hand_live&&handstream::trial.anchored&&(hand_watched||!seen.name_known)&&handstream::related(object)){
        seen.hands=true;
#if !defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        // Observation only. A resource guard is not an exact initial bridge.
        seen.hands_restore_echo=handstream::trial.releasing;
#endif
        seen.hands_required=true;
    }
    // Deliberately do not inspect/consume component::self. Two addresses do
    // not prove the initial owned Args->invoke bridge; CF/PE cannot borrow it.
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
    namespace stow=vrequip::stow;
    seen.stow_dirty=stow::lease.dirty();seen.pending=stow::pending.active();
    const bool stow_watched=seen.name_known&&stow_name(label,seen.pending);
    seen.recognized=seen.recognized||stow_watched;
    if((stow::dirty_visible.load(std::memory_order_acquire)||stow::pending_visible.load(std::memory_order_acquire))&&
       (seen.stow_dirty||seen.pending)&&(stow_watched||!seen.name_known)&&stow::related(object)){
        seen.stow=true;seen.stow_restore_echo=stow::mutating;seen.stow_required=true;
        // The old Hammerburst exemption needs GetStateName/can_wait. NoAuth
        // retains this as an unresolved pending obligation; no state query.
        seen.pending_state_boundary=seen.pending&&seen.name_known&&
            (!wcscmp(label,L"BeginState")||!wcscmp(label,L"EndState"));
    }
#endif
    seen.unknown_name=!seen.name_known&&(seen.hands||seen.stow);
    seen.required=seen.hands_required||seen.stow_required;
    seen.owned_restore_echo=seen.hands_restore_echo||seen.stow_restore_echo;
    return seen;
}

template<class T>inline bool advance_epoch(T& value) noexcept {
    if(value==(std::numeric_limits<T>::max)())return false;
    ++value;return true;
}
inline void revoke_no_engine(Seen const& seen) noexcept {
    const auto gt=game_thread.load(std::memory_order_acquire);
    if(!seen.gt||!gt||seen.thread!=gt||GetCurrentThreadId()!=gt)return;
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
    if(seen.hands_required){
        namespace hand=handstream;
        const bool dirty=hand::trial.lease.dirty()
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
            ||hand::component_dirty()
#endif
            ;
        if(!advance_epoch(hand::trial.epoch)||dirty)hand::stop_requested.store(true,std::memory_order_release);
        hand::trial.stability.reset();hand::trial.consumer.reset();
#if defined(GEARS_PLAYGROUND_HAND_IK_XR_STREAM_TRIAL)
        hand::trial.xr_ready=false;
#if defined(GEARS_PLAYGROUND_HAND_RIG_TRACKING_TRIAL)
        hand::trial.xr_policy.revoke();
#else
        gears_hand_stream::Context blocked{};blocked.action_epoch=hand::trial.epoch;
        // Zero is an explicitly unusable observation, never a fresh clock or
        // ticket. Call only the pure policy, not its PG timing/log wrapper.
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
        (void)hand::trial.xr_policy.observe_certified(blocked,nullptr,0,0);
#else
        (void)hand::trial.xr_policy.observe(blocked,nullptr,0);
#endif
#endif
        for(auto& binding:hand::trial.xr_binding)binding.reset();
#endif
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        // These pure fences retain pending journal bits. Do not call
        // component_reset_binding: some configurations can finish/end there.
        hand::component::mount.reset();
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
        hand::component::lease.cancel_forward();
        auto& lifecycle=hand::component_continuous::state;
        lifecycle.closed=true;lifecycle.deferred=true;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
        lifecycle.resume_stable=false;lifecycle.pause=hand::component_continuous::Pause::boundary;
        if(!lifecycle.terminal&&!hand::trial.ended)lifecycle.interval=hand::component_continuous::Interval::closing;
#endif
#endif
        if(dirty){
            hand::hand_writes_relinquished=true;
            if(hand::component_dirty())hand::component::lease.relinquish();
        }
#endif
        if(dirty)hand::safe_to_unload.store(false,std::memory_order_release);
    }
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
    if(seen.stow_required){
        namespace stow=vrequip::stow;
        stow::stop_requested.store(true,std::memory_order_release);
        (void)advance_epoch(stow::epoch);
        stow::lease.revoke(); // No backend, no read, no setter, dirty retained.
        // Keep pending, its model/slot/ack and visibility unresolved. The
        // original commit_allowed/poll sees Stop and cannot forward it later.
    }
#endif
}
} // namespace weaponaction_handoff
