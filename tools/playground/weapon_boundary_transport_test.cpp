// Actual PG transport, admission/pool and actual PG client stop. The provider,
// original body and successful stage Finish below are controlled host seams.
// No Native provider, engine helper, action routing or restoration is proved.
#include "../vr-common/weapon_shot_scope.h"
#include "../vr-common/hand_animation_observation.h"
#include <windows.h>
#include <atomic>
#include <cstdlib>
#include <type_traits>
#include <thread>
namespace {
#include "weapon_restore_stage.inl"
}
#include "weapon_hand_offline_fixture.h"
namespace {
std::atomic<unsigned> callbacks{};
#include "weapon_shot_scope.inl"
#include "weapon_call_subscriber.inl"
namespace handanim {
using Token=gears_hand_animation::Token;using Source=gears_hand_animation::Source;
struct Probe {gears_hand_animation::Ledger ledger;} probe;
Token enter(uptr,uptr,void*,Source,uptr){return {};}
bool leave(Token const&,uptr,uptr,void*,uptr){return true;}
}
#include "weapon_action_subscriber.inl"
#include "weapon_action_boundary_client.inl"
namespace b=gears_weapon_boundary;
constexpr DWORD export_fault=0xe1530042ul,original_fault=0xe1530043ul;
char const* mode{};unsigned revokes{},opens{},closes{},unregisters{},originals{},notices{};
std::uint64_t host_generation{8};b::State* tls{};b::Receipt expected_receipt{},last_close{};
abi::Reply no_scope_reply=abi::Reply::busy;
bool is(char const* value){return !std::strcmp(mode,value);}
void worker_stop(abi::Reply expected){
    std::thread worker([expected]{assert(weaponhand::stop()==expected);});worker.join();
}
abi::Reply host_unregister(std::uint64_t cookie)noexcept {
    assert(cookie==33);++unregisters;return abi::Reply::quiescent;
}
abi::Reply revoke(b::Request const* request,std::uint64_t* output)noexcept {
    assert(b::request_valid(*request)&&request->hand_cookie==33&&request->native_started_qpc==22);
    assert(request->context==reinterpret_cast<uptr>(&weaponhand::client)&&request->thread_id==GetCurrentThreadId());
    ++revokes;
    if(request->flags&b::unresolved){
        ++notices;if(is("notice_seh"))RaiseException(export_fault,0,0,nullptr);
    }else{
        if(is("revoke_seh"))RaiseException(export_fault,0,0,nullptr);
        if(is("revoke_no_scope"))return no_scope_reply;
        if(is("revoke_zero")){*output=0;return abi::Reply::accepted;}
    }
    *output=++host_generation;return abi::Reply::accepted;
}
abi::Reply open(b::Request const* request,b::State* state,b::Receipt* receipt)noexcept {
    ++opens;assert(b::request_valid(*request));
    if(is("no_scope"))return no_scope_reply;
    if(is("no_scope_reuse")&&opens==1)return abi::Reply::busy;
    if(is("nested")&&tls)return abi::Reply::reentrant;
    if(is("dirty_no_scope")){state->storage[0]=std::byte{7};return abi::Reply::busy;}
    if(is("invalid_reply"))return abi::Reply::invalid;
    if(is("quiescent_reply"))return abi::Reply::quiescent;
    if(is("unknown_reply"))return static_cast<abi::Reply>(999);
    if(is("open_zero_seh"))RaiseException(export_fault,0,0,nullptr);
    assert(!tls);tls=state;state->storage[0]=std::byte{7};
    receipt->hand_cookie=request->hand_cookie;receipt->native_started_qpc=request->native_started_qpc;
    receipt->scope_id=71;receipt->revocation_generation=is("higher_generation")?host_generation+4:host_generation;
    receipt->thread_id=request->thread_id;receipt->flags=b::scope_active|b::engine_calls_allowed;
    expected_receipt=*receipt;
    if(is("open_seh"))RaiseException(export_fault,0,0,nullptr);
    if(is("bad_receipt"))receipt->hand_cookie=34;
    if(is("changed_request"))const_cast<b::Request*>(request)->object=0xabc;
    return abi::Reply::accepted;
}
b::End close(b::State* state,b::Receipt const* receipt,b::Finish const* finish)noexcept {
    ++closes;assert(state==tls&&b::pool_same_receipt(*receipt,expected_receipt));last_close=*receipt;
    assert(finish->outcome==(is("original_seh")?b::Outcome::original_exception:b::Outcome::returned));
    if(is("close_seh"))RaiseException(export_fault,0,0,nullptr);
    tls=nullptr;*state={};
    if(is("close_after_seh"))RaiseException(export_fault,0,0,nullptr);
    if(is("close_unresolved")||is("incomplete")||is("notice_seh"))return b::End::unresolved;
    if(is("changed_close_receipt"))const_cast<b::Receipt*>(receipt)->revocation_generation=999;
    if(is("changed_finish"))const_cast<b::Finish*>(finish)->flags=0;
    return b::End::closed;
}
void connect(){
    game_thread.store(0);
    assert(weaponhand::client.begin_connect({11,GetCurrentProcessId()})==abi::Reply::accepted);
    abi::RegistrationResult registration{};registration.cookie=33;registration.native_started_qpc=22;
    assert(weaponhand::client.connected(abi::Reply::accepted,registration)==abi::Reply::accepted);
    weaponcall::attached_cookie.store(33);weaponcall::attached_native.store(22);
    weaponaction::attached_cookie.store(33);weaponaction::attached_native.store(22);
    weaponhand::native_module=LoadLibraryW(L"kernel32.dll");assert(weaponhand::native_module);
    weaponhand::host_unregister=host_unregister;
    assert(weaponboundary::admission.connect({33,22,&weaponhand::client,revoke,open,close}));
    game_thread.store(GetCurrentThreadId());
}
b::Finish complete_finish(){
    b::Finish finish{};finish.flags=b::stage_closed|b::handback_complete;return finish;
}
void original_with_finally(weaponboundary::Transport& scope,bool terminal_incomplete){
    auto finish=complete_finish();
    if(terminal_incomplete)finish.flags=b::stage_closed;
    __try{
        ++originals;
        assert(weaponboundary::admission.in_flight()==1&&!closes);
        const auto held=weaponhand::native_module;worker_stop(abi::Reply::draining);
        assert(weaponhand::client.phase()==abi::ClientPhase::draining&&weaponhand::native_module==held);
        if(is("original_seh"))RaiseException(original_fault,0,0,nullptr);
    }__finally{
        if(AbnormalTermination())finish.outcome=b::Outcome::original_exception;
        (void)weaponboundary::finish_transport(scope,finish);
    }
}
bool run_original(weaponboundary::Transport& scope,bool incomplete){
    __try{original_with_finally(scope,incomplete);return true;}
    __except(GetExceptionCode()==original_fault?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return false;}
}
void assert_retained(){
    assert(!weaponboundary::admission.in_flight()&&weaponboundary::admission.faulted());
    assert(!weaponrestore::healthy()&&!weaponboundary::local_quiescent());
    const auto held=weaponhand::native_module;assert(held);worker_stop(abi::Reply::draining);
    assert(weaponhand::native_module==held&&weaponhand::client.phase()==abi::ClientPhase::draining);
}
}
int main(int argc,char** argv){
    assert(argc>=2&&argc<=3);mode=argv[1];connect();
    if(argc==3){
        const unsigned value=static_cast<unsigned>(std::strtoul(argv[2],nullptr,10));
        no_scope_reply=static_cast<abi::Reply>(value);assert(b::pool_no_scope_reply(no_scope_reply));
    }
    weaponboundary::Transport scope{};
    if(is("sealed")){
        weaponboundary::seal();assert(weaponboundary::begin_transport(scope,b::Source::process_event,0x111,0x222,b::related,true)==weaponboundary::Delivery::no_scope);
        assert(!revokes&&!opens&&!closes&&!weaponboundary::admission.in_flight());
        assert(weaponboundary::finish_transport(scope,complete_finish())==weaponboundary::Delivery::no_scope);
        worker_stop(abi::Reply::quiescent);assert(!weaponhand::native_module);return 0;
    }
    const auto source=is("owned")?b::Source::owned_bridge:b::Source::process_event;
    const auto begun=weaponboundary::begin_transport(scope,source,0x111,0x222,b::related,!is("no_open"));
    assert(weaponboundary::admission.in_flight()==1&&scope.request.source==source);
    if(is("no_scope_reuse")){
        assert(begun==weaponboundary::Delivery::no_scope&&!(scope.flags&weaponboundary::cell_owned));
        weaponboundary::Transport newer{};
        assert(weaponboundary::begin_transport(newer,source,0x111,0x223,b::related,true)==weaponboundary::Delivery::active);
        assert(newer.cell.index==scope.cell.index&&newer.cell.generation>scope.cell.generation);
        assert(weaponboundary::admission.in_flight()==2);
        assert(weaponboundary::finish_transport(scope,complete_finish())==weaponboundary::Delivery::no_scope);
        assert(weaponboundary::finish_transport(scope,complete_finish())==weaponboundary::Delivery::no_scope);
        b::PoolDiagnostic live{};assert(weaponboundary::states.describe(newer.cell,live));
        assert(live.phase==b::PoolPhase::active&&live.state==tls&&!closes&&weaponboundary::admission.in_flight()==1);
        worker_stop(abi::Reply::draining);assert(weaponhand::native_module);
        assert(weaponboundary::finish_transport(newer,complete_finish())==weaponboundary::Delivery::closed);
        assert(weaponboundary::finish_transport(scope,complete_finish())==weaponboundary::Delivery::no_scope);
        assert(closes==1&&!weaponboundary::admission.in_flight()&&!weaponboundary::states.retained()&&weaponrestore::healthy());
        worker_stop(abi::Reply::quiescent);assert(!weaponhand::native_module);
        std::puts("WeaponBoundaryTransportTest no_scope_reuse passed: old original-gap finish/retry cannot touch reused cell generation");
        return 0;
    }
    if(is("nested")){
        assert(begun==weaponboundary::Delivery::active);const auto frozen=scope.receipt.revocation_generation;
        weaponboundary::Transport nested{};
        assert(weaponboundary::begin_transport(nested,source,0x111,0x223,b::related,true)==weaponboundary::Delivery::no_scope);
        assert(weaponboundary::admission.in_flight()==2&&host_generation>frozen);
        assert(weaponboundary::finish_transport(nested,complete_finish())==weaponboundary::Delivery::no_scope);
        assert(weaponboundary::admission.in_flight()==1);
    }
    if(is("duplicate_begin")){
        assert(begun==weaponboundary::Delivery::active);
        assert(weaponboundary::begin_transport(scope,source,0x111,0x222,b::related,true)==weaponboundary::Delivery::unresolved);
    }
    // Controlled interrupted-journal fixtures, not thrown C++ through a
    // noexcept provider. Unknown export completion must never become clean.
    if(is("lost_open_return"))scope.flags&=~weaponboundary::open_normal;
    if(is("lost_revoke_return"))scope.flags&=~weaponboundary::revoke_normal;
    const bool incomplete=is("incomplete")||is("notice_seh");
    assert(run_original(scope,incomplete)==!is("original_seh"));assert(originals==1);
    assert(!weaponboundary::admission.in_flight()&&(scope.flags&weaponboundary::finished));
    const bool faulted=is("revoke_zero")||is("revoke_seh")||is("open_seh")||is("open_zero_seh")||
        is("bad_receipt")||is("changed_request")||is("dirty_no_scope")||is("invalid_reply")||is("quiescent_reply")||
        is("unknown_reply")||is("close_seh")||is("close_after_seh")||is("close_unresolved")||
        is("changed_close_receipt")||is("changed_finish")||is("duplicate_begin")||
        is("lost_open_return")||is("lost_revoke_return")||incomplete;
    if(faulted){
        assert(scope.delivery==weaponboundary::Delivery::unresolved);assert_retained();
        if(scope.flags&weaponboundary::cell_owned)assert(weaponboundary::states.isolated()==1);
    }else{
        assert(weaponrestore::healthy()&&!weaponboundary::admission.faulted()&&!weaponboundary::states.retained());
        if(is("no_scope")||is("revoke_no_scope")||is("no_open")){
            assert(!closes&&scope.delivery==weaponboundary::Delivery::no_scope);
        }else{
            assert(closes==1&&scope.delivery==weaponboundary::Delivery::closed&&!tls);
            assert(b::pool_same_receipt(last_close,expected_receipt));
        }
        const auto calls=closes;
        assert(weaponboundary::finish_transport(scope,complete_finish())==scope.delivery&&closes==calls);
        worker_stop(abi::Reply::quiescent);assert(!weaponhand::native_module);
        assert(weaponhand::client.phase()==abi::ClientPhase::off);
    }
    std::printf("WeaponBoundaryTransportTest %s passed; actual PG transport/client, controlled provider/original/Finish only\n",mode);
}
