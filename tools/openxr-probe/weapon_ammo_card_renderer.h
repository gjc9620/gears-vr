#pragma once
#include "weapon_ammo_card_pixels.h"
#include "transition_card_layout.h"
#include <wrl/client.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <algorithm>
#include <cstring>
#include <functional>
#include <limits>
#include <stdexcept>

namespace weapon_ammo_card {
inline bool make_layer(Plan const& plan,XrSpace local_space,XrSwapchain chain,XrCompositionLayerQuad& out){
    if(!local_space||!chain||!valid(plan.identity)||!valid(plan.frame)||!valid(plan.counts)||
       !gears_vr::stereo_pose_valid(plan.pose_in_local_m)||
       !std::isfinite(plan.width_m)||!std::isfinite(plan.height_m)||
       plan.width_m<.04||plan.width_m>.4||plan.height_m<.02||plan.height_m>.2||
       std::abs(plan.width_m/plan.height_m-3)>1e-9)return false;
    auto const& p=plan.pose_in_local_m;auto const& q=p.orientation;
    XrCompositionLayerQuad layer{XR_TYPE_COMPOSITION_LAYER_QUAD};
    layer.space=local_space;layer.eyeVisibility=XR_EYE_VISIBILITY_BOTH;
    layer.pose={{float(q.x),float(q.y),float(q.z),float(q.w)},
        {float(p.position.x),float(p.position.y),float(p.position.z)}};
    if(!std::isfinite(layer.pose.position.x)||!std::isfinite(layer.pose.position.y)||
       !std::isfinite(layer.pose.position.z))return false;
    layer.size={float(plan.width_m),float(plan.height_m)};
    layer.subImage.swapchain=chain;
    layer.subImage.imageRect.extent={std::int32_t(pixels::width),std::int32_t(pixels::height)};
    out=layer;return true;
}

// Candidate only: no production caller or HUD-suppression hook is installed.
// Own and destroy on the XR thread before its LOCAL space/session. Append the
// returned opaque quad after the matching world layer for scene-independent
// contrast. It deliberately has no scene-depth occlusion or HMD-facing rotation.
class Renderer {
    template<class T> using Ptr=Microsoft::WRL::ComPtr<T>;
    XrSession owner_{};
    std::uint64_t bridge_session_{};
    bool& teardown_unsafe_;
    XrSwapchain chain_{};
    std::vector<XrSwapchainImageD3D12KHR> images_;
    Ptr<ID3D12Device> device_;
    Ptr<ID3D12CommandQueue> queue_;
    Ptr<ID3D12CommandAllocator> allocator_;
    Ptr<ID3D12GraphicsCommandList> commands_;
    Ptr<ID3D12Resource> upload_;
    Ptr<ID3D12Fence> fence_;
    HANDLE event_{};
    DXGI_FORMAT format_{};
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint_{};
    std::uint64_t bytes_{},serial_{};
    bool acquired_{},waited_{},in_flight_{},release_failed_{},retained_{},failed_{},current_{};
    Counts displayed_{};
    FrameKey last_frame_{};

    static void exact(XrResult result,char const* action){
        if(result!=XR_SUCCESS)throw std::runtime_error(std::string(action)+": "+std::to_string(result));
    }
    static void dx(HRESULT result,char const* action){
        if(FAILED(result))throw std::runtime_error(std::string(action)+": "+std::to_string(result));
    }
    bool completed()const noexcept {
        if(!fence_)return false;
        const auto value=fence_->GetCompletedValue();
        return value!=(std::numeric_limits<std::uint64_t>::max)()&&value>=serial_;
    }
    void teardown()noexcept {
        if(retained_)return;
        if(in_flight_&&completed())in_flight_=false;
        bool unsafe=in_flight_||release_failed_;
        if(acquired_&&!unsafe){
            if(!waited_)unsafe=true;
            else{XrSwapchainImageReleaseInfo release{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
                if(xrReleaseSwapchainImage(chain_,&release)!=XR_SUCCESS)unsafe=true;
                else acquired_=waited_=false;}
        }
        if(!unsafe&&chain_){
            if(xrDestroySwapchain(chain_)!=XR_SUCCESS)unsafe=true;
            else chain_=XR_NULL_HANDLE;
        }
        // Match the established XR resource-retention policy. A device-removal,
        // unconfirmed GPU fence or unowned image must not be freed underneath XR.
        if(unsafe){
            teardown_unsafe_=true;retained_=true;
            commands_.Detach();allocator_.Detach();upload_.Detach();fence_.Detach();device_.Detach();queue_.Detach();
        }else if(event_){CloseHandle(event_);event_=nullptr;}
        current_=false;
    }
    void barrier(ID3D12Resource* image,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition.pResource=image;b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        b.Transition.StateBefore=before;b.Transition.StateAfter=after;commands_->ResourceBarrier(1,&b);
    }
    void upload(Counts const& counts){
        std::vector<std::uint8_t> rgba;
        if(!pixels::rgba(counts,rgba))throw std::runtime_error("Invalid ammunition card values");
        // Previous writes are complete before reusing either upload or commands.
        void* mapped{};D3D12_RANGE read{0,0};dx(upload_->Map(0,&read,&mapped),"Map ammo card");
        std::memset(mapped,0,std::size_t(bytes_));
        for(unsigned y=0;y<pixels::height;++y){
            auto* dst=static_cast<unsigned char*>(mapped)+footprint_.Offset+std::size_t(y)*footprint_.Footprint.RowPitch;
            auto const* src=rgba.data()+std::size_t(y)*pixels::width*4;
            for(unsigned x=0;x<pixels::width;++x){const bool bgra=format_==DXGI_FORMAT_B8G8R8A8_UNORM;
                dst[x*4]=src[x*4+(bgra?2:0)];dst[x*4+1]=src[x*4+1];
                dst[x*4+2]=src[x*4+(bgra?0:2)];dst[x*4+3]=src[x*4+3];}
        }
        D3D12_RANGE written{0,std::size_t(bytes_)};upload_->Unmap(0,&written);
        std::uint32_t index{};XrSwapchainImageAcquireInfo acquire{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        exact(xrAcquireSwapchainImage(chain_,&acquire,&index),"Acquire ammo card");acquired_=true;
        XrSwapchainImageWaitInfo wait{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wait.timeout=1000000000;
        exact(xrWaitSwapchainImage(chain_,&wait),"Wait ammo card");waited_=true;
        if(index>=images_.size()||!images_[index].texture||
           !transition_card_layout::valid_image(images_[index].texture->GetDesc(),format_,pixels::width,pixels::height))
            throw std::runtime_error("Ammo-card image layout changed");
        auto* image=images_[index].texture;
        dx(allocator_->Reset(),"Reset ammo-card allocator");dx(commands_->Reset(allocator_.Get(),nullptr),"Reset ammo-card commands");
        barrier(image,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_DEST);
        D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=image;dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        src.pResource=upload_.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src.PlacedFootprint=footprint_;
        commands_->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
        barrier(image,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_RENDER_TARGET);
        dx(commands_->Close(),"Close ammo-card commands");
        ID3D12CommandList* lists[]{commands_.Get()};in_flight_=true;queue_->ExecuteCommandLists(1,lists);
        dx(queue_->Signal(fence_.Get(),++serial_),"Signal ammo-card fence");
        if(!completed()){
            dx(fence_->SetEventOnCompletion(serial_,event_),"Set ammo-card fence event");
            if(WaitForSingleObject(event_,3000)!=WAIT_OBJECT_0||!completed())
                throw std::runtime_error("Ammo-card GPU completion unconfirmed");
        }
        in_flight_=false;XrSwapchainImageReleaseInfo release{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        const auto result=xrReleaseSwapchainImage(chain_,&release);
        if(result!=XR_SUCCESS){release_failed_=true;exact(result,"Release ammo card");}
        acquired_=waited_=false;displayed_=counts;current_=true;++uploads;
    }
public:
    std::uint64_t uploads{},rendered{};
    Renderer(XrSession owner,std::uint64_t bridge_session,ID3D12Device* device,
        ID3D12CommandQueue* queue,bool& teardown_unsafe):
        owner_(owner),bridge_session_(bridge_session),teardown_unsafe_(teardown_unsafe),device_(device),queue_(queue){
        try{
            if(!owner_||!bridge_session_||!device||!queue||queue->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT)
                throw std::runtime_error("Ammo card requires its XR session, bridge session and direct queue");
            std::uint32_t count{};exact(xrEnumerateSwapchainFormats(owner_,0,&count,nullptr),"Count ammo-card formats");
            if(!count||count>256)throw std::runtime_error("Invalid ammo-card format count");
            std::vector<std::int64_t> formats(count);const auto capacity=count;
            exact(xrEnumerateSwapchainFormats(owner_,capacity,&count,formats.data()),"Read ammo-card formats");
            if(count!=capacity)throw std::runtime_error("Ammo-card formats changed");
            for(auto f:{DXGI_FORMAT_B8G8R8A8_UNORM,DXGI_FORMAT_R8G8B8A8_UNORM})
                if(std::find(formats.begin(),formats.end(),std::int64_t(f))!=formats.end()){format_=f;break;}
            if(!format_)throw std::runtime_error("No 8-bit ammo-card format");
            XrSwapchainCreateInfo info{XR_TYPE_SWAPCHAIN_CREATE_INFO};
            info.usageFlags=XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT|XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
            info.format=format_;info.sampleCount=1;info.width=pixels::width;info.height=pixels::height;
            info.faceCount=info.arraySize=info.mipCount=1;
            exact(xrCreateSwapchain(owner_,&info,&chain_),"Create ammo-card swapchain");
            exact(xrEnumerateSwapchainImages(chain_,0,&count,nullptr),"Count ammo-card images");
            if(!count||count>16)throw std::runtime_error("Invalid ammo-card image count");
            images_.resize(count,{XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR});const auto image_capacity=count;
            exact(xrEnumerateSwapchainImages(chain_,image_capacity,&count,
                reinterpret_cast<XrSwapchainImageBaseHeader*>(images_.data())),"Read ammo-card images");
            if(count!=image_capacity||!images_[0].texture)throw std::runtime_error("Ammo-card images changed");
            auto desc=images_[0].texture->GetDesc();desc.Format=format_;UINT rows{};UINT64 row_size{};
            device_->GetCopyableFootprints(&desc,0,1,0,&footprint_,&rows,&row_size,&bytes_);
            if(!transition_card_layout::valid_image(images_[0].texture->GetDesc(),format_,pixels::width,pixels::height)||
               !transition_card_layout::valid_footprint(footprint_,bytes_,format_,pixels::width,pixels::height,rows,row_size))
                throw std::runtime_error("Invalid ammo-card upload layout");
            D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_UPLOAD;
            D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=bytes_;
            buffer.Height=buffer.DepthOrArraySize=buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            dx(device_->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,IID_PPV_ARGS(&upload_)),"Create ammo-card upload");
            dx(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator_)),"Create ammo-card allocator");
            dx(device_->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator_.Get(),nullptr,IID_PPV_ARGS(&commands_)),"Create ammo-card commands");
            dx(commands_->Close(),"Close initial ammo-card commands");
            dx(device_->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence_)),"Create ammo-card fence");
            event_=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event_)throw std::runtime_error("Ammo-card fence event unavailable");
        }catch(...){teardown();throw;}
    }
    Renderer(Renderer const&)=delete;
    Renderer& operator=(Renderer const&)=delete;
    ~Renderer(){teardown();}
    bool render(Options options,AmmoFrame const& ammo,WeaponFrame const& weapon,Mount const& mount,
        gears_vr::StereoSample const& eyes,std::function<std::int64_t()> const& now_ns,
        XrSpace local_space,XrCompositionLayerQuad& out){
        Plan plan{};
        if(failed_||retained_||!local_space||!now_ns||eyes.session!=bridge_session_||
           !prepare(options,ammo,weapon,mount,eyes,now_ns(),plan)||
           (valid(last_frame_)&&(plan.frame.space_epoch<last_frame_.space_epoch||
              plan.frame.sequence<last_frame_.sequence||plan.frame.predicted_time<last_frame_.predicted_time)))return false;
        try{
            auto const c=plan.counts;
            if(!current_||c.kind!=displayed_.kind||c.in_gun!=displayed_.in_gun||
               c.spare!=displayed_.spare||c.infinite_spare!=displayed_.infinite_spare)upload(c);
            // GPU/driver waits can outlive the input freshness budget. Read the
            // same host monotonic clock again instead of reusing pre-wait time.
            if(!prepare(options,ammo,weapon,mount,eyes,now_ns(),plan))return false;
            if(!make_layer(plan,local_space,chain_,out))return false;
            last_frame_=plan.frame;++rendered;return true;
        }catch(...){failed_=true;teardown();throw;}
    }
};
}
