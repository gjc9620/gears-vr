// Independent D3D12 fixture, no game/OpenXR session and no screenshots.
#include "eye_chain_policy.h"
#include "../vr-common/gpu_blit.h"
#include <cstdio>
#include <cstdlib>
#include <memory>

using namespace gears_vr;
static D3D12_RESOURCE_DESC buffer_desc(UINT64 bytes) {
    D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=bytes;
    d.Height=d.DepthOrArraySize=d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;return d;
}
static ComPtr<ID3D12Resource> resource(ID3D12Device* device,D3D12_RESOURCE_DESC const& desc,
    D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state) {
    D3D12_HEAP_PROPERTIES heap{};heap.Type=type;ComPtr<ID3D12Resource> value;
    gpu_check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&value)),"fixture resource");return value;
}
static void run(ID3D12Device* device,ID3D12CommandQueue* queue,unsigned size,DXGI_FORMAT format) {
    gears_eye_chain::Limits limits{4096,4096,{4096,4096},{4096,4096},{1,1}};
    gears_eye_chain::Plan plan{};
    if(!gears_eye_chain::make_plan(size*2,size,limits,plan))throw std::runtime_error("fixture size policy rejected");
    StereoImageTicket ticket{};ticket.id=1;ticket.rect[0]=plan.crops[0];ticket.rect[1]=plan.crops[1];
    if(!gears_eye_chain::ticket_crops_match(plan,ticket))throw std::runtime_error("fixture crop policy rejected");
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width=size*2;desc.Height=size;desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    auto source=resource(device,desc,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST);
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT upload_layout{};UINT64 upload_bytes{};
    device->GetCopyableFootprints(&desc,0,1,0,&upload_layout,nullptr,nullptr,&upload_bytes);
    auto upload=resource(device,buffer_desc(upload_bytes),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);
    void* mapped{};D3D12_RANGE none{0,0};gpu_check(upload->Map(0,&none,&mapped),"fixture upload map");
    for(unsigned y=0;y<size;++y)for(unsigned x=0;x<size*2;++x) {
        auto* pixel=static_cast<unsigned char*>(mapped)+upload_layout.Offset+UINT64(y)*upload_layout.Footprint.RowPitch+x*4;
        pixel[0]=x<size?17:213;pixel[1]=static_cast<unsigned char>(x%251);
        pixel[2]=static_cast<unsigned char>(y%239);pixel[3]=255;
    }
    upload->Unmap(0,nullptr);
    std::array<ComPtr<ID3D12Resource>,2> destinations;
    std::array<std::unique_ptr<GpuBlit>,2> conversions;
    desc.Width=size;desc.Format=format;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    const bool convert=format!=DXGI_FORMAT_R8G8B8A8_UNORM;
    for(unsigned eye=0;eye<2;++eye) {
        destinations[eye]=resource(device,desc,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_RENDER_TARGET);
        if(convert)conversions[eye]=std::make_unique<GpuBlit>(device,format);
    }
    // 25 pixels per eye, independently selected corners/edges/interior. GPU
    // readback is only 25 KiB even when each eye image is 4096 squared.
    constexpr unsigned samples=50;
    auto readback=resource(device,buffer_desc(samples*512),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> commands;
    gpu_check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"fixture allocator");
    gpu_check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&commands)),"fixture list");
    D3D12_TEXTURE_COPY_LOCATION to{};to.pResource=source.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    D3D12_TEXTURE_COPY_LOCATION from{};from.pResource=upload.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;from.PlacedFootprint=upload_layout;
    commands->CopyTextureRegion(&to,0,0,0,&from,nullptr);
    const auto source_state=convert?D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:D3D12_RESOURCE_STATE_COPY_SOURCE;
    gpu_barrier(commands.Get(),source.Get(),D3D12_RESOURCE_STATE_COPY_DEST,source_state);
    const unsigned positions[]={0,1,size/2,size-2,size-1};
    for(unsigned eye=0;eye<2;++eye) {
        auto* destination=destinations[eye].Get();
        if(convert)conversions[eye]->draw_crop(commands.Get(),source.Get(),destination,eye*size,0);
        else {
            gpu_barrier(commands.Get(),destination,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_DEST);
            to={};to.pResource=destination;to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            from={};from.pResource=source.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            D3D12_BOX box{eye*size,0,0,(eye+1)*size,size,1};commands->CopyTextureRegion(&to,0,0,0,&from,&box);
            gpu_barrier(commands.Get(),destination,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_RENDER_TARGET);
        }
        gpu_barrier(commands.Get(),destination,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE);
        for(unsigned y=0;y<5;++y)for(unsigned x=0;x<5;++x) {
            const unsigned ordinal=eye*25+y*5+x;
            to={};to.pResource=readback.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            to.PlacedFootprint.Offset=UINT64(ordinal)*512;to.PlacedFootprint.Footprint={format,1,1,1,256};
            from={};from.pResource=destination;from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            D3D12_BOX box{positions[x],positions[y],0,positions[x]+1,positions[y]+1,1};
            commands->CopyTextureRegion(&to,0,0,0,&from,&box);
        }
    }
    gpu_check(commands->Close(),"fixture close");ComPtr<ID3D12Fence> fence;
    gpu_check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"fixture fence");
    HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event)throw std::runtime_error("fixture event");
    ID3D12CommandList* lists[]={commands.Get()};queue->ExecuteCommandLists(1,lists);
    // On unconfirmed GPU work, terminate this standalone fixture without
    // unwinding and freeing resources that may still be referenced by commands.
    if(FAILED(queue->Signal(fence.Get(),1))||FAILED(fence->SetEventOnCompletion(1,event))||
       WaitForSingleObject(event,30000)!=WAIT_OBJECT_0||fence->GetCompletedValue()!=1) {
        std::fputs("fixture GPU completion unconfirmed\n",stderr);std::_Exit(3);
    }
    CloseHandle(event);D3D12_RANGE range{0,samples*512};gpu_check(readback->Map(0,&range,&mapped),"fixture readback map");
    unsigned mismatches=0;
    for(unsigned eye=0;eye<2;++eye)for(unsigned y=0;y<5;++y)for(unsigned x=0;x<5;++x) {
        auto* pixel=static_cast<unsigned char*>(mapped)+(eye*25+y*5+x)*512;
        unsigned expected[4]={eye?213u:17u,(eye*size+positions[x])%251,positions[y]%239,255};
        if(convert)std::swap(expected[0],expected[2]);
        const int tolerance=format==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB?1:0;
        for(unsigned channel=0;channel<4;++channel)if(std::abs(int(pixel[channel])-int(expected[channel]))>tolerance)++mismatches;
    }
    readback->Unmap(0,&none);
    if(mismatches)throw std::runtime_error("fixture pixel mismatches: "+std::to_string(mismatches));
    std::printf("eye crop fixture: source=%ux%u eye=%ux%u format=%u pixels=50 mismatches=0 native_game_pixels=0\n",size*2,size,size,size,unsigned(format));
}
int main() {
    try {
        ComPtr<IDXGIFactory4> factory;gpu_check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"fixture factory");
        ComPtr<ID3D12Device> device;bool warp=false;
        if(FAILED(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)))) {
            ComPtr<IDXGIAdapter> adapter;gpu_check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)),"fixture WARP");
            gpu_check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)),"fixture device");warp=true;
        }
        ComPtr<ID3D12CommandQueue> queue;D3D12_COMMAND_QUEUE_DESC desc{};
        gpu_check(device->CreateCommandQueue(&desc,IID_PPV_ARGS(&queue)),"fixture queue");
        const auto luid=device->GetAdapterLuid();std::printf("independent D3D12 crop fixture: warp=%u adapter=%08x:%08x no_XR=1 no_game=1\n",unsigned(warp),unsigned(luid.HighPart),luid.LowPart);
        for(unsigned size:{2048u,4096u})for(auto format:{DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_B8G8R8A8_UNORM,DXGI_FORMAT_B8G8R8A8_UNORM_SRGB})run(device.Get(),queue.Get(),size,format);
        return 0;
    }catch(std::exception const& error){std::fprintf(stderr,"%s\n",error.what());return 1;}
}
