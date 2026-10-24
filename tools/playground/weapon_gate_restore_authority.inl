// Include at PG namespace scope AFTER the actual subscriber, restore backend
// and PE/owned entry definitions. Native still owns the sole CF hook/original.
// This opt-in adapter only restores already owned fields. It grants no forward
// write, NormalFrame/Tick stage, engine pointer, or external Open receipt.
#if defined(GEARS_PLAYGROUND_WEAPON_GATE_RESTORE_TRIAL)
#if !defined(GEARS_PLAYGROUND_WEAPON_ACTION_ENTRY_TRIAL) || !defined(GEARS_PLAYGROUND_WEAPON_ACTION_GATE_TRIAL) || \
    !defined(GEARS_PLAYGROUND_WEAPON_HAND_PARTICIPANT_TRIAL) || !defined(GEARS_PLAYGROUND_RESTORE_STAGE_CORE)
#error Gate restoration requires the real hand/Gate/entry registration and existing restoration coordinator
#endif
namespace weapongaterestore {
namespace abi=gears_weapon_action;
using Base=weaponrestore::BoundaryBackend;
using Stage=weaponrestore::Journal<Base::Snapshot>;
struct Backend;
struct Authority {
    abi::Invocation call{};
    abi::State* state{};
    Authority* address{};Authority* previous{};
    Backend* backend{};Stage* stage{};
    weaponentry::Entry* entry{};
    bool linked{};
};
static_assert(std::is_trivially_copyable_v<Authority>&&std::is_trivially_destructible_v<Authority>);
thread_local Authority* current{}; // A denial marker without the exact proof below.

void poison_no_engine() noexcept {
    Base backend{};weaponrestore::poison(backend);
}
inline bool same_scope(Authority const& authority) noexcept {
    if(!authority.state||!abi::invocation_valid(authority.call)||
       authority.call.thread_id!=GetCurrentThreadId())return false;
    const auto journal=weaponaction::load(authority.state);
    constexpr auto required=weaponaction::created|weaponaction::leased|weaponaction::linked;
    // The immutable flag lives through original, but its permission does not.
    // Only the pre-observer begin phase may claim/run this restore adapter.
    constexpr auto terminal=weaponaction::unresolved|weaponaction::link_left|weaponaction::closed|
        weaponaction::transaction_release_attempted|weaponaction::callback_release_attempted|
        weaponaction::begin_done|weaponaction::ignored|weaponaction::observer_attempted|
        weaponaction::observer_done|weaponaction::observer_close_attempted|weaponaction::observer_closed;
    return journal.this_state==reinterpret_cast<std::uintptr_t>(authority.state)&&
        abi::same(journal.call,authority.call)&&!journal.reserved&&!(journal.flags&~weaponaction::all_flags)&&
        (journal.flags&required)==required&&!(journal.flags&terminal)&&!journal.observer&&
        weaponaction::top==authority.state&&weaponaction::depth==journal.depth&&journal.depth!=0&&
        authority.call.hand_cookie==weaponaction::attached_cookie.load(std::memory_order_acquire)&&
        authority.call.native_started_qpc==weaponaction::attached_native.load(std::memory_order_acquire)&&
        authority.call.hand_cookie==weaponhand::client.cookie()&&
        authority.call.native_started_qpc==weaponhand::client.native_started_qpc();
}
inline bool entry_matches(Authority const& authority) noexcept {
    return weaponentry::top==authority.entry&&(!authority.entry||
        (authority.entry->address==reinterpret_cast<std::uintptr_t>(authority.entry)&&
         authority.entry->linked&&!authority.entry->finished&&!authority.entry->fault));
}
inline bool registration_retained(abi::Invocation const& call) noexcept {
    const auto phase=weaponhand::client.phase();
    // Stop closes future admission, not a Native scope already holding this
    // exact gap. Its admitted rollback may finish while the client drains.
    return !weaponaction::faulted.load(std::memory_order_acquire)&&
        !weaponcall::faulted.load(std::memory_order_acquire)&&
        call.hand_cookie==weaponcall::attached_cookie.load(std::memory_order_acquire)&&
        call.native_started_qpc==weaponcall::attached_native.load(std::memory_order_acquire)&&
        (weaponhand::client.registration_alive(call.hand_cookie,call.native_started_qpc)||
         phase==gears_weapon_hand::ClientPhase::draining);
}
inline bool grant(Authority const& authority,Backend const* backend,bool entered) noexcept {
    if(authority.address!=&authority||current!=&authority||!authority.linked||authority.previous||
       authority.backend!=backend||!authority.stage||!same_scope(authority)||!entry_matches(authority)||
       !weaponrestore::healthy()||!enabled.load(std::memory_order_acquire)||
       !weaponhand::current_gt(nullptr)||!registration_retained(authority.call)||
       authority.call.flags!=abi::engine_calls_allowed)return false;
    const auto journal=weaponaction::load(authority.state);
    if(journal.depth!=1||journal.prior_state||!(journal.flags&weaponaction::restore_claimed)||
       (journal.flags&(weaponaction::restore_closed|weaponaction::restore_complete)))return false;
    if(entered)return weaponrestore::active_stage==authority.stage&&
        (authority.stage->flags&(weaponrestore::created|weaponrestore::consumed|weaponrestore::entered))==
            (weaponrestore::created|weaponrestore::consumed|weaponrestore::entered);
    return !weaponrestore::active()&&authority.stage->flags==weaponrestore::created;
}

// Reuse the actual snapshot, explicit finishes, verdict and POD cleanup. The
// inherited external Request/Receipt retain their ungranted defaults and are
// NEVER consulted as a
// grant here. Only these four methods differ from the external-boundary path.
struct Backend:Base {
    Authority* authority{};
    bool admissible()const noexcept {
        if(!authority||!grant(*authority,this,false))return false;
        const auto gt=game_thread.load(std::memory_order_acquire);
        if(!gt||GetCurrentThreadId()!=gt||!seen.gt||seen.thread!=gt||seen.unknown_name||
           !disarmed.known||disarmed.thread!=gt||seen.object!=authority->call.self||
           seen.function!=authority->call.callee||disarmed.object!=seen.object||disarmed.function!=seen.function)return false;
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
        return false;
#endif
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
        if(vrequip::stow::mutating||vrequip::stow::yielding)return false;
#endif
        return seen.hands||seen.stow;
    }
    bool begin(){
        if(!authority||!grant(*authority,this,true)){
            guard_mismatch=true;weaponrestore::poison(*this);return false;
        }
        inside=true;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
        handstream::component_continuous::state.action_depth=1;hold=true;
#endif
        return true;
    }
    weaponrestore::Step hands(){
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
        if(seen.hands_required){
            const auto result=handstream::before_action_checked(authority->call.self,authority->call.callee
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
                ,false // A CF notice is never the initial owned Args/invoke hop.
#endif
            );
            return result==handstream::BeforeActionOutcome::clean?weaponrestore::Step::clean:weaponrestore::Step::unresolved;
        }
#endif
        return weaponrestore::Step::skipped;
    }
    weaponrestore::Step stow(){
        if(!weaponrestore::healthy()||guard_mismatch)return weaponrestore::Step::unresolved;
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
        if(seen.stow_required){
            const auto result=vrequip::stow::before_action_checked(authority->call.self,authority->call.callee);
            if(result==vrequip::stow::BeforeActionOutcome::pending_state_exempt){stow_exempt=true;return weaponrestore::Step::exempt;}
            return result==vrequip::stow::BeforeActionOutcome::clean?weaponrestore::Step::clean:weaponrestore::Step::unresolved;
        }
#endif
        return weaponrestore::Step::skipped;
    }
};
static_assert(std::is_trivially_destructible_v<Backend>&&std::is_trivially_destructible_v<Stage>);

inline void stage_cpp(Backend& backend,Stage& stage) noexcept {
    try{
        // Publication was revoked before claiming the stage. This only disarms
        // old forward plans/bindings; private rollback and stow epoch remain.
        backend.disarmed=weaponrestore::prepare_forward_no_engine(backend.seen,false);
        (void)weaponrestore::run(backend,stage);
    }catch(...){
        stage.result.outcome=weaponrestore::Outcome::unresolved;weaponrestore::poison(backend);
    }
}
inline bool run_stage(Authority& authority,Backend& backend,Stage& stage){
    authority.previous=current;authority.linked=true;current=&authority;
    bool complete=false;
    __try{stage_cpp(backend,stage);}
    __finally{
        __try{
            if(!same_scope(authority)||current!=&authority||!entry_matches(authority))weaponrestore::poison(backend);
            else{
                auto journal=weaponaction::load(authority.state);
                const bool closed=!(stage.flags&weaponrestore::entered)||(stage.flags&weaponrestore::cleanup_done);
                if(closed)journal.flags|=weaponaction::restore_closed;
                complete=closed&&weaponrestore::healthy()&&
                    (stage.result.outcome==weaponrestore::Outcome::complete||stage.result.outcome==weaponrestore::Outcome::exempt);
                if(complete){journal.flags|=weaponaction::restore_complete;
                    if(stage.result.outcome==weaponrestore::Outcome::exempt)journal.flags|=weaponaction::restore_exempt;}
                else journal.flags|=weaponaction::unresolved;
                weaponaction::store(authority.state,journal);
            }
        }__finally{current=authority.previous;authority.linked=false;}
    }
    return complete;
}

inline abi::BeginReply restore_body(void* context,abi::Invocation const* call,abi::State* state){
    if(context!=&weaponhand::client||!call||!state){poison_no_engine();return abi::BeginReply::handback_unresolved;}
    Authority authority{};authority.call=*call;authority.state=state;authority.address=&authority;authority.entry=weaponentry::top;
    if(!same_scope(authority)){poison_no_engine();return abi::BeginReply::handback_unresolved;}
    const auto journal=weaponaction::load(state);
    if((journal.flags&weaponaction::restore_claimed)||
       ((call->flags&abi::engine_calls_allowed)&&(!weaponhand::current_gt(nullptr)||journal.depth!=1))){
        poison_no_engine();return abi::BeginReply::handback_unresolved;
    }
    // Inspect is separately engine-free and rejects off-GT before GT-owned
    // state. Neither a component/stow self token nor a releasing flag is used.
    const auto seen=weaponaction_handoff::inspect(call->self,call->callee);
    if(!seen.hands&&!seen.stow)return abi::BeginReply::unrelated;
    if(!weaponentry::cancel_gate_post())return abi::BeginReply::handback_unresolved;
    weaponrestore::revoke_publication_no_engine();
    const bool permitted=(call->flags&abi::engine_calls_allowed)&&journal.depth==1&&!current&&
        !weaponrestore::active()&&weaponrestore::healthy()&&enabled.load(std::memory_order_acquire)&&
        registration_retained(*call)&&seen.gt&&!seen.unknown_name&&entry_matches(authority);
    if(!permitted){
        const bool clean=weaponrestore::healthy()&&!weaponaction::faulted.load(std::memory_order_acquire)&&
            weaponentry::no_engine_handback(seen);
        weaponaction_handoff::revoke_no_engine(seen);
        return clean?abi::BeginReply::handback_complete:abi::BeginReply::handback_unresolved;
    }
    Backend backend{};Stage stage{};backend.authority=&authority;backend.seen=seen;
    authority.backend=&backend;authority.stage=&stage;
    auto claimed=weaponaction::load(state);
    claimed.flags|=weaponaction::restore_claimed;weaponaction::store(state,claimed);
    // Claim is durable in the whole-gap Native State BEFORE the first helper;
    // run() additionally consumes this exact local Stage before Backend.begin.
    const bool complete=run_stage(authority,backend,stage);
    if(!complete)weaponaction_handoff::revoke_no_engine(seen);
    return complete?abi::BeginReply::handback_complete:abi::BeginReply::handback_unresolved;
}
inline abi::BeginReply restore_cpp(void* context,abi::Invocation const* call,abi::State* state) noexcept {
    try{return restore_body(context,call,state);}
    catch(...){poison_no_engine();return abi::BeginReply::handback_unresolved;}
}
abi::BeginReply restore(void* context,abi::Invocation const* call,abi::State* state) noexcept {
    // Only PG pre-action work is inside this barrier. Native invokes original
    // later; no original RAX, arguments or exceptions pass through this code.
    __try{return restore_cpp(context,call,state);}
    __except(weaponrestore::exception_filter(GetExceptionCode())){
        poison_no_engine();return abi::BeginReply::handback_unresolved;
    }
}
} // namespace weapongaterestore
#endif
