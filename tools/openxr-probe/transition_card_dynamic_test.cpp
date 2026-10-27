// Actual TransitionCard + real D3D12 WARP upload/readback. XR symbols below
// are an own-process protocol seam: no loader, XR session, Game or window.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D12
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>
#include "transition_card.h"

XrSession session{};XrSpace view_space{};bool xr_gpu_teardown_unsafe{};
namespace {
using Microsoft::WRL::ComPtr;
unsigned checks{},cases{};
void check(bool okay,char const* what){++checks;if(!okay)throw std::runtime_error(what);}
#define C(value) check(bool(value),#value)
void dx(HRESULT value){if(FAILED(value))throw std::runtime_error("Fixture D3D12 operation failed");}
constexpr unsigned width=transition_card_pixels::width,height=transition_card_pixels::height;
constexpr std::size_t pixel_bytes=std::size_t(width)*height*4;
XrSession owner(){return reinterpret_cast<XrSession>(std::uintptr_t(0x5100));}
XrSpace space(){return reinterpret_cast<XrSpace>(std::uintptr_t(0x5200));}
XrSwapchain handle(){return reinterpret_cast<XrSwapchain>(std::uintptr_t(0x5300));}

struct Gpu {
    ComPtr<ID3D12Device> device;ComPtr<ID3D12CommandQueue> queue;
    Gpu(){ComPtr<IDXGIFactory4> factory;ComPtr<IDXGIAdapter> warp;
        dx(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));dx(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)));
        dx(D3D12CreateDevice(warp.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)));
        D3D12_COMMAND_QUEUE_DESC desc{};desc.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;dx(device->CreateCommandQueue(&desc,IID_PPV_ARGS(&queue)));}
    void flush(){ComPtr<ID3D12Fence> fence;dx(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));dx(queue->Signal(fence.Get(),1));
        HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);C(event!=nullptr);
        const auto hr=fence->SetEventOnCompletion(1,event);const auto waited=SUCCEEDED(hr)?WaitForSingleObject(event,10000):WAIT_FAILED;CloseHandle(event);
        C(SUCCEEDED(hr)&&waited==WAIT_OBJECT_0&&fence->GetCompletedValue()==1);}
    void submit(ID3D12GraphicsCommandList* commands){dx(commands->Close());ID3D12CommandList* lists[]{commands};queue->ExecuteCommandLists(1,lists);flush();}
    void list(ComPtr<ID3D12CommandAllocator>& allocator,ComPtr<ID3D12GraphicsCommandList>& commands){
        dx(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
        dx(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&commands)));}
    void clear(ID3D12Resource* image,DXGI_FORMAT format){ComPtr<ID3D12DescriptorHeap> heap;D3D12_DESCRIPTOR_HEAP_DESC hd{};
        hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;hd.NumDescriptors=1;dx(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap)));
        D3D12_RENDER_TARGET_VIEW_DESC desc{};desc.Format=format;desc.ViewDimension=D3D12_RTV_DIMENSION_TEXTURE2D;
        const auto rtv=heap->GetCPUDescriptorHandleForHeapStart();device->CreateRenderTargetView(image,&desc,rtv);
        ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> commands;list(allocator,commands);
        const float zero[4]{};commands->ClearRenderTargetView(rtv,zero,0,nullptr);submit(commands.Get());}
    std::vector<std::uint8_t> read(ID3D12Resource* image,DXGI_FORMAT format){auto desc=image->GetDesc();desc.Format=format;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout{};UINT rows{};UINT64 row_bytes{},bytes{};device->GetCopyableFootprints(&desc,0,1,0,&layout,&rows,&row_bytes,&bytes);
        C(transition_card_layout::valid_footprint(layout,bytes,format,width,height,rows,row_bytes));
        D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=bytes;buffer.Height=1;
        buffer.DepthOrArraySize=buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_READBACK;ComPtr<ID3D12Resource> result;
        dx(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&result)));
        ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> commands;list(allocator,commands);
        D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition.pResource=image;
        barrier.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_RENDER_TARGET;barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;
        commands->ResourceBarrier(1,&barrier);D3D12_TEXTURE_COPY_LOCATION source{},destination{};
        source.pResource=image;source.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        destination.pResource=result.Get();destination.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;destination.PlacedFootprint=layout;
        commands->CopyTextureRegion(&destination,0,0,0,&source,nullptr);std::swap(barrier.Transition.StateBefore,barrier.Transition.StateAfter);commands->ResourceBarrier(1,&barrier);submit(commands.Get());
        void* mapped{};D3D12_RANGE range{0,std::size_t(bytes)};dx(result->Map(0,&range,&mapped));std::vector<std::uint8_t> rgba(pixel_bytes);
        for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){const auto* p=static_cast<unsigned char const*>(mapped)+layout.Offset+std::size_t(y)*layout.Footprint.RowPitch+x*4;
            auto* out=rgba.data()+(std::size_t(y)*width+x)*4;const bool bgra=menu_swapchain_format::bgra(format);
            out[0]=p[bgra?2:0];out[1]=p[1];out[2]=p[bgra?0:2];out[3]=p[3];}
        D3D12_RANGE none{0,0};result->Unmap(0,&none);return rgba;
    }
};

enum class Fault {none,acquire,acquire_pending,wait,wait_pending,timeout,release,release_pending,destroy,bad_index,owner_during_wait};
struct Runtime {
    Gpu* gpu{};Fault fault{};DXGI_FORMAT format{DXGI_FORMAT_B8G8R8A8_UNORM};bool typeless{},dynamic{true},acquired{},waited{};
    unsigned creates{},acquires{},waits{},releases{},destroys{},next{},index{},last{};XrSwapchainCreateFlags flags{};
    std::vector<ComPtr<ID3D12Resource>> images;
} rt;
// Retained fake-runtime image ownership lasts to process exit, matching the
// card's deliberate retained GPU references. No real XR handles are involved.
std::vector<ComPtr<ID3D12Resource>> retained_images;
void begin(Gpu& gpu,Fault fault=Fault::none,DXGI_FORMAT format=DXGI_FORMAT_B8G8R8A8_UNORM,bool typeless=false,bool dynamic=true){
    for(auto& image:rt.images)retained_images.push_back(image);rt={};rt.gpu=&gpu;rt.fault=fault;rt.format=format;rt.typeless=typeless;rt.dynamic=dynamic;
    session=owner();view_space=space();xr_gpu_teardown_unsafe=false;++cases;
}
std::vector<std::uint8_t> pixels(unsigned seed){std::vector<std::uint8_t> result(pixel_bytes);
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){auto* p=result.data()+(std::size_t(y)*width+x)*4;
        p[0]=std::uint8_t(x*13+y+seed);p[1]=std::uint8_t(x+y*19+seed*3);p[2]=std::uint8_t(x*7+y*11+seed*5);p[3]=std::uint8_t(255-(x+y+seed)%31);}return result;}
bool throws_render(TransitionCard& card){XrCompositionLayerQuad quad{};std::memset(&quad,0x5a,sizeof quad);std::array<unsigned char,sizeof quad> before{};std::memcpy(before.data(),&quad,sizeof quad);
    bool threw=false;try{card.render(quad);}catch(std::runtime_error const&){threw=true;}C(!std::memcmp(before.data(),&quad,sizeof quad));return threw;}
void quad_check(XrCompositionLayerQuad const& quad){C(quad.type==XR_TYPE_COMPOSITION_LAYER_QUAD&&quad.space==view_space&&quad.subImage.swapchain==handle());
    C(quad.pose.position.z==-2&&quad.size.width==1.4f&&quad.size.height==1.4f*height/width);
    C(quad.subImage.imageRect.extent.width==int(width)&&quad.subImage.imageRect.extent.height==int(height));}
} // namespace

extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateSwapchainFormats(XrSession value,std::uint32_t capacity,std::uint32_t* count,std::int64_t* formats){
    C(value==owner()&&count);*count=1;if(capacity){C(formats&&capacity>=1);formats[0]=rt.format;}return XR_SUCCESS;
}
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrCreateSwapchain(XrSession value,XrSwapchainCreateInfo const* info,XrSwapchain* chain){
    C(value==owner()&&info&&chain);++rt.creates;rt.flags=info->createFlags;
    C(info->createFlags==(rt.dynamic?0:XR_SWAPCHAIN_CREATE_STATIC_IMAGE_BIT));
    C(info->usageFlags==(XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT|XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT));
    C(info->format==rt.format&&info->width==width&&info->height==height&&info->sampleCount==1&&info->faceCount==1&&info->arraySize==1&&info->mipCount==1);
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=width;desc.Height=height;
    desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    desc.Format=rt.typeless?(menu_swapchain_format::rgba(rt.format)?DXGI_FORMAT_R8G8B8A8_TYPELESS:DXGI_FORMAT_B8G8R8A8_TYPELESS):rt.format;
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    for(unsigned i=0;i<(rt.dynamic?2u:1u);++i){ComPtr<ID3D12Resource> image;
        dx(rt.gpu->device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&image)));
        rt.gpu->clear(image.Get(),rt.format);rt.images.push_back(image);}
    *chain=handle();return XR_SUCCESS;
}
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateSwapchainImages(XrSwapchain chain,std::uint32_t capacity,std::uint32_t* count,XrSwapchainImageBaseHeader* output){
    C(chain==handle()&&count);*count=std::uint32_t(rt.images.size());if(capacity){C(output&&capacity>=*count);
        auto* images=reinterpret_cast<XrSwapchainImageD3D12KHR*>(output);for(unsigned i=0;i<*count;++i){C(images[i].type==XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR);images[i].texture=rt.images[i].Get();}}return XR_SUCCESS;
}
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrAcquireSwapchainImage(XrSwapchain chain,XrSwapchainImageAcquireInfo const*,std::uint32_t* index){
    C(chain==handle()&&index&&!rt.acquired);++rt.acquires;if(rt.fault==Fault::acquire)return XR_ERROR_RUNTIME_FAILURE;
    rt.acquired=true;rt.waited=false;rt.index=rt.next++%unsigned(rt.images.size());*index=rt.fault==Fault::bad_index?99:rt.index;
    return rt.fault==Fault::acquire_pending?XR_SESSION_LOSS_PENDING:XR_SUCCESS;
}
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrWaitSwapchainImage(XrSwapchain chain,XrSwapchainImageWaitInfo const* info){
    C(chain==handle()&&rt.acquired&&!rt.waited&&info&&info->timeout==1000000000);++rt.waits;
    if(rt.fault==Fault::wait)return XR_ERROR_SESSION_LOST;if(rt.fault==Fault::timeout)return XR_TIMEOUT_EXPIRED;
    rt.waited=true;if(rt.fault==Fault::owner_during_wait)session=reinterpret_cast<XrSession>(std::uintptr_t(0x5400));
    return rt.fault==Fault::wait_pending?XR_SESSION_LOSS_PENDING:XR_SUCCESS;
}
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrReleaseSwapchainImage(XrSwapchain chain,XrSwapchainImageReleaseInfo const*){
    C(chain==handle()&&rt.acquired&&rt.waited&&session==owner());++rt.releases;if(rt.fault==Fault::release)return XR_ERROR_RUNTIME_FAILURE;
    rt.last=rt.index;rt.acquired=rt.waited=false;return rt.fault==Fault::release_pending?XR_SESSION_LOSS_PENDING:XR_SUCCESS;
}
extern "C" XRAPI_ATTR XrResult XRAPI_CALL xrDestroySwapchain(XrSwapchain chain){
    C(chain==handle()&&!rt.acquired&&session==owner());++rt.destroys;if(rt.fault==Fault::destroy)return XR_ERROR_RUNTIME_FAILURE;return XR_SUCCESS;
}

namespace {
void dynamic_updates(Gpu& gpu,DXGI_FORMAT format,bool typeless){begin(gpu,Fault::none,format,typeless);const auto a=pixels(3),b=pixels(17),c=pixels(31);
    {TransitionCard card(gpu.device.Get(),gpu.queue.Get(),a);C(rt.flags==0&&card.uploads==0&&rt.acquires==0);XrCompositionLayerQuad quad{};
        C(card.set_pixels(a)&&rt.acquires==0);C(!card.set_pixels({})&&!card.set_pixels(std::vector<std::uint8_t>(pixel_bytes-1))&&!card.set_pixels(std::vector<std::uint8_t>(pixel_bytes+1)));
        C(card.render(quad)&&card.uploads==1&&rt.acquires==1&&rt.releases==1);quad_check(quad);C(gpu.read(rt.images[rt.last].Get(),format)==a);
        C(card.set_pixels(a)&&card.render(quad)&&card.uploads==1&&rt.acquires==1);
        C(card.set_pixels(b)&&card.set_pixels(a)&&card.render(quad)&&card.uploads==1&&rt.acquires==1); // Cancel pending back to current.
        auto submitted=b;C(card.set_pixels(submitted)&&card.uploads==1&&rt.acquires==1);submitted[0]^=255;
        C(gpu.read(rt.images[rt.last].Get(),format)==a);C(card.render(quad)&&card.uploads==2&&rt.acquires==2&&rt.releases==2);
        C(rt.last==1&&gpu.read(rt.images[rt.last].Get(),format)==b);
        C(card.set_pixels(c)&&card.render(quad)&&card.uploads==3&&rt.acquires==3&&rt.last==0);C(gpu.read(rt.images[rt.last].Get(),format)==c);
        C(card.render(quad)&&card.uploads==3&&rt.acquires==3);
    }C(rt.destroys==1&&!xr_gpu_teardown_unsafe);std::printf("dynamic GPU format=%u typeless=%u: three contents, rotated images, exact RGBA readback passed\n",unsigned(format),unsigned(typeless));
}
void static_cards(Gpu& gpu){for(auto variant:{transition_card_pixels::Variant::returning_to_game,transition_card_pixels::Variant::starting_vr,transition_card_pixels::Variant::tracking_lost}){
    begin(gpu,Fault::none,DXGI_FORMAT_B8G8R8A8_UNORM,false,false);
    {TransitionCard card(gpu.device.Get(),gpu.queue.Get(),variant);XrCompositionLayerQuad quad{};C(!card.set_pixels(pixels(5)));
        for(unsigned i=0;i<3;++i)C(card.render(quad));C(card.uploads==1&&card.rendered==3&&rt.acquires==1&&rt.releases==1);quad_check(quad);
        C(rt.flags==XR_SWAPCHAIN_CREATE_STATIC_IMAGE_BIT&&gpu.read(rt.images[0].Get(),rt.format)==transition_card_pixels::rgba(variant));}
    C(rt.destroys==1&&!xr_gpu_teardown_unsafe);
}
    begin(gpu,Fault::acquire_pending,DXGI_FORMAT_B8G8R8A8_UNORM,false,false);
    {TransitionCard card(gpu.device.Get(),gpu.queue.Get(),transition_card_pixels::Variant::starting_vr);
        C(throws_render(card)&&card.uploads==0);C(rt.flags==XR_SWAPCHAIN_CREATE_STATIC_IMAGE_BIT&&rt.acquires==1&&rt.waits==0&&rt.releases==0&&rt.destroys==0);
        C(gpu.read(rt.images[0].Get(),rt.format)==std::vector<std::uint8_t>(pixel_bytes));}
    C(xr_gpu_teardown_unsafe&&rt.destroys==0);
}
void protocol_fault(Gpu& gpu,Fault fault){begin(gpu,fault);const auto a=pixels(7);
    {TransitionCard card(gpu.device.Get(),gpu.queue.Get(),a);C(throws_render(card));C(card.uploads==0&&!card.set_pixels(pixels(9)));
        XrCompositionLayerQuad quad{};C(!card.render(quad));C(rt.acquires==1);
        if(fault==Fault::acquire){C(rt.waits==0&&rt.releases==0&&rt.destroys==1&&!xr_gpu_teardown_unsafe);}
        else if(fault==Fault::acquire_pending){C(rt.waits==0&&rt.releases==0&&rt.destroys==0&&xr_gpu_teardown_unsafe);}
        else if(fault==Fault::bad_index){C(rt.waits==1&&rt.releases==1&&rt.destroys==1&&!xr_gpu_teardown_unsafe);}
        else{C(rt.waits==1&&rt.destroys==0&&xr_gpu_teardown_unsafe);C(rt.releases==(fault==Fault::release||fault==Fault::release_pending?1u:0u));}
        C(gpu.read(rt.images[rt.index].Get(),rt.format)==(fault==Fault::release||fault==Fault::release_pending?a:std::vector<std::uint8_t>(pixel_bytes)));
    }
    C(rt.destroys==(fault==Fault::acquire||fault==Fault::bad_index?1u:0u));
}
void gpu_timeout(Gpu& gpu){begin(gpu);const auto a=pixels(11),b=pixels(13),c=pixels(15);ComPtr<ID3D12Fence> gate;
    dx(gpu.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&gate)));
    {TransitionCard card(gpu.device.Get(),gpu.queue.Get(),a);XrCompositionLayerQuad quad{};C(card.render(quad)&&card.uploads==1);
        C(card.set_pixels(b));dx(gpu.queue->Wait(gate.Get(),1));bool threw=false;
        struct Unblock {ID3D12Fence* fence;~Unblock(){fence->Signal(1);}} unblock{gate.Get()};
        try{threw=throws_render(card);}catch(...){gate->Signal(1);gpu.flush();throw;}
        // The production 3-second completion deadline actually expires while
        // real GPU work is queued behind this own-process fence.
        C(threw&&xr_gpu_teardown_unsafe&&card.uploads==1&&rt.acquires==2&&rt.releases==1&&rt.destroys==0);
        C(!card.set_pixels(c)&&!card.render(quad));C(rt.acquires==2&&rt.releases==1&&rt.destroys==0);
        dx(gate->Signal(1));gpu.flush();C(gpu.read(rt.images[1].Get(),rt.format)==b);C(gpu.read(rt.images[0].Get(),rt.format)==a);
    }C(rt.releases==1&&rt.destroys==0&&xr_gpu_teardown_unsafe);
}
void lifecycle(Gpu& gpu){begin(gpu);const auto a=pixels(23);bool rejected=false;
    try{TransitionCard bad(gpu.device.Get(),gpu.queue.Get(),std::vector<std::uint8_t>(pixel_bytes-1));}catch(std::runtime_error const&){rejected=true;}
    C(rejected&&rt.creates==0&&rt.acquires==0);
    {TransitionCard card(gpu.device.Get(),gpu.queue.Get(),a);XrCompositionLayerQuad quad{};view_space=XR_NULL_HANDLE;
        C(card.set_pixels(pixels(25))&&!card.render(quad)&&rt.acquires==0);view_space=space();C(card.render(quad)&&card.uploads==1);
        C(gpu.read(rt.images[0].Get(),rt.format)==pixels(25));}C(rt.destroys==1&&!xr_gpu_teardown_unsafe);
    begin(gpu);{TransitionCard card(gpu.device.Get(),gpu.queue.Get(),a);XrCompositionLayerQuad quad{};session=XR_NULL_HANDLE;
        C(!card.set_pixels(pixels(29))&&!card.render(quad));session=owner();C(!card.set_pixels(a)&&!card.render(quad));}
    C(rt.acquires==0&&rt.releases==0&&rt.destroys==0&&xr_gpu_teardown_unsafe);
    begin(gpu,Fault::destroy);{TransitionCard card(gpu.device.Get(),gpu.queue.Get(),a);XrCompositionLayerQuad quad{};C(card.render(quad));}
    C(rt.releases==1&&rt.destroys==1&&xr_gpu_teardown_unsafe);
}
} // namespace

int main(){try{Gpu gpu;
    for(auto format:{DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_B8G8R8A8_UNORM,DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,DXGI_FORMAT_B8G8R8A8_UNORM_SRGB})
        for(bool typeless:{false,true})dynamic_updates(gpu,format,typeless);
    static_cards(gpu);for(auto fault:{Fault::acquire,Fault::acquire_pending,Fault::wait,Fault::wait_pending,Fault::timeout,Fault::release,Fault::release_pending,Fault::bad_index,Fault::owner_during_wait})protocol_fault(gpu,fault);
    gpu_timeout(gpu);lifecycle(gpu);gpu.flush();std::printf("TransitionCardDynamicTest: %u cases %u checks passed; actual class + real WARP GPU; fake XR symbols, no XR session/Game/window; no HMD acceptance\n",cases,checks);return 0;
}catch(std::exception const& error){std::fprintf(stderr,"TransitionCardDynamicTest failed after %u cases/%u checks: %s\n",cases,checks,error.what());return 1;}}
