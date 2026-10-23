#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>
#include "transition_card_layout.h"
#include "menu_pixels.h"
using Microsoft::WRL::ComPtr;
static void checked(HRESULT hr){if(FAILED(hr))throw std::runtime_error("D3D12 menu upload fixture failed");}

static void run(ID3D12Device* device,ID3D12CommandQueue* queue,DXGI_FORMAT format,bool typeless){
    constexpr unsigned width=513,height=3;
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width=width;desc.Height=height;desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;
    desc.Format=typeless?(menu_swapchain_format::rgba(format)?DXGI_FORMAT_R8G8B8A8_TYPELESS:DXGI_FORMAT_B8G8R8A8_TYPELESS):format;
    desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    if(!transition_card_layout::valid_image(desc,format,width,height))throw std::runtime_error("fixture image rejected");
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    ComPtr<ID3D12Resource> image;
    checked(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&image)));
    auto typed=desc;typed.Format=format;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT rows{};UINT64 row_bytes{},bytes{};
    device->GetCopyableFootprints(&typed,0,1,0,&footprint,&rows,&row_bytes,&bytes);
    if(!transition_card_layout::valid_footprint(footprint,bytes,format,width,height,rows,row_bytes))throw std::runtime_error("fixture footprint rejected");
    D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=bytes;buffer.Height=1;
    buffer.DepthOrArraySize=buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ComPtr<ID3D12Resource> upload,readback;
    heap.Type=D3D12_HEAP_TYPE_UPLOAD;
    checked(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&upload)));
    heap.Type=D3D12_HEAP_TYPE_READBACK;
    checked(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback)));
    std::vector<unsigned char> bgra(width*height*4);
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){
        auto* pixel=bgra.data()+(y*width+x)*4;
        pixel[0]=static_cast<unsigned char>(x*17+y);pixel[1]=static_cast<unsigned char>(x+y*33);
        pixel[2]=static_cast<unsigned char>(x*3+y*71);pixel[3]=0;
    }
    void* data{};D3D12_RANGE none{0,0};checked(upload->Map(0,&none,&data));std::memset(data,0,size_t(bytes));
    for(unsigned y=0;y<height;++y)menu_pixels::upload_row(static_cast<unsigned char*>(data)+footprint.Offset+size_t(y)*footprint.Footprint.RowPitch,
        bgra.data()+y*width*4,width,menu_swapchain_format::rgba(format));
    upload->Unmap(0,nullptr);
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> commands;
    checked(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
    checked(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&commands)));
    D3D12_TEXTURE_COPY_LOCATION texture{};texture.pResource=image.Get();texture.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    D3D12_TEXTURE_COPY_LOCATION source{};source.pResource=upload.Get();source.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;source.PlacedFootprint=footprint;
    commands->CopyTextureRegion(&texture,0,0,0,&source,nullptr);
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource=image.Get();barrier.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_DEST;barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;
    commands->ResourceBarrier(1,&barrier);
    auto destination=source;destination.pResource=readback.Get();commands->CopyTextureRegion(&destination,0,0,0,&texture,nullptr);
    checked(commands->Close());ID3D12CommandList* lists[]{commands.Get()};queue->ExecuteCommandLists(1,lists);
    ComPtr<ID3D12Fence> fence;checked(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));
    checked(queue->Signal(fence.Get(),1));HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    if(!event)throw std::runtime_error("fixture fence event failed");
    const auto completion=fence->SetEventOnCompletion(1,event);
    const auto waited=SUCCEEDED(completion)?WaitForSingleObject(event,10000):WAIT_FAILED;CloseHandle(event);
    if(waited!=WAIT_OBJECT_0||fence->GetCompletedValue()!=1)throw std::runtime_error("fixture GPU incomplete");
    D3D12_RANGE range{0,size_t(bytes)};checked(readback->Map(0,&range,&data));unsigned mismatches{};
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){
        const auto* pixel=static_cast<const unsigned char*>(data)+footprint.Offset+size_t(y)*footprint.Footprint.RowPitch+x*4;
        const auto* original=bgra.data()+(y*width+x)*4;const bool rgba=menu_swapchain_format::rgba(format);
        if(pixel[0]!=original[rgba?2:0]||pixel[1]!=original[1]||pixel[2]!=original[rgba?0:2]||pixel[3]!=255)++mismatches;
    }
    readback->Unmap(0,&none);
    if(mismatches)throw std::runtime_error("menu upload GPU byte mismatch");
    std::printf("menu upload: format=%u typeless=%u pixels=%u mismatches=0 no_XR=1 no_game=1\n",unsigned(format),unsigned(typeless),width*height);
}
int main(){
    try{
        ComPtr<IDXGIFactory4> factory;checked(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
        ComPtr<IDXGIAdapter> adapter;checked(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
        ComPtr<ID3D12Device> device;checked(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)));
        ComPtr<ID3D12CommandQueue> queue;D3D12_COMMAND_QUEUE_DESC desc{};checked(device->CreateCommandQueue(&desc,IID_PPV_ARGS(&queue)));
        for(auto format:{DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_B8G8R8A8_UNORM,DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,DXGI_FORMAT_B8G8R8A8_UNORM_SRGB})
            for(bool typeless:{false,true})run(device.Get(),queue.Get(),format,typeless);
        std::puts("menu upload D3D12 WARP checks passed; no compositor/colorimetry/headset acceptance claimed");return 0;
    }catch(std::exception const& error){std::fprintf(stderr,"%s\n",error.what());return 1;}
}
