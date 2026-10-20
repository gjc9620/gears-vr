// CPU-side interface/descriptor query, only while the engine owns bound inputs.
// No copy, barriers, command submission or mutation of resource contents.
struct ResourceIdentity {
    uintptr_t native{},resource{},canonical{};HRESULT result=E_POINTER;
    uintptr_t engine_wrapper{};uint32_t engine_state{};unsigned char engine_flags{};bool engine_state_read{};
    D3D12_RESOURCE_DESC desc{};
};
bool system_resource_methods(uintptr_t native){
    uintptr_t table{};std::array<uintptr_t,3> methods{};
    if(!native||!safe_copy(&table,reinterpret_cast<void*>(native),8)||!table||
       !safe_copy(methods.data(),reinterpret_cast<void*>(table),sizeof(methods)))return false;
    HMODULE core=GetModuleHandleW(L"D3D12Core.dll"),api=GetModuleHandleW(L"d3d12.dll");
    for(auto method:methods){HMODULE owner{};
        if(!method||!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(method),&owner)||(!core&&!api)||(owner!=core&&owner!=api))return false;
    }
    return true;
}
void query_native_resource(uintptr_t native,ResourceIdentity& identity){
    identity.native=native;
    if(!system_resource_methods(native)){identity.result=E_INVALIDARG;return;}
    ID3D12Resource* resource{};
    identity.result=reinterpret_cast<IUnknown*>(native)->QueryInterface(IID_PPV_ARGS(&resource));
    if(FAILED(identity.result)||!resource)return;
    identity.resource=reinterpret_cast<uintptr_t>(resource);identity.desc=resource->GetDesc();
    identity.canonical=canonical_identity(resource);
    resource->Release();
}
void query_engine_resource(uintptr_t wrapper,ResourceIdentity& identity){
    uintptr_t wrapper_table{},native{};
    if(!wrapper||!safe_copy(&wrapper_table,reinterpret_cast<void*>(wrapper),8)||wrapper_table!=base+0x133ecf8||
       !safe_copy(&native,reinterpret_cast<void*>(wrapper+0xc),8))return;
    identity.engine_wrapper=wrapper;
    // 1c60f0 reads +80 as the engine's tracked state; +84 bit 0 chooses
    // another transition path. These fields alone do not prove GPU state.
    identity.engine_state_read=safe_copy(&identity.engine_state,reinterpret_cast<void*>(wrapper+0x80),4)&&
        safe_copy(&identity.engine_flags,reinterpret_cast<void*>(wrapper+0x84),1);
    query_native_resource(native,identity);
}
void query_bound_resource(uintptr_t object,ResourceIdentity& identity){
    uintptr_t table{},wrapper{};
    if(!object||!safe_copy(&table,reinterpret_cast<void*>(object),8)||table!=base+0x1667bb8||
       !safe_copy(&wrapper,reinterpret_cast<void*>(object+0x60),8))return;
    query_engine_resource(wrapper,identity);
}
void query_public_resource(uintptr_t public_pointer,ResourceIdentity& identity){
    uintptr_t owner{},wrapper{};
    // Native source chain used by 1c5dd8, gated again by engine/system vtables.
    if(public_pointer<0x13c||!safe_copy(&owner,reinterpret_cast<void*>(public_pointer-0x130),8)||!owner||
       !safe_copy(&wrapper,reinterpret_cast<void*>(owner+0xc),8))return;
    query_engine_resource(wrapper,identity);
}
bool resource_identity_test(){
    ID3D12Device* device{};ID3D12Resource* resource{};
    if(FAILED(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))))return false;
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width=17;desc.Height=9;desc.DepthOrArraySize=1;desc.MipLevels=1;
    desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;
    HRESULT result=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&resource));
    bool ok=false;
    if(SUCCEEDED(result)){
        ResourceIdentity identity{};query_native_resource(reinterpret_cast<uintptr_t>(resource),identity);
        ok=identity.result==S_OK&&identity.resource==reinterpret_cast<uintptr_t>(resource)&&
            identity.canonical&&identity.canonical==canonical_identity(resource)&&
            identity.desc.Width==17&&identity.desc.Height==9&&identity.desc.Format==desc.Format;
        ResourceIdentity rejected{};query_native_resource(1,rejected);ok=ok&&rejected.result==E_INVALIDARG;
        std::array<unsigned char,0x90> wrapper{};uintptr_t table=base+0x133ecf8,native=reinterpret_cast<uintptr_t>(resource);
        uint32_t state=128;memcpy(wrapper.data(),&table,8);memcpy(wrapper.data()+0xc,&native,8);memcpy(wrapper.data()+0x80,&state,4);
        ResourceIdentity packed{};query_engine_resource(reinterpret_cast<uintptr_t>(wrapper.data()),packed);
        ok=ok&&packed.result==S_OK&&packed.resource==native&&packed.engine_state_read&&packed.engine_state==128&&packed.engine_flags==0;
        table=0;memcpy(wrapper.data(),&table,8);ResourceIdentity bad_wrapper{};
        query_engine_resource(reinterpret_cast<uintptr_t>(wrapper.data()),bad_wrapper);
        ok=ok&&bad_wrapper.result==E_POINTER&&!bad_wrapper.native;
        resource->Release();
    }
    device->Release();return ok;
}
