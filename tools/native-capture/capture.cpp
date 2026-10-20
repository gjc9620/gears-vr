#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <bcrypt.h>
#include <intrin.h>
#include <array>
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cwchar>
#include <cstring>
#include <share.h>
#include <cmath>
#include "MinHook.h"
#include "../vr-common/pose_bridge.h"
#include "../vr-common/stereo_bridge.h"
#include "../vr-common/gpu_share.h"
#if defined(GEARS_DESKTOP_MIRROR) && GEARS_DESKTOP_MIRROR
#include "../vr-common/gpu_mirror.h"
#endif
#include "../vr-common/stereo_ticket_store.h"
#include "render_cost.h"
#include "atomic_status_file.h"
#include "resource_resize_probe.h"
#include "swapchain_resize_mode.h"
#include "swapchain_startup_trace.h"
#include "source_view_shadow.h"
#include "native_source_view_args.h"
#include "source_eye_args.h"
#include "two_view_renderer_shadow.h"
#include "split_source_views.h"
#include "submitted_view_lifetime.h"
#include "continuous_owner.h"
#include "target_prime_owner.h"
#include "gpu_viewport_stats.h"
#include "owned_attachment_observer.h"
#include "owned_depth_clear_trial.h"
#include "frame_link.h"
#include "final_eye_diagnostics.h"
#include "owned_plain_output.h"
#include "final_transfer_policy.h"
#include "dlss_output_control.h"
#include "ngx_route_observer.h"
#include "ngx_stereo_gpu_submit.h"
#include "ngx_typed_copy.h"
#include "ngx_typed_quartet.h"
#include "ngx_epoch_pair_probe.h"
#if defined(GEARS_NGX_MV_DESCRIPTOR_OBSERVER) && GEARS_NGX_MV_DESCRIPTOR_OBSERVER
#include "ngx_mv_descriptor_provenance.h"
#include "ngx_mv_heap_census.h"
#if defined(GEARS_NGX_STEREO_HOST_OBSERVER) && GEARS_NGX_STEREO_HOST_OBSERVER && \
    defined(GEARS_NGX_EPOCH_PAIR_OBSERVER) && GEARS_NGX_EPOCH_PAIR_OBSERVER
#include "ngx_mv_target_window.h"
#include "ngx_mv_sdk_receipt.h"
#include "ngx_mv_compute_binding.h"
#include "ngx_mv_compute_snapshot.h"
#include "ngx_root_signature_provenance.h"
#endif
#endif
#if defined(GEARS_NGX_OUTPUT_DELAYED_CAPTURE) && GEARS_NGX_OUTPUT_DELAYED_CAPTURE
#include "ngx_output_delayed_gate.h"
#endif
#include "outage_trace.h"
#include "outage_sources.h"
#include "anchor_observer.h"
#if defined(GEARS_PAWNLESS_CAMERA_OBSERVER) && GEARS_PAWNLESS_CAMERA_OBSERVER
#include "pawnless_camera_observer.h"
#endif
#include "camera_closer.h"
#if defined(GEARS_SIXDOF_TICKET_DIAGNOSTICS) && GEARS_SIXDOF_TICKET_DIAGNOSTICS
#include "../vr-common/sixdof_ticket_mailbox.h"
#endif
#include "anchor_config_loader.h"
#include "hook_entry_attestation.h"
#include "world_rebind_metadata.h"
#include "world_epoch_gate.h"
#include "world_status_cadence.h"
#include "ui_observer.h"
#include <exception>
#if defined(GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER) && GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER
#include "crosshair_assignment_config_loader.h"
#include "mounted_recovery_guard.h"
#include "crosshair_tick_evidence.h"
#include "crosshair_finite_query.h"
#include "crosshair_weapon_binding.h"
#endif
#include "ui_canvas_batch.h"
#include "ui_canvas_text_batch.h"
#include "ui_payload_receipts.h"
#include "ui_index_membership.h"
#include "ui_hud_plan.h"
#include "ui_hud_layout.h"
#include "ui_reticle_direction_plan.h"
#include "ui_scene_transform_trial.h"
#include "path_choice_position_trial.h"
#if (defined(GEARS_UI_SCENE_SOURCE_TRIAL) && GEARS_UI_SCENE_SOURCE_TRIAL) || (defined(GEARS_PATH_CHOICE_POSITION_TRIAL) && GEARS_PATH_CHOICE_POSITION_TRIAL)
#include "native_metadata_contract.h"
#endif
#include <limits>
#include <memory>
#include <type_traits>
#include "swapchain_binding_observer.h"

namespace {
struct Vec {float x,y,z;};
struct Rot {int pitch,yaw,roll;};
using SceneFn=void*(__fastcall*)(void*,void*,Vec*,Rot*,void*,void*);
using EventFn=void(__fastcall*)(void*,Vec*,Rot*);
using FamilyFn=void(__fastcall*)(void*,void*);
FamilyFn original_family{};
using CloneFn=void*(__fastcall*)(void*,void*);
CloneFn original_clone{};
void* clone_target{};
thread_local void* submitting_family{};
using RendererFn=void*(__fastcall*)(void*,void*,void*,void*);
RendererFn original_renderer{};
void* renderer_target{};
using ConsumeFn=void(__fastcall*)(void*);
using DestroyRendererFn=void*(__fastcall*)(void*);
ConsumeFn original_consume{};
DestroyRendererFn original_destroy_renderer{};
void* consume_target{};void* destroy_renderer_target{};
ConsumeFn original_prepare{};
void* prepare_target{};
thread_local void* consuming_renderer{};
thread_local bool consuming_owned_scene{};
bool family_diagnostics{};
bool shadow_view_enabled{};
std::atomic<unsigned> shadow_attempted{0};
gears_shadow::API shadow_api{};
gears_shadow::Result shadow_result{};
bool constructor_shadow_enabled{};
bool constructor_state_enabled{};
bool constructor_eye_enabled{};
bool renderer_pair_enabled{};
bool submit_pair_enabled{};
bool projection_control{};
bool stereo_xr{};
bool simulator_operator{};
bool frame_link_diagnostics{};
bool stereo_gpu_publish{};
gears_vr::GpuPublisher stereo_publisher;
#if defined(GEARS_DESKTOP_MIRROR) && GEARS_DESKTOP_MIRROR
gears_vr::DesktopMirror desktop_mirror;
std::atomic<unsigned long long> mirror_inventory_rejected{},mirror_present1_full{};
#endif
#if defined(GEARS_NATIVE_OWNED_XR_PROBE) && GEARS_NATIVE_OWNED_XR_PROBE
constexpr bool owned_xr_probe=true;
#else
constexpr bool owned_xr_probe=false;
#endif
gears_vr::StereoTicketStore stereo_ticket_store;
uint64_t stereo_window_retained{};
thread_local gears_submitted::FrameTicket consuming_frozen_frame{};
thread_local unsigned stereo_publish_bypass{};
struct StereoPublishBypass {StereoPublishBypass(){++stereo_publish_bypass;}~StereoPublishBypass(){--stereo_publish_bypass;}};
std::atomic<LONGLONG> stereo_publish_deadline{};
struct StereoPublishCounts {
    std::atomic<uint64_t> bind_requests{},prepare_ok{},prepare_failed{},ticket_retains{},ticket_duplicates{},ticket_rejected{},ticket_missing{},rect_rejected{},capture_attempts{},capture_skipped{},last_ticket{};
    std::atomic<unsigned> stopped{},drain_ok{},close_ok{};
} stereo_publish_counts;
gears_frame_link::Tracker frame_link;
gears_swapchain_binding::Observer swapchain_bindings;
struct BindingOwner {IUnknown* chain{};IUnknown* queue{};};
std::array<BindingOwner,8> binding_owners{};
std::mutex binding_owner_mutex;
// The capture hooks enter this lock thousands of times per submitted frame.
// CRITICAL_SECTION retains recursive ownership on Windows while avoiding the
// heavier std::recursive_mutex path on the render thread.
class FrameHookMutex {
    CRITICAL_SECTION section_{};
public:
    FrameHookMutex(){InitializeCriticalSection(&section_);SetCriticalSectionSpinCount(&section_,400);}
    ~FrameHookMutex(){DeleteCriticalSection(&section_);}
    FrameHookMutex(const FrameHookMutex&)=delete;
    FrameHookMutex& operator=(const FrameHookMutex&)=delete;
    void lock(){EnterCriticalSection(&section_);}
    bool try_lock(){return TryEnterCriticalSection(&section_)!=0;}
    void unlock(){LeaveCriticalSection(&section_);}
};
FrameHookMutex frame_hook_mutex;
thread_local uint64_t consuming_frame_ticket{};
struct FinalEye {bool valid{}; unsigned eye{}; gears_frame_link::Rect rect{}; unsigned width{},height{};};
bool owned_plain_output_attested{};
FinalEye classify_final_eye(uintptr_t caller,uint64_t ticket,float x,float y,float width,float height,unsigned target_width,unsigned target_height,bool source_attested=false,bool output_controlled=false,bool owned_plain=false) {
    bool exact=(gears_final_transfer::eye_caller(caller,source_attested,output_controlled)||(owned_plain&&(caller==0xd83cbf||caller==0x2b11a0)))&&ticket&&target_width&&!(target_width%2)&&target_height&&std::isfinite(x)&&
        std::isfinite(y)&&std::isfinite(width)&&std::isfinite(height)&&y==0&&height==float(target_height)&&
        width==float(target_width/2)&&(x==0||x==float(target_width/2));
    if(!exact)return {};
    return {true,x==0?0u:1u,{x==0?0u:target_width/2,0,target_width/2,target_height},target_width,target_height};
}
thread_local FinalEye final_eye{};
thread_local gears_final_eye_diagnostics::Candidate final_eye_candidate{};
gears_final_eye_diagnostics::Recorder final_eye_failures;
// Last inventory already read by Present; no diagnostic COM calls or retained
// resource interfaces. Protected by frame_hook_mutex like Draw/Present.
struct FinalEyeInventory {
    uintptr_t chain{};uint64_t epoch{};long long qpc{};
    unsigned width{},height{},count{};bool valid{};std::array<uintptr_t,16> resources{};
} final_eye_inventory;
void print_late_quad(FILE* status);
void observe_late_execute(uintptr_t queue,gears_frame_link::ListRef ref,uint64_t order,unsigned member);
void print_unpossessed_quads(FILE* status);
void observe_unpossessed_quad_execute(uintptr_t queue,gears_frame_link::ListRef ref,uint64_t order,unsigned member);
namespace death_source_live {void progress(FILE*);void print(FILE*);unsigned status_interval_ms();}
namespace death_hdr_timeline {void print(FILE*);void executed(uintptr_t,gears_frame_link::ListRef,uint64_t,unsigned);}
namespace death_constant_observer {void print(FILE*);}
namespace dlss_route_observer {void print(FILE*);void draw(uintptr_t,uintptr_t,unsigned,unsigned,unsigned);}
namespace ngx_route_observer {void print(FILE*);}
namespace owned_ui_live {void progress(FILE*);}
namespace owned_material {void progress(FILE*);}
namespace ngx_mv_descriptor_capture {void arm();void close();void release_drained();void print(FILE*);
void accepted_eval(unsigned,uint64_t,uint64_t,uint64_t,uintptr_t,uintptr_t,uintptr_t,gears_ngx_stereo::TypedResourceLease const*);
#if defined(GEARS_NGX_MV_DESCRIPTOR_OBSERVER) && GEARS_NGX_MV_DESCRIPTOR_OBSERVER
void stop_target();
bool snapshot_target_seed(uint64_t&,gears_ngx_mv_census::Use&,gears_ngx_stereo::TypedResourceLease&);
#if defined(GEARS_NGX_STEREO_HOST_OBSERVER) && GEARS_NGX_STEREO_HOST_OBSERVER && defined(GEARS_NGX_EPOCH_PAIR_OBSERVER) && GEARS_NGX_EPOCH_PAIR_OBSERVER
gears_ngx_mv_compute::DescriptorLookup lookup_target(uintptr_t,uint64_t);
using ExactTarget=gears_ngx_mv_compute::ExactTarget;
using gears_ngx_mv_compute::same_exact;
ExactTarget lookup_exact_target(uint64_t);
#endif
#endif
}
namespace death_dof_control {void print(FILE*);}
namespace vr_dof_compat {void print(FILE*);void progress(FILE*);void retire();bool publication(uint64_t,uint64_t);bool retire_owned(uint64_t,uint64_t);}
namespace death_pso_cache {void print(FILE*);void poll();}
namespace gears_death_hdr {struct Event;}
namespace death_hdr_pixels {
void sample(gears_death_hdr::Event const&,uintptr_t);void poll();void print(FILE*);void progress(FILE*);
std::unique_lock<FrameHookMutex> execution_lock();
bool prepare_copy(ID3D12GraphicsCommandList*,const D3D12_TEXTURE_COPY_LOCATION*,UINT,UINT,UINT,const D3D12_TEXTURE_COPY_LOCATION*,const D3D12_BOX*,uintptr_t);
void append_after_original(bool,ID3D12GraphicsCommandList*,const D3D12_TEXTURE_COPY_LOCATION*,const D3D12_TEXTURE_COPY_LOCATION*);
}
void print_backbuffer_copies(FILE* status);
void observe_backbuffer_copy_execute(uintptr_t queue,gears_frame_link::ListRef ref,uint64_t order,unsigned member);
namespace final_transfer_live {
namespace normalizer {bool publication_allowed();}
bool enabled();bool tracks(gears_frame_link::ListRef,uintptr_t);
bool observe(ID3D12GraphicsCommandList*,uintptr_t,unsigned,unsigned,unsigned,gears_viewport::TargetDraw const&,gears_frame_link::ListRef);
void record_source_draw();void print(FILE*);
}
#include "dlss_output_control_live.h"
#include "ngx_output_pixels_live.h"
#if defined(GEARS_NGX_MV_DESCRIPTOR_OBSERVER) && GEARS_NGX_MV_DESCRIPTOR_OBSERVER
// The integration wiring macro is intentionally declared later, next to the
// implementation includes. Header layout must use the early build options.
static_assert(GEARS_NGX_MV_TARGET_CAPTURE==1,"MV descriptor candidate must compile the target scheduler");
#else
static_assert(GEARS_NGX_MV_TARGET_CAPTURE==0,"Default capture must keep target scheduling disabled");
#endif
namespace ngx_mv_target_capture {
void poll();void close();void release_drained();void print(FILE*);
#if GEARS_NGX_MV_TARGET_CAPTURE
void invalidate(unsigned);
void eval_enter(ngx_stereo_host::WrapperScope&,bool,uintptr_t,uintptr_t,void const*,bool);
void eval_leave(ngx_stereo_host::WrapperScope&,bool,bool,uint64_t);
void sdk_enter(ngx_stereo_host::WrapperScope*,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
void sdk_leave(ngx_stereo_host::WrapperScope*,uint32_t);
#endif
}
namespace ngx_mv_compute_capture {
void stop();void release_drained();void print(FILE*);void register_worker();void poll_picked_blob();
}
#include "owned_source_pixels.h"
struct FrameLinkCounters {
    std::atomic<uint64_t> resets{},reset_failed{},draws{},filtered{},unknown{},missing_target{},missing_ticket{},bad_quad{},
        executes{},execute_rejected{},presents{},linked{},contaminated{},present_failed{},tests{},unbound{},changed{},last_ticket{};
    std::array<std::atomic<uint64_t>,12> rejection{};
};
FrameLinkCounters frame_counts;
struct ExecuteQueueCount {
    uintptr_t identity{};unsigned type{};uint64_t calls{},members{},unknown_members{},rejected{};
    uint64_t post_anchor_calls{},post_anchor_unknown{};
    std::array<uintptr_t,16> first_unknown{};unsigned first_unknown_count{};
};
std::array<ExecuteQueueCount,16> frame_queues{};
std::array<uintptr_t,4096> frame_unique_lists{};
uint64_t frame_unique_list_overflow{},frame_unique_queue_overflow{};
unsigned frame_unique_list_count{};
bool frame_trial_anchored{};
struct FrameReadFailure {unsigned stage{},index{};HRESULT result{};};
FrameReadFailure frame_first_read_failure;
void frame_read_failure(unsigned stage,unsigned index,HRESULT result){
    if(!frame_first_read_failure.stage)frame_first_read_failure={stage,index,result};
}
struct FrameBufferCount {std::atomic<uintptr_t> resource{};std::atomic<uint64_t> draws{},left{},right{},unknown{},presents{},linked{},contaminated{};};
std::array<FrameBufferCount,32> frame_buffers{};
FrameBufferCount* frame_buffer_count(uintptr_t resource) {
    if(!resource)return nullptr;
    for(auto& entry:frame_buffers)if(entry.resource.load()==resource)return &entry;
    for(auto& entry:frame_buffers){uintptr_t empty{};if(entry.resource.compare_exchange_strong(empty,resource))return &entry;}
    frame_link.invalidate();return nullptr;
}
double stereo_units_per_metre=100;
double camera_closer_metres{};
std::atomic<unsigned long long> camera_closer_applied{},camera_closer_inactive{},camera_closer_rejected{};
gears_vr::StereoBridge stereo_bridge;
gears_vr::StereoGate stereo_gate;
std::mutex stereo_mutex;
std::atomic<bool> outage_diagnostics{false};
gears_outage::ActivitySnapshot outage_built;
gears_outage::Window outage_window;
// Same frame_hook_mutex as Draw/Execute/outer Present. TLS contains only this
// hook call's existing overlay query result; no extra tracker query is issued.
gears_outage_sources::Recorder outage_sources;
gears_outage_sources::Interval outage_source_interval;
thread_local gears_outage_sources::Overlay outage_source_overlay;
bool anchor_observer_enabled=false;
std::atomic<unsigned long long> anchor_submission_matched{0},anchor_submission_rejected{0};
std::array<std::atomic<unsigned long long>,3> anchor_pawn_reassociated_guards{};
std::array<std::atomic<unsigned long long>,3> anchor_unpossessed_guards{};
struct UnpossessedSubmission {uint64_t ticket{},world{};long long qpc{};unsigned thread{};};
constexpr unsigned unpossessed_submission_capacity=512;
std::array<UnpossessedSubmission,unpossessed_submission_capacity> unpossessed_submissions{};
std::atomic<unsigned> unpossessed_submission_count{};
std::atomic<uint64_t> unpossessed_first_ticket{},unpossessed_last_ticket{};
bool origin_trial_enabled=false;
gears_origin_trial::Trial origin_trial;
std::atomic<unsigned long long> origin_prepare_failed{0},origin_commit_failed{0};
gears_anchor::Config anchor_config;
void configure_unpossessed_camera(gears_anchor::Config& c){
#if defined(GEARS_UNPOSSESSED_VIEW_TARGET) && GEARS_UNPOSSESSED_VIEW_TARGET
    c.unpossessed_camera_enabled=c.cinematic_enabled&&!c.mounted_enabled;
#else
    c.unpossessed_camera_enabled=false;
#endif
}
std::atomic<uint64_t> world_candidate_generation{},world_prepare_requests{},world_prepare_rejected{};
std::atomic<uint64_t> committed_world_generation{1};
#if defined(GEARS_OWNED_WORLD_HANDOFF) && GEARS_OWNED_WORLD_HANDOFF
constexpr bool world_handoff_supported=true;
#elif (defined(GEARS_OWNED_UI_ROUTE_PROBE) && GEARS_OWNED_UI_ROUTE_PROBE) || (defined(GEARS_OWNED_OUTPUT_TRACKING) && GEARS_OWNED_OUTPUT_TRACKING) || (defined(GEARS_NATIVE_OWNED_PIXELS) && GEARS_NATIVE_OWNED_PIXELS)
constexpr bool world_handoff_supported=false;
#else
constexpr bool world_handoff_supported=true;
#endif
gears_world::EpochGate world_gate;
thread_local gears_world::ThreadState world_thread;
thread_local uint64_t thread_world_generation{};
thread_local gears_anchor::Context anchor_context;
struct AnchorRow {gears_anchor::Receipt receipt{};uintptr_t renderer{};unsigned long long ticket{};unsigned stage{},reason{};};
std::array<AnchorRow,64> anchor_rows{};
std::array<AnchorRow,16> anchor_rejects{};
std::atomic<unsigned> anchor_row_count{0},anchor_reject_count{0};
struct AnchorPawnReject {gears_anchor::PawnBindingDiagnostic diagnostic{};long long qpc{};unsigned thread{};};
std::array<AnchorPawnReject,16> anchor_pawn_rejects{};
std::atomic<unsigned> anchor_pawn_reject_count{};
#if defined(GEARS_PAWNLESS_CAMERA_OBSERVER) && GEARS_PAWNLESS_CAMERA_OBSERVER
struct PawnlessSceneRow {
    gears_pawnless_camera::Snapshot before{},after{};
    uintptr_t local{},family{},source{},renderer_family{},renderer_source{};
    long long begin_qpc{},end_qpc{},renderer_qpc{};unsigned thread{},read_mask{},renderer_thread{};
    std::array<unsigned char,0x430> scene_bytes{},renderer_bytes{};
};
std::array<PawnlessSceneRow,16> pawnless_scene_rows{};
std::mutex pawnless_scene_mutex;
unsigned pawnless_scene_count{};
std::atomic<long long> pawnless_scene_next{};
#endif
// Bounded observational state identities; no ViewState lifetime/compatibility assertion.
struct AnchorStateTransition {gears_anchor::Identity pawn{};uintptr_t source{},original_state{},right_state{},right_table{};
    long long qpc{};unsigned thread{},read_mask{};};
std::array<AnchorStateTransition,16> anchor_state_transitions{};
std::atomic<unsigned> anchor_state_transition_count{0};
// Four accepted live-family observations only. Pointer consistency is not an
// ownership/lifetime assertion; all bytes are copied before original_family.
struct AnchorTargetSnapshot {
    uintptr_t family{},source{},canvas{},target{},target_after{},vtable{},vtable_after{};
    long long qpc{};unsigned thread{},read_mask{},method_mask{};bool consistent{};
    std::array<unsigned char,96> target_bytes{};
    std::array<unsigned char,64> canvas_bytes{};
    std::array<uintptr_t,10> methods{};
    std::array<std::array<unsigned char,16>,10> prefixes{};
};
std::array<AnchorTargetSnapshot,4> anchor_target_snapshots{};
std::atomic<unsigned> anchor_target_snapshot_count{0};
thread_local gears_anchor::Identity anchor_last_state_pawn{};
thread_local uintptr_t anchor_last_observed_right_state{};
std::array<std::array<std::atomic<unsigned long long>,gears_anchor::reason_count>,3> anchor_reasons{};
std::atomic<long long> anchor_first_renderer{0},anchor_last_renderer{0},anchor_next_sample{0};
std::atomic<unsigned long long> anchor_sampled_out{0},anchor_capacity_dropped{0};
std::atomic<long long> anchor_dropped_first{0},anchor_dropped_last{0};
std::array<std::atomic<unsigned long long>,3> anchor_guard_calls{};
std::array<std::atomic<long long>,3> anchor_guard_min{LLONG_MAX,LLONG_MAX,LLONG_MAX},anchor_guard_max{};
std::array<gears_outage::Activity,2> outage_draws{}; // Existing frame_hook_mutex only.
std::array<unsigned long long,11> stereo_rejections{};
unsigned long long stereo_accepted{};
std::atomic<std::uint64_t> stereo_ticket_counter{0};
std::uint64_t stereo_first_ticket{},stereo_last_ticket{};
gears_vr::StereoSample stereo_first_sample{},stereo_last_sample{};
gears_vr::Pose stereo_reference{};
HANDLE stereo_recenter_event{};
bool stereo_recenter_key_down{};
std::atomic<unsigned long long> stereo_recenter_requests{},stereo_heading_deferred{},stereo_reference_generation{};
struct StereoReferenceEvent {
    std::uint64_t generation{},ticket{},session{},sequence{};
    long long qpc{},received_ns{};
    gears_vr::Pose reference{};
};
std::array<StereoReferenceEvent,32> stereo_reference_events{};
std::uint64_t stereo_reference_event_count{};
StereoReferenceEvent stereo_latest_reference{};
long long stereo_first_qpc{},stereo_last_qpc{};
bool submit_background{};
bool submit_output_bounds{};
bool submit_mono_control{};
bool submit_mono_independent_state{};
bool submit_duplicate_control{};
std::atomic<uintptr_t> submit_original_state{0};
std::atomic<unsigned> submit_attempted{0},submit_consumed{0},submit_prepared{0},submit_destroyed{0};
std::atomic<uintptr_t> submit_root{0};
std::atomic<unsigned> submit_owned_verified{0},submit_sources_released{0},submit_original_preserved{0};
unsigned submit_source_checks{},submit_rejection{};
struct SplitFailure {
    gears_split_sources::InputEvidence input{};
    uintptr_t before_owner{},before_state{},before140{},before418{};
    long long begin_qpc{},end_qpc{};
    unsigned rejection{},checks{};
};
SplitFailure first_split_failure;
std::atomic<unsigned> split_failure_phase{0};
std::atomic<uint64_t> source_map_candidates{},source_map_accepted{},source_map_rejected{};
std::atomic<unsigned> source_map_max_nodes{},source_map_last_checks{};
std::array<float,4> submit_rectangle{};
unsigned submit_pair_seconds{};
unsigned trial_seconds{30};
bool continuous_session{};
bool submit_session_enabled(){return continuous_session||submit_pair_seconds!=0;}
enum class SessionPhase:unsigned {running,stop_requested,native_closed,draining,stopped};
enum class SessionStop:unsigned {none,event,f8,focus_lost,proof_failure,owner_exited,owner_error};
DWORD session_owner_pid{};
std::uint64_t session_owner_creation_time{};
std::atomic<bool> session_owner_exit_observed{},session_owner_error_observed{};
std::atomic<SessionPhase> session_phase{SessionPhase::running};
std::atomic<SessionStop> session_reason{SessionStop::none};
std::atomic<ULONGLONG> session_stop_tick{};
std::atomic<LONGLONG> session_stop_qpc{};
std::atomic<DWORD> session_stop_thread{};
std::atomic<unsigned> session_stop_source_line{};
std::atomic<const char*> session_stop_source_file{"none"};
std::atomic<bool> session_stop_recorded{};
std::atomic<bool> session_cleanup_timeout{};
std::atomic<unsigned> session_native_services{};
std::atomic<unsigned> session_cancelled_builds{};
std::recursive_mutex session_native_mutex;
thread_local unsigned session_build_depth{};
bool session_status_available{};
LONGLONG session_started_qpc{};
struct SessionActivationGate {
    enum class State:unsigned {initializing,ready,closed};
    std::atomic<State> state{State::initializing};
    bool publish(){auto expected=State::initializing;
        return state.compare_exchange_strong(expected,State::ready,std::memory_order_release,std::memory_order_relaxed);}
    void close(){state.exchange(State::closed,std::memory_order_acq_rel);}
    bool ready()const{return state.load(std::memory_order_acquire)==State::ready;}
};
SessionActivationGate session_activation_gate;
std::atomic<LONGLONG> session_activation_ready_qpc{};
struct SessionHookEntry{void* target{};void* detour{};std::array<unsigned char,32> bytes{};};
#if defined(GEARS_NATIVE_RESIZE_PROBE) && GEARS_NATIVE_RESIZE_PROBE
constexpr size_t session_hook_base_capacity=60,viewport_hook_base_capacity=29;
#elif defined(GEARS_NATIVE_OWNED_PIXELS) && GEARS_NATIVE_OWNED_PIXELS
constexpr size_t session_hook_base_capacity=59,viewport_hook_base_capacity=29;
#elif defined(GEARS_NGX_STEREO_HOST_OBSERVER) && GEARS_NGX_STEREO_HOST_OBSERVER
constexpr size_t session_hook_base_capacity=42,viewport_hook_base_capacity=13;
#elif defined(GEARS_OWNED_OUTPUT_TRACKING) && GEARS_OWNED_OUTPUT_TRACKING
constexpr size_t session_hook_base_capacity=40,viewport_hook_base_capacity=11;
#else
constexpr size_t session_hook_base_capacity=39,viewport_hook_base_capacity=10;
#endif
#if defined(GEARS_NGX_MV_DESCRIPTOR_OBSERVER) && GEARS_NGX_MV_DESCRIPTOR_OBSERVER
constexpr unsigned ngx_mv_descriptor_hooks=5;
#else
constexpr unsigned ngx_mv_descriptor_hooks=0;
#endif
#if GEARS_NGX_MV_TARGET_CAPTURE
#if defined(GEARS_DEATH_PSO_CACHE) && GEARS_DEATH_PSO_CACHE
constexpr unsigned ngx_mv_compute_hooks=4;
#else
constexpr unsigned ngx_mv_compute_hooks=7;
#endif
#else
constexpr unsigned ngx_mv_compute_hooks=0;
#endif
constexpr size_t viewport_hook_capacity=viewport_hook_base_capacity+ngx_mv_descriptor_hooks+ngx_mv_compute_hooks;
#if defined(GEARS_OWNED_UI_ROUTE_PROBE) && GEARS_OWNED_UI_ROUTE_PROBE
#if defined(GEARS_OWNED_UI_MATERIAL) && GEARS_OWNED_UI_MATERIAL
constexpr size_t session_hook_capacity=session_hook_base_capacity+4;
#else
constexpr size_t session_hook_capacity=session_hook_base_capacity+2;
#endif
#else
constexpr size_t session_hook_capacity=session_hook_base_capacity;
#endif
#if defined(GEARS_GAME_RESOLUTION) && GEARS_GAME_RESOLUTION
constexpr unsigned game_resize_hooks=2;
#else
constexpr unsigned game_resize_hooks=0;
#endif
constexpr unsigned dxgi_resize_hooks=GEARS_DXGI_RESIZE_HOOKS?2:0;
#if defined(GEARS_GAME_UI_MATERIAL) && GEARS_GAME_UI_MATERIAL
constexpr unsigned game_material_hooks=1;
#else
constexpr unsigned game_material_hooks=0;
#endif
#if defined(GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER) && GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER
constexpr unsigned crosshair_assignment_hooks=1;
#else
constexpr unsigned crosshair_assignment_hooks=0;
#endif
#if defined(GEARS_UI_CALL_BOUNDARY_OBSERVER) && GEARS_UI_CALL_BOUNDARY_OBSERVER
constexpr unsigned ui_call_boundary_hooks=1;
#else
constexpr unsigned ui_call_boundary_hooks=0;
#endif
#if defined(GEARS_UI_SCENE_ROOT_OBSERVER) && GEARS_UI_SCENE_ROOT_OBSERVER
constexpr unsigned ui_scene_root_hooks=1;
#else
constexpr unsigned ui_scene_root_hooks=0;
#endif
#if defined(GEARS_PATH_CHOICE_LAYOUT_OBSERVER) && GEARS_PATH_CHOICE_LAYOUT_OBSERVER
constexpr unsigned path_layout_hooks=2;
#else
constexpr unsigned path_layout_hooks=0;
#endif
#if defined(GEARS_PATH_CHOICE_CLOSE_OBSERVER) && GEARS_PATH_CHOICE_CLOSE_OBSERVER
constexpr unsigned path_close_hooks=1;
#else
constexpr unsigned path_close_hooks=0;
#endif
#if defined(GEARS_DEATH_CONSTANT_OBSERVER) && GEARS_DEATH_CONSTANT_OBSERVER
constexpr unsigned death_constant_hooks=1;
#else
constexpr unsigned death_constant_hooks=0;
#endif
#if defined(GEARS_DLSS_ROUTE_OBSERVER) && GEARS_DLSS_ROUTE_OBSERVER
constexpr unsigned dlss_route_hooks=2;
#else
constexpr unsigned dlss_route_hooks=0;
#endif
#if defined(GEARS_DLSS_INPUT_BIND_OBSERVER) && GEARS_DLSS_INPUT_BIND_OBSERVER
constexpr unsigned dlss_input_bind_hooks=1;
#else
constexpr unsigned dlss_input_bind_hooks=0;
#endif
#if defined(GEARS_NGX_ROUTE_OBSERVER) && GEARS_NGX_ROUTE_OBSERVER
constexpr unsigned ngx_route_hooks=2;
#else
constexpr unsigned ngx_route_hooks=0;
#endif
#if defined(GEARS_DEATH_PSO_CACHE) && GEARS_DEATH_PSO_CACHE
constexpr unsigned death_pso_hooks=3;
#else
constexpr unsigned death_pso_hooks=0;
#endif
#if defined(GEARS_DEATH_DOF_CONTROL) && GEARS_DEATH_DOF_CONTROL
constexpr unsigned death_dof_hooks=1;
#else
constexpr unsigned death_dof_hooks=0;
#endif
#if defined(GEARS_VR_DOF_COMPAT) && GEARS_VR_DOF_COMPAT
constexpr unsigned vr_dof_hooks=1;
#else
constexpr unsigned vr_dof_hooks=0;
#endif
#if defined(GEARS_NGX_STEREO_HOST_OBSERVER) && GEARS_NGX_STEREO_HOST_OBSERVER
constexpr unsigned ngx_stereo_host_hooks=3;
#else
constexpr unsigned ngx_stereo_host_hooks=0;
#endif
std::array<SessionHookEntry,ngx_mv_compute_hooks+ngx_mv_descriptor_hooks+ngx_stereo_host_hooks+session_hook_capacity+dxgi_resize_hooks+game_material_hooks+crosshair_assignment_hooks+ui_call_boundary_hooks+ui_scene_root_hooks+path_layout_hooks+path_close_hooks+death_constant_hooks+dlss_route_hooks+dlss_input_bind_hooks+ngx_route_hooks+death_pso_hooks+death_dof_hooks+vr_dof_hooks> session_hook_entries{};
unsigned session_hook_entry_count{};
gears_submitted::Lifetime submitted_lifetime;
std::atomic<bool> world_quiesce_requested{};
gears_submitted::Lifetime::QuiesceToken world_quiesce_token{}; // session_native_mutex
std::atomic<unsigned> world_handoff_phase{}; // 0 idle, 1 requested, 2 native closed, 3 committed, 4 stale
std::atomic<uint64_t> world_handoff_commits{},epoch_submit_total{};
std::atomic<uint64_t> world_commit_request{};
std::atomic<unsigned> submit_total{0},submit_retained{0},submit_skipped{0};
LONGLONG submit_deadline{},submit_not_before{};
struct RectangleTrace {uintptr_t object{};std::array<float,4> source{};std::array<int,4> target{};
    std::array<float,4> sampling{};std::array<int,2> buffer{};};
std::array<RectangleTrace,10> rectangle_traces{};
std::array<std::atomic<unsigned>,10> rectangle_trace_phases{};
gears_pair_shadow::Result renderer_pair_result{};
thread_local bool unsubmitted_renderer{};
thread_local void* pair_canvas{};
thread_local void* pair_transform{};
std::array<float,32> constructor_eye_matrices{};
unsigned constructor_eye_shifted{},constructor_combined_valid{};
double constructor_combined_relative_error{};
gears_source_args::Constructor source_constructor{};
unsigned constructor_derived_finite{};
double constructor_derived_error{};
using StateFactoryFn=void*(__fastcall*)();
using StateDeleteFn=void*(__fastcall*)(void*,unsigned);
StateFactoryFn state_factory{};
ConsumeFn state_release{};
StateDeleteFn original_state_delete{};
void* state_delete_target{};
uintptr_t state_expected_table{};
bool shadow_state_enabled{};
std::atomic<unsigned> state_attempted{0},state_created{0},state_release_returned{0},state_destroyed{0};
std::atomic<unsigned> state_release_started{};
std::atomic<unsigned> state_delete_entered{0},state_original_preserved{0};
std::atomic<uintptr_t> state_temporary{0};
unsigned state_delete_flags{},state_delete_thread{};
uintptr_t state_delete_caller{};
void* family_target{};
bool reservation_ready{};
std::atomic<unsigned> reservation_started{0};
HANDLE reservation_event{};
using PresentFn=HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*,UINT,UINT);
using Present1Fn=HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain1*,UINT,UINT,const DXGI_PRESENT_PARAMETERS*);
using ExecuteFn=void(STDMETHODCALLTYPE*)(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*);
SceneFn original_scene{}; EventFn original_event{};
PresentFn original_present{}; Present1Fn original_present1{};
ExecuteFn original_execute{};
bool gpu_diagnostics{};
bool viewport_diagnostics{};
bool color_copy_diagnostics{};
thread_local bool trial_draw_scope{};
gears_viewport::Tracker viewport_stats;
// Preserve original diagnostic interface addresses; frame association owns a
// separate descriptor/binding map with canonical IUnknown resource identities.
gears_viewport::Tracker frame_target_stats;
// Owned recordings may cross desktop Present. Retire only submitted members;
// a new recording requires an observed Reset (or initial open creation).
gears_viewport::Tracker owned_target_stats{true};
#if defined(GEARS_NATIVE_OWNED_PIXELS) && GEARS_NATIVE_OWNED_PIXELS
constexpr bool owned_bindings_enabled=true;
#else
constexpr bool owned_bindings_enabled=false;
#endif
thread_local unsigned frame_present_depth{};
thread_local bool frame_inside_original_present{};
struct FramePresentScope {
    bool observe{};
    FramePresentScope(){observe=frame_present_depth++==0;}
    ~FramePresentScope(){--frame_present_depth;}
};
// Candidate stacks only: resource identities must subsequently match Present buffers.
struct OutputStack {gears_viewport::Target target{};unsigned kind{},count{};std::array<void*,24> frames{};};
std::array<OutputStack,8> output_stacks{};
std::mutex output_stack_mutex;
std::atomic<unsigned> output_stack_attempts{0};
struct PostNode {uintptr_t object{},table{},function{};unsigned flags{},valid{};};
struct PostView {uintptr_t view{},array{};int count{},capacity{};unsigned valid{};
    std::array<float,4> source{};std::array<int,4> target{};std::array<PostNode,16> nodes{};};
struct PostSnapshot {uintptr_t renderer{};unsigned view_count{};std::array<PostView,2> views{};};
std::array<PostSnapshot,4> output_post_snapshots{};
using QuadFn=void(__fastcall*)(float,float,float,float,float,float,float,float,unsigned,unsigned,unsigned,unsigned);
QuadFn original_quad{};void* quad_target{};
struct QuadTrace {uintptr_t caller{};std::array<float,8> rectangles{};std::array<unsigned,4> dimensions{};uint64_t calls{};};
std::array<QuadTrace,64> quad_traces{};
std::mutex quad_mutex;uint64_t quad_overflow{};
using ResetListFn=HRESULT(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12CommandAllocator*,ID3D12PipelineState*);
using ViewportsFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,const D3D12_VIEWPORT*);
using ScissorsFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,const D3D12_RECT*);
using DrawFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,UINT,UINT,UINT);
using DrawIndexedFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,UINT,UINT,INT,UINT);
using TargetsFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,const D3D12_CPU_DESCRIPTOR_HANDLE*,BOOL,const D3D12_CPU_DESCRIPTOR_HANDLE*);
using CreateRtvFn=void(STDMETHODCALLTYPE*)(ID3D12Device*,ID3D12Resource*,const D3D12_RENDER_TARGET_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
using CopyDescriptorsFn=void(STDMETHODCALLTYPE*)(ID3D12Device*,UINT,const D3D12_CPU_DESCRIPTOR_HANDLE*,const UINT*,UINT,const D3D12_CPU_DESCRIPTOR_HANDLE*,const UINT*,D3D12_DESCRIPTOR_HEAP_TYPE);
using CopySimpleFn=void(STDMETHODCALLTYPE*)(ID3D12Device*,UINT,D3D12_CPU_DESCRIPTOR_HANDLE,D3D12_CPU_DESCRIPTOR_HANDLE,D3D12_DESCRIPTOR_HEAP_TYPE);
ResetListFn original_reset_list{};ViewportsFn original_viewports{};ScissorsFn original_scissors{};
DrawFn original_draw{};DrawIndexedFn original_draw_indexed{};
TargetsFn original_targets{};CreateRtvFn original_create_rtv{};CopyDescriptorsFn original_copy_descriptors{};CopySimpleFn original_copy_simple{};
std::array<void*,viewport_hook_capacity> viewport_targets{};
#if defined(GEARS_OWNED_OUTPUT_TRACKING) && GEARS_OWNED_OUTPUT_TRACKING
using BarrierFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,const D3D12_RESOURCE_BARRIER*);
BarrierFn original_resource_barrier{};
#endif
#if defined(GEARS_SWAPCHAIN_STARTUP_TRACE) && GEARS_SWAPCHAIN_STARTUP_TRACE
constexpr unsigned prime_hook_base_count=8;
#else
constexpr unsigned prime_hook_base_count=6;
#endif
constexpr unsigned prime_hook_count=prime_hook_base_count+ngx_mv_descriptor_hooks;
std::array<void*,prime_hook_count> prime_targets{};
std::atomic<bool> prime_recording{},prime_ready{},prime_handoff{},prime_done{};
std::atomic<unsigned> prime_started{};
std::mutex prime_mutex;
HANDLE prime_event{};
gears_session::Owner prime_owner;
DWORD prime_owner_pid{};
std::uint64_t prime_owner_creation_time{};
bool prime_owner_allows_handoff(bool continuous,DWORD pid,std::uint64_t created) {
    return !prime_done.load()&&prime_event&&WaitForSingleObject(prime_event,0)==WAIT_TIMEOUT&&
        (!prime_owner.handle||(continuous&&
        gears_prime::same_owner(prime_owner_pid,prime_owner_creation_time,pid,created)&&
        prime_owner.state()==gears_session::OwnerState::alive));
}
std::array<std::array<unsigned char,32>,prime_hook_count> prime_original_bytes{};
using CreateSwapChainFn=HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory*,IUnknown*,DXGI_SWAP_CHAIN_DESC*,IDXGISwapChain**);
using CreateHwndFn=HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory2*,IUnknown*,HWND,const DXGI_SWAP_CHAIN_DESC1*,const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*,IDXGIOutput*,IDXGISwapChain1**);
using CreateCompositionFn=HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory2*,IUnknown*,const DXGI_SWAP_CHAIN_DESC1*,IDXGIOutput*,IDXGISwapChain1**);
CreateSwapChainFn original_create_swapchain{};
CreateHwndFn original_create_hwnd{};
CreateCompositionFn original_create_composition{};
void* execute_target{};
HMODULE own_module{};
uintptr_t base{},expected_local{},expected_pc{},camera_cache{};
uintptr_t expected_camera{},camera_yaw{};
bool wobble_enabled{},self_testing{};
HANDLE frame_test_waiting{}; // CPU self-test handshake, never set in a capture session.
bool sweep_enabled{};
bool live_enabled{};
gears_vr::PoseBridge live_bridge;
gears_vr::PoseGate live_gate;
SRWLOCK live_lock=SRWLOCK_INIT;
thread_local unsigned live_status{};
thread_local unsigned long long live_sequence{};
std::atomic<bool> live_aborted{false};
double selftest_elapsed=4;
uintptr_t allowed_caller=0x15d0ca;
long long qpc_frequency{};
std::atomic<long long> wobble_start{0};
std::atomic<unsigned> wobble_state{0}; // 0 waiting, 1 running, 2 complete, 3 aborted
thread_local void* scene_local{};
thread_local Rot* scene_rotation{};
thread_local int scene_delta{};
thread_local unsigned scene_phase{};
thread_local Vec scene_translation{};
thread_local Rot scene_rotation_delta{};
thread_local float observed_stamp{};
thread_local bool stamp_known{};
std::atomic<unsigned> active{0},started{0};
std::atomic<unsigned long long> dropped{0};
std::atomic<bool> recording{false};
SRWLOCK queue_lock=SRWLOCK_INIT;
struct Record {
    unsigned kind,thread,focused,valid;
    long long time,duration;
    uintptr_t object,result,caller;
    Vec position; Rot rotation; float camera_time;
    int source_yaw,yaw_delta,cache_yaw;
    unsigned test_state;
    unsigned pose_phase;
    unsigned pose_status;
    unsigned long long pose_sequence;
    uintptr_t gpu_device,gpu_buffer,gpu_window;
    unsigned queue_type,command_count,buffer_index,buffer_count,width,height,format,swap_flags;
    Vec source_position,translation_delta;
    Rot source_rotation,rotation_delta;
    std::array<float,32> matrices;
    uintptr_t family_address{},family_array{};
    int family_count{},family_capacity{};
    std::array<uintptr_t,4> family_views{};
    uintptr_t clone_source{},clone_owner{};
    std::array<float,32> source_matrices{};
    uintptr_t renderer_input_family{},renderer_storage{};
    int renderer_count{},renderer_capacity{};
    std::array<uintptr_t,4> renderer_sources{};
    std::uint64_t stereo_ticket{},stereo_head_flags{},stereo_view_flags{},stereo_reference_generation{};
    unsigned stereo_source{};
};
std::array<Record,8192> ring{};
size_t read_index{},write_index{};
unsigned long long emitted[13]{};
void* scene_target{}; void* event_target{}; void* present_target{}; void* present1_target{};
HANDLE stop_event{};
wchar_t directory[MAX_PATH]{};
FILE* diagnostic_status{};
// The hook and game share an address space. A guarded local copy avoids the
// kernel transition on the thousands of small reads in each submitted frame.
// Keep SEH in a separate function so profiling scopes still unwind normally.
__declspec(noinline) bool local_copy_guarded(void* out,const void* from,size_t bytes) {
    if(!from||!out)return false;
    __try {std::memcpy(out,from,bytes);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
bool safe_copy(void* out,const void* from,size_t bytes) {
    gears_render_cost::Scope cost(gears_render_cost::safe_read);
    return local_copy_guarded(out,from,bytes);
}
#include "native_surface_probe.inl"
#include "native_owned_probe.inl"
uintptr_t canonical_identity(IUnknown* object);
long long now();
void owned_ui_published(uint64_t ticket);
bool owned_ui_ready(uint64_t ticket);
bool owned_ui_can_bind_frame();
void native_owned_pipeline_failure(unsigned reason);
bool verified_stereo_draw(gears_viewport::DrawState const&,FinalEye const&,uintptr_t,unsigned,unsigned,unsigned);
#include "owned_scene_gpu.inl"
#include "native_owned_scene.inl"
// Created before the ordinary Scope/locks in each RHI entry. Its destructor
// also runs after a world-gate passthrough return, keeping terminal retirement
// service alive after submitted native state has closed. Its independent count
// covers retirement after ordinary Scope releases world admission, including
// the publisher-bypass path which never constructs an ordinary Scope.
struct NativeOwnedShutdownRhiScope {
    struct Activity {
        Activity(){++active;}
        ~Activity(){--active;}
    } activity;
    NativeOwnedShutdownRhiScope()=default;
    NativeOwnedShutdownRhiScope(NativeOwnedShutdownRhiScope const&)=delete;
    NativeOwnedShutdownRhiScope& operator=(NativeOwnedShutdownRhiScope const&)=delete;
    ~NativeOwnedShutdownRhiScope(){native_owned_scene_shutdown_rhi();}
};
#include "desktop_mirror_readback.inl"
#include "ngx_root_signature_capture.inl"
void trace_rectangle(unsigned index,uintptr_t view) {
    if(!submit_pair_enabled || !view)return;
    unsigned expected=0;if(!rectangle_trace_phases[index].compare_exchange_strong(expected,1))return;
    auto& trace=rectangle_traces[index];trace.object=view;
    bool read=safe_copy(trace.source.data(),reinterpret_cast<void*>(view+0x48),16) &&
        safe_copy(trace.target.data(),reinterpret_cast<void*>(view+0x58),16) &&
        safe_copy(trace.sampling.data(),reinterpret_cast<void*>(view+0x3d0),16) &&
        safe_copy(trace.buffer.data(),reinterpret_cast<void*>(base+0x1c3b3b8),8);
    rectangle_trace_phases[index]=read?2:3;
}
long long now() {LARGE_INTEGER value;QueryPerformanceCounter(&value);return value.QuadPart;}
// Bounded, monotonic callback progress: callbacks never take a diagnostics lock.
// Commit baselines provide world counts; QPC endpoints remain session cumulative.
// A rejected scope can race commit, so baseline attribution is observational.
enum class WorldProgress : unsigned {family,renderer,family_scope_reject,renderer_scope_reject,
    candidate,try_pair,quiesce,closed,not_before,foreground,count};
struct WorldProgressSample {std::atomic<uint64_t> count{};std::atomic<long long> first{},last{};};
std::mutex world_progress_snapshot_mutex; // Commit/status only, never callbacks.
std::array<WorldProgressSample,static_cast<unsigned>(WorldProgress::count)> world_progress{};
std::array<uint64_t,static_cast<unsigned>(WorldProgress::count)> world_progress_baseline{};
uint64_t world_progress_generation{1};
long long world_progress_commit_qpc{}; // Zero denotes the initial world.
void note_world_progress(WorldProgress stage){
    if(!continuous_session)return;
    const auto qpc=now();auto& sample=world_progress[static_cast<unsigned>(stage)];
    sample.count.fetch_add(1,std::memory_order_relaxed);
    auto first=sample.first.load(std::memory_order_relaxed);
    while((!first||qpc<first)&&!sample.first.compare_exchange_weak(first,qpc,std::memory_order_relaxed)){}
    auto last=sample.last.load(std::memory_order_relaxed);
    while(qpc>last&&!sample.last.compare_exchange_weak(last,qpc,std::memory_order_relaxed)){}
}
void reset_world_progress(uint64_t generation){
    std::lock_guard<std::mutex> lock(world_progress_snapshot_mutex);
    for(unsigned i=0;i<world_progress.size();++i)world_progress_baseline[i]=world_progress[i].count.load(std::memory_order_relaxed);
    world_progress_generation=generation;world_progress_commit_qpc=now();
}
void print_world_progress(FILE* output){
    std::lock_guard<std::mutex> lock(world_progress_snapshot_mutex);
    fprintf(output,"wg_schema=1 wg_generation=%llu wg_commit_qpc=%lld wg_tuple=total,baseline,session_first_qpc,session_last_qpc wg_baseline_racy_scope=1 ",
        static_cast<unsigned long long>(world_progress_generation),world_progress_commit_qpc);
    const char* names[]={"family","renderer","family_scope_reject","renderer_scope_reject",
        "candidate","try_pair","quiesce","closed","not_before","foreground"};
    for(unsigned i=0;i<world_progress.size();++i){const auto& s=world_progress[i];
        fprintf(output,"wg_%s=%llu,%llu,%lld,%lld ",names[i],static_cast<unsigned long long>(s.count.load(std::memory_order_relaxed)),
            static_cast<unsigned long long>(world_progress_baseline[i]),s.first.load(std::memory_order_relaxed),s.last.load(std::memory_order_relaxed));}
}
#if defined(GEARS_PATH_CHOICE_POSITION_TRIAL) && GEARS_PATH_CHOICE_POSITION_TRIAL
namespace path_choice_position_live {void request_drain();bool draining();}
std::atomic<unsigned> path_position_stop_record{}; // 0 empty, 1 writer, 2 published.
SessionStop path_position_stop_reason=SessionStop::none;
const char* path_position_stop_file{};
unsigned path_position_stop_line{};
#endif
void request_session_stop_at(SessionStop reason,const char* source_file,unsigned source_line) {
    if(!continuous_session)return;
#if defined(GEARS_PATH_CHOICE_POSITION_TRIAL) && GEARS_PATH_CHOICE_POSITION_TRIAL
    if(session_phase.load()!=SessionPhase::running)return;
    unsigned empty=0;
    if(path_position_stop_record.compare_exchange_strong(empty,1)){
        path_position_stop_reason=reason;path_position_stop_file=source_file;path_position_stop_line=source_line;
        path_position_stop_record.store(2,std::memory_order_release);
    }
    if(path_position_stop_record.load(std::memory_order_acquire)!=2)return;
    // Keep normal scene-hook admission open while its owner restores the lease.
    // The trial bounds this drain and explicitly records unresolved ownership.
    path_choice_position_live::request_drain();
    if(path_choice_position_live::draining())return;
    reason=path_position_stop_reason;source_file=path_position_stop_file;source_line=path_position_stop_line;
#endif
    session_activation_gate.close();
    auto running=SessionPhase::running;
    if(!session_phase.compare_exchange_strong(running,SessionPhase::stop_requested))return;
    session_reason=reason;session_stop_tick=GetTickCount64();
    session_stop_qpc=now();session_stop_thread=GetCurrentThreadId();
    session_stop_source_file=source_file;session_stop_source_line=source_line;
    // Publish the complete first-request record. __FILE__ has static lifetime;
    // later requests lose the phase CAS and cannot overwrite these fields.
    session_stop_recorded.store(true,std::memory_order_release);
    // The Lifetime mutex serializes this barrier with begin. It grants no claim.
    submitted_lifetime.request_stop();
}
// Covers every later call and .inl include, including graphics_lock_observer.
#define request_session_stop(reason) request_session_stop_at((reason),__FILE__,__LINE__)
void native_owned_pipeline_failure(unsigned reason){
#if defined(GEARS_NATIVE_OWNED_SCENE_PROBE) && GEARS_NATIVE_OWNED_SCENE_PROBE
    // Called with frame_hook_mutex. Retain native objects before closing
    // admission; do not acquire session_native_mutex in this lock order.
    gears_owned_scene_live::pipeline_failed=true;
    gears_owned_scene_live::pipeline_failure_reason=reason;
    gears_owned_scene_live::shutdown_conflict=true;submit_retained=1;
    gears_owned_scene_live::trial.error=25;
    owned_scene_gpu::fault(24);frame_link.invalidate_all();
    {StereoPublishBypass bypass;stereo_publisher.mark_stopped();stereo_publish_counts.stopped=1;}
    session_activation_gate.close();request_session_stop(SessionStop::proof_failure);
#else
    (void)reason;
#endif
}
bool publish_session_activation_ready(bool prerequisites){
    if(!prerequisites||session_phase.load()!=SessionPhase::running||submitted_lifetime.stopping())return false;
    const auto ready_qpc=now();
    if(!session_activation_gate.publish())return false;
    session_activation_ready_qpc=ready_qpc;
    return true;
}
void observe_session_owner(gears_session::OwnerState state) {
    if(state==gears_session::OwnerState::exited){session_owner_exit_observed=true;request_session_stop(SessionStop::owner_exited);}
    else if(state==gears_session::OwnerState::error){session_owner_error_observed=true;request_session_stop(SessionStop::owner_error);}
}
void release_session_state_claim(bool claim) {
    if(claim&&state_created.load()==1&&!submit_retained.load()) {
        unsigned fresh=0;if(!state_release_started.compare_exchange_strong(fresh,1))return;
        state_release(reinterpret_cast<void*>(state_temporary.load()));state_release_returned=1;
    }
}
void service_world_quiesce_native(){
    if(!continuous_session||session_build_depth||!world_quiesce_requested.load()||submitted_lifetime.stopping())return;
    std::lock_guard<std::recursive_mutex> lock(session_native_mutex);
    if(submitted_lifetime.stopping()||submit_retained.load()||(state_created.load()&&!submit_sources_released.load()))return;
    auto result=submitted_lifetime.quiesce_close(committed_world_generation.load());
    if(!result.accepted)return;
    world_quiesce_token=result.token;
    if(world_handoff_phase.load()==1)world_handoff_phase=2;
    release_session_state_claim(result.release_claim);
}
void service_session_stop_native() {
    service_world_quiesce_native();
    if(!continuous_session||session_build_depth||!submitted_lifetime.stopping())return;
    std::lock_guard<std::recursive_mutex> lock(session_native_mutex);
    ++session_native_services;
    release_session_state_claim(submitted_lifetime.close());
    auto requested=SessionPhase::stop_requested;
    session_phase.compare_exchange_strong(requested,SessionPhase::native_closed);
}
struct SessionBuildScope {
    bool enabled{continuous_session};
    SessionBuildScope(){if(enabled)++session_build_depth;}
    ~SessionBuildScope(){if(enabled){--session_build_depth;service_session_stop_native();}}
};
bool session_native_cleanup_ready() {
    return submitted_lifetime.is_closed()&&!submitted_lifetime.pending()&&!submit_retained.load()&&
        (!state_created.load()||(state_release_returned.load()==1&&state_destroyed.load()==1&&(state_delete_flags&1)));
}
bool session_cleanup_expired() {
    auto tick=session_stop_tick.load();return tick&&GetTickCount64()-tick>=30000;
}
std::mutex projection_mutex;
long long projection_origin{};
unsigned projection_rejected{};
struct ProjectionAggregate {unsigned long long count{},mismatch{};long long first{},last{},pair_first{},pair_last{};
    std::array<std::array<float,16>,2> expected{},effective{};
    std::array<float,2> residual_min{},residual_max{},residual9_min{},residual9_max{};};
std::array<std::array<ProjectionAggregate,4>,5> projection_aggregates{};
void projection_observe(unsigned stage,uintptr_t renderer,const std::array<uintptr_t,4>& views,int count,long long timestamp) {
    if(!projection_control)return;
    gears_projection::Metadata metadata{};
    if(!submitted_lifetime.projection(renderer,metadata))return;
    std::array<std::array<float,16>,2> effective{};
    bool matching=count==2;
    for(unsigned i=0;i<2;++i){
        matching=safe_copy(effective[i].data(),reinterpret_cast<void*>(views[i]+0xb0),64) && matching;
        matching=gears_projection::matches(effective[i],metadata.expected[i],stage==3) && matching;
    }
    std::lock_guard<std::mutex> lock(projection_mutex);
    auto& a=projection_aggregates[static_cast<unsigned>(metadata.phase)][stage];
    for(unsigned i=0;i<2;++i){float r=effective[i][8]-metadata.expected[i][8],r9=effective[i][9]-metadata.expected[i][9];
        if(!a.count){a.residual_min[i]=a.residual_max[i]=r;a.residual9_min[i]=a.residual9_max[i]=r9;}
        else {a.residual_min[i]=std::min(a.residual_min[i],r);a.residual_max[i]=std::max(a.residual_max[i],r);
            a.residual9_min[i]=std::min(a.residual9_min[i],r9);a.residual9_max[i]=std::max(a.residual9_max[i],r9);}}
    if(!a.count){a.first=timestamp;a.pair_first=metadata.pair_qpc;}
    a.first=std::min(a.first,timestamp);a.pair_first=std::min(a.pair_first,metadata.pair_qpc);
    a.last=std::max(a.last,timestamp);a.pair_last=std::max(a.pair_last,metadata.pair_qpc);
    ++a.count;if(!matching)++a.mismatch;
    a.expected=metadata.expected;a.effective=effective;
}
void print_projection(FILE* file) {
    if(!projection_control)return;
    std::lock_guard<std::mutex> lock(projection_mutex);
    fprintf(file,"projection_control enabled=1 origin_qpc=%lld frequency=%lld rejected=%u\n",projection_origin,qpc_frequency,projection_rejected);
    const char* phases[]={"A1","B","A2","C","A3"};const char* stages[]={"built","consume_before","prepare_before","prepare_after"};
    for(unsigned p=0;p<5;++p)for(unsigned s=0;s<4;++s){const auto& a=projection_aggregates[p][s];
        fprintf(file,"projection_phase phase=%s stage=%s count=%llu mismatch=%llu first_qpc=%lld last_qpc=%lld pair_first_qpc=%lld pair_last_qpc=%lld",
            phases[p],stages[s],a.count,a.mismatch,a.first,a.last,a.pair_first,a.pair_last);
        for(unsigned i=0;i<2;++i)fprintf(file," expected_%s_p00=%.9g expected_%s_p8=%.9g expected_%s_p9=%.9g effective_%s_p00=%.9g effective_%s_p8=%.9g effective_%s_p9=%.9g residual_%s_p8_min=%.9g residual_%s_p8_max=%.9g residual_%s_p9_min=%.9g residual_%s_p9_max=%.9g",
            i?"r":"l",a.expected[i][0],i?"r":"l",a.expected[i][8],i?"r":"l",a.expected[i][9],
            i?"r":"l",a.effective[i][0],i?"r":"l",a.effective[i][8],i?"r":"l",a.effective[i][9],
            i?"r":"l",a.residual_min[i],i?"r":"l",a.residual_max[i],i?"r":"l",a.residual9_min[i],i?"r":"l",a.residual9_max[i]);
        fprintf(file,"\n");}
}
struct StereoAggregate {unsigned long long count{},mismatch{},sequence_first{},sequence_last{},ticket_first{},ticket_last{};
    long long first{},last{};double view_error_max{};
    std::array<float,2> residual8_min{},residual8_max{},residual9_min{},residual9_max{};};
std::array<StereoAggregate,4> stereo_aggregates{};
#if defined(GEARS_SIXDOF_TICKET_DIAGNOSTICS) && GEARS_SIXDOF_TICKET_DIAGNOSTICS
gears_sixdof::NativeRecorder sixdof_native;
gears_sixdof::PinMailbox sixdof_pins;
bool sixdof_pin_init_attempted{};
#endif
struct OriginStage {long long qpc{};unsigned reads{};bool matching{};gears_vr::StereoMatrices actual{};};
struct OriginRow {gears_submitted::FrameTicket frame{};std::array<OriginStage,4> stages{};unsigned mask{};};
std::array<std::array<OriginRow,12>,3> origin_rows{};
std::array<unsigned,3> origin_row_counts{};
std::array<long long,3> origin_next_sample{};
std::array<std::array<unsigned long long,4>,3> origin_stage_counts{},origin_stage_mismatches{};
unsigned long long origin_duplicate_stages{};
void stereo_observe(unsigned stage,uintptr_t renderer,const std::array<uintptr_t,4>& views,int count,long long timestamp) {
    if(!stereo_xr)return;
    gears_submitted::FrameTicket ticket{};
    if(!submitted_lifetime.frame(renderer,ticket))return;
    bool matching=count==2;double error_max{};std::array<float,2> r8{},r9{};OriginStage observation{};
    observation.qpc=timestamp;
    for(unsigned i=0;i<2;++i){auto& view=observation.actual.view[i];auto& projection=observation.actual.projection[i];
        bool view_read=safe_copy(view.data(),reinterpret_cast<void*>(views[i]+0x70),64);
        bool projection_read=safe_copy(projection.data(),reinterpret_cast<void*>(views[i]+0xb0),64);
        if(view_read)observation.reads|=1u<<i;if(projection_read)observation.reads|=4u<<i;
        matching=view_read&&projection_read&&matching;
        for(unsigned j=0;j<16;++j){matching=matching && std::isfinite(view[j]) && view[j]==ticket.expected.view[i][j];
            if(std::isfinite(view[j]))error_max=std::max(error_max,std::abs(double(view[j])-ticket.expected.view[i][j]));
            else error_max=INFINITY;}
        matching=gears_projection::matches(projection,ticket.expected.projection[i],stage==3) && matching;
        r8[i]=projection[8]-ticket.expected.projection[i][8];r9[i]=projection[9]-ticket.expected.projection[i][9];
    }
    std::lock_guard<std::mutex> lock(stereo_mutex);auto& a=stereo_aggregates[stage];
    if(!a.count){a.first=a.last=timestamp;a.sequence_first=a.sequence_last=ticket.sample.sequence;
        a.ticket_first=a.ticket_last=ticket.id;
        a.residual8_min=a.residual8_max=r8;a.residual9_min=a.residual9_max=r9;}
    a.first=std::min(a.first,timestamp);a.last=std::max(a.last,timestamp);
    a.sequence_first=std::min(a.sequence_first,ticket.sample.sequence);a.sequence_last=std::max(a.sequence_last,ticket.sample.sequence);
    a.ticket_first=std::min(a.ticket_first,ticket.id);a.ticket_last=std::max(a.ticket_last,ticket.id);
    a.view_error_max=std::max(a.view_error_max,error_max);
    for(unsigned i=0;i<2;++i){a.residual8_min[i]=std::min(a.residual8_min[i],r8[i]);a.residual8_max[i]=std::max(a.residual8_max[i],r8[i]);
        a.residual9_min[i]=std::min(a.residual9_min[i],r9[i]);a.residual9_max[i]=std::max(a.residual9_max[i],r9[i]);}
    ++a.count;if(!matching)++a.mismatch;
#if defined(GEARS_SIXDOF_TICKET_DIAGNOSTICS) && GEARS_SIXDOF_TICKET_DIAGNOSTICS
    // Passive copy of the independently read engine-stage matrices above.
    // Recent storage rolls for the whole session; snapshot requests pin exact
    // session/world/ticket rows, including captures long after startup.
    if(!sixdof_pin_init_attempted){sixdof_pin_init_attempted=true;sixdof_pins.create();}
    gears_sixdof::NativeRow diagnostic{};
    diagnostic.key={GetCurrentProcessId(),sixdof_pins.start,ticket.sample.session,ticket.world_generation,ticket.id};
    diagnostic.reference_generation=ticket.reference_generation;diagnostic.space_epoch=ticket.sample.space_epoch;
    diagnostic.sequence=ticket.sample.sequence;diagnostic.units_per_metre=stereo_units_per_metre;
    diagnostic.reference=ticket.reference;diagnostic.head=ticket.sample.head;
    diagnostic.source_view=ticket.source_view;
    for(unsigned eye=0;eye<2;++eye){diagnostic.eyes[eye]=ticket.sample.eyes[eye].pose;diagnostic.expected[eye]=ticket.expected.view[eye];}
    gears_sixdof::Stage actual{};actual.qpc=timestamp;actual.count=count;
    actual.reads=observation.reads&3;
    for(unsigned eye=0;eye<2;++eye)actual.actual[eye]=observation.actual.view[eye];
    sixdof_native.observe(diagnostic,stage,actual);sixdof_pins.service(sixdof_native);
#endif
    if(origin_trial_enabled&&ticket.origin.enabled){
        auto phase=static_cast<unsigned>(ticket.origin.selection.phase);
        if(phase<3&&stage<4){
            ++origin_stage_counts[phase][stage];if(!matching)++origin_stage_mismatches[phase][stage];
            OriginRow* row=nullptr;
            for(unsigned i=0;i<origin_row_counts[phase];++i)if(origin_rows[phase][i].frame.id==ticket.id)row=&origin_rows[phase][i];
            if(!row&&stage==0&&origin_row_counts[phase]<origin_rows[phase].size()&&timestamp>=origin_next_sample[phase]){
                row=&origin_rows[phase][origin_row_counts[phase]++];row->frame=ticket;origin_next_sample[phase]=timestamp+qpc_frequency;}
            if(row){if(row->mask&(1u<<stage))++origin_duplicate_stages;
                else {observation.matching=matching;row->stages[stage]=observation;row->mask|=1u<<stage;}}
        }
    }
    if(stage==0&&outage_diagnostics.load())outage_built.write({ticket.id,a.count,ticket.sample.sequence,timestamp,ticket.sample.received_ns});
}
void print_stereo_pose(FILE* file,const gears_vr::Pose& pose) {
    fprintf(file," position=%.17g,%.17g,%.17g orientation=%.17g,%.17g,%.17g,%.17g",pose.position.x,pose.position.y,pose.position.z,
        pose.orientation.x,pose.orientation.y,pose.orientation.z,pose.orientation.w);
}
unsigned foreground();
bool create_stereo_recenter_event() {
    if(!stereo_xr)return true;
    wchar_t name[128];swprintf_s(name,L"Local\\GearsVR.Stereo.Recenter.%lu",GetCurrentProcessId());
    HANDLE event=CreateEventW(nullptr,FALSE,FALSE,name);
    if(!event)return false;
    if(GetLastError()==ERROR_ALREADY_EXISTS){CloseHandle(event);return false;}
    stereo_recenter_event=event;return true;
}
void close_stereo_recenter_event() {
    // A failed hook installation may still have a callback in flight. Retain
    // the handle until process exit in that failure case, never race a wait.
    if(stereo_recenter_event&&!active.load()){CloseHandle(stereo_recenter_event);stereo_recenter_event=nullptr;}
}
void poll_stereo_recenter() {
    // Called under stereo_mutex, once before freezing a pair. No worker or
    // receiver can alter the reference of an already constructed ticket.
    if(!stereo_recenter_event)return;
    const bool key_down=foreground()&&(GetAsyncKeyState(VK_F9)&0x8000)!=0;
    const bool key_edge=key_down&&!stereo_recenter_key_down;stereo_recenter_key_down=key_down;
    const DWORD event=WaitForSingleObject(stereo_recenter_event,0);
    if(key_edge||event==WAIT_OBJECT_0){++stereo_recenter_requests;stereo_gate.request_recenter();}
    if(event!=WAIT_TIMEOUT&&event!=WAIT_OBJECT_0&&continuous_session)request_session_stop(SessionStop::proof_failure);
}
void record_stereo_reference(const gears_submitted::FrameTicket& frame,long long timestamp) {
    if(frame.reference_generation==stereo_reference_generation.load())return;
    StereoReferenceEvent event{frame.reference_generation,frame.id,frame.sample.session,frame.sample.sequence,
        timestamp,frame.sample.received_ns,frame.reference};
    if(stereo_reference_event_count<stereo_reference_events.size())stereo_reference_events[stereo_reference_event_count]=event;
    ++stereo_reference_event_count;stereo_latest_reference=event;stereo_reference_generation=event.generation;
}
void print_stereo(FILE* file) {
    if(!stereo_xr)return;
    std::lock_guard<std::mutex> lock(stereo_mutex);
#if defined(GEARS_SIXDOF_TICKET_DIAGNOSTICS) && GEARS_SIXDOF_TICKET_DIAGNOSTICS
    // Final requests remain serviceable even if rendering stopped immediately
    // after the last GPU snapshot. Output is bounded to eight pinned tickets.
    sixdof_pins.service(sixdof_native);sixdof_pins.dump(file);sixdof_native.dump(file);
#endif
    fprintf(file,"camera_closer experimental=1 metres=%.9g applied=%llu inactive=%llu rejected=%llu clearance_metres=0.4 direction=original_camera collision_certified=0 gameplay_camera_modified=0\n",
        camera_closer_metres,camera_closer_applied.load(),camera_closer_inactive.load(),camera_closer_rejected.load());
    fprintf(file,"stereo_xr enabled=1 units_per_metre=%.9g accepted=%llu first_qpc=%lld last_qpc=%lld first_sequence=%llu last_sequence=%llu first_ticket=%llu last_ticket=%llu gpu_published=%u simulator_operator=%d mode=%s\n",
        stereo_units_per_metre,stereo_accepted,stereo_first_qpc,stereo_last_qpc,stereo_first_sample.sequence,stereo_last_sample.sequence,stereo_first_ticket,stereo_last_ticket,stereo_publisher.frames.load(),simulator_operator,simulator_operator?"simulated":"real");
    const char* reasons[]={"accepted","initial","untracked","inactive","stale","replayed","session_changed","invalid","bridge_read","matrix","heading_unavailable"};
    for(unsigned i=0;i<stereo_rejections.size();++i)fprintf(file,"stereo_rejection reason=%s count=%llu\n",reasons[i],stereo_rejections[i]);
    fprintf(file,"stereo_recenter policy=local_y_up_heading requests=%llu committed_generation=%llu heading_deferred=%llu reference_events=%llu event_limit=%zu\n",
        stereo_recenter_requests.load(),stereo_reference_generation.load(),stereo_heading_deferred.load(),stereo_reference_event_count,stereo_reference_events.size());
    auto print_reference=[&](const char* edge,const StereoReferenceEvent& event){
        fprintf(file,"stereo_reference_event edge=%s generation=%llu ticket=%llu session=%llu sequence=%llu qpc=%lld received_ns=%lld",
            edge,event.generation,event.ticket,event.session,event.sequence,event.qpc,event.received_ns);
        print_stereo_pose(file,event.reference);fprintf(file,"\n");};
    for(size_t i=0;i<(std::min)(std::uint64_t(stereo_reference_events.size()),stereo_reference_event_count);++i)
        print_reference("retained",stereo_reference_events[i]);
    if(stereo_reference_event_count)print_reference("latest",stereo_latest_reference);
    if(stereo_accepted){fprintf(file,"stereo_reference");print_stereo_pose(file,stereo_reference);fprintf(file,"\n");
        const gears_vr::StereoSample* samples[]={&stereo_first_sample,&stereo_last_sample};
        for(unsigned k=0;k<2;++k){const auto& sample=*samples[k];
            fprintf(file,"stereo_sample edge=%s session=%llu sequence=%llu received_ns=%lld predicted_time=%lld head_flags=%llu view_flags=%llu focused=%u should_render=%u source=%u ipd_metres=%.9g",
                k?"last":"first",sample.session,sample.sequence,sample.received_ns,sample.predicted_time,sample.head_flags,sample.view_flags,sample.focused,sample.should_render,
                static_cast<unsigned>(sample.source),gears_vr::stereo_distance(sample.eyes[0].pose.position,sample.eyes[1].pose.position));print_stereo_pose(file,sample.head);fprintf(file,"\n");
            for(unsigned i=0;i<2;++i){auto f=sample.eyes[i].fov;fprintf(file,"stereo_eye edge=%s eye=%u",k?"last":"first",i);
                print_stereo_pose(file,sample.eyes[i].pose);fprintf(file," fov=%.9g,%.9g,%.9g,%.9g\n",f.left,f.right,f.up,f.down);}}
    }
    const char* stages[]={"built","consume_before","prepare_before","prepare_after"};
    for(unsigned s=0;s<4;++s){const auto& a=stereo_aggregates[s];
        fprintf(file,"stereo_stage stage=%s count=%llu mismatch=%llu first_qpc=%lld last_qpc=%lld sequence_first=%llu sequence_last=%llu ticket_first=%llu ticket_last=%llu view_error_max=%.9g",
            stages[s],a.count,a.mismatch,a.first,a.last,a.sequence_first,a.sequence_last,a.ticket_first,a.ticket_last,a.view_error_max);
        for(unsigned i=0;i<2;++i)fprintf(file," residual_%s_p8_min=%.9g residual_%s_p8_max=%.9g residual_%s_p9_min=%.9g residual_%s_p9_max=%.9g",
            i?"r":"l",a.residual8_min[i],i?"r":"l",a.residual8_max[i],i?"r":"l",a.residual9_min[i],i?"r":"l",a.residual9_max[i]);fprintf(file,"\n");}
}
unsigned foreground() {
    DWORD pid{};GetWindowThreadProcessId(GetForegroundWindow(),&pid);return pid==GetCurrentProcessId();
}
void reset_world_thread_context();
// World-independent factory/descriptor/resize observations keep their existing
// locks. Graphics policy also stays active while closed, preserving DLSS locks;
// a concurrent permanent policy fault wins via Lifetime's stop/reopen barrier.
struct Scope {
    gears_world::RecursiveAdmission admission{world_thread,world_gate};
    Scope(){++active;if(admission&&world_thread.depth==1){const auto generation=committed_world_generation.load();
        if(thread_world_generation&&thread_world_generation!=generation)reset_world_thread_context();
        thread_world_generation=generation;}}
    ~Scope(){--active;}
    bool world()const{return bool(admission);}
};
#include "graphics_lock_observer.inl"
bool read_attested_hook_entry(void* output,void const* source,size_t size);
#include "ui_observer_hooks.inl"
#include "path_choice_close_observer.inl"
uintptr_t canonical_identity(IUnknown* object) {
    IUnknown* identity{};
    if(!object || FAILED(object->QueryInterface(IID_PPV_ARGS(&identity))))return 0;
    auto value=reinterpret_cast<uintptr_t>(identity);identity->Release();return value;
}
bool hold_binding(IDXGISwapChain* chain,ID3D12CommandQueue* queue) {
    IUnknown* chain_identity{};IUnknown* queue_identity{};
    if(FAILED(chain->QueryInterface(IID_PPV_ARGS(&chain_identity)))||FAILED(queue->QueryInterface(IID_PPV_ARGS(&queue_identity)))) {
        if(chain_identity)chain_identity->Release();if(queue_identity)queue_identity->Release();return false;
    }
    std::lock_guard<std::mutex> lock(binding_owner_mutex);
    for(auto& owner:binding_owners)if(owner.chain==chain_identity){bool same=owner.queue==queue_identity;
        chain_identity->Release();queue_identity->Release();return same;}
    for(auto& owner:binding_owners)if(!owner.chain){owner={chain_identity,queue_identity};return true;}
    chain_identity->Release();queue_identity->Release();return false;
}
void bind_stereo_publisher(uintptr_t chain_id,uintptr_t queue_id) {
    if(owned_xr_probe||!stereo_gpu_publish||stereo_publisher.state.load()!=0||stereo_publish_counts.stopped.load())return;
    std::lock_guard<std::mutex> lock(binding_owner_mutex);
    for(auto& owner:binding_owners)if(reinterpret_cast<uintptr_t>(owner.chain)==chain_id){
        gears_vr::ComPtr<IDXGISwapChain> chain;gears_vr::ComPtr<ID3D12CommandQueue> queue;
        if(!owner.queue||reinterpret_cast<uintptr_t>(owner.queue)!=queue_id||FAILED(owner.chain->QueryInterface(IID_PPV_ARGS(&chain)))||
           FAILED(owner.queue->QueryInterface(IID_PPV_ARGS(&queue)))){frame_link.invalidate();return;}
        StereoPublishBypass bypass;
        if(stereo_publisher.bind(queue.Get(),chain.Get(),simulator_operator))++stereo_publish_counts.bind_requests;
        return;
    }
    frame_link.invalidate();
}
void retire_stereo_window() {
    vr_dof_compat::retire();
    dlss_output_control::retire();
    if(!stereo_gpu_publish)return;
    if(!stereo_ticket_store.retire_through(stereo_window_retained)){++stereo_publish_counts.ticket_rejected;frame_link.invalidate();}
    stereo_window_retained=0;
}
bool retain_stereo_draw() {
    auto const& frame=consuming_frozen_frame;
    if(!frame.enabled||!frame.id||frame.id!=consuming_frame_ticket){++stereo_publish_counts.ticket_missing;frame_link.invalidate();return false;}
    gears_vr::StereoTicketGeometry geometry{frame.source_view,frame.source_projection,frame.expected,frame.finite_aim,frame.aiming};
    auto retained=stereo_ticket_store.retain(frame.id,frame.sample,simulator_operator,ui_observer_enabled?&geometry:nullptr);
    if(!gears_vr::StereoTicketStore::accepted(retained)){++stereo_publish_counts.ticket_rejected;frame_link.invalidate();return false;}
    if(retained==gears_vr::TicketRetainResult::retained)++stereo_publish_counts.ticket_retains;else ++stereo_publish_counts.ticket_duplicates;
    stereo_window_retained=std::max(stereo_window_retained,frame.id);return true;
}
bool verified_stereo_draw(gears_viewport::DrawState const& state,FinalEye const& eye,uintptr_t caller,unsigned kind,unsigned elements,unsigned instances) {
    auto const& v=state.viewport;auto const& s=state.scissor;
    return eye.valid&&caller==base+0x1af92b&&kind==1&&elements==6&&instances==1&&state.tracked&&state.viewport_count==1&&state.scissor_count==1&&
        v.x==0&&v.y==0&&v.width==float(eye.width)&&v.height==float(eye.height)&&
        std::isfinite(s.x)&&std::isfinite(s.y)&&std::isfinite(s.width)&&std::isfinite(s.height)&&s.width>0&&s.height>0&&
        s.x<=float(eye.rect.x)&&s.y<=float(eye.rect.y)&&double(s.x)+s.width>=double(eye.rect.x)+eye.rect.width&&
        double(s.y)+s.height>=double(eye.rect.y)+eye.rect.height;
}
void release_bindings() {
    // Call only after all hooks are disabled and callbacks have drained. Holding
    // chain/queue identities prevents reuse; buffers are NEVER held long term.
    ngx_mv_descriptor_capture::release_drained();
    ngx_mv_target_capture::release_drained();
    std::lock_guard<std::mutex> lock(binding_owner_mutex);
    for(auto& owner:binding_owners){if(owner.chain)owner.chain->Release();if(owner.queue)owner.queue->Release();owner={};}
}
void write_swapchain_output_status(bool sample,const char* phase) {
    // Diagnostic only: query retained chain identities, never retain backbuffers
    // or invoke window operations. One writer spans prime -> native handoff.
    static std::mutex output_mutex;
    std::lock_guard<std::mutex> writer(output_mutex);
    std::array<gears_vr::ComPtr<IUnknown>,8> owners;
    if(sample){
        std::lock_guard<std::mutex> lock(binding_owner_mutex);
        for(size_t i=0;i<binding_owners.size();++i)if(binding_owners[i].chain){
            binding_owners[i].chain->AddRef();owners[i].Attach(binding_owners[i].chain);
        }
    }
    uintptr_t identity{},window{};unsigned width{},height{},candidates{};bool windowed{};
    // Timestamp the beginning of the query bracket, so a slow COM query never
    // makes an old observation appear fresh to the external recovery owner.
    LARGE_INTEGER qpc{},frequency{};const bool clock_valid=QueryPerformanceCounter(&qpc)&&QueryPerformanceFrequency(&frequency);
    // COM calls run after dropping the owner mutex. Local AddRefs remain valid
    // even if cleanup releases the original binding while a query is in flight.
    for(auto const& owner:owners)if(owner){
        gears_vr::ComPtr<IDXGISwapChain> chain;DXGI_SWAP_CHAIN_DESC desc{};
        if(FAILED(owner->QueryInterface(IID_PPV_ARGS(&chain)))||FAILED(chain->GetDesc(&desc)))continue;
        DWORD pid{};if(desc.OutputWindow)GetWindowThreadProcessId(desc.OutputWindow,&pid);
        if(pid!=GetCurrentProcessId()||!desc.BufferDesc.Width||!desc.BufferDesc.Height||!desc.BufferCount)continue;
        ++candidates;identity=reinterpret_cast<uintptr_t>(owner.Get());window=reinterpret_cast<uintptr_t>(desc.OutputWindow);
        width=desc.BufferDesc.Width;height=desc.BufferDesc.Height;windowed=desc.Windowed!=FALSE;
    }
    FILETIME creation{},exit{},kernel{},user{};
    const bool process_valid=GetProcessTimes(GetCurrentProcess(),&creation,&exit,&kernel,&user)!=FALSE;
    const auto creation_time=(static_cast<unsigned long long>(creation.dwHighDateTime)<<32)|creation.dwLowDateTime;
    const bool valid=sample&&process_valid&&clock_valid&&candidates==1;
    wchar_t path[MAX_PATH];if(!GetModuleFileNameW(own_module,path,MAX_PATH))return;
    auto slash=wcsrchr(path,L'\\');if(!slash)return;
    wcscpy_s(slash+1,MAX_PATH-static_cast<size_t>(slash+1-path),L"swapchain-output-status.txt");
    wchar_t temporary[MAX_PATH];if(swprintf_s(temporary,L"%s.tmp",path)<0)return;
    FILE* output=_wfsopen(temporary,L"w",_SH_DENYWR);if(!output)return;
    fprintf(output,"schema=1 pid=%lu creation_time=%llu phase=%s valid=%u candidates=%u hwnd=%llu chain=%llu width=%u height=%u windowed=%u qpc=%lld qpc_frequency=%lld complete=1\n",
        GetCurrentProcessId(),creation_time,phase,valid?1u:0u,candidates,
        static_cast<unsigned long long>(valid?window:0),static_cast<unsigned long long>(valid?identity:0),
        valid?width:0,valid?height:0,valid&&windowed?1u:0u,qpc.QuadPart,frequency.QuadPart);
    const bool written=fflush(output)==0;const bool closed=fclose(output)==0;
    // A reader that holds the old file without delete sharing can defer this
    // update; consumers must reject stale QPC evidence in that case.
    if(written&&closed)MoveFileExW(temporary,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
}
void frame_seen_list(uintptr_t identity) {
    if(!identity)return;
    auto bucket=((identity>>4)^(identity>>17))%frame_unique_lists.size();
    for(size_t probe=0;probe<frame_unique_lists.size();++probe){auto& entry=frame_unique_lists[(bucket+probe)%frame_unique_lists.size()];
        if(entry==identity)return;if(!entry){entry=identity;++frame_unique_list_count;return;}}
    ++frame_unique_list_overflow;
}
void remember_swapchain(unsigned method,IUnknown* input,IDXGISwapChain* chain) {
    if(!(prime_recording.load() || (frame_link_diagnostics && recording.load())) || !input || !chain)return;
    ID3D12CommandQueue* queue{};DXGI_SWAP_CHAIN_DESC desc{};
    if(FAILED(input->QueryInterface(IID_PPV_ARGS(&queue))))return;
    if(FAILED(chain->GetDesc(&desc))){queue->Release();return;}
    DWORD owner{};if(desc.OutputWindow)GetWindowThreadProcessId(desc.OutputWindow,&owner);
    auto chain_id=canonical_identity(chain);
    gears_swapchain_binding::Binding binding{chain_id,canonical_identity(queue),reinterpret_cast<uintptr_t>(desc.OutputWindow),
        desc.BufferDesc.Width,desc.BufferDesc.Height,method};
    bool accepted=swapchain_bindings.bind(binding,owner==GetCurrentProcessId(),queue->GetDesc().Type==D3D12_COMMAND_LIST_TYPE_DIRECT);
    if(accepted&&!hold_binding(chain,queue)){swapchain_bindings.invalidate();accepted=false;}
    queue->Release();
    if(!accepted)return;
    if(!desc.BufferCount || desc.BufferCount>16){swapchain_bindings.invalidate();return;}
    for(unsigned i=0;i<desc.BufferCount;++i){ID3D12Resource* buffer{};
        if(FAILED(chain->GetBuffer(i,IID_PPV_ARGS(&buffer)))){swapchain_bindings.invalidate();break;}
        auto resource=canonical_identity(buffer);buffer->Release();
        if(!swapchain_bindings.buffer(chain_id,resource,i)){swapchain_bindings.invalidate();break;}
    }
}
HRESULT STDMETHODCALLTYPE hook_create_swapchain(IDXGIFactory* factory,IUnknown* input,DXGI_SWAP_CHAIN_DESC* desc,IDXGISwapChain** out) {
    Scope scope;
    const auto startup=gears_swapchain_startup::begin_factory(gears_swapchain_startup::create_legacy,factory,input,nullptr,desc,sizeof(*desc),nullptr,nullptr,out,_ReturnAddress(),prime_recording.load(),recording.load());
    auto result=original_create_swapchain(factory,input,desc,out);
    IDXGISwapChain* chain{};if(SUCCEEDED(result)&&out&&safe_copy(&chain,out,sizeof(chain)))remember_swapchain(1,input,chain);
    gears_swapchain_startup::finish(startup,result,chain);
    return result;
}
HRESULT STDMETHODCALLTYPE hook_create_hwnd(IDXGIFactory2* factory,IUnknown* input,HWND window,const DXGI_SWAP_CHAIN_DESC1* desc,
    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* full,IDXGIOutput* restrict_output,IDXGISwapChain1** out) {
    Scope scope;
    const auto startup=gears_swapchain_startup::begin_factory(gears_swapchain_startup::create_hwnd,factory,input,window,desc,sizeof(*desc),full,restrict_output,out,_ReturnAddress(),prime_recording.load(),recording.load());
    auto result=original_create_hwnd(factory,input,window,desc,full,restrict_output,out);
    IDXGISwapChain1* chain{};if(SUCCEEDED(result)&&out&&safe_copy(&chain,out,sizeof(chain)))remember_swapchain(2,input,chain);
    gears_swapchain_startup::finish(startup,result,chain);
    return result;
}
HRESULT STDMETHODCALLTYPE hook_create_composition(IDXGIFactory2* factory,IUnknown* input,const DXGI_SWAP_CHAIN_DESC1* desc,
    IDXGIOutput* restrict_output,IDXGISwapChain1** out) {
    Scope scope;
    const auto startup=gears_swapchain_startup::begin_factory(gears_swapchain_startup::create_composition,factory,input,nullptr,desc,sizeof(*desc),nullptr,restrict_output,out,_ReturnAddress(),prime_recording.load(),recording.load());
    auto result=original_create_composition(factory,input,desc,restrict_output,out);
    IDXGISwapChain1* chain{};if(SUCCEEDED(result)&&out&&safe_copy(&chain,out,sizeof(chain)))remember_swapchain(3,input,chain);
    gears_swapchain_startup::finish(startup,result,chain);
    return result;
}
std::unique_lock<FrameHookMutex> frame_lock() {
    gears_render_cost::Scope cost(gears_render_cost::frame_lock);
    std::unique_lock<FrameHookMutex> lock(frame_hook_mutex,std::defer_lock);
    bool required=frame_link_diagnostics&&recording.load();
    required=required||(owned_bindings_enabled&&prime_recording.load());
    required=required||native_owned_scene_retained();
#if defined(GEARS_OWNED_OUTPUT_TRACKING) && GEARS_OWNED_OUTPUT_TRACKING
    required=required||frame_link.owned_status().enabled;
#endif
    if(required){
        if(self_testing&&frame_test_waiting)SetEvent(frame_test_waiting);
        lock.lock();
    }
    return lock;
}
void record_final_eye_failure(unsigned stage,unsigned reason,uintptr_t caller=0,uintptr_t list=0,
    gears_viewport::TargetDraw binding={},gears_frame_link::ListRef ref={}) {
    if((stage!=1&&!final_eye_candidate.ticket)||(final_eye_candidate.caller!=0xd83b1c&&final_eye_candidate.caller!=0xd808d8&&final_eye_candidate.caller!=0x2b11a0))return;
    // Keep the diagnostic snapshot safe even if recording was switched off
    // between hook admission and frame_lock's conditional acquisition.
    std::lock_guard<FrameHookMutex> diagnostic_guard(frame_hook_mutex);
    gears_final_eye_diagnostics::Row row{};
    const auto registered=swapchain_bindings.registered(final_eye_inventory.chain);
    row.epoch={committed_world_generation.load(),registered.generation,final_eye_inventory.epoch,final_eye_inventory.chain};
    auto& g=row.geometry;g.stage=stage;g.reason=reason;g.quad=final_eye_candidate;g.draw_caller=caller;
    g.resource=binding.target.resource;g.target_width=binding.target.width;g.target_height=binding.target.height;
    g.targets=binding.targets;g.mip=binding.target.mip;g.format=binding.target.format;
    g.provenance=frame_target_stats.binding_provenance(list);
    g.eye_valid=final_eye.valid;g.eye=final_eye.eye;g.eye_width=final_eye.width;g.eye_height=final_eye.height;
    g.eye_rect={final_eye.rect.x,final_eye.rect.y,final_eye.rect.width,final_eye.rect.height};
    row.first_qpc=now();row.list=list;row.list_generation=ref.generation;row.inventory_qpc=final_eye_inventory.qpc;
    row.present_width=final_eye_inventory.width;row.present_height=final_eye_inventory.height;
    row.present_count=final_eye_inventory.count;row.present_valid=final_eye_inventory.valid;
    row.present_resources=final_eye_inventory.resources;
    final_eye_failures.record(row);
}
struct FrameWorkSample {
    unsigned kind{},present_depth{},inside_present{},inside_consume{},ticket_present{},draw_kind{},elements{},instances{},targets{};
    uintptr_t caller{},list{},queue{};gears_viewport::Target target{};gears_viewport::DrawState rectangles{};
    uint64_t calls{};long long first_qpc{};unsigned stack_count{};std::array<void*,24> stack{};
};
std::array<FrameWorkSample,64> frame_work_samples{};
uint64_t frame_work_sample_overflow{};unsigned frame_work_stacks{};
std::array<uint64_t,4> frame_work_partition_dropped{};
std::array<unsigned,4> frame_work_partition_stacks{};
void sample_frame_work(unsigned kind,uintptr_t caller,uintptr_t list,uintptr_t queue,
    gears_viewport::TargetDraw binding,gears_viewport::DrawState rectangles,unsigned draw_kind,unsigned elements,unsigned instances) {
    if(!frame_trial_anchored)return;
    FrameWorkSample value{};value.kind=kind;value.caller=caller;value.list=list;value.queue=queue;
    value.present_depth=frame_present_depth;value.inside_present=frame_inside_original_present?1:0;
    value.inside_consume=consuming_renderer?1:0;value.ticket_present=consuming_frame_ticket?1:0;
    value.target=binding.target;value.targets=binding.targets;value.rectangles=rectangles;
    value.draw_kind=draw_kind;value.elements=elements;value.instances=instances;value.calls=1;
    // Preserve room for later pause/UI draw routes after noisy unknown Execute.
    // Same-class matching is unchanged; total storage and stack budget stay fixed.
    const unsigned partition=kind==2?0:kind==1?1:value.ticket_present?2:3;
    constexpr unsigned starts[]={0,8,12,44},ends[]={8,12,44,64},stack_limits[]={1,0,9,6};
    FrameWorkSample* empty{};
    for(unsigned i=starts[partition];i<ends[partition];++i){auto& entry=frame_work_samples[i];if(!entry.calls){if(!empty)empty=&entry;continue;}
        if(entry.kind==value.kind&&entry.caller==value.caller&&entry.queue==value.queue&&
           (kind!=2||entry.list==value.list)&&entry.present_depth==value.present_depth&&entry.inside_present==value.inside_present&&
           entry.inside_consume==value.inside_consume&&entry.ticket_present==value.ticket_present&&entry.target==value.target&&
           entry.targets==value.targets&&entry.draw_kind==value.draw_kind&&entry.elements==value.elements&&entry.instances==value.instances&&
           entry.rectangles.tracked==rectangles.tracked&&entry.rectangles.viewport==rectangles.viewport&&entry.rectangles.scissor==rectangles.scissor&&
           entry.rectangles.viewport_count==rectangles.viewport_count&&entry.rectangles.scissor_count==rectangles.scissor_count){++entry.calls;return;}}
    if(!empty){++frame_work_sample_overflow;++frame_work_partition_dropped[partition];return;}
    value.first_qpc=now();
    if(frame_work_partition_stacks[partition]<stack_limits[partition]){++frame_work_stacks;++frame_work_partition_stacks[partition];value.stack_count=CaptureStackBackTrace(0,static_cast<DWORD>(value.stack.size()),value.stack.data(),nullptr);}
    *empty=value;
}
void observe_frame_draw(ID3D12GraphicsCommandList* list,uintptr_t caller,unsigned kind,unsigned elements,unsigned instances) {
    if(!recording.load())return;
    dlss_route_observer::draw(reinterpret_cast<uintptr_t>(list),caller,kind,elements,instances);
    ngx_stereo_host::draw(list,caller,elements,instances);
    gears_render_cost::Scope cost(gears_render_cost::frame_draw);
    if(owned_bindings_enabled){
        const auto identity=canonical_identity(list);
        const auto binding=owned_target_stats.binding(identity);
        const bool known=!owned_target_stats.descriptor_capacity_lost()&&
            owned_target_stats.draw_state(identity).tracked&&(!binding.targets||binding.target.resource);
#if defined(GEARS_NATIVE_OWNED_PIXELS) && GEARS_NATIVE_OWNED_PIXELS
        if(!known)owned_scene_gpu::uncertain(owned_scene_gpu::Unknown::descriptor);
#endif
        if(known)native_owned_scene_draw(list,binding,caller,kind,elements,instances);
    }
    if(!frame_link_diagnostics)return;
    auto binding=frame_target_stats.binding(reinterpret_cast<uintptr_t>(list));
    auto diagnostic_failure=[&](unsigned reason){if(final_eye_candidate.ticket)record_final_eye_failure(2,reason,caller-base,reinterpret_cast<uintptr_t>(list),
        binding,frame_link.current(canonical_identity(list)));};
    if(frame_target_stats.descriptor_capacity_lost()){diagnostic_failure(64);frame_link.invalidate();return;}
    if(!owned_bindings_enabled)native_owned_scene_draw(list,binding,caller,kind,elements,instances);
    if(!binding.targets){diagnostic_failure(128);return;} // No render target: no observed color write.
    if(!binding.target.resource){
        diagnostic_failure(256);++frame_counts.missing_target;frame_link.invalidate();return;}
    const bool known_buffer=swapchain_bindings.known_buffer(binding.target.resource);
    gears_frame_link::ListRef ref{};gears_final_transfer::EyeDrawEvidence output_evidence{};
    if(dlss_output_control::scoped()){
        ref=frame_link.current(canonical_identity(list));
        output_evidence=dlss_output_control::draw(reinterpret_cast<uintptr_t>(list),caller,kind,elements,instances,binding,ref,
            frame_target_stats.draw_state(reinterpret_cast<uintptr_t>(list)));}
    bool exact=gears_final_transfer::eligible_eye_target(final_eye_candidate.caller,
        final_eye.valid&&(final_eye_candidate.caller!=0xd83cbf||output_evidence.eye==final_eye.eye),
        consuming_frame_ticket,final_eye.width,final_eye.height,binding,final_transfer_live::enabled(),output_evidence,committed_world_generation.load());
    // The authenticated source path uses only a single-mip R10G10B10A2 target.
    // Keep unrelated scene draws on the original cheap rejection path.
    if(!known_buffer&&(!final_transfer_live::enabled()||binding.target.format!=24||binding.target.mip)){
        diagnostic_failure(512);++frame_counts.filtered;return;}
    if(!ref.identity)ref=frame_link.current(canonical_identity(list));
    if(!known_buffer&&!exact&&!final_transfer_live::tracks(ref,binding.target.resource)){
        diagnostic_failure(512);++frame_counts.filtered;return;}
    ++frame_counts.draws;
    auto stats=frame_buffer_count(binding.target.resource);if(stats)++stats->draws;
    if(known_buffer&&final_transfer_live::observe(list,caller,kind,elements,instances,binding,ref))return;
    if(!exact)record_final_eye_failure(2,(!final_eye.valid?1u:0u)|(!consuming_frame_ticket?2u:0u)|
        (binding.targets!=1?4u:0u)|(binding.target.mip?8u:0u)|
        (binding.target.width!=final_eye.width?16u:0u)|(binding.target.height!=final_eye.height?32u:0u),
        caller-base,reinterpret_cast<uintptr_t>(list),binding,ref);
    if(exact&&stereo_gpu_publish){
        if(!verified_stereo_draw(frame_target_stats.draw_state(reinterpret_cast<uintptr_t>(list)),final_eye,caller,kind,elements,instances)){
            record_final_eye_failure(2,1024,caller-base,reinterpret_cast<uintptr_t>(list),binding,ref);
            ++stereo_publish_counts.rect_rejected;frame_link.invalidate();return;}
        if(!retain_stereo_draw())return;
        if(!known_buffer)final_transfer_live::record_source_draw();
    }
    sample_frame_work(exact?1:0,caller,reinterpret_cast<uintptr_t>(list),0,binding,
        frame_target_stats.draw_state(reinterpret_cast<uintptr_t>(list)),kind,elements,instances);
    if(exact){if(stats){if(final_eye.eye)++stats->right;else ++stats->left;}
        if(outage_diagnostics.load()&&final_eye.eye<2){auto& latest=outage_draws[final_eye.eye];
            latest={consuming_frame_ticket,latest.count+1,consuming_frozen_frame.sample.sequence,now(),consuming_frozen_frame.sample.received_ns};}
        frame_link.draw_final(ref,consuming_frame_ticket,final_eye.eye,
        {binding.target.resource,final_eye.width,final_eye.height},final_eye.rect);}
    else {++frame_counts.unknown;if(!consuming_frame_ticket)++frame_counts.missing_ticket;
        if(outage_diagnostics.load()&&!frame_inside_original_present){
            gears_outage_sources::Draw draw{};draw.qpc=now();draw.list=canonical_identity(list);
            draw.generation=ref.generation;draw.target=binding.target.resource;draw.caller=caller;
            draw.kind=kind;draw.elements=elements;draw.instances=instances;
            draw.width=static_cast<unsigned>(binding.target.width);draw.height=binding.target.height;
            draw.overlay=outage_source_overlay;
#if defined(GEARS_UI_MATERIAL_ROUTE) && GEARS_UI_MATERIAL_ROUTE
            if(ui_material_context){const auto& context=*ui_material_context;
                draw.material_sequence=context.sequence;draw.material_invocation=context.invocation;
                draw.material_payload=context.consumer.receipt.payload;draw.material_command=context.consumer.command;
                draw.material_proxy=context.consumer.receipt.proxy;draw.material_kind=context.consumer.receipt.kind;}
#endif
            outage_sources.unknown(draw);
        }
        if(stats)++stats->unknown;
        frame_link.draw_unknown(ref,binding.target.resource);}
}
void observe_frame_execute(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList* const* members,uintptr_t caller,
                           gears_frame_link::ExecutionToken* owned_token=nullptr) {
    if(!owned_token&&(!frame_link_diagnostics || !recording.load()))return;
    ++frame_counts.executes;
    auto queue_id=canonical_identity(queue);ExecuteQueueCount* stats{};ExecuteQueueCount* empty{};
    for(auto& entry:frame_queues){if(entry.identity==queue_id&&queue_id)stats=&entry;if(!entry.identity&&!empty)empty=&entry;}
    if(!stats&&empty&&queue_id){stats=empty;stats->identity=queue_id;stats->type=queue->GetDesc().Type;}
    if(!stats)++frame_unique_queue_overflow;else {++stats->calls;stats->members+=count;if(frame_trial_anchored)++stats->post_anchor_calls;}
    std::array<ID3D12CommandList*,gears_frame_link::Tracker::list_capacity> lists{};
    std::array<gears_frame_link::ListRef,gears_frame_link::Tracker::list_capacity> refs{};
    if(!count || count>lists.size() || !safe_copy(lists.data(),members,size_t(count)*sizeof(lists[0]))) {
        ++frame_counts.execute_rejected;if(stats)++stats->rejected;
        if(owned_token)frame_link.prepare_execute(queue_id,nullptr,count,*owned_token);
        else frame_link.execute(queue_id,nullptr,count);
        return;
    }
    for(unsigned i=0;i<count;++i){auto identity=canonical_identity(lists[i]);frame_seen_list(identity);refs[i]=frame_link.current(identity);
        if(!refs[i].generation)sample_frame_work(2,caller,identity,queue_id,
            frame_target_stats.binding(reinterpret_cast<uintptr_t>(lists[i])),frame_target_stats.draw_state(reinterpret_cast<uintptr_t>(lists[i])),0,count,0);
        if(stats&&!refs[i].generation){++stats->unknown_members;
            if(frame_trial_anchored)++stats->post_anchor_unknown;
            bool saved=false;for(unsigned j=0;j<stats->first_unknown_count;++j)if(stats->first_unknown[j]==identity)saved=true;
            if(!saved&&stats->first_unknown_count<stats->first_unknown.size())stats->first_unknown[stats->first_unknown_count++]=identity;}}
    if(frame_link_diagnostics&&recording.load())for(unsigned i=0;i<count;++i){
        observe_late_execute(queue_id,refs[i],frame_counts.executes.load(),i);
        observe_unpossessed_quad_execute(queue_id,refs[i],frame_counts.executes.load(),i);
        death_hdr_timeline::executed(queue_id,refs[i],frame_counts.executes.load(),i);
        observe_backbuffer_copy_execute(queue_id,refs[i],frame_counts.executes.load(),i);}
    const bool accepted=owned_token?frame_link.prepare_execute(queue_id,refs.data(),count,*owned_token):
        frame_link.execute(queue_id,refs.data(),count);
    if(!accepted){++frame_counts.execute_rejected;if(stats)++stats->rejected;}
}
void outage_begin(gears_outage::Sample& sample,long long timestamp) {
    outage_source_interval=outage_sources.begin(timestamp,{outage_draws[0].ticket,outage_draws[1].ticket});
    sample.enter_qpc=timestamp;sample.pub_before=stereo_publisher.frames.load();
    if(outage_built.read(sample.built_pre))sample.valid|=gears_outage::built_before;
    sample.draw=outage_draws;
    if(sample.draw[0].count)sample.valid|=gears_outage::left_draw;
    if(sample.draw[1].count)sample.valid|=gears_outage::right_draw;
}
void outage_finish(gears_outage::Sample& sample,long long timestamp,HRESULT result) {
    sample.original_end_qpc=timestamp;sample.present_result=result;sample.valid|=gears_outage::present_return;
    sample.pub_after=stereo_publisher.frames.load();
    if(outage_built.read(sample.built_post))sample.valid|=gears_outage::built_after;
    outage_window.observe(sample); // Existing outer frame lock still owned even if recording turned off.
    outage_source_interval.end_qpc=timestamp;outage_source_interval.published=sample.published;
    outage_source_interval.published_qpc=sample.published_qpc;outage_source_interval.target=sample.buffer;
    outage_source_interval.ticket=sample.ticket;outage_source_interval.rejection=sample.rejection;
    outage_sources.observe(outage_source_interval);
}
void observe_frame_present(IDXGISwapChain* chain,UINT flags,gears_outage::Sample* diagnostic=nullptr) {
    if(!frame_link_diagnostics || !recording.load())return;
    owned_scene_gpu::present();
    auto identity=canonical_identity(chain);auto binding=swapchain_bindings.registered(identity);
    if(!binding.chain){++frame_counts.unbound;frame_link.invalidate();frame_link.present(0,0);ngx_output_pixels::end_ticket(0,false);retire_stereo_window();frame_target_stats.clear_bindings();
        if(outage_diagnostics.load())outage_draws={};return;}
    if(binding.resizing){frame_link.invalidate_all();ngx_output_pixels::end_ticket(0,false);retire_stereo_window();frame_target_stats.clear_bindings();return;}
    ++frame_counts.presents;if(flags&DXGI_PRESENT_TEST){++frame_counts.tests;
        frame_link.invalidate();frame_link.present(0,0);ngx_output_pixels::end_ticket(0,false);retire_stereo_window();frame_target_stats.clear_bindings();
        if(outage_diagnostics.load())outage_draws={};return;}
    DXGI_SWAP_CHAIN_DESC desc{};std::array<uintptr_t,16> resources{};auto desc_result=chain->GetDesc(&desc);bool valid=SUCCEEDED(desc_result);
    if(!valid)frame_read_failure(1,0,desc_result);
    valid=valid&&desc.BufferCount&&desc.BufferCount<=resources.size();
    if(!desc.BufferCount||desc.BufferCount>resources.size())frame_read_failure(2,desc.BufferCount,E_INVALIDARG);
    if(valid)for(unsigned i=0;i<desc.BufferCount;++i){ID3D12Resource* buffer{};
        auto buffer_result=chain->GetBuffer(i,IID_PPV_ARGS(&buffer));
        if(FAILED(buffer_result)){frame_read_failure(3,i,buffer_result);valid=false;break;}
        resources[i]=canonical_identity(buffer);buffer->Release();if(!resources[i]){frame_read_failure(4,i,S_OK);valid=false;break;}
    }
    {
    std::lock_guard<FrameHookMutex> diagnostic_guard(frame_hook_mutex);
    if(final_eye_inventory.chain!=identity||final_eye_inventory.width!=desc.BufferDesc.Width||
       final_eye_inventory.height!=desc.BufferDesc.Height||final_eye_inventory.count!=desc.BufferCount||
       final_eye_inventory.valid!=valid||final_eye_inventory.resources!=resources)++final_eye_inventory.epoch;
    final_eye_inventory.chain=identity;final_eye_inventory.qpc=now();final_eye_inventory.width=desc.BufferDesc.Width;
    final_eye_inventory.height=desc.BufferDesc.Height;final_eye_inventory.count=desc.BufferCount;
    final_eye_inventory.valid=valid;final_eye_inventory.resources=resources;
    }
    bool anchored=false;
    if(valid&&!binding.anchored&&!(flags&DXGI_PRESENT_TEST)) {
        anchored=swapchain_bindings.anchor(identity,desc.BufferDesc.Width,desc.BufferDesc.Height,resources.data(),desc.BufferCount);
        if(anchored){frame_trial_anchored=true;frame_link.invalidate_all();retire_stereo_window();frame_target_stats.clear_bindings();}
    }
    bool compared=anchored||swapchain_bindings.unchanged(identity,desc.BufferDesc.Width,desc.BufferDesc.Height,resources.data(),desc.BufferCount);
    bool inventory_ok=valid&&compared;
    if(inventory_ok)bind_stereo_publisher(identity,binding.queue);
    if(!inventory_ok) {
        ++frame_counts.changed;swapchain_bindings.invalidate();frame_link.invalidate();
    }
    uintptr_t resource{};IDXGISwapChain3* current{};
    auto current_result=valid?chain->QueryInterface(IID_PPV_ARGS(&current)):E_FAIL;
    if(valid&&SUCCEEDED(current_result)) {
        auto index=current->GetCurrentBackBufferIndex();current->Release();
        if(index<desc.BufferCount)resource=resources[index];else frame_read_failure(6,index,E_INVALIDARG);
    }
    else if(valid)frame_read_failure(5,0,current_result);
    if(!resource || flags&DXGI_PRESENT_TEST)frame_link.invalidate();
    if(frame_target_stats.clear_bindings()||frame_target_stats.descriptor_capacity_lost())frame_link.invalidate();
#if defined(GEARS_GAME_UI_MATERIAL) && GEARS_GAME_UI_MATERIAL
    if(game_material::block_present())frame_link.invalidate();
#endif
    if(ui_semantic_consumer_pending)frame_link.invalidate(); // Nonblocking: semantic consumers have not passed their final identity bracket.
    auto result=frame_link.present(resource,inventory_ok?binding.queue:0);
    ngx_output_pixels::end_ticket(result.ticket,result.linked&&!result.contaminated,
        {true,result.linked,result.contaminated,result.rejection,resource});
    if(diagnostic){diagnostic->valid|=gears_outage::association;diagnostic->buffer=resource;diagnostic->queue=binding.queue;
        diagnostic->ticket=result.ticket;diagnostic->rejection=result.rejection;diagnostic->linked=result.linked;diagnostic->contaminated=result.contaminated;}
    if(!owned_xr_probe&&stereo_gpu_publish&&result.linked&&inventory_ok&&final_transfer_live::normalizer::publication_allowed()&&vr_dof_compat::publication(committed_world_generation.load(),result.ticket)&&
       dlss_output_control::publication(committed_world_generation.load(),result.ticket)){
        gears_vr::StereoImageTicket image{};image.id=result.ticket;
        if(!stereo_ticket_store.lookup(result.ticket,image.sample,simulator_operator)){++stereo_publish_counts.ticket_missing;}
        else {
            if(diagnostic){diagnostic->valid|=gears_outage::image_sample;diagnostic->image_session=image.sample.session;
                diagnostic->image_sequence=image.sample.sequence;diagnostic->image_received_ns=image.sample.received_ns;}
            for(unsigned eye=0;eye<2;++eye){auto r=result.eyes[eye];image.rect[eye]={static_cast<int>(r.x),static_cast<int>(r.y),static_cast<int>(r.width),static_cast<int>(r.height)};}
            ++stereo_publish_counts.capture_attempts;StereoPublishBypass bypass;
            auto deadline=stereo_publish_deadline.load();
            gears_vr::GpuCaptureTrace capture_trace{};bool captured=false;
            if(!stereo_publish_counts.stopped.load()){
                if(!deadline||now()<deadline){
                    captured=stereo_publisher.capture(chain,&image,diagnostic?&capture_trace:nullptr);
                    if(diagnostic)diagnostic->valid|=gears_outage::capture_called;
                }else capture_trace.reason=gears_vr::GpuCaptureReason::native_deadline;
            }else capture_trace.reason=gears_vr::GpuCaptureReason::native_stopped;
            if(captured)stereo_publish_counts.last_ticket=result.ticket;else ++stereo_publish_counts.capture_skipped;
            if(diagnostic){diagnostic->capture_reason=static_cast<unsigned>(capture_trace.reason);diagnostic->capture_valid=capture_trace.valid;
                diagnostic->capture_state=capture_trace.state;diagnostic->consumer=static_cast<unsigned>(capture_trace.consumer);
                diagnostic->serial_before=capture_trace.serial_before;diagnostic->serial_after=capture_trace.serial_after;
                diagnostic->ready_completed=capture_trace.ready_completed;diagnostic->done_completed=capture_trace.done_completed;
                diagnostic->capture_result=capture_trace.result;
                if(captured){diagnostic->published=true;diagnostic->published_qpc=now();diagnostic->valid|=gears_outage::publication;}}
        }
    }
    retire_stereo_window();
    if(auto stats=frame_buffer_count(resource)){++stats->presents;if(result.linked)++stats->linked;if(result.contaminated)++stats->contaminated;}
    if(result.linked){++frame_counts.linked;frame_counts.last_ticket=result.ticket;}
    if(result.contaminated)++frame_counts.contaminated;
    for(unsigned i=0;i<frame_counts.rejection.size();++i)if(result.rejection&(1u<<i))++frame_counts.rejection[i];
    if(outage_diagnostics.load())outage_draws={}; // Same pre-original consume boundary as Tracker::present.
}
void retire_original_present_interval() {
    // The outer Present hook still owns frame_hook_mutex here. Reentrant work
    // from original Present is an isolated observation interval; other threads
    // acquire the ordering lock only after this retirement completes. This is
    // not recovery: lost capacity remains sticky, and future work needs Reset.
    frame_link.retire_present_interval();
    owned_scene_gpu::present();
    retire_stereo_window();
    if(frame_target_stats.clear_bindings())frame_link.invalidate_capacity();
    if(outage_diagnostics.load())outage_draws={}; // Isolated original-Present work cannot become next-frame evidence.
}
void stereo_publish_worker_tick() {
    ngx_mv_target_capture::poll();
    death_hdr_pixels::poll();
    ngx_output_pixels::poll();
    owned_source_pixels::poll();
    death_pso_cache::poll();
    ngx_mv_compute_capture::poll_picked_blob();
    ngx_root_signature_capture::poll();
    desktop_mirror_readback::poll();
    if(!stereo_gpu_publish)return;
    if(continuous_session&&submitted_lifetime.stopping()&&!stereo_publish_counts.stopped.load()){
        stereo_publisher.mark_stopped();stereo_publish_counts.stopped=1;
    }
    auto deadline=stereo_publish_deadline.load();
    if(!stereo_publish_counts.stopped.load()&&((deadline&&now()>=deadline)||
        (submitted_lifetime.is_closed()&&!submitted_lifetime.pending()&&!submitted_lifetime.snapshot().quiescing))){
        stereo_publisher.mark_stopped();stereo_publish_counts.stopped=1;
    }
    StereoPublishBypass bypass;
#if defined(GEARS_DESKTOP_MIRROR) && GEARS_DESKTOP_MIRROR
    const auto mirror_state=desktop_mirror.status().state;
    if(!stereo_publish_counts.stopped.load()&&mirror_state==1)desktop_mirror.prepare();
    if(mirror_state==2||mirror_state==3)desktop_mirror.poll();
#endif
    if(!stereo_publish_counts.stopped.load()&&stereo_publisher.state.load()==1){
        if(stereo_publisher.prepare())++stereo_publish_counts.prepare_ok;else ++stereo_publish_counts.prepare_failed;
    }
    stereo_publisher.poll();
}
bool close_stereo_publisher() {
    if(!stereo_gpu_publish)return true;
    stereo_publisher.mark_stopped();stereo_publish_counts.stopped=1;
    if(active.load())return false; // Retain all ownership if callbacks did not drain.
    StereoPublishBypass bypass;
    bool drained=stereo_publisher.drain(1000);
#if defined(GEARS_DESKTOP_MIRROR) && GEARS_DESKTOP_MIRROR
    const bool mirror_drained=desktop_mirror.drain(1000);drained=drained&&mirror_drained;
#endif
    stereo_publish_counts.drain_ok=drained?1:0;
    if(continuous_session&&!drained)return false; // Keep ownership for a later drain attempt.
    bool closed=stereo_publisher.close();
#if defined(GEARS_DESKTOP_MIRROR) && GEARS_DESKTOP_MIRROR
    closed=desktop_mirror.close(1000)&&closed;
#endif
    stereo_publish_counts.close_ok=closed?1:0;
    return drained&&closed;
}
void write_session_status(bool resources_retained=true,bool entries_restored=false) {
    resources_retained=resources_retained||native_surface_probe_retained();
    resources_retained=resources_retained||native_owned_probe_retained();
    resources_retained=resources_retained||native_owned_scene_retained();
    if(!continuous_session)return;
    write_swapchain_output_status(recording.load()&&session_phase.load()==SessionPhase::running,"running");
    wchar_t path[MAX_PATH];swprintf_s(path,L"%s\\session-status.txt",directory);
    gears_status::AtomicFile publication(path);
    FILE* output=publication.get();
    if(!output){session_status_available=false;return;}
    const char* phases[]={"running","stop_requested","native_closed","draining","stopped"};
    const char* reasons[]={"none","event","f8","focus_lost","proof_failure","owner_exited","owner_error"};
    auto phase=session_phase.load();auto reason=session_reason.load();
    const bool stop_recorded=session_stop_recorded.load(std::memory_order_acquire);
    const char* label=session_cleanup_timeout.load()&&phase!=SessionPhase::stopped?"incomplete":phases[static_cast<unsigned>(phase)];
    fprintf(output,"continuous_session=1 pid=%lu started_qpc=%lld phase=%s phase_code=%u reason=%s reason_code=%u stop_tick=%llu closed=%d pending=%u state_created=%u release_returned=%u state_destroyed=%u state_flags=%u retained=%u active=%u publisher_stopped=%u drain_ok=%u close_ok=%u cleanup_timeout=%u resources_retained=%u entries_restored=%u native_services=%u cancelled_builds=%u published=%u simulator_operator=%d csv_enabled=0 ",
        GetCurrentProcessId(),session_started_qpc,label,static_cast<unsigned>(phase),reasons[static_cast<unsigned>(reason)],static_cast<unsigned>(reason),
        session_stop_tick.load(),submitted_lifetime.is_closed(),submitted_lifetime.pending(),state_created.load(),state_release_returned.load(),state_destroyed.load(),
        state_destroyed.load()?state_delete_flags:0,submit_retained.load(),active.load(),stereo_publish_counts.stopped.load(),
        stereo_publish_counts.drain_ok.load(),stereo_publish_counts.close_ok.load(),session_cleanup_timeout.load()?1:0,
        resources_retained?1:0,entries_restored?1:0,session_native_services.load(),session_cancelled_builds.load(),stereo_publisher.frames.load(),simulator_operator);
    fprintf(output,"profile_consume_thread=%lu ",gears_render_cost::first_thread.load(std::memory_order_relaxed));
    fprintf(output,"activation_ready_qpc=%lld activation_qpc_frequency=%lld ",session_activation_ready_qpc.load(),qpc_frequency);
    ui_scene_trial_live::progress(output);
    owned_ui_live::progress(output);
#if defined(GEARS_OWNED_UI_MATERIAL) && GEARS_OWNED_UI_MATERIAL
    owned_material::progress(output);
#endif
    path_choice_position_live::progress(output);
    path_choice_layout_observer::progress(output);
    path_choice_close_observer::progress(output);
    // Diagnostic rates use this sample's QPC, not the owner's polling time.
    // Existing bound/top-level Present observations exclude DXGI_PRESENT_TEST
    // separately; they are not compositor display or engine-update counts.
    fprintf(output,"telemetry_schema=1 telemetry_qpc=%lld telemetry_qpc_frequency=%lld telemetry_present_enabled=%u telemetry_present_calls=%llu telemetry_present_tests=%llu telemetry_present_failures=%llu ",
        now(),qpc_frequency,frame_link_diagnostics?1u:0u,frame_counts.presents.load(),
        frame_counts.tests.load(),frame_counts.present_failed.load());
    owned_scene_gpu::print_progress(output);
    native_owned_scene_progress(output);
#if defined(GEARS_DESKTOP_MIRROR) && GEARS_DESKTOP_MIRROR
    const auto mirror=desktop_mirror.status();
    fprintf(output,"mirror_state=%u mirror_captures=%llu mirror_presents=%llu mirror_stale=%llu mirror_busy=%llu mirror_inventory_rejected=%llu mirror_timeouts=%llu mirror_reason=%u mirror_held=%u mirror_format=%u mirror_async=%u mirror_pending_draws=%u mirror_wait_ns=%llu mirror_max_wait_ns=%llu ",
        mirror.state,mirror.captures,mirror.presents,mirror.stale,mirror.busy,mirror_inventory_rejected.load(),mirror.timeouts,static_cast<unsigned>(mirror.reason),mirror.destination_held?1:0,mirror.destination_format,
        gears_vr::DesktopMirror::asynchronous?1:0,mirror.pending_draws,mirror.wait_ns,mirror.max_wait_ns);
#endif
    fprintf(output,"stop_recorded=%u stop_qpc=%lld stop_qpc_frequency=%lld stop_thread=%lu stop_source_file=%s stop_source_line=%u ",
        stop_recorded?1:0,stop_recorded?session_stop_qpc.load():0,qpc_frequency,
        stop_recorded?session_stop_thread.load():0,stop_recorded?session_stop_source_file.load():"none",
        stop_recorded?session_stop_source_line.load():0);
    fprintf(output,"recenter_requests=%llu reference_generation=%llu heading_deferred=%llu ",stereo_recenter_requests.load(),stereo_reference_generation.load(),stereo_heading_deferred.load());
    fprintf(output,"anchor_submission_required=%u anchor_submission_matched=%llu anchor_submission_rejected=%llu ",
        anchor_observer_enabled?1:0,anchor_submission_matched.load(),anchor_submission_rejected.load());
    fprintf(output,"unpossessed_submitted=%u unpossessed_first_ticket=%llu unpossessed_last_ticket=%llu ",
        unpossessed_submission_count.load(),unpossessed_first_ticket.load(),unpossessed_last_ticket.load());
    death_source_live::progress(output);
    death_hdr_pixels::progress(output);
    vr_dof_compat::progress(output);
    dlss_output_control::progress(output);
    fprintf(output,"world_generation=%llu world_candidate_generation=%llu world_prepare_requests=%llu world_prepare_rejected=%llu ",
        static_cast<unsigned long long>(frame_link.world_epoch()),static_cast<unsigned long long>(world_candidate_generation.load()),
        static_cast<unsigned long long>(world_prepare_requests.load()),static_cast<unsigned long long>(world_prepare_rejected.load()));
    fprintf(output,"world_handoff_supported=%u world_handoff_phase=%u world_handoff_commits=%llu world_quiesce_requested=%u state_release_started=%u epoch_submit_total=%llu ",
        world_handoff_supported?1:0,
        world_handoff_phase.load(),static_cast<unsigned long long>(world_handoff_commits.load()),world_quiesce_requested.load()?1:0,
        state_release_started.load(),static_cast<unsigned long long>(epoch_submit_total.load()));
    print_world_progress(output);
    fprintf(output,"owner_pid=%lu owner_creation_time=%llu stop_reason=%s owner_exit_observed=%u owner_error_observed=%u\n",session_owner_pid,
        static_cast<unsigned long long>(session_owner_creation_time),reasons[static_cast<unsigned>(reason)],
        session_owner_exit_observed.load()?1:0,session_owner_error_observed.load()?1:0);
    ngx_output_pixels::print_control(output);
    fprintf(output,"complete=1\n"); // Last field, after every diagnostic producer.
    session_status_available=publication.commit();
}
void print_anchor(FILE* status,bool hooks_disabled){
    fprintf(status,"anchor_submission_guard required=%u matched=%llu rejected=%llu policy=skip_unassociated_candidate\n",
        anchor_observer_enabled?1:0,anchor_submission_matched.load(),anchor_submission_rejected.load());
    if(!anchor_observer_enabled){fprintf(status,"anchor_observer enabled=0\n");return;}
    fprintf(status,"anchor_pawn_policy same_level_only=1 same_class_outer=1 startup_config_immutable=1 full_pawn_signature=1 scene_prepost_identity=1 absolute_uobject_serial_verified=0 accepted_reassociated_guard_scene=%llu accepted_reassociated_guard_family=%llu accepted_reassociated_guard_renderer=%llu\n",
        anchor_pawn_reassociated_guards[0].load(),anchor_pawn_reassociated_guards[1].load(),anchor_pawn_reassociated_guards[2].load());
    fprintf(status,"anchor_camera_policy cinematic_enabled=%u dynamic_target_requires_cinematic_mode=1 dynamic_target_exact_camera_actor=1 target_same_world_info=1 target_registered_level=1 controlled_possession_remains_pawn=1 unpossessed_enabled=%u unpossessed_requires_certified_pawn_target=1 unpossessed_requires_null_links=1\n",anchor_config.cinematic_enabled?1:0,anchor_config.unpossessed_camera_enabled?1:0);
    if(recording.load()||active.load()||!hooks_disabled){fprintf(status,"anchor_observer enabled=1 available=0 active=%u hooks_disabled=%u\n",active.load(),hooks_disabled?1:0);return;}
    fprintf(status,"anchor_observer enabled=1 available=1 schema=1 capacity=64 rows=%u reject_capacity=16 reject_rows=%u identity_count=%u accepted_scene=%llu accepted_family=%llu accepted_renderer=%llu first_renderer_qpc=%lld last_renderer_qpc=%lld sampled_out=%llu capacity_dropped=%llu dropped_first_qpc=%lld dropped_last_qpc=%lld qpc_frequency=%lld atomic_engine_snapshot=0 renderer_alignment_verified=0 origin_modified=%u source_view_bytes_matched=64 projection_bytes_observed=64 sample_policy=first4_then_1s\n",
        std::min<unsigned>(anchor_row_count.load(),64),std::min<unsigned>(anchor_reject_count.load(),16),anchor_config.identity_count,
        anchor_reasons[0][0].load(),anchor_reasons[1][0].load(),anchor_reasons[2][0].load(),anchor_first_renderer.load(),anchor_last_renderer.load(),
        anchor_sampled_out.load(),anchor_capacity_dropped.load(),anchor_dropped_first.load(),anchor_dropped_last.load(),qpc_frequency,
        origin_trial_enabled&&origin_stage_counts[1][0]?1:0);
    const char* stages[]={"scene","family","renderer"};
    for(unsigned stage=0;stage<3;++stage)fprintf(status,"anchor_guard stage=%s calls=%llu min_qpc=%lld max_qpc=%lld\n",stages[stage],anchor_guard_calls[stage].load(),anchor_guard_calls[stage].load()?anchor_guard_min[stage].load():0,anchor_guard_max[stage].load());
    for(unsigned stage=0;stage<3;++stage)for(unsigned reason=0;reason<gears_anchor::reason_count;++reason)
        fprintf(status,"anchor_reason stage=%s reason=%s count=%llu\n",stages[stage],gears_anchor::reason_name(reason),anchor_reasons[stage][reason].load());
    auto print=[&](AnchorRow const& row,const char* scope,unsigned index){auto const& r=row.receipt;auto const& a=r.anchor;
        fprintf(status,"anchor_row scope=%s index=%u stage=%s reason=%s scene_invocation=%llu thread=%u local=%llx family=%llx source=%llx source_owner=%llx pawn=%llx controller=%llx camera=%llx level=%llx scene_qpc=%lld family_qpc=%lld renderer_qpc=%lld camera_stamp_bits=%u location_x=%.9g location_y=%.9g location_z=%.9g eye_height=%.9g base_eye_height=%.9g candidate_x=%.9g candidate_y=%.9g candidate_z=%.9g renderer=%llx registered_ticket=%llu",
            scope,index,stages[row.stage],gears_anchor::reason_name(row.reason),r.invocation,r.thread,r.key.local,r.key.family,r.key.source,r.key.owner,
            a.pawn,a.controller,a.camera,a.level,r.scene_qpc,r.family_qpc,r.renderer_qpc,r.key.stamp,a.location[0],a.location[1],a.location[2],
            a.eye_height,a.base_eye_height,a.location[0],a.location[1],a.location[2]+a.eye_height,row.renderer,row.ticket);
        fprintf(status," cinematic=%u target=%llx target_index=%u target_name_index=%u target_number=%u target_class=%llx target_outer=%llx",
            a.cinematic?1:0,a.target_identity.address,a.target_identity.index,a.target_identity.name_index,
            a.target_identity.number,a.target_identity.cls,a.target_identity.outer);
        fprintf(status," target_level=%llx target_level_index=%u unpossessed=%u",a.target_level_identity.address,a.target_level_identity.index,a.unpossessed?1:0);
        const std::array<unsigned char,128>* matrices[]={&r.key.matrices,&r.family_matrices,&r.renderer_matrices};
        const char* labels[]={"scene_matrix","family_matrix","renderer_matrix"};
        for(unsigned stage=0;stage<3;++stage){fprintf(status," %s=",labels[stage]);for(unsigned j=0;j<32;++j){float value{};memcpy(&value,matrices[stage]->data()+4*j,4);fprintf(status,"%s%.9g",j?",":"",value);}}
        fprintf(status,"\n");};
    for(unsigned i=0;i<std::min<unsigned>(anchor_row_count.load(),64);++i)print(anchor_rows[i],"sampled",i);
    for(unsigned i=0;i<std::min<unsigned>(anchor_reject_count.load(),16);++i)print(anchor_rejects[i],"first_rejections",i);
    fprintf(status,"unpossessed_camera_guards scene=%llu family=%llu renderer=%llu submitted=%u stored=%u capacity=%u first_ticket=%llu last_ticket=%llu camera_closer_allowed=0 crosshair_attach_allowed=0\n",
        anchor_unpossessed_guards[0].load(),anchor_unpossessed_guards[1].load(),anchor_unpossessed_guards[2].load(),
        unpossessed_submission_count.load(),std::min<unsigned>(unpossessed_submission_count.load(),unpossessed_submission_capacity),unpossessed_submission_capacity,unpossessed_first_ticket.load(),unpossessed_last_ticket.load());
    for(unsigned i=0;i<std::min<unsigned>(unpossessed_submission_count.load(),unpossessed_submission_capacity);++i){auto const& r=unpossessed_submissions[i];
        fprintf(status,"unpossessed_submission index=%u ticket=%llu world=%llu qpc=%lld thread=%u\n",i,r.ticket,r.world,r.qpc,r.thread);}
    fprintf(status,"anchor_pawn_binding_rejections count=%u stored=%u capacity=16 additional_reads=0\n",
        anchor_pawn_reject_count.load(),std::min<unsigned>(anchor_pawn_reject_count.load(),16));
    for(unsigned i=0;i<std::min<unsigned>(anchor_pawn_reject_count.load(),16);++i){const auto& r=anchor_pawn_rejects[i];const auto& d=r.diagnostic;
        fprintf(status,"anchor_pawn_binding_rejection index=%u qpc=%lld thread=%u stage=%s field=%s address=%llx expected=%llx actual=%llx expected_valid=%u actual_valid=%u\n",
            i,r.qpc,r.thread,gears_anchor::pawn_binding_stage_name(d.stage),gears_anchor::pawn_binding_field_name(d.field),
            d.address,d.expected,d.actual,d.expected_valid?1:0,d.actual_valid?1:0);}
    fprintf(status,"anchor_state_observer capacity=16 rows=%u dropped=%u policy=first_pawn_or_right_state_transition_per_thread phase=renderer_guard_before_submit lifetime_verified=0\n",
        std::min<unsigned>(anchor_state_transition_count.load(),16),anchor_state_transition_count.load()>16?anchor_state_transition_count.load()-16:0);
    for(unsigned i=0;i<std::min<unsigned>(anchor_state_transition_count.load(),16);++i){auto const& r=anchor_state_transitions[i];
        fprintf(status,"anchor_state_transition index=%u qpc=%lld thread=%u pawn=%llx pawn_index=%u pawn_name_index=%u pawn_number=%u pawn_class=%llx pawn_outer=%llx source=%llx original_state=%llx right_state=%llx right_table=%llx read_mask=%u\n",
            i,r.qpc,r.thread,r.pawn.address,r.pawn.index,r.pawn.name_index,r.pawn.number,r.pawn.cls,r.pawn.outer,
            r.source,r.original_state,r.right_state,r.right_table,r.read_mask);}
#if defined(GEARS_PAWNLESS_CAMERA_OBSERVER) && GEARS_PAWNLESS_CAMERA_OBSERVER
    {
        std::lock_guard<std::mutex> lock(pawnless_scene_mutex);
        fprintf(status,"pawnless_scene_observer compiled=1 rows=%u capacity=16 period_ms=250 admission_changes=0 lifetime_verified=0 local_controller_offset_certified=0\n",pawnless_scene_count);
        auto pass=[&](unsigned index,const char* label,gears_pawnless_camera::Pass const& p){
            fprintf(status,"pawnless_camera_pass index=%u label=%s read_mask=%u range_rejected=%u registry_valid=%u identity_match=%u link_match=%u links=%llx,%llx,%llx,%llx,%llx,%llx stamp=%u time=%.9g location=%.9g,%.9g,%.9g fov=%.9g finite_location=%u valid_fov=%u finite_stamp=%u\n",
                index,label,p.read_mask,p.range_rejected_mask,p.registry_valid_mask,p.identity_match_mask,p.link_match_mask,
                p.links[0],p.links[1],p.links[2],p.links[3],p.links[4],p.links[5],p.timestamp,p.time,
                p.location[0],p.location[1],p.location[2],p.fov,p.finite_location?1:0,p.valid_fov?1:0,p.finite_timestamp?1:0);
        };
        auto snapshot=[&](unsigned index,const char* label,gears_pawnless_camera::Snapshot const& s){
            fprintf(status,"pawnless_camera_snapshot index=%u label=%s config_valid=%u sampled=%u initial_pawn_read=%u initial_pawn=%llx stable_mask=%u reads_complete=%u reads_stable=%u expected=%llx,%llx,%llx,%llx\n",
                index,label,s.config_valid?1:0,s.sampled?1:0,s.initial_pawn_read?1:0,s.initial_pawn,s.stable_mask,s.reads_complete?1:0,s.reads_stable?1:0,
                s.expected[0],s.expected[1],s.expected[2],s.expected[3]);
        };
        auto source=[&](unsigned index,const char* label,std::array<unsigned char,0x430> const& bytes){
            uintptr_t owner{},state{};std::array<float,32> matrices{};std::array<float,4> rect{};
            memcpy(&owner,bytes.data(),8);memcpy(&state,bytes.data()+8,8);memcpy(rect.data(),bytes.data()+0x48,16);memcpy(matrices.data(),bytes.data()+0x70,128);
            fprintf(status,"pawnless_source index=%u label=%s owner=%llx state=%llx raw_rect=%.9g,%.9g,%.9g,%.9g matrices=",index,label,owner,state,rect[0],rect[1],rect[2],rect[3]);
            for(unsigned n=0;n<matrices.size();++n)fprintf(status,"%s%.9g",n?",":"",matrices[n]);fprintf(status,"\n");
        };
        for(unsigned i=0;i<pawnless_scene_count;++i){auto const& r=pawnless_scene_rows[i];
            fprintf(status,"pawnless_scene index=%u begin_qpc=%lld end_qpc=%lld renderer_qpc=%lld thread=%u renderer_thread=%u local=%llx family=%llx source=%llx renderer_family=%llx renderer_source=%llx read_mask=%u bits=family:1,result_matches:2,scene_copy:4,renderer_copy:8,matrices_equal:16,bytes_equal:32\n",
                i,r.begin_qpc,r.end_qpc,r.renderer_qpc,r.thread,r.renderer_thread,r.local,r.family,r.source,r.renderer_family,r.renderer_source,r.read_mask);
            snapshot(i,"pre",r.before);snapshot(i,"post",r.after);
            pass(i,"pre_begin",r.before.before);pass(i,"pre_end",r.before.after);pass(i,"post_begin",r.after.before);pass(i,"post_end",r.after.after);
            if(r.read_mask&4)source(i,"scene",r.scene_bytes);if(r.read_mask&8)source(i,"renderer",r.renderer_bytes);
        }
    }
#else
    fprintf(status,"pawnless_scene_observer compiled=0 rows=0 admission_changes=0 additional_reads=0\n");
#endif
    unsigned image_size{};IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS64 nt{};
    if(safe_copy(&dos,reinterpret_cast<void*>(base),sizeof(dos))&&dos.e_magic==IMAGE_DOS_SIGNATURE&&
       dos.e_lfanew>0&&dos.e_lfanew<0x100000&&base<=UINTPTR_MAX-unsigned(dos.e_lfanew)&&
       safe_copy(&nt,reinterpret_cast<void*>(base+unsigned(dos.e_lfanew)),sizeof(nt))&&nt.Signature==IMAGE_NT_SIGNATURE)
        image_size=nt.OptionalHeader.SizeOfImage;
    fprintf(status,"anchor_target_snapshot capacity=4 rows=%u game_base=%llx game_size=%u virtual_calls=0 writes=0 ownership_proven=0 full_payload_consistency_proven=0\n",
        anchor_target_snapshot_count.load(),base,image_size);
    auto bytes=[&](const char* label,const auto& value){fprintf(status," %s=",label);for(auto byte:value)fprintf(status,"%02x",unsigned(byte));};
    for(unsigned i=0;i<anchor_target_snapshot_count.load();++i){auto const& row=anchor_target_snapshots[i];
        fprintf(status,"anchor_target_row index=%u family=%llx source=%llx canvas=%llx target=%llx target_after=%llx vtable=%llx vtable_after=%llx qpc=%lld thread=%u read_mask=%u method_mask=%u pointer_consistent=%u read_bits=family_target:1,target_bytes:2,vtable:4,vtable_slots:8,canvas:16,family_target_after:32,vtable_after:64,target_header_match:128\n",
            i,row.family,row.source,row.canvas,row.target,row.target_after,row.vtable,row.vtable_after,row.qpc,row.thread,row.read_mask,row.method_mask,row.consistent?1:0);
        fprintf(status,"anchor_target_bytes index=%u",i);bytes("target96",row.target_bytes);bytes("canvas64",row.canvas_bytes);fprintf(status,"\n");
        fprintf(status,"anchor_target_vtable index=%u bytes80=",i);
        const auto* table_bytes=reinterpret_cast<const unsigned char*>(row.methods.data());
        for(unsigned j=0;j<80;++j)fprintf(status,"%02x",unsigned(table_bytes[j]));fprintf(status,"\n");
        for(unsigned method=0;method<row.methods.size();++method){auto address=row.methods[method];
            bool in_game=image_size&&address>=base&&address-base<image_size;
            fprintf(status,"anchor_target_method index=%u slot=%u address=%llx in_game=%u rva=%llx prefix_read=%u",i,method,address,in_game?1:0,in_game?address-base:0,(row.method_mask>>method)&1);
            bytes("prefix16",row.prefixes[method]);fprintf(status,"\n");}
    }
}
void print_origin_trial(FILE* status,bool hooks_disabled){
    if(!origin_trial_enabled){fprintf(status,"origin_trial enabled=0\n");return;}
    if(recording.load()||active.load()||!hooks_disabled){fprintf(status,"origin_trial enabled=1 available=0\n");return;}
    std::lock_guard<std::mutex> lock(stereo_mutex);
    fprintf(status,"origin_trial enabled=1 available=1 schema=1 epoch_qpc=%lld frequency=%lld prepare_failed=%llu commit_failed=%llu duplicate_stages=%llu capacity_per_phase=12 sample_period_seconds=1 original_source_write_intended=0\n",
        origin_trial.epoch_qpc(),qpc_frequency,origin_prepare_failed.load(),origin_commit_failed.load(),origin_duplicate_stages);
    auto matrix=[&](const char* name,const void* bytes){fprintf(status," %s=",name);
        for(unsigned j=0;j<16;++j){float v{};memcpy(&v,static_cast<const unsigned char*>(bytes)+4*j,4);fprintf(status,"%s%.9g",j?",":"",v);}};
    const char* phases[]={"A1","B","A2"};const char* stages[]={"built","consume_before","prepare_before","prepare_after"};
    for(unsigned p=0;p<3;++p){
        for(unsigned s=0;s<4;++s)fprintf(status,"origin_count phase=%s stage=%s count=%llu mismatch=%llu\n",
            phases[p],stages[s],origin_stage_counts[p][s],origin_stage_mismatches[p][s]);
        for(unsigned i=0;i<origin_row_counts[p];++i){auto const& row=origin_rows[p][i];auto const& f=row.frame;
            auto const& r=f.origin.receipt;auto const& a=r.anchor;auto const& sample=f.sample;
            fprintf(status,"origin_row phase=%s ticket=%llu selection_qpc=%lld mask=%u invocation=%llu thread=%u scene_qpc=%lld family_qpc=%lld renderer_qpc=%lld family=0x%llx source=0x%llx owner=0x%llx local=0x%llx pawn=0x%llx controller=0x%llx camera=0x%llx level=0x%llx stamp=%u location=%.9g,%.9g,%.9g eye_height=%.9g base_eye_height=%.9g candidate=%.9g,%.9g,%.9g generation=%llu session=%llu sequence=%llu received_ns=%lld predicted_time=%lld sample_source=%u head_flags=%llu view_flags=%llu focused=%u should_render=%u units_per_metre=%.17g",
                phases[p],f.id,f.origin.selection.phase_qpc,row.mask,r.invocation,r.thread,r.scene_qpc,r.family_qpc,r.renderer_qpc,
                r.key.family,r.key.source,r.key.owner,r.key.local,a.pawn,a.controller,a.camera,a.level,r.key.stamp,
                a.location[0],a.location[1],a.location[2],a.eye_height,a.base_eye_height,a.location[0],a.location[1],a.location[2]+a.eye_height,
                f.reference_generation,sample.session,sample.sequence,sample.received_ns,sample.predicted_time,static_cast<unsigned>(sample.source),
                sample.head_flags,sample.view_flags,sample.focused,sample.should_render,stereo_units_per_metre);
            matrix("scene_view",r.key.matrices.data());matrix("family_view",r.family_matrices.data());
            matrix("original_view",r.renderer_matrices.data());matrix("original_projection",r.renderer_matrices.data()+64);
            matrix("base_view",f.origin.base.data());fprintf(status,"\n");
            const gears_vr::Pose* poses[]={&f.reference,&sample.head,&sample.eyes[0].pose,&sample.eyes[1].pose};
            const char* kinds[]={"reference","head","left","right"};
            for(unsigned k=0;k<4;++k){fprintf(status,"origin_pose ticket=%llu kind=%s",f.id,kinds[k]);print_stereo_pose(status,*poses[k]);
        if(k>=2){auto v=sample.eyes[k-2].fov;fprintf(status," fov=%.17g,%.17g,%.17g,%.17g",v.left,v.right,v.up,v.down);}fprintf(status,"\n");}
            for(unsigned eye=0;eye<2;++eye){fprintf(status,"origin_expected ticket=%llu eye=%u",f.id,eye);
                matrix("view",f.expected.view[eye].data());matrix("projection",f.expected.projection[eye].data());fprintf(status,"\n");}
            for(unsigned s=0;s<4;++s)if(row.mask&(1u<<s))for(unsigned eye=0;eye<2;++eye){auto const& observed=row.stages[s];
                fprintf(status,"origin_actual ticket=%llu stage=%s eye=%u qpc=%lld reads=%u matching=%u",f.id,stages[s],eye,observed.qpc,observed.reads,observed.matching?1:0);
                matrix("view",observed.actual.view[eye].data());matrix("projection",observed.actual.projection[eye].data());fprintf(status,"\n");}
        }
    }
}
void print_outage(FILE* status,bool hooks_disabled) {
    if(!outage_diagnostics.load()){fprintf(status,"outage_trace enabled=0\n");return;}
    // Scope covers the whole callback, including post-original observations.
    // Never read non-atomic Window storage while a callback might still own it.
    if(recording.load()||active.load()||!hooks_disabled){
        fprintf(status,"outage_trace enabled=1 available=0 unavailable_active_callbacks=%u hooks_disabled=%u\n",active.load(),hooks_disabled?1:0);return;}
    fprintf(status,"outage_sources enabled=1 threshold_ms=500 gap_capacity=4 recent_capacity=16 gaps=%u total_gaps=%llu overwritten=%llu original_present_draws_excluded=1 acceptance_unchanged=1 material_receipt_is_candidate_only=1 overlay_flags=queried:1,linked:2,plan:4,chain:8\n",
        outage_sources.count(),outage_sources.total(),outage_sources.total()-outage_sources.count());
    auto source_interval=[&](unsigned gap,const char* role,unsigned row,gears_outage_sources::Interval const& interval){
        fprintf(status,"outage_source_interval gap=%u role=%s row=%u qpc=%lld end_qpc=%lld target=%llx ticket=%llu reject=%u published=%u published_qpc=%lld left_ticket=%llu right_ticket=%llu unknowns=%llu\n",
            gap,role,row,interval.qpc,interval.end_qpc,static_cast<unsigned long long>(interval.target),interval.ticket,interval.rejection,interval.published?1:0,
            interval.published_qpc,interval.eye_ticket[0],interval.eye_ticket[1],interval.unknowns);
        auto draw=[&](const char* kind,gears_outage_sources::Draw const& d){if(!d.qpc)return;
            fprintf(status,"outage_source_draw gap=%u role=%s row=%u selected=%s qpc=%lld list=%llx generation=%llu target=%llx caller=%llx draw_kind=%u elements=%u instances=%u size=%u,%u material_sequence=%llu material_invocation=%llu material_payload=%llx material_command=%llx material_proxy=%llx material_kind=%u overlay_flags=%u overlay_reject=%u overlay_ticket=%llu\n",
                gap,role,row,kind,d.qpc,static_cast<unsigned long long>(d.list),d.generation,static_cast<unsigned long long>(d.target),static_cast<unsigned long long>(d.caller),d.kind,d.elements,d.instances,d.width,d.height,
                d.material_sequence,d.material_invocation,static_cast<unsigned long long>(d.material_payload),static_cast<unsigned long long>(d.material_command),static_cast<unsigned long long>(d.material_proxy),d.material_kind,
                d.overlay.flags,d.overlay.rejection,d.overlay.ticket);};
        draw("first",interval.first);draw("material",interval.material);draw("overlay",interval.overlay);
    };
    for(unsigned i=0;i<outage_sources.count();++i){const auto& gap=outage_sources.at(i);
        fprintf(status,"outage_source_gap gap=%u previous_publish_qpc=%lld trigger_qpc=%lld recovered=%u observed_intervals=%llu recent_count=%u\n",
            i,gap.previous_publish,gap.trigger_qpc,gap.recovered?1:0,gap.intervals,gap.recent_count);
        if(gap.first_unknown.unknowns)source_interval(i,"first_unknown",0,gap.first_unknown);
        source_interval(i,"trigger",0,gap.trigger);
        for(unsigned row=0;row<gap.recent_count;++row)source_interval(i,"recent",row,gap.recent[row]);
        source_interval(i,"last",0,gap.last);
    }
    if(outage_sources.has_longest()){const auto& gap=outage_sources.longest();
        fprintf(status,"outage_source_longest gap=4 previous_publish_qpc=%lld trigger_qpc=%lld recovered=%u observed_intervals=%llu recent_count=%u\n",
            gap.previous_publish,gap.trigger_qpc,gap.recovered?1:0,gap.intervals,gap.recent_count);
        if(gap.first_unknown.unknowns)source_interval(4,"first_unknown",0,gap.first_unknown);
        source_interval(4,"trigger",0,gap.trigger);
        for(unsigned row=0;row<gap.recent_count;++row)source_interval(4,"recent",row,gap.recent[row]);
        source_interval(4,"last",0,gap.last);
    }
    fprintf(status,"outage_trace enabled=1 available=1 qpc_frequency=%lld threshold_ms=100 capacity=256 rows=%u triggered=%u finished=%u trigger_mask=%u end=%u first_window_only=1\n",
        outage_window.frequency(),outage_window.count(),outage_window.triggered()?1:0,outage_window.finished()?1:0,outage_window.trigger(),outage_window.end());
    fprintf(status,"outage_schema validbits=association:1,built_pre:2,built_post:4,left_draw:8,right_draw:16,capture_called:32,image_sample:64,publication:128,present_return:256 capture_validbits=state:1,serial:2,consumer:4,ready:8,done:16,hresult:32 draw_scope=pre_original_current_interval built_count=cumulative trigger2=last_publication_to_original_end_age\n");
    for(unsigned i=0;i<outage_window.count();++i){auto const& a=outage_window.at(i);
        fprintf(status,"outage_row index=%u valid=%u enter_qpc=%lld original_begin_qpc=%lld original_end_qpc=%lld present_result=%ld previous_publish_qpc=%lld published_qpc=%lld published=%u pub_before=%llu pub_after=%llu buffer=%p queue=%p ticket=%llu reject=%u linked=%u contaminated=%u image_session=%llu image_sequence=%llu image_received_ns=%lld capture_reason=%s capture_valid=%u capture_state=%u capture_result=%ld consumer=%u serial_before=%llu serial_after=%llu ready_completed=%llu done_completed=%llu",
            i,a.valid,a.enter_qpc,a.original_begin_qpc,a.original_end_qpc,static_cast<long>(a.present_result),a.previous_publish_qpc,a.published_qpc,a.published?1:0,a.pub_before,a.pub_after,reinterpret_cast<void*>(a.buffer),reinterpret_cast<void*>(a.queue),a.ticket,a.rejection,a.linked?1:0,a.contaminated?1:0,a.image_session,a.image_sequence,a.image_received_ns,
            gears_vr::gpu_capture_reason_name(static_cast<gears_vr::GpuCaptureReason>(a.capture_reason)),a.capture_valid,a.capture_state,static_cast<long>(a.capture_result),a.consumer,a.serial_before,a.serial_after,a.ready_completed,a.done_completed);
        auto activity=[&](const char* name,gears_outage::Activity const& b){fprintf(status," %s_ticket=%llu %s_count=%llu %s_sequence=%llu %s_qpc=%lld %s_received_ns=%lld",name,b.ticket,name,b.count,name,b.sequence,name,b.qpc,name,b.received_ns);};
        activity("built_pre",a.built_pre);activity("built_post",a.built_post);activity("left_draw",a.draw[0]);activity("right_draw",a.draw[1]);fprintf(status,"\n");
    }
}

void print_frame_link(FILE* status) {
    if(!frame_link_diagnostics)return;
    std::lock_guard<FrameHookMutex> diagnostic_guard(frame_hook_mutex);
    print_late_quad(status);
    print_unpossessed_quads(status);
    death_source_live::print(status);
    death_hdr_timeline::print(status);
    death_hdr_pixels::print(status);
    death_constant_observer::print(status);
    dlss_route_observer::print(status);
    ngx_route_observer::print(status);
    ngx_mv_descriptor_capture::print(status);
    ngx_mv_target_capture::print(status);
    ngx_stereo_host::print(status);
    ngx_output_pixels::print(status);
    owned_source_pixels::print(status);
    death_dof_control::print(status);
    vr_dof_compat::print(status);
    dlss_output_control::print(status);
    death_pso_cache::print(status);
    print_backbuffer_copies(status);
    final_transfer_live::print(status);
    fprintf(status,"final_eye_failure_inventory rows=%u limit=%u per_epoch_limit=%u missing_ticket_per_epoch_limit=1 missing_ticket_global_limit=16 dropped=%llu resource_desc_source=tracked_rtv present_source=existing_inventory stage_bits=quad:1,draw:2 reason_bits=eye_invalid:1,ticket_missing:2,targets_not_one:4,mip:8,width:16,height:32,descriptor_capacity:64,no_targets:128,no_resource:256,not_known_buffer:512,verified_rect:1024\n",
        final_eye_failures.size,gears_final_eye_diagnostics::Recorder::capacity,gears_final_eye_diagnostics::Recorder::per_epoch,final_eye_failures.dropped);
    for(unsigned i=0;i<final_eye_failures.size;++i){const auto& r=final_eye_failures.rows[i];const auto& g=r.geometry;
        fprintf(status,"final_eye_failure row=%u world=%llu resize_generation=%llu inventory_epoch=%llu chain=%llx stage=%u reason=%u count=%llu first_qpc=%lld last_qpc=%lld quad_caller=%llx ticket=%llu last_ticket=%llu input_rect=%.9g,%.9g,%.9g,%.9g quad_target=%u,%u draw_caller=%llx list=%llx list_generation=%llu rtv_resource=%llx rtv_size=%llu,%u targets=%u mip=%u format=%u eye_valid=%u eye=%u eye_size=%u,%u eye_rect=%u,%u,%u,%u present_size=%u,%u present_count=%u present_valid=%u inventory_qpc=%lld\n",
            i,r.epoch.world,r.epoch.resize,r.epoch.inventory,static_cast<unsigned long long>(r.epoch.chain),g.stage,g.reason,r.count,r.first_qpc,r.last_qpc,
            static_cast<unsigned long long>(g.quad.caller),g.quad.ticket,r.last_ticket,g.quad.rect[0],g.quad.rect[1],g.quad.rect[2],g.quad.rect[3],g.quad.width,g.quad.height,
            static_cast<unsigned long long>(g.draw_caller),static_cast<unsigned long long>(r.list),r.list_generation,static_cast<unsigned long long>(g.resource),g.target_width,g.target_height,
            g.targets,g.mip,g.format,g.eye_valid?1u:0u,g.eye,g.eye_width,g.eye_height,g.eye_rect[0],g.eye_rect[1],g.eye_rect[2],g.eye_rect[3],
            r.present_width,r.present_height,r.present_count,r.present_valid?1u:0u,r.inventory_qpc);
        const auto& p=g.provenance;const auto& t=p.trace;
        fprintf(status,"final_eye_descriptor row=%u bind_handle=%llx write_serial=%llu origin_create_serial=%llu create_epoch=%llu last_write_epoch=%llu copy_source_handle=%llx source_write_serial=%llu creation_live=%llx descriptor_source=om_bind_snapshot desc_source=tracked_create_not_fresh_query descriptor_epoch_source=cached_present_inventory_may_lag_resize coalescing=create_origin_not_copy_serial reason512_means=not_current_known_buffer present_resources=",
            i,static_cast<unsigned long long>(p.handle),t.write_serial,t.origin_create_serial,t.create_epoch,t.last_write_epoch,
            static_cast<unsigned long long>(t.copy_source_handle),t.source_write_serial,static_cast<unsigned long long>(t.creation_live));
        for(unsigned j=0;j<r.present_resources.size();++j)fprintf(status,"%s%llx",j?",":"",static_cast<unsigned long long>(r.present_resources[j]));
        fprintf(status,"\n");}
    if(stereo_gpu_publish)fprintf(status,"stereo_gpu_publish enabled=1 state=%u published=%u busy=%u rate_limited=%u bind_requests=%llu prepare_ok=%llu prepare_failed=%llu ticket_retains=%llu ticket_duplicates=%llu ticket_rejected=%llu ticket_missing=%llu rect_rejected=%llu capture_attempts=%llu capture_skipped=%llu stopped=%u drain_ok=%u close_ok=%u retired_id=%llu pending_tickets=%zu last_ticket=%llu simulator_operator=%d\n",
        stereo_publisher.state.load(),stereo_publisher.frames.load(),stereo_publisher.busy.load(),stereo_publisher.rate_limited.load(),stereo_publish_counts.bind_requests.load(),
        stereo_publish_counts.prepare_ok.load(),stereo_publish_counts.prepare_failed.load(),stereo_publish_counts.ticket_retains.load(),
        stereo_publish_counts.ticket_duplicates.load(),stereo_publish_counts.ticket_rejected.load(),stereo_publish_counts.ticket_missing.load(),
        stereo_publish_counts.rect_rejected.load(),stereo_publish_counts.capture_attempts.load(),stereo_publish_counts.capture_skipped.load(),
        stereo_publish_counts.stopped.load(),stereo_publish_counts.drain_ok.load(),stereo_publish_counts.close_ok.load(),
        stereo_ticket_store.retired_id(),stereo_ticket_store.size(),stereo_publish_counts.last_ticket.load(),simulator_operator);
    auto bindings=swapchain_bindings.snapshot();
#if defined(GEARS_DESKTOP_MIRROR) && GEARS_DESKTOP_MIRROR
    const auto mirror=desktop_mirror.status();
    fprintf(status,"desktop_mirror enabled=1 state=%u captures=%llu presents=%llu busy=%llu stale=%llu rejected=%llu inventory_rejected=%llu present1_full=%llu ticket=%llu received_ns=%lld wait_ns=%llu max_wait_ns=%llu timeouts=%llu error=%lx unsafe=%u synchronous_present=%u pending_draws=%u destination_format=%u\n",
        mirror.state,mirror.captures,mirror.presents,mirror.busy,mirror.stale,mirror.rejected,mirror_inventory_rejected.load(),mirror_present1_full.load(),
        mirror.ticket,mirror.received_ns,mirror.wait_ns,mirror.max_wait_ns,mirror.timeouts,static_cast<unsigned long>(mirror.error),mirror.unsafe?1:0,gears_vr::DesktopMirror::asynchronous?0:1,mirror.pending_draws,mirror.destination_format);
    desktop_mirror_readback::print(status);
#endif
    auto overlay_stats=frame_link.snapshot();
    fprintf(status,"frame_link_unknown_coalesced count=%llu adjacent_same_target_only=1 unknown_rejection_preserved=1\n",overlay_stats.unknown_coalesced);
    fprintf(status,"frame_link_overlay recorded=%llu executed=%llu rejected=%llu single_event_pair=1 coordinates_live_verified=0\n",
        overlay_stats.overlay_recorded,overlay_stats.overlay_executed,overlay_stats.overlay_rejected);
    fprintf(status,"frame_link_overlay_source local_queries=%llu executed_queries=%llu executed_recorded=%llu executed_submitted=%llu no_executed=%llu invalid_list=%llu window_rejected=%llu capacity=%llu polluted=%llu ticket=%llu target=%llu incompatible_list=%llu\n",
        overlay_stats.overlay_local_queries,overlay_stats.overlay_executed_queries,overlay_stats.overlay_from_executed_recorded,
        overlay_stats.overlay_from_executed_executed,overlay_stats.overlay_no_executed_queries,overlay_stats.overlay_invalid_list_queries,
        overlay_stats.overlay_window_rejected_queries,overlay_stats.overlay_capacity_queries,overlay_stats.overlay_polluted_queries,
        overlay_stats.overlay_ticket_queries,overlay_stats.overlay_target_queries,overlay_stats.overlay_incompatible_queries);
    for(size_t i=0;i<overlay_stats.overlay_no_executed_snapshot_count;++i){auto const& row=overlay_stats.overlay_no_executed_snapshots[i];
        fprintf(status,"frame_link_overlay_missing index=%zu list=%llx generation=%llu target=%llx size=%u,%u maximum_ticket=%llu retired_ticket=%llu executed_mask=%u executed_ticket=%llu executed_queue=%llx other_complete_recorded_lists=%u unique_list=%llx unique_generation=%llu unique_ticket=%llu\n",
            i,row.current.identity,row.current.generation,row.target.resource,row.target.width,row.target.height,row.maximum_ticket,row.retired_ticket,
            row.executed_mask,row.executed_ticket,row.executed_queue,row.other_complete_recorded_lists,
            row.unique_recorded_ref.identity,row.unique_recorded_ref.generation,row.unique_recorded_ticket);}
    fprintf(status,"frame_link enabled=1 resets=%llu reset_failed=%llu draws=%llu filtered=%llu unknown=%llu missing_target=%llu missing_ticket=%llu bad_quad=%llu executes=%llu execute_rejected=%llu presents=%llu linked=%llu contaminated=%llu present_failed=%llu tests=%llu unbound=%llu changed=%llu last_ticket=%llu gpu_published=%u pixels_proven=0 completion_proven=%u clear_copy_indirect_observed=0 resize_hooks_enabled=%u destroy_observed=0 fixed_size_only=%u\n",
        frame_counts.resets.load(),frame_counts.reset_failed.load(),frame_counts.draws.load(),frame_counts.filtered.load(),frame_counts.unknown.load(),
        frame_counts.missing_target.load(),frame_counts.missing_ticket.load(),frame_counts.bad_quad.load(),frame_counts.executes.load(),frame_counts.execute_rejected.load(),
        frame_counts.presents.load(),frame_counts.linked.load(),frame_counts.contaminated.load(),frame_counts.present_failed.load(),frame_counts.tests.load(),
        frame_counts.unbound.load(),frame_counts.changed.load(),frame_counts.last_ticket.load(),stereo_publisher.frames.load(),
        stereo_gpu_publish&&stereo_publisher.frames.load()&&stereo_publish_counts.drain_ok.load()?1:0,game_resize_hooks?1:0,game_resize_hooks?0:1);
    fprintf(status,"frame_link_binding accepted=%llu excluded=%llu conflicts=%llu overflow=%llu buffers=%llu changed=%llu anchors=%llu rejected=%u chain_queue_refs_held=1 backbuffer_refs_held=0\n",
        bindings.accepted,bindings.excluded,bindings.conflicts,bindings.overflow,bindings.buffers,bindings.changed,bindings.anchors,bindings.rejected?1:0);
    auto tracker=frame_link.snapshot();auto target_tracker=frame_target_stats.snapshot();
    fprintf(status,"frame_link_capacity list_limit=%zu list_highwater=%zu active_lists=%zu event_limit=%zu event_highwater=%zu list_overflow=%llu event_overflow=%llu target_overflow=%llu sticky=%u target_binding_overflow=%llu descriptor_evictions=%llu descriptor_capacity_bad=%u\n",
        gears_frame_link::Tracker::list_capacity,tracker.list_highwater,tracker.active_lists,gears_frame_link::Tracker::events_per_list,tracker.event_highwater,
        tracker.list_overflow,tracker.event_overflow,tracker.target_overflow,tracker.capacity_bad?1:0,target_tracker.overflow,target_tracker.descriptor_evictions,frame_target_stats.descriptor_capacity_lost()?1:0);
    fprintf(status,"target_binding_capacity list_limit=%zu active_lists=%zu list_highwater=%zu first_failed_list=%llx\n",
        gears_viewport::Tracker::list_capacity,target_tracker.active_lists,target_tracker.list_highwater,
        static_cast<unsigned long long>(target_tracker.first_failed_list));
    fprintf(status,"frame_link_execute_batches unknown=%llu duplicate=%llu invalid=%llu metadata=%llu\n",
        tracker.unknown_batches,tracker.duplicate_batches,tracker.invalid_batches,tracker.metadata_batches);
    const gears_swapchain_binding::Comparison details[]={swapchain_bindings.first_comparison(),swapchain_bindings.first_failure()};
    for(unsigned d=0;d<2;++d){const auto& detail=details[d];if(!detail.recorded)continue;
        fprintf(status,"frame_link_compare kind=%s anchor=%u chain=%llx reason_mask=%u expected_size=%u,%u actual_size=%u,%u expected_count=%u actual_count=%u reason_bits=invalid:1,dimensions:2,count:4,identity:8,missing_binding:16,rejected:32\n",
            d?"first_failure":"first",detail.anchor?1:0,static_cast<unsigned long long>(detail.chain),detail.reason,
            detail.expected_width,detail.expected_height,detail.actual_width,detail.actual_height,detail.expected_count,detail.actual_count);
        for(unsigned i=0;i<detail.expected.size()&&(i<detail.expected_count||i<detail.actual_count);++i)
            fprintf(status,"frame_link_compare_buffer kind=%s index=%u expected=%llx actual=%llx\n",d?"first_failure":"first",i,
                static_cast<unsigned long long>(detail.expected[i]),static_cast<unsigned long long>(detail.actual[i]));
    }
    if(!active.load()){
        fprintf(status,"frame_link_work_inventory sample_limit=%zu stack_limit=16 samples_overflow=%llu stacks=%u game_base=%llx kind_bits=unknown_draw:0,final_draw:1,unknown_execute:2 post_anchor_only=1 rectangles_observed_only=1\n",
            frame_work_samples.size(),frame_work_sample_overflow,frame_work_stacks,static_cast<unsigned long long>(base));
        for(unsigned p=0;p<4;++p)fprintf(status,"frame_link_work_partition partition=%u dropped=%llu stacks=%u layout=unknown_execute:8/1,final_draw:4/0,ticket_draw:32/9,no_ticket_draw:20/6\n",
            p,frame_work_partition_dropped[p],frame_work_partition_stacks[p]);
        for(unsigned i=0;i<frame_work_samples.size();++i){const auto& entry=frame_work_samples[i];if(!entry.calls)continue;
            fprintf(status,"frame_link_work sample=%u kind=%u calls=%llu first_qpc=%lld caller=%llx caller_rva=%llx first_list=%llx queue=%llx present_depth=%u inside_original_present=%u inside_consume=%u ticket_present=%u draw_kind=%u elements_or_batch_count=%u instances=%u targets=%u resource=%llx target_size=%llu,%u tracked_rects=%u viewport_count=%u viewport=%.9g,%.9g,%.9g,%.9g scissor_count=%u scissor=%.9g,%.9g,%.9g,%.9g stack_count=%u\n",
                i,entry.kind,entry.calls,entry.first_qpc,static_cast<unsigned long long>(entry.caller),static_cast<unsigned long long>(entry.caller-base),
                static_cast<unsigned long long>(entry.list),static_cast<unsigned long long>(entry.queue),entry.present_depth,entry.inside_present,
                entry.inside_consume,entry.ticket_present,entry.draw_kind,entry.elements,entry.instances,entry.targets,
                static_cast<unsigned long long>(entry.target.resource),entry.target.width,entry.target.height,entry.rectangles.tracked?1:0,
                entry.rectangles.viewport_count,entry.rectangles.viewport.x,entry.rectangles.viewport.y,entry.rectangles.viewport.width,entry.rectangles.viewport.height,
                entry.rectangles.scissor_count,entry.rectangles.scissor.x,entry.rectangles.scissor.y,entry.rectangles.scissor.width,entry.rectangles.scissor.height,entry.stack_count);
            for(unsigned s=0;s<entry.stack_count;++s)fprintf(status,"frame_link_work_stack sample=%u index=%u address=%llx\n",
                i,s,static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(entry.stack[s])));
        }
        if(frame_first_read_failure.stage)fprintf(status,"frame_link_read_failure stage=%u index=%u hr=%08lx stages=desc:1,count:2,getbuffer:3,canonical_identity_zero:4,swapchain3_qi:5,current_index:6\n",
            frame_first_read_failure.stage,frame_first_read_failure.index,static_cast<unsigned long>(frame_first_read_failure.result));
        fprintf(status,"frame_link_inventory unique_lists_lower_bound=%u list_identity_capacity=%zu list_overflow_observations=%llu queue_identity_capacity=%zu queue_overflow_observations=%llu\n",
            frame_unique_list_count,frame_unique_lists.size(),frame_unique_list_overflow,frame_queues.size(),frame_unique_queue_overflow);
        for(const auto& entry:frame_queues)if(entry.identity){fprintf(status,"frame_link_queue identity=%llx type=%u calls=%llu members=%llu unknown_members=%llu rejected=%llu post_anchor_calls=%llu post_anchor_unknown=%llu\n",
            static_cast<unsigned long long>(entry.identity),entry.type,entry.calls,entry.members,entry.unknown_members,entry.rejected,entry.post_anchor_calls,entry.post_anchor_unknown);
            for(unsigned i=0;i<entry.first_unknown_count;++i)fprintf(status,"frame_link_unknown_member queue=%llx sample=%u identity=%llx\n",
                static_cast<unsigned long long>(entry.identity),i,static_cast<unsigned long long>(entry.first_unknown[i]));}
    }
    const char* names[]={"missing_pair","metadata_lost","capacity","queue_conflict","ticket_conflict","target_conflict","duplicate_eye","polluted","unknown_list","duplicate_execute","bad_rectangle","stale_ticket"};
    for(unsigned i=0;i<frame_counts.rejection.size();++i)fprintf(status,"frame_link_rejection reason=%s count=%llu\n",names[i],frame_counts.rejection[i].load());
    for(const auto& entry:frame_buffers)if(entry.resource.load())fprintf(status,
        "frame_link_backbuffer resource=%llx draws=%llu left=%llu right=%llu unknown=%llu presents=%llu linked=%llu contaminated=%llu\n",
        static_cast<unsigned long long>(entry.resource.load()),entry.draws.load(),entry.left.load(),entry.right.load(),entry.unknown.load(),
        entry.presents.load(),entry.linked.load(),entry.contaminated.load());
}
int offset_at(double seconds) {
    if(seconds<3 || seconds>=15) return 0;
    return static_cast<int>(std::lround((1.5*65536.0/360.0)*std::sin((seconds-3)*6.283185307179586/4.0)));
}
void apply_wobble(Record& record,void* pc,Rot* rotation) {
    Rot raw{};
    if(!safe_copy(&raw,rotation,sizeof(raw))) return;
    record.source_yaw=raw.yaw;
    if(!wobble_enabled || !recording.load() || reinterpret_cast<uintptr_t>(pc)!=expected_pc ||
       reinterpret_cast<uintptr_t>(scene_local)!=expected_local || scene_rotation!=rotation || record.caller!=allowed_caller) return;
    unsigned state=wobble_state.load();
    if(state>=2) {record.test_state=state;return;}
    float stamp{};
    bool readable=camera_cache && safe_copy(&stamp,reinterpret_cast<void*>(camera_cache),4);
    bool advancing=readable && stamp_known && stamp!=observed_stamp;
    if(readable) {observed_stamp=stamp;stamp_known=true;}
    if(!self_testing && !record.focused) {
        if(state==1) wobble_state=3;
        record.test_state=wobble_state.load();return;
    }
    if(!self_testing) {
        uintptr_t linked{};
        if(!safe_copy(&linked,reinterpret_cast<void*>(expected_local+0x68),8) || linked!=expected_pc ||
           !safe_copy(&linked,reinterpret_cast<void*>(expected_pc+0x37c),8) || linked!=expected_camera) {
            wobble_state=3;record.test_state=3;return;
        }
    }
    long long origin=wobble_start.load();
    if(!origin && (self_testing || advancing)) {
        long long zero=0;
        wobble_start.compare_exchange_strong(zero,record.time);
        origin=wobble_start.load();wobble_state=1;
    }
    if(!origin) return;
    double elapsed=self_testing?4.0:static_cast<double>(record.time-origin)/qpc_frequency;
    if(elapsed>=15) {wobble_state=2;record.test_state=2;return;}
    int delta=offset_at(elapsed);
    if(delta) {
        unsigned bits=static_cast<unsigned>(raw.yaw)+static_cast<unsigned>(delta);
        int yaw{};memcpy(&yaw,&bits,4);
        SIZE_T written{};
        if(WriteProcessMemory(GetCurrentProcess(),reinterpret_cast<char*>(rotation)+4,&yaw,4,&written) && written==4) {
            record.yaw_delta=delta;record.valid|=64;scene_delta=delta;
        } else wobble_state=3;
    }
    record.test_state=wobble_state.load();
}
void apply_sweep(Record& record,void* pc,Vec* position,Rot* rotation) {
    Vec source_position{};Rot source_rotation{};
    if(!safe_copy(&source_position,position,sizeof(Vec)) || !safe_copy(&source_rotation,rotation,sizeof(Rot))) return;
    record.source_position=source_position;record.source_rotation=source_rotation;
    if(!sweep_enabled || !recording.load() || reinterpret_cast<uintptr_t>(pc)!=expected_pc ||
       reinterpret_cast<uintptr_t>(scene_local)!=expected_local || scene_rotation!=rotation || record.caller!=allowed_caller) return;
    unsigned state=wobble_state.load();
    if(state>=2) {record.test_state=state;record.pose_phase=state==2?7:0;scene_phase=record.pose_phase;return;}
    float stamp{};
    bool readable=camera_cache && safe_copy(&stamp,reinterpret_cast<void*>(camera_cache),4);
    bool advancing=readable && stamp_known && stamp!=observed_stamp;
    if(readable) {observed_stamp=stamp;stamp_known=true;}
    if(!self_testing && !record.focused) {
        if(state==1)wobble_state=3;
        record.test_state=wobble_state.load();return;
    }
    if(!self_testing) {
        uintptr_t linked{};
        if(!safe_copy(&linked,reinterpret_cast<void*>(expected_local+0x68),8) || linked!=expected_pc ||
           !safe_copy(&linked,reinterpret_cast<void*>(expected_pc+0x37c),8) || linked!=expected_camera) {
            wobble_state=3;record.test_state=3;return;
        }
    }
    long long origin=wobble_start.load();
    if(!origin && (self_testing || advancing)) {
        long long zero=0;wobble_start.compare_exchange_strong(zero,record.time);
        origin=wobble_start.load();wobble_state=1;
    }
    if(!origin)return;
    double elapsed=self_testing?selftest_elapsed:static_cast<double>(record.time-origin)/qpc_frequency;
    if(elapsed>=27) {wobble_state=2;record.test_state=2;record.pose_phase=7;scene_phase=7;return;}
    record.test_state=1;
    if(elapsed<3)return;
    unsigned phase=1+static_cast<unsigned>((elapsed-3)/4);
    double wave=std::sin((elapsed-3-(phase-1)*4)*6.283185307179586/4);
    int angle=static_cast<int>(std::lround(wave*1.5*65536.0/360.0));
    Vec delta{};Rot delta_rotation{};
    if(phase==1)delta_rotation.pitch=angle;
    if(phase==2)delta_rotation.yaw=angle;
    if(phase==3)delta_rotation.roll=angle;
    double yaw=source_rotation.yaw*6.283185307179586/65536.0;
    if(phase==4) {delta.x=static_cast<float>(2*wave*std::cos(yaw));delta.y=static_cast<float>(2*wave*std::sin(yaw));}
    if(phase==5) {delta.x=static_cast<float>(-2*wave*std::sin(yaw));delta.y=static_cast<float>(2*wave*std::cos(yaw));}
    if(phase==6)delta.z=static_cast<float>(2*wave);
    Vec output_position{source_position.x+delta.x,source_position.y+delta.y,source_position.z+delta.z};
    Rot output_rotation{source_rotation.pitch+delta_rotation.pitch,source_rotation.yaw+delta_rotation.yaw,source_rotation.roll+delta_rotation.roll};
    SIZE_T written{};
    if(!WriteProcessMemory(GetCurrentProcess(),position,&output_position,sizeof(Vec),&written) || written!=sizeof(Vec) ||
       !WriteProcessMemory(GetCurrentProcess(),rotation,&output_rotation,sizeof(Rot),&written) || written!=sizeof(Rot)) {
        wobble_state=3;record.test_state=3;return;
    }
    record.pose_phase=phase;record.translation_delta=delta;record.rotation_delta=delta_rotation;
    record.source_yaw=source_rotation.yaw;record.yaw_delta=delta_rotation.yaw;
    scene_phase=phase;scene_translation=delta;scene_rotation_delta=delta_rotation;scene_delta=delta_rotation.yaw;
    if(angle || delta.x || delta.y || delta.z)record.valid|=64;
}
Record start_record(unsigned kind,void* object,void* caller) {
    Record r{};r.kind=kind;r.thread=GetCurrentThreadId();r.time=now();r.focused=foreground();
    r.object=reinterpret_cast<uintptr_t>(object);r.caller=reinterpret_cast<uintptr_t>(caller)-base;
    return r;
}
void apply_live(Record& record,void* pc,Vec* position,Rot* rotation) {
    if(!live_enabled || !recording.load() || reinterpret_cast<uintptr_t>(pc)!=expected_pc ||
       reinterpret_cast<uintptr_t>(scene_local)!=expected_local || scene_rotation!=rotation || record.caller!=allowed_caller)return;
    if(!TryAcquireSRWLockExclusive(&live_lock))return;
    struct Unlock {~Unlock(){ReleaseSRWLockExclusive(&live_lock);}} unlock;
    Vec p{};Rot r{};
    if(!safe_copy(&p,position,sizeof(p)) || !safe_copy(&r,rotation,sizeof(r)))return;
    record.source_position=p;record.source_rotation=r;
    uintptr_t link{};
    bool linked=self_testing || (safe_copy(&link,reinterpret_cast<void*>(expected_local+0x68),8) && link==expected_pc &&
        safe_copy(&link,reinterpret_cast<void*>(expected_pc+0x37c),8) && link==expected_camera);
    if(GetAsyncKeyState(VK_F8)&0x8000)live_aborted=true;
    if(GetAsyncKeyState(VK_F9)&0x8000)live_gate.recenter();
    if(live_aborted)live_gate.arm(false);
    gears_vr::PoseSample sample{};live_bridge.read(sample);
    const double unit=6.283185307179586/65536;
    gears_vr::Viewpoint clean{{p.x,p.y,p.z},{r.pitch*unit,r.yaw*unit,r.roll*unit}};
    auto result=live_gate.update(clean,sample,gears_vr::monotonic_ns(),(record.focused || self_testing) && linked);
    record.pose_status=live_status=static_cast<unsigned>(result.status);
    record.pose_sequence=live_sequence=sample.sequence;
    if(result.status!=gears_vr::PoseStatus::applied)return;
    auto output=gears_vr::bounded_view(clean,result.view);
    Vec next_p{static_cast<float>(output.position.x),static_cast<float>(output.position.y),static_cast<float>(output.position.z)};
    Rot delta{gears_vr::to_rotator_units(output.rotation.pitch-clean.rotation.pitch),
        gears_vr::to_rotator_units(output.rotation.yaw-clean.rotation.yaw),gears_vr::to_rotator_units(output.rotation.roll-clean.rotation.roll)};
    auto add=[](int a,int b){unsigned bits=static_cast<unsigned>(a)+static_cast<unsigned>(b);int value;memcpy(&value,&bits,4);return value;};
    Rot next_r{add(r.pitch,delta.pitch),add(r.yaw,delta.yaw),add(r.roll,delta.roll)};
    SIZE_T written{};
    if(!WriteProcessMemory(GetCurrentProcess(),position,&next_p,sizeof(next_p),&written) || written!=sizeof(next_p) ||
       !WriteProcessMemory(GetCurrentProcess(),rotation,&next_r,sizeof(next_r),&written) || written!=sizeof(next_r)) {
        live_aborted=true;return;
    }
    scene_translation=record.translation_delta={next_p.x-p.x,next_p.y-p.y,next_p.z-p.z};
    scene_rotation_delta=record.rotation_delta=delta;
    record.source_yaw=r.yaw;record.yaw_delta=scene_delta=delta.yaw;
    record.pose_phase=scene_phase=8;
    record.valid|=64;
}
void enqueue(const Record& value) {
    if(!recording.load(std::memory_order_relaxed)) return;
    if(continuous_session)return; // Proof observers remain active; only per-frame CSV is suppressed.
    if(!TryAcquireSRWLockExclusive(&queue_lock)) {++dropped;return;}
    if(write_index-read_index<ring.size()) ring[write_index++%ring.size()]=value;
    else ++dropped;
    ReleaseSRWLockExclusive(&queue_lock);
}
void inspect_family(Record& record,void* family){
    record.family_address=reinterpret_cast<uintptr_t>(family);
    struct Header{uintptr_t data;int count,capacity;} header{};
    if(!safe_copy(&header,family,sizeof(header)))return;
    record.family_array=header.data;record.family_count=header.count;record.family_capacity=header.capacity;
    // Capacity is allocator slack, not the number of pointers we read. UE
    // arrays can reserve more than sixteen slots while holding one view.
    if(header.count<0 || header.count>4 || header.capacity<header.count || header.capacity>1024 || (header.count && (!header.data || header.data%8)))return;
    if(header.count && !safe_copy(record.family_views.data(),reinterpret_cast<void*>(header.data),size_t(header.count)*8))return;
    record.family_array=header.data;record.family_count=header.count;record.family_capacity=header.capacity;record.valid|=128;
}
void* __fastcall construct_from_snapshot(void* destination,void* source) {
    gears_source_args::Snapshot input{};
    if(!safe_copy(input.bytes.data(),source,input.bytes.size()))return nullptr;
    if(input.value<float>(0x50)<=0 || input.value<float>(0x54)<=0)return nullptr;
    if(constructor_eye_enabled) {
        gears_eye_args::Result shifted{};
        if(!gears_eye_args::shift_right(input,1.f,shifted))return nullptr;
        constructor_eye_shifted=1;
        memcpy(constructor_eye_matrices.data(),input.bytes.data()+0x70,128);
    }
    void* state=constructor_state_enabled?reinterpret_cast<void*>(state_temporary.load()):input.pointer(8);
    if(constructor_state_enabled && (!state || state_created.load()!=1))return nullptr;
    void* result=input.invoke(source_constructor,destination,input.pointer(0),state);
    gears_source_args::Snapshot output{};
    if(result==destination && safe_copy(output.bytes.data(),destination,output.bytes.size())) {
        constructor_derived_finite=1;
        if(constructor_eye_enabled) {
            constructor_combined_valid=1;
            for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col) {
                double expected=0;
                for(unsigned k=0;k<4;++k)expected+=double(input.value<float>(0x70+16*row+4*k))*input.value<float>(0xb0+16*k+4*col);
                float actual=output.value<float>(0x2b0+16*row+4*col);
                if(!std::isfinite(expected) || !std::isfinite(actual))constructor_combined_valid=0;
                else constructor_combined_relative_error=(std::max)(constructor_combined_relative_error,
                    std::fabs(double(actual)-expected)/(1.+std::fabs(expected)));
            }
        }
        // Known initialized matrix blocks; exclude the owned map at +0x140,
        // padding and fields whose initialization has not been established.
        for(size_t offset:{size_t(0x160),size_t(0x1a0),size_t(0x1e0),size_t(0x220),size_t(0x260),size_t(0x2b0),size_t(0x2f0)})
            for(unsigned i=0;i<16;++i) {
                float a=input.value<float>(offset+4*i),b=output.value<float>(offset+4*i);
                if(!std::isfinite(a) || !std::isfinite(b))constructor_derived_finite=0;
                else constructor_derived_error=(std::max)(constructor_derived_error,double(std::fabs(a-b)));
            }
    }
    return result;
}
void try_shadow_state(const Record& record) {
    if(!shadow_state_enabled || constructor_state_enabled || !recording.load() || record.caller!=0x2c8909 ||
       !(record.valid&128) || record.family_count!=1)return;
    unsigned expected=0;if(!state_attempted.compare_exchange_strong(expected,1))return;
    uintptr_t original_state{};
    if(!safe_copy(&original_state,reinterpret_cast<void*>(record.family_views[0]+8),8) || !original_state)return;
    void* temporary=state_factory();uintptr_t table{};
    state_temporary.store(reinterpret_cast<uintptr_t>(temporary));
    if(!temporary || reinterpret_cast<uintptr_t>(temporary)==original_state ||
       !safe_copy(&table,temporary,8) || table!=state_expected_table)return;
    state_created=1;
    // The ordinary virtual release wrapper queues resource release and deferred
    // deletion when required. Do not manually call the destructor/free here.
    state_release(temporary);state_release_returned=1;
    uintptr_t unchanged{};
    if(safe_copy(&unchanged,reinterpret_cast<void*>(record.family_views[0]+8),8) && unchanged==original_state)state_original_preserved=1;
}
void* __fastcall hook_state_delete(void* state,unsigned flags) {
    Scope scope;if(!scope.world())return original_state_delete(state,flags);
    unsigned expected=0;
    const bool observe=reinterpret_cast<uintptr_t>(state)==state_temporary.load() && state_created.load()==1 &&
        state_delete_entered.compare_exchange_strong(expected,1);
    if(observe){state_delete_flags=flags;state_delete_thread=GetCurrentThreadId();state_delete_caller=reinterpret_cast<uintptr_t>(_ReturnAddress())-base;}
    void* result=original_state_delete(state,flags);
    // The deleting wrapper has already released the storage. Only keep numeric
    // identities and completion metadata; never inspect this object afterward.
    if(observe)++state_destroyed;
    return result;
}
bool probe_renderer_pair(void* family,void* view) {
    unsubmitted_renderer=true;
    renderer_pair_result=gears_pair_shadow::run({shadow_api.allocate,original_renderer,original_destroy_renderer,safe_copy},
        family,view,pair_canvas,pair_transform);
    unsubmitted_renderer=false;
    return !renderer_pair_result.retain_inputs;
}
void run_source_shadow(const Record& record,void* family) {
    if(shadow_view_enabled && recording.load() && (record.valid&128) && record.family_count==1) {
        unsigned expected=0;
        if(shadow_attempted.compare_exchange_strong(expected,1)) {
            uintptr_t expected_state=0,original_state=0;
            bool ready=true;
            if(constructor_state_enabled) {
                state_attempted=1;
                ready=safe_copy(&original_state,reinterpret_cast<void*>(record.family_views[0]+8),8) && original_state;
                if(ready) {
                    void* state=state_factory();uintptr_t table{};
                    expected_state=reinterpret_cast<uintptr_t>(state);state_temporary=expected_state;
                    ready=state && expected_state!=original_state && safe_copy(&table,state,8) && table==state_expected_table;
                    if(ready)state_created=1;
                }
            }
            if(ready)shadow_result=gears_shadow::run(shadow_api,family,reinterpret_cast<void*>(record.family_views[0]),expected_state,
                constructor_eye_enabled?constructor_eye_matrices.data():nullptr,constructor_eye_enabled,
                renderer_pair_enabled?probe_renderer_pair:nullptr);
            // Only release the independent state after its view has been safely
            // destroyed, or when no view was allocated. A quarantined view
            // retains its state until process exit; never free beneath it.
            if(constructor_state_enabled && ready && (!shadow_result.temporary || (shadow_result.checks&32))) {
                state_release(reinterpret_cast<void*>(expected_state));state_release_returned=1;
                uintptr_t unchanged{};
                if(safe_copy(&unchanged,reinterpret_cast<void*>(record.family_views[0]+8),8) && unchanged==original_state)state_original_preserved=1;
            }
            shadow_attempted.store(2);
        }
    }
}
void anchor_note(unsigned stage,gears_anchor::Reason reason,gears_anchor::Receipt const& receipt={},uintptr_t renderer=0,unsigned long long ticket=0){
    ++anchor_reasons[stage][reason];
    if(reason!=gears_anchor::accepted){auto index=anchor_reject_count.fetch_add(1);
        if(index<anchor_rejects.size())anchor_rejects[index]={receipt,renderer,ticket,stage,reason};return;}
    if(stage!=2)return;
    auto qpc=receipt.renderer_qpc;long long zero=0;anchor_first_renderer.compare_exchange_strong(zero,qpc);anchor_last_renderer=qpc;
    auto next=anchor_next_sample.load();auto count=anchor_row_count.load();
    if(count>=4&&(qpc<next||!anchor_next_sample.compare_exchange_strong(next,qpc+qpc_frequency))){++anchor_sampled_out;return;}
    if(count<4)anchor_next_sample=qpc+qpc_frequency;
    auto index=anchor_row_count.fetch_add(1);
    if(index<anchor_rows.size())anchor_rows[index]={receipt,renderer,ticket,stage,reason};
    else {++anchor_capacity_dropped;zero=0;anchor_dropped_first.compare_exchange_strong(zero,qpc);anchor_dropped_last=qpc;}
}
bool anchor_key(gears_anchor::Key& key,gears_anchor::Anchor& anchor,uintptr_t family,uintptr_t source,uintptr_t local,unsigned stage){
    const auto guard_started=now();
    struct Timer {long long started;unsigned stage;~Timer(){auto duration=now()-started;++anchor_guard_calls[stage];
        auto value=anchor_guard_min[stage].load();while(duration<value&&!anchor_guard_min[stage].compare_exchange_weak(value,duration)){}
        value=anchor_guard_max[stage].load();while(duration>value&&!anchor_guard_max[stage].compare_exchange_weak(value,duration)){}
    }} timer{guard_started,stage};
    key.family=family;key.source=source;key.local=local;
    if(anchor_config.role_camera>=anchor_config.identity_count)return false;
    std::uint32_t before{};auto camera=anchor_config.identities[anchor_config.role_camera].address;
    if(!safe_copy(&before,reinterpret_cast<void*>(camera+anchor_config.camera_timestamp),4))return false;
    if(!family||!source||!safe_copy(&key.owner,reinterpret_cast<void*>(source),8)||
        !safe_copy(key.matrices.data(),reinterpret_cast<void*>(source+0x70),128))return false;
    for(unsigned i=0;i<32;++i){float value{};memcpy(&value,key.matrices.data()+i*4,4);if(!std::isfinite(value))return false;}
    const bool valid=gears_anchor::guard(anchor_config,safe_copy,anchor,key.stamp)&&before==key.stamp&&local==expected_local&&key.owner==family;
    if(valid&&stage<3&&anchor.unpossessed)++anchor_unpossessed_guards[stage];
    if(valid&&stage<3&&anchor.pawn!=anchor_config.identities[anchor_config.role_pawn].address)++anchor_pawn_reassociated_guards[stage];
    const auto observed_right_state=state_temporary.load();
    if(valid&&stage==2&&(!gears_anchor::equal_identity(anchor_last_state_pawn,anchor.pawn_identity)||
        observed_right_state!=anchor_last_observed_right_state)){
        anchor_last_state_pawn=anchor.pawn_identity;
        anchor_last_observed_right_state=observed_right_state;
        const auto index=anchor_state_transition_count.fetch_add(1);
        if(index<anchor_state_transitions.size()){
            AnchorStateTransition row{};row.pawn=anchor.pawn_identity;row.source=source;row.qpc=now();row.thread=GetCurrentThreadId();
            if(source<=UINTPTR_MAX-8&&safe_copy(&row.original_state,reinterpret_cast<void*>(source+8),8))row.read_mask|=1;
            row.right_state=observed_right_state;
            if(row.right_state&&safe_copy(&row.right_table,reinterpret_cast<void*>(row.right_state),8))row.read_mask|=2;
            anchor_state_transitions[index]=row;
        }
    }
    return valid;
}
void anchor_target_snapshot(void* canvas,void* family,gears_anchor::Receipt const& receipt) {
    unsigned index=anchor_target_snapshot_count.load();
    while(index<anchor_target_snapshots.size()&&!anchor_target_snapshot_count.compare_exchange_weak(index,index+1)){}
    if(index>=anchor_target_snapshots.size())return;
    auto& row=anchor_target_snapshots[index];row.family=reinterpret_cast<uintptr_t>(family);
    row.canvas=reinterpret_cast<uintptr_t>(canvas);row.source=receipt.key.source;
    row.qpc=receipt.family_qpc;row.thread=GetCurrentThreadId();
    if(row.family&&row.family<=UINTPTR_MAX-0x10&&safe_copy(&row.target,reinterpret_cast<void*>(row.family+0x10),8))row.read_mask|=1;
    if(row.target&&row.target<=UINTPTR_MAX-row.target_bytes.size()&&safe_copy(row.target_bytes.data(),reinterpret_cast<void*>(row.target),row.target_bytes.size()))row.read_mask|=2;
    if(safe_copy(&row.vtable,reinterpret_cast<void*>(row.target),8))row.read_mask|=4;
    if(row.vtable&&row.vtable<=UINTPTR_MAX-sizeof(row.methods)&&safe_copy(row.methods.data(),reinterpret_cast<void*>(row.vtable),sizeof(row.methods)))row.read_mask|=8;
    if(row.canvas&&row.canvas<=UINTPTR_MAX-row.canvas_bytes.size()&&safe_copy(row.canvas_bytes.data(),canvas,row.canvas_bytes.size()))row.read_mask|=16;
    if(row.read_mask&8)for(unsigned i=0;i<row.methods.size();++i){auto address=row.methods[i];
        if(address&&address<=UINTPTR_MAX-16&&safe_copy(row.prefixes[i].data(),reinterpret_cast<void*>(address),16))row.method_mask|=1u<<i;}
    if(row.family&&row.family<=UINTPTR_MAX-0x10&&safe_copy(&row.target_after,reinterpret_cast<void*>(row.family+0x10),8))row.read_mask|=32;
    if(safe_copy(&row.vtable_after,reinterpret_cast<void*>(row.target_after),8))row.read_mask|=64;
    uintptr_t copied_table{};memcpy(&copied_table,row.target_bytes.data(),8);
    if((row.read_mask&6)==6&&copied_table==row.vtable)row.read_mask|=128;
    row.consistent=(row.read_mask&239)==239&&row.target&&row.target==row.target_after&&row.vtable&&row.vtable==row.vtable_after;
}
void __fastcall hook_family(void* canvas,void* family){
    note_world_progress(WorldProgress::family);
    Scope scope;if(!scope.world()){note_world_progress(WorldProgress::family_scope_reject);return original_family(canvas,family);}
    service_session_stop_native();auto record=start_record(7,canvas,_ReturnAddress());
    inspect_family(record,family);
    std::optional<gears_anchor::Context::FamilyScope> anchor_scope;
    if(anchor_observer_enabled&&recording.load()){
        gears_anchor::Key key{};gears_anchor::Anchor anchor{};
        bool valid=anchor_key(key,anchor,reinterpret_cast<uintptr_t>(family),record.family_views[0],expected_local,1);
        anchor_scope.emplace(anchor_context,key,anchor,GetCurrentThreadId(),record.time,valid,record.family_count,record.caller);
        anchor_note(1,anchor_scope->reason,anchor_scope->receipt());
        if(anchor_scope->reason==gears_anchor::accepted)anchor_target_snapshot(canvas,family,anchor_scope->receipt());
    }
    if(record.caller==0x2c8909 && record.family_count==1)trace_rectangle(1,record.family_views[0]);
    try_shadow_state(record);
    if(!renderer_pair_enabled && !submit_pair_enabled && record.caller==0x2c8909)run_source_shadow(record,family);
    owned_ui_live::FamilyScope ui_family(canvas,family,anchor_scope&&anchor_scope->reason==gears_anchor::accepted);
    void* previous=submitting_family;submitting_family=family;
    original_family(canvas,family);
    submitting_family=previous;
    record.duration=now()-record.time;enqueue(record);
}
void* __fastcall hook_clone(void* destination,void* source){
    Scope scope;if(!scope.world())return original_clone(destination,source);auto record=start_record(8,destination,_ReturnAddress());
    bool observe=submitting_family!=nullptr && !unsubmitted_renderer;
    if(observe){
        inspect_family(record,submitting_family);
        record.clone_source=reinterpret_cast<uintptr_t>(source);
        if(source && safe_copy(record.source_matrices.data(),reinterpret_cast<char*>(source)+0x70,128))record.valid|=256;
    }
    void* result=original_clone(destination,source);
    if(observe){
        record.result=reinterpret_cast<uintptr_t>(result);
        if(destination && safe_copy(record.matrices.data(),reinterpret_cast<char*>(destination)+0x70,128))record.valid|=512;
        if(safe_copy(&record.clone_owner,destination,8))record.valid|=1024;
        record.duration=now()-record.time;enqueue(record);
    }
    return result;
}
#include "geometry_observer.h"

// One successful owned selection only; bootstrap renderers cannot claim this.
// Read-only bounded diagnostics, serialized by the existing frame hook lock.
namespace owned_ticket_size_trace {
#ifndef GEARS_OWNED_SIZE_TRACE
#define GEARS_OWNED_SIZE_TRACE 1
#endif
#if defined(GEARS_OWNED_SOURCE_PIXELS) && GEARS_OWNED_SOURCE_PIXELS && !GEARS_OWNED_SIZE_TRACE
#error Owned source pixels requires owned size trace selection
#endif
// The depth-clear control consumes this OM sidecar independently of the
// one-ticket source diagnostics. Keep its existing lifecycle available.
gears_owned_attachments::Tracker<> attachments;
#if GEARS_OWNED_SIZE_TRACE
#ifndef GEARS_SIZE_TRACE_MIN_WORLD
#define GEARS_SIZE_TRACE_MIN_WORLD 0
#endif
struct Row {
    bool present{};unsigned mask{},eye{},stage{},thread{};long long qpc{};
    uintptr_t view{},family{},target{},resource{};
    std::array<float,4> source{},sampling{};std::array<int,4> output{};
    uint64_t target_width{};unsigned target_height{},target_format{};
    std::array<unsigned,2> sampling_buffer{};
};
struct DrawRow {
    bool present{};unsigned eye{},stage{},mode{},indexed{},elements{},instances{},thread{};
    long long qpc{};uintptr_t view{},list{},caller{};
    gears_viewport::TargetDraw binding{};gears_viewport::DrawState state{};
};
// All direct draws in the selected owned consumer, including passes outside
// hook_geometry. The first RTV already has canonical IUnknown identity in this
// tracker; no draw-time COM/resource query or retained lease is needed.
struct DrawRangeRow {
    gears_viewport::Target target{};gears_viewport::DrawState state{};
    gears_owned_attachments::Snapshot attachments{};uint64_t last_bind_serial{};
    uintptr_t caller{},quad_caller{},first_list{},last_list{},first_view{},last_view{};
    uint64_t calls{},first_ordinal{},last_ordinal{};
    unsigned eye=2,stage=4,mode=2,indexed{},targets{},eye_sources{};bool descriptor_capacity_lost{};
};
uintptr_t active_quad_caller();
uint64_t ticket{},world{};uintptr_t renderer{},adapter{};bool finished{};
std::array<Row,8> rows{};std::array<DrawRow,16> draws{};
std::array<DrawRangeRow,128> draw_ranges{};unsigned draw_range_count{};
struct ProbeStack {uint64_t ordinal{};uintptr_t list{},view{};unsigned eye=2,count{};
    gears_owned_attachments::Snapshot attachments{};std::array<void*,12> frames{};};
std::array<ProbeStack,2> probe_stacks{};unsigned probe_stack_count{};
uint64_t direct_draw_count{},draw_range_overflow{},draw_unknown_eye{},draw_missing_target{};
uint64_t draw_overflow_first{},draw_overflow_last{};
bool same_draw_state(gears_viewport::DrawState const& a,gears_viewport::DrawState const& b){
    // Compare rectangle/depth bits, including signed zero and nonfinite values;
    // never compare struct padding or merge merely similar rectangles.
    auto equal=[](float x,float y){return !memcmp(&x,&y,sizeof(x));};
    auto rectangle=[&](gears_viewport::Rect const& x,gears_viewport::Rect const& y){
        return equal(x.x,y.x)&&equal(x.y,y.y)&&equal(x.width,y.width)&&equal(x.height,y.height);};
    return a.tracked==b.tracked&&a.viewport_count==b.viewport_count&&a.scissor_count==b.scissor_count&&
        a.depth_known==b.depth_known&&rectangle(a.viewport,b.viewport)&&rectangle(a.scissor,b.scissor)&&
        equal(a.min_depth,b.min_depth)&&equal(a.max_depth,b.max_depth);
}
bool matches(void* root,gears_submitted::FrameTicket const& frame){
    return ticket&&frame.enabled&&frame.id==ticket&&frame.world_generation==world&&
        reinterpret_cast<uintptr_t>(root)==renderer;
}
void views(unsigned stage,void* root,gears_submitted::FrameTicket const& frame,uintptr_t left,uintptr_t right){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(finished||stage>=4||!matches(root,frame))return;
    const uintptr_t addresses[]={left,right};
    for(unsigned eye=0;eye<2;++eye){auto& row=rows[stage*2+eye];if(row.present)continue;
        row.present=true;row.eye=eye;row.stage=stage;row.view=addresses[eye];row.thread=GetCurrentThreadId();row.qpc=now();
        if(!row.view)continue;
        // RIP decoded from 116590/1165bf; IDA's cs display label is 0x1000 lower.
        if(safe_copy(row.sampling_buffer.data(),reinterpret_cast<void*>(base+0x1c3b3b8),8))row.mask|=128;
        if(safe_copy(row.source.data(),reinterpret_cast<void*>(row.view+0x48),16))row.mask|=1;
        if(safe_copy(row.output.data(),reinterpret_cast<void*>(row.view+0x58),16))row.mask|=2;
        if(safe_copy(row.sampling.data(),reinterpret_cast<void*>(row.view+0x3d0),16))row.mask|=4;
        if(safe_copy(&row.family,reinterpret_cast<void*>(row.view),8)&&row.family)row.mask|=8;
        if((row.mask&8)&&safe_copy(&row.target,reinterpret_cast<void*>(row.family+0x10),8))row.mask|=16;
#if defined(GEARS_NATIVE_OWNED_SCENE_PROBE) && GEARS_NATIVE_OWNED_SCENE_PROBE
        // No untrusted target virtual calls. Only the retained owned resource,
        // under its existing API-order lock, can provide an actual GetDesc.
        if((row.mask&16)&&row.target==adapter&&
           reinterpret_cast<uintptr_t>(gears_owned_scene_live::owner.borrow_adapter())==adapter){
            row.mask|=32;const auto snap=gears_owned_scene_live::owner.snapshot();
            if(snap.phase==gears_native_owned::Phase::live&&snap.resource){
                row.resource=snap.resource;
                const auto desc=reinterpret_cast<ID3D12Resource*>(snap.resource)->GetDesc();
                row.target_width=desc.Width;row.target_height=desc.Height;row.target_format=unsigned(desc.Format);row.mask|=64;
            }
        }
#endif
    }
}
void select(void* root,gears_submitted::FrameTicket const& frame,uintptr_t target,uintptr_t left,uintptr_t right){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(ticket||!root||!target||!frame.enabled||!frame.id||!frame.world_generation)return;
    if(frame.world_generation<2||frame.world_generation<GEARS_SIZE_TRACE_MIN_WORLD)return;
    ticket=frame.id;world=frame.world_generation;renderer=reinterpret_cast<uintptr_t>(root);adapter=target;
    views(0,root,frame,left,right);
}
#else
void views(unsigned,void*,gears_submitted::FrameTicket const&,uintptr_t,uintptr_t){}
void select(void*,gears_submitted::FrameTicket const&,uintptr_t,uintptr_t,uintptr_t){}
#endif
void bind_attachments(ID3D12GraphicsCommandList* list,UINT count,D3D12_CPU_DESCRIPTOR_HANDLE const* handles,
    BOOL contiguous,UINT stride,D3D12_CPU_DESCRIPTOR_HANDLE const* depth){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
#if GEARS_OWNED_SIZE_TRACE && (!defined(GEARS_OWNED_DEPTH_CLEAR_TRIAL) || !GEARS_OWNED_DEPTH_CLEAR_TRIAL)
    if(finished)return;
#endif
    D3D12_CPU_DESCRIPTOR_HANDLE depth_value{};
    const bool depth_read=depth&&safe_copy(&depth_value,depth,sizeof(depth_value));
    const auto snapshot=gears_owned_attachments::capture(count,contiguous!=FALSE,stride,frame_target_stats.descriptor_capacity_lost(),
        [&](unsigned i,uintptr_t& out){D3D12_CPU_DESCRIPTOR_HANDLE value{};
            if(!handles||!safe_copy(&value,handles+i,sizeof(value)))return false;out=value.ptr;return true;},
        [&](uintptr_t handle){return frame_target_stats.lookup(handle);},depth!=nullptr,depth_read,depth_value.ptr);
    attachments.bind(reinterpret_cast<uintptr_t>(list),snapshot);
}
void retire_attachments(uintptr_t list){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);attachments.retire(list);
}
void clear_attachments(){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);attachments.clear();
}
#if GEARS_OWNED_SIZE_TRACE
void draw(ID3D12GraphicsCommandList* list,uintptr_t caller,bool indexed,unsigned elements,unsigned instances){
    if(!recording.load()||!elements||!instances||!consuming_owned_scene)return;
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(finished||!matches(consuming_renderer,consuming_frozen_frame))return;
    const auto ordinal=++direct_draw_count;
    const auto list_address=reinterpret_cast<uintptr_t>(list);
    const auto binding=frame_target_stats.binding(list_address);
    const auto state=frame_target_stats.draw_state(list_address);
    const auto bound_attachments=attachments.binding(list_address);
    const bool capacity_lost=frame_target_stats.descriptor_capacity_lost();
    const auto draw_caller=caller>=base?caller-base:caller;
    const auto quad_caller=active_quad_caller();
    const unsigned stage=current_geometry?current_geometry->stage:4,mode=current_geometry?current_geometry->mode:2;
    unsigned eye=2,eye_source=0;uintptr_t view{};
    if(current_geometry&&current_geometry->eye<2){
        eye=current_geometry->eye;view=current_geometry->view;eye_source=1;
    }else{
        // Same bounded current-view attribution as observe_engine_view_draw;
        // viewport position alone never establishes an engine eye.
        uintptr_t tls{},context{};struct Header{uintptr_t data;int count,capacity;} views{};
        if(safe_copy(&tls,reinterpret_cast<void*>(__readgsqword(0x58)),8)&&tls&&
           safe_copy(&context,reinterpret_cast<void*>(tls+0x20),8)&&context&&
           safe_copy(&view,reinterpret_cast<void*>(context+0x14c0),8)&&
           safe_copy(&views,reinterpret_cast<char*>(consuming_renderer)+0x68,sizeof(views))&&views.data&&
           views.count==2&&views.capacity>=2&&views.capacity<=1024&&view>=views.data&&
           (view-views.data)%0xd30==0&&(view-views.data)/0xd30<2){
            eye=unsigned((view-views.data)/0xd30);eye_source=2;
        }
    }
    if(eye==2)++draw_unknown_eye;
    if(!binding.target.resource)++draw_missing_target;
    if(probe_stack_count<probe_stacks.size()&&binding.targets==0&&draw_caller==0x1af345&&
       state.viewport_count==1&&state.viewport==gears_viewport::Rect{0,0,3840,2160}){
        auto& probe=probe_stacks[probe_stack_count++];probe.ordinal=ordinal;probe.list=list_address;
        probe.view=view;probe.eye=eye;probe.attachments=bound_attachments;
        probe.count=CaptureStackBackTrace(0,static_cast<DWORD>(probe.frames.size()),probe.frames.data(),nullptr);
    }
    DrawRangeRow* range{};
    for(unsigned i=0;i<draw_range_count;++i){auto& candidate=draw_ranges[i];
        if(candidate.target==binding.target&&candidate.targets==binding.targets&&candidate.eye==eye&&
           candidate.indexed==unsigned(indexed)&&candidate.caller==draw_caller&&
           candidate.quad_caller==quad_caller&&candidate.stage==stage&&candidate.mode==mode&&
           gears_owned_attachments::same(candidate.attachments,bound_attachments)&&
           candidate.descriptor_capacity_lost==capacity_lost&&same_draw_state(candidate.state,state)){
            range=&candidate;break;
        }
    }
    if(!range&&draw_range_count<draw_ranges.size()){
        range=&draw_ranges[draw_range_count++];range->target=binding.target;range->state=state;
        range->targets=binding.targets;range->eye=eye;range->indexed=indexed;range->caller=draw_caller;
        range->quad_caller=quad_caller;range->stage=stage;range->mode=mode;
        range->attachments=bound_attachments;
        range->descriptor_capacity_lost=capacity_lost;range->first_ordinal=ordinal;
        range->first_list=list_address;range->first_view=view;
    }
    if(range){++range->calls;range->last_ordinal=ordinal;range->last_list=list_address;
        range->last_view=view;range->eye_sources|=eye_source;range->last_bind_serial=bound_attachments.serial;}
    else {if(!draw_range_overflow)draw_overflow_first=ordinal;++draw_range_overflow;draw_overflow_last=ordinal;}
    // Preserve the existing first draw for each geometry stage/mode as a
    // separate diagnostic; it no longer bounds the all-pass observations.
    if(!current_geometry)return;
    auto const& geometry=*current_geometry;
    if(geometry.eye>=2||geometry.stage>=4||geometry.mode>=2)return;
    auto& row=draws[geometry.eye*8+geometry.stage*2+geometry.mode];if(row.present)return;
    row.present=true;row.eye=geometry.eye;row.stage=geometry.stage;row.mode=geometry.mode;
    row.indexed=indexed;row.elements=elements;row.instances=instances;row.thread=GetCurrentThreadId();row.qpc=now();
    row.view=geometry.view;row.list=reinterpret_cast<uintptr_t>(list);row.caller=caller>=base?caller-base:caller;
    row.binding=viewport_stats.binding(row.list);row.state=viewport_stats.draw_state(row.list);
}
void finish(void* root,gears_submitted::FrameTicket const& frame){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(matches(root,frame))finished=true;
}
void print(FILE* status){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);unsigned mask=0,draw_count=0;
    for(unsigned i=0;i<rows.size();++i)if(rows[i].present)mask|=1u<<i;
    for(auto const& row:draws)draw_count+=row.present?1:0;
    fprintf(status,"owned_ticket_size_trace ticket=%llu world=%llu renderer=%llx adapter=%llx rows_mask=%u finished=%u draw_rows=%u selection=successful_owned_only stages=source_constructed,renderer_built,prepare_before,prepare_after mask_bits=floatrect:1,intrect:2,sampling:4,family:8,target:16,owned_adapter_match:32,resource_desc:64 direct_draws_only=1 pixels_verified=0\n",
        ticket,world,static_cast<unsigned long long>(renderer),static_cast<unsigned long long>(adapter),mask,finished?1:0,draw_count);
    for(auto const& row:rows)if(row.present){fprintf(status,"owned_ticket_sampling_buffer ticket=%llu stage=%u eye=%u width=%u height=%u readable=%u min_world=%u\n",ticket,row.stage,row.eye,row.sampling_buffer[0],row.sampling_buffer[1],(row.mask&128)?1:0,unsigned(GEARS_SIZE_TRACE_MIN_WORLD));fprintf(status,
        "owned_ticket_size_row ticket=%llu world=%llu renderer=%llx stage=%u eye=%u qpc=%lld thread=%u mask=%u view=%llx family=%llx target=%llx source=%.9g,%.9g,%.9g,%.9g output=%d,%d,%d,%d sampling=%.9g,%.9g,%.9g,%.9g resource=%llx target_width=%llu target_height=%u target_format=%u\n",
        ticket,world,static_cast<unsigned long long>(renderer),row.stage,row.eye,row.qpc,row.thread,row.mask,
        static_cast<unsigned long long>(row.view),static_cast<unsigned long long>(row.family),static_cast<unsigned long long>(row.target),
        row.source[0],row.source[1],row.source[2],row.source[3],row.output[0],row.output[1],row.output[2],row.output[3],
        row.sampling[0],row.sampling[1],row.sampling[2],row.sampling[3],static_cast<unsigned long long>(row.resource),row.target_width,row.target_height,row.target_format);}
    for(auto const& row:draws)if(row.present){auto const& b=row.binding;auto const& s=row.state;fprintf(status,
        "owned_ticket_size_draw ticket=%llu world=%llu renderer=%llx eye=%u stage=%u mode=%u qpc=%lld thread=%u view=%llx list=%llx caller=%llx indexed=%u elements=%u instances=%u targets=%u resource=%llx width=%llu height=%u mip=%u format=%u tracked=%u viewport_count=%u scissor_count=%u viewport=%.9g,%.9g,%.9g,%.9g scissor=%.9g,%.9g,%.9g,%.9g\n",
        ticket,world,static_cast<unsigned long long>(renderer),row.eye,row.stage,row.mode,row.qpc,row.thread,
        static_cast<unsigned long long>(row.view),static_cast<unsigned long long>(row.list),static_cast<unsigned long long>(row.caller),row.indexed,row.elements,row.instances,
        b.targets,static_cast<unsigned long long>(b.target.resource),static_cast<unsigned long long>(b.target.width),b.target.height,b.target.mip,b.target.format,
        s.tracked?1:0,s.viewport_count,s.scissor_count,s.viewport.x,s.viewport.y,s.viewport.width,s.viewport.height,s.scissor.x,s.scissor.y,s.scissor.width,s.scissor.height);
    }
    fprintf(status,"owned_ticket_draw_range_summary ticket=%llu world=%llu renderer=%llx rows=%u limit=128 total=%llu overflow_calls=%llu overflow_first_ordinal=%llu overflow_last_ordinal=%llu unknown_eye_calls=%llu missing_target_calls=%llu finished=%u aggregation=exact_tuple ordinal=cpu_pre_original_nonempty_direct_draw primary_rtv_slot=0 attachment_slots=8 dsv_size_known=0 first_viewport_scissor_only=1 canonical_resource=1 eye_unknown=2 geometry_absent_stage=4 geometry_absent_mode=2 quad_absent=0 eye_source_bits=geometry:1,current_view:2 same_thread_consumer_only=1 observes_compute_copy_clear_indirect=0 pixels_verified=0\n",
        ticket,world,static_cast<unsigned long long>(renderer),draw_range_count,direct_draw_count,draw_range_overflow,
        draw_overflow_first,draw_overflow_last,draw_unknown_eye,draw_missing_target,finished?1:0);
    for(unsigned i=0;i<draw_range_count;++i){auto const& row=draw_ranges[i];auto const& t=row.target;auto const& s=row.state;
        fprintf(status,"owned_ticket_draw_range row=%u ticket=%llu world=%llu renderer=%llx calls=%llu first_ordinal=%llu last_ordinal=%llu eye=%u eye_sources=%u stage=%u mode=%u indexed=%u caller=%llx quad_caller=%llx first_list=%llx last_list=%llx first_view=%llx last_view=%llx targets=%u canonical=%llx width=%llu height=%u mip=%u format=%u descriptor_capacity_lost=%u tracked=%u viewport_count=%u scissor_count=%u viewport=%.9g,%.9g,%.9g,%.9g scissor=%.9g,%.9g,%.9g,%.9g depth_known=%u depth=%.9g,%.9g\n",
            i,ticket,world,static_cast<unsigned long long>(renderer),row.calls,row.first_ordinal,row.last_ordinal,row.eye,row.eye_sources,
            row.stage,row.mode,row.indexed,static_cast<unsigned long long>(row.caller),static_cast<unsigned long long>(row.quad_caller),static_cast<unsigned long long>(row.first_list),static_cast<unsigned long long>(row.last_list),
            static_cast<unsigned long long>(row.first_view),static_cast<unsigned long long>(row.last_view),row.targets,static_cast<unsigned long long>(t.resource),
            static_cast<unsigned long long>(t.width),t.height,t.mip,t.format,unsigned(row.descriptor_capacity_lost),unsigned(s.tracked),s.viewport_count,s.scissor_count,
            s.viewport.x,s.viewport.y,s.viewport.width,s.viewport.height,s.scissor.x,s.scissor.y,s.scissor.width,s.scissor.height,
            unsigned(s.depth_known),s.min_depth,s.max_depth);
        const auto& a=row.attachments;
        fprintf(status,"owned_ticket_draw_attachments row=%u ticket=%llu world=%llu observed=%u first_bind_serial=%llu last_bind_serial=%llu targets=%u count_valid=%u read_mask=%u known_mask=%u rtv_complete=%u descriptor_capacity_lost=%u dsv_present=%u dsv_read=%u dsv_handle=%llx dsv_size_known=0 binding_overflow=%llu\n",
            i,ticket,world,unsigned(a.observed),a.serial,row.last_bind_serial,a.count,unsigned(a.count_valid),a.read_mask,a.known_mask,
            unsigned(a.complete()),unsigned(a.descriptor_capacity_lost),unsigned(a.depth_present),unsigned(a.depth_read),
            static_cast<unsigned long long>(a.depth_handle),attachments.overflow());
        for(unsigned slot=0;slot<std::min(a.count,8u);++slot){auto const& attachment=a.targets[slot];
            fprintf(status,"owned_ticket_draw_attachment row=%u ticket=%llu world=%llu slot=%u handle=%llx readable=%u known=%u canonical=%llx width=%llu height=%u mip=%u format=%u\n",
                i,ticket,world,slot,static_cast<unsigned long long>(a.handles[slot]),(a.read_mask>>slot)&1u,(a.known_mask>>slot)&1u,
                static_cast<unsigned long long>(attachment.resource),static_cast<unsigned long long>(attachment.width),attachment.height,attachment.mip,attachment.format);
        }
    }
    for(unsigned i=0;i<probe_stack_count;++i){auto const& probe=probe_stacks[i];auto const& a=probe.attachments;
        fprintf(status,"owned_ticket_draw_probe_stack row=%u ticket=%llu world=%llu ordinal=%llu eye=%u list=%llx view=%llx caller=1af345 targets=0 viewport=0,0,3840,2160 bind_observed=%u bind_serial=%llu dsv_present=%u dsv_read=%u dsv_handle=%llx dsv_size_known=0 module_base=%llx stack_count=%u stack=",
            i,ticket,world,probe.ordinal,probe.eye,static_cast<unsigned long long>(probe.list),static_cast<unsigned long long>(probe.view),
            unsigned(a.observed),a.serial,unsigned(a.depth_present),unsigned(a.depth_read),static_cast<unsigned long long>(a.depth_handle),
            static_cast<unsigned long long>(base),probe.count);
        for(unsigned frame=0;frame<probe.count;++frame)fprintf(status,"%s%llx",frame?",":"",static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(probe.frames[frame])));
        fprintf(status,"\n");
    }
}
#else
void draw(ID3D12GraphicsCommandList*,uintptr_t,bool,unsigned,unsigned){}
void finish(void*,gears_submitted::FrameTicket const&){}
void print(FILE*){}
#endif
}

#include "resource_identity_observer.h"
#include "late_quad_observer.inl"

namespace owned_ticket_size_trace {
#if GEARS_OWNED_SIZE_TRACE
uintptr_t active_quad_caller(){return late_quad_candidate.active?late_quad_candidate.caller:0;}
// Ordered CPU pre-original draw samples, never aggregate across passes. The
// existing TLS candidate contains the arguments actually forwarded to this quad.
struct QuadDrawRow {
    gears_late_quad::Key key{};std::array<ResourceIdentity,4> sources{};
    uintptr_t list{},view{};unsigned eye=2,thread{};long long qpc{};
};
std::array<QuadDrawRow,64> quad_draws{};unsigned quad_draw_count{};
uint64_t quad_draw_dropped{};bool quad_draw_overflow{};
void quad_draw(ID3D12GraphicsCommandList* list,uintptr_t caller,unsigned kind,unsigned elements,unsigned instances){
    if(!late_quad_candidate.active||!recording.load()||!elements||!instances||!consuming_owned_scene)return;
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(finished||world<2||!matches(consuming_renderer,consuming_frozen_frame))return;
    if(quad_draw_count==quad_draws.size()){
        quad_draw_overflow=true;if(quad_draw_dropped!=UINT64_MAX)++quad_draw_dropped;return;
    }
    auto& row=quad_draws[quad_draw_count++];auto& key=row.key;
    row.list=reinterpret_cast<uintptr_t>(list);row.qpc=now();row.thread=GetCurrentThreadId();
    key.args=late_quad_candidate.args;key.dimensions=late_quad_candidate.dimensions;
    key.quad_caller=late_quad_candidate.caller;key.draw_caller=caller>=base?caller-base:caller;
    key.kind=kind;key.elements=elements;key.instances=instances;
    const auto binding=frame_target_stats.binding(row.list);key.target=binding.target;key.targets=binding.targets;
    key.rectangles=frame_target_stats.draw_state(row.list);
    key.known_buffer=swapchain_bindings.known_buffer(key.target.resource);
    key.descriptor_capacity_lost=frame_target_stats.descriptor_capacity_lost();
    uintptr_t tls{};unsigned char count{};
    if(safe_copy(&tls,reinterpret_cast<void*>(__readgsqword(0x58)),8)&&tls&&
       safe_copy(&key.context,reinterpret_cast<void*>(tls+0x20),8)&&key.context){
        key.read_mask|=1;
        if(safe_copy(&count,reinterpret_cast<void*>(key.context+0x65d),1)){
            key.read_mask|=2;key.slot_count=count;
            const auto slots=std::min(unsigned(count),4u);
            if(!slots||safe_copy(key.objects.data(),reinterpret_cast<void*>(key.context+0x768),slots*8))key.read_mask|=4;
        }
        if(safe_copy(&row.view,reinterpret_cast<void*>(key.context+0x14c0),8)){
            key.read_mask|=8;struct Header{uintptr_t data;int count,capacity;} views{};
            if(safe_copy(&views,reinterpret_cast<char*>(consuming_renderer)+0x68,sizeof(views))&&views.data&&
               views.count==2&&views.capacity>=2&&views.capacity<=1024&&row.view>=views.data&&
               (row.view-views.data)%0xd30==0&&(row.view-views.data)/0xd30<2){
                row.eye=unsigned((row.view-views.data)/0xd30);key.read_mask|=16;
            }
        }
    }
    // Only these newly appended rows query engine-owned resources. No COM lease
    // is retained; overflow performs no TLS/resource queries or additional reads.
    if((key.read_mask&7)==7)for(unsigned slot=0;slot<std::min(key.slot_count,4u);++slot)
        query_bound_resource(key.objects[slot],row.sources[slot]);
}
void print_quad_draws(FILE* file){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    fprintf(file,"owned_ticket_quad_summary ticket=%llu world=%llu renderer=%llx rows=%u limit=64 overflow=%u dropped=%llu finished=%u ordered=1 aggregation=0 source_limit=4 sampling=cpu_pre_original_forwarded_draw direct_same_thread_only=1 pixels_verified=0 read_bits=context:1,slot_count:2,first4_objects:4,current_view:8,current_eye:16\n",
        ticket,world,static_cast<unsigned long long>(renderer),quad_draw_count,quad_draw_overflow?1:0,quad_draw_dropped,finished?1:0);
    for(unsigned i=0;i<quad_draw_count;++i){const auto& row=quad_draws[i];const auto& k=row.key;const auto& s=k.rectangles;
        fprintf(file,"owned_ticket_quad row=%u ticket=%llu world=%llu renderer=%llx qpc=%lld thread=%u view=%llx eye=%u quad_caller=%llx draw_caller=%llx kind=%u elements=%u instances=%u args=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g dimensions=%u,%u,%u,%u context=%llx read_mask=%u slot_count=%u list=%llx resource=%llx width=%llu height=%u mip=%u format=%u targets=%u known_buffer=%u descriptor_capacity_lost=%u tracked=%u viewport_count=%u scissor_count=%u viewport=%.9g,%.9g,%.9g,%.9g scissor=%.9g,%.9g,%.9g,%.9g depth_known=%u depth=%.9g,%.9g\n",
            i,ticket,world,static_cast<unsigned long long>(renderer),row.qpc,row.thread,static_cast<unsigned long long>(row.view),row.eye,
            static_cast<unsigned long long>(k.quad_caller),static_cast<unsigned long long>(k.draw_caller),k.kind,k.elements,k.instances,
            k.args[0],k.args[1],k.args[2],k.args[3],k.args[4],k.args[5],k.args[6],k.args[7],k.dimensions[0],k.dimensions[1],k.dimensions[2],k.dimensions[3],
            static_cast<unsigned long long>(k.context),k.read_mask,k.slot_count,static_cast<unsigned long long>(row.list),static_cast<unsigned long long>(k.target.resource),
            static_cast<unsigned long long>(k.target.width),k.target.height,k.target.mip,k.target.format,k.targets,unsigned(k.known_buffer),unsigned(k.descriptor_capacity_lost),
            unsigned(s.tracked),s.viewport_count,s.scissor_count,s.viewport.x,s.viewport.y,s.viewport.width,s.viewport.height,s.scissor.x,s.scissor.y,s.scissor.width,s.scissor.height,
            unsigned(s.depth_known),s.min_depth,s.max_depth);
        for(unsigned slot=0;slot<std::min(k.slot_count,4u);++slot){const auto& source=row.sources[slot];const auto& d=source.desc;
            fprintf(file,"owned_ticket_quad_source row=%u ticket=%llu world=%llu slot=%u object=%llx native=%llx resource=%llx canonical=%llx hr=%08lx width=%llu height=%u dimension=%u depth=%u mips=%u format=%u samples=%u flags=%u wrapper=%llx state_read=%u engine_state=%u engine_flags=%u\n",
                i,ticket,world,slot,static_cast<unsigned long long>(k.objects[slot]),static_cast<unsigned long long>(source.native),
                static_cast<unsigned long long>(source.resource),static_cast<unsigned long long>(source.canonical),static_cast<unsigned long>(source.result),
                d.Width,d.Height,unsigned(d.Dimension),unsigned(d.DepthOrArraySize),unsigned(d.MipLevels),unsigned(d.Format),d.SampleDesc.Count,unsigned(d.Flags),
                static_cast<unsigned long long>(source.engine_wrapper),unsigned(source.engine_state_read),source.engine_state,unsigned(source.engine_flags));
        }
    }
}
#else
void quad_draw(ID3D12GraphicsCommandList*,uintptr_t,unsigned,unsigned,unsigned){}
void print_quad_draws(FILE*){}
#endif
}

#include "unpossessed_quad_observer.inl"
#include "death_hdr_timeline.inl"
#include "final_transfer_live.inl"
#include "backbuffer_copy_observer.inl"
#include "final_input_observer.h"
#include "quad_source_control.h"
#include "color_copy_observer.h"
#include "first_view_mask_control.h"
#include "death_source_control.inl"
#include "death_hdr_pixels.inl"
#include "death_constant_observer.inl"
#include "dlss_route_observer.inl"
#include "dlss_output_control.inl"
#include "ngx_route_observer.inl"
#include "ngx_output_pixels.inl"
#define GEARS_NGX_CAPTURE_CONTROL_WIRING 1
void inspect_completed_renderer(Record&,void*);
namespace ngx_capture_control {
#if defined(GEARS_NGX_STEREO_HOST_OBSERVER) && GEARS_NGX_STEREO_HOST_OBSERVER
void detached(std::shared_ptr<ngx_stereo_host::ControlOwner> const&);
#endif
bool copy(ID3D12GraphicsCommandList*,D3D12_TEXTURE_COPY_LOCATION const*,UINT,UINT,UINT,D3D12_TEXTURE_COPY_LOCATION const*,D3D12_BOX const*,uintptr_t,ngx_stereo_host::CopyFn);
void execute(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*,ngx_stereo_host::ExecuteFn);
}
#define GEARS_NGX_EPOCH_PAIR_WIRING 1
#include "ngx_epoch_pair_probe.inl"
#include "ngx_stereo_host_live.inl"
#include "ngx_mv_target_capture.inl"
#include "ngx_mv_compute_capture.inl"
#include "death_pso_cache.inl"
#include "ngx_control_capture.inl"
#include "ngx_output_pixels_native.inl"
#include "owned_source_pixels.inl"
#include "owned_depth_clear_trial.inl"
#include "death_dof_control.inl"
#include "vr_dof_compat.inl"

void inspect_completed_renderer(Record& record,void* destination){
    if(!destination)return;
    if(stereo_xr){gears_submitted::FrameTicket frozen{};
        if(submitted_lifetime.frame(reinterpret_cast<uintptr_t>(destination),frozen)&&frozen.enabled){
            record.stereo_ticket=frozen.id;record.stereo_source=static_cast<unsigned>(frozen.sample.source);
            record.stereo_head_flags=frozen.sample.head_flags;record.stereo_view_flags=frozen.sample.view_flags;
            record.stereo_reference_generation=frozen.reference_generation;}}
    inspect_family(record,reinterpret_cast<char*>(destination)+8);
    struct Header{uintptr_t data;int count,capacity;} storage{};
    if(safe_copy(&storage,reinterpret_cast<char*>(destination)+0x68,sizeof(storage))){
        record.renderer_storage=storage.data;record.renderer_count=storage.count;record.renderer_capacity=storage.capacity;
        if(storage.data && storage.data%8==0 && storage.count>0 && storage.count<=4 && storage.capacity>=storage.count && storage.capacity<=1024)record.valid|=4096;
    }
    uintptr_t view=record.family_views[0];
    if(view && safe_copy(record.matrices.data(),reinterpret_cast<void*>(view+0x70),128))record.valid|=512;
    if(safe_copy(&record.clone_owner,reinterpret_cast<void*>(view),8))record.valid|=1024;
}
void check_ngx_constructor_source(gears_submitted::FrameTicket const& frame){
#if defined(GEARS_NGX_INPUT_RECT_EXPERIMENT) && GEARS_NGX_INPUT_RECT_EXPERIMENT
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(frame.ngx_constructor_source.revision&&(!frame.enabled||frame.world_generation!=frame.ngx_constructor_source.world||
       !ngx_stereo_host::native_create_matches(frame.ngx_constructor_source))){
        ngx_epoch_pair::changed();request_session_stop(SessionStop::proof_failure);}
#else
    (void)frame;
#endif
}
void __fastcall hook_consume(void* renderer){
    NativeOwnedShutdownRhiScope owned_shutdown_scope;
    Scope scope;if(!scope.world())return original_consume(renderer);auto record=start_record(10,renderer,_ReturnAddress());
    inspect_completed_renderer(record,renderer);
    if(submit_pair_enabled && (submit_session_enabled()?submitted_lifetime.contains(reinterpret_cast<uintptr_t>(renderer)):
        reinterpret_cast<uintptr_t>(renderer)==submit_root.load() && !submit_destroyed.load())){++submit_consumed;record.test_state=77;}
    projection_observe(1,reinterpret_cast<uintptr_t>(renderer),record.family_views,record.family_count,record.time);
    stereo_observe(1,reinterpret_cast<uintptr_t>(renderer),record.family_views,record.family_count,record.time);
    void* saved=consuming_renderer;consuming_renderer=renderer;
    auto saved_ticket=consuming_frame_ticket;gears_submitted::FrameTicket consume_frame{};
    auto saved_frozen_frame=consuming_frozen_frame;
    consuming_frame_ticket=frame_link_diagnostics&&submitted_lifetime.frame(reinterpret_cast<uintptr_t>(renderer),consume_frame)?consume_frame.id:0;
    consuming_frozen_frame=consume_frame;check_ngx_constructor_source(consume_frame);
    ngx_mv_target_capture::ConsumeScope target_consume(consume_frame,renderer,saved!=nullptr);
    const auto dof_consume=vr_dof_compat::begin(consume_frame,renderer!=nullptr);
    dlss_output_control::ConsumeScope dlss_consume(consume_frame.world_generation,consume_frame.id,renderer&&consume_frame.enabled);
    observe_unpossessed_ticket();
    // Allocation probes run only on this RHI callback. The metadata probe
    // releases immediately; the scene probe retains an owner for a later family.
    if(!self_testing&&continuous_session&&recording.load()&&consume_frame.enabled&&
       !submitted_lifetime.stopping()&&(!native_surface_probe_tick(consume_frame.id)||!native_owned_probe_tick(consume_frame.id,renderer)||
       !native_owned_scene_tick(consume_frame.id)))
        request_session_stop(SessionStop::proof_failure);
    if(!native_owned_scene_begin(renderer,consume_frame.id,consume_frame.world_generation,saved!=nullptr))request_session_stop(SessionStop::proof_failure);
    const bool saved_owned_scene=consuming_owned_scene;
    consuming_owned_scene=native_owned_scene_consuming(renderer,consume_frame.id);
      bool saved_draw_scope=trial_draw_scope;trial_draw_scope=record.test_state==77;
    {gears_render_cost::Window cost(owned_scene_gpu::render_cost(consume_frame.id));original_consume(renderer);}
    target_consume.finish();
    owned_ticket_size_trace::finish(renderer,consume_frame);
    check_ngx_constructor_source(consume_frame);vr_dof_compat::finish(dof_consume);
    native_owned_scene_end(renderer,consume_frame.id); // Identity value only; original may have destroyed it.
    consuming_owned_scene=saved_owned_scene;
      trial_draw_scope=saved_draw_scope;
    consuming_renderer=saved;
    consuming_frame_ticket=saved_ticket;
    consuming_frozen_frame=saved_frozen_frame;
    // The original consumer can free renderer. Only our local record is used
    // after it returns; never dereference the game object here.
    record.duration=now()-record.time;enqueue(record);
}
void __fastcall hook_prepare(void* renderer){
    Scope scope;if(!scope.world())return original_prepare(renderer);bool observe=renderer && renderer==consuming_renderer;
    auto record=start_record(12,renderer,_ReturnAddress());
    if(observe){
        Record before{};inspect_completed_renderer(before,renderer);
        owned_ticket_size_trace::views(2,renderer,consuming_frozen_frame,before.family_views[0],before.family_views[1]);
        ngx_epoch_pair::prepare(1,renderer,before);
        projection_observe(2,reinterpret_cast<uintptr_t>(renderer),before.family_views,before.family_count,now());
        stereo_observe(2,reinterpret_cast<uintptr_t>(renderer),before.family_views,before.family_count,now());
        if(submit_pair_enabled && before.family_count==(submit_mono_control?1:2) && (submit_session_enabled()?submitted_lifetime.contains(reinterpret_cast<uintptr_t>(renderer)):
            reinterpret_cast<uintptr_t>(renderer)==submit_root.load())) {
            trace_rectangle(6,before.family_views[0]);trace_rectangle(7,before.family_views[1]);
        }
        record.clone_source=before.family_views[0];
        record.source_matrices=before.matrices;
        if(before.valid&512)record.valid|=256;
        if(safe_copy(&record.renderer_sources[0],reinterpret_cast<void*>(record.clone_source+8),8))record.valid|=8192;
    }
    if(observe)check_ngx_constructor_source(consuming_frozen_frame);
    original_prepare(renderer);
    if(observe){
        check_ngx_constructor_source(consuming_frozen_frame);
        inspect_completed_renderer(record,renderer);
        owned_ticket_size_trace::views(3,renderer,consuming_frozen_frame,record.family_views[0],record.family_views[1]);
        ngx_epoch_pair::prepare(2,renderer,record);
        if(submit_pair_enabled && (submit_session_enabled()?submitted_lifetime.contains(reinterpret_cast<uintptr_t>(renderer)):
            reinterpret_cast<uintptr_t>(renderer)==submit_root.load() && !submit_destroyed.load())) {
            uintptr_t right_state{};
            unsigned index=submit_mono_control?0:1;
            uintptr_t expected_state=submit_mono_control && !submit_mono_independent_state?submit_original_state.load():state_temporary.load();
            if(record.family_count==(submit_mono_control?1:2) && safe_copy(&right_state,reinterpret_cast<void*>(record.family_views[index]+8),8) && right_state==expected_state)++submit_prepared;
            record.test_state=77;
            trace_rectangle(8,record.family_views[0]);trace_rectangle(9,record.family_views[1]);
            projection_observe(3,reinterpret_cast<uintptr_t>(renderer),record.family_views,record.family_count,now());
            stereo_observe(3,reinterpret_cast<uintptr_t>(renderer),record.family_views,record.family_count,now());
        }
        if(safe_copy(&record.renderer_sources[1],reinterpret_cast<void*>(record.family_views[0]+8),8))record.valid|=16384;
        record.duration=now()-record.time;enqueue(record);
    }
}
void* __fastcall hook_destroy_renderer(void* renderer){
    Scope scope;if(!scope.world())return original_destroy_renderer(renderer);auto record=start_record(11,renderer,_ReturnAddress());
    std::unique_lock<std::recursive_mutex> session_lock(session_native_mutex,std::defer_lock);
    if(continuous_session)session_lock.lock();
    service_session_stop_native();
    inspect_completed_renderer(record,renderer);
    const bool submitted=submit_pair_enabled && (submit_session_enabled()?submitted_lifetime.contains(reinterpret_cast<uintptr_t>(renderer)):
        submit_attempted.load()==2 && reinterpret_cast<uintptr_t>(renderer)==submit_root.load() && !submit_destroyed.load());
    if(submitted)record.test_state=77;
    void* result=original_destroy_renderer(renderer);
    native_owned_scene_destroyed(renderer,record.stereo_ticket); // Frozen value; no dereference after destruction.
    if(submitted) {
        ++submit_destroyed;
        // The submitted renderer can use its state until destruction returns.
        // Release on this native thread, never on the collection worker.
        bool release=submit_session_enabled()?submitted_lifetime.retire(reinterpret_cast<uintptr_t>(renderer)):true;
        if(release && !submit_retained.load() && submit_sources_released && state_created.load()==1) {
            release_session_state_claim(true);
        }
    }
    record.result=reinterpret_cast<uintptr_t>(result);
    record.duration=now()-record.time;enqueue(record);return result;
}
bool try_submit_pair(void* destination,void* family,void* canvas,void* transform,Record& record,void*& returned,
    gears_anchor::Receipt const* anchor=nullptr) {
    native_owned_scene_pace(); // No session/frame locks or pair snapshot held here.
    note_world_progress(WorldProgress::try_pair);
    std::unique_lock<std::recursive_mutex> session_lock(session_native_mutex,std::defer_lock);
    if(continuous_session)session_lock.lock();
    if(continuous_session){
        if((GetAsyncKeyState(VK_F8)&0x8000))request_session_stop(SessionStop::f8);
        if(state_created.load()&&!foreground()&&!submit_background)request_session_stop(SessionStop::focus_lost);
        service_session_stop_native();
        if(!session_activation_gate.ready())return false;
    }
    if(submit_session_enabled()) {
        if(world_quiesce_requested.load()){note_world_progress(WorldProgress::quiesce);return false;}
        if(submitted_lifetime.is_closed()){note_world_progress(WorldProgress::closed);return false;}
        if(!continuous_session&&!submit_deadline && now()<submit_not_before){note_world_progress(WorldProgress::not_before);return false;}
        if(!(continuous_session?state_created.load()!=0:submit_deadline!=0) && !foreground() && !submit_background){note_world_progress(WorldProgress::foreground);return false;}
        if(submit_deadline && (now()>=submit_deadline || (!foreground() && !submit_background) || (GetAsyncKeyState(VK_F8)&0x8000))) {
            if(submitted_lifetime.close() && state_created.load() && !submit_retained.load()) {
                state_release(reinterpret_cast<void*>(state_temporary.load()));state_release_returned=1;
            }
            return false;
        }
    } else {
        unsigned expected=0;
        if(!submit_attempted.compare_exchange_strong(expected,1))return false;
    }
    // An observer rejection must not silently turn into an unbound stereo
    // candidate. Missing/changed receipts skip this candidate only: an engine
    // pause or a finite observation race can recover on a fresh invocation.
    auto reject_anchor_submission=[&](){
        ++anchor_submission_rejected;
        if(origin_trial_enabled){++origin_prepare_failed;request_session_stop(SessionStop::proof_failure);service_session_stop_native();}
        return false;
    };
    if(anchor_observer_enabled&&!anchor)return reject_anchor_submission();
    SessionBuildScope build_scope;
    alignas(8) std::array<unsigned char,0x60> original_family_bytes{},split_family{},after_family{};
    Record input{};inspect_family(input,family);
    gears_source_args::Snapshot original_source{},after_source{};
    if(!(input.valid&128) || input.family_count!=1 || !safe_copy(original_family_bytes.data(),family,0x60) ||
       !safe_copy(original_source.bytes.data(),reinterpret_cast<void*>(input.family_views[0]),0x430)){
        if(anchor_observer_enabled)return reject_anchor_submission();
        return false;}
    if(anchor_observer_enabled){
        // Bind to the bytes actually used below, after copying the source.
        // No independent view state or renderer has been allocated yet.
        if(!gears_anchor::submission_matches(anchor,reinterpret_cast<uintptr_t>(family),input.family_views[0],
            expected_local,GetCurrentThreadId(),original_source.bytes.data()+0x70,now()))return reject_anchor_submission();
        ++anchor_submission_matched;
    }
    uintptr_t original_state=reinterpret_cast<uintptr_t>(original_source.pointer(8));
    gears_submitted::FrameTicket frame{};gears_vr::Pose frame_reference{};
    frame.unpossessed=anchor&&anchor->anchor.unpossessed;
    frame.world_generation=committed_world_generation.load();
    if(anchor_observer_enabled&&anchor){
        auto evidence=anchor->crosshair;
        frame.crosshair=gears_crosshair_scene::take(evidence,anchor->key,anchor->invocation,GetCurrentThreadId());
    }
    frame.enabled=frame_link_diagnostics;
    if(stereo_xr){
        // Read once per attempted pair; both constructors and all later stages
        // use this frozen ticket, never a second live sample.
        std::lock_guard<std::mutex> lock(stereo_mutex);gears_vr::StereoSample sample{};
        poll_stereo_recenter();
        if(!stereo_bridge.read(sample)){++stereo_rejections[8];return false;}
#if defined(GEARS_RUNTIME_RENDER_SIZE) && GEARS_RUNTIME_RENDER_SIZE
        if(!gears_native_render_size::matches_session(sample.session)){++stereo_rejections[8];return false;}
#endif
        auto result=stereo_gate.update(sample,gears_vr::monotonic_ns(),simulator_operator);
        if(result.heading_deferred)++stereo_heading_deferred;
        if(!result){++stereo_rejections[static_cast<unsigned>(result.status)];
            if(continuous_session&&(result.status==gears_vr::StereoStatus::session_changed||result.status==gears_vr::StereoStatus::invalid)){
                request_session_stop(SessionStop::proof_failure);service_session_stop_native();}
            return false;}
        gears_vr::StereoMatrix view{},projection{};memcpy(view.data(),original_source.bytes.data()+0x70,64);
        memcpy(projection.data(),original_source.bytes.data()+0xb0,64);
        // The original camera defines Canvas screen placement. Retain its
        // copied matrices before any experimental origin adjustment.
        frame.source_view=view;frame.source_projection=projection;
        if(origin_trial_enabled){
            if(!gears_origin_trial::prepare(origin_trial,anchor,reinterpret_cast<uintptr_t>(family),input.family_views[0],
                expected_local,GetCurrentThreadId(),view,projection,now(),qpc_frequency,frame.origin)){
                ++origin_prepare_failed;request_session_stop(SessionStop::proof_failure);service_session_stop_native();return false;}
            view=frame.origin.base;
        }
        if(camera_closer_metres>0){
            const bool gameplay=anchor&&!anchor->anchor.unpossessed&&anchor_config.cinematic_enabled&&!anchor_config.mounted_enabled&&
                !anchor->anchor.cinematic&&anchor->anchor.target_identity.address==anchor->anchor.pawn;
            gears_vr::Vector pawn_eye{};
            if(gameplay)pawn_eye={anchor->anchor.location[0],anchor->anchor.location[1],
                double(anchor->anchor.location[2])+anchor->anchor.eye_height};
            gears_camera_closer::Result closer{};
            if(!gears_camera_closer::prepare(view,camera_closer_metres,stereo_units_per_metre,gameplay,pawn_eye,closer)){
                ++camera_closer_rejected;return false;}
            view=closer.view;
            if(closer.applied_units>0)++camera_closer_applied;else ++camera_closer_inactive;
        }
        if(!gears_vr::stereo_matrices(view,projection,result.reference,result.sample,stereo_units_per_metre,frame.expected,simulator_operator)){
            ++stereo_rejections[9];
            if(origin_trial_enabled){request_session_stop(SessionStop::proof_failure);service_session_stop_native();}
            return false;}
        frame.enabled=true;frame.sample=result.sample;frame_reference=result.reference;
        frame.reference_generation=result.reference_generation;frame.reference=result.reference;
    }
    submit_original_state=original_state;
    trace_rectangle(2,input.family_views[0]);
    memcpy(submit_rectangle.data(),original_source.bytes.data()+0x48,16);
    void* state=reinterpret_cast<void*>(state_temporary.load());
    if(!submit_session_enabled() || !state_created.load()) {
        state_attempted=1;
        state=state_factory();uintptr_t table{};state_temporary=reinterpret_cast<uintptr_t>(state);
        if(!state || state_temporary.load()==original_state || !safe_copy(&table,state,8) || table!=state_expected_table) {
            if(submit_session_enabled()){submit_retained=1;
                if(continuous_session){request_session_stop(SessionStop::proof_failure);service_session_stop_native();}
                else submitted_lifetime.close();}return false;
        }
        state_created=1;
        if(submit_pair_seconds&&!submit_deadline){submit_deadline=now()+static_cast<LONGLONG>(submit_pair_seconds)*qpc_frequency;
            if(stereo_gpu_publish)stereo_publish_deadline.store(submit_deadline);}
    }
    gears_split_sources::API api{shadow_api.allocate,source_constructor,shadow_api.destroy,shadow_api.free,safe_copy};
    // Freeze once before either constructor. Registration commits the first origin.
    gears_projection::Metadata projection{};projection.enabled=projection_control;projection.pair_qpc=now();
    if(projection_control){std::lock_guard<std::mutex> lock(projection_mutex);
        projection.phase=gears_projection::phase(projection.pair_qpc,projection_origin?projection_origin:projection.pair_qpc,qpc_frequency);}
    const auto split_begin_qpc=now();
    const auto owned_scene_target=(!self_testing&&continuous_session&&stereo_xr&&!projection_control&&
        !submit_mono_control&&!submit_duplicate_control)?native_owned_scene_candidate():0;
#if defined(GEARS_OWNED_UI_BATCH) && GEARS_OWNED_UI_BATCH
    if(!self_testing&&!owned_scene_target){
        std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        // One owned frame at a time. While its Canvas is pending, forward the
        // ordinary native scene instead of building an unbound stereo clone.
        // A retired world needs exactly one bootstrap renderer to reach RHI
        // recreation; keep the old ticket until that recreation actually runs.
        if(gears_owned_scene_live::trial.ticket&&!native_owned_scene_try_bootstrap()){
            native_owned_scene_pending_fallback(destination);return false;
        }
    }
#endif
    const gears_split_sources::FixedOutputBounds owned_bounds{gears_native_render_size::width(),gears_native_render_size::height()};
    const gears_split_sources::FixedOutputBounds* constructor_output=owned_scene_target?&owned_bounds:nullptr;
    const gears_split_sources::RenderInputBounds* constructor_input=nullptr;
#if defined(GEARS_NGX_STEREO_HOST_OBSERVER) && GEARS_NGX_STEREO_HOST_OBSERVER
    // The default path requires a real SDK owner. The private constructor-only
    // experiment changes local source inputs while preserving SDK passthrough.
    // A readable zero native handle preserves the non-SR path.
    ngx_stereo_host::NativeCreateSource native_source;
    gears_split_sources::FixedOutputBounds native_output;
    gears_split_sources::RenderInputBounds native_input;
    bool use_native_input=bool(ngx_stereo_host::current_control());
#if defined(GEARS_NGX_INPUT_RECT_EXPERIMENT) && GEARS_NGX_INPUT_RECT_EXPERIMENT
    {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        use_native_input=use_native_input||(!self_testing&&continuous_session&&recording.load()&&
            ngx_epoch_pair::enabled&&ngx_epoch_pair::probe.armed&&!ngx_epoch_pair::probe.finished);}
#endif
    if(stereo_xr&&!projection_control&&!submit_mono_control&&!submit_duplicate_control&&use_native_input){
        const auto native_source_status=ngx_stereo_host::native_create_state(frame.world_generation,native_source);
        if(native_source_status==ngx_stereo_host::NativeCreateAvailability::unavailable)return false;
        if(native_source_status==ngx_stereo_host::NativeCreateAvailability::ready){
            native_output=owned_scene_target?owned_bounds:
                gears_split_sources::FixedOutputBounds{native_source.fields[2],native_source.fields[3]};
            if(!native_source.split_input(native_output.width,native_output.height,native_input.width,native_input.height))return false;
            constructor_output=&native_output;constructor_input=&native_input;
        }
    }
#endif
    // Native source construction derives output bounds/UVs from family+0x10.
    // Use the same caller-owned family copy as the renderer, while retaining
    // original family/source authentication. This stack copy outlives clone release.
    split_family=original_family_bytes;
    if(owned_scene_target)memcpy(split_family.data()+0x10,&owned_scene_target,sizeof(owned_scene_target));
    auto sources=gears_split_sources::create(api,family,reinterpret_cast<void*>(input.family_views[0]),state,submit_output_bounds,submit_mono_control || submit_duplicate_control,projection_control,projection.phase,stereo_xr?&frame.expected:nullptr,
        constructor_output,constructor_input,owned_scene_target?split_family.data():family);
#if defined(GEARS_NGX_STEREO_HOST_OBSERVER) && GEARS_NGX_STEREO_HOST_OBSERVER
    if(constructor_input&&sources.checks==63){
        if(!ngx_stereo_host::native_create_matches(native_source)){
            // Reuse existing safe clone cleanup/rejection; never submit views
            // constructed across a Create/Release/world/parameter change.
            sources.checks=0;sources.rejection=15;
        }else frame.ngx_constructor_source=native_source;
    }
#endif
    if(projection_control)for(unsigned i=0;i<2;++i)memcpy(projection.expected[i].data(),sources.inputs[i].bytes.data()+0xb0,64);
    submit_source_checks=sources.checks;submit_rejection=sources.rejection;
    if(sources.input_evidence.field140){
        ++source_map_candidates;source_map_last_checks=sources.map_checks;
        auto maximum=source_map_max_nodes.load();
        while(maximum<sources.map_nodes&&!source_map_max_nodes.compare_exchange_weak(maximum,sources.map_nodes)){}
        if(sources.checks==63&&sources.map_checks==63)++source_map_accepted;else ++source_map_rejected;
    }
    if(sources.checks!=63) {
        unsigned empty=0;
        if(split_failure_phase.compare_exchange_strong(empty,1)){
            first_split_failure={sources.input_evidence,
                reinterpret_cast<uintptr_t>(original_source.pointer(0)),reinterpret_cast<uintptr_t>(original_source.pointer(8)),
                reinterpret_cast<uintptr_t>(original_source.pointer(0x140)),reinterpret_cast<uintptr_t>(original_source.pointer(0x418)),
                split_begin_qpc,now(),sources.rejection,sources.checks};
            split_failure_phase.store(2);
        }
        if(projection_control){std::lock_guard<std::mutex> lock(projection_mutex);++projection_rejected;}
        bool no_views=!sources.views[0] && !sources.views[1];
        bool cleaned=no_views || gears_split_sources::release(api,sources);
        if(submit_session_enabled()) {
            if(!cleaned)submit_retained=1;
            if(continuous_session){request_session_stop(SessionStop::proof_failure);service_session_stop_native();}
            else if(submitted_lifetime.close() && cleaned){state_release(state);state_release_returned=1;}
        } else if(cleaned){state_release(state);state_release_returned=1;}
        state_original_preserved=safe_copy(after_source.bytes.data(),reinterpret_cast<void*>(input.family_views[0]),0x430) && after_source.bytes==original_source.bytes;
        submit_attempted=3; // Publish a completed rejection, never claim submission.
        return false;
    }
    trace_rectangle(3,reinterpret_cast<uintptr_t>(sources.views[0]));trace_rectangle(4,reinterpret_cast<uintptr_t>(sources.views[1]));
    crosshair_live::freeze(frame,anchor);
    if(frame.enabled)frame.id=++stereo_ticket_counter;
    if(owned_scene_target){
        // Both constructor input snapshots retain the original integer output
        // rectangle (+58..64); only their float eye rectangles were replaced.
        // Freeze scalars now, while the authenticated original source is live.
        std::array<int,4> original_output{},left_output{},right_output{};uintptr_t original_target{};
        memcpy(original_output.data(),original_source.bytes.data()+0x58,sizeof(original_output));
        memcpy(left_output.data(),sources.inputs[0].bytes.data()+0x58,sizeof(left_output));
        memcpy(right_output.data(),sources.inputs[1].bytes.data()+0x58,sizeof(right_output));
        memcpy(&original_target,original_family_bytes.data()+0x10,sizeof(original_target));
        if(original_source.pointer(0)==family&&sources.inputs[0].pointer(0)==family&&sources.inputs[1].pointer(0)==family&&
           sources.input_evidence.source==input.family_views[0]&&sources.input_evidence.family==reinterpret_cast<uintptr_t>(family))
            frame.source_output=gears_source_output::freeze(frame.id,frame.world_generation,input.family_views[0],
                reinterpret_cast<uintptr_t>(family),original_target,original_output,left_output,right_output);
    }
    if(frame.crosshair){if(frame.id)frame.crosshair->ticket=frame.id;else frame.crosshair.reset();}
    const bool register_submission=submit_session_enabled();
    if(register_submission && !submitted_lifetime.begin(reinterpret_cast<uintptr_t>(destination),projection,frame)) {
        if(!gears_split_sources::release(api,sources))submit_retained=1;
        if(continuous_session){++session_cancelled_builds;
            if(!submitted_lifetime.stopping())request_session_stop(SessionStop::proof_failure);
            service_session_stop_native();}
        else ++submit_skipped;
        return false;
    }
    if(register_submission)crosshair_live::registered(frame);
    // Admission owns the ticket now. Even an impossible commit failure must
    // finish constructing this renderer so its normal retirement can drain it.
    if(origin_trial_enabled&&!origin_trial.commit(frame.origin.selection,true)){
        ++origin_commit_failed;request_session_stop(SessionStop::proof_failure);}
    if(owned_scene_target){
        if(!native_owned_scene_select(destination,frame.id,owned_scene_target))request_session_stop(SessionStop::proof_failure);
        else {
            owned_ui_live::selected(frame.id);
            owned_ticket_size_trace::select(destination,frame,owned_scene_target,
                reinterpret_cast<uintptr_t>(sources.views[0]),reinterpret_cast<uintptr_t>(sources.views[1]));
        }
    }
    if(stereo_xr&&frame.enabled){std::lock_guard<std::mutex> lock(stereo_mutex);auto timestamp=now();
        record_stereo_reference(frame,timestamp);
        if(!stereo_accepted){stereo_first_sample=frame.sample;stereo_reference=frame_reference;stereo_first_qpc=timestamp;stereo_first_ticket=frame.id;}
        ++stereo_accepted;stereo_last_sample=frame.sample;stereo_last_qpc=timestamp;stereo_last_ticket=frame.id;}
    if(projection_control){std::lock_guard<std::mutex> lock(projection_mutex);if(!projection_origin)projection_origin=projection.pair_qpc;}
    uintptr_t pointers[]={reinterpret_cast<uintptr_t>(sources.views[0]),reinterpret_cast<uintptr_t>(sources.views[1])};
      if(submit_mono_independent_state)pointers[0]=pointers[1];
    int count=submit_mono_control?1:2;
    gears_shadow::Header pair{reinterpret_cast<uintptr_t>(pointers),count,count};memcpy(split_family.data(),&pair,sizeof(pair));
    record.renderer_input_family=reinterpret_cast<uintptr_t>(split_family.data());
    record.renderer_sources={pointers[0],pointers[1],0,0};record.clone_source=pointers[0];
    memcpy(record.source_matrices.data(),sources.inputs[0].bytes.data()+0x70,128);
    submit_root=reinterpret_cast<uintptr_t>(destination);
    void* previous=submitting_family;submitting_family=split_family.data();
    // This constructor argument is loaded from FCanvas+0x18 by the family
    // caller. It is not the FCanvas pointer received by hook_family; an owned
    // Canvas target must not be substituted here based on the parameter name.
    returned=original_renderer(destination,split_family.data(),canvas,transform);
    submitting_family=previous;
    Record built{};inspect_completed_renderer(built,destination);
    owned_ticket_size_trace::views(1,destination,frame,built.family_views[0],built.family_views[1]);
    trace_rectangle(5,built.family_views[0]);
    bool verified=returned==destination && built.family_count==count && built.renderer_count==count &&
        built.family_views[0]==built.renderer_storage && (count==1 || built.family_views[1]==built.renderer_storage+0xd30);
    for(unsigned i=0;verified && i<static_cast<unsigned>(count);++i) {
        uintptr_t owner{},associated{};unsigned index{};std::array<unsigned char,128> matrix{};
        std::array<unsigned char,16> rectangle{};
        unsigned source_index=submit_mono_independent_state?1:i;
        verified=safe_copy(&owner,reinterpret_cast<void*>(built.family_views[i]),8) && owner==submit_root.load()+8 &&
            safe_copy(&associated,reinterpret_cast<void*>(built.family_views[i]+8),8) && associated==(source_index?state_temporary.load():original_state) &&
            safe_copy(&index,reinterpret_cast<void*>(built.family_views[i]+0xd24),4) && index==i &&
            safe_copy(matrix.data(),reinterpret_cast<void*>(built.family_views[i]+0x70),128) &&
            !memcmp(matrix.data(),sources.inputs[source_index].bytes.data()+0x70,128) &&
            safe_copy(rectangle.data(),reinterpret_cast<void*>(built.family_views[i]+0x48),16) &&
            !memcmp(rectangle.data(),sources.inputs[source_index].bytes.data()+0x48,16);
    }
    if(frame.ngx_constructor_source.revision&&!ngx_stereo_host::native_create_matches(frame.ngx_constructor_source))verified=false;
    if(owned_scene_target&&!native_owned_scene_built(destination,frame.id,verified))verified=false;
    ngx_epoch_pair::built(destination,frame,built,verified,sources.inputs[0].bytes.data()+0x48,sources.inputs[1].bytes.data()+0x48);
    if(!ngx_capture_control::constructed(destination,frame,built,verified))verified=false;
    projection_observe(0,reinterpret_cast<uintptr_t>(destination),built.family_views,built.family_count,now());
    stereo_observe(0,reinterpret_cast<uintptr_t>(destination),built.family_views,built.family_count,now());
    bool source_released=gears_split_sources::release(api,sources);
    bool original_preserved=safe_copy(after_family.data(),family,0x60) && after_family==original_family_bytes &&
        safe_copy(after_source.bytes.data(),reinterpret_cast<void*>(input.family_views[0]),0x430) && after_source.bytes==original_source.bytes;
    bool first=epoch_submit_total.load()==0;
    submit_owned_verified=first?verified:submit_owned_verified && verified;
    submit_sources_released=first?source_released:submit_sources_released && source_released;
    submit_original_preserved=first?original_preserved:submit_original_preserved && original_preserved;
    if(!source_released)submit_retained=1;
    ++submit_total;++epoch_submit_total;
    state_original_preserved=submit_original_preserved.load();
    submit_attempted=2;
    if(submit_session_enabled() && (!verified || !source_released || !original_preserved)) {
        // Stop producing immediately; the registered renderer still owns its
        // copies and must retire through the native destruction path.
        if(continuous_session){request_session_stop(SessionStop::proof_failure);service_session_stop_native();}
        else submitted_lifetime.close();
    }
    return true;
}
void* __fastcall hook_renderer(void* destination,void* family,void* canvas,void* transform){
    note_world_progress(WorldProgress::renderer);
    Scope scope;if(!scope.world()){note_world_progress(WorldProgress::renderer_scope_reject);return original_renderer(destination,family,canvas,transform);}
    auto record=start_record(9,destination,_ReturnAddress());
    if(submitting_family&&family==submitting_family&&record.caller==0x3a1595)note_world_progress(WorldProgress::candidate);
    service_session_stop_native();
    gears_anchor::Receipt associated_anchor{};bool anchor_associated=false;
#if defined(GEARS_PAWNLESS_CAMERA_OBSERVER) && GEARS_PAWNLESS_CAMERA_OBSERVER
    if(anchor_observer_enabled&&recording.load()){
        Record candidate{};inspect_family(candidate,family);
        if((candidate.valid&128)&&candidate.family_count==1){
            std::lock_guard<std::mutex> lock(pawnless_scene_mutex);
            for(unsigned i=0;i<pawnless_scene_count;++i){auto& row=pawnless_scene_rows[i];
                if(row.renderer_qpc||row.family!=reinterpret_cast<uintptr_t>(family)||row.source!=candidate.family_views[0]||
                   row.thread!=GetCurrentThreadId()||record.time<row.end_qpc||record.time-row.end_qpc>qpc_frequency/2)continue;
                row.renderer_family=reinterpret_cast<uintptr_t>(family);row.renderer_source=candidate.family_views[0];
                row.renderer_qpc=record.time;row.renderer_thread=GetCurrentThreadId();
                if(safe_copy(row.renderer_bytes.data(),reinterpret_cast<void*>(row.renderer_source),row.renderer_bytes.size())){
                    row.read_mask|=8;if(!memcmp(row.scene_bytes.data()+0x70,row.renderer_bytes.data()+0x70,128))row.read_mask|=16;
                    if(row.scene_bytes==row.renderer_bytes)row.read_mask|=32;
                }
            }
        }
    }
#endif
    if(anchor_observer_enabled&&recording.load()){
        Record input{};inspect_family(input,family);gears_anchor::Key key{};gears_anchor::Anchor anchor{};
        bool valid=anchor_key(key,anchor,reinterpret_cast<uintptr_t>(family),input.family_views[0],expected_local,2);
        auto reason=anchor_context.consume(key,anchor,GetCurrentThreadId(),record.time,valid,input.family_count,record.caller,associated_anchor);
        anchor_associated=reason==gears_anchor::accepted;
        if(!anchor_associated)anchor_note(2,reason,associated_anchor,reinterpret_cast<uintptr_t>(destination));
    }
    bool observe=submitting_family!=nullptr;
    if(observe){
        Record input{};inspect_family(input,family);
        if(renderer_pair_enabled && family==submitting_family && record.caller==0x3a1595) {
            pair_canvas=canvas;pair_transform=transform;
            run_source_shadow(input,family);
            pair_canvas=nullptr;pair_transform=nullptr;
        }
        record.renderer_input_family=reinterpret_cast<uintptr_t>(family);
        record.renderer_sources=input.family_views;
        if(input.valid&128)record.valid|=2048;
        record.clone_source=input.family_views[0];
        if(record.clone_source && safe_copy(record.source_matrices.data(),reinterpret_cast<void*>(record.clone_source+0x70),128))record.valid|=256;
    }
    void* result=nullptr;
    bool handled=submit_pair_enabled && observe && family==submitting_family && record.caller==0x3a1595 && recording.load() &&
        try_submit_pair(destination,family,canvas,transform,record,result,anchor_associated?&associated_anchor:nullptr);
    if(!handled){
        result=original_renderer(destination,family,canvas,transform);
        native_owned_scene_fallback_built(destination,result);
    }
    if(handled)record.test_state=77;
    if(anchor_associated){gears_submitted::FrameTicket frame{};
        auto registered=handled&&submitted_lifetime.frame(reinterpret_cast<uintptr_t>(destination),frame);
        if(registered&&associated_anchor.anchor.unpossessed){const auto index=unpossessed_submission_count.fetch_add(1);
            if(index<unpossessed_submissions.size())unpossessed_submissions[index]={frame.id,committed_world_generation.load(),now(),GetCurrentThreadId()};
            uint64_t empty=0;unpossessed_first_ticket.compare_exchange_strong(empty,frame.id);unpossessed_last_ticket=frame.id;}
        anchor_note(2,gears_anchor::accepted,associated_anchor,reinterpret_cast<uintptr_t>(destination),registered?frame.id:0);}
    if(observe){
        record.result=reinterpret_cast<uintptr_t>(result);
        inspect_completed_renderer(record,destination);
        record.duration=now()-record.time;enqueue(record);
    }
    return result;
}
void* __fastcall hook_scene(void* local,void* family,Vec* position,Rot* rotation,void* viewport,void* drawer) {
    Scope scope;if(!scope.world())return original_scene(local,family,position,rotation,viewport,drawer);service_session_stop_native();auto record=start_record(1,local,_ReturnAddress());
    auto crosshair_evidence=crosshair_live::scene(local);
    const bool anchor_observing=anchor_observer_enabled&&recording.load();
    gears_anchor::PawnBinding pawn_before_scene{};
    gears_anchor::PawnBindingDiagnostic pawn_diagnostic{};
    const bool pawn_before_valid=anchor_observing&&
        gears_anchor::capture_pawn_binding(anchor_config,safe_copy,pawn_before_scene,&pawn_diagnostic);
    if(anchor_observing&&!pawn_before_valid){const auto index=anchor_pawn_reject_count.fetch_add(1);
        if(index<anchor_pawn_rejects.size())anchor_pawn_rejects[index]={pawn_diagnostic,now(),GetCurrentThreadId()};}
#if defined(GEARS_PAWNLESS_CAMERA_OBSERVER) && GEARS_PAWNLESS_CAMERA_OBSERVER
    PawnlessSceneRow pawnless_row{};bool pawnless_sample=false;
    if(anchor_observing&&!pawn_before_valid&&pawn_diagnostic.field==gears_anchor::PawnBindingField::controller_pawn&&
       pawn_diagnostic.actual_valid&&!pawn_diagnostic.actual){
        const auto timestamp=now();auto next=pawnless_scene_next.load();
        bool room=false;{std::lock_guard<std::mutex> lock(pawnless_scene_mutex);room=pawnless_scene_count<pawnless_scene_rows.size();}
        if(room&&qpc_frequency>0&&timestamp>=next&&pawnless_scene_next.compare_exchange_strong(next,timestamp+qpc_frequency/4)){
            pawnless_sample=gears_pawnless_camera::observe(anchor_config,safe_copy,pawnless_row.before);
            pawnless_row.begin_qpc=timestamp;pawnless_row.thread=GetCurrentThreadId();
            pawnless_row.local=reinterpret_cast<uintptr_t>(local);pawnless_row.family=reinterpret_cast<uintptr_t>(family);
        }
    }
#endif
    struct AnchorSceneScope {bool enabled;explicit AnchorSceneScope(bool e):enabled(e){if(e)anchor_context.begin_scene();}
        ~AnchorSceneScope(){if(enabled)anchor_context.end_scene();}} anchor_scene(anchor_observing);
    void* saved_local=scene_local;Rot* saved_rotation=scene_rotation;int saved_delta=scene_delta;
    unsigned saved_phase=scene_phase;Vec saved_translation=scene_translation;Rot saved_rotation_delta=scene_rotation_delta;
    scene_local=local;scene_rotation=rotation;scene_delta=0;
    scene_phase=0;scene_translation={};scene_rotation_delta={};
    void* result=original_scene(local,family,position,rotation,viewport,drawer);
#if defined(GEARS_PAWNLESS_CAMERA_OBSERVER) && GEARS_PAWNLESS_CAMERA_OBSERVER
    if(pawnless_sample){
        gears_pawnless_camera::observe(anchor_config,safe_copy,pawnless_row.after);
        pawnless_row.end_qpc=now();Record candidate{};inspect_family(candidate,family);
        if((candidate.valid&128)&&candidate.family_count==1){pawnless_row.read_mask|=1;pawnless_row.source=candidate.family_views[0];
            if(pawnless_row.source==reinterpret_cast<uintptr_t>(result))pawnless_row.read_mask|=2;
            if(pawnless_row.source&&safe_copy(pawnless_row.scene_bytes.data(),reinterpret_cast<void*>(pawnless_row.source),pawnless_row.scene_bytes.size()))pawnless_row.read_mask|=4;
        }
        std::lock_guard<std::mutex> lock(pawnless_scene_mutex);
        if(pawnless_scene_count<pawnless_scene_rows.size())pawnless_scene_rows[pawnless_scene_count++]=pawnless_row;
    }
#endif
    if(anchor_observing){gears_anchor::Receipt receipt{};receipt.invocation=++anchor_context.sequence;
        receipt.thread=GetCurrentThreadId();receipt.scene_qpc=now();
        const bool anchor_valid=pawn_before_valid&&anchor_key(receipt.key,receipt.anchor,reinterpret_cast<uintptr_t>(family),reinterpret_cast<uintptr_t>(result),reinterpret_cast<uintptr_t>(local),0)&&
            gears_anchor::scene_binding_matches(pawn_before_scene,receipt.anchor);
        if(anchor_valid&&!receipt.anchor.unpossessed)crosshair_live::attach(std::move(crosshair_evidence),receipt);
        auto reason=anchor_valid?anchor_context.offer(receipt):gears_anchor::identity_failed;
        if(reason==gears_anchor::accepted)crosshair_live::accepted(receipt);
        if(reason!=gears_anchor::accepted)anchor_context.discard();anchor_note(0,reason,receipt);}
    if(record.object==expected_local)trace_rectangle(0,reinterpret_cast<uintptr_t>(result));
    if(family_diagnostics)inspect_family(record,family);
    record.duration=now()-record.time;record.result=reinterpret_cast<uintptr_t>(result);
    if(safe_copy(&record.position,position,sizeof(Vec))) record.valid|=1;
    if(safe_copy(&record.rotation,rotation,sizeof(Rot))) record.valid|=2;
    if(result && safe_copy(record.matrices.data(),reinterpret_cast<char*>(result)+0x70,128)) record.valid|=4;
    if(camera_cache && safe_copy(&record.camera_time,reinterpret_cast<void*>(camera_cache),4)) record.valid|=8;
    if(record.object==expected_local) record.valid|=16;
    record.yaw_delta=scene_delta;record.source_yaw=record.rotation.yaw-scene_delta;record.test_state=wobble_state.load();
    if(scene_delta) record.valid|=64;
    if(camera_yaw) safe_copy(&record.cache_yaw,reinterpret_cast<void*>(camera_yaw),4);
    record.pose_phase=scene_phase;record.translation_delta=scene_translation;record.rotation_delta=scene_rotation_delta;
    if(live_enabled){record.pose_status=live_status;record.pose_sequence=live_sequence;}
    record.source_position={record.position.x-scene_translation.x,record.position.y-scene_translation.y,record.position.z-scene_translation.z};
    record.source_rotation={record.rotation.pitch-scene_rotation_delta.pitch,record.rotation.yaw-scene_rotation_delta.yaw,record.rotation.roll-scene_rotation_delta.roll};
    if(scene_rotation_delta.pitch || scene_rotation_delta.yaw || scene_rotation_delta.roll ||
       scene_translation.x || scene_translation.y || scene_translation.z)record.valid|=64;
    scene_local=saved_local;scene_rotation=saved_rotation;scene_delta=saved_delta;
    scene_phase=saved_phase;scene_translation=saved_translation;scene_rotation_delta=saved_rotation_delta;
    enqueue(record);return result;
}
void __fastcall hook_event(void* pc,Vec* position,Rot* rotation) {
    Scope scope;if(!scope.world())return original_event(pc,position,rotation);auto record=start_record(2,pc,_ReturnAddress());
    original_event(pc,position,rotation);
    record.duration=now()-record.time;
    apply_wobble(record,pc,rotation);
    apply_sweep(record,pc,position,rotation);
    apply_live(record,pc,position,rotation);
    if(safe_copy(&record.position,position,sizeof(Vec))) record.valid|=1;
    if(safe_copy(&record.rotation,rotation,sizeof(Rot))) record.valid|=2;
    if(record.object==expected_pc) record.valid|=16;
    enqueue(record);
}
void inspect_swapchain(Record& record,IDXGISwapChain* chain) {
    if(!gpu_diagnostics || !recording.load())return;
    DXGI_SWAP_CHAIN_DESC desc{};
    if(SUCCEEDED(chain->GetDesc(&desc))) {
        record.width=desc.BufferDesc.Width;record.height=desc.BufferDesc.Height;
        record.format=desc.BufferDesc.Format;record.buffer_count=desc.BufferCount;record.swap_flags=desc.Flags;
        record.gpu_window=reinterpret_cast<uintptr_t>(desc.OutputWindow);
    }
    ID3D12Device* device{};
    if(SUCCEEDED(chain->GetDevice(IID_PPV_ARGS(&device)))) {
        record.gpu_device=reinterpret_cast<uintptr_t>(device);device->Release();record.valid|=128;
    }
    IDXGISwapChain3* current{};
    if(SUCCEEDED(chain->QueryInterface(IID_PPV_ARGS(&current)))) {
        record.buffer_index=current->GetCurrentBackBufferIndex();current->Release();
        ID3D12Resource* buffer{};
        if(SUCCEEDED(chain->GetBuffer(record.buffer_index,IID_PPV_ARGS(&buffer)))) {
            record.gpu_buffer=reinterpret_cast<uintptr_t>(buffer);buffer->Release();record.valid|=256;
        }
    }
}
void STDMETHODCALLTYPE hook_execute(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList* const* lists) {
    NativeOwnedShutdownRhiScope owned_shutdown_scope;
    final_transfer_live::normalizer::boundary();
    if(stereo_publish_bypass){owned_source_pixels::execute_passthrough(queue,count,lists,death_hdr_pixels::execute_passthrough);return;}
    Scope scope;if(!scope.world()){auto unknown=frame_lock();frame_link.invalidate();owned_ticket_size_trace::clear_attachments();owned_source_pixels::execute_passthrough(queue,count,lists,death_hdr_pixels::execute_passthrough);return;}auto frame_guard=frame_lock();auto record=start_record(5,queue,_ReturnAddress());
    gears_frame_link::ExecutionToken owned_token{};
    gears_frame_link::ExecutionToken* owned=nullptr;
#if defined(GEARS_OWNED_OUTPUT_TRACKING) && GEARS_OWNED_OUTPUT_TRACKING
    if(frame_link.owned_status().enabled)owned=&owned_token;
#endif
    observe_frame_execute(queue,count,lists,reinterpret_cast<uintptr_t>(_ReturnAddress()),owned);
    if(gpu_diagnostics && recording.load()) {
        record.queue_type=queue->GetDesc().Type;record.command_count=count;
        ID3D12Device* device{};
        if(SUCCEEDED(queue->GetDevice(IID_PPV_ARGS(&device)))) {
            record.gpu_device=reinterpret_cast<uintptr_t>(device);device->Release();record.valid|=128;
        }
    }
    // Even a failed prepare forwards the game submission exactly once. Only a
    // successful original-call interval may advance owned submitted state.
    const auto scene_receipt=native_owned_scene_prepare(queue,count,lists);
    const auto scene_gpu_token=owned_scene_gpu::prepare(queue,count,lists,reinterpret_cast<uintptr_t>(_ReturnAddress()));
    // Snapshot the original submission membership before forwarding. This is
    // CPU binding retirement only; GPU generations and execution proof remain.
    std::array<ID3D12CommandList*,gears_frame_link::Tracker::list_capacity> binding_lists{};
    std::array<uintptr_t,gears_frame_link::Tracker::list_capacity> binding_ids{};
    const bool retire_bindings=owned_bindings_enabled&&(recording.load()||prime_recording.load())&&
        count&&count<=binding_lists.size()&&safe_copy(binding_lists.data(),lists,count*sizeof(binding_lists[0]));
    if(retire_bindings)for(unsigned i=0;i<count;++i)binding_ids[i]=canonical_identity(binding_lists[i]);
    auto pixel_guard=death_hdr_pixels::execution_lock();
    const auto pixel_token=death_hdr_pixels::prepare_execute(queue,count,lists);
    owned_source_pixels::execute_passthrough(queue,count,lists,original_execute);
    death_hdr_pixels::after_execute(pixel_token,queue);
    owned_scene_gpu::commit(scene_gpu_token);
    native_owned_scene_commit(scene_receipt);
    if(owned&&owned_token.value&&!frame_link.commit_execute(owned_token))++frame_counts.execute_rejected;
    if(retire_bindings)for(unsigned i=0;i<count;++i)owned_target_stats.retire(binding_ids[i]);
    if(retire_bindings)for(unsigned i=0;i<count;++i)owned_ticket_size_trace::retire_attachments(reinterpret_cast<uintptr_t>(binding_lists[i]));
    else if(count)owned_ticket_size_trace::clear_attachments(); // Membership unreadable/unbounded: retain no stale OM state.
    record.duration=now()-record.time;enqueue(record);
}
bool desktop_mirror_present(IDXGISwapChain* chain,UINT flags){
#if defined(GEARS_DESKTOP_MIRROR) && GEARS_DESKTOP_MIRROR
    if(!recording.load()||!stereo_gpu_publish||stereo_publish_counts.stopped.load()||
       (flags&(DXGI_PRESENT_TEST|DXGI_PRESENT_DO_NOT_WAIT)))return false;
    const auto identity=canonical_identity(chain);const auto binding=swapchain_bindings.lookup(identity);
    if(!binding.chain||!owned_scene_gpu::queue||binding.queue!=canonical_identity(owned_scene_gpu::queue)){
        ++mirror_inventory_rejected;return false;
    }
    DXGI_SWAP_CHAIN_DESC desc{};std::array<uintptr_t,16> resources{};
    if(FAILED(chain->GetDesc(&desc))||!desc.BufferCount||desc.BufferCount>resources.size()){
        ++mirror_inventory_rejected;return false;
    }
    for(unsigned i=0;i<desc.BufferCount;++i){gears_vr::ComPtr<ID3D12Resource> buffer;
        if(FAILED(chain->GetBuffer(i,IID_PPV_ARGS(&buffer)))){++mirror_inventory_rejected;return false;}
        resources[i]=canonical_identity(buffer.Get());
    }
    if(!swapchain_bindings.unchanged(identity,desc.BufferDesc.Width,desc.BufferDesc.Height,resources.data(),desc.BufferCount)){
        ++mirror_inventory_rejected;return false;
    }
    gears_vr::ComPtr<IDXGISwapChain3> current;gears_vr::ComPtr<ID3D12Resource> buffer;
    if(FAILED(chain->QueryInterface(IID_PPV_ARGS(&current)))){++mirror_inventory_rejected;return false;}
    const auto index=current->GetCurrentBackBufferIndex();
    if(index>=desc.BufferCount||FAILED(chain->GetBuffer(index,IID_PPV_ARGS(&buffer)))||canonical_identity(buffer.Get())!=resources[index]){
        ++mirror_inventory_rejected;return false;
    }
    // The game is about to call Present on this queue. D3D12's documented
    // Present contract requires this backbuffer in PRESENT/COMMON here.
    // Mirror commands restore that state on this same queue before Present.
    // Async slots retain their backbuffers until fence completion; the resize
    // wrapper drains those slots before forwarding ResizeBuffers.
    StereoPublishBypass bypass;
    return desktop_mirror.present(buffer.Get(),owned_scene_gpu::queue,gears_vr::monotonic_ns());
#else
    (void)chain;(void)flags;return false;
#endif
}
HRESULT STDMETHODCALLTYPE hook_present(IDXGISwapChain* chain,UINT interval,UINT flags) {
    final_transfer_live::normalizer::boundary();
    Scope scope;if(!scope.world()){auto unknown=frame_lock();frame_link.invalidate();return original_present(chain,interval,flags);}auto frame_guard=frame_lock();
    FramePresentScope frame_present;
    auto record=start_record(3,chain,_ReturnAddress());
    inspect_swapchain(record,chain);
    bool frame_interval=frame_present.observe&&frame_guard.owns_lock();
    std::optional<gears_outage::Sample> diagnostic;
    if(frame_interval&&outage_diagnostics.load()){diagnostic.emplace();outage_begin(*diagnostic,record.time);}
    if(frame_interval)observe_frame_present(chain,flags,diagnostic?&*diagnostic:nullptr);
#if defined(GEARS_OWNED_UI_BATCH) && GEARS_OWNED_UI_BATCH
    if(frame_interval&&recording.load()&&!(flags&DXGI_PRESENT_TEST))native_owned_scene_copy();
#endif
    auto saved_inside_present=frame_inside_original_present;frame_inside_original_present=true;
    if(frame_interval)desktop_mirror_present(chain,flags);
    if(diagnostic)diagnostic->original_begin_qpc=now();
    HRESULT result=original_present(chain,interval,flags);
    if(diagnostic)outage_finish(*diagnostic,now(),result);
    frame_inside_original_present=saved_inside_present;
    if(frame_interval)retire_original_present_interval();
    if(frame_present.observe&&frame_link_diagnostics&&recording.load()&&FAILED(result))++frame_counts.present_failed;
    record.duration=now()-record.time;record.result=static_cast<unsigned long>(result);
    if(!(flags&DXGI_PRESENT_TEST)) record.valid|=32;
    enqueue(record);return result;
}
HRESULT STDMETHODCALLTYPE hook_present1(IDXGISwapChain1* chain,UINT interval,UINT flags,const DXGI_PRESENT_PARAMETERS* params) {
    final_transfer_live::normalizer::boundary();
    Scope scope;if(!scope.world()){auto unknown=frame_lock();frame_link.invalidate();return original_present1(chain,interval,flags,params);}auto frame_guard=frame_lock();
    FramePresentScope frame_present;
    auto record=start_record(4,chain,_ReturnAddress());
    inspect_swapchain(record,chain);
    bool frame_interval=frame_present.observe&&frame_guard.owns_lock();
    std::optional<gears_outage::Sample> diagnostic;
    if(frame_interval&&outage_diagnostics.load()){diagnostic.emplace();outage_begin(*diagnostic,record.time);}
    if(frame_interval)observe_frame_present(chain,flags,diagnostic?&*diagnostic:nullptr);
#if defined(GEARS_OWNED_UI_BATCH) && GEARS_OWNED_UI_BATCH
    if(frame_interval&&recording.load()&&!(flags&DXGI_PRESENT_TEST))native_owned_scene_copy();
#endif
    auto saved_inside_present=frame_inside_original_present;frame_inside_original_present=true;
    DXGI_PRESENT_PARAMETERS full_update{};
    const bool mirrored=frame_interval&&desktop_mirror_present(chain,flags);
#if defined(GEARS_DESKTOP_MIRROR) && GEARS_DESKTOP_MIRROR
    if(mirrored)++mirror_present1_full;
#endif
    if(diagnostic)diagnostic->original_begin_qpc=now();
    HRESULT result=original_present1(chain,interval,flags,mirrored?&full_update:params);
    if(diagnostic)outage_finish(*diagnostic,now(),result);
    frame_inside_original_present=saved_inside_present;
    if(frame_interval)retire_original_present_interval();
    if(frame_present.observe&&frame_link_diagnostics&&recording.load()&&FAILED(result))++frame_counts.present_failed;
    record.duration=now()-record.time;record.result=static_cast<unsigned long>(result);
    if(!(flags&DXGI_PRESENT_TEST)) record.valid|=32;
    enqueue(record);return result;
}
bool hash_matches() {
    wchar_t path[MAX_PATH];GetModuleFileNameW(nullptr,path,MAX_PATH);
    const wchar_t* name=wcsrchr(path,L'\\');
    if(!name || _wcsicmp(name+1,L"GOWDE-Steam.exe")) return false;
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
    if(file==INVALID_HANDLE_VALUE) return false;
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};
    bool ok=BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(ok) ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
    std::array<unsigned char,65536> buffer;DWORD bytes{};
    if(ok) {
        for(;;) {
            if(!ReadFile(file,buffer.data(),static_cast<DWORD>(buffer.size()),&bytes,nullptr)) {ok=false;break;}
            if(!bytes) break;
            if(BCryptHashData(hash,buffer.data(),bytes,0)<0) {ok=false;break;}
        }
    }
    unsigned char digest[32]{};char hex[65]{};
    if(ok) ok=BCryptFinishHash(hash,digest,32,0)>=0;
    for(unsigned i=0;i<32;++i) sprintf_s(hex+2*i,3,"%02x",digest[i]);
    if(hash) BCryptDestroyHash(hash);
    if(algorithm) BCryptCloseAlgorithmProvider(algorithm,0);
    CloseHandle(file);
    return ok && !strcmp(hex,"bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4");
}
void path_for(wchar_t* result,const wchar_t* filename) {swprintf_s(result,MAX_PATH,L"%s\\%s",directory,filename);}
uintptr_t ini_address(const wchar_t* ini,const wchar_t* key) {
    wchar_t value[64];GetPrivateProfileStringW(L"Capture",key,L"0",value,64,ini);
    return _wcstoui64(value,nullptr,0);
}
bool load_anchor_config(const wchar_t* ini){
    gears_anchor::Config c{};
    if(!gears_anchor::parse_config([&](const wchar_t* key,wchar_t* value,unsigned capacity){
            return GetPrivateProfileStringW(L"AnchorObserver",key,L"",value,capacity,ini);
        },reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)),c))return false;
    configure_unpossessed_camera(c);
    gears_anchor::Anchor a{};std::uint32_t stamp{};
    const bool expected_identity=c.role_local<c.identity_count&&c.role_controller<c.identity_count&&c.role_camera<c.identity_count&&
        c.identities[c.role_local].address==expected_local&&c.identities[c.role_controller].address==expected_pc&&
        c.identities[c.role_camera].address==expected_camera&&expected_camera+c.camera_timestamp==camera_cache;
    gears_anchor::BoundedGuardDiagnostic guard_diagnostic{};
    const bool admitted=expected_identity&&gears_anchor::bounded_guard(c,safe_copy,
        [](){return std::uint64_t(GetTickCount64());},[](unsigned milliseconds){Sleep(milliseconds);},a,stamp,&guard_diagnostic);
    if(diagnostic_status){auto const& last=guard_diagnostic.last;
        fprintf(diagnostic_status,"anchor_startup_guard expected_identity=%u admitted=%u attempts=%u elapsed_ms=%llu reason=%s last_reason=%s stage=%s before_stamp=%u strict_after_stamp=%u final_stamp=%u timestamp_only=%u\n",
            expected_identity?1:0,admitted?1:0,guard_diagnostic.attempts,static_cast<unsigned long long>(guard_diagnostic.elapsed_ms),
            expected_identity?gears_anchor::guard_reason_name(guard_diagnostic.reason):"not_attempted",
            gears_anchor::guard_reason_name(last.reason),gears_anchor::guard_stage_name(last.stage),last.before_stamp,last.strict_after_stamp,last.final_stamp,last.timestamp_only?1:0);
        fflush(diagnostic_status);}
    if(admitted)anchor_config=c;
    return admitted;
}
void flush(FILE* file,unsigned max_batches=0) {
    std::array<Record,128> batch;
    unsigned batches=0;
    for(;!max_batches||batches<max_batches;++batches) {
        AcquireSRWLockExclusive(&queue_lock);
        size_t count=0;
        while(read_index<write_index && count<batch.size()) batch[count++]=ring[read_index++%ring.size()];
        ReleaseSRWLockExclusive(&queue_lock);
        if(!count) break;
        for(size_t i=0;i<count;++i) {
            const auto& r=batch[i];++emitted[r.kind];
            fprintf(file,"%u,%u,%u,%u,%lld,%lld,0x%llx,0x%llx,0x%llx,%.9g,%.9g,%.9g,%d,%d,%d,%.9g",
                r.kind,r.thread,r.focused,r.valid,r.time,r.duration,
                static_cast<unsigned long long>(r.object),static_cast<unsigned long long>(r.result),
                static_cast<unsigned long long>(r.caller),r.position.x,r.position.y,r.position.z,
                r.rotation.pitch,r.rotation.yaw,r.rotation.roll,r.camera_time);
            fprintf(file,",%d,%d,%d,%u",r.source_yaw,r.yaw_delta,r.cache_yaw,r.test_state);
            for(float value:r.matrices) fprintf(file,",%.9g",value);
            fprintf(file,",%u,%.9g,%.9g,%.9g,%d,%d,%d,%.9g,%.9g,%.9g,%d,%d,%d",
                r.pose_phase,r.source_position.x,r.source_position.y,r.source_position.z,
                r.source_rotation.pitch,r.source_rotation.yaw,r.source_rotation.roll,
                r.translation_delta.x,r.translation_delta.y,r.translation_delta.z,
                r.rotation_delta.pitch,r.rotation_delta.yaw,r.rotation_delta.roll);
            fprintf(file,",%u,%llu,0x%llx,0x%llx,0x%llx,%u,%u,%u,%u,%u,%u,%u,%u",r.pose_status,r.pose_sequence,
                static_cast<unsigned long long>(r.gpu_device),static_cast<unsigned long long>(r.gpu_buffer),static_cast<unsigned long long>(r.gpu_window),
                r.queue_type,r.command_count,r.buffer_index,r.buffer_count,r.width,r.height,r.format,r.swap_flags);
            fprintf(file,",0x%llx,0x%llx,%d,%d",static_cast<unsigned long long>(r.family_address),static_cast<unsigned long long>(r.family_array),r.family_count,r.family_capacity);
            for(auto view:r.family_views)fprintf(file,",0x%llx",static_cast<unsigned long long>(view));
            fprintf(file,",0x%llx,0x%llx",static_cast<unsigned long long>(r.clone_source),static_cast<unsigned long long>(r.clone_owner));
            for(float value:r.source_matrices)fprintf(file,",%.9g",value);
            fprintf(file,",0x%llx,0x%llx,%d,%d",static_cast<unsigned long long>(r.renderer_input_family),static_cast<unsigned long long>(r.renderer_storage),r.renderer_count,r.renderer_capacity);
            for(auto view:r.renderer_sources)fprintf(file,",0x%llx",static_cast<unsigned long long>(view));
            fprintf(file,",%llu,%u,%llu,%llu,%llu\n",r.stereo_ticket,r.stereo_source,r.stereo_head_flags,r.stereo_view_flags,r.stereo_reference_generation);
        }
        fflush(file);
    }
}
bool install(void* target,void* detour,void** original) {
    if(continuous_session){
        if(session_hook_entry_count==session_hook_entries.size()){
            if(diagnostic_status)fprintf(diagnostic_status,"hook_entry_capacity_exhausted count=%u capacity=%zu target=%p\n",session_hook_entry_count,session_hook_entries.size(),target);
            return false;
        }
        auto& entry=session_hook_entries[session_hook_entry_count];entry.target=target;entry.detour=detour;
        auto primed=std::find(prime_targets.begin(),prime_targets.end(),target);
        if(prime_ready.load()&&primed!=prime_targets.end())entry.bytes=prime_original_bytes[static_cast<size_t>(primed-prime_targets.begin())];
        else if(graphics_lock_primed_bytes(target,entry.bytes)){}
        else if(!safe_copy(entry.bytes.data(),target,entry.bytes.size()))return false;
        ++session_hook_entry_count;
    }
    MH_STATUS created=MH_CreateHook(target,detour,original);
    if(created==MH_ERROR_ALREADY_CREATED && *original &&
         ((reservation_ready && (target==scene_target || target==event_target || target==family_target || target==clone_target || target==renderer_target || target==consume_target || target==destroy_renderer_target || target==prepare_target || target==state_delete_target || target==quad_target || target==geometry_target || (first_view_mask_reserved && target==first_view_mask_target))) ||
          (prime_ready.load() && std::find(prime_targets.begin(),prime_targets.end(),target)!=prime_targets.end())||
          graphics_lock_observer_reserved_target(target)||
          (ui_reserved&&(std::find(ui_targets.begin(),ui_targets.end(),target)!=ui_targets.end()||target==ui_execute_target||target==ui_subtitle_target
#if defined(GEARS_UI_SCENE_ROOT_OBSERVER) && GEARS_UI_SCENE_ROOT_OBSERVER
             ||target==ui_scene_root_live::target
#endif
#if defined(GEARS_UI_CALL_BOUNDARY_OBSERVER) && GEARS_UI_CALL_BOUNDARY_OBSERVER
             ||target==ui_call_boundary::target
#endif
#if defined(GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER) && GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER
             ||target==crosshair_live::target
#endif
#if defined(GEARS_UI_MATERIAL_ROUTE) && GEARS_UI_MATERIAL_ROUTE
             ||target==ui_material_execute_target
#if defined(GEARS_GAME_UI_MATERIAL) && GEARS_GAME_UI_MATERIAL
             ||target==game_material::upload_target
#endif
#if defined(GEARS_OWNED_UI_MATERIAL) && GEARS_OWNED_UI_MATERIAL
             ||target==owned_material::produce_target||target==owned_material::upload_target
#endif
#endif
#if defined(GEARS_OWNED_UI_ROUTE_PROBE) && GEARS_OWNED_UI_ROUTE_PROBE
             ||target==owned_ui_live::flush_target||target==owned_ui_live::produce_target
#endif
          ))))created=MH_OK;
    MH_STATUS queued=created==MH_OK?MH_QueueEnableHook(target):MH_ERROR_NOT_CREATED;
    if(diagnostic_status)fprintf(diagnostic_status,"hook_target=%p create=%s queue=%s\n",target,MH_StatusToString(created),MH_StatusToString(queued));
    return created==MH_OK && queued==MH_OK;
}
bool session_hooks_restored() {
    if(active.load())return false;
    for(unsigned i=0;i<session_hook_entry_count;++i){std::array<unsigned char,32> bytes{};
        if(!safe_copy(bytes.data(),session_hook_entries[i].target,bytes.size())||bytes!=session_hook_entries[i].bytes)return false;}
    if(prime_ready.load())for(unsigned i=0;i<prime_targets.size();++i){std::array<unsigned char,32> bytes{};
        if(!safe_copy(bytes.data(),prime_targets[i],bytes.size())||bytes!=prime_original_bytes[i])return false;}
#if defined(GEARS_PATH_CHOICE_CLOSE_OBSERVER) && GEARS_PATH_CHOICE_CLOSE_OBSERVER
    if(!path_choice_close_observer::restored())return false;
#endif
    return true;
}
#include "hook_entry_manifest.inl"
#if defined(GEARS_PATH_CHOICE_CLOSE_OBSERVER) && GEARS_PATH_CHOICE_CLOSE_OBSERVER
bool path_choice_close_observer::read_own_entry(void* output,void const* source,std::size_t size){
    if(!hook_entry_manifest_ready.load(std::memory_order_acquire))return false;
    for(unsigned i=0;i<hook_entry_proof_count;++i){auto const& proof=hook_entry_proofs[i];
        if(proof.target==reinterpret_cast<uintptr_t>(path_choice_close_observer::target)&&
           proof.detour==reinterpret_cast<uintptr_t>(path_choice_close_observer::hook))
            return gears_path_choice_close::read_own_prefix(proof,reinterpret_cast<uintptr_t>(path_choice_close_observer::target),
                reinterpret_cast<uintptr_t>(path_choice_close_observer::hook),output,source,size,safe_copy);}
    return false;
}
#endif
#include "world_rebind_prepare.inl"
#include "world_rebind_commit.inl"
#include "resource_resize_probe.inl"
#if defined(GEARS_OWNED_OUTPUT_TRACKING) && GEARS_OWNED_OUTPUT_TRACKING
uintptr_t barrier_identity(IUnknown* object) {
    __try{return canonical_identity(object);}
    __except(EXCEPTION_EXECUTE_HANDLER){return 0;}
}
void STDMETHODCALLTYPE hook_resource_barrier(ID3D12GraphicsCommandList* list,UINT count,const D3D12_RESOURCE_BARRIER* barriers) {
    if(stereo_publish_bypass){original_resource_barrier(list,count,barriers);return;}
    Scope scope;if(!scope.world()){auto unknown=frame_lock();frame_link.invalidate();return original_resource_barrier(list,count,barriers);}auto frame_guard=frame_lock();const auto owned=frame_link.owned_status();
#if defined(GEARS_NGX_EPOCH_PAIR_OBSERVER) && GEARS_NGX_EPOCH_PAIR_OBSERVER
    ngx_epoch_pair::BarrierScope epoch_barriers(list,count,barriers,reinterpret_cast<uintptr_t>(_ReturnAddress()));
#endif
    ngx_capture_control::acquire_barriers(count,barriers);
    ngx_stereo_host::barriers(list,count,barriers,reinterpret_cast<uintptr_t>(_ReturnAddress()));
    death_hdr_timeline::barriers_event(list,count,barriers,reinterpret_cast<uintptr_t>(_ReturnAddress())-base);
    {gears_render_cost::Scope cost(gears_render_cost::barrier_observation);
#if defined(GEARS_NATIVE_OWNED_PIXELS) && GEARS_NATIVE_OWNED_PIXELS
    if(owned_scene_gpu::watching()){
        if(count>1024||(count&&!barriers))owned_scene_gpu::uncertain(owned_scene_gpu::Unknown::unreadable);
        else for(UINT i=0;i<count;++i){D3D12_RESOURCE_BARRIER b{};
            if(!safe_copy(&b,barriers+i,sizeof(b))){owned_scene_gpu::uncertain(owned_scene_gpu::Unknown::unreadable);break;}
            owned_scene_gpu::barrier(list,b);}
    }
#endif
    if(owned.enabled){
        const auto ref=frame_link.current(barrier_identity(list));
        auto fault=[&](){frame_link.record_owned_barrier_fault(ref,owned.generation);};
        // Bounded acquisition; no guessed state if a batch cannot be inspected.
        if(count>1024||(count&&!barriers))fault();
        else for(UINT i=0;i<count;++i){
            D3D12_RESOURCE_BARRIER b{};
            if(!safe_copy(&b,barriers+i,sizeof(b))){fault();break;}
            if(b.Type==D3D12_RESOURCE_BARRIER_TYPE_TRANSITION){
                auto resource=barrier_identity(b.Transition.pResource);
                if(!resource)fault();
                else if(resource==owned.target.resource)
                    frame_link.record_transition(ref,resource,owned.generation,b.Transition.StateBefore,
                        b.Transition.StateAfter,b.Transition.Subresource,b.Flags);
            }else if(b.Type==D3D12_RESOURCE_BARRIER_TYPE_ALIASING){
                auto before=barrier_identity(b.Aliasing.pResourceBefore),after=barrier_identity(b.Aliasing.pResourceAfter);
                // Null is a wildcard, not proof that this allocation is absent.
                if(!before||!after||before==owned.target.resource||after==owned.target.resource)fault();
            }else if(b.Type==D3D12_RESOURCE_BARRIER_TYPE_UAV){
                auto resource=barrier_identity(b.UAV.pResource);
                if(!resource||resource==owned.target.resource)fault();
            }else fault();
        }
    }
    }original_resource_barrier(list,count,barriers);
#if defined(GEARS_NGX_EPOCH_PAIR_OBSERVER) && GEARS_NGX_EPOCH_PAIR_OBSERVER
    epoch_barriers.returned(list);
#endif
}
#endif
#include "owned_scene_gpu_hooks.inl"
HRESULT STDMETHODCALLTYPE hook_reset_list(ID3D12GraphicsCommandList* list,ID3D12CommandAllocator* allocator,ID3D12PipelineState* state) {
    // Alias QI and its temporary Release must remain inside the teardown drain,
    // including bypass/unowned paths and unwinding before the ordinary Scope.
    NativeOwnedShutdownRhiScope::Activity compute_reset_activity;
    ngx_mv_compute_capture::ResetScope compute_reset(list);
    if(stereo_publish_bypass){const auto hr=owned_source_pixels::reset_passthrough(list,allocator,state,death_pso_cache::reset_passthrough);compute_reset.finish(hr);return hr;}
    Scope scope;if(!scope.world()){auto unknown=frame_lock();frame_link.invalidate();owned_ticket_size_trace::retire_attachments(reinterpret_cast<uintptr_t>(list));
        const auto hr=owned_source_pixels::reset_passthrough(list,allocator,state,death_pso_cache::reset_passthrough);compute_reset.finish(hr);return hr;}auto frame_guard=frame_lock();
#if defined(GEARS_NGX_EPOCH_PAIR_OBSERVER) && GEARS_NGX_EPOCH_PAIR_OBSERVER
    const auto epoch_reset_before=ngx_epoch_pair::list(list);
#endif
    HRESULT result=owned_source_pixels::reset_passthrough(list,allocator,state,original_reset_list);
    owned_ticket_size_trace::retire_attachments(reinterpret_cast<uintptr_t>(list)); // Success starts unbound; failure cannot certify old state.
    death_hdr_pixels::reset_after_original(list,result);
    death_pso_cache::reset(list,result);
    gears_render_cost::Scope cost(gears_render_cost::reset_tracking);
    owned_scene_gpu::reset(list,result);
    if(owned_bindings_enabled&&(recording.load()||prime_recording.load())){
        const auto identity=canonical_identity(list);
        if(SUCCEEDED(result))owned_target_stats.reset(identity);else owned_target_stats.retire(identity);
    }
    if(recording.load()&&SUCCEEDED(result))viewport_stats.reset(reinterpret_cast<uintptr_t>(list));
    bool tracking=frame_link_diagnostics&&recording.load();
#if defined(GEARS_OWNED_OUTPUT_TRACKING) && GEARS_OWNED_OUTPUT_TRACKING
    tracking=tracking||frame_link.owned_status().enabled;
#endif
    gears_frame_link::ListRef actual_reset{};
    if(tracking) {
        auto identity=canonical_identity(list);
        frame_seen_list(identity);
        if(SUCCEEDED(result)){++frame_counts.resets;actual_reset=frame_link.reset_success(identity);
            ngx_capture_control::reset_command(list);
            if(!frame_target_stats.reset(reinterpret_cast<uintptr_t>(list)))frame_link.invalidate();}
        else {++frame_counts.reset_failed;frame_link.invalidate_list(identity);}
    }
#if defined(GEARS_NGX_EPOCH_PAIR_OBSERVER) && GEARS_NGX_EPOCH_PAIR_OBSERVER
    ngx_epoch_pair::reset(epoch_reset_before,list,result);
#endif
    compute_reset.finish(result,actual_reset);
    return result;
}
void STDMETHODCALLTYPE hook_viewports(ID3D12GraphicsCommandList* list,UINT count,const D3D12_VIEWPORT* values) {
    final_transfer_live::normalizer::bind(1,reinterpret_cast<uintptr_t>(list),count,values,stereo_publish_bypass!=0);
    final_transfer_live::bracket::consumed_viewport(reinterpret_cast<uintptr_t>(list),count,values,stereo_publish_bypass!=0);
    if(stereo_publish_bypass){original_viewports(list,count,values);return;}
    Scope scope;if(!scope.world()){auto unknown=frame_lock();frame_link.invalidate();return original_viewports(list,count,values);}auto frame_guard=frame_lock();
    if(recording.load()||(owned_bindings_enabled&&prime_recording.load())) {
        D3D12_VIEWPORT value{};unsigned category=gears_viewport::unknown;
        const bool known=count==1&&safe_copy(&value,values,sizeof(value));
        if(known)category=gears_viewport::classify(value.TopLeftX,value.TopLeftY,value.Width,value.Height);
        else if(count>1)category=gears_viewport::other;
        viewport_stats.set(reinterpret_cast<uintptr_t>(list),category,false,
            {value.TopLeftX,value.TopLeftY,value.Width,value.Height},count,known,value.MinDepth,value.MaxDepth);
        if(frame_link_diagnostics)frame_target_stats.set(reinterpret_cast<uintptr_t>(list),category,false,
            {value.TopLeftX,value.TopLeftY,value.Width,value.Height},count,known,value.MinDepth,value.MaxDepth);
        if(owned_bindings_enabled)owned_target_stats.set(canonical_identity(list),category,false,
            {value.TopLeftX,value.TopLeftY,value.Width,value.Height},count,known,value.MinDepth,value.MaxDepth);
    }
    original_viewports(list,count,values);
}
void STDMETHODCALLTYPE hook_scissors(ID3D12GraphicsCommandList* list,UINT count,const D3D12_RECT* values) {
    final_transfer_live::normalizer::bind(2,reinterpret_cast<uintptr_t>(list),count,values,stereo_publish_bypass!=0);
    final_transfer_live::bracket::consumed_scissor(reinterpret_cast<uintptr_t>(list),count,values,stereo_publish_bypass!=0);
    if(stereo_publish_bypass){original_scissors(list,count,values);return;}
    Scope scope;if(!scope.world()){auto unknown=frame_lock();frame_link.invalidate();return original_scissors(list,count,values);}auto frame_guard=frame_lock();
    if(recording.load()||(owned_bindings_enabled&&prime_recording.load())) {
        D3D12_RECT value{};unsigned category=gears_viewport::unknown;
        if(count==1&&safe_copy(&value,values,sizeof(value)))category=gears_viewport::classify(static_cast<float>(value.left),static_cast<float>(value.top),
            static_cast<float>(static_cast<int64_t>(value.right)-value.left),static_cast<float>(static_cast<int64_t>(value.bottom)-value.top));
        else if(count>1)category=gears_viewport::other;
        viewport_stats.set(reinterpret_cast<uintptr_t>(list),category,true,
            {static_cast<float>(value.left),static_cast<float>(value.top),
             static_cast<float>(static_cast<int64_t>(value.right)-value.left),
             static_cast<float>(static_cast<int64_t>(value.bottom)-value.top)});
        if(frame_link_diagnostics)frame_target_stats.set(reinterpret_cast<uintptr_t>(list),category,true,
            {static_cast<float>(value.left),static_cast<float>(value.top),
             static_cast<float>(static_cast<int64_t>(value.right)-value.left),static_cast<float>(static_cast<int64_t>(value.bottom)-value.top)},count);
        if(owned_bindings_enabled)owned_target_stats.set(canonical_identity(list),category,true,
            {static_cast<float>(value.left),static_cast<float>(value.top),
             static_cast<float>(static_cast<int64_t>(value.right)-value.left),static_cast<float>(static_cast<int64_t>(value.bottom)-value.top)},count);
    }
    original_scissors(list,count,values);
}
void observe_output_post(unsigned attempt){
    if(attempt>=output_post_snapshots.size()||!consuming_renderer)return;
    auto& sample=output_post_snapshots[attempt];sample.renderer=reinterpret_cast<uintptr_t>(consuming_renderer);
    Record record{};inspect_completed_renderer(record,consuming_renderer);
    if(!(record.valid&4096)||record.renderer_count!=2||record.family_count!=2)return;
    sample.view_count=2;
    for(unsigned eye=0;eye<2;++eye){auto& view=sample.views[eye];
        view.view=record.renderer_storage+uintptr_t(eye)*0xd30;
        if(record.family_views[eye]!=view.view)continue;
        if(safe_copy(view.source.data(),reinterpret_cast<void*>(view.view+0x48),16)&&
           safe_copy(view.target.data(),reinterpret_cast<void*>(view.view+0x58),16))view.valid|=1;
        struct Header{uintptr_t data;int count,capacity;} header{};
        if(!safe_copy(&header,reinterpret_cast<void*>(view.view+0xc84),sizeof(header)))continue;
        view.array=header.data;view.count=header.count;view.capacity=header.capacity;
        if(header.count<0||header.capacity<header.count||header.capacity>1024||
           (header.count&&(!header.data||header.data%8)))continue;
        view.valid|=2;
        for(unsigned i=0;i<std::min<unsigned>(static_cast<unsigned>(header.count),view.nodes.size());++i){auto& node=view.nodes[i];
            if(!safe_copy(&node.object,reinterpret_cast<void*>(header.data+i*8),8)||!node.object)continue;
            if(safe_copy(&node.table,reinterpret_cast<void*>(node.object),8)&&node.table&&
               safe_copy(&node.function,reinterpret_cast<void*>(node.table+8),8)&&
               safe_copy(&node.flags,reinterpret_cast<void*>(node.object+8),4))node.valid=1;
        }
    }
}
void __fastcall hook_quad(float x,float y,float width,float height,float u,float v,float uw,float vh,
    unsigned target_width,unsigned target_height,unsigned texture_width,unsigned texture_height){
    Scope scope;uintptr_t caller=reinterpret_cast<uintptr_t>(_ReturnAddress())-base;vr_dof_compat::quad(caller,scope.world());
    dlss_route_observer::quad(caller,{x,y,width,height,u,v,uw,vh},{target_width,target_height,texture_width,texture_height});
    dlss_output_control::QuadScope dlss_quad(scope.world(),caller,{x,y,width,height,u,v,uw,vh},{target_width,target_height,texture_width,texture_height});
    dlss_quad.forward(x,y,width,height,u,v,uw,vh);
    final_transfer_live::normalizer::Scope normalize(caller,{x,y,width,height,u,v,uw,vh},
        {target_width,target_height,texture_width,texture_height},scope.world());
    normalize.forward(u,v,uw,vh);
    if(!scope.world())return original_quad(x,y,width,height,u,v,uw,vh,target_width,target_height,texture_width,texture_height);
    final_transfer_live::bracket::Scope transfer_bracket(caller,{x,y,width,height,u,v,uw,vh},
        {target_width,target_height,texture_width,texture_height});
    death_constant_observer::quad(caller,{x,y,width,height,u,v,uw,vh},{target_width,target_height,texture_width,texture_height});
    death_dof_control::quad(caller,{x,y,width,height,u,v,uw,vh},{target_width,target_height,texture_width,texture_height});
    if((caller>=0xd82f58&&caller<0xd83db9)||caller==0xd808d8)observe_final_inputs();
    gears_quad_source::Args original_args{{x,y,width,height,u,v,uw,vh},{target_width,target_height,texture_width,texture_height}};
    gears_quad_source::Decision control;
    if(quad_source_control_enabled()){control=prepare_quad_source_control(original_args,caller);u=control.forwarded.rectangle[4];}
    const auto death_control=death_source_live::prepare({original_args.rectangle,original_args.dimensions},caller);
    if(death_control.decision.modified)u=death_control.decision.forwarded.rectangle[4];
    if(recording.load()&&viewport_diagnostics&&trial_draw_scope&&(self_testing||(caller>=0xd82f58&&caller<0xd83db9)||caller==0xd808d8)){
        QuadTrace value{caller,{x,y,width,height,u,v,uw,vh},{target_width,target_height,texture_width,texture_height},1};
        bool saved=false;std::lock_guard<std::mutex> lock(quad_mutex);
        for(auto& entry:quad_traces){if(!entry.calls){entry=value;saved=true;break;}
            if(entry.caller==value.caller&&entry.rectangles==value.rectangles&&entry.dimensions==value.dimensions){++entry.calls;saved=true;break;}}
        if(!saved)++quad_overflow;
    }
    bool previous=in_final_quad;in_final_quad=previous||(caller>=0xd82f58&&caller<0xd83db9)||caller==0xd808d8;
    auto saved_eye=final_eye;final_eye={};
    const auto saved_candidate=final_eye_candidate;final_eye_candidate={};
    const bool owned_plain=gears_owned_plain_output::eligible(owned_plain_output_attested,owned_bindings_enabled&&consuming_owned_scene,
        scope.world()&&consuming_frame_ticket&&consuming_frozen_frame.enabled&&consuming_frozen_frame.id==consuming_frame_ticket&&
        consuming_frozen_frame.world_generation==committed_world_generation.load()&&
        target_width==gears_native_render_size::width()&&target_height==gears_native_render_size::height(),
        dlss_quad.candidate(),caller,{x,y,width,height,u,v,uw,vh},{target_width,target_height,texture_width,texture_height});
    if(frame_link_diagnostics&&recording.load()&&(caller==0xd83b1c||caller==0xd808d8||caller==0x2b11a0||(caller==0xd83cbf&&dlss_quad.candidate())||owned_plain)) {
        final_eye_candidate={caller,consuming_frame_ticket,{x,y,width,height},target_width,target_height};
        const bool source_attested=final_transfer_live::enabled()&&u==x&&v==y&&uw==width&&vh==height&&
            texture_width==target_width&&texture_height==target_height;
        final_eye=classify_final_eye(caller,consuming_frame_ticket,x,y,width,height,target_width,target_height,source_attested,dlss_quad.candidate(),owned_plain);
        if(!final_eye.valid)++frame_counts.bad_quad;
        if(!final_eye.valid)record_final_eye_failure(1,1u|(!consuming_frame_ticket?2u:0u));
    }
    const auto saved_late=late_quad_candidate;late_quad_candidate={};
    if(frame_link_diagnostics&&recording.load()){
        late_quad_candidate={true,{x,y,width,height,u,v,uw,vh},{target_width,target_height,texture_width,texture_height},caller};
        std::lock_guard<FrameHookMutex> guard(frame_hook_mutex);++late_quad_calls;
    }
    observe_unpossessed_quad_begin();
    original_quad(x,y,width,height,u,v,uw,vh,target_width,target_height,texture_width,texture_height);
    death_source_live::complete(death_control);
    if(late_quad_candidate.active&&!late_quad_candidate.draws){
        observe_unpossessed_quad_draw(nullptr,0,0,0,0);
        death_hdr_timeline::draw(nullptr,0);
        std::lock_guard<FrameHookMutex> guard(frame_hook_mutex);++late_quad_without_draw;
    }
    late_quad_candidate=saved_late;
    final_eye=saved_eye;
    final_eye_candidate=saved_candidate;
    in_final_quad=previous;
    if(quad_source_control_enabled())complete_quad_source_control(original_args,control);
}
void observe_output_stack(ID3D12GraphicsCommandList* list,unsigned kind) {
    if(!trial_draw_scope||output_stack_attempts.load()>=64)return;
    gears_render_cost::Scope cost(gears_render_cost::output_stack);
    auto binding=viewport_stats.binding(reinterpret_cast<uintptr_t>(list));
    if(binding.viewport!=gears_viewport::full||binding.targets!=1||!binding.target.resource||
       binding.target.width!=1920||binding.target.height!=1080||binding.target.format!=24)return;
    unsigned attempt=output_stack_attempts.load();
    do {if(attempt>=64)return;}while(!output_stack_attempts.compare_exchange_weak(attempt,attempt+1));
    observe_output_post(attempt);
    OutputStack value{};value.target=binding.target;value.kind=kind;
    value.count=CaptureStackBackTrace(0,static_cast<DWORD>(value.frames.size()),value.frames.data(),nullptr);
    if(!value.count)return;
    std::lock_guard<std::mutex> lock(output_stack_mutex);
    for(auto& entry:output_stacks){
        if(!entry.count){entry=value;return;}
        if(entry.target==value.target&&entry.kind==value.kind&&entry.count==value.count&&entry.frames==value.frames)return;
    }
}
#if defined(GEARS_OWNED_UI_MATERIAL) && GEARS_OWNED_UI_MATERIAL
#include "owned_material_draw.inl"
#endif
#if defined(GEARS_GAME_UI_MATERIAL) && GEARS_GAME_UI_MATERIAL
#include "game_ui_material_draw.inl"
#endif
void STDMETHODCALLTYPE hook_draw(ID3D12GraphicsCommandList* list,UINT vertices,UINT instances,UINT start,UINT first_instance) {
    final_transfer_live::normalizer::draw_call(reinterpret_cast<uintptr_t>(list),reinterpret_cast<uintptr_t>(_ReturnAddress())-base,0,vertices,instances,stereo_publish_bypass!=0);
    final_transfer_live::bracket::draw_call(reinterpret_cast<uintptr_t>(list),reinterpret_cast<uintptr_t>(_ReturnAddress())-base,0,vertices,instances,stereo_publish_bypass!=0);
    if(stereo_publish_bypass){original_draw(list,vertices,instances,start,first_instance);return;}
    Scope scope;if(!scope.world()){auto unknown=frame_lock();frame_link.invalidate();return original_draw(list,vertices,instances,start,first_instance);}auto frame_guard=frame_lock();const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
    if(outage_diagnostics.load())outage_source_overlay={};
    observe_late_draw(list,caller,0,vertices,instances);
    observe_unpossessed_quad_draw(list,caller,0,vertices,instances);
    death_pso_cache::draw(list);
    death_hdr_timeline::draw(list,caller>=base?caller-base:caller);
#if defined(GEARS_UI_MATERIAL_ROUTE) && GEARS_UI_MATERIAL_ROUTE
    if(ui_material_context&&recording.load()){
        const auto binding=frame_target_stats.binding(reinterpret_cast<uintptr_t>(list));
        gears_frame_link::Target target{};
        if(binding.targets==1&&!binding.target.mip&&binding.target.width<=8192&&!frame_target_stats.descriptor_capacity_lost())
            target={binding.target.resource,static_cast<unsigned>(binding.target.width),binding.target.height};
        observe_ui_material_gpu(caller,frame_link.current(canonical_identity(list)),target,{false,vertices,instances,start,first_instance,0});
#if defined(GEARS_GAME_UI_MATERIAL) && GEARS_GAME_UI_MATERIAL
        if(game_material::draw(list,caller,{false,vertices,instances,start,first_instance,0}))return;
#endif
#if defined(GEARS_OWNED_UI_MATERIAL) && GEARS_OWNED_UI_MATERIAL
        if(owned_material::draw(list,caller,{false,vertices,instances,start,first_instance,0}))return;
#endif
    }
#endif
    dlss_output_control::DrawCompletion dlss_binding_completion(reinterpret_cast<uintptr_t>(list));
    if(gears_viewport::has_draw_work(vertices,instances))observe_frame_draw(list,caller,0,vertices,instances);
    if(recording.load()){gears_render_cost::Scope cost(gears_render_cost::viewport_draw);viewport_stats.draw(reinterpret_cast<uintptr_t>(list),trial_draw_scope?1:0);}
    if(recording.load())observe_output_stack(list,0);
    owned_ticket_size_trace::quad_draw(list,caller,0,vertices,instances);
    owned_ticket_size_trace::draw(list,caller,false,vertices,instances);
    geometry_draw(list,false,vertices,instances);
    observe_engine_view_draw(list,false,vertices,instances);
    {gears_render_cost::Scope cost(gears_render_cost::draw_forward);
#if defined(GEARS_OWNED_DEPTH_CLEAR_TRIAL) && GEARS_OWNED_DEPTH_CLEAR_TRIAL
        owned_depth_clear_trial::draw(list,caller,vertices,instances,start,first_instance);
#else
        original_draw(list,vertices,instances,start,first_instance);
#endif
    }
}
void STDMETHODCALLTYPE hook_draw_indexed(ID3D12GraphicsCommandList* list,UINT indices,UINT instances,UINT start,INT vertex,UINT first_instance) {
    final_transfer_live::normalizer::draw_call(reinterpret_cast<uintptr_t>(list),reinterpret_cast<uintptr_t>(_ReturnAddress())-base,1,indices,instances,stereo_publish_bypass!=0);
    final_transfer_live::bracket::draw_call(reinterpret_cast<uintptr_t>(list),reinterpret_cast<uintptr_t>(_ReturnAddress())-base,1,indices,instances,stereo_publish_bypass!=0);
    if(stereo_publish_bypass){original_draw_indexed(list,indices,instances,start,vertex,first_instance);return;}
    Scope scope;if(!scope.world()){auto unknown=frame_lock();frame_link.invalidate();return original_draw_indexed(list,indices,instances,start,vertex,first_instance);}auto frame_guard=frame_lock();const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
    if(outage_diagnostics.load())outage_source_overlay={};
    observe_late_draw(list,caller,1,indices,instances);
    observe_unpossessed_quad_draw(list,caller,1,indices,instances);
    death_pso_cache::draw(list);
    death_hdr_timeline::draw(list,caller>=base?caller-base:caller);
#if defined(GEARS_UI_MATERIAL_ROUTE) && GEARS_UI_MATERIAL_ROUTE
    if(ui_material_context&&recording.load()){
        const auto binding=frame_target_stats.binding(reinterpret_cast<uintptr_t>(list));
        gears_frame_link::Target target{};
        if(binding.targets==1&&!binding.target.mip&&binding.target.width<=8192&&!frame_target_stats.descriptor_capacity_lost())
            target={binding.target.resource,static_cast<unsigned>(binding.target.width),binding.target.height};
        observe_ui_material_gpu(caller,frame_link.current(canonical_identity(list)),target,{true,indices,instances,start,first_instance,vertex});
#if defined(GEARS_OWNED_UI_MATERIAL) && GEARS_OWNED_UI_MATERIAL
        if(ui_material_context->owned_ticket)owned_material::fail(*ui_material_context,26);
#endif
    }
#endif
    // The native segment loop issues zero draws too. Consume its ordinal before
    // any viewport/work/ticket filters; synthetic eye draws bypass this hook.
    observe_ui_index_callback(caller,{indices,instances,start,vertex,first_instance});
#if defined(GEARS_OWNED_UI_BATCH) && GEARS_OWNED_UI_BATCH
    if(recording.load()&&!gears_owned_scene_live::pipeline_failed&&owned_ui_live::route.ticket&&gears_viewport::has_draw_work(indices,instances)){
        const auto route=owned_ui_live::route;
        const auto binding=owned_target_stats.binding(canonical_identity(list));
        const auto saved=owned_target_stats.draw_state(canonical_identity(list));
        const auto ref=owned_scene_gpu::list_ref(canonical_identity(list));
        const gears_frame_link::Target target{route.resource,gears_native_render_size::width(),gears_native_render_size::height()};
        const auto pair=owned_scene_gpu::tracker.overlay_pair(ref,target);
        std::array<void*,12> stack{};std::array<uintptr_t,12> addresses{};
        const auto count=CaptureStackBackTrace(0,static_cast<DWORD>(stack.size()),stack.data(),nullptr);
        for(unsigned i=0;i<count;++i)addresses[i]=reinterpret_cast<uintptr_t>(stack[i]);
        gears_viewport::OverlayPlan plan{};
        const bool chain=ui_observed_overlay_chain(addresses.data(),count);
        const bool admitted=caller==base+0x172741&&!owned_target_stats.descriptor_capacity_lost()&&
            binding.targets==1&&!binding.target.mip&&binding.target.resource==route.resource&&
            binding.target.width==target.width&&binding.target.height==target.height&&
            pair.linked&&pair.ticket==route.ticket&&chain;
        bool reticle=false,plane=false,sample_found=false;
        gears_vr::StereoSample hud_sample{};
        auto hud_parameters=ui_hud_parameters;
        gears_ui_hud::Diagnostic hud_diagnostic{};
        if(admitted){
            reticle=prepare_ui_reticle_overlay(route.ticket,saved,target.width,target.height,plan);
            if(!reticle){
                hud_parameters=gears_ui_hud::for_source(ui_hud_parameters,ui_source_aspect());
                // Menu glyphs lack semantic sidecar labels but are still exact
                // Canvas members. Use the same ticket's head-locked panel.
                sample_found=ui_ticket_sample(route.ticket,hud_sample);
                plane=sample_found&&
                    prepare_ui_panel(route.ticket,saved,target.width,target.height,hud_sample,plan,hud_parameters,&hud_diagnostic);
            }
        }
        if(admitted&&(reticle||plane)){
            const auto* layout=plane?prepare_ui_hud_layout(saved,target.width,target.height,plan):nullptr;
            auto viewport=[&](gears_viewport::Rect r,float a,float b){D3D12_VIEWPORT v{r.x,r.y,r.width,r.height,a,b};original_viewports(list,1,&v);};
            auto scissor=[&](gears_viewport::Rect r){D3D12_RECT s{LONG(r.x),LONG(r.y),LONG(r.x+r.width),LONG(r.y+r.height)};original_scissors(list,1,&s);};
            unsigned mask{};
            try {if(layout)mask=gears_ui_layout::replay(saved,*layout,viewport,scissor,[&](unsigned offset,unsigned count){original_draw_indexed(list,count,instances,start+offset,vertex,first_instance);});
                else mask=gears_viewport::replay_overlay(saved,plan,viewport,scissor,[&](unsigned){original_draw_indexed(list,indices,instances,start,vertex,first_instance);});}
            catch(...){owned_ui_live::fault(route.ticket,10);throw;}
            if(!owned_scene_gpu::tracker.draw_overlay_pair(ref,pair.ticket,target,pair.eyes,mask)){
                owned_ui_live::fault(route.ticket,11);return;
            }
            ++owned_ui_live::route_draws;
            if(layout&&mask==3)++ui_layout_counts.applied;
            if(reticle)++owned_ui_live::route_reticle_draws;else ++owned_ui_live::route_plane_draws;
            observe_ui_gpu_payload(pair.ticket,ref,target,indices,instances,start,vertex,plane,&plan,reticle,&plan,layout&&mask==3);
            return; // Overlay work is not an extra world-geometry draw.
        }
        owned_ui_live::reject_draw({route.ticket,pair.ticket,caller-base,binding.target.resource,route.resource,
            unsigned(admitted)|(unsigned(chain)<<1)|(unsigned(pair.linked)<<2)|(unsigned(sample_found)<<3)|
            (unsigned(owned_target_stats.descriptor_capacity_lost())<<4)|(unsigned(binding.targets)<<8)|
            (unsigned(binding.target.mip)<<16),unsigned(binding.target.width),binding.target.height,saved,
            route.source_width,route.source_height,hud_parameters,sample_found,hud_sample,hud_diagnostic});
        owned_ui_live::fault(route.ticket,9);
    }
#endif
    if(frame_link_diagnostics&&stereo_gpu_publish&&recording.load()&&!frame_target_stats.descriptor_capacity_lost()&&
       !consuming_renderer&&!consuming_frame_ticket&&
       caller-base==0x172741&&gears_viewport::has_draw_work(indices,instances)){
        auto binding=frame_target_stats.binding(reinterpret_cast<uintptr_t>(list));
        auto saved=frame_target_stats.draw_state(reinterpret_cast<uintptr_t>(list));
        auto ref=frame_link.current(canonical_identity(list));
        if(binding.targets==1&&binding.target.mip==0&&binding.target.width<=8192&&
           (swapchain_bindings.known_buffer(binding.target.resource)||final_transfer_live::tracks(ref,binding.target.resource))){
            gears_frame_link::Target target{binding.target.resource,static_cast<unsigned>(binding.target.width),binding.target.height};
            auto pair=frame_link.overlay_pair(ref,target);
            auto plan=gears_viewport::overlay_plan(saved,target.width,target.height);
            std::array<void*,12> stack{};const unsigned count=pair.linked&&plan.valid?CaptureStackBackTrace(0,static_cast<DWORD>(stack.size()),stack.data(),nullptr):0;
            std::array<uintptr_t,12> addresses{};for(unsigned i=0;i<count;++i)addresses[i]=reinterpret_cast<uintptr_t>(stack[i]);
            const bool overlay_chain=pair.linked&&plan.valid&&(gears_viewport::overlay_caller_chain(addresses.data(),count,base)||
                ui_observed_overlay_chain(addresses.data(),count));
            if(outage_diagnostics.load())outage_source_overlay={1u|(pair.linked?2u:0u)|(plan.valid?4u:0u)|(overlay_chain?8u:0u),pair.rejection,pair.ticket};
            if(overlay_chain){
                const bool reticle_direction=prepare_ui_reticle_overlay(pair.ticket,saved,target.width,target.height,plan);
                const bool hud_plane=!reticle_direction&&prepare_ui_hud_overlay(pair.ticket,saved,target.width,target.height,plan);
                const auto* layout=hud_plane?prepare_ui_hud_layout(saved,target.width,target.height,plan):nullptr;
                auto viewport=[&](gears_viewport::Rect r,float minimum,float maximum){D3D12_VIEWPORT v{r.x,r.y,r.width,r.height,minimum,maximum};original_viewports(list,1,&v);};
                auto scissor=[&](gears_viewport::Rect r){D3D12_RECT s{static_cast<LONG>(r.x),static_cast<LONG>(r.y),static_cast<LONG>(r.x+r.width),static_cast<LONG>(r.y+r.height)};original_scissors(list,1,&s);};
                unsigned mask=0;
                try {if(layout)mask=gears_ui_layout::replay(saved,*layout,viewport,scissor,[&](unsigned offset,unsigned count){original_draw_indexed(list,count,instances,start+offset,vertex,first_instance);});
                    else mask=gears_viewport::replay_overlay(saved,plan,viewport,scissor,[&](unsigned){original_draw_indexed(list,indices,instances,start,vertex,first_instance);});}
                catch(...){frame_link.invalidate();throw;}
                frame_link.draw_overlay_pair(ref,pair.ticket,target,pair.eyes,mask);
                if(layout&&mask==3)++ui_layout_counts.applied;
                observe_ui_gpu_payload(pair.ticket,ref,target,indices,instances,start,vertex,hud_plane&&mask==3,hud_plane?&plan:nullptr,
                    reticle_direction&&mask==3,reticle_direction?&plan:nullptr,layout&&mask==3);
                return;
            }
        }
    }
    dlss_output_control::DrawCompletion dlss_binding_completion(reinterpret_cast<uintptr_t>(list));
    if(gears_viewport::has_draw_work(indices,instances))observe_frame_draw(list,caller,1,indices,instances);
    if(recording.load()){gears_render_cost::Scope cost(gears_render_cost::viewport_draw);viewport_stats.draw(reinterpret_cast<uintptr_t>(list),trial_draw_scope?1:0);}
    if(recording.load())observe_output_stack(list,1);
    owned_ticket_size_trace::quad_draw(list,caller,1,indices,instances);
    owned_ticket_size_trace::draw(list,caller,true,indices,instances);
    geometry_draw(list,true,indices,instances);
    observe_engine_view_draw(list,true,indices,instances);
    {gears_render_cost::Scope cost(gears_render_cost::draw_forward);original_draw_indexed(list,indices,instances,start,vertex,first_instance);}
}
void STDMETHODCALLTYPE hook_targets(ID3D12GraphicsCommandList* list,UINT count,const D3D12_CPU_DESCRIPTOR_HANDLE* handles,BOOL single,const D3D12_CPU_DESCRIPTOR_HANDLE* depth) {
    final_transfer_live::normalizer::bind(0,reinterpret_cast<uintptr_t>(list),count,handles,stereo_publish_bypass!=0);
    final_transfer_live::bracket::consumed_target(reinterpret_cast<uintptr_t>(list),count,handles,stereo_publish_bypass!=0);
    if(stereo_publish_bypass){original_targets(list,count,handles,single,depth);return;}
    Scope scope;if(!scope.world()){auto unknown=frame_lock();frame_link.invalidate();owned_ticket_size_trace::retire_attachments(reinterpret_cast<uintptr_t>(list));return original_targets(list,count,handles,single,depth);}auto frame_guard=frame_lock();
    if(recording.load()){D3D12_CPU_DESCRIPTOR_HANDLE first{};
        if(count)safe_copy(&first,handles,sizeof(first));
        viewport_stats.bind(reinterpret_cast<uintptr_t>(list),count,first.ptr);}
    const bool legacy=frame_link_diagnostics&&recording.load();
    const bool owned=owned_bindings_enabled&&(recording.load()||prime_recording.load());
    UINT attachment_stride{}; // Reuse the existing MRT bind's device query below.
    if(legacy||owned){D3D12_CPU_DESCRIPTOR_HANDLE first{};
        auto reject=[&](){if(legacy)frame_link.invalidate();
#if defined(GEARS_NATIVE_OWNED_PIXELS) && GEARS_NATIVE_OWNED_PIXELS
            if(owned)owned_scene_gpu::uncertain(owned_scene_gpu::Unknown::descriptor);
#endif
        };
        if(count&&!safe_copy(&first,handles,sizeof(first)))reject();
        if(legacy&&!frame_target_stats.bind(reinterpret_cast<uintptr_t>(list),count,first.ptr))frame_link.invalidate();
        if(owned)owned_target_stats.bind(canonical_identity(list),count,first.ptr);
        // Existing diagnostic state stores only the first RTV. Conservatively
        // mark any other verified backbuffer in an MRT bind as an unknown write,
        // so a later Draw cannot evade contamination by targeting a non-first RTV.
        // This may reject a bind without a Draw; it cannot establish a false link.
        if(count>1){
            if(count>D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT)reject();
            else {ID3D12Device* device{};UINT stride{};
                if(single){if(SUCCEEDED(list->GetDevice(IID_PPV_ARGS(&device)))){stride=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);device->Release();}
                    attachment_stride=stride;
                    if(!stride)reject();}
                for(UINT i=1;i<count;++i){D3D12_CPU_DESCRIPTOR_HANDLE handle{};
                    if(single){uint64_t offset=uint64_t(i)*stride;
                        if(!stride||offset>UINTPTR_MAX-first.ptr){reject();break;}handle.ptr=first.ptr+static_cast<uintptr_t>(offset);}
                    else if(!safe_copy(&handle,handles+i,sizeof(handle))){reject();break;}
#if defined(GEARS_NATIVE_OWNED_PIXELS) && GEARS_NATIVE_OWNED_PIXELS
                    if(owned){const auto target=owned_target_stats.lookup(handle.ptr);
                        if(!target.resource||owned_target_stats.descriptor_capacity_lost())owned_scene_gpu::uncertain(owned_scene_gpu::Unknown::descriptor);
                        else owned_scene_gpu::use(list,target.resource,false);}
#endif
                    if(!legacy)continue;
                    auto target=frame_target_stats.lookup(handle.ptr);
                    if(!target.resource){++frame_counts.missing_target;frame_link.invalidate();}
                    else if(swapchain_bindings.known_buffer(target.resource)){++frame_counts.unknown;
                        frame_link.draw_unknown(frame_link.current(canonical_identity(list)),target.resource);}
                }
            }
        }
    }
    if(legacy||owned)owned_ticket_size_trace::bind_attachments(list,count,handles,single,attachment_stride,depth);
    ngx_stereo_host::targets(list,count,handles,single,depth);
    death_hdr_timeline::targets_event(list,count,single!=FALSE,reinterpret_cast<uintptr_t>(_ReturnAddress())-base);
    original_targets(list,count,handles,single,depth);
}
#include "ngx_mv_descriptor_capture.inl"
void STDMETHODCALLTYPE hook_create_rtv(ID3D12Device* device,ID3D12Resource* resource,const D3D12_RENDER_TARGET_VIEW_DESC* view,D3D12_CPU_DESCRIPTOR_HANDLE handle) {
    if(stereo_publish_bypass){original_create_rtv(device,resource,view,handle);return;}
    Scope scope;auto frame_guard=frame_lock();original_create_rtv(device,resource,view,handle);
    if(recording.load()||prime_recording.load()){
        gears_viewport::Target target{};D3D12_RENDER_TARGET_VIEW_DESC description{};
        std::lock_guard<FrameHookMutex> diagnostic_guard(frame_hook_mutex);
        const auto descriptor_epoch=final_eye_inventory.epoch;
        bool valid=!view||safe_copy(&description,view,sizeof(description));
        unsigned mip=0;
        if(view&&valid)switch(description.ViewDimension){
            case D3D12_RTV_DIMENSION_TEXTURE1D:mip=description.Texture1D.MipSlice;break;
            case D3D12_RTV_DIMENSION_TEXTURE1DARRAY:mip=description.Texture1DArray.MipSlice;break;
            case D3D12_RTV_DIMENSION_TEXTURE2D:mip=description.Texture2D.MipSlice;break;
            case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:mip=description.Texture2DArray.MipSlice;break;
            case D3D12_RTV_DIMENSION_TEXTURE3D:mip=description.Texture3D.MipSlice;break;
            case D3D12_RTV_DIMENSION_TEXTURE2DMS:case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY:break;
            default:valid=false;
        }
        if(resource&&valid){auto desc=resource->GetDesc();
            if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER&&mip<desc.MipLevels&&mip<32)
                target={reinterpret_cast<uintptr_t>(resource),std::max<uint64_t>(1,desc.Width>>mip),
                    std::max<UINT>(1,desc.Height>>mip),static_cast<unsigned>(view?description.Format:desc.Format),mip};}
        viewport_stats.create(handle.ptr,target,descriptor_epoch,reinterpret_cast<uintptr_t>(resource));
        if(recording.load()||prime_recording.load()){auto canonical_target=target;
            canonical_target.resource=target.resource?canonical_identity(resource):0;
            frame_target_stats.create(handle.ptr,canonical_target,descriptor_epoch,reinterpret_cast<uintptr_t>(resource));
            if(owned_bindings_enabled)owned_target_stats.create(handle.ptr,canonical_target,descriptor_epoch,reinterpret_cast<uintptr_t>(resource));}
    }
}
bool flatten_descriptors(UINT count,const D3D12_CPU_DESCRIPTOR_HANDLE* starts,const UINT* sizes,UINT stride,std::array<uintptr_t,128>& output,unsigned& total) {
    total=0;if(count>output.size())return false;
    for(UINT i=0;i<count;++i){D3D12_CPU_DESCRIPTOR_HANDLE start{};UINT size=1;
        if(!safe_copy(&start,starts+i,sizeof(start))||(sizes&&!safe_copy(&size,sizes+i,sizeof(size)))||size>output.size()-total)return false;
        for(UINT j=0;j<size;++j){uint64_t offset=static_cast<uint64_t>(j)*stride;
            if(offset>UINTPTR_MAX-start.ptr)return false;output[total++]=start.ptr+static_cast<uintptr_t>(offset);}}
    return true;
}
void STDMETHODCALLTYPE hook_copy_descriptors(ID3D12Device* device,UINT destinations,const D3D12_CPU_DESCRIPTOR_HANDLE* destination_starts,const UINT* destination_sizes,
    UINT sources,const D3D12_CPU_DESCRIPTOR_HANDLE* source_starts,const UINT* source_sizes,D3D12_DESCRIPTOR_HEAP_TYPE type) {
    Scope scope;auto frame_guard=frame_lock();
#if defined(GEARS_NGX_MV_DESCRIPTOR_OBSERVER) && GEARS_NGX_MV_DESCRIPTOR_OBSERVER
    ngx_mv_descriptor_capture::Call observation(type==D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    ngx_mv_descriptor_capture::Ranges captured_dest{},captured_src{};
    if(observation){captured_dest=ngx_mv_descriptor_capture::read_ranges(destinations,destination_starts,destination_sizes);
        captured_src=ngx_mv_descriptor_capture::read_ranges(sources,source_starts,source_sizes);}
#endif
    original_copy_descriptors(device,destinations,destination_starts,destination_sizes,sources,source_starts,source_sizes,type);
#if defined(GEARS_NGX_MV_DESCRIPTOR_OBSERVER) && GEARS_NGX_MV_DESCRIPTOR_OBSERVER
    if(observation)ngx_mv_descriptor_capture::copied(device,captured_dest,captured_src,reinterpret_cast<uintptr_t>(_ReturnAddress()));
#endif
    if((recording.load()||prime_recording.load())&&type==D3D12_DESCRIPTOR_HEAP_TYPE_RTV){std::array<uintptr_t,128> dest{},src{};unsigned dc{},sc{};
        std::lock_guard<FrameHookMutex> diagnostic_guard(frame_hook_mutex);
        const auto descriptor_epoch=final_eye_inventory.epoch;
        UINT stride=device->GetDescriptorHandleIncrementSize(type);
        if(flatten_descriptors(destinations,destination_starts,destination_sizes,stride,dest,dc)&&
           flatten_descriptors(sources,source_starts,source_sizes,stride,src,sc)&&dc==sc){viewport_stats.copy(dest.data(),src.data(),dc,descriptor_epoch);frame_target_stats.copy(dest.data(),src.data(),dc,descriptor_epoch);
            if(owned_bindings_enabled)owned_target_stats.copy(dest.data(),src.data(),dc,descriptor_epoch);}
        else {viewport_stats.reject_copy();frame_target_stats.reject_copy();if(owned_bindings_enabled)owned_target_stats.reject_copy();if(frame_link_diagnostics)frame_link.invalidate();}}
}
void STDMETHODCALLTYPE hook_copy_simple(ID3D12Device* device,UINT count,D3D12_CPU_DESCRIPTOR_HANDLE dest,D3D12_CPU_DESCRIPTOR_HANDLE src,D3D12_DESCRIPTOR_HEAP_TYPE type) {
    Scope scope;auto frame_guard=frame_lock();
#if defined(GEARS_NGX_MV_DESCRIPTOR_OBSERVER) && GEARS_NGX_MV_DESCRIPTOR_OBSERVER
    ngx_mv_descriptor_capture::Call observation(type==D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
#endif
    original_copy_simple(device,count,dest,src,type);
#if defined(GEARS_NGX_MV_DESCRIPTOR_OBSERVER) && GEARS_NGX_MV_DESCRIPTOR_OBSERVER
    if(observation){
        ngx_mv_descriptor_capture::Ranges d{},s{};d.count=s.count=1;d.sizes_present=s.sizes_present=true;
        d.starts[0]=dest.ptr;s.starts[0]=src.ptr;d.sizes[0]=s.sizes[0]=count;
        ngx_mv_descriptor_capture::copied(device,d,s,reinterpret_cast<uintptr_t>(_ReturnAddress()));}
#endif
    if((recording.load()||prime_recording.load())&&type==D3D12_DESCRIPTOR_HEAP_TYPE_RTV){std::array<uintptr_t,128> destinations{},sources{};unsigned dc{},sc{};
        std::lock_guard<FrameHookMutex> diagnostic_guard(frame_hook_mutex);
        const auto descriptor_epoch=final_eye_inventory.epoch;
        UINT stride=device->GetDescriptorHandleIncrementSize(type);
        if(flatten_descriptors(1,&dest,&count,stride,destinations,dc)&&flatten_descriptors(1,&src,&count,stride,sources,sc))
            {viewport_stats.copy(destinations.data(),sources.data(),dc,descriptor_epoch);frame_target_stats.copy(destinations.data(),sources.data(),dc,descriptor_epoch);
                if(owned_bindings_enabled)owned_target_stats.copy(destinations.data(),sources.data(),dc,descriptor_epoch);}
        else {viewport_stats.reject_copy();frame_target_stats.reject_copy();if(owned_bindings_enabled)owned_target_stats.reject_copy();if(frame_link_diagnostics)frame_link.invalidate();}}
}
bool viewport_hooks(ID3D12Device* device) {
    ngx_mv_descriptor_capture::arm();
    ID3D12CommandAllocator* allocator{};ID3D12GraphicsCommandList* list{};bool success=false;
    if(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)))&&
       SUCCEEDED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator,nullptr,IID_PPV_ARGS(&list)))) {
        void** table=*reinterpret_cast<void***>(list);
        void** device_table=*reinterpret_cast<void***>(device);
        viewport_targets={table[10],table[21],table[22],table[12],table[13],table[46],device_table[20],device_table[23],device_table[24],color_copy_diagnostics?table[16]:nullptr};
#if defined(GEARS_OWNED_OUTPUT_TRACKING) && GEARS_OWNED_OUTPUT_TRACKING
        // SDK C vtable offsets are compile-time checked by D3D12AbiTest.
        // OMSetStencilRef is 24; SetPipelineState 25; ResourceBarrier 26.
        viewport_targets[10]=table[26];
#endif
        success=install(viewport_targets[0],reinterpret_cast<void*>(hook_reset_list),reinterpret_cast<void**>(&original_reset_list));
        success=install(viewport_targets[1],reinterpret_cast<void*>(hook_viewports),reinterpret_cast<void**>(&original_viewports))&&success;
        success=install(viewport_targets[2],reinterpret_cast<void*>(hook_scissors),reinterpret_cast<void**>(&original_scissors))&&success;
        success=install(viewport_targets[3],reinterpret_cast<void*>(hook_draw),reinterpret_cast<void**>(&original_draw))&&success;
        success=install(viewport_targets[4],reinterpret_cast<void*>(hook_draw_indexed),reinterpret_cast<void**>(&original_draw_indexed))&&success;
        success=install(viewport_targets[5],reinterpret_cast<void*>(hook_targets),reinterpret_cast<void**>(&original_targets))&&success;
        success=install(viewport_targets[6],reinterpret_cast<void*>(hook_create_rtv),reinterpret_cast<void**>(&original_create_rtv))&&success;
        success=install(viewport_targets[7],reinterpret_cast<void*>(hook_copy_descriptors),reinterpret_cast<void**>(&original_copy_descriptors))&&success;
        success=install(viewport_targets[8],reinterpret_cast<void*>(hook_copy_simple),reinterpret_cast<void**>(&original_copy_simple))&&success;
#if defined(GEARS_NGX_MV_DESCRIPTOR_OBSERVER) && GEARS_NGX_MV_DESCRIPTOR_OBSERVER
        viewport_targets[viewport_hook_base_capacity]=device_table[14];
        viewport_targets[viewport_hook_base_capacity+1]=device_table[17];
        viewport_targets[viewport_hook_base_capacity+2]=device_table[18];
        viewport_targets[viewport_hook_base_capacity+3]=device_table[19];
        success=install(viewport_targets[viewport_hook_base_capacity],reinterpret_cast<void*>(ngx_mv_descriptor_capture::heap),reinterpret_cast<void**>(&ngx_mv_descriptor_capture::original_heap))&&success;
        success=install(viewport_targets[viewport_hook_base_capacity+1],reinterpret_cast<void*>(ngx_mv_descriptor_capture::cbv),reinterpret_cast<void**>(&ngx_mv_descriptor_capture::original_cbv))&&success;
        success=install(viewport_targets[viewport_hook_base_capacity+2],reinterpret_cast<void*>(ngx_mv_descriptor_capture::srv),reinterpret_cast<void**>(&ngx_mv_descriptor_capture::original_srv))&&success;
        success=install(viewport_targets[viewport_hook_base_capacity+3],reinterpret_cast<void*>(ngx_mv_descriptor_capture::uav),reinterpret_cast<void**>(&ngx_mv_descriptor_capture::original_uav))&&success;
        viewport_targets[viewport_hook_base_capacity+4]=device_table[16];
        success=install(viewport_targets[viewport_hook_base_capacity+4],reinterpret_cast<void*>(ngx_root_signature_capture::create),reinterpret_cast<void**>(&ngx_root_signature_capture::original))&&success;
#endif
        if(color_copy_diagnostics)success=install(viewport_targets[9],reinterpret_cast<void*>(hook_color_copy),reinterpret_cast<void**>(&original_color_copy))&&success;
#if defined(GEARS_DEATH_PSO_CACHE) && GEARS_DEATH_PSO_CACHE
        death_pso_cache::targets={table[25],table[11],table[27]};
        success=install(death_pso_cache::targets[0],reinterpret_cast<void*>(death_pso_cache::set),reinterpret_cast<void**>(&death_pso_cache::original_set))&&success;
        success=install(death_pso_cache::targets[1],reinterpret_cast<void*>(death_pso_cache::clear),reinterpret_cast<void**>(&death_pso_cache::original_clear))&&success;
        success=install(death_pso_cache::targets[2],reinterpret_cast<void*>(death_pso_cache::bundle),reinterpret_cast<void**>(&death_pso_cache::original_bundle))&&success;
#endif
        success=ngx_mv_compute_capture::install_hooks(table)&&success;
#if defined(GEARS_OWNED_OUTPUT_TRACKING) && GEARS_OWNED_OUTPUT_TRACKING
        success=install(viewport_targets[10],reinterpret_cast<void*>(hook_resource_barrier),reinterpret_cast<void**>(&original_resource_barrier))&&success;
#endif
#if defined(GEARS_NATIVE_OWNED_PIXELS) && GEARS_NATIVE_OWNED_PIXELS
        success=owned_scene_gpu_install(device,list)&&success;
#endif
#if (defined(GEARS_NATIVE_OWNED_PIXELS) && GEARS_NATIVE_OWNED_PIXELS) || (defined(GEARS_NGX_STEREO_HOST_OBSERVER) && GEARS_NGX_STEREO_HOST_OBSERVER)
        success=command_list_birth_install(device)&&success;
#endif
        list->Close();
    }
    if(list)list->Release();if(allocator)allocator->Release();return success;
}
#include "swapchain_resize.inl"
bool present_hooks() {
    IDXGIFactory2* factory{};ID3D12Device* device{};ID3D12CommandQueue* queue{};IDXGISwapChain1* chain{};
    bool success=false;
    if(prime_ready.load()) {
        if(!install(prime_targets[3],reinterpret_cast<void*>(hook_create_swapchain),reinterpret_cast<void**>(&original_create_swapchain)) ||
           !install(prime_targets[4],reinterpret_cast<void*>(hook_create_hwnd),reinterpret_cast<void**>(&original_create_hwnd)) ||
           !install(prime_targets[5],reinterpret_cast<void*>(hook_create_composition),reinterpret_cast<void**>(&original_create_composition)))return false;
    }
    if(SUCCEEDED(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory))) &&
       SUCCEEDED(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)))) {
        D3D12_COMMAND_QUEUE_DESC q{};
        if(SUCCEEDED(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)))) {
            bool execute_ok=true;
              if(viewport_diagnostics)execute_ok=viewport_hooks(device);
            if(gpu_diagnostics) {
                execute_target=(*reinterpret_cast<void***>(queue))[10];
                  execute_ok=install(execute_target,reinterpret_cast<void*>(hook_execute),reinterpret_cast<void**>(&original_execute))&&execute_ok;
            }
            DXGI_SWAP_CHAIN_DESC1 desc{};desc.Width=16;desc.Height=16;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count=1;desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.BufferCount=2;
            desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;desc.AlphaMode=DXGI_ALPHA_MODE_PREMULTIPLIED;
            if(SUCCEEDED(factory->CreateSwapChainForComposition(queue,&desc,nullptr,&chain))) {
                void** table=*reinterpret_cast<void***>(chain);
                present_target=table[8];present1_target=table[22];
                success=install(present_target,reinterpret_cast<void*>(hook_present),reinterpret_cast<void**>(&original_present));
                if(present1_target!=present_target)
                    success=install(present1_target,reinterpret_cast<void*>(hook_present1),reinterpret_cast<void**>(&original_present1)) && success;
#if GEARS_DXGI_RESIZE_HOOKS
                success=game_resize::install_hooks(chain)&&success;
#endif
                success=success && execute_ok;
            }
        }
    }
    if(chain) chain->Release();if(queue) queue->Release();if(device) device->Release();if(factory) factory->Release();
    return success;
}
DWORD WINAPI target_prime_watchdog(void* argument) {
    ngx_root_signature_capture::register_worker(true);
    FILE* status=static_cast<FILE*>(argument);
    gears_swapchain_startup::Export startup_export;
    HANDLE waits[]{prime_event,prime_owner.handle};
    const auto deadline=GetTickCount64()+180000;
    DWORD waited{};
    for(;;){
        waited=prime_owner.handle?WaitForMultipleObjects(2,waits,FALSE,500):WaitForSingleObject(prime_event,500);
        if(waited!=WAIT_TIMEOUT)break;
        if(!prime_owner.handle&&GetTickCount64()>=deadline)break;
        write_swapchain_output_status(prime_recording.load(),"prime");
        if(gears_swapchain_startup::print(status,&startup_export))fflush(status);
        ngx_root_signature_capture::poll(); // Watchdog owns no frame/observer lock here.
    }
    {std::lock_guard<std::mutex> lock(prime_mutex);
    if(prime_handoff.load()){
        auto stats=viewport_stats.snapshot();
        fprintf(status,"handed_off creates=%llu copies=%llu evictions=%llu\n",stats.descriptor_creates,stats.descriptor_copies,stats.descriptor_evictions);
    }else{
        ngx_mv_target_capture::close();ngx_mv_descriptor_capture::close();
        prime_recording=false;bool disabled=disable_graphics_lock_prime();
        for(auto target:prime_targets){auto result=MH_DisableHook(target);disabled=disabled&&(result==MH_OK||result==MH_ERROR_DISABLED);}
        // Apply can still be inside a native collector/commit when cancellation
        // arrives. Leave time for it to return within the launcher's 15s cleanup.
        ULONGLONG deadline=GetTickCount64()+10000;while(active.load()&&GetTickCount64()<deadline)Sleep(1);
        bool restored=graphics_lock_prime_restored();std::array<unsigned char,32> bytes{};
        for(unsigned i=0;i<prime_targets.size();++i)
            restored=restored&&safe_copy(bytes.data(),prime_targets[i],bytes.size())&&!memcmp(bytes.data(),prime_original_bytes[i].data(),bytes.size());
        const char* reason=waited==WAIT_TIMEOUT?"timeout":waited==WAIT_OBJECT_0?"cancel":
            waited==WAIT_OBJECT_0+1?"owner_exited":"owner_error";
        fprintf(status,"stopped reason=%s disable_status=%u active=%u entries_match=%u\n",reason,disabled?0:1,active.load(),restored?1:0);
        print_graphics_lock_observer(status,disabled);
    }
    if(!prime_handoff.load()){
        write_swapchain_output_status(false,"stopped");
        if(!active.load())release_bindings();
    }
    }
    ngx_root_signature_capture::poll(); // Closed/drained owner cleanup left no callback lock.
    gears_swapchain_startup::print(status,&startup_export);
    ngx_mv_descriptor_capture::print(status);
    fclose(status);prime_done=true;return 0;
}
DWORD begin_target_prime(const gears_prime::OwnerArgument* owner_argument=nullptr) {
    std::lock_guard<std::mutex> lock(prime_mutex);
    if(prime_started.load()||started.load())return ERROR_ALREADY_EXISTS;
    struct FailedOwnerCleanup {~FailedOwnerCleanup(){if(!prime_ready.load()&&prime_owner.handle){
        CloseHandle(prime_owner.handle);prime_owner.handle=nullptr;prime_owner_pid=0;prime_owner_creation_time=0;}}} owner_cleanup;
    if(owner_argument&&(!gears_prime::valid(*owner_argument)||
       !prime_owner.open(owner_argument->pid,owner_argument->creation_time)))return ERROR_INVALID_PARAMETER;
    if(prime_started.exchange(1)||started.load())return ERROR_ALREADY_EXISTS;
    if(owner_argument){prime_owner_pid=owner_argument->pid;prime_owner_creation_time=owner_argument->creation_time;}
    wchar_t path[MAX_PATH];GetModuleFileNameW(own_module,path,MAX_PATH);auto slash=wcsrchr(path,L'\\');
    if(!slash)return ERROR_BAD_PATHNAME;
    wcscpy_s(slash+1,MAX_PATH-static_cast<size_t>(slash+1-path),L"target-prime-status.txt");
    FILE* status=_wfsopen(path,L"w",_SH_DENYWR);if(!status)return ERROR_WRITE_FAULT;
    ID3D12Device* device{};IDXGIFactory2* factory{};
    if(FAILED(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)))){fclose(status);return ERROR_NOT_SUPPORTED;}
    if(FAILED(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory)))){device->Release();fclose(status);return ERROR_NOT_SUPPORTED;}
    auto table=*reinterpret_cast<void***>(device);auto factory_table=*reinterpret_cast<void***>(factory);
    prime_targets={table[20],table[23],table[24],factory_table[10],factory_table[15],factory_table[24]};
#if defined(GEARS_NGX_MV_DESCRIPTOR_OBSERVER) && GEARS_NGX_MV_DESCRIPTOR_OBSERVER
    prime_targets[prime_hook_base_count]=table[14];prime_targets[prime_hook_base_count+1]=table[17];
    prime_targets[prime_hook_base_count+2]=table[18];prime_targets[prime_hook_base_count+3]=table[19];
    prime_targets[prime_hook_base_count+4]=table[16];
#endif
#if defined(GEARS_SWAPCHAIN_STARTUP_TRACE) && GEARS_SWAPCHAIN_STARTUP_TRACE
    // Discover the existing resize entrypoints before the game's first chain.
    // This private composition chain has no HWND and is released before hooks
    // are installed. The usual prime lease owns both entrypoints and originals.
    bool resize_targets_ready=false;
    {
        gears_vr::ComPtr<ID3D12CommandQueue> queue;gears_vr::ComPtr<IDXGISwapChain1> chain;
        gears_vr::ComPtr<IDXGISwapChain3> extended;D3D12_COMMAND_QUEUE_DESC queue_desc{};
        DXGI_SWAP_CHAIN_DESC1 desc{};desc.Width=16;desc.Height=16;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count=1;desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.BufferCount=2;
        desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;desc.AlphaMode=DXGI_ALPHA_MODE_PREMULTIPLIED;
        if(SUCCEEDED(device->CreateCommandQueue(&queue_desc,IID_PPV_ARGS(&queue)))&&
           SUCCEEDED(factory->CreateSwapChainForComposition(queue.Get(),&desc,nullptr,&chain))&&
           SUCCEEDED(chain->QueryInterface(IID_PPV_ARGS(&extended)))){
            prime_targets[6]=(*reinterpret_cast<void***>(chain.Get()))[13];
            prime_targets[7]=(*reinterpret_cast<void***>(extended.Get()))[39];resize_targets_ready=true;
        }
    }
    if(!resize_targets_ready){device->Release();factory->Release();fclose(status);return ERROR_NOT_SUPPORTED;}
#endif
    device->Release();factory->Release();
    std::array<void*,prime_hook_count> detours{reinterpret_cast<void*>(hook_create_rtv),reinterpret_cast<void*>(hook_copy_descriptors),reinterpret_cast<void*>(hook_copy_simple),
        reinterpret_cast<void*>(hook_create_swapchain),reinterpret_cast<void*>(hook_create_hwnd),reinterpret_cast<void*>(hook_create_composition)};
    std::array<void**,prime_hook_count> originals{reinterpret_cast<void**>(&original_create_rtv),reinterpret_cast<void**>(&original_copy_descriptors),reinterpret_cast<void**>(&original_copy_simple),
        reinterpret_cast<void**>(&original_create_swapchain),reinterpret_cast<void**>(&original_create_hwnd),reinterpret_cast<void**>(&original_create_composition)};
#if defined(GEARS_NGX_MV_DESCRIPTOR_OBSERVER) && GEARS_NGX_MV_DESCRIPTOR_OBSERVER
    detours[prime_hook_base_count]=reinterpret_cast<void*>(ngx_mv_descriptor_capture::heap);
    detours[prime_hook_base_count+1]=reinterpret_cast<void*>(ngx_mv_descriptor_capture::cbv);
    detours[prime_hook_base_count+2]=reinterpret_cast<void*>(ngx_mv_descriptor_capture::srv);
    detours[prime_hook_base_count+3]=reinterpret_cast<void*>(ngx_mv_descriptor_capture::uav);
    detours[prime_hook_base_count+4]=reinterpret_cast<void*>(ngx_root_signature_capture::create);
    originals[prime_hook_base_count]=reinterpret_cast<void**>(&ngx_mv_descriptor_capture::original_heap);
    originals[prime_hook_base_count+1]=reinterpret_cast<void**>(&ngx_mv_descriptor_capture::original_cbv);
    originals[prime_hook_base_count+2]=reinterpret_cast<void**>(&ngx_mv_descriptor_capture::original_srv);
    originals[prime_hook_base_count+3]=reinterpret_cast<void**>(&ngx_mv_descriptor_capture::original_uav);
    originals[prime_hook_base_count+4]=reinterpret_cast<void**>(&ngx_root_signature_capture::original);
#endif
#if defined(GEARS_SWAPCHAIN_STARTUP_TRACE) && GEARS_SWAPCHAIN_STARTUP_TRACE
    detours[6]=reinterpret_cast<void*>(game_resize::hook);detours[7]=reinterpret_cast<void*>(game_resize::hook1);
    originals[6]=reinterpret_cast<void**>(&game_resize::original);originals[7]=reinterpret_cast<void**>(&game_resize::original1);
#endif
    bool ok=true;
    for(unsigned i=0;i<prime_targets.size();++i){
        if(!safe_copy(prime_original_bytes[i].data(),prime_targets[i],32)){ok=false;break;}
        auto created=MH_CreateHook(prime_targets[i],detours[i],originals[i]);
        auto queued=created==MH_OK?MH_QueueEnableHook(prime_targets[i]):MH_ERROR_NOT_CREATED;
        fprintf(status,"hook_target=%p create=%s queue=%s\n",prime_targets[i],MH_StatusToString(created),MH_StatusToString(queued));
        fprintf(status,"prime_entry target=%p before=",prime_targets[i]);
        for(auto byte:prime_original_bytes[i])fprintf(status,"%02x",byte);fprintf(status,"\n");
        if(created!=MH_OK||queued!=MH_OK){ok=false;break;}
    }
    wchar_t name[128];swprintf_s(name,L"Local\\GearsVR.Capture.TargetPrime.%lu",GetCurrentProcessId());
    if(ok)prime_event=CreateEventW(nullptr,TRUE,FALSE,name);
    ok=ok&&prime_event;
    if(ok&&prime_owner.handle)ok=prime_owner.state()==gears_session::OwnerState::alive;
    if(ok)ok=begin_graphics_lock_prime(status,prime_owner.handle!=nullptr);
    if(ok){ngx_mv_descriptor_capture::arm();prime_recording=true;ok=MH_ApplyQueued()==MH_OK;}
    HANDLE thread{};
    if(ok){thread=CreateThread(nullptr,0,target_prime_watchdog,status,0,nullptr);ok=thread!=nullptr;}
    if(!ok){ngx_mv_target_capture::close();ngx_mv_descriptor_capture::close();prime_recording=false;disable_graphics_lock_prime();for(auto target:prime_targets){MH_QueueDisableHook(target);MH_DisableHook(target);}ngx_mv_descriptor_capture::release_drained();ngx_mv_descriptor_capture::print(status);fprintf(status,"prime_setup_failed\n");fclose(status);return ERROR_INVALID_STATE;}
    if(prime_owner.handle)fprintf(status,"capturing pid=%lu lifetime=owner owner_pid=%lu owner_creation_time=%llu native_hooks_enabled=%u graphics_hooks_enabled=%u scene_hooks_enabled=0 descriptor_hooks_enabled=%u factory_hooks_enabled=3\n",GetCurrentProcessId(),prime_owner_pid,static_cast<unsigned long long>(prime_owner_creation_time),graphics_lock_prime_count(),graphics_lock_prime_count(),3+ngx_mv_descriptor_hooks);
    else fprintf(status,"capturing pid=%lu seconds=180 native_hooks_enabled=0 descriptor_hooks_enabled=%u factory_hooks_enabled=3\n",GetCurrentProcessId(),3+ngx_mv_descriptor_hooks);fflush(status);
    prime_ready=true;CloseHandle(thread);return 0;
}
void reset_world_thread_context(){
    // Called only by the next admitted outer Scope on its own thread. No old
    // FamilyScope/Restore object can still be alive at this epoch boundary.
    const auto sequence=anchor_context.sequence;anchor_context={};anchor_context.sequence=sequence;
    anchor_last_state_pawn={};anchor_last_observed_right_state=0;
    submitting_family=nullptr;consuming_renderer=nullptr;consuming_owned_scene=false;
    consuming_frame_ticket=0;consuming_frozen_frame={};final_eye={};final_eye_candidate={};late_quad_candidate={};outage_source_overlay={};
    unsubmitted_renderer=false;pair_canvas=pair_transform=nullptr;trial_draw_scope=false;
    scene_local=nullptr;scene_rotation=nullptr;scene_delta=0;scene_phase=0;
    scene_translation={};scene_rotation_delta={};observed_stamp=0;stamp_known=false;
    reset_ui_world_tls();
}
DWORD WINAPI worker(void*) {
    desktop_mirror_readback::start_worker();
    ngx_mv_compute_capture::register_worker();
    ngx_root_signature_capture::register_worker();
    // GearsCaptureStart's one-shot started flag forbids reopening this gate.
    struct ActivationExit {~ActivationExit(){session_activation_gate.close();}} activation_exit;
    gears_session::Owner continuous_owner; // Retain the process object through all cleanup and early returns.
    wchar_t ini[MAX_PATH],csv[MAX_PATH],status_path[MAX_PATH];
    path_for(ini,L"capture.ini");path_for(csv,L"capture.csv");path_for(status_path,L"capture-status.txt");
    FILE* status=_wfsopen(status_path,L"w",_SH_DENYWR);
    if(!status) return 1;
    diagnostic_status=status;
    if(!hash_matches() || ini_address(ini,L"pid")!=GetCurrentProcessId()) {
        fprintf(status,"rejected: executable hash/name or PID mismatch\n");fclose(status);return 2;
    }
    const auto current_base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    if(reservation_ready&&base!=current_base){fprintf(status,"rejected: reserved module base changed\n");fclose(status);return 2;}
    if(!reservation_ready)base=current_base; // Startup settings hooks read the immutable reserved base.
    initialize_native_owned_probe(true); // hash_matches/PID gate above passed; no probe allocations here.
    initialize_native_owned_scene(true);
    quad_target=reinterpret_cast<void*>(base+0x1af42c);
    geometry_target=reinterpret_cast<void*>(base+0x13cfcc);
    expected_local=ini_address(ini,L"local_player");expected_pc=ini_address(ini,L"controller");
    camera_cache=ini_address(ini,L"timestamp");
    expected_camera=ini_address(ini,L"camera");camera_yaw=ini_address(ini,L"camera_yaw");
    wobble_enabled=GetPrivateProfileIntW(L"Capture",L"wobble",0,ini)==1;
    sweep_enabled=GetPrivateProfileIntW(L"Capture",L"sweep",0,ini)==1;
    live_enabled=GetPrivateProfileIntW(L"Capture",L"live_head",0,ini)==1;
    gpu_diagnostics=GetPrivateProfileIntW(L"Capture",L"gpu_diagnostics",0,ini)==1;
      viewport_diagnostics=GetPrivateProfileIntW(L"Capture",L"viewport_diagnostics",0,ini)==1;
      if(viewport_diagnostics&&!gpu_diagnostics){fclose(status);return 4;}
    family_diagnostics=GetPrivateProfileIntW(L"Capture",L"family_diagnostics",0,ini)==1;
    shadow_view_enabled=GetPrivateProfileIntW(L"Capture",L"shadow_view",0,ini)==1;
    constructor_shadow_enabled=GetPrivateProfileIntW(L"Capture",L"shadow_constructor",0,ini)==1;
    constructor_state_enabled=GetPrivateProfileIntW(L"Capture",L"constructor_state",0,ini)==1;
    constructor_eye_enabled=GetPrivateProfileIntW(L"Capture",L"constructor_eye",0,ini)==1;
    renderer_pair_enabled=GetPrivateProfileIntW(L"Capture",L"renderer_pair",0,ini)==1;
    submit_pair_enabled=GetPrivateProfileIntW(L"Capture",L"submit_pair",0,ini)==1;
    submit_output_bounds=GetPrivateProfileIntW(L"Capture",L"submit_output_bounds",0,ini)==1;
    submit_mono_control=GetPrivateProfileIntW(L"Capture",L"submit_mono_control",0,ini)==1;
    submit_mono_independent_state=GetPrivateProfileIntW(L"Capture",L"submit_mono_independent_state",0,ini)==1;
    if(submit_mono_independent_state && !submit_mono_control)return 4;
    if(submit_mono_control && submit_output_bounds)return 4;
    submit_duplicate_control=GetPrivateProfileIntW(L"Capture",L"submit_duplicate_control",0,ini)==1;
    if(submit_duplicate_control && (submit_mono_control || submit_output_bounds))return 4;
    submit_pair_seconds=GetPrivateProfileIntW(L"Capture",L"submit_pair_seconds",0,ini);
    continuous_session=GetPrivateProfileIntW(L"Capture",L"continuous_session",0,ini)==1;
    if(continuous_session){
        wchar_t owner_pid[64]{},owner_created[64]{};std::uint64_t parsed_pid{},parsed_created{};
        GetPrivateProfileStringW(L"Capture",L"continuous_owner_pid",L"",owner_pid,64,ini);
        GetPrivateProfileStringW(L"Capture",L"continuous_owner_creation_time",L"",owner_created,64,ini);
        if(!gears_session::decimal(owner_pid,MAXDWORD,parsed_pid)||
           !gears_session::decimal(owner_created,(std::numeric_limits<std::uint64_t>::max)(),parsed_created)||
           !continuous_owner.open(static_cast<DWORD>(parsed_pid),parsed_created)){
            if(prime_owner.handle)fprintf(status,"rejected: continuous owner identity invalid native_hooks_enabled=0 capture_activated=0 prime_watchdog_retained=%u prime_watchdog_lifetime=owner owner_pid=%lu owner_creation_time=%llu\n",prime_ready.load()?1:0,prime_owner_pid,static_cast<unsigned long long>(prime_owner_creation_time));
            else fprintf(status,"rejected: continuous owner identity invalid native_hooks_enabled=0 capture_activated=0 prime_watchdog_retained=%u prime_watchdog_seconds=180\n",prime_ready.load()?1:0);
            fclose(status);return 2;
        }
        session_owner_pid=static_cast<DWORD>(parsed_pid);session_owner_creation_time=parsed_created;
        fprintf(status,"continuous_owner_verified=1 owner_pid=%lu owner_creation_time=%llu\n",session_owner_pid,
            static_cast<unsigned long long>(session_owner_creation_time));
    }
    trial_seconds=GetPrivateProfileIntW(L"Capture",L"trial_seconds",30,ini);
    submit_background=GetPrivateProfileIntW(L"Capture",L"submit_background",0,ini)!=0;
    quad_left_source_control=GetPrivateProfileIntW(L"Capture",L"quad_left_source_control",0,ini)==1;
    quad_right_source_control=GetPrivateProfileIntW(L"Capture",L"quad_right_source_control",0,ini)==1;
    color_copy_diagnostics=GetPrivateProfileIntW(L"Capture",L"color_copy_diagnostics",0,ini)==1;
    first_view_mask_control=GetPrivateProfileIntW(L"Capture",L"first_view_mask_control",0,ini)==1;
    view_union_mask_fix=GetPrivateProfileIntW(L"Capture",L"view_union_mask_fix",0,ini)==1;
    projection_control=GetPrivateProfileIntW(L"Capture",L"projection_control",0,ini)==1;
    stereo_xr=GetPrivateProfileIntW(L"Capture",L"stereo_xr",0,ini)==1;
    simulator_operator=GetPrivateProfileIntW(L"Capture",L"simulator_operator",0,ini)==1;
    frame_link_diagnostics=GetPrivateProfileIntW(L"Capture",L"frame_link_diagnostics",0,ini)==1;
    stereo_gpu_publish=GetPrivateProfileIntW(L"Capture",L"stereo_gpu_publish",0,ini)==1;
    anchor_observer_enabled=GetPrivateProfileIntW(L"Capture",L"anchor_observer",0,ini)==1;
    if(anchor_observer_enabled&&(!family_diagnostics||!stereo_xr||!frame_link_diagnostics||!load_anchor_config(ini))){
        fprintf(status,"rejected: anchor observer requires stereo frame-link family diagnostics and fresh reflection certificate\n");fclose(status);return 2;}
    ui_observer_enabled=GetPrivateProfileIntW(L"Capture",L"ui_observer",0,ini)==1;
    if(ui_observer_enabled&&(!ui_reserved||!continuous_session||!anchor_observer_enabled||!load_ui_config(ini))){
        fprintf(status,"rejected: UI observer requires reserved entries, continuous stereo and fresh UI identities\n");fclose(status);return 2;}
    crosshair_live::enabled=GetPrivateProfileIntW(L"Capture",L"crosshair_assignment_observer",0,ini)==1;
    if(crosshair_live::enabled&&(!ui_observer_enabled||!anchor_observer_enabled||!continuous_session||
       GetPrivateProfileIntW(L"Capture",L"origin_trial",0,ini)!=0||!crosshair_live::load(ini,status))){
        fprintf(status,"rejected: crosshair observer requires compiled support and fresh assignment/tick bindings\n");fclose(status);return 2;}
    ui_hud_plane_enabled=GetPrivateProfileIntW(L"Capture",L"ui_hud_plane",0,ini)==1;
    if(!ui_reticle_late_sample::configure(GetPrivateProfileIntW(L"Capture",L"ui_reticle_late_sample",0,ini)==1)){
        fprintf(status,"rejected: late reticle sampling is not compiled\n");fclose(status);return 2;}
    if(ui_reticle_late_sample::enabled&&(!ui_observer_enabled||!ui_hud_plane_enabled||!continuous_session||!ui_reticle_direction_enabled)){
        fprintf(status,"rejected: late reticle sampling requires continuous UI/HUD observers and compiled reticle direction\n");fclose(status);return 2;}
    ui_hud_parameters=gears_ui_hud::Parameters{};
    ui_hud_layout_parameters=gears_ui_layout::Parameters{};
    const wchar_t* layout_keys[]{L"ui_hud_element_scale",L"ui_hud_center_scale"};
    double* layout_values[]{&ui_hud_layout_parameters.element_scale,&ui_hud_layout_parameters.center_scale};
    for(unsigned i=0;i<2;++i){wchar_t value[64]{},presence[64]{};
        const auto length=GetPrivateProfileStringW(L"Capture",layout_keys[i],L"__absent__",value,64,ini);
        GetPrivateProfileStringW(L"Capture",layout_keys[i],L"__missing__",presence,64,ini);
        const bool explicit_value=std::wcscmp(value,L"__absent__")!=0||std::wcscmp(presence,L"__missing__")!=0;
        if(explicit_value&&(!ui_hud_plane_enabled||length>=63||
           !gears_ui_hud::parse_bounded_decimal(value,i?0.:1.,i?1.:2.,*layout_values[i]))){
            fprintf(status,"rejected: HUD layout requires enabled HUD plane, element scale 1..2 and center scale 0..1\n");fclose(status);return 2;}
    }
    wchar_t hud_fov_text[64]{};
    const auto hud_fov_length=GetPrivateProfileStringW(L"Capture",L"ui_hud_horizontal_fov_degrees",L"__absent__",hud_fov_text,64,ini);
    // A literal sentinel must still be rejected as an explicit invalid value.
    wchar_t hud_fov_presence[64]{};
    GetPrivateProfileStringW(L"Capture",L"ui_hud_horizontal_fov_degrees",L"__missing__",hud_fov_presence,64,ini);
    const bool hud_fov_explicit=std::wcscmp(hud_fov_text,L"__absent__")!=0||std::wcscmp(hud_fov_presence,L"__missing__")!=0;
    if(hud_fov_explicit&&(!ui_hud_plane_enabled||hud_fov_length>=63||
       !gears_ui_hud::parse_horizontal_fov(hud_fov_text,ui_hud_parameters.horizontal_fov_degrees))){
        fprintf(status,"rejected: HUD horizontal FOV requires enabled HUD plane and a finite decimal in 45..100\n");fclose(status);return 2;}
    if(ui_hud_plane_enabled&&(!ui_observer_enabled||!continuous_session||!stereo_gpu_publish)){
        fprintf(status,"rejected: HUD plane requires continuous stereo publication and authenticated UI ranges\n");fclose(status);return 2;}
    origin_trial_enabled=GetPrivateProfileIntW(L"Capture",L"origin_trial",0,ini)==1;
    if(origin_trial_enabled&&(!anchor_observer_enabled||!continuous_session||!stereo_gpu_publish)){
        fprintf(status,"rejected: origin trial requires continuous stereo GPU and exact anchor observer\n");fclose(status);return 2;}
    outage_diagnostics=GetPrivateProfileIntW(L"Capture",L"outage_diagnostics",0,ini)==1;
    if(outage_diagnostics.load()&&(!stereo_xr||!stereo_gpu_publish||!frame_link_diagnostics)){
        fprintf(status,"rejected: outage diagnostics requires stereo XR GPU frame-link\n");fclose(status);return 2;}
    if(continuous_session&&(!stereo_xr||!stereo_gpu_publish||!frame_link_diagnostics||!view_union_mask_fix||
       submit_pair_seconds||projection_control||simulator_operator||submit_mono_control||submit_mono_independent_state||
       submit_duplicate_control||quad_source_control_enabled()||first_view_mask_control)){
        fprintf(status,"rejected: continuous session requires exclusive real stereo GPU union with pair seconds zero\n");fclose(status);return 2;}
    if(simulator_operator&&(!stereo_xr||!stereo_gpu_publish||!frame_link_diagnostics||!view_union_mask_fix)){
        fprintf(status,"rejected: simulator operator requires stereo XR, GPU publication, frame link and validated union\n");fclose(status);return 2;}
    if((trial_seconds!=30&&trial_seconds!=120)||(trial_seconds==120&&!view_union_mask_fix)){
        fprintf(status,"rejected: trial seconds must be 30 or 120; 120 requires validated view union\n");fclose(status);return 2;}
    if(stereo_gpu_publish&&(!stereo_xr||!frame_link_diagnostics||!view_union_mask_fix||(!continuous_session&&submit_pair_seconds!=trial_seconds))){
        fprintf(status,"rejected: stereo GPU publish requires XR, frame link and validated bounded union\n");fclose(status);return 2;}
    if(frame_link_diagnostics&&(!view_union_mask_fix||!prime_ready.load()||prime_done.load()||(!continuous_session&&submit_pair_seconds!=trial_seconds)||!submit_pair_enabled||!submit_output_bounds)) {
        fprintf(status,"rejected: frame link requires bounded view union pair and active target/factory prime\n");fclose(status);return 2;
    }
    {wchar_t value[64]{};GetPrivateProfileStringW(L"Capture",L"camera_closer_metres",L"0",value,64,ini);
        wchar_t* end{};camera_closer_metres=wcstod(value,&end);
        if(end==value||*end||!gears_camera_closer::valid(camera_closer_metres)||
           (camera_closer_metres>0&&(!stereo_xr||!continuous_session||!anchor_observer_enabled||!anchor_config.cinematic_enabled||origin_trial_enabled))){
            fprintf(status,"rejected: camera closer requires 0-0.5 metres and stereo continuous cinematic/anchor guards, without origin trial\n");fclose(status);return 2;}}
    if(stereo_xr){wchar_t scale[64]{};GetPrivateProfileStringW(L"Capture",L"stereo_units_per_metre",L"100",scale,64,ini);
        wchar_t* end{};stereo_units_per_metre=wcstod(scale,&end);
        if(end==scale || *end || !std::isfinite(stereo_units_per_metre) || stereo_units_per_metre<10 || stereo_units_per_metre>500 ||
           !view_union_mask_fix || projection_control || live_enabled){fprintf(status,"rejected: stereo XR requires exclusive view union pair and valid scale\n");fclose(status);return 2;}}
    if(projection_control && (!view_union_mask_fix||trial_seconds!=30)){fprintf(status,"rejected: projection control requires thirty-second view union mask fix\n");fclose(status);return 2;}
    if(view_union_mask_fix&&(!submit_pair_enabled||!submit_output_bounds||!viewport_diagnostics||(!continuous_session&&submit_pair_seconds!=trial_seconds)||
       submit_mono_control||submit_mono_independent_state||submit_duplicate_control||quad_source_control_enabled()||first_view_mask_control)){
        fprintf(status,"rejected: view union mask fix requires exclusive bounded split pair with output bounds and viewport diagnostics\n");fclose(status);return 2;
    }
    if(first_view_mask_control&&(!submit_pair_enabled||!submit_output_bounds||!viewport_diagnostics||submit_pair_seconds!=10||
       submit_mono_control||submit_mono_independent_state||submit_duplicate_control||quad_source_control_enabled())){
        fprintf(status,"rejected: first view mask control requires exclusive ten-second split pair with output bounds and viewport diagnostics\n");fclose(status);return 2;
    }
    if(first_view_mask_hook_enabled()&&reservation_ready&&!first_view_mask_reserved){
        fprintf(status,"rejected: first view mask trampoline was not reserved at startup\n");fclose(status);return 2;
    }
    if(color_copy_diagnostics&&(!submit_pair_enabled||!viewport_diagnostics||quad_source_control_enabled())){
        fprintf(status,"rejected: color copy diagnostics require viewport pair without quad source control\n");fclose(status);return 2;
    }
    if(quad_left_source_control&&quad_right_source_control){
        fprintf(status,"rejected: quad source control modes are mutually exclusive\n");fclose(status);return 2;
    }
    if(quad_source_control_enabled()&&(!submit_pair_enabled||!submit_output_bounds||!viewport_diagnostics||submit_pair_seconds!=10||
       submit_mono_control||submit_mono_independent_state||submit_duplicate_control)){
        fprintf(status,"rejected: quad source control requires exclusive ten-second split pair with output bounds and viewport diagnostics\n");fclose(status);return 2;
    }
    if(submit_background && !continuous_session && (!submit_pair_enabled||!viewport_diagnostics||!submit_pair_seconds||
       (submit_pair_seconds>10&&!(view_union_mask_fix&&submit_pair_seconds==trial_seconds)))){
        fprintf(status,"rejected: background mode requires one-to-ten-second viewport pair diagnostics or validated bounded view union fix\n");fclose(status);return 2;
    }
    if(submit_pair_seconds>60&&!(trial_seconds==120&&view_union_mask_fix&&submit_pair_seconds==120))return 4;
    if(submit_pair_enabled && (!constructor_eye_enabled || renderer_pair_enabled)){fprintf(status,"rejected: submit pair requires exclusive shifted-eye mode\n");fclose(status);return 3;}
    if(renderer_pair_enabled && !constructor_eye_enabled){fprintf(status,"rejected: renderer pair requires independent shifted eye\n");fclose(status);return 3;}
    if(constructor_eye_enabled && !constructor_state_enabled){fprintf(status,"rejected: eye requires independent constructor state\n");fclose(status);return 3;}
    if(constructor_state_enabled && !constructor_shadow_enabled){fprintf(status,"rejected: independent constructor state requires full constructor mode\n");fclose(status);return 3;}
    if(constructor_shadow_enabled && !shadow_view_enabled){fprintf(status,"rejected: constructor requires shadow view mode\n");fclose(status);return 3;}
    shadow_state_enabled=GetPrivateProfileIntW(L"Capture",L"shadow_state",0,ini)==1;
    if(shadow_state_enabled && (!family_diagnostics || (shadow_view_enabled && !constructor_state_enabled))){fprintf(status,"rejected: shadow state requires exclusive family diagnostics\n");fclose(status);return 3;}
    if(constructor_state_enabled && !shadow_state_enabled){fprintf(status,"rejected: independent constructor state requires lifetime observation\n");fclose(status);return 3;}
    if(shadow_view_enabled && !family_diagnostics){fprintf(status,"rejected: shadow copy requires family diagnostics\n");fclose(status);return 3;}
    unsigned observer_overlap=viewport_diagnostics&&submit_pair_enabled&&gpu_diagnostics&&family_diagnostics?1:0;
    if(int(wobble_enabled)+int(sweep_enabled)+int(live_enabled)+int(gpu_diagnostics)+int(family_diagnostics)-observer_overlap>1) {fprintf(status,"rejected: mutually exclusive capture modes\n");fclose(status);return 3;}
    if(live_enabled) {
        if(!live_bridge.open(GetCurrentProcessId(),false)){fprintf(status,"rejected: pose producer unavailable\n");fclose(status);return 3;}
        live_gate.arm(true);
    }
    if(stereo_xr && !stereo_bridge.open(GetCurrentProcessId(),false)){fprintf(status,"rejected: stereo XR producer unavailable\n");fclose(status);return 3;}
    uintptr_t linked{};
    if(!expected_local || !expected_pc || !safe_copy(&linked,reinterpret_cast<void*>(expected_local+0x68),8) || linked!=expected_pc) {
        fprintf(status,"rejected: local player/controller link mismatch\n");fclose(status);return 3;
    }
    scene_target=reinterpret_cast<void*>(base+0x15cf08);event_target=reinterpret_cast<void*>(base+0x15c968);
    const unsigned char scene_bytes[]={0x48,0x8b,0xc4,0x53,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57};
    const unsigned char event_bytes[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x08,0x48,0x89,0x68,0x10,0x48,0x89,0x70,0x18};
    unsigned char bytes[16]{};
    if(first_view_mask_hook_enabled()){
        first_view_mask_target=reinterpret_cast<void*>(base+0x2a7e48);
        if(!safe_copy(bytes,first_view_mask_target,sizeof(first_view_mask_signature))||memcmp(bytes,first_view_mask_signature,sizeof(first_view_mask_signature))||
           !safe_copy(bytes,reinterpret_cast<void*>(base+0x119bdd),sizeof(first_view_mask_call_signature))||memcmp(bytes,first_view_mask_call_signature,sizeof(first_view_mask_call_signature))){
            fprintf(status,"rejected: first view mask entry/call signature mismatch\n");fclose(status);return 4;
        }
    }
    if(!safe_copy(bytes,scene_target,sizeof(scene_bytes)) || memcmp(bytes,scene_bytes,sizeof(scene_bytes)) ||
       !safe_copy(bytes,event_target,sizeof(event_bytes)) || memcmp(bytes,event_bytes,sizeof(event_bytes))) {
        fprintf(status,"rejected: candidate prologue mismatch\n");fclose(status);return 4;
    }
    if(family_diagnostics){
        family_target=reinterpret_cast<void*>(base+0x3a148c);
        const unsigned char signature[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x18,0x55};
        if(!safe_copy(bytes,family_target,sizeof(signature)) || memcmp(bytes,signature,sizeof(signature))){fprintf(status,"rejected: family candidate prologue mismatch\n");fclose(status);return 4;}
        clone_target=reinterpret_cast<void*>(base+0x193d50);
        const unsigned char clone_signature[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x10,0x48,0x89,0x68,0x18,0x48,0x89,0x70,0x20};
        if(!safe_copy(bytes,clone_target,sizeof(clone_signature)) || memcmp(bytes,clone_signature,sizeof(clone_signature))){fprintf(status,"rejected: clone candidate prologue mismatch\n");fclose(status);return 4;}
        renderer_target=reinterpret_cast<void*>(base+0x116250);
        const unsigned char signature_renderer[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x18,0x48,0x89,0x70,0x20,0x57,0x41,0x54};
        if(!safe_copy(bytes,renderer_target,sizeof(signature_renderer)) || memcmp(bytes,signature_renderer,sizeof(signature_renderer))){fprintf(status,"rejected: renderer prologue mismatch\n");fclose(status);return 4;}
        consume_target=reinterpret_cast<void*>(base+0x15a80c);destroy_renderer_target=reinterpret_cast<void*>(base+0x15a974);
        const unsigned char signature_consume[]={0x48,0x89,0x5c,0x24,0x10,0x55,0x48,0x8b,0xec,0x48,0x83,0xec,0x70};
        const unsigned char signature_destroy[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xd9};
        if(!safe_copy(bytes,consume_target,sizeof(signature_consume)) || memcmp(bytes,signature_consume,sizeof(signature_consume)) ||
           !safe_copy(bytes,destroy_renderer_target,sizeof(signature_destroy)) || memcmp(bytes,signature_destroy,sizeof(signature_destroy))){fprintf(status,"rejected: consume/destruction prologue mismatch\n");fclose(status);return 4;}
        prepare_target=reinterpret_cast<void*>(base+0x1fc060);
        const unsigned char signature_prepare[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x10,0x48,0x89,0x70,0x18,0x57,0x41,0x54};
        if(!safe_copy(bytes,prepare_target,sizeof(signature_prepare)) || memcmp(bytes,signature_prepare,sizeof(signature_prepare))){fprintf(status,"rejected: preparation prologue mismatch\n");fclose(status);return 4;}
    }
    if(viewport_diagnostics && (!safe_copy(bytes,geometry_target,sizeof(geometry_signature))||memcmp(bytes,geometry_signature,sizeof(geometry_signature)))){fprintf(status,"rejected: geometry prologue mismatch\n");fclose(status);return 4;}
    if(viewport_diagnostics){const unsigned char signature_quad[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x08,0x48,0x89,0x70,0x10,0x48,0x89,0x78,0x18};
        if(!safe_copy(bytes,quad_target,sizeof(signature_quad))||memcmp(bytes,signature_quad,sizeof(signature_quad))){fprintf(status,"rejected: quad prologue mismatch\n");fclose(status);return 4;}}
    if(shadow_view_enabled) {
        struct Gate {uintptr_t rva;unsigned char bytes[16];unsigned size;};
        const Gate gates[]={
            {0x1c03c8,{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57},16},
            {0x194454,{0x48,0x89,0x5c,0x24,0x18,0x48,0x89,0x4c,0x24,0x08,0x56,0x57,0x41,0x56,0x48,0x83},16},
            {0x15ca7c,{0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0xf9,0x48,0x8b,0x89},16},
            {0x53ea50,{0xe9,0xab,0xe8,0xb9,0xff},5}};
        for(const auto& gate:gates)if(!safe_copy(bytes,reinterpret_cast<void*>(base+gate.rva),gate.size) || memcmp(bytes,gate.bytes,gate.size)) {
            fprintf(status,"rejected: source copy lifetime signature mismatch rva=0x%llx\n",static_cast<unsigned long long>(gate.rva));fclose(status);return 4;
        }
        shadow_api={reinterpret_cast<gears_shadow::Allocate>(base+0x1c03c8),reinterpret_cast<gears_shadow::Copy>(base+0x194454),
            reinterpret_cast<gears_shadow::Destroy>(base+0x15ca7c),reinterpret_cast<gears_shadow::Free>(base+0x53ea50),safe_copy};
        if(constructor_shadow_enabled) {
            const unsigned char signature[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x10,0x48,0x89,0x70,0x18,0x48,0x89,0x78,0x20};
            source_constructor=reinterpret_cast<gears_source_args::Constructor>(base+0x15e428);
            if(!safe_copy(bytes,reinterpret_cast<void*>(source_constructor),sizeof(signature)) || memcmp(bytes,signature,sizeof(signature))) {
                fprintf(status,"rejected: full source constructor signature mismatch\n");fclose(status);return 4;
            }
            shadow_api.copy=construct_from_snapshot;
        }
    }
    if(shadow_state_enabled) {
        const unsigned char factory_bytes[]={0x48,0x83,0xec,0x28,0xba,0x08,0,0,0,0xb9,0x80,0x02,0,0};
        const unsigned char release_bytes[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xc1,0x48,0x8d,0x51,0x1c,0x48,0x8b,0xd9};
        const unsigned char delete_bytes[]={0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x8b,0xda,0x48,0x8b,0xf9};
        state_factory=reinterpret_cast<StateFactoryFn>(base+0x33bdd4);state_release=reinterpret_cast<ConsumeFn>(base+0x4ce80c);
        state_delete_target=reinterpret_cast<void*>(base+0x33bc70);state_expected_table=base+0x1353918;
        if(!safe_copy(bytes,reinterpret_cast<void*>(state_factory),sizeof(factory_bytes)) || memcmp(bytes,factory_bytes,sizeof(factory_bytes)) ||
           !safe_copy(bytes,reinterpret_cast<void*>(state_release),sizeof(release_bytes)) || memcmp(bytes,release_bytes,sizeof(release_bytes)) ||
           !safe_copy(bytes,state_delete_target,sizeof(delete_bytes)) || memcmp(bytes,delete_bytes,sizeof(delete_bytes))) {
            fprintf(status,"rejected: view state lifetime signature mismatch\n");fclose(status);return 4;
        }
    }
    FILE* file=_wfsopen(csv,L"w",_SH_DENYWR);
    if(!file) {fclose(status);return 5;}
    fprintf(file,"kind,thread,focused,valid,qpc,duration_qpc,object,result,caller_rva,x,y,z,pitch,yaw,roll,camera_timestamp,source_yaw,yaw_delta,cache_yaw,test_state");
    for(unsigned i=0;i<32;++i) fprintf(file,",matrix%u",i);
    fprintf(file,",pose_phase,source_x,source_y,source_z,source_pitch,source_rot_yaw,source_roll,delta_x,delta_y,delta_z,delta_pitch,delta_rot_yaw,delta_roll,pose_status,pose_sequence,gpu_device,gpu_buffer,gpu_window,queue_type,command_count,buffer_index,buffer_count,width,height,format,swap_flags,family_address,family_array,family_count,family_capacity,family_view0,family_view1,family_view2,family_view3,clone_source,clone_owner");
    for(unsigned i=0;i<32;++i)fprintf(file,",source_matrix%u",i);
    fprintf(file,",renderer_input_family,renderer_storage,renderer_count,renderer_capacity,renderer_source0,renderer_source1,renderer_source2,renderer_source3,stereo_ticket,stereo_source,stereo_head_flags,stereo_view_flags,stereo_reference_generation\n");fflush(file);
    LARGE_INTEGER frequency;QueryPerformanceFrequency(&frequency);
    qpc_frequency=frequency.QuadPart;
    if(outage_diagnostics.load()){outage_window.reset(qpc_frequency);outage_sources.reset(qpc_frequency);outage_source_interval={};outage_draws={};}
    fprintf(status,"qpc_frequency=%lld\n",frequency.QuadPart);
    fprintf(status,"wobble_enabled=%d amplitude_degrees=1.5 baseline_seconds=3 wave_seconds=12\n",wobble_enabled);
    fprintf(status,"sweep_enabled=%d rotation_degrees=1.5 translation_units=2 baseline_seconds=3 axis_seconds=4 total_seconds=27\n",sweep_enabled);
    fprintf(status,"live_head_enabled=%d rotation_limit_degrees=5 translation_limit_units=2 max_age_ms=100 F8=abort F9=recenter\n",live_enabled);
    fprintf(status,"gpu_diagnostics=%d\n",gpu_diagnostics);
    fprintf(status,"family_diagnostics=%d candidate_rva=0x3a148c\n",family_diagnostics);
    MH_STATUS init=MH_Initialize();
    fprintf(status,"minhook_initialize=%s\n",MH_StatusToString(init));
    bool ok=(init==MH_OK || (reservation_ready && init==MH_ERROR_ALREADY_INITIALIZED))&&death_constant_observer::attest(status)&&death_dof_control::attest(status)&&vr_dof_compat::attest(status);
    owned_plain_output_attested=false;
    if(ok&&owned_bindings_enabled){
        owned_plain_output_attested=final_transfer_live::verify_range(status,0xd82f58,0xc78,"1efb10541bf56d87b31aae4f452da0d281afd552da16b4bfd16d653f2cd425ea")&&
            final_transfer_live::verify_range(status,0xd83bd0,0x1e9,"e051a2d325a4d5ebc12be670d63cd8eae1eead437f33dfa39946de852e61e724")&&
            final_transfer_live::verify_range(status,0x2b0c04,0x651,"5088f5f7fb8c718769016b98194413e42aa466a30bfdfeab01d081c07075515d")&&
            final_transfer_live::verify_range(status,0x8ee78c,0x3ee,"024e4777801ff3236360d5ead3b10a2a02717438020dd634537a47723453e138");
        fprintf(status,"owned_plain_output_identity attested=%u caller=d83cbf identity_uv_only=1 runtime_owned_only=1\n",unsigned(owned_plain_output_attested));
        ok=owned_plain_output_attested;
    }
    if(ok)ok=dlss_route_observer::enable(status);
    if(ok)ok=ngx_route_observer::enable(status);
    if(ok)ok=ngx_stereo_host::enable(status);
    if(ok)ok=ngx_stereo_host::install_hooks();
#if defined(GEARS_NGX_ROUTE_OBSERVER) && GEARS_NGX_ROUTE_OBSERVER
    if(ok)ok=install(ngx_route_observer::create_target,reinterpret_cast<void*>(ngx_route_observer::hook_create),reinterpret_cast<void**>(&ngx_route_observer::original_create))&&
        install(ngx_route_observer::evaluate_target,reinterpret_cast<void*>(ngx_route_observer::hook_evaluate),reinterpret_cast<void**>(&ngx_route_observer::original_evaluate));
#endif
#if defined(GEARS_DLSS_ROUTE_OBSERVER) && GEARS_DLSS_ROUTE_OBSERVER
    if(ok)ok=install(dlss_route_observer::output_target,reinterpret_cast<void*>(dlss_route_observer::hook_output),reinterpret_cast<void**>(&dlss_route_observer::original_output))&&
        install(dlss_route_observer::setter_target,reinterpret_cast<void*>(dlss_route_observer::hook_setter),reinterpret_cast<void**>(&dlss_route_observer::original_setter));
#endif
#if defined(GEARS_DLSS_INPUT_BIND_OBSERVER) && GEARS_DLSS_INPUT_BIND_OBSERVER
    if(ok)ok=install(dlss_route_observer::bind_target,reinterpret_cast<void*>(dlss_route_observer::hook_bind),reinterpret_cast<void**>(&dlss_route_observer::original_bind));
#endif
#if defined(GEARS_DEATH_CONSTANT_OBSERVER) && GEARS_DEATH_CONSTANT_OBSERVER
    if(ok)ok=install(death_constant_observer::target,reinterpret_cast<void*>(death_constant_observer::hook),reinterpret_cast<void**>(&death_constant_observer::original));
#endif
#if defined(GEARS_DEATH_DOF_CONTROL) && GEARS_DEATH_DOF_CONTROL
    if(ok)ok=install(death_dof_control::target,reinterpret_cast<void*>(death_dof_control::hook),reinterpret_cast<void**>(&death_dof_control::original));
#endif
#if defined(GEARS_VR_DOF_COMPAT) && GEARS_VR_DOF_COMPAT
    if(ok)ok=install(vr_dof_compat::target,reinterpret_cast<void*>(vr_dof_compat::hook),reinterpret_cast<void**>(&vr_dof_compat::original));
#endif
    ok=ok && install(scene_target,reinterpret_cast<void*>(hook_scene),reinterpret_cast<void**>(&original_scene))
        && install(event_target,reinterpret_cast<void*>(hook_event),reinterpret_cast<void**>(&original_event));
    if(ok && family_diagnostics)ok=install(family_target,reinterpret_cast<void*>(hook_family),reinterpret_cast<void**>(&original_family));
    if(ok && family_diagnostics)ok=install(clone_target,reinterpret_cast<void*>(hook_clone),reinterpret_cast<void**>(&original_clone));
    if(ok && family_diagnostics)ok=install(renderer_target,reinterpret_cast<void*>(hook_renderer),reinterpret_cast<void**>(&original_renderer));
    if(ok && family_diagnostics)ok=install(consume_target,reinterpret_cast<void*>(hook_consume),reinterpret_cast<void**>(&original_consume)) &&
        install(destroy_renderer_target,reinterpret_cast<void*>(hook_destroy_renderer),reinterpret_cast<void**>(&original_destroy_renderer));
    if(ok && family_diagnostics)ok=install(prepare_target,reinterpret_cast<void*>(hook_prepare),reinterpret_cast<void**>(&original_prepare));
    if(ok && shadow_state_enabled)ok=install(state_delete_target,reinterpret_cast<void*>(hook_state_delete),reinterpret_cast<void**>(&original_state_delete));
    if(ok && viewport_diagnostics)ok=install(geometry_target,reinterpret_cast<void*>(hook_geometry),reinterpret_cast<void**>(&original_geometry));
    if(ok && viewport_diagnostics)ok=install(quad_target,reinterpret_cast<void*>(hook_quad),reinterpret_cast<void**>(&original_quad));
    if(ok && first_view_mask_hook_enabled())ok=install(first_view_mask_target,reinterpret_cast<void*>(hook_first_view_mask),reinterpret_cast<void**>(&original_first_view_mask));
    if(ok&&ui_observer_enabled)for(unsigned i=0;i<4&&ok;++i)
        ok=install(ui_targets[i],reinterpret_cast<void*>(ui_detours[i]),reinterpret_cast<void**>(&ui_original[i]));
#if defined(GEARS_UI_SCENE_ROOT_OBSERVER) && GEARS_UI_SCENE_ROOT_OBSERVER
    if(ok&&ui_observer_enabled)ok=install(ui_scene_root_live::target,reinterpret_cast<void*>(ui_scene_root_live::hook),reinterpret_cast<void**>(&ui_scene_root_live::original));
#endif
#if defined(GEARS_PATH_CHOICE_LAYOUT_OBSERVER) && GEARS_PATH_CHOICE_LAYOUT_OBSERVER
    if(ok&&ui_observer_enabled)ok=path_choice_layout_observer::enable(status)&&
        install(path_choice_layout_observer::target,reinterpret_cast<void*>(path_choice_layout_observer::hook),reinterpret_cast<void**>(&path_choice_layout_observer::original));
#endif
#if defined(GEARS_PATH_CHOICE_CLOSE_OBSERVER) && GEARS_PATH_CHOICE_CLOSE_OBSERVER
    if(ok&&ui_observer_enabled)ok=path_choice_close_observer::enable(status)&&
        install(path_choice_close_observer::target,reinterpret_cast<void*>(path_choice_close_observer::hook),reinterpret_cast<void**>(&path_choice_close_observer::original));
#endif
#if defined(GEARS_UI_CALL_BOUNDARY_OBSERVER) && GEARS_UI_CALL_BOUNDARY_OBSERVER
    if(ok&&ui_observer_enabled)ok=install(ui_call_boundary::target,reinterpret_cast<void*>(ui_call_boundary::hook),reinterpret_cast<void**>(&ui_call_boundary::original));
#endif
#if defined(GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER) && GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER
    if(ok&&crosshair_live::enabled)ok=install(crosshair_live::target,reinterpret_cast<void*>(crosshair_live::hook),reinterpret_cast<void**>(&crosshair_live::original));
#endif
    if(ok&&ui_observer_enabled)ok=install(ui_execute_target,reinterpret_cast<void*>(hook_ui_execute),reinterpret_cast<void**>(&ui_original_execute));
    if(ok&&ui_observer_enabled)ok=install(ui_subtitle_target,reinterpret_cast<void*>(hook_ui_subtitle),reinterpret_cast<void**>(&ui_original_subtitle));
#if defined(GEARS_OWNED_UI_ROUTE_PROBE) && GEARS_OWNED_UI_ROUTE_PROBE
    if(ok&&ui_observer_enabled)ok=install(owned_ui_live::flush_target,reinterpret_cast<void*>(owned_ui_live::hook_flush),reinterpret_cast<void**>(&owned_ui_live::original_flush))&&
        install(owned_ui_live::produce_target,reinterpret_cast<void*>(owned_ui_live::hook_produce),reinterpret_cast<void**>(&owned_ui_live::original_produce));
#endif
#if defined(GEARS_UI_MATERIAL_ROUTE) && GEARS_UI_MATERIAL_ROUTE
    if(ok&&ui_observer_enabled)ok=install(ui_material_execute_target,reinterpret_cast<void*>(hook_ui_material_execute),reinterpret_cast<void**>(&ui_original_material_execute));
#if defined(GEARS_GAME_UI_MATERIAL) && GEARS_GAME_UI_MATERIAL
    if(ok&&ui_observer_enabled)ok=install(game_material::upload_target,reinterpret_cast<void*>(game_material::hook_upload),reinterpret_cast<void**>(&game_material::original_upload));
#endif
#if defined(GEARS_OWNED_UI_MATERIAL) && GEARS_OWNED_UI_MATERIAL
    if(ok&&ui_observer_enabled)ok=install(owned_material::produce_target,reinterpret_cast<void*>(owned_material::hook_produce),reinterpret_cast<void**>(&owned_material::original_produce))&&
        install(owned_material::upload_target,reinterpret_cast<void*>(owned_material::hook_upload),reinterpret_cast<void**>(&owned_material::original_upload));
#endif
#endif
    if(ok&&continuous_session)ok=enable_graphics_lock_observer();
    if(ok)ok=enable_resource_resize_probe(status);
    if(ok)ok=final_transfer_live::enable(status);
    if(ok)ok=death_source_live::enable(status);
    if(ok)ok=death_hdr_pixels::enable(status);
    if(ok)ok=ngx_output_pixels::enable(status);
    if(ok)ok=owned_source_pixels::enable(status);
    if(ok)ok=owned_depth_clear_trial::enable(status);
    bool graphics=ok && present_hooks();
    if(gpu_diagnostics && !graphics)ok=false;
    if(ok)ok=create_quad_source_events();
    if(ok)ok=create_first_view_mask_events();
    if(ok)ok=create_stereo_recenter_event();
    fprintf(status,"present_hooks_created=%d\n",graphics);
    if(ok) {
        std::lock_guard<std::mutex> lock(prime_mutex);
        if(prime_ready.load()&&!prime_owner_allows_handoff(continuous_session,session_owner_pid,session_owner_creation_time)){
            fprintf(status,"rejected: target prime owner or pending state invalid\n");ok=false;
        }
        if(ok&&prime_ready.load()){
            ok=graphics_lock_accept_handoff();
            if(ok){prime_handoff=true;SetEvent(prime_event);}
        }
        if(ok){
        submit_not_before=continuous_session?0:now()+3*qpc_frequency;recording=true;prime_recording=false;
        MH_STATUS applied=MH_ApplyQueued();fprintf(status,"minhook_apply=%s\n",MH_StatusToString(applied));ok=applied==MH_OK;
        if(ok&&continuous_session&&!seal_session_hook_manifest(status))
            fprintf(status,"hook_entry_manifest available=0 recovery_certificate_unavailable=1\n");
        }
    }
    if(!ok) {
        session_activation_gate.close();
        ngx_mv_target_capture::close();ngx_mv_descriptor_capture::close();
        recording=false;MH_STATUS disabled=MH_DisableHook(MH_ALL_HOOKS);
        close_quad_source_events();
        close_first_view_mask_events();
        close_stereo_recenter_event();
        ngx_mv_descriptor_capture::release_drained();ngx_mv_descriptor_capture::print(status);
        fprintf(status,"hook installation failed disable_status=%d active=%u\n",disabled,active.load());
        diagnostic_status=nullptr;fclose(file);fclose(status);return 6;
    }
    wchar_t name[128];swprintf_s(name,L"Local\\GearsVR.Capture.Stop.%lu",GetCurrentProcessId());
    stop_event=CreateEventW(nullptr,TRUE,FALSE,name);
    const unsigned seconds=GetPrivateProfileIntW(L"Capture",L"seconds",90,ini);
    fprintf(status,"capturing pid=%lu seconds=%u\n",GetCurrentProcessId(),seconds);fflush(status);
    const ULONGLONG deadline=GetTickCount64()+static_cast<ULONGLONG>(seconds)*1000;
    if(continuous_session){
        session_started_qpc=now();write_session_status();
        WorldRebindPrepareRequest world_rebind_request;
        if(!session_status_available||!stop_event)request_session_stop(SessionStop::proof_failure);
        publish_session_activation_ready(session_status_available&&stop_event);
        ULONGLONG next_status=0;
        gears_world::StatusCadence world_status;
        for(;;){
#if defined(GEARS_PATH_CHOICE_POSITION_TRIAL) && GEARS_PATH_CHOICE_POSITION_TRIAL
            if(path_position_stop_record.load(std::memory_order_acquire)==2)
                request_session_stop_at(path_position_stop_reason,path_position_stop_file,path_position_stop_line);
#endif
            observe_session_owner(continuous_owner.state());
#if defined(GEARS_NGX_OUTPUT_DELAYED_CAPTURE) && GEARS_NGX_OUTPUT_DELAYED_CAPTURE
            {wchar_t arm_path[MAX_PATH];path_for(arm_path,L"ngx-output-arm.ini");FILETIME created{},exited{},kernel{},user{};LARGE_INTEGER frequency{};
                if(GetProcessTimes(GetCurrentProcess(),&created,&exited,&kernel,&user)&&QueryPerformanceFrequency(&frequency))
                    ngx_output_pixels::worker_request(arm_path,GetCurrentProcessId(),gears_session::creation_time(created),uint64_t(session_started_qpc),uint64_t(frequency.QuadPart));}
#endif
#if defined(GEARS_UI_SCENE_SOURCE_TRIAL) && GEARS_UI_SCENE_SOURCE_TRIAL
            {wchar_t trial_path[MAX_PATH];path_for(trial_path,L"path-choice-trial.ini");ui_scene_trial_live::poll(trial_path,status);}
#endif
#if defined(GEARS_PATH_CHOICE_POSITION_TRIAL) && GEARS_PATH_CHOICE_POSITION_TRIAL
            {wchar_t trial_path[MAX_PATH];path_for(trial_path,L"path-choice-position-trial.ini");path_choice_position_live::poll(trial_path,status);}
#endif
#if defined(GEARS_PATH_CHOICE_CLOSE_OBSERVER) && GEARS_PATH_CHOICE_CLOSE_OBSERVER
            {wchar_t close_path[MAX_PATH];path_for(close_path,L"path-choice-close-observer.ini");path_choice_close_observer::poll(close_path,status);}
#endif
            if(stop_event&&WaitForSingleObject(stop_event,0)==WAIT_OBJECT_0)request_session_stop(SessionStop::event);
            if(GetAsyncKeyState(VK_F8)&0x8000)request_session_stop(SessionStop::f8);
            if(state_created.load()&&!submit_background&&!foreground())request_session_stop(SessionStop::focus_lost);
            if(stereo_publisher.state.load()==3)request_session_stop(SessionStop::proof_failure);
            stereo_publish_worker_tick();
            {wchar_t arm_path[MAX_PATH];path_for(arm_path,L"reticle-sample-arm.ini");ui_reticle_late_sample::poll(arm_path,status);}
            world_rebind_request.poll(status);
            advance_world_handoff(status);
            {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);owned_scene_gpu::poll();
                if(owned_scene_gpu::publication_failed())request_session_stop(SessionStop::proof_failure);}
            if(submitted_lifetime.stopping()){
                if(submitted_lifetime.is_closed()){auto requested=SessionPhase::stop_requested;
                    session_phase.compare_exchange_strong(requested,SessionPhase::native_closed);}
                if(session_native_cleanup_ready()){
                    if(!native_owned_scene_retained())break;
                    auto owned_cleanup=[&](bool bridges_closed){
                        std::lock_guard<std::recursive_mutex> native_guard(session_native_mutex);
                        bool canvas_drained=true;
#if defined(GEARS_OWNED_UI_ROUTE_PROBE) && GEARS_OWNED_UI_ROUTE_PROBE
                        canvas_drained=owned_ui_live::ledger.cpu_drained();
#endif
                        return native_owned_scene_shutdown_worker(session_native_cleanup_ready(),canvas_drained,bridges_closed);
                    };
                    owned_cleanup(false); // Freeze CPU/Canvas proof before stopping GPU publication.
                    const bool bridges_closed=close_stereo_publisher(); // Retain the active-callback guard.
                    if(owned_cleanup(bridges_closed))break;
                    if(session_cleanup_expired()){session_cleanup_timeout=true;break;} // Fail closed; keep resources.
                }
                if(session_cleanup_expired())session_cleanup_timeout=true;
            }
            const bool world_progress=world_status.changed({committed_world_generation.load(),world_prepare_requests.load(),
                world_prepare_rejected.load(),world_candidate_generation.load(),world_handoff_phase.load()});
            if(world_progress||GetTickCount64()>=next_status){write_session_status();next_status=GetTickCount64()+death_source_live::status_interval_ms();}
            flush(file,4);Sleep(20);
        }
        session_phase=SessionPhase::draining;write_session_status();
    }else while(GetTickCount64()<deadline && (!stop_event || WaitForSingleObject(stop_event,20)!=WAIT_OBJECT_0)) {
        stereo_publish_worker_tick();
        flush(file,4);refresh_quad_source_events();refresh_first_view_mask_events();if(!stop_event) Sleep(20);
    }
    // Disable entry patches; leave the DLL and trampolines resident so delayed
    // in-flight execution can never jump into freed code.
    session_activation_gate.close();
    ngx_mv_target_capture::close();ngx_mv_descriptor_capture::close();
    death_hdr_pixels::stop();
    ngx_output_pixels::stop();
    owned_source_pixels::stop();
    MH_STATUS disabled=MH_DisableHook(MH_ALL_HOOKS);
    recording=false;
    for(unsigned i=0;i<100 && active.load();++i) Sleep(10);
    if(continuous_session)while(active.load()){
        if(session_cleanup_expired())session_cleanup_timeout=true;
        write_session_status();Sleep(100);
    }
    if(continuous_session)while(disabled!=MH_OK||!session_hooks_restored()){
        if(session_cleanup_expired())session_cleanup_timeout=true;
        write_session_status();Sleep(100);disabled=MH_DisableHook(MH_ALL_HOOKS);
    }
    close_quad_source_events();
    close_first_view_mask_events();
    flush(file);
    fprintf(status,"stopped disable_status=%d active=%u dropped=%llu scene=%llu event=%llu present=%llu present1=%llu execute=%llu\n",
        disabled,active.load(),dropped.load(),emitted[1],emitted[2],emitted[3],emitted[4],emitted[5]);
    bool stereo_closed=close_stereo_publisher();
    if(continuous_session)while(!stereo_closed){
        if(session_cleanup_expired())session_cleanup_timeout=true;
        write_session_status();Sleep(100);stereo_closed=close_stereo_publisher();
    }
    desktop_mirror_readback::poll();ngx_mv_compute_capture::poll_picked_blob();ngx_root_signature_capture::poll();death_pso_cache::poll();death_pso_cache::stop();
    print_frame_link(status);
    print_outage(status,disabled==MH_OK);
    print_anchor(status,disabled==MH_OK);
    print_ui_observer(status,disabled==MH_OK);
    crosshair_live::print(status);
    print_native_surface_probe(status);
    print_native_owned_probe(status);
    print_native_owned_scene(status);
    print_resource_resize_probe(status);
    gears_swapchain_startup::print(status);
#if defined(GEARS_GAME_RESOLUTION) && GEARS_GAME_RESOLUTION
    fprintf(status,"game_resolution_resize calls=%u completed=%u failed=%u rejected=%u drain_failed=%u\n",
        game_resize::calls.load(),game_resize::completed.load(),game_resize::failed.load(),game_resize::rejected.load(),game_resize::drain_failed.load());
    game_resize::print(status);
#endif
    print_graphics_lock_observer(status,disabled==MH_OK);
    print_origin_trial(status,disabled==MH_OK);
    if(!active.load()&&stereo_closed)release_bindings();
    ngx_mv_descriptor_capture::print(status);
    if(continuous_session){
        {std::lock_guard<FrameHookMutex> guard(frame_hook_mutex);retire_stereo_window();}
    }
    if(prime_ready.load())for(unsigned i=0;i<prime_targets.size();++i){std::array<unsigned char,32> bytes{};
        bool restored=!active.load()&&safe_copy(bytes.data(),prime_targets[i],bytes.size())&&bytes==prime_original_bytes[i];
        fprintf(status,"prime_entry target=%p restored=%u before=",prime_targets[i],restored?1:0);
        for(auto byte:prime_original_bytes[i])fprintf(status,"%02x",byte);fprintf(status,"\n");
    }
    fprintf(status,"wobble_final_state=%u\n",wobble_state.load());
    fprintf(status,"family_calls=%llu\n",emitted[7]);
    fprintf(status,"renderer_consume_calls=%llu renderer_destroy_calls=%llu\n",emitted[10],emitted[11]);
      fprintf(status,"renderer_prepare_calls=%llu\n",emitted[12]);
      if(viewport_diagnostics) {
          auto stats=viewport_stats.snapshot();
          fprintf(status,"gpu_viewport resets=%llu viewports=%llu scissors=%llu overflow=%llu untracked_draws=%llu\n",
              stats.resets,stats.viewports,stats.scissors,stats.overflow,stats.untracked_draws);
          for(unsigned phase=0;phase<3;++phase)for(unsigned viewport=0;viewport<5;++viewport)for(unsigned scissor=0;scissor<5;++scissor)
              if(stats.draws[phase][viewport][scissor])fprintf(status,"gpu_draw phase=%u viewport=%u scissor=%u calls=%llu\n",phase,viewport,scissor,stats.draws[phase][viewport][scissor]);
          fprintf(status,"gpu_rect_overflow=%llu\n",stats.rect_overflow);
          for(const auto& entry:stats.trial_rects)if(entry.calls)
              fprintf(status,"gpu_trial_rect viewport=(%.9g,%.9g,%.9g,%.9g) scissor=(%.9g,%.9g,%.9g,%.9g) calls=%llu\n",
                  entry.viewport.x,entry.viewport.y,entry.viewport.width,entry.viewport.height,
                  entry.scissor.x,entry.scissor.y,entry.scissor.width,entry.scissor.height,entry.calls);
          fprintf(status,"gpu_targets binds=%llu creates=%llu copies=%llu evictions=%llu rejected_copies=%llu histogram_overflow=%llu\n",
              stats.target_binds,stats.descriptor_creates,stats.descriptor_copies,stats.descriptor_evictions,stats.copy_rejected,stats.target_overflow);
          for(const auto& entry:stats.trial_targets)if(entry.calls)
              fprintf(status,"gpu_trial_target viewport=%u targets=%u resource=%llx width=%llu height=%u format=%u mip=%u calls=%llu\n",
                  entry.viewport,entry.targets,static_cast<unsigned long long>(entry.target.resource),entry.target.width,
                  entry.target.height,entry.target.format,entry.target.mip,entry.calls);
          fprintf(status,"submit_background=%u\n",submit_background);
          print_geometry(status);
          print_final_inputs(status);
          print_quad_source_control(status);
          print_color_copy(status);
          print_first_view_mask(status);
          print_view_union_mask(status);
          print_projection(status);
          print_stereo(status);
          fprintf(status,"gpu_output_quad_overflow=%llu limit=64\n",quad_overflow);
          for(const auto& entry:quad_traces)if(entry.calls)fprintf(status,
              "gpu_output_quad caller=%llx dest=%.9g,%.9g,%.9g,%.9g source=%.9g,%.9g,%.9g,%.9g target_dimensions=%u,%u texture_dimensions=%u,%u calls=%llu\n",
              static_cast<unsigned long long>(entry.caller),entry.rectangles[0],entry.rectangles[1],entry.rectangles[2],entry.rectangles[3],
              entry.rectangles[4],entry.rectangles[5],entry.rectangles[6],entry.rectangles[7],entry.dimensions[0],entry.dimensions[1],entry.dimensions[2],entry.dimensions[3],entry.calls);
          fprintf(status,"gpu_output_stack_attempts=%u limit=64 slots=8 frames=24\n",output_stack_attempts.load());
          for(unsigned i=0;i<output_post_snapshots.size();++i){const auto& sample=output_post_snapshots[i];if(!sample.renderer)continue;
              fprintf(status,"gpu_output_post index=%u renderer=%llx view_count=%u nodes_limit=16\n",i,
                  static_cast<unsigned long long>(sample.renderer),sample.view_count);
              for(unsigned eye=0;eye<sample.view_count;++eye){const auto& view=sample.views[eye];
                  fprintf(status,"gpu_output_post_view index=%u eye=%u view=%llx valid=%u array=%llx count=%d capacity=%d source=%.9g,%.9g,%.9g,%.9g target=%d,%d,%d,%d\n",
                      i,eye,static_cast<unsigned long long>(view.view),view.valid,static_cast<unsigned long long>(view.array),view.count,view.capacity,
                      view.source[0],view.source[1],view.source[2],view.source[3],view.target[0],view.target[1],view.target[2],view.target[3]);
                  for(unsigned n=0;n<view.nodes.size();++n){const auto& node=view.nodes[n];if(!node.object)continue;
                      fprintf(status,"gpu_output_post_node index=%u eye=%u node=%u object=%llx table=%llx function=%llx flags=%u valid=%u\n",
                          i,eye,n,static_cast<unsigned long long>(node.object),static_cast<unsigned long long>(node.table),
                          static_cast<unsigned long long>(node.function),node.flags,node.valid);
                  }
              }
          }
          for(unsigned i=0;i<output_stacks.size();++i){const auto& entry=output_stacks[i];if(!entry.count)continue;
              fprintf(status,"gpu_output_stack index=%u resource=%llx kind=%u count=%u\n",i,
                  static_cast<unsigned long long>(entry.target.resource),entry.kind,entry.count);
              for(unsigned j=0;j<entry.count;++j){HMODULE module{};wchar_t path[MAX_PATH]{};
                  if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                     reinterpret_cast<LPCWSTR>(entry.frames[j]),&module))GetModuleFileNameW(module,path,MAX_PATH);
                  const wchar_t* name=wcsrchr(path,L'\\');name=name?name+1:path;
                  fprintf(status,"gpu_output_frame index=%u frame=%u address=%llx module=%ls rva=%llx\n",i,j,
                      static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(entry.frames[j])),name,
                      static_cast<unsigned long long>(module?reinterpret_cast<uintptr_t>(entry.frames[j])-reinterpret_cast<uintptr_t>(module):0));
              }}
      }
    unsigned shadow_state=shadow_attempted.load();
    const auto completed_shadow=shadow_state==2?shadow_result:gears_shadow::Result{};
    fprintf(status,"shadow_view_enabled=%d attempted=%u stage=%u checks=%u nonempty_arrays=%u source=0x%llx temporary=0x%llx\n",
        shadow_view_enabled,shadow_state,completed_shadow.stage,completed_shadow.checks,completed_shadow.nonempty_arrays,
        static_cast<unsigned long long>(completed_shadow.source),static_cast<unsigned long long>(completed_shadow.temporary));
    fprintf(status,"constructor_shadow_enabled=%d derived_finite=%u derived_max_error=%.9g\n",constructor_shadow_enabled,
        shadow_state==2?constructor_derived_finite:0,shadow_state==2?constructor_derived_error:0.);
    fprintf(status,"constructor_state_enabled=%d\n",constructor_state_enabled);
    const auto pair_result=shadow_state==2?renderer_pair_result:gears_pair_shadow::Result{};
    fprintf(status,"renderer_pair_enabled=%d checks=%u renderer=0x%llx retained=%d\n",renderer_pair_enabled,pair_result.checks,
        static_cast<unsigned long long>(pair_result.renderer),pair_result.retain_inputs);
    const unsigned submit_phase=submit_attempted.load();
    fprintf(status,"submit_pair_enabled=%d attempted=%u root=0x%llx owned=%u sources_released=%u original_preserved=%u consumed=%u prepared=%u destroyed=%u\n",
        submit_pair_enabled,submit_phase,static_cast<unsigned long long>(submit_root.load()),
        submit_phase==2?submit_owned_verified.load():0,submit_phase==2?submit_sources_released.load():0,submit_phase==2?submit_original_preserved.load():0,
        submit_consumed.load(),submit_prepared.load(),submit_destroyed.load());
    fprintf(status,"submit_inputs checks=%u rejection=%u rectangle=%.9g,%.9g,%.9g,%.9g\n",
        submit_phase>=2?submit_source_checks:0,submit_phase>=2?submit_rejection:0,
        submit_phase>=2?submit_rectangle[0]:0.,submit_phase>=2?submit_rectangle[1]:0.,
        submit_phase>=2?submit_rectangle[2]:0.,submit_phase>=2?submit_rectangle[3]:0.);
    fprintf(status,"submit_lifetime seconds=%u total=%u closed=%d pending=%u retained=%u skipped=%u trial_seconds=%u validated_union=%d\n",
        submit_pair_seconds,submit_total.load(),submitted_lifetime.is_closed(),submitted_lifetime.pending(),submit_retained.load(),submit_skipped.load(),trial_seconds,view_union_mask_fix);
    fprintf(status,"source_map_validation candidates=%llu accepted=%llu rejected=%llu max_nodes=%u last_checks=%u supported_nonempty=1 max_supported_nodes=%u map_check_bits=input:1,left:2,right:4,independent:8,contents:16,source_unchanged:32\n",
        source_map_candidates.load(),source_map_accepted.load(),source_map_rejected.load(),source_map_max_nodes.load(),source_map_last_checks.load(),gears_source_map::max_nodes);
    if(split_failure_phase.load()==2){
        auto const& f=first_split_failure;auto const& e=f.input;
        fprintf(status,"split_input_failure available=1 rejection=%u checks=%u read_succeeded=%u failure_mask=%u family=%llx source=%llx observed_family=%llx original_state=%llx right_state=%llx field140=%llx field418=%llx before_owner=%llx before_state=%llx before140=%llx before418=%llx begin_qpc=%lld end_qpc=%lld reason_bits=read_failed:1,family_mismatch:2,missing_original_state:4,missing_right_state:8,aliased_state:16,field140_nonzero:32,field418_nonzero:64\n",
            f.rejection,f.checks,e.read_succeeded?1:0,e.failure_mask,e.family,e.source,e.observed_family,e.original_state,e.right_state,
            e.field140,e.field418,f.before_owner,f.before_state,f.before140,f.before418,f.begin_qpc,f.end_qpc);
    }
    fprintf(status,"submit_output_bounds=%d\n",submit_output_bounds);
#if defined(GEARS_RUNTIME_RENDER_SIZE) && GEARS_RUNTIME_RENDER_SIZE
    fprintf(status,"resolution_source=openxr_recommended_scaled independent_engine_target=1\n");
#endif
#ifdef GEARS_GAME_RESOLUTION
    fprintf(status,"resolution_source=game\n");
#endif
    fprintf(status,"submit_view_count=%u submit_event_marker=77\n",submit_mono_control?1:2);
    fprintf(status,"submit_duplicate_control=%d\n",submit_duplicate_control);
    fprintf(status,"submit_mono_independent_state=%d\n",submit_mono_independent_state);
    owned_ticket_size_trace::print(status);
    owned_ticket_size_trace::print_quad_draws(status);
    owned_depth_clear_trial::print(status);
    for(unsigned i=0;i<rectangle_traces.size();++i) {
        unsigned phase=rectangle_trace_phases[i].load();auto trace=phase==2?rectangle_traces[i]:RectangleTrace{};
        fprintf(status,"rectangle_trace index=%u phase=%u object=0x%llx source=%.9g,%.9g,%.9g,%.9g target=%d,%d,%d,%d\n",
            i,phase,static_cast<unsigned long long>(trace.object),trace.source[0],trace.source[1],trace.source[2],trace.source[3],
            trace.target[0],trace.target[1],trace.target[2],trace.target[3]);
        fprintf(status,"sampling_trace index=%u phase=%u uv=%.9g,%.9g,%.9g,%.9g buffer=%d,%d\n",
            i,phase,trace.sampling[0],trace.sampling[1],trace.sampling[2],trace.sampling[3],trace.buffer[0],trace.buffer[1]);
    }
    fprintf(status,"constructor_eye_enabled=%d shifted=%u combined_valid=%u combined_relative_error=%.9g\n",constructor_eye_enabled,
        shadow_state==2?constructor_eye_shifted:0,shadow_state==2?constructor_combined_valid:0,
        shadow_state==2?constructor_combined_relative_error:0.);
    unsigned state_done=state_destroyed.load();
    fprintf(status,"shadow_state_enabled=%d attempted=%u created=%u release_returned=%u destroyed=%u original_preserved=%u flags=%u thread=%u caller=0x%llx temporary=0x%llx\n",
        shadow_state_enabled,state_attempted.load(),state_created.load(),state_release_returned.load(),state_done,state_original_preserved.load(),
        state_done?state_delete_flags:0,state_done?state_delete_thread:0,
        static_cast<unsigned long long>(state_done?state_delete_caller:0),static_cast<unsigned long long>(state_temporary.load()));
    stereo_bridge.close();
    diagnostic_status=nullptr;fclose(file);fclose(status);
    // Terminal publication promises the complete, formerly exclusive report is readable.
    if(continuous_session){observe_session_owner(continuous_owner.state());session_phase=SessionPhase::stopped;
        write_session_status(false,disabled==MH_OK&&session_hooks_restored());}
    close_stereo_recenter_event();
    if(stop_event) CloseHandle(stop_event);stop_event=nullptr;
    return disabled==MH_OK?0:7;
}
// Test real trampoline execution and six-argument forwarding before injection.
__declspec(noinline) void fake_event(void* pc,Vec* p,Rot* r) {p->x+=static_cast<float>(reinterpret_cast<uintptr_t>(pc));r->yaw+=7;}
struct FakeView {char prefix[0x70];float matrices[32];};
FakeView fake_view{};
unsigned clone_forward_count{};
unsigned renderer_forward_count{};
unsigned consume_forward_count{},destroy_forward_count{};
__declspec(noinline) void* fake_destroy_renderer(void* renderer){
    ++destroy_forward_count;VirtualFree(renderer,0,MEM_RELEASE);return renderer;
}
bool continuous_test_destroy_returned{},continuous_test_release_order_bad{};
unsigned continuous_test_release_calls{};
DWORD continuous_test_release_thread{};
std::atomic<unsigned> continuous_test_executes{};
void STDMETHODCALLTYPE fake_continuous_execute(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*){++continuous_test_executes;}
void* __fastcall fake_continuous_destroy(void* renderer){
    VirtualFree(renderer,0,MEM_RELEASE);continuous_test_destroy_returned=true;return renderer;
}
void __fastcall fake_continuous_release(void*){
    if(!continuous_test_destroy_returned)continuous_test_release_order_bad=true;
    ++continuous_test_release_calls;continuous_test_release_thread=GetCurrentThreadId();
    state_delete_flags=1;state_destroyed=1;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureContinuousCheck(void*) {
    // CPU-only: actual production stop/native destroy helpers with fake native
    // destructor/state release. No D3D device, GPU commands or XR session.
    if(!graphics_lock_observer_self_test())return 240;
    // Test the production gate on private instances, including publish/close
    // overlap. Closed remains terminal regardless of which operation wins.
    {
        SessionActivationGate gate;
        if(gate.ready()||!gate.publish()||!gate.ready()||gate.publish())return 249;
        gate.close();if(gate.ready()||gate.publish())return 250;
        for(unsigned i=0;i<32;++i){
            SessionActivationGate raced;
            HANDLE publisher=CreateThread(nullptr,0,[](void* value)->DWORD{
                static_cast<SessionActivationGate*>(value)->publish();return 0;
            },&raced,0,nullptr);
            if(!publisher)return 251;
            raced.close();const auto waited=WaitForSingleObject(publisher,INFINITE);CloseHandle(publisher);
            if(waited!=WAIT_OBJECT_0||raced.ready()||raced.publish())return 252;
        }
    }
    continuous_session=true;stereo_gpu_publish=true;submit_pair_enabled=true;
    if(session_activation_gate.ready()||publish_session_activation_ready(false)||session_activation_ready_qpc.load())return 246;
    if(!publish_session_activation_ready(true)||!session_activation_gate.ready()||
       session_activation_ready_qpc.load()<=0)return 247;
    std::uint64_t parsed{};FILETIME created{},exited{},kernel{},user{};
    if(!GetProcessTimes(GetCurrentProcess(),&created,&exited,&kernel,&user))return 210;
    auto identity=gears_session::creation_time(created);
    gears_session::Owner owner,wrong_owner;
    if(!owner.open(GetCurrentProcessId(),identity)||owner.state()!=gears_session::OwnerState::alive||
       wrong_owner.open(GetCurrentProcessId(),identity+1)||wrong_owner.open(0,identity)||
       !gears_session::decimal(L"4294967295",MAXDWORD,parsed)||parsed!=MAXDWORD||
       gears_session::decimal(L"4294967296",MAXDWORD,parsed)||
       gears_session::decimal(L"18446744073709551616",(std::numeric_limits<std::uint64_t>::max)(),parsed)||
       gears_session::decimal(L"-1",MAXDWORD,parsed)||gears_session::decimal(L"12x",MAXDWORD,parsed)||
       gears_session::decimal(L"0",MAXDWORD,parsed)||
       gears_session::owner_wait_state(WAIT_FAILED)!=gears_session::OwnerState::error)return 211;
    observe_session_owner(owner.state());
    if(submitted_lifetime.stopping()||session_phase.load()!=SessionPhase::running)return 212;
    if(!submit_session_enabled()||submit_pair_seconds)return 190;
    state_created=1;state_temporary=1;submit_sources_released=1;
    state_release=fake_continuous_release;original_destroy_renderer=fake_continuous_destroy;
    void* renderer=VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!renderer||!submitted_lifetime.begin(reinterpret_cast<uintptr_t>(renderer)))return 191;
    HANDLE ready=CreateEventW(nullptr,TRUE,FALSE,nullptr),done=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    HANDLE events[]={ready,done};HANDLE thread{};DWORD error=0;
    if(!ready||!done)return 192;
    {
        std::lock_guard<std::recursive_mutex> native_lock(session_native_mutex);
        SessionBuildScope building;
        const auto first_stop_before=now();const unsigned first_stop_line=__LINE__+1;
        request_session_stop(SessionStop::event);
        const auto first_stop_after=now(),first_stop_qpc=session_stop_qpc.load();
        if(session_activation_gate.ready()||publish_session_activation_ready(true))error=248;
        const auto first_stop_tick=session_stop_tick.load();const auto first_stop_thread=session_stop_thread.load();
        const auto first_stop_file=session_stop_source_file.load();const auto first_stop_source_line=session_stop_source_line.load();
        observe_session_owner(gears_session::owner_wait_state(WAIT_OBJECT_0)); // Evidence survives an earlier first stop reason.
        observe_session_owner(gears_session::owner_wait_state(WAIT_FAILED));
        if(session_reason.load()!=SessionStop::event||!session_owner_exit_observed.load()||!session_owner_error_observed.load()||
           submitted_lifetime.is_closed()||continuous_test_release_calls)error=213;
        if(!session_stop_recorded.load()||first_stop_qpc<=0||first_stop_qpc<first_stop_before||first_stop_qpc>first_stop_after||
           first_stop_thread!=GetCurrentThreadId()||!first_stop_tick||!first_stop_file||strcmp(first_stop_file,__FILE__)||
           first_stop_source_line!=first_stop_line||session_stop_qpc.load()!=first_stop_qpc||session_stop_tick.load()!=first_stop_tick||
           session_stop_thread.load()!=first_stop_thread||session_stop_source_file.load()!=first_stop_file||
           session_stop_source_line.load()!=first_stop_source_line)error=213;
        service_session_stop_native(); // Reentrant construction must defer close.
        stereo_publish_worker_tick(); // Mark absent publisher stopped, no GPU work.
        session_stop_tick=GetTickCount64()-30001;
        if(session_cleanup_expired())session_cleanup_timeout=true;
        if(!submitted_lifetime.stopping()||submitted_lifetime.is_closed()||session_native_cleanup_ready()||
           submitted_lifetime.begin(2)||continuous_test_release_calls||!session_cleanup_timeout.load()||
           !stereo_publish_counts.stopped.load())error=193;
        thread=CreateThread(nullptr,0,[](void* value)->DWORD{
            auto handles=static_cast<HANDLE*>(value);Scope scope;SetEvent(handles[0]);
            service_session_stop_native();SetEvent(handles[1]);return 0;
        },events,0,nullptr);
        if(!thread||WaitForSingleObject(ready,1000)!=WAIT_OBJECT_0||WaitForSingleObject(done,0)!=WAIT_TIMEOUT)error=194;
    }
    if(thread&&WaitForSingleObject(thread,1000)!=WAIT_OBJECT_0)error=195;
    if(thread)CloseHandle(thread);CloseHandle(ready);CloseHandle(done);
    if(!submitted_lifetime.is_closed()||submitted_lifetime.pending()!=1||continuous_test_release_calls||session_native_cleanup_ready())error=196;
    hook_destroy_renderer(renderer);service_session_stop_native();service_session_stop_native();
    if(!continuous_test_destroy_returned||continuous_test_release_order_bad||continuous_test_release_calls!=1||
       continuous_test_release_thread!=GetCurrentThreadId()||!session_native_cleanup_ready()||submitted_lifetime.pending()||active.load())error=197;
    // A detour blocked before acquiring the frame mutex must already be active.
    auto saved_execute=original_execute;original_execute=fake_continuous_execute;
    frame_link_diagnostics=true;self_testing=true;recording=true;
    frame_test_waiting=CreateEventW(nullptr,TRUE,FALSE,nullptr);HANDLE blocked{};
    if(!frame_test_waiting)return 205;
    {
        std::lock_guard<FrameHookMutex> frame_guard(frame_hook_mutex);
        blocked=CreateThread(nullptr,0,[](void*)->DWORD{hook_execute(nullptr,0,nullptr);return 0;},nullptr,0,nullptr);
        if(!blocked||WaitForSingleObject(frame_test_waiting,1000)!=WAIT_OBJECT_0||active.load()!=2||continuous_test_executes.load())error=206;
        recording=false;
        if(!active.load()||session_hooks_restored())error=207;
    }
    if(blocked&&WaitForSingleObject(blocked,1000)!=WAIT_OBJECT_0)error=208;
    if(blocked)CloseHandle(blocked);CloseHandle(frame_test_waiting);frame_test_waiting=nullptr;
    original_execute=saved_execute;frame_link_diagnostics=false;self_testing=false;
    if(active.load()||continuous_test_executes.load()!=1)error=209;
    // Rejected world admission still serializes real GPU forwarding against
    // Present/commit. A closed gate must not create an invisible Execute gap.
    gears_world::EpochGate::CloseToken world_closed{};
    if(!world_gate.try_close(world_closed))return 241;
    original_execute=fake_continuous_execute;frame_link_diagnostics=true;self_testing=true;recording=true;
    frame_test_waiting=CreateEventW(nullptr,TRUE,FALSE,nullptr);blocked=nullptr;
    if(!frame_test_waiting)return 242;
    {
        std::lock_guard<FrameHookMutex> frame_guard(frame_hook_mutex);
        blocked=CreateThread(nullptr,0,[](void*)->DWORD{hook_execute(nullptr,0,nullptr);return 0;},nullptr,0,nullptr);
        if(!blocked||WaitForSingleObject(frame_test_waiting,1000)!=WAIT_OBJECT_0||active.load()!=2||continuous_test_executes.load()!=1)error=243;
    }
    if(blocked&&WaitForSingleObject(blocked,1000)!=WAIT_OBJECT_0)error=244;
    if(blocked)CloseHandle(blocked);CloseHandle(frame_test_waiting);frame_test_waiting=nullptr;
    if(!world_gate.open(world_closed)||active.load()||continuous_test_executes.load()!=2)error=245;
    original_execute=saved_execute;frame_link_diagnostics=false;self_testing=false;
    // Retirement stays visible after ordinary world admission has gone away.
    // It must not keep the epoch gate admitted merely to keep active nonzero.
    {
        NativeOwnedShutdownRhiScope retirement;
        {Scope ordinary;if(active.load()!=2)error=246;}
        gears_world::EpochGate::CloseToken retirement_closed{};
        if(active.load()!=1||world_thread.depth||!world_gate.try_close(retirement_closed))error=247;
        else if(!world_gate.open(retirement_closed))error=248;
    }
    if(active.load())error=249;
    // CSV suppression must not turn off the proof-recording switch.
    recording=true;Record row{};auto before=write_index;enqueue(row);
    if(write_index!=before||!recording.load())error=198;
    recording=false;continuous_session=false;
    if(submit_session_enabled())error=199;
    submit_pair_seconds=30;if(!submit_session_enabled())error=200;
    submit_pair_seconds=120;if(!submit_session_enabled())error=201;
    submit_pair_seconds=0;
    FILE* output{};
    if(tmpfile_s(&output)||!output)return 202;
    recording=true;auto first=read_index;
    for(unsigned i=0;i<300;++i)enqueue(row);
    flush(output,1);
    if(read_index!=first+128||write_index-read_index!=172)error=203;
    flush(output);recording=false;fclose(output);
    if(read_index!=write_index)error=204;
    printf("Continuous CPU stop: admission, paused timeout retention, construction/reentrant barrier, native destructor then single release, CSV/proof separation passed\n");
    return error;
}
__declspec(noinline) void fake_consume(void* renderer){
    fake_destroy_renderer(renderer);++consume_forward_count;
}
FakeView fake_rendered_view{};
uintptr_t fake_rendered_pointer{};
unsigned prepare_forward_count{};
__declspec(noinline) void fake_prepare(void* renderer){
    ++prepare_forward_count;
    uintptr_t view{};memcpy(&view,reinterpret_cast<char*>(renderer)+0x68,8);
    float x=.125f,y=-.25f;uintptr_t state=0x2222;
    memcpy(reinterpret_cast<void*>(view+0xd0),&x,4);memcpy(reinterpret_cast<void*>(view+0xd4),&y,4);
    memcpy(reinterpret_cast<void*>(view+8),&state,8);
}
__declspec(noinline) void* fake_renderer(void* destination,void* family,void* canvas,void* transform){
    renderer_forward_count+=canvas==reinterpret_cast<void*>(5) && transform==reinterpret_cast<void*>(6)?1:100;
    uintptr_t array{};memcpy(&array,family,8);uintptr_t source{};memcpy(&source,reinterpret_cast<void*>(array),8);
    memcpy(&fake_rendered_view,reinterpret_cast<void*>(source),sizeof(FakeView));
    uintptr_t owner=reinterpret_cast<uintptr_t>(destination)+8;memcpy(fake_rendered_view.prefix,&owner,8);
    fake_rendered_pointer=reinterpret_cast<uintptr_t>(&fake_rendered_view);
    struct Header{uintptr_t data;int count,capacity;} pointers{reinterpret_cast<uintptr_t>(&fake_rendered_pointer),1,1},storage{fake_rendered_pointer,1,1};
    memcpy(reinterpret_cast<char*>(destination)+8,&pointers,16);memcpy(reinterpret_cast<char*>(destination)+0x68,&storage,16);
    return destination;
}
__declspec(noinline) void* fake_clone(void* destination,void* source){
    ++clone_forward_count;memcpy(destination,source,sizeof(FakeView));return destination;
}
unsigned family_forward_count{};
void* expected_test_family{};
__declspec(noinline) void fake_family(void* canvas,void* family){
    if(canvas==reinterpret_cast<void*>(1) && family==expected_test_family)++family_forward_count;
    else family_forward_count+=100;
}
bool arguments_good{};
unsigned fake_state_factory_calls{},fake_state_release_calls{},fake_state_delete_calls{};
void* fake_state_pending{};
__declspec(noinline) void* fake_state_factory() {
    ++fake_state_factory_calls;
    void* value=VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(value)memcpy(value,&state_expected_table,8);return value;
}
__declspec(noinline) void fake_state_release(void* value) {++fake_state_release_calls;fake_state_pending=value;}
__declspec(noinline) void* fake_state_delete(void* value,unsigned flags) {
    ++fake_state_delete_calls;if(flags&1)VirtualFree(value,0,MEM_RELEASE);return value;
}
DWORD WINAPI fake_state_queue(void* value) {
    StateDeleteFn volatile invoke=fake_state_delete;return invoke(value,1)==value?0:1;
}
__declspec(noinline) void* fake_scene(void* lp,void* family,Vec* p,Rot* r,void* vp,void* drawer) {
    arguments_good=lp==reinterpret_cast<void*>(1) && family==reinterpret_cast<void*>(2) && vp==reinterpret_cast<void*>(3) && drawer==reinterpret_cast<void*>(4);
    fake_event(reinterpret_cast<void*>(5),p,r);
    for(int i=0;i<32;++i) fake_view.matrices[i]=static_cast<float>(i+10);
    return &fake_view;
}
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureStart(void*) {
    if(started.exchange(1)) return ERROR_ALREADY_EXISTS;
    wchar_t path[MAX_PATH];GetModuleFileNameW(own_module,path,MAX_PATH);
    wchar_t* slash=wcsrchr(path,L'\\');if(!slash) return ERROR_BAD_PATHNAME;*slash=0;wcscpy_s(directory,path);
    HANDLE thread=CreateThread(nullptr,0,worker,nullptr,0,nullptr);
    if(!thread) return GetLastError();CloseHandle(thread);return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureReserve(void*) {
    // Create trampolines before level loading fills nearby address space.
    // MH_CreateHook alone does not patch or enable the game entry points.
    if(reservation_started.exchange(1) || started.load())return ERROR_ALREADY_EXISTS;
    if(!hash_matches())return ERROR_REVISION_MISMATCH;
    base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    scene_target=reinterpret_cast<void*>(base+0x15cf08);event_target=reinterpret_cast<void*>(base+0x15c968);family_target=reinterpret_cast<void*>(base+0x3a148c);
    clone_target=reinterpret_cast<void*>(base+0x193d50);
    renderer_target=reinterpret_cast<void*>(base+0x116250);
    consume_target=reinterpret_cast<void*>(base+0x15a80c);destroy_renderer_target=reinterpret_cast<void*>(base+0x15a974);
    const unsigned char a[]={0x48,0x8b,0xc4,0x53,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57};
    const unsigned char b[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x08,0x48,0x89,0x68,0x10,0x48,0x89,0x70,0x18};
    const unsigned char c[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x18,0x55};
    const unsigned char d[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x10,0x48,0x89,0x68,0x18,0x48,0x89,0x70,0x20};
    const unsigned char e[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x18,0x48,0x89,0x70,0x20,0x57,0x41,0x54};
    const unsigned char f[]={0x48,0x89,0x5c,0x24,0x10,0x55,0x48,0x8b,0xec,0x48,0x83,0xec,0x70};
    prepare_target=reinterpret_cast<void*>(base+0x1fc060);
    const unsigned char h[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x10,0x48,0x89,0x70,0x18,0x57,0x41,0x54};
    const unsigned char g[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xd9};
    state_delete_target=reinterpret_cast<void*>(base+0x33bc70);
    const unsigned char j[]={0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x8b,0xda,0x48,0x8b,0xf9};
    unsigned char read[16];
    if(!safe_copy(read,scene_target,sizeof(a)) || memcmp(read,a,sizeof(a)) ||
       !safe_copy(read,event_target,sizeof(b)) || memcmp(read,b,sizeof(b)) ||
       !safe_copy(read,family_target,sizeof(c)) || memcmp(read,c,sizeof(c)) ||
       !safe_copy(read,clone_target,sizeof(d)) || memcmp(read,d,sizeof(d)) ||
       !safe_copy(read,renderer_target,sizeof(e)) || memcmp(read,e,sizeof(e)) ||
       !safe_copy(read,consume_target,sizeof(f)) || memcmp(read,f,sizeof(f)) ||
       !safe_copy(read,destroy_renderer_target,sizeof(g)) || memcmp(read,g,sizeof(g)) ||
       !safe_copy(read,prepare_target,sizeof(h)) || memcmp(read,h,sizeof(h)) ||
       !safe_copy(read,state_delete_target,sizeof(j)) || memcmp(read,j,sizeof(j)))return ERROR_INVALID_DATA;
    if(MH_Initialize()!=MH_OK)return ERROR_INVALID_STATE;
    if(!reserve_ui_hooks())return ERROR_NOT_ENOUGH_MEMORY;
    if(!reserve_graphics_lock_observer())return ERROR_INVALID_DATA;
    quad_target=reinterpret_cast<void*>(base+0x1af42c);
    geometry_target=reinterpret_cast<void*>(base+0x13cfcc);
    const unsigned char quad_signature[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x08,0x48,0x89,0x70,0x10,0x48,0x89,0x78,0x18};
    if(!safe_copy(read,quad_target,sizeof(quad_signature))||memcmp(read,quad_signature,sizeof(quad_signature)))return ERROR_INVALID_DATA;
    if(!safe_copy(read,geometry_target,sizeof(geometry_signature))||memcmp(read,geometry_signature,sizeof(geometry_signature)))return ERROR_INVALID_DATA;
    if(MH_CreateHook(geometry_target,reinterpret_cast<void*>(hook_geometry),reinterpret_cast<void**>(&original_geometry))!=MH_OK)return ERROR_NOT_ENOUGH_MEMORY;
    if(!safe_copy(read,geometry_target,sizeof(geometry_signature))||memcmp(read,geometry_signature,sizeof(geometry_signature)))return ERROR_INVALID_DATA;
    if(MH_CreateHook(quad_target,reinterpret_cast<void*>(hook_quad),reinterpret_cast<void**>(&original_quad))!=MH_OK)return ERROR_NOT_ENOUGH_MEMORY;
    if(!safe_copy(read,quad_target,sizeof(quad_signature))||memcmp(read,quad_signature,sizeof(quad_signature)))return ERROR_INVALID_DATA;
    if(MH_CreateHook(scene_target,reinterpret_cast<void*>(hook_scene),reinterpret_cast<void**>(&original_scene))!=MH_OK ||
       MH_CreateHook(event_target,reinterpret_cast<void*>(hook_event),reinterpret_cast<void**>(&original_event))!=MH_OK ||
       MH_CreateHook(family_target,reinterpret_cast<void*>(hook_family),reinterpret_cast<void**>(&original_family))!=MH_OK ||
       MH_CreateHook(clone_target,reinterpret_cast<void*>(hook_clone),reinterpret_cast<void**>(&original_clone))!=MH_OK ||
       MH_CreateHook(renderer_target,reinterpret_cast<void*>(hook_renderer),reinterpret_cast<void**>(&original_renderer))!=MH_OK ||
       MH_CreateHook(consume_target,reinterpret_cast<void*>(hook_consume),reinterpret_cast<void**>(&original_consume))!=MH_OK ||
       MH_CreateHook(destroy_renderer_target,reinterpret_cast<void*>(hook_destroy_renderer),reinterpret_cast<void**>(&original_destroy_renderer))!=MH_OK ||
       MH_CreateHook(prepare_target,reinterpret_cast<void*>(hook_prepare),reinterpret_cast<void**>(&original_prepare))!=MH_OK ||
       MH_CreateHook(state_delete_target,reinterpret_cast<void*>(hook_state_delete),reinterpret_cast<void**>(&original_state_delete))!=MH_OK)return ERROR_NOT_ENOUGH_MEMORY;
    // Independently re-read the entries to confirm reservation left them alone.
    if(!safe_copy(read,scene_target,sizeof(a)) || memcmp(read,a,sizeof(a)) ||
       !safe_copy(read,event_target,sizeof(b)) || memcmp(read,b,sizeof(b)) ||
       !safe_copy(read,family_target,sizeof(c)) || memcmp(read,c,sizeof(c)) ||
       !safe_copy(read,clone_target,sizeof(d)) || memcmp(read,d,sizeof(d)) ||
       !safe_copy(read,renderer_target,sizeof(e)) || memcmp(read,e,sizeof(e)) ||
       !safe_copy(read,consume_target,sizeof(f)) || memcmp(read,f,sizeof(f)) ||
       !safe_copy(read,destroy_renderer_target,sizeof(g)) || memcmp(read,g,sizeof(g)) ||
       !safe_copy(read,prepare_target,sizeof(h)) || memcmp(read,h,sizeof(h)) ||
       !safe_copy(read,state_delete_target,sizeof(j)) || memcmp(read,j,sizeof(j)))return ERROR_INVALID_DATA;
    wchar_t name[128];swprintf_s(name,L"Local\\GearsVR.Capture.Reserved.%lu",GetCurrentProcessId());
    reservation_event=CreateEventW(nullptr,TRUE,TRUE,name);
    if(!reservation_event)return GetLastError();reservation_ready=true;return 0;
}
// Optional reserve keeps older DLL launch paths compatible. Never enables the hook.
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureReserveFirstViewMask(void*) {
    if(!reservation_ready||started.load()||first_view_mask_reserved)return ERROR_INVALID_STATE;
    if(!hash_matches())return ERROR_REVISION_MISMATCH;
    first_view_mask_target=reinterpret_cast<void*>(base+0x2a7e48);
    unsigned char before[32]{},after[32]{},call[5]{};
    if(!safe_copy(before,first_view_mask_target,32)||memcmp(before,first_view_mask_signature,sizeof(first_view_mask_signature))||
       !safe_copy(call,reinterpret_cast<void*>(base+0x119bdd),5)||memcmp(call,first_view_mask_call_signature,5))return ERROR_INVALID_DATA;
    if(MH_CreateHook(first_view_mask_target,reinterpret_cast<void*>(hook_first_view_mask),reinterpret_cast<void**>(&original_first_view_mask))!=MH_OK)return ERROR_NOT_ENOUGH_MEMORY;
    if(!safe_copy(after,first_view_mask_target,32)||memcmp(before,after,32))return ERROR_INVALID_DATA;
    wchar_t name[128];swprintf_s(name,L"Local\\GearsVR.Capture.FirstViewMaskReserved.%lu",GetCurrentProcessId());
    first_view_mask_reserved_event=CreateEventW(nullptr,TRUE,TRUE,name);
    if(!first_view_mask_reserved_event)return GetLastError();first_view_mask_reserved=true;return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureTargetPrime(void*) {
    if(!reservation_ready||started.load())return ERROR_INVALID_STATE;
    return begin_target_prime();
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureTargetPrimeOwned(void* argument) {
    if(!reservation_ready||started.load())return ERROR_INVALID_STATE;
    gears_prime::OwnerArgument copied{};
    if(!argument||!safe_copy(&copied,argument,sizeof(copied)))return ERROR_INVALID_PARAMETER;
    return begin_target_prime(&copied);
}
namespace {
QuadTrace fake_quad_last{};unsigned fake_quad_calls{};
__declspec(noinline) void __fastcall fake_quad(float x,float y,float w,float h,float u,float v,float uw,float vh,
    unsigned tw,unsigned th,unsigned sw,unsigned sh){
    fake_quad_last={0,{x,y,w,h,u,v,uw,vh},{tw,th,sw,sh},0};++fake_quad_calls;
}
bool quad_source_forwarding_test(){
    using namespace gears_quad_source;
    Args right{{960,0,960,1080,960,0,960,1080},{1920,1080,1920,1080}};
    auto left=right;left.rectangle[0]=left.rectangle[4]=0;
    QuadFn volatile invoke=fake_quad;
    for(unsigned source=left_source;source<=right_source;++source){
        const Gate valid{true,true,true,true,true,0xd83b1c,0x1000,source==left_source?0x1d30U:0x1000U,2,2,source};
        for(unsigned i=0;i<8;++i){auto args=source==left_source?right:left;auto gate=valid;long long origin=10000,tick=14000;
            switch(i){case 0:gate.enabled=false;break;case 1:tick=12999;break;case 2:break;
            case 3:tick=16000;break;case 4:tick=20000;break;
            case 5:gate.current=source==left_source?0x1000:0x1d30;args=source==left_source?left:right;break;
            case 6:++gate.caller;break;case 7:++args.dimensions[2];break;}
            const auto decision=decide(args,gate,tick,1000,origin);auto expected=args;
            if(i==2)expected.rectangle[4]=source==left_source?0.f:960.f;
            // Mirror hook_quad's twelve-argument forwarding and exercise its real
            // Windows x64 float/stack ABI with a noinline target.
            const auto& r=args.rectangle;const auto& d=args.dimensions;
            invoke(r[0],r[1],r[2],r[3],decision.forwarded.rectangle[4],r[5],r[6],r[7],d[0],d[1],d[2],d[3]);
            if(fake_quad_last.rectangles!=expected.rectangle||fake_quad_last.dimensions!=expected.dimensions||
               fake_quad_calls!=source*8+i+1)return false;
        }
    }
    fake_quad_calls=0;fake_quad_last={};return true;
}
}
namespace {
void* geometry_expected_renderer{};void* geometry_expected_view{};
unsigned geometry_expected_stage{},geometry_expected_mode{},geometry_fake_calls{};bool geometry_fake_valid=true;
__declspec(noinline) unsigned __fastcall fake_geometry(void* renderer,void* view,unsigned stage,unsigned mode){
    ++geometry_fake_calls;
    geometry_fake_valid=geometry_fake_valid&&renderer==geometry_expected_renderer&&view==geometry_expected_view&&
        stage==geometry_expected_stage&&mode==geometry_expected_mode;
    geometry_draw(nullptr,true,9,2);geometry_draw(nullptr,false,6,1);
    return 0xc0ffee17U;
}
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureGeometryCheck(void*) {
    if(!final_input_decoder_test())return 152;
    unsigned char before[32]{},after[32]{};
    if(!safe_copy(before,reinterpret_cast<void*>(fake_geometry),32)||MH_Initialize()!=MH_OK)return 140;
    if(!install(reinterpret_cast<void*>(fake_geometry),reinterpret_cast<void*>(hook_geometry),reinterpret_cast<void**>(&original_geometry)))return 141;
    std::array<unsigned char,0x80> renderer{};std::array<unsigned char,0x1a60> views{};
    struct Header{uintptr_t data;int count,capacity;} header{reinterpret_cast<uintptr_t>(views.data()),2,2};
    memcpy(renderer.data()+0x68,&header,sizeof(header));
    uintptr_t list_values[3]={1,2,3};Header list{reinterpret_cast<uintptr_t>(list_values),3,3};
    memcpy(views.data()+0xd30+0x570+2*0x10,&list,sizeof(list));
    const auto clean_views=views;
    geometry_expected_renderer=renderer.data();geometry_expected_view=views.data()+0xd30;
    geometry_expected_stage=2;geometry_expected_mode=1;
    consuming_renderer=renderer.data();recording=true;viewport_diagnostics=true;trial_draw_scope=true;
    if(MH_ApplyQueued()!=MH_OK)return 142;
    GeometryFn volatile invoke=fake_geometry;
    if(invoke(renderer.data(),geometry_expected_view,2,1)!=0xc0ffee17U)return 143;
    auto& entry=geometry_traces[0];
    if(entry.eye!=1||entry.stage!=2||entry.mode!=1||entry.calls!=1||entry.count!=3||entry.valid!=15||
       entry.draws!=1||entry.indexed!=1||entry.elements!=24||current_geometry||!geometry_fake_valid)return 144;
    // No trial scope: original still runs, but no geometry evidence is recorded.
    trial_draw_scope=false;
    if(invoke(renderer.data(),geometry_expected_view,2,1)!=0xc0ffee17U||entry.calls!=1)return 145;
    // Misaligned view fails closed without changing original call/return.
    trial_draw_scope=true;geometry_expected_view=views.data()+1;
    if(invoke(renderer.data(),geometry_expected_view,2,1)!=0xc0ffee17U||geometry_rejected!=1||entry.calls!=1)return 146;
    geometry_expected_view=views.data();geometry_expected_stage=0;geometry_expected_mode=0;
    if(invoke(renderer.data(),geometry_expected_view,0,0)!=0xc0ffee17U||geometry_traces[1].empty_lists!=1)return 147;
    if(views!=clean_views||geometry_fake_calls!=4||!geometry_fake_valid||current_geometry)return 148;
    recording=false;trial_draw_scope=false;consuming_renderer=nullptr;
    if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK)return 149;
    if(!safe_copy(after,reinterpret_cast<void*>(fake_geometry),32)||memcmp(before,after,32)||active.load())return 150;
    if(invoke(renderer.data(),geometry_expected_view,0,0)!=0xc0ffee17U||geometry_fake_calls!=5)return 151;
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureResourceIdentityCheck(void*) {
    return resource_identity_test()?0:153;
}
namespace {
struct ColorCopyCall {ID3D12GraphicsCommandList* list{};const D3D12_TEXTURE_COPY_LOCATION* destination{};
    UINT x{},y{},z{};const D3D12_TEXTURE_COPY_LOCATION* source{};const D3D12_BOX* box{};};
ColorCopyCall color_copy_last{};unsigned color_copy_forwarded{};
__declspec(noinline) void STDMETHODCALLTYPE fake_color_copy(ID3D12GraphicsCommandList* list,const D3D12_TEXTURE_COPY_LOCATION* destination,
    UINT x,UINT y,UINT z,const D3D12_TEXTURE_COPY_LOCATION* source,const D3D12_BOX* box){
    color_copy_last={list,destination,x,y,z,source,box};++color_copy_forwarded;
}
}
namespace {
uint32_t first_view_mask_fake_bits{};unsigned first_view_mask_fake_calls{};
__declspec(noinline) void __fastcall fake_first_view_mask(float value){
    memcpy(&first_view_mask_fake_bits,&value,sizeof(value));++first_view_mask_fake_calls;
}
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureFirstViewMaskCheck(void*) {
    using namespace gears_first_view_mask;
    if(!self_test())return 180;
    unsigned char before[32]{},after[32]{};
    if(!safe_copy(before,reinterpret_cast<void*>(fake_first_view_mask),32)||MH_Initialize()!=MH_OK||
       !install(reinterpret_cast<void*>(fake_first_view_mask),reinterpret_cast<void*>(hook_first_view_mask),reinterpret_cast<void**>(&original_first_view_mask)))return 181;
    if(MH_ApplyQueued()!=MH_OK)return 182;
    LARGE_INTEGER frequency;QueryPerformanceFrequency(&frequency);qpc_frequency=frequency.QuadPart;
    FirstViewMaskFn volatile invoke=fake_first_view_mask;
    const Gate valid{true,true,true,true,true,0x119be2};const auto sample=valid_test_snapshot();long long origin=10000;
    const long long ticks[]={12999,13000,15999,16000,19999,20000};
    // Pure guard decisions feed an actual x64 float call through MinHook and
    // the production hook/trampoline. No production caller/structure bypass.
    for(unsigned i=0;i<6;++i){const auto decision=decide(0,valid,sample,ticks[i],1000,origin);
        const auto calls=first_view_mask_fake_calls;invoke(decision.forwarded);
        if(first_view_mask_fake_calls!=calls+1||first_view_mask_fake_bits!=((i==1||i==2)?0x3f800000U:0U))return 183;
    }
    const uint32_t patterns[]={0,0x80000000U,0xc0100000U,0x3f900000U,0x7fc12345U,0x7f800000U};
    first_view_mask_control=true;recording=true;viewport_diagnostics=true;trial_draw_scope=true;
    consuming_renderer=reinterpret_cast<void*>(1); // Must be ignored at the wrong return address.
    for(auto bits:patterns){float value{};memcpy(&value,&bits,4);
        const auto decision=decide(value,valid,sample,14000,1000,origin);
        uint32_t effective{};memcpy(&effective,&decision.forwarded,4);
        if(effective!=(bits?bits:0x3f800000U)||decision.modified!=(bits==0))return 184;
        const auto calls=first_view_mask_fake_calls;invoke(value);
        if(first_view_mask_fake_calls!=calls+1||first_view_mask_fake_bits!=bits||first_view_mask_origin)return 185;
    }
    Snapshot invalid{};if(collect_first_view_mask(1,1,invalid)||invalid.readable)return 186;
    if(!create_first_view_mask_events())return 187;
    // An expired clock must reset a formerly signalled phase, including when
    // no more eligible native calls arrive during capture shutdown.
    {std::lock_guard<std::mutex> lock(first_view_mask_mutex);first_view_mask_origin=now()-11*qpc_frequency;
        SetEvent(first_view_mask_events[1]);first_view_mask_event_phase=gears_quad_source::b;}
    refresh_first_view_mask_events();
    for(auto event:first_view_mask_events)if(WaitForSingleObject(event,0)!=WAIT_TIMEOUT)return 188;
    close_first_view_mask_events();for(auto event:first_view_mask_events)if(event)return 189;
    first_view_mask_control=false;recording=false;trial_draw_scope=false;consuming_renderer=nullptr;
    if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK||!safe_copy(after,reinterpret_cast<void*>(fake_first_view_mask),32)||memcmp(before,after,32)||active.load())return 190;
    const auto calls=first_view_mask_fake_calls;invoke(.25f);
    if(first_view_mask_fake_calls!=calls+1||first_view_mask_fake_bits!=0x3e800000U)return 191;
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureViewUnionMaskCheck(void*) {
    using namespace gears_view_union_mask;
    if(!self_test()||!lifetime_boundary_self_test())return 200;
    unsigned char before[32]{},after[32]{};
    if(!safe_copy(before,reinterpret_cast<void*>(fake_first_view_mask),32)||MH_Initialize()!=MH_OK||
       !install(reinterpret_cast<void*>(fake_first_view_mask),reinterpret_cast<void*>(hook_first_view_mask),reinterpret_cast<void**>(&original_first_view_mask)))return 201;
    if(MH_ApplyQueued()!=MH_OK)return 202;
    LARGE_INTEGER frequency;QueryPerformanceFrequency(&frequency);qpc_frequency=frequency.QuadPart;
    FirstViewMaskFn volatile invoke=fake_first_view_mask;
    const Gate valid{true,true,true,true,true,0x119be2};long long origin=10000;
    const std::array<int,4> geometries[]={{1920,1080,960,960},{1919,1079,959,960},{1920,1080,640,960}};
    for(const auto& geometry:geometries){const auto sample=sample_for(geometry[0],geometry[1],geometry[2],geometry[3]);
        const auto input=boundary(static_cast<float>(geometry[2]),static_cast<float>(geometry[0]));
        const auto decision=decide_union(input,valid,sample,20000,1000,origin);const auto calls=first_view_mask_fake_calls;
        invoke(decision.forwarded);
        if(decision.rejection||first_view_mask_fake_calls!=calls+1||first_view_mask_fake_bits!=bits(decision.union_t))return 203;
    }
    const auto sample=sample_for(1920,1080,960,960);
    // A complete tracked consumer remains corrected across producer expiry.
    for(auto tick:{39999LL,40000LL,40001LL}){const auto decision=decide_union(0,valid,sample,tick,1000,origin);invoke(decision.forwarded);
        if(first_view_mask_fake_bits!=0x3f800000U)return 204;}
    view_union_mask_fix=true;recording=true;viewport_diagnostics=true;trial_draw_scope=true;consuming_renderer=reinterpret_cast<void*>(1);
    const uint32_t patterns[]={0,0x80000000U,0xc0100000U,0x3f900000U,0x7fc12345U,0x7f800000U};
    for(auto raw:patterns){float value{};memcpy(&value,&raw,4);const auto calls=first_view_mask_fake_calls;invoke(value);
        if(first_view_mask_fake_calls!=calls+1||first_view_mask_fake_bits!=raw||view_union_mask_origin)return 205;}
    if(view_union_mask_trace.calls!=6||view_union_mask_trace.rejected!=6||view_union_mask_trace.accepted||view_union_mask_trace.modified||
       view_union_mask_trace.reasons[gears_view_union_mask::caller]!=6)return 206;
    // Detailed geometry capacity cannot truncate accepted/modified totals.
    for(unsigned i=0;i<17;++i){auto s=sample_for(1920+i,1080,960,960+i);const auto input=boundary(960.f,static_cast<float>(1920+i));
        auto decision=decide_union(input,valid,s,20000,1000,origin);complete_view_union_mask(input,decision,s);}
    if(view_union_mask_trace.calls!=23||view_union_mask_trace.accepted!=17||view_union_mask_trace.modified!=17||
       view_union_mask_trace.rejected!=6||view_union_mask_trace.overflow!=1||!view_union_mask_trace.first||view_union_mask_trace.last<view_union_mask_trace.first)return 207;
    view_union_mask_fix=false;recording=false;trial_draw_scope=false;consuming_renderer=nullptr;
    if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK||!safe_copy(after,reinterpret_cast<void*>(fake_first_view_mask),32)||memcmp(before,after,32)||active.load())return 208;
    const auto calls=first_view_mask_fake_calls;invoke(.25f);
    if(first_view_mask_fake_calls!=calls+1||first_view_mask_fake_bits!=0x3e800000U)return 209;
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureColorCopyCheck(void*) {
    ID3D12Device* device{};ID3D12Resource* source_resource{};ID3D12Resource* destination_resource{};
    if(FAILED(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))))return 160;
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC description{};description.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;description.Width=16;description.Height=8;
    description.DepthOrArraySize=1;description.MipLevels=1;description.Format=DXGI_FORMAT_R8G8B8A8_UNORM;description.SampleDesc.Count=1;
    if(FAILED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&description,D3D12_RESOURCE_STATE_COPY_SOURCE,nullptr,IID_PPV_ARGS(&source_resource)))||
       FAILED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&description,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&destination_resource))))return 161;
    alignas(8) std::array<unsigned char,0x90> source_engine{},destination_engine{};
    alignas(8) std::array<unsigned char,0x140> source_texture{},destination_texture{};
    alignas(8) std::array<unsigned char,0x20> owner{};
    alignas(8) std::array<unsigned char,0x70> bound{};
    alignas(8) std::array<unsigned char,0x80> renderer{};
    alignas(8) std::array<unsigned char,0x1a60> views{};
    auto pointer=[](auto& bytes,size_t offset,uintptr_t value){memcpy(bytes.data()+offset,&value,8);};
    pointer(source_engine,0,base+0x133ecf8);pointer(destination_engine,0,base+0x133ecf8);
    pointer(source_engine,0xc,reinterpret_cast<uintptr_t>(source_resource));pointer(destination_engine,0xc,reinterpret_cast<uintptr_t>(destination_resource));
    pointer(owner,0xc,reinterpret_cast<uintptr_t>(source_engine.data()));pointer(source_texture,0xc,reinterpret_cast<uintptr_t>(owner.data()));
    pointer(bound,0,base+0x1667bb8);pointer(bound,0x60,reinterpret_cast<uintptr_t>(destination_engine.data()));
    pointer(destination_texture,0x38,reinterpret_cast<uintptr_t>(bound.data()));
    uintptr_t source_public=reinterpret_cast<uintptr_t>(source_texture.data())+0x13c,destination_public=reinterpret_cast<uintptr_t>(destination_texture.data())+0x13c;
    color_copy_test_source_global=reinterpret_cast<uintptr_t>(&source_public);color_copy_test_destination_global=reinterpret_cast<uintptr_t>(&destination_public);
    struct Header {uintptr_t data;int count,capacity;} header{reinterpret_cast<uintptr_t>(views.data()),2,2};memcpy(renderer.data()+0x68,&header,sizeof(header));
    ColorCopyLinks links{};
    if(!color_copy_links(color_copy_test_source_global,color_copy_test_destination_global,links)||
       links.source!=reinterpret_cast<uintptr_t>(source_resource)||links.destination!=reinterpret_cast<uintptr_t>(destination_resource))return 162;
    D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=source_resource;dst.pResource=destination_resource;
    src.Type=dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;src.SubresourceIndex=7;dst.SubresourceIndex=9;
    D3D12_BOX box{1,2,3,4,5,6};const auto clean_source=src,clean_destination=dst;const auto clean_box=box;
    unsigned char before[32]{},after[32]{};
    if(!safe_copy(before,reinterpret_cast<void*>(fake_color_copy),32)||MH_Initialize()!=MH_OK||
       !install(reinterpret_cast<void*>(fake_color_copy),reinterpret_cast<void*>(hook_color_copy),reinterpret_cast<void**>(&original_color_copy)))return 163;
    color_copy_diagnostics=true;viewport_diagnostics=true;self_testing=true;recording=true;trial_draw_scope=true;consuming_renderer=renderer.data();
    if(MH_ApplyQueued()!=MH_OK)return 164;
    ColorCopyFn volatile invoke=fake_color_copy;auto* list=reinterpret_cast<ID3D12GraphicsCommandList*>(0x12345678);
    auto call=[&](const D3D12_TEXTURE_COPY_LOCATION* destination,const D3D12_TEXTURE_COPY_LOCATION* source,const D3D12_BOX* bounds){
        const auto previous=color_copy_forwarded;invoke(list,destination,11,13,17,source,bounds);
        return color_copy_forwarded==previous+1&&color_copy_last.list==list&&color_copy_last.destination==destination&&
            color_copy_last.x==11&&color_copy_last.y==13&&color_copy_last.z==17&&color_copy_last.source==source&&color_copy_last.box==bounds;
    };
    if(!call(&dst,&src,nullptr)||!call(&dst,&src,&box))return 165;
    bool found_null=false,found_box=false;uint64_t recorded{};
    for(const auto& e:color_copy_traces)if(e.calls){recorded+=e.calls;
        if(e.source_subresource!=7||e.destination_subresource!=9||e.relation!=3||e.xyz!=std::array<unsigned,3>{11,13,17})return 166;
        found_null|=!e.has_box;found_box|=e.has_box&&e.box==std::array<unsigned,6>{1,2,3,4,5,6};
    }
    if(recorded!=2||!found_null||!found_box)return 167;
    auto other_src=src,other_dst=dst;other_src.pResource=reinterpret_cast<ID3D12Resource*>(0x43210000);other_dst.pResource=reinterpret_cast<ID3D12Resource*>(0x43220000);
    if(!call(&other_dst,&other_src,&box)||color_copy_unrelated!=1)return 168;
    if(!call(reinterpret_cast<const D3D12_TEXTURE_COPY_LOCATION*>(1),&src,&box)||color_copy_args_rejected!=1)return 169;
    const auto old_global=color_copy_test_destination_global;color_copy_test_destination_global=1;
    if(!call(&dst,&src,&box)||color_copy_links_rejected!=1)return 170;
    color_copy_test_destination_global=old_global;
    if(!call(&other_dst,&src,nullptr)||!call(&dst,&other_src,nullptr))return 171;
    ColorCopyTrace decoded{};auto placed=src;placed.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    placed.PlacedFootprint.Offset=512;placed.PlacedFootprint.Footprint={DXGI_FORMAT_R8G8B8A8_UNORM,16,8,1,256};
    if(decode_color_copy(links,&dst,0,0,0,&placed,nullptr,decoded)||decoded.source_subresource!=UINT_MAX||
       decoded.source_footprint.Offset!=512||decoded.source_footprint.Footprint.RowPitch!=256)return 172;
    const auto attempts=color_copy_attempts;trial_draw_scope=false;
    if(!call(&dst,&src,&box)||color_copy_attempts!=attempts)return 173;
    if(memcmp(&src,&clean_source,sizeof(src))||memcmp(&dst,&clean_destination,sizeof(dst))||memcmp(&box,&clean_box,sizeof(box)))return 174;
    recording=false;trial_draw_scope=false;consuming_renderer=nullptr;self_testing=false;color_copy_diagnostics=false;
    if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK||!safe_copy(after,reinterpret_cast<void*>(fake_color_copy),32)||memcmp(before,after,32)||active.load())return 175;
    if(!call(&dst,&src,nullptr)||color_copy_attempts!=attempts)return 176;
    // Different command lists merge; classification still counts every call
    // after the bounded detailed trace fills up.
    color_copy_traces={};color_copy_overflow=color_copy_matched=color_copy_full_subresource_pair=0;
    color_copy_partial_or_other_pair=color_copy_source_only=color_copy_destination_only=0;
    ColorCopyTrace aggregate{};aggregate.relation=3;aggregate.list=0x1000;
    record_color_copy(aggregate);aggregate.list=0x2000;record_color_copy(aggregate);
    if(color_copy_traces[0].calls!=2||color_copy_traces[0].list!=0x1000||color_copy_traces[1].calls)return 177;
    for(unsigned i=1;i<40;++i){aggregate.caller=i;record_color_copy(aggregate);}
    aggregate.has_box=true;record_color_copy(aggregate);
    aggregate.relation=1;record_color_copy(aggregate);aggregate.relation=2;record_color_copy(aggregate);
    if(color_copy_matched!=44||color_copy_full_subresource_pair!=41||color_copy_partial_or_other_pair!=1||
       color_copy_source_only!=1||color_copy_destination_only!=1||color_copy_overflow!=11)return 178;
    source_resource->Release();destination_resource->Release();device->Release();return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureQuadCheck(void*) {
    if(!gears_quad_source::self_test()||!quad_source_forwarding_test())return 137;
    unsigned char before[32]{},after[32]{};
    if(!safe_copy(before,reinterpret_cast<void*>(fake_quad),32)||MH_Initialize()!=MH_OK)return 130;
    if(!install(reinterpret_cast<void*>(fake_quad),reinterpret_cast<void*>(hook_quad),reinterpret_cast<void**>(&original_quad)))return 131;
    recording=true;viewport_diagnostics=true;self_testing=true;trial_draw_scope=true;
    if(MH_ApplyQueued()!=MH_OK)return 132;
    QuadFn volatile invoke=fake_quad;
    invoke(1.25f,2.5f,3.75f,4.25f,5.5f,6.75f,7.25f,8.5f,901,1002,1103,1204);
    if(fake_quad_calls!=1||fake_quad_last.rectangles!=std::array<float,8>{1.25f,2.5f,3.75f,4.25f,5.5f,6.75f,7.25f,8.5f}||
       fake_quad_last.dimensions!=std::array<unsigned,4>{901,1002,1103,1204}||quad_traces[0].calls!=1||
       quad_traces[0].rectangles!=fake_quad_last.rectangles||quad_traces[0].dimensions!=fake_quad_last.dimensions)return 133;
    trial_draw_scope=false;invoke(-1,-2,-3,-4,-5,-6,-7,-8,11,12,13,14);
    if(fake_quad_calls!=2||fake_quad_last.rectangles[7]!=-8||fake_quad_last.dimensions[3]!=14||quad_traces[0].calls!=1||quad_traces[1].calls)return 134;
    recording=false;if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK)return 135;
    if(!safe_copy(after,reinterpret_cast<void*>(fake_quad),32)||memcmp(before,after,32)||active.load())return 136;
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureFamilyCheck(void*) {
    uintptr_t views[]={reinterpret_cast<uintptr_t>(&fake_view),reinterpret_cast<uintptr_t>(&fake_view)+16};
    struct Header{uintptr_t data;int count,capacity;} header{reinterpret_cast<uintptr_t>(views),2,2};
    expected_test_family=&header;unsigned char before[32],after[32];
    if(!safe_copy(before,reinterpret_cast<void*>(fake_family),32) || MH_Initialize()!=MH_OK)return 70;
    if(!install(reinterpret_cast<void*>(fake_family),reinterpret_cast<void*>(hook_family),reinterpret_cast<void**>(&original_family)))return 71;
    recording=true;if(MH_ApplyQueued()!=MH_OK)return 72;
    FamilyFn volatile invoke=fake_family;invoke(reinterpret_cast<void*>(1),&header);
    recording=false;if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK)return 73;
    if(!safe_copy(after,reinterpret_cast<void*>(fake_family),32) || memcmp(before,after,32) || active.load() || family_forward_count!=1 || write_index!=1)return 74;
    const auto& record=ring[0];
    if(record.kind!=7 || !(record.valid&128) || record.family_count!=2 || record.family_views[0]!=views[0] || record.family_views[1]!=views[1] || record.family_address!=reinterpret_cast<uintptr_t>(&header))return 75;
    Record invalid{};header.count=5;header.capacity=5;inspect_family(invalid,&header);if(invalid.valid&128)return 76;
    header.count=2;header.capacity=1;inspect_family(invalid,&header);if(invalid.valid&128)return 77;
    header.count=1;header.capacity=1;header.data=8;inspect_family(invalid,&header);if(invalid.valid&128)return 78;
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureCloneCheck(void*) {
    FakeView source{},destination{};uintptr_t owner=0x12345678;
    memcpy(source.prefix,&owner,8);for(unsigned i=0;i<32;++i)source.matrices[i]=float(i)+.25f;
    uintptr_t views[]={reinterpret_cast<uintptr_t>(&source)};
    struct Header{uintptr_t data;int count,capacity;} header{reinterpret_cast<uintptr_t>(views),1,17};
    unsigned char before[32],after[32];
    if(!safe_copy(before,reinterpret_cast<void*>(fake_clone),32) || MH_Initialize()!=MH_OK)return 80;
    if(!install(reinterpret_cast<void*>(fake_clone),reinterpret_cast<void*>(hook_clone),reinterpret_cast<void**>(&original_clone)))return 81;
    recording=true;if(MH_ApplyQueued()!=MH_OK)return 82;
    submitting_family=&header;CloneFn volatile invoke=fake_clone;void* result=invoke(&destination,&source);submitting_family=nullptr;
    // Unrelated calls must forward but must not generate a family observation.
    invoke(&destination,&source);
    recording=false;if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK)return 83;
    if(!safe_copy(after,reinterpret_cast<void*>(fake_clone),32) || memcmp(before,after,32) || active.load() || clone_forward_count!=2 || write_index!=1)return 84;
    const auto& record=ring[0];
    if(result!=&destination || record.kind!=8 || record.object!=reinterpret_cast<uintptr_t>(&destination) || record.result!=record.object || record.clone_source!=views[0] || record.clone_owner!=owner || record.valid!=(128|256|512|1024))return 85;
    if(memcmp(record.matrices.data(),source.matrices,128) || memcmp(record.source_matrices.data(),source.matrices,128) || memcmp(&source,&destination,sizeof(source)))return 86;
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureRendererCheck(void*) {
    FakeView source{};for(unsigned i=0;i<32;++i)source.matrices[i]=float(i)-.125f;
    uintptr_t views[]={reinterpret_cast<uintptr_t>(&source)};
    struct Header{uintptr_t data;int count,capacity;} header{reinterpret_cast<uintptr_t>(views),1,17};
    alignas(8) unsigned char destination[0x140]{};unsigned char before[32],after[32];
    if(!safe_copy(before,reinterpret_cast<void*>(fake_renderer),32) || MH_Initialize()!=MH_OK)return 90;
    if(!install(reinterpret_cast<void*>(fake_renderer),reinterpret_cast<void*>(hook_renderer),reinterpret_cast<void**>(&original_renderer)))return 91;
    recording=true;if(MH_ApplyQueued()!=MH_OK)return 92;
    submitting_family=&header;RendererFn volatile invoke=fake_renderer;
    void* result=invoke(destination,&header,reinterpret_cast<void*>(5),reinterpret_cast<void*>(6));submitting_family=nullptr;
    invoke(destination,&header,reinterpret_cast<void*>(5),reinterpret_cast<void*>(6));
    recording=false;if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK)return 93;
    if(!safe_copy(after,reinterpret_cast<void*>(fake_renderer),32) || memcmp(before,after,32) || active.load() || renderer_forward_count!=2 || write_index!=1)return 94;
    const auto& r=ring[0];uintptr_t owner=reinterpret_cast<uintptr_t>(destination)+8;
    if(result!=destination || r.kind!=9 || r.result!=r.object || r.valid!=(128|256|512|1024|2048|4096) || r.renderer_input_family!=reinterpret_cast<uintptr_t>(&header) || r.renderer_sources[0]!=views[0] || r.family_views[0]!=fake_rendered_pointer || r.renderer_storage!=fake_rendered_pointer || r.renderer_count!=1 || r.clone_owner!=owner || r.family_address!=owner)return 95;
    if(memcmp(r.matrices.data(),source.matrices,128) || memcmp(r.source_matrices.data(),source.matrices,128))return 96;
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureLifetimeCheck(void*) {
    FakeView source{};for(unsigned i=0;i<32;++i)source.matrices[i]=float(i)+.5f;
    uintptr_t views[]={reinterpret_cast<uintptr_t>(&source)};
    struct Header{uintptr_t data;int count,capacity;} header{reinterpret_cast<uintptr_t>(views),1,17};
    void* destination=VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!destination)return 100;
    fake_renderer(destination,&header,reinterpret_cast<void*>(5),reinterpret_cast<void*>(6));
    unsigned char consume_before[32],destroy_before[32],after[32];
    if(!safe_copy(consume_before,reinterpret_cast<void*>(fake_consume),32) || !safe_copy(destroy_before,reinterpret_cast<void*>(fake_destroy_renderer),32) || MH_Initialize()!=MH_OK)return 101;
    if(!install(reinterpret_cast<void*>(fake_consume),reinterpret_cast<void*>(hook_consume),reinterpret_cast<void**>(&original_consume)) ||
       !install(reinterpret_cast<void*>(fake_destroy_renderer),reinterpret_cast<void*>(hook_destroy_renderer),reinterpret_cast<void**>(&original_destroy_renderer)))return 102;
    recording=true;if(MH_ApplyQueued()!=MH_OK)return 103;
    ConsumeFn volatile invoke=fake_consume;invoke(destination);
    recording=false;if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK)return 104;
    if(!safe_copy(after,reinterpret_cast<void*>(fake_consume),32) || memcmp(after,consume_before,32) || !safe_copy(after,reinterpret_cast<void*>(fake_destroy_renderer),32) || memcmp(after,destroy_before,32) || active.load())return 105;
    if(consume_forward_count!=1 || destroy_forward_count!=1 || write_index!=2 || ring[0].kind!=11 || ring[1].kind!=10)return 106;
    // The original really releases the page. Observation must be pre-call.
    if(safe_copy(after,destination,8))return 107;
    for(unsigned i=0;i<2;++i){const auto& r=ring[i];
        if(r.object!=reinterpret_cast<uintptr_t>(destination) || r.valid!=(128|512|1024|4096) || r.family_views[0]!=fake_rendered_pointer || r.clone_owner!=reinterpret_cast<uintptr_t>(destination)+8 || memcmp(r.matrices.data(),source.matrices,128))return 108;
    }
    if(ring[0].result!=reinterpret_cast<uintptr_t>(destination))return 109;
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCapturePrepareCheck(void*) {
    FakeView source{};for(unsigned i=0;i<32;++i)source.matrices[i]=float(i)+.5f;
    uintptr_t initial_state=0x1111;memcpy(source.prefix+8,&initial_state,8);
    uintptr_t views[]={reinterpret_cast<uintptr_t>(&source)};
    struct Header{uintptr_t data;int count,capacity;} header{reinterpret_cast<uintptr_t>(views),1,17};
    alignas(8) unsigned char destination[0x140]{};fake_renderer(destination,&header,reinterpret_cast<void*>(5),reinterpret_cast<void*>(6));
    unsigned char before[32],after[32];
    if(!safe_copy(before,reinterpret_cast<void*>(fake_prepare),32) || MH_Initialize()!=MH_OK)return 110;
    if(!install(reinterpret_cast<void*>(fake_prepare),reinterpret_cast<void*>(hook_prepare),reinterpret_cast<void**>(&original_prepare)))return 111;
    recording=true;if(MH_ApplyQueued()!=MH_OK)return 112;
    consuming_renderer=destination;ConsumeFn volatile invoke=fake_prepare;invoke(destination);consuming_renderer=nullptr;
    invoke(destination);recording=false;if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK)return 113;
    if(!safe_copy(after,reinterpret_cast<void*>(fake_prepare),32) || memcmp(before,after,32) || active.load() || prepare_forward_count!=2 || write_index!=1)return 114;
    const auto& r=ring[0];
    if(r.kind!=12 || r.object!=reinterpret_cast<uintptr_t>(destination) || r.clone_source!=r.family_views[0] || r.valid!=(128|256|512|1024|4096|8192|16384) || r.renderer_sources[0]!=0x1111 || r.renderer_sources[1]!=0x2222)return 115;
    if(memcmp(r.source_matrices.data(),source.matrices,128) || r.matrices[24]!=.125f || r.matrices[25]!=-.25f)return 116;
    for(unsigned i=0;i<32;++i)if(i!=24 && i!=25 && r.matrices[i]!=source.matrices[i])return 117;
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureStateCheck(void*) {
    FakeView source{};uintptr_t prior=0x1111;memcpy(source.prefix+8,&prior,8);
    Record record{};record.caller=0x2c8909;record.valid=128;record.family_count=1;
    record.family_views[0]=reinterpret_cast<uintptr_t>(&source);
    state_factory=fake_state_factory;state_release=fake_state_release;state_expected_table=0x12345678;
    unsigned char before[32],after[32];
    if(!safe_copy(before,reinterpret_cast<void*>(fake_state_delete),32) || MH_Initialize()!=MH_OK)return 120;
    if(!install(reinterpret_cast<void*>(fake_state_delete),reinterpret_cast<void*>(hook_state_delete),reinterpret_cast<void**>(&original_state_delete)))return 121;
    recording=true;shadow_state_enabled=true;if(MH_ApplyQueued()!=MH_OK)return 122;
    // Wrong context/count must not allocate; the eligible context acts once.
    record.family_count=2;try_shadow_state(record);record.family_count=1;
    record.caller=0;try_shadow_state(record);record.caller=0x2c8909;
    try_shadow_state(record);try_shadow_state(record);
    if(fake_state_factory_calls!=1 || fake_state_release_calls!=1 || state_created!=1 || state_release_returned!=1 || state_original_preserved!=1 || state_destroyed || !fake_state_pending)return 123;
    HANDLE thread=CreateThread(nullptr,0,fake_state_queue,fake_state_pending,0,nullptr);if(!thread)return 124;
    DWORD waited=WaitForSingleObject(thread,5000),exit_code{};GetExitCodeThread(thread,&exit_code);CloseHandle(thread);
    if(waited!=WAIT_OBJECT_0 || exit_code || fake_state_delete_calls!=1 || state_destroyed!=1 || state_delete_flags!=1 || state_delete_thread==GetCurrentThreadId())return 125;
    if(safe_copy(after,fake_state_pending,8))return 126;
    recording=false;shadow_state_enabled=false;
    if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK || !safe_copy(after,reinterpret_cast<void*>(fake_state_delete),32) || memcmp(before,after,32) || active.load())return 127;
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureSelfTest(void*) {
    base=0;expected_local=1;expected_pc=5;
    unsigned char scene_before[32],event_before[32];
    if(!safe_copy(scene_before,reinterpret_cast<void*>(fake_scene),32) ||
       !safe_copy(event_before,reinterpret_cast<void*>(fake_event),32)) return 9;
    if(MH_Initialize()!=MH_OK) return 10;
    if(!install(reinterpret_cast<void*>(fake_scene),reinterpret_cast<void*>(hook_scene),reinterpret_cast<void**>(&original_scene)) ||
       !install(reinterpret_cast<void*>(fake_event),reinterpret_cast<void*>(hook_event),reinterpret_cast<void**>(&original_event))) return 11;
    recording=true;if(MH_ApplyQueued()!=MH_OK) return 12;
    Vec p{1,2,3};Rot rot{4,5,6};
    auto invoke=reinterpret_cast<SceneFn>(reinterpret_cast<void*>(fake_scene));
    void* result=invoke(reinterpret_cast<void*>(1),reinterpret_cast<void*>(2),&p,&rot,reinterpret_cast<void*>(3),reinterpret_cast<void*>(4));
    recording=false;if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK) return 13;
    if(!arguments_good || result!=&fake_view || p.x!=6 || p.y!=2 || p.z!=3 || rot.yaw!=12) return 14;
    if(write_index!=2 || dropped.load()!=0 || ring[0].kind!=2 || ring[1].kind!=1 || ring[1].valid!=23) return 15;
    for(int i=0;i<32;++i) if(ring[1].matrices[i]!=i+10) return 16;
    unsigned char after[32];
    if(!safe_copy(after,reinterpret_cast<void*>(fake_scene),32) || memcmp(after,scene_before,32) ||
       !safe_copy(after,reinterpret_cast<void*>(fake_event),32) || memcmp(after,event_before,32)) return 17;
    Vec p2{1,2,3};Rot r2{4,5,6};
    if(invoke(reinterpret_cast<void*>(1),reinterpret_cast<void*>(2),&p2,&r2,reinterpret_cast<void*>(3),reinterpret_cast<void*>(4))!=&fake_view ||
       p2.x!=6 || r2.yaw!=12 || write_index!=2) return 18;
    // Apply only through a matching scene context/caller; a standalone event
    // must retain its original yaw, and disabling hooks must restore behavior.
    allowed_caller=ring[0].caller;self_testing=true;wobble_enabled=true;recording=true;
    if(MH_QueueEnableHook(reinterpret_cast<void*>(fake_scene))!=MH_OK ||
       MH_QueueEnableHook(reinterpret_cast<void*>(fake_event))!=MH_OK || MH_ApplyQueued()!=MH_OK) return 19;
    Vec p3{1,2,3};Rot r3{4,5,6};
    invoke(reinterpret_cast<void*>(1),reinterpret_cast<void*>(2),&p3,&r3,reinterpret_cast<void*>(3),reinterpret_cast<void*>(4));
    if(r3.yaw!=12+offset_at(4) || p3.x!=6 || ring[3].yaw_delta!=offset_at(4)) return 20;
    Vec p4{1,2,3};Rot r4{4,5,6};fake_event(reinterpret_cast<void*>(5),&p4,&r4);
    if(r4.yaw!=12 || ring[4].yaw_delta!=0) return 21;
    recording=false;if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK) return 22;
    if(offset_at(2)!=0 || offset_at(15)!=0 || offset_at(4)<=0 || offset_at(6)>=0) return 23;
    wobble_enabled=false;sweep_enabled=true;recording=true;wobble_state=0;wobble_start=0;
    if(MH_QueueEnableHook(reinterpret_cast<void*>(fake_scene))!=MH_OK ||
       MH_QueueEnableHook(reinterpret_cast<void*>(fake_event))!=MH_OK || MH_ApplyQueued()!=MH_OK)return 24;
    for(unsigned phase=1;phase<=6;++phase) {
        selftest_elapsed=4+(phase-1)*4;Vec test_p{1,2,3};Rot test_r{4,5,6};
        invoke(reinterpret_cast<void*>(1),reinterpret_cast<void*>(2),&test_p,&test_r,reinterpret_cast<void*>(3),reinterpret_cast<void*>(4));
        const auto& record=ring[(write_index-1)%ring.size()];
        if(record.pose_phase!=phase)return 25;
        if(phase<=3) {
            if(test_r.pitch!=4+(phase==1?offset_at(4):0) || test_r.yaw!=12+(phase==2?offset_at(4):0) ||
               test_r.roll!=6+(phase==3?offset_at(4):0))return 26;
        } else if(std::fabs(std::sqrt(record.translation_delta.x*record.translation_delta.x+
                       record.translation_delta.y*record.translation_delta.y+record.translation_delta.z*record.translation_delta.z)-2)>0.001)return 27;
    }
    selftest_elapsed=28;Vec end_p{1,2,3};Rot end_r{4,5,6};
    invoke(reinterpret_cast<void*>(1),reinterpret_cast<void*>(2),&end_p,&end_r,reinterpret_cast<void*>(3),reinterpret_cast<void*>(4));
    if(wobble_state!=2 || end_p.x!=6 || end_r.pitch!=4 || end_r.yaw!=12 || end_r.roll!=6)return 28;
    recording=false;if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK)return 29;
    // Real shared-memory producer/consumer through the hooked matching caller.
    sweep_enabled=false;live_enabled=true;live_gate.arm(true);
    gears_vr::PoseBridge producer,duplicate;
    if(!producer.open(GetCurrentProcessId(),true) || duplicate.open(GetCurrentProcessId(),true) ||
       !live_bridge.open(GetCurrentProcessId(),false))return 30;
    gears_vr::PoseSample sample{{{0,0,0},{0,0,0,1}},1,0,15,true};
    recording=true;
    if(MH_QueueEnableHook(reinterpret_cast<void*>(fake_scene))!=MH_OK ||
       MH_QueueEnableHook(reinterpret_cast<void*>(fake_event))!=MH_OK || MH_ApplyQueued()!=MH_OK)return 32;
    // Hook application can suspend/enumerate many threads under game load. Test
    // fresh pose consumption after setup, rather than timing that setup as pose age.
    sample.received_ns=gears_vr::monotonic_ns();
    if(!producer.publish(sample))return 31;
    Vec live_p{1,2,3};Rot live_r{4,5,6};
    invoke(reinterpret_cast<void*>(1),reinterpret_cast<void*>(2),&live_p,&live_r,reinterpret_cast<void*>(3),reinterpret_cast<void*>(4));
    if(live_p.x!=6 || live_r.yaw!=12 || live_status!=static_cast<unsigned>(gears_vr::PoseStatus::recentered)){
        printf("Live self-test initial status=%u age_ns=%llu x=%.9g yaw=%d\n",live_status,
            static_cast<unsigned long long>(gears_vr::monotonic_ns()-sample.received_ns),live_p.x,live_r.yaw);return 33;}
    sample.sequence++;sample.received_ns=gears_vr::monotonic_ns();sample.head.position.x=.2;
    sample.head.orientation={0,std::sin(.2),0,std::cos(.2)};
    if(!producer.publish(sample))return 34;
    Vec moved_p{1,2,3};Rot moved_r{4,5,6};
    invoke(reinterpret_cast<void*>(1),reinterpret_cast<void*>(2),&moved_p,&moved_r,reinterpret_cast<void*>(3),reinterpret_cast<void*>(4));
    const auto& live_record=ring[(write_index-1)%ring.size()];
    double length=std::sqrt(live_record.translation_delta.x*live_record.translation_delta.x+
        live_record.translation_delta.y*live_record.translation_delta.y+live_record.translation_delta.z*live_record.translation_delta.z);
    if(live_status!=static_cast<unsigned>(gears_vr::PoseStatus::applied) || length<1.99 || length>2.001 ||
       std::abs(live_record.rotation_delta.yaw)>911 || std::abs(live_record.rotation_delta.yaw)<909)return 35;
    sample.sequence++;sample.received_ns=gears_vr::monotonic_ns()-200000000;
    if(!producer.publish(sample))return 36;
    Vec stale_p{1,2,3};Rot stale_r{4,5,6};
    invoke(reinterpret_cast<void*>(1),reinterpret_cast<void*>(2),&stale_p,&stale_r,reinterpret_cast<void*>(3),reinterpret_cast<void*>(4));
    if(stale_p.x!=6 || stale_p.y!=2 || stale_r.yaw!=12 ||
       live_status!=static_cast<unsigned>(gears_vr::PoseStatus::stale))return 37;
    recording=false;live_enabled=false;live_bridge.close();
    if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK)return 38;
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureGPUCheck(void*) {
    gpu_diagnostics=true;
    auto initialized=MH_Initialize();
    if((initialized!=MH_OK&&!(prime_ready.load()&&initialized==MH_ERROR_ALREADY_INITIALIZED))||!present_hooks())return 50;
    unsigned char before[32],after[32];
    if(!safe_copy(before,execute_target,32))return 51;
    std::array<std::array<unsigned char,32>,viewport_hook_capacity> viewport_before{};
    if(viewport_diagnostics)for(unsigned i=0;i<viewport_targets.size();++i){
        if(!viewport_targets[i])continue;
        auto primed=std::find(prime_targets.begin(),prime_targets.end(),viewport_targets[i]);
        if(prime_ready.load()&&primed!=prime_targets.end())viewport_before[i]=prime_original_bytes[static_cast<size_t>(primed-prime_targets.begin())];
        else if(!safe_copy(viewport_before[i].data(),viewport_targets[i],32))return 58;
    }
    ID3D12Device* device{};ID3D12CommandQueue* queue{};ID3D12CommandAllocator* allocator{};
    ID3D12GraphicsCommandList* list{};ID3D12Fence* fence{};
    ID3D12Resource* target_texture{};ID3D12DescriptorHeap* target_heap{};
    DWORD result=0;HANDLE event{};D3D12_COMMAND_QUEUE_DESC desc{};
    if(FAILED(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))) ||
       FAILED(device->CreateCommandQueue(&desc,IID_PPV_ARGS(&queue))) ||
       FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))) ||
       FAILED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator,nullptr,IID_PPV_ARGS(&list))) ||
       FAILED(list->Close()) || FAILED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence))))result=52;
    if(!result) {
        {std::lock_guard<std::mutex> lock(prime_mutex);
            if(prime_ready.load()){prime_handoff=true;SetEvent(prime_event);}
            recording=true;prime_recording=false;if(MH_ApplyQueued()!=MH_OK)result=53;}
        if(!result&&viewport_diagnostics) {
            if(FAILED(list->Reset(allocator,nullptr)))result=59;
            else {
                D3D12_VIEWPORT viewport{960,0,960,1080,0,1};D3D12_RECT scissor{960,0,1920,1080};
                list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
                D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
                D3D12_RESOURCE_DESC texture{};texture.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;texture.Width=1920;texture.Height=1080;
                texture.DepthOrArraySize=1;texture.MipLevels=1;texture.Format=DXGI_FORMAT_R8G8B8A8_UNORM;texture.SampleDesc.Count=1;texture.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
                D3D12_DESCRIPTOR_HEAP_DESC handles{};handles.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;handles.NumDescriptors=3;
                if(FAILED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&texture,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&target_texture)))||
                   FAILED(device->CreateDescriptorHeap(&handles,IID_PPV_ARGS(&target_heap))))result=63;
                if(!result){auto first=target_heap->GetCPUDescriptorHandleForHeapStart();auto second=first,third=first;
                    UINT stride=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);second.ptr+=stride;third.ptr+=2*stride;
                    device->CreateRenderTargetView(target_texture,nullptr,first);
                    device->CopyDescriptorsSimple(1,second,first,D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
                    device->CopyDescriptors(1,&third,nullptr,1,&second,nullptr,D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
                    list->OMSetRenderTargets(1,&third,FALSE,nullptr);
                    auto mapped=viewport_stats.lookup(third.ptr);
                    if(mapped.resource!=reinterpret_cast<uintptr_t>(target_texture)||mapped.width!=1920||mapped.height!=1080)result=64;
                }
                if(FAILED(list->Close()))result=60;
                auto stats=viewport_stats.snapshot();
                if(stats.resets!=1||stats.viewports!=1||stats.scissors!=1||stats.overflow||stats.target_binds!=1||stats.descriptor_creates!=1||stats.descriptor_copies!=2||stats.copy_rejected)result=61;
            }
        }
        if(!result) {
            ID3D12CommandList* lists[]={list};queue->ExecuteCommandLists(1,lists);
            event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
            if(!event || FAILED(queue->Signal(fence,1)) || FAILED(fence->SetEventOnCompletion(1,event)) ||
               WaitForSingleObject(event,5000)!=WAIT_OBJECT_0)result=54;
            if(!result && (write_index!=1 || ring[0].kind!=5 || ring[0].object!=reinterpret_cast<uintptr_t>(queue) ||
               ring[0].gpu_device!=reinterpret_cast<uintptr_t>(device) || ring[0].queue_type!=D3D12_COMMAND_LIST_TYPE_DIRECT ||
               ring[0].command_count!=1 || !(ring[0].valid&128)))result=55;
        }
    }
    ngx_mv_descriptor_capture::close();
    if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK && !result)result=56;
    recording=false;
    ngx_mv_descriptor_capture::release_drained();
    if(!result && (!safe_copy(after,execute_target,32) || memcmp(before,after,32)))result=57;
    if(!result&&viewport_diagnostics)for(unsigned i=0;i<viewport_targets.size();++i)
        if(viewport_targets[i]&&(!safe_copy(after,viewport_targets[i],32)||memcmp(after,viewport_before[i].data(),32)))result=62;
    if(target_heap)target_heap->Release();if(target_texture)target_texture->Release();
    if(event)CloseHandle(event);if(fence)fence->Release();if(list)list->Release();
    if(allocator)allocator->Release();if(queue)queue->Release();if(device)device->Release();
    return result;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureViewportCheck(void*) {
    viewport_diagnostics=true;return GearsCaptureGPUCheck(nullptr);
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureColorCopySystemCheck(void*) {
    color_copy_diagnostics=true;viewport_diagnostics=true;return GearsCaptureGPUCheck(nullptr);
}
struct FrameIntervalFixture {
    ID3D12CommandQueue* queue{};ID3D12CommandList* list{};uintptr_t resource{};
    uint64_t ticket{};gears_frame_link::ListRef recorded{};
    bool context_ok{true},fill_capacity{},fill_bindings{},cross_thread{};
    HRESULT result{E_FAIL};HANDLE ready{},done{},thread{};
} frame_interval_fixture;
std::atomic<unsigned> fake_frame_execute_forwarded{};
void STDMETHODCALLTYPE fake_frame_execute(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*) {++fake_frame_execute_forwarded;}
unsigned fake_frame_reset_forwarded{};
HRESULT STDMETHODCALLTYPE fake_frame_reset(ID3D12GraphicsCommandList*,ID3D12CommandAllocator*,ID3D12PipelineState*) {++fake_frame_reset_forwarded;return S_OK;}
DWORD WINAPI fake_frame_external_execute(void*) {
    auto& fixture=frame_interval_fixture;SetEvent(fixture.ready);
    ID3D12CommandList* members[]{fixture.list};hook_execute(fixture.queue,1,members);
    SetEvent(fixture.done);return 0;
}
HRESULT STDMETHODCALLTYPE fake_frame_present(IDXGISwapChain*,UINT,UINT) {
    auto& fixture=frame_interval_fixture;if(!fixture.queue)return E_FAIL;
    fixture.context_ok=fixture.context_ok&&frame_inside_original_present&&frame_present_depth!=0;
    ID3D12CommandList* members[]{fixture.list};
    // Emulate DXGI's unobserved-list Execute through the actual Execute hook.
    hook_execute(fixture.queue,1,members);
    auto identity=canonical_identity(fixture.list);
    fixture.recorded=frame_link.reset_success(identity);
    frame_target_stats.reset(reinterpret_cast<uintptr_t>(fixture.list));
    frame_target_stats.create(32,{fixture.resource,640,480,28,0});
    frame_target_stats.bind(reinterpret_cast<uintptr_t>(fixture.list),1,32);
    frame_link.draw_final(fixture.recorded,fixture.ticket,0,{fixture.resource,640,480},{0,0,320,480});
    frame_link.draw_final(fixture.recorded,fixture.ticket,1,{fixture.resource,640,480},{320,0,320,480});
    hook_execute(fixture.queue,1,members);
    if(fixture.fill_capacity)for(uintptr_t i=1;i<=gears_frame_link::Tracker::list_capacity+1;++i)frame_link.reset_success(i);
    if(fixture.fill_bindings)for(uintptr_t i=1;i<=gears_viewport::Tracker::list_capacity+1;++i)frame_target_stats.reset(i);
    if(fixture.cross_thread){
        fixture.thread=CreateThread(nullptr,0,fake_frame_external_execute,nullptr,0,nullptr);
        fixture.context_ok=fixture.context_ok&&fixture.thread&&WaitForSingleObject(fixture.ready,5000)==WAIT_OBJECT_0&&
            WaitForSingleObject(fixture.done,0)==WAIT_TIMEOUT;
    }
    return fixture.result;
}
HRESULT STDMETHODCALLTYPE fake_frame_present1(IDXGISwapChain1* chain,UINT interval,UINT flags,const DXGI_PRESENT_PARAMETERS*) {
    auto result=hook_present(chain,interval,flags);
    if(frame_interval_fixture.queue)frame_interval_fixture.context_ok=frame_interval_fixture.context_ok&&
        frame_inside_original_present&&frame_link.current(canonical_identity(frame_interval_fixture.list)).generation==frame_interval_fixture.recorded.generation;
    return result;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureFrameLinkCheck(void* test_mode) {
    // Explicit ABI mode1 verifies simulated provenance through the same Draw,
    // ticket store, publisher and GPU receiver path; nullptr keeps real defaults.
    simulator_operator=test_mode==reinterpret_cast<void*>(1);
    // CPU association and factory identity checks, followed by a local GPU copy
    // through the production hook path. This test uses no game or XR session.
    auto left=classify_final_eye(0xd83b1c,1,0,0,320,480,640,480);
    auto right=classify_final_eye(0xd83b1c,1,320,0,320,480,640,480);
    if(!left.valid||left.eye||!right.valid||right.eye!=1||classify_final_eye(0xd83b1d,1,0,0,320,480,640,480).valid||
       classify_final_eye(0xd83b1c,0,0,0,320,480,640,480).valid||classify_final_eye(0xd83b1c,1,.5f,0,320,480,640,480).valid||
       !classify_final_eye(0xd808d8,1,0,0,320,480,640,480).valid||
       classify_final_eye(0xd808d8,1,0,0,640,480,640,480).valid||
       classify_final_eye(0xd808d8,0,0,0,320,480,640,480).valid)return 140;
    auto source_left=classify_final_eye(0x2b11a0,1,0,0,1920,2160,3840,2160,true);
    auto source_right=classify_final_eye(0x2b11a0,1,1920,0,1920,2160,3840,2160,true);
    if(!source_left.valid||source_left.eye||!source_right.valid||source_right.eye!=1||
       classify_final_eye(0x2b11a0,1,0,0,1920,2160,3840,2160).valid||
       classify_final_eye(0xd83cbf,1,0,0,1920,2160,3840,2160,true).valid||
       classify_final_eye(0x2b11a0,0,0,0,1920,2160,3840,2160,true).valid||
       classify_final_eye(0x2b11a0,1,0,0,3840,2160,3840,2160,true).valid)return 140;
    for(bool overwrite:{false,true}){
        auto tracker=std::make_unique<gears_frame_link::Tracker>();auto ref=tracker->reset_success(1);
        const gears_frame_link::Target source{10,3840,2160},output{11,3840,2161};
        if(!tracker->draw_final(ref,1,0,source,source_left.rect)||
           !tracker->draw_final(ref,1,1,source,source_right.rect)||!tracker->draw_transfer(ref,source,output))return 140;
        if(overwrite&&!tracker->draw_unknown(ref,source.resource))return 140;
        tracker->execute(2,&ref,1);
        if(tracker->present(output.resource,2).linked==overwrite)return 140;
    }
    gears_submitted::FrameTicket synthetic{},copied{};synthetic.enabled=true;synthetic.id=1;
    // This 64-slot fixture is large; the production lifetime already lives in
    // static storage. Keep it off the GPU hook test's nested thread stack.
    auto lifetime=std::make_unique<gears_submitted::Lifetime>();
    if(!lifetime->begin(1,{},synthetic)||!lifetime->frame(1,copied)||copied.id!=1||copied.sample.sequence)return 141;
    if(MH_Initialize()!=MH_OK||begin_target_prime())return 142;
    IDXGIFactory2* factory{};ID3D12Device* device{};ID3D12CommandQueue* queue{};
    IDXGISwapChain1* chain{};IDXGISwapChain1* composition{};IDXGISwapChain* legacy{};
    HWND window=CreateWindowExW(0,L"STATIC",L"Frame link ABI test",WS_POPUP,0,0,640,480,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    HWND legacy_window=CreateWindowExW(0,L"STATIC",L"Frame link legacy ABI test",WS_POPUP,0,0,640,480,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    DWORD result{};D3D12_COMMAND_QUEUE_DESC queue_desc{};
    if(!window||!legacy_window||FAILED(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory)))||FAILED(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)))||
       FAILED(device->CreateCommandQueue(&queue_desc,IID_PPV_ARGS(&queue))))result=143;
    DXGI_SWAP_CHAIN_DESC1 desc{};desc.Width=640;desc.Height=480;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count=1;desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.BufferCount=2;desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;
    if(!result&&FAILED(factory->CreateSwapChainForHwnd(queue,window,&desc,nullptr,nullptr,&chain)))result=144;
    if(!result){
        auto binding=swapchain_bindings.lookup(canonical_identity(chain));ID3D12Resource* buffer{};
        if(binding.queue!=canonical_identity(queue)||binding.method!=2||FAILED(chain->GetBuffer(0,IID_PPV_ARGS(&buffer))))result=145;
        if(!result){auto identity=canonical_identity(buffer);buffer->Release();
            if(!swapchain_bindings.known_buffer(identity))result=146;
            frame_link_diagnostics=true;recording=true;
            observe_frame_present(chain,0); // Baseline anchor consumes pretrial work.
            if(swapchain_bindings.snapshot().anchors!=1||frame_counts.linked.load())result=159;
            auto list=frame_link.reset_success(1);
            if(!frame_link.draw_final(list,1,0,{identity,640,480},left.rect)||!frame_link.draw_final(list,1,1,{identity,640,480},right.rect)||
               !frame_link.execute(binding.queue,&list,1))result=147;
            observe_frame_present(chain,0);
            if(frame_counts.linked.load()!=1||frame_counts.last_ticket.load()!=1)result=148;
            observe_frame_present(chain,0);if(frame_counts.linked.load()!=1)result=149;
            auto next=frame_link.reset_success(1);
            if(!frame_link.draw_final(next,2,0,{identity,640,480},left.rect)||!frame_link.draw_final(next,2,1,{identity,640,480},right.rect)||
               !frame_link.execute(binding.queue,&next,1))result=156;
            auto saved_present=original_present;auto saved_present1=original_present1;
            original_present=fake_frame_present;original_present1=fake_frame_present1;
            if(hook_present1(chain,0,0,nullptr)!=E_FAIL||frame_counts.linked.load()!=2||frame_counts.presents.load()!=4||
               frame_counts.present_failed.load()!=1||frame_present_depth)result=157;
            original_present=saved_present;original_present1=saved_present1;
            // A failed original Present has consumed the ticket and cannot replay.
            observe_frame_present(chain,0);if(frame_counts.linked.load()!=2)result=158;
            // Exercise the actual Present/Present1 and Execute hooks with fake
            // original calls. No GPU submission is made by these fixtures.
            ID3D12CommandAllocator* interval_allocator{};ID3D12GraphicsCommandList* interval_list{};
            if(FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&interval_allocator)))||
               FAILED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,interval_allocator,nullptr,IID_PPV_ARGS(&interval_list))))result=160;
            if(!result){
                interval_list->Close();auto saved_execute=original_execute;
                original_execute=fake_frame_execute;original_present=fake_frame_present;original_present1=fake_frame_present1;
                frame_interval_fixture.queue=queue;frame_interval_fixture.list=interval_list;frame_interval_fixture.resource=identity;
                uint64_t ticket=10;
                auto pair_for_test=[&](uint64_t id){auto ref=frame_link.reset_success(1);
                    return frame_link.draw_final(ref,id,0,{identity,640,480},left.rect)&&
                        frame_link.draw_final(ref,id,1,{identity,640,480},right.rect)&&frame_link.execute(binding.queue,&ref,1);};
                for(unsigned scenario=0;scenario<4&&!result;++scenario){
                    bool nested=scenario&1,test=scenario&2;auto linked_before=frame_counts.linked.load();
                    if(!pair_for_test(++ticket)){result=161;break;}
                    frame_interval_fixture.ticket=++ticket;frame_interval_fixture.result=nested?E_FAIL:S_OK;
                    auto flags=test?DXGI_PRESENT_TEST:0;
                    auto hr=nested?hook_present1(chain,0,flags,nullptr):hook_present(chain,0,flags);
                    auto list_id=canonical_identity(interval_list);
                    if(hr!=frame_interval_fixture.result||!frame_interval_fixture.context_ok||frame_present_depth||frame_inside_original_present||
                       frame_counts.linked.load()!=linked_before+(test?0:1)||frame_link.current(list_id).generation||
                       frame_target_stats.binding(reinterpret_cast<uintptr_t>(interval_list)).targets||frame_link.snapshot().active_lists)result=162;
                    // Neither an interval generation nor its ticket can escape.
                    if(frame_link.execute(binding.queue,&frame_interval_fixture.recorded,1))result=163;
                    observe_frame_present(chain,0);
                    auto fresh=frame_link.reset_success(1);
                    if(frame_link.draw_final(fresh,frame_interval_fixture.ticket,0,{identity,640,480},left.rect))result=164;
                    observe_frame_present(chain,0);
                    linked_before=frame_counts.linked.load();
                    if(!pair_for_test(++ticket))result=165;
                    frame_interval_fixture.ticket=++ticket;
                    hook_present(chain,0,0);
                    if(frame_counts.linked.load()!=linked_before+1)result=166;
                }
                // Unknown Execute outside original Present still rejects game
                // candidates; the boundary is not a global unknown-list filter.
                ID3D12CommandList* members[]{interval_list};auto linked_before=frame_counts.linked.load();
                hook_execute(queue,1,members);if(pair_for_test(++ticket))result=167;
                frame_interval_fixture.ticket=++ticket;hook_present(chain,0,0);
                if(frame_counts.linked.load()!=linked_before)result=168;
                // Cross-thread Execute waits until the outer hook retires its
                // interval, then enters the next window and must reject there.
                frame_interval_fixture.ready=CreateEventW(nullptr,TRUE,FALSE,nullptr);
                frame_interval_fixture.done=CreateEventW(nullptr,TRUE,FALSE,nullptr);
                frame_interval_fixture.cross_thread=true;frame_interval_fixture.ticket=++ticket;
                hook_present(chain,0,0);frame_interval_fixture.cross_thread=false;
                if(!frame_interval_fixture.thread||WaitForSingleObject(frame_interval_fixture.done,5000)!=WAIT_OBJECT_0||
                   WaitForSingleObject(frame_interval_fixture.thread,5000)!=WAIT_OBJECT_0||!frame_interval_fixture.context_ok)result=169;
                if(frame_interval_fixture.thread)CloseHandle(frame_interval_fixture.thread);
                if(frame_interval_fixture.ready)CloseHandle(frame_interval_fixture.ready);
                if(frame_interval_fixture.done)CloseHandle(frame_interval_fixture.done);
                if(pair_for_test(++ticket))result=170;
                frame_interval_fixture.ticket=++ticket;hook_present(chain,0,0);
                // List/event capacity faults survive interval retirement. OM
                // overflow is propagated before the next frame can be linked.
                for(unsigned mode=0;mode<2;++mode){
                    frame_interval_fixture.fill_capacity=mode==0;frame_interval_fixture.fill_bindings=mode==1;
                    frame_interval_fixture.ticket=++ticket;hook_present(chain,0,0);
                    if(!frame_link.snapshot().capacity_bad||pair_for_test(++ticket))result=171;
                    frame_interval_fixture.fill_capacity=frame_interval_fixture.fill_bindings=false;
                    frame_interval_fixture.ticket=++ticket;hook_present(chain,0,0);
                    if(!frame_link.snapshot().capacity_bad)result=172;
                    frame_link.invalidate_all();observe_frame_present(chain,0);
                }
                frame_interval_fixture={};original_execute=saved_execute;
                if(!result){
                    auto bypass_executes=frame_counts.executes.load();auto bypass_resets=frame_counts.resets.load();
                    auto forwarded_execute=fake_frame_execute_forwarded.load();auto forwarded_reset=fake_frame_reset_forwarded;
                    auto saved_reset=original_reset_list;auto saved_copy=original_color_copy;auto forwarded_copy=color_copy_forwarded;
                    original_execute=fake_frame_execute;original_reset_list=fake_frame_reset;original_color_copy=fake_color_copy;
                    {StereoPublishBypass bypass;
                        hook_execute(queue,1,members);hook_reset_list(interval_list,interval_allocator,nullptr);
                        hook_color_copy(interval_list,nullptr,0,0,0,nullptr,nullptr);}
                    original_execute=saved_execute;original_reset_list=saved_reset;original_color_copy=saved_copy;
                    if(stereo_publish_bypass||frame_counts.executes.load()!=bypass_executes||frame_counts.resets.load()!=bypass_resets||
                       fake_frame_execute_forwarded.load()!=forwarded_execute+1||fake_frame_reset_forwarded!=forwarded_reset+1||
                       color_copy_forwarded!=forwarded_copy+1)result=185;
                    // A mirror uses graphics operations as well as copying.
                    // They must forward exactly once without touching native
                    // UI contexts or dereferencing its deliberately fake inputs.
                    static std::array<unsigned,6> mirror_forwarded{};mirror_forwarded={};
                    const auto saved_viewports=original_viewports;const auto saved_scissors=original_scissors;
                    const auto saved_draw=original_draw;const auto saved_indexed=original_draw_indexed;
                    const auto saved_targets=original_targets;const auto saved_rtv=original_create_rtv;
                    original_viewports=[](ID3D12GraphicsCommandList*,UINT,const D3D12_VIEWPORT*){++mirror_forwarded[0];};
                    original_scissors=[](ID3D12GraphicsCommandList*,UINT,const D3D12_RECT*){++mirror_forwarded[1];};
                    original_draw=[](ID3D12GraphicsCommandList*,UINT,UINT,UINT,UINT){++mirror_forwarded[2];};
                    original_draw_indexed=[](ID3D12GraphicsCommandList*,UINT,UINT,UINT,INT,UINT){++mirror_forwarded[3];};
                    original_targets=[](ID3D12GraphicsCommandList*,UINT,const D3D12_CPU_DESCRIPTOR_HANDLE*,BOOL,const D3D12_CPU_DESCRIPTOR_HANDLE*){++mirror_forwarded[4];};
                    original_create_rtv=[](ID3D12Device*,ID3D12Resource*,const D3D12_RENDER_TARGET_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE){++mirror_forwarded[5];};
                    {StereoPublishBypass bypass;
                        hook_viewports(nullptr,1,reinterpret_cast<const D3D12_VIEWPORT*>(1));
                        hook_scissors(nullptr,1,reinterpret_cast<const D3D12_RECT*>(1));
                        hook_draw(nullptr,3,1,0,0);hook_draw_indexed(nullptr,3,1,0,0,0);
                        hook_targets(nullptr,1,reinterpret_cast<const D3D12_CPU_DESCRIPTOR_HANDLE*>(1),FALSE,nullptr);
                        hook_create_rtv(nullptr,reinterpret_cast<ID3D12Resource*>(1),nullptr,{1});}
                    original_viewports=saved_viewports;original_scissors=saved_scissors;original_draw=saved_draw;
                    original_draw_indexed=saved_indexed;original_targets=saved_targets;original_create_rtv=saved_rtv;
                    for(auto count:mirror_forwarded)if(count!=1)result=186;
                    // Native publish integration: the exact production Draw
                    // observer retains a frozen value, the real owner binds,
                    // worker prepare creates resources, and pre-Present copy
                    // publishes that same ticket to an actual GPU receiver.
                    stereo_gpu_publish=true;uint64_t image_id=++ticket;
                    gears_vr::StereoSample frozen{};frozen.session=777;frozen.sequence=1;frozen.received_ns=1000000000;
                    frozen.predicted_time=2000000000;frozen.head_flags=frozen.view_flags=15;frozen.focused=frozen.should_render=1;
                    if(simulator_operator){frozen.source=gears_vr::StereoSource::MetaOperatorSimulated;frozen.head_flags=3;}
                    frozen.head.orientation={};
                    for(unsigned eye=0;eye<2;++eye){frozen.eyes[eye].pose.orientation={};frozen.eyes[eye].pose.position.x=eye?.032:-.032;
                        frozen.eyes[eye].fov={-.8,.7,.75,-.65};}
                    gears_vr::StereoGate test_gate;
                    if(!test_gate.update(frozen,frozen.received_ns,simulator_operator))result=186;
                    if(simulator_operator){gears_vr::StereoGate strict_gate;
                        if(strict_gate.update(frozen,frozen.received_ns))result=187;}
                    consuming_frozen_frame.enabled=true;consuming_frozen_frame.id=image_id;consuming_frozen_frame.sample=frozen;
                    consuming_frozen_frame.world_generation=committed_world_generation.load();
                    consuming_frame_ticket=image_id;
#if defined(GEARS_VR_DOF_COMPAT) && GEARS_VR_DOF_COMPAT
                    {
                    struct FixtureMode {
                        bool saved=self_testing;
                        FixtureMode(){self_testing=true;}
                        ~FixtureMode(){self_testing=saved;}
                    } fixture_mode;
                    // This legacy fixture does not run original_consume. Give
                    // its synthetic owned ticket a completed NoSite receipt;
                    // the real Present publication gate still decides admission.
                    if(!self_testing)result=191;
                    else {
                        std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
                        const gears_vr_dof::Key key{consuming_frozen_frame.world_generation,image_id};
                        if(!key.world||key.world!=committed_world_generation.load())result=194;
                        else {
                            if(vr_dof_compat::publication(key.world,key.id))result=192; // Missing must reject.
                            if(!vr_dof_compat::store.begin(key)||vr_dof_compat::publication(key.world,key.id))result=193; // Incomplete must reject.
                            vr_dof_compat::store.finish(key);
                        }
                    }
                    } // Restore self_testing before the real Present gate runs.
#endif
#if defined(GEARS_DLSS_OUTPUT_CONTROL) && GEARS_DLSS_OUTPUT_CONTROL
                    {
                        // This dedicated GPU fixture does not invoke consume or
                        // Output. Establish an explicit completed NoSite record;
                        // production missing/incomplete records must still fail.
                        std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
                        const gears_vr_dof::Key key{consuming_frozen_frame.world_generation,image_id};
                        if(!key.world||dlss_output_control::publication(key.world,key.id))result=195;
                        else if(!dlss_output_control::store.begin(key)||dlss_output_control::publication(key.world,key.id))result=196;
                        else {
                            dlss_output_control::store.finish(key);
                            if(!dlss_output_control::publication(key.world,key.id))result=197;
                        }
                    }
#endif
                    auto raw_list=reinterpret_cast<uintptr_t>(interval_list);auto ref=frame_link.reset_success(canonical_identity(interval_list));
                    frame_target_stats.reset(raw_list);frame_target_stats.create(32,{identity,640,480,28,0});frame_target_stats.bind(raw_list,1,32);
                    frame_target_stats.set(raw_list,gears_viewport::other,false,{0,0,640,480});
                    frame_target_stats.set(raw_list,gears_viewport::other,true,{0,0,16384,16384});
                    // Emulate hook_quad's caller scope as well as its classified
                    // eye. The production target policy validates both.
                    const auto saved_test_candidate=final_eye_candidate;
                    final_eye_candidate={0xd83b1c,image_id,{0,0,320,480},640,480};
                    final_eye=left;observe_frame_draw(interval_list,base+0x1af92b,1,6,1);
                    final_eye_candidate.rect={320,0,320,480};
                    final_eye=right;observe_frame_draw(interval_list,base+0x1af92b,1,6,1);final_eye={};
                    final_eye_candidate=saved_test_candidate;
                    auto rectangles=frame_target_stats.draw_state(raw_list);
                    if(!verified_stereo_draw(rectangles,left,base+0x1af92b,1,6,1))result=173;
                    rectangles.viewport.width=320;
                    if(verified_stereo_draw(rectangles,left,base+0x1af92b,1,6,1))result=174;
                    rectangles=frame_target_stats.draw_state(raw_list);rectangles.scissor.width=320;
                    if(verified_stereo_draw(rectangles,right,base+0x1af92b,1,6,1)||
                       verified_stereo_draw(frame_target_stats.draw_state(raw_list),left,base+0x1af92b,0,6,1))result=175;
                    gears_vr::StereoSample copied_sample{};
                    if(!stereo_ticket_store.lookup(image_id,copied_sample,simulator_operator)||!gears_vr::stereo_same(frozen,copied_sample)||
                       !frame_link.execute(binding.queue,&ref,1))result=176;
                    if(simulator_operator&&stereo_ticket_store.lookup(image_id,copied_sample))result=188;
                    bind_stereo_publisher(canonical_identity(chain),binding.queue);stereo_publish_worker_tick();
                    if(owned_xr_probe){
                        // Owned mode must not let a legitimate legacy Present
                        // claim the fixed publisher binding or publish a frame.
                        hook_present(chain,0,0);
                        if(stereo_publisher.state.load()!=0||stereo_publish_counts.prepare_ok.load()||stereo_publisher.frames.load()||
                           stereo_publish_counts.last_ticket.load()||stereo_ticket_store.size()||stereo_window_retained||
                           stereo_ticket_store.retired_id()!=image_id)result=190;
                    }else{
                    if(stereo_publisher.state.load()!=2||stereo_publish_counts.prepare_ok.load()!=1)result=177;
                    try{
                        gears_vr::GpuReceiver receiver;receiver.open(device,GetCurrentProcessId(),simulator_operator);
                        hook_present(chain,0,0);
                        if(stereo_publisher.frames.load()!=1||stereo_publish_counts.last_ticket.load()!=image_id||
                           stereo_ticket_store.size()||stereo_window_retained||stereo_ticket_store.retired_id()!=image_id)result=178;
                        if(!receiver.acquire(queue)||receiver.ticket().id!=image_id||!gears_vr::stereo_same(receiver.ticket().sample,frozen))result=179;
                        else receiver.acknowledge(queue);
                        if(!close_stereo_publisher())result=180;
                        receiver.close();
                    }catch(...){result=181;close_stereo_publisher();}
                    }
                    // A retained future id survives the consumed window's
                    // boundary; a global newly-created ticket counter is not
                    // a valid retirement frontier.
                    consuming_frozen_frame.id=++ticket;consuming_frame_ticket=ticket;
                    if(!retain_stereo_draw())result=182;
                    auto boundary=stereo_window_retained;stereo_window_retained=0;
                    if(!gears_vr::StereoTicketStore::accepted(stereo_ticket_store.retain(++ticket,frozen,simulator_operator)))result=183;
                    stereo_window_retained=boundary;retire_stereo_window();
                    if(!stereo_ticket_store.lookup(ticket,copied_sample,simulator_operator))result=184;
                    stereo_window_retained=ticket;retire_stereo_window();
                    consuming_frozen_frame={};consuming_frame_ticket=0;stereo_gpu_publish=false;
                }
                original_present=saved_present;original_present1=saved_present1;
            }
            if(interval_list)interval_list->Release();if(interval_allocator)interval_allocator->Release();
            recording=false;
        }
    }
    DXGI_SWAP_CHAIN_DESC old_desc{};old_desc.BufferDesc.Width=640;old_desc.BufferDesc.Height=480;
    old_desc.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;old_desc.SampleDesc.Count=1;old_desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;
    old_desc.BufferCount=2;old_desc.OutputWindow=legacy_window;old_desc.Windowed=TRUE;old_desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;
    if(!result&&FAILED(factory->CreateSwapChain(queue,&old_desc,&legacy)))result=150;
    if(!result&&swapchain_bindings.lookup(canonical_identity(legacy)).method!=1)result=151;
    desc.Width=16;desc.Height=16;desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;desc.AlphaMode=DXGI_ALPHA_MODE_PREMULTIPLIED;
    if(!result&&FAILED(factory->CreateSwapChainForComposition(queue,&desc,nullptr,&composition)))result=152;
    if(!result&&(swapchain_bindings.lookup(canonical_identity(composition)).chain||!swapchain_bindings.snapshot().excluded))result=153;
    SetEvent(prime_event);ULONGLONG deadline=GetTickCount64()+5000;
    while(!prime_done.load()&&GetTickCount64()<deadline)Sleep(1);
    if(!prime_done.load()||active.load()||prime_recording.load())result=154;
    std::array<unsigned char,32> bytes{};
    for(unsigned i=0;i<prime_targets.size();++i)
        if(!safe_copy(bytes.data(),prime_targets[i],bytes.size())||bytes!=prime_original_bytes[i])result=155;
    if(composition)composition->Release();if(legacy)legacy->Release();if(chain)chain->Release();
    if(queue)queue->Release();if(device)device->Release();if(factory)factory->Release();if(window)DestroyWindow(window);if(legacy_window)DestroyWindow(legacy_window);
    return result;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCapturePrimeCheck(void*) {
    if(MH_Initialize()!=MH_OK||begin_target_prime())return 65;
    ID3D12Device* device{};ID3D12Resource* texture{};ID3D12DescriptorHeap* descriptors{};DWORD result=0;
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC description{};description.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;description.Width=32;description.Height=16;
    description.DepthOrArraySize=1;description.MipLevels=2;description.Format=DXGI_FORMAT_R8G8B8A8_UNORM;description.SampleDesc.Count=1;description.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    D3D12_DESCRIPTOR_HEAP_DESC handles{};handles.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;handles.NumDescriptors=1;
    if(FAILED(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)))||
       FAILED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&description,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&texture)))||
       FAILED(device->CreateDescriptorHeap(&handles,IID_PPV_ARGS(&descriptors))))result=66;
    if(!result){auto handle=descriptors->GetCPUDescriptorHandleForHeapStart();D3D12_RENDER_TARGET_VIEW_DESC view{};
        view.Format=description.Format;view.ViewDimension=D3D12_RTV_DIMENSION_TEXTURE2D;view.Texture2D.MipSlice=1;
        device->CreateRenderTargetView(texture,&view,handle);auto mapped=viewport_stats.lookup(handle.ptr);
        if(mapped.resource!=reinterpret_cast<uintptr_t>(texture)||mapped.width!=16||mapped.height!=8||mapped.mip!=1)result=67;
    }
    SetEvent(prime_event);ULONGLONG deadline=GetTickCount64()+5000;
    while(!prime_done.load()&&GetTickCount64()<deadline)Sleep(1);
    if(!prime_done.load()||prime_recording.load()||active.load())result=68;
    std::array<unsigned char,32> bytes{};
    for(unsigned i=0;i<prime_targets.size();++i)
        if(!safe_copy(bytes.data(),prime_targets[i],bytes.size())||memcmp(bytes.data(),prime_original_bytes[i].data(),bytes.size()))result=69;
    if(descriptors)descriptors->Release();if(texture)texture->Release();if(device)device->Release();return result;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCapturePrimeHandoffCheck(void*) {
    if(MH_Initialize()!=MH_OK||begin_target_prime())return 65;
    viewport_diagnostics=true;DWORD result=GearsCaptureGPUCheck(nullptr);
    if(!prime_handoff.load())SetEvent(prime_event);
    ULONGLONG deadline=GetTickCount64()+5000;while(!prime_done.load()&&GetTickCount64()<deadline)Sleep(1);
    if(!prime_done.load()||prime_recording.load())result=68;
    return result;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureNgxMvDescriptorsCheck(void*) {
#if defined(GEARS_NGX_MV_DESCRIPTOR_OBSERVER) && GEARS_NGX_MV_DESCRIPTOR_OBSERVER
    FILETIME created{},exited{},kernel{},user{};
    if(!GetProcessTimes(GetCurrentProcess(),&created,&exited,&kernel,&user))return 300;
    gears_prime::OwnerArgument owner{24,1,GetCurrentProcessId(),0,gears_session::creation_time(created)};
    if(MH_Initialize()!=MH_OK||begin_target_prime(&owner))return 301;
    ngx_mv_descriptor_capture::Bank::Snapshot retained;
    DWORD result=ngx_mv_descriptor_capture::native_check(retained);
    if(!prime_event||!SetEvent(prime_event)){if(!result)result=323;}
    const auto deadline=GetTickCount64()+5000;
    while(!prime_done.load()&&GetTickCount64()<deadline)Sleep(1);
    if(!result&&(!prime_done.load()||prime_recording.load()||active.load()))result=324;
    std::array<unsigned char,32> bytes{};
    if(!result)for(unsigned i=0;i<prime_targets.size();++i)
        if(!safe_copy(bytes.data(),prime_targets[i],bytes.size())||bytes!=prime_original_bytes[i]){result=325;break;}
    if(!result&&!graphics_lock_prime_restored())result=326;
    if(!result)result=ngx_mv_descriptor_capture::native_closed_check(retained);
    if(!result)result=ngx_root_signature_capture::native_closed_check();
    retained={}; // Last fixture snapshot is released after admission/drain checks.
    printf("NGX MV descriptor callbacks self-test result=%lu CPU_only=1 selected_pair_proof=0 producer_authorized=0\n",result);
    return result;
#else
    return ERROR_NOT_SUPPORTED;
#endif
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCapturePrimeOwnedCheck(void* argument) {
    DWORD mode{};if(!safe_copy(&mode,argument,sizeof(mode))||mode>2)return 220;
    FILETIME created{},exited{},kernel{},user{};
    if(!GetProcessTimes(GetCurrentProcess(),&created,&exited,&kernel,&user))return 221;
    gears_prime::OwnerArgument identity{24,1,GetCurrentProcessId(),0,gears_session::creation_time(created)};
    auto wrong=identity;wrong.creation_time++;
    if(begin_target_prime(&wrong)!=ERROR_INVALID_PARAMETER||prime_started.load()||prime_owner.handle||prime_recording.load())return 222;
    wrong=identity;wrong.version=2;
    if(begin_target_prime(&wrong)!=ERROR_INVALID_PARAMETER||prime_started.load())return 223;
    HANDLE child_event{};PROCESS_INFORMATION child{};
    if(mode==1){
        wchar_t event_name[128]{};swprintf_s(event_name,L"Local\\GearsVR.PrimeOwnerSelfTest.%lu",GetCurrentProcessId());
        child_event=CreateEventW(nullptr,TRUE,FALSE,event_name);if(!child_event)return 224;
        wchar_t executable[MAX_PATH]{};GetModuleFileNameW(nullptr,executable,MAX_PATH);
        wchar_t command[MAX_PATH+200]{};swprintf_s(command,L"\"%s\" --prime-owner-child \"%s\"",executable,event_name);
        STARTUPINFOW startup{};startup.cb=sizeof(startup);
        if(!CreateProcessW(nullptr,command,nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&child)){
            CloseHandle(child_event);return 225;
        }
        CloseHandle(child.hThread);
        if(!GetProcessTimes(child.hProcess,&created,&exited,&kernel,&user)){
            SetEvent(child_event);CloseHandle(child_event);CloseHandle(child.hProcess);return 226;
        }
        identity.pid=child.dwProcessId;identity.creation_time=gears_session::creation_time(created);
    }
    DWORD result=0;
    if(MH_Initialize()!=MH_OK||begin_target_prime(&identity))result=227;
    if(!result){
        std::lock_guard<std::mutex> lock(prime_mutex);
        if(!prime_owner_allows_handoff(true,identity.pid,identity.creation_time)||
           prime_owner_allows_handoff(false,identity.pid,identity.creation_time)||
           prime_owner_allows_handoff(true,identity.pid,identity.creation_time+1))result=228;
    }
    if(!result&&begin_target_prime(&identity)!=ERROR_ALREADY_EXISTS)result=229;
    if(!result&&mode==2){viewport_diagnostics=true;result=GearsCaptureGPUCheck(nullptr);
        if(!prime_handoff.load())result=230;
    }
    if(mode==1)SetEvent(child_event);
    else if(prime_event&&!prime_handoff.load()){
        std::lock_guard<std::mutex> lock(prime_mutex);SetEvent(prime_event);
        if(prime_owner_allows_handoff(true,identity.pid,identity.creation_time))result=235;
    }
    ULONGLONG deadline=GetTickCount64()+5000;
    while(!prime_done.load()&&GetTickCount64()<deadline)Sleep(1);
    if(!prime_done.load()||prime_recording.load()||active.load())result=231;
    std::array<unsigned char,32> bytes{};
    for(unsigned i=0;i<prime_targets.size();++i)
        if(!safe_copy(bytes.data(),prime_targets[i],bytes.size())||bytes!=prime_original_bytes[i])result=232;
    if(mode==1){if(WaitForSingleObject(child.hProcess,5000)!=WAIT_OBJECT_0||
        prime_owner.state()!=gears_session::OwnerState::exited)result=233;
        CloseHandle(child_event);CloseHandle(child.hProcess);}
    if(mode==2&&prime_event){SetEvent(prime_event);if(active.load())result=234;}
    return result;
}
#include "owned_output_tracking_check.inl"
#include "owned_scene_gpu_check.inl"
#include "resource_resize_probe_check.inl"
#include "ui_execute_bypass_check.inl"
#include "ui_material_bypass_check.inl"
#include "ui_scene_root_bypass_check.inl"
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,void*) {
    // Keep CRT thread notifications enabled for the statically linked runtime.
    if(reason==DLL_PROCESS_ATTACH) own_module=module;
    return TRUE;
}
