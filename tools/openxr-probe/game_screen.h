#pragma once
#include <d3d11.h>
#include <dwmapi.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <mutex>
#include "startup_capture_size.h"

// Separate-process HWND-only diagnostic. No game Hook or display capture.
class GameWindowFrames {
    using Frame=winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame;
    struct State {std::mutex mutex;Frame latest{nullptr};};
    std::shared_ptr<State> state=std::make_shared<State>();
    winrt::com_ptr<ID3D11Device> device;
    winrt::com_ptr<ID3D11DeviceContext> context;
    winrt::com_ptr<ID3D11Texture2D> staging;
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool pool{nullptr};
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession capture{nullptr};
    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice capture_device{nullptr};
    unsigned pool_width{},pool_height{};
    winrt::event_token token{};
    HWND hwnd{};DWORD pid{};unsigned source_width{},source_height{};
    struct ProcessHandle {HANDLE value{};~ProcessHandle(){if(value)CloseHandle(value);}};
    bool startup=false;ProcessHandle target_process;uint64_t target_creation{};
    bool ready_window() const {
        DWORD actual{};GetWindowThreadProcessId(hwnd,&actual);
        if(!IsWindow(hwnd)||actual!=pid)throw std::runtime_error("Game HWND/PID identity changed");
        if(startup){FILETIME created{},exited{},kernel{},user{};
            if(WaitForSingleObject(target_process.value,0)!=WAIT_TIMEOUT||
               !GetProcessTimes(target_process.value,&created,&exited,&kernel,&user)||
               (uint64_t(created.dwHighDateTime)<<32|created.dwLowDateTime)!=target_creation)
                throw std::runtime_error("Game process identity changed or exited");}
        return !IsIconic(hwnd);
    }
public:
    unsigned width=960,height=540,received=0;
    std::vector<unsigned char> pixels;
    GameWindowFrames(HWND window,DWORD target_pid,bool startup_panel=false):hwnd(window),pid(target_pid),startup(startup_panel) {
        using namespace winrt;
        using namespace winrt::Windows::Graphics::Capture;
        using namespace winrt::Windows::Graphics::DirectX;
        using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
        DWORD actual{};GetWindowThreadProcessId(hwnd,&actual);
        if(!IsWindow(hwnd) || actual!=pid || (!startup&&IsIconic(hwnd)))throw std::runtime_error("Game HWND/PID absent or minimized");
        HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|(startup?SYNCHRONIZE:0),FALSE,pid);
        wchar_t path[32768];DWORD count=32768;
        bool queried=process && QueryFullProcessImageNameW(process,0,path,&count);
        if(startup)target_process.value=process;else if(process)CloseHandle(process);
        const wchar_t* name=queried?wcsrchr(path,L'\\'):nullptr;
        if(!name || _wcsicmp(name+1,L"GOWDE-Steam.exe"))throw std::runtime_error("Capture target is not the game");
        if(startup){
            FILETIME created{},exited{},kernel{},user{};
            if(!target_process.value||!GetProcessTimes(target_process.value,&created,&exited,&kernel,&user)){
                throw std::runtime_error("Capture game identity unavailable");}
            target_creation=uint64_t(created.dwHighDateTime)<<32|created.dwLowDateTime;}
        if(!GraphicsCaptureSession::IsSupported())throw std::runtime_error("WGC unavailable");
        RECT client{};GetClientRect(hwnd,&client);source_width=client.right;source_height=client.bottom;
        if(!source_width || !source_height)throw std::runtime_error("Empty game client");
        width=std::min(startup?1920u:960u,source_width);height=std::max(1u,unsigned(uint64_t(source_height)*width/source_width));
        if(height>(startup?2160u:1280u))throw std::runtime_error("Unexpected game aspect ratio");
        pixels.resize(size_t(width)*height*4);
        check_hresult(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            nullptr,0,D3D11_SDK_VERSION,device.put(),nullptr,context.put()));
        com_ptr<IInspectable> inspectable;check_hresult(CreateDirect3D11DeviceFromDXGIDevice(device.as<IDXGIDevice>().get(),inspectable.put()));
        auto interop=get_activation_factory<GraphicsCaptureItem,IGraphicsCaptureItemInterop>();GraphicsCaptureItem item{nullptr};
        check_hresult(interop->CreateForWindow(hwnd,guid_of<GraphicsCaptureItem>(),put_abi(item)));
        capture_device=inspectable.as<IDirect3DDevice>();auto initial_size=item.Size();
        pool_width=initial_size.Width;pool_height=initial_size.Height;
        pool=Direct3D11CaptureFramePool::CreateFreeThreaded(capture_device,DirectXPixelFormat::B8G8R8A8UIntNormalized,2,initial_size);
        capture=pool.CreateCaptureSession(item);capture.IsCursorCaptureEnabled(false);
        token=pool.FrameArrived([shared=state](auto const& sender,auto const&) {
            std::lock_guard<std::mutex> lock(shared->mutex);
            try {auto frame=sender.TryGetNextFrame();if(frame){if(shared->latest)shared->latest.Close();shared->latest=frame;}}
            catch(winrt::hresult_error const&){}
        });
        capture.StartCapture();
    }
    ~GameWindowFrames(){
        if(capture)capture.Close();if(pool){pool.FrameArrived(token);pool.Close();}
        std::lock_guard<std::mutex> lock(state->mutex);if(state->latest)state->latest.Close();
    }
    bool available() const {return ready_window();}
    bool update() {
        try{return update_frame();}catch(winrt::hresult_error const&){if(!startup)throw;return false;}
    }
    bool update_frame() {
        using namespace winrt;
        if(!ready_window()){if(startup)return false;throw std::runtime_error("Game window minimized");}
        Frame frame{nullptr};{std::lock_guard<std::mutex> lock(state->mutex);frame=state->latest;state->latest=nullptr;}
        if(!frame)return false;
        auto access=frame.Surface().as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
        com_ptr<ID3D11Texture2D> texture;check_hresult(access->GetInterface(IID_PPV_ARGS(texture.put())));
        D3D11_TEXTURE2D_DESC desc{};texture->GetDesc(&desc);auto content=frame.ContentSize();
        if(startup&&(content.Width<=0||content.Height<=0)){frame.Close();return false;}
        if(startup&&(unsigned(content.Width)!=pool_width||unsigned(content.Height)!=pool_height)){
            // Close the outstanding frame before Recreate. No callback mutex is
            // held while the pool replaces its buffers on the capture thread.
            frame.Close();pool.Recreate(capture_device,
                winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,2,content);
            pool_width=content.Width;pool_height=content.Height;return false;
        }
        RECT client{},bounds{};POINT origin{};
        GetClientRect(hwnd,&client);ClientToScreen(hwnd,&origin);
        auto bounds_result=DwmGetWindowAttribute(hwnd,DWMWA_EXTENDED_FRAME_BOUNDS,&bounds,sizeof(bounds));
        if(startup&&FAILED(bounds_result)){frame.Close();return false;}check_hresult(bounds_result);
        LONG left=std::max(0L,origin.x-bounds.left),top=std::max(0L,origin.y-bounds.top);
        if(startup&&(client.right<=0||client.bottom<=0)){frame.Close();return false;}
        if(startup&&(client.right!=LONG(source_width)||client.bottom!=LONG(source_height))){
            unsigned fitted_width{},fitted_height{};
            if(!startup_capture_size::fit(client.right,client.bottom,fitted_width,fitted_height)){
                frame.Close();throw std::runtime_error("Unexpected game aspect ratio after resize");}
            source_width=client.right;source_height=client.bottom;width=fitted_width;height=fitted_height;
            pixels.resize(size_t(width)*height*4);staging=nullptr;
        }
        if(client.right!=LONG(source_width) || client.bottom!=LONG(source_height) ||
           left+client.right>content.Width || top+client.bottom>content.Height ||
           left+client.right>LONG(desc.Width) || top+client.bottom>LONG(desc.Height))
            {if(startup){frame.Close();return false;}throw std::runtime_error("Game resized or client crop invalid; restart display test");}
        if(!staging) {
            desc.Width=source_width;desc.Height=source_height;desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;
            desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;desc.MiscFlags=0;
            check_hresult(device->CreateTexture2D(&desc,nullptr,staging.put()));
        }
        D3D11_BOX box{UINT(left),UINT(top),0,UINT(left+client.right),UINT(top+client.bottom),1};
        context->CopySubresourceRegion(staging.get(),0,0,0,0,texture.get(),0,&box);
        D3D11_MAPPED_SUBRESOURCE mapped{};check_hresult(context->Map(staging.get(),0,D3D11_MAP_READ,0,&mapped));
        // Cheap diagnostic downsample; this is not the final GPU-only path.
        for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x) {
            const auto* input=static_cast<const unsigned char*>(mapped.pData)+size_t(uint64_t(y)*source_height/height)*mapped.RowPitch+size_t(uint64_t(x)*source_width/width)*4;
            std::memcpy(pixels.data()+(size_t(y)*width+x)*4,input,4);
        }
        context->Unmap(staging.get(),0);frame.Close();++received;return true;
    }
};

class GameScreen {
    GameWindowFrames source;
    XrSwapchain chain{};
    std::vector<XrSwapchainImageD3D12KHR> images;
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> commands;
    ComPtr<ID3D12Resource> upload;
    ComPtr<ID3D12Fence> fence;
    HANDLE event{};uint64_t serial=0;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
    DXGI_FORMAT format{};
    std::string prefix;
    std::chrono::steady_clock::time_point next_capture{},last_frame{};
    bool current=false,startup=false;
    unsigned chain_width{},chain_height{};
    void create_display_chain() {
        XrSwapchain next{};
        XrSwapchainCreateInfo create{XR_TYPE_SWAPCHAIN_CREATE_INFO};create.usageFlags=XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT|XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
        create.format=format;create.width=source.width;create.height=source.height;create.sampleCount=1;create.faceCount=1;create.arraySize=1;create.mipCount=1;
        xr_check(xrCreateSwapchain(session,&create,&next),"create game display swapchain");
        try {
            uint32_t count{};xr_check(xrEnumerateSwapchainImages(next,0,&count,nullptr),"game images");
            if(!count)throw std::runtime_error("Empty game display swapchain");
            std::vector<XrSwapchainImageD3D12KHR> next_images(count,{XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR});
            xr_check(xrEnumerateSwapchainImages(next,count,&count,reinterpret_cast<XrSwapchainImageBaseHeader*>(next_images.data())),"read game images");
            auto desc=next_images[0].texture->GetDesc();UINT64 bytes{};D3D12_PLACED_SUBRESOURCE_FOOTPRINT next_footprint{};
            device->GetCopyableFootprints(&desc,0,1,0,&next_footprint,nullptr,nullptr,&bytes);
            D3D12_HEAP_PROPERTIES props{};props.Type=D3D12_HEAP_TYPE_UPLOAD;
            D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=bytes;buffer.Height=1;
            buffer.DepthOrArraySize=1;buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            ComPtr<ID3D12Resource> next_upload;
            dx_check(device->CreateCommittedResource(&props,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&next_upload)),"game upload");
            // Each preceding upload waited its GPU fence before image release.
            if(chain)xr_check(xrDestroySwapchain(chain),"destroy resized game display swapchain");
            chain=next;next=XR_NULL_HANDLE;images=std::move(next_images);upload=std::move(next_upload);footprint=next_footprint;
            chain_width=source.width;chain_height=source.height;current=false;
        }catch(...){if(next)xrDestroySwapchain(next);throw;}
    }
    void barrier(ID3D12Resource* image,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after) {
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition.pResource=image;b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        b.Transition.StateBefore=before;b.Transition.StateAfter=after;commands->ResourceBarrier(1,&b);
    }
public:
    unsigned rendered=0,uploads=0,changes=0;
    uint64_t first_hash=0,last_hash=0;
    bool gpu_pixels_verified=false;
    GameScreen(ID3D12Device* d,ID3D12CommandQueue* q,HWND hwnd,DWORD pid,std::string output,bool startup_panel=false):source(hwnd,pid,startup_panel),device(d),queue(q),prefix(std::move(output)),startup(startup_panel) {
        uint32_t count{};xr_check(xrEnumerateSwapchainFormats(session,0,&count,nullptr),"game formats");
        std::vector<int64_t> formats(count);xr_check(xrEnumerateSwapchainFormats(session,count,&count,formats.data()),"read game formats");
        for(auto candidate:{DXGI_FORMAT_B8G8R8A8_UNORM,DXGI_FORMAT_R8G8B8A8_UNORM})
            if(std::find(formats.begin(),formats.end(),int64_t(candidate))!=formats.end()){format=candidate;break;}
        if(!format)throw std::runtime_error("No game display swapchain format");
        create_display_chain();
        dx_check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"game allocator");
        dx_check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&commands)),"game commands");dx_check(commands->Close(),"game initial close");
        dx_check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"game fence");event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
        if(!event)throw std::runtime_error("Game fence event unavailable");
    }
    ~GameScreen(){if(chain)xrDestroySwapchain(chain);if(event)CloseHandle(event);}
    bool render(XrCompositionLayerQuad& quad) {
        if(startup&&!source.available())return false;
        auto now=std::chrono::steady_clock::now();
        if(now>=next_capture) {
            next_capture=now+std::chrono::milliseconds(startup?33:67);
            const bool updated=source.update();
            if(startup&&(chain_width!=source.width||chain_height!=source.height)){
                // A resized client can precede its matching WGC frame. Do not
                // submit the old swapchain with the new client image extent.
                current=false;if(updated)create_display_chain();
            }
            if(updated) {
                uint64_t hash=14695981039346656037ULL;for(auto v:source.pixels)hash=(hash^v)*1099511628211ULL;
                if(uploads && hash!=last_hash)++changes;last_hash=hash;if(!uploads)first_hash=hash;
                void* mapped{};D3D12_RANGE read{0,0};dx_check(upload->Map(0,&read,&mapped),"map game upload");
                for(unsigned y=0;y<source.height;++y) {
                    auto* dst=static_cast<unsigned char*>(mapped)+footprint.Offset+size_t(y)*footprint.Footprint.RowPitch;
                    const auto* src=source.pixels.data()+size_t(y)*source.width*4;
                    for(unsigned x=0;x<source.width;++x){dst[x*4]=src[x*4+(format==DXGI_FORMAT_R8G8B8A8_UNORM?2:0)];dst[x*4+1]=src[x*4+1];dst[x*4+2]=src[x*4+(format==DXGI_FORMAT_R8G8B8A8_UNORM?0:2)];dst[x*4+3]=255;}
                }
                upload->Unmap(0,nullptr);
                uint32_t index{};XrSwapchainImageAcquireInfo acquire{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};xr_check(xrAcquireSwapchainImage(chain,&acquire,&index),"acquire game image");
                XrSwapchainImageWaitInfo wait{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wait.timeout=1000000000;xr_check(xrWaitSwapchainImage(chain,&wait),"wait game image");
                auto* image=images.at(index).texture;
                dx_check(allocator->Reset(),"game allocator reset");dx_check(commands->Reset(allocator.Get(),nullptr),"game commands reset");
                barrier(image,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_DEST);
                D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=image;dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                src.pResource=upload.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src.PlacedFootprint=footprint;
                commands->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
                ComPtr<ID3D12Resource> readback;
                if(!uploads&&!startup) {
                    D3D12_HEAP_PROPERTIES props{};props.Type=D3D12_HEAP_TYPE_READBACK;auto desc=upload->GetDesc();
                    dx_check(device->CreateCommittedResource(&props,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback)),"game readback");
                    barrier(image,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_COPY_SOURCE);
                    D3D12_TEXTURE_COPY_LOCATION rb{},from{};rb.pResource=readback.Get();rb.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;rb.PlacedFootprint=footprint;
                    from.pResource=image;from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                    commands->CopyTextureRegion(&rb,0,0,0,&from,nullptr);
                    barrier(image,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
                } else barrier(image,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_RENDER_TARGET);
                dx_check(commands->Close(),"close game commands");ID3D12CommandList* lists[]={commands.Get()};queue->ExecuteCommandLists(1,lists);
                dx_check(queue->Signal(fence.Get(),++serial),"signal game fence");
                if(fence->GetCompletedValue()<serial){dx_check(fence->SetEventOnCompletion(serial,event),"game fence event");if(WaitForSingleObject(event,3000)!=WAIT_OBJECT_0)throw std::runtime_error("Game GPU fence timeout");}
                if(readback) {
                    void* data{};D3D12_RANGE range{0,size_t(readback->GetDesc().Width)};dx_check(readback->Map(0,&range,&data),"map game readback");
                    FILE* file{};fopen_s(&file,(prefix+"-game-submitted.ppm").c_str(),"wb");
                    if(!file)throw std::runtime_error("Save submitted game frame failed");fprintf(file,"P6\n%u %u\n255\n",source.width,source.height);
                    gpu_pixels_verified=true;
                    for(unsigned y=0;y<source.height;++y)for(unsigned x=0;x<source.width;++x) {
                        const auto* pixel=static_cast<const unsigned char*>(data)+footprint.Offset+size_t(y)*footprint.Footprint.RowPitch+x*4;
                        unsigned char rgb[3]={pixel[format==DXGI_FORMAT_B8G8R8A8_UNORM?2:0],pixel[1],pixel[format==DXGI_FORMAT_B8G8R8A8_UNORM?0:2]};
                        const auto* original=source.pixels.data()+(size_t(y)*source.width+x)*4;
                        if(rgb[0]!=original[2] || rgb[1]!=original[1] || rgb[2]!=original[0] || pixel[3]!=255)gpu_pixels_verified=false;
                        fwrite(rgb,1,3,file);
                    }
                    fclose(file);D3D12_RANGE written{0,0};readback->Unmap(0,&written);
                    if(!gpu_pixels_verified)throw std::runtime_error("Game GPU readback pixels differ from source");
                }
                XrSwapchainImageReleaseInfo release{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};xr_check(xrReleaseSwapchainImage(chain,&release),"release game image");
                ++uploads;current=true;last_frame=now;
                if(uploads==1&&!startup) {
                    FILE* file{};fopen_s(&file,(prefix+"-game-source.ppm").c_str(),"wb");if(!file)throw std::runtime_error("Save game source failed");
                    fprintf(file,"P6\n%u %u\n255\n",source.width,source.height);
                    for(size_t i=0;i<source.pixels.size();i+=4){unsigned char rgb[3]={source.pixels[i+2],source.pixels[i+1],source.pixels[i]};fwrite(rgb,1,3,file);}fclose(file);
                }
            }
        }
        if(!current || now-last_frame>std::chrono::seconds(1))return false;
        quad={XR_TYPE_COMPOSITION_LAYER_QUAD};quad.space=view_space;quad.eyeVisibility=XR_EYE_VISIBILITY_BOTH;
        quad.pose.orientation.w=1;quad.pose.position.z=-2;quad.size={1.6f,1.6f*source.height/source.width};
        quad.subImage.swapchain=chain;quad.subImage.imageRect.extent={int32_t(source.width),int32_t(source.height)};
        ++rendered;return true;
    }
};
