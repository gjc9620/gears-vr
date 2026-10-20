// Shared DXGI call wrappers. Only game-resolution mode participates in its
// transport lifecycle. A runtime-size startup candidate records the boundary
// and may resolve zero dimensions from fresh evidence, without changing its
// owned render-size plan. Queue arguments remain unchanged; forward once.
#if GEARS_DXGI_RESIZE_HOOKS
namespace game_resize {
using ResizeFn=HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*,UINT,UINT,UINT,DXGI_FORMAT,UINT);
using Resize1Fn=HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain3*,UINT,UINT,UINT,DXGI_FORMAT,UINT,const UINT*,IUnknown* const*);
ResizeFn original{};Resize1Fn original1{};
thread_local unsigned depth{};
struct Depth {Depth(){++depth;}~Depth(){--depth;}};
struct LastErrorBoundary {DWORD value{GetLastError()};~LastErrorBoundary(){SetLastError(value);}};
// An asynchronous mirror owns backbuffer references until its queue fence
// completes. Keep capture submissions excluded through the original resize.
struct MirrorResizeBoundary {
    std::unique_lock<FrameHookMutex> lock{frame_hook_mutex,std::defer_lock};
    bool ready{true};
    explicit MirrorResizeBoundary(IDXGISwapChain* chain){
#if defined(GEARS_DESKTOP_MIRROR) && GEARS_DESKTOP_MIRROR
        if constexpr(gears_vr::DesktopMirror::asynchronous){
            if(depth!=1)return;
            lock.lock();
            if(!swapchain_bindings.registered(canonical_identity(chain)).chain){lock.unlock();return;}
            StereoPublishBypass bypass;
            ready=desktop_mirror.drain(1000);
        }
#else
        (void)chain;
#endif
    }
};
#if GEARS_DXGI_RESIZE_TRANSPORT
std::atomic<unsigned> calls{},completed{},failed{},rejected{},drain_failed{};
struct ResizeTrace {
    unsigned method{},count{},width{},height{},format{},flags{};uintptr_t chain{};
    uint64_t generation_before{},generation_after{};long long begin_qpc{},end_qpc{};
    HRESULT pre_result{E_PENDING},result{E_PENDING};DXGI_SWAP_CHAIN_DESC pre{},post{};
    std::array<uintptr_t,16> identities{},interfaces{};
    std::array<D3D12_RESOURCE_DESC,16> descriptions{};
    bool readable{},admitted{},queue_verified{};
};
std::array<ResizeTrace,8> resize_traces{};unsigned resize_trace_count{};
struct Boundary {uintptr_t identity{},queue{};bool begun{},publisher{};unsigned trace_index{8};};
Boundary begin(IDXGISwapChain* chain,unsigned method,UINT count,UINT width,UINT height,DXGI_FORMAT format,UINT flags){
    Boundary out{};
    if(depth!=1||!recording.load()||!frame_link_diagnostics||self_testing)return out;
    {
        auto guard=frame_lock();out.identity=canonical_identity(chain);
        const auto binding=swapchain_bindings.registered(out.identity);
        if(!binding.chain)return {};
        // Bounded live snapshots, only while the caller owns the interfaces.
        // No saved resource address is queried after its lifetime ends.
        {std::lock_guard<FrameHookMutex> diagnostic_guard(frame_hook_mutex);
        if(resize_trace_count<resize_traces.size()){
            out.trace_index=resize_trace_count++;auto& trace=resize_traces[out.trace_index];
            trace.method=method;trace.count=count;trace.width=width;trace.height=height;
            trace.format=unsigned(format);trace.flags=flags;trace.chain=out.identity;
            trace.generation_before=binding.generation;trace.begin_qpc=now();
            trace.pre_result=chain->GetDesc(&trace.pre);
        }}
        out.queue=binding.queue;++calls;
        out.begun=swapchain_bindings.begin_resize(out.identity);
        frame_link.invalidate_all();retire_stereo_window();frame_target_stats.clear_bindings();
        if(!out.begun){++rejected;return out;}
        if(stereo_gpu_publish&&!stereo_publish_counts.stopped.load()){
            StereoPublishBypass bypass;out.publisher=stereo_publisher.begin_resize(chain);
            if(!out.publisher){++rejected;swapchain_bindings.invalidate();}
        }
    }
    // The producer's pending copy can retain the old backbuffer. Finish it and
    // release that reference before forwarding DXGI ResizeBuffers. XR holds an
    // independent shared texture and retires it through the ready==2 handshake.
    if(out.publisher&&!stereo_publisher.drain(3000))++drain_failed;
    return out;
}
void finish(Boundary const& boundary,IDXGISwapChain* chain,HRESULT result,bool queue_verified=true){
    if(!boundary.begun)return;
    auto guard=frame_lock();
    std::lock_guard<FrameHookMutex> diagnostic_guard(frame_hook_mutex);
    DXGI_SWAP_CHAIN_DESC desc{};std::array<uintptr_t,16> resources{};
    ResizeTrace* trace=boundary.trace_index<resize_trace_count?&resize_traces[boundary.trace_index]:nullptr;
    bool readable=SUCCEEDED(chain->GetDesc(&desc))&&desc.BufferCount&&desc.BufferCount<=resources.size();
    if(readable)for(unsigned i=0;i<desc.BufferCount;++i){gears_vr::ComPtr<ID3D12Resource> buffer;
        if(FAILED(chain->GetBuffer(i,IID_PPV_ARGS(&buffer)))||!(resources[i]=canonical_identity(buffer.Get()))){readable=false;break;}
        if(trace){trace->interfaces[i]=reinterpret_cast<uintptr_t>(buffer.Get());trace->identities[i]=resources[i];
            trace->descriptions[i]=buffer->GetDesc();}}
    // A failed API call retains the old queue and buffers. Authenticate that
    // inventory below; only a successful queue migration changes ownership.
    if(SUCCEEDED(result)&&!queue_verified)swapchain_bindings.invalidate();
    const bool admitted=swapchain_bindings.complete_resize(boundary.identity,desc.BufferDesc.Width,desc.BufferDesc.Height,
        readable?resources.data():nullptr,readable?desc.BufferCount:0,SUCCEEDED(result));
    if(SUCCEEDED(result)&&admitted)++completed;else if(FAILED(result))++failed;else ++rejected;
    if(trace){trace->post=desc;trace->result=result;trace->readable=readable;trace->admitted=admitted;
        trace->queue_verified=queue_verified;trace->end_qpc=now();
        trace->generation_after=swapchain_bindings.registered(boundary.identity).generation;}
    frame_link.invalidate_all();retire_stereo_window();frame_target_stats.clear_bindings();
    if(boundary.publisher){StereoPublishBypass bypass;stereo_publisher.end_resize();}
}
#endif
HRESULT STDMETHODCALLTYPE hook(IDXGISwapChain* chain,UINT count,UINT width,UINT height,DXGI_FORMAT format,UINT flags){
    LastErrorBoundary error;Scope scope;Depth nested;
    const auto startup=gears_swapchain_startup::begin_resize(gears_swapchain_startup::resize,chain,count,width,height,format,flags,
        _ReturnAddress(),prime_recording.load(),recording.load());
#if GEARS_DXGI_RESIZE_TRANSPORT
    const auto boundary=begin(chain,0,count,width,height,format,flags);
#endif
    const auto dimensions=gears_swapchain_startup::prepare_resize(startup,chain,width,height,depth==1);
    MirrorResizeBoundary mirror_boundary(chain);
    SetLastError(error.value);
    const auto result=mirror_boundary.ready?original(chain,count,dimensions.width,dimensions.height,format,flags):DXGI_ERROR_WAS_STILL_DRAWING;
    error.value=GetLastError();
    gears_swapchain_startup::finish(startup,result,chain);
#if GEARS_DXGI_RESIZE_TRANSPORT
    finish(boundary,chain,result);
#endif
    return result;
}
HRESULT STDMETHODCALLTYPE hook1(IDXGISwapChain3* chain,UINT count,UINT width,UINT height,DXGI_FORMAT format,UINT flags,
                                const UINT* nodes,IUnknown* const* queues){
    LastErrorBoundary error;Scope scope;Depth nested;
    const auto startup=gears_swapchain_startup::begin_resize(gears_swapchain_startup::resize1,chain,count,width,height,format,flags,
        _ReturnAddress(),prime_recording.load(),recording.load(),nodes,queues);
#if GEARS_DXGI_RESIZE_TRANSPORT
    const auto boundary=begin(chain,1,count,width,height,format,flags);bool same_queue=true;
    if(boundary.begun&&queues){
        DXGI_SWAP_CHAIN_DESC desc{};const auto n=count?count:(SUCCEEDED(chain->GetDesc(&desc))?desc.BufferCount:0);
        same_queue=n&&n<=16;
        for(unsigned i=0;same_queue&&i<n;++i){IUnknown* value{};
            same_queue=safe_copy(&value,queues+i,sizeof(value))&&value&&canonical_identity(value)==boundary.queue;}
    }
#endif
    const auto dimensions=gears_swapchain_startup::prepare_resize(startup,chain,width,height,depth==1);
    MirrorResizeBoundary mirror_boundary(chain);
    SetLastError(error.value);
    const auto result=mirror_boundary.ready?original1(chain,count,dimensions.width,dimensions.height,format,flags,nodes,queues):DXGI_ERROR_WAS_STILL_DRAWING;
    error.value=GetLastError();
    gears_swapchain_startup::finish(startup,result,chain);
#if GEARS_DXGI_RESIZE_TRANSPORT
    finish(boundary,chain,result,same_queue);
#endif
    return result;
}
bool install_hooks(IDXGISwapChain1* chain){
    auto table=*reinterpret_cast<void***>(chain);
    bool ok=install(table[13],reinterpret_cast<void*>(hook),reinterpret_cast<void**>(&original));
    gears_vr::ComPtr<IDXGISwapChain3> extended;
    if(FAILED(chain->QueryInterface(IID_PPV_ARGS(&extended))))return false;
    auto table3=*reinterpret_cast<void***>(extended.Get());
    return install(table3[39],reinterpret_cast<void*>(hook1),reinterpret_cast<void**>(&original1))&&ok;
}
#if GEARS_DXGI_RESIZE_TRANSPORT
void print(FILE* file){
    std::lock_guard<FrameHookMutex> diagnostic_guard(frame_hook_mutex);
    fprintf(file,"resize_trace_summary rows=%u limit=8 dimensions_source=live_api no_resource_refs_retained=1\n",resize_trace_count);
    for(unsigned i=0;i<resize_trace_count;++i){const auto& t=resize_traces[i];
        fprintf(file,"resize_trace row=%u method=%u chain=%llx generation=%llu,%llu qpc=%lld,%lld requested=%u,%u,%u,%u,%u pre_hr=%08lx pre_size=%u,%u pre_count=%u result=%08lx post_size=%u,%u post_count=%u readable=%u admitted=%u queue_verified=%u\n",
            i,t.method,static_cast<unsigned long long>(t.chain),t.generation_before,t.generation_after,t.begin_qpc,t.end_qpc,
            t.count,t.width,t.height,t.format,t.flags,static_cast<unsigned long>(t.pre_result),t.pre.BufferDesc.Width,t.pre.BufferDesc.Height,t.pre.BufferCount,
            static_cast<unsigned long>(t.result),t.post.BufferDesc.Width,t.post.BufferDesc.Height,t.post.BufferCount,unsigned(t.readable),unsigned(t.admitted),unsigned(t.queue_verified));
        for(unsigned j=0;j<t.post.BufferCount&&j<t.identities.size();++j){const auto& d=t.descriptions[j];
            fprintf(file,"resize_trace_buffer row=%u index=%u canonical=%llx interface=%llx dimension=%u width=%llu height=%u format=%u samples=%u\n",
                i,j,static_cast<unsigned long long>(t.identities[j]),static_cast<unsigned long long>(t.interfaces[j]),unsigned(d.Dimension),d.Width,d.Height,unsigned(d.Format),d.SampleDesc.Count);}
    }
}
#endif
}
#endif
