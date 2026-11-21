// Actual new read adapter plus existing real typed metadata/identity reader on
// owned memory. Runtime registration and world admission are injected here;
// this does not certify a loaded Native module or a game action.
#include "original_input_cleanup_context_policy.h"
#include "native_action_context.h"
#include "world_epoch_gate.h"
#define main existing_scene_binding_test_main
#define Scope ExistingSceneBindingFixtureScope
#include "weapon_hand_scene_binding_test.cpp"
#undef main
#undef Scope
gears_world::EpochGate cleanup_world_gate;
thread_local gears_world::ThreadState cleanup_world_thread;
struct Scope {
    gears_world::RecursiveAdmission admission{cleanup_world_thread,cleanup_world_gate};
    bool world()const{return bool(admission);}
};
namespace cleanup_test {
struct Session {unsigned game_pid{},owner_worker_thread{};uint64_t game_start{},native_started_qpc{};};
struct Broker {
    uint64_t cookie_value{41},revision_value{3};bool registered{true},terminal{};
    unsigned callbacks{},transactions{},aux{},actions{},external{},invalidations{};bool auxiliary_debt{};
    uint64_t cookie()const{return cookie_value;}uint64_t lifecycle_revision()const{return revision_value;}
    bool registration_current(uint64_t c,uint64_t n)const{return registered&&c==cookie_value&&n==23;}
    bool draining()const{return terminal;}unsigned pending_callbacks()const{return callbacks;}unsigned pending_transactions()const{return transactions;}
    unsigned pending_aux_scopes()const{return aux;}bool aux_debt()const{return auxiliary_debt;}unsigned pending_action_scopes()const{return actions;}
    unsigned pending_external_scopes()const{return external;}bool pending_action_invalidation()const{return invalidations!=0;}
} broker;
Session session_value{};
}
namespace weapon_hand_runtime {
bool enabled=true;
namespace detail {std::atomic<bool> admission_closed{},stop_requested{},unknown_fault{};}
cleanup_test::Broker* current(){return &cleanup_test::broker;}
cleanup_test::Session session(){return cleanup_test::session_value;}
}
#include "original_input_cleanup_context_live.inl"
namespace cleanup=original_input_cleanup_context_live;
namespace capi=gears_weapon_original_input_cleanup_context;
namespace ch=gears_weapon_hand;
capi::Request cleanup_request(){
    setup();weapon_hand_runtime::enabled=true;weapon_hand_runtime::detail::admission_closed=false;
    weapon_hand_runtime::detail::stop_requested=false;weapon_hand_runtime::detail::unknown_fault=false;
    gears_native_context::thread_ledger={};cleanup_test::broker={};
    cleanup_test::session_value={GetCurrentProcessId(),GetCurrentThreadId()+1,19,23};
    h::Binding sampled{};C(take(sampled));capi::Request r{};
    r.parent.key={19,23,41,3,17,1,GetCurrentProcessId(),GetCurrentThreadId()};
    r.parent.world=cleanup::object(sampled.world);r.parent.controller=cleanup::object(sampled.controller);
    r.parent.pawn=cleanup::object(sampled.pawn);r.parent.mesh=cleanup::object(sampled.mesh);r.parent.weapon=cleanup::object(sampled.weapon);
    r.parent.reference.orientation.w=1;r.parent.reference_generation=1;return r;
}
// The existing fixture safe_copy only reads its arena; requests also reside
// there so the production safe-copy input/closing read is actually exercised.
unsigned ask(capi::Request const& r){const auto address=arena+0x80000;put(address,r);
    if(!read_hook)memcpy(saved_arena.data(),reinterpret_cast<void*>(arena),saved_arena.size());
    const auto result=cleanup::query(reinterpret_cast<capi::Request const*>(address));
    if(!read_hook)C(!memcmp(saved_arena.data(),reinterpret_cast<void*>(arena),saved_arena.size()));C(native_calls==0);return result;
}
int main(){
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,1024*1024,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));C(arena!=0);if(!arena)return 1;
    auto r=cleanup_request();C(ask(r)==unsigned(capi::Result::same_incarnation));C(native_calls==0);
    // Reference/pose/frame may have expired; these historical source markers
    // do not masquerade as a current pose and do not replace actual ancestry.
    r.parent.key.lifecycle=1;r.parent.key.frame=1;r.parent.reference_generation=7;
    recording=false;C(ask(r)==unsigned(capi::Result::same_incarnation));recording=true;
    for(unsigned field=0;field<8;++field){r=cleanup_request();
        if(field==0)++r.size;if(field==1)++r.version;if(field==2)r.reserved[0]=1;if(field==3)r.reserved[1]=1;
        if(field==4)r.parent.weapon={};if(field==5)++r.parent.key.registration_cookie;if(field==6)++r.parent.key.game_start;if(field==7)++r.parent.key.native_started_qpc;
        C(ask(r)==unsigned(capi::Result::rejected));}
    for(unsigned field=0;field<9;++field){r=cleanup_request();
        if(field==0)cleanup_test::broker.registered=false;if(field==1)cleanup_test::broker.terminal=true;
        if(field==2)cleanup_test::broker.callbacks=1;if(field==3)cleanup_test::broker.transactions=1;if(field==4)cleanup_test::broker.aux=1;
        if(field==5)cleanup_test::broker.actions=1;if(field==6)cleanup_test::broker.external=1;if(field==7)cleanup_test::broker.invalidations=1;if(field==8)cleanup_test::broker.auxiliary_debt=true;
        C(ask(r)==unsigned(capi::Result::unavailable));}
    for(unsigned field=0;field<4;++field){r=cleanup_request();
        if(field==0)weapon_hand_runtime::enabled=false;if(field==1)weapon_hand_runtime::detail::admission_closed=true;
        if(field==2)weapon_hand_runtime::detail::stop_requested=true;if(field==3)weapon_hand_runtime::detail::unknown_fault=true;
        C(ask(r)==unsigned(capi::Result::unavailable));}
    for(unsigned i=0;i<unsigned(gears_native_context::Kind::count);++i){r=cleanup_request();gears_native_context::Ticket ticket{};
        C(gears_native_context::enter(gears_native_context::thread_ledger,ticket,static_cast<gears_native_context::Kind>(i)));
        C(ask(r)==unsigned(capi::Result::unavailable));C(gears_native_context::leave(gears_native_context::thread_ledger,ticket));}
    r=cleanup_request();gears_native_context::thread_ledger.poisoned=true;C(ask(r)==unsigned(capi::Result::unavailable));
    r=cleanup_request();h::game_thread=GetCurrentThreadId()+1;C(ask(r)==unsigned(capi::Result::unavailable));
    r=cleanup_request();cleanup_test::session_value.owner_worker_thread=GetCurrentThreadId();C(ask(r)==unsigned(capi::Result::unavailable));
    for(unsigned field=0;field<7;++field){r=cleanup_request();
        if(field==0)put(fixture_pawn+0x1d0,fixture_weapon2);if(field==1)put(fixture_pawn+0x180,fixture_weapon);
        if(field==2)put(fixture_pc+0x188,uintptr_t(0));if(field==3)put(fixture_pc+b::metadata.controller_pawn,uintptr_t(0));
        if(field==4)put(fixture_weapon+0x218,fixture_pc);if(field==5)++committed_world_generation;if(field==6)put(fixture_weapon+76,1u);
        C(ask(r)==unsigned(capi::Result::rejected));}
    r=cleanup_request();gears_world::EpochGate::CloseToken closed{};C(cleanup_world_gate.try_close(closed));C(ask(r)==unsigned(capi::Result::unavailable));C(cleanup_world_gate.open(closed));
    r=cleanup_request();plan(fixture_weapon+8,2,reinterpret_cast<uintptr_t>(&cleanup_test::broker.revision_value),4,8);C(ask(r)==unsigned(capi::Result::rejected));read_hook=nullptr;
    r=cleanup_request();plan(fixture_weapon+8,2,0,0,0);change_generation=true;C(ask(r)==unsigned(capi::Result::rejected));read_hook=nullptr;
    r=cleanup_request();plan(arena+0x80000,2,arena+0x80000+offsetof(capi::Request,reserved),1,4);C(ask(r)==unsigned(capi::Result::rejected));read_hook=nullptr;
    r=cleanup_request();const auto address=arena+0x80000;put(address,r);denied=address;C(cleanup::query(reinterpret_cast<capi::Request const*>(address))==unsigned(capi::Result::rejected));denied=0;
    C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);printf("original_input_cleanup_context: %u checks, %u failures; actual adapter/typed owned-memory identity, injected runtime/world gate.\n",checks,failures);
    return failures?1:0;
}
