#include "../vr-common/gpu_share.h"
#include "../vr-common/stereo_bridge.h"
#include "../vr-common/pose_bridge.h"
#include <cstdio>
#include <cstdlib>
#include <cerrno>

using namespace gears_vr;
// Diagnostic native-resolution pixels only. No engine scene, game process or
// physical tracking acceptance is claimed by this producer.
struct Fixture {
    ComPtr<IDXGIFactory4> factory;
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<IDXGISwapChain1> chain;
    ComPtr<ID3D12Resource> source;
    ComPtr<ID3D12DescriptorHeap> rtv;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> commands;
    ComPtr<ID3D12Fence> fence,consumer_done;
    GpuPublisher publisher;
    StereoBridge bridge;
    UINT64 fence_serial{};
    unsigned eye_size{};
    bool submitted{},closed{};

    explicit Fixture(unsigned size):eye_size(size) {
        gpu_check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"factory");
        gpu_check(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)),"device");
        D3D12_COMMAND_QUEUE_DESC q{};
        gpu_check(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)),"queue");
        DXGI_SWAP_CHAIN_DESC1 desc{};desc.Width=2*size;desc.Height=size;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count=1;desc.BufferCount=2;desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;desc.AlphaMode=DXGI_ALPHA_MODE_PREMULTIPLIED;
        gpu_check(factory->CreateSwapChainForComposition(queue.Get(),&desc,nullptr,&chain),"composition swapchain");
        gpu_check(chain->GetBuffer(0,IID_PPV_ARGS(&source)),"source buffer");
        D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;hd.NumDescriptors=1;
        gpu_check(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&rtv)),"RTV heap");
        device->CreateRenderTargetView(source.Get(),nullptr,rtv->GetCPUDescriptorHandleForHeapStart());
        gpu_check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"allocator");
        gpu_check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&commands)),"commands");
        gpu_check(commands->Close(),"initial close");
        gpu_check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"local fence");
        if(!publisher.bind(queue.Get(),chain.Get())||!publisher.prepare())throw std::runtime_error("Publisher preparation rejected");
        HANDLE handle{};
        gpu_check(device->OpenSharedHandleByName(gpu_name(GetCurrentProcessId(),L"done").c_str(),GENERIC_ALL,&handle),"consumer fence handle");
        const HRESULT opened=device->OpenSharedHandle(handle,IID_PPV_ARGS(&consumer_done));CloseHandle(handle);
        gpu_check(opened,"consumer fence");
    }
    bool wait_fence(UINT64 serial,unsigned timeout) {
        const auto deadline=GetTickCount64()+timeout;
        do {const auto value=fence->GetCompletedValue();if(value==~UINT64(0))return false;
            if(value>=serial)return true;Sleep(1);}while(GetTickCount64()<deadline);
        return false;
    }
    void draw() {
        gpu_check(allocator->Reset(),"allocator reset");gpu_check(commands->Reset(allocator.Get(),nullptr),"command reset");
        gpu_barrier(commands.Get(),source.Get(),D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_RENDER_TARGET);
        const auto target=rtv->GetCPUDescriptorHandleForHeapStart();
        const float backgrounds[2][4]={{.8f,.08f,.12f,1},{.06f,.18f,.85f,1}};
        const float corner_colors[4][4]={{1,1,1,1},{0,1,0,1},{1,1,0,1},{1,0,1,1}};
        const float stripe[4]={0,0,0,1};
        for(unsigned eye=0;eye<2;++eye){const LONG x=LONG(eye*eye_size),n=LONG(eye_size);
            const D3D12_RECT region{x,0,x+n,n};commands->ClearRenderTargetView(target,backgrounds[eye],1,&region);
            const D3D12_RECT corners[4]={{x,0,x+64,64},{x+n-64,0,x+n,64},
                {x,n-64,x+64,n},{x+n-64,n-64,x+n,n}};
            for(unsigned k=0;k<4;++k)commands->ClearRenderTargetView(target,corner_colors[k],1,&corners[k]);
            const D3D12_RECT center{x+n/2-4,0,x+n/2+4,n};
            commands->ClearRenderTargetView(target,stripe,1,&center);
        }
        gpu_barrier(commands.Get(),source.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PRESENT);
        gpu_check(commands->Close(),"clear close");ID3D12CommandList* lists[]={commands.Get()};
        submitted=true;queue->ExecuteCommandLists(1,lists);
    }
    bool close() noexcept {
        if(closed)return true;closed=true;
        bool safe=true;
        if(submitted){const auto serial=++fence_serial;
            safe=SUCCEEDED(queue->Signal(fence.Get(),serial))&&wait_fence(serial,5000);}
        // No new clear/capture is possible once run has returned (or unwound).
        // Leave acquisition enabled until the last published slot is consumed:
        // setting stopped first would strand an as-yet unacquired final image.
        const auto deadline=GetTickCount64()+5000;bool acknowledged=publisher.frames.load()==0;
        while(safe&&!acknowledged&&GetTickCount64()<deadline){const auto value=consumer_done->GetCompletedValue();
            if(value==~UINT64(0))break;acknowledged=value>=publisher.frames.load();if(!acknowledged)Sleep(1);}
        // Failure/unwind also reaches this bounded stop path. Consumer-owned
        // open resource references remain its responsibility; an ACK timeout
        // reports a failed test, not proof that its GPU work has completed.
        publisher.mark_stopped();
        const bool drained=publisher.drain(5000);
        const bool publisher_closed=publisher.close();bridge.close();
        if(!safe){source.Detach();rtv.Detach();allocator.Detach();commands.Detach();fence.Detach();
            consumer_done.Detach();chain.Detach();queue.Detach();device.Detach();factory.Detach();}
        printf("{\"cleanup\":true,\"queue_complete\":%s,\"producer_drained\":%s,\"consumer_acknowledged\":%s,\"publisher_closed\":%s}\n",
            safe?"true":"false",drained?"true":"false",acknowledged?"true":"false",publisher_closed?"true":"false");fflush(stdout);
        return safe&&drained&&publisher_closed&&acknowledged;
    }
    ~Fixture(){if(!closed)close();}
    unsigned run(unsigned seconds) {
        StereoGate gate;StereoSample frozen{};bool ready=false;
        const auto wait_deadline=GetTickCount64()+30000;
        printf("{\"diagnostic_test_pattern\":true,\"pid\":%lu,\"eye_size\":%u,\"source_width\":%u,\"source_height\":%u,\"ready\":true,\"awaiting_stereo_bridge\":true,\"corner_pixels\":64,\"stripe_pixels\":8}\n",
            GetCurrentProcessId(),eye_size,2*eye_size,eye_size);fflush(stdout);
        bool bridge_open=false;
        while(GetTickCount64()<wait_deadline){
            if(!bridge_open)bridge_open=bridge.open(GetCurrentProcessId(),false);
            StereoSample sample{};
            if(bridge_open&&bridge.read(sample)){auto result=gate.update(sample,monotonic_ns());
                if(result){frozen=result.sample;ready=true;break;}}
            Sleep(5);
        }
        if(!ready)throw std::runtime_error("No fresh focused tracked stereo sample within30s");
        printf("{\"writing\":true,\"duration_seconds\":%u,\"stereo_session\":%llu}\n",seconds,frozen.session);fflush(stdout);
        const auto deadline=GetTickCount64()+UINT64(seconds)*1000;
        UINT64 ticket_id=0,last_sample_sequence=0;unsigned published=0;
        while(GetTickCount64()<deadline){
            StereoSample sample{};
            if(!bridge.read(sample)){Sleep(1);continue;}
            const auto accepted=gate.update(sample,monotonic_ns());
            if(!accepted||sample.sequence==last_sample_sequence){Sleep(1);continue;}
            // Freeze BEFORE the GPU clear. No newer pose is substituted into its
            // ticket while attempting to publish this independently owned image.
            frozen=accepted.sample;last_sample_sequence=frozen.sequence;
            StereoImageTicket ticket{};ticket.id=++ticket_id;ticket.sample=frozen;
            ticket.rect[0]={0,0,int(eye_size),int(eye_size)};
            ticket.rect[1]={int(eye_size),0,int(eye_size),int(eye_size)};
            draw();
            GpuCaptureTrace trace{};const bool captured=publisher.capture(chain.Get(),&ticket,&trace);
            // This signal follows BOTH our clear and any producer copy on the
            // same queue. Only then may this single allocator/source be reused.
            gpu_check(queue->Signal(fence.Get(),++fence_serial),"frame completion signal");
            if(!wait_fence(fence_serial,5000))throw std::runtime_error("Frame GPU timeout/device removed");
            publisher.poll();
            if(captured)++published;
            if(publisher.state.load()!=2)throw std::runtime_error("GPU publisher failed");
            Sleep(1);
        }
        printf("{\"writing\":false,\"stopped\":false,\"awaiting_final_ack\":true,\"frames\":%u,\"attempted_tickets\":%llu,\"diagnostic_only\":true}\n",published,ticket_id);fflush(stdout);
        return published;
    }
};
static unsigned number(const char* value){char* end{};errno=0;const auto n=std::strtoul(value,&end,10);
    if(errno||!value[0]||*end||n>120000)throw std::runtime_error("Invalid numeric argument");return unsigned(n);}
int main(int argc,char** argv){
    try {
        if(argc<2||argc>3)throw std::runtime_error("Usage: StereoTransferFixture.exe 2048|4096 [seconds=30]");
        const unsigned size=number(argv[1]),seconds=argc==3?number(argv[2]):30;
        if((size!=2048&&size!=4096)||seconds<1||seconds>120)throw std::runtime_error("Require eye size2048/4096 and duration1..120s");
        Fixture fixture(size);const auto frames=fixture.run(seconds);const bool safe=fixture.close();
        return frames&&safe?0:3;
    }catch(std::exception const& error){fprintf(stderr,"StereoTransferFixture rejected: %s\n",error.what());fflush(stderr);return 2;}
}
