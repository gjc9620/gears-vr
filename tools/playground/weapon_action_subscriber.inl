// Private supplement: Native still owns the only CallFunction hook/original.
// STREAM/STOW remain disabled until their independent PE/bridge pre-action
// permission and no-engine handback paths are connected and validated.
namespace weaponaction {
namespace actionabi=gears_weapon_action;
namespace handabi=gears_weapon_hand;
std::atomic<std::uint64_t> attached_cookie{},attached_native{},transactions{};
std::atomic<bool> faulted{};
constexpr unsigned maximum_depth=128;
enum Flags:unsigned {created=1,leased=2,linked=4,ignored=8,begin_done=16,
    observer_attempted=32,observer_done=64,observer_close_attempted=128,observer_closed=256,
    unresolved=512,link_left=1024,transaction_left=2048,callback_left=4096,closed=8192,
    transaction_release_attempted=16384,callback_release_attempted=32768};
constexpr unsigned all_flags=65535;
struct Journal {
    actionabi::Invocation call{};
    handanim::Token observer{};
    unsigned flags{};
    std::uintptr_t prior_state{};
    unsigned depth{},reserved{};
    std::uintptr_t this_state{};
};
static_assert(sizeof(Journal)==sizeof(actionabi::State)&&std::is_trivially_copyable_v<Journal>);
thread_local actionabi::State* top{};
thread_local unsigned depth{};
void store(actionabi::State* state,Journal const& journal) noexcept {std::memcpy(state->storage.data(),&journal,sizeof(journal));}
Journal load(actionabi::State const* state) noexcept {
    Journal journal{};std::memcpy(&journal,state->storage.data(),sizeof(journal));return journal;
}
void poison() noexcept {
    faulted.store(true,std::memory_order_release);
    auto registration=weaponhand::client.registration();handabi::Invalidation message{};
    message.registration_cookie=weaponhand::client.cookie();message.native_started_qpc=weaponhand::client.native_started_qpc();
    message.reason=handabi::InvalidateReason::callback_fault;registration.invalidate(registration.context,&message);
}
template<class T>bool release_count(std::atomic<T>& count) noexcept {
    auto value=count.load(std::memory_order_acquire);
    do{if(!value)return false;}
    while(!count.compare_exchange_weak(value,value-1,std::memory_order_acq_rel,std::memory_order_acquire));
    return true;
}
bool ready() noexcept {
    const auto cookie=attached_cookie.load(std::memory_order_acquire),native=attached_native.load(std::memory_order_acquire);
    return !faulted.load(std::memory_order_acquire)&&weaponcall::ready()&&
        weaponhand::client.registration_alive(cookie,native)&&attached_cookie.load(std::memory_order_acquire)==cookie;
}
bool local_quiescent() noexcept {return !faulted.load(std::memory_order_acquire)&&!transactions.load(std::memory_order_acquire);}
bool detached() noexcept {
    if(!local_quiescent())return false;
    attached_cookie.store(0,std::memory_order_release);attached_native.store(0,std::memory_order_release);return true;
}
bool matches(actionabi::Invocation const& call) noexcept {
    return ready()&&call.hand_cookie==attached_cookie.load(std::memory_order_acquire)&&
        call.native_started_qpc==attached_native.load(std::memory_order_acquire);
}
void mark_unresolved(actionabi::State* state) noexcept {
    if(state){auto journal=load(state);journal.flags|=unresolved;store(state,journal);}
    poison();
}
actionabi::BeginReply begin_body(void* context,actionabi::Invocation const* call,actionabi::State* state){
    if(!call||!state){poison();return actionabi::BeginReply::handback_unresolved;}
    for(auto value:state->storage)if(value!=std::byte{}){poison();return actionabi::BeginReply::handback_unresolved;}
    Journal journal{};journal.call=*call;journal.flags=created;journal.this_state=reinterpret_cast<std::uintptr_t>(state);store(state,journal);
    if(context!=&weaponhand::client||!actionabi::invocation_valid(*call)||call->thread_id!=GetCurrentThreadId()){
        mark_unresolved(state);return actionabi::BeginReply::handback_unresolved;
    }
    if(!weaponcall::reserve_count(callbacks)){mark_unresolved(state);return actionabi::BeginReply::handback_unresolved;}
    if(!weaponcall::reserve_count(transactions)){
        if(!release_count(callbacks))poison();mark_unresolved(state);return actionabi::BeginReply::handback_unresolved;
    }
    journal.flags|=leased;store(state,journal);
    if(depth>=maximum_depth){mark_unresolved(state);return actionabi::BeginReply::handback_unresolved;}
    journal.prior_state=reinterpret_cast<std::uintptr_t>(top);journal.depth=depth+1;journal.flags|=linked;
    store(state,journal);top=state;++depth;
    if(!enabled.load(std::memory_order_acquire)||!weaponhand::current_gt(nullptr)||!matches(*call)){
        journal.flags|=ignored|begin_done;store(state,journal);return actionabi::BeginReply::unrelated;
    }
    if((call->flags&actionabi::engine_calls_allowed)&&journal.depth!=1){
        mark_unresolved(state);return actionabi::BeginReply::handback_unresolved;
    }
    // No stream/stow writer is enabled in this protocol candidate. Neither
    // an outer permission flag nor an observer token authorizes one here.
    // Optional observation is the existing read-only path, independent of V1.
    journal.flags|=observer_attempted;store(state,journal);
    journal.observer=handanim::enter(call->self,call->callee,nullptr,handanim::Source::call_function,call->caller_frame);
    journal.flags|=observer_done|begin_done;store(state,journal);
    return actionabi::BeginReply::unrelated;
}
actionabi::BeginReply __cdecl begin(void* context,actionabi::Invocation const* call,actionabi::State* state) noexcept {
    // This catch surrounds this callback's C++ work only. The actual game
    // original is invoked later by Native, entirely outside this catch.
    try{return begin_body(context,call,state);}
    catch(...){mark_unresolved(state);return actionabi::BeginReply::handback_unresolved;}
}
bool cleanup(Journal& journal,actionabi::State* state) noexcept {
    bool clean=(journal.flags&begin_done)&&!(journal.flags&unresolved);
    if((journal.flags&observer_attempted)&&!(journal.flags&observer_done))clean=false;
    if(journal.observer&&!(journal.flags&observer_closed))clean=false;
    if((journal.flags&linked)&&!(journal.flags&link_left)){
        if(top==state&&depth==journal.depth){
            top=reinterpret_cast<actionabi::State*>(journal.prior_state);--depth;
            journal.flags|=link_left;store(state,journal);
        }else clean=false;
    }
    // A precise local lease can close while independent sticky fault/debt
    // retains residency. Never leave a pointer to Native's expiring State.
    if((journal.flags&leased)&&(!(journal.flags&linked)||(journal.flags&link_left))){
        if(!(journal.flags&transaction_release_attempted)){
            journal.flags|=transaction_release_attempted;store(state,journal);
            if(release_count(transactions))journal.flags|=transaction_left;else clean=false;
            store(state,journal);
        }
        if(!(journal.flags&callback_release_attempted)){
            journal.flags|=callback_release_attempted;store(state,journal);
            if(release_count(callbacks))journal.flags|=callback_left;else clean=false;
            store(state,journal);
        }
    }else clean=false;
    if(!(journal.flags&transaction_left)||!(journal.flags&callback_left))clean=false;
    if(clean){journal.flags|=closed;store(state,journal);}
    else{journal.flags|=unresolved;store(state,journal);poison();}
    return clean;
}
actionabi::End finish_body(void* context,actionabi::Invocation const* call,actionabi::State* state,bool normal,bool failed){
    if(!call||!state||context!=&weaponhand::client){poison();return actionabi::End::unresolved;}
    auto journal=load(state);
    if(!actionabi::invocation_valid(*call)||!actionabi::same(journal.call,*call)||call->thread_id!=GetCurrentThreadId()||
       journal.this_state!=reinterpret_cast<std::uintptr_t>(state)||!(journal.flags&created)||journal.reserved||
       (journal.flags&~all_flags)){poison();return actionabi::End::unresolved;}
    if(journal.flags&closed)return actionabi::End::closed;
    if((journal.flags&linked)&&!(journal.flags&link_left)&&(top!=state||depth!=journal.depth)){
        mark_unresolved(state);return actionabi::End::unresolved;
    }
    if(failed)journal.flags|=unresolved;
    bool clean{};
    __try{
        if(journal.observer&&!(journal.flags&observer_closed)&&!(journal.flags&observer_close_attempted)){
            journal.flags|=observer_close_attempted;store(state,journal);
            bool observer_was_closed{};
            if(normal)observer_was_closed=handanim::leave(journal.observer,call->self,call->callee,nullptr,call->caller_frame);
            // On unwind only close the recorded ledger entry: no successful
            // post-animation snapshot, engine call or restoration is emitted.
            else observer_was_closed=handanim::probe.ledger.leave(journal.observer);
            observer_was_closed=observer_was_closed&&handanim::probe.ledger.depth()==journal.observer.depth;
            if(observer_was_closed)journal.flags|=observer_closed;else journal.flags|=unresolved;
            store(state,journal);
        }
    }__finally{clean=cleanup(journal,state);}
    return clean?actionabi::End::closed:actionabi::End::unresolved;
}
actionabi::End finish(void* context,actionabi::Invocation const* call,actionabi::State* state,bool normal,bool failed) noexcept {
    try{return finish_body(context,call,state,normal,failed);}
    catch(...){mark_unresolved(state);return actionabi::End::unresolved;}
}
actionabi::End __cdecl returned(void* context,actionabi::Invocation const* call,actionabi::State* state,std::uint64_t) noexcept {
    return finish(context,call,state,true,false);
}
actionabi::End __cdecl unwind(void* context,actionabi::Invocation const* call,actionabi::State* state,actionabi::UnwindReason reason) noexcept {
    const bool failed=reason!=actionabi::UnwindReason::original_exception;
    if(failed)poison();return finish(context,call,state,false,failed);
}
actionabi::Registration registration() noexcept {
    actionabi::Registration value{};value.hand_cookie=weaponhand::client.cookie();value.native_started_qpc=weaponhand::client.native_started_qpc();
    value.context=&weaponhand::client;value.begin=begin;value.returned=returned;value.unwind=unwind;return value;
}
bool call_attach(actionabi::Attach function,actionabi::Registration const* candidate,handabi::RegistrationResult* receipt,handabi::Reply& reply) noexcept {
    __try{reply=function(candidate,receipt);return true;}
    __except(GetExceptionCode()==0xe06d7363ul||GetExceptionCode()==EXCEPTION_STACK_OVERFLOW?
        EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER){return false;}
}
handabi::Reply attach(void* image) noexcept {
    const auto native=static_cast<HMODULE>(image);
    if(!native||native!=weaponhand::native_module||!weaponcall::ready()||faulted.load(std::memory_order_acquire)||
       transactions.load(std::memory_order_acquire)||attached_cookie.load(std::memory_order_acquire))return handabi::Reply::busy;
    const auto address=GetProcAddress(native,actionabi::attach_export);
    if(!weaponhand::executable(address,native))return handabi::Reply::unavailable;
    auto candidate=registration();handabi::RegistrationResult receipt{};auto reply=handabi::Reply::invalid;
    const auto completed=call_attach(reinterpret_cast<actionabi::Attach>(address),&candidate,&receipt,reply);
    if(!completed||(reply==handabi::Reply::accepted&&(receipt.size!=sizeof(receipt)||receipt.abi_version!=handabi::version||
       receipt.cookie!=candidate.hand_cookie||receipt.native_started_qpc!=candidate.native_started_qpc))){poison();return handabi::Reply::invalid;}
    if(reply!=handabi::Reply::accepted)return reply;
    if(!weaponhand::client.registration_alive(candidate.hand_cookie,candidate.native_started_qpc)||!weaponcall::ready()){
        poison();return handabi::Reply::unavailable;
    }
    attached_native.store(candidate.native_started_qpc,std::memory_order_release);
    attached_cookie.store(candidate.hand_cookie,std::memory_order_release);return reply;
}
} // namespace weaponaction
