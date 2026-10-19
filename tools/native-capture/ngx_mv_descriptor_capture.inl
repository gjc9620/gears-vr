// Private, CPU-only early descriptor evidence. Included inside capture.cpp's
// namespace after Scope/frame_lock. The first observed typed heap selects a
// candidate device, never an attested game device or an NGX epoch/ticket.
namespace ngx_mv_descriptor_capture {
#if defined(GEARS_NGX_MV_DESCRIPTOR_OBSERVER) && GEARS_NGX_MV_DESCRIPTOR_OBSERVER
using Bank=gears_ngx_mv::DescriptorBank<>;
using Ranges=gears_ngx_mv::Ranges<128>;
using HeapFn=HRESULT(STDMETHODCALLTYPE*)(ID3D12Device*,const D3D12_DESCRIPTOR_HEAP_DESC*,REFIID,void**);
using CbvFn=void(STDMETHODCALLTYPE*)(ID3D12Device*,const D3D12_CONSTANT_BUFFER_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
using SrvFn=void(STDMETHODCALLTYPE*)(ID3D12Device*,ID3D12Resource*,const D3D12_SHADER_RESOURCE_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
using UavFn=void(STDMETHODCALLTYPE*)(ID3D12Device*,ID3D12Resource*,ID3D12Resource*,const D3D12_UNORDERED_ACCESS_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
HeapFn original_heap{};CbvFn original_cbv{};SrvFn original_srv{};UavFn original_uav{};
std::recursive_mutex mutex;
std::unique_ptr<Bank> bank;
gears_ngx_mv::Scope lifetime{};
bool armed{},accepting{},failed{},released{};
unsigned final_loss{};size_t final_heaps{},final_cells{};
uint64_t heap_calls{},uav_calls{},invalidations{},copies{},other_devices{},missing_birth{},recursions{},uncertainties{};
thread_local unsigned depth{};
void fail(){failed=true;++uncertainties;if(bank)bank->uncertain();}
template<class F> void attempt(F&& fn){try{fn();}catch(...){fail();}}
struct Call {
    std::unique_lock<std::recursive_mutex> lock{mutex,std::defer_lock};bool entered{},admitted{};
    explicit Call(bool relevant=true){if(!relevant)return;lock.lock();if(!accepting)return;
        entered=true;if(depth++){++recursions;fail();return;}admitted=!failed&&(!bank||!bank->failed());}
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
void arm(){std::lock_guard<std::recursive_mutex> lock(mutex);if(!armed){armed=true;accepting=true;}}
void close(){std::lock_guard<std::recursive_mutex> lock(mutex);accepting=false;}
void release_drained(){
    // Closing admission precedes disabling hooks. Taking this lock drains every
    // admitted original/post-call update; active additionally covers Scope.
    std::unique_ptr<Bank> retired;
    {std::lock_guard<std::recursive_mutex> lock(mutex);
        if(accepting||active.load()||depth)return;
        if(bank){final_loss=static_cast<unsigned>(bank->loss());final_heaps=bank->heaps();final_cells=bank->cells();retired=std::move(bank);}
        released=true;}
    // No observer lock across native Release: a closed passthrough callback
    // may still enter the DLL. It cannot access the detached bank.
    retired.reset();
}
void print(FILE* status){std::lock_guard<std::recursive_mutex> lock(mutex);if(!status)return;
    fprintf(status,"ngx_mv_descriptors enabled=1 admission=%u scope=device_lifetime capture_generation=%llu device=%p device_selection=first_observed_cbv_heap_candidate device_attested=0 world=0 ticket=0 heaps=%zu cells=%zu heap_calls=%llu uav_calls=%llu invalidations=%llu copies=%llu other_devices=%llu missing_birth=%llu recursion=%llu uncertainties=%llu failed=%u loss=%u released=%u selected_pair_proof=0 producer_authorized=0\n",
        accepting?1:0,static_cast<unsigned long long>(lifetime.epoch),reinterpret_cast<void*>(lifetime.device),
        bank?bank->heaps():final_heaps,bank?bank->cells():final_cells,
        static_cast<unsigned long long>(heap_calls),static_cast<unsigned long long>(uav_calls),
        static_cast<unsigned long long>(invalidations),static_cast<unsigned long long>(copies),
        static_cast<unsigned long long>(other_devices),static_cast<unsigned long long>(missing_birth),
        static_cast<unsigned long long>(recursions),static_cast<unsigned long long>(uncertainties),
        failed||(bank?bank->failed():final_loss!=0)?1:0,bank?static_cast<unsigned>(bank->loss()):final_loss,released?1:0);
}
HRESULT STDMETHODCALLTYPE heap(ID3D12Device* device,const D3D12_DESCRIPTOR_HEAP_DESC* desc,REFIID iid,void** output){
    Scope scope;auto frame_guard=frame_lock();Call call;
    D3D12_DESCRIPTOR_HEAP_DESC before{};bool readable=false;
    if(call)readable=safe_copy(&before,desc,sizeof(before));
    const auto result=original_heap(device,desc,iid,output);
    if(call)attempt([&]{
        if(!readable){fail();return;}
        if(before.Type!=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV||FAILED(result))return;
        ++heap_calls;IUnknown* raw{};Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> typed;
        if(!safe_copy(&raw,output,sizeof(raw))||!raw||FAILED(raw->QueryInterface(IID_PPV_ARGS(&typed)))){fail();return;}
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
#else
void arm(){}void close(){}void release_drained(){}void print(FILE*){}
#endif
}
