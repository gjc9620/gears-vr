#include "held_outcome_journal.h"
#include "ready_wrist_entry_journal.h"
#include "full_body_c0_native_backend.h"
#include "body_mount_commit.h"
#include "native_atom_overwrite_live.h"
#include "full_body_c0_first_failure.h"
#include "body_c0_arm_alpha_policy.h"
#include "body_head_authored_mount.h"
#include "../vr-common/weapon_main_mount_provider.h"
namespace rig_tracking_live {bool retained_core_identity(gears_native_rig_tracking::CoreSnapshot const&)noexcept;}
namespace cover_control_observer_live {bool retained_boundary_current()noexcept;}
namespace full_body_c0_native_live {
namespace budget=::gears_body_c0_budget;
namespace held_outcome=::gears_body_held_outcome;
namespace native=gears_full_body_c0_native;namespace tx=gears_full_body_c0_transaction;
namespace body=gears_full_body_c0;namespace hand=continuous_hand_owner_live;
namespace phase=cover_pose_phase_live;namespace binding=weapon_hand_scene_binding;
using Entry=gears_continuous_hand_entry::Entry;
bool enabled{};
#if defined(GEARS_FULL_BODY_C0_NATIVE_BACKEND) && GEARS_FULL_BODY_C0_NATIVE_BACKEND
namespace hands=gears_body_c0_hands;
namespace main_mount=gears_weapon_main_mount_provider;
namespace body_mount=gears_body_main_mount_provider;
namespace ready_wrist {void source_rejected(Entry const*,tx::Stage,tx::Verdict,unsigned) noexcept;
void alpha_event(gears_body_c0_arm_alpha::Diagnostic const&) noexcept;}
bool hands_enabled{};
bool arm_alpha_enabled{};
bool held_main_mount{},diagnostic_held_main_mount{};
std::atomic<uint64_t> main_mount_attempts{},main_mount_ready{},main_mount_not_ready{},main_mount_context_rejected{},main_mount_stale{},main_mount_fault{},main_mount_commits{},main_mount_last_query{},main_mount_max_ns{};
std::array<std::atomic<uint64_t>,unsigned(gears_weapon_hand::MainMountQueryResult::Branch::count)> main_mount_branches{};
std::atomic<uint64_t> main_mount_stale_projection{},main_mount_stale_broker{},main_mount_stale_c0{},main_mount_post_admission_rejected{},main_mount_source_failure_query{};
std::atomic<unsigned> main_mount_last_callback_status{UINT32_MAX},main_mount_last_source_failure_mask{},main_mount_c0_rejection{};
std::array<std::atomic<int64_t>,6> main_mount_last_failure_ages{};
std::atomic<uint64_t> arm_alpha_owner_calls{},arm_alpha_owner_granted{},arm_alpha_owner_rejected{},arm_alpha_threshold_rejected{};
std::atomic<uint64_t> hands_attempts{},hands_solved{},hands_commits{};
std::array<std::atomic<uint64_t>,unsigned(hands::Status::residual)+1> hands_statuses{};
std::atomic<bool> stopping{};std::atomic<unsigned> callbacks{},obligations{};
std::atomic<uint64_t> captures{},commits{},rejected{};std::atomic<DWORD> owner_thread{};
struct ActualContext final:native::Context {
    Entry const* entry{};Entry copy{};gears_cover_pose_phase::OwnerKey owner{};
    gears_native_rig_tracking::CoreSnapshot source{};uint64_t tick{};bool expired{true};
    Entry const* tick_entry{};Entry tick_copy{};::Scope* world_lease{};uint64_t conflict{};
    binding::Binding actor{};gears_anchor::Identity asset{};
    gears_body_tick_input::Input body_input{};int64_t basis_ns{};body_tick_input_live::Parent mount_parent{};
    gears_body_tick_source::Input body_source{};body_tick_source_live::Parent source_parent{},queried_parent{};
    tx::Verdict rejection_{tx::Verdict::denied};
    tx::Verdict rejection() const noexcept override{return rejection_;}
    bool retained_current() {auto const& f=hand::slot;auto const& o=f.data.owner;
        return f.active&&o.valid&&f.tick_depth==1&&f.tick_address==tick_entry&&tick_entry&&
            !std::memcmp(tick_entry,&tick_copy,sizeof tick_copy)&&f.lease==world_lease&&world_lease&&world_lease->world()&&
            tick_copy.invocation==tick&&tick_copy.thread==copy.thread&&f.key.outer_tick==tick&&f.data.record.tick==tick&&
            f.compose.outer_tick==tick&&f.compose.scope.invocation==copy.invocation&&f.compose.scope.arguments&&
            o.thread==copy.thread&&GetCurrentThreadId()==f.key.request.native.game_thread&&
            f.conflict==conflict&&hand::conflicts.load()==conflict&&hand::matching_ticks.load()==1&&
            owner.world_generation==committed_world_generation.load()&&owner.session==uint64_t(session_started_qpc)&&
            binding::same_key(actor,o.binding)&&actor.scene_invocation==o.binding.scene_invocation&&actor.special_move==o.binding.special_move&&
            binding::same(asset,f.data.asset)&&binding::current_actor_retained(actor)&&hand::tick_target(owner.mesh.address);
    }
    bool current(tx::Stage stage) noexcept override {budget::Span measured(budget::Op::context);try{
        rejection_=tx::Verdict::denied;
        // Consumption notification and finally authenticate the retained old
        // scope. Neither can grant a new write, or require an already-burned
        // source latch to become fresh again.
        const bool retained=stage==tx::Stage::rollback||stage==tx::Stage::scope_end||stage==tx::Stage::consumer_entry;
        if(!retained&&stopping.load())return false;
        if(stage==tx::Stage::commit&&!native_atom_overwrite_live::ready_for_commit())return false;
        if(expired||!entry||GetCurrentThreadId()!=copy.thread||
           !(retained?retained_current():hand::owner_current())||
           hand::slot.compose_depth!=1||hand::slot.compose_address!=entry||!hand::slot.compose.admitted||
           hand::slot.tick!=tick||hand::slot.data.record.compose!=copy.invocation||
           !phase::same(owner,hand::slot.data.owner.key)||std::memcmp(&source,&hand::slot.data.record.source,sizeof source)||
           std::memcmp(entry,&copy,sizeof copy)||!hand::metadata(false)||hand::slot.data.signature!=body::rig::marcus115_signature||
           !fresh_pose_producer_live::certified||!fresh_pose_producer_live::own_entries()||!fresh_pose_producer_live::caller_current(0)||
           !fresh_pose_producer_live::caller_current(1))return false;
        if(!(copy.kind==0&&copy.selected&&copy.compose_depth&&copy.depth==1&&
            copy.ra==base+gears_cover_pose_phase::compose_caller&&copy.rcx==owner.mesh.address&&copy.rdx==copy.rcx+0x27c&&copy.r8==copy.rcx+0x28c))return false;
        if(!retained){
            // Explicit two-source contract. Authenticate every immutable body
            // dependency before classifying elapsed time; hard still dominates.
            if(body_tick_source_live::enabled){
                const auto verdict=body_tick_source_live::current(body_source,source,tick,copy.thread,basis_ns);
                if(verdict==gears_body_tick_input::Verdict::hard)return false;
                if(queried_parent.cookie&&!body_tick_source_live::parent_idle_current(queried_parent))return false;
                bool equipment_fresh=true;
                if(source_parent.query){
                    if(!body_tick_source_live::parent_idle_current(source_parent))return false;
                    const auto now=rig_tracking_live::clock_ns();
                    if(source_parent.equipment_begin_ns<=0||source_parent.equipment_end_ns<source_parent.equipment_begin_ns||
                       source_parent.equipment_end_ns>now)return false;
                    equipment_fresh=uint64_t(now-source_parent.equipment_begin_ns)<=body_mount::equipment_age_ns;
                    if(!body_tick_source_live::parent_idle_current(source_parent))return false;
                }
                if(!equipment_fresh||verdict!=gears_body_tick_input::Verdict::okay){rejection_=tx::Verdict::source_age_expired;
                    ready_wrist::source_rejected(entry,stage,rejection_,unsigned(verdict==gears_body_tick_input::Verdict::source_age_only)|unsigned(!equipment_fresh)*2);
                    return false;}
                return true;
            }
            const auto body_verdict=body_tick_input_live::enabled?
                body_tick_input_live::current(body_input,source,tick,copy.thread,basis_ns):gears_body_tick_input::Verdict::okay;
            if(body_verdict==gears_body_tick_input::Verdict::hard)return false;
            if(body_tick_input_live::enabled&&mount_parent.query&&!body_tick_input_live::parent_current(mount_parent))return false;
            rig_tracking_live::TickFailure why{};const bool parent_fresh=hand::same_source(&why);
            if(!parent_fresh&&why!=rig_tracking_live::TickFailure::source_age_only)return false;
            if(!parent_fresh||body_verdict==gears_body_tick_input::Verdict::source_age_only){rejection_=tx::Verdict::source_age_expired;return false;}}

        return true;
    }catch(...){return false;}}
};
struct State {
    ActualContext context{};native::Backend backend{};tx::Kernel kernel{};tx::Lease lease{};
    body::Frame source{};body::Output output{};body::Workspace workspace{};body::Model model{};
    tx::Desired desired{};tx::Stamp stamp{};bool captured{},prepared{},notified{},atom_installed{},joint_committed{},consumer_verified{};
    uint32_t alpha_grant_mask{},alpha_grant_count{};std::array<uint64_t,4> alpha_grant_nodes{};std::array<uint32_t,4> alpha_grant_original{};
    hands::Workspace hand_work{};
    hands::adapter::MountKind right_mount{hands::adapter::MountKind::free_hand};
    main_mount::Response mount_response{};
    body_mount::Source mount_source{};body_mount::Response body_mount_response{};
    bool body_query_active{};
    main_mount::BodyGeometry mount_body{};
    gears_rig_tracking::Frame mount_rig{};
    uint64_t hand_record{};gears_vr::StereoMatrix body_render_base{};
    struct Recovery {
        binding::Binding actor{};gears_anchor::Identity asset{};gears_native_rig_tracking::CoreSnapshot source{};
        gears_cover_pose_phase::Metadata metadata{};tx::Stamp stamp{};tx::Desired desired{};
        std::array<body::rig::ReferenceRecord68,body::bones> reference{};double units{},model_scale{};bool active{};
    } recovery;
};
// One registered GT owner, persistent storage so unknown end never destroys debt.
State state{};
// This private callback is installed by Native, never supplied by PG. The
// broker invokes it outside its mutex with its own callback reservation live.
// source_parent remains empty until the returned query is accepted, avoiding
// the NORMAL idle-parent check while this exact query owns one callback.
bool body_mount_policy_enabled() noexcept {return body_tick_source_live::enabled&&main_mount_policy_enabled();}
bool body_mount_source_current(body_mount::Source const& input) noexcept {
    auto& journal=held_outcome::recorder;const auto ordinal=journal.guard_begin();
    using Gate=held_outcome::SourceGate;
    const auto fail=[&](Gate gate){journal.guard_end(ordinal,gate);return false;};
    try{
        // Exact original short-circuit order. These are observations of the
        // actual evaluations, never extra guard calls or alternate permission.
        if(!body_mount_policy_enabled())return fail(Gate::policy);
        if(GetCurrentThreadId()!=owner_thread.load())return fail(Gate::thread);
        if(!state.body_query_active)return fail(Gate::query_scope);
        if(!state.captured)return fail(Gate::captured);
        if(state.prepared)return fail(Gate::prepared);
        if(state.notified)return fail(Gate::notified);
        if(state.context.source_parent.query)return fail(Gate::accepted_parent);
        if(state.context.queried_parent.cookie)return fail(Gate::queried_parent);
        if(std::memcmp(&input,&state.mount_source,sizeof input))return fail(Gate::input_bytes);
        if(input.scene_ticket!=state.context.source.source_ticket)return fail(Gate::scene_ticket);
        if(input.scene_cache_revision!=state.context.body_source.scene_cache_revision)return fail(Gate::cache_revision);
        if(input.basis.compose!=state.context.copy.invocation)return fail(Gate::compose);
        if(input.basis.capture_begin_ns!=state.context.basis_ns)return fail(Gate::capture);
        if(std::memcmp(&input.basis.component_to_world,&state.source.component_to_world,sizeof input.basis.component_to_world))return fail(Gate::component);
        if(std::memcmp(&input.basis.base_view,&state.body_render_base,sizeof input.basis.base_view))return fail(Gate::view);
        if(input.basis.geometry.geometry!=state.model.signature)return fail(Gate::geometry);
        if(input.basis.geometry.model_scale!=state.source.model_scale)return fail(Gate::model_scale);
        if(input.basis.geometry.units_per_metre!=state.source.units_per_metre)return fail(Gate::units);
        const bool current=state.context.current(tx::Stage::prepare);
        journal.guard_end(ordinal,current?Gate::okay:Gate::context,current?held_outcome::unknown:uint64_t(state.context.rejection()));
        return current;
    }catch(...){return fail(Gate::exception);}}

namespace failure=gears_full_body_c0_first_failure;
bool diagnostic_first_failure{};
// Bounded observation only. The GT writes owned POD after a real joint commit;
// the collector exports once, after drain and all body obligations are zero.
// No target record is a source/permission or a producer-retirement receipt.
constexpr unsigned hand_trace_capacity=32;
constexpr unsigned hand_trace_bones[9]={8,10,11,32,34,35,51,52,55};
struct HandTraceRecord {uint64_t sequence{},tick{},compose{},qpc{},publication{},source_ns{},mesh{};
    uint32_t pose_epoch{},normal_return{};body::Pose wrist[2]{};
    uint64_t consumer_qpc{},end_begin_qpc{},end_end_qpc{};
    uint32_t consumer_verified{},consumer_status{},end_status{},end_pose_epoch{};
    body::Atom before[9]{},planned[9]{},consumer[9]{},after_local[9]{};
    body::Matrix64 after_space[9]{},component{};body::Pose head_goal{};
    uint32_t alpha_grant_mask{},alpha_grant_count{};uint64_t alpha_grant_nodes[4]{};uint32_t alpha_grant_original[4]{};};
struct HandTraceHeader {char magic[8]{'C','0','H','A','N','D','0','1'};
    uint32_t version{3},header_bytes{sizeof(HandTraceHeader)},record_bytes{sizeof(HandTraceRecord)},count{},capacity{hand_trace_capacity},reserved{};
    uint64_t first{},last{},pid{},session{},qpc_frequency{};};
static_assert(sizeof(body::Pose)==56&&sizeof(HandTraceRecord)==2120&&sizeof(HandTraceHeader)==72);
std::array<HandTraceRecord,hand_trace_capacity> hand_trace{};uint64_t hand_trace_sequence{};
std::atomic<bool> hand_trace_attempted{},hand_trace_written{};std::atomic<unsigned> hand_trace_error{};
std::array<gears_body_tick_commit::Record,hand_trace_capacity> tick_commits{};
std::array<gears_body_mount_commit::Record,hand_trace_capacity> source_commits{};
std::atomic<bool> tick_commits_attempted{},tick_commits_written{};std::atomic<unsigned> tick_commits_error{};
constexpr unsigned alpha_event_capacity=64;
struct AlphaRecord {uint64_t sequence{},tick{},compose{},thread{};uint32_t count{},dropped{};
    std::array<gears_body_c0_arm_alpha::Diagnostic,alpha_event_capacity> events{};};
struct AlphaHeader {char magic[8]{'A','R','M','A','L','P','0','1'};
    uint32_t version{1},header_bytes{sizeof(AlphaHeader)},record_bytes{sizeof(AlphaRecord)},count{},capacity{hand_trace_capacity},event_capacity{alpha_event_capacity};
    uint64_t pid{},native_started{},read_only{1},writer{};};
static_assert(sizeof(AlphaHeader)==64&&std::is_trivially_copyable_v<AlphaRecord>);
std::array<AlphaRecord,hand_trace_capacity> alpha_records{};
std::atomic<bool> alpha_attempted{},alpha_written{};std::atomic<unsigned> alpha_error{};
// Observation only, on the existing GT boundaries. This does not call a guard,
// mutate a source latch, arm CAS, or supply retirement/physics authority. Keep
// both read times and unsuccessful reads; a matching epoch is not an end fence.
unsigned observe_hand_trace(HandTraceRecord& row,bool after) noexcept {
    namespace p=gears_cover_pose_phase;
    if(GetCurrentThreadId()!=owner_thread.load()||!state.captured||!state.prepared||
       row.mesh!=state.context.owner.mesh.address||row.compose!=state.context.copy.invocation)return 2;
    p::Request request{};request.owner=state.context.owner;
    auto reader=[](void* out,void const* at,size_t size){return native::read_memory(out,reinterpret_cast<uintptr_t>(at),size);};
    p::Metadata before{},again{};
    if(!p::metadata(reader,request,before)||!p::metadata_equal(before,state.backend.metadata()))return 2;
    auto* atoms=after?row.after_local:row.consumer;
    if(after){std::memcpy(&row.component,&before.component,64);row.end_pose_epoch=before.pose_epoch;}
    for(unsigned i=0;i<9;++i){const unsigned bone=hand_trace_bones[i];
        if(!native::read_memory(&atoms[i],before.local.data+bone*32,sizeof(body::Atom)))return 3;
        if(after&&!native::read_memory(&row.after_space[i],before.space.data+bone*64,64))return 4;}
    body::Atom atom{};body::Matrix64 matrix{};
    for(unsigned i=0;i<9;++i){const unsigned bone=hand_trace_bones[i];
        if(!native::read_memory(&atom,before.local.data+bone*32,sizeof atom)||std::memcmp(&atom,&atoms[i],sizeof atom))return 5;
        if(after&&(!native::read_memory(&matrix,before.space.data+bone*64,64)||std::memcmp(&matrix,&row.after_space[i],64)))return 6;}
    if(!p::metadata(reader,request,again)||!p::metadata_equal(before,again))return 7;
    return 1;
}
void trace_hands_commit() noexcept {
    // First32 successful joint commits only; no allocation, rotation or GT I/O.
    state.hand_record=0;if(!diagnostic_first_failure||hand_trace_sequence>=hand_trace_capacity||
        (diagnostic_held_main_mount&&state.right_mount!=hands::adapter::MountKind::held_lancer))return;
    const auto seq=++hand_trace_sequence;auto& row=hand_trace[(seq-1)%hand_trace_capacity];row={};
    row.sequence=seq;row.tick=state.context.tick;row.compose=state.context.copy.invocation;row.qpc=uint64_t(phase::qpc());
    row.publication=state.context.source.publication_sequence;row.source_ns=uint64_t(state.context.source.stereo.received_ns);
    row.mesh=state.context.owner.mesh.address;row.pose_epoch=state.backend.installation_epoch();
    for(unsigned side=0;side<2;++side)row.wrist[side]=state.hand_work.computed_wrist[side];
    for(unsigned i=0;i<9;++i){const unsigned bone=hand_trace_bones[i];row.before[i]=state.source.local[bone];row.planned[i]=state.output.local[bone];}
    row.head_goal=state.output.head_world;
    if(body_tick_source_live::enabled){auto& proof=source_commits[seq-1];
        proof.sequence=seq;proof.tick=row.tick;proof.compose=row.compose;proof.committed_ns=rig_tracking_live::clock_ns();
        proof.capture_ns=state.context.basis_ns;proof.input=state.context.body_source;proof.parent=state.context.source_parent;proof.queried_parent=state.context.queried_parent;
        proof.query_source=state.mount_source;proof.response=state.body_mount_response;
        proof.base_view=state.body_render_base;std::memcpy(&proof.component,&state.source.component_to_world,sizeof proof.component);
        proof.geometry=state.model.signature;proof.avatar_asset=rig_tracking_live::object(state.context.asset);
    }else if(body_tick_input_live::enabled){auto& proof=tick_commits[seq-1];
        std::memcpy(&proof.input,&state.context.body_input,sizeof proof.input);proof.parent=state.context.mount_parent;
        proof.sequence=seq;proof.tick=row.tick;proof.compose=row.compose;proof.committed_ns=rig_tracking_live::clock_ns();
        proof.capture_ns=state.context.basis_ns;proof.base_view=state.body_render_base;
        proof.component=state.source.component_to_world;proof.geometry=state.model.signature;
        proof.avatar_asset=rig_tracking_live::object(state.context.asset);proof.mount=state.mount_response;}
    auto& alpha=alpha_records[seq-1];alpha={};alpha.sequence=seq;alpha.tick=row.tick;alpha.compose=row.compose;alpha.thread=state.context.copy.thread;
    state.hand_record=seq;
}
bool alpha_control_drained()noexcept {
#if defined(GEARS_COVER_CONTROL_OBSERVER) && GEARS_COVER_CONTROL_OBSERVER && defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
    return cover_control_observer_live::stopping.load()&&!cover_control_observer_live::in_flight.load()&&!cover_control_observer_live::compose_in_flight.load();
#else
    return false;
#endif
}
void export_alpha_records()noexcept {
    if(!body_tick_input_live::enabled||!diagnostic_first_failure||!diagnostic_held_main_mount||!stopping.load()||callbacks.load()||obligations.load()||
       !hand::stopping.load()||hand::callbacks.load()||hand::live_frames.load()||!alpha_control_drained()||
       GetCurrentThreadId()==owner_thread.load()||alpha_attempted.exchange(true))return;
    wchar_t path[MAX_PATH]{};if(!directory[0]||swprintf_s(path,L"%s\\body-arm-alpha.bin",directory)<=0){alpha_error=ERROR_INVALID_NAME;return;}
    AlphaHeader header{};header.count=unsigned(hand_trace_sequence);header.pid=GetCurrentProcessId();header.native_started=uint64_t(session_started_qpc);
    HANDLE file=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);if(file==INVALID_HANDLE_VALUE){alpha_error=GetLastError();return;}
    auto write=[&](void const* data,DWORD size){DWORD written{};const bool good=WriteFile(file,data,size,&written,nullptr)&&written==size;if(!good)alpha_error=GetLastError();return good;};
    bool good=header.count<=hand_trace_capacity&&write(&header,sizeof header);if(good&&header.count)good=write(alpha_records.data(),DWORD(header.count*sizeof(AlphaRecord)));
    if(!CloseHandle(file)){alpha_error=GetLastError();good=false;}alpha_written=good;
}
void export_tick_commits()noexcept {
    if(!body_tick_input_live::enabled||!stopping.load()||callbacks.load()||obligations.load()||
       !hand::stopping.load()||hand::callbacks.load()||hand::live_frames.load()||
       GetCurrentThreadId()==owner_thread.load()||tick_commits_attempted.exchange(true))return;
    wchar_t path[MAX_PATH]{};
    if(!directory[0]||swprintf_s(path,body_tick_source_live::enabled?L"%s\\body-mount-source-commits.bin":L"%s\\body-tick-commits.bin",directory)<=0){tick_commits_error=ERROR_INVALID_NAME;return;}
    gears_body_tick_commit::Header header{};header.count=unsigned(hand_trace_sequence);header.pid=GetCurrentProcessId();header.native_started=uint64_t(session_started_qpc);
    HANDLE file=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){tick_commits_error=GetLastError();return;}
    auto write=[&](void const* data,DWORD n){DWORD written{};const bool ok=WriteFile(file,data,n,&written,nullptr)&&written==n;if(!ok)tick_commits_error=GetLastError();return ok;};
    bool okay{};
    if(body_tick_source_live::enabled){gears_body_mount_commit::Header next{};
        next.count=header.count;next.pid=header.pid;next.native_started=header.native_started;
        okay=next.count<=hand_trace_capacity&&write(&next,sizeof next);
        if(okay&&next.count)okay=write(source_commits.data(),DWORD(next.count*sizeof(gears_body_mount_commit::Record)));
    }else{okay=header.count<=hand_trace_capacity&&write(&header,sizeof header);
        if(okay&&header.count)okay=write(tick_commits.data(),DWORD(header.count*sizeof(gears_body_tick_commit::Record)));}
    if(!CloseHandle(file)){tick_commits_error=GetLastError();okay=false;}tick_commits_written=okay;
}
void export_hand_trace() noexcept {
    if(!hands_enabled||!diagnostic_first_failure||!stopping.load()||callbacks.load()||obligations.load()||
       GetCurrentThreadId()==owner_thread.load()||hand_trace_attempted.exchange(true))return;
    wchar_t path[MAX_PATH]{};if(!directory[0]||swprintf_s(path,L"%s\\body-hands-commits.bin",directory)<=0){hand_trace_error=ERROR_INVALID_NAME;return;}
    LARGE_INTEGER frequency{};if(!QueryPerformanceFrequency(&frequency)||frequency.QuadPart<=0){hand_trace_error=ERROR_INVALID_DATA;return;}
    HandTraceHeader header{};header.last=hand_trace_sequence;header.count=unsigned((std::min)(uint64_t(hand_trace_capacity),header.last));
    header.first=header.count?header.last-header.count+1:0;header.pid=GetCurrentProcessId();header.session=uint64_t(session_started_qpc);header.qpc_frequency=uint64_t(frequency.QuadPart);
    HANDLE file=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){hand_trace_error=GetLastError();return;}
    auto write=[&](void const* p,DWORD n){DWORD bytes{};const bool ok=WriteFile(file,p,n,&bytes,nullptr)&&bytes==n;if(!ok)hand_trace_error=GetLastError();return ok;};
    bool ok=write(&header,sizeof header);
    for(uint64_t n=0;ok&&n<header.count;++n){const auto seq=header.first+n;
        if(hand_trace[(seq-1)%hand_trace_capacity].sequence!=seq){hand_trace_error=ERROR_INVALID_DATA;ok=false;}}
    if(ok&&header.count){const auto start=unsigned((header.first-1)%hand_trace_capacity);
        const unsigned first=(std::min)(header.count,hand_trace_capacity-start);
        ok=write(hand_trace.data()+start,DWORD(first*sizeof(HandTraceRecord)));
        if(ok&&first<header.count)ok=write(hand_trace.data(),DWORD((header.count-first)*sizeof(HandTraceRecord)));}
    if(!CloseHandle(file)){hand_trace_error=GetLastError();ok=false;}hand_trace_written=ok;
}
failure::Counters failure_counts;failure::First first_failure,first_commit_failure;
std::atomic<uint64_t> admission_skipped{},age_recovered{},recovery_rejected{},recovery_deferred{};
std::atomic<bool> failure_file_attempted{},failure_file_written{};std::atomic<unsigned> failure_file_error{};
std::atomic<bool> commit_failure_file_attempted{},commit_failure_file_written{};std::atomic<unsigned> commit_failure_file_error{};
void remember_failure(Entry const&e,body::Goal const&goal,failure::Stage stage,unsigned detail,
                      failure::Timing timing,body::Report const*report=nullptr,body::Output const*output=nullptr,unsigned unit=UINT_MAX)noexcept {
    // An ineligible/cross-thread call is counted but cannot read another GT's
    // capture. The first failed apply on its actual captured C0 is retained.
    if(!diagnostic_first_failure||GetCurrentThreadId()!=owner_thread.load()||!state.captured||state.context.entry!=&e||
       (first_failure.state()&&(stage!=failure::Stage::commit||first_commit_failure.state())))return;
    if(auto const*primary=first_failure.published())if(primary->header.stage==unsigned(failure::Stage::commit))return;
    failure::Header h{};h.stage=unsigned(stage);h.detail=detail;h.unit=unit;h.thread=GetCurrentThreadId();h.session=uint64_t(session_started_qpc);
    h.tick=state.context.tick;h.compose=e.invocation;h.qpc=uint64_t(phase::qpc());h.ra=e.ra;h.rcx=e.rcx;h.rdx=e.rdx;h.r8=e.r8;
    h.registry_version=native_atom_overwrite_live::journal_version();h.kernel_pending=state.kernel.pending();h.kernel_fields=state.kernel.outstanding_fields();
    h.kernel_protocol=state.kernel.protocol_debt();h.kernel_flags=unsigned(state.kernel.failed())|unsigned(state.kernel.consumed())*2|unsigned(state.kernel.original_exception())*4|unsigned(state.kernel.stopping())*8;
    LARGE_INTEGER frequency{};if(QueryPerformanceFrequency(&frequency))timing.frequency=uint64_t(frequency.QuadPart);timing.end=h.qpc;h.timing=timing;
    auto const&source=state.context.source;h.core_published_ns=source.published_ns;h.stereo_received_ns=uint64_t(source.stereo.received_ns);h.controllers_received_ns=uint64_t(source.controllers.received_ns);
    const auto begin=failure::ns(timing.begin,timing.frequency),end=failure::ns(timing.end,timing.frequency);
    h.core_age_begin=failure::age(h.core_published_ns,begin);h.core_age_end=failure::age(h.core_published_ns,end);
    h.stereo_age_begin=failure::age(h.stereo_received_ns,begin);h.stereo_age_end=failure::age(h.stereo_received_ns,end);
    h.controllers_age_begin=failure::age(h.controllers_received_ns,begin);h.controllers_age_end=failure::age(h.controllers_received_ns,end);
    if(!first_failure.save(h,state.stamp,state.backend.metadata(),state.source,goal,report,output,state.desired)&&stage==failure::Stage::commit)
        first_commit_failure.save(h,state.stamp,state.backend.metadata(),state.source,goal,report,output,state.desired);
}
struct RetirementContext final:gears_atom_overwrite::Context {
    binding::Binding actor{};gears_anchor::Identity asset{};gears_cover_pose_phase::OwnerKey owner{};
    uint64_t thread_token()const noexcept override{return GetCurrentThreadId();}
    bool current(tx::Stamp const&s,gears_atom_overwrite::Observation&out)noexcept override{try{
        if(GetCurrentThreadId()!=owner_thread.load()||!native_atom_overwrite_live::world_current()||!tx::same(s,state.stamp)||
           owner.session!=uint64_t(session_started_qpc)||owner.world_generation!=committed_world_generation.load()||
           !binding::current_actor_storage(actor)||!phase::own_entries()||!hand::tick_target(owner.mesh.address)||
           !fresh_pose_producer_live::own_entries()||!state.backend.retirement_metadata(out.metadata))return false;
        {std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);gears_anchor::Identity current{};
            if(!lock.owns_lock()||!binding::metadata_current()||!binding::identity(out.metadata.asset,current,true)||!binding::same(current,asset))return false;}
        out.journal_version=native_atom_overwrite_live::journal_version();out.qpc=uint64_t(phase::qpc());
        return binding::current_actor_storage(actor)&&native_atom_overwrite_live::world_current();
    }catch(...){return false;}}
} retirement_context;
void install_atom_receipt()noexcept {
    if(state.atom_installed||!state.kernel.outstanding_fields())return;
    retirement_context.actor=state.context.actor;retirement_context.asset=state.context.asset;retirement_context.owner=state.context.owner;
    if(native_atom_overwrite_live::install(retirement_context,state.stamp,state.backend.installation_epoch())){
        state.backend.receipt_source(native_atom_overwrite_live::receipts());state.atom_installed=true;}
}
struct Call {Call(){++callbacks;}~Call(){--callbacks;}};
void arm_alpha_diagnostic(gears_body_c0_arm_alpha::Diagnostic const& event) noexcept {
    // POD observation only. A refused owner/pair still belongs in the trace;
    // this function never asks a guard again and never supplies permission.
    if(!body_tick_input_live::enabled||!diagnostic_first_failure||!diagnostic_held_main_mount||
       GetCurrentThreadId()!=owner_thread.load())return;
    Call call;
    ready_wrist::alpha_event(event);
    if(!state.hand_record||state.hand_record>hand_trace_capacity||!state.captured||!state.prepared||!state.notified||
       !state.joint_committed||!state.consumer_verified||state.right_mount!=hands::adapter::MountKind::held_lancer)return;
    const auto index=unsigned(state.hand_record-1);auto& row=alpha_records[index];auto const& hand_row=hand_trace[index];
    if(row.sequence!=state.hand_record||hand_row.sequence!=row.sequence||row.tick!=event.outer_tick||row.compose!=event.compose||
       row.thread!=event.thread||event.thread!=GetCurrentThreadId()||state.context.tick!=row.tick||state.context.copy.invocation!=row.compose)return;
    if(event.end&&event.end!=11&&event.end!=35)return;
    if(row.count<alpha_event_capacity)row.events[row.count++]=event;else ++row.dropped;
}
void sync() noexcept {obligations=state.kernel.pending();}
bool same_entry(Entry const& e) noexcept {return state.context.entry==&e&&!std::memcmp(&e,&state.context.copy,sizeof e);}
#include "ready_wrist_entry_live.inl"
void arm_weight_return(gears_body_c0_arm_alpha::WeightReturn const& input) noexcept {
    if(!ready_wrist::recorder.enabled||GetCurrentThreadId()!=owner_thread.load()||!state.context.entry)return;
    ready_wrist::weight_return(*state.context.entry,input);
}
bool arm_control_policy_enabled() noexcept {return enabled&&hands_enabled&&arm_alpha_enabled;}
bool main_mount_policy_enabled() noexcept {
#if defined(GEARS_WEAPON_MAIN_MOUNT_PROVIDER) && GEARS_WEAPON_MAIN_MOUNT_PROVIDER
    return arm_control_policy_enabled()&&held_main_mount;
#else
    return false;
#endif
}
void note_mount_prequery(held_outcome::PreQueryGate gate,held_outcome::U detail=held_outcome::unknown,
    held_outcome::U cookie=held_outcome::unknown,held_outcome::U parent_cookie=held_outcome::unknown,
    held_outcome::U revision=held_outcome::unknown,held_outcome::U generation=held_outcome::unknown) noexcept {
    auto* e=state.context.entry;if(e)held_outcome::recorder.prequery_result(reinterpret_cast<uintptr_t>(e),e->invocation,
        held_outcome::U(gate),detail,cookie,parent_cookie,revision,generation);
}
hands::adapter::MountKind select_body_mount(gears_native_rig_tracking::CoreSnapshot const& source) noexcept {
#if defined(GEARS_BODY_MAIN_MOUNT_SOURCE) && GEARS_BODY_MAIN_MOUNT_SOURCE
    using Kind=hands::adapter::MountKind;using Code=gears_weapon_hand::MainMountQueryResult::Code;
    state.mount_source={};state.body_mount_response={};state.context.source_parent={};state.context.queried_parent={};
    auto* broker=weapon_hand_runtime::current();const auto cookie=broker?broker->cookie():0;
    auto const& i=state.context.body_source.observed;
    using Gate=held_outcome::PreQueryGate;auto prequery=Gate::passed;auto detail=held_outcome::unknown;
    if(!body_mount_policy_enabled())prequery=Gate::body_policy;
    else if(!cookie)prequery=Gate::body_cookie;
    else if(i.parent_cookie!=cookie)prequery=Gate::parent_cookie;
    else if(!broker->registration_current(cookie,source.native.native_started_qpc))prequery=Gate::registration_current;
    else if(!state.context.current(tx::Stage::prepare)){prequery=Gate::context_prepare;detail=unsigned(state.context.rejection());}
    if(prequery!=Gate::passed){note_mount_prequery(prequery,detail,cookie,
        prequery>=Gate::parent_cookie?i.parent_cookie:held_outcome::unknown);++main_mount_not_ready;return Kind::free_hand;}
    body_tick_source_live::Parent parent{};parent.cookie=cookie;parent.native_started=source.native.native_started_qpc;
    parent.revision=broker->lifecycle_revision();parent.generation=broker->action_generation();
    parent.tick=state.context.tick;parent.compose=state.context.copy.invocation;
    if(!parent.revision||!parent.generation){note_mount_prequery(!parent.revision?Gate::lifecycle_revision:Gate::action_generation,
        held_outcome::unknown,cookie,i.parent_cookie,parent.revision,parent.generation);++main_mount_not_ready;return Kind::free_hand;}
    note_mount_prequery(Gate::passed,held_outcome::unknown,cookie,i.parent_cookie,parent.revision,parent.generation);
    auto& s=state.mount_source;
    // Historical association only: copy the real original payload unchanged.
    // This explicitly makes no old V1 freshness claim and never calls a V1
    // operation using a forged fresh Frame.
    gears_rig_tracking::detail::copy_payload(source,s.original_scene);s.original_scene.native.registration_cookie=cookie;
    s.scene_ticket=source.source_ticket;s.scene_cache_revision=state.context.body_source.scene_cache_revision;
    s.metadata_begin_ns=state.context.body_source.metadata_begin_ns;s.metadata_end_ns=state.context.body_source.metadata_end_ns;
    auto& t=s.tracking;t.tick=i.tick;t.thread=i.thread;t.parent_cookie=i.parent_cookie;t.begin_ns=i.begin_ns;t.end_ns=i.end_ns;
    std::memcpy(&t.stereo,&i.stereo,sizeof t.stereo);std::memcpy(&t.controllers,&i.controllers,sizeof t.controllers);
    std::memcpy(&t.continuity,&i.continuity,sizeof t.continuity);t.native_reference=i.native_reference;
    t.reference_revision=i.reference_revision;t.publisher_pid=i.publisher_pid;t.publisher_start=i.publisher_start;
    s.basis.compose=parent.compose;s.basis.capture_begin_ns=state.context.basis_ns;s.basis.geometry=state.mount_body;
    std::memcpy(&s.basis.component_to_world,&state.source.component_to_world,sizeof s.basis.component_to_world);
    std::memcpy(&s.basis.base_view,&state.body_render_base,sizeof s.basis.base_view);
    struct QueryScope{QueryScope(){state.body_query_active=true;}~QueryScope(){state.body_query_active=false;}};
    held_outcome::recorder.query_begin();
    const auto begin=rig_tracking_live::clock_ns();budget::recorder.mark(budget::Stage::query_begin);
    gears_weapon_hand::MainMountQueryResult result{};
    {QueryScope scope;result=weapon_hand_runtime::query_body_mount(s,state.body_mount_response);}
    held_outcome::recorder.query_end(result.query_id,unsigned(result.code),unsigned(result.branch),result.callback_status,
        state.body_mount_response.equipment_begin_ns,state.body_mount_response.equipment_end_ns);
    // Persist the actual pre-query parent even for a non-ready answer. A
    // changed/revoked registration cannot turn into a permitted freehand write.
    // This is assigned only AFTER return: source callbacks never reject their
    // own precisely reserved pending callback via a normal-idle predicate.
    state.context.queried_parent=parent;
    budget::recorder.mark(budget::Stage::query_end);budget::recorder.query_result(result.query_id,unsigned(result.code),unsigned(result.branch));
    const auto finish=rig_tracking_live::clock_ns();if(finish>=begin){const auto elapsed=uint64_t(finish-begin);auto maximum=main_mount_max_ns.load();
        while(maximum<elapsed&&!main_mount_max_ns.compare_exchange_weak(maximum,elapsed)){} }
    main_mount_last_query=result.query_id;if(unsigned(result.branch)<main_mount_branches.size())++main_mount_branches[unsigned(result.branch)];
    if(result.callback_status!=UINT32_MAX)main_mount_last_callback_status=result.callback_status;
    if(result.code!=Code::ready){if(result.code==Code::context_rejected)++main_mount_context_rejected;
        else if(result.code==Code::stale){++main_mount_stale;++main_mount_stale_broker;}
        else if(result.code==Code::fault)++main_mount_fault;else ++main_mount_not_ready;return Kind::free_hand;}
    auto const& r=state.body_mount_response;auto const& m=r.mount;
    parent.query=result.query_id;parent.equipment_begin_ns=r.equipment_begin_ns;parent.equipment_end_ns=r.equipment_end_ns;
    gears_weapon_lancer_wrist_mount::Profile profile{};
    if(r.size!=sizeof r||r.abi_version!=body_mount::version||r.query_id!=result.query_id||m.size!=sizeof m||m.abi_version!=main_mount::version||
       m.query_id!=r.query_id||!r.hand_lifecycle||r.hand_lifecycle!=m.key.lifecycle||r.action_generation!=parent.generation||
       !body_mount::same(r.source,body_mount::source_key(s))||
       m.key.registration_cookie!=cookie||m.key.frame!=source.scene_invocation||m.key.game_pid!=source.native.game_pid||m.key.thread_id!=source.native.game_thread||
       m.key.game_start!=source.native.game_start||m.key.native_started_qpc!=source.native.native_started_qpc||m.key.world_generation!=source.world_generation||
       !m.hold_generation||!gears_weapon_hand::same(m.world,source.world)||!gears_weapon_hand::same(m.controller,source.controller)||
       !gears_weapon_hand::same(m.pawn,source.pawn)||!gears_weapon_hand::same(m.mesh,source.mesh)||!gears_weapon_hand::same(m.weapon,source.weapon)||
       !gears_weapon_hand::identity_valid(m.weapon)||!gears_weapon_hand::identity_valid(m.component)||!gears_weapon_hand::identity_valid(m.asset)||m.component.outer!=m.weapon.address||
       m.provider_revision!=main_mount::provider_revision||m.body_revision!=source.binding_generation||m.basis_revision!=main_mount::supported_basis_revision||
       m.basis!=main_mount::Basis::marcus_mesh_face_forward_y||m.reserved||m.geometry!=state.model.signature||m.model_scale!=state.source.model_scale||m.units_per_metre!=source.units_per_metre||
       m.calibration_known!=1||m.calibration_reserved||!gears_weapon_lancer_wrist_mount::marcus115(m.geometry,m.model_scale,m.units_per_metre,profile)){
        ++main_mount_not_ready;return Kind::free_hand;}
    auto const& c=profile.mount.calibration;
    if(m.grip_to_wrist_metres.x!=c.grip_to_wrist_metres.x||m.grip_to_wrist_metres.y!=c.grip_to_wrist_metres.y||m.grip_to_wrist_metres.z!=c.grip_to_wrist_metres.z||
       m.grip_to_wrist_rotation.x!=c.grip_to_wrist_rotation.x||m.grip_to_wrist_rotation.y!=c.grip_to_wrist_rotation.y||m.grip_to_wrist_rotation.z!=c.grip_to_wrist_rotation.z||m.grip_to_wrist_rotation.w!=c.grip_to_wrist_rotation.w||
       !body_tick_source_live::parent_current(parent)){++main_mount_not_ready;return Kind::free_hand;}
    state.context.source_parent=parent;state.mount_response=m;
    if(!state.context.current(tx::Stage::prepare)){++main_mount_stale;++main_mount_stale_c0;main_mount_c0_rejection=unsigned(state.context.rejection());return Kind::free_hand;}
    ++main_mount_ready;return Kind::held_lancer;
#else
    (void)source;return hands::adapter::MountKind::free_hand;
#endif
}
hands::adapter::MountKind select_main_mount(gears_native_rig_tracking::CoreSnapshot const& source,
    gears_body_head_authored::Marker const* marker) noexcept {
    using Kind=hands::adapter::MountKind;
    state.mount_response={};state.mount_rig={};state.mount_body={};state.context.mount_parent={};
#if defined(GEARS_WEAPON_MAIN_MOUNT_PROVIDER) && GEARS_WEAPON_MAIN_MOUNT_PROVIDER
    if(!main_mount_policy_enabled())return Kind::free_hand;
    ++main_mount_attempts;
    gears_body_head_authored::EyeHmdWorld authored_eye{};
    using Gate=held_outcome::PreQueryGate;auto prequery=Gate::passed;auto detail=held_outcome::unknown;
    if(!marker)prequery=Gate::marker_missing;
    else {const auto basis=marker->basis();
        if(basis!=gears_body_head_authored::Basis::marcus_mesh_face_forward_y){prequery=Gate::marker_basis;detail=unsigned(basis);}
        else {const auto eye_status=gears_body_head_authored::eye_from_head(*marker,state.model,state.source,{state.output.head_world},authored_eye);
            if(eye_status!=gears_body_head_authored::Status::okay){prequery=Gate::eye_status;detail=unsigned(eye_status);}}
    }
    if(prequery!=Gate::passed){note_mount_prequery(prequery,detail);++main_mount_not_ready;return Kind::free_hand;}
    auto* broker=weapon_hand_runtime::current();const auto cookie=broker?broker->cookie():0;
    if(!cookie){note_mount_prequery(Gate::main_cookie,held_outcome::unknown,cookie);++main_mount_not_ready;return Kind::free_hand;}
    auto& body=state.mount_body;body.mesh=source.mesh;body.geometry=state.model.signature;
    body.model_scale=state.source.model_scale;body.units_per_metre=state.source.units_per_metre;
    body.revision=source.binding_generation;body.basis_revision=main_mount::supported_basis_revision;
    body.basis=main_mount::Basis::marcus_mesh_face_forward_y;
    if(body_tick_source_live::enabled)return select_body_mount(source);
    const gears_rig_tracking::NativeKey key{source.native.game_pid,source.native.game_thread,
        source.native.game_start,source.native.native_started_qpc,cookie};
    if(!gears_native_rig_tracking::project_for_authenticated_registration(source,key,rig_tracking_live::clock_ns(),
        50000000,state.mount_rig,simulator_operator)){++main_mount_stale;++main_mount_stale_projection;return Kind::free_hand;}

    body_tick_input_live::Parent retained_parent{};
    if(body_tick_input_live::enabled&&!body_tick_input_live::parent_snapshot(state.context.body_input,state.context.copy.invocation,retained_parent)){++main_mount_not_ready;return Kind::free_hand;}
    const auto begin=rig_tracking_live::clock_ns();budget::recorder.mark(budget::Stage::query_begin);
    const auto result=weapon_hand_runtime::query_main_mount(state.mount_rig,body,state.mount_response);
    budget::recorder.mark(budget::Stage::query_end);budget::recorder.query_result(result.query_id,unsigned(result.code),unsigned(result.branch));
    const auto finish=rig_tracking_live::clock_ns();
    if(finish>=begin){const auto elapsed=uint64_t(finish-begin);auto maximum=main_mount_max_ns.load();
        while(maximum<elapsed&&!main_mount_max_ns.compare_exchange_weak(maximum,elapsed)){} }
    main_mount_last_query=result.query_id;
    if(unsigned(result.branch)<main_mount_branches.size())++main_mount_branches[unsigned(result.branch)];
    if(result.callback_status!=UINT32_MAX)main_mount_last_callback_status=result.callback_status;
    if(result.source_failure_mask){main_mount_source_failure_query=result.query_id;main_mount_last_source_failure_mask=result.source_failure_mask;
        for(unsigned i=0;i<result.source_ages_ns.size();++i)main_mount_last_failure_ages[i]=result.source_ages_ns[i];}
    using Code=gears_weapon_hand::MainMountQueryResult::Code;
    if(result.code!=Code::ready){
        if(result.code==Code::context_rejected)++main_mount_context_rejected;
        else if(result.code==Code::stale){++main_mount_stale;++main_mount_stale_broker;}
        else if(result.code==Code::fault)++main_mount_fault;
        else ++main_mount_not_ready;
        return Kind::free_hand;
    }
    if(body_tick_input_live::enabled){
        auto const& r=state.mount_response;auto const& i=state.context.body_input;
        // The unchanged PG answer selects authored calibration. Join its
        // original Scene/key and live parent to this exact real Tick/C0.
        retained_parent.query=result.query_id;
        if(r.query_id!=result.query_id||r.key.registration_cookie!=i.parent_cookie||r.key.frame!=source.scene_invocation||
           r.key.game_pid!=source.native.game_pid||r.key.thread_id!=source.native.game_thread||
           r.key.game_start!=source.native.game_start||r.key.native_started_qpc!=source.native.native_started_qpc||
           r.key.world_generation!=source.world_generation||!r.key.lifecycle||!r.hold_generation||
           !gears_weapon_hand::same(r.world,source.world)||!gears_weapon_hand::same(r.controller,source.controller)||
           !gears_weapon_hand::same(r.pawn,source.pawn)||!gears_weapon_hand::same(r.mesh,source.mesh)||
           !gears_weapon_hand::same(r.weapon,source.weapon)||!gears_weapon_hand::identity_valid(r.component)||!gears_weapon_hand::identity_valid(r.asset)||
           r.geometry!=state.model.signature||r.body_revision!=source.binding_generation||r.units_per_metre!=source.units_per_metre||
           r.model_scale!=state.source.model_scale||!body_tick_input_live::parent_current(retained_parent)){
            ++main_mount_not_ready;state.mount_response={};return Kind::free_hand;}
        state.context.mount_parent=retained_parent;
    }
    // A reply is data, not a new source/lease or renewed clock. The original
    // current C0/immutable latch and remaining admission budget still decide
    // whether any body/arm write can happen after the metadata callback.
    if(!state.context.current(tx::Stage::prepare)){budget::recorder.mark(budget::Stage::post_current_end);++main_mount_stale;++main_mount_stale_c0;main_mount_c0_rejection=unsigned(state.context.rejection());state.mount_response={};return Kind::free_hand;}
    budget::recorder.mark(budget::Stage::post_current_end);++main_mount_ready;return Kind::held_lancer;
#else
    (void)source;(void)marker;return Kind::free_hand;
#endif
}
bool arm_control_policy_active() noexcept {return arm_control_policy_enabled()&&!stopping.load();}
gears_body_c0_arm_alpha::RetainedControl retained_control_current(phase::Scope const& scope,uint64_t tick) noexcept {
    using Result=gears_body_c0_arm_alpha::RetainedControl;
#if defined(GEARS_BODY_TICK_INPUT) && GEARS_BODY_TICK_INPUT
    if(!body_tick_input_live::enabled)return Result::not_applicable;
    Call call;auto& f=hand::slot;
    auto deny=[&]()noexcept {if(GetCurrentThreadId()==owner_thread.load()&&f.active)f.hard_control_fault=true;return Result::denied;};
    try{
        if(GetCurrentThreadId()!=owner_thread.load()||!arm_control_policy_enabled()||stopping.load()||!recording.load()||
           !hand::enabled||hand::stopping.load()||hand::failed.load()||!hand::passive()||f.hard_control_fault||
           !cover_control_observer_live::retained_boundary_current()||
           !f.active||f.tick_depth!=1||f.compose_depth!=1||f.control_depth>1||!tick||f.tick!=tick||
           &scope!=&f.compose.scope||scope.invocation!=state.context.copy.invocation||!scope.arguments||scope.kind||
           scope.owner.thread!=GetCurrentThreadId()||!phase::same(scope.owner.key,state.context.owner)||
           !state.captured||!state.prepared||!state.notified||!state.joint_committed||!state.consumer_verified||state.context.expired||
           !state.kernel.retained_consumer(state.backend,state.lease,state.stamp)||
           state.model.signature!=body::rig::marcus115_signature)return deny();
        // No fresh source or re-notification is obtained here. The exact live
        // consumed lease authenticates use of the already-written Local pose.
        // Hard data/continuity checks still run independently after the old
        // observation latch has burned; elapsed age cannot grant new writes.
        if(body_tick_source_live::enabled){
            if(!state.context.current(tx::Stage::consumer_entry)||!rig_tracking_live::retained_core_identity(state.context.source)||
               !body_tick_source_live::retained_current(state.context.body_source,state.context.source,tick,state.context.copy.thread)||
               (state.context.queried_parent.cookie&&!body_tick_source_live::parent_idle_current(state.context.queried_parent))||
               (state.context.source_parent.query&&!body_tick_source_live::parent_idle_current(state.context.source_parent)))return deny();
            return Result::verified;
        }
        if(!state.context.current(tx::Stage::consumer_entry)||!rig_tracking_live::retained_core_identity(state.context.source)||
           body_tick_input_live::current(state.context.body_input,state.context.source,tick,state.context.copy.thread,state.context.basis_ns)==gears_body_tick_input::Verdict::hard||
           (state.context.mount_parent.query&&!body_tick_input_live::parent_current(state.context.mount_parent)))return deny();
        return Result::verified;
    }catch(...){return deny();}
#else
    (void)scope;(void)tick;return Result::not_applicable;
#endif
}
bool arm_control_scope(phase::Scope const*& scope,uint64_t& outer_tick) noexcept {
    Call call;scope=nullptr;outer_tick=0;
    try{auto const& f=hand::slot;
        // Admission precedes Native.pose_begin and grants no alpha authority.
        if(!arm_control_policy_enabled()||stopping.load()||!f.active||!f.compose.admitted||!f.tick||
           !f.compose_address||!f.compose.scope.arguments||f.compose.scope.kind!=0||
           GetCurrentThreadId()!=f.key.request.native.game_thread||
           f.data.signature!=body::rig::marcus115_signature||!hand::control_current(f.compose.scope))return false;
        scope=&f.compose.scope;outer_tick=f.tick;return true;
    }catch(...){return false;}
}
bool owns_arm_control(gears_body_c0_arm_alpha::Proof const& p) noexcept {
    Call call;++arm_alpha_owner_calls;
    gears_body_c0_arm_alpha::Diagnostic event{};event.compose=p.compose;event.outer_tick=p.outer_tick;
    event.compute_ordinal=p.compute_ordinal;event.weight_ordinal=p.weight_ordinal;event.token=p.token;
    event.node_argument=p.node_argument;event.scope_address=p.scope_address;event.thread=p.thread;event.kind=p.kind;event.end=p.side?35u:11u;
    event.stage=static_cast<uint32_t>(gears_body_c0_arm_alpha::Stage::owner);event.original_alpha_bits=p.original_alpha_bits;
    event.flags=16u|(p.normal_return?8u:0u);event.qpc=phase::qpc();
    auto deny=[&](unsigned reason)noexcept{++arm_alpha_owner_rejected;event.reason=reason;arm_alpha_diagnostic(event);return false;};
    try{namespace pose=gears_cover_pose_phase;auto const& f=hand::slot;
        if(body_tick_input_live::enabled&&retained_control_current(f.compose.scope,f.tick)!=gears_body_c0_arm_alpha::RetainedControl::verified)return deny(15);
        if(!arm_control_policy_enabled()||stopping.load()||GetCurrentThreadId()!=owner_thread.load()||
           !state.captured||!state.prepared||!state.notified||!state.joint_committed||!state.consumer_verified||
           state.context.expired||!state.context.entry||state.kernel.failed()||state.kernel.protocol_debt()||state.kernel.original_exception())return deny(1);
        if(!p.normal_return||p.thread!=GetCurrentThreadId()||p.thread!=state.context.copy.thread||
           p.compose!=state.context.copy.invocation||p.outer_tick!=state.context.tick||
           p.scope_address!=reinterpret_cast<uintptr_t>(&f.compose.scope)||
           !phase::same(p.owner,state.context.owner)||p.side>=2||p.kind>=2||!p.token||p.token==UINT64_MAX||
           !(state.hand_work.solved_mask&(1u<<p.side))||
           p.compute_ordinal!=p.token||p.weight_ordinal!=p.token+1||p.original_alpha_bits!=0x3f800000u||
           p.cached_scale_flag||p.scale.count||!p.node_argument||p.node.address!=p.node_argument||
           !p.root.address||p.node.outer!=p.root.address)return deny(2);
        const unsigned count=p.kind?1u:3u;const unsigned chain[2][3]={{8,10,11},{32,34,35}};
        if(p.affected_count!=count||p.output.count!=int(count)||p.affected_header.count!=int(count)||
           !pose::header_valid(p.output,64)||!pose::header_valid(p.affected_header,4)||!pose::header_valid(p.scale,1,true))return deny(3);
        for(unsigned i=0;i<count;++i)if(p.affected[i]!=int32_t(p.kind?chain[p.side][2]:chain[p.side][i]))return deny(4);
        if(!pose::metadata_equal(p.metadata,state.backend.metadata()))return deny(5);
        if(p.metadata.local.data!=state.stamp.binding.local)return deny(6);
        if(state.model.signature!=body::rig::marcus115_signature)return deny(7);
        const bool before=state.context.current(tx::Stage::consumer_entry);event.owner_current_before=before?2u:1u;
        if(!before)return deny(8);
        // Encoded RIP is RVA17070d8, not the old IDA annotation17060d8.
        // Historical data is not a live permit: authenticate current raw4.
        uint32_t threshold_bits{};float threshold{};
        if(!native::read_memory(&threshold_bits,base+0x17070d8,sizeof threshold_bits)){++arm_alpha_threshold_rejected;return deny(9);}
        if(threshold_bits!=0x3727c5acu){++arm_alpha_threshold_rejected;return deny(10);}
        std::memcpy(&threshold,&threshold_bits,sizeof threshold);
        if(!std::isfinite(threshold)||threshold<0)return deny(11);
        const bool after=state.context.current(tx::Stage::consumer_entry);event.owner_current_after=after?2u:1u;
        if(!after)return deny(12);if(stopping.load())return deny(13);
        const unsigned role=p.side*2+p.kind;state.alpha_grant_mask|=1u<<role;++state.alpha_grant_count;
        state.alpha_grant_nodes[role]=p.node_argument;state.alpha_grant_original[role]=p.original_alpha_bits;
        ++arm_alpha_owner_granted;event.flags|=32u;event.reason=0;arm_alpha_diagnostic(event);return true;
    }catch(...){return deny(14);}
}
bool digest(void const* p,size_t n,body::rig::Signature& result){
    BCRYPT_ALG_HANDLE a{};BCRYPT_HASH_HANDLE h{};bool ok=BCryptOpenAlgorithmProvider(&a,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(ok)ok=BCryptCreateHash(a,&h,nullptr,0,nullptr,0,0)>=0;if(ok)ok=BCryptHashData(h,reinterpret_cast<PUCHAR>(const_cast<void*>(p)),ULONG(n),0)>=0;
    if(ok)ok=BCryptFinishHash(h,result.data(),ULONG(result.size()),0)>=0;if(h)BCryptDestroyHash(h);if(a)BCryptCloseAlgorithmProvider(a,0);return ok;
}
bool same_recovery_session(gears_native_rig_tracking::CoreSnapshot const& a,gears_native_rig_tracking::CoreSnapshot const& b){
    namespace core=gears_native_rig_tracking;
    return core::same(a.native,b.native)&&a.world_generation==b.world_generation&&a.stereo.source==b.stereo.source&&
        a.units_per_metre==b.units_per_metre;
}
bool same_recovery_avatar(binding::Binding const& a,binding::Binding const& b){
    return a.context_valid&&b.context_valid&&a.weapon_known&&b.weapon_known&&
        a.world_generation==b.world_generation&&a.special_move==b.special_move&&
        binding::same(a.controller,b.controller)&&binding::same(a.pawn,b.pawn)&&
        binding::same(a.mesh,b.mesh)&&binding::same(a.world,b.world);
}
enum class RecoveryResult {okay,defer,hard};
RecoveryResult recovery_current(){auto const& old=state.recovery;auto const& m=state.backend.metadata();
    if(!(old.active&&state.kernel.settled_source_age()&&!state.prepared&&!state.atom_installed&&!stopping.load()&&
        state.stamp.frame>old.stamp.frame&&state.stamp.producer>old.stamp.producer&&state.stamp.scope!=old.stamp.scope&&
        tx::same(state.stamp.binding,old.stamp.binding)&&same_recovery_avatar(old.actor,state.context.actor)&&binding::same(old.asset,state.context.asset)&&
        same_recovery_session(old.source,state.context.source)&&old.units==state.source.units_per_metre&&old.model_scale==state.source.model_scale&&
        old.desired.count==state.desired.count&&old.desired.indices==state.desired.indices&&
        !std::memcmp(old.reference.data(),state.source.reference.data(),sizeof old.reference)&&
        old.metadata.asset==m.asset&&old.metadata.master==m.master&&old.metadata.physics_asset==m.physics_asset&&old.metadata.physics_instance==m.physics_instance&&
        gears_cover_pose_phase::header_equal(old.metadata.reference,m.reference)&&gears_cover_pose_phase::header_equal(old.metadata.local,m.local)&&
        gears_cover_pose_phase::header_equal(old.metadata.desired,m.desired)&&gears_cover_pose_phase::header_equal(old.metadata.space,m.space)&&
        native_atom_overwrite_live::ready_for_commit()))return RecoveryResult::hard;
    // The old operation is fully rolled back and normally closed with no
    // fields/lease/protocol debt. Its gun or input cohort need not survive a
    // normal equipment change/recenter. This is a NEW independently captured
    // operation on the same authenticated avatar/storage, not a renewal of
    // the old source or permission. Its complete current source, actor,
    // reference, registration and time limits still decide every new write.
    if(!state.context.current(tx::Stage::prepare)){
        // Classify HERE, immediately after this exact failed guard. A new
        // capture can itself reach its deadline; that grants no write and
        // does not turn the already-settled old age episode into a hard fault.
        return !stopping.load()&&state.context.rejection()==tx::Verdict::source_age_expired?RecoveryResult::defer:RecoveryResult::hard;}
    return stopping.load()?RecoveryResult::hard:RecoveryResult::okay;
}
// Additional admission reserve; the original per-write 50 ms limit remains.
bool admission_budget(){
    if(body_tick_source_live::enabled){
        auto verdict=held_outcome::unknown;bool admitted=false;
        if(state.context.basis_ns>0){const auto actual=body_tick_source_live::current(state.context.body_source,state.context.source,state.context.tick,state.context.copy.thread,state.context.basis_ns);
            verdict=unsigned(actual);admitted=actual==gears_body_tick_input::Verdict::okay;}
        held_outcome::recorder.admission(verdict,admitted,state.context.basis_ns);return admitted;
    }
    if(body_tick_input_live::enabled)return state.context.basis_ns>0&&
        body_tick_input_live::current(state.context.body_input,state.context.source,state.context.tick,state.context.copy.thread,state.context.basis_ns)==gears_body_tick_input::Verdict::okay;
    auto const& source=state.context.source;const auto now=rig_tracking_live::clock_ns();constexpr uint64_t budget=40000000;
    const auto scene_ns=failure::ns(uint64_t(source.scene_qpc),source.qpc_frequency);
    return gears_native_rig_tracking::core_snapshot_valid(source,now,budget,simulator_operator)&&scene_ns&&scene_ns<=INT64_MAX&&
        rig_tracking_live::source_fresh(int64_t(scene_ns),now,budget)&&
        rig_tracking_live::source_fresh(source.stereo.received_ns,now,budget)&&
        (!(source.flags&gears_rig_tracking::controllers_matched)||rig_tracking_live::source_fresh(source.controllers.received_ns,now,budget));
}
void pose_begin(Entry const& e) noexcept {Call call;if(e.kind==0)budget::recorder.mark_entry(budget::Stage::native_entry,reinterpret_cast<uintptr_t>(&e),e.invocation);
    using Begin=held_outcome::NativeBegin;
    const auto note=[&](Begin result){held_outcome::recorder.native_result(reinterpret_cast<uintptr_t>(&e),e.invocation,result);ready_wrist::native_result(e,result);};
    if(!enabled||stopping.load()||e.kind!=0||!e.selected){note(Begin::policy);return;}
    DWORD expected{};owner_thread.compare_exchange_strong(expected,GetCurrentThreadId());if(owner_thread.load()!=GetCurrentThreadId()){note(Begin::thread);++rejected;return;}
    ready_wrist::begin_unknown(e);
    if(state.captured){note(Begin::already_captured);++rejected;return;}
    if(state.kernel.pending()){note(Begin::kernel_pending);++rejected;return;}
    try{
        if(state.kernel.settled_source_age()&&!state.recovery.active){auto& old=state.recovery;
            old.actor=state.context.actor;old.asset=state.context.asset;old.source=state.context.source;old.metadata=state.backend.metadata();
            old.stamp=state.stamp;old.desired=state.desired;old.reference=state.source.reference;old.units=state.source.units_per_metre;old.model_scale=state.source.model_scale;old.active=true;}
        state.backend.~Backend();::new(static_cast<void*>(&state.backend)) native::Backend;
        state.lease.~Lease();::new(static_cast<void*>(&state.lease)) tx::Lease;
        state.context.entry=&e;std::memcpy(&state.context.copy,&e,sizeof e);state.context.owner=hand::slot.data.owner.key;
        state.context.source=hand::slot.data.record.source;state.context.tick=hand::slot.tick;state.context.expired=false;
        state.context.tick_entry=hand::slot.tick_address;if(!state.context.tick_entry){note(Begin::tick_entry);state.context.expired=true;++rejected;return;}
        std::memcpy(&state.context.tick_copy,state.context.tick_entry,sizeof state.context.tick_copy);
        state.context.world_lease=hand::slot.lease;state.context.conflict=hand::slot.conflict;
        state.context.actor=hand::slot.data.owner.binding;state.context.asset=hand::slot.data.asset;
        state.context.body_input={};state.context.basis_ns=0;state.context.mount_parent={};
        state.context.body_source={};state.context.source_parent={};state.context.queried_parent={};state.body_query_active=false;
        if(body_tick_source_live::enabled){
            const bool copied=body_tick_source_live::copy(state.context.tick,e.thread,state.context.source,state.context.body_source);
            ready_wrist::begin(e,copied);
            if(!copied){note(Begin::body_source_copy);state.context.expired=true;++rejected;return;}
        }else if(body_tick_input_live::enabled&&!body_tick_input_live::copy(state.context.tick,e.thread,state.context.source,state.context.body_input)){note(Begin::body_input_copy);state.context.expired=true;++rejected;return;}
        tx::Stamp seed{};seed.binding.owner=state.context.owner.mesh.address;seed.binding.model=hand::slot.data.asset.address;
        seed.binding.metadata_generation=state.context.owner.world_generation;seed.binding.session=state.context.owner.session;
        seed.binding.world_generation=state.context.owner.world_generation;
        // Private real-event order stamp; not a Core/tick number. Actual tick/
        // Compose identities remain independently checked by ActualContext.
        seed.frame=seed.producer=seed.scope=uint64_t(phase::qpc());
        state.prepared=false;state.notified=false;state.atom_installed=false;state.hand_record=0;
        state.right_mount=hands::adapter::MountKind::free_hand;state.mount_response={};
        state.joint_committed=false;state.consumer_verified=false;state.hand_work.solved_mask=0;state.alpha_grant_mask=state.alpha_grant_count=0;
        state.alpha_grant_nodes={};state.alpha_grant_original={};
        if(body_tick_input_live::enabled)state.context.basis_ns=rig_tracking_live::clock_ns();
        native::CaptureObservation capture_observation;
        const bool capture_ok=budget::bracket(budget::Stage::capture_begin,budget::Stage::capture_end,[&]{return state.backend.capture(state.context,state.context.owner,seed,state.source,state.desired,
            (held_outcome::recorder.exact(reinterpret_cast<uintptr_t>(&e),e.invocation)||ready_wrist::current(e))?&capture_observation:nullptr);});
        held_outcome::recorder.capture_result(reinterpret_cast<uintptr_t>(&e),e.invocation,capture_observation.reason,capture_observation.context_verdict);
        ready_wrist::captured(e,capture_observation,capture_ok);
        if(!capture_ok){note(Begin::backend_capture);state.context.expired=true;state.backend.invalidate();++rejected;return;}
        if(!budget::bracket(budget::Stage::model_begin,budget::Stage::model_end,[&]{return body::prepare_model(state.source.reference,digest,state.model);})){note(Begin::prepare_model);state.context.expired=true;state.backend.invalidate();++rejected;return;}
        state.stamp=state.backend.stamp();state.captured=true;++captures;
        if(state.recovery.active){
            const auto recovery=recovery_current();
            if(recovery==RecoveryResult::defer){note(Begin::recovery_defer);state.context.expired=true;state.backend.invalidate();state.captured=false;++recovery_deferred;return;}
            if(recovery!=RecoveryResult::okay){note(Begin::recovery_hard);state.kernel.request_stop();++recovery_rejected;return;}
            // Old Kernel has no fields, lease or protocol debt and an exact
            // normal end. Only the GT owns its lifetime; external drain touches
            // the permanent stopping flag, never this reconstructible object.
            const auto serial=state.kernel.serial();state.kernel.~Kernel();::new(static_cast<void*>(&state.kernel)) tx::Kernel(serial);
            state.recovery={};++age_recovered;sync();}
        note(Begin::captured);
    }catch(...){note(Begin::exception);state.context.expired=true;state.backend.invalidate();++rejected;}
}
bool apply_impl(Entry const& e,body::Goal const& goal,gears_native_rig_tracking::CoreSnapshot const& source,gears_vr::StereoMatrix const* render_base,
    gears_body_head_authored::Marker const* marker=nullptr,gears_vr::Pose const* eye=nullptr) noexcept {
    Call call;budget::recorder.mark(budget::Stage::apply_entry);++failure_counts.calls;failure::Gate gate=failure::Gate::okay;failure::Timing timing{};
    if(diagnostic_first_failure)timing.begin=uint64_t(phase::qpc());
    if(!enabled)gate=failure::Gate::disabled;else if(stopping.load())gate=failure::Gate::stopping;
    else if(GetCurrentThreadId()!=owner_thread.load())gate=failure::Gate::thread;else if(!state.captured)gate=failure::Gate::capture;
    else if(state.prepared)gate=failure::Gate::prepared;else if(state.notified)gate=failure::Gate::notified;
    else if(!same_entry(e))gate=failure::Gate::entry;else if(std::memcmp(&source,&state.context.source,sizeof source))gate=failure::Gate::source;
    else if(!state.context.current(tx::Stage::prepare))gate=failure::Gate::context;
    else if(!native_atom_overwrite_live::ready_for_commit())gate=failure::Gate::receipt;
    else if(body_tick_input_live::enabled&&(!render_base||!marker))gate=failure::Gate::context;
    budget::recorder.mark(budget::Stage::apply_gate_end);if(diagnostic_first_failure)timing.gate=uint64_t(phase::qpc());
    ++failure_counts.gates[unsigned(gate)];if(gate!=failure::Gate::okay){remember_failure(e,goal,failure::Stage::gate,unsigned(gate),timing);return false;}
    try{
        if(body_tick_source_live::enabled)held_outcome::recorder.input(state.context.body_source,state.context.basis_ns);
        state.body_render_base=render_base?*render_base:gears_vr::StereoMatrix{};
        if(!admission_budget()){budget::recorder.age_rejected();++admission_skipped;++rejected;return false;}
        const auto report=body::plan(state.model,state.source,goal,state.workspace,state.output);budget::recorder.mark(budget::Stage::body_plan_end);if(diagnostic_first_failure)timing.plan=uint64_t(phase::qpc());++failure_counts.plans[unsigned(report.status)];
        ready_wrist::body_plan(e,unsigned(report.status));
        if(report.status!=body::Status::solved){remember_failure(e,goal,failure::Stage::plan,unsigned(report.status),timing,&report);++rejected;return false;}
        state.right_mount=select_main_mount(source,marker);held_outcome::recorder.selected_mount(unsigned(state.right_mount));
        ready_wrist::mount(e,unsigned(state.right_mount));
        if(main_mount_policy_enabled()&&!admission_budget()){budget::recorder.mark(budget::Stage::post40_end);budget::recorder.age_rejected();++main_mount_post_admission_rejected;++admission_skipped;++rejected;return false;}
        budget::recorder.mark(budget::Stage::post40_end);
        bool hand_plan{};
        if(hands_enabled){++hands_attempts;hands::Result hand_result{};
            // The current standing policy is the only hands consumer. A body
            // pose can bootstrap the first camera basis, but cannot relabel a
            // previous FP basis or write a reconstructed Core/hand timestamp.
            if(!render_base)hand_result.status=hands::Status::basis;
            else if(goal.scope!=body::Scope::stationary||state.context.actor.special_move||
                (source.flags&gears_rig_tracking::cover_key_known)||source.cover_type||source.cover_link.address||source.cover_slot!=-1)
                hand_result.status=hands::Status::invalid_body;
            else if(eye&&first_person_live::body_basis&&body_tick_input_live::enabled)
                hand_result=body_tick_source_live::enabled?
                    hands::compose_eye(state.model,state.source,state.context.body_source,
                        e.invocation,rig_tracking_live::object(state.context.asset),*render_base,*eye,
                        rig_tracking_live::clock_ns(),simulator_operator,state.hand_work,state.output,state.right_mount):
                    hands::compose_eye(state.model,state.source,state.context.body_input,
                        e.invocation,rig_tracking_live::object(state.context.asset),*render_base,*eye,
                        rig_tracking_live::clock_ns(),simulator_operator,state.hand_work,state.output,state.right_mount);
            else if(body_tick_source_live::enabled)hand_result=hands::compose(state.model,state.source,state.context.body_source,rig_tracking_live::object(state.context.asset),
                *render_base,rig_tracking_live::clock_ns(),simulator_operator,state.hand_work,state.output,state.right_mount);
            else if(body_tick_input_live::enabled)hand_result=hands::compose(state.model,state.source,state.context.body_input,rig_tracking_live::object(state.context.asset),
                *render_base,rig_tracking_live::clock_ns(),simulator_operator,state.hand_work,state.output,state.right_mount);
            else hand_result=hands::compose(state.model,state.source,source,rig_tracking_live::object(state.context.asset),
                *render_base,rig_tracking_live::clock_ns(),simulator_operator,state.hand_work,state.output,state.right_mount);
            ready_wrist::target(e,hand_result,render_base);
            ++hands_statuses[unsigned(hand_result.status)];hand_plan=hand_result.status==hands::Status::okay&&state.hand_work.solved_mask!=0;
            if(hand_plan)++hands_solved;
            else if(hand_result.status!=hands::Status::basis&&hand_result.status!=hands::Status::hand_untracked&&
                    hand_result.status!=hands::Status::unreachable){++rejected;return false;}}
        budget::recorder.mark(budget::Stage::hands_end);if(!admission_budget()){budget::recorder.age_rejected();budget::recorder.mark(budget::Stage::final40_end);++admission_skipped;++rejected;return false;}
        budget::recorder.mark(budget::Stage::final40_end);auto result=state.kernel.prepare(state.backend,state.stamp,state.source,state.output,state.desired,state.lease);sync();
        budget::recorder.mark(budget::Stage::prepare_end);if(diagnostic_first_failure)timing.prepare=uint64_t(phase::qpc());
        held_outcome::recorder.prepared(unsigned(result.code));++failure_counts.prepares[unsigned(result.code)];
        ready_wrist::prepared(e,result);
        if(result.code!=tx::Code::prepared){remember_failure(e,goal,failure::Stage::prepare,unsigned(result.code),timing,&report,&state.output,result.unit);++rejected;return false;}state.prepared=true;
        result=state.kernel.commit(state.backend,state.lease,state.stamp);budget::recorder.mark(budget::Stage::commit_end);if(diagnostic_first_failure)timing.commit=uint64_t(phase::qpc());sync();install_atom_receipt();
        held_outcome::recorder.committed(unsigned(result.code));++failure_counts.commits[unsigned(result.code)];
        ready_wrist::committed(e,result);
        if(result.code==tx::Code::committed){++commits;if(hand_plan){state.joint_committed=true;++hands_commits;
            if(state.right_mount==hands::adapter::MountKind::held_lancer)++main_mount_commits;trace_hands_commit();
            if(state.right_mount==hands::adapter::MountKind::held_lancer)held_outcome::recorder.held(main_mount_commits.load(),state.hand_record);}return true;}
        remember_failure(e,goal,failure::Stage::commit,unsigned(result.code),timing,&report,&state.output,result.unit);++rejected;return false;
    }catch(...){state.kernel.request_stop();sync();++failure_counts.exceptions;remember_failure(e,goal,failure::Stage::exception,0,timing);++rejected;return false;}
}
bool apply(Entry const& e,body::Goal const& goal,gears_native_rig_tracking::CoreSnapshot const& source) noexcept {
    return apply_impl(e,goal,source,nullptr);
}
bool apply_with_view(Entry const& e,body::Goal const& goal,gears_native_rig_tracking::CoreSnapshot const& source,gears_vr::StereoMatrix const& render_base) noexcept {
    return apply_impl(e,goal,source,&render_base);
}
bool apply_with_view_marker(Entry const& e,body::Goal const& goal,gears_native_rig_tracking::CoreSnapshot const& source,
    gears_vr::StereoMatrix const& render_base,gears_body_head_authored::Marker const& marker,gears_vr::Pose const* eye) noexcept {
    return apply_impl(e,goal,source,&render_base,&marker,eye);
}
void before_original(Entry const& e) noexcept {Call call;
    if(ready_wrist::current(e)&&!state.prepared)ready_wrist::consumer_pre(e);
    if(e.kind!=0||GetCurrentThreadId()!=owner_thread.load()||!state.prepared)return;
    install_atom_receipt();
    auto key=state.stamp;if(!same_entry(e))key.scope=0;
    state.notified=true;const auto result=state.kernel.notify_consumer_entry(state.backend,state.lease,key);sync();
    state.consumer_verified=result.code==tx::Code::consumer_marked;
    if(state.hand_record){auto& row=hand_trace[(state.hand_record-1)%hand_trace_capacity];if(row.sequence==state.hand_record){
        row.consumer_qpc=uint64_t(phase::qpc());row.consumer_verified=result.code==tx::Code::consumer_marked;
        if(row.consumer_verified)row.consumer_status=observe_hand_trace(row,false);}}
    ready_wrist::consumer_pre(e);
}
void after_original(Entry const& e) noexcept {
    if(!ready_wrist::recorder.enabled||e.kind!=0||GetCurrentThreadId()!=owner_thread.load()||!state.captured)return;
    ready_wrist::raw_return(e);
}
void pose_end(Entry const& e,bool returned,bool original_called) noexcept {Call call;
    // Passive diagnostic only; no old C0 Context is made current at Tick end.
    if(e.kind==2)ready_wrist::outer_end(e,returned,original_called);
    ready_wrist::Finish ready_finish{e,returned,original_called};
    if(GetCurrentThreadId()!=owner_thread.load()||e.kind!=0||!state.captured)return;
    ready_wrist::returned_end(e,returned,original_called);
    if(state.prepared){auto key=state.stamp;if(!same_entry(e))key.scope=0;
        if(state.hand_record&&same_entry(e)&&returned&&original_called){auto& row=hand_trace[(state.hand_record-1)%hand_trace_capacity];
            if(row.sequence==state.hand_record){row.end_begin_qpc=uint64_t(phase::qpc());row.end_status=observe_hand_trace(row,true);row.end_end_qpc=uint64_t(phase::qpc());}}
        const auto result=state.kernel.finish(state.backend,state.lease,key,original_called?(returned?tx::End::original_returned:tx::End::original_threw):tx::End::before_consumer);sync();
        ready_wrist::closed(e,result);
        if(state.hand_record&&result.code==tx::Code::scope_closed&&returned&&original_called&&!state.kernel.failed()&&
           !state.kernel.protocol_debt()&&!state.kernel.original_exception()){
            auto& row=hand_trace[(state.hand_record-1)%hand_trace_capacity];if(row.sequence==state.hand_record){row.normal_return=1;
                row.alpha_grant_mask=state.alpha_grant_mask;row.alpha_grant_count=state.alpha_grant_count;
                std::copy(state.alpha_grant_nodes.begin(),state.alpha_grant_nodes.end(),row.alpha_grant_nodes);
                std::copy(state.alpha_grant_original.begin(),state.alpha_grant_original.end(),row.alpha_grant_original);}}}
    state.context.expired=true;state.backend.invalidate();state.captured=false;state.prepared=false;
    state.joint_committed=false;state.consumer_verified=false;
    // Kernel/Lease/Backend storage remains alive while pending; never restore at
    // tick-end, stop, teardown or value equality. Fresh is explicitly unavailable.
}
bool configure(wchar_t const* ini,FILE* f){wchar_t value[8]{};GetPrivateProfileStringW(L"FullBodyC0Native",L"enabled",L"0",value,8,ini);
    if(wcscmp(value,L"0")&&wcscmp(value,L"1"))return false;enabled=!wcscmp(value,L"1");
    if(enabled&&(!hand::enabled||!fresh_pose_producer_live::enabled))return false;
    wchar_t hands_value[8]{};GetPrivateProfileStringW(L"FullBodyC0Native",L"hands",L"0",hands_value,8,ini);
    if(wcscmp(hands_value,L"0")&&wcscmp(hands_value,L"1"))return false;hands_enabled=!wcscmp(hands_value,L"1");
    if(hands_enabled&&(!enabled||GetPrivateProfileIntW(L"BodyC0GoalPolicy",L"enabled",0,ini)!=1))return false;
    wchar_t alpha_value[8]{};GetPrivateProfileStringW(L"FullBodyC0Native",L"arm_control_ownership",L"0",alpha_value,8,ini);
    if(wcscmp(alpha_value,L"0")&&wcscmp(alpha_value,L"1"))return false;arm_alpha_enabled=!wcscmp(alpha_value,L"1");
    if(arm_alpha_enabled&&(!enabled||!hands_enabled||GetPrivateProfileIntW(L"CoverControlObserver",L"enabled",0,ini)!=1||
       GetPrivateProfileIntW(L"CoverFutureGObserver",L"enabled",0,ini)!=0))return false;
    wchar_t diagnostic[8]{};GetPrivateProfileStringW(L"FullBodyC0Native",L"diagnostic_first_failure",L"0",diagnostic,8,ini);
    if(wcscmp(diagnostic,L"0")&&wcscmp(diagnostic,L"1"))return false;diagnostic_first_failure=!wcscmp(diagnostic,L"1");
    wchar_t mount_value[8]{};GetPrivateProfileStringW(L"FullBodyC0Native",L"held_main_mount",L"0",mount_value,8,ini);
    if(wcscmp(mount_value,L"0")&&wcscmp(mount_value,L"1"))return false;held_main_mount=!wcscmp(mount_value,L"1");
    if(held_main_mount&&!main_mount_policy_enabled())return false;
    wchar_t trace_value[8]{};GetPrivateProfileStringW(L"FullBodyC0Native",L"diagnostic_held_main_mount",L"0",trace_value,8,ini);
    if(wcscmp(trace_value,L"0")&&wcscmp(trace_value,L"1"))return false;diagnostic_held_main_mount=!wcscmp(trace_value,L"1");
    if(diagnostic_held_main_mount&&(!held_main_mount||!diagnostic_first_failure))return false;
    if(!body_tick_tracking_probe_live::configure(ini,f))return false;
    if(!body_tick_input_live::configure(ini,f,enabled&&hands_enabled&&arm_alpha_enabled))return false;
    if(!body_tick_source_live::configure(ini,f,enabled&&hands_enabled&&arm_alpha_enabled&&held_main_mount))return false;
    wchar_t ready_value[8]{};GetPrivateProfileStringW(L"FullBodyC0Native",L"diagnostic_ready_wrist",L"0",ready_value,8,ini);
    if(wcscmp(ready_value,L"0")&&wcscmp(ready_value,L"1"))return false;
    ready_wrist::recorder.enabled=!wcscmp(ready_value,L"1");
    if(ready_wrist::recorder.enabled&&(!simulator_operator||!hands_enabled||!diagnostic_held_main_mount||!body_tick_source_live::enabled))return false;
    held_outcome::recorder.prequery_enabled=ready_wrist::recorder.enabled;
    budget::recorder.clock=[]()noexcept{return rig_tracking_live::clock_ns();};
    budget::recorder.thread=[]()noexcept{return uint64_t(GetCurrentThreadId());};
    budget::recorder.cookie=[]()noexcept{auto* broker=weapon_hand_runtime::current();return broker?broker->cookie():uint64_t(0);};
    budget::recorder.enabled=main_mount_policy_enabled()&&diagnostic_held_main_mount;
    held_outcome::recorder.clock=budget::recorder.clock;held_outcome::recorder.thread=budget::recorder.thread;
    held_outcome::recorder.cookie=budget::recorder.cookie;
    held_outcome::recorder.enabled=body_tick_source_live::enabled&&main_mount_policy_enabled()&&diagnostic_held_main_mount;
    if(f)fprintf(f,"full_body_c0_native_compiled=1 full_body_c0_native_enabled=%u synchronous_C0_inference=1 global_reader_fence=0 fresh_retirement=actual_atom_receipt goal_policy_external=1 body_hands_enabled=%u body_hands_length=preserve body_hands_writer=shared_C0 body_arm_control_ownership=%u\n",unsigned(enabled),unsigned(hands_enabled),unsigned(arm_alpha_enabled));return true;
}
void request_drain(){stopping=true;} // GT write guards observe this permanent flag; no Kernel lifetime race.
unsigned pending_count(){const uint64_t n=uint64_t(callbacks.load())+obligations.load();return n>UINT_MAX?UINT_MAX:unsigned(n);}
void export_failure(failure::First const&first,wchar_t const*name,std::atomic<bool>&attempted,std::atomic<bool>&written,std::atomic<unsigned>&error)noexcept {
    // Only the existing collector/stop-summary caller may do I/O; never the GT.
    auto const* record=first.published();if(!record||GetCurrentThreadId()==owner_thread.load()||attempted.exchange(true))return;
    wchar_t path[MAX_PATH]{};if(!directory[0]||swprintf_s(path,L"%s\\%s",directory,name)<=0){error=ERROR_INVALID_NAME;return;}
    HANDLE file=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){error=GetLastError();return;}
    DWORD bytes{};bool ok=WriteFile(file,record,sizeof *record,&bytes,nullptr)&&bytes==sizeof *record;
    if(!ok)error=GetLastError();if(!CloseHandle(file)){error=GetLastError();ok=false;}written=ok;
}
std::atomic<bool> budget_attempted{},budget_written{};std::atomic<unsigned> budget_error{};
void export_budget()noexcept {
    auto& r=budget::recorder;
    if(!r.enabled||!stopping.load()||callbacks.load()||obligations.load()||!hand::stopping.load()||hand::callbacks.load()||hand::live_frames.load()||r.active.load(std::memory_order_acquire)||
       GetCurrentThreadId()==owner_thread.load()||budget_attempted.exchange(true))return;
    wchar_t path[MAX_PATH]{};if(!directory[0]||swprintf_s(path,L"%s\\body-c0-budget.bin",directory)<=0){budget_error=ERROR_INVALID_NAME;return;}
    budget::Header h{};h.query_count=r.saved.load(std::memory_order_acquire);h.no_query_count=unsigned(r.no_query_saved.load(std::memory_order_acquire));
    h.pid=GetCurrentProcessId();h.native_started=uint64_t(session_started_qpc);h.attempts=r.attempts.load();h.ignored=r.ignored.load();h.nested=r.nested.load();
    if(h.query_count>32){budget_error=ERROR_INVALID_DATA;return;}
    HANDLE file=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);if(file==INVALID_HANDLE_VALUE){budget_error=GetLastError();return;}
    auto write=[&](void const* p,DWORD n){DWORD bytes{};const bool ok=WriteFile(file,p,n,&bytes,nullptr)&&bytes==n;if(!ok)budget_error=GetLastError();return ok;};
    bool ok=write(&h,sizeof h);if(ok&&h.query_count)ok=write(r.queries.data(),DWORD(h.query_count*sizeof(budget::Record)));
    if(ok&&h.no_query_count)ok=write(&r.no_query,sizeof r.no_query);
    if(!CloseHandle(file)){budget_error=GetLastError();ok=false;}budget_written=ok;
}
std::atomic<bool> held_outcome_attempted{},held_outcome_written{};std::atomic<unsigned> held_outcome_error{};
void export_held_outcome()noexcept {
    auto& r=held_outcome::recorder;
    if(!r.enabled||!stopping.load()||callbacks.load()||obligations.load()||!hand::stopping.load()||hand::callbacks.load()||hand::live_frames.load()||r.active.load(std::memory_order_acquire)||
       GetCurrentThreadId()==owner_thread.load()||held_outcome_attempted.exchange(true))return;
    wchar_t path[MAX_PATH]{};if(!directory[0]||swprintf_s(path,L"%s\\body-held-outcomes.bin",directory)<=0){held_outcome_error=ERROR_INVALID_NAME;return;}
    held_outcome::Header h{};h.count=r.saved.load(std::memory_order_acquire);h.pid=GetCurrentProcessId();h.native_started=uint64_t(session_started_qpc);
    h.arm_kind=r.arm_kind.load(std::memory_order_acquire);
    h.armed_ns=r.armed_ns;h.deadline_ns=r.deadline_ns;h.ignored_before_arm=r.ignored_before_arm;h.dropped_capacity=r.dropped_capacity;h.nested=r.nested;h.foreign_thread=r.foreign_thread.load();
    if(h.count>held_outcome::capacity){held_outcome_error=ERROR_INVALID_DATA;return;}
    HANDLE file=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);if(file==INVALID_HANDLE_VALUE){held_outcome_error=GetLastError();return;}
    auto write=[&](void const* p,DWORD n){DWORD bytes{};const bool ok=WriteFile(file,p,n,&bytes,nullptr)&&bytes==n;if(!ok)held_outcome_error=GetLastError();return ok;};
    bool ok=write(&h,sizeof h);if(ok&&h.count)ok=write(r.rows.data(),DWORD(h.count*sizeof(held_outcome::Record)));
    if(!CloseHandle(file)){held_outcome_error=GetLastError();ok=false;}held_outcome_written=ok;
}
void summary(FILE* f){ready_wrist::export_records();export_held_outcome();export_failure(first_failure,L"body-c0-first-failure.bin",failure_file_attempted,failure_file_written,failure_file_error);
    export_failure(first_commit_failure,L"body-c0-first-commit-failure.bin",commit_failure_file_attempted,commit_failure_file_written,commit_failure_file_error);
    export_hand_trace();export_budget();export_tick_commits();export_alpha_records();body_tick_input_live::summary(f);body_tick_source_live::summary(f);
    if(f)fprintf(f,"body_ready_wrist_enabled=%u body_ready_wrist_records=%u body_ready_wrist_active=%u body_ready_wrist_written=%u body_ready_wrist_error=%u ",
        unsigned(ready_wrist::recorder.enabled),ready_wrist::recorder.saved.load(),unsigned(ready_wrist::recorder.active.load()),unsigned(ready_wrist::written.load()),ready_wrist::error.load());
    if(f)fprintf(f,"body_alpha_trace_records=%llu body_alpha_trace_written=%u body_alpha_trace_error=%u ",hand_trace_sequence,unsigned(alpha_written.load()),alpha_error.load());
    if(f)fprintf(f,"body_tick_commits_records=%llu body_tick_commits_written=%u body_tick_commits_error=%u ",hand_trace_sequence,unsigned(tick_commits_written.load()),tick_commits_error.load());
    body_tick_tracking_probe_live::summary(f,stopping.load()&&!callbacks.load()&&!obligations.load()&&hand::stopping.load()&&!hand::callbacks.load()&&!hand::live_frames.load()&&GetCurrentThreadId()!=owner_thread.load());
    if(f){fprintf(f,"body_budget_enabled=%u body_budget_queries=%u body_budget_no_query=%u body_budget_active=%u body_budget_written=%u body_budget_error=%u ",
        unsigned(budget::recorder.enabled),budget::recorder.saved.load(),unsigned(budget::recorder.no_query_saved.load()),unsigned(budget::recorder.active.load()),unsigned(budget_written.load()),budget_error.load());}
    if(f)fprintf(f,"body_mount_prequery_enabled=%u body_mount_prequery_arm=%llu ",unsigned(held_outcome::recorder.prequery_enabled),held_outcome::recorder.arm_kind.load());
    if(f){fprintf(f,"body_native_enabled=%u body_native_captures=%llu body_native_commits=%llu body_native_rejected=%llu body_native_pending=%u body_native_fresh_retirement=atom_receipt body_native_admission_skipped=%llu body_native_age_recovered=%llu body_native_recovery_rejected=%llu body_native_recovery_deferred=%llu ",unsigned(enabled),captures.load(),commits.load(),rejected.load(),pending_count(),admission_skipped.load(),age_recovered.load(),recovery_rejected.load(),recovery_deferred.load());
    fprintf(f,"body_hands_enabled=%u body_hands_attempts=%llu body_hands_solved=%llu body_hands_commits=%llu ",unsigned(hands_enabled),hands_attempts.load(),hands_solved.load(),hands_commits.load());
    fprintf(f,"body_arm_alpha_enabled=%u body_arm_alpha_owner_calls=%llu body_arm_alpha_owner_granted=%llu body_arm_alpha_owner_rejected=%llu body_arm_alpha_threshold_rejected=%llu ",unsigned(arm_alpha_enabled),arm_alpha_owner_calls.load(),arm_alpha_owner_granted.load(),arm_alpha_owner_rejected.load(),arm_alpha_threshold_rejected.load());
    fprintf(f,"body_hands_trace_attempted=%u body_hands_trace_written=%u body_hands_trace_error=%u ",unsigned(hand_trace_attempted.load()),unsigned(hand_trace_written.load()),hand_trace_error.load());
    fprintf(f,"body_main_mount_enabled=%u body_main_mount_attempts=%llu body_main_mount_ready=%llu body_main_mount_not_ready=%llu body_main_mount_context_rejected=%llu body_main_mount_stale=%llu body_main_mount_fault=%llu body_main_mount_commits=%llu body_main_mount_last_query=%llu body_main_mount_max_ns=%llu body_main_mount_trace_selected_only=%u ",
        unsigned(main_mount_policy_enabled()),main_mount_attempts.load(),main_mount_ready.load(),main_mount_not_ready.load(),main_mount_context_rejected.load(),main_mount_stale.load(),main_mount_fault.load(),main_mount_commits.load(),main_mount_last_query.load(),main_mount_max_ns.load(),unsigned(diagnostic_held_main_mount));
    fprintf(f,"body_main_mount_stale_projection=%llu body_main_mount_stale_broker=%llu body_main_mount_stale_c0=%llu body_main_mount_post_admission_rejected=%llu body_main_mount_last_callback_status=%u body_main_mount_last_source_failure_mask=%u body_main_mount_source_failure_query=%llu body_main_mount_c0_rejection=%u ",
        main_mount_stale_projection.load(),main_mount_stale_broker.load(),main_mount_stale_c0.load(),main_mount_post_admission_rejected.load(),main_mount_last_callback_status.load(),main_mount_last_source_failure_mask.load(),main_mount_source_failure_query.load(),main_mount_c0_rejection.load());
    for(unsigned i=0;i<main_mount_branches.size();++i)fprintf(f,"body_main_mount_query_branch_%u=%llu ",i,main_mount_branches[i].load());
    for(unsigned i=0;i<main_mount_last_failure_ages.size();++i)fprintf(f,"body_main_mount_last_failure_age_%u_ns=%lld ",i,main_mount_last_failure_ages[i].load());
    for(unsigned i=0;i<hands_statuses.size();++i)fprintf(f,"body_hands_status_%u=%llu ",i,hands_statuses[i].load());
    fprintf(f,"body_native_first_failure_enabled=%u body_native_commit_failure_written=%u body_native_commit_failure_error=%u ",unsigned(diagnostic_first_failure),unsigned(commit_failure_file_written.load()),commit_failure_file_error.load());
    fprintf(f,"body_native_apply_calls=%llu body_native_apply_exceptions=%llu body_native_failure_state=%u body_native_failure_written=%u body_native_failure_error=%u ",failure_counts.calls.load(),failure_counts.exceptions.load(),first_failure.state(),unsigned(failure_file_written.load()),failure_file_error.load());
    for(unsigned i=0;i<unsigned(failure::Gate::count);++i)fprintf(f,"body_native_apply_gate_%u=%llu ",i,failure_counts.gates[i].load());
    for(unsigned i=0;i<failure::statuses;++i)fprintf(f,"body_native_plan_status_%u=%llu ",i,failure_counts.plans[i].load());
    for(unsigned i=0;i<failure::codes;++i)fprintf(f,"body_native_prepare_code_%u=%llu body_native_commit_code_%u=%llu ",i,failure_counts.prepares[i].load(),i,failure_counts.commits[i].load());}}
void atom_overwrite_completed(uint64_t event)noexcept {
    if(GetCurrentThreadId()!=owner_thread.load()||!state.atom_installed||!state.kernel.outstanding_fields())return;
    const auto version=native_atom_overwrite_live::journal_version();const auto result=state.kernel.retire_fresh(state.backend,{event});sync();
    if(result.code==tx::Code::fresh_retired){native_atom_overwrite_live::retired(version,event);state.atom_installed=false;}
}
#else
bool configure(wchar_t const* ini,FILE*){return GetPrivateProfileIntW(L"FullBodyC0Native",L"enabled",0,ini)==0&&GetPrivateProfileIntW(L"FullBodyC0Native",L"hands",0,ini)==0&&GetPrivateProfileIntW(L"FullBodyC0Native",L"arm_control_ownership",0,ini)==0&&GetPrivateProfileIntW(L"FullBodyC0Native",L"held_main_mount",0,ini)==0&&GetPrivateProfileIntW(L"FullBodyC0Native",L"diagnostic_held_main_mount",0,ini)==0;}
bool main_mount_policy_enabled() noexcept{return false;}
bool body_mount_policy_enabled() noexcept{return false;}
bool body_mount_source_current(gears_body_main_mount_provider::Source const&) noexcept{return false;}
bool arm_control_policy_enabled() noexcept{return false;}
bool arm_control_policy_active() noexcept{return false;}
gears_body_c0_arm_alpha::RetainedControl retained_control_current(phase::Scope const&,uint64_t) noexcept{return gears_body_c0_arm_alpha::RetainedControl::not_applicable;}
bool arm_control_scope(phase::Scope const*& scope,uint64_t& outer_tick) noexcept{scope=nullptr;outer_tick=0;return false;}
bool owns_arm_control(gears_body_c0_arm_alpha::Proof const&) noexcept{return false;}
void arm_weight_return(gears_body_c0_arm_alpha::WeightReturn const&) noexcept{}
void pose_begin(Entry const&) noexcept {}bool apply(Entry const&,body::Goal const&,gears_native_rig_tracking::CoreSnapshot const&) noexcept{return false;}
bool apply_with_view(Entry const&,body::Goal const&,gears_native_rig_tracking::CoreSnapshot const&,gears_vr::StereoMatrix const&) noexcept{return false;}
bool apply_with_view_marker(Entry const&,body::Goal const&,gears_native_rig_tracking::CoreSnapshot const&,gears_vr::StereoMatrix const&,gears_body_head_authored::Marker const&,gears_vr::Pose const*) noexcept{return false;}
void before_original(Entry const&) noexcept {}void after_original(Entry const&) noexcept {}void pose_end(Entry const&,bool,bool) noexcept {}void request_drain(){}unsigned pending_count(){return 0;}void summary(FILE*){}
void atom_overwrite_completed(uint64_t)noexcept{}
#endif
}
