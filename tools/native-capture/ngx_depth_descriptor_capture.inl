// CPU DSV provenance, using the existing RTV descriptor-ledger policy.
// This does not certify depth writes, pipeline state, or GPU contents.
namespace ngx_depth_capture {
#if defined(GEARS_NGX_MV_DESCRIPTOR_OBSERVER) && GEARS_NGX_MV_DESCRIPTOR_OBSERVER
using CreateFn=void(STDMETHODCALLTYPE*)(ID3D12Device*,ID3D12Resource*,D3D12_DEPTH_STENCIL_VIEW_DESC const*,D3D12_CPU_DESCRIPTOR_HANDLE);
CreateFn original{};gears_viewport::Tracker descriptors,all_descriptors;
uint64_t creates{},lookups{},copy_calls{},copy_rejections{};
void STDMETHODCALLTYPE create(ID3D12Device* device,ID3D12Resource* resource,D3D12_DEPTH_STENCIL_VIEW_DESC const* view,D3D12_CPU_DESCRIPTOR_HANDLE handle){
    Scope scope;auto guard=frame_lock();original(device,resource,view,handle);
    if(!recording.load()&&!prime_recording.load())return;
    gears_viewport::Target target;D3D12_DEPTH_STENCIL_VIEW_DESC description{};
    bool valid=!view||safe_copy(&description,view,sizeof(description));unsigned mip{};
    if(view&&valid){
        if(description.ViewDimension==D3D12_DSV_DIMENSION_TEXTURE2D)mip=description.Texture2D.MipSlice;
        else if(description.ViewDimension==D3D12_DSV_DIMENSION_TEXTURE2DARRAY){mip=description.Texture2DArray.MipSlice;valid=description.Texture2DArray.FirstArraySlice==0&&description.Texture2DArray.ArraySize==1;}
        else valid=false;}
    if(resource&&valid){auto desc=resource->GetDesc();
        if(desc.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&desc.DepthOrArraySize==1&&desc.SampleDesc.Count==1&&
           (desc.Flags&D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)&&mip<desc.MipLevels&&mip<32){
            target={canonical_identity(resource),std::max<uint64_t>(1,desc.Width>>mip),std::max<UINT>(1,desc.Height>>mip),unsigned(view?description.Format:desc.Format),mip};}}
    all_descriptors.create(handle.ptr,target,final_eye_inventory.epoch,reinterpret_cast<uintptr_t>(resource));
    if(view&&(description.Flags&D3D12_DSV_FLAG_READ_ONLY_DEPTH))target={};
    descriptors.create(handle.ptr,target,final_eye_inventory.epoch,reinterpret_cast<uintptr_t>(resource));++creates;
}
gears_viewport::Target lookup(uint64_t handle){++lookups;return descriptors.descriptor_capacity_lost()?gears_viewport::Target{}:descriptors.lookup(handle);}
gears_viewport::Target lookup_any(uint64_t handle){return all_descriptors.descriptor_capacity_lost()?gears_viewport::Target{}:all_descriptors.lookup(handle);}
void copy(uintptr_t const* destination,uintptr_t const* source,unsigned count){++copy_calls;descriptors.copy(destination,source,count,final_eye_inventory.epoch);all_descriptors.copy(destination,source,count,final_eye_inventory.epoch);}
void reject_copy(){++copy_rejections;descriptors.reject_copy();all_descriptors.reject_copy();}
void print(FILE* file){fprintf(file,"ngx_depth_descriptors creates=%llu lookups=%llu copy_calls=%llu copy_rejections=%llu capacity_lost=%u cpu_provenance_only=1 depth_write_verified=0 gpu_verified=0\n",creates,lookups,copy_calls,copy_rejections,unsigned(descriptors.descriptor_capacity_lost()));}
DWORD native_check(ID3D12Device* device){
    gears_vr::ComPtr<ID3D12DescriptorHeap> heap;gears_vr::ComPtr<ID3D12Resource> resource;
    D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_DSV;hd.NumDescriptors=2;
    if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap))))return 370;
    D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;rd.Width=rd.Height=16;
    rd.DepthOrArraySize=rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Format=DXGI_FORMAT_D32_FLOAT;rd.Flags=D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    D3D12_CLEAR_VALUE clear{};clear.Format=rd.Format;clear.DepthStencil.Depth=1;
    if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_DEPTH_WRITE,&clear,IID_PPV_ARGS(&resource))))return 371;
    auto first=heap->GetCPUDescriptorHandleForHeapStart(),second=first;second.ptr+=device->GetDescriptorHandleIncrementSize(hd.Type);
    D3D12_DEPTH_STENCIL_VIEW_DESC view{};view.Format=rd.Format;view.ViewDimension=D3D12_DSV_DIMENSION_TEXTURE2D;
    device->CreateDepthStencilView(resource.Get(),&view,first);auto current=lookup(first.ptr);
    if(current.resource!=canonical_identity(resource.Get())||current.width!=16||current.height!=16||current.mip)return 372;
    device->CopyDescriptorsSimple(1,second,first,hd.Type);if(!(lookup(second.ptr)==current))return 373;
    view.Flags=D3D12_DSV_FLAG_READ_ONLY_DEPTH;device->CreateDepthStencilView(resource.Get(),&view,first);
    if(lookup(first.ptr).resource||!(lookup(second.ptr)==current)||!(lookup_any(first.ptr)==current))return 374;
    const UINT count=1;device->CopyDescriptors(1,&second,&count,1,&first,&count,hd.Type);
    if(lookup(second.ptr).resource||!(lookup_any(second.ptr)==current))return 375;
    view.Flags=D3D12_DSV_FLAG_NONE;view.ViewDimension=D3D12_DSV_DIMENSION_TEXTURE2DARRAY;view.Texture2DArray.ArraySize=1;
    device->CreateDepthStencilView(resource.Get(),&view,first);if(!(lookup(first.ptr)==current))return 376;
    return 0;
}
#endif
}
