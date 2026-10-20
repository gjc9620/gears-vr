#include "ngx_typed_resources.h"
#include "ngx_typed_copy.h"
#include "ngx_stereo_native_api.h"
#include <dxgi1_4.h>
#include <atomic>
#include <cstdio>
#include <stdexcept>
using Microsoft::WRL::ComPtr;
using namespace gears_ngx_stereo;
static unsigned checks{};
static void check(bool value){++checks;if(!value)throw std::runtime_error("typed resource contract rejected");}
static void hr(HRESULT value){check(SUCCEEDED(value));}
struct Witness final:IUnknown {
    std::atomic<ULONG> references{1};bool* destroyed;
    explicit Witness(bool& flag):destroyed(&flag){}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out) override {
        if(!out)return E_POINTER;*out=nullptr;if(id!=IID_IUnknown)return E_NOINTERFACE;
        *out=static_cast<IUnknown*>(this);AddRef();return S_OK;}
    ULONG STDMETHODCALLTYPE AddRef() override{return ++references;}
    ULONG STDMETHODCALLTYPE Release() override{const auto n=--references;if(!n){*destroyed=true;delete this;}return n;}
};
static ComPtr<ID3D12Resource> make(ID3D12Device* device,DXGI_FORMAT format,unsigned mips=1,bool rt=false){
    D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=128;d.Height=64;
    d.DepthOrArraySize=1;d.MipLevels=UINT16(mips);d.Format=format;d.SampleDesc.Count=1;
    if(rt)d.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;ComPtr<ID3D12Resource> result;
    hr(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&result)));return result;
}
static unsigned copied{};
static void STDMETHODCALLTYPE original_copy(ID3D12GraphicsCommandList* list,D3D12_TEXTURE_COPY_LOCATION const* dst,UINT x,UINT y,UINT z,
    D3D12_TEXTURE_COPY_LOCATION const* src,D3D12_BOX const* box){++copied;list->CopyTextureRegion(dst,x,y,z,src,box);}
static void barrier(ID3D12GraphicsCommandList* list,ID3D12Resource* resource,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){
    D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=resource;
    b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;b.Transition.StateBefore=before;b.Transition.StateAfter=after;list->ResourceBarrier(1,&b);
}
static void typed_copy_gpu(ID3D12Device* device){
    auto src=make(device,DXGI_FORMAT_R8G8B8A8_UNORM,1,true),dst=make(device,DXGI_FORMAT_R8G8B8A8_UNORM,1,true);
    ComPtr<ID3D12CommandQueue> queue;ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;ComPtr<ID3D12Fence> fence;
    D3D12_COMMAND_QUEUE_DESC q{};hr(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)));
    hr(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
    hr(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));
    hr(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));
    D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;hd.NumDescriptors=2;ComPtr<ID3D12DescriptorHeap> heap;
    hr(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap)));auto a=heap->GetCPUDescriptorHandleForHeapStart(),b=a;b.ptr+=device->GetDescriptorHandleIncrementSize(hd.Type);
    device->CreateRenderTargetView(src.Get(),nullptr,a);device->CreateRenderTargetView(dst.Get(),nullptr,b);
    barrier(list.Get(),src.Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_RENDER_TARGET);
    barrier(list.Get(),dst.Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_RENDER_TARGET);
    const float red[]{1,0,0,1},blue[]{0,0,1,1};list->ClearRenderTargetView(a,red,0,nullptr);list->ClearRenderTargetView(b,blue,0,nullptr);
    barrier(list.Get(),src.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE);
    barrier(list.Get(),dst.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_DEST);
    D3D12_TEXTURE_COPY_LOCATION s{},d{};s.pResource=src.Get();s.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;d=s;d.pResource=dst.Get();
    ComPtr<IUnknown> identity;hr(list.As(&identity));const auto canonical=reinterpret_cast<uintptr_t>(identity.Get());uint64_t generation=7;
    const auto current=[](void* value,ID3D12GraphicsCommandList*,uintptr_t,uint64_t expected){return *static_cast<uint64_t*>(value)==expected;};
    TypedCopyBoundary boundary(list.Get(),canonical,7,10,1,d,s,&generation,current,original_copy);
    auto source=TypedResourceLease::observe(src.Get()),target=TypedResourceLease::observe(dst.Get());
    CopyPlan plan{{0,0,0,64,64,1},{64,0,0},source.canonical,target.canonical,canonical,7,10,1};
    for(unsigned fault=0;fault<9;++fault){auto wrong=plan;
        if(fault==0)++wrong.generation;if(fault==1)++wrong.list;if(fault==2)++wrong.ticket;if(fault==3)wrong.ordinal=0;
        if(fault==4)++wrong.source;if(fault==5)++wrong.target;if(fault==6)wrong.destination[0]=65;
        if(fault==7)wrong.source_box[5]=2;if(fault==8)wrong.source_box[0]=1;
        check(!boundary.copy(wrong)&&copied==0);}
    generation=8;check(!boundary.copy(plan)&&copied==0);generation=7;
    NativeDispatchFunctions functions{};functions.copy_context=&boundary;functions.copy=TypedCopyBoundary::dispatch;
    NativeDispatchBackend backend(functions);backend.copy(plan);check(copied==1);check(!boundary.copy(plan)&&copied==1);
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};UINT64 bytes{};const auto desc=dst->GetDesc();device->GetCopyableFootprints(&desc,0,1,0,&fp,nullptr,nullptr,&bytes);
    D3D12_RESOURCE_DESC rb{};rb.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rb.Width=bytes;rb.Height=1;rb.DepthOrArraySize=1;rb.MipLevels=1;rb.SampleDesc.Count=1;rb.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_READBACK;ComPtr<ID3D12Resource> readback;
    hr(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rb,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback)));
    barrier(list.Get(),dst.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_COPY_SOURCE);
    D3D12_TEXTURE_COPY_LOCATION read{};read.pResource=readback.Get();read.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;read.PlacedFootprint=fp;
    list->CopyTextureRegion(&read,0,0,0,&d,nullptr);hr(list->Close());ID3D12CommandList* commands[]{list.Get()};queue->ExecuteCommandLists(1,commands);
    const auto signal=queue->Signal(fence.Get(),1);const auto deadline=GetTickCount64()+5000;
    while(SUCCEEDED(signal)&&fence->GetCompletedValue()<1&&GetTickCount64()<deadline)Sleep(1);
    if(FAILED(signal)||fence->GetCompletedValue()!=1){
        // An uncertain completion must not release the last GPU-used references.
        src.Detach();dst.Detach();list.Detach();allocator.Detach();queue.Detach();fence.Detach();heap.Detach();readback.Detach();
        throw std::runtime_error("typed copy GPU completion unavailable; resources quarantined");}
    check(true);
    void* data{};D3D12_RANGE range{0,SIZE_T(bytes)};hr(readback->Map(0,&range,&data));bool pixels=true;
    for(unsigned y=0;y<64;++y)for(unsigned x=0;x<128;++x){auto* p=static_cast<unsigned char*>(data)+size_t(y)*fp.Footprint.RowPitch+x*4;
        if(p[0]!=(x>=64?255:0)||p[1]!=0||p[2]!=(x<64?255:0)||p[3]!=255)pixels=false;}
    D3D12_RANGE empty{};readback->Unmap(0,&empty);check(pixels); // Right copied; left remained untouched.
}
int main(){try{
    ComPtr<ID3D12Device> device;hr(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)));
    const DXGI_FORMAT formats[]{DXGI_FORMAT_R11G11B10_FLOAT,DXGI_FORMAT_R11G11B10_FLOAT,DXGI_FORMAT_R32_FLOAT,DXGI_FORMAT_R16G16_FLOAT};
    std::array<bool,4> destroyed{};std::array<TypedResourceLease,4> native{};std::array<uintptr_t,4> raw{};
    static const GUID key={0x5699a542,0x4090,0x42b7,{0xaf,0x24,0xe9,0x41,0xa7,0xb2,0x6c,0x56}};
    for(unsigned i=0;i<4;++i){auto resource=make(device.Get(),formats[i]);auto* witness=new Witness(destroyed[i]);
        const auto result=resource->SetPrivateDataInterface(key,witness);witness->Release();hr(result);
        D3D12_RESOURCE_BARRIER boundary{};boundary.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;boundary.Transition.pResource=resource.Get();
        native[i]=TypedResourceLease::observe(boundary.Transition.pResource);raw[i]=reinterpret_cast<uintptr_t>(resource.Get());
        check(native[i].retained()&&native[i].get()==resource.Get());resource.Reset();check(!destroyed[i]);}
    std::array<Resource,4> resolved{};check(resolve_typed_resources(native,raw,resolved));
    check(resolved[0].identity==native[0].canonical&&resolved[1].identity==native[2].canonical&&
        resolved[2].identity==native[3].canonical&&resolved[3].identity==native[1].canonical);
    check(resolved[1].format==DXGI_FORMAT_R32_FLOAT&&resolved[2].format==DXGI_FORMAT_R16G16_FLOAT);
    const auto original=resolved;auto unknown=raw;unknown[2]=1;
    check(!resolve_typed_resources(native,unknown,resolved)&&resolved[0].identity==original[0].identity); // 1 is never dereferenced.
    auto alias=native;alias[2]=alias[0];auto alias_raw=raw;alias_raw[2]=raw[0];check(!resolve_typed_resources(alias,alias_raw,resolved));alias={};
    auto missing=native;missing[3].release();check(!resolve_typed_resources(missing,raw,resolved));missing={};
    auto mip=make(device.Get(),DXGI_FORMAT_R16G16_FLOAT,2);auto wrong=native;wrong[3]=TypedResourceLease::observe(mip.Get());
    auto wrong_raw=raw;wrong_raw[3]=wrong[3].raw;check(!resolve_typed_resources(wrong,wrong_raw,resolved));wrong={};mip.Reset();
    ComPtr<IDXGIFactory4> factory;ComPtr<IDXGIAdapter> warp;ComPtr<ID3D12Device> foreign_device;
    hr(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));hr(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)));
    hr(D3D12CreateDevice(warp.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&foreign_device)));
    auto foreign=make(foreign_device.Get(),DXGI_FORMAT_R16G16_FLOAT);wrong=native;wrong[3]=TypedResourceLease::observe(foreign.Get());
    wrong_raw[3]=wrong[3].raw;check(!resolve_typed_resources(wrong,wrong_raw,resolved));wrong={};foreign.Reset();
    // Simulate ticket-bank rotation while an Eval row still owns all four leases.
    auto row=native;native={};for(bool flag:destroyed)check(!flag);
    check(resolve_typed_resources(row,raw,resolved));
    for(auto& resource:row)resource.release();for(bool flag:destroyed)check(flag);
    check(!resolve_typed_resources(row,raw,resolved));check(!TypedResourceLease::observe(nullptr).retained());
    typed_copy_gpu(device.Get());
    std::printf("NgxTypedResourcesTest: %u checks passed (real D3D12 ownership and boxed-copy pixels; no SDK/producer-domain claim)\n",checks);return 0;
}catch(std::exception const& e){std::fprintf(stderr,"NgxTypedResourcesTest: %s\n",e.what());return 1;}}
