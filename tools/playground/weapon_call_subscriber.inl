// Private fire/result subscriber. Native owns CallFunction and original. This
// initial channel deliberately excludes animation/stream/stow handoff code.
namespace weaponcall {
namespace callabi=gears_weapon_call;
namespace handabi=gears_weapon_hand;
std::atomic<std::uint64_t> attached_cookie{},attached_native{},transactions{};
std::atomic<bool> faulted{};
enum Flags:unsigned {created=1,leased=2,begin_done=4,ignored=8,raw_left=16,
    writer_done=32,scope_left=64,closed=128,unified_uncertain=256};
#if defined(GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE)
constexpr unsigned journal_valid_flags=511u;
#else
constexpr unsigned journal_valid_flags=255u;
#endif
struct Journal {
    callabi::Invocation call{};
    gears_weapon_shot::Token scope{};
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_RESULT_PROBE)
    gripdamage::Token raw{};
#else
    std::array<unsigned,4> raw{};
#endif
    unsigned flags{},reserved{};
};
static_assert(sizeof(Journal)<=sizeof(callabi::State)&&std::is_trivially_copyable_v<Journal>);
void store(callabi::State* state,Journal const& journal) noexcept {std::memcpy(state->storage.data(),&journal,sizeof(journal));}
Journal load(callabi::State const* state) noexcept {
    Journal journal{};std::memcpy(&journal,state->storage.data(),sizeof(journal));return journal;
}
void poison() noexcept {
    faulted.store(true,std::memory_order_release);
    auto registration=weaponhand::client.registration();handabi::Invalidation message{};
    message.registration_cookie=weaponhand::client.cookie();message.native_started_qpc=weaponhand::client.native_started_qpc();
    message.reason=handabi::InvalidateReason::callback_fault;registration.invalidate(registration.context,&message);
}
template<class T>bool reserve_count(std::atomic<T>& count) noexcept {
    auto previous=count.load(std::memory_order_acquire);
    do{if(previous==(std::numeric_limits<T>::max)())return false;}
    while(!count.compare_exchange_weak(previous,previous+1,std::memory_order_acq_rel,std::memory_order_acquire));
    return true;
}
bool ready() noexcept {
    const auto cookie=attached_cookie.load(std::memory_order_acquire),native=attached_native.load(std::memory_order_acquire);
    return !faulted.load(std::memory_order_acquire)&&weaponhand::client.registration_alive(cookie,native)&&
        attached_cookie.load(std::memory_order_acquire)==cookie;
}
bool call_matches(callabi::Invocation const& call) noexcept {
    return ready()&&call.hand_cookie==attached_cookie.load(std::memory_order_acquire)&&
        call.native_started_qpc==attached_native.load(std::memory_order_acquire);
}
bool local_quiescent() noexcept {return !faulted.load(std::memory_order_acquire)&&!transactions.load(std::memory_order_acquire);}
bool detached() noexcept {
    if(!local_quiescent())return false;
    attached_cookie.store(0,std::memory_order_release);attached_native.store(0,std::memory_order_release);return true;
}
void __cdecl begin(void* context,callabi::Invocation const* call,callabi::State* state) noexcept {
    if(!call||!state){poison();return;}
    for(auto value:state->storage)if(value!=std::byte{}){poison();return;}
    Journal journal{};journal.call=*call;journal.flags=created;store(state,journal);
    if(context!=&weaponhand::client||!callabi::invocation_valid(*call)||call->thread_id!=GetCurrentThreadId()){
        poison();return;
    }
    if(!reserve_count(callbacks)){poison();return;}
    if(!reserve_count(transactions)){--callbacks;poison();return;}
    journal.flags|=leased;store(state,journal);
    // Before attach acknowledgement or on another thread there is no old
    // authorized scope to borrow. Terminal stop blocks writes independently.
    if(!enabled.load(std::memory_order_acquire)||!weaponhand::current_gt(nullptr)||!call_matches(*call)){
        journal.flags|=ignored|begin_done;store(state,journal);return;
    }
    // Do not gate this on armed(): soft loss must still mask unknown nesting.
    journal.scope=weaponshot::enter(call->self,call->caller_frame,call->callee);store(state,journal);
#if defined(GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE)
    unified_weapon_fire::begin(*call,journal.scope);
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_RESULT_PROBE)
    journal.raw=gripdamage::enter(call->self,call->caller_frame,call->result,call->callee);store(state,journal);
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_WRITE_TRIAL)
    gripdamage_write::enter(journal.raw);
#endif
#endif
    journal.flags|=begin_done;store(state,journal);
}
bool raw_closed(Journal const& journal) noexcept {
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_RESULT_PROBE)
    const auto& token=journal.raw;if(!token)return true;
    const auto& state=gripdamage::state;
    return token.generation==state.generation&&token.row<state.count&&
        state.rows[token.row].token.call==token.call&&state.rows[token.row].closed&&state.depth==token.depth;
#else
    (void)journal;return true;
#endif
}
bool cleanup(Journal& journal,callabi::State* state) noexcept {
    bool clean=(journal.flags&begin_done)!=0&&!(journal.flags&unified_uncertain);
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_RESULT_PROBE)
    if(!(journal.flags&raw_left)){
        if(!raw_closed(journal))gripdamage::leave(journal.raw,false);
        if(raw_closed(journal))journal.flags|=raw_left;else clean=false;
        store(state,journal);
    }
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_WRITE_TRIAL)
    if(!(journal.flags&writer_done)){gripdamage_write::abandon(journal.raw);journal.flags|=writer_done;store(state,journal);}
#endif
    if(!raw_closed(journal))clean=false;
#endif
    if(!(journal.flags&scope_left)){
#if defined(GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE)
        if(!unified_weapon_fire::cleanup(journal.call,journal.scope)){
            journal.flags|=unified_uncertain;store(state,journal);clean=false;
        }
#endif
        weaponshot::leave(journal.scope);
        if(!weaponshot::stack.poisoned()&&(!journal.scope.entered||weaponshot::stack.depth()==journal.scope.previous_depth))
            journal.flags|=scope_left;
        else clean=false;
        store(state,journal);
    }
    if(clean&&(journal.flags&leased)){
        journal.flags|=closed;store(state,journal);--transactions;--callbacks;
    }else{clean=false;poison();}
    return clean;
}
callabi::End finish(void* context,callabi::Invocation const* call,callabi::State* state,bool normal) noexcept {
    if(!call||!state||context!=&weaponhand::client){poison();return callabi::End::unresolved;}
    auto journal=load(state);
    if(!callabi::invocation_valid(*call)||!callabi::same(journal.call,*call)||call->thread_id!=GetCurrentThreadId()||
       !(journal.flags&created)||journal.reserved||(journal.flags&~journal_valid_flags)){poison();return callabi::End::unresolved;}
    if(journal.flags&closed)return callabi::End::closed; // Retry never repeats a write or decrements a lease.
    bool clean{};
    __try{
        if(normal&&(journal.flags&begin_done)&&!(journal.flags&ignored)){
#if defined(GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE)
            if(!unified_weapon_fire::normal_return(*call)){
                journal.flags|=unified_uncertain;store(state,journal);poison();
            }
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_RESULT_PROBE)
            gripdamage::leave(journal.raw,true);
            if(raw_closed(journal))journal.flags|=raw_left;else poison();
            store(state,journal);
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_WRITE_TRIAL)
            if(journal.flags&raw_left){gripdamage_write::complete(journal.raw);journal.flags|=writer_done;store(state,journal);}
#endif
#endif
        }
    }__finally{clean=cleanup(journal,state);}
    return clean?callabi::End::closed:callabi::End::unresolved;
}
callabi::End __cdecl returned(void* context,callabi::Invocation const* call,callabi::State* state,std::uint64_t) noexcept {
    return finish(context,call,state,true);
}
callabi::End __cdecl unwind(void* context,callabi::Invocation const* call,callabi::State* state,callabi::UnwindReason reason) noexcept {
    if(reason!=callabi::UnwindReason::original_exception&&reason!=callabi::UnwindReason::notification_fault){poison();return callabi::End::unresolved;}
    if(reason==callabi::UnwindReason::notification_fault)poison();
    return finish(context,call,state,false);
}
callabi::Registration registration() noexcept {
    callabi::Registration registration{};registration.hand_cookie=weaponhand::client.cookie();
    registration.native_started_qpc=weaponhand::client.native_started_qpc();registration.context=&weaponhand::client;
    registration.begin=begin;registration.returned=returned;registration.unwind=unwind;return registration;
}
bool call_attach(callabi::Attach fn,callabi::Registration const* registration,handabi::RegistrationResult* receipt,handabi::Reply& reply) noexcept {
    __try{reply=fn(registration,receipt);return true;}
    __except(GetExceptionCode()==0xe06d7363ul||GetExceptionCode()==EXCEPTION_STACK_OVERFLOW?
        EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER){return false;}
}
handabi::Reply attach(void* image) noexcept {
    const auto native=static_cast<HMODULE>(image);
    if(!native||native!=weaponhand::native_module||faulted.load(std::memory_order_acquire)||
       transactions.load(std::memory_order_acquire)||attached_cookie.load(std::memory_order_acquire))return handabi::Reply::busy;
    const auto address=GetProcAddress(native,callabi::attach_export);
    if(!weaponhand::executable(address,native))return handabi::Reply::unavailable;
    auto candidate=registration();handabi::RegistrationResult receipt{};auto reply=handabi::Reply::invalid;
    const auto completed=call_attach(reinterpret_cast<callabi::Attach>(address),&candidate,&receipt,reply);
    if(!completed||(reply==handabi::Reply::accepted&&(receipt.size!=sizeof(receipt)||receipt.abi_version!=handabi::version||
       receipt.cookie!=candidate.hand_cookie||receipt.native_started_qpc!=candidate.native_started_qpc))){poison();return handabi::Reply::invalid;}
    if(reply!=handabi::Reply::accepted)return reply;
    if(!weaponhand::client.registration_alive(candidate.hand_cookie,candidate.native_started_qpc)){poison();return handabi::Reply::unavailable;}
    attached_native.store(candidate.native_started_qpc,std::memory_order_release);
    attached_cookie.store(candidate.hand_cookie,std::memory_order_release);return reply;
}
} // namespace weaponcall
