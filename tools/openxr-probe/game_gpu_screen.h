#pragma once
#include "../vr-common/gpu_share.h"
#include "../vr-common/gpu_blit.h"

// Mono diagnostic quad. GPU-only copies between game and XR devices on the
// same adapter. No WGC, per-frame CPU pixel readback, scaling or stereo views.
class GameGpuScreen {
    gears_vr::GpuReceiver receiver;
    XrSwapchain chain{};
    std::vector<XrSwapchainImageD3D12KHR> images;
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> commands;
    ComPtr<ID3D12Fence> fence;
    HANDLE event{};UINT64 serial=0;
    std::chrono::steady_clock::time_point last_frame{};
    bool current=false,in_flight=false;
    std::unique_ptr<gears_vr::GpuBlit> blit;
    ComPtr<ID3D12Resource> snapshot_buffer;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT snapshot_layout{};
    UINT64 snapshot_bytes=0;
    std::string snapshot_path;
    std::chrono::steady_clock::time_point snapshot_after=std::chrono::steady_clock::now()+std::chrono::seconds(10);
    void save_snapshot(){
        void* mapped{};D3D12_RANGE range{0,size_t(snapshot_bytes)};
        dx_check(snapshot_buffer->Map(0,&range,&mapped),"GPU snapshot map");
        const auto* data=static_cast<const unsigned char*>(mapped)+snapshot_layout.Offset;
        auto format=snapshot_layout.Footprint.Format;
        unsigned width=std::min(receiver.width,640u),height=unsigned(uint64_t(receiver.height)*width/receiver.width);
        std::vector<unsigned char> rgb(size_t(width)*height*3);
        for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){
            const auto* p=data+size_t(uint64_t(y)*receiver.height/height)*snapshot_layout.Footprint.RowPitch+size_t(uint64_t(x)*receiver.width/width)*4;
            auto* out=rgb.data()+(size_t(y)*width+x)*3;
            if(format==DXGI_FORMAT_R10G10B10A2_UNORM){uint32_t value{};memcpy(&value,p,4);for(unsigned c=0;c<3;++c)out[c]=static_cast<unsigned char>(((value>>(c*10))&1023)*255/1023);}
            else {bool bgra=format==DXGI_FORMAT_B8G8R8A8_UNORM || format==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;out[0]=p[bgra?2:0];out[1]=p[1];out[2]=p[bgra?0:2];}
        }
        D3D12_RANGE written{0,0};snapshot_buffer->Unmap(0,&written);
        FILE* file{};fopen_s(&file,snapshot_path.c_str(),"wb");if(!file)throw std::runtime_error("GPU snapshot file unavailable");
        fprintf(file,"P6\n%u %u\n255\n",width,height);auto count=fwrite(rgb.data(),1,rgb.size(),file);fclose(file);
        if(count!=rgb.size())throw std::runtime_error("GPU snapshot write incomplete");
        snapshot_saved=true;snapshot_buffer.Reset();
    }
public:
    unsigned rendered=0,copies=0;
    bool snapshot_saved=false;
    GameGpuScreen(ID3D12Device* d,ID3D12CommandQueue* q,DWORD pid,const char* snapshot=nullptr):device(d),queue(q),snapshot_path(snapshot?snapshot:""){
        try{
        receiver.open(d,pid);
        uint32_t count{};xr_check(xrEnumerateSwapchainFormats(session,0,&count,nullptr),"GPU game formats");
        std::vector<int64_t> formats(count);xr_check(xrEnumerateSwapchainFormats(session,count,&count,formats.data()),"GPU game formats read");
        DXGI_FORMAT format=receiver.format;
        if(std::find(formats.begin(),formats.end(),int64_t(format))==formats.end()){
            format=DXGI_FORMAT_UNKNOWN;
            for(auto candidate:{DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_B8G8R8A8_UNORM})
                if(std::find(formats.begin(),formats.end(),int64_t(candidate))!=formats.end()){format=candidate;break;}
            if(!format)throw std::runtime_error("No supported SDR GPU conversion target");
            blit=std::make_unique<gears_vr::GpuBlit>(d,format);
        }
        XrSwapchainCreateInfo create{XR_TYPE_SWAPCHAIN_CREATE_INFO};create.usageFlags=XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT|XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
        create.format=format;create.width=receiver.width;create.height=receiver.height;create.sampleCount=1;create.faceCount=1;create.arraySize=1;create.mipCount=1;
        xr_check(xrCreateSwapchain(session,&create,&chain),"GPU game swapchain");
        xr_check(xrEnumerateSwapchainImages(chain,0,&count,nullptr),"GPU game images");images.resize(count,{XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR});
        xr_check(xrEnumerateSwapchainImages(chain,count,&count,reinterpret_cast<XrSwapchainImageBaseHeader*>(images.data())),"GPU game images read");
        dx_check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"GPU game allocator");
        dx_check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&commands)),"GPU game commands");dx_check(commands->Close(),"GPU game initial close");
        dx_check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"GPU game fence");event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
        if(!event)throw std::runtime_error("GPU game fence event unavailable");
        }catch(...){if(chain){xrDestroySwapchain(chain);chain=XR_NULL_HANDLE;}if(event){CloseHandle(event);event=nullptr;}throw;}
    }
    ~GameGpuScreen(){
        if(in_flight){commands.Detach();allocator.Detach();fence.Detach();device.Detach();queue.Detach();snapshot_buffer.Detach();blit.release();}
        else if(chain)xrDestroySwapchain(chain);
        if(event)CloseHandle(event);
    }
    bool render(XrCompositionLayerQuad& quad){
        if(receiver.stopped())return false;
        auto now=std::chrono::steady_clock::now();
        if(receiver.acquire(queue.Get())){
            uint32_t index{};XrSwapchainImageAcquireInfo acquire{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};xr_check(xrAcquireSwapchainImage(chain,&acquire,&index),"GPU game acquire");
            XrSwapchainImageWaitInfo wait{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wait.timeout=1000000000;xr_check(xrWaitSwapchainImage(chain,&wait),"GPU game wait");
            auto* image=images.at(index).texture;
            dx_check(allocator->Reset(),"GPU game reset");dx_check(commands->Reset(allocator.Get(),nullptr),"GPU game commands reset");
            if(blit){
                gears_vr::gpu_barrier(commands.Get(),receiver.texture.Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                blit->draw(commands.Get(),receiver.texture.Get(),image);
                gears_vr::gpu_barrier(commands.Get(),receiver.texture.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COMMON);
            }else{
                gears_vr::gpu_barrier(commands.Get(),receiver.texture.Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_SOURCE);
                gears_vr::gpu_barrier(commands.Get(),image,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_DEST);
                commands->CopyResource(image,receiver.texture.Get());
                gears_vr::gpu_barrier(commands.Get(),receiver.texture.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COMMON);
                gears_vr::gpu_barrier(commands.Get(),image,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_RENDER_TARGET);
            }
            if(!snapshot_path.empty() && !snapshot_saved && now>=snapshot_after){
                auto desc=image->GetDesc();device->GetCopyableFootprints(&desc,0,1,0,&snapshot_layout,nullptr,nullptr,&snapshot_bytes);
                D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_READBACK;
                D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=snapshot_bytes;buffer.Height=1;buffer.DepthOrArraySize=1;buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
                dx_check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&snapshot_buffer)),"GPU snapshot buffer");
                gears_vr::gpu_barrier(commands.Get(),image,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE);
                D3D12_TEXTURE_COPY_LOCATION to{};to.pResource=snapshot_buffer.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=snapshot_layout;
                D3D12_TEXTURE_COPY_LOCATION from{};from.pResource=image;from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                commands->CopyTextureRegion(&to,0,0,0,&from,nullptr);
                gears_vr::gpu_barrier(commands.Get(),image,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
            }
            dx_check(commands->Close(),"GPU game close");ID3D12CommandList* lists[]={commands.Get()};in_flight=true;queue->ExecuteCommandLists(1,lists);
            receiver.acknowledge(queue.Get());dx_check(queue->Signal(fence.Get(),++serial),"GPU game local fence");
            if(fence->GetCompletedValue()==~UINT64(0))throw std::runtime_error("GPU game device removed");
            if(fence->GetCompletedValue()<serial){dx_check(fence->SetEventOnCompletion(serial,event),"GPU game fence event");if(WaitForSingleObject(event,3000)!=WAIT_OBJECT_0)throw std::runtime_error("GPU game fence timeout");}
            if(fence->GetCompletedValue()==~UINT64(0))throw std::runtime_error("GPU game device removed");
            in_flight=false;
            if(snapshot_buffer)save_snapshot();
            XrSwapchainImageReleaseInfo release{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};xr_check(xrReleaseSwapchainImage(chain,&release),"GPU game release");
            ++copies;current=true;last_frame=now;
        }
        if(!current || now-last_frame>std::chrono::seconds(1))return false;
        quad={XR_TYPE_COMPOSITION_LAYER_QUAD};quad.space=view_space;quad.eyeVisibility=XR_EYE_VISIBILITY_BOTH;
        quad.pose.orientation.w=1;quad.pose.position.z=-2;quad.size={1.6f,1.6f*receiver.height/receiver.width};
        quad.subImage.swapchain=chain;quad.subImage.imageRect.extent={int32_t(receiver.width),int32_t(receiver.height)};++rendered;return true;
    }
};
