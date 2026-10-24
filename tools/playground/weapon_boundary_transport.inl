// Include after the local boundary admission/client definitions. This is only
// export transport and lifetime accounting. It runs no old action helper and
// does not turn an accepted Receipt into a restoration or forward-write grant.
namespace weaponboundary {
enum class Delivery:unsigned {idle,no_scope,active,closed,unresolved};
enum TransportFlags:unsigned {
    started=1,entry_owned=2,revoke_attempted=4,revoke_normal=8,cell_owned=16,
    open_attempted=32,open_normal=64,native_active=128,notice_attempted=256,
    close_attempted=512,finish_attempted=1024,entry_release_attempted=2048,
    finished=4096,transport_fault=8192
};
struct Transport {
    boundaryabi::Admission::Entry entry{};
    boundaryabi::PoolTicket cell{};
    boundaryabi::Request request{};
    boundaryabi::Receipt receipt{}; // Frozen Open receipt, never a global generation.
    std::uintptr_t address{};
    std::uint64_t revoked_generation{};
    unsigned flags{},exception_code{};
    gears_weapon_hand::Reply revoke_reply{gears_weapon_hand::Reply::invalid};
    gears_weapon_hand::Reply open_reply{gears_weapon_hand::Reply::invalid};
    Delivery delivery{Delivery::idle};
};
static_assert(std::is_standard_layout_v<Transport>&&std::is_trivially_copyable_v<Transport>);

inline int transport_filter(unsigned code) noexcept {
    return code==0xe06d7363ul||code==EXCEPTION_STACK_OVERFLOW?EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER;
}
inline void transport_poison(Transport& scope) noexcept {
    scope.flags|=transport_fault;scope.delivery=Delivery::unresolved;
    // Quarantine while the Entry still keeps all API/context/module state alive.
    // Isolated cells are never reset or reused, even if their State looks zero.
    if(scope.flags&cell_owned)(void)states.isolate(scope.cell);
    admission.quarantine();
#if defined(GEARS_PLAYGROUND_RESTORE_STAGE_CORE)
    weaponrestore::sticky_fault.store(true,std::memory_order_release);
#endif
    auto registration=weaponhand::client.registration();
    gears_weapon_hand::Invalidation invalidation{};
    invalidation.registration_cookie=weaponhand::client.cookie();
    invalidation.native_started_qpc=weaponhand::client.native_started_qpc();
    invalidation.reason=gears_weapon_hand::InvalidateReason::callback_fault;
    registration.invalidate(registration.context,&invalidation);
}
inline bool transport_shape(Transport const& scope) noexcept {
    return scope.address==reinterpret_cast<std::uintptr_t>(&scope)&&(scope.flags&started);
}
// A noexcept provider may still raise SEH. Catch only that export's fault;
// original PE/bridge exceptions never pass through one of these barriers.
inline bool transport_revoke(boundaryabi::Revoke call,boundaryabi::Request const& request,
                             std::uint64_t& generation,gears_weapon_hand::Reply& reply,unsigned& code) noexcept {
    __try{reply=call(&request,&generation);return true;}
    __except(transport_filter(GetExceptionCode())){code=GetExceptionCode();return false;}
}
inline bool transport_open(boundaryabi::Open call,boundaryabi::PoolOpenCall const& buffers,
                           gears_weapon_hand::Reply& reply,unsigned& code) noexcept {
    __try{reply=call(buffers.request,buffers.state,buffers.receipt);return true;}
    __except(transport_filter(GetExceptionCode())){code=GetExceptionCode();return false;}
}
inline bool transport_close(boundaryabi::Close call,boundaryabi::PoolCloseCall const& buffers,
                            boundaryabi::End& end,unsigned& code) noexcept {
    __try{end=call(buffers.state,buffers.receipt,buffers.finish);return true;}
    __except(transport_filter(GetExceptionCode())){code=GetExceptionCode();return false;}
}

// Call only AFTER the entry's pure inspection and local forward disarm. A
// caller that cannot run a stage still revokes, with request_open=false. The
// same Transport must remain at this address until finish_transport returns.
inline Delivery begin_transport(Transport& scope,boundaryabi::Source source,std::uintptr_t object,
                                std::uintptr_t function,unsigned flags,bool request_open) noexcept {
    using Reply=gears_weapon_hand::Reply;
    if(scope.flags||scope.address){transport_poison(scope);return scope.delivery;}
    scope.flags=started;scope.address=reinterpret_cast<std::uintptr_t>(&scope);scope.delivery=Delivery::no_scope;
    if(!admission.acquire(scope.entry))return scope.delivery;
    scope.flags|=entry_owned;
    // The closing-bit/count claim precedes EVERY non-atomic Binding/API read.
    const auto binding=admission.view(scope.entry);
    if(!binding){transport_poison(scope);return scope.delivery;}
    scope.request.source=source;scope.request.flags=flags;
    scope.request.hand_cookie=binding->cookie;scope.request.native_started_qpc=binding->native_started_qpc;
    scope.request.context=reinterpret_cast<std::uintptr_t>(binding->context);
    scope.request.object=object;scope.request.function=function;scope.request.thread_id=GetCurrentThreadId();
    if(!boundaryabi::request_valid(scope.request)){transport_poison(scope);return scope.delivery;}
    scope.flags|=revoke_attempted;
    if(!transport_revoke(binding->revoke,scope.request,scope.revoked_generation,scope.revoke_reply,scope.exception_code)){
        transport_poison(scope);return scope.delivery;
    }
    scope.flags|=revoke_normal;
    if(scope.revoke_reply!=Reply::accepted){
        if(!boundaryabi::pool_no_scope_reply(scope.revoke_reply))transport_poison(scope);
        return scope.delivery;
    }
    if(!scope.revoked_generation){transport_poison(scope);return scope.delivery;}
    if(!request_open||(flags&boundaryabi::unresolved))return scope.delivery;
    if(!states.claim(scope.cell,scope.request,scope.revoked_generation))return scope.delivery;
    scope.flags|=cell_owned;
    boundaryabi::PoolOpenCall buffers{};
    if(!states.begin_open(scope.cell,buffers)){transport_poison(scope);return scope.delivery;}
    scope.flags|=open_attempted;
    if(!transport_open(binding->open,buffers,scope.open_reply,scope.exception_code)){
        transport_poison(scope);return scope.delivery;
    }
    scope.flags|=open_normal;
    const auto result=states.open_returned(scope.cell,scope.open_reply);
    if(result==boundaryabi::PoolOpen::no_scope){scope.flags&=~cell_owned;return scope.delivery;}
    if(result!=boundaryabi::PoolOpen::active){transport_poison(scope);return scope.delivery;}
    boundaryabi::PoolDiagnostic diagnostic{};
    if(!states.describe(scope.cell,diagnostic)||!diagnostic.receipt_frozen||
       diagnostic.phase!=boundaryabi::PoolPhase::active||!boundaryabi::pool_same_request(diagnostic.request,scope.request)){
        transport_poison(scope);return scope.delivery;
    }
    scope.receipt=diagnostic.receipt;scope.flags|=native_active;scope.delivery=Delivery::active;return scope.delivery;
}

inline void transport_notice_unresolved(Transport& scope) noexcept {
    if(scope.flags&notice_attempted)return;
    scope.flags|=notice_attempted;
    const auto binding=admission.view(scope.entry);
    if(!binding){transport_poison(scope);return;}
    // A distinct terminal notification, never a retry of an unknown Open or
    // Close. It cannot change the immutable Receipt used by an accepted Close.
    auto request=scope.request;request.flags|=boundaryabi::unresolved;
    if(!boundaryabi::request_valid(request)){transport_poison(scope);return;}
    std::uint64_t generation{};gears_weapon_hand::Reply reply=gears_weapon_hand::Reply::invalid;
    if(!transport_revoke(binding->revoke,request,generation,reply,scope.exception_code)||
       reply!=gears_weapon_hand::Reply::accepted||!generation)transport_poison(scope);
}
inline void transport_release_entry(Transport& scope) noexcept {
    if(!(scope.flags&entry_owned)||(scope.flags&entry_release_attempted))return;
    scope.flags|=entry_release_attempted;
    if(!admission.release(scope.entry))transport_poison(scope);
}
// The caller records the one restoration stage separately and supplies its
// actual Finish. This closes accounting only; no restore, finish helper,
// reflection, logging, engine call or module release occurs here.
inline Delivery finish_transport(Transport& scope,boundaryabi::Finish const& finish) noexcept {
    if(!scope.flags&&!scope.address)return Delivery::idle;
    if(!transport_shape(scope)){transport_poison(scope);return scope.delivery;}
    if(scope.flags&finish_attempted){
        if(!(scope.flags&finished))transport_poison(scope);
        return scope.delivery;
    }
    scope.flags|=finish_attempted;
    __try{
        // An outer original finally may also be reached after an export never
        // returned to its completion bit. A caller's Finish cannot turn that
        // unknown history into a clean NoScope or a known accepted Close.
        if(((scope.flags&entry_owned)&&!(scope.flags&revoke_normal))||
           ((scope.flags&open_attempted)&&!(scope.flags&open_normal))||
           ((scope.flags&cell_owned)&&!(scope.flags&native_active)))transport_poison(scope);
        const bool complete=boundaryabi::pool_finish_shape(finish)&&
            finish.flags==(boundaryabi::stage_closed|boundaryabi::handback_complete)&&
            finish.outcome!=boundaryabi::Outcome::notification_fault&&!(scope.flags&transport_fault);
        if(!complete&&(scope.flags&entry_owned))transport_notice_unresolved(scope);
        if(scope.flags&native_active){
            const auto binding=admission.view(scope.entry);boundaryabi::PoolCloseCall buffers{};
            if(!binding||!states.begin_close(scope.cell,finish,buffers)){transport_poison(scope);}
            else{
                scope.flags|=close_attempted;boundaryabi::End end=boundaryabi::End::unresolved;
                if(!transport_close(binding->close,buffers,end,scope.exception_code))transport_poison(scope);
                else if(states.close_returned(scope.cell,end)!=boundaryabi::PoolClose::recycled)transport_poison(scope);
                else{scope.flags&=~(cell_owned|native_active);scope.delivery=Delivery::closed;}
            }
        }
        if(!complete)transport_poison(scope);
        if(scope.flags&transport_fault)scope.delivery=Delivery::unresolved;
    }__finally{
        if(AbnormalTermination())transport_poison(scope);
        transport_release_entry(scope);scope.flags|=finished;
    }
    return scope.delivery;
}
} // namespace weaponboundary
