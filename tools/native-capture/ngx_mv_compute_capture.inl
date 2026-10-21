// Targeted CPU command-list observations only. Typed arguments are retained;
// NGX integers are never converted to COM. No added commands or GPU proof.
#include "ngx_mv_constants_capture.inl"
namespace ngx_mv_compute_capture {
#if GEARS_NGX_MV_TARGET_CAPTURE
using gears_ngx_mv_compute::Recording;
using gears_ngx_mv_compute::DescriptorLookup;
using HeapLease=gears_ngx_mv::HeapLease;
using HeapsFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,ID3D12DescriptorHeap* const*);
using RootFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12RootSignature*);
using TableFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,D3D12_GPU_DESCRIPTOR_HANDLE);
using DispatchFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,UINT,UINT);
using ClearFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12PipelineState*);
using BundleFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12GraphicsCommandList*);
using SetFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12PipelineState*);
using CopyResourceFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12Resource*,ID3D12Resource*);
HeapsFn original_heaps{};RootFn original_root{};TableFn original_table{};DispatchFn original_dispatch{};
ClearFn original_clear{};BundleFn original_bundle{};
SetFn original_set{};
CopyResourceFn original_copy_resource{};
struct List {
    uintptr_t raw{},canonical{};uint64_t generation{};
    uint64_t reset_prior_generation{};bool reset_prior_known{};
    unsigned callback_busy{},reset_busy{};bool revoked{},history_pinned{};
    gears_vr::ComPtr<ID3D12GraphicsCommandList> held;
    gears_vr::ComPtr<IUnknown> identity;
    HeapLease heap;
    gears_vr::ComPtr<ID3D12RootSignature> root;
    gears_vr::ComPtr<IUnknown> root_identity;
};
// Declared before the callback's frame lock. Detached owners and validation
// temporaries therefore release only after that lock has been dropped.
struct Retirement {
    List retired;
    gears_vr::ComPtr<IUnknown> identity,device_identity;
    gears_vr::ComPtr<ID3D12Device> device;
};
struct ListUse {
    List* list{};
    explicit ListUse(List* value=nullptr):list(value){if(list)++list->callback_busy;}
    void attach(List* value){list=value;if(list)++list->callback_busy;}
    ~ListUse(){if(list)--list->callback_busy;}
};
std::array<List,16> lists;unsigned list_count{};
gears_ngx_mv_compute::Bindings bindings;
struct Row {
    uint64_t world{},ticket{},generation{};uintptr_t renderer{},raw{},canonical{},root{},heap{},caller{};
    uint64_t root3{};std::array<UINT,3> groups{};DescriptorLookup before{},after{};
    bool returned{},stable{},caller_candidate{};
};
std::array<Row,32> rows;unsigned row_count{};
bool stopped{},failed{},leases_released{};unsigned failure{};
uint64_t dispatches{},unbound{},unrelated{},invalidations{},waiting_bindings{},capture_bindings{};
uint64_t mutation{};
uint64_t foreign_device_skips{},reuse_attempts{},reused{};
struct Occupancy {unsigned occupied{},eligible{},partial{},pinned{},busy{},unrevoked{},unknown_blocked{},stale_blocked{};} capacity_occupancy;
using ExactTarget=ngx_mv_descriptor_capture::ExactTarget;
using PsoWitnessKey=gears_ngx_mv_compute::PsoWitnessKey;
struct PsoWitness:gears_ngx_mv_compute::PsoWitnessValidity {
    PsoWitnessKey key;
    gears_vr::ComPtr<ID3D12GraphicsCommandList> list;
    gears_vr::ComPtr<IUnknown> list_identity;
    gears_vr::ComPtr<ID3D12PipelineState> pso;
    gears_vr::ComPtr<IUnknown> pso_identity;
};
PsoWitness pso_pending,pso_promoted;
struct RootWitness:gears_ngx_mv_compute::PsoWitnessValidity {gears_ngx_mv_compute::RootWitnessKey key;};
RootWitness root_pending,root_promoted;
uint64_t root_serial{},root_sets{},root_candidates{},root_promotions{},root_unknown{},root_rejected{};
thread_local unsigned root_set_depth{};
void root_invalidate(){++root_serial;root_pending.revoke();root_promoted.revoke();}
uint64_t pso_serial{},pso_sets{},pso_candidates{},pso_discards{},pso_promotions{},pso_qi_failures{};unsigned pso_last_discard{};
thread_local unsigned pso_set_depth{};
void pso_invalidate(unsigned reason){
    ++pso_serial;if(pso_pending.valid)++pso_discards;pso_pending.revoke();pso_promoted.revoke();pso_last_discard=reason;
    // Owners and historical stable Dispatch evidence survive revocation.
}
struct ExactAssociation {bool active{};uintptr_t canonical{},renderer{};uint64_t generation{},ticket{},world{},serial{};ExactTarget descriptor;};
ExactAssociation exact_association;
gears_vr::ComPtr<ID3D12GraphicsCommandList> exact_list_owner;
gears_vr::ComPtr<IUnknown> exact_identity_owner;
// A later native binding ends the current observation, while its immutable
// Dispatch receipt and typed owners survive until the whole selected window
// drains. This permits both eyes without extending the window or recycling a
// COM address into a new recording generation.
struct ExactOwners {
    gears_vr::ComPtr<ID3D12GraphicsCommandList> list;
    gears_vr::ComPtr<IUnknown> identity;
    PsoWitness pso;RootWitness root;ExactAssociation association;unsigned reason{};
};
std::array<ExactOwners,8> exact_history;unsigned exact_history_count{},exact_inflight{};
bool exact_stopped{},exact_arming{};unsigned exact_reason{};uint64_t exact_serial{},exact_tables{},exact_misses{},exact_dispatches{},exact_overflow{};
bool exact_pending(){return !exact_stopped&&ngx_mv_target_capture::pending();}
void exact_stop(unsigned reason){if(!exact_stopped){exact_stopped=true;exact_reason=reason;++exact_serial;pso_invalidate(3);root_invalidate();ngx_mv_constants_capture::invalidate();}exact_association.active=false;exact_arming=false;}
void exact_mutation(unsigned reason){
    if(!exact_pending())return;++exact_serial;
    if(exact_arming||exact_inflight){exact_stop(reason);return;}
    if(!exact_association.active)return;
    if(exact_history_count==exact_history.size()){++exact_overflow;exact_stop(6);return;}
    auto& saved=exact_history[exact_history_count++];
    saved.list.Swap(exact_list_owner);saved.identity.Swap(exact_identity_owner);
    saved.pso=std::move(pso_promoted);pso_promoted={};
    saved.root=root_promoted;root_promoted={};saved.association=exact_association;saved.reason=reason;
    exact_association={};
}
struct ExactDispatch {ExactAssociation association;ExactTarget after;uintptr_t caller{},pso{};uint64_t pso_serial{};std::array<UINT,3> groups{};bool returned{},stable{},pso_stable{},root_stable{},constants_stable{};gears_ngx_mv_compute::RootWitnessKey root;ngx_mv_constants_capture::Binding constants;bool executed{};uintptr_t queue{};uint64_t execute_order{};};
std::array<ExactDispatch,8> exact_rows;unsigned exact_count{};
enum class CallerOperation:unsigned {heaps,root,table,dispatch};
std::array<uint64_t,4> unknown_skipped{};
struct CallerRow {
    CallerOperation operation{};unsigned phase{};uintptr_t caller{},rva{};bool in_game_image{},historical_candidate{};
    uint64_t calls{},bank_failed_calls{},first_ticket{},last_ticket{};
};
std::array<CallerRow,32> caller_rows;unsigned caller_row_count{};
uint64_t caller_calls{},caller_overflow_calls{};unsigned caller_image_size{};bool caller_image_checked{};
thread_local unsigned depth{};
struct EvidenceContext {const char* operation="none";uintptr_t raw{};bool owned_known{},owned{};};
thread_local EvidenceContext evidence_context;
struct EvidenceScope {
    EvidenceContext previous;
    EvidenceScope(char const* operation,ID3D12GraphicsCommandList* list):previous(evidence_context){
        evidence_context={operation,reinterpret_cast<uintptr_t>(list),false,false};
    }
    ~EvidenceScope(){evidence_context=previous;}
};
struct FirstFailure {
    bool present{},owned_known{},owned{},bypass{},prior_stored_known{};
    char const* site="none";char const* operation="none";
    uintptr_t raw{},canonical{},current_identity{},renderer{};
    uint64_t stored_generation{},current_generation{},prior_stored_generation{},ticket{},world{},frozen_ticket{},frozen_world{},mutation_at_failure{};
    unsigned reason{},core_loss{},window_phase{},depth_at_failure{},lists_at_failure{},rows_at_failure{};
    HRESULT result=E_PENDING;
    Occupancy occupancy{};
} first_failure;
enum Failure:unsigned {invalid_list=1,list_capacity,invalid_heap,invalid_root,nested,descriptor_capacity,row_capacity,changed,unwinding,invalid_generation};
void lose(unsigned reason,char const* site="unspecified",List const* held=nullptr,uintptr_t raw=0,uintptr_t canonical=0,
          HRESULT result=E_PENDING,uint64_t prior_stored=0,bool prior_known=false){
    if(!failed){
        // Scalar evidence only. current() reads the tracker, never the native
        // command-list object. This does not alter rejection or retry policy.
        auto& f=first_failure;f.present=true;f.reason=reason;f.site=site;f.operation=evidence_context.operation;
        f.raw=raw?raw:held?held->raw:evidence_context.raw;f.canonical=canonical?canonical:held?held->canonical:0;
        f.stored_generation=held?held->generation:0;
        if(f.canonical){const auto live=frame_link.current(f.canonical);f.current_identity=live.identity;f.current_generation=live.generation;}
        f.prior_stored_generation=prior_known?prior_stored:held?held->reset_prior_generation:0;
        f.prior_stored_known=prior_known||(held&&held->reset_prior_known);f.result=result;
        f.owned_known=evidence_context.owned_known;f.owned=evidence_context.owned;f.bypass=stereo_publish_bypass!=0;
        f.ticket=consuming_frame_ticket;f.world=committed_world_generation.load();
        f.frozen_ticket=consuming_frozen_frame.id;f.frozen_world=consuming_frozen_frame.world_generation;
        f.renderer=reinterpret_cast<uintptr_t>(consuming_renderer);f.window_phase=unsigned(ngx_mv_target_capture::window.snapshot().phase);
        f.core_loss=unsigned(bindings.loss());f.depth_at_failure=depth;f.lists_at_failure=list_count;f.rows_at_failure=row_count;f.mutation_at_failure=mutation;
        f.occupancy=capacity_occupancy;
        failed=true;failure=reason;bindings.uncertain();
    }stopped=true;
}
bool pending(){return !stopped&&!failed&&ngx_mv_target_capture::pending();}
bool target_observing(bool owned){
    ngx_mv_target_capture::poll();
    if(!ngx_mv_target_capture::pending()||!owned||stereo_publish_bypass||!recording.load())return false;
    auto const& s=ngx_mv_target_capture::window.snapshot();
    if(!consuming_frozen_frame.enabled||!consuming_renderer||consuming_frozen_frame.world_generation!=s.selection.world||
       committed_world_generation.load()!=s.selection.world||consuming_frame_ticket!=consuming_frozen_frame.id)return false;
    if(s.phase==gears_ngx_mv_target::Phase::capturing)
        return consuming_frame_ticket==s.consume.ticket&&reinterpret_cast<uintptr_t>(consuming_renderer)==s.consume.renderer;
    return consuming_frame_ticket>=s.selection.ticket;
}
bool observing(bool owned){
    evidence_context.owned_known=true;evidence_context.owned=owned;
    return target_observing(owned)&&!stopped&&!failed;
}
struct RasterBinding {uintptr_t raw{},canonical{};uint64_t generation{};std::array<gears_viewport::Target,2> targets{};bool depth_writable{};};
struct RasterRow {uintptr_t resource{},list{},caller{};uint64_t generation{},count{};unsigned input{},view_mask{},first_eval{},last_eval{};gears_ngx_stereo::Rect clip{};bool descriptor_writable{},executed{};uintptr_t queue{};uint64_t execute_order{};};
struct DepthMismatch {uintptr_t handle{},resource{},list{};uint64_t width{};unsigned height{},format{};};
std::array<DepthMismatch,8> depth_mismatches;unsigned depth_mismatch_count{};uint64_t depth_mismatch_overflow{};
std::array<RasterBinding,16> raster_bindings;
std::array<RasterRow,64> raster_rows;unsigned raster_count{},raster_seed_attempts{},raster_seed_failure{};
std::array<gears_ngx_stereo::Resource,4> raster_inputs;
std::array<gears_ngx_stereo::Rect,2> raster_views;
bool raster_seeded{};uint64_t raster_binding_overflow{},raster_overflow{},raster_unknown_rectangles{};
struct DepthCopy {uintptr_t source{},destination{},list{},caller{};uint64_t generation{};unsigned kind{},source_subresource{},destination_subresource{},view_mask{};bool returned{},executed{};uint64_t calls{};unsigned eval_count{};uintptr_t queue{};uint64_t execute_order{};};
std::array<DepthCopy,8> depth_copies;std::array<gears_ngx_stereo::TypedResourceLease,8> depth_copy_sources;unsigned depth_copy_count{};uint64_t depth_copy_overflow{};
struct SdkRef {uintptr_t raw{},canonical{},queue{};uint64_t generation{},order{};bool executed{};gears_vr::ComPtr<ID3D12GraphicsCommandList> held;};
bool raster_active();
std::array<SdkRef,4> sdk_barriers;std::array<SdkRef,2> sdk_evals,sdk_copies;
uint64_t selected_execute_order{},raster_retired_bindings{},sdk_copy_before_evals{},sdk_copy_repeats{};
struct GpuProof {bool attempted{},signalled{},complete{},removed{},resident{};unsigned failure{};HRESULT result{E_PENDING};uint64_t value{},order{};uintptr_t queue_id{};gears_vr::ComPtr<ID3D12CommandQueue> queue;gears_vr::ComPtr<ID3D12Fence> fence;} gpu_proof;
void gpu_poll(){
#if defined(GEARS_NGX_MV_GPU_COMPLETION) && GEARS_NGX_MV_GPU_COMPLETION
    gears_vr::ComPtr<ID3D12Fence> fence;
    {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!gpu_proof.signalled||gpu_proof.complete||gpu_proof.removed)return;fence=gpu_proof.fence;}
    const auto completed=fence->GetCompletedValue();
    {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);gpu_proof.value=completed;gpu_proof.removed=completed==UINT64_MAX;gpu_proof.complete=completed!=UINT64_MAX&&completed>=1;}
#endif
}
void sdk_barrier(ID3D12GraphicsCommandList* list,uintptr_t caller){
#if defined(GEARS_NGX_MV_GPU_COMPLETION) && GEARS_NGX_MV_GPU_COMPLETION
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(!raster_active()||caller!=base+0x9a35c0||!list)return;
    const auto canonical=canonical_identity(list);const auto current=frame_link.current(canonical);
    if(!canonical||current.identity!=canonical||!current.generation||list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT){gpu_proof.failure=1;return;}
    for(auto& row:sdk_barriers)if(row.raw==reinterpret_cast<uintptr_t>(list)&&row.generation==current.generation)return;
    for(auto& row:sdk_barriers)if(!row.raw){row.raw=reinterpret_cast<uintptr_t>(list);row.canonical=canonical;row.generation=current.generation;row.held=list;return;}
    gpu_proof.failure=2;
#else
    (void)list;(void)caller;
#endif
}
void sdk_eval(uintptr_t raw,unsigned ordinal){
#if defined(GEARS_NGX_MV_GPU_COMPLETION) && GEARS_NGX_MV_GPU_COMPLETION
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!raster_active()||ordinal>=2)return;
    for(auto const& row:sdk_barriers)if(row.raw==raw&&frame_link.current(row.canonical).generation==row.generation){sdk_evals[ordinal]=row;return;}
    gpu_proof.failure=3;
#else
    (void)raw;(void)ordinal;
#endif
}
void sdk_copy(ID3D12GraphicsCommandList* list,uintptr_t caller){
#if defined(GEARS_NGX_MV_GPU_COMPLETION) && GEARS_NGX_MV_GPU_COMPLETION
    if(caller!=0x1c6093)return;const auto ordinal=ngx_mv_target_capture::window.snapshot().eval_count;
    if(!ordinal){++sdk_copy_before_evals;return;}
    if(ordinal>2){gpu_proof.failure=4;return;}
    const auto canonical=canonical_identity(list);const auto current=frame_link.current(canonical);
    if(!canonical||current.identity!=canonical||!current.generation){gpu_proof.failure=5;return;}
    if(sdk_copies[ordinal-1].raw){
        auto const& old=sdk_copies[ordinal-1];
        if(old.raw==reinterpret_cast<uintptr_t>(list)&&old.canonical==canonical&&old.generation==current.generation){++sdk_copy_repeats;return;}
        gpu_proof.failure=4;return;
    }
    auto& row=sdk_copies[ordinal-1];row.raw=reinterpret_cast<uintptr_t>(list);row.canonical=canonical;row.generation=current.generation;row.held=list;
#else
    (void)list;(void)caller;
#endif
}
void executed(ID3D12CommandQueue* queue,UINT count,ExecuteList const* members,bool readable){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
    ngx_trial_producer::executed(count,members,readable);
#endif
    const auto phase=ngx_mv_target_capture::window.snapshot().phase;
    if((phase!=gears_ngx_mv_target::Phase::capturing&&phase!=gears_ngx_mv_target::Phase::frozen)||!readable||count>64)return;
    const auto order=++selected_execute_order;const auto queue_id=canonical_identity(queue);
    auto match=[&](uintptr_t canonical,uint64_t generation){unsigned matches{};for(unsigned i=0;i<count;++i)if(members[i].canonical==canonical&&members[i].generation==generation)++matches;return matches==1;};
    for(auto& binding:raster_bindings)if(binding.canonical&&match(binding.canonical,binding.generation)){binding={};++raster_retired_bindings;}
    for(unsigned i=0;i<raster_count;++i){auto& row=raster_rows[i];if(!row.executed&&match(row.list,row.generation)){row.executed=true;row.queue=queue_id;row.execute_order=order;}}
    for(unsigned i=0;i<depth_copy_count;++i){auto& row=depth_copies[i];if(!row.executed&&match(row.list,row.generation)){row.executed=true;row.queue=queue_id;row.execute_order=order;}}
    for(unsigned i=0;i<exact_count;++i){auto& row=exact_rows[i];if(!row.executed&&match(row.association.canonical,row.association.generation)){row.executed=true;row.queue=queue_id;row.execute_order=order;}}
#if defined(GEARS_NGX_MV_GPU_COMPLETION) && GEARS_NGX_MV_GPU_COMPLETION
    for(auto* rows:{&sdk_evals,&sdk_copies})for(auto& row:*rows)if(row.raw&&!row.executed&&match(row.canonical,row.generation)){row.executed=true;row.queue=queue_id;row.order=order;}
    if(gpu_proof.attempted||gpu_proof.failure||!raster_seeded||raster_binding_overflow||raster_overflow||depth_copy_overflow||exact_count!=2)return;
    for(auto const& row:sdk_evals)if(!row.executed||row.queue!=queue_id)return;
    for(auto const& row:sdk_copies)if(!row.executed||row.queue!=queue_id)return;
    unsigned color_views{},depth_views{};
    for(unsigned i=0;i<raster_count;++i){auto const& row=raster_rows[i];if(row.first_eval)continue;
        if(!row.executed||row.queue!=queue_id||row.execute_order>sdk_evals[0].order)return;
        if(!row.input)color_views|=row.view_mask;}
    for(unsigned i=0;i<depth_copy_count;++i){auto const& row=depth_copies[i];if(row.eval_count)continue;
        if(!row.executed||row.queue!=queue_id||row.execute_order>sdk_evals[0].order)return;depth_views|=row.view_mask;}
    if(color_views!=3||depth_views!=3)return;
    for(unsigned i=0;i<exact_count;++i){auto const& row=exact_rows[i];if(!row.executed||row.queue!=queue_id||!row.returned||!row.stable||!row.pso_stable||!row.root_stable||!row.constants_stable||row.execute_order>sdk_evals[0].order)return;}
    for(unsigned i=0;i<2;++i)if(!ngx_mv_target_capture::results[i].accepted||!ngx_mv_target_capture::results[i].sdk_returned)return;
    gpu_proof.attempted=true;gpu_proof.queue_id=queue_id;gpu_proof.order=order;
    gears_vr::ComPtr<ID3D12Device> device;
    if(queue->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT||FAILED(queue->GetDevice(IID_PPV_ARGS(&device)))||canonical_identity(device.Get())!=ngx_mv_target_capture::selected_mv.device){gpu_proof.failure=6;return;}
    gpu_proof.result=device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&gpu_proof.fence));
    if(FAILED(gpu_proof.result)||!gpu_proof.fence){gpu_proof.failure=7;return;}
    gpu_proof.queue=queue;gpu_proof.result=queue->Signal(gpu_proof.fence.Get(),1);gpu_proof.signalled=SUCCEEDED(gpu_proof.result);
    if(!gpu_proof.signalled)gpu_proof.failure=8;
#endif
}
bool raster_active(){return target_observing(true)&&ngx_mv_target_capture::window.snapshot().phase==gears_ngx_mv_target::Phase::capturing;}
bool raster_seed(){
    if(raster_seeded)return true;if(++raster_seed_attempts>16)return false;
    auto const& selection=ngx_mv_target_capture::window.snapshot().selection;auto const& probe=ngx_epoch_pair::probe;
    if(probe.revision!=selection.revision||probe.world!=selection.world||!probe.eval_count){raster_seed_failure=1;return false;}
    constexpr unsigned native_order[]{0,2,3,1};
    for(unsigned i=0;i<4;++i){bool found{};const auto raw=probe.evals[0].input.resources[native_order[i]];
        for(unsigned j=0;j<probe.resource_count;++j)if(probe.resources[j].lease.raw==raw&&probe.resources[j].lease.materialize(raster_inputs[i])){found=true;break;}
        if(!found||(i&&raster_inputs[i].device!=raster_inputs[0].device)){raster_seed_failure=2;return false;}}
    if(raster_inputs[2].identity!=selection.mv){raster_seed_failure=3;return false;}
    Record built;inspect_completed_renderer(built,consuming_renderer);
    if(built.family_count!=2||built.renderer_count!=2){raster_seed_failure=4;return false;}
    for(unsigned i=0;i<2;++i){std::array<float,4> rect{};
        if(!safe_copy(rect.data(),reinterpret_cast<void const*>(built.family_views[i]+0x48),sizeof(rect))){raster_seed_failure=5;return false;}
        for(auto value:rect)if(!std::isfinite(value)||value<0||value>8192||std::floor(value)!=value){raster_seed_failure=6;return false;}
        raster_views[i]={uint32_t(rect[0]),uint32_t(rect[1]),uint32_t(rect[2]),uint32_t(rect[3])};
        for(auto const& input:raster_inputs)if(!gears_ngx_stereo::contains(input.extent,raster_views[i])){raster_seed_failure=7;return false;}}
    raster_seeded=true;return true;
}
void raster_targets(ID3D12GraphicsCommandList* list,D3D12_CPU_DESCRIPTOR_HANDLE const* depth){
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
    ngx_trial_producer::targets(list,depth);
#endif
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!raster_active()||!raster_seed())return;
    const auto canonical=canonical_identity(list);const auto recording=frame_link.current(canonical);
    if(!canonical||recording.identity!=canonical||!recording.generation)return;
    auto bound=frame_target_stats.binding(reinterpret_cast<uintptr_t>(list));RasterBinding value;
    value.raw=reinterpret_cast<uintptr_t>(list);value.canonical=canonical;value.generation=recording.generation;
    if(!frame_target_stats.descriptor_capacity_lost()&&bound.targets==1&&bound.target.resource==raster_inputs[0].identity&&!bound.target.mip)value.targets[0]=bound.target;
    D3D12_CPU_DESCRIPTOR_HANDLE handle{};if(depth&&safe_copy(&handle,depth,sizeof(handle))){auto target=ngx_depth_capture::lookup(handle.ptr);
        value.depth_writable=target.resource!=0;if(!target.resource)target=ngx_depth_capture::lookup_any(handle.ptr);
        if(target.resource&&!target.mip&&target.width==raster_inputs[1].extent.width&&target.height==raster_inputs[1].extent.height)value.targets[1]=target;
        else{bool seen{};for(unsigned i=0;i<depth_mismatch_count;++i)if(depth_mismatches[i].handle==handle.ptr&&depth_mismatches[i].resource==target.resource){seen=true;break;}
            if(!seen){if(depth_mismatch_count<depth_mismatches.size())depth_mismatches[depth_mismatch_count++]={handle.ptr,target.resource,canonical,target.width,target.height,target.format};else ++depth_mismatch_overflow;}}}
    RasterBinding* slot{};for(auto& old:raster_bindings)if(old.canonical==canonical){slot=&old;break;}
    if(!slot&&!value.targets[0].resource&&!value.targets[1].resource)return;
    if(!slot)for(auto& empty:raster_bindings)if(!empty.canonical){slot=&empty;break;}
    if(slot)*slot=value;else ++raster_binding_overflow;
}
void record_depth_copy(ID3D12GraphicsCommandList* list,ID3D12Resource* destination,ID3D12Resource* source,uintptr_t caller,unsigned kind,unsigned src_sub,unsigned dst_sub){
    if(!raster_active()||!raster_seed()||!destination||!source)return;
    if(canonical_identity(destination)!=raster_inputs[1].identity)return;
    auto typed=gears_ngx_stereo::TypedResourceLease::observe(source);gears_ngx_stereo::Resource resource;
    if(!typed.materialize(resource)||resource.device!=raster_inputs[1].device||!gears_ngx_stereo::same(resource.extent,raster_inputs[1].extent))return;
    const auto canonical=canonical_identity(list);const auto recording=frame_link.current(canonical);
    if(!canonical||recording.identity!=canonical||!recording.generation)return;
    unsigned mask{};for(unsigned i=0;i<raster_count;++i){auto const& row=raster_rows[i];
        if(row.input==1&&row.resource==resource.identity&&row.descriptor_writable)mask|=row.view_mask;}
    const auto ordinal=ngx_mv_target_capture::window.snapshot().eval_count;unsigned index=depth_copy_count;
    for(unsigned i=0;i<depth_copy_count;++i){auto const& old=depth_copies[i];if(old.source==resource.identity&&old.destination==raster_inputs[1].identity&&old.kind==kind&&old.source_subresource==src_sub&&old.destination_subresource==dst_sub&&old.eval_count==ordinal){index=i;break;}}
    if(index==depth_copy_count){if(depth_copy_count==depth_copies.size()){++depth_copy_overflow;return;}++depth_copy_count;depth_copy_sources[index]=std::move(typed);}
    const auto calls=depth_copies[index].calls+1;
    depth_copies[index]={resource.identity,raster_inputs[1].identity,canonical,caller,recording.generation,kind,src_sub,dst_sub,mask,true,false,calls,ordinal,0,0};
}
void texture_copy(ID3D12GraphicsCommandList* list,D3D12_TEXTURE_COPY_LOCATION const* destination,UINT x,UINT y,UINT z,
    D3D12_TEXTURE_COPY_LOCATION const* source,D3D12_BOX const* box,uintptr_t caller){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!raster_active()||!raster_seed())return;
    D3D12_TEXTURE_COPY_LOCATION d{},s{};D3D12_BOX region{};
    if(!safe_copy(&d,destination,sizeof(d))||!safe_copy(&s,source,sizeof(s))||x||y||z||d.Type!=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX||s.Type!=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX||d.SubresourceIndex||s.SubresourceIndex)return;
    if(!box&&d.pResource&&s.pResource&&canonical_identity(d.pResource)==raster_inputs[0].identity&&canonical_identity(s.pResource)==raster_inputs[3].identity)sdk_copy(list,caller);
    if(box&&(!safe_copy(&region,box,sizeof(region))||region.left||region.top||region.front||region.right!=raster_inputs[1].extent.width||region.bottom!=raster_inputs[1].extent.height||region.back!=1))return;
    record_depth_copy(list,d.pResource,s.pResource,caller,1,s.SubresourceIndex,d.SubresourceIndex);
}
void STDMETHODCALLTYPE copy_resource(ID3D12GraphicsCommandList* list,ID3D12Resource* destination,ID3D12Resource* source){
#if defined(GEARS_NATIVE_OWNED_PIXELS) && GEARS_NATIVE_OWNED_PIXELS
    // One hook owns this ABI when both trackers are compiled. Keep the owned
    // use observation and bypass behavior of its former dedicated wrapper.
    if(stereo_publish_bypass){original_copy_resource(list,destination,source);return;}
#endif
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());Scope scope;auto guard=frame_lock();
#if defined(GEARS_NATIVE_OWNED_PIXELS) && GEARS_NATIVE_OWNED_PIXELS
    if(owned_scene_gpu::watching()){
        owned_scene_gpu::use(list,canonical_identity(destination),true);owned_scene_gpu::use(list,canonical_identity(source),true);}
#endif
    original_copy_resource(list,destination,source);
#if defined(GEARS_NGX_OWNED_SCENE_TRIAL) && GEARS_NGX_OWNED_SCENE_TRIAL
    D3D12_TEXTURE_COPY_LOCATION d{},s{};d.pResource=destination;s.pResource=source;
    d.Type=s.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    ngx_capture_control::owned_copy_observe(list,&d,0,0,0,&s,nullptr,caller,2);
#endif
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
    ngx_trial_producer::depth_copy(destination,source,caller);
#endif
    record_depth_copy(list,destination,source,caller,2,0,0);
}
void raster_draw(ID3D12GraphicsCommandList* list,uintptr_t caller,unsigned elements,unsigned instances){
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
    ngx_trial_producer::draw(list,elements,instances);
#endif
    if(!elements||!instances)return;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!raster_active()||!raster_seed())return;
    const auto canonical=canonical_identity(list);const auto recording=frame_link.current(canonical);RasterBinding const* bound{};
    for(auto const& old:raster_bindings)if(old.canonical==canonical&&old.generation==recording.generation){bound=&old;break;}
    if(!bound)return;auto state=frame_target_stats.draw_state(reinterpret_cast<uintptr_t>(list));
    if(!state.tracked||state.viewport_count!=1||state.scissor_count!=1){++raster_unknown_rectangles;return;}
    const auto& v=state.viewport;const auto& s=state.scissor;
    const double left=std::max<double>(0,std::max(v.x,s.x)),top=std::max<double>(0,std::max(v.y,s.y));
    const double right=std::min<double>(double(v.x)+v.width,double(s.x)+s.width),bottom=std::min<double>(double(v.y)+v.height,double(s.y)+s.height);
    for(double edge:{left,top,right,bottom})if(!std::isfinite(edge)||edge<0||edge>8192||std::floor(edge)!=edge){++raster_unknown_rectangles;return;}
    if(right<=left||bottom<=top)return;gears_ngx_stereo::Rect clip{uint32_t(left),uint32_t(top),uint32_t(right-left),uint32_t(bottom-top)};
    const auto ordinal=ngx_mv_target_capture::window.snapshot().eval_count;
    for(unsigned input=0;input<2;++input){auto const& target=bound->targets[input];if(!target.resource||!gears_ngx_stereo::contains(raster_inputs[input].extent,clip))continue;
        unsigned mask{};for(unsigned eye=0;eye<2;++eye){auto const& r=raster_views[eye];
            if(clip.x<=r.x&&clip.y<=r.y&&uint64_t(clip.x)+clip.width>=uint64_t(r.x)+r.width&&uint64_t(clip.y)+clip.height>=uint64_t(r.y)+r.height)mask|=1u<<eye;}
        RasterRow* row{};for(unsigned i=0;i<raster_count;++i){auto& old=raster_rows[i];
            if(old.input==input&&old.resource==target.resource&&old.list==canonical&&old.generation==recording.generation&&old.descriptor_writable==(input==0||bound->depth_writable)&&old.clip.x==clip.x&&old.clip.y==clip.y&&old.clip.width==clip.width&&old.clip.height==clip.height){row=&old;break;}}
        if(!row){if(raster_count==raster_rows.size()){++raster_overflow;continue;}row=&raster_rows[raster_count++];
            *row={target.resource,canonical,caller,recording.generation,0,input,mask,ordinal,ordinal,clip,input==0||bound->depth_writable};}
        ++row->count;row->last_eval=ordinal;row->caller=caller;
    }
}
void census(CallerOperation operation,uintptr_t caller,bool owned){
    // Scalar call-site evidence remains useful after the binding bank fails.
    // It never admits a list, retains COM, or extends the selected Window.
    if(!target_observing(owned))return;
    if(!caller_image_checked){
        caller_image_checked=true;IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS64 nt{};
        if(base&&safe_copy(&dos,reinterpret_cast<void*>(base),sizeof(dos))&&dos.e_magic==IMAGE_DOS_SIGNATURE&&
           dos.e_lfanew>0&&dos.e_lfanew<0x100000&&safe_copy(&nt,reinterpret_cast<void*>(base+dos.e_lfanew),sizeof(nt))&&
           nt.Signature==IMAGE_NT_SIGNATURE&&nt.OptionalHeader.Magic==IMAGE_NT_OPTIONAL_HDR64_MAGIC)
            caller_image_size=nt.OptionalHeader.SizeOfImage;
    }
    ++caller_calls;const auto phase=unsigned(ngx_mv_target_capture::window.snapshot().phase);
    const bool in_game=caller_image_size&&caller>=base&&caller-base<caller_image_size;
    const auto rva=in_game?caller-base:0;
    CallerRow* row{};
    for(unsigned i=0;i<caller_row_count;++i){auto& candidate=caller_rows[i];
        if(candidate.operation==operation&&candidate.phase==phase&&candidate.caller==caller){row=&candidate;break;}}
    if(!row){
        if(caller_row_count==caller_rows.size()){++caller_overflow_calls;return;}
        row=&caller_rows[caller_row_count++];row->operation=operation;row->phase=phase;row->caller=caller;row->rva=rva;row->in_game_image=in_game;
        // Current call location alone does not establish historical identity.
        // Candidate labels also require the existing loaded-host attestation.
        row->historical_candidate=in_game&&ngx_stereo_host::attested&&(
            (operation==CallerOperation::table&&rva==0x17b99a)||(operation==CallerOperation::dispatch&&rva==0x1ac833));
        row->first_ticket=consuming_frame_ticket;
    }
    ++row->calls;if(failed)++row->bank_failed_calls;row->last_ticket=consuming_frame_ticket;
}
struct Depth {
    bool outer,relevant;int exceptions=std::uncaught_exceptions();
    explicit Depth(bool observed):outer(depth++==0),relevant(observed){if(!outer&&relevant&&pending())lose(nested,"depth_enter");}
    ~Depth(){--depth;if(relevant&&std::uncaught_exceptions()!=exceptions&&pending())lose(unwinding,"depth_unwind");}
};
List* known(ID3D12GraphicsCommandList* list){
    if(!list)return nullptr;const auto raw=reinterpret_cast<uintptr_t>(list);
    for(unsigned i=0;i<list_count;++i)if(lists[i].raw==raw)return &lists[i];return nullptr;
}
uint64_t resolved_raw{},resolved_alias{},resolved_unmatched{},resolve_qi_failed{},resolve_reentrant{};
thread_local bool resolving{};
struct Resolution {
    List* held{};uintptr_t raw{},canonical{};unsigned kind{};
    gears_vr::ComPtr<IUnknown> identity;
    gears_vr::ComPtr<ID3D12Device> device;
    gears_vr::ComPtr<IUnknown> device_identity;
    explicit Resolution(ID3D12GraphicsCommandList* list):raw(reinterpret_cast<uintptr_t>(list)){
        {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
            if((!pending()&&!exact_pending())||!list)return;
            if(resolving){++resolve_reentrant;exact_stop(1);if(pending())lose(nested,"resolve_reentrant",nullptr,raw);return;}
            // A callback nested under our native forward still owns its outer
            // frame lock. Reject both raw and alias paths before any fast return.
            if(depth){++resolve_reentrant;exact_stop(1);if(pending())lose(nested,"resolve_nested",nullptr,raw);return;}
            if((held=known(list))){++held->callback_busy;canonical=held->canonical;kind=1;++resolved_raw;return;}}
        // Typed callback input only; never reconstruct COM from a scalar key.
        // QI and its eventual temporary Release both run outside our lock.
        resolving=true;HRESULT hr=E_FAIL;
        try{hr=list->QueryInterface(IID_PPV_ARGS(&identity));}catch(...){resolving=false;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);++resolve_qi_failed;exact_stop(1);if(pending())lose(unwinding,"resolve_qi_unwind",nullptr,raw);throw;}
        resolving=false;canonical=reinterpret_cast<uintptr_t>(identity.Get());
        std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!pending()&&!exact_pending())return;
        if(FAILED(hr)||!canonical){++resolve_qi_failed;kind=4;exact_stop(1);if(pending())lose(invalid_list,"resolve_qi_failed",nullptr,raw,canonical,hr);return;}
        for(unsigned i=0;i<list_count;++i)if(lists[i].canonical==canonical){held=&lists[i];++held->callback_busy;kind=2;++resolved_alias;return;}
        kind=3;++resolved_unmatched;
    }
    bool selected_device(ID3D12GraphicsCommandList* list){
        {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!exact_pending()||!canonical)return false;
            if(resolving||depth){exact_stop(1);return false;}}
        resolving=true;HRESULT hr=E_FAIL;
        try{hr=list->GetDevice(IID_PPV_ARGS(&device));if(SUCCEEDED(hr))hr=device?device.As(&device_identity):E_FAIL;}
        catch(...){resolving=false;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);exact_stop(1);throw;}
        resolving=false;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        if(FAILED(hr)||!device_identity){exact_stop(1);return false;}
        return exact_pending()&&reinterpret_cast<uintptr_t>(device_identity.Get())==ngx_mv_target_capture::selected_mv.device;
    }
    ~Resolution(){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(held)--held->callback_busy;}
    // identity releases after the destructor body's short lock has gone away.
};
bool exact_identity_current(ExactAssociation const& a){
    const auto live=frame_link.current(a.canonical);auto const& w=ngx_mv_target_capture::window.snapshot();
    return a.canonical&&a.generation&&live.identity==a.canonical&&live.generation==a.generation&&
        w.phase==gears_ngx_mv_target::Phase::capturing&&w.selection.world==a.world&&w.consume.ticket==a.ticket&&w.consume.renderer==a.renderer;
}
bool witness_matches(PsoWitness const& witness,ExactAssociation const& a){
    return gears_ngx_mv_compute::pso_witness_matches(witness.key,witness.valid,a.canonical,a.generation,a.world,a.ticket,a.renderer,pso_serial);
}
bool root_matches(RootWitness const& witness,ExactAssociation const& a){
    return gears_ngx_mv_compute::root_witness_matches(witness.key,witness.valid,a.canonical,a.generation,a.world,a.ticket,a.renderer,root_serial);
}
PsoWitness* matching_pso(ExactAssociation const& a){
    if(witness_matches(pso_promoted,a))return &pso_promoted;
    for(unsigned i=exact_history_count;i;--i)if(witness_matches(exact_history[i-1].pso,a))return &exact_history[i-1].pso;
    return nullptr;
}
RootWitness* matching_root(ExactAssociation const& a){
    if(root_matches(root_promoted,a))return &root_promoted;
    for(unsigned i=exact_history_count;i;--i)if(root_matches(exact_history[i-1].root,a))return &exact_history[i-1].root;
    return nullptr;
}
// Root identity is obtained only from this typed setter. The serialized-root
// bank owns the matching object through the observation; no scalar becomes COM.
// Temporary QI/GetDevice owners release after the callback's frame lock.
struct RootScope {
    Resolution& resolved;RootWitness candidate;bool entered{},outer{},attempted{},finished{},owned{};
    gears_vr::ComPtr<IUnknown> identity,device_identity;
    gears_vr::ComPtr<ID3D12Device> device;
    RootScope(Resolution& r,ID3D12RootSignature* native,uintptr_t caller,bool own):resolved(r),owned(own){
        {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!exact_pending())return;
            entered=true;outer=root_set_depth++==0;++root_sets;root_invalidate();
            if(!outer||!native||!ngx_stereo_host::attested||!target_observing(owned)||!resolved.canonical||
               (caller!=base+0x138124&&caller!=base+0x22018d))return;
            const auto live=frame_link.current(resolved.canonical);auto const& w=ngx_mv_target_capture::window.snapshot();
            if(live.identity!=resolved.canonical||!live.generation){++root_rejected;return;}
            candidate.key={resolved.canonical,0,0,reinterpret_cast<uintptr_t>(consuming_renderer),live.generation,w.selection.world,consuming_frame_ticket,root_serial};}
        if(resolving){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);++root_rejected;return;}
        resolving=true;HRESULT hr=E_FAIL;
        try{hr=native->QueryInterface(IID_PPV_ARGS(&identity));if(SUCCEEDED(hr))hr=native->GetDevice(IID_PPV_ARGS(&device));
            if(SUCCEEDED(hr))hr=device.As(&device_identity);}
        catch(...){resolving=false;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);++root_rejected;root_invalidate();return;}
        resolving=false;
        std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        if(FAILED(hr)||!identity||!device_identity||!exact_pending()||candidate.key.serial!=root_serial||
           reinterpret_cast<uintptr_t>(device_identity.Get())!=ngx_mv_target_capture::selected_mv.device){++root_rejected;return;}
        candidate.key.root=reinterpret_cast<uintptr_t>(identity.Get());candidate.key.device=reinterpret_cast<uintptr_t>(device_identity.Get());
        const auto source=ngx_root_signature_capture::lookup(candidate.key.root,candidate.key.device);
        if(!source.found||!source.retained||source.fixture){++root_unknown;return;}
        candidate.key.provenance=source.index;candidate.key.size=source.size;attempted=true;
    }
    void finish(){
        finished=true;if(!attempted)return;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        const auto live=frame_link.current(candidate.key.canonical);
        if(!exact_pending()||!target_observing(owned)||live.identity!=candidate.key.canonical||
           !gears_ngx_mv_compute::root_witness_matches(candidate.key,true,candidate.key.canonical,live.generation,
                committed_world_generation.load(),consuming_frame_ticket,reinterpret_cast<uintptr_t>(consuming_renderer),root_serial)){
            ++root_rejected;return;}
        candidate.valid=true;root_pending=candidate;++root_candidates;
    }
    ~RootScope(){if(entered){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);--root_set_depth;if(!finished)root_invalidate();}}
};
struct PsoSetActivity {
    PsoWitness retired;bool outer{},entered{};
    PsoSetActivity(){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!ngx_mv_target_capture::pending())return;
        entered=true;outer=pso_set_depth++==0;++pso_sets;
        pso_invalidate(1);if(outer){retired=std::move(pso_pending);pso_pending=PsoWitness{};}}
    ~PsoSetActivity(){if(entered)--pso_set_depth;}
};
struct SetScope {
    // Activity revokes before alias QI can reenter. All local owners die after
    // the enclosing callback lock; Scope must enclose this entire object.
    PsoSetActivity activity;Resolution resolved;PsoWitness candidate;
    gears_vr::ComPtr<ID3D12Device> device;gears_vr::ComPtr<IUnknown> device_identity;
    bool owned{},attempted{},finished{};
    SetScope(ID3D12GraphicsCommandList* list,ID3D12PipelineState* pso,uintptr_t caller,bool own):resolved(activity.outer?list:nullptr),owned(own){
        {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
            if(!activity.outer||!pso||!ngx_stereo_host::attested||caller!=base+0x17b514||!target_observing(owned)||!resolved.canonical)return;
            auto const& w=ngx_mv_target_capture::window.snapshot();const auto live=frame_link.current(resolved.canonical);
            if(live.identity!=resolved.canonical||!live.generation){++pso_discards;pso_last_discard=6;return;}
            candidate.key={resolved.canonical,0,reinterpret_cast<uintptr_t>(consuming_renderer),live.generation,w.selection.world,consuming_frame_ticket,pso_serial};}
        try{if(!resolved.selected_device(list)){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);++pso_discards;pso_last_discard=7;return;}}
        catch(...){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);++pso_discards;pso_invalidate(4);return;}
        // Actual typed callback arguments only. Unknown PSO identity/device
        // drops this witness, without stopping the exact-MV association.
        resolving=true;HRESULT hr=E_FAIL;
        try{hr=pso->QueryInterface(IID_PPV_ARGS(&candidate.pso_identity));
            if(SUCCEEDED(hr))hr=pso->GetDevice(IID_PPV_ARGS(&device));
            if(SUCCEEDED(hr))hr=device?device.As(&device_identity):E_FAIL;}
        catch(...){resolving=false;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);++pso_qi_failures;++pso_discards;pso_invalidate(4);return;}
        resolving=false;
        {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
            if(FAILED(hr)||!candidate.pso_identity||!device_identity){++pso_qi_failures;++pso_discards;pso_last_discard=4;return;}
            if(reinterpret_cast<uintptr_t>(device_identity.Get())!=ngx_mv_target_capture::selected_mv.device){++pso_discards;pso_last_discard=7;return;}
            candidate.key.pso=reinterpret_cast<uintptr_t>(candidate.pso_identity.Get());
            IUnknown* id=resolved.identity?resolved.identity.Get():resolved.held?resolved.held->identity.Get():nullptr;
            if(!id||reinterpret_cast<uintptr_t>(id)!=candidate.key.canonical)return;
            candidate.list=list;candidate.list_identity=id;candidate.pso=pso;attempted=true;++pso_candidates;}
    }
    void finish(){
        finished=true;if(!attempted)return;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        const auto live=frame_link.current(candidate.key.canonical);
        if(!target_observing(owned)||live.identity!=candidate.key.canonical||live.generation!=candidate.key.generation||
           !gears_ngx_mv_compute::pso_witness_matches(candidate.key,true,candidate.key.canonical,live.generation,
                committed_world_generation.load(),consuming_frame_ticket,reinterpret_cast<uintptr_t>(consuming_renderer),pso_serial)){
            ++pso_discards;pso_last_discard=3;return;}
        candidate.valid=true;pso_pending=std::move(candidate);
    }
    ~SetScope(){if(activity.entered&&!finished){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);pso_invalidate(5);}}
};
struct ExactTableCall {
    Resolution& resolved;ID3D12GraphicsCommandList* list;ExactAssociation candidate;bool owned{},attempted{},finished{};
    ExactTableCall(Resolution& r,ID3D12GraphicsCommandList* l,UINT index,uint64_t gpu,uintptr_t caller,bool own,bool device):resolved(r),list(l),owned(own){
        if(index!=3)return;exact_mutation(2);
        if(!exact_pending()||!device||!ngx_stereo_host::attested||caller!=base+0x17b99a||!target_observing(owned))return;
        auto const& w=ngx_mv_target_capture::window.snapshot();if(w.phase!=gears_ngx_mv_target::Phase::capturing)return;
        ++exact_tables;candidate.canonical=r.canonical;candidate.generation=frame_link.current(r.canonical).generation;
        candidate.ticket=w.consume.ticket;candidate.renderer=w.consume.renderer;candidate.world=w.selection.world;candidate.serial=exact_serial;
        if(!exact_identity_current(candidate)){exact_stop(3);return;}
        candidate.descriptor=ngx_mv_descriptor_capture::lookup_exact_target(gpu);
        if(!candidate.descriptor.match){++exact_misses;return;}
        attempted=true;exact_arming=true;
    }
    void finish(){
        finished=true;if(!attempted||!exact_pending())return;
        const auto after=ngx_mv_descriptor_capture::lookup_exact_target(candidate.descriptor.gpu);
        if(!target_observing(owned)||!exact_identity_current(candidate)||exact_serial!=candidate.serial||
           !ngx_mv_descriptor_capture::same_exact(candidate.descriptor,after)){exact_stop(4);return;}
        IUnknown* id=resolved.identity?resolved.identity.Get():resolved.held?resolved.held->identity.Get():nullptr;
        if(!id||reinterpret_cast<uintptr_t>(id)!=candidate.canonical||exact_list_owner){exact_stop(1);return;}
        exact_list_owner=list;exact_identity_owner=id;candidate.active=true;exact_association=candidate;exact_arming=false;
        if(!pso_promoted.pso&&witness_matches(pso_pending,candidate)){pso_promoted=std::move(pso_pending);pso_pending=PsoWitness{};++pso_promotions;}
        else if(pso_pending.valid)pso_invalidate(3);
        if(!root_promoted.key.root&&root_matches(root_pending,candidate)){root_promoted=root_pending;root_pending={};++root_promotions;}
    }
    ~ExactTableCall(){if(attempted&&!finished)exact_stop(5);}
};
struct ExactDispatchCall {
    ExactDispatch* row{};bool owned{},finished{};
    ExactDispatchCall(Resolution const& resolved,uintptr_t caller,UINT x,UINT y,UINT z,bool own):owned(own){
        if(!exact_pending()||!exact_association.active)return;
        if(!resolved.canonical){exact_stop(1);return;}
        if(resolved.canonical!=exact_association.canonical)return;
        if(!target_observing(owned)||!exact_identity_current(exact_association)){exact_stop(3);return;}
        if(!ngx_stereo_host::attested||caller!=base+0x1ac833)return;
        const auto current=ngx_mv_descriptor_capture::lookup_exact_target(exact_association.descriptor.gpu);
        if(!ngx_mv_descriptor_capture::same_exact(exact_association.descriptor,current)){exact_stop(4);return;}
        ++exact_dispatches;if(exact_count==exact_rows.size()){++exact_overflow;exact_stop(6);return;}
        row=&exact_rows[exact_count++];row->association=exact_association;row->caller=caller;row->groups={x,y,z};
        if(auto* pso=matching_pso(exact_association)){row->pso=pso->key.pso;row->pso_serial=pso_serial;}
        if(auto* root=matching_root(exact_association))row->root=root->key;
        row->constants=ngx_mv_constants_capture::snapshot(resolved.raw,resolved.canonical,row->association.generation);
        ++exact_inflight;
    }
    void finish(){
        finished=true;if(!row)return;row->returned=true;row->after=ngx_mv_descriptor_capture::lookup_exact_target(row->association.descriptor.gpu);
        row->stable=exact_pending()&&exact_association.active&&target_observing(owned)&&exact_identity_current(row->association)&&
            exact_serial==row->association.serial&&ngx_mv_descriptor_capture::same_exact(row->association.descriptor,row->after);
        if(!row->stable)exact_stop(4);
        auto* pso=matching_pso(row->association);auto* root=matching_root(row->association);
        row->pso_stable=row->stable&&pso&&row->pso&&row->pso==pso->key.pso&&row->pso_serial==pso_serial;
        if(pso)pso->attest_stable_dispatch(row->pso_stable);
        row->root_stable=row->stable&&root&&row->root.root&&row->root.serial==root_serial;
        if(root)root->attest_stable_dispatch(row->root_stable);
        row->constants_stable=row->stable&&row->root_stable&&row->pso_stable&&ngx_mv_constants_capture::stable(row->constants);
    }
    ~ExactDispatchCall(){if(row){if(!finished)exact_stop(5);--exact_inflight;}}
};
struct DispatchIdentity {
    uintptr_t raw{},canonical{},retained_raw{},current_identity{};
    uint64_t stored_generation{},current_generation{},calls{};unsigned kind{},phase{};
};
std::array<DispatchIdentity,8> dispatch_identities;unsigned dispatch_identity_count{};uint64_t dispatch_identity_overflow{};
void note_dispatch_identity(Resolution const& resolved){
    DispatchIdentity value;value.raw=resolved.raw;value.canonical=resolved.canonical;value.kind=resolved.kind;
    value.phase=unsigned(ngx_mv_target_capture::window.snapshot().phase);
    if(resolved.held){value.retained_raw=resolved.held->raw;value.stored_generation=resolved.held->generation;}
    if(value.canonical){const auto live=frame_link.current(value.canonical);value.current_identity=live.identity;value.current_generation=live.generation;}
    for(unsigned i=0;i<dispatch_identity_count;++i){auto& old=dispatch_identities[i];
        if(old.raw==value.raw&&old.canonical==value.canonical&&old.retained_raw==value.retained_raw&&old.current_identity==value.current_identity&&
           old.stored_generation==value.stored_generation&&old.current_generation==value.current_generation&&old.kind==value.kind&&old.phase==value.phase){++old.calls;return;}}
    if(dispatch_identity_count==dispatch_identities.size()){++dispatch_identity_overflow;return;}
    value.calls=1;dispatch_identities[dispatch_identity_count++]=value;
}
bool may_admit(CallerOperation operation,uintptr_t caller){
    const auto phase=ngx_mv_target_capture::window.snapshot().phase;
    return operation==CallerOperation::root&&ngx_stereo_host::attested&&caller==base+0x22018d&&
        (phase==gears_ngx_mv_target::Phase::waiting||phase==gears_ngx_mv_target::Phase::capturing);
}
bool scoped_list(Resolution const& resolved,CallerOperation operation,uintptr_t caller){
    if(resolved.held||may_admit(operation,caller))return true;
    ++unknown_skipped[unsigned(operation)];return false;
}
Recording ref(List const& list){return {list.canonical,list.generation,frame_link.current(list.canonical).generation};}
List* reclaim(){
    ++reuse_attempts;capacity_occupancy={};List* victim{};
    for(unsigned i=0;i<list_count;++i){auto& held=lists[i];auto& o=capacity_occupancy;++o.occupied;
        const auto live=frame_link.current(held.canonical);const bool empty=bindings.binding_empty(held.canonical);
        const bool busy=held.callback_busy||held.reset_busy;
        const bool identity_matches=live.identity==held.canonical||(!held.generation&&!live.generation&&!live.identity);
        const bool eligible=identity_matches&&gears_ngx_mv_compute::reclaimable(held.revoked,busy,held.history_pinned,empty,held.generation,live.generation);
        // Counts overlap deliberately: each scalar states a protection present
        // on this bounded snapshot, rather than hiding multiple blockers.
        if(eligible){++o.eligible;if(!victim)victim=&held;}
        if(!empty)++o.partial;if(held.history_pinned)++o.pinned;if(busy)++o.busy;if(!held.revoked)++o.unrevoked;
        if(!held.generation&&live.generation)++o.unknown_blocked;
        if(!identity_matches||(held.generation&&held.generation!=live.generation))++o.stale_blocked;
    }return victim;
}
List* track(ID3D12GraphicsCommandList* list,Retirement& retirement,Resolution& resolved,CallerOperation operation,uintptr_t caller){
    auto* old=resolved.held;if(old){const auto observed=ref(*old);
        if(!old->generation||!observed.attested_current_generation||observed.attested_current_generation!=old->generation){
            lose(invalid_generation,"track_existing",old,reinterpret_cast<uintptr_t>(list));return nullptr;}return old;}
    if(!may_admit(operation,caller))return nullptr;
    if(!list||list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT)return nullptr;
    auto& id=retirement.identity;auto& device=retirement.device;auto& device_id=retirement.device_identity;
    id=std::move(resolved.identity);
    if(!id||FAILED(list->GetDevice(IID_PPV_ARGS(&device)))||FAILED(device.As(&device_id))||!device_id){
        lose(invalid_list,"track_identity",nullptr,reinterpret_cast<uintptr_t>(list),reinterpret_cast<uintptr_t>(id.Get()));return nullptr;}
    // A verified list belonging to another device cannot describe the selected
    // target. It neither consumes a row nor invalidates this device's evidence.
    if(reinterpret_cast<uintptr_t>(device_id.Get())!=ngx_mv_target_capture::selected_mv.device){++foreign_device_skips;return nullptr;}
    const auto canonical=reinterpret_cast<uintptr_t>(id.Get());const auto current=frame_link.current(canonical);
    if(!canonical||current.identity!=canonical||!current.generation){lose(invalid_generation,"track_new_generation",nullptr,reinterpret_cast<uintptr_t>(list),canonical);return nullptr;}
    // Resolution ran before the callback lock. Another callback may have
    // admitted this same canonical identity in that interval; reuse strictly.
    for(unsigned i=0;i<list_count;++i)if(lists[i].canonical==canonical){
        auto& held=lists[i];if(!held.generation||held.generation!=current.generation){lose(invalid_generation,"track_setup_alias",&held,reinterpret_cast<uintptr_t>(list));return nullptr;}return &held;}
    List* slot{};
    if(list_count==lists.size()){
        slot=reclaim();if(!slot){lose(list_capacity,"track_capacity",nullptr,reinterpret_cast<uintptr_t>(list),canonical);return nullptr;}
        if(!bindings.erase_empty(slot->canonical)){lose(list_capacity,"track_reclaim_core",slot);return nullptr;}
        retirement.retired=std::move(*slot);*slot=List{};++reused;
    }else slot=&lists[list_count++];
    auto& out=*slot;out.raw=reinterpret_cast<uintptr_t>(list);out.canonical=canonical;
    out.generation=current.generation;out.held=list;out.identity=std::move(id);return &out;
}
bool current(List const& list){ngx_mv_target_capture::poll();const auto r=ref(list);if(!r.attested_current_generation||r.generation!=r.attested_current_generation){lose(invalid_generation,"current",&list);return false;}return pending();}
void noted(List& list){if(bindings.failed()){lose(invalid_generation,"noted_core",&list);return;}list.revoked=false;++mutation;
    if(ngx_mv_target_capture::window.snapshot().phase==gears_ngx_mv_target::Phase::waiting)++waiting_bindings;else ++capture_bindings;}
void invalidate_held(List* held,unsigned kind){
    if(!pending()||!held)return;
    // Erasing knowledge requires only the retained identity. It cannot admit a
    // recording and must not validate a generation already replaced by Reset.
    ++invalidations;++mutation;held->revoked=true;bindings.forget_binding(held->canonical);
    (void)kind;
}
struct RevocationScope {
    Resolution resolved;unsigned kind;bool finished{};
    RevocationScope(ID3D12GraphicsCommandList* list,unsigned value):resolved(list),kind(value){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(ngx_mv_target_capture::pending()){pso_invalidate(2);root_invalidate();ngx_mv_constants_capture::invalidate();}exact_mutation(8);invalidate_held(resolved.held,kind);}
    void finish(){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);finished=true;invalidate_held(resolved.held,kind);}
    ~RevocationScope(){if(!finished){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);invalidate_held(resolved.held,kind);if(pending())lose(unwinding,"revoke_unwind",resolved.held,resolved.raw);}}
};
struct ResetScope {
    Resolution resolved;ID3D12GraphicsCommandList* list;List* retained{};bool returned{};
    explicit ResetScope(ID3D12GraphicsCommandList* l):resolved(l),list(l){
        std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);EvidenceScope evidence("reset_begin",list);
        if(ngx_mv_target_capture::pending()){pso_invalidate(2);root_invalidate();ngx_mv_constants_capture::invalidate();}
        exact_mutation(8);
        if(!pending())return;auto* held=resolved.held;if(!held)return;
        retained=held;++held->reset_busy;
        held->reset_prior_generation=held->generation;held->reset_prior_known=true;
        invalidate_held(held,0);held->generation=0;
    }
    void finish(HRESULT hr,gears_frame_link::ListRef actual={}){
        EvidenceScope evidence("reset_finish",list);
        returned=true;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        auto* held=retained;if(held){--held->reset_busy;retained=nullptr;}if(!pending()||!held)return;
        if(FAILED(hr)){lose(invalid_generation,"reset_finish_failed",held,reinterpret_cast<uintptr_t>(list),0,hr);return;}
        // A successful bypass/unowned Reset supplies no birth receipt. Leave
        // this list unknown rather than borrowing a still-visible old tracker
        // generation. Only this exact Reset invocation may supply `actual`.
        if(!actual.identity&&!actual.generation)return;
        const auto live=frame_link.current(held->canonical);
        if(actual.identity!=held->canonical||!actual.generation||live.identity!=actual.identity||live.generation!=actual.generation){
            lose(invalid_generation,"reset_finish_attestation",held,reinterpret_cast<uintptr_t>(list),0,hr);return;
        }
        ++mutation;held->generation=actual.generation;
        bindings.reset({held->canonical,actual.generation,live.generation});
        if(bindings.failed())lose(invalid_generation,"reset_finish_core",held,reinterpret_cast<uintptr_t>(list),0,hr);
    }
    ~ResetScope(){if(!returned){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);EvidenceScope evidence("reset_unwind",list);
        auto* held=retained;if(held){--held->reset_busy;retained=nullptr;}if(pending())lose(unwinding,"reset_unwind",held,reinterpret_cast<uintptr_t>(list));}}
};
void STDMETHODCALLTYPE heaps(ID3D12GraphicsCommandList* list,UINT count,ID3D12DescriptorHeap* const* native){
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());Scope scope;Resolution resolved(list);Retirement retirement;
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);census(CallerOperation::heaps,caller,scope.world());EvidenceScope evidence("heaps",list);
    ngx_mv_constants_capture::invalidate();exact_mutation(9);
    const bool observe=observing(scope.world()),scoped=observe&&scoped_list(resolved,CallerOperation::heaps,caller);Depth call(resolved.held||scoped);
    List* held=call.outer&&scoped?track(list,retirement,resolved,CallerOperation::heaps,caller):nullptr;ListUse use(held);
    HeapLease selected;std::array<ID3D12DescriptorHeap*,2> values{};
    if(held){
        if(count>values.size()||(count&&!safe_copy(values.data(),native,count*sizeof(values[0]))))lose(invalid_heap,"heaps_array",held);
        else for(UINT i=0;i<count;++i){if(!values[i]){lose(invalid_heap,"heaps_null",held);break;}
            const auto desc=values[i]->GetDesc();if(desc.Type!=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)continue;
            if(selected.retained()){lose(invalid_heap,"heaps_duplicate",held);break;}selected=HeapLease::observe(values[i]);
            if(!selected.retained()||!selected.visible||selected.device!=ngx_mv_target_capture::selected_mv.device){lose(invalid_heap,"heaps_identity",held);break;}}
    }else invalidate_held(resolved.held,0);
    const auto before=mutation;original_heaps(list,count,native);
    if(held&&current(*held)){if(before!=mutation){lose(changed,"heaps_post_mutation",held);return;}
        held->heap=std::move(selected);bindings.shader_heap(ref(*held),held->heap.canonical);noted(*held);}
}
void STDMETHODCALLTYPE root(ID3D12GraphicsCommandList* list,ID3D12RootSignature* native){
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());Scope scope;Resolution resolved(list);Retirement retirement;
    RootScope witness(resolved,native,caller,scope.world());
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);census(CallerOperation::root,caller,scope.world());EvidenceScope evidence("root",list);
    ngx_mv_constants_capture::invalidate();exact_mutation(9);
    const bool observe=observing(scope.world()),scoped=observe&&scoped_list(resolved,CallerOperation::root,caller);Depth call(resolved.held||scoped);
    List* held=call.outer&&scoped?track(list,retirement,resolved,CallerOperation::root,caller):nullptr;ListUse use(held);
    gears_vr::ComPtr<ID3D12RootSignature> owner;gears_vr::ComPtr<IUnknown> id;
    if(held&&native){gears_vr::ComPtr<ID3D12Device> device;gears_vr::ComPtr<IUnknown> di;owner=native;
        if(FAILED(native->QueryInterface(IID_PPV_ARGS(&id)))||FAILED(native->GetDevice(IID_PPV_ARGS(&device)))||FAILED(device.As(&di))||
           reinterpret_cast<uintptr_t>(di.Get())!=ngx_mv_target_capture::selected_mv.device)lose(invalid_root,"root_identity",held);
    }else if(!held)invalidate_held(resolved.held,0);
    const auto before=mutation;original_root(list,native);witness.finish();
    if(held&&current(*held)){if(before!=mutation){lose(changed,"root_post_mutation",held);return;}held->root=std::move(owner);held->root_identity=std::move(id);
        bindings.compute_root_signature(ref(*held),reinterpret_cast<uintptr_t>(held->root_identity.Get()));noted(*held);}
}
void STDMETHODCALLTYPE table(ID3D12GraphicsCommandList* list,UINT index,D3D12_GPU_DESCRIPTOR_HANDLE gpu){
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());Scope scope;Resolution resolved(list);Retirement retirement;
    const bool probe_device=index==3&&ngx_stereo_host::attested&&caller==base+0x17b99a&&resolved.selected_device(list);
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);census(CallerOperation::table,caller,scope.world());EvidenceScope evidence("table",list);
    const bool observe=observing(scope.world()),scoped=observe&&scoped_list(resolved,CallerOperation::table,caller);Depth call(resolved.held||scoped);
    List* held=call.outer&&scoped?track(list,retirement,resolved,CallerOperation::table,caller):nullptr;ListUse use(held);if(!held)invalidate_held(resolved.held,0);
    ExactTableCall probe(resolved,list,index,gpu.ptr,caller,scope.world(),probe_device);
    const auto before=mutation;original_table(list,index,gpu);probe.finish();
    auto constants_identity=resolved.canonical;
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
    // Diagnostic Resolution deliberately sleeps before its selected window.
    // Live control resolves the actual typed list independently for CB0.
    if(index==1&&ngx_trial_producer::active())constants_identity=canonical_identity(list);
#endif
    ngx_mv_constants_capture::table(resolved.raw,constants_identity,frame_link.current(constants_identity).generation,index,gpu.ptr,caller);
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
    ngx_trial_producer::table(list,index,gpu.ptr,caller);
#endif
    if(held&&current(*held)){if(before!=mutation){lose(changed,"table_post_mutation",held);return;}bindings.compute_root_table(ref(*held),index,gpu.ptr);noted(*held);}
}
void STDMETHODCALLTYPE dispatch(ID3D12GraphicsCommandList* list,UINT x,UINT y,UINT z){
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());Scope scope;
    Resolution resolved(list);Retirement retirement;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);census(CallerOperation::dispatch,caller,scope.world());EvidenceScope evidence("dispatch",list);
    if(target_observing(scope.world()))note_dispatch_identity(resolved);
    const bool observe=observing(scope.world()),scoped=observe&&scoped_list(resolved,CallerOperation::dispatch,caller);Depth call(resolved.held||scoped);
    if(!observe)invalidate_held(resolved.held,0);
    Row* row{};List* held{};ListUse use;
    ExactDispatchCall probe(resolved,caller,x,y,z,scope.world());
    if(call.outer&&observe&&ngx_mv_target_capture::window.snapshot().phase==gears_ngx_mv_target::Phase::capturing){
        ++dispatches;if(scoped)held=track(list,retirement,resolved,CallerOperation::dispatch,caller);use.attach(held);
        if(held){const auto candidate=bindings.dispatch(ref(*held));
            if(!candidate.binding_observed)++unbound;
            else {auto observed=ngx_mv_descriptor_capture::lookup_target(candidate.shader_visible_heap,candidate.root3_gpu);
                if(observed.overflow)lose(descriptor_capacity,"dispatch_descriptor_capacity",held);
                else if(!observed.ready||(!observed.exact_base_mv&&!observed.count))++unrelated;
                else if(row_count==rows.size())lose(row_capacity,"dispatch_row_capacity",held);
                else {held->history_pinned=true;row=&rows[row_count++];auto const& w=ngx_mv_target_capture::window.snapshot();
                    row->world=w.selection.world;row->ticket=w.consume.ticket;row->renderer=w.consume.renderer;
                    row->raw=reinterpret_cast<uintptr_t>(list);row->canonical=held->canonical;row->generation=held->generation;
                    row->root=candidate.compute_root_signature;row->heap=candidate.shader_visible_heap;row->root3=candidate.root3_gpu;
                    row->caller=caller;row->groups={x,y,z};row->before=observed;
                    row->caller_candidate=ngx_stereo_host::attested&&caller==base+0x1ac833;}
            }
        }
    }
    // No descriptor observer mutex is held across the native Dispatch.
    const auto before=mutation;
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
    const auto trial_identity=ngx_trial_producer::active()?canonical_identity(list):resolved.canonical;
    const auto trial_constants=ngx_mv_constants_capture::snapshot(resolved.raw,trial_identity,frame_link.current(trial_identity).generation);
#endif
    original_dispatch(list,x,y,z);probe.finish();
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
    ngx_trial_producer::motion(list,x,y,z,caller,trial_constants);
#endif
    if(row){row->returned=true;row->after=ngx_mv_descriptor_capture::lookup_target(row->heap,row->root3);
        row->stable=held&&current(*held)&&before==mutation&&gears_ngx_mv_compute::same(row->before,row->after);
        if(!row->stable)lose(changed,"dispatch_post",held);}
}
void STDMETHODCALLTYPE clear(ID3D12GraphicsCommandList* list,ID3D12PipelineState* pso){
    Scope scope;RevocationScope revoke(list,1);std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);EvidenceScope evidence("clear",list);original_clear(list,pso);revoke.finish();
}
void STDMETHODCALLTYPE set(ID3D12GraphicsCommandList* list,ID3D12PipelineState* pso){
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());Scope scope;SetScope witness(list,pso,caller,scope.world());
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);original_set(list,pso);witness.finish();
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
    ngx_trial_producer::pso(list,pso,caller);
#endif
}
void STDMETHODCALLTYPE bundle(ID3D12GraphicsCommandList* list,ID3D12GraphicsCommandList* child){
#if defined(GEARS_NATIVE_OWNED_PIXELS) && GEARS_NATIVE_OWNED_PIXELS
    if(stereo_publish_bypass){original_bundle(list,child);return;}
#endif
    Scope scope;RevocationScope revoke(list,2);std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);EvidenceScope evidence("bundle",list);
#if defined(GEARS_NATIVE_OWNED_PIXELS) && GEARS_NATIVE_OWNED_PIXELS
    owned_scene_gpu::uncertain(owned_scene_gpu::Unknown::bundle);
#endif
    original_bundle(list,child);revoke.finish();
}
bool install_hooks(void** vtable){
    constexpr unsigned first=viewport_hook_base_capacity+ngx_mv_descriptor_hooks;
    viewport_targets[first]=vtable[28];viewport_targets[first+1]=vtable[29];viewport_targets[first+2]=vtable[31];viewport_targets[first+3]=vtable[14];
    bool ok=install(viewport_targets[first],reinterpret_cast<void*>(heaps),reinterpret_cast<void**>(&original_heaps));
    ok=install(viewport_targets[first+1],reinterpret_cast<void*>(root),reinterpret_cast<void**>(&original_root))&&ok;
    ok=install(viewport_targets[first+2],reinterpret_cast<void*>(ngx_mv_compute_capture::table),reinterpret_cast<void**>(&original_table))&&ok;
    ok=install(viewport_targets[first+3],reinterpret_cast<void*>(dispatch),reinterpret_cast<void**>(&original_dispatch))&&ok;
#if !defined(GEARS_DEATH_PSO_CACHE) || !GEARS_DEATH_PSO_CACHE
    static_assert(ngx_mv_compute_hooks==8,"direct PSO and depth CopyResource observers");
    viewport_targets[first+4]=vtable[11];viewport_targets[first+5]=vtable[27];viewport_targets[first+6]=vtable[25];
    ok=install(viewport_targets[first+4],reinterpret_cast<void*>(clear),reinterpret_cast<void**>(&original_clear))&&ok;
    ok=install(viewport_targets[first+5],reinterpret_cast<void*>(bundle),reinterpret_cast<void**>(&original_bundle))&&ok;
    ok=install(viewport_targets[first+6],reinterpret_cast<void*>(set),reinterpret_cast<void**>(&original_set))&&ok;
    viewport_targets[first+7]=vtable[17];
    ok=install(viewport_targets[first+7],reinterpret_cast<void*>(copy_resource),reinterpret_cast<void**>(&original_copy_resource))&&ok;
#else
    static_assert(ngx_mv_compute_hooks==5,"PSO cache owns Set/Clear/Bundle hooks");
    viewport_targets[first+4]=vtable[17];
    ok=install(viewport_targets[first+4],reinterpret_cast<void*>(copy_resource),reinterpret_cast<void**>(&original_copy_resource))&&ok;
#endif
    return ok;
}
void stop(){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);stopped=true;exact_stop(7);}
void release_drained(){
    {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!stopped||active.load()||recording.load()||exact_pending())return;}
    gpu_poll();
#if defined(GEARS_NGX_MV_GPU_COMPLETION) && GEARS_NGX_MV_GPU_COMPLETION
    if(gpu_proof.signalled&&!gpu_proof.complete&&!gpu_proof.removed){
        HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
        if(event){if(SUCCEEDED(gpu_proof.fence->SetEventOnCompletion(1,event)))WaitForSingleObject(event,1000);CloseHandle(event);}
        gpu_poll();
        if(!gpu_proof.complete&&!gpu_proof.removed){
            HMODULE pinned{};GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&gpu_proof),&pinned);
            gpu_proof.resident=true;return; // Preserve queued fence/list owners until normal process teardown.
        }
    }
#endif
    std::array<List,16> retired;
    std::array<ExactOwners,8> retired_history;
    std::array<gears_ngx_stereo::TypedResourceLease,8> retired_depth_sources;
    std::array<SdkRef,4> retired_barriers;std::array<SdkRef,2> retired_evals,retired_copies;
    gears_vr::ComPtr<ID3D12CommandQueue> retired_queue;gears_vr::ComPtr<ID3D12Fence> retired_fence;
    PsoWitness retired_pending,retired_promoted;
    gears_vr::ComPtr<ID3D12GraphicsCommandList> retired_exact_list;gears_vr::ComPtr<IUnknown> retired_exact_identity;
    {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        if(!stopped||active.load()||recording.load()||exact_pending())return;
        for(unsigned i=0;i<list_count;++i)if(lists[i].callback_busy||lists[i].reset_busy)return;
        lists.swap(retired);exact_list_owner.Swap(retired_exact_list);exact_identity_owner.Swap(retired_exact_identity);
        for(unsigned i=0;i<exact_history_count;++i){auto& old=exact_history[i];auto& out=retired_history[i];
            old.list.Swap(out.list);old.identity.Swap(out.identity);out.pso=std::move(old.pso);}
        retired_depth_sources.swap(depth_copy_sources);
        retired_barriers.swap(sdk_barriers);retired_evals.swap(sdk_evals);retired_copies.swap(sdk_copies);
        retired_queue.Swap(gpu_proof.queue);retired_fence.Swap(gpu_proof.fence);
        retired_pending=std::move(pso_pending);retired_promoted=std::move(pso_promoted);leases_released=true;}
    // Local retired owners release after the observation lock is dropped.
}
bool snapshot_picked_pso(gears_vr::ComPtr<ID3D12PipelineState>& out,PsoWitnessKey& key){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(out||leases_released||ngx_mv_target_capture::window.snapshot().phase!=gears_ngx_mv_target::Phase::frozen)return false;
    for(unsigned i=0;i<exact_history_count;++i){auto const& pso=exact_history[i].pso;
        if(pso.historical_stable&&pso.pso&&pso.pso_identity){out=pso.pso;key=pso.key;return true;}}
    if(!pso_promoted.historical_stable||!pso_promoted.pso||!pso_promoted.pso_identity)return false;
    out=pso_promoted.pso;key=pso_promoted.key;return true;
}
struct BlobReceipt {
    PsoWitnessKey key;bool attempted{},completed{},saved{},details_readable{};HRESULT hr=E_PENDING;
    size_t size{};DWORD error{},written{},worker{};wchar_t path[MAX_PATH]{};
} blob_receipt;
std::atomic<DWORD> blob_worker_thread{};
void register_worker(){DWORD empty{};blob_worker_thread.compare_exchange_strong(empty,GetCurrentThreadId());}
HRESULT picked_cached_blob(ID3D12PipelineState* pso,ID3DBlob** blob){
    __try{return pso->GetCachedBlob(blob);}__except(EXCEPTION_EXECUTE_HANDLER){*blob=nullptr;return E_FAIL;}
}
bool picked_blob_data(ID3DBlob* blob,size_t& size,void*& data){
    __try{size=blob->GetBufferSize();data=blob->GetBufferPointer();return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){size=0;data=nullptr;return false;}
}
void poll_picked_blob(){
    const auto worker=blob_worker_thread.load();if(!worker||worker!=GetCurrentThreadId())return;
    // This independent activity outlives every native Release below. Worker
    // calls own no callback/frame lock; self-test/RHI callers are rejected.
    NativeOwnedShutdownRhiScope::Activity activity;
    gpu_poll();
    gears_vr::ComPtr<ID3D12PipelineState> pso;gears_vr::ComPtr<ID3DBlob> blob;PsoWitnessKey key;
    {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        if(blob_receipt.attempted||!snapshot_picked_pso(pso,key))return;
        blob_receipt.attempted=true;blob_receipt.key=key;blob_receipt.worker=worker;}
    BlobReceipt result;result.attempted=true;result.key=key;result.worker=worker;
    ID3DBlob* returned{};result.hr=picked_cached_blob(pso.Get(),&returned);blob.Attach(returned);
    if(SUCCEEDED(result.hr)&&blob){
        void* data{};result.details_readable=picked_blob_data(blob.Get(),result.size,data);
        if(!result.details_readable||!data||!result.size)result.error=ERROR_INVALID_DATA;
        else if(!gears_ngx_mv_compute::pso_blob_save_size(result.size))result.error=ERROR_FILE_TOO_LARGE;
        else {
            wchar_t parent[MAX_PATH]{};const auto length=GetModuleFileNameW(own_module,parent,MAX_PATH);
            auto* slash=length&&length<MAX_PATH?wcsrchr(parent,L'\\'):nullptr;
            if(!slash)result.error=length>=MAX_PATH?ERROR_BUFFER_OVERFLOW:ERROR_PATH_NOT_FOUND;
            else {*slash=0;
                if(_snwprintf_s(result.path,MAX_PATH,_TRUNCATE,L"%s\\ngx-mv-picked-pso-%lu-%llu-%llu-%lld.raw",parent,GetCurrentProcessId(),key.world,key.ticket,now())<0)result.error=ERROR_BUFFER_OVERFLOW;
                else {
                    HANDLE file=CreateFileW(result.path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
                    if(file==INVALID_HANDLE_VALUE)result.error=GetLastError();
                    else {
                        if(!WriteFile(file,data,static_cast<DWORD>(result.size),&result.written,nullptr))result.error=GetLastError();
                        else if(result.written!=result.size)result.error=ERROR_WRITE_FAULT;
                        else if(!FlushFileBuffers(file))result.error=GetLastError();
                        else result.saved=true;
                        if(!CloseHandle(file)){result.saved=false;if(!result.error)result.error=GetLastError();}
                    }
                }
            }
        }
    }else if(SUCCEEDED(result.hr))result.error=ERROR_INVALID_DATA;
    // Explicitly finish all COM ownership outside locks and before publishing
    // completion. The activity guard remains alive through both Releases.
    blob.Reset();pso.Reset();result.completed=true;
    {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);blob_receipt=result;}
}
void print(FILE* file){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!file)return;
    ngx_mv_constants_capture::print(file);
    ngx_depth_capture::print(file);
    fprintf(file,"ngx_selected_gpu attempted=%u signalled=%u complete=%u device_removed=%u resident_pending=%u failure=%u result=%08lx completed_value=%llu required_value=1 queue=%llx signal_order=%llu signals_limit=1 actual_submission_membership=1 intermediate_dependencies_verified=0 pixels_verified=0 playable_dlss_verified=0\n",
        unsigned(gpu_proof.attempted),unsigned(gpu_proof.signalled),unsigned(gpu_proof.complete),unsigned(gpu_proof.removed),unsigned(gpu_proof.resident),gpu_proof.failure,static_cast<unsigned long>(gpu_proof.result),gpu_proof.value,gpu_proof.queue_id,gpu_proof.order);
    for(unsigned i=0;i<2;++i){auto const& eval=sdk_evals[i];auto const& copy=sdk_copies[i];fprintf(file,"ngx_selected_gpu_eye eye=%u eval=%llx,%llx,%llu eval_executed=%u eval_queue=%llx eval_order=%llu copy=%llx,%llx,%llu copy_executed=%u copy_queue=%llx copy_order=%llu\n",
        i,eval.raw,eval.canonical,eval.generation,unsigned(eval.executed),eval.queue,eval.order,copy.raw,copy.canonical,copy.generation,unsigned(copy.executed),copy.queue,copy.order);}
    fprintf(file,"ngx_selected_gpu_extra_copies before_evals=%llu same_recording_repeats=%llu\n",sdk_copy_before_evals,sdk_copy_repeats);
    fprintf(file,"ngx_selected_raster seeded=%u seed_attempts=%u seed_failure=%u rows=%u limit=64 binding_overflow=%llu retired_bindings=%llu row_overflow=%llu unknown_rectangles=%llu color=%llx depth=%llx mv=%llx output=%llx viewport_scissor_intersection_only=1 draw_return_verified=0 shader_write_verified=0 gpu_verified=0\n",
        unsigned(raster_seeded),raster_seed_attempts,raster_seed_failure,raster_count,raster_binding_overflow,raster_retired_bindings,raster_overflow,raster_unknown_rectangles,raster_inputs[0].identity,raster_inputs[1].identity,raster_inputs[2].identity,raster_inputs[3].identity);
    for(unsigned i=0;i<raster_count;++i){auto const& row=raster_rows[i];fprintf(file,"ngx_selected_raster_row row=%u input=%u resource=%llx list=%llx generation=%llu calls=%llu caller=%llx clip=%u,%u,%u,%u contains_views=%u eval_count_first=%u eval_count_last=%u descriptor_write_allowed=%u cpu_binding_candidate=1 gpu_verified=0\n",
        i,row.input,row.resource,row.list,row.generation,row.count,row.caller,row.clip.x,row.clip.y,row.clip.width,row.clip.height,row.view_mask,row.first_eval,row.last_eval,unsigned(row.descriptor_writable));
        fprintf(file,"ngx_selected_raster_submission row=%u executed=%u queue=%llx order=%llu\n",i,unsigned(row.executed),row.queue,row.execute_order);}
    fprintf(file,"ngx_selected_depth_mismatches rows=%u limit=8 overflow=%llu\n",depth_mismatch_count,depth_mismatch_overflow);
    for(unsigned i=0;i<depth_mismatch_count;++i){auto const& row=depth_mismatches[i];fprintf(file,"ngx_selected_depth_mismatch row=%u handle=%llx resource=%llx list=%llx size=%llu,%u format=%u selected_depth=%llx scalar_provenance_only=1\n",i,row.handle,row.resource,row.list,row.width,row.height,row.format,raster_inputs[1].identity);}
    fprintf(file,"ngx_selected_depth_copies rows=%u limit=8 overflow=%llu shader_depth_semantics_verified=0 gpu_execution_verified=0\n",depth_copy_count,depth_copy_overflow);
    for(unsigned i=0;i<depth_copy_count;++i){auto const& row=depth_copies[i];fprintf(file,"ngx_selected_depth_copy row=%u kind=%u source=%llx destination=%llx list=%llx generation=%llu caller=%llx subresources=%u,%u returned=%u raster_source_views=%u calls=%llu eval_count=%u execute_returned=%u queue=%llx execute_order=%llu full_extent=1 gpu_execution_verified=0\n",
        i,row.kind,row.source,row.destination,row.list,row.generation,row.caller,row.source_subresource,row.destination_subresource,unsigned(row.returned),row.view_mask,row.calls,row.eval_count,unsigned(row.executed),row.queue,row.execute_order);}
    fprintf(file,"ngx_mv_exact_history slots=%u limit=8 dispatch_inflight=%u owners_released=%u rearm_scope=same_selected_consume_window mutation_during_setter_or_dispatch_rejected=1 gpu_execution_verified=0\n",
        exact_history_count,exact_inflight,unsigned(leases_released));
    fprintf(file,"ngx_mv_compute enabled=1 lists=%u list_limit=16 rows=%u row_limit=32 failed=%u failure=%u core_loss=%u stopped=%u leases_released=%u dispatches=%llu unbound=%llu unrelated=%llu invalidations=%llu waiting_bindings=%llu capture_bindings=%llu row_scope=selected_mv_related table_membership_verified=0 gpu_execution_verified=0 producer_authorized=0\n",
        list_count,row_count,unsigned(failed),failure,unsigned(bindings.loss()),unsigned(stopped),unsigned(leases_released),dispatches,unbound,unrelated,invalidations,waiting_bindings,capture_bindings);
    fprintf(file,"ngx_mv_compute_admission scope=new_admission_setup22018d incomplete_coverage=1 new_phase=waiting_or_capturing attestation_required=1 known_lookup=typed_raw_then_canonical_qi skip_scope=bank_pending_owned skipped_unknown_heaps=%llu skipped_unknown_root=%llu skipped_unknown_table=%llu skipped_unknown_dispatch=%llu selected_mv_inferred=0\n",
        unknown_skipped[unsigned(CallerOperation::heaps)],unknown_skipped[unsigned(CallerOperation::root)],unknown_skipped[unsigned(CallerOperation::table)],unknown_skipped[unsigned(CallerOperation::dispatch)]);
    fprintf(file,"ngx_mv_compute_resolution raw_matches=%llu alias_matches=%llu unmatched=%llu qi_failures=%llu reentrant_rejections=%llu scope=bank_or_exact_probe_pending temporary_release_after_local_frame_lock=1 alias_bank=0\n",
        resolved_raw,resolved_alias,resolved_unmatched,resolve_qi_failed,resolve_reentrant);
    auto const& picked=pso_promoted.key;
    fprintf(file,"ngx_mv_pso_witness sets=%llu candidates=%llu promotions=%llu discards=%llu qi_failures=%llu last_discard=%u serial=%llu pending_valid=%u pending_retained=%u promoted_current_valid=%u promoted_retained=%u historical_stable_dispatch=%u canonical=%llx pso=%llx generation=%llu world=%llu ticket=%llu renderer=%llx witness_serial=%llu scope=attested_set17b514_single_pending_single_promoted blob_calls=%u gpu_execution_verified=0 producer_authorized=0\n",
        pso_sets,pso_candidates,pso_promotions,pso_discards,pso_qi_failures,pso_last_discard,pso_serial,unsigned(pso_pending.valid),unsigned(bool(pso_pending.pso)),unsigned(pso_promoted.valid),unsigned(bool(pso_promoted.pso)),unsigned(pso_promoted.historical_stable),picked.canonical,picked.pso,picked.generation,picked.world,picked.ticket,picked.renderer,picked.serial,unsigned(blob_receipt.attempted));
    auto const& b=blob_receipt;auto const& k=b.key;
    auto const& rk=root_promoted.key;
    fprintf(file,"ngx_mv_root_witness sets=%llu candidates=%llu promotions=%llu unknown=%llu rejected=%llu serial=%llu pending_valid=%u current_valid=%u historical_stable_dispatch=%u canonical=%llx root=%llx device=%llx generation=%llu world=%llu ticket=%llu renderer=%llx witness_serial=%llu provenance_row=%u serialized_size=%zu scope=typed_set138124_or22018d_exact_mv_dispatch gpu_execution_verified=0\n",
        root_sets,root_candidates,root_promotions,root_unknown,root_rejected,root_serial,unsigned(root_pending.valid),unsigned(root_promoted.valid),unsigned(root_promoted.historical_stable),
        rk.canonical,rk.root,rk.device,rk.generation,rk.world,rk.ticket,rk.renderer,rk.serial,rk.provenance,rk.size);
    for(unsigned i=0;i<exact_count;++i){auto const& r=exact_rows[i];auto const& key=r.root;
        fprintf(file,"ngx_mv_exact_root row=%u stable=%u canonical=%llx root=%llx device=%llx generation=%llu world=%llu ticket=%llu renderer=%llx witness_serial=%llu provenance_row=%u serialized_size=%zu\n",
            i,unsigned(r.root_stable),key.canonical,key.root,key.device,key.generation,key.world,key.ticket,key.renderer,key.serial,key.provenance,key.size);}
    for(unsigned i=0;i<exact_count;++i){auto const& r=exact_rows[i];auto const& c=r.constants;auto const& u=c.upload;
        fprintf(file,"ngx_mv_exact_constants row=%u valid=%u stable=%u context_view_index=%u logical_eye_verified=0 context=%llx view=%llx bank=%llx meta=%llx source=%llx mapped=%llx ring=%llx resource=%llx cbv_cpu=%llx cbv_gpu=%llx table_gpu=%llx offset=%u used=%u bytes=160 source_rect=%.9g,%.9g,%.9g,%.9g target_rect=%d,%d,%d,%d values=",
            i,unsigned(c.valid),unsigned(r.constants_stable),c.context_index,u.context,c.view,u.bank,u.meta,u.data,u.mapped,u.ring,u.resource,u.cpu,u.gpu,c.gpu,u.offset,u.used,
            c.source_rectangle[0],c.source_rectangle[1],c.source_rectangle[2],c.source_rectangle[3],c.target_rectangle[0],c.target_rectangle[1],c.target_rectangle[2],c.target_rectangle[3]);
        for(auto byte:u.upload)fprintf(file,"%02x",unsigned(byte));fprintf(file," cpu_upload_only=1 gpu_execution_verified=0\n");}
    fprintf(file,"ngx_mv_pso_blob attempted=%u completed=%u saved=%u hr=%08lx size=%zu limit=4194304 written=%lu error=%lu details_readable=%u worker=%lu canonical=%llx pso=%llx generation=%llu world=%llu ticket=%llu renderer=%llx witness_serial=%llu path=%ls attempts_limit=1 create_new=1 worker_only=1 internal_allocation_bounded=0 interpretation=opaque_cache_only shader_identity_verified=0 gpu_execution_verified=0 producer_authorized=0\n",
        unsigned(b.attempted),unsigned(b.completed),unsigned(b.saved),static_cast<unsigned long>(b.hr),b.size,b.written,b.error,unsigned(b.details_readable),b.worker,k.canonical,k.pso,k.generation,k.world,k.ticket,k.renderer,k.serial,b.path);
    fprintf(file,"ngx_mv_exact_probe tables=%llu misses=%llu dispatches=%llu rows=%u limit=8 overflow_calls=%llu active=%u arming=%u stopped=%u reason=%u serial=%llu typed_list_retained=%u identity_retained=%u scope=owned_capturing_attested_table17b99a_dispatch1ac833 independent_of_binding_bank=1 heap_observed=0 root_witness_separate=1 full_bindings_verified=0 table_extent_verified=0 gpu_execution_verified=0 producer_authorized=0 reason_identity=1 reason_table_overwrite=2 reason_generation=3 reason_changed=4 reason_unwind=5 reason_capacity=6 reason_window_end=7 reason_reset_clear_bundle=8 reason_heap_root=9\n",
        exact_tables,exact_misses,exact_dispatches,exact_count,exact_overflow,unsigned(exact_association.active),unsigned(exact_arming),unsigned(exact_stopped),exact_reason,exact_serial,unsigned(bool(exact_list_owner)),unsigned(bool(exact_identity_owner)));
    for(unsigned i=0;i<exact_count;++i){auto const& r=exact_rows[i];auto const& a=r.association;auto const& d=a.descriptor;
        fprintf(file,"ngx_mv_exact_dispatch row=%u world=%llu ticket=%llu renderer=%llx canonical=%llx generation=%llu serial=%llu heap=%llx resource=%llx gpu=%llx version=%llu origin=%llu source_handle=%llx source_version=%llu after_match=%u after_version=%llu after_origin=%llu caller=%llx groups=%u,%u,%u returned=%u stable=%u pso=%llx pso_serial=%llu pso_stable=%u heap_observed=0 root_observed=%u full_bindings_verified=0 gpu_execution_verified=0 producer_authorized=0\n",
            i,a.world,a.ticket,a.renderer,a.canonical,a.generation,a.serial,d.heap,d.resource,d.gpu,d.version,d.origin,d.source_handle,d.source_version,unsigned(r.after.match),r.after.version,r.after.origin,r.caller,r.groups[0],r.groups[1],r.groups[2],unsigned(r.returned),unsigned(r.stable),r.pso,r.pso_serial,unsigned(r.pso_stable),unsigned(r.root_stable));}
    fprintf(file,"ngx_mv_compute_dispatch_identities rows=%u limit=8 overflow_calls=%llu kind_not_attempted=0 kind_raw=1 kind_alias=2 kind_unmatched=3 kind_qi_failed=4\n",dispatch_identity_count,dispatch_identity_overflow);
    for(unsigned i=0;i<dispatch_identity_count;++i){auto const& d=dispatch_identities[i];
        fprintf(file,"ngx_mv_compute_dispatch_identity row=%u raw=%llx canonical=%llx retained_raw=%llx stored_generation=%llu current_identity=%llx current_generation=%llu kind=%u phase=%u calls=%llu scalar_only=1\n",
            i,d.raw,d.canonical,d.retained_raw,d.stored_generation,d.current_identity,d.current_generation,d.kind,d.phase,d.calls);}
    auto const& o=capacity_occupancy;
    fprintf(file,"ngx_mv_compute_reuse attempts=%llu reused=%llu foreign_device_skips=%llu capacity_snapshot=%u occupied=%u eligible=%u partial_or_absent=%u pinned=%u busy=%u unrevoked=%u unknown_blocked=%u stale_blocked=%u blocker_counts_overlap=1\n",
        reuse_attempts,reused,foreign_device_skips,unsigned(reuse_attempts!=0),o.occupied,o.eligible,o.partial,o.pinned,o.busy,o.unrevoked,o.unknown_blocked,o.stale_blocked);
    auto const& f=first_failure;
    fprintf(file,"ngx_mv_compute_first_failure present=%u reason=%u site=%s operation=%s raw=%llx canonical=%llx stored_generation=%llu current_identity=%llx current_generation=%llu prior_stored_known=%u prior_stored_generation=%llu result=%08lx bypass=%u owned_known=%u owned=%u ticket=%llu world=%llu frozen_ticket=%llu frozen_world=%llu renderer=%llx phase=%u core_loss_at_failure=%u depth=%u lists=%u rows=%u mutation=%llu\n",
        unsigned(f.present),f.reason,f.site,f.operation,f.raw,f.canonical,f.stored_generation,f.current_identity,f.current_generation,
        unsigned(f.prior_stored_known),f.prior_stored_generation,static_cast<unsigned long>(f.result),unsigned(f.bypass),unsigned(f.owned_known),unsigned(f.owned),
        f.ticket,f.world,f.frozen_ticket,f.frozen_world,f.renderer,f.window_phase,f.core_loss,f.depth_at_failure,f.lists_at_failure,f.rows_at_failure,f.mutation_at_failure);
    auto const& fo=f.occupancy;
    fprintf(file,"ngx_mv_compute_first_failure_occupancy present=%u occupied=%u eligible=%u partial_or_absent=%u pinned=%u busy=%u unrevoked=%u unknown_blocked=%u stale_blocked=%u blocker_counts_overlap=1\n",
        unsigned(f.present),fo.occupied,fo.eligible,fo.partial,fo.pinned,fo.busy,fo.unrevoked,fo.unknown_blocked,fo.stale_blocked);
    fprintf(file,"ngx_mv_compute_callers rows=%u limit=32 calls=%llu overflow_calls=%llu module_base=%llx module_size=%u scalar_only=1 scope=target_pending_owned_consume bank_failure_independent=1 table_membership_verified=0 gpu_execution_verified=0 producer_authorized=0\n",
        caller_row_count,caller_calls,caller_overflow_calls,base,caller_image_size);
    constexpr char const* operations[]={"heaps","root","table","dispatch"};
    for(unsigned i=0;i<caller_row_count;++i){auto const& c=caller_rows[i];
        fprintf(file,"ngx_mv_compute_caller row=%u operation=%s phase=%u caller=%llx rva=%llx rva_valid=%u calls=%llu bank_failed_calls=%llu first_ticket=%llu last_ticket=%llu historical_caller_candidate=%u semantics_verified=0\n",
            i,operations[unsigned(c.operation)],c.phase,c.caller,c.rva,unsigned(c.in_game_image),c.calls,c.bank_failed_calls,c.first_ticket,c.last_ticket,unsigned(c.historical_candidate));}
    for(unsigned i=0;i<row_count;++i){auto const& r=rows[i];
        fprintf(file,"ngx_mv_compute_dispatch row=%u world=%llu ticket=%llu renderer=%llx list_raw=%llx list=%llx generation=%llu root=%llx heap=%llx root3=%llx groups=%u,%u,%u caller=%llx historical_caller_candidate=%u returned=%u stable=%u exact_base_mv=%u candidates=%u table_membership_verified=0 gpu_execution_verified=0 producer_authorized=0\n",
            i,r.world,r.ticket,r.renderer,r.raw,r.canonical,r.generation,r.root,r.heap,r.root3,r.groups[0],r.groups[1],r.groups[2],r.caller,unsigned(r.caller_candidate),unsigned(r.returned),unsigned(r.stable),unsigned(r.before.exact_base_mv),r.before.count);
        for(unsigned stage=0;stage<2;++stage){auto const& lookup=stage?r.after:r.before;
            fprintf(file,"ngx_mv_compute_lookup row=%u stage=%u ready=%u exact_base_mv=%u count=%u overflow=%u\n",i,stage,unsigned(lookup.ready),unsigned(lookup.exact_base_mv),lookup.count,unsigned(lookup.overflow));
            for(unsigned j=0;j<lookup.count;++j){auto const& c=lookup.candidates[j];
                fprintf(file,"ngx_mv_compute_candidate row=%u stage=%u candidate=%u cpu=%llx gpu=%llx version=%llu origin=%llu source=%llx source_version=%llu offset=%llu offset_only=1 table_membership_verified=0\n",i,stage,j,c.cpu,c.gpu,c.version,c.origin,c.source_handle,c.source_version,c.offset);}}
    }
}
#else
void raster_targets(ID3D12GraphicsCommandList*,D3D12_CPU_DESCRIPTOR_HANDLE const*){}
void raster_draw(ID3D12GraphicsCommandList*,uintptr_t,unsigned,unsigned){}
void texture_copy(ID3D12GraphicsCommandList*,D3D12_TEXTURE_COPY_LOCATION const*,UINT,UINT,UINT,D3D12_TEXTURE_COPY_LOCATION const*,D3D12_BOX const*,uintptr_t){}
struct ResetScope {explicit ResetScope(ID3D12GraphicsCommandList*){}void finish(HRESULT,gears_frame_link::ListRef={}){}};
struct RevocationScope {RevocationScope(ID3D12GraphicsCommandList*,unsigned){}void finish(){}};
struct SetScope {SetScope(ID3D12GraphicsCommandList*,ID3D12PipelineState*,uintptr_t,bool){}void finish(){}};
void register_worker(){}void poll_picked_blob(){}
void stop(){}void release_drained(){}void print(FILE*){}
bool install_hooks(void**){return true;}
void sdk_barrier(ID3D12GraphicsCommandList*,uintptr_t){}void sdk_eval(uintptr_t,unsigned){}
void executed(ID3D12CommandQueue*,UINT,ExecuteList const*,bool){}
#endif
}
