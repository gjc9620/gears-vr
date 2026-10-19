#ifndef GEARS_DESKTOP_MIRROR_READBACK
#define GEARS_DESKTOP_MIRROR_READBACK 1
#endif
#include "../vr-common/gpu_mirror.h"
#include <d3d12sdklayers.h>
#include <cstdio>
#include <vector>
#include <stdexcept>
using namespace gears_vr;
static unsigned checks{};
static void require(bool ok,const char* message){++checks;if(!ok)throw std::runtime_error(message);}
#if GEARS_DESKTOP_MIRROR_READBACK
static std::vector<unsigned char> read_completed(DesktopMirror::Readback const& receipt){
    require(receipt.buffer!=nullptr,"worker received readback ownership");
    const auto width=static_cast<unsigned>(receipt.destination.Width),height=receipt.destination.Height;
    std::vector<unsigned char> rgb(static_cast<size_t>(width)*height*3);void* mapped{};
    D3D12_RANGE range{0,static_cast<SIZE_T>(receipt.bytes)};gpu_check(receipt.buffer->Map(0,&range,&mapped),"completed readback map");
    const auto format=receipt.destination.Format;const bool bgra=format==DXGI_FORMAT_B8G8R8A8_UNORM||format==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){
        const auto* pixel=static_cast<unsigned char*>(mapped)+receipt.footprint.Offset+size_t(y)*receipt.footprint.Footprint.RowPitch+x*4;
        auto* out=rgb.data()+(size_t(y)*width+x)*3;
        if(format==DXGI_FORMAT_R10G10B10A2_UNORM){uint32_t packed{};memcpy(&packed,pixel,4);
            for(unsigned c=0;c<3;++c)out[c]=static_cast<unsigned char>((((packed>>(c*10))&1023)*255+511)/1023);
        }else{out[0]=pixel[bgra?2:0];out[1]=pixel[1];out[2]=pixel[bgra?0:2];}
    }
    D3D12_RANGE written{0,0};receipt.buffer->Unmap(0,&written);return rgb;
}
#endif
struct GPU {
    ComPtr<ID3D12Device> device;ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;
    ComPtr<ID3D12Fence> fence;uint64_t serial{};HANDLE event{};
    GPU(){
        ComPtr<ID3D12Debug> debug;if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))debug->EnableDebugLayer();
        ComPtr<IDXGIFactory4> factory;ComPtr<IDXGIAdapter> warp;
        gpu_check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"factory");gpu_check(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)),"WARP");
        gpu_check(D3D12CreateDevice(warp.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)),"device");
        D3D12_COMMAND_QUEUE_DESC q{};gpu_check(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)),"queue");
        gpu_check(device->CreateCommandAllocator(q.Type,IID_PPV_ARGS(&allocator)),"allocator");
        gpu_check(device->CreateCommandList(0,q.Type,allocator.Get(),nullptr,IID_PPV_ARGS(&list)),"list");gpu_check(list->Close(),"close");
        gpu_check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"fence");event=CreateEventW(nullptr,FALSE,FALSE,nullptr);require(event!=nullptr,"event");
    }
    ~GPU(){if(event)CloseHandle(event);}
    void finish(){gpu_check(queue->Signal(fence.Get(),++serial),"finish signal");
        if(fence->GetCompletedValue()<serial){gpu_check(fence->SetEventOnCompletion(serial,event),"finish event");require(WaitForSingleObject(event,10000)==WAIT_OBJECT_0,"GPU wait");}}
    void begin(){finish();gpu_check(allocator->Reset(),"reset");gpu_check(list->Reset(allocator.Get(),nullptr),"reset list");}
    void submit(){gpu_check(list->Close(),"close");ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);finish();}
    ComPtr<ID3D12Resource> texture(unsigned width,unsigned height,DXGI_FORMAT format,D3D12_RESOURCE_STATES state){
        D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=width;d.Height=height;d.DepthOrArraySize=1;d.MipLevels=1;d.Format=format;d.SampleDesc.Count=1;d.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        D3D12_HEAP_PROPERTIES h{};h.Type=D3D12_HEAP_TYPE_DEFAULT;ComPtr<ID3D12Resource> result;
        gpu_check(device->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&d,state,nullptr,IID_PPV_ARGS(&result)),"texture");return result;
    }
    void colors(ID3D12Resource* image,bool split){
        D3D12_DESCRIPTOR_HEAP_DESC d{};d.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;d.NumDescriptors=1;ComPtr<ID3D12DescriptorHeap> heap;
        gpu_check(device->CreateDescriptorHeap(&d,IID_PPV_ARGS(&heap)),"RTV");auto handle=heap->GetCPUDescriptorHandleForHeapStart();
        const auto desc=image->GetDesc();D3D12_RENDER_TARGET_VIEW_DESC v{};v.ViewDimension=D3D12_RTV_DIMENSION_TEXTURE2D;
        v.Format=(desc.Format==DXGI_FORMAT_B8G8R8A8_UNORM||desc.Format==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)?DXGI_FORMAT_B8G8R8A8_UNORM:DXGI_FORMAT_R8G8B8A8_UNORM;
        device->CreateRenderTargetView(image,&v,handle);begin();
        const float left[]={.5f,.125f,.0625f,1},right[]={0,0,1,1},green[]={0,1,0,1};
        if(split){D3D12_RECT a{0,0,LONG(desc.Width/2),LONG(desc.Height)},b{LONG(desc.Width/2),0,LONG(desc.Width),LONG(desc.Height)};
            list->ClearRenderTargetView(handle,left,1,&a);list->ClearRenderTargetView(handle,right,1,&b);}
        else list->ClearRenderTargetView(handle,green,0,nullptr);
        submit();
    }
    std::vector<unsigned char> read(ID3D12Resource* image,D3D12_RESOURCE_STATES state){
        auto d=image->GetDesc();D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout{};uint64_t size{};device->GetCopyableFootprints(&d,0,1,0,&layout,nullptr,nullptr,&size);
        D3D12_RESOURCE_DESC b{};b.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;b.Width=size;b.Height=1;b.DepthOrArraySize=1;b.MipLevels=1;b.SampleDesc.Count=1;b.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        D3D12_HEAP_PROPERTIES h{};h.Type=D3D12_HEAP_TYPE_READBACK;ComPtr<ID3D12Resource> buffer;
        gpu_check(device->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&b,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&buffer)),"readback");
        begin();gpu_barrier(list.Get(),image,state,D3D12_RESOURCE_STATE_COPY_SOURCE);
        D3D12_TEXTURE_COPY_LOCATION to{},from{};to.pResource=buffer.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=layout;
        from.pResource=image;from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        gpu_barrier(list.Get(),image,D3D12_RESOURCE_STATE_COPY_SOURCE,state);submit();
        std::vector<unsigned char> pixels(static_cast<size_t>(d.Width)*d.Height*4);void* mapped{};D3D12_RANGE range{0,static_cast<SIZE_T>(size)};
        gpu_check(buffer->Map(0,&range,&mapped),"readback map");
        const bool bgra=d.Format==DXGI_FORMAT_B8G8R8A8_UNORM||d.Format==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        for(unsigned y=0;y<d.Height;++y)for(unsigned x=0;x<d.Width;++x){
            auto p=static_cast<unsigned char*>(mapped)+layout.Offset+static_cast<size_t>(y)*layout.Footprint.RowPitch+x*4;
            auto out=pixels.data()+(static_cast<size_t>(y)*static_cast<size_t>(d.Width)+x)*4;
            if(d.Format==DXGI_FORMAT_R10G10B10A2_UNORM){uint32_t packed{};memcpy(&packed,p,sizeof(packed));
                for(unsigned c=0;c<3;++c)out[c]=static_cast<unsigned char>((((packed>>(c*10))&1023)*255+511)/1023);
                out[3]=static_cast<unsigned char>((packed>>30)*85);continue;}
            out[0]=p[bgra?2:0];out[1]=p[1];out[2]=p[bgra?0:2];out[3]=p[3];}
        D3D12_RANGE written{0,0};buffer->Unmap(0,&written);return pixels;
    }
    void verify(){ComPtr<ID3D12InfoQueue> info;if(FAILED(device.As(&info))){std::printf("debug_layer=unavailable; pixel/state/recovery checks still executed\n");return;}
        for(uint64_t i=0;i<info->GetNumStoredMessagesAllowedByRetrievalFilter();++i){SIZE_T n{};gpu_check(info->GetMessage(i,nullptr,&n),"message size");std::vector<unsigned char> data(n);
            auto message=reinterpret_cast<D3D12_MESSAGE*>(data.data());gpu_check(info->GetMessage(i,message,&n),"message");
            if(message->Severity==D3D12_MESSAGE_SEVERITY_ERROR||message->Severity==D3D12_MESSAGE_SEVERITY_CORRUPTION)throw std::runtime_error(message->pDescription);}
        std::printf("debug_layer=available errors=0 corruption=0\n");
    }
};
#if GEARS_DESKTOP_MIRROR_ASYNC
// Release artificial queue gates even when an assertion throws. No game or
// visible desktop window participates in these scheduling/lifetime tests.
struct QueueGate {
    ComPtr<ID3D12Fence> fence;
    explicit QueueGate(GPU& gpu){gpu_check(gpu.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"gate fence");
        gpu_check(gpu.queue->Wait(fence.Get(),1),"gate queue");}
    void open(){gpu_check(fence->Signal(1),"open gate");}
    ~QueueGate(){if(fence)fence->Signal(1);}
};
struct HiddenWindow {
    HWND value{CreateWindowExW(0,L"STATIC",L"Mirror WARP resize fixture",WS_POPUP,0,0,32,32,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr)};
    ~HiddenWindow(){if(value)DestroyWindow(value);}
};
static void async_checks(GPU& gpu){
    auto source=gpu.texture(18,7,DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_RESOURCE_STATE_RENDER_TARGET);gpu.colors(source.Get(),true);
    {
        DesktopMirror mirror;require(mirror.bind(gpu.queue.Get(),source.Get(),1)&&mirror.prepare(),"async prepare");
        const auto stamp=monotonic_ns();require(mirror.capture(source.Get(),1,1,stamp,D3D12_RESOURCE_STATE_RENDER_TARGET)&&mirror.drain(),"async snapshot ready");
        std::array<ComPtr<ID3D12Resource>,3> destinations={
            gpu.texture(16,8,DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_RESOURCE_STATE_PRESENT),
            gpu.texture(13,11,DXGI_FORMAT_R10G10B10A2_UNORM,D3D12_RESOURCE_STATE_PRESENT),
            gpu.texture(8,16,DXGI_FORMAT_B8G8R8A8_UNORM,D3D12_RESOURCE_STATE_PRESENT)};
        const auto wait_before=mirror.status().wait_ns;
        QueueGate first(gpu);
        require(mirror.present(destinations[0].Get(),gpu.queue.Get(),stamp+1),"first blocked draw returns success without CPU fence wait");
        ComPtr<ID3D12Fence> first_done;gpu_check(gpu.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&first_done)),"first draw marker");
        gpu_check(gpu.queue->Signal(first_done.Get(),1),"first draw marker signal");
        QueueGate second(gpu);
#if GEARS_DESKTOP_MIRROR_READBACK
        require(mirror.arm_readback(),"arm second slot readback");DesktopMirror::Readback receipt;
#endif
        require(mirror.present(destinations[1].Get(),gpu.queue.Get(),stamp+2)&&mirror.present(destinations[2].Get(),gpu.queue.Get(),stamp+3),"all independent slots enqueue behind GPU gate");
        const auto full=mirror.status();
        require(full.pending_draws==DesktopMirror::draw_slot_count&&full.destination_held&&full.wait_ns==wait_before&&first_done->GetCompletedValue()==0,"all slots retained while GPU is blocked; no CPU waits");
        require(!mirror.present(destinations[0].Get(),gpu.queue.Get(),stamp+4)&&mirror.status().reason==DesktopMirror::Reason::draw_busy&&mirror.status().presents==3,"full ring skips without resetting allocator or RTV");
        first.open();gpu_check(first_done->SetEventOnCompletion(1,gpu.event),"first marker event");require(WaitForSingleObject(gpu.event,5000)==WAIT_OBJECT_0,"first slot finishes independently");
        require(!mirror.poll()&&mirror.status().pending_draws==2,"poll releases only completed slot");
#if GEARS_DESKTOP_MIRROR_READBACK
        require(mirror.readback_status().state==DesktopMirror::ReadbackState::submitted&&!mirror.take_readback(receipt),"other slot completion cannot release second slot readback");
#endif
        require(mirror.present(destinations[0].Get(),gpu.queue.Get(),stamp+5)&&mirror.status().pending_draws==3&&mirror.status().wait_ns==wait_before,"only completed slot may be reused while others remain blocked");
        second.open();gpu.finish();require(mirror.poll()&&!mirror.status().destination_held&&!mirror.status().pending_draws,"all completed slot references released");
        for(auto const& destination:destinations){
            const auto d=destination->GetDesc();const auto rectangle=mirror_rectangle(unsigned(d.Width),d.Height,9,7);const auto pixels=gpu.read(destination.Get(),D3D12_RESOURCE_STATE_PRESENT);
            for(unsigned y=0;y<d.Height;++y)for(unsigned x=0;x<unsigned(d.Width);++x){const auto* pixel=&pixels[(size_t(y)*size_t(d.Width)+x)*4];
                const bool inside=x>=rectangle.x&&x<rectangle.x+rectangle.width&&y>=rectangle.y&&y<rectangle.y+rectangle.height;
                require(inside?(abs(int(pixel[0])-128)<=2&&abs(int(pixel[1])-32)<=2&&abs(int(pixel[2])-16)<=2):(pixel[0]==0&&pixel[1]==0&&pixel[2]==0),"each in-flight slot kept its own target and RTV");}
        }
        require(mirror.retire_source(1),"all draw slots retire safely");
#if GEARS_DESKTOP_MIRROR_READBACK
        require(mirror.take_readback(receipt)&&receipt.draw_serial==2&&receipt.destination.Format==DXGI_FORMAT_R10G10B10A2_UNORM&&receipt.footprint.Footprint.RowPitch>13*4,"second-slot RGB10 receipt survives other slot reuse and source retirement");
        require(!read_completed(receipt).empty(),"asynchronous completed readback remains readable");
#endif
        require(mirror.close(0),"async retired close");
    }
    {
        HiddenWindow window;require(window.value&&!IsWindowVisible(window.value),"resize fixture never shows a window");
        ComPtr<IDXGIFactory4> factory;gpu_check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"resize factory");
        DXGI_SWAP_CHAIN_DESC1 description{};description.Width=32;description.Height=32;description.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
        description.SampleDesc.Count=1;description.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;description.BufferCount=2;description.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;
        ComPtr<IDXGISwapChain1> swap;gpu_check(factory->CreateSwapChainForHwnd(gpu.queue.Get(),window.value,&description,nullptr,nullptr,&swap),"hidden resize swapchain");
        DesktopMirror mirror;require(mirror.bind(gpu.queue.Get(),source.Get(),2)&&mirror.prepare(),"resize prepare");
        const auto stamp=monotonic_ns();require(mirror.capture(source.Get(),2,2,stamp,D3D12_RESOURCE_STATE_RENDER_TARGET)&&mirror.drain(),"resize snapshot");
        QueueGate gate(gpu);
        // Without a real Present, only buffer zero is current. Two queued
        // draws may reference it; touching the noncurrent flip buffer is invalid.
        for(unsigned i=0;i<2;++i){ComPtr<ID3D12Resource> backbuffer;gpu_check(swap->GetBuffer(0,IID_PPV_ARGS(&backbuffer)),"resize buffer");
            require(mirror.present(backbuffer.Get(),gpu.queue.Get(),stamp+1),"queue held swapchain buffer");}
        require(mirror.status().pending_draws==2&&mirror.status().destination_held&&!mirror.poll(),"mirror retains both slot references after caller releases them");
        require(!mirror.retire_source(2)&&mirror.status().state==5,"source retirement cannot release pending draw buffers");
        gate.open();gpu.finish();require(mirror.drain(0)&&!mirror.status().destination_held,"resize drain releases every backbuffer");
        gpu_check(swap->ResizeBuffers(2,48,24,DXGI_FORMAT_R8G8B8A8_UNORM,0),"ResizeBuffers succeeds after all slots drain");
        require(mirror.retire_source(2)&&mirror.close(0)&&!mirror.status().unsafe,"retirement and close release all slots");
    }
}
#endif
int main(){try{
    GPU gpu;const DXGI_FORMAT formats[]={DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_B8G8R8A8_UNORM,DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,DXGI_FORMAT_B8G8R8A8_UNORM_SRGB};
    const DXGI_FORMAT destinations[]={DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_B8G8R8A8_UNORM,DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,DXGI_FORMAT_R10G10B10A2_UNORM};
    for(auto source_format:formats)for(auto shape:{std::array<unsigned,2>{16,8},std::array<unsigned,2>{18,7}}){
        auto source=gpu.texture(shape[0],shape[1],source_format,D3D12_RESOURCE_STATE_RENDER_TARGET);gpu.colors(source.Get(),true);
        DesktopMirror mirror;require(mirror.bind(gpu.queue.Get(),source.Get(),9),"bind");require(mirror.status().state==1&&!mirror.status().captures,"bind is metadata only");
        require(!mirror.capture(source.Get(),9,1,monotonic_ns(),D3D12_RESOURCE_STATE_RENDER_TARGET),"capture before prepare rejected");require(mirror.prepare(),"prepare");
        const auto received=monotonic_ns();require(mirror.capture(source.Get(),9,1,received,D3D12_RESOURCE_STATE_RENDER_TARGET),"capture");
        // These RT operations also verify the source was restored by capture.
        auto original=gpu.read(source.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET);require(original[0]>=127&&original[1]>=31&&original[(15*4)+2]==255,"left/right source colors");
        gpu.colors(source.Get(),false);auto changed=gpu.read(source.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET);require(changed[0]==0&&changed[1]==255,"source changed after snapshot");
        for(auto dest_format:destinations)for(bool tall:{false,true}){
            const unsigned width=tall?8:16,height=tall?16:8;auto dest=gpu.texture(width,height,dest_format,D3D12_RESOURCE_STATE_PRESENT);
            require(mirror.present(dest.Get(),gpu.queue.Get(),received+1),"present");auto pixels=gpu.read(dest.Get(),D3D12_RESOURCE_STATE_PRESENT);
            const auto rectangle=mirror_rectangle(width,height,shape[0]/2,shape[1]);
            for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){auto p=&pixels[(y*width+x)*4];const bool inside=x>=rectangle.x&&x<rectangle.x+rectangle.width&&y>=rectangle.y&&y<rectangle.y+rectangle.height;
                if(inside)require(abs(int(p[0])-128)<=2&&abs(int(p[1])-32)<=2&&abs(int(p[2])-16)<=2&&p[3]==255,"snapshot mono color, no right-eye bleed");
                else require(p[0]==0&&p[1]==0&&p[2]==0&&p[3]==255,"letterbox black");}
        }
        auto dest=gpu.texture(8,8,DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_RESOURCE_STATE_PRESENT);
        require(!mirror.present(dest.Get(),gpu.queue.Get(),received+100000001),"stale present rejected");
        require(!mirror.capture(source.Get(),8,2,monotonic_ns(),D3D12_RESOURCE_STATE_RENDER_TARGET),"wrong generation rejected");
        require(!mirror.capture(source.Get(),9,2,monotonic_ns()-100000001,D3D12_RESOURCE_STATE_RENDER_TARGET),"stale capture rejected");
        ComPtr<ID3D12CommandQueue> other;D3D12_COMMAND_QUEUE_DESC q{};gpu_check(gpu.device->CreateCommandQueue(&q,IID_PPV_ARGS(&other)),"other queue");
        require(!mirror.present(dest.Get(),other.Get(),received+2),"different queue rejected");
        // Capture is also valid from certified COMMON/PSR and restores each.
        uint64_t ticket=2;D3D12_RESOURCE_STATES current=D3D12_RESOURCE_STATE_RENDER_TARGET;
        for(auto idle:{D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE}){
            gpu.begin();gpu_barrier(gpu.list.Get(),source.Get(),current,idle);gpu.submit();current=idle;
            require(mirror.capture(source.Get(),9,ticket++,monotonic_ns(),idle),"alternate certified source idle");require(mirror.drain(),"copy drain");
            auto restored=gpu.read(source.Get(),idle);require(restored[0]==0&&restored[1]==255,"alternate idle restored");
        }
        require(mirror.drain()&&mirror.close()&&!mirror.status().unsafe&&mirror.status().state==4,"drain close");
        require(!mirror.present(dest.Get(),gpu.queue.Get(),received+3),"closed rejected");
    }
#if GEARS_DESKTOP_MIRROR_READBACK
    for(auto format:destinations){
        // Odd destination width forces row padding; the receipt must contain
        // the actual post-draw RGB10/BGRA/sRGB destination, including its bars.
        auto source=gpu.texture(18,7,DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_RESOURCE_STATE_RENDER_TARGET);gpu.colors(source.Get(),true);
        auto dest=gpu.texture(13,11,format,D3D12_RESOURCE_STATE_PRESENT);
        DesktopMirror mirror;DesktopMirror::Readback receipt;
        require(!mirror.take_readback(receipt)&&mirror.arm_readback()&&!mirror.arm_readback(),"one explicit diagnostic arm");
        require(mirror.bind(gpu.queue.Get(),source.Get(),17)&&mirror.prepare(),"readback mirror prepare");
        const auto received=monotonic_ns();
        require(mirror.capture(source.Get(),17,91,received,D3D12_RESOURCE_STATE_RENDER_TARGET)&&mirror.present(dest.Get(),gpu.queue.Get(),received+1),"readback draw");
        require(mirror.drain(),"readback draw completion in either scheduling mode");
        require(mirror.readback_status().state==DesktopMirror::ReadbackState::ready&&!mirror.status().destination_held,"completed draw retains no destination");
        const auto expected=gpu.read(dest.Get(),D3D12_RESOURCE_STATE_PRESENT); // Also validates restored state.
        require(mirror.retire_source(17)&&mirror.readback_status().held,"completed readback survives source retirement");
        require(mirror.bind(gpu.queue.Get(),source.Get(),18)&&mirror.prepare(),"readback source rebind");
        gpu.colors(source.Get(),false);const auto later=monotonic_ns();
        require(mirror.capture(source.Get(),18,92,later,D3D12_RESOURCE_STATE_RENDER_TARGET)&&mirror.present(dest.Get(),gpu.queue.Get(),later+1),"later draw cannot replace receipt");
        require(mirror.close()&&mirror.take_readback(receipt),"worker ownership survives safe close");
        require(receipt.generation==17&&receipt.ticket==91&&receipt.received_ns==received&&receipt.present_ns==received+1&&receipt.draw_serial==1,"immutable frame receipt");
        require(receipt.source.Width==18&&receipt.source.Height==7&&receipt.destination.Width==13&&receipt.destination.Height==11&&receipt.destination.Format==format,"immutable source and destination metadata");
        require(receipt.footprint.Footprint.RowPitch>13*4&&receipt.bytes>=receipt.footprint.Offset+uint64_t(receipt.footprint.Footprint.RowPitch)*10+13*4,"readback uses padded footprint");
        const auto rectangle=mirror_rectangle(13,11,9,7);
        require(receipt.rectangle.x==rectangle.x&&receipt.rectangle.y==rectangle.y&&receipt.rectangle.width==rectangle.width&&receipt.rectangle.height==rectangle.height,"receipt preserves mirror rectangle");
        const auto pixels=read_completed(receipt);
        for(size_t i=0;i<pixels.size()/3;++i)for(unsigned c=0;c<3;++c)require(pixels[i*3+c]==expected[i*4+c],"post-draw destination readback pixels");
        DesktopMirror::Readback second;require(!mirror.take_readback(second)&&!mirror.readback_status().held&&!mirror.arm_readback(),"completed handoff occurs once");
    }
#endif
    DesktopMirror invalid;auto non_square=gpu.texture(17,8,DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_RESOURCE_STATE_RENDER_TARGET);
    require(!invalid.bind(gpu.queue.Get(),non_square.Get(),1),"odd SBS width rejected");require(invalid.close(),"empty close");
    {
        // Explicitly stalled queue: no background thread and no game involved.
        // An unconfirmed timeout must retain destination/allocators/resources.
        auto source=gpu.texture(16,8,DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_RESOURCE_STATE_RENDER_TARGET);gpu.colors(source.Get(),true);
        auto dest=gpu.texture(8,8,DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_RESOURCE_STATE_PRESENT);
        DesktopMirror stalled;require(stalled.bind(gpu.queue.Get(),source.Get(),1)&&stalled.prepare(),"stalled mirror prepare");
#if GEARS_DESKTOP_MIRROR_READBACK
        require(stalled.arm_readback(),"arm unsafe-close receipt");DesktopMirror::Readback abandoned;
#endif
        ComPtr<ID3D12Fence> blocker;gpu_check(gpu.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&blocker)),"blocker");
        gpu_check(gpu.queue->Wait(blocker.Get(),1),"controlled queue stall");
        const auto received=monotonic_ns();require(stalled.capture(source.Get(),1,1,received,D3D12_RESOURCE_STATE_RENDER_TARGET),"first queued copy");
        require(!stalled.capture(source.Get(),1,2,received,D3D12_RESOURCE_STATE_RENDER_TARGET)&&stalled.status().busy==1,"copy allocator busy skips without waiting");
        if constexpr(DesktopMirror::asynchronous){
            require(stalled.present(dest.Get(),gpu.queue.Get(),received+1)&&!stalled.status().wait_ns,"async draw queues without waiting");
            require(!stalled.drain(0),"blocked async close preparation times out");
        }else require(!stalled.present(dest.Get(),gpu.queue.Get(),received+1),"bounded draw timeout");
        auto failed=stalled.status();
        require(failed.state==3&&failed.timeouts>=1&&FAILED(failed.error),"timeout evidence");
        if constexpr(!DesktopMirror::asynchronous)require(failed.wait_ns&&failed.max_wait_ns,"synchronous draw wait measured");
        require(!stalled.close(0)&&stalled.status().unsafe,"unconfirmed close retains all GPU references");
#if GEARS_DESKTOP_MIRROR_READBACK
        require(stalled.readback_status().state==DesktopMirror::ReadbackState::unsafe&&!stalled.take_readback(abandoned),"unsafe close never hands off in-flight readback");
#endif
        gpu_check(blocker->Signal(1),"release controlled stall");gpu.finish();
        require(!stalled.close(0),"retention decision remains explicit after detach");
    }
    {
        auto source=gpu.texture(16,8,DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_RESOURCE_STATE_RENDER_TARGET);gpu.colors(source.Get(),true);
        auto dest=gpu.texture(8,8,DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_RESOURCE_STATE_PRESENT);
        DesktopMirror recovered;require(recovered.bind(gpu.queue.Get(),source.Get(),1)&&recovered.prepare(),"recovery prepare");
#if GEARS_DESKTOP_MIRROR_READBACK
        require(recovered.arm_readback(),"arm recoverable receipt");DesktopMirror::Readback delayed;
#endif
        ComPtr<ID3D12Fence> blocker;gpu_check(gpu.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&blocker)),"recovery blocker");
        gpu_check(gpu.queue->Wait(blocker.Get(),1),"recovery queue stall");
        const auto received=monotonic_ns();require(recovered.capture(source.Get(),1,1,received,D3D12_RESOURCE_STATE_RENDER_TARGET),"recovery capture");
        if constexpr(DesktopMirror::asynchronous){
            require(recovered.present(dest.Get(),gpu.queue.Get(),received+1),"recovery async draw queued");
            require(!recovered.drain(0),"recovery async drain timeout");
        }else require(!recovered.present(dest.Get(),gpu.queue.Get(),received+1),"recovery draw timeout");
        require(recovered.status().destination_held&&!recovered.poll(),"pending draw retains destination without waiting");
#if GEARS_DESKTOP_MIRROR_READBACK
        require(recovered.readback_status().state==DesktopMirror::ReadbackState::submitted&&recovered.readback_status().held&&!recovered.take_readback(delayed),"pending fence prohibits readback handoff");
#endif
        require(!recovered.drain(0)&&recovered.status().state==3&&!recovered.status().unsafe,"continuous shutdown can retry drain without detach");
        gpu_check(blocker->Signal(1),"resume timed-out queue");gpu.finish();
        require(recovered.poll()&&!recovered.status().destination_held,"worker releases completed destination");
#if GEARS_DESKTOP_MIRROR_READBACK
        require(recovered.take_readback(delayed)&&delayed.ticket==1,"late fence completion permits readback handoff");
        const auto delayed_pixels=read_completed(delayed);require(!delayed_pixels.empty(),"late completed buffer is readable");
#endif
        require(recovered.status().state==3,"timeout remains disabled after GPU recovery");
        require(recovered.drain(0)&&recovered.close(0)&&!recovered.status().unsafe,"continuous retry can close safely");
    }
    {
        auto first=gpu.texture(16,8,DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_RESOURCE_STATE_RENDER_TARGET);gpu.colors(first.Get(),true);
        auto next=gpu.texture(20,10,DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_RESOURCE_STATE_RENDER_TARGET);gpu.colors(next.Get(),false);
        auto dest=gpu.texture(10,10,DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_RESOURCE_STATE_PRESENT);
        DesktopMirror mirror;require(mirror.bind(gpu.queue.Get(),first.Get(),7)&&mirror.prepare(),"handoff first binding");
        ComPtr<ID3D12Fence> blocker;gpu_check(gpu.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&blocker)),"handoff blocker");
        gpu_check(gpu.queue->Wait(blocker.Get(),1),"handoff stall");
        const auto stamp=monotonic_ns();
        require(mirror.capture(first.Get(),7,41,stamp,D3D12_RESOURCE_STATE_RENDER_TARGET),"handoff pending copy");
        require(!mirror.retire_source(6)&&mirror.status().state==2,"stale retirement cannot suspend current source");
        require(!mirror.retire_source(7)&&mirror.status().state==5,"pending copy retains source");
        require(!mirror.capture(first.Get(),7,42,stamp,D3D12_RESOURCE_STATE_RENDER_TARGET),"retiring source cannot capture");
        require(!mirror.present(dest.Get(),gpu.queue.Get(),stamp+1),"retiring snapshot cannot present");
        require(!mirror.bind(gpu.queue.Get(),next.Get(),8),"pending source cannot be replaced");
        gpu_check(blocker->Signal(1),"handoff resume");gpu.finish();
        require(mirror.retire_source(7)&&mirror.retire_source(7)&&mirror.status().state==0,"completed retirement is retryable");
        require(!mirror.bind(gpu.queue.Get(),next.Get(),7),"retired resource generation cannot repeat");
        require(mirror.bind(gpu.queue.Get(),next.Get(),8)&&mirror.prepare(),"new source binding");
        require(!mirror.capture(next.Get(),8,41,monotonic_ns(),D3D12_RESOURCE_STATE_RENDER_TARGET),"old ticket cannot replay after bind");
        const auto next_stamp=monotonic_ns();
        require(mirror.capture(next.Get(),8,42,next_stamp,D3D12_RESOURCE_STATE_RENDER_TARGET)&&mirror.drain(),"new source capture");
        require(mirror.present(dest.Get(),gpu.queue.Get(),next_stamp+1),"new source present");
        const auto pixels=gpu.read(dest.Get(),D3D12_RESOURCE_STATE_PRESENT);
        require(pixels[0]==0&&pixels[1]==255&&pixels[2]==0,"handoff displays new source pixels");
        require(mirror.status().captures==2&&mirror.status().ticket==42,"handoff preserves counters");
        require(mirror.close()&&!mirror.retire_source(8)&&!mirror.bind(gpu.queue.Get(),first.Get(),9),"terminal close cannot reopen");
    }
#if GEARS_DESKTOP_MIRROR_ASYNC
    async_checks(gpu);
#endif
    gpu.verify();std::printf("desktop mirror WARP: %u checks passed (async=%u, formats, snapshot, states, bars, close, timeout recovery, source handoff)\n",checks,DesktopMirror::asynchronous?1u:0u);return 0;
}catch(std::exception const& error){std::fprintf(stderr,"mirror test: %s\n",error.what());return 1;}}
