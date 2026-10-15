#include "../vr-common/gpu_share.h"
#include "../vr-common/gpu_blit.h"
#include <vector>
#include <cstdio>
#include <cmath>
#include <memory>
#include <d3d12sdklayers.h>
#include <limits>
using namespace gears_vr;
struct Gpu {
    ComPtr<IDXGIFactory4> factory;ComPtr<ID3D12Device> device;ComPtr<ID3D12CommandQueue> queue;
    Gpu(){gpu_check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"factory");gpu_check(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)),"device");
        D3D12_COMMAND_QUEUE_DESC q{};gpu_check(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)),"queue");}
    void verify(){
        ComPtr<ID3D12InfoQueue> info;if(FAILED(device.As(&info)))return;
        for(UINT64 i=0;i<info->GetNumStoredMessagesAllowedByRetrievalFilter();++i){
            SIZE_T size{};gpu_check(info->GetMessage(i,nullptr,&size),"debug message size");std::vector<unsigned char> storage(size);
            auto* message=reinterpret_cast<D3D12_MESSAGE*>(storage.data());gpu_check(info->GetMessage(i,message,&size),"debug message");
            if(message->Severity==D3D12_MESSAGE_SEVERITY_ERROR || message->Severity==D3D12_MESSAGE_SEVERITY_CORRUPTION)throw std::runtime_error(std::string("D3D12 validation: ")+message->pDescription);
        }
    }
};
StereoImageTicket test_ticket(unsigned serial,unsigned width,unsigned height,bool allow_sim=false){
    StereoImageTicket t{};t.id=1000+serial;auto& s=t.sample;
    s.session=777;s.sequence=serial;s.received_ns=1000000000LL+serial;s.predicted_time=2000000000LL+serial;
    s.head_flags=s.view_flags=15;s.focused=s.should_render=1;s.head.orientation={};s.head.position.x=serial*.01;
    if(allow_sim){s.source=StereoSource::MetaOperatorSimulated;s.head_flags=3;}
    for(unsigned i=0;i<2;++i){s.eyes[i].pose.orientation={};s.eyes[i].pose.position.x=s.head.position.x+(i?.032:-.032);
        s.eyes[i].fov={-.8,.7,.75,-.65};}
    t.rect[0]={0,0,static_cast<int>(width/2),static_cast<int>(height)};
    t.rect[1]={static_cast<int>(width/2),0,static_cast<int>(width-width/2),static_cast<int>(height)};return t;
}
bool same_ticket(StereoImageTicket const& a,StereoImageTicket const& b){
    if(a.id!=b.id||!stereo_same(a.sample,b.sample))return false;
    for(unsigned i=0;i<2;++i)if(a.rect[i].x!=b.rect[i].x||a.rect[i].y!=b.rect[i].y||a.rect[i].width!=b.rect[i].width||a.rect[i].height!=b.rect[i].height)return false;
    return true;
}
int consume(DWORD pid,unsigned expected,const wchar_t* output,bool stereo=false,bool allow_sim=false){
    Gpu gpu;GpuReceiver receiver;receiver.open(gpu.device.Get(),pid,allow_sim);
    // A second consumer must be rejected, including in the same process.
    if(expected){bool rejected=false;try{GpuReceiver duplicate;duplicate.open(gpu.device.Get(),pid,allow_sim);}catch(...){rejected=true;}if(!rejected)throw std::runtime_error("Duplicate consumer accepted");}
    ComPtr<ID3D12Resource> buffer;ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> commands;
    ComPtr<ID3D12Fence> fence;D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 bytes{};
    ComPtr<ID3D12Resource> converted;std::unique_ptr<GpuBlit> blit;auto desc=receiver.texture->GetDesc();
    DXGI_FORMAT pixel_format=receiver.format;
    if(expected && receiver.format==DXGI_FORMAT_R10G10B10A2_UNORM){
        desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        D3D12_HEAP_PROPERTIES props{};props.Type=D3D12_HEAP_TYPE_DEFAULT;
        gpu_check(gpu.device->CreateCommittedResource(&props,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&converted)),"conversion target");
        blit=std::make_unique<GpuBlit>(gpu.device.Get(),desc.Format);pixel_format=desc.Format;
    }
    gpu.device->GetCopyableFootprints(&desc,0,1,0,&footprint,nullptr,nullptr,&bytes);
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_READBACK;
    D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=bytes;rd.Height=1;rd.DepthOrArraySize=1;rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    gpu_check(gpu.device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&buffer)),"receiver readback");
    gpu_check(gpu.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"receiver allocator");
    gpu_check(gpu.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&commands)),"receiver commands");gpu_check(commands->Close(),"initial close");
    gpu_check(gpu.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"receiver fence");
    unsigned frames=0,changes=0;UINT64 last_hash=0;
    ULONGLONG deadline=GetTickCount64()+(expected?15000:20000);
    while(GetTickCount64()<deadline && !receiver.stopped() && (!expected || frames<expected)){
        if(!receiver.acquire(gpu.queue.Get())){Sleep(1);continue;}
        auto acquired_ticket=receiver.ticket();
        if(expected && (stereo?!same_ticket(acquired_ticket,test_ticket(frames+1,receiver.width,receiver.height,allow_sim)):!stereo_image_ticket_empty(acquired_ticket)))
            throw std::runtime_error("Ticket does not match the acquired frame sequence");
        if(expected && !frames)Sleep(150); // Slow reader: producer must skip.
        if(!same_ticket(receiver.ticket(),acquired_ticket))throw std::runtime_error("Busy producer changed private acquired ticket");
        gpu_check(allocator->Reset(),"allocator reset");gpu_check(commands->Reset(allocator.Get(),nullptr),"commands reset");
        if(blit){
            gpu_barrier(commands.Get(),receiver.texture.Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            blit->draw(commands.Get(),receiver.texture.Get(),converted.Get());
            gpu_barrier(commands.Get(),receiver.texture.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COMMON);
            gpu_barrier(commands.Get(),converted.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE);
        }else gpu_barrier(commands.Get(),receiver.texture.Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_SOURCE);
        D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=buffer.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=footprint;
        src.pResource=blit?converted.Get():receiver.texture.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;commands->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
        if(blit)gpu_barrier(commands.Get(),converted.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
        else gpu_barrier(commands.Get(),receiver.texture.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COMMON);
        gpu_check(commands->Close(),"close copy");ID3D12CommandList* lists[]={commands.Get()};gpu.queue->ExecuteCommandLists(1,lists);
        try{
            receiver.acknowledge(gpu.queue.Get());gpu_check(gpu.queue->Signal(fence.Get(),frames+1),"local fence");
            if(!same_ticket(receiver.ticket(),acquired_ticket))throw std::runtime_error("Ack cleared or changed acquired ticket");
            ULONGLONG wait_deadline=GetTickCount64()+3000;
            while(fence->GetCompletedValue()<frames+1 && GetTickCount64()<wait_deadline)Sleep(1);
            if(fence->GetCompletedValue()<frames+1 || fence->GetCompletedValue()==~UINT64(0))throw std::runtime_error("Consumer fence timeout/device removed");
        }catch(...){buffer.Detach();commands.Detach();allocator.Detach();fence.Detach();converted.Detach();blit.release();gpu.queue.Detach();gpu.device.Detach();throw;}
        unsigned char* data{};D3D12_RANGE range{0,static_cast<SIZE_T>(bytes)};gpu_check(buffer->Map(0,&range,reinterpret_cast<void**>(&data)),"map verification");
        unsigned out_width=expected?receiver.width:(receiver.width<640?receiver.width:640);
        unsigned out_height=static_cast<unsigned>(UINT64(receiver.height)*out_width/receiver.width);
        std::vector<unsigned char> rgb(static_cast<size_t>(out_width)*out_height*3);UINT64 hash=14695981039346656037ull;bool matches=true;
        int expected_rgb[]={static_cast<int>(std::lround((receiver.sequence()%8+1)*.1*255)),128,191};
        if(stereo && expected && receiver.ticket().sample.sequence!=receiver.sequence())throw std::runtime_error("Image and ticket GPU sequence differ");
        for(unsigned y=0;y<out_height;++y)for(unsigned x=0;x<out_width;++x){
            auto* p=data+footprint.Offset+static_cast<size_t>(UINT64(y)*receiver.height/out_height)*footprint.Footprint.RowPitch+static_cast<size_t>(UINT64(x)*receiver.width/out_width)*4;
            auto* out=rgb.data()+(static_cast<size_t>(y)*out_width+x)*3;
            if(pixel_format==DXGI_FORMAT_R10G10B10A2_UNORM){unsigned v{};memcpy(&v,p,4);for(unsigned c=0;c<3;++c)out[c]=static_cast<unsigned char>((((v>>(10*c))&1023)*255+511)/1023);}
            else{out[0]=p[pixel_format==DXGI_FORMAT_B8G8R8A8_UNORM?2:0];out[1]=p[1];out[2]=p[pixel_format==DXGI_FORMAT_B8G8R8A8_UNORM?0:2];}
            for(unsigned c=0;c<3;++c){hash=(hash^out[c])*1099511628211ull;if(expected && abs(int(out[c])-expected_rgb[c])>1)matches=false;}
        }
        D3D12_RANGE empty{};buffer->Unmap(0,&empty);if(!matches)throw std::runtime_error("Shared GPU pixels differ from known source");
        if(frames && hash!=last_hash)++changes;last_hash=hash;++frames;
        if(output && frames==1){FILE* file{};_wfopen_s(&file,output,L"wb");if(!file)throw std::runtime_error("Output unavailable");fprintf(file,"P6\n%u %u\n255\n",out_width,out_height);bool saved=fwrite(rgb.data(),1,rgb.size(),file)==rgb.size();if(fclose(file))saved=false;if(!saved)throw std::runtime_error("Save frame failed");}
        if(!expected)Sleep(33);
    }
    gpu.verify();printf("{\"received\":%u,\"changes\":%u,\"width\":%u,\"height\":%u,\"format\":%u,\"last_sequence\":%llu,\"known_pixels_verified\":%s}\n",frames,changes,receiver.width,receiver.height,receiver.format,receiver.sequence(),expected?"true":"false");
    return expected?(frames==expected && changes==expected-1?0:1):(frames>=10?0:1);
}
void produce(DXGI_FORMAT format,bool stereo=false,bool allow_sim=false){
    Gpu gpu;ComPtr<IDXGISwapChain1> chain;DXGI_SWAP_CHAIN_DESC1 desc{};desc.Width=19;desc.Height=13;desc.Format=format;desc.SampleDesc.Count=1;desc.BufferCount=2;
    desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;desc.AlphaMode=DXGI_ALPHA_MODE_PREMULTIPLIED;
    gpu_check(gpu.factory->CreateSwapChainForComposition(gpu.queue.Get(),&desc,nullptr,&chain),"test swapchain");GpuPublisher publisher;
    if(!publisher.bind(gpu.queue.Get(),chain.Get(),allow_sim) || !publisher.prepare())throw std::runtime_error("Publisher setup failed");
    HANDLE metadata_map=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,gpu_name(GetCurrentProcessId(),L"metadata").c_str());
    auto* metadata=metadata_map?static_cast<GpuPacket*>(MapViewOfFile(metadata_map,FILE_MAP_ALL_ACCESS,0,0,sizeof(GpuPacket))):nullptr;
    if(!metadata)throw std::runtime_error("Test metadata map unavailable");
    // Reject both legacy versions, bad sizes and changed policy before claiming.
    for(unsigned corrupt=0;corrupt<4;++corrupt){
        auto version=metadata->version,size=metadata->size,policy=metadata->allow_simulated;
        if(corrupt==0)metadata->version=1;if(corrupt==1)metadata->version=2;
        if(corrupt==2)metadata->size=56;if(corrupt==3)metadata->allow_simulated=2;
        bool rejected=false;try{GpuReceiver old;old.open(gpu.device.Get(),GetCurrentProcessId(),allow_sim);}catch(...){rejected=true;}
        if(publisher.capture(chain.Get()))throw std::runtime_error("Producer accepted mismatched protocol");
        metadata->version=version;metadata->size=size;metadata->allow_simulated=policy;
        if(!rejected)throw std::runtime_error("Receiver accepted v1/wrong-size packet");
    }
    bool policy_rejected=false;try{GpuReceiver mismatched;mismatched.open(gpu.device.Get(),GetCurrentProcessId(),!allow_sim);}catch(...){policy_rejected=true;}
    if(!policy_rejected)throw std::runtime_error("Receiver silently accepted different simulation policy");
    wchar_t exe[32768];GetModuleFileNameW(nullptr,exe,32768);std::wstring command=L"\""+std::wstring(exe)+L"\" --consume "+std::to_wstring(GetCurrentProcessId())+L" 12"+(allow_sim?L" --simulated":stereo?L" --stereo":L"");
    STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION child{};
    if(!CreateProcessW(nullptr,command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&child))throw std::runtime_error("Test consumer launch failed");
    CloseHandle(child.hThread);
    ComPtr<ID3D12Resource> source;gpu_check(chain->GetBuffer(0,IID_PPV_ARGS(&source)),"test source");
    ComPtr<ID3D12DescriptorHeap> rtv;D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;hd.NumDescriptors=1;gpu_check(gpu.device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&rtv)),"RTV");gpu.device->CreateRenderTargetView(source.Get(),nullptr,rtv->GetCPUDescriptorHandleForHeapStart());
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> commands;gpu_check(gpu.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"clear allocator");
    gpu_check(gpu.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&commands)),"clear list");gpu_check(commands->Close(),"clear initial close");
    ComPtr<ID3D12Fence> clear_fence;gpu_check(gpu.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&clear_fence)),"clear fence");
    ULONGLONG consumer_deadline=GetTickCount64()+5000;
    while(!InterlockedCompareExchange(&metadata->consumer,0,0)&&GetTickCount64()<consumer_deadline)Sleep(1);
    if(!InterlockedCompareExchange(&metadata->consumer,0,0))throw std::runtime_error("Consumer did not claim slot");
    // Geometry/session rejection must occur without queueing or publishing.
    for(unsigned failure=0;failure<13;++failure){auto invalid=test_ticket(1,desc.Width,desc.Height,allow_sim);
        if(failure==0)++invalid.rect[1].x;if(failure==1)--invalid.rect[1].width;
        if(failure==2)invalid.rect[0].width=-1;if(failure==3)invalid.sample.head.position.x=std::numeric_limits<double>::quiet_NaN();
        if(failure==4)invalid.sample.session=0;if(failure==5)invalid.sample.sequence=0;
        if(failure==6)invalid.sample.view_flags=7;if(failure==7)invalid.sample.focused=0;if(failure==8)invalid.id=0;
        if(failure==9)invalid.sample.source=static_cast<StereoSource>(99);
        if(failure==10)invalid.sample.head_flags=1;if(failure==11)invalid.sample.view_flags=3;
        if(failure==12){invalid.sample.source=allow_sim?StereoSource::RuntimeTracked:StereoSource::MetaOperatorSimulated;invalid.sample.head_flags=3;}
        if(publisher.capture(chain.Get(),&invalid)||publisher.frames||InterlockedCompareExchange64(&metadata->sequence,0,0)||!stereo_image_ticket_empty(metadata->ticket))
            throw std::runtime_error("Invalid stereo ticket altered published slot");
    }
    unsigned busy_ticket_checks=0,trace_success=0,trace_busy=0;
    for(unsigned serial=1;serial<=12;++serial){
        gpu_check(allocator->Reset(),"clear reset");gpu_check(commands->Reset(allocator.Get(),nullptr),"clear command reset");
        gpu_barrier(commands.Get(),source.Get(),D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_RENDER_TARGET);
        float color[]={float(serial%8+1)*.1f,.5f,.75f,1.f};commands->ClearRenderTargetView(rtv->GetCPUDescriptorHandleForHeapStart(),color,0,nullptr);
        gpu_barrier(commands.Get(),source.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PRESENT);gpu_check(commands->Close(),"clear close");
        ID3D12CommandList* lists[]={commands.Get()};gpu.queue->ExecuteCommandLists(1,lists);
        try{
            gpu_check(gpu.queue->Signal(clear_fence.Get(),serial),"clear signal");
            ULONGLONG deadline=GetTickCount64()+5000;
            auto ticket=test_ticket(serial,desc.Width,desc.Height,allow_sim);
            if(allow_sim&&serial>1){
                auto previous_sequence=InterlockedCompareExchange64(&metadata->sequence,0,0);auto previous_ticket=metadata->ticket;
                auto wrong_source=ticket;wrong_source.sample.source=StereoSource::RuntimeTracked;wrong_source.sample.head_flags=15;
                if(publisher.capture(chain.Get(),&wrong_source)||InterlockedCompareExchange64(&metadata->sequence,0,0)!=previous_sequence||!same_ticket(metadata->ticket,previous_ticket))
                    throw std::runtime_error("Source switch altered the frozen GPU slot");
            }
            while(publisher.frames<serial && GetTickCount64()<deadline){
                auto previous_sequence=InterlockedCompareExchange64(&metadata->sequence,0,0);auto previous_ticket=metadata->ticket;
                GpuCaptureTrace trace{};bool tracing=(serial%2)==0;
                bool published=publisher.capture(chain.Get(),stereo?&ticket:nullptr,tracing?&trace:nullptr);
                if(tracing){
                    if(published){++trace_success;
                        const unsigned expected=capture_state_valid|capture_serial_valid|capture_consumer_valid|capture_ready_valid|capture_done_valid;
                        if(trace.reason!=GpuCaptureReason::published||(trace.valid&expected)!=expected||trace.serial_after!=serial||trace.serial_before+1!=trace.serial_after||trace.state!=2)
                            throw std::runtime_error("Successful capture trace inconsistent");
                    }else if(trace.reason==GpuCaptureReason::producer_in_flight||trace.reason==GpuCaptureReason::consumer_not_done){++trace_busy;
                        if((trace.valid&(capture_serial_valid|capture_ready_valid|capture_done_valid))!=(capture_serial_valid|capture_ready_valid|capture_done_valid)||trace.serial_before!=trace.serial_after)
                            throw std::runtime_error("Busy capture trace inconsistent");
                    }else if(trace.reason!=GpuCaptureReason::rate_limited)throw std::runtime_error("Unexpected capture trace rejection");
                }
                if(!published && (InterlockedCompareExchange64(&metadata->sequence,0,0)!=previous_sequence||!same_ticket(metadata->ticket,previous_ticket)))
                    throw std::runtime_error("Busy slot replaced prior sequence/ticket");
                if(!published && previous_sequence)++busy_ticket_checks;
                if(published && (InterlockedCompareExchange64(&metadata->sequence,0,0)!=serial ||
                    (stereo?!same_ticket(metadata->ticket,ticket):!stereo_image_ticket_empty(metadata->ticket))))
                    throw std::runtime_error("Published ticket does not identify the copied image");
                Sleep(1);
            }
            while(clear_fence->GetCompletedValue()<serial && GetTickCount64()<deadline)Sleep(1);
            if(clear_fence->GetCompletedValue()<serial || clear_fence->GetCompletedValue()==~UINT64(0))throw std::runtime_error("Clear fence timeout/device removed");
        }catch(...){source.Detach();rtv.Detach();commands.Detach();allocator.Detach();clear_fence.Detach();chain.Detach();gpu.queue.Detach();gpu.device.Detach();CloseHandle(child.hProcess);throw;}
        if(publisher.frames!=serial){CloseHandle(child.hProcess);throw std::runtime_error("GPU producer stalled");}
    }
    DWORD wait=WaitForSingleObject(child.hProcess,10000),code=1;if(wait==WAIT_OBJECT_0)GetExitCodeProcess(child.hProcess,&code);CloseHandle(child.hProcess);
    // Stop notification forbids further captures while preserving the shared
    // slot and handles until the producer's GPU work is drained and closed.
    auto stopped_sequence=InterlockedCompareExchange64(&metadata->sequence,0,0);auto stopped_ticket=metadata->ticket;
    publisher.mark_stopped();publisher.mark_stopped();
    if(publisher.state!=4||!InterlockedCompareExchange(&metadata->stopped,0,0)||publisher.capture(chain.Get())||
       InterlockedCompareExchange64(&metadata->sequence,0,0)!=stopped_sequence||!same_ticket(metadata->ticket,stopped_ticket))
        throw std::runtime_error("Stopped producer submitted work or changed the retained slot");
    if(!publisher.drain(1000))throw std::runtime_error("Stopped producer failed to drain");
    UnmapViewOfFile(metadata);CloseHandle(metadata_map);
    publisher.poll();bool released=publisher.close();
    gpu.verify();
    printf("{\"format\":%u,\"published\":%u,\"busy_skips\":%u,\"rate_limited\":%u,\"consumer_exit\":%lu,\"resources_released\":%s}\n",format,publisher.frames.load(),publisher.busy.load(),publisher.rate_limited.load(),code,released?"true":"false");
    if(stereo?publisher.rate_limited.load()!=0:publisher.rate_limited.load()==0)throw std::runtime_error("Stereo/legacy pacing policy violated");
    if(code || !released || !publisher.busy)throw std::runtime_error("Cross-process GPU self-test failed");
    if(trace_success!=6||!trace_busy)throw std::runtime_error("Trace/null success and busy branches not exercised");
    printf("{\"trace_success\":%u,\"trace_busy\":%u,\"null_success\":6}\n",trace_success,trace_busy);
    if(!busy_ticket_checks)throw std::runtime_error("Busy ticket preservation not exercised");
    printf("{\"stereo_tickets\":%s,\"simulated_source\":%s,\"invalid_ticket_cases\":13,\"busy_ticket_checks\":%u}\n",stereo?"true":"false",allow_sim?"true":"false",busy_ticket_checks);
}
static unsigned resize_checks{};
static void resize_check(bool value,const char* message){++resize_checks;if(!value)throw std::runtime_error(message);}
struct ResizeGpuWork {
    Gpu& gpu;ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;
    ComPtr<ID3D12Fence> fence;UINT64 serial{};
    explicit ResizeGpuWork(Gpu& value):gpu(value){
        gpu_check(gpu.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"resize allocator");
        gpu_check(gpu.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)),"resize list");
        gpu_check(list->Close(),"resize initial close");gpu_check(gpu.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"resize local fence");
    }
    void finish(){
        gpu_check(gpu.queue->Signal(fence.Get(),++serial),"resize finish signal");const auto deadline=GetTickCount64()+5000;
        while(fence->GetCompletedValue()<serial&&GetTickCount64()<deadline)Sleep(1);
        resize_check(fence->GetCompletedValue()!=~UINT64(0)&&fence->GetCompletedValue()>=serial,"resize GPU completion timeout");
    }
    void begin(){finish();gpu_check(allocator->Reset(),"resize allocator reset");gpu_check(list->Reset(allocator.Get(),nullptr),"resize list reset");}
    void execute(){gpu_check(list->Close(),"resize list close");ID3D12CommandList* lists[]{list.Get()};gpu.queue->ExecuteCommandLists(1,lists);}
};
void resize_transport_test(){
    // Separate endpoint queues, opening the actual shared resource/fences on
    // the receiving endpoint. D3D12 may return the same adapter device singleton
    // in one process; this still performs a GPU transfer and readback, not a mock.
    Gpu producer,consumer;ResizeGpuWork writer(producer),reader(consumer);
    ComPtr<IDXGISwapChain1> chain;DXGI_SWAP_CHAIN_DESC1 desc{};desc.Width=640;desc.Height=360;
    desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.BufferCount=2;
    desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;desc.AlphaMode=DXGI_ALPHA_MODE_PREMULTIPLIED;
    gpu_check(producer.factory->CreateSwapChainForComposition(producer.queue.Get(),&desc,nullptr,&chain),"resize test chain");
    ComPtr<IDXGISwapChain3> current;gpu_check(chain.As(&current),"resize chain3");
    GpuPublisher publisher;GpuReceiver receiver;
    resize_check(publisher.bind(producer.queue.Get(),chain.Get())&&publisher.prepare(),"resize publisher initial prepare");
    receiver.open(consumer.device.Get(),GetCurrentProcessId());
    resize_check(receiver.generation()==1&&receiver.width==640&&receiver.height==360,"resize initial generation/dimensions");
    auto fill=[&](float value){
        ComPtr<ID3D12Resource> source;gpu_check(chain->GetBuffer(current->GetCurrentBackBufferIndex(),IID_PPV_ARGS(&source)),"resize source");
        const auto d=source->GetDesc();ComPtr<ID3D12DescriptorHeap> heap;D3D12_DESCRIPTOR_HEAP_DESC h{};h.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;h.NumDescriptors=1;
        gpu_check(producer.device->CreateDescriptorHeap(&h,IID_PPV_ARGS(&heap)),"resize RTV heap");const auto handle=heap->GetCPUDescriptorHandleForHeapStart();
        producer.device->CreateRenderTargetView(source.Get(),nullptr,handle);writer.begin();
        gpu_barrier(writer.list.Get(),source.Get(),D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_RENDER_TARGET);
        const float left[]{value,.25f,.5f,1},right[]{.125f,.75f,value,1};
        const D3D12_RECT a{0,0,LONG(d.Width/2),LONG(d.Height)},b{LONG(d.Width/2),0,LONG(d.Width),LONG(d.Height)};
        writer.list->ClearRenderTargetView(handle,left,1,&a);writer.list->ClearRenderTargetView(handle,right,1,&b);
        gpu_barrier(writer.list.Get(),source.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PRESENT);
        writer.execute();writer.finish(); // Local source/RTV references end before ResizeBuffers.
        writer.begin();gpu_check(writer.list->Close(),"retire resize source recording");
    };
    auto publish=[&](unsigned id,float value){
        fill(value);auto ticket=test_ticket(id,receiver.width,receiver.height);
        const auto deadline=GetTickCount64()+5000;bool captured=false;
        while(!captured&&GetTickCount64()<deadline){captured=publisher.capture(chain.Get(),&ticket);if(!captured)Sleep(1);}
        resize_check(captured,"resize image publish failed");return ticket;
    };
    auto read=[&](StereoImageTicket const& expected,float value,bool begin_resize_while_reading){
        resize_check(receiver.acquire(consumer.queue.Get()),"resize image acquire failed");
        resize_check(same_ticket(receiver.ticket(),expected),"resize ticket/rectangle belongs to another image");
        auto d=receiver.texture->GetDesc();D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout{};UINT64 bytes{};
        consumer.device->GetCopyableFootprints(&d,0,1,0,&layout,nullptr,nullptr,&bytes);
        ComPtr<ID3D12Resource> readback;D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_READBACK;
        D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=bytes;buffer.Height=1;
        buffer.DepthOrArraySize=1;buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        gpu_check(consumer.device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback)),"resize readback");
        reader.begin();gpu_barrier(reader.list.Get(),receiver.texture.Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_SOURCE);
        D3D12_TEXTURE_COPY_LOCATION to{},from{};to.pResource=readback.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=layout;
        from.pResource=receiver.texture.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;reader.list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        gpu_barrier(reader.list.Get(),receiver.texture.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COMMON);
        ComPtr<ID3D12Fence> gate;bool began=true,pending_protected=true,done_protected=true;
        if(begin_resize_while_reading){
            gpu_check(consumer.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&gate)),"resize reader gate");
            gpu_check(consumer.queue->Wait(gate.Get(),1),"stall resize reader");
            began=publisher.begin_resize(chain.Get());pending_protected=!receiver.release_for_resize();
        }
        try{
            reader.execute();receiver.acknowledge(consumer.queue.Get());
            if(gate){done_protected=!receiver.release_for_resize();gpu_check(gate->Signal(1),"resume resize reader");}
            reader.finish();
        }catch(...){if(gate)gate->Signal(1);reader.finish();throw;}
        resize_check(began&&pending_protected&&done_protected,"resize released consumer before acquired/ack GPU work completed");
        void* mapped{};D3D12_RANGE range{0,SIZE_T(bytes)};gpu_check(readback->Map(0,&range,&mapped),"resize readback map");
        bool matched=true;
        for(unsigned y=0;y<d.Height;++y)for(unsigned x=0;x<d.Width;++x){
            const auto* p=static_cast<unsigned char*>(mapped)+layout.Offset+size_t(y)*layout.Footprint.RowPitch+x*4;
            const float color[]{x<d.Width/2?value:.125f,x<d.Width/2?.25f:.75f,x<d.Width/2?.5f:value,1};
            for(unsigned c=0;c<4;++c)if(std::abs(int(p[c])-int(std::lround(color[c]*255)))>1)matched=false;
        }
        D3D12_RANGE empty{};readback->Unmap(0,&empty);resize_check(matched,"resize shared GPU pixels stale or wrong extent");
        reader.begin();gpu_check(reader.list->Close(),"retire resize consumer recording");
    };
    auto rebuild=[&](unsigned width,unsigned height,bool fail,bool already_begun){
        const auto previous=receiver.generation();
        const unsigned old_width=receiver.width,old_height=receiver.height;
        if(!already_begun)resize_check(publisher.begin_resize(chain.Get()),"resize begin rejected");
        resize_check(receiver.reconfiguring()&&!receiver.stopped(),"resize notification must not terminate receiver");
        resize_check(!receiver.acquire(consumer.queue.Get()),"resize admitted old image");
        auto blocked=test_ticket(99,receiver.width,receiver.height);resize_check(!publisher.capture(chain.Get(),&blocked),"resize producer submitted during transition");
        resize_check(publisher.drain(1000),"resize producer source did not drain");
        ComPtr<ID3D12Resource> held;
        if(fail)gpu_check(chain->GetBuffer(0,IID_PPV_ARGS(&held)),"hold buffer for controlled resize failure");
        const auto resized=chain->ResizeBuffers(2,width,height,desc.Format,0);
        held.Reset();publisher.end_resize();
        resize_check(fail?FAILED(resized):SUCCEEDED(resized),"actual DXGI resize outcome differed");
        resize_check(receiver.release_for_resize(),"drained resize receiver did not release claim");
        const auto deadline=GetTickCount64()+2000;
        while(publisher.state==5&&GetTickCount64()<deadline){publisher.poll();Sleep(1);}
        resize_check(publisher.state==1,"resize handoff stuck after consumer released an unread/latest slot");
        resize_check(publisher.prepare(),"resize publisher rebuild failed");
        // Reopen through the public API on the same drained receiver. Its old
        // serial/ticket must not suppress the new generation starting at one.
        receiver.open(consumer.device.Get(),GetCurrentProcessId());
        resize_check(receiver.generation()==previous+1&&receiver.width==(fail?old_width:width)&&receiver.height==(fail?old_height:height),"resize generation/dimensions not replaced");
        resize_check(receiver.sequence()==0&&stereo_image_ticket_empty(receiver.ticket()),"resize receiver retained prior generation history");
        resize_check(!receiver.acquire(consumer.queue.Get()),"new generation fabricated a frame before publication");
    };
    read(publish(1,.15f),.15f,false);read(publish(2,.30f),.30f,true);
    rebuild(1280,720,false,true);
    auto wrong=test_ticket(3,640,360);resize_check(!publisher.capture(chain.Get(),&wrong),"old-sized ticket admitted to resized image");
    read(publish(3,.45f),.45f,false);
    (void)publish(4,.60f); // Intentionally unacquired: nobody can ack this slot after ready becomes 2.
    rebuild(640,360,false,false);
    read(publish(5,.75f),.75f,false);
    rebuild(1280,720,true,false); // Real failed ResizeBuffers, original 640x360 extent remains valid.
    read(publish(6,.90f),.90f,false);
    {
        const auto previous=receiver.generation();
        resize_check(publisher.begin_resize(chain.Get())&&publisher.drain(1000),"consecutive resize first begin/drain");
        gpu_check(chain->ResizeBuffers(2,1280,720,desc.Format,0),"consecutive resize first DXGI call");
        publisher.end_resize();resize_check(receiver.release_for_resize(),"consecutive resize consumer retirement");
        publisher.poll();resize_check(publisher.state==1,"consecutive resize first poll did not reserve rebuild");
        // Deliberately skip prepare: the publisher still owns consumer==-1.
        // A second real resize must preserve that ownership rather than wait
        // forever for a nonexistent consumer to clear it back to zero.
        resize_check(publisher.begin_resize(chain.Get())&&publisher.drain(1000),"consecutive resize second begin/drain");
        gpu_check(chain->ResizeBuffers(2,640,360,desc.Format,0),"consecutive resize second DXGI call");
        publisher.end_resize();publisher.poll();
        resize_check(publisher.state==1&&publisher.prepare(),"consecutive resize lost publisher rebuild reservation");
        receiver.open(consumer.device.Get(),GetCurrentProcessId());
        // Only the final inventory was published, so one transport generation
        // is committed even though DXGI completed two resize boundaries.
        resize_check(receiver.generation()==previous+1&&receiver.width==640&&receiver.height==360,"consecutive resize final generation/extent");
        resize_check(receiver.sequence()==0&&stereo_image_ticket_empty(receiver.ticket())&&!receiver.acquire(consumer.queue.Get()),"consecutive resize retained an old image");
        read(publish(7,.65f),.65f,false);
    }
    publisher.mark_stopped();resize_check(publisher.drain(1000)&&publisher.close(),"resize final publisher close failed");
    receiver.close();producer.verify();consumer.verify();
    printf("{\"resize_transport\":true,\"checks\":%u,\"sizes\":\"640x360->1280x720->640x360\",\"failed_resize_rebuilt\":true,\"pixels_and_tickets_verified\":true,\"unacquired_slot_retired\":true,\"consumer_gpu_done_required\":true,\"consecutive_resize_before_prepare\":true}\n",resize_checks);
}
int wmain(int argc,wchar_t** argv){
    try{
        ComPtr<ID3D12Debug> debug;bool validation=SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));if(validation)debug->EnableDebugLayer();
        printf("{\"d3d12_debug_layer\":%s}\n",validation?"true":"false");
        if(argc>=4 && !wcscmp(argv[1],L"--consume")){
            bool simulated=argc>4&&!wcscmp(argv[4],L"--simulated");bool stereo=simulated||(argc>4&&!wcscmp(argv[4],L"--stereo"));
            return consume(wcstoul(argv[2],nullptr,10),wcstoul(argv[3],nullptr,10),argc>4&&!stereo?argv[4]:nullptr,stereo,simulated);}
        if(argc==2&&!wcscmp(argv[1],L"--trace-stereo")){produce(DXGI_FORMAT_R8G8B8A8_UNORM,true);return 0;}
        if(argc==2&&!wcscmp(argv[1],L"--resize")){resize_transport_test();return 0;}
        if(argc!=1)return 2;
        produce(DXGI_FORMAT_R8G8B8A8_UNORM);produce(DXGI_FORMAT_R10G10B10A2_UNORM);
        produce(DXGI_FORMAT_R8G8B8A8_UNORM,true);produce(DXGI_FORMAT_R10G10B10A2_UNORM,true);
        produce(DXGI_FORMAT_R8G8B8A8_UNORM,true,true);produce(DXGI_FORMAT_R10G10B10A2_UNORM,true,true);resize_transport_test();return 0;
    }catch(std::exception const& e){fprintf(stderr,"%s\n",e.what());return 1;}
}
