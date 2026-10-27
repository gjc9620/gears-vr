// Actual PG stop path with local hand/call/Gate delivery, fake boundary APIs
// and an owned kernel32 module reference. No Native provider or game runs.
#include "../vr-common/weapon_shot_scope.h"
#include "../vr-common/hand_animation_observation.h"
#include <windows.h>
#include <atomic>
#include <type_traits>
#if defined(GEARS_PLAYGROUND_RESTORE_STAGE_CORE)
namespace {
#include "weapon_restore_stage.inl"
}
#endif
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
unsigned unregistrations{};
abi::Reply host_unregister(std::uint64_t cookie) noexcept {assert(cookie==33);++unregistrations;return abi::Reply::quiescent;}
abi::Reply revoke(gears_weapon_boundary::Request const*,std::uint64_t* generation) noexcept {*generation=8;return abi::Reply::accepted;}
abi::Reply open(gears_weapon_boundary::Request const*,gears_weapon_boundary::State*,gears_weapon_boundary::Receipt*) noexcept {return abi::Reply::unavailable;}
gears_weapon_boundary::End close(gears_weapon_boundary::State*,gears_weapon_boundary::Receipt const*,gears_weapon_boundary::Finish const*) noexcept {return gears_weapon_boundary::End::closed;}
}
int main(int argc,char** argv){
    assert(argc<=2);const bool retain_fault=argc==2&&std::strcmp(argv[1],"faulted")==0;
#if defined(GEARS_PLAYGROUND_RESTORE_STAGE_CORE)
    const bool stage_fault=argc==2&&std::strcmp(argv[1],"stage_faulted")==0;
#else
    const bool stage_fault=false;
#endif
    game_thread.store(0);
    assert(weaponhand::start(GetModuleHandleW(L"kernel32.dll"))==abi::Reply::unavailable);
    assert(!weaponhand::native_module&&weaponhand::client.phase()==abi::ClientPhase::off);
    assert(weaponhand::client.begin_connect({11,GetCurrentProcessId()})==abi::Reply::accepted);
    abi::RegistrationResult receipt{};receipt.cookie=33;receipt.native_started_qpc=22;
    assert(weaponhand::client.connected(abi::Reply::accepted,receipt)==abi::Reply::accepted);
    weaponcall::attached_cookie.store(33);weaponcall::attached_native.store(22);
    weaponaction::attached_cookie.store(33);weaponaction::attached_native.store(22);
    weaponhand::native_module=LoadLibraryW(L"kernel32.dll");assert(weaponhand::native_module);
    const auto held=weaponhand::native_module;weaponhand::host_unregister=host_unregister;
    assert(weaponboundary::admission.connect({33,22,&weaponhand::client,revoke,open,close}));
    game_thread.store(GetCurrentThreadId());
    assert(weaponhand::stop()==abi::Reply::reentrant&&weaponhand::client.phase()==abi::ClientPhase::active&&!unregistrations);
    gears_weapon_boundary::Admission::Entry after_rejection{};
    assert(weaponboundary::admission.acquire(after_rejection)&&weaponboundary::admission.release(after_rejection));
    game_thread.store(0);
    gears_weapon_boundary::Admission::Entry entry{};assert(weaponboundary::admission.acquire(entry));
    const auto api=weaponboundary::admission.view(entry);assert(api);
    gears_weapon_boundary::Request request{};gears_weapon_boundary::State state{};gears_weapon_boundary::Receipt native_receipt{};
    assert(api->open(&request,&state,&native_receipt)==abi::Reply::unavailable&&!native_receipt.flags);
    gears_weapon_boundary::Request accepted_request{};
    accepted_request.source=gears_weapon_boundary::Source::process_event;accepted_request.flags=gears_weapon_boundary::related;
    accepted_request.hand_cookie=33;accepted_request.native_started_qpc=22;
    accepted_request.context=reinterpret_cast<uptr>(&weaponhand::client);accepted_request.object=0x111;accepted_request.function=0x222;
    accepted_request.thread_id=GetCurrentThreadId();
    gears_weapon_boundary::Receipt accepted_receipt{};accepted_receipt.hand_cookie=33;accepted_receipt.native_started_qpc=22;
    accepted_receipt.scope_id=77;accepted_receipt.revocation_generation=8;accepted_receipt.thread_id=GetCurrentThreadId();
    accepted_receipt.flags=gears_weapon_boundary::scope_active|gears_weapon_boundary::engine_calls_allowed;
    game_thread.store(GetCurrentThreadId());
    assert(weaponboundary::grant_matches(entry,accepted_request,accepted_receipt,abi::Reply::accepted));
    auto wrong_receipt=accepted_receipt;wrong_receipt.hand_cookie=34;
    assert(!weaponboundary::grant_matches(entry,accepted_request,wrong_receipt,abi::Reply::accepted));
    game_thread.store(0);
    assert(weaponhand::stop()==abi::Reply::draining);
    assert(unregistrations==1&&weaponhand::native_quiescent&&weaponhand::native_module==held);
    assert(weaponhand::client.phase()==abi::ClientPhase::draining); // clear_gt/finish_disconnect did not run.
    game_thread.store(GetCurrentThreadId());
    assert(!weaponhand::client.registration_alive(33,22));
    assert(weaponboundary::grant_matches(entry,accepted_request,accepted_receipt,abi::Reply::accepted));
    game_thread.store(0);
    gears_weapon_boundary::Admission::Entry denied{};assert(!weaponboundary::admission.acquire(denied));
    std::uint64_t generation{};assert(api->revoke(&request,&generation)==abi::Reply::accepted&&generation==8);
    if(retain_fault)weaponboundary::admission.quarantine();
    assert(weaponboundary::admission.release(entry));
#if defined(GEARS_PLAYGROUND_RESTORE_STAGE_CORE)
    if(stage_fault)weaponrestore::sticky_fault.store(true,std::memory_order_release);
#endif
    if(retain_fault||stage_fault){
        assert(weaponhand::stop()==abi::Reply::draining&&unregistrations==1);
        assert(weaponhand::client.phase()==abi::ClientPhase::draining&&weaponhand::native_module==held);
        assert(!weaponboundary::admission.in_flight());
        if(retain_fault)assert(!weaponboundary::local_quiescent());
        if(stage_fault)assert(weaponboundary::local_quiescent()); // Independent stage debt still blocks finish_disconnect.
        std::puts("WeaponBoundaryLifecycleTest faulted passed: zero local count cannot erase sticky debt or release Native/client state");
        return 0; // Retain uncertain module/context until this fixture process exits.
    }
    assert(weaponhand::stop()==abi::Reply::quiescent&&unregistrations==1);
    assert(weaponhand::client.phase()==abi::ClientPhase::off&&!weaponhand::native_module&&!weaponhand::host_unregister);
    assert(!weaponcall::attached_cookie.load()&&!weaponaction::attached_cookie.load()&&weaponboundary::local_quiescent());
    assert(weaponhand::stop()==abi::Reply::quiescent);
    std::puts("WeaponBoundaryLifecycleTest passed: unsafe no-scope PG entry blocks actual finish_disconnect/Native FreeLibrary; local API/receipt fixture only");
}
