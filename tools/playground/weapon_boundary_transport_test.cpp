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
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_ENTRY_TRIAL)
namespace weaponentry {
// Link-only host seam: this fixture never dispatches the subscriber callback.
// It does not include weapon_action_entry.inl or prove actual CF routing.
bool revoke_call(uptr,uptr,bool& related) noexcept {related=false;return true;}
}
#endif
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
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_ENTRY_TRIAL)
    if((is("entry_request_cookie")||is("entry_request_context"))&&
       (request->hand_cookie!=33||request->context!=reinterpret_cast<uptr>(&weaponhand::client))){
        assert(b::request_valid(*request)&&(request->flags&b::unresolved));
        ++revokes;++notices;return abi::Reply::wrong_cookie;
    }
#endif
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
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_ENTRY_TRIAL)
    if(is("entry_no_scope"))return no_scope_reply;
    if(is("entry_stale_ticket")&&opens==1)return abi::Reply::busy;
#endif
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
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_ENTRY_TRIAL)
    if(is("entry_generation_advance"))receipt->revocation_generation=host_generation+4;
#endif
    expected_receipt=*receipt;
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_ENTRY_TRIAL)
    if(is("entry_open_unknown"))RaiseException(export_fault,0,0,nullptr);
#endif
    if(is("open_seh"))RaiseException(export_fault,0,0,nullptr);
    if(is("bad_receipt"))receipt->hand_cookie=34;
    if(is("changed_request"))const_cast<b::Request*>(request)->object=0xabc;
    return abi::Reply::accepted;
}
b::End close(b::State* state,b::Receipt const* receipt,b::Finish const* finish)noexcept {
    ++closes;assert(state==tls&&b::pool_same_receipt(*receipt,expected_receipt));last_close=*receipt;
    assert(finish->outcome==(is("original_seh")?b::Outcome::original_exception:b::Outcome::returned));
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_ENTRY_TRIAL)
    if(is("entry_close_unknown"))RaiseException(export_fault,0,0,nullptr);
#endif
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
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_ENTRY_TRIAL)
// Only the transport's actual claim/check is exercised here. Incrementing this
// counter stands for a host stage body; it is not BoundaryBackend/restoration.
bool entry_host_stage(weaponboundary::Transport& entry_scope,uptr journal_address,unsigned& stage_bodies){
    if(!weaponboundary::claim_stage(entry_scope,journal_address))return false;
    assert(weaponrestore::healthy()&&weaponboundary::stage_grant_matches(entry_scope,journal_address));
    ++stage_bodies;return true;
}
void entry_retained(unsigned in_flight,b::State const* retained_state){
    assert(weaponboundary::admission.in_flight()==in_flight&&weaponboundary::admission.faulted());
    assert(!weaponrestore::healthy()&&!weaponboundary::local_quiescent());
    if(retained_state)assert(tls==retained_state&&retained_state->storage[0]==std::byte{7});
    const auto held=weaponhand::native_module;assert(held);worker_stop(abi::Reply::draining);
    assert(weaponhand::native_module==held&&weaponhand::client.phase()==abi::ClientPhase::draining);
}
void entry_mode(){
    // Actual coordinator Journal storage supplies the stable address only.
    // No synthetic Backend is substituted for the production grant predicate.
    weaponrestore::Journal<unsigned> entry_journal{},other_journal{};
    const auto journal_address=reinterpret_cast<uptr>(&entry_journal);
    const auto other_address=reinterpret_cast<uptr>(&other_journal);
    unsigned stage_bodies{};
    weaponboundary::Transport entry_scope{},retired{};
    if(is("entry_stale_ticket")){
        assert(weaponboundary::begin_transport(retired,b::Source::process_event,0x111,0x221,b::related,true)==weaponboundary::Delivery::no_scope);
        assert(weaponboundary::finish_transport(retired,complete_finish())==weaponboundary::Delivery::no_scope);
    }
    const auto delivery=weaponboundary::begin_transport(entry_scope,b::Source::process_event,0x111,0x222,b::related,!is("entry_no_open"));
    if(is("entry_no_scope")||is("entry_no_open")||is("entry_open_unknown")){
        assert(delivery==(is("entry_open_unknown")?weaponboundary::Delivery::unresolved:weaponboundary::Delivery::no_scope));
        auto* const retained_state=tls;
        assert(!weaponboundary::stage_grant_matches(entry_scope,journal_address));
        assert(!entry_host_stage(entry_scope,journal_address,stage_bodies)&&!stage_bodies);
        assert(weaponboundary::finish_transport(entry_scope,complete_finish())==weaponboundary::Delivery::unresolved);
        assert(!closes);
        if(is("entry_open_unknown")){
            b::PoolDiagnostic isolated{};
            assert(retained_state&&weaponboundary::states.describe_isolated(entry_scope.cell.index,isolated));
            assert(isolated.state==retained_state&&isolated.receipt_frozen==false);
        }else assert(!tls&&!weaponboundary::states.retained());
        entry_retained(0,retained_state);return;
    }
    assert(delivery==weaponboundary::Delivery::active&&weaponboundary::admission.in_flight()==1);
    b::PoolDiagnostic native_cell{};
    assert(weaponboundary::states.describe(entry_scope.cell,native_cell));
    assert(native_cell.phase==b::PoolPhase::active&&native_cell.receipt_frozen&&native_cell.state==tls);
    assert(b::pool_same_request(native_cell.request,entry_scope.request)&&b::pool_same_receipt(native_cell.receipt,expected_receipt));
    assert(b::pool_same_receipt(entry_scope.receipt,expected_receipt));
    auto* const retained_state=tls;
    const auto native_cell_index=entry_scope.cell.index;
    const auto frozen_generation=entry_scope.receipt.revocation_generation;
    if(is("entry_copy")||is("entry_copy_rebased")){
        auto copied_transport=entry_scope;
        if(is("entry_copy_rebased"))copied_transport.address=reinterpret_cast<uptr>(&copied_transport);
        assert(!weaponboundary::stage_grant_matches(copied_transport,journal_address));
        assert(!entry_host_stage(copied_transport,journal_address,stage_bodies)&&!stage_bodies);
        assert(!closes&&weaponboundary::states.phase(entry_scope.cell.index)==b::PoolPhase::active);
        // An invalid copy must not release the original Entry or current cell.
        entry_retained(1,retained_state);return;
    }
    if(is("entry_stopped")){
        assert(weaponboundary::finish_transport(entry_scope,complete_finish())==weaponboundary::Delivery::closed);
        worker_stop(abi::Reply::quiescent);
        assert(weaponhand::client.phase()==abi::ClientPhase::off&&!weaponhand::native_module);
        assert(!weaponboundary::stage_grant_matches(entry_scope,journal_address));
        assert(!entry_host_stage(entry_scope,journal_address,stage_bodies)&&!stage_bodies);
        assert(closes==1&&!tls&&!weaponboundary::states.retained());return;
    }
    if(is("entry_draining")){
        worker_stop(abi::Reply::draining);
        assert(weaponhand::client.phase()==abi::ClientPhase::draining&&weaponhand::native_module);
    }
    if(is("entry_generation_advance"))host_generation+=5;
    if(is("entry_receipt_cookie"))++entry_scope.receipt.hand_cookie;
    if(is("entry_receipt_scope"))++entry_scope.receipt.scope_id;
    if(is("entry_receipt_flags"))entry_scope.receipt.flags=b::scope_active;
    if(is("entry_receipt_generation"))++entry_scope.receipt.revocation_generation;
    if(is("entry_request_cookie"))++entry_scope.request.hand_cookie;
    if(is("entry_request_context"))++entry_scope.request.context;
    if(is("entry_wrong_entry"))++entry_scope.entry.generation;
    if(is("entry_wrong_cell"))++entry_scope.cell.index;
    if(is("entry_stale_ticket")){
        assert(entry_scope.cell.index==retired.cell.index&&entry_scope.cell.generation>retired.cell.generation);
        entry_scope.cell.generation=retired.cell.generation;
    }
    if(is("entry_disabled"))enabled.store(false,std::memory_order_release);
    const bool malformed=is("entry_receipt_cookie")||is("entry_receipt_scope")||is("entry_receipt_flags")||
        is("entry_receipt_generation")||is("entry_request_cookie")||is("entry_request_context")||
        is("entry_wrong_entry")||is("entry_wrong_cell")||is("entry_stale_ticket")||is("entry_disabled")||is("entry_zero_journal");
    bool claimed{};
    if(is("entry_foreign_gt")){
        std::thread worker([&]{claimed=entry_host_stage(entry_scope,journal_address,stage_bodies);});worker.join();
    }else claimed=entry_host_stage(entry_scope,is("entry_zero_journal")?0:journal_address,stage_bodies);
    if(malformed||is("entry_foreign_gt")){
        assert(!claimed&&!stage_bodies&&!weaponboundary::stage_grant_matches(entry_scope,journal_address));
        assert(weaponboundary::finish_transport(entry_scope,complete_finish())==weaponboundary::Delivery::unresolved);
        assert(!closes&&weaponboundary::states.retained()==1);
        if(is("entry_wrong_cell")||is("entry_stale_ticket"))assert(weaponboundary::states.phase(native_cell_index)==b::PoolPhase::active);
        entry_retained(is("entry_wrong_entry")?1u:0u,retained_state);return;
    }
    assert(claimed&&stage_bodies==1&&weaponboundary::stage_grant_matches(entry_scope,journal_address));
    assert(!weaponboundary::stage_grant_matches(entry_scope,other_address)&&!weaponboundary::stage_grant_matches(entry_scope,0));
    if(is("entry_other_journal")||is("entry_repeat_claim")){
        assert(!entry_host_stage(entry_scope,is("entry_other_journal")?other_address:journal_address,stage_bodies));
        assert(stage_bodies==1&&!weaponboundary::stage_grant_matches(entry_scope,journal_address));
        assert(weaponboundary::finish_transport(entry_scope,complete_finish())==weaponboundary::Delivery::unresolved);
        assert(!closes&&weaponboundary::states.isolated()==1);entry_retained(0,retained_state);return;
    }
    if(is("entry_generation_advance")){
        host_generation+=7;assert(host_generation>frozen_generation);
        assert(entry_scope.receipt.revocation_generation==frozen_generation&&weaponboundary::stage_grant_matches(entry_scope,journal_address));
    }
    assert(is("entry_accept")||is("entry_draining")||is("entry_generation_advance")||is("entry_close_unknown"));
    const auto closed=weaponboundary::finish_transport(entry_scope,complete_finish());
    assert(!weaponboundary::stage_grant_matches(entry_scope,journal_address)&&stage_bodies==1);
    if(is("entry_close_unknown")){
        assert(closed==weaponboundary::Delivery::unresolved&&closes==1&&weaponboundary::states.isolated()==1);
        b::PoolDiagnostic isolated{};assert(weaponboundary::states.describe_isolated(entry_scope.cell.index,isolated));
        assert(isolated.receipt_frozen&&b::pool_same_receipt(isolated.receipt,expected_receipt));
        entry_retained(0,retained_state);return;
    }
    assert(closed==weaponboundary::Delivery::closed&&closes==1&&!tls&&!weaponboundary::states.retained());
    assert(b::pool_same_receipt(last_close,expected_receipt)&&last_close.revocation_generation==frozen_generation);
    assert(weaponrestore::healthy()&&!weaponboundary::admission.in_flight());
    worker_stop(abi::Reply::quiescent);assert(!weaponhand::native_module);
}
#endif
}
int main(int argc,char** argv){
    assert(argc>=2&&argc<=3);mode=argv[1];connect();
    if(argc==3){
        const unsigned value=static_cast<unsigned>(std::strtoul(argv[2],nullptr,10));
        no_scope_reply=static_cast<abi::Reply>(value);assert(b::pool_no_scope_reply(no_scope_reply));
    }
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_ENTRY_TRIAL)
    if(!std::strncmp(mode,"entry_",6)){
        entry_mode();
        std::printf("WeaponBoundaryTransportTest %s passed; actual ENTRY transport/client/pool, host Native/stage body seams only\n",mode);
        return 0;
    }
#endif
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
    return 0;
}
