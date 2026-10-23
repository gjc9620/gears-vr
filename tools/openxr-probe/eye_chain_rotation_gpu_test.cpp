// Repeated production GpuBlit exercise; independent of a game or XR runtime.
#include "../vr-common/gpu_blit.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>
using namespace gears_vr;
static void require(bool value,char const* message){if(!value)throw std::runtime_error(message);}
static D3D12_RESOURCE_DESC buffer_desc(UINT64 bytes){
    D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=bytes;
    d.Height=d.DepthOrArraySize=d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;return d;
}
static ComPtr<ID3D12Resource> make_resource(ID3D12Device* device,D3D12_RESOURCE_DESC const& desc,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state){
    D3D12_HEAP_PROPERTIES heap{};heap.Type=type;ComPtr<ID3D12Resource> result;
    gpu_check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&result)),"rotation resource");return result;
}
static void run(ID3D12Device* device,ID3D12CommandQueue* queue,bool typeless){
    constexpr unsigned width=257,height=129,frames=96,slots=3;
    constexpr auto format=DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width=width*2;desc.Height=height;desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    auto source=make_resource(device,desc,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST);
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT input{},output{};UINT64 input_bytes{},output_bytes{};
    device->GetCopyableFootprints(&desc,0,1,0,&input,nullptr,nullptr,&input_bytes);
    auto upload=make_resource(device,buffer_desc(input_bytes),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);
    desc.Width=width;desc.Format=format;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    device->GetCopyableFootprints(&desc,0,1,0,&output,nullptr,nullptr,&output_bytes);
    const UINT64 eye_stride=(output_bytes+511)&~UINT64(511);
    auto readback=make_resource(device,buffer_desc(eye_stride*2),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    if(typeless)desc.Format=DXGI_FORMAT_R8G8B8A8_TYPELESS;
    std::array<std::array<ComPtr<ID3D12Resource>,slots>,2> images;
    std::array<std::unique_ptr<GpuBlit>,2> blits;
    for(unsigned eye=0;eye<2;++eye){
        blits[eye]=std::make_unique<GpuBlit>(device,format);
        for(auto& image:images[eye])image=make_resource(device,desc,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_RENDER_TARGET);
    }
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> commands;ComPtr<ID3D12Fence> fence;
    gpu_check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"rotation allocator");
    gpu_check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&commands)),"rotation list");
    gpu_check(commands->Close(),"rotation initial close");
    gpu_check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"rotation fence");
    HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);require(event!=nullptr,"rotation event");
    UINT64 mismatches{},pixels{};D3D12_RANGE none{0,0};
    for(unsigned frame=0;frame<frames;++frame){
        void* mapped{};gpu_check(upload->Map(0,&none,&mapped),"rotation upload map");
        for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width*2;++x){
            auto* pixel=static_cast<unsigned char*>(mapped)+input.Offset+size_t(y)*input.Footprint.RowPitch+x*4;
            pixel[0]=static_cast<unsigned char>(17+(x/width)*101+frame%61);
            pixel[1]=static_cast<unsigned char>((x+frame*3)%251);pixel[2]=static_cast<unsigned char>((y+frame*7)%239);pixel[3]=255;
        }
        upload->Unmap(0,nullptr);
        gpu_check(allocator->Reset(),"rotation reset allocator");gpu_check(commands->Reset(allocator.Get(),nullptr),"rotation reset list");
        if(frame)gpu_barrier(commands.Get(),source.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_DEST);
        D3D12_TEXTURE_COPY_LOCATION to{},from{};to.pResource=source.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        from.pResource=upload.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;from.PlacedFootprint=input;
        commands->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        gpu_barrier(commands.Get(),source.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        for(unsigned eye=0;eye<2;++eye){
            const unsigned index=eye?(frame*2+1)%slots:frame%slots;
            auto* image=images[eye][index].Get();blits[eye]->draw_crop(commands.Get(),source.Get(),image,eye*width,0);
            gpu_barrier(commands.Get(),image,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE);
            to={};from={};to.pResource=readback.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            to.PlacedFootprint=output;to.PlacedFootprint.Offset=eye_stride*eye;
            from.pResource=image;from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            commands->CopyTextureRegion(&to,0,0,0,&from,nullptr);
            gpu_barrier(commands.Get(),image,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
        }
        gpu_check(commands->Close(),"rotation close");ID3D12CommandList* lists[]{commands.Get()};queue->ExecuteCommandLists(1,lists);
        const UINT64 serial=UINT64(frame)+1;
        if(FAILED(queue->Signal(fence.Get(),serial))||FAILED(fence->SetEventOnCompletion(serial,event))||
           WaitForSingleObject(event,10000)!=WAIT_OBJECT_0||fence->GetCompletedValue()!=serial){
            std::fputs("rotation completion unconfirmed\n",stderr);std::_Exit(3);
        }
        D3D12_RANGE range{0,size_t(eye_stride*2)};gpu_check(readback->Map(0,&range,&mapped),"rotation read map");
        for(unsigned eye=0;eye<2;++eye)for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){
            const auto* pixel=static_cast<unsigned char*>(mapped)+eye_stride*eye+size_t(y)*output.Footprint.RowPitch+x*4;
            const unsigned expected[]={17+eye*101+frame%61,(x+eye*width+frame*3)%251,(y+frame*7)%239,255};
            for(unsigned c=0;c<4;++c)if(std::abs(int(pixel[c])-int(expected[c]))>1)++mismatches;
            ++pixels;
        }
        readback->Unmap(0,&none);
    }
    CloseHandle(event);require(mismatches==0,"rotation image pixel mismatch");
    std::printf("rotation SRGB29: typeless=%u frames=%u images_per_eye=%u pixels=%llu mismatches=0 no_XR=1 no_game=1\n",
        unsigned(typeless),frames,slots,static_cast<unsigned long long>(pixels));
}
int main(int argc,char** argv){
    try{
        require(argc==1||(argc==2&&std::strcmp(argv[1],"--hardware")==0),"Expected optional --hardware");
        const bool hardware=argc==2;
        ComPtr<IDXGIFactory4> factory;gpu_check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"rotation factory");
        ComPtr<IDXGIAdapter> adapter;
        if(hardware){
            ComPtr<IDXGIAdapter1> selected;gpu_check(factory->EnumAdapters1(0,&selected),"rotation hardware adapter");
            DXGI_ADAPTER_DESC1 info{};gpu_check(selected->GetDesc1(&info),"rotation adapter info");
            require((info.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)==0,"First adapter is software; hardware test refused");
            std::printf("rotation hardware: vendor=%u device=%u luid=%ld:%lu no_XR=1 no_game=1\n",
                info.VendorId,info.DeviceId,info.AdapterLuid.HighPart,info.AdapterLuid.LowPart);
            gpu_check(selected.As(&adapter),"rotation hardware interface");
        }else gpu_check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)),"rotation WARP");
        ComPtr<ID3D12Device> device;gpu_check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)),"rotation device");
        ComPtr<ID3D12CommandQueue> queue;D3D12_COMMAND_QUEUE_DESC desc{};gpu_check(device->CreateCommandQueue(&desc,IID_PPV_ARGS(&queue)),"rotation queue");
        run(device.Get(),queue.Get(),false);run(device.Get(),queue.Get(),true);return 0;
    }catch(std::exception const& error){std::fprintf(stderr,"%s\n",error.what());return 1;}
}
