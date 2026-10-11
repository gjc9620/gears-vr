#include "../vr-common/gpu_share.h"
#include "../vr-common/gpu_blit.h"
#include <vector>
#include <cstdio>
#include <cmath>
#include <memory>
#include <d3d12sdklayers.h>
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
int consume(DWORD pid,unsigned expected,const wchar_t* output){
    Gpu gpu;GpuReceiver receiver;receiver.open(gpu.device.Get(),pid);
    // A second consumer must be rejected, including in the same process.
    if(expected){bool rejected=false;try{GpuReceiver duplicate;duplicate.open(gpu.device.Get(),pid);}catch(...){rejected=true;}if(!rejected)throw std::runtime_error("Duplicate consumer accepted");}
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
        if(expected && !frames)Sleep(150); // Slow reader: producer must skip.
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
            ULONGLONG wait_deadline=GetTickCount64()+3000;
            while(fence->GetCompletedValue()<frames+1 && GetTickCount64()<wait_deadline)Sleep(1);
            if(fence->GetCompletedValue()<frames+1 || fence->GetCompletedValue()==~UINT64(0))throw std::runtime_error("Consumer fence timeout/device removed");
        }catch(...){buffer.Detach();commands.Detach();allocator.Detach();fence.Detach();converted.Detach();blit.release();gpu.queue.Detach();gpu.device.Detach();throw;}
        unsigned char* data{};D3D12_RANGE range{0,static_cast<SIZE_T>(bytes)};gpu_check(buffer->Map(0,&range,reinterpret_cast<void**>(&data)),"map verification");
        unsigned out_width=expected?receiver.width:(receiver.width<640?receiver.width:640);
        unsigned out_height=static_cast<unsigned>(UINT64(receiver.height)*out_width/receiver.width);
        std::vector<unsigned char> rgb(static_cast<size_t>(out_width)*out_height*3);UINT64 hash=14695981039346656037ull;bool matches=true;
        int expected_rgb[]={static_cast<int>(std::lround((receiver.sequence()%8+1)*.1*255)),128,191};
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
void produce(DXGI_FORMAT format){
    Gpu gpu;ComPtr<IDXGISwapChain1> chain;DXGI_SWAP_CHAIN_DESC1 desc{};desc.Width=19;desc.Height=13;desc.Format=format;desc.SampleDesc.Count=1;desc.BufferCount=2;
    desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;desc.AlphaMode=DXGI_ALPHA_MODE_PREMULTIPLIED;
    gpu_check(gpu.factory->CreateSwapChainForComposition(gpu.queue.Get(),&desc,nullptr,&chain),"test swapchain");GpuPublisher publisher;
    if(!publisher.bind(gpu.queue.Get(),chain.Get()) || !publisher.prepare())throw std::runtime_error("Publisher setup failed");
    wchar_t exe[32768];GetModuleFileNameW(nullptr,exe,32768);std::wstring command=L"\""+std::wstring(exe)+L"\" --consume "+std::to_wstring(GetCurrentProcessId())+L" 12";
    STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION child{};
    if(!CreateProcessW(nullptr,command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&child))throw std::runtime_error("Test consumer launch failed");
    CloseHandle(child.hThread);
    ComPtr<ID3D12Resource> source;gpu_check(chain->GetBuffer(0,IID_PPV_ARGS(&source)),"test source");
    ComPtr<ID3D12DescriptorHeap> rtv;D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;hd.NumDescriptors=1;gpu_check(gpu.device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&rtv)),"RTV");gpu.device->CreateRenderTargetView(source.Get(),nullptr,rtv->GetCPUDescriptorHandleForHeapStart());
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> commands;gpu_check(gpu.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"clear allocator");
    gpu_check(gpu.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&commands)),"clear list");gpu_check(commands->Close(),"clear initial close");
    ComPtr<ID3D12Fence> clear_fence;gpu_check(gpu.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&clear_fence)),"clear fence");
    for(unsigned serial=1;serial<=12;++serial){
        gpu_check(allocator->Reset(),"clear reset");gpu_check(commands->Reset(allocator.Get(),nullptr),"clear command reset");
        gpu_barrier(commands.Get(),source.Get(),D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_RENDER_TARGET);
        float color[]={float(serial%8+1)*.1f,.5f,.75f,1.f};commands->ClearRenderTargetView(rtv->GetCPUDescriptorHandleForHeapStart(),color,0,nullptr);
        gpu_barrier(commands.Get(),source.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PRESENT);gpu_check(commands->Close(),"clear close");
        ID3D12CommandList* lists[]={commands.Get()};gpu.queue->ExecuteCommandLists(1,lists);
        try{
            gpu_check(gpu.queue->Signal(clear_fence.Get(),serial),"clear signal");
            ULONGLONG deadline=GetTickCount64()+5000;
            while(publisher.frames<serial && GetTickCount64()<deadline){publisher.capture(chain.Get());Sleep(1);}
            while(clear_fence->GetCompletedValue()<serial && GetTickCount64()<deadline)Sleep(1);
            if(clear_fence->GetCompletedValue()<serial || clear_fence->GetCompletedValue()==~UINT64(0))throw std::runtime_error("Clear fence timeout/device removed");
        }catch(...){source.Detach();rtv.Detach();commands.Detach();allocator.Detach();clear_fence.Detach();chain.Detach();gpu.queue.Detach();gpu.device.Detach();CloseHandle(child.hProcess);throw;}
        if(publisher.frames!=serial){CloseHandle(child.hProcess);throw std::runtime_error("GPU producer stalled");}
    }
    DWORD wait=WaitForSingleObject(child.hProcess,10000),code=1;if(wait==WAIT_OBJECT_0)GetExitCodeProcess(child.hProcess,&code);CloseHandle(child.hProcess);
    publisher.poll();bool released=publisher.close();
    gpu.verify();
    printf("{\"format\":%u,\"published\":%u,\"busy_skips\":%u,\"consumer_exit\":%lu,\"resources_released\":%s}\n",format,publisher.frames.load(),publisher.busy.load(),code,released?"true":"false");
    if(code || !released || !publisher.busy)throw std::runtime_error("Cross-process GPU self-test failed");
}
int wmain(int argc,wchar_t** argv){
    try{
        ComPtr<ID3D12Debug> debug;bool validation=SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));if(validation)debug->EnableDebugLayer();
        printf("{\"d3d12_debug_layer\":%s}\n",validation?"true":"false");
        if(argc>=4 && !wcscmp(argv[1],L"--consume"))return consume(wcstoul(argv[2],nullptr,10),wcstoul(argv[3],nullptr,10),argc>4?argv[4]:nullptr);
        if(argc!=1)return 2;
        produce(DXGI_FORMAT_R8G8B8A8_UNORM);produce(DXGI_FORMAT_R10G10B10A2_UNORM);return 0;
    }catch(std::exception const& e){fprintf(stderr,"%s\n",e.what());return 1;}
}
