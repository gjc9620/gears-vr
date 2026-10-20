#pragma once
#include <vector>
#include <memory>
#include <algorithm>
#include <cstdint>

// Deliberately tiny diagnostic eye images. This is not stereo scene rendering.
class TestPattern {
    struct Eye {XrSwapchain chain{};std::vector<XrSwapchainImageD3D12KHR> images;};
    Eye eyes[2];
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> commands;
    ComPtr<ID3D12DescriptorHeap> heap;
    ComPtr<ID3D12Fence> fence;
    HANDLE event{};
    uint64_t serial=0;
    DXGI_FORMAT format{};
    const unsigned width=512,height=512;
    bool saved[2]{};
    std::string prefix;
    void finish() {
        dx_check(commands->Close(),"close commands");
        ID3D12CommandList* lists[]={commands.Get()};queue->ExecuteCommandLists(1,lists);
        dx_check(queue->Signal(fence.Get(),++serial),"signal fence");
        if(fence->GetCompletedValue()<serial) {
            dx_check(fence->SetEventOnCompletion(serial,event),"fence event");
            if(WaitForSingleObject(event,3000)!=WAIT_OBJECT_0)
                throw std::runtime_error("Diagnostic GPU fence timeout");
        }
    }
    void barrier(ID3D12Resource* image,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after) {
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition.pResource=image;b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        b.Transition.StateBefore=before;b.Transition.StateAfter=after;commands->ResourceBarrier(1,&b);
    }
public:
    unsigned rendered=0;
    uint64_t checksums[2]{};
    TestPattern(ID3D12Device* d,ID3D12CommandQueue* q,const std::string& output_prefix):device(d),queue(q),prefix(output_prefix) {
        uint32_t count{};xr_check(xrEnumerateSwapchainFormats(session,0,&count,nullptr),"enumerate formats");
        std::vector<int64_t> formats(count);xr_check(xrEnumerateSwapchainFormats(session,count,&count,formats.data()),"read formats");
        for(auto candidate:{DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_B8G8R8A8_UNORM,DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,DXGI_FORMAT_B8G8R8A8_UNORM_SRGB})
            if(std::find(formats.begin(),formats.end(),int64_t(candidate))!=formats.end()) {format=candidate;break;}
        if(!format)throw std::runtime_error("No 8-bit diagnostic swapchain format");
        for(auto& eye:eyes) {
            XrSwapchainCreateInfo create{XR_TYPE_SWAPCHAIN_CREATE_INFO};
            create.usageFlags=XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT|XR_SWAPCHAIN_USAGE_TRANSFER_SRC_BIT;
            create.format=format;create.sampleCount=1;create.width=width;create.height=height;
            create.faceCount=1;create.arraySize=1;create.mipCount=1;
            xr_check(xrCreateSwapchain(session,&create,&eye.chain),"create diagnostic swapchain");
            xr_check(xrEnumerateSwapchainImages(eye.chain,0,&count,nullptr),"enumerate images");
            eye.images.resize(count,{XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR});
            xr_check(xrEnumerateSwapchainImages(eye.chain,count,&count,reinterpret_cast<XrSwapchainImageBaseHeader*>(eye.images.data())),"read images");
        }
        dx_check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"allocator");
        dx_check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&commands)),"commands");
        dx_check(commands->Close(),"initial close");
        D3D12_DESCRIPTOR_HEAP_DESC desc{};desc.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;desc.NumDescriptors=1;
        dx_check(device->CreateDescriptorHeap(&desc,IID_PPV_ARGS(&heap)),"RTV heap");
        dx_check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"fence");
        event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
        if(!event)throw std::runtime_error("Fence event failed");
    }
    ~TestPattern() {for(auto& eye:eyes)if(eye.chain)xrDestroySwapchain(eye.chain);if(event)CloseHandle(event);}
    void render(const XrView views[2],XrCompositionLayerProjectionView output[2]) {
        for(unsigned e=0;e<2;++e) {
            auto& eye=eyes[e];uint32_t index{};
            XrSwapchainImageAcquireInfo acquire{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
            xr_check(xrAcquireSwapchainImage(eye.chain,&acquire,&index),"acquire image");
            XrSwapchainImageWaitInfo wait{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wait.timeout=1000000000;
            xr_check(xrWaitSwapchainImage(eye.chain,&wait),"wait image");
            ID3D12Resource* image=eye.images.at(index).texture;
            dx_check(allocator->Reset(),"reset allocator");dx_check(commands->Reset(allocator.Get(),nullptr),"reset commands");
            auto handle=heap->GetCPUDescriptorHandleForHeapStart();
            D3D12_RENDER_TARGET_VIEW_DESC rtv{};rtv.Format=format;rtv.ViewDimension=D3D12_RTV_DIMENSION_TEXTURE2D;
            device->CreateRenderTargetView(image,&rtv,handle);
            // OpenXR guarantees RENDER_TARGET after wait; preserve it at release.
            const float background[2][4]={{0.25f,0.02f,0.02f,1},{0.02f,0.02f,0.25f,1}};
            commands->ClearRenderTargetView(handle,background[e],0,nullptr);
            const float white[4]={0.8f,0.8f,0.8f,1};
            D3D12_RECT vertical{248,100,264,412},horizontal{100,248,412,264};
            commands->ClearRenderTargetView(handle,white,1,&vertical);commands->ClearRenderTargetView(handle,white,1,&horizontal);
            ComPtr<ID3D12Resource> readback;D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout{};UINT64 size{};
            if(!saved[e]) {
                auto desc=image->GetDesc();device->GetCopyableFootprints(&desc,0,1,0,&layout,nullptr,nullptr,&size);
                D3D12_HEAP_PROPERTIES properties{};properties.Type=D3D12_HEAP_TYPE_READBACK;
                D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=size;
                buffer.Height=1;buffer.DepthOrArraySize=1;buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
                dx_check(device->CreateCommittedResource(&properties,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback)),"readback buffer");
                barrier(image,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE);
                D3D12_TEXTURE_COPY_LOCATION destination{};destination.pResource=readback.Get();destination.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;destination.PlacedFootprint=layout;
                D3D12_TEXTURE_COPY_LOCATION source{};source.pResource=image;source.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                commands->CopyTextureRegion(&destination,0,0,0,&source,nullptr);
                barrier(image,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
            }
            finish();
            if(readback) {
                void* mapped{};D3D12_RANGE range{0,size_t(size)};dx_check(readback->Map(0,&range,&mapped),"map readback");
                const auto* pixels=static_cast<const unsigned char*>(mapped)+layout.Offset;
                std::string name=prefix+(e?"-right.ppm":"-left.ppm");FILE* file{};fopen_s(&file,name.c_str(),"wb");
                if(!file)throw std::runtime_error("Cannot save diagnostic image");
                fprintf(file,"P6\n%u %u\n255\n",width,height);uint64_t hash=14695981039346656037ULL;
                for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x) {
                    const auto* p=pixels+size_t(y)*layout.Footprint.RowPitch+x*4;
                    bool bgra=format==DXGI_FORMAT_B8G8R8A8_UNORM || format==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
                    unsigned char rgb[3]={p[bgra?2:0],p[1],p[bgra?0:2]};
                    fwrite(rgb,1,3,file);for(auto value:rgb)hash=(hash^value)*1099511628211ULL;
                }
                fclose(file);D3D12_RANGE written{0,0};readback->Unmap(0,&written);checksums[e]=hash;saved[e]=true;
            }
            XrSwapchainImageReleaseInfo release{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};xr_check(xrReleaseSwapchainImage(eye.chain,&release),"release image");
            output[e]={XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};output[e].pose=views[e].pose;output[e].fov=views[e].fov;
            output[e].subImage.swapchain=eye.chain;output[e].subImage.imageRect.extent={int32_t(width),int32_t(height)};
        }
        ++rendered;
    }
};
