// Actual coordinator + concrete adapter, controlled old-helper/client seams.
// No old engine helper, game reflection, transport or Native provider runs.
#include <windows.h>
#include <array>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
#include "../vr-common/weapon_boundary_admission.h"
#include "../vr-common/weapon_boundary_state_pool.h"
#include "../vr-common/hand_stream_ticket_policy.h"
#include "../vr-common/hand_stream_grip_binding.h"
#include "../vr-common/hand_ik_trial_policy.h"
#include "../vr-common/free_hand_control.h"
#include "../vr-common/weapon_mesh_component_lease.h"
#ifdef NDEBUG
#error WeaponRestoreBackendTest requires active assertions
#endif
namespace {
using uptr=std::uintptr_t;namespace abi=gears_weapon_boundary;namespace handabi=gears_weapon_hand;
thread_local bool inside{};
std::atomic<DWORD> game_thread{};std::atomic<bool> enabled{true};
#include "weapon_restore_stage.inl"
enum class Mode {normal,owned,exempt,bad_exempt,hand_false,finish_false,stow_false,residue,guard_hands,guard_finish,
    nested,nested_poison,cpp_hands,seh_hands,cpp_finish,seh_finish,cpp_stow,seh_stow,lexical,retry,denied};
Mode mode{};std::vector<std::string> calls;unsigned hands_calls{},stow_calls{},finish_calls{},diagnostic_samples{},diagnostic_restores{},close_calls{};
bool last_owned{};void nested_notice();
namespace weaponaction_handoff {
struct Seen {uptr object{},function{};DWORD thread{};bool gt{},hands{},stow{},hands_required{},stow_required{},unknown_name{},pending_state_boundary{};};
inline bool advance_epoch(unsigned& value)noexcept {if(value==(std::numeric_limits<unsigned>::max)())return false;++value;return true;}
}
namespace weaponhand {
struct ClientHost {
    unsigned soft{},fault{};bool published{true};
    std::uint64_t cookie()const noexcept{return 41;}std::uint64_t native_started_qpc()const noexcept{return 71;}
    static void invalidate(void* context,handabi::Invalidation const* value)noexcept {
        auto& self=*static_cast<ClientHost*>(context);assert(value->registration_cookie==41&&value->native_started_qpc==71);
        self.published=false;if(value->reason==handabi::InvalidateReason::callback_fault)++self.fault;else ++self.soft;
    }
    handabi::Registration registration()noexcept {handabi::Registration r{};r.context=this;r.invalidate=invalidate;return r;}
} client;
}
namespace handstream {
struct HandLease {bool pending{true},fault{},private_busy{},private_restoring{};
    bool dirty()const noexcept{return pending;}bool faulted()const noexcept{return fault;}
    bool busy()const noexcept{return private_busy;}bool restoring()const noexcept{return private_restoring;}
};
struct Trial {
    HandLease lease;bool releasing{},ended{},xr_ready{true};unsigned epoch{3};
    gears_hand_ik::StabilityGate stability;gears_weapon::FreeHandControl consumer;
    gears_hand_stream::Policy xr_policy;std::array<gears_hand_stream::GripHandBinding,2> xr_binding;
    struct Timing {unsigned open_scopes{3};} timing;struct Reach {bool busy{};} reach;
} trial;
std::atomic<bool> stop_requested{},safe_to_unload{true};bool dual_draining{},hand_writes_relinquished{};
namespace component {
struct SelfSetter {uptr object{11},function{12};bool armed{};} self;
struct Resource {bool pending{true},fault{},relinquished{},private_busy{};
    void relinquish()noexcept{relinquished=true;fault=true;}
} lease;
struct Meta {unsigned group{2};bool forward{true},wanted_known{},wanted_original{};gears_weapon::WeaponMeshComponentValue expected{},wanted{};};
struct DiagnosticData {unsigned depth{4};Meta meta{};bool names{true},paused{},snapshot{true},aggregate{},slot{true};
    unsigned snapshot_group{1},aggregate_group{2},callback_index{3};
} diagnostics;
struct ForwardDiagnostics:DiagnosticData {
    ForwardDiagnostics()noexcept{static_cast<DiagnosticData&>(*this)=diagnostics;++diagnostic_samples;}
    void restore()const noexcept{diagnostics=static_cast<DiagnosticData const&>(*this);++diagnostic_restores;}
};
}
inline bool component_dirty(){return component::lease.pending;}inline bool component_faulted(){return component::lease.fault;}
enum class BeforeActionOutcome {ignored,clean,unresolved};
BeforeActionOutcome before_action_checked(uptr,uptr,bool owned);
namespace component_continuous {
struct State {unsigned action_depth{},operation_depth{};bool finishing{},closed{},deferred{},cleanup_failed{};} state;
inline void close(wchar_t const*,bool force)noexcept{assert(force);++close_calls;state.closed=true;state.deferred=true;}
bool finish();
}
}
namespace vrequip {namespace stow {
struct Resource {bool pending{true},fault{},revoked{},private_busy{};
    bool dirty()const noexcept{return pending;}bool faulted()const noexcept{return fault;}
    void revoke()noexcept{if(pending){revoked=true;fault=true;}}
} lease;
struct Pending {bool value{};bool active()const noexcept{return value;}} pending;
std::atomic<bool> stop_requested{},pending_visible{};bool mutating{},yielding{};unsigned epoch{7};
enum class BeforeActionOutcome {ignored,pending_state_exempt,clean,unresolved};
BeforeActionOutcome before_action_checked(uptr,uptr);
}}
namespace weaponboundary {
abi::Admission admission;abi::Request expected_request{};abi::Receipt expected_receipt{};bool grant_live{true};unsigned grant_calls{};
// Controlled grant service, not a clone/proof of the production client/provider.
// The actual Admission still rejects copied/released Entry identities.
bool grant_matches(abi::Admission::Entry const& entry,abi::Request const& request,abi::Receipt const& receipt,handabi::Reply reply)noexcept {
    ++grant_calls;const auto binding=admission.view(entry);
    return grant_live&&binding&&binding->cookie==request.hand_cookie&&binding->native_started_qpc==request.native_started_qpc&&
        binding->context==&weaponhand::client&&reinterpret_cast<uptr>(binding->context)==request.context&&
        abi::pool_same_request(expected_request,request)&&abi::pool_same_receipt(expected_receipt,receipt)&&reply==handabi::Reply::accepted;
}
}
#include "weapon_restore_backend.inl"
constexpr DWORD test_exception=0xe1422051;
void corrupt_lexical_and_resources(){
    handstream::trial.releasing=true;handstream::dual_draining=true;handstream::component::self={99,100,true};
    auto& state=handstream::component_continuous::state;state.action_depth=8;state.operation_depth=9;state.finishing=true;
    handstream::trial.timing.open_scopes=99;handstream::trial.reach.busy=true;vrequip::stow::mutating=true;vrequip::stow::yielding=true;
    auto& d=handstream::component::diagnostics;d.depth=88;d.names=false;d.paused=true;d.snapshot=false;d.aggregate=true;d.slot=false;
    d.snapshot_group=90;d.aggregate_group=91;d.callback_index=92;d.meta.group=93;d.meta.expected.translation[0]=94;d.meta.wanted.rotation[0]=95;
    handstream::trial.lease.pending=true;handstream::trial.lease.private_busy=true;handstream::trial.lease.private_restoring=true;
    handstream::component::lease.pending=true;handstream::component::lease.private_busy=true;
    vrequip::stow::lease.pending=true;vrequip::stow::lease.private_busy=true;
}
void inject(bool cpp){corrupt_lexical_and_resources();if(cpp)throw std::runtime_error("controlled old helper failure");RaiseException(test_exception,0,0,nullptr);}
handstream::BeforeActionOutcome handstream::before_action_checked(uptr object,uptr function,bool owned){
    assert(object==100&&function==200&&component_continuous::state.action_depth==1);++hands_calls;last_owned=owned;
    calls.push_back(owned?"hands:owned":"hands:external");
    if(mode==Mode::cpp_hands||mode==Mode::seh_hands)inject(mode==Mode::cpp_hands);
    if(mode==Mode::nested||mode==Mode::nested_poison){nested_notice();if(!weaponrestore::healthy())return BeforeActionOutcome::clean;}
    if(mode==Mode::guard_hands){component_continuous::state.action_depth=2;trial.lease.private_busy=true;return BeforeActionOutcome::clean;}
    trial.lease.pending=false;component::lease.pending=false;++trial.epoch;
    return mode==Mode::hand_false?BeforeActionOutcome::unresolved:BeforeActionOutcome::clean;
}
bool handstream::component_continuous::finish(){
    assert(!state.action_depth);++finish_calls;calls.push_back("finish:"+std::to_string(finish_calls));
    if(mode==Mode::cpp_finish||mode==Mode::seh_finish)inject(mode==Mode::cpp_finish);
    if(mode==Mode::guard_finish){state.operation_depth=2;trial.lease.private_busy=true;trial.lease.pending=true;return true;}
    if(mode==Mode::residue&&finish_calls==2)component::lease.fault=true;
    return !(mode==Mode::finish_false&&finish_calls==1);
}
vrequip::stow::BeforeActionOutcome vrequip::stow::before_action_checked(uptr object,uptr function){
    assert(object==100&&function==200&&handstream::component_continuous::state.action_depth==1);++stow_calls;calls.push_back("stow");
    if(mode==Mode::cpp_stow||mode==Mode::seh_stow)inject(mode==Mode::cpp_stow);
    if(mode==Mode::exempt||mode==Mode::bad_exempt)return BeforeActionOutcome::pending_state_exempt;
    lease.pending=false;pending.value=false;pending_visible=false;++epoch;
    if(mode==Mode::lexical){handstream::component::diagnostics.paused=true;handstream::trial.timing.open_scopes=44;}
    return mode==Mode::stow_false?BeforeActionOutcome::unresolved:BeforeActionOutcome::clean;
}
handabi::Reply no_revoke(abi::Request const*,std::uint64_t*)noexcept{return handabi::Reply::unavailable;}
handabi::Reply no_open(abi::Request const*,abi::State*,abi::Receipt*)noexcept{return handabi::Reply::unavailable;}
abi::End no_close(abi::State*,abi::Receipt const*,abi::Finish const*)noexcept{return abi::End::unresolved;}
struct Harness {
    abi::Admission::Entry entry{};weaponrestore::BoundaryBackend backend;
    Harness(){
        assert(weaponrestore::healthy()&&!weaponrestore::active());game_thread=GetCurrentThreadId();enabled=true;inside=false;
        calls.clear();hands_calls=stow_calls=finish_calls=diagnostic_samples=diagnostic_restores=close_calls=0;last_owned=false;
        handstream::trial={};handstream::component::lease={};handstream::component::self={};handstream::component::diagnostics={};
        handstream::component::diagnostics.meta.expected.translation[0]=0x12;handstream::component::diagnostics.meta.wanted.rotation[0]=0x34;
        handstream::component_continuous::state={};handstream::stop_requested=false;handstream::safe_to_unload=true;
        handstream::dual_draining=handstream::hand_writes_relinquished=false;vrequip::stow::lease={};vrequip::stow::pending={};
        vrequip::stow::stop_requested=false;vrequip::stow::pending_visible=false;vrequip::stow::mutating=vrequip::stow::yielding=false;
        vrequip::stow::epoch=7;weaponhand::client={};weaponboundary::grant_live=true;weaponboundary::grant_calls=0;
        abi::Binding binding{41,71,&weaponhand::client,no_revoke,no_open,no_close};assert(weaponboundary::admission.connect(binding));
        assert(weaponboundary::admission.acquire(entry));
        auto& request=backend.request;request.source=mode==Mode::owned?abi::Source::owned_bridge:abi::Source::process_event;
        request.flags=abi::related;request.hand_cookie=41;request.native_started_qpc=71;request.context=reinterpret_cast<uptr>(&weaponhand::client);
        request.object=100;request.function=200;request.thread_id=game_thread.load();
        backend.receipt={};backend.receipt.hand_cookie=41;backend.receipt.native_started_qpc=71;backend.receipt.scope_id=81;
        backend.receipt.revocation_generation=91;backend.receipt.thread_id=game_thread.load();backend.receipt.flags=abi::scope_active|abi::engine_calls_allowed;
        backend.open_reply=handabi::Reply::accepted;backend.lifetime=&entry;
        backend.seen={100,200,game_thread.load(),true,true,true,true,true,false,false};
        if(mode==Mode::exempt||mode==Mode::bad_exempt){backend.seen.hands=false;backend.seen.hands_required=false;
            backend.seen.pending_state_boundary=mode==Mode::exempt;vrequip::stow::pending.value=true;vrequip::stow::pending_visible=true;}
        weaponboundary::expected_request=request;weaponboundary::expected_receipt=backend.receipt;
    }
    void prepare(){const auto epoch=vrequip::stow::epoch;backend.disarmed=weaponrestore::prepare_forward_no_engine(backend.seen);
        assert(backend.disarmed.known&&vrequip::stow::epoch==epoch);}
    ~Harness(){if(weaponrestore::healthy()){assert(weaponboundary::admission.release(entry));weaponboundary::admission.seal();assert(weaponboundary::admission.detach());}}
};
weaponrestore::BoundaryBackend* parent_backend{};
void nested_notice(){
    assert(parent_backend&&weaponrestore::active());auto child=*parent_backend;child.hold=false;child.guard_mismatch=false;
    weaponrestore::Journal<weaponrestore::BoundaryBackend::Snapshot> journal{};
    const auto grants=weaponboundary::grant_calls;assert(weaponrestore::run(child,journal).outcome==weaponrestore::Outcome::denied);
    assert(weaponboundary::grant_calls==grants&&journal.flags==weaponrestore::created);
    if(mode==Mode::nested_poison)assert(weaponrestore::run(child,journal).outcome==weaponrestore::Outcome::unresolved);
}
void lexical_restored(){
    assert(!inside&&!weaponrestore::active()&&!handstream::trial.releasing&&!handstream::dual_draining);
    auto const& state=handstream::component_continuous::state;assert(!state.action_depth&&!state.operation_depth&&!state.finishing);
    assert(handstream::component::self.object==11&&handstream::component::self.function==12&&!handstream::component::self.armed);
    assert(handstream::trial.timing.open_scopes==3&&!handstream::trial.reach.busy&&!vrequip::stow::mutating&&!vrequip::stow::yielding);
    auto const& d=handstream::component::diagnostics;assert(d.depth==4&&d.names&&!d.paused&&d.snapshot&&!d.aggregate&&d.slot&&
        d.snapshot_group==1&&d.aggregate_group==2&&d.callback_index==3&&d.meta.group==2&&d.meta.expected.translation[0]==0x12&&d.meta.wanted.rotation[0]==0x34);
}
void sticky_blocks_later(Harness& h){
    assert(!weaponrestore::healthy()&&handstream::stop_requested&&!handstream::safe_to_unload&&vrequip::stow::stop_requested&&weaponhand::client.fault);
    const auto old_calls=calls;const auto epoch=handstream::trial.epoch;const auto soft=weaponhand::client.soft;
    assert(!weaponrestore::prepare_forward_no_engine(h.backend.seen).known&&handstream::trial.epoch==epoch&&weaponhand::client.soft==soft);
    weaponrestore::Journal<weaponrestore::BoundaryBackend::Snapshot> later{};
    assert(weaponrestore::run(h.backend,later).outcome==weaponrestore::Outcome::denied&&calls==old_calls);
    // Actual reset/unload/VM health gates are outside these includes. Assert
    // their concrete barrier outputs here; do not fake those control paths.
}
unsigned denied_cases(){unsigned count{};
    for(unsigned which=0;which<17;++which){Harness h;h.prepare();abi::Admission::Entry copied=h.entry;
        switch(which){case 0:h.backend.lifetime=nullptr;break;case 1:h.backend.lifetime=&copied;break;
        case 2:weaponboundary::grant_live=false;break;case 3:h.backend.receipt.scope_id=0;break;
        case 4:h.backend.receipt.flags=abi::scope_active;break;case 5:h.backend.open_reply=handabi::Reply::unavailable;break;
        case 6:h.backend.disarmed.function++;break;case 7:h.backend.disarmed.hand_epoch++;break;
        case 8:h.backend.request.context++;break;case 9:h.backend.receipt.revocation_generation++;break;
        case 10:h.backend.seen.unknown_name=true;break;case 11:h.backend.disarmed.known=false;break;
        case 12:enabled=false;break;case 13:game_thread=GetCurrentThreadId()+1;break;
        case 14:handstream::trial.lease.private_busy=true;break;case 15:handstream::component::self.armed=true;break;
        case 16:assert(weaponboundary::admission.release(h.entry));break;}
        const auto epoch=handstream::trial.epoch;const auto soft=weaponhand::client.soft;const auto closed=close_calls;
        weaponrestore::Journal<weaponrestore::BoundaryBackend::Snapshot> journal{};assert(!diagnostic_samples);
        assert(weaponrestore::run(h.backend,journal).outcome==weaponrestore::Outcome::denied&&!diagnostic_samples&&calls.empty());
        assert(handstream::trial.epoch==epoch&&weaponhand::client.soft==soft&&close_calls==closed&&weaponrestore::healthy());
        if(which==13){assert(!weaponrestore::prepare_forward_no_engine(h.backend.seen).known&&handstream::trial.epoch==epoch&&weaponhand::client.soft==soft);}
        ++count;
    }
    return count;
}
Mode parse(char const* text){
    const char* names[]={"normal","owned","exempt","bad_exempt","hand_false","finish_false","stow_false","residue","guard_hands","guard_finish",
        "nested","nested_poison","cpp_hands","seh_hands","cpp_finish","seh_finish","cpp_stow","seh_stow","lexical","retry","denied"};
    for(unsigned i=0;i<sizeof(names)/sizeof(names[0]);++i)if(!std::strcmp(text,names[i]))return static_cast<Mode>(i);
    assert(false);return Mode::normal;
}
}
int main(int argc,char** argv){assert(argc==2);mode=parse(argv[1]);
    if(mode==Mode::denied){const auto count=denied_cases();std::printf("WeaponRestoreBackendTest denied passed: %u exact-admission/grant/GT cases; host services only\n",count);return 0;}
    Harness h;h.prepare();parent_backend=&h.backend;weaponrestore::Journal<weaponrestore::BoundaryBackend::Snapshot> journal{};
    const auto result=weaponrestore::run(h.backend,journal);lexical_restored();assert(journal.flags&weaponrestore::cleanup_done);
    if(mode==Mode::normal||mode==Mode::owned||mode==Mode::nested||mode==Mode::retry){
        assert(result.outcome==weaponrestore::Outcome::complete&&weaponrestore::healthy());
        assert(calls==std::vector<std::string>({mode==Mode::owned?"hands:owned":"hands:external","finish:1","stow","finish:2"}));
        assert(hands_calls==1&&stow_calls==1&&finish_calls==2&&last_owned==(mode==Mode::owned));
        assert(handstream::trial.epoch==5&&vrequip::stow::epoch==8);
    }else if(mode==Mode::exempt||mode==Mode::bad_exempt){
        assert(result.outcome==(mode==Mode::exempt?weaponrestore::Outcome::exempt:weaponrestore::Outcome::unresolved)&&weaponrestore::healthy());
        assert(!hands_calls&&!finish_calls&&stow_calls==1&&vrequip::stow::pending.active()&&vrequip::stow::pending_visible&&vrequip::stow::lease.dirty());
    }else if(mode==Mode::hand_false||mode==Mode::finish_false||mode==Mode::stow_false||mode==Mode::residue){
        assert(result.outcome==weaponrestore::Outcome::unresolved&&weaponrestore::healthy()&&stow_calls==1&&finish_calls==2);
    }else{
        assert(result.outcome==weaponrestore::Outcome::unresolved&&!weaponrestore::healthy());
        if(mode==Mode::guard_hands||mode==Mode::guard_finish||mode==Mode::nested_poison)assert(!stow_calls);
        if(mode==Mode::guard_hands||mode==Mode::nested_poison)assert(!finish_calls);
        if(mode==Mode::cpp_hands||mode==Mode::seh_hands)assert(!stow_calls&&!finish_calls);
        if(mode==Mode::cpp_finish||mode==Mode::seh_finish)assert(!stow_calls&&finish_calls==1);
        if(mode==Mode::cpp_stow||mode==Mode::seh_stow)assert(stow_calls==1&&finish_calls==1);
        if(mode==Mode::cpp_hands||mode==Mode::cpp_finish||mode==Mode::cpp_stow)assert(journal.flags&weaponrestore::cpp_fault);
        if(mode==Mode::seh_hands||mode==Mode::seh_finish||mode==Mode::seh_stow)assert((journal.flags&weaponrestore::seh_fault)&&journal.seh_code==test_exception);
        if(mode==Mode::cpp_hands||mode==Mode::seh_hands||mode==Mode::cpp_finish||mode==Mode::seh_finish||mode==Mode::cpp_stow||mode==Mode::seh_stow){
            assert(handstream::trial.lease.dirty()&&handstream::trial.lease.busy()&&handstream::trial.lease.restoring()&&
                handstream::component::lease.pending&&handstream::component::lease.private_busy&&vrequip::stow::lease.dirty()&&vrequip::stow::lease.private_busy);
        }
        sticky_blocks_later(h);
    }
    if(mode==Mode::retry){const auto previous=calls;assert(weaponrestore::run(h.backend,journal).outcome==weaponrestore::Outcome::unresolved);
        assert(journal.result.outcome==weaponrestore::Outcome::unresolved&&calls==previous);sticky_blocks_later(h);}
    std::printf("WeaponRestoreBackendTest %s passed; actual stage/backend, controlled helper/grant/client effects only\n",argv[1]);
}
