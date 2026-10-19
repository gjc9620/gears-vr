// Private, CPU-only early descriptor evidence. Included inside capture.cpp's
// namespace after Scope/frame_lock. The first observed typed heap selects a
// candidate device, never an attested game device or an NGX epoch/ticket.
namespace ngx_mv_descriptor_capture {
#if defined(GEARS_NGX_MV_DESCRIPTOR_OBSERVER) && GEARS_NGX_MV_DESCRIPTOR_OBSERVER
using Bank=gears_ngx_mv::ObserverDescriptorBank<>;
using Ranges=gears_ngx_mv::Ranges<128>;
using HeapFn=HRESULT(STDMETHODCALLTYPE*)(ID3D12Device*,const D3D12_DESCRIPTOR_HEAP_DESC*,REFIID,void**);
using CbvFn=void(STDMETHODCALLTYPE*)(ID3D12Device*,const D3D12_CONSTANT_BUFFER_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
using SrvFn=void(STDMETHODCALLTYPE*)(ID3D12Device*,ID3D12Resource*,const D3D12_SHADER_RESOURCE_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
using UavFn=void(STDMETHODCALLTYPE*)(ID3D12Device*,ID3D12Resource*,ID3D12Resource*,const D3D12_UNORDERED_ACCESS_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
HeapFn original_heap{};CbvFn original_cbv{};SrvFn original_srv{};UavFn original_uav{};
std::recursive_mutex mutex;
std::unique_ptr<Bank> bank;
gears_ngx_mv_census::Census heap_census;
gears_ngx_mv::Scope lifetime{};
bool armed{},accepting{},failed{},released{};
unsigned final_loss{};size_t final_heaps{},final_cells{};
Bank::CellCounts final_cell_counts{};Bank::Saturation final_saturation{};
uint64_t heap_calls{},uav_calls{},invalidations{},copies{},other_devices{},missing_birth{},recursions{},uncertainties{};
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
void fail(){failed=true;++uncertainties;if(bank)bank->uncertain();}
template<class F> void attempt(F&& fn){try{fn();}catch(...){fail();}}
struct Call {
    std::unique_lock<std::recursive_mutex> lock{mutex,std::defer_lock};bool entered{},admitted{},census_admitted{};
    explicit Call(bool relevant=true){if(!relevant)return;lock.lock();if(!accepting)return;
        entered=true;if(depth++){++recursions;fail();return;}census_admitted=true;admitted=!failed&&(!bank||!bank->failed());}
    ~Call(){if(entered)--depth;}
    explicit operator bool()const{return admitted&&!failed&&(!bank||!bank->failed());}
};
bool selected(ID3D12Device* device){
    if(!bank)return false;const auto identity=canonical_identity(device);
    if(!identity){fail();return false;}
    if(identity!=lifetime.device){++other_devices;return false;}return true;
}
Ranges read_ranges(UINT count,const D3D12_CPU_DESCRIPTOR_HANDLE* starts,const UINT* sizes){
    Ranges out;out.count=count;out.sizes_present=sizes!=nullptr;
    if(count>out.starts.size()){out.readable=false;return out;}
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE,128> handles{};
    if(count&&(!safe_copy(handles.data(),starts,count*sizeof(handles[0]))||
       (sizes&&!safe_copy(out.sizes.data(),sizes,count*sizeof(out.sizes[0]))))){out.readable=false;return out;}
    for(unsigned i=0;i<count;++i)out.starts[i]=handles[i].ptr;return out;
}
void copied(ID3D12Device* device,Ranges const& destination,Ranges const& source){
    ++copies;if(!bank){++missing_birth;fail();return;}
    if(selected(device))bank->copy_sparse(lifetime,destination,source);
}
void arm(){std::lock_guard<std::recursive_mutex> lock(mutex);if(!armed){armed=true;accepting=true;heap_census.arm();}}
void close(){std::lock_guard<std::recursive_mutex> lock(mutex);accepting=false;heap_census.close();}
void accepted_eval(unsigned ordinal,uint64_t revision,uint64_t world,uint64_t ticket,uintptr_t renderer,uintptr_t handle,uintptr_t parameters,
    gears_ngx_stereo::TypedResourceLease const* mv){
    std::lock_guard<std::recursive_mutex> lock(mutex);
    heap_census.accepted(ordinal,{revision,world,ticket,renderer,handle,parameters});
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
    std::unique_ptr<Bank> retired;
    {std::lock_guard<std::recursive_mutex> lock(mutex);
        if(accepting||active.load()||depth)return;
        if(bank){final_loss=static_cast<unsigned>(bank->loss());final_heaps=bank->heaps();final_cells=bank->cells();
            final_cell_counts=bank->cell_counts();final_saturation=bank->saturation();retired=std::move(bank);}
        released=true;}
    // No observer lock across native Release: a closed passthrough callback
    // may still enter the DLL. It cannot access the detached bank.
    retired.reset();
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
    fprintf(status,"ngx_mv_descriptor_cells known_uav=%zu non_uav=%zu unused=%zu reuses=%llu reuse_non_uav=%u saturation_site=%u saturation_schema=0:none,1:heaps,2:cells,3:ranges,4:serial saturation_heaps=%zu saturation_known_uav=%zu saturation_non_uav=%zu saturation_unused=%zu saturation_reuses=%llu known_uav_eviction=0 capacity_recovery=0 producer_authorized=0\n",
        occupancy.known_uav,occupancy.non_uav,occupancy.unused,occupancy.reuses,unsigned(Bank::reuse_non_uav),unsigned(saturation.site),saturation.heaps,
        saturation.cells.known_uav,saturation.cells.non_uav,saturation.cells.unused,saturation.cells.reuses);
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
    if(call)attempt([&]{
        if(!readable){fail();return;}
        if(before.Type!=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV||FAILED(result))return;
        ++heap_calls;Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> typed;
        if(!output_readable||!raw||FAILED(raw->QueryInterface(IID_PPV_ARGS(&typed)))){fail();return;}
        if(!bank){const auto identity=canonical_identity(device);if(!identity){fail();return;}
            // One immutable bank: no world-reset clearing or device rebinding.
            lifetime=gears_ngx_mv::lifetime_scope(1,identity);bank=std::make_unique<Bank>();
            if(!bank->begin(lifetime)){fail();return;}}
        if(selected(device))bank->observe_heap(lifetime,typed.Get());
    });return result;
}
void STDMETHODCALLTYPE cbv(ID3D12Device* device,const D3D12_CONSTANT_BUFFER_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE handle){
    Scope scope;auto frame_guard=frame_lock();Call call;original_cbv(device,desc,handle);
    if(call)attempt([&]{++invalidations;if(selected(device))bank->invalidate_if_tracked(lifetime,handle.ptr);});
}
void STDMETHODCALLTYPE srv(ID3D12Device* device,ID3D12Resource* resource,const D3D12_SHADER_RESOURCE_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE handle){
    Scope scope;auto frame_guard=frame_lock();Call call;original_srv(device,resource,desc,handle);
    if(call)attempt([&]{++invalidations;if(selected(device))bank->invalidate_if_tracked(lifetime,handle.ptr);});
}
void STDMETHODCALLTYPE uav(ID3D12Device* device,ID3D12Resource* resource,ID3D12Resource* counter,const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE handle){
    Scope scope;auto frame_guard=frame_lock();Call call;gears_ngx_mv::ViewDescription before;
    if(call){before.provided=desc!=nullptr;before.readable=!desc||safe_copy(&before.value,desc,sizeof(before.value));}
    original_uav(device,resource,counter,desc,handle);
    if(call)attempt([&]{++uav_calls;if(!bank){++missing_birth;fail();return;}
        if(selected(device)){if(!bank->contains_cpu(handle.ptr))++missing_birth;
            bank->write_uav(lifetime,handle.ptr,resource,counter,before);}});
}
// Dedicated self-test only. All writes below enter the real patched D3D12
// device methods; helper access is restricted to inspecting resulting evidence.
DWORD native_check(Bank::Snapshot& retained){
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cpu_heap,gpu_heap;
    Microsoft::WRL::ComPtr<ID3D12Resource> first,second,constant;
    if(FAILED(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))))return 302;
    auto table=*reinterpret_cast<void***>(device.Get());
    const unsigned slots[]{14,17,18,19};
    for(unsigned i=0;i<4;++i)if(table[slots[i]]!=prime_targets[prime_hook_base_count+i])return 303;
    if(table[23]!=prime_targets[1]||table[24]!=prime_targets[2])return 304;
    {std::lock_guard<std::recursive_mutex> lock(mutex);
        if(!armed||!accepting||failed||bank||heap_calls||uav_calls||copies)return 305;}
    D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=4;
    if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&cpu_heap))))return 306;
    hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&gpu_heap))))return 307;
    const auto device_id=canonical_identity(device.Get());
    {std::lock_guard<std::recursive_mutex> lock(mutex);
        if(!bank||bank->failed()||failed||heap_calls!=2||bank->heaps()!=2||bank->cells()||
           lifetime.kind!=gears_ngx_mv::ScopeKind::device_lifetime||!lifetime.epoch||
           lifetime.world||lifetime.ticket||lifetime.device!=device_id)return 308;}
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
    return 0;
}
DWORD native_closed_check(Bank::Snapshot const& retained){
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if(accepting||!released||bank||active.load()||depth||failed||final_loss||final_heaps!=2||final_cells!=3)return 327;
    if(!retained.retained_uav()||!gears_ngx_mv::same(retained.scope,lifetime)||retained.scope.world||retained.scope.ticket)return 328;
    return 0;
}
#else
void arm(){}void close(){}void release_drained(){}void print(FILE*){}
#endif
}
