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
#include <limits>
#include "startup_capture_size.h"
#include "menu_capture_epoch.h"
#include "capture_frame_mailbox.h"
#include "pending_capture_frame.h"
#include "shutdown_diagnostics.h"
#include "outage_diagnostics.h"
#include "transition_card_layout.h"
#include "menu_pixels.h"

inline std::int64_t menu_capture_qpc_ticks(){
    LARGE_INTEGER counter{},frequency{};
    if(!QueryPerformanceCounter(&counter)||!QueryPerformanceFrequency(&frequency)||counter.QuadPart<0||frequency.QuadPart<=0)
        throw std::runtime_error("Menu capture QPC unavailable");
    const long double ticks=static_cast<long double>(counter.QuadPart)*10000000.0L/frequency.QuadPart;
    if(ticks>static_cast<long double>((std::numeric_limits<std::int64_t>::max)()))throw std::runtime_error("Menu capture QPC overflow");
    return static_cast<std::int64_t>(ticks);
}

// Separate-process HWND-only diagnostic. No game Hook or display capture.
class GameWindowFrames {
    using Frame=winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame;
    using State=CaptureFrameMailbox<Frame>;
    std::shared_ptr<State> state=std::make_shared<State>();
    PendingCaptureFrame<Frame> pending_frame;
    winrt::com_ptr<ID3D11Device> device;
    winrt::com_ptr<ID3D11DeviceContext> context;
    winrt::com_ptr<ID3D11Texture2D> staging;
    menu_pixels::Sampler pixel_sampler;
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool pool{nullptr};
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession capture{nullptr};
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item{nullptr};
    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice capture_device{nullptr};
    unsigned pool_width{},pool_height{};
    winrt::event_token token{},closed_token{};
    HWND hwnd{};DWORD pid{};unsigned source_width{},source_height{};
    struct ProcessHandle {HANDLE value{};~ProcessHandle(){if(value)CloseHandle(value);}};
    bool startup=false;ProcessHandle target_process;uint64_t target_creation{};
    bool recover_window=false;
    bool capture_suspended=false,resume_pending=false;
    HWND resume_window{};
    uint64_t generation_=0;
    std::int64_t next_window_probe=0;
    MenuCaptureEpoch menu_epoch;
    outage_diagnostics::MenuCaptureSnapshot diagnostic_;
    bool reject_capture(outage_diagnostics::CaptureResult reason){diagnostic_.result=reason;return false;}
    outage_diagnostics::CaptureResult timestamp_rejection(std::int64_t captured,std::int64_t now)const{
        using R=outage_diagnostics::CaptureResult;
        if(menu_epoch.started()<=0||captured<=menu_epoch.started())return R::pre_epoch;
        if(now<captured)return R::future;return R::expired;
    }
    void require_process()const{
        if(startup){FILETIME created{},exited{},kernel{},user{};
            if(WaitForSingleObject(target_process.value,0)!=WAIT_TIMEOUT||
               !GetProcessTimes(target_process.value,&created,&exited,&kernel,&user)||
               (uint64_t(created.dwHighDateTime)<<32|created.dwLowDateTime)!=target_creation)
                throw std::runtime_error("Game process identity changed or exited");}
    }
    bool matches(HWND window)const{
        DWORD actual{};GetWindowThreadProcessId(window,&actual);
        return IsWindow(window)&&actual==pid;
    }
    bool usable(HWND window)const{
        RECT client{};
        return matches(window)&&IsWindowVisible(window)&&!IsIconic(window)&&
            GetClientRect(window,&client)&&client.right>=640&&client.bottom>=360;
    }
    void request_borderless_access(){
        using namespace winrt::Windows::Graphics::Capture;
        // Request once before starting capture, never on the XR recovery path.
        // Windows retains authority over consent and policy; capture still works
        // with its indicator if borderless access is unavailable or denied.
        try{
            auto access=winrt::try_get_activation_factory<GraphicsCaptureAccess,IGraphicsCaptureAccessStatics>();
            if(!access){
                printf("{\"wgc_borderless_access\":\"unsupported\"}\n");
            }else{
                const auto status=access.RequestAccessAsync(GraphicsCaptureAccessKind::Borderless).get();
                printf("{\"wgc_borderless_access_status\":%d}\n",int(status));
            }
        }catch(winrt::hresult_error const& error){
            printf("{\"wgc_borderless_access\":\"failed\",\"hresult\":%ld}\n",long(error.code()));
        }
        fflush(stdout);
    }
    void configure_capture_border(){
        using namespace winrt::Windows::Graphics::Capture;
        try{
            auto border=capture.try_as<IGraphicsCaptureSession3>();
            if(!border){
                printf("{\"wgc_borderless_request\":\"unsupported\"}\n");
            }else{
                border.IsBorderRequired(false);
                // This reports the session preference, not proof that Windows
                // hides the indicator (policy or another capturer can retain it).
                printf("{\"wgc_borderless_request\":\"accepted\",\"border_required\":%s,\"visual_verified\":false}\n",
                    border.IsBorderRequired()?"true":"false");
            }
        }catch(winrt::hresult_error const& error){
            printf("{\"wgc_borderless_request\":\"failed\",\"hresult\":%ld}\n",long(error.code()));
        }
        fflush(stdout);
    }
    bool retire_capture()noexcept{
        // Never hold the callback mutex while Close waits for WGC callbacks.
        pending_frame.discard();
        state->retire();
        try{if(item)item.Closed(closed_token);}catch(winrt::hresult_error const&){}
        try{if(pool)pool.FrameArrived(token);}catch(winrt::hresult_error const&){}
        bool closed=true;
        try{if(capture)capture.Close();}catch(winrt::hresult_error const&){closed=false;}
        try{if(pool)pool.Close();}catch(winrt::hresult_error const&){closed=false;}
        const bool bound=hwnd||capture||pool;
        capture=nullptr;pool=nullptr;item=nullptr;hwnd=nullptr;staging=nullptr;
        if(bound)++generation_;
        return closed;
    }
    bool open_capture(HWND window){
        using namespace winrt;
        using namespace winrt::Windows::Graphics::Capture;
        using namespace winrt::Windows::Graphics::DirectX;
        require_process();
        RECT client{};
        if(!matches(window)||!GetClientRect(window,&client)||client.right<=0||client.bottom<=0)return false;
        if(recover_window&&!usable(window))return false;
        source_width=client.right;source_height=client.bottom;
        width=std::min(startup?1920u:960u,source_width);height=std::max(1u,unsigned(uint64_t(source_height)*width/source_width));
        if(height>(startup?2160u:1280u))throw std::runtime_error("Unexpected game aspect ratio");
        pixels.resize(size_t(width)*height*4);
        hwnd=window;state=std::make_shared<State>();
        if(startup)menu_epoch.begin(menu_capture_qpc_ticks());
        try{
            auto interop=get_activation_factory<GraphicsCaptureItem,IGraphicsCaptureItemInterop>();
            shutdown_diagnostics::initialization("wgc_create_for_window_enter");
            check_hresult(interop->CreateForWindow(hwnd,guid_of<GraphicsCaptureItem>(),put_abi(item)));
            shutdown_diagnostics::initialization("wgc_create_for_window_completed");
            closed_token=item.Closed([shared=state](auto const&,auto const&){shared->mark_closed();});
            const auto initial_size=item.Size();
            if(initial_size.Width<=0||initial_size.Height<=0){retire_capture();return false;}
            pool_width=initial_size.Width;pool_height=initial_size.Height;
            pool=Direct3D11CaptureFramePool::CreateFreeThreaded(capture_device,DirectXPixelFormat::B8G8R8A8UIntNormalized,2,initial_size);
            capture=pool.CreateCaptureSession(item);capture.IsCursorCaptureEnabled(false);
            configure_capture_border();
            token=pool.FrameArrived([shared=state](auto const& sender,auto const&) {
                try{auto frame=sender.TryGetNextFrame();if(frame)shared->publish(std::move(frame));}
                catch(winrt::hresult_error const&){}
            });
            require_process();
            if(!matches(hwnd)||(recover_window&&!usable(hwnd))){retire_capture();return false;}
            shutdown_diagnostics::initialization("wgc_start_capture_enter");
            capture.StartCapture();
            shutdown_diagnostics::initialization("wgc_start_capture_completed");
            ++generation_;
            if(resume_pending){
                printf("{\"wgc_capture_resumed\":true,\"pid\":%lu,\"hwnd\":%llu,\"generation\":%llu,\"capture_epoch_100ns\":%lld,\"fresh_frame_required\":true}\n",
                    pid,uint64_t(window_identity()),generation_,menu_epoch.started());fflush(stdout);resume_pending=false;
            }
            return true;
        }catch(winrt::hresult_error const& error){
            const bool transient=recover_window&&(!usable(window)||error.code()==RO_E_CLOSED||
                error.code()==HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE));
            retire_capture();require_process();if(transient)return false;throw;
        }catch(...){retire_capture();throw;}
    }
    HWND replacement_window()const{
        struct Candidates{DWORD pid;HWND window{};unsigned count{};} candidates{pid};
        const auto visit=[](HWND window,LPARAM param)->BOOL{
            auto& found=*reinterpret_cast<Candidates*>(param);DWORD actual{};RECT client{};
            GetWindowThreadProcessId(window,&actual);
            if(actual==found.pid&&IsWindowVisible(window)&&!IsIconic(window)&&GetClientRect(window,&client)&&
                client.right>=640&&client.bottom>=360){found.window=window;++found.count;}
            return TRUE;
        };
        if(!EnumWindows(visit,reinterpret_cast<LPARAM>(&candidates)))throw std::runtime_error("Enumerate game windows failed");
        return candidates.count==1?candidates.window:nullptr;
    }
    bool ready_window(){
        require_process();
        if(capture_suspended)return false;
        if(!recover_window){
            if(!matches(hwnd))throw std::runtime_error("Game HWND/PID identity changed");
            return !IsIconic(hwnd);
        }
        const bool current=capture&&usable(hwnd)&&!state->closed();
        const auto now=menu_capture_qpc_ticks();
        if(!current&&(hwnd||capture||pool)){retire_capture();menu_epoch.begin(now);}
        if(now<next_window_probe)return current;
        next_window_probe=now+2500000; // 250 ms; never wait on the XR frame thread.
        const auto candidate=replacement_window();
        if(current&&candidate==hwnd)return true;
        if(hwnd||capture||pool){retire_capture();menu_epoch.begin(now);}
        return candidate&&open_capture(candidate);
    }
public:
    unsigned width=960,height=540,received=0;
    std::vector<unsigned char> pixels;
    GameWindowFrames(HWND window,DWORD target_pid,bool startup_panel=false,uint64_t expected_creation=0):pid(target_pid),startup(startup_panel),recover_window(startup_panel&&expected_creation!=0) {
        using namespace winrt;
        using namespace winrt::Windows::Graphics::Capture;
        using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
        if(!recover_window&&(!matches(window)||(!startup&&IsIconic(window))))throw std::runtime_error("Game HWND/PID absent or minimized");
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
            target_creation=uint64_t(created.dwHighDateTime)<<32|created.dwLowDateTime;
            if(expected_creation&&target_creation!=expected_creation)throw std::runtime_error("Capture game creation identity mismatch");}
        if(!GraphicsCaptureSession::IsSupported())throw std::runtime_error("WGC unavailable");
        request_borderless_access();
        check_hresult(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            nullptr,0,D3D11_SDK_VERSION,device.put(),nullptr,context.put()));
        com_ptr<IInspectable> inspectable;check_hresult(CreateDirect3D11DeviceFromDXGIDevice(device.as<IDXGIDevice>().get(),inspectable.put()));
        capture_device=inspectable.as<IDirect3DDevice>();
        if(recover_window)ready_window();
        else if(!open_capture(window))throw std::runtime_error("Game capture window unavailable");
    }
    ~GameWindowFrames(){retire_capture();}
    bool available(){return ready_window();}
    void suspend_capture(){
        if(!startup||capture_suspended)return;
        resume_window=hwnd;capture_suspended=true;resume_pending=true;
        const bool had_binding=hwnd||capture||pool;
        const bool closed=retire_capture();menu_epoch.begin(menu_capture_qpc_ticks());
        printf("{\"wgc_capture_suspended\":true,\"pid\":%lu,\"had_binding\":%s,\"close_succeeded\":%s,\"generation\":%llu}\n",
            pid,had_binding?"true":"false",closed?"true":"false",generation_);fflush(stdout);
    }
    void resume_capture(){
        if(!capture_suspended)return;
        require_process();capture_suspended=false;next_window_probe=0;
        // Recoverable bindings reacquire the unique usable window in available().
        // Legacy startup bindings keep their original strict HWND identity.
        if(!recover_window&&!open_capture(resume_window))throw std::runtime_error("Game capture window unavailable on resume");
    }
    uint64_t generation()const{return generation_;}
    void begin_menu_epoch(){
        if(!startup)return;
        menu_epoch.begin(menu_capture_qpc_ticks());
        pending_frame.discard();state->discard();
        // The pool owns two buffers. The timestamp gate also rejects any old
        // buffer whose callback arrives after this bounded drain completes.
        if(pool)try{for(unsigned i=0;i<2;++i){auto pending=pool.TryGetNextFrame();if(!pending)break;pending.Close();}}
        catch(winrt::hresult_error const&){if(!recover_window)throw;}
    }
    std::int64_t menu_started()const{return menu_epoch.started();}
    std::int64_t menu_captured()const{return menu_epoch.captured();}
    std::uintptr_t window_identity()const{return reinterpret_cast<std::uintptr_t>(hwnd);}
    bool menu_fresh(std::int64_t* observed_now=nullptr,std::int64_t max_age=MenuCaptureEpoch::maximum_age)const{
        if(!startup)return true;
        const auto now=menu_capture_qpc_ticks();if(observed_now)*observed_now=now;
        return menu_epoch.fresh(now,max_age);
    }
    outage_diagnostics::MenuCaptureSnapshot diagnostic_snapshot()const{
        auto result=diagnostic_;result.generation=generation_;result.hwnd=uint64_t(window_identity());
        result.epoch_100ns=menu_epoch.started();result.accepted_100ns=menu_epoch.captured();
        result.pending_reused=pending_frame.reused;result.pending_dropped=pending_frame.dropped;return result;
    }
    bool update() {
        ++diagnostic_.attempts;diagnostic_.result=outage_diagnostics::CaptureResult::capture_error;
        try{return update_frame();}catch(winrt::hresult_error const&){if(!startup)throw;return reject_capture(outage_diagnostics::CaptureResult::capture_error);}
    }
    bool update_frame() {
        using namespace winrt;
        using R=outage_diagnostics::CaptureResult;
        if(!ready_window()){if(startup)return reject_capture(R::window_unavailable);throw std::runtime_error("Game window minimized");}
        Frame frame{nullptr};
        if(startup&&pending_frame.has_frame()){
            frame=pending_frame.take(menu_capture_qpc_ticks(),menu_epoch.started(),generation_);
            // Preserve this imminent frame instead of replacing it with the
            // latest future frame. Never wait/sleep on the XR thread.
            if(!frame&&pending_frame.has_frame())return reject_capture(R::future);
        }
        if(!frame)frame=state->take();
        if(!frame){++diagnostic_.no_frames;return reject_capture(R::no_frame);}
        const std::int64_t captured=startup?frame.SystemRelativeTime().count():0;
        diagnostic_.last_taken_100ns=captured;
        if(startup){const auto now=menu_capture_qpc_ticks();
            if(!menu_epoch.eligible(captured,now)){
                const auto reason=timestamp_rejection(captured,now);
                if(reason==R::future){
                    diagnostic_.future_ahead_last_100ns=captured-now;
                    diagnostic_.future_ahead_max_100ns=std::max(diagnostic_.future_ahead_max_100ns,captured-now);
                    pending_frame.defer(std::move(frame),captured,now,menu_epoch.started(),generation_);
                }else frame.Close();
                return reject_capture(reason);
            }}
        auto access=frame.Surface().as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
        com_ptr<ID3D11Texture2D> texture;check_hresult(access->GetInterface(IID_PPV_ARGS(texture.put())));
        D3D11_TEXTURE2D_DESC desc{};texture->GetDesc(&desc);auto content=frame.ContentSize();
        if(startup&&(content.Width<=0||content.Height<=0)){frame.Close();return reject_capture(R::invalid_crop);}
        if(startup&&(unsigned(content.Width)!=pool_width||unsigned(content.Height)!=pool_height)){
            // Close the outstanding frame before Recreate. No callback mutex is
            // held while the pool replaces its buffers on the capture thread.
            ++generation_;menu_epoch.begin(menu_capture_qpc_ticks());pending_frame.discard();state->discard();
            frame.Close();pool.Recreate(capture_device,
                winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,2,content);
            pool_width=content.Width;pool_height=content.Height;return reject_capture(R::resize);
        }
        RECT client{},bounds{};POINT origin{};
        GetClientRect(hwnd,&client);ClientToScreen(hwnd,&origin);
        auto bounds_result=DwmGetWindowAttribute(hwnd,DWMWA_EXTENDED_FRAME_BOUNDS,&bounds,sizeof(bounds));
        if(startup&&FAILED(bounds_result)){frame.Close();return reject_capture(R::invalid_crop);}check_hresult(bounds_result);
        LONG left=std::max(0L,origin.x-bounds.left),top=std::max(0L,origin.y-bounds.top);
        if(startup&&(client.right<=0||client.bottom<=0)){frame.Close();return reject_capture(R::invalid_crop);}
        if(startup&&(client.right!=LONG(source_width)||client.bottom!=LONG(source_height))){
            unsigned fitted_width{},fitted_height{};
            if(!startup_capture_size::fit(client.right,client.bottom,fitted_width,fitted_height)){
                frame.Close();throw std::runtime_error("Unexpected game aspect ratio after resize");}
            source_width=client.right;source_height=client.bottom;width=fitted_width;height=fitted_height;
            pixels.resize(size_t(width)*height*4);staging=nullptr;
            ++generation_;menu_epoch.begin(menu_capture_qpc_ticks());pending_frame.discard();state->discard();
            frame.Close();return reject_capture(R::resize);
        }
        if(client.right!=LONG(source_width) || client.bottom!=LONG(source_height) ||
           left+client.right>content.Width || top+client.bottom>content.Height ||
           left+client.right>LONG(desc.Width) || top+client.bottom>LONG(desc.Height))
            {if(startup){frame.Close();return reject_capture(R::invalid_crop);}throw std::runtime_error("Game resized or client crop invalid; restart display test");}
        if(!staging) {
            desc.Width=source_width;desc.Height=source_height;desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;
            desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;desc.MiscFlags=0;
            check_hresult(device->CreateTexture2D(&desc,nullptr,staging.put()));
        }
        D3D11_BOX box{UINT(left),UINT(top),0,UINT(left+client.right),UINT(top+client.bottom),1};
        context->CopySubresourceRegion(staging.get(),0,0,0,0,texture.get(),0,&box);
        D3D11_MAPPED_SUBRESOURCE mapped{};check_hresult(context->Map(staging.get(),0,D3D11_MAP_READ,0,&mapped));
        pixel_sampler.copy(pixels.data(),width,height,static_cast<const unsigned char*>(mapped.pData),
                           mapped.RowPitch,source_width,source_height);
        context->Unmap(staging.get(),0);frame.Close();
        if(recover_window&&(!usable(hwnd)||state->closed())){
            retire_capture();menu_epoch.begin(menu_capture_qpc_ticks());return reject_capture(R::window_unavailable);
        }
        if(startup){const auto now=menu_capture_qpc_ticks();
            if(!menu_epoch.accept(captured,now))return reject_capture(timestamp_rejection(captured,now));}
        ++received;++diagnostic_.accepted;diagnostic_.result=R::accepted;return true;
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
    uint64_t source_generation=0;
    unsigned chain_width{},chain_height{};
    outage_diagnostics::MenuSnapshot diagnostic_;
    bool performance_diagnostics=false;
    std::chrono::steady_clock::time_point performance_start{},render_start{};
    unsigned performance_uploads=0,performance_calls=0,performance_rendered=0;
    double performance_cpu_ms=0,performance_max_ms=0;
    bool finish_diagnostic(bool result){
        if(performance_diagnostics){
            const auto now=std::chrono::steady_clock::now();
            const double cost=std::chrono::duration<double,std::milli>(now-render_start).count();
            ++performance_calls;if(result)++performance_rendered;
            performance_cpu_ms+=cost;performance_max_ms=std::max(performance_max_ms,cost);
            const double seconds=std::chrono::duration<double>(now-performance_start).count();
            if(seconds>=1){
                printf("{\"menu_capture_performance\":true,\"observed_ns\":%lld,\"seconds\":%.6f,\"new_frames\":%u,\"render_calls\":%u,\"rendered\":%u,\"render_cpu_ms\":%.3f,\"maximum_render_ms\":%.3f,\"width\":%u,\"height\":%u}\n",
                    gears_vr::monotonic_ns(),seconds,uploads-performance_uploads,performance_calls,performance_rendered,
                    performance_cpu_ms,performance_max_ms,source.width,source.height);fflush(stdout);
                performance_start=now;performance_uploads=uploads;performance_calls=performance_rendered=0;
                performance_cpu_ms=performance_max_ms=0;
            }
        }
        diagnostic_.capture=source.diagnostic_snapshot();diagnostic_.current=current;return result;
    }
    void create_display_chain() {
        XrSwapchain next{};
        XrSwapchainCreateInfo create{XR_TYPE_SWAPCHAIN_CREATE_INFO};create.usageFlags=XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT|XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
        create.format=format;create.width=source.width;create.height=source.height;create.sampleCount=1;create.faceCount=1;create.arraySize=1;create.mipCount=1;
        xr_check(xrCreateSwapchain(session,&create,&next),"create game display swapchain");
        try {
            uint32_t count{};xr_check(xrEnumerateSwapchainImages(next,0,&count,nullptr),"game images");
            if(!count)throw std::runtime_error("Empty game display swapchain");
            std::vector<XrSwapchainImageD3D12KHR> next_images(count,{XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR});
            const auto capacity=count;
            xr_check(xrEnumerateSwapchainImages(next,count,&count,reinterpret_cast<XrSwapchainImageBaseHeader*>(next_images.data())),"read game images");
            if(count!=capacity||!next_images[0].texture)throw std::runtime_error("Game display images changed");
            const auto desc=next_images[0].texture->GetDesc();UINT64 bytes{},row_size{};UINT rows{};
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT next_footprint{};
            if(!transition_card_layout::valid_image(desc,format,source.width,source.height))
                throw std::runtime_error("Invalid game display image layout");
            // A runtime may return a typeless resource in the selected format
            // family; buffer copies still require the selected typed footprint.
            auto copy_desc=desc;copy_desc.Format=format;
            device->GetCopyableFootprints(&copy_desc,0,1,0,&next_footprint,&rows,&row_size,&bytes);
            std::fprintf(stderr,"Game display layout: selected=%u resource={dimension=%u width=%llu height=%u format=%u array=%u mips=%u samples=%u quality=%u layout=%u flags=%u} upload={format=%u width=%u height=%u depth=%u pitch=%u offset=%llu rows=%u row_bytes=%llu bytes=%llu}\n",
                unsigned(format),unsigned(desc.Dimension),static_cast<unsigned long long>(desc.Width),desc.Height,
                unsigned(desc.Format),unsigned(desc.DepthOrArraySize),unsigned(desc.MipLevels),
                desc.SampleDesc.Count,desc.SampleDesc.Quality,unsigned(desc.Layout),unsigned(desc.Flags),
                unsigned(next_footprint.Footprint.Format),next_footprint.Footprint.Width,next_footprint.Footprint.Height,next_footprint.Footprint.Depth,
                next_footprint.Footprint.RowPitch,static_cast<unsigned long long>(next_footprint.Offset),rows,
                static_cast<unsigned long long>(row_size),static_cast<unsigned long long>(bytes));
            if(!transition_card_layout::valid_footprint(next_footprint,bytes,format,source.width,source.height,rows,row_size))
                throw std::runtime_error("Invalid game display upload footprint");
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
    GameScreen(ID3D12Device* d,ID3D12CommandQueue* q,HWND hwnd,DWORD pid,std::string output,bool startup_panel=false,uint64_t expected_creation=0):source(hwnd,pid,startup_panel,expected_creation),device(d),queue(q),prefix(std::move(output)),startup(startup_panel) {
        source_generation=source.generation();
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
    struct WindowSnapshot {
        uint64_t generation{},hwnd{};std::int64_t epoch_100ns{},captured_100ns{};
        unsigned width{},height{},uploads{};uint64_t hash{};
    };
    WindowSnapshot window_snapshot()const{return {source.generation(),uint64_t(source.window_identity()),
        source.menu_started(),source.menu_captured(),source.width,source.height,uploads,last_hash};}
    // Bounded source evidence only; submitted compositor pixels are checked separately.
    void save_source_preview(std::string const& path)const{
        const unsigned width=std::min(640u,source.width);
        const unsigned height=std::max(1u,unsigned(uint64_t(source.height)*width/source.width));
        FILE* output{};fopen_s(&output,path.c_str(),"wb");if(!output)return;
        fprintf(output,"P6\n%u %u\n255\n",width,height);
        for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){
            const auto* pixel=source.pixels.data()+
                (size_t(uint64_t(y)*source.height/height)*source.width+size_t(uint64_t(x)*source.width/width))*4;
            const unsigned char rgb[]={pixel[2],pixel[1],pixel[0]};fwrite(rgb,1,3,output);
        }
        fclose(output);
    }
    // Same XR render thread as render(); leaves GPU objects/fence ownership intact.
    void begin_menu_epoch(){
        if(!startup)return;current=false;next_capture={};source.begin_menu_epoch();
    }
    void suspend_capture(){
        if(!startup)return;current=false;next_capture={};source.suspend_capture();
    }
    outage_diagnostics::MenuSnapshot diagnostic_snapshot()const{
        return diagnostic_;
    }
    void enable_performance_diagnostics(){performance_diagnostics=true;performance_start=std::chrono::steady_clock::now();}
    bool window_fresh(std::int64_t max_age){
        return startup&&current&&source.available()&&source_generation==source.generation()&&source.menu_fresh(nullptr,max_age);
    }
    bool render(XrCompositionLayerQuad& quad,std::int64_t max_age=MenuCaptureEpoch::maximum_age) {
        if(performance_diagnostics)render_start=std::chrono::steady_clock::now();
        using Reason=outage_diagnostics::MenuRenderReason;
        diagnostic_={};diagnostic_.cached_only=false;diagnostic_.render_attempted=true;
        diagnostic_.reason=Reason::in_progress;
        if(startup){
            source.resume_capture();
            const bool available=source.available();
            diagnostic_.available_checked=true;diagnostic_.available=available;
            if(source_generation!=source.generation()){
                source_generation=source.generation();current=false;next_capture={};
                diagnostic_.generation_changed=true;diagnostic_.reason=Reason::generation_changed;
            }
            if(!available){current=false;diagnostic_.reason=Reason::window_unavailable;return finish_diagnostic(false);}
        }
        auto now=std::chrono::steady_clock::now();
        // Startup/recovery panels consume the latest available WGC frame on
        // each XR tick. Missing or slightly future frames cost no extra 33 ms
        // before retry; the mailbox and timestamp/epoch gates still apply.
        if(startup||now>=next_capture) {
            if(!startup)next_capture=now+std::chrono::milliseconds(67);
            diagnostic_.update_attempted=true;
            const bool updated=source.update();diagnostic_.updated=updated;
            if(startup&&source_generation!=source.generation()){
                source_generation=source.generation();current=false;
                diagnostic_.generation_changed=true;diagnostic_.reason=Reason::generation_changed;
            }
            if(startup&&(chain_width!=source.width||chain_height!=source.height)){
                // A resized client can precede its matching WGC frame. Do not
                // submit the old swapchain with the new client image extent.
                current=false;if(updated)create_display_chain();
            }
            if(updated) {
                const auto hash=menu_pixels::fingerprint(source.pixels.data(),size_t(source.width)*source.height);
                if(uploads && hash!=last_hash)++changes;last_hash=hash;if(!uploads)first_hash=hash;
                void* mapped{};D3D12_RANGE read{0,0};dx_check(upload->Map(0,&read,&mapped),"map game upload");
                for(unsigned y=0;y<source.height;++y) {
                    auto* dst=static_cast<unsigned char*>(mapped)+footprint.Offset+size_t(y)*footprint.Footprint.RowPitch;
                    const auto* src=source.pixels.data()+size_t(y)*source.width*4;
                    menu_pixels::upload_row(dst,src,source.width,format==DXGI_FORMAT_R8G8B8A8_UNORM);
                }
                upload->Unmap(0,nullptr);
                uint32_t index{};XrSwapchainImageAcquireInfo acquire{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};xr_check(xrAcquireSwapchainImage(chain,&acquire,&index),"acquire game image");
                XrSwapchainImageWaitInfo wait{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wait.timeout=1000000000;xr_check(xrWaitSwapchainImage(chain,&wait),"wait game image");
                if(index>=images.size()||!images[index].texture)throw std::runtime_error("Invalid game display image index");
                auto* image=images[index].texture;
                if(!transition_card_layout::valid_image(image->GetDesc(),format,chain_width,chain_height))
                    throw std::runtime_error("Game display image layout changed");
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
        if(startup){
            // The window may close while the upload waits for its GPU fence.
            const bool available=source.available();
            diagnostic_.available_checked=true;diagnostic_.available=available;
            // Same short-circuit order and calls as the display gate; diagnostics
            // never obtain a second freshness time or probe window availability.
            if(!available){current=false;diagnostic_.reason=Reason::window_unavailable;}
            else if(source_generation!=source.generation()){
                current=false;diagnostic_.generation_changed=true;diagnostic_.reason=Reason::generation_changed;
            }else{
                diagnostic_.fresh_checked=true;
                diagnostic_.fresh=source.menu_fresh(&diagnostic_.fresh_now_100ns,max_age);
                if(!diagnostic_.fresh){current=false;diagnostic_.reason=Reason::capture_not_fresh;}
            }
            source_generation=source.generation();
        }
        diagnostic_.current=current;
        if(!current || (!startup&&now-last_frame>std::chrono::seconds(1))){
            if(diagnostic_.reason==Reason::in_progress)diagnostic_.reason=current?Reason::image_expired:Reason::no_uploaded_image;
            return finish_diagnostic(false);
        }
        quad={XR_TYPE_COMPOSITION_LAYER_QUAD};quad.space=view_space;quad.eyeVisibility=XR_EYE_VISIBILITY_BOTH;
        quad.pose.orientation.w=1;quad.pose.position.z=-2;quad.size={1.6f,1.6f*source.height/source.width};
        quad.subImage.swapchain=chain;quad.subImage.imageRect.extent={int32_t(source.width),int32_t(source.height)};
        ++rendered;diagnostic_.reason=Reason::rendered;return finish_diagnostic(true);
    }
};
