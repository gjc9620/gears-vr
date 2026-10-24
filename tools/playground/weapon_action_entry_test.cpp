// Actual ENTRY controller, coordinator, writerless concrete Backend, transport,
// clients, subscribers, Admission and StatePool. Handoff inspection/revocation,
// Native exports, original and legacy body are controlled host seams only.
// No STREAM/STOW helper, initial self consumption, game or Native provider proof.
#include "../vr-common/weapon_shot_scope.h"
#include "../vr-common/hand_animation_observation.h"
#include <windows.h>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <type_traits>
#if !defined(GEARS_PLAYGROUND_WEAPON_ACTION_ENTRY_TRIAL) || !defined(GEARS_PLAYGROUND_WEAPON_HAND_PARTICIPANT_TRIAL)
#error WeaponActionEntryTest requires ENTRY and HAND_PARTICIPANT
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL) || defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
#error This fixture covers the actual no-participant Backend branch only
#endif
namespace {
#include "weapon_restore_stage.inl"
}
#include "weapon_hand_offline_fixture.h"
namespace {
namespace boundaryabi=gears_weapon_boundary;
namespace actionabi=gears_weapon_action;
std::atomic<unsigned> callbacks{};
constexpr DWORD cleanup_seh_code=0xe1530053ul;
thread_local bool fail_inside_assignment{};
unsigned cleanup_faults{};
// Host seam only: actual cleanup writes this public lexical flag. One SEH at
// that assignment exercises its real barriers without a production test hook.
struct InsideProxy {
    bool value{};
    operator bool() const noexcept {return value;}
    InsideProxy& operator=(bool next) noexcept {
        if(fail_inside_assignment){fail_inside_assignment=false;++cleanup_faults;RaiseException(cleanup_seh_code,0,0,nullptr);}
        value=next;return *this;
    }
};
thread_local InsideProxy inside;
struct Request {volatile LONG state;unsigned size;uptr object,function;unsigned char params[4096];};
void __fastcall host_original(uptr,uptr,void*);
using Event=void(__fastcall*)(uptr,uptr,void*);
Event original=host_original;
void __fastcall event_hook_legacy(uptr,uptr,void*);
bool invoke_engine(Request*);
void check_closed(bool);
namespace weaponentry {bool revoke_call(uptr,uptr,bool&) noexcept;}
#include "weapon_shot_scope.inl"
#include "weapon_call_subscriber.inl"
unsigned observer_enters{},observer_leaves{};
namespace handanim {
using Token=gears_hand_animation::Token;using Source=gears_hand_animation::Source;
struct Probe {gears_hand_animation::Ledger ledger;} probe;
Token enter(uptr,uptr,void*,Source,uptr){++observer_enters;return {};}
bool leave(Token const&,uptr,uptr,void*,uptr){++observer_leaves;return true;}
}
#include "weapon_action_subscriber.inl"
#include "weapon_action_boundary_client.inl"

constexpr uptr action_object=0x111,action_function=0x222,tick_object=0x333,tick_function=0x444;
constexpr DWORD inspect_seh_code=0xe1530051ul,original_seh_code=0xe1530052ul;
char const* mode{};char original_cpp_tag{};
struct OriginalCpp {void const* tag;unsigned value;};
struct InspectCpp {};
unsigned inspections{},local_revokes{},native_revokes{},native_opens{},native_closes{},unregisters{};
unsigned legacy_calls{},post_calls{},original_calls{},engine_calls{},stage_observations{},subscriber_begins{};
bool host_dirty{},host_forward_open=true,host_pending{},host_busy{};
std::uint64_t native_generation{8};
boundaryabi::State* native_tls{};
boundaryabi::Receipt frozen_receipt{};
boundaryabi::Finish last_finish{};
std::array<unsigned char,32> opaque_params{};
struct Dispatch {uptr object{},function{};void* params{};unsigned callbacks{};bool recursive{},bare{},inside{};};
std::array<Dispatch,8> dispatches{};
std::unique_ptr<Memory> publication_memory;
abi::GripSnapshot publication_before{};
bool is(char const* value){return !std::strcmp(mode,value);}

namespace weaponaction_handoff {
// Controlled metadata interface; actual object/name/ownership inspection is not
// copied or claimed. Dirty/busy/pending bits remain visible after host revoke.
struct Seen {
    uptr object{},function{};std::uint32_t thread{};
    bool gt{},name_known{},recognized{},unknown_name{};
    bool hands{},stow{},hands_required{},stow_required{},required{};
    bool hands_restore_echo{},stow_restore_echo{},owned_restore_echo{};
    bool hands_dirty{},component_dirty{},stow_dirty{},pending{};
    bool hands_restoration_busy{},pending_state_boundary{},hands_forward_fence_supported{};
};
Seen inspect(uptr object,uptr function){
    ++inspections;
    if(is("inspect_cpp")||is("cf_inspect_cpp"))throw InspectCpp{};
    if(is("inspect_seh")||is("cf_inspect_seh"))RaiseException(inspect_seh_code,0,0,nullptr);
    Seen seen{};seen.object=object;seen.function=function;seen.thread=GetCurrentThreadId();
    seen.gt=seen.thread==game_thread.load(std::memory_order_acquire);
    if(!seen.gt)return seen;
    seen.name_known=!is("unknown_no_scope");seen.unknown_name=!seen.name_known;
    seen.hands=object==action_object&&function==action_function;
    seen.hands_required=seen.hands;seen.required=seen.hands;
    seen.recognized=seen.hands&&seen.name_known;seen.hands_dirty=host_dirty;
    seen.hands_restoration_busy=host_busy;seen.pending=host_pending;
    seen.hands_forward_fence_supported=true;return seen;
}
void revoke_no_engine(Seen const& seen) noexcept {
    assert(seen.gt);++local_revokes;host_forward_open=false;
    // Do not clear dirty/pending/busy or pretend to restore an engine resource.
}
}
#include "weapon_restore_backend.inl"
#include "weapon_action_entry.inl"

void worker_stop(abi::Reply expected){
    std::thread worker([expected]{assert(weaponhand::stop()==expected);});worker.join();
}
abi::Reply host_unregister(std::uint64_t cookie) noexcept {
    assert(cookie==33);++unregisters;return abi::Reply::quiescent;
}
abi::Reply native_revoke(boundaryabi::Request const* request,std::uint64_t* generation) noexcept {
    assert(boundaryabi::request_valid(*request)&&request->hand_cookie==33&&request->native_started_qpc==22);
    assert(request->context==reinterpret_cast<uptr>(&weaponhand::client)&&request->thread_id==GetCurrentThreadId());
    if(publication_memory){abi::GripSnapshot old{};assert(!weaponhand::client.armed()&&!weaponhand::client.snapshot_on_gt(old));}
    ++native_revokes;*generation=++native_generation;return abi::Reply::accepted;
}
abi::Reply native_open(boundaryabi::Request const* request,boundaryabi::State* state,boundaryabi::Receipt* receipt) noexcept {
    ++native_opens;assert(boundaryabi::request_valid(*request)&&!native_tls);
    if(is("clean_no_scope")||is("dirty_no_scope")||is("owned_dirty_no_scope"))return abi::Reply::busy;
    native_tls=state;state->storage[0]=std::byte{7};
    receipt->hand_cookie=request->hand_cookie;receipt->native_started_qpc=request->native_started_qpc;
    receipt->scope_id=71;receipt->revocation_generation=native_generation;
    receipt->thread_id=request->thread_id;receipt->flags=boundaryabi::scope_active|boundaryabi::engine_calls_allowed;
    frozen_receipt=*receipt;return abi::Reply::accepted;
}
boundaryabi::End native_close(boundaryabi::State* state,boundaryabi::Receipt const* receipt,boundaryabi::Finish const* finish) noexcept {
    ++native_closes;assert(state==native_tls&&boundaryabi::pool_same_receipt(*receipt,frozen_receipt));last_finish=*finish;
    if(finish->flags!=(boundaryabi::stage_closed|boundaryabi::handback_complete)||finish->outcome==boundaryabi::Outcome::notification_fault)
        return boundaryabi::End::unresolved;
    native_tls=nullptr;*state={};return boundaryabi::End::closed;
}
void connect(){
    game_thread.store(0);assert(weaponhand::client.begin_connect({11,GetCurrentProcessId()})==abi::Reply::accepted);
    abi::RegistrationResult registration{};registration.cookie=33;registration.native_started_qpc=22;
    assert(weaponhand::client.connected(abi::Reply::accepted,registration)==abi::Reply::accepted);
    weaponcall::attached_cookie.store(33);weaponcall::attached_native.store(22);
    weaponaction::attached_cookie.store(33);weaponaction::attached_native.store(22);
    weaponhand::native_module=LoadLibraryW(L"kernel32.dll");assert(weaponhand::native_module);
    weaponhand::host_unregister=host_unregister;
    assert(weaponboundary::admission.connect({33,22,&weaponhand::client,native_revoke,native_open,native_close}));
    game_thread.store(GetCurrentThreadId());
}
void arm_publication(){
    // Reuse the existing artificial metadata fixture and actual client callbacks.
    // The Frame/decision below are host inputs, not a Native owner receipt.
    publication_memory=std::make_unique<Memory>();auto& frame=publication_memory->frame;
    frame.key={11,22,33,1,1,1,GetCurrentProcessId(),GetCurrentThreadId()};
    frame.flags=abi::scene_owned|abi::input_matched|abi::gameplay_interactable;
    frame.now_ns=weaponhand::clock_ns(nullptr);frame.reference_generation=1;
    frame.stereo_status=unsigned(gears_vr::StereoStatus::accepted);
    auto& stereo=frame.stereo;stereo.session=1;stereo.sequence=100;stereo.received_ns=frame.now_ns-1000000;
    stereo.predicted_time=123;stereo.head_flags=stereo.view_flags=15;stereo.focused=stereo.visible=stereo.should_render=1;
    stereo.head={{0,1.6,0},{0,0,0,1}};stereo.eyes[0].pose.position={-.032,1.6,0};stereo.eyes[1].pose.position={.032,1.6,0};
    for(auto& eye:stereo.eyes){eye.pose.orientation={0,0,0,1};eye.fov={-.8,.8,.8,-.8};}
    auto& controllers=frame.controllers;controllers.session=1;controllers.sequence=100;controllers.space_epoch=stereo.space_epoch;
    controllers.predicted_time=123;controllers.focused=1;controllers.received_ns=stereo.received_ns;controllers.sync_ns=controllers.received_ns-1;
    for(auto& hand:controllers.hands){hand.grip_active=1;hand.grip_flags=15;}
    controllers.hands[1].grip.position={0,1,0};controllers.hands[0].grip.position={0,1,-.5};
    frame.left_grip_known=frame.left_tracked=1;
    const auto registration=weaponhand::client.registration();abi::Proposal proposal{};
    assert(registration.prepare(registration.context,&frame,&proposal)==unsigned(abi::Reply::accepted));
    abi::Decision decision{};decision.key=frame.key;decision.proposal_nonce=proposal.nonce;decision.now_ns=frame.now_ns;
    decision.status=abi::DecisionStatus::no_candidate;
    assert(registration.commit(registration.context,&frame,&proposal,&decision)==unsigned(abi::Reply::accepted));
    assert(weaponhand::client.armed()&&weaponhand::client.snapshot_on_gt(publication_before)&&publication_before.grip==abi::Grip::single);
}
void check_publication_revoked(){
    assert(publication_memory&&!weaponhand::client.armed());abi::GripSnapshot old{};
    assert(publication_before.current(publication_memory->frame.key,publication_memory->frame.weapon,weaponhand::clock_ns(nullptr)));
    assert(!weaponhand::client.snapshot_on_gt(old));
}
void observe_actual_stage(weaponentry::Entry& entry){
    assert(!weaponrestore::active()&&entry.stage_closed);
    if(!(entry.stage.flags&weaponrestore::consumed))return;
    ++stage_observations;
    assert(entry.stage.flags==(weaponrestore::created|weaponrestore::consumed|weaponrestore::entered|weaponrestore::body_returned|weaponrestore::cleanup_done));
    assert(entry.stage.result.outcome==weaponrestore::Outcome::complete&&entry.stage.result.hands==weaponrestore::Step::skipped&&
        entry.stage.result.stow==weaponrestore::Step::skipped&&entry.stage.result.hands_finished&&entry.stage.result.all_finished);
    assert(entry.backend.transport==&entry.transport&&entry.backend.lifetime==&entry.transport.entry);
    assert(entry.backend.stage_address==reinterpret_cast<uptr>(&entry.stage)&&entry.transport.stage_address==entry.backend.stage_address);
    assert(weaponboundary::stage_grant_matches(entry.transport,entry.backend.stage_address)&&entry.backend.admissible());
    boundaryabi::PoolDiagnostic diagnostic{};assert(weaponboundary::states.describe(entry.transport.cell,diagnostic));
    assert(diagnostic.phase==boundaryabi::PoolPhase::active&&diagnostic.receipt_frozen&&diagnostic.state==native_tls);
    assert(boundaryabi::pool_same_receipt(diagnostic.receipt,entry.transport.receipt));
    const auto exact_stage=entry.backend.stage_address;entry.backend.stage_address=reinterpret_cast<uptr>(&entry.lexical);
    assert(!entry.backend.admissible());entry.backend.stage_address=exact_stage;
}
void subscriber_call(bool related,bool dirty=false,bool wrong_cookie=false,bool wrong_native=false,bool prep_fault=false){
    actionabi::Invocation call{};call.hand_cookie=wrong_cookie?34u:33u;call.native_started_qpc=wrong_native?23u:22u;
    call.gate_id=101;call.thread_id=GetCurrentThreadId();call.self=related?action_object:tick_object;
    call.callee=related?action_function:tick_function;call.caller_frame=0x777;call.result=0x888;
    actionabi::State state{};const auto before_callbacks=callbacks.load();auto* const owner=weaponentry::top;
    host_dirty=dirty;const auto begin=weaponaction::begin(&weaponhand::client,&call,&state);++subscriber_begins;
    assert(callbacks.load()==before_callbacks+1&&weaponaction::transactions.load()==1&&weaponaction::top==&state&&weaponaction::depth==1);
    assert(weaponentry::top==owner);
    const bool unresolved=dirty||prep_fault;
    assert(begin==(unresolved?actionabi::BeginReply::handback_unresolved:actionabi::BeginReply::unrelated));
    if(is("cf_original_tick")){
        // The real subscriber State remains linked across the controlled CF
        // original gap. Its nested PE must independently take the bare route.
        weaponentry::process_event(tick_object,tick_function,opaque_params.data());
        assert(weaponaction::top==&state&&weaponaction::depth==1&&callbacks.load()==before_callbacks+1);
    }
    const auto ended=weaponaction::returned(&weaponhand::client,&call,&state,0x12345678);
    assert(ended==(unresolved?actionabi::End::unresolved:actionabi::End::closed));
    assert(callbacks.load()==before_callbacks&&!weaponaction::transactions.load()&&!weaponaction::top&&!weaponaction::depth);
    assert(weaponentry::top==owner&&host_dirty==dirty);
}
void subscriber_nested_permission(){
    actionabi::Invocation outer{};outer.hand_cookie=33;outer.native_started_qpc=22;outer.gate_id=201;
    outer.thread_id=GetCurrentThreadId();outer.self=tick_object;outer.callee=tick_function;
    actionabi::State outer_state{};
    assert(weaponaction::begin(&weaponhand::client,&outer,&outer_state)==actionabi::BeginReply::unrelated);
    const auto before_inspections=inspections,before_observers=observer_enters;
    auto nested=outer;nested.gate_id=202;nested.self=action_object;nested.callee=action_function;nested.flags=actionabi::engine_calls_allowed;
    actionabi::State nested_state{};
    assert(weaponaction::begin(&weaponhand::client,&nested,&nested_state)==actionabi::BeginReply::handback_unresolved);
    assert(weaponaction::depth==2&&weaponaction::top==&nested_state&&callbacks.load()==2&&weaponaction::transactions.load()==2);
    assert(inspections==before_inspections&&observer_enters==before_observers&&!local_revokes&&!native_revokes&&!native_opens&&!engine_calls);
    assert(weaponaction::faulted.load()&&weaponrestore::healthy());
    assert(weaponaction::returned(&weaponhand::client,&nested,&nested_state,0)==actionabi::End::unresolved);
    assert(weaponaction::depth==1&&weaponaction::top==&outer_state&&callbacks.load()==1&&weaponaction::transactions.load()==1);
    assert(weaponaction::returned(&weaponhand::client,&outer,&outer_state,0)==actionabi::End::closed);
    check_closed(false);
    const auto held=weaponhand::native_module;worker_stop(abi::Reply::draining);assert(held&&weaponhand::native_module==held&&weaponrestore::healthy());
}
void __fastcall host_original(uptr object,uptr function,void* params){
    auto* const entry=weaponentry::top;assert(entry&&entry->object==object&&entry->function==function&&entry->params==params);
    assert(entry->original_started&&!entry->original_returned&&original_calls<dispatches.size());
    const auto entry_callbacks=callbacks.load();
    dispatches[original_calls++]={object,function,params,entry_callbacks,entry->recursive,entry->bare,inside};
    static_cast<unsigned char*>(params)[2]=0x5a;
    if(entry->prepared)observe_actual_stage(*entry);
    if(original_calls==1){
        if(is("recursive_same"))weaponentry::process_event(object,function,params);
        if(is("recursive_tick"))weaponentry::process_event(tick_object,tick_function,params);
        if(is("post_nested_revoke"))weaponentry::process_event(action_object,action_function,params);
        if(is("post_cf_revoke"))subscriber_call(true);
        assert(weaponentry::top==entry&&callbacks.load()==entry_callbacks);
        if(is("stop_gap")){
            const auto held=weaponhand::native_module;assert(held&&weaponboundary::admission.in_flight()==1);
            worker_stop(abi::Reply::draining);
            assert(weaponhand::native_module==held&&weaponhand::client.phase()==abi::ClientPhase::draining&&!native_closes);
        }
        if(is("stage_retry")){
            const auto original_flags=entry->stage.flags;
            const auto retried=weaponrestore::run(entry->backend,entry->stage);
            assert(retried.outcome==weaponrestore::Outcome::unresolved&&entry->stage.flags==original_flags&&!weaponrestore::healthy());
            assert(!weaponrestore::active()&&native_opens==1&&stage_observations==1);
        }
        if(is("actual_wrong_journal")){
            const auto original_flags=entry->stage.flags,transport_flags=entry->transport.flags;
            assert(entry->backend.admissible());
            weaponrestore::Journal<weaponrestore::BoundaryBackend::Snapshot> different{};
            assert(entry->backend.stage_address!=reinterpret_cast<uptr>(&different));
            const auto rejected=weaponrestore::run(entry->backend,different);
            assert(rejected.outcome==weaponrestore::Outcome::unresolved&&entry->backend.guard_mismatch&&!weaponrestore::healthy());
            assert((different.flags&(weaponrestore::consumed|weaponrestore::entered|weaponrestore::cleanup_done))==
                (weaponrestore::consumed|weaponrestore::entered|weaponrestore::cleanup_done));
            assert(!(different.flags&weaponrestore::body_returned)&&!rejected.hands_finished&&!rejected.all_finished);
            assert(entry->stage.flags==original_flags&&entry->transport.flags==transport_flags&&!weaponrestore::active());
            assert(native_opens==1&&!native_closes&&stage_observations==1);
        }
        if(is("actual_wrong_request")||is("actual_wrong_receipt")){
            const auto request_copy=entry->backend.request;const auto receipt_copy=entry->backend.receipt;
            const auto original_flags=entry->stage.flags,transport_flags=entry->transport.flags;
            // Both edits remain wire-shaped; only the Backend copy changes.
            // Existing object/thread/cookie checks alone cannot bind these.
            if(is("actual_wrong_request"))entry->backend.request.source=boundaryabi::Source::owned_bridge;
            else ++entry->backend.receipt.scope_id;
            assert(boundaryabi::request_valid(entry->backend.request)&&entry->backend.receipt.scope_id);
            assert(!entry->backend.admissible()&&weaponrestore::healthy());
            assert(weaponboundary::stage_grant_matches(entry->transport,entry->backend.stage_address));
            assert(boundaryabi::pool_same_request(entry->transport.request,request_copy)&&
                boundaryabi::pool_same_receipt(entry->transport.receipt,receipt_copy));
            entry->backend.request=request_copy;entry->backend.receipt=receipt_copy;assert(entry->backend.admissible());
            assert(entry->stage.flags==original_flags&&entry->transport.flags==transport_flags&&native_opens==1&&!native_closes);
        }
    }
    if(is("cleanup_seh")||is("cleanup_seh_original_cpp")||is("cleanup_seh_original_seh"))fail_inside_assignment=true;
    if(is("original_bare_cpp")||is("original_related_cpp")||is("cleanup_seh_original_cpp"))throw OriginalCpp{&original_cpp_tag,531};
    if(is("original_seh_inside"))inside=true;
    if(is("original_bare_seh")||is("original_related_seh")||is("original_seh_inside")||is("cleanup_seh_original_seh")){
        const ULONG_PTR details[]={0x1234,0x5678};RaiseException(original_seh_code,0,2,details);
    }
}
void __fastcall event_hook_legacy(uptr object,uptr function,void* params){
    ++legacy_calls;assert(weaponentry::top&&!weaponentry::top->bare&&!weaponentry::top->recursive);
    if(is("ui_consumed"))return;
    weaponentry::legacy_original(object,function,params);
    if(weaponentry::post_allowed())++post_calls;
}
bool invoke_engine(Request* request){
    ++engine_calls;assert(request);host_original(request->object,request->function,request->params);return !is("owned_false");
}
unsigned caught_seh{};std::array<ULONG_PTR,2> caught_details{};
int outside_filter(EXCEPTION_POINTERS* exception) noexcept {
    if(exception->ExceptionRecord->ExceptionCode!=original_seh_code)return EXCEPTION_CONTINUE_SEARCH;
    ++caught_seh;assert(exception->ExceptionRecord->NumberParameters==2);
    caught_details[0]=exception->ExceptionRecord->ExceptionInformation[0];caught_details[1]=exception->ExceptionRecord->ExceptionInformation[1];
    return EXCEPTION_EXECUTE_HANDLER;
}
bool call_seh_outside(uptr object,uptr function,void* params){
    __try{weaponentry::process_event(object,function,params);return false;}
    __except(outside_filter(GetExceptionInformation())){return true;}
}
void check_closed(bool saved_inside){
    assert(!callbacks.load()&&!weaponentry::top&&!weaponrestore::active()&&inside==saved_inside);
    assert(!weaponaction::top&&!weaponaction::depth&&!weaponaction::transactions.load());
}
void check_end(bool faulted){
    if(faulted){
        assert(!weaponrestore::healthy());const auto held=weaponhand::native_module;assert(held);
        worker_stop(abi::Reply::draining);assert(weaponhand::native_module==held);
    }else{
        assert(weaponrestore::healthy()&&!weaponboundary::states.retained()&&!native_tls&&!weaponboundary::admission.in_flight());
        worker_stop(abi::Reply::quiescent);assert(!weaponhand::native_module&&weaponhand::client.phase()==abi::ClientPhase::off);
    }
}
}
int main(int argc,char** argv){
    _set_error_mode(_OUT_TO_STDERR);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    assert(argc==2);mode=argv[1];
    constexpr char const* entry_cases[]={"accepted","legacy_tick","ui_consumed","recursive_same","recursive_tick","post_nested_revoke","post_cf_revoke",
        "inside_bare","disabled_bare","original_bare_cpp","original_bare_seh","original_related_cpp","original_related_seh","original_seh_inside",
        "stop_gap","stage_retry","actual_wrong_journal","actual_wrong_request","actual_wrong_receipt","clean_no_scope","dirty_no_scope","unknown_no_scope","inspect_cpp","inspect_seh",
        "cleanup_seh","cleanup_seh_original_cpp","cleanup_seh_original_seh","cf_related","cf_dirty","cf_unrelated","cf_wrong_cookie","cf_wrong_native",
        "cf_original_tick","cf_inspect_cpp","cf_inspect_seh","cf_nested_permission","owned_accept","owned_false","owned_dirty_no_scope"};
    bool known{};for(auto const entry_case:entry_cases)known=known||!std::strcmp(mode,entry_case);assert(known);
    connect();opaque_params[2]=7;
    if(is("cf_related")||is("clean_no_scope"))arm_publication();
    if(is("cf_nested_permission"))subscriber_nested_permission();
    else if(is("cf_related")||is("cf_dirty")||is("cf_unrelated")||is("cf_wrong_cookie")||is("cf_wrong_native")||
       is("cf_original_tick")||is("cf_inspect_cpp")||is("cf_inspect_seh")){
        const bool dirty=is("cf_dirty"),related=!is("cf_unrelated")&&!is("cf_original_tick");
        const bool prep_fault=is("cf_inspect_cpp")||is("cf_inspect_seh");
        subscriber_call(related,dirty,is("cf_wrong_cookie"),is("cf_wrong_native"),prep_fault);check_closed(false);
        const bool exact_related=is("cf_related")||dirty;
        assert(original_calls==(is("cf_original_tick")?1u:0u)&&!legacy_calls&&!engine_calls&&!native_opens&&!native_closes);
        assert(inspections==(is("cf_original_tick")?2u:exact_related||is("cf_unrelated")||prep_fault?1u:0u));
        assert(local_revokes==(exact_related?1u:0u)&&observer_enters==(is("cf_unrelated")||is("cf_original_tick")?1u:0u)&&!observer_leaves);
        if(is("cf_original_tick"))assert(dispatches[0].recursive&&dispatches[0].bare&&dispatches[0].callbacks==2&&dispatches[0].params==opaque_params.data());
        assert((native_revokes!=0)==exact_related&&host_dirty==dirty);
        if(publication_memory)check_publication_revoked();check_end(dirty||prep_fault);
    }else if(is("owned_accept")||is("owned_false")||is("owned_dirty_no_scope")){
        Request request{};request.object=action_object;request.function=action_function;request.params[2]=7;
        host_dirty=is("owned_dirty_no_scope");const auto returned=weaponentry::invoke_owned(&request);check_closed(false);
        assert(returned==is("owned_accept"));
        if(host_dirty){assert(!engine_calls&&!original_calls&&!stage_observations&&!host_forward_open&&host_dirty);}
        else{
            assert(engine_calls==1&&original_calls==1&&stage_observations==1&&native_opens==1&&native_closes==1);
            assert(dispatches[0].object==request.object&&dispatches[0].function==request.function&&dispatches[0].params==request.params&&request.params[2]==0x5a);
        }
        assert(!legacy_calls);check_end(host_dirty);
    }else{
        const bool unrelated=is("legacy_tick")||is("ui_consumed")||is("post_nested_revoke")||is("post_cf_revoke")||
            is("inside_bare")||is("disabled_bare")||is("original_bare_cpp")||is("original_bare_seh");
        const auto object=unrelated?tick_object:action_object,function=unrelated?tick_function:action_function;
        inside=is("inside_bare")||is("original_bare_cpp")||is("original_bare_seh");const bool saved_inside=inside;
        if(is("disabled_bare"))enabled.store(false,std::memory_order_release);
        host_dirty=is("dirty_no_scope");host_busy=host_dirty;host_pending=host_dirty;
        bool cpp_caught{};
        if(is("original_bare_cpp")||is("original_related_cpp")||is("cleanup_seh_original_cpp")){
            try{weaponentry::process_event(object,function,opaque_params.data());}
            catch(OriginalCpp const& error){assert(error.tag==&original_cpp_tag&&error.value==531);cpp_caught=true;}
            assert(cpp_caught);
        }else if(is("original_bare_seh")||is("original_related_seh")||is("original_seh_inside")||is("cleanup_seh_original_seh")){
            assert(call_seh_outside(object,function,opaque_params.data()));
            assert(caught_seh==1&&caught_details[0]==0x1234&&caught_details[1]==0x5678);
        }else weaponentry::process_event(object,function,opaque_params.data());
        check_closed(saved_inside);
        if(is("ui_consumed"))assert(legacy_calls==1&&!original_calls&&opaque_params[2]==7);
        else{
            assert(original_calls>=1&&dispatches[0].object==object&&dispatches[0].function==function&&dispatches[0].params==opaque_params.data());
            assert(dispatches[0].callbacks==1&&opaque_params[2]==0x5a);
        }
        if(is("recursive_same")||is("recursive_tick")||is("post_nested_revoke")){
            assert(original_calls==2&&dispatches[1].recursive&&dispatches[1].bare&&dispatches[1].callbacks==2&&dispatches[1].params==opaque_params.data());
            assert(dispatches[1].object==(is("recursive_tick")?tick_object:action_object)&&dispatches[1].function==(is("recursive_tick")?tick_function:action_function));
        }else assert(original_calls==(is("ui_consumed")?0u:1u));
        if(is("legacy_tick"))assert(legacy_calls==1&&post_calls==1&&!native_opens&&!native_revokes);
        else if(is("post_nested_revoke")||is("post_cf_revoke"))assert(legacy_calls==1&&!post_calls&&native_revokes==1&&!native_opens&&weaponrestore::healthy());
        else if(!is("ui_consumed"))assert(!legacy_calls&&!post_calls);
        if(is("accepted")||is("recursive_same")||is("recursive_tick")||is("stop_gap")||is("original_related_cpp")||is("original_related_seh")||
           is("actual_wrong_request")||is("actual_wrong_receipt"))
            assert(stage_observations==1&&native_opens==1&&native_closes==1);
        if(is("original_related_cpp")||is("original_related_seh"))assert(last_finish.outcome==boundaryabi::Outcome::original_exception);
        if(is("dirty_no_scope"))assert(host_dirty&&host_busy&&host_pending&&!host_forward_open&&!stage_observations&&!native_closes&&native_opens==1);
        if(is("clean_no_scope"))assert(!host_dirty&&!stage_observations&&native_opens==1&&!native_closes&&!host_forward_open);
        if(is("unknown_no_scope"))assert(!native_opens&&!stage_observations&&local_revokes==1&&!host_forward_open);
        if(is("inspect_cpp")||is("inspect_seh"))assert(!native_revokes&&!native_opens&&!stage_observations);
        if(is("original_seh_inside"))assert(native_tls&&native_tls->storage[0]==std::byte{7}&&weaponboundary::states.isolated()==1&&!native_closes);
        const bool cleanup_seh=is("cleanup_seh")||is("cleanup_seh_original_cpp")||is("cleanup_seh_original_seh");
        if(cleanup_seh)assert(cleanup_faults==1&&!fail_inside_assignment&&native_tls&&native_tls->storage[0]==std::byte{7}&&
            weaponboundary::states.isolated()==1&&!weaponboundary::admission.in_flight()&&!native_closes&&weaponboundary::admission.faulted());
        const bool faulted=is("dirty_no_scope")||is("unknown_no_scope")||is("inspect_cpp")||is("inspect_seh")||is("stage_retry")||
            is("actual_wrong_journal")||is("original_seh_inside")||cleanup_seh;
        if(publication_memory)check_publication_revoked();check_end(faulted);
    }
    std::printf("WeaponActionEntryTest %s passed; actual writerless controller/backend/transport/client/subscribers; host handoff/Native/original/legacy seams only\n",mode);
    return 0;
}
