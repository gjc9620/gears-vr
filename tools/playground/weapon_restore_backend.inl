// Concrete existing-engine adapter, after phase1/stow/client definitions.
// Not called by PE/bridge yet. Native grant + local lifetime + forward-disarm
// proof are mandatory; standalone legacy candidates cannot authorize a stage.
namespace weaponrestore {
struct ForwardDisarm {
    uptr object{},function{};DWORD thread{};unsigned hand_epoch{};bool known{};
};
inline ForwardDisarm prepare_forward_no_engine(weaponaction_handoff::Seen const& seen) noexcept {
    ForwardDisarm out{};
    const auto gt=game_thread.load(std::memory_order_acquire);
    if(!healthy()||!seen.gt||!gt||seen.thread!=gt||GetCurrentThreadId()!=gt||(!seen.hands&&!seen.stow))return out;
#if defined(GEARS_PLAYGROUND_WEAPON_HAND_PARTICIPANT_TRIAL)
    auto registration=weaponhand::client.registration();gears_weapon_hand::Invalidation invalidation{};
    invalidation.registration_cookie=weaponhand::client.cookie();invalidation.native_started_qpc=weaponhand::client.native_started_qpc();
    invalidation.reason=gears_weapon_hand::InvalidateReason::context_lost;registration.invalidate(registration.context,&invalidation);
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
    if(seen.hands_required){
        namespace hand=handstream;
        if(!weaponaction_handoff::advance_epoch(hand::trial.epoch)){
            sticky_fault.store(true,std::memory_order_release);hand::stop_requested=true;hand::safe_to_unload=false;return out;
        }
        hand::trial.stability.reset();hand::trial.consumer.reset();
#if defined(GEARS_PLAYGROUND_HAND_IK_XR_STREAM_TRIAL)
        hand::trial.xr_ready=false;gears_hand_stream::Context blocked{};blocked.action_epoch=hand::trial.epoch;
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
        (void)hand::trial.xr_policy.observe_certified(blocked,nullptr,0,0);
#else
        (void)hand::trial.xr_policy.observe(blocked,nullptr,0);
#endif
        for(auto& binding:hand::trial.xr_binding)binding.reset();
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
        hand::component_continuous::close(L"native_pre_action",true);
#endif
    }
    out.hand_epoch=handstream::trial.epoch;
#endif
    // No stow epoch/pending/lease change: its epoch belongs to the rollback
    // Key and may advance only at the original post-restoration point.
    out.object=seen.object;out.function=seen.function;out.thread=gt;out.known=true;return out;
}
struct BoundaryBackend {
    struct Snapshot {
        bool inside{},hand_releasing{},dual{},finishing{},stow_mutating{},stow_yielding{},reach_busy{};
        unsigned action_depth{},operation_depth{},timing_open{};
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        handstream::component::SelfSetter self{};
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
        // ForwardDiagnostics' default constructor samples TLS. Keep bytes here
        // so Journal{} itself never reads GT-only state before admissible().
        std::array<std::byte,sizeof(handstream::component::ForwardDiagnostics)> diagnostics{};
#endif
#endif
    };
    gears_weapon_boundary::Request request{};gears_weapon_boundary::Receipt receipt{};
    gears_weapon_hand::Reply open_reply{gears_weapon_hand::Reply::unavailable};
    weaponaction_handoff::Seen seen{};ForwardDisarm disarmed{};
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_BOUNDARY_TRIAL)
    gears_weapon_boundary::Admission::Entry const* lifetime{};
#endif
    bool hold{},guard_mismatch{},stow_exempt{};
    bool admissible()const noexcept {
#if !defined(GEARS_PLAYGROUND_WEAPON_ACTION_BOUNDARY_TRIAL) || !defined(GEARS_PLAYGROUND_WEAPON_HAND_PARTICIPANT_TRIAL)
        return false;
#else
        namespace abi=gears_weapon_boundary;
        const auto gt=game_thread.load(std::memory_order_acquire);
        if(!gt||GetCurrentThreadId()!=gt||!enabled.load(std::memory_order_acquire)||!seen.gt||seen.thread!=gt||
           seen.unknown_name||!disarmed.known||disarmed.thread!=gt||disarmed.object!=request.object||disarmed.function!=request.function||
           seen.object!=request.object||seen.function!=request.function||!abi::request_valid(request)||request.thread_id!=gt||
           open_reply!=gears_weapon_hand::Reply::accepted||receipt.size!=sizeof(receipt)||receipt.abi_version!=abi::version||
           receipt.hand_cookie!=request.hand_cookie||receipt.native_started_qpc!=request.native_started_qpc||
           receipt.thread_id!=gt||!receipt.scope_id||!receipt.revocation_generation||
           receipt.flags!=(abi::scope_active|abi::engine_calls_allowed))return false;
        if(!lifetime)return false;
        if(!weaponboundary::grant_matches(*lifetime,request,receipt,open_reply))return false;
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
        if(handstream::trial.releasing||handstream::trial.epoch!=disarmed.hand_epoch)return false;
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL) && defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
        auto const& lifecycle=handstream::component_continuous::state;
        if(handstream::dual_draining||lifecycle.action_depth||lifecycle.operation_depth||lifecycle.finishing||
           handstream::component::self.armed)return false;
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
        if(handstream::trial.lease.busy()||handstream::trial.lease.restoring())return false;
#endif
#else
        return false; // Legacy hand writers lack a certified in-flight fence.
#endif
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
        if(vrequip::stow::mutating||vrequip::stow::yielding)return false;
#endif
        return seen.hands||seen.stow;
#endif
    }
    bool snapshot_no_engine(Snapshot& out)const noexcept {
        out.inside=inside;
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
        out.hand_releasing=handstream::trial.releasing;
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        out.dual=handstream::dual_draining;out.self=handstream::component::self;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
        const handstream::component::ForwardDiagnostics diagnostic{};std::memcpy(out.diagnostics.data(),&diagnostic,sizeof(diagnostic));
#endif
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
        auto const& lifecycle=handstream::component_continuous::state;
        out.action_depth=lifecycle.action_depth;out.operation_depth=lifecycle.operation_depth;out.finishing=lifecycle.finishing;
#endif
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
        out.timing_open=handstream::trial.timing.open_scopes;
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
        out.reach_busy=handstream::trial.reach.busy;
#endif
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
        out.stow_mutating=vrequip::stow::mutating;out.stow_yielding=vrequip::stow::yielding;
#endif
        return true;
    }
    void begin(){
        inside=true;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
        handstream::component_continuous::state.action_depth=1;hold=true;
#endif
    }
    Step hands(){
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
        if(seen.hands_required){
            const auto result=handstream::before_action_checked(request.object,request.function
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
                ,request.source==gears_weapon_boundary::Source::owned_bridge
#endif
            );
            return result==handstream::BeforeActionOutcome::clean?Step::clean:Step::unresolved;
        }
#endif
        return Step::skipped;
    }
    bool drop_hold_no_engine() noexcept {
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
        auto& lifecycle=handstream::component_continuous::state;
        if(!hold||lifecycle.action_depth!=1||lifecycle.operation_depth||lifecycle.finishing){
            guard_mismatch=true;weaponrestore::poison(*this);return false;
        }
        lifecycle.action_depth=0;hold=false;
#endif
        return true;
    }
    bool finish_hands(){
        if(!drop_hold_no_engine())return false;
        if(!healthy()||guard_mismatch)return false;
        bool finished=true;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
        if(seen.hands_required)finished=handstream::component_continuous::finish();
        if(!healthy()||handstream::component_continuous::state.action_depth||
           handstream::component_continuous::state.operation_depth||handstream::component_continuous::state.finishing){
            guard_mismatch=true;weaponrestore::poison(*this);return false;
        }
        handstream::component_continuous::state.action_depth=1;hold=true;
#endif
        return finished;
    }
    Step stow(){
        if(!healthy()||guard_mismatch)return Step::unresolved;
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
        if(seen.stow_required){
            const auto result=vrequip::stow::before_action_checked(request.object,request.function);
            if(result==vrequip::stow::BeforeActionOutcome::pending_state_exempt){stow_exempt=true;return Step::exempt;}
            return result==vrequip::stow::BeforeActionOutcome::clean?Step::clean:Step::unresolved;
        }
#endif
        return Step::skipped;
    }
    bool finish_all(){
        if(!drop_hold_no_engine())return false;
        if(!healthy()||guard_mismatch)return false;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
        // An authenticated pending-state exemption did not mutate a journal;
        // do not invent a new engine drain if hands had no obligation either.
        if(seen.hands_required||(seen.stow_required&&!stow_exempt))return handstream::component_continuous::finish();
#endif
        return true;
    }
    Outcome verdict(Result const& result)const noexcept {
        if(!healthy()||guard_mismatch||!result.hands_finished||!result.all_finished)return Outcome::unresolved;
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
        if(seen.hands_required&&(result.hands!=Step::clean||handstream::trial.lease.dirty()||handstream::trial.lease.faulted()
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
            ||handstream::component_dirty()||handstream::component_faulted()||handstream::hand_writes_relinquished
#endif
            ))return Outcome::unresolved;
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
        if(seen.stow_required){
            if(result.stow==Step::exempt&&stow_exempt&&seen.pending_state_boundary)return Outcome::exempt;
            if(result.stow!=Step::clean||vrequip::stow::lease.dirty()||vrequip::stow::lease.faulted()||
               vrequip::stow::pending.active()||vrequip::stow::pending_visible.load(std::memory_order_acquire))return Outcome::unresolved;
        }
#endif
        return Outcome::complete;
    }
    void poison_no_engine() noexcept {
        [[maybe_unused]] const auto gt=game_thread.load(std::memory_order_acquire);
        [[maybe_unused]] const bool on_gt=gt&&GetCurrentThreadId()==gt;
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
        handstream::stop_requested=true;handstream::safe_to_unload=false;
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        if(on_gt){handstream::hand_writes_relinquished=true;handstream::component::lease.relinquish();}
#endif
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
        vrequip::stow::stop_requested=true;if(on_gt)vrequip::stow::lease.revoke();
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_HAND_PARTICIPANT_TRIAL)
        auto registration=weaponhand::client.registration();gears_weapon_hand::Invalidation invalidation{};
        invalidation.registration_cookie=weaponhand::client.cookie();invalidation.native_started_qpc=weaponhand::client.native_started_qpc();
        invalidation.reason=gears_weapon_hand::InvalidateReason::callback_fault;registration.invalidate(registration.context,&invalidation);
#endif
    }
    bool cleanup_no_engine(Snapshot const& saved,bool normal) noexcept {
        bool exact=inside;
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
        exact=exact&&(!normal||handstream::trial.releasing==saved.hand_releasing);handstream::trial.releasing=saved.hand_releasing;
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        exact=exact&&(!normal||handstream::dual_draining==saved.dual);handstream::dual_draining=saved.dual;
        exact=exact&&(!normal||(handstream::component::self.object==saved.self.object&&
            handstream::component::self.function==saved.self.function&&handstream::component::self.armed==saved.self.armed));
        handstream::component::self=saved.self;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
        const handstream::component::ForwardDiagnostics current_diagnostic{};
        handstream::component::ForwardDiagnostics diagnostic{};std::memcpy(&diagnostic,saved.diagnostics.data(),sizeof(diagnostic));
        exact=exact&&(!normal||(current_diagnostic.depth==diagnostic.depth&&current_diagnostic.names==diagnostic.names&&
            current_diagnostic.paused==diagnostic.paused&&current_diagnostic.snapshot==diagnostic.snapshot&&
            current_diagnostic.aggregate==diagnostic.aggregate&&current_diagnostic.slot==diagnostic.slot&&
            current_diagnostic.snapshot_group==diagnostic.snapshot_group&&current_diagnostic.aggregate_group==diagnostic.aggregate_group&&
            current_diagnostic.meta.group==diagnostic.meta.group&&current_diagnostic.meta.forward==diagnostic.meta.forward&&
            current_diagnostic.meta.wanted_known==diagnostic.meta.wanted_known&&current_diagnostic.meta.wanted_original==diagnostic.meta.wanted_original&&
            gears_weapon::mesh_component_exact(current_diagnostic.meta.expected,diagnostic.meta.expected)&&
            gears_weapon::mesh_component_exact(current_diagnostic.meta.wanted,diagnostic.meta.wanted)));
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
        exact=exact&&(!normal||current_diagnostic.callback_index==diagnostic.callback_index);
#endif
        diagnostic.restore();
#endif
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
        auto& lifecycle=handstream::component_continuous::state;
        exact=exact&&(!normal||(!lifecycle.action_depth&&!lifecycle.operation_depth&&!lifecycle.finishing));
        lifecycle.action_depth=saved.action_depth;lifecycle.operation_depth=saved.operation_depth;lifecycle.finishing=saved.finishing;hold=false;
#endif
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
        exact=exact&&(!normal||handstream::trial.timing.open_scopes==saved.timing_open);handstream::trial.timing.open_scopes=saved.timing_open;
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
        exact=exact&&(!normal||handstream::trial.reach.busy==saved.reach_busy);handstream::trial.reach.busy=saved.reach_busy;
#endif
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
        exact=exact&&(!normal||(vrequip::stow::mutating==saved.stow_mutating&&vrequip::stow::yielding==saved.stow_yielding));
        vrequip::stow::mutating=saved.stow_mutating;vrequip::stow::yielding=saved.stow_yielding;
#endif
        inside=saved.inside;return exact;
    }
};
static_assert(std::is_trivially_copyable_v<BoundaryBackend::Snapshot>);
} // namespace weaponrestore
