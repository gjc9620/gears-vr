// Actual PG hand/call/Gate code. Native delivery and optional observer are
// local fixtures, not a provider, engine, lock, restoration or runtime proof.
#include "../vr-common/weapon_shot_scope.h"
#include "../vr-common/hand_animation_observation.h"
#include "weapon_hand_offline_fixture.h"
#include <stdexcept>
namespace {
std::atomic<unsigned> callbacks{};
#include "weapon_shot_scope.inl"
#include "weapon_call_subscriber.inl"
constexpr DWORD fault_code=0xe1230042ul;
namespace handanim {
using Token=gears_hand_animation::Token;
using Source=gears_hand_animation::Source;
struct Probe {gears_hand_animation::Ledger ledger;} probe;
enum class Fault {none,begin_cpp_before,begin_cpp_after,begin_seh_after,end_cpp_before,end_seh_after,end_silent_mismatch};
Fault fault{};unsigned posts{};
Token enter(uptr,uptr,void*,Source source,uptr){
    if(fault==Fault::begin_cpp_before)throw std::runtime_error("observer begin");
    auto token=probe.ledger.enter(gears_hand_animation::Kind::slot_play,source);
    if(fault==Fault::begin_cpp_after)throw std::runtime_error("observer partial begin");
    if(fault==Fault::begin_seh_after)RaiseException(fault_code,0,0,nullptr);
    return token;
}
bool leave(Token const& token,uptr,uptr,void*,uptr){
    ++posts;
    if(fault==Fault::end_cpp_before)throw std::runtime_error("observer end");
    if(fault==Fault::end_silent_mismatch){auto wrong=token;++wrong.call;return probe.ledger.leave(wrong);}
    const bool token_closed=probe.ledger.leave(token);
    if(fault==Fault::end_seh_after)RaiseException(fault_code,0,0,nullptr);
    return token_closed;
}
}
#include "weapon_action_subscriber.inl"
namespace gate=gears_weapon_action;
void connect(){
    game_thread.store(0);
    assert(weaponhand::client.begin_connect({11,GetCurrentProcessId()})==abi::Reply::accepted);
    abi::RegistrationResult receipt{};receipt.cookie=33;receipt.native_started_qpc=22;
    assert(weaponhand::client.connected(abi::Reply::accepted,receipt)==abi::Reply::accepted);
    game_thread.store(GetCurrentThreadId());
    weaponcall::attached_native.store(22);weaponcall::attached_cookie.store(33);
    weaponaction::attached_native.store(22);weaponaction::attached_cookie.store(33);
}
gate::Invocation invocation(std::uint64_t id){
    gate::Invocation call{};call.hand_cookie=33;call.native_started_qpc=22;call.gate_id=id;
    call.thread_id=GetCurrentThreadId();call.self=0x111;call.caller_frame=0x222;call.result=0x333;call.callee=0x444;return call;
}
bool guarded_begin(gate::Invocation const* call,gate::State* state,gate::BeginReply& reply){
    __try{reply=weaponaction::begin(&weaponhand::client,call,state);return true;}
    __except(GetExceptionCode()==fault_code?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return false;}
}
bool guarded_end(gate::Invocation const* call,gate::State* state,gate::End& reply){
    __try{reply=weaponaction::returned(&weaponhand::client,call,state,0xabcdef);return true;}
    __except(GetExceptionCode()==fault_code?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return false;}
}
void debt(){
    assert(weaponaction::faulted.load()&&!weaponaction::local_quiescent()&&!weaponaction::detached());
    assert(!weaponhand::client.registration_alive(33,22));
}
void exact_local_close(){assert(!callbacks.load()&&!weaponaction::transactions.load()&&!weaponaction::top&&!weaponaction::depth);}
}
int main(int argc,char** argv){
    assert(argc==2);const std::string mode=argv[1];
    // Fault fixture objects remain valid until process exit. No fabricated
    // Native quiescence receipt releases an unresolved module/context.
    auto* memory=new Memory;connect();auto call=invocation(1);gate::State state{};
    gate::BeginReply begun=gate::BeginReply::handback_unresolved;gate::End ended=gate::End::unresolved;
    if(mode=="nested_stop"){
        call.flags=gate::engine_calls_allowed;
        assert(guarded_begin(&call,&state,begun)&&begun==gate::BeginReply::unrelated);
        assert(callbacks.load()==1&&weaponaction::transactions.load()==1&&handanim::probe.ledger.depth()==1);
        assert(!weaponaction::local_quiescent()&&!weaponaction::detached()); // Original gap still leased.
        auto inner=invocation(2);gate::State inner_state{};
        assert(guarded_begin(&inner,&inner_state,begun)&&weaponaction::depth==2);
        assert(weaponaction::returned(&weaponhand::client,&inner,&inner_state,9)==gate::End::closed);
        assert(weaponaction::depth==1&&callbacks.load()==1&&handanim::posts==1);
        game_thread.store(0);assert(weaponhand::client.begin_disconnect()==abi::Reply::draining);game_thread.store(GetCurrentThreadId());
        assert(weaponaction::unwind(&weaponhand::client,&call,&state,gate::UnwindReason::original_exception)==gate::End::closed);
        assert(handanim::posts==1&&!handanim::probe.ledger.depth());exact_local_close();
        assert(weaponaction::returned(&weaponhand::client,&call,&state,0)==gate::End::closed);exact_local_close();
        assert(weaponaction::local_quiescent()&&weaponaction::detached());
        game_thread.store(0);assert(weaponhand::client.finish_disconnect(33)==abi::Reply::quiescent);delete memory;
    }else if(mode=="foreign_thread"){
        game_thread.store(0);assert(guarded_begin(&call,&state,begun)&&begun==gate::BeginReply::unrelated);
        assert(!handanim::probe.ledger.calls());assert(guarded_end(&call,&state,ended)&&ended==gate::End::closed);
        exact_local_close();assert(weaponaction::local_quiescent());
    }else if(mode=="changed_invocation"){
        assert(guarded_begin(&call,&state,begun));auto wrong=call;++wrong.result;
        assert(weaponaction::returned(&weaponhand::client,&wrong,&state,0)==gate::End::unresolved);
        assert(callbacks.load()==1&&weaponaction::top==&state);debt();
        assert(weaponaction::unwind(&weaponhand::client,&call,&state,gate::UnwindReason::notification_fault)==gate::End::unresolved);
        exact_local_close();debt();
    }else if(mode=="nested_permission"){
        assert(guarded_begin(&call,&state,begun));auto inner=invocation(2);inner.flags=gate::engine_calls_allowed;gate::State inner_state{};
        assert(guarded_begin(&inner,&inner_state,begun)&&begun==gate::BeginReply::handback_unresolved);
        assert(weaponaction::unwind(&weaponhand::client,&inner,&inner_state,gate::UnwindReason::notification_fault)==gate::End::unresolved);
        assert(weaponaction::depth==1&&handanim::probe.ledger.depth()==1);
        assert(weaponaction::unwind(&weaponhand::client,&call,&state,gate::UnwindReason::original_exception)==gate::End::closed);
        exact_local_close();debt();
    }else if(mode=="wrong_end_order"){
        assert(guarded_begin(&call,&state,begun));auto inner=invocation(2);gate::State inner_state{};
        assert(guarded_begin(&inner,&inner_state,begun));
        assert(weaponaction::returned(&weaponhand::client,&call,&state,0)==gate::End::unresolved);
        assert(!handanim::posts&&handanim::probe.ledger.depth()==2&&weaponaction::depth==2);
        assert(weaponaction::unwind(&weaponhand::client,&inner,&inner_state,gate::UnwindReason::original_exception)==gate::End::closed);
        assert(weaponaction::unwind(&weaponhand::client,&call,&state,gate::UnwindReason::notification_fault)==gate::End::unresolved);
        exact_local_close();debt();
    }else if(mode=="capacity_overflow"){
        assert(guarded_begin(&call,&state,begun));
        for(unsigned i=1;i<gears_hand_animation::max_calls;++i){
            const auto token=handanim::probe.ledger.enter(gears_hand_animation::Kind::slot_play,handanim::Source::call_function);
            assert(token&&handanim::probe.ledger.leave(token));
        }
        assert(!handanim::probe.ledger.enter(gears_hand_animation::Kind::slot_play,handanim::Source::call_function));
        assert(handanim::probe.ledger.overflow()&&handanim::probe.ledger.depth()==1);
        assert(guarded_end(&call,&state,ended)&&ended==gate::End::closed);
        exact_local_close();assert(weaponaction::local_quiescent()&&!weaponaction::faulted.load());
    }else if(mode=="callback_underflow"||mode=="transaction_underflow"){
        assert(guarded_begin(&call,&state,begun));
        if(mode=="callback_underflow")callbacks.store(0);else weaponaction::transactions.store(0);
        assert(guarded_end(&call,&state,ended)&&ended==gate::End::unresolved);debt();
        callbacks.store(1);weaponaction::transactions.store(1); // A later, distinct lease must not be stolen.
        assert(weaponaction::unwind(&weaponhand::client,&call,&state,gate::UnwindReason::notification_fault)==gate::End::unresolved);
        assert(callbacks.load()==1&&weaponaction::transactions.load()==1);debt();
    }else if(mode=="begin_cpp_before"||mode=="begin_cpp_after"||mode=="begin_seh_after"){
        handanim::fault=mode=="begin_cpp_before"?handanim::Fault::begin_cpp_before:
            mode=="begin_cpp_after"?handanim::Fault::begin_cpp_after:handanim::Fault::begin_seh_after;
        const bool returned=guarded_begin(&call,&state,begun);
        if(mode=="begin_seh_after")assert(!returned);else assert(returned&&begun==gate::BeginReply::handback_unresolved);
        assert(weaponaction::unwind(&weaponhand::client,&call,&state,gate::UnwindReason::notification_fault)==gate::End::unresolved);
        exact_local_close();debt();
        assert(handanim::probe.ledger.depth()==(mode=="begin_cpp_before"?0u:1u));
    }else if(mode=="end_cpp_before"||mode=="end_seh_after"||mode=="end_silent_mismatch"){
        assert(guarded_begin(&call,&state,begun));
        handanim::fault=mode=="end_cpp_before"?handanim::Fault::end_cpp_before:
            mode=="end_seh_after"?handanim::Fault::end_seh_after:handanim::Fault::end_silent_mismatch;
        const bool returned=guarded_end(&call,&state,ended);
        if(mode=="end_seh_after")assert(!returned);else assert(returned&&ended==gate::End::unresolved);
        assert(weaponaction::unwind(&weaponhand::client,&call,&state,gate::UnwindReason::notification_fault)==gate::End::unresolved);
        exact_local_close();debt();assert(handanim::posts==1); // Unknown post is never retried.
    }else{assert(false);}
    std::printf("WeaponActionSubscriberProtocolTest %s passed; actual PG, observer/local delivery fixtures only\n",argv[1]);
}
