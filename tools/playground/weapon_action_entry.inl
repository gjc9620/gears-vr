// Invocation-local routing only. Normal stream/stow Tick writers remain
// excluded from the runtime ENTRY profile until their own safe point exists.
namespace weaponentry {
using Source=gears_weapon_boundary::Source;
struct Entry;
thread_local Entry* top{};
thread_local std::uint64_t revocations{}; // A post-work cancellation fence, not a grant.
struct Entry {
    weaponboundary::Transport transport{};
    weaponrestore::BoundaryBackend backend{};
    weaponrestore::Journal<weaponrestore::BoundaryBackend::Snapshot> stage{};
    weaponrestore::BoundaryBackend::Snapshot lexical{};
    weaponaction_handoff::Seen seen{};
    Entry* previous{};std::uintptr_t address{};
    uptr object{},function{};void* params{};Source source{};
    bool callback{},release_attempted{},linked{},prepared{},finished{},gt_snapshot{},lexical_taken{},legacy_body{};
    bool recursive{},related{},bare{},handback{},stage_closed{},fault{};
    bool original_started{},original_returned{};
    std::uint64_t post_revision{};
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL) && defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    handstream::tick_boundary_diagnostic::Token* tick{};
    handstream::component_trace::AuthObservation* authorization{};
#endif
#if defined(GEARS_PLAYGROUND_HAND_TRANSPORT_LIVENESS_TRIAL)
    unsigned equipment_depth{};
#endif
};
static_assert(std::is_trivially_copyable_v<Entry>&&std::is_trivially_destructible_v<Entry>);

inline void poison(Entry& entry) noexcept {
    entry.fault=true;weaponrestore::poison(entry.backend);
    if(entry.transport.flags)weaponboundary::transport_poison(entry.transport);
}
inline void cancel_post(Entry& entry) noexcept {
    if(revocations==UINT64_MAX)poison(entry);else ++revocations;
}
inline bool no_engine_handback(weaponaction_handoff::Seen const& seen) noexcept {
    if(!seen.gt||seen.unknown_name)return false;
    if((seen.hands_required&&(seen.hands_dirty||seen.component_dirty||seen.hands_restoration_busy))||
       (seen.stow_required&&(seen.stow_dirty||seen.pending)))return false;
    bool clean=true;
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
    if(seen.hands_required)clean=seen.hands_forward_fence_supported&&!seen.hands_dirty&&!seen.component_dirty&&
        !seen.hands_restoration_busy&&!handstream::trial.lease.faulted()
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        &&!handstream::component_faulted()&&!handstream::hand_writes_relinquished
#endif
        ;
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
    if(seen.stow_required)clean=clean&&!seen.stow_dirty&&!seen.pending&&!vrequip::stow::mutating&&
        !vrequip::stow::yielding&&!vrequip::stow::lease.faulted();
#endif
    return clean;
}
inline void capture(Entry& entry) noexcept {
    entry.lexical.inside=inside;
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL) && defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    entry.tick=handstream::tick_boundary_diagnostic::current;
    entry.authorization=handstream::component_trace::observed_authorization;
#endif
#if defined(GEARS_PLAYGROUND_HAND_TRANSPORT_LIVENESS_TRIAL)
    entry.equipment_depth=handstream::equipment_entry_depth;
#endif
    const auto gt=game_thread.load(std::memory_order_acquire);
    entry.lexical_taken=true;
    if(gt&&GetCurrentThreadId()==gt)entry.gt_snapshot=entry.backend.snapshot_no_engine(entry.lexical);
}
inline bool lexical_exact(Entry const& entry) noexcept {
    auto const& saved=entry.lexical;bool same=inside==saved.inside;
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
    same=same&&handstream::trial.releasing==saved.hand_releasing;
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    same=same&&handstream::dual_draining==saved.dual&&handstream::component::self.object==saved.self.object&&
        handstream::component::self.function==saved.self.function&&handstream::component::self.armed==saved.self.armed;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    const handstream::component::ForwardDiagnostics now{};
    handstream::component::ForwardDiagnostics old{};std::memcpy(&old,saved.diagnostics.data(),sizeof(old));
    same=same&&now.depth==old.depth&&now.names==old.names&&now.paused==old.paused&&
        now.snapshot==old.snapshot&&now.aggregate==old.aggregate&&now.slot==old.slot&&
        now.snapshot_group==old.snapshot_group&&now.aggregate_group==old.aggregate_group&&
        now.meta.group==old.meta.group&&now.meta.forward==old.meta.forward&&
        now.meta.wanted_known==old.meta.wanted_known&&now.meta.wanted_original==old.meta.wanted_original&&
        gears_weapon::mesh_component_exact(now.meta.expected,old.meta.expected)&&
        gears_weapon::mesh_component_exact(now.meta.wanted,old.meta.wanted);
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
    same=same&&now.callback_index==old.callback_index;
#endif
#endif
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    auto const& state=handstream::component_continuous::state;
    same=same&&state.action_depth==saved.action_depth&&state.operation_depth==saved.operation_depth&&state.finishing==saved.finishing;
#endif
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    same=same&&handstream::trial.timing.open_scopes==saved.timing_open;
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    same=same&&handstream::trial.reach.busy==saved.reach_busy;
#endif
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
    same=same&&vrequip::stow::mutating==saved.stow_mutating&&vrequip::stow::yielding==saved.stow_yielding&&
        vrequip::stow::same_initial_self(vrequip::stow::initial_self,saved.stow_self);
#endif
    return same;
}
inline void prepare(Entry& entry,Source source,uptr object,uptr function,void* params,bool hand_self=false,bool stow_self=false){
    if(entry.address){poison(entry);return;}
    entry.address=reinterpret_cast<std::uintptr_t>(&entry);entry.source=source;
    entry.object=object;entry.function=function;entry.params=params;
    entry.callback=weaponcall::reserve_count(callbacks);
    if(!entry.callback){poison(entry);return;}
    // The existing Gate's linked depth already spans the complete Native CF
    // original gap, including exact returned/unwind. It is a denial marker.
    entry.previous=top;entry.recursive=top!=nullptr||weaponaction::depth!=0;top=&entry;entry.linked=true;
    // Owned callers consume their token BEFORE this snapshot. Never re-arm it
    // by saving its pre-consumption value and writing that value in finally.
    capture(entry);
    entry.seen=weaponaction_handoff::inspect(object,function);
    if(hand_self){entry.seen.hands=false;entry.seen.hands_required=false;}
    if(stow_self){entry.seen.stow=false;entry.seen.stow_required=false;}
    entry.related=entry.seen.hands||entry.seen.stow;
    entry.bare=entry.related||entry.recursive||inside||weaponrestore::active()||!weaponrestore::healthy()||!enabled.load(std::memory_order_acquire);
    if(!entry.related){entry.handback=true;entry.stage_closed=true;entry.prepared=true;return;}
    cancel_post(entry);
    weaponrestore::revoke_publication_no_engine();
    const bool request_open=!entry.recursive&&!weaponrestore::active()&&weaponrestore::healthy()&&entry.seen.gt&&!entry.seen.unknown_name;
    auto disarmed=request_open?weaponrestore::prepare_forward_no_engine(entry.seen,false):weaponrestore::ForwardDisarm{};
    if(!request_open)weaponaction_handoff::revoke_no_engine(entry.seen);
    const auto flags=gears_weapon_boundary::related|(entry.seen.unknown_name?gears_weapon_boundary::unresolved:0u);
    const auto delivery=weaponboundary::begin_transport(entry.transport,source,object,function,flags,request_open&&disarmed.known);
    if(delivery==weaponboundary::Delivery::active){
        auto& backend=entry.backend;backend.request=entry.transport.request;backend.receipt=entry.transport.receipt;
        backend.open_reply=entry.transport.open_reply;backend.seen=entry.seen;backend.disarmed=disarmed;
        backend.lifetime=&entry.transport.entry;backend.transport=&entry.transport;
        backend.stage_address=reinterpret_cast<std::uintptr_t>(&entry.stage);
        if(weaponboundary::claim_stage(entry.transport,backend.stage_address)){
            const auto result=weaponrestore::run(backend,entry.stage);
            entry.stage_closed=(entry.stage.flags&weaponrestore::cleanup_done)||!(entry.stage.flags&weaponrestore::entered);
            entry.handback=result.outcome==weaponrestore::Outcome::complete||result.outcome==weaponrestore::Outcome::exempt;
        }
    }else{
        entry.stage_closed=true;entry.handback=no_engine_handback(entry.seen);
    }
    if(!entry.handback||delivery!=weaponboundary::Delivery::active){
        if(request_open)weaponaction_handoff::revoke_no_engine(entry.seen);
        if(!entry.handback&&(entry.transport.flags&weaponboundary::entry_owned))weaponboundary::transport_notice_unresolved(entry.transport);
    }
    entry.prepared=true;
}
inline void prepare_cpp(Entry& entry,Source source,uptr object,uptr function,void* params,bool hand_self,bool stow_self) noexcept {
    // This barrier surrounds only our pre-action work, never the game body.
    try{prepare(entry,source,object,function,params,hand_self,stow_self);}
    catch(...){poison(entry);}
}
inline void prepare_guarded(Entry& entry,Source source,uptr object,uptr function,void* params,bool hand_self=false,bool stow_self=false) noexcept {
    __try{prepare_cpp(entry,source,object,function,params,hand_self,stow_self);}
    __except(weaponrestore::exception_filter(GetExceptionCode())){poison(entry);}
}
inline void restore_lexical_gt(Entry& entry) noexcept {
        if(entry.gt_snapshot){
            const auto gt=game_thread.load(std::memory_order_acquire);
            if(!gt||GetCurrentThreadId()!=gt)poison(entry);
            else{
                if(!lexical_exact(entry))poison(entry);
                // This existing helper's false mode only restores public
                // lexical POD. It never drains or clears resource journals.
                (void)entry.backend.cleanup_no_engine(entry.lexical,false);
            }
        }
}
inline void restore_lexical_tls(Entry& entry) noexcept {
    if(!entry.lexical_taken)return;
    bool changed=false;
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL) && defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    changed=handstream::tick_boundary_diagnostic::current!=entry.tick||
        handstream::component_trace::observed_authorization!=entry.authorization;
    handstream::tick_boundary_diagnostic::current=entry.tick;
    handstream::component_trace::observed_authorization=entry.authorization;
    if(changed&&entry.gt_snapshot&&GetCurrentThreadId()==game_thread.load(std::memory_order_acquire))
        handstream::tick_boundary_diagnostic::state.disabled=true;
#endif
#if defined(GEARS_PLAYGROUND_HAND_TRANSPORT_LIVENESS_TRIAL)
    changed=changed||handstream::equipment_entry_depth!=entry.equipment_depth;
    handstream::equipment_entry_depth=entry.equipment_depth;
#endif
    inside=entry.lexical.inside;
    if(changed)poison(entry);
}
// No export and no retry. An unknown failure in our own cleanup retains the
// Native cell and quarantines the channel before exact local release. This
// fallback cannot replace the original exception with another cleanup fault.
inline void retain_without_exports(Entry& entry) noexcept {
    entry.fault=true;weaponrestore::poison(entry.backend);weaponboundary::admission.quarantine();
    auto& scope=entry.transport;
    if(!scope.flags)return;
    scope.flags|=weaponboundary::transport_fault;scope.delivery=weaponboundary::Delivery::unresolved;
    if(scope.flags&weaponboundary::cell_owned)(void)weaponboundary::states.isolate(scope.cell);
    if((scope.flags&weaponboundary::entry_owned)&&!(scope.flags&weaponboundary::entry_release_attempted)){
        scope.flags|=weaponboundary::entry_release_attempted;(void)weaponboundary::admission.release(scope.entry);
    }
    scope.flags|=weaponboundary::finish_attempted|weaponboundary::finished;
}
inline void restore_lexical_guarded(Entry& entry) noexcept {
    __try{
        __try{restore_lexical_gt(entry);}
        __finally{restore_lexical_tls(entry);}
    }
    __except(weaponrestore::exception_filter(GetExceptionCode())){retain_without_exports(entry);}
}
inline void retire_guarded(Entry& entry,bool abnormal) noexcept {
    __try{
        gears_weapon_boundary::Finish result{};
        result.outcome=entry.fault?gears_weapon_boundary::Outcome::notification_fault:
            abnormal&&entry.original_started?gears_weapon_boundary::Outcome::original_exception:gears_weapon_boundary::Outcome::returned;
        if(entry.stage_closed)result.flags|=gears_weapon_boundary::stage_closed;
        if(entry.handback&&!entry.fault)result.flags|=gears_weapon_boundary::handback_complete;
        (void)weaponboundary::finish_transport(entry.transport,result);
    }__except(weaponrestore::exception_filter(GetExceptionCode())){retain_without_exports(entry);}
}
inline void finish(Entry& entry,bool abnormal) noexcept {
    if(entry.finished)return;
    if(entry.address!=reinterpret_cast<std::uintptr_t>(&entry)){retain_without_exports(entry);return;}
    if(!entry.prepared||(abnormal&&entry.legacy_body))retain_without_exports(entry);
    __try{restore_lexical_guarded(entry);}
    __finally{
        __try{retire_guarded(entry,abnormal);}
        __finally{
            if(entry.linked){
                if(top!=&entry)retain_without_exports(entry);
                top=entry.previous;entry.linked=false;
            }
            if(entry.callback&&!entry.release_attempted){
                entry.release_attempted=true;if(!weaponaction::release_count(callbacks))retain_without_exports(entry);
            }
            entry.finished=true;
        }
    }
}

// Native owns CF's whole original gap and PG's existing Gate transactions
// retain this client/module. This helper opens no extra scope and performs no
// engine work; its short local Entry protects only the Revoke API tuple.
inline bool revoke_call_body(Entry& notice,uptr object,uptr function,bool& related){
    const auto seen=weaponaction_handoff::inspect(object,function);
    related=seen.hands||seen.stow;if(!related)return true;
    notice.lexical.inside=inside;cancel_post(notice);
    weaponrestore::revoke_publication_no_engine();
    const bool clean=no_engine_handback(seen);weaponaction_handoff::revoke_no_engine(seen);
    const auto flags=gears_weapon_boundary::related|(clean?0u:gears_weapon_boundary::unresolved);
    // The two-value wire uses owned_bridge for this notification only. Actual
    // provenance is the enclosing Gate's linked CF journal; no self proof is
    // consumed here, no scope is opened, and no restoration is authorized.
    (void)weaponboundary::begin_transport(notice.transport,Source::owned_bridge,object,function,flags,false);
    gears_weapon_boundary::Finish result{};result.flags=gears_weapon_boundary::stage_closed|(clean?gears_weapon_boundary::handback_complete:0u);
    const auto delivery=weaponboundary::finish_transport(notice.transport,result);
    return clean&&delivery!=weaponboundary::Delivery::unresolved;
}
inline bool revoke_call_cpp(Entry& notice,uptr object,uptr function,bool& related) noexcept {
    try{return revoke_call_body(notice,object,function,related);}
    catch(...){related=true;retain_without_exports(notice);return false;}
}
inline bool revoke_call(uptr object,uptr function,bool& related) noexcept {
    Entry notice{};
    __try{return revoke_call_cpp(notice,object,function,related);}
    __except(weaponrestore::exception_filter(GetExceptionCode())){
        related=true;retain_without_exports(notice);return false;
    }
}
inline void legacy_original(uptr object,uptr function,void* params){
    if(top){
        if(top->source!=Source::process_event||top->object!=object||top->function!=function||top->params!=params)poison(*top);
        top->post_revision=revocations;top->original_started=true;
    }
    original(object,function,params);
    if(top)top->original_returned=true;
}
inline bool post_allowed() noexcept {
    return top&&top->source==Source::process_event&&!top->recursive&&!top->fault&&top->original_returned&&
        top->post_revision==revocations&&weaponrestore::healthy();
}
inline bool owned_body(Request* request){
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    component_names::Scope names(false);component_cost::InvokePause pause;
#endif
    return weaponrestore::healthy()&&invoke_engine(request);
}
inline bool invoke_owned(Request* request){
    const auto gt=game_thread.load(std::memory_order_acquire);
    if(!weaponrestore::healthy()||!request||!gt||GetCurrentThreadId()!=gt)return false;
    bool hand_self=false,stow_self=false;
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    hand_self=handstream::component::consume_self(request->object,request->function);
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
    stow_self=vrequip::stow::consume_initial_self(request);
#endif
    Entry entry{};bool result=false;
    __try{
        prepare_guarded(entry,Source::owned_bridge,request->object,request->function,request->params,hand_self,stow_self);
        if(entry.prepared&&entry.handback&&!entry.fault){entry.original_started=true;result=owned_body(request);entry.original_returned=true;}
    }__finally{finish(entry,AbnormalTermination()!=FALSE);}
    return result;
}
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
inline bool forward_body(Request* request,ComponentForwardGate& gate){
    component_names::Scope names(false);component_cost::InvokePause pause;
    return weaponrestore::healthy()&&invoke_component_forward_engine(request,gate);
}
inline bool invoke_forward(Request* request,ComponentForwardGate& gate){
    if(!weaponrestore::healthy()||!gate.enter_route(request))return false;
    const auto gt=game_thread.load(std::memory_order_acquire);
    if(!gt||GetCurrentThreadId()!=gt)return false;
    const bool self=handstream::component::consume_self(request->object,request->function);
    Entry entry{};bool result=false;
    __try{
        prepare_guarded(entry,Source::owned_bridge,request->object,request->function,request->params,self,false);
        if(entry.prepared&&entry.handback&&!entry.fault){entry.original_started=true;result=forward_body(request,gate);entry.original_returned=true;}
    }__finally{finish(entry,AbnormalTermination()!=FALSE);}
    return result;
}
#endif
inline void process_event(uptr object,uptr function,void* params){
    Entry entry{};
    __try{
        prepare_guarded(entry,Source::process_event,object,function,params);
        if(entry.bare||!entry.prepared){
            entry.original_started=true;original(object,function,params);entry.original_returned=true;
        }else{entry.legacy_body=true;event_hook_legacy(object,function,params);}
    }__finally{finish(entry,AbnormalTermination()!=FALSE);}
}
} // namespace weaponentry
