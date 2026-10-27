// Private, CPU-only early descriptor evidence. Included inside capture.cpp's
// namespace after Scope/frame_lock. The first observed typed heap selects a
// candidate device, never an attested game device or an NGX epoch/ticket.
namespace ngx_mv_descriptor_capture {
#if defined(GEARS_NGX_MV_DESCRIPTOR_OBSERVER) && GEARS_NGX_MV_DESCRIPTOR_OBSERVER
using Bank=gears_ngx_mv::ObserverDescriptorBank<>;
using SourceBank=gears_ngx_mv::SourceDescriptorLedger<>;
using TargetBank=gears_ngx_mv::SelectedMvDescriptorBank<>;
using Ranges=Bank::CopyRanges;
static_assert(Bank::copy_capacity==SourceBank::copy_capacity,"one guarded copy format for independently admitted observers");
static_assert(Bank::copy_capacity==TargetBank::copy_capacity,"selected target uses the same guarded copy format");
using HeapFn=HRESULT(STDMETHODCALLTYPE*)(ID3D12Device*,const D3D12_DESCRIPTOR_HEAP_DESC*,REFIID,void**);
using CbvFn=void(STDMETHODCALLTYPE*)(ID3D12Device*,const D3D12_CONSTANT_BUFFER_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
using SrvFn=void(STDMETHODCALLTYPE*)(ID3D12Device*,ID3D12Resource*,const D3D12_SHADER_RESOURCE_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
using UavFn=void(STDMETHODCALLTYPE*)(ID3D12Device*,ID3D12Resource*,ID3D12Resource*,const D3D12_UNORDERED_ACCESS_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
HeapFn original_heap{};CbvFn original_cbv{};SrvFn original_srv{};UavFn original_uav{};
std::recursive_mutex mutex;
std::unique_ptr<Bank> bank;
std::unique_ptr<SourceBank> source_bank;
std::unique_ptr<TargetBank> target_bank;
gears_ngx_stereo::TypedResourceLease target_mv;
gears_ngx_mv_census::Census heap_census;
gears_ngx_mv::Scope lifetime{};
gears_ngx_mv::Scope source_lifetime{};
gears_ngx_mv::Scope target_lifetime{};
bool armed{},accepting{},failed{},released{};
bool source_failed{};
bool source_frozen{},target_attempted{},target_seeded{},target_failed{},target_stopped{};
unsigned final_loss{};size_t final_heaps{},final_cells{};
Bank::CellCounts final_cell_counts{};Bank::Saturation final_saturation{};
unsigned source_final_loss{};size_t source_final_heaps{},source_final_cells{};
SourceBank::CellCounts source_final_counts{};SourceBank::Saturation source_final_saturation{};
unsigned target_final_loss{};size_t target_final_heaps{},target_final_cells{},target_seed_count{};
TargetBank::CellCounts target_final_counts{};TargetBank::Saturation target_final_saturation{};
uint64_t heap_calls{},uav_calls{},invalidations{},copies{},other_devices{},missing_birth{},recursions{},uncertainties{};
uint64_t source_heaps{},source_uavs{},source_invalidations{},source_copies{},source_other_devices{},source_missing_birth{},source_uncertainties{};
uint64_t target_heaps{},target_uavs{},target_invalidations{},target_copies{},target_other_devices{},target_unrelated_uavs{},target_uncertainties{};
thread_local unsigned depth{};
// Frozen scalar audit only; no extra COM owners survive the bank's existing
// drained teardown. Identity is historical at accepted Eval, not later use.
struct MvMatch {uintptr_t heap{};uint64_t cpu{},gpu{},version{},origin{},source_handle{},source_version{};};
struct MvJoin {
    gears_ngx_mv_census::Use use{};uintptr_t raw{},canonical{},device{};uint64_t capture_generation{};
    size_t matches{},saved{};bool present{},typed_retained{},bank_ready{},device_match{};
    std::array<MvMatch,8> rows{};
};
std::array<MvJoin,2> mv_joins{};
struct SourceMatch {
    gears_ngx_mv::Scope scope{};MvMatch descriptor{};uintptr_t heap_raw{},resource{},counter_raw{},counter{},counter_device{};
    bool visible{};gears_ngx_mv::ViewDescription view{};
};
struct SourceJoin {MvJoin summary{};std::array<SourceMatch,8> rows{};bool legacy_failed_at_eval{};};
SourceJoin source_join{}; // First accepted Eval only; scalar historical audit.
bool legacy_live(){return accepting&&!failed&&(!bank||!bank->failed());}
bool source_live(){return accepting&&!source_frozen&&!source_failed&&(!source_bank||!source_bank->failed());}
bool target_live(){return accepting&&!target_stopped&&target_seeded&&!target_failed&&target_bank&&!target_bank->failed();}
void fail(){if(!legacy_live())return;failed=true;++uncertainties;if(bank)bank->uncertain();}
void source_fail(){if(!source_live())return;source_failed=true;++source_uncertainties;if(source_bank)source_bank->uncertain();}
void target_fail(){if(!accepting||target_failed)return;target_failed=true;++target_uncertainties;if(target_bank&&!target_bank->failed())target_bank->uncertain();}
// Future window end/timeout may freeze admission without detaching any lease.
// Only existing close/drain retirement below releases native ownership.
void stop_target(){std::lock_guard<std::recursive_mutex> lock(mutex);target_stopped=true;}
// Copy only an already seeded retained identity. The scheduler calls this
// under frame_hook_mutex, then validates Create after this lock is released.
// This API never invokes host/frame callbacks while owning descriptor mutex.
bool snapshot_target_seed(uint64_t& observation,gears_ngx_mv_census::Use& use,gears_ngx_stereo::TypedResourceLease& mv){
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if(!target_live()||!target_mv.retained()||!source_join.summary.present)return false;
    observation=target_lifetime.epoch;use=source_join.summary.use;mv=target_mv;return true;
}
#if GEARS_NGX_MV_TARGET_CAPTURE
ExactTarget lookup_exact_target(uint64_t gpu){
    std::lock_guard<std::recursive_mutex> lock(mutex);ExactTarget out;
    if(!target_live()||!target_mv.retained()||!gpu)return out;
    out.ready=true;const auto exact=target_bank->lookup(target_lifetime,gpu,true);
    if(!exact.heap.visible||exact.gpu!=gpu||!exact.same_mv(target_mv))return out;
    out.match=true;out.heap=exact.heap.canonical;out.resource=exact.resource.canonical;out.gpu=exact.gpu;
    out.version=exact.version;out.origin=exact.origin;out.source_handle=exact.source_handle;out.source_version=exact.source_version;return out;
}
gears_ngx_mv_compute::DescriptorLookup lookup_target(uintptr_t heap,uint64_t gpu){
    std::lock_guard<std::recursive_mutex> lock(mutex);gears_ngx_mv_compute::DescriptorLookup out;
    if(!target_live()||!target_mv.retained()||!heap||!gpu)return out;
    out.ready=true;
    const auto exact=target_bank->lookup(target_lifetime,gpu,true);
    out.exact_base_mv=exact.heap.canonical==heap&&exact.same_mv(target_mv);
    target_bank->visit_mv(target_lifetime,target_mv,[&](TargetBank::Snapshot const& s){
        if(s.heap.canonical!=heap||!s.heap.visible||!s.heap.stride||gpu<s.heap.gpu||s.gpu<gpu)return;
        const auto base_index=(gpu-s.heap.gpu)/s.heap.stride;
        if((gpu-s.heap.gpu)%s.heap.stride||base_index>=s.heap.count||(s.gpu-gpu)%s.heap.stride)return;
        if(out.count==out.candidates.size()){out.overflow=true;return;}
        out.candidates[out.count++]={s.cpu,s.gpu,s.version,s.origin,s.source_handle,s.source_version,(s.gpu-gpu)/s.heap.stride};
    });return out;
}
#endif
template<class F> void attempt(F&& fn){try{fn();}catch(...){fail();}}
template<class F> void source_attempt(F&& fn){try{fn();}catch(...){source_fail();}}
template<class F> void target_attempt(F&& fn){try{fn();}catch(...){target_fail();}}
struct Call {
    std::unique_lock<std::recursive_mutex> lock{mutex,std::defer_lock};bool entered{},admitted{},source_admitted{},target_admitted{},census_admitted{};
    explicit Call(bool relevant=true){if(!relevant)return;lock.lock();if(!accepting)return;
        entered=true;if(depth++){++recursions;fail();source_fail();if(target_live())target_fail();return;}
        census_admitted=true;admitted=legacy_live();source_admitted=source_live();target_admitted=target_live();}
    ~Call(){if(entered)--depth;}
    bool legacy()const{return admitted&&legacy_live();}
    bool source()const{return source_admitted&&source_live();}
    bool target()const{return target_admitted&&target_live();}
    explicit operator bool()const{return legacy()||source()||target();}
};
bool selected(ID3D12Device* device){
    if(!bank)return false;const auto identity=canonical_identity(device);
    if(!identity){fail();return false;}
    if(identity!=lifetime.device){++other_devices;return false;}return true;
}
bool source_selected(ID3D12Device* device){
    if(!source_bank)return false;const auto identity=canonical_identity(device);
    if(!identity){source_fail();return false;}
    if(identity!=source_lifetime.device){++source_other_devices;return false;}return true;
}
bool target_selected(ID3D12Device* device){
    const auto identity=canonical_identity(device);if(!identity){target_fail();return false;}
    if(identity!=target_lifetime.device){++target_other_devices;return false;}return true;
}
void seed_target(gears_ngx_stereo::TypedResourceLease const* mv){
    if(target_attempted||target_stopped)return;target_attempted=true;
    if(!source_live()||!source_bank||!mv||!mv->retained()||mv->device!=source_lifetime.device||!source_join.summary.matches)return;
    target_attempt([&]{
        // Independent CPU observation generation, not a future consume ticket.
        target_lifetime=gears_ngx_mv::lifetime_scope(2,mv->device);target_mv=*mv;target_bank=std::make_unique<TargetBank>();
        if(!target_bank->begin(target_lifetime))return;
        if(!source_bank->visit_heap_leases(source_lifetime,[&](gears_ngx_mv::HeapLease const& lease){return target_bank->observe_heap_lease(target_lifetime,lease);})){
            target_fail();return;}
        bool complete=true;
        const auto matches=source_bank->visit_mv(source_lifetime,*mv,[&](SourceBank::Snapshot const& snapshot){
            if(complete){complete=target_bank->seed_current_mv(target_lifetime,*source_bank,snapshot,*mv);if(complete)++target_seed_count;}});
        if(!complete||!matches||matches!=target_seed_count||matches!=source_join.summary.matches||target_bank->failed()){target_fail();return;}
        target_seeded=true;source_frozen=true;
    });
}
void target_uav(ID3D12Resource* resource,ID3D12Resource* counter,gears_ngx_mv::ViewDescription const& view,uint64_t cpu){
    if(!view.readable){target_fail();return;}
    // Exact equality with the retained typed interface needs no new probe.
    // Distinct interface addresses may alias the same COM resource, so use only
    // canonical QI for that fallback; unrelated UAVs never take a typed lease.
    const auto identity=!resource?uintptr_t{}:reinterpret_cast<uintptr_t>(resource)==target_mv.raw?target_mv.canonical:canonical_identity(resource);
    if(resource&&!identity){target_fail();return;}
    if(resource&&identity==target_mv.canonical)
        target_bank->write_uav(target_lifetime,cpu,resource,counter,view);
    else{++target_unrelated_uavs;target_bank->invalidate_if_tracked(target_lifetime,cpu);}
}
Ranges read_ranges(UINT count,const D3D12_CPU_DESCRIPTOR_HANDLE* starts,const UINT* sizes){
    Ranges out;out.count=count;out.sizes_present=sizes!=nullptr;
    if(count>out.starts.size()){out.readable=false;return out;}
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE,Bank::copy_capacity> handles{};
    if(count&&(!safe_copy(handles.data(),starts,count*sizeof(handles[0]))||
       (sizes&&!safe_copy(out.sizes.data(),sizes,count*sizeof(out.sizes[0]))))){out.readable=false;return out;}
    for(unsigned i=0;i<count;++i)out.starts[i]=handles[i].ptr;return out;
}
void copied(ID3D12Device* device,Ranges const& destination,Ranges const& source,uintptr_t caller){
    ngx_mv_constants_capture::copy(caller,destination,source);
    if(legacy_live())attempt([&]{++copies;if(!bank){++missing_birth;fail();return;}
        if(selected(device))bank->copy_sparse(lifetime,destination,source);});
    if(source_live())source_attempt([&]{++source_copies;if(!source_bank){++source_missing_birth;source_fail();return;}
        if(source_selected(device))source_bank->copy_sparse(source_lifetime,destination,source);});
    if(target_live())target_attempt([&]{++target_copies;if(target_selected(device))target_bank->copy_sparse(target_lifetime,destination,source);});
}
void arm(){std::lock_guard<std::recursive_mutex> lock(mutex);if(!armed){armed=true;accepting=true;heap_census.arm();ngx_root_signature_capture::arm();}}
void close(){std::lock_guard<std::recursive_mutex> lock(mutex);accepting=false;heap_census.close();ngx_root_signature_capture::close();}
void accepted_eval(unsigned ordinal,uint64_t revision,uint64_t world,uint64_t ticket,uintptr_t renderer,uintptr_t handle,uintptr_t parameters,
    gears_ngx_stereo::TypedResourceLease const* mv){
    std::lock_guard<std::recursive_mutex> lock(mutex);
    heap_census.accepted(ordinal,{revision,world,ticket,renderer,handle,parameters});
    if(ordinal==0&&!source_join.summary.present){auto& j=source_join.summary;
        source_join.legacy_failed_at_eval=failed||(bank&&bank->failed());
        j.present=true;j.use={revision,world,ticket,renderer,handle,parameters};j.capture_generation=source_lifetime.epoch;
        j.typed_retained=mv&&mv->retained();j.bank_ready=source_live()&&source_bank;
        if(j.typed_retained){j.raw=mv->raw;j.canonical=mv->canonical;j.device=mv->device;
            j.device_match=j.bank_ready&&mv->device==source_lifetime.device;
            if(j.device_match)j.matches=source_bank->visit_mv(source_lifetime,*mv,[&](SourceBank::Snapshot const& snapshot){
                if(j.saved==source_join.rows.size())return;
                auto& row=source_join.rows[j.saved++];row.scope=snapshot.scope;
                row.descriptor={snapshot.heap.canonical,snapshot.cpu,snapshot.gpu,snapshot.version,snapshot.origin,snapshot.source_handle,snapshot.source_version};
                row.heap_raw=snapshot.heap.raw;row.resource=snapshot.resource.raw;row.counter_raw=snapshot.counter.raw;
                row.counter=snapshot.counter.canonical;row.counter_device=snapshot.counter.device;
                row.visible=snapshot.heap.visible;row.view=snapshot.view;
            });}
        seed_target(mv);}
    if(ordinal>=mv_joins.size()||mv_joins[ordinal].present)return;
    auto& join=mv_joins[ordinal];join.present=true;join.use={revision,world,ticket,renderer,handle,parameters};
    join.capture_generation=lifetime.epoch;join.typed_retained=mv&&mv->retained();
    join.bank_ready=accepting&&!failed&&bank&&!bank->failed();
    if(!join.typed_retained)return;
    join.raw=mv->raw;join.canonical=mv->canonical;join.device=mv->device;
    join.device_match=join.bank_ready&&mv->device==lifetime.device;
    if(!join.device_match)return;
    join.matches=bank->visit_mv(lifetime,*mv,[&](Bank::Snapshot const& snapshot){
        if(join.saved==join.rows.size())return;
        join.rows[join.saved++]={snapshot.heap.canonical,snapshot.cpu,snapshot.gpu,snapshot.version,snapshot.origin,snapshot.source_handle,snapshot.source_version};
    });
}
void release_drained(){
    // Closing admission precedes disabling hooks. Taking this lock drains every
    // admitted original/post-call update; active additionally covers Scope.
    std::unique_ptr<Bank> retired;std::unique_ptr<SourceBank> source_retired;std::unique_ptr<TargetBank> target_retired;
    gears_ngx_stereo::TypedResourceLease retired_mv;
    {std::lock_guard<std::recursive_mutex> lock(mutex);
        if(accepting||active.load()||depth)return;
        if(bank){final_loss=static_cast<unsigned>(bank->loss());final_heaps=bank->heaps();final_cells=bank->cells();
            final_cell_counts=bank->cell_counts();final_saturation=bank->saturation();retired=std::move(bank);}
        if(source_bank){source_final_loss=static_cast<unsigned>(source_bank->loss());source_final_heaps=source_bank->heaps();source_final_cells=source_bank->cells();
            source_final_counts=source_bank->cell_counts();source_final_saturation=source_bank->saturation();source_retired=std::move(source_bank);}
        if(target_bank){target_final_loss=static_cast<unsigned>(target_bank->loss());target_final_heaps=target_bank->heaps();target_final_cells=target_bank->cells();
            target_final_counts=target_bank->cell_counts();target_final_saturation=target_bank->saturation();target_retired=std::move(target_bank);}
        retired_mv=std::move(target_mv);
        released=true;}
    // No observer lock across native Release: a closed passthrough callback
    // may still enter the DLL. It cannot access the detached bank.
    retired.reset();source_retired.reset();target_retired.reset();retired_mv.release();
    ngx_root_signature_capture::release_drained();
}
void print_census(FILE* status,char const* stage,unsigned ordinal,gears_ngx_mv_census::Counts const& c,gears_ngx_mv_census::Use use={}){
    fprintf(status,"ngx_mv_heap_census stage=%s ordinal=%u revision=%llu world=%llu ticket=%llu renderer=%llx handle=%llx parameters=%llx all_devices=1 calls=%llu cumulative_creations=%llu visible_creations=%llu visible_descriptors_sum=%llu visible_descriptors_max=%llu nonvisible_creations=%llu nonvisible_descriptors_sum=%llu nonvisible_descriptors_max=%llu api_failures=%llu descriptor_unreadable=%llu output_unreadable=%llu non_creation=%llu other_heap_type=%llu saturated=%u reentrant_excluded_total=%llu cumulative_not_live_heaps=1 descriptors_not_vram_bytes=1 retained_extra_com=0 producer_authorized=0\n",
        stage,ordinal,static_cast<unsigned long long>(use.revision),static_cast<unsigned long long>(use.world),static_cast<unsigned long long>(use.ticket),
        static_cast<unsigned long long>(use.renderer),static_cast<unsigned long long>(use.handle),static_cast<unsigned long long>(use.parameters),
        static_cast<unsigned long long>(c.calls),static_cast<unsigned long long>(c.creations),
        static_cast<unsigned long long>(c.visible.creations),static_cast<unsigned long long>(c.visible.descriptors),static_cast<unsigned long long>(c.visible.max_descriptors),
        static_cast<unsigned long long>(c.nonvisible.creations),static_cast<unsigned long long>(c.nonvisible.descriptors),static_cast<unsigned long long>(c.nonvisible.max_descriptors),
        static_cast<unsigned long long>(c.failed),static_cast<unsigned long long>(c.descriptor_unreadable),static_cast<unsigned long long>(c.output_unreadable),
        static_cast<unsigned long long>(c.non_creation),static_cast<unsigned long long>(c.other_type),c.overflow?1u:0u,static_cast<unsigned long long>(recursions));
}
void print(FILE* status){std::lock_guard<std::recursive_mutex> lock(mutex);if(!status)return;
    fprintf(status,"ngx_mv_descriptors enabled=1 admission=%u scope=device_lifetime capture_generation=%llu device=%p device_selection=first_observed_cbv_heap_candidate device_attested=0 world=0 ticket=0 heaps=%zu cells=%zu heap_capacity=%zu cell_capacity=%zu heap_calls=%llu uav_calls=%llu invalidations=%llu copies=%llu other_devices=%llu missing_birth=%llu recursion=%llu uncertainties=%llu failed=%u loss=%u released=%u selected_pair_proof=0 producer_authorized=0\n",
        accepting?1:0,static_cast<unsigned long long>(lifetime.epoch),reinterpret_cast<void*>(lifetime.device),
        bank?bank->heaps():final_heaps,bank?bank->cells():final_cells,Bank::heap_capacity,Bank::cell_capacity,
        static_cast<unsigned long long>(heap_calls),static_cast<unsigned long long>(uav_calls),
        static_cast<unsigned long long>(invalidations),static_cast<unsigned long long>(copies),
        static_cast<unsigned long long>(other_devices),static_cast<unsigned long long>(missing_birth),
        static_cast<unsigned long long>(recursions),static_cast<unsigned long long>(uncertainties),
        failed||(bank?bank->failed():final_loss!=0)?1:0,bank?static_cast<unsigned>(bank->loss()):final_loss,released?1:0);
    const auto occupancy=bank?bank->cell_counts():final_cell_counts;
    const auto saturation=bank?bank->saturation():final_saturation;
    fprintf(status,"ngx_mv_descriptor_cells known_uav=%zu non_uav=%zu unused=%zu reuses=%llu reuse_non_uav=%u saturation_site=%u saturation_schema=0:none,1:heaps,2:cells,3:ranges,4:serial saturation_heaps=%zu saturation_known_uav=%zu saturation_non_uav=%zu saturation_unused=%zu saturation_reuses=%llu copy_range_capacity=%zu copy_descriptor_capacity=%zu copy_array_scratch_bytes=%zu known_uav_eviction=0 capacity_recovery=0 producer_authorized=0\n",
        occupancy.known_uav,occupancy.non_uav,occupancy.unused,occupancy.reuses,unsigned(Bank::reuse_non_uav),unsigned(saturation.site),saturation.heaps,
        saturation.cells.known_uav,saturation.cells.non_uav,saturation.cells.unused,saturation.cells.reuses,Bank::copy_capacity,Bank::copy_capacity,Bank::copy_scratch_bytes);
    const auto source_counts=source_bank?source_bank->cell_counts():source_final_counts;
    const auto source_saturation=source_bank?source_bank->saturation():source_final_saturation;
    fprintf(status,"ngx_mv_source_ledger enabled=1 independent_of_legacy_bank=1 admission=%u capture_generation=%llu device=%llx scope=device_lifetime world=0 ticket=0 heaps=%zu cells=%zu heap_capacity=%zu cell_capacity=%zu copy_capacity=%zu known_uav=%zu non_uav=%zu unused=%zu heap_calls=%llu uav_calls=%llu invalidations=%llu copies=%llu other_devices=%llu missing_birth=%llu uncertainties=%llu failed=%u loss=%u saturation_site=%u saturation_known_uav=%zu saturation_non_uav=%zu released=%u frozen_after_seed=%u direct_visible_births_retained=1 new_visible_copy_replicas_retained=0 producer_authorized=0\n",
        unsigned(source_live()),source_lifetime.epoch,source_lifetime.device,source_bank?source_bank->heaps():source_final_heaps,source_bank?source_bank->cells():source_final_cells,
        SourceBank::heap_capacity,SourceBank::cell_capacity,SourceBank::copy_capacity,source_counts.known_uav,source_counts.non_uav,source_counts.unused,
        source_heaps,source_uavs,source_invalidations,source_copies,source_other_devices,source_missing_birth,source_uncertainties,
        unsigned(source_failed||(source_bank?source_bank->failed():source_final_loss!=0)),source_bank?unsigned(source_bank->loss()):source_final_loss,
        unsigned(source_saturation.site),source_saturation.cells.known_uav,source_saturation.cells.non_uav,unsigned(released),unsigned(source_frozen));
    const auto target_counts=target_bank?target_bank->cell_counts():target_final_counts;
    const auto target_saturation=target_bank?target_bank->saturation():target_final_saturation;
    fprintf(status,"ngx_mv_target_bank attempted=%u seeded=%u admission=%u stopped=%u independent_of_legacy_bank=1 capture_generation=%llu scope=device_lifetime world=0 ticket=0 device=%llx mv_raw=%llx mv_canonical=%llx seeds=%zu heaps=%zu cells=%zu heap_capacity=%zu cell_capacity=%zu copy_capacity=%zu known_mv=%zu non_uav=%zu unused=%zu reuses=%llu heap_calls=%llu uav_calls=%llu unrelated_uavs=%llu invalidations=%llu copies=%llu other_devices=%llu uncertainties=%llu failed=%u loss=%u saturation_site=%u released=%u consume_window_status=separate_receipt consume_scope_verified=0 collection_until_explicit_stop_or_close=1 dispatch_binding_verified=0 producer_authorized=0\n",
        unsigned(target_attempted),unsigned(target_seeded),unsigned(target_live()),unsigned(target_stopped),target_lifetime.epoch,target_lifetime.device,
        source_join.summary.raw,source_join.summary.canonical,target_seed_count,target_bank?target_bank->heaps():target_final_heaps,target_bank?target_bank->cells():target_final_cells,
        TargetBank::heap_capacity,TargetBank::cell_capacity,TargetBank::copy_capacity,target_counts.known_uav,target_counts.non_uav,target_counts.unused,target_counts.reuses,
        target_heaps,target_uavs,target_unrelated_uavs,target_invalidations,target_copies,target_other_devices,target_uncertainties,
        unsigned(target_failed||(target_bank?target_bank->failed():target_final_loss!=0)),target_bank?unsigned(target_bank->loss()):target_final_loss,unsigned(target_saturation.site),unsigned(released));
    if(source_join.summary.present){auto const& j=source_join.summary;
        fprintf(status,"ngx_mv_source_eval ordinal=0 revision=%llu world=%llu ticket=%llu renderer=%llx handle=%llx parameters=%llx mv_raw=%llx mv_canonical=%llx mv_device=%llx typed_retained=%u ledger_ready=%u device_match=%u legacy_failed_at_eval=%u matches=%zu saved=%zu truncated=%u snapshot_at_eval=1 scalar_only=1 dispatch_binding_verified=0 producer_authorized=0\n",
            j.use.revision,j.use.world,j.use.ticket,j.use.renderer,j.use.handle,j.use.parameters,j.raw,j.canonical,j.device,unsigned(j.typed_retained),unsigned(j.bank_ready),unsigned(j.device_match),unsigned(source_join.legacy_failed_at_eval),j.matches,j.saved,unsigned(j.matches>j.saved));
        for(size_t i=0;i<j.saved;++i){auto const& row=source_join.rows[i];auto const& r=row.descriptor;
            fprintf(status,"ngx_mv_source_match index=%zu creation_scope_kind=%u creation_generation=%llu creation_world=%llu creation_ticket=%llu creation_device=%llx heap_raw=%llx heap=%llx visible=%u cpu=%llu gpu=%llu version=%llu origin=%llu source_handle=%llu source_version=%llu resource_raw=%llx resource_canonical=%llx counter_raw=%llx counter_canonical=%llx counter_device=%llx view_readable=%u view_provided=%u view_bytes=",
                i,unsigned(row.scope.kind),row.scope.epoch,row.scope.world,row.scope.ticket,row.scope.device,row.heap_raw,r.heap,unsigned(row.visible),r.cpu,r.gpu,r.version,r.origin,r.source_handle,r.source_version,row.resource,j.canonical,row.counter_raw,row.counter,row.counter_device,unsigned(row.view.readable),unsigned(row.view.provided));
            auto const* bytes=reinterpret_cast<unsigned char const*>(&row.view.value);
            for(size_t b=0;b<sizeof(row.view.value);++b)fprintf(status,"%02x",unsigned(bytes[b]));
            fprintf(status," snapshot_current_at_eval=1 historical_cpu_provenance=1 producer_authorized=0\n");}}
    for(unsigned i=0;i<heap_census.snapshots().size();++i){auto const& s=heap_census.snapshots()[i];
        if(s.present)print_census(status,"accepted_eval",i,s.counts,s.use);}
    for(unsigned i=0;i<mv_joins.size();++i){auto const& j=mv_joins[i];if(!j.present)continue;
        fprintf(status,"ngx_mv_eval_join ordinal=%u revision=%llu world=%llu ticket=%llu renderer=%llx handle=%llx parameters=%llx capture_generation=%llu mv_raw=%llx mv_canonical=%llx mv_device=%llx typed_retained=%u bank_ready=%u device_match=%u matches=%zu saved=%zu row_capacity=%zu truncated=%u snapshot_at_eval=1 scalar_only=1 descriptor_bound_at_dispatch_verified=0 producer_authorized=0\n",
            i,j.use.revision,j.use.world,j.use.ticket,j.use.renderer,j.use.handle,j.use.parameters,j.capture_generation,j.raw,j.canonical,j.device,
            unsigned(j.typed_retained),unsigned(j.bank_ready),unsigned(j.device_match),j.matches,j.saved,j.rows.size(),unsigned(j.matches>j.saved));
        for(size_t k=0;k<j.saved;++k){auto const& r=j.rows[k];
            fprintf(status,"ngx_mv_eval_descriptor ordinal=%u index=%zu heap=%llx cpu=%llu gpu=%llu version=%llu origin=%llu source_handle=%llu source_version=%llu creation_world=0 creation_ticket=0 historical_cpu_provenance=1 producer_authorized=0\n",
                i,k,r.heap,r.cpu,r.gpu,r.version,r.origin,r.source_handle,r.source_version);}}
    print_census(status,heap_census.active()?"current":"final",2,heap_census.counts());
    ngx_root_signature_capture::print(status);
}
HRESULT STDMETHODCALLTYPE heap(ID3D12Device* device,const D3D12_DESCRIPTOR_HEAP_DESC* desc,REFIID iid,void** output){
    Scope scope;auto frame_guard=frame_lock();Call call;
    D3D12_DESCRIPTOR_HEAP_DESC before{};bool readable=false;
    if(call.census_admitted)readable=safe_copy(&before,desc,sizeof(before));
    const auto result=original_heap(device,desc,iid,output);
    // Scalar census survives a failed provenance bank. Copy only the returned
    // address; no QI/AddRef/GetDesc or retained object is needed for counting.
    IUnknown* raw{};bool output_readable=true;
    if(call.census_admitted){
        const bool eligible=readable&&before.Type==D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        if(eligible&&SUCCEEDED(result)&&output)output_readable=safe_copy(&raw,output,sizeof(raw));
        heap_census.record({readable,eligible,FAILED(result),output_readable,SUCCEEDED(result)&&raw!=nullptr,
            (before.Flags&D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)!=0,before.NumDescriptors});
    }
    if(call){try{
        if(!readable){if(call.legacy())fail();if(call.source())source_fail();if(call.target())target_fail();return result;}
        if(before.Type!=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV||FAILED(result))return result;
        if(call.legacy())++heap_calls;if(call.source())++source_heaps;if(call.target())++target_heaps;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> typed;
        if(!output_readable||!raw||FAILED(raw->QueryInterface(IID_PPV_ARGS(&typed)))){
            if(call.legacy())fail();if(call.source())source_fail();if(call.target())target_fail();return result;}
        if(call.legacy())attempt([&]{if(!bank){const auto identity=canonical_identity(device);if(!identity){fail();return;}
            // One immutable bank: no world-reset clearing or device rebinding.
            lifetime=gears_ngx_mv::lifetime_scope(1,identity);bank=std::make_unique<Bank>();
            if(!bank->begin(lifetime)){fail();return;}}
            if(selected(device))bank->observe_heap(lifetime,typed.Get());});
        if(call.source())source_attempt([&]{if(!source_bank){const auto identity=canonical_identity(device);if(!identity){source_fail();return;}
            source_lifetime=gears_ngx_mv::lifetime_scope(1,identity);source_bank=std::make_unique<SourceBank>();
            if(!source_bank->begin(source_lifetime)){source_fail();return;}}
            if(source_selected(device))source_bank->observe_heap(source_lifetime,typed.Get());});
        if(call.target())target_attempt([&]{if(target_selected(device))target_bank->observe_heap(target_lifetime,typed.Get());});
        }catch(...){if(call.legacy())fail();if(call.source())source_fail();if(call.target())target_fail();}
    }return result;
}
void STDMETHODCALLTYPE cbv(ID3D12Device* device,const D3D12_CONSTANT_BUFFER_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE handle){
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
    Scope scope;auto frame_guard=frame_lock();Call call;original_cbv(device,desc,handle);
    ngx_mv_constants_capture::write(handle.ptr);ngx_mv_constants_capture::cbv(caller,desc,handle);
    if(call.legacy())attempt([&]{++invalidations;if(selected(device))bank->invalidate_if_tracked(lifetime,handle.ptr);});
    if(call.source())source_attempt([&]{++source_invalidations;if(source_selected(device))source_bank->invalidate_if_tracked(source_lifetime,handle.ptr);});
    if(call.target())target_attempt([&]{++target_invalidations;if(target_selected(device))target_bank->invalidate_if_tracked(target_lifetime,handle.ptr);});
}
void STDMETHODCALLTYPE srv(ID3D12Device* device,ID3D12Resource* resource,const D3D12_SHADER_RESOURCE_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE handle){
    Scope scope;auto frame_guard=frame_lock();Call call;original_srv(device,resource,desc,handle);
    ngx_mv_constants_capture::write(handle.ptr);
    if(call.legacy())attempt([&]{++invalidations;if(selected(device))bank->invalidate_if_tracked(lifetime,handle.ptr);});
    if(call.source())source_attempt([&]{++source_invalidations;if(source_selected(device))source_bank->invalidate_if_tracked(source_lifetime,handle.ptr);});
    if(call.target())target_attempt([&]{++target_invalidations;if(target_selected(device))target_bank->invalidate_if_tracked(target_lifetime,handle.ptr);});
}
void STDMETHODCALLTYPE uav(ID3D12Device* device,ID3D12Resource* resource,ID3D12Resource* counter,const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE handle){
    Scope scope;auto frame_guard=frame_lock();Call call;gears_ngx_mv::ViewDescription before;
    if(call){before.provided=desc!=nullptr;before.readable=!desc||safe_copy(&before.value,desc,sizeof(before.value));}
    original_uav(device,resource,counter,desc,handle);
    ngx_mv_constants_capture::write(handle.ptr);
    if(call.legacy())attempt([&]{++uav_calls;if(!bank){++missing_birth;fail();return;}
        if(selected(device)){if(!bank->contains_cpu(handle.ptr))++missing_birth;
            bank->write_uav(lifetime,handle.ptr,resource,counter,before);}});
    if(call.source())source_attempt([&]{++source_uavs;if(!source_bank){++source_missing_birth;source_fail();return;}
        if(source_selected(device)){if(!source_bank->contains_cpu(handle.ptr))++source_missing_birth;
            source_bank->write_uav(source_lifetime,handle.ptr,resource,counter,before);}});
    if(call.target())target_attempt([&]{++target_uavs;if(target_selected(device))target_uav(resource,counter,before,handle.ptr);});
}
// Dedicated self-test only. All writes below enter the real patched D3D12
// device methods; helper access is restricted to inspecting resulting evidence.
DWORD native_check(Bank::Snapshot& retained){
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cpu_heap,gpu_heap;
    Microsoft::WRL::ComPtr<ID3D12Resource> first,second,constant;
    if(FAILED(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))))return 302;
    auto table=*reinterpret_cast<void***>(device.Get());
    const unsigned slots[]{14,17,18,19,16};
    for(unsigned i=0;i<5;++i)if(table[slots[i]]!=prime_targets[prime_hook_base_count+i])return 303;
    if(table[23]!=prime_targets[1]||table[24]!=prime_targets[2])return 304;
    {std::lock_guard<std::recursive_mutex> lock(mutex);
        if(!armed||!accepting||failed||bank||heap_calls||uav_calls||copies||source_failed||source_bank||source_heaps||source_uavs||source_copies||
           target_bank||target_attempted||target_mv.retained())return 305;}
    if(const auto root_check=ngx_root_signature_capture::native_check(device.Get()))return root_check;
    D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=4;
    if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&cpu_heap))))return 306;
    hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&gpu_heap))))return 307;
    const auto device_id=canonical_identity(device.Get());
    {std::lock_guard<std::recursive_mutex> lock(mutex);
        if(!bank||bank->failed()||failed||heap_calls!=2||bank->heaps()!=2||bank->cells()||
           lifetime.kind!=gears_ngx_mv::ScopeKind::device_lifetime||!lifetime.epoch||
           lifetime.world||lifetime.ticket||lifetime.device!=device_id)return 308;}
    {std::lock_guard<std::recursive_mutex> lock(mutex);
        if(!source_bank||source_bank->failed()||source_heaps!=2||source_bank->heaps()!=2||source_bank->cells()||
           !gears_ngx_mv::same(source_lifetime,lifetime))return 330;}
    {std::lock_guard<std::recursive_mutex> lock(mutex);auto const& c=heap_census.counts();
        if(c.creations!=2||c.visible.creations!=1||c.nonvisible.creations!=1||
           c.visible.descriptors!=4||c.nonvisible.descriptors!=4||c.visible.max_descriptors!=4||c.nonvisible.max_descriptors!=4||
           c.failed||c.descriptor_unreadable||c.output_unreadable||c.overflow)return 329;}
    D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;rd.Width=16;rd.Height=16;
    rd.DepthOrArraySize=1;rd.MipLevels=1;rd.Format=DXGI_FORMAT_R32_UINT;rd.SampleDesc.Count=1;
    rd.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&first)))||
       FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&second))))return 309;
    auto cpu=cpu_heap->GetCPUDescriptorHandleForHeapStart();auto cpu1=cpu,cpu2=cpu,cpu3=cpu;
    auto gpu_cpu=gpu_heap->GetCPUDescriptorHandleForHeapStart();auto gpu=gpu_heap->GetGPUDescriptorHandleForHeapStart();
    const auto stride=device->GetDescriptorHandleIncrementSize(hd.Type);
    cpu1.ptr+=stride;cpu2.ptr+=2*stride;cpu3.ptr+=3*stride;gpu_cpu.ptr+=stride;gpu.ptr+=stride;
    auto snapshot=[&](uint64_t handle,bool is_gpu=false){std::lock_guard<std::recursive_mutex> lock(mutex);
        return bank?bank->lookup(lifetime,handle,is_gpu):Bank::Snapshot{};};
    auto current=[&](Bank::Snapshot const& value){std::lock_guard<std::recursive_mutex> lock(mutex);
        return bank&&bank->current(value);};
    D3D12_UNORDERED_ACCESS_VIEW_DESC uv{};uv.Format=rd.Format;uv.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(first.Get(),nullptr,&uv,cpu);
    auto original=snapshot(cpu.ptr);auto typed_first=gears_ngx_stereo::TypedResourceLease::observe(first.Get());
    if(!original.retained_uav()||!original.same_mv(typed_first)||original.resource.raw!=reinterpret_cast<uintptr_t>(first.Get())||
       original.heap.canonical!=canonical_identity(cpu_heap.Get())||original.resource.device!=device_id||
       !original.view.provided||original.view.value.Format!=rd.Format||original.view.value.ViewDimension!=uv.ViewDimension||
       original.counter.retained()||!current(original)||Bank::Snapshot::producer_authorized)return 310;
    device->CopyDescriptorsSimple(1,cpu1,cpu,hd.Type);
    auto simple=snapshot(cpu1.ptr);
    if(!simple.same_mv(typed_first)||simple.version<=original.version||simple.origin!=original.origin||
       simple.source_handle!=cpu.ptr||simple.source_version!=original.version||!current(simple))return 311;
    // Null size arrays exercise native one-descriptor-per-range semantics.
    device->CopyDescriptors(1,&gpu_cpu,nullptr,1,&cpu1,nullptr,hd.Type);
    auto copied=snapshot(gpu.ptr,true);
    if(!copied.same_mv(typed_first)||copied.heap.canonical!=canonical_identity(gpu_heap.Get())||
       copied.cpu!=gpu_cpu.ptr||copied.gpu!=gpu.ptr||copied.version<=simple.version||
       copied.origin!=original.origin||copied.source_handle!=cpu1.ptr||copied.source_version!=simple.version||!current(copied))return 312;
    retained=copied; // Deliberately retain historical typed evidence past bank teardown.
    device->CreateUnorderedAccessView(second.Get(),nullptr,&uv,cpu);
    auto rewritten=snapshot(cpu.ptr);auto typed_second=gears_ngx_stereo::TypedResourceLease::observe(second.Get());
    if(!rewritten.same_mv(typed_second)||rewritten.version<=copied.version||rewritten.origin!=rewritten.version||
       current(original)||!original.same_mv(typed_first)||!current(simple)||!current(copied))return 313;
    device->CopyDescriptorsSimple(1,cpu1,cpu,hd.Type);
    auto recopied=snapshot(cpu1.ptr);
    if(!recopied.same_mv(typed_second)||recopied.source_version!=rewritten.version||
       recopied.origin!=rewritten.origin||recopied.version<=rewritten.version||current(simple))return 314;
    D3D12_SHADER_RESOURCE_VIEW_DESC sv{};sv.Format=rd.Format;sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sv.Texture2D.MipLevels=1;
    device->CreateShaderResourceView(second.Get(),&sv,cpu1);
    auto erased=snapshot(cpu1.ptr);
    if(erased.known_uav||erased.resource.retained()||erased.version<=recopied.version||current(recopied))return 315;
    device->CreateShaderResourceView(first.Get(),&sv,cpu2); // Untracked SRV must allocate no cell.
    if(snapshot(cpu2.ptr).version)return 316;
    // Unknown native SRV copied over a tracked UAV must erase that target.
    device->CopyDescriptors(1,&gpu_cpu,nullptr,1,&cpu2,nullptr,hd.Type);
    auto unknown=snapshot(gpu.ptr,true);
    if(unknown.known_uav||unknown.resource.retained()||unknown.version<=copied.version||current(copied)||!retained.same_mv(typed_first))return 317;
    rd={};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=256;rd.Height=1;rd.DepthOrArraySize=1;
    rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&constant))))return 318;
    D3D12_CONSTANT_BUFFER_VIEW_DESC cv{};cv.BufferLocation=constant->GetGPUVirtualAddress();cv.SizeInBytes=256;
    if(!cv.BufferLocation)return 319;
    device->CreateConstantBufferView(&cv,cpu); // Tracked UAV becomes a CBV.
    auto cbv_erased=snapshot(cpu.ptr);
    if(cbv_erased.known_uav||cbv_erased.resource.retained()||cbv_erased.version<=rewritten.version||current(rewritten))return 320;
    device->CreateConstantBufferView(&cv,cpu3); // Unrelated CBV must allocate no cell.
    if(snapshot(cpu3.ptr).version)return 321;
    {std::lock_guard<std::recursive_mutex> lock(mutex);
        if(!bank||failed||bank->failed()||bank->cells()!=3||heap_calls!=2||uav_calls!=2||copies!=4||
           invalidations!=4||other_devices||missing_birth||recursions||uncertainties)return 322;}
    // Deliberate fixture-only loss: verify real patched device callbacks still
    // feed the independent ledger while every legacy count/cell stays frozen.
    {std::lock_guard<std::recursive_mutex> lock(mutex);
        if(!source_bank||source_failed||source_bank->failed()||source_bank->cells()!=2||
           source_bank->lookup(source_lifetime,gpu_cpu.ptr).version||source_uavs!=2||source_copies!=4||source_invalidations!=4)return 331;
        bank->uncertain();}
    device->CreateUnorderedAccessView(first.Get(),nullptr,&uv,cpu);
    device->CopyDescriptorsSimple(1,cpu1,cpu,hd.Type);
    SourceBank::Snapshot source_alias;
    {std::lock_guard<std::recursive_mutex> lock(mutex);source_alias=source_bank->lookup(source_lifetime,cpu1.ptr);
        if(!source_alias.same_mv(typed_first)||source_alias.source_handle!=cpu.ptr||!source_bank->current(source_alias))return 332;}
    device->CreateShaderResourceView(second.Get(),&sv,cpu1);
    device->CopyDescriptorsSimple(1,gpu_cpu,cpu,hd.Type); // Suppressed visible replica.
    {std::lock_guard<std::recursive_mutex> lock(mutex);
        if(source_bank->current(source_alias)||source_bank->lookup(source_lifetime,gpu_cpu.ptr).version||source_bank->cells()!=2)return 333;}
    device->CreateUnorderedAccessView(first.Get(),nullptr,&uv,gpu_cpu); // Direct visible birth retained.
    SourceBank::Snapshot visible_birth;
    {std::lock_guard<std::recursive_mutex> lock(mutex);visible_birth=source_bank->lookup(source_lifetime,gpu_cpu.ptr);
        if(!visible_birth.same_mv(typed_first)||!visible_birth.heap.visible)return 334;}
    device->CopyDescriptorsSimple(1,gpu_cpu,cpu1,hd.Type); // Unknown alias erases tracked visible birth.
    accepted_eval(0,1,2,3,4,5,6,&typed_first);
    {std::lock_guard<std::recursive_mutex> lock(mutex);
        if(source_failed||source_bank->failed()||source_bank->current(visible_birth)||source_bank->cells()!=3||
           source_uavs!=4||source_copies!=7||source_invalidations!=5||source_other_devices||source_missing_birth||source_uncertainties)return 335;
        auto const& j=source_join.summary;
        if(!j.present||!j.bank_ready||!j.typed_retained||!j.device_match||j.matches!=1||j.saved!=1||
           !source_join.legacy_failed_at_eval||!gears_ngx_mv::same(source_join.rows[0].scope,source_lifetime))return 336;
        if(bank->loss()!=gears_ngx_mv::Loss::unreadable||bank->cells()!=3||heap_calls!=2||uav_calls!=2||copies!=4||invalidations!=4||
           failed||uncertainties||recursions)return 337;}
    TargetBank::Snapshot seeded_target,target_alias,target_visible;
    {std::lock_guard<std::recursive_mutex> lock(mutex);
        if(!target_seeded||target_failed||target_stopped||!source_frozen||source_live()||!target_live()||target_seed_count!=1||target_bank->heaps()!=2||target_bank->cells()!=1)return 339;
        seeded_target=target_bank->lookup(target_lifetime,cpu.ptr);
        if(!seeded_target.same_mv(typed_first)||!seeded_target.seed||!gears_ngx_mv::same(seeded_target.seed->scope,source_lifetime)||
           !gears_ngx_mv::same(seeded_target.scope,target_lifetime)||target_lifetime.world||target_lifetime.ticket)return 340;}
    device->CopyDescriptorsSimple(1,cpu1,cpu,hd.Type);
    device->CopyDescriptorsSimple(1,gpu_cpu,cpu1,hd.Type);
    {std::lock_guard<std::recursive_mutex> lock(mutex);
        target_alias=target_bank->lookup(target_lifetime,cpu1.ptr);target_visible=target_bank->lookup(target_lifetime,gpu_cpu.ptr);
        if(!target_alias.same_mv(typed_first)||!target_visible.same_mv(typed_first)||target_alias.seed!=seeded_target.seed||
           target_visible.seed!=seeded_target.seed||target_visible.source_version!=target_alias.version)return 341;}
#if GEARS_NGX_MV_TARGET_CAPTURE
    const auto selected_lookup=lookup_target(target_visible.heap.canonical,target_visible.gpu);
    if(!selected_lookup.ready||!selected_lookup.exact_base_mv||selected_lookup.overflow||selected_lookup.count!=1||
       selected_lookup.candidates[0].offset||selected_lookup.candidates[0].version!=target_visible.version)return 350;
    if(lookup_target(target_visible.heap.canonical+1,target_visible.gpu).count||
       lookup_target(target_visible.heap.canonical,target_visible.gpu+1).count)return 351;
    const auto exact_lookup=lookup_exact_target(target_visible.gpu);
    if(!exact_lookup.ready||!exact_lookup.match||exact_lookup.heap!=target_visible.heap.canonical||
       exact_lookup.resource!=typed_first.canonical||exact_lookup.version!=target_visible.version||exact_lookup.origin!=target_visible.origin)return 353;
    if(lookup_exact_target(target_visible.gpu+1).match||lookup_exact_target(target_visible.gpu+4*stride).match||
       lookup_exact_target(0).match)return 354;
#endif
    device->CreateUnorderedAccessView(second.Get(),nullptr,&uv,cpu2); // Unrelated native birth consumes no target cell.
    device->CopyDescriptorsSimple(1,cpu3,cpu2,hd.Type); // Unrelated native copy consumes no target cell.
    {std::lock_guard<std::recursive_mutex> lock(mutex);
        if(target_bank->cells()!=3||target_bank->lookup(target_lifetime,cpu2.ptr).version||target_bank->lookup(target_lifetime,cpu3.ptr).version)return 342;}
    device->CreateShaderResourceView(second.Get(),&sv,cpu1);
    device->CopyDescriptorsSimple(1,gpu_cpu,cpu1,hd.Type);
    device->CreateUnorderedAccessView(second.Get(),nullptr,&uv,cpu); // Unrelated replacement erases the tracked MV.
    {std::lock_guard<std::recursive_mutex> lock(mutex);
        if(target_bank->current(target_alias)||target_bank->current(target_visible)||target_bank->current(seeded_target)||target_bank->cell_counts().known_uav)return 343;}
#if GEARS_NGX_MV_TARGET_CAPTURE
    const auto invalidated_lookup=lookup_target(target_visible.heap.canonical,target_visible.gpu);
    if(!invalidated_lookup.ready||invalidated_lookup.exact_base_mv||invalidated_lookup.count||
       gears_ngx_mv_compute::same(selected_lookup,invalidated_lookup))return 352;
    const auto erased_exact=lookup_exact_target(target_visible.gpu);
    if(!erased_exact.ready||erased_exact.match||same_exact(exact_lookup,erased_exact))return 355;
#endif
    device->CreateUnorderedAccessView(first.Get(),nullptr,&uv,cpu); // Selected typed native rewrite becomes fresh local provenance.
    device->CopyDescriptorsSimple(1,gpu_cpu,cpu,hd.Type);
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> late_heap;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&late_heap))))return 344;
    TargetBank::Snapshot frozen_target;
    {std::lock_guard<std::recursive_mutex> lock(mutex);frozen_target=target_bank->lookup(target_lifetime,cpu.ptr);
        if(!frozen_target.same_mv(typed_first)||frozen_target.seed||target_bank->heaps()!=3||target_bank->cells()!=3||target_bank->cell_counts().known_uav!=2||
           target_heaps!=1||target_uavs!=3||target_unrelated_uavs!=2||target_invalidations!=1||target_copies!=5||target_other_devices||target_uncertainties||target_failed||target_bank->failed())return 345;}
    stop_target();
    device->CreateUnorderedAccessView(second.Get(),nullptr,&uv,cpu); // Admission stopped; keep only frozen historical evidence.
    {std::lock_guard<std::recursive_mutex> lock(mutex);
        if(target_live()||!target_stopped||!target_bank||!target_mv.retained()||target_uavs!=3||
           target_bank->lookup(target_lifetime,cpu.ptr).version!=frozen_target.version)return 346;
        if(source_heaps!=2||source_uavs!=4||source_copies!=7||source_invalidations!=5||source_bank->cells()!=3||
           bank->loss()!=gears_ngx_mv::Loss::unreadable||heap_calls!=2||uav_calls!=2||copies!=4||invalidations!=4)return 347;}
    return 0;
}
DWORD native_closed_check(Bank::Snapshot const& retained){
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if(accepting||!released||bank||active.load()||depth||failed||final_loss!=unsigned(gears_ngx_mv::Loss::unreadable)||final_heaps!=2||final_cells!=3)return 327;
    if(source_bank||source_failed||source_final_loss||source_final_heaps!=2||source_final_cells!=3||source_final_counts.known_uav!=1||source_final_counts.non_uav!=2)return 338;
    if(target_bank||target_mv.retained()||target_failed||target_final_loss||target_final_heaps!=3||target_final_cells!=3||
       target_final_counts.known_uav!=2||target_final_counts.non_uav!=1||!source_frozen||!target_stopped)return 348;
    if(!retained.retained_uav()||!gears_ngx_mv::same(retained.scope,lifetime)||retained.scope.world||retained.scope.ticket)return 328;
    return 0;
}
#else
void arm(){}void close(){}void release_drained(){}void print(FILE*){}
#endif
}
