// Targeted CPU command-list observations only. Typed arguments are retained;
// NGX integers are never converted to COM. No added commands or GPU proof.
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
HeapsFn original_heaps{};RootFn original_root{};TableFn original_table{};DispatchFn original_dispatch{};
ClearFn original_clear{};BundleFn original_bundle{};
struct List {
    uintptr_t raw{},canonical{};uint64_t generation{};
    gears_vr::ComPtr<ID3D12GraphicsCommandList> held;
    gears_vr::ComPtr<IUnknown> identity;
    HeapLease heap;
    gears_vr::ComPtr<ID3D12RootSignature> root;
    gears_vr::ComPtr<IUnknown> root_identity;
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
        f.prior_stored_generation=prior_stored;f.prior_stored_known=prior_known;f.result=result;
        f.owned_known=evidence_context.owned_known;f.owned=evidence_context.owned;f.bypass=stereo_publish_bypass!=0;
        f.ticket=consuming_frame_ticket;f.world=committed_world_generation.load();
        f.frozen_ticket=consuming_frozen_frame.id;f.frozen_world=consuming_frozen_frame.world_generation;
        f.renderer=reinterpret_cast<uintptr_t>(consuming_renderer);f.window_phase=unsigned(ngx_mv_target_capture::window.snapshot().phase);
        f.core_loss=unsigned(bindings.loss());f.depth_at_failure=depth;f.lists_at_failure=list_count;f.rows_at_failure=row_count;f.mutation_at_failure=mutation;
        failed=true;failure=reason;bindings.uncertain();
    }stopped=true;
}
bool pending(){return !stopped&&!failed&&ngx_mv_target_capture::pending();}
bool observing(bool owned){
    evidence_context.owned_known=true;evidence_context.owned=owned;
    ngx_mv_target_capture::poll();
    if(!pending()||!owned||stereo_publish_bypass||!recording.load())return false;
    auto const& s=ngx_mv_target_capture::window.snapshot();
    if(!consuming_frozen_frame.enabled||!consuming_renderer||consuming_frozen_frame.world_generation!=s.selection.world||
       committed_world_generation.load()!=s.selection.world||consuming_frame_ticket!=consuming_frozen_frame.id)return false;
    if(s.phase==gears_ngx_mv_target::Phase::capturing)
        return consuming_frame_ticket==s.consume.ticket&&reinterpret_cast<uintptr_t>(consuming_renderer)==s.consume.renderer;
    return consuming_frame_ticket>=s.selection.ticket;
}
struct Depth {
    bool outer;int exceptions=std::uncaught_exceptions();
    Depth():outer(depth++==0){if(!outer&&pending())lose(nested,"depth_enter");}
    ~Depth(){--depth;if(std::uncaught_exceptions()!=exceptions&&pending())lose(unwinding,"depth_unwind");}
};
List* known(ID3D12GraphicsCommandList* list){
    if(!list)return nullptr;const auto id=canonical_identity(list);
    for(unsigned i=0;i<list_count;++i)if(lists[i].canonical==id)return &lists[i];return nullptr;
}
Recording ref(List const& list){return {list.canonical,list.generation,frame_link.current(list.canonical).generation};}
List* track(ID3D12GraphicsCommandList* list){
    if(!list||list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT)return nullptr;
    auto* old=known(list);if(old){if(ref(*old).attested_current_generation!=old->generation){lose(invalid_generation,"track_existing",old,reinterpret_cast<uintptr_t>(list));return nullptr;}return old;}
    if(list_count==lists.size()){lose(list_capacity,"track_capacity",nullptr,reinterpret_cast<uintptr_t>(list));return nullptr;}
    gears_vr::ComPtr<IUnknown> id;gears_vr::ComPtr<ID3D12Device> device;gears_vr::ComPtr<IUnknown> device_id;
    if(FAILED(list->QueryInterface(IID_PPV_ARGS(&id)))||FAILED(list->GetDevice(IID_PPV_ARGS(&device)))||FAILED(device.As(&device_id))||
       reinterpret_cast<uintptr_t>(device_id.Get())!=ngx_mv_target_capture::selected_mv.device){lose(invalid_list,"track_identity",nullptr,reinterpret_cast<uintptr_t>(list),reinterpret_cast<uintptr_t>(id.Get()));return nullptr;}
    const auto canonical=reinterpret_cast<uintptr_t>(id.Get());const auto current=frame_link.current(canonical);
    if(!canonical||!current.generation){lose(invalid_generation,"track_new_generation",nullptr,reinterpret_cast<uintptr_t>(list),canonical);return nullptr;}
    auto& out=lists[list_count++];out.raw=reinterpret_cast<uintptr_t>(list);out.canonical=canonical;
    out.generation=current.generation;out.held=list;out.identity=std::move(id);return &out;
}
bool current(List const& list){ngx_mv_target_capture::poll();const auto r=ref(list);if(!r.attested_current_generation||r.generation!=r.attested_current_generation){lose(invalid_generation,"current",&list);return false;}return pending();}
void noted(List const& list){if(bindings.failed()){lose(invalid_generation,"noted_core",&list);return;}++mutation;
    if(ngx_mv_target_capture::window.snapshot().phase==gears_ngx_mv_target::Phase::waiting)++waiting_bindings;else ++capture_bindings;}
void invalidate(ID3D12GraphicsCommandList* list,unsigned kind){
    std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);if(!pending())return;auto* held=known(list);if(!held)return;
    ++invalidations;++mutation;if(kind==2)bindings.execute_bundle(ref(*held));else bindings.clear_state(ref(*held));
    if(bindings.failed())lose(invalid_generation,kind==2?"invalidate_bundle":kind==1?"invalidate_clear":"invalidate_reset_or_unobserved",held,reinterpret_cast<uintptr_t>(list));
}
struct ResetScope {
    ID3D12GraphicsCommandList* list;bool returned{};
    explicit ResetScope(ID3D12GraphicsCommandList* l):list(l){EvidenceScope evidence("reset_begin",list);invalidate(list,0);}
    void finish(HRESULT hr){
        EvidenceScope evidence("reset_finish",list);
        returned=true;std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);if(!pending())return;
        auto* held=known(list);if(!held)return;
        if(FAILED(hr)){lose(invalid_generation,"reset_finish_failed",held,reinterpret_cast<uintptr_t>(list),0,hr);return;}
        const auto prior_stored=held->generation;
        ++mutation;held->generation=frame_link.current(held->canonical).generation;bindings.reset(ref(*held));
        if(bindings.failed())lose(invalid_generation,"reset_finish_core",held,reinterpret_cast<uintptr_t>(list),0,hr,prior_stored,true);
    }
    ~ResetScope(){if(!returned){std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);EvidenceScope evidence("reset_unwind",list);if(pending())lose(unwinding,"reset_unwind",nullptr,reinterpret_cast<uintptr_t>(list));}}
};
void STDMETHODCALLTYPE heaps(ID3D12GraphicsCommandList* list,UINT count,ID3D12DescriptorHeap* const* native){
    Scope scope;std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);EvidenceScope evidence("heaps",list);Depth call;
    List* held=call.outer&&observing(scope.world())?track(list):nullptr;
    HeapLease selected;std::array<ID3D12DescriptorHeap*,2> values{};
    if(held){
        if(count>values.size()||(count&&!safe_copy(values.data(),native,count*sizeof(values[0]))))lose(invalid_heap,"heaps_array",held);
        else for(UINT i=0;i<count;++i){if(!values[i]){lose(invalid_heap,"heaps_null",held);break;}
            const auto desc=values[i]->GetDesc();if(desc.Type!=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)continue;
            if(selected.retained()){lose(invalid_heap,"heaps_duplicate",held);break;}selected=HeapLease::observe(values[i]);
            if(!selected.retained()||!selected.visible||selected.device!=ngx_mv_target_capture::selected_mv.device){lose(invalid_heap,"heaps_identity",held);break;}}
    }else invalidate(list,0);
    const auto before=mutation;original_heaps(list,count,native);
    if(held&&current(*held)){if(before!=mutation){lose(changed,"heaps_post_mutation",held);return;}
        held->heap=std::move(selected);bindings.shader_heap(ref(*held),held->heap.canonical);noted(*held);}
}
void STDMETHODCALLTYPE root(ID3D12GraphicsCommandList* list,ID3D12RootSignature* native){
    Scope scope;std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);EvidenceScope evidence("root",list);Depth call;
    List* held=call.outer&&observing(scope.world())?track(list):nullptr;
    gears_vr::ComPtr<ID3D12RootSignature> owner;gears_vr::ComPtr<IUnknown> id;
    if(held&&native){gears_vr::ComPtr<ID3D12Device> device;gears_vr::ComPtr<IUnknown> di;owner=native;
        if(FAILED(native->QueryInterface(IID_PPV_ARGS(&id)))||FAILED(native->GetDevice(IID_PPV_ARGS(&device)))||FAILED(device.As(&di))||
           reinterpret_cast<uintptr_t>(di.Get())!=ngx_mv_target_capture::selected_mv.device)lose(invalid_root,"root_identity",held);
    }else if(!held)invalidate(list,0);
    const auto before=mutation;original_root(list,native);
    if(held&&current(*held)){if(before!=mutation){lose(changed,"root_post_mutation",held);return;}held->root=std::move(owner);held->root_identity=std::move(id);
        bindings.compute_root_signature(ref(*held),reinterpret_cast<uintptr_t>(held->root_identity.Get()));noted(*held);}
}
void STDMETHODCALLTYPE table(ID3D12GraphicsCommandList* list,UINT index,D3D12_GPU_DESCRIPTOR_HANDLE gpu){
    Scope scope;std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);EvidenceScope evidence("table",list);Depth call;
    List* held=call.outer&&observing(scope.world())?track(list):nullptr;if(!held)invalidate(list,0);
    const auto before=mutation;original_table(list,index,gpu);
    if(held&&current(*held)){if(before!=mutation){lose(changed,"table_post_mutation",held);return;}bindings.compute_root_table(ref(*held),index,gpu.ptr);noted(*held);}
}
void STDMETHODCALLTYPE dispatch(ID3D12GraphicsCommandList* list,UINT x,UINT y,UINT z){
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());Scope scope;
    std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);EvidenceScope evidence("dispatch",list);Depth call;
    Row* row{};List* held{};
    if(call.outer&&observing(scope.world())&&ngx_mv_target_capture::window.snapshot().phase==gears_ngx_mv_target::Phase::capturing){
        ++dispatches;held=track(list);
        if(held){const auto candidate=bindings.dispatch(ref(*held));
            if(!candidate.binding_observed)++unbound;
            else {auto observed=ngx_mv_descriptor_capture::lookup_target(candidate.shader_visible_heap,candidate.root3_gpu);
                if(observed.overflow)lose(descriptor_capacity,"dispatch_descriptor_capacity",held);
                else if(!observed.ready||(!observed.exact_base_mv&&!observed.count))++unrelated;
                else if(row_count==rows.size())lose(row_capacity,"dispatch_row_capacity",held);
                else {row=&rows[row_count++];auto const& w=ngx_mv_target_capture::window.snapshot();
                    row->world=w.selection.world;row->ticket=w.consume.ticket;row->renderer=w.consume.renderer;
                    row->raw=reinterpret_cast<uintptr_t>(list);row->canonical=held->canonical;row->generation=held->generation;
                    row->root=candidate.compute_root_signature;row->heap=candidate.shader_visible_heap;row->root3=candidate.root3_gpu;
                    row->caller=caller;row->groups={x,y,z};row->before=observed;
                    row->caller_candidate=ngx_stereo_host::attested&&caller==base+0x1ac833;}
            }
        }
    }
    // No descriptor observer mutex is held across the native Dispatch.
    const auto before=mutation;original_dispatch(list,x,y,z);
    if(row){row->returned=true;row->after=ngx_mv_descriptor_capture::lookup_target(row->heap,row->root3);
        row->stable=held&&current(*held)&&before==mutation&&gears_ngx_mv_compute::same(row->before,row->after);
        if(!row->stable)lose(changed,"dispatch_post",held);}
}
void STDMETHODCALLTYPE clear(ID3D12GraphicsCommandList* list,ID3D12PipelineState* pso){
    Scope scope;std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);EvidenceScope evidence("clear",list);invalidate(list,1);original_clear(list,pso);invalidate(list,1);
}
void STDMETHODCALLTYPE bundle(ID3D12GraphicsCommandList* list,ID3D12GraphicsCommandList* child){
    Scope scope;std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);EvidenceScope evidence("bundle",list);invalidate(list,2);original_bundle(list,child);invalidate(list,2);
}
bool install_hooks(void** vtable){
    constexpr unsigned first=viewport_hook_base_capacity+ngx_mv_descriptor_hooks;
    viewport_targets[first]=vtable[28];viewport_targets[first+1]=vtable[29];viewport_targets[first+2]=vtable[31];viewport_targets[first+3]=vtable[14];
    bool ok=install(viewport_targets[first],reinterpret_cast<void*>(heaps),reinterpret_cast<void**>(&original_heaps));
    ok=install(viewport_targets[first+1],reinterpret_cast<void*>(root),reinterpret_cast<void**>(&original_root))&&ok;
    ok=install(viewport_targets[first+2],reinterpret_cast<void*>(ngx_mv_compute_capture::table),reinterpret_cast<void**>(&original_table))&&ok;
    ok=install(viewport_targets[first+3],reinterpret_cast<void*>(dispatch),reinterpret_cast<void**>(&original_dispatch))&&ok;
#if !defined(GEARS_DEATH_PSO_CACHE) || !GEARS_DEATH_PSO_CACHE
    viewport_targets[first+4]=vtable[11];viewport_targets[first+5]=vtable[27];
    ok=install(viewport_targets[first+4],reinterpret_cast<void*>(clear),reinterpret_cast<void**>(&original_clear))&&ok;
    ok=install(viewport_targets[first+5],reinterpret_cast<void*>(bundle),reinterpret_cast<void**>(&original_bundle))&&ok;
#endif
    return ok;
}
void stop(){std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);stopped=true;}
void release_drained(){
    std::array<List,16> retired;
    {std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);
        if(!stopped||active.load()||recording.load())return;lists.swap(retired);leases_released=true;}
    // Local retired owners release after the observation lock is dropped.
}
void print(FILE* file){
    std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);if(!file)return;
    fprintf(file,"ngx_mv_compute enabled=1 lists=%u list_limit=16 rows=%u row_limit=32 failed=%u failure=%u core_loss=%u stopped=%u leases_released=%u dispatches=%llu unbound=%llu unrelated=%llu invalidations=%llu waiting_bindings=%llu capture_bindings=%llu row_scope=selected_mv_related table_membership_verified=0 gpu_execution_verified=0 producer_authorized=0\n",
        list_count,row_count,unsigned(failed),failure,unsigned(bindings.loss()),unsigned(stopped),unsigned(leases_released),dispatches,unbound,unrelated,invalidations,waiting_bindings,capture_bindings);
    auto const& f=first_failure;
    fprintf(file,"ngx_mv_compute_first_failure present=%u reason=%u site=%s operation=%s raw=%llx canonical=%llx stored_generation=%llu current_identity=%llx current_generation=%llu prior_stored_known=%u prior_stored_generation=%llu result=%08lx bypass=%u owned_known=%u owned=%u ticket=%llu world=%llu frozen_ticket=%llu frozen_world=%llu renderer=%llx phase=%u core_loss_at_failure=%u depth=%u lists=%u rows=%u mutation=%llu\n",
        unsigned(f.present),f.reason,f.site,f.operation,f.raw,f.canonical,f.stored_generation,f.current_identity,f.current_generation,
        unsigned(f.prior_stored_known),f.prior_stored_generation,static_cast<unsigned long>(f.result),unsigned(f.bypass),unsigned(f.owned_known),unsigned(f.owned),
        f.ticket,f.world,f.frozen_ticket,f.frozen_world,f.renderer,f.window_phase,f.core_loss,f.depth_at_failure,f.lists_at_failure,f.rows_at_failure,f.mutation_at_failure);
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
struct ResetScope {explicit ResetScope(ID3D12GraphicsCommandList*){}void finish(HRESULT){}};
void stop(){}void release_drained(){}void print(FILE*){}void invalidate(ID3D12GraphicsCommandList*,unsigned){}
bool install_hooks(void**){return true;}
#endif
}
