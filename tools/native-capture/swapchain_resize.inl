// DXGI lifecycle handling for the game-resolution transport. No requested
// dimensions or queue arguments are changed. Call only the original API once.
#if defined(GEARS_GAME_RESOLUTION) && GEARS_GAME_RESOLUTION
namespace game_resize {
using ResizeFn=HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*,UINT,UINT,UINT,DXGI_FORMAT,UINT);
using Resize1Fn=HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain3*,UINT,UINT,UINT,DXGI_FORMAT,UINT,const UINT*,IUnknown* const*);
ResizeFn original{};Resize1Fn original1{};
thread_local unsigned depth{};
std::atomic<unsigned> calls{},completed{},failed{},rejected{},drain_failed{};
struct Depth {Depth(){++depth;}~Depth(){--depth;}};
struct Boundary {uintptr_t identity{},queue{};bool begun{},publisher{};};
Boundary begin(IDXGISwapChain* chain){
    Boundary out{};
    if(depth!=1||!recording.load()||!frame_link_diagnostics||self_testing)return out;
    {
        auto guard=frame_lock();out.identity=canonical_identity(chain);
        const auto binding=swapchain_bindings.registered(out.identity);
        if(!binding.chain)return {};
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
    DXGI_SWAP_CHAIN_DESC desc{};std::array<uintptr_t,16> resources{};
    bool readable=SUCCEEDED(chain->GetDesc(&desc))&&desc.BufferCount&&desc.BufferCount<=resources.size();
    if(readable)for(unsigned i=0;i<desc.BufferCount;++i){gears_vr::ComPtr<ID3D12Resource> buffer;
        if(FAILED(chain->GetBuffer(i,IID_PPV_ARGS(&buffer)))||!(resources[i]=canonical_identity(buffer.Get()))){readable=false;break;}}
    // A failed API call retains the old queue and buffers. Authenticate that
    // inventory below; only a successful queue migration changes ownership.
    if(SUCCEEDED(result)&&!queue_verified)swapchain_bindings.invalidate();
    const bool admitted=swapchain_bindings.complete_resize(boundary.identity,desc.BufferDesc.Width,desc.BufferDesc.Height,
        readable?resources.data():nullptr,readable?desc.BufferCount:0,SUCCEEDED(result));
    if(SUCCEEDED(result)&&admitted)++completed;else if(FAILED(result))++failed;else ++rejected;
    frame_link.invalidate_all();retire_stereo_window();frame_target_stats.clear_bindings();
    if(boundary.publisher){StereoPublishBypass bypass;stereo_publisher.end_resize();}
}
HRESULT STDMETHODCALLTYPE hook(IDXGISwapChain* chain,UINT count,UINT width,UINT height,DXGI_FORMAT format,UINT flags){
    Scope scope;Depth nested;const auto boundary=begin(chain);
    const auto result=original(chain,count,width,height,format,flags);
    finish(boundary,chain,result);return result;
}
HRESULT STDMETHODCALLTYPE hook1(IDXGISwapChain3* chain,UINT count,UINT width,UINT height,DXGI_FORMAT format,UINT flags,
                                const UINT* nodes,IUnknown* const* queues){
    Scope scope;Depth nested;const auto boundary=begin(chain);bool same_queue=true;
    if(boundary.begun&&queues){
        DXGI_SWAP_CHAIN_DESC desc{};const auto n=count?count:(SUCCEEDED(chain->GetDesc(&desc))?desc.BufferCount:0);
        same_queue=n&&n<=16;
        for(unsigned i=0;same_queue&&i<n;++i){IUnknown* value{};
            same_queue=safe_copy(&value,queues+i,sizeof(value))&&value&&canonical_identity(value)==boundary.queue;}
    }
    const auto result=original1(chain,count,width,height,format,flags,nodes,queues);
    finish(boundary,chain,result,same_queue);return result;
}
bool install_hooks(IDXGISwapChain1* chain){
    auto table=*reinterpret_cast<void***>(chain);
    bool ok=install(table[13],reinterpret_cast<void*>(hook),reinterpret_cast<void**>(&original));
    gears_vr::ComPtr<IDXGISwapChain3> extended;
    if(FAILED(chain->QueryInterface(IID_PPV_ARGS(&extended))))return false;
    auto table3=*reinterpret_cast<void***>(extended.Get());
    return install(table3[39],reinterpret_cast<void*>(hook1),reinterpret_cast<void**>(&original1))&&ok;
}
}
#endif
