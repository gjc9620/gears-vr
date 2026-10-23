#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>
#include "transition_card_pixels.h"
#include "transition_card_layout.h"

// These are owned by the existing XR session, not by the game-image receiver.
extern XrSession session;
extern XrSpace view_space;
extern bool xr_gpu_teardown_unsafe;

// A literal UI card, never a reconstructed or stale game frame. Create/destroy
// on the XR thread, with destruction before the owning session is destroyed.
class TransitionCard {
    template<class T> using Ptr=Microsoft::WRL::ComPtr<T>;
    const XrSession owner_session;
    XrSwapchain chain{XR_NULL_HANDLE};
    std::vector<XrSwapchainImageD3D12KHR> images;
    Ptr<ID3D12Device> device;
    Ptr<ID3D12CommandQueue> queue;
    Ptr<ID3D12CommandAllocator> allocator;
    Ptr<ID3D12GraphicsCommandList> commands;
    Ptr<ID3D12Resource> upload;
    Ptr<ID3D12Fence> fence;
    HANDLE event{};
    DXGI_FORMAT format{};
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
    std::uint64_t serial{};
    bool acquired{},waited{},in_flight{},current{},failed{},retained{},release_failed{};
    const std::vector<std::uint8_t> pixels;

    static void exact(XrResult result,char const* action){
        if(result!=XR_SUCCESS)throw std::runtime_error(std::string(action)+": "+std::to_string(result));
    }
    static void dx(HRESULT result,char const* action){
        if(FAILED(result))throw std::runtime_error(std::string(action)+": "+std::to_string(result));
    }
    bool completed()const noexcept{
        if(!fence)return false;
        const auto value=fence->GetCompletedValue();
        // UINT64_MAX denotes device removal, not a completed submission.
        return value!=(std::numeric_limits<std::uint64_t>::max)()&&value>=serial;
    }
    void teardown()noexcept{
        if(retained)return;
        if(in_flight&&completed())in_flight=false;
        bool unsafe=in_flight||release_failed;
        if(acquired&&!unsafe){
            if(!waited)unsafe=true;
            else{
                XrSwapchainImageReleaseInfo release{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
                if(xrReleaseSwapchainImage(chain,&release)!=XR_SUCCESS)unsafe=true;
                else acquired=waited=false;
            }
        }
        // The chain goes first while our device/queue/upload are still alive.
        if(!unsafe&&chain){
            if(xrDestroySwapchain(chain)!=XR_SUCCESS)unsafe=true;
            else chain=XR_NULL_HANDLE;
        }
        if(unsafe){
            xr_gpu_teardown_unsafe=true;retained=true;
            // Keep the swapchain/session, GPU objects and completion-event
            // handle alive until process exit if ownership/work is uncertain.
            commands.Detach();allocator.Detach();upload.Detach();fence.Detach();device.Detach();queue.Detach();
        }else if(event){CloseHandle(event);event=nullptr;}
        current=false;
    }
    void barrier(ID3D12Resource* image,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){
        D3D12_RESOURCE_BARRIER value{};value.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        value.Transition.pResource=image;value.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        value.Transition.StateBefore=before;value.Transition.StateAfter=after;commands->ResourceBarrier(1,&value);
    }
    void upload_once(){
        std::uint32_t index{};
        XrSwapchainImageAcquireInfo acquire{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        exact(xrAcquireSwapchainImage(chain,&acquire,&index),"Acquire transition card");acquired=true;
        XrSwapchainImageWaitInfo wait{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wait.timeout=1000000000;
        // XR_TIMEOUT_EXPIRED is a positive result, but never authorizes writes
        // or release. Only an exact success transfers image-write authority.
        exact(xrWaitSwapchainImage(chain,&wait),"Wait transition card");waited=true;
        if(index>=images.size()||!images[index].texture)throw std::runtime_error("Invalid transition-card image index");
        auto* image=images[index].texture;const auto desc=image->GetDesc();
        if(!transition_card_layout::valid_image(desc,format,transition_card_pixels::width,transition_card_pixels::height))
            throw std::runtime_error("Transition-card image layout changed");
        dx(allocator->Reset(),"Reset transition-card allocator");dx(commands->Reset(allocator.Get(),nullptr),"Reset transition-card commands");
        barrier(image,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_DEST);
        D3D12_TEXTURE_COPY_LOCATION destination{},source{};
        destination.pResource=image;destination.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        source.pResource=upload.Get();source.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;source.PlacedFootprint=footprint;
        commands->CopyTextureRegion(&destination,0,0,0,&source,nullptr);
        barrier(image,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_RENDER_TARGET);
        dx(commands->Close(),"Close transition-card commands");
        ID3D12CommandList* lists[]{commands.Get()};in_flight=true;
        queue->ExecuteCommandLists(1,lists);
        dx(queue->Signal(fence.Get(),++serial),"Signal transition-card fence");
        if(!completed()){
            dx(fence->SetEventOnCompletion(serial,event),"Set transition-card fence event");
            if(WaitForSingleObject(event,3000)!=WAIT_OBJECT_0||!completed())
                throw std::runtime_error("Transition-card GPU completion unconfirmed");
        }
        in_flight=false;
        XrSwapchainImageReleaseInfo release{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        const auto released=xrReleaseSwapchainImage(chain,&release);
        if(released!=XR_SUCCESS){release_failed=true;exact(released,"Release transition card");}
        acquired=waited=false;current=true;++uploads;
    }
public:
    unsigned uploads{},rendered{};
    TransitionCard(ID3D12Device* d,ID3D12CommandQueue* q,
        transition_card_pixels::Variant variant=transition_card_pixels::Variant::returning_to_game):
        owner_session(session),device(d),queue(q),pixels(transition_card_pixels::rgba(variant)){
        try{
            if(!owner_session||!d||!q||q->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT)
                throw std::runtime_error("Transition-card session/device/direct queue required");
            std::uint32_t count{};exact(xrEnumerateSwapchainFormats(owner_session,0,&count,nullptr),"Count transition-card formats");
            if(!count)throw std::runtime_error("No transition-card formats");
            std::vector<std::int64_t> formats(count);
            exact(xrEnumerateSwapchainFormats(owner_session,count,&count,formats.data()),"Read transition-card formats");
            format=menu_swapchain_format::choose(formats);
            if(!format)throw std::runtime_error("No transition-card 8-bit SDR format");
            std::printf("{\"xr_color_policy\":\"%s\",\"path\":\"transition_card\",\"selected_format\":%u}\n",menu_swapchain_format::srgb(format)?"srgb":"unorm",unsigned(format));std::fflush(stdout);
            XrSwapchainCreateInfo create{XR_TYPE_SWAPCHAIN_CREATE_INFO};create.createFlags=XR_SWAPCHAIN_CREATE_STATIC_IMAGE_BIT;
            create.usageFlags=XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT|XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
            create.format=format;create.sampleCount=1;create.width=transition_card_pixels::width;create.height=transition_card_pixels::height;
            create.faceCount=create.arraySize=create.mipCount=1;
            exact(xrCreateSwapchain(owner_session,&create,&chain),"Create transition-card swapchain");
            exact(xrEnumerateSwapchainImages(chain,0,&count,nullptr),"Count transition-card images");
            if(!count||count>16)throw std::runtime_error("Invalid transition-card image count");
            images.resize(count,{XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR});const auto capacity=count;
            exact(xrEnumerateSwapchainImages(chain,capacity,&count,reinterpret_cast<XrSwapchainImageBaseHeader*>(images.data())),"Read transition-card images");
            if(count!=capacity||!images[0].texture)throw std::runtime_error("Transition-card images changed");
            const auto image_desc=images[0].texture->GetDesc();UINT64 bytes{},row_size{};UINT rows{};
            // OpenXR runtimes may expose a typeless texture of the selected
            // format family. The buffer copy footprint must use its typed format.
            auto copy_desc=image_desc;copy_desc.Format=format;
            device->GetCopyableFootprints(&copy_desc,0,1,0,&footprint,&rows,&row_size,&bytes);
            std::fprintf(stderr,"Transition-card layout: selected=%u resource={dimension=%u width=%llu height=%u format=%u array=%u mips=%u samples=%u quality=%u layout=%u flags=%u} upload={format=%u width=%u height=%u depth=%u pitch=%u offset=%llu rows=%u row_bytes=%llu bytes=%llu}\n",
                unsigned(format),unsigned(image_desc.Dimension),static_cast<unsigned long long>(image_desc.Width),image_desc.Height,
                unsigned(image_desc.Format),unsigned(image_desc.DepthOrArraySize),unsigned(image_desc.MipLevels),
                image_desc.SampleDesc.Count,image_desc.SampleDesc.Quality,unsigned(image_desc.Layout),unsigned(image_desc.Flags),
                unsigned(footprint.Footprint.Format),footprint.Footprint.Width,footprint.Footprint.Height,footprint.Footprint.Depth,
                footprint.Footprint.RowPitch,static_cast<unsigned long long>(footprint.Offset),rows,
                static_cast<unsigned long long>(row_size),static_cast<unsigned long long>(bytes));
            if(!transition_card_layout::valid_image(image_desc,format,transition_card_pixels::width,transition_card_pixels::height))
                throw std::runtime_error("Invalid transition-card image layout");
            if(!transition_card_layout::valid_footprint(footprint,bytes,format,transition_card_pixels::width,transition_card_pixels::height,rows,row_size))
                throw std::runtime_error("Invalid transition-card upload footprint");
            D3D12_HEAP_PROPERTIES properties{};properties.Type=D3D12_HEAP_TYPE_UPLOAD;
            D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=bytes;buffer.Height=1;
            buffer.DepthOrArraySize=buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            dx(device->CreateCommittedResource(&properties,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&upload)),"Create transition-card upload");
            void* mapped{};D3D12_RANGE read{0,0};dx(upload->Map(0,&read,&mapped),"Map transition-card upload");
            std::memset(mapped,0,std::size_t(bytes));
            for(unsigned y=0;y<transition_card_pixels::height;++y){
                auto* destination=static_cast<unsigned char*>(mapped)+footprint.Offset+std::size_t(y)*footprint.Footprint.RowPitch;
                const auto* source=pixels.data()+std::size_t(y)*transition_card_pixels::width*4;
                for(unsigned x=0;x<transition_card_pixels::width;++x){const bool bgra=menu_swapchain_format::bgra(format);
                    destination[x*4]=source[x*4+(bgra?2:0)];destination[x*4+1]=source[x*4+1];
                    destination[x*4+2]=source[x*4+(bgra?0:2)];destination[x*4+3]=source[x*4+3];}
            }
            D3D12_RANGE written{0,std::size_t(bytes)};upload->Unmap(0,&written);
            dx(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"Create transition-card allocator");
            dx(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&commands)),"Create transition-card commands");
            dx(commands->Close(),"Initial transition-card command close");
            dx(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"Create transition-card fence");
            event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event)throw std::runtime_error("Transition-card fence event unavailable");
        }catch(...){teardown();throw;}
    }
    TransitionCard(TransitionCard const&)=delete;
    TransitionCard& operator=(TransitionCard const&)=delete;
    ~TransitionCard(){teardown();}
    bool save_source_preview(std::string const& path)const{return transition_card_pixels::save_ppm(path,pixels);}
    bool render(XrCompositionLayerQuad& quad){
        if(failed||retained||session!=owner_session||!view_space)return false;
        try{if(!current)upload_once();}catch(...){failed=true;teardown();throw;}
        XrCompositionLayerQuad result{XR_TYPE_COMPOSITION_LAYER_QUAD};result.space=view_space;result.eyeVisibility=XR_EYE_VISIBILITY_BOTH;
        result.pose.orientation.w=1;result.pose.position.z=-2;
        result.size={1.4f,1.4f*transition_card_pixels::height/transition_card_pixels::width};
        result.subImage.swapchain=chain;result.subImage.imageRect.extent={std::int32_t(transition_card_pixels::width),std::int32_t(transition_card_pixels::height)};
        quad=result;++rendered;return true;
    }
};
