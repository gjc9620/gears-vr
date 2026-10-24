// Production PG subscriber with Raw/writer OFF and an explicit local Host.
// Fault cases run in separate processes and retain their unresolved storage
// until process exit; they never manufacture a quiescent Native receipt.
#include "../vr-common/weapon_shot_scope.h"
#include "weapon_hand_offline_fixture.h"
namespace {
std::atomic<unsigned> callbacks{};
#include "weapon_shot_scope.inl"
#include "weapon_call_subscriber.inl"
constexpr DWORD fault_code=0xe1230001ul;
bool guarded_begin(gears_weapon_call::Invocation const* call,gears_weapon_call::State* state){
    __try{weaponcall::begin(&weaponhand::client,call,state);return true;}
    __except(GetExceptionCode()==fault_code?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return false;}
}
void connect(){
    game_thread.store(0);assert(weaponhand::client.begin_connect({11,GetCurrentProcessId()})==abi::Reply::accepted);
    abi::RegistrationResult receipt{};receipt.cookie=33;receipt.native_started_qpc=22;
    assert(weaponhand::client.connected(abi::Reply::accepted,receipt)==abi::Reply::accepted);
    game_thread.store(GetCurrentThreadId());weaponcall::attached_native.store(22);weaponcall::attached_cookie.store(33);
}
gears_weapon_call::Invocation invocation(std::uint64_t id){
    gears_weapon_call::Invocation call{};call.hand_cookie=33;call.native_started_qpc=22;call.call_id=id;
    call.thread_id=GetCurrentThreadId();return call; // Opaque/null callee creates a NoAuth mask.
}
}
int main(int argc,char** argv){
    assert(argc==2);const std::string mode=argv[1];
    // On fault paths storage deliberately has process lifetime, as Native's
    // sticky debt must never permit the tested callback context to be freed.
    auto* memory=new Memory;connect();
    auto call=invocation(1);gears_weapon_call::State state{};
    if(mode=="normal"){
        assert(guarded_begin(&call,&state)&&callbacks.load()==1&&weaponcall::transactions.load()==1);
        assert(weaponshot::stack.depth()==1); // Raw OFF still masks unknown mode.
        auto registration=weaponhand::client.registration();abi::Invalidation lost{};
        lost.registration_cookie=33;lost.native_started_qpc=22;lost.reason=abi::InvalidateReason::input_lost;
        registration.invalidate(registration.context,&lost);
        auto nested=invocation(2);gears_weapon_call::State nested_state{};
        assert(guarded_begin(&nested,&nested_state)&&weaponshot::stack.depth()==2);
        assert(weaponcall::returned(&weaponhand::client,&nested,&nested_state,0xabcdef)==gears_weapon_call::End::closed);
        assert(weaponshot::stack.depth()==1&&callbacks.load()==1);
        assert(weaponcall::unwind(&weaponhand::client,&call,&state,gears_weapon_call::UnwindReason::original_exception)==gears_weapon_call::End::closed);
        assert(!callbacks.load()&&weaponshot::stack.drained()&&weaponcall::local_quiescent());
        assert(weaponcall::returned(&weaponhand::client,&call,&state,0)==gears_weapon_call::End::closed&&!callbacks.load());
        assert(weaponcall::detached());game_thread.store(0);
        assert(weaponhand::client.begin_disconnect()==abi::Reply::draining);
        assert(weaponhand::client.finish_disconnect(33)==abi::Reply::quiescent);delete memory;
    }else if(mode=="partial_begin_fault"){
        mutate_read=base+0x1c24730;mutation=[]{RaiseException(fault_code,0,0,nullptr);};
        assert(!guarded_begin(&call,&state));
        assert(callbacks.load()==1&&weaponcall::transactions.load()==1);
        assert(weaponcall::unwind(&weaponhand::client,&call,&state,gears_weapon_call::UnwindReason::notification_fault)==gears_weapon_call::End::unresolved);
        assert(!weaponcall::local_quiescent()&&!weaponcall::detached()&&callbacks.load()==1);
        assert(!weaponhand::client.registration_alive(33,22));
    }else if(mode=="notice_fault_debt"){
        assert(guarded_begin(&call,&state));
        assert(weaponcall::unwind(&weaponhand::client,&call,&state,gears_weapon_call::UnwindReason::notification_fault)==gears_weapon_call::End::closed);
        assert(!callbacks.load()&&weaponshot::stack.drained());
        assert(!weaponcall::local_quiescent()&&!weaponcall::detached()); // Counts zero alone never clears debt.
        assert(weaponcall::returned(&weaponhand::client,&call,&state,0)==gears_weapon_call::End::closed);
        assert(!weaponcall::local_quiescent()&&!weaponhand::client.registration_alive(33,22));
    }else if(mode=="changed_invocation"){
        assert(guarded_begin(&call,&state));auto wrong=call;wrong.callee=8;
        assert(weaponcall::returned(&weaponhand::client,&wrong,&state,0)==gears_weapon_call::End::unresolved);
        assert(callbacks.load()==1&&!weaponcall::local_quiescent());
        assert(weaponcall::unwind(&weaponhand::client,&call,&state,gears_weapon_call::UnwindReason::notification_fault)==gears_weapon_call::End::closed);
        assert(!callbacks.load()&&!weaponcall::local_quiescent());
    }else{assert(false);}
    std::printf("WeaponCallSubscriberProtocolTest %s passed; Raw/writer OFF, artificial memory/local Host only\n",argv[1]);
}
