#include "owned_scene_readback.h"
#include <dxgi1_4.h>
#include <d3d12sdklayers.h>
#include <fstream>
#include <iostream>
#include <stdexcept>

using Microsoft::WRL::ComPtr;
static void require(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
static void check(HRESULT hr,const char* message) {require(SUCCEEDED(hr),message);}
struct GPU {
    ComPtr<ID3D12Device> device; ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12CommandAllocator> allocator; ComPtr<ID3D12GraphicsCommandList> list;
    ComPtr<ID3D12Fence> fence; UINT64 serial{}; bool debug_enabled{};
    GPU() {
        ComPtr<ID3D12Debug> debug;
        if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {debug->EnableDebugLayer();debug_enabled=true;}
        ComPtr<IDXGIFactory4> factory; ComPtr<IDXGIAdapter> warp;
        check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"factory");
        check(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)),"WARP adapter");
        check(D3D12CreateDevice(warp.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)),"WARP device");
        D3D12_COMMAND_QUEUE_DESC q{}; q.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;
        check(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)),"queue");
        check(device->CreateCommandAllocator(q.Type,IID_PPV_ARGS(&allocator)),"allocator");
        check(device->CreateCommandList(0,q.Type,allocator.Get(),nullptr,IID_PPV_ARGS(&list)),"list");
        check(list->Close(),"initial close");
        check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"fence");
    }
    void finish() {
        check(queue->Signal(fence.Get(),++serial),"signal"); const auto deadline=GetTickCount64()+10000;
        while(fence->GetCompletedValue()<serial && GetTickCount64()<deadline)Sleep(1);
        require(fence->GetCompletedValue()!=UINT64_MAX && fence->GetCompletedValue()>=serial,"completion timeout");
    }
    void begin() {finish();check(allocator->Reset(),"reset allocator");check(list->Reset(allocator.Get(),nullptr),"reset list");}
    void submit() {check(list->Close(),"close");ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);finish();}
    void transition(ID3D12Resource* texture,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after) {
        if(before==after)return;D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition.pResource=texture;b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        b.Transition.StateBefore=before;b.Transition.StateAfter=after;list->ResourceBarrier(1,&b);
    }
    void verify() {
        ComPtr<ID3D12InfoQueue> info;if(FAILED(device.As(&info)))return;
        for(UINT64 i=0;i<info->GetNumStoredMessagesAllowedByRetrievalFilter();++i) {
            SIZE_T size{};check(info->GetMessage(i,nullptr,&size),"debug size");std::vector<unsigned char> memory(size);
            auto* message=reinterpret_cast<D3D12_MESSAGE*>(memory.data());check(info->GetMessage(i,message,&size),"debug message");
            if(message->Severity==D3D12_MESSAGE_SEVERITY_ERROR || message->Severity==D3D12_MESSAGE_SEVERITY_CORRUPTION)
                throw std::runtime_error(message->pDescription);
        }
    }
};
static ComPtr<ID3D12Resource> texture(GPU& gpu,UINT width=16,UINT height=8) {
    D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=width;d.Height=height;
    d.DepthOrArraySize=1;d.MipLevels=1;d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;d.SampleDesc.Count=1;
    d.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;D3D12_HEAP_PROPERTIES h{};h.Type=D3D12_HEAP_TYPE_DEFAULT;
    ComPtr<ID3D12Resource> t;check(gpu.device->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&d,
        D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&t)),"texture");return t;
}
static std::wstring output_path(unsigned state) {
    wchar_t temp[MAX_PATH]{};require(GetTempPathW(MAX_PATH,temp)>0,"temp directory");
    return std::wstring(temp)+L"gears_owned_readback_"+std::to_wstring(GetCurrentProcessId())+L"_"+
        std::to_wstring(GetTickCount64())+L"_"+std::to_wstring(state)+L".ppm";
}
static void known_pixels(GPU& gpu,D3D12_RESOURCE_STATES state,unsigned image_height=8) {
    auto t=texture(gpu,16,image_height);ComPtr<ID3D12DescriptorHeap> heap;D3D12_DESCRIPTOR_HEAP_DESC hd{};
    hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;hd.NumDescriptors=1;
    check(gpu.device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap)),"RTV heap");
    auto rtv=heap->GetCPUDescriptorHandleForHeapStart();gpu.device->CreateRenderTargetView(t.Get(),nullptr,rtv);
    gpu.begin();const float red[4]={1,0,0,1}, green[4]={0,1,0,1};
    D3D12_RECT left{0,0,8,LONG(image_height)},right{8,0,16,LONG(image_height)};
    gpu.list->ClearRenderTargetView(rtv,red,1,&left);gpu.list->ClearRenderTargetView(rtv,green,1,&right);
    gpu.transition(t.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,state);gpu.submit();
    const auto path=output_path(static_cast<unsigned>(state));
    owned_scene_readback::Probe probe;
    require(probe.queue(t.Get(),gpu.queue.Get(),state,true,path.c_str()),"readback queue");
    const auto deadline=GetTickCount64()+10000;
    while(probe.result().status==owned_scene_readback::Status::pending && GetTickCount64()<deadline) {probe.poll();Sleep(1);}
    const auto& r=probe.result();require(r.queued&&r.completed&&r.saved,"readback save");
    require(r.width==16&&r.height==image_height&&r.eyes[0].nonblack==8*image_height&&r.eyes[1].nonblack==8*image_height&&r.differing_pixels==8*image_height,"pixel counts");
    require(r.eyes[0].min_rgb==std::array<unsigned char,3>{{255,0,0}}&&r.eyes[0].min_rgb==r.eyes[0].max_rgb,"left red");
    require(r.eyes[1].min_rgb==std::array<unsigned char,3>{{0,255,0}}&&r.eyes[1].min_rgb==r.eyes[1].max_rgb,"right green");
    require(r.eyes[0].hash!=r.eyes[1].hash,"eye hashes");
    {owned_scene_readback::Probe duplicate;
     require(duplicate.queue(t.Get(),gpu.queue.Get(),state,true,path.c_str()),"existing file copy queue");
     const auto until=GetTickCount64()+10000;
     while(duplicate.result().status==owned_scene_readback::Status::pending && GetTickCount64()<until) {duplicate.poll();Sleep(1);}
     require(duplicate.result().completed&&!duplicate.result().saved&&
         duplicate.result().error==owned_scene_readback::Error::file_open,"existing file must not overwrite");}
    std::ifstream ppm(path,std::ios::binary);std::string magic;unsigned width{},height{},max{};
    ppm>>magic>>width>>height>>max;ppm.get();require(magic=="P6"&&width==16&&height==image_height&&max==255,"PPM header");
    std::vector<unsigned char> data(16*image_height*3);ppm.read(reinterpret_cast<char*>(data.data()),data.size());
    require(ppm.gcount()==static_cast<std::streamsize>(data.size())&&ppm.peek()==std::char_traits<char>::eof(),"PPM exact size");
    for(unsigned y=0;y<image_height;++y)for(unsigned x=0;x<16;++x) {auto* p=&data[(y*16+x)*3];require(p[0]==(x<8?255:0)&&p[1]==(x<8?0:255)&&p[2]==0,"PPM pixels");}
    ppm.close();require(DeleteFileW(path.c_str())!=0,"remove test PPM");
    // A subsequent clear uses exactly the original idle state as StateBefore.
    gpu.begin();gpu.transition(t.Get(),state,D3D12_RESOURCE_STATE_RENDER_TARGET);
    const float blue[4]={0,0,1,1};gpu.list->ClearRenderTargetView(rtv,blue,0,nullptr);
    gpu.transition(t.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,state);gpu.submit();gpu.verify();
    require(!probe.queue(t.Get(),gpu.queue.Get(),state,true,path.c_str())&&
        probe.result().error==owned_scene_readback::Error::repeated_queue,"repeated queue rejection");
}
int main() {
    try {
        GPU gpu;auto t=texture(gpu);const auto path=output_path(99);
        {owned_scene_readback::Probe p;require(!p.queue(t.Get(),gpu.queue.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,false,path.c_str()),"uncertified accepted");}
        {owned_scene_readback::Probe p;require(!p.queue(t.Get(),gpu.queue.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,true,path.c_str()),"unsupported state accepted");}
        {owned_scene_readback::Probe p;require(!p.queue(t.Get(),gpu.queue.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,true,L"relative.ppm"),"relative path accepted");}
        {auto odd=texture(gpu,15,8);owned_scene_readback::Probe p;require(!p.queue(odd.Get(),gpu.queue.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,true,path.c_str()),"odd width accepted");}
        {ComPtr<ID3D12CommandQueue> compute;D3D12_COMMAND_QUEUE_DESC q{};q.Type=D3D12_COMMAND_LIST_TYPE_COMPUTE;
         check(gpu.device->CreateCommandQueue(&q,IID_PPV_ARGS(&compute)),"compute queue");owned_scene_readback::Probe p;
         require(!p.queue(t.Get(),compute.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,true,path.c_str()),"compute accepted");}
        for(unsigned state:{0u,4u,128u})for(unsigned height:{8u,7u})known_pixels(gpu,static_cast<D3D12_RESOURCE_STATES>(state),height);
        std::cout<<"PASS: WARP readback RGB/PPM, idle-state restoration (0,4,128), rejection checks; debug layer "
                 <<(gpu.debug_enabled?"enabled":"unavailable")<<"\n";return 0;
    }catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
