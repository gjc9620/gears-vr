// One bounded original-swapchain diagnostic over the existing prime owner.
// Opt-in only; never a stereo ticket, menu/action permit or replacement writer.
// Reuse the established queue-bound one-shot copy, including its GPU lifetime.
namespace startup_menu_readback {
OneShotReadback copy;
std::mutex mutex;
std::atomic<bool> enabled{},retired{};
uintptr_t identity{};
uintptr_t source_queue{};
bool saved{},reported{};
wchar_t path[MAX_PATH]{};
void arm() {
    wchar_t value[2]{};
    enabled=prime_owner.handle&&GetEnvironmentVariableW(L"GEARSVR_TEST_NATIVE_MENU_READBACK",value,2)==1&&value[0]==L'1';
}
void present(IDXGISwapChain* chain,UINT flags) {
    if(!enabled.load()||retired.load()||!prime_recording.load()||recording.load()||(flags&DXGI_PRESENT_TEST))return;
    std::unique_lock<std::mutex> lock(mutex,std::try_to_lock);if(!lock.owns_lock()||copy.state.load()!=2)return;
    const auto binding=swapchain_bindings.lookup(canonical_identity(chain));
    if(!binding.chain||binding.chain!=identity||!binding.queue||binding.resizing||binding.rejected)return;
    StereoPublishBypass bypass;copy.capture(chain,5000);
}
void poll(FILE* status) {
    if(!enabled.load()||retired.load())return;
    std::lock_guard<FrameHookMutex> frame(frame_hook_mutex);
    std::lock_guard<std::mutex> lock(mutex);StereoPublishBypass bypass;
    if(retired.load())return; // A resize may have retired us while waiting for these locks.
    if(!identity&&prime_recording.load()&&!recording.load()&&prime_present_observation.calls.load()>=180){
        std::lock_guard<std::mutex> owners(binding_owner_mutex);
        gears_vr::ComPtr<IDXGISwapChain> selected;gears_vr::ComPtr<ID3D12CommandQueue> queue;unsigned candidates{};
        for(auto const& owner:binding_owners){
            if(!owner.chain||!owner.queue)continue;
            gears_vr::ComPtr<IDXGISwapChain> chain;DXGI_SWAP_CHAIN_DESC desc{};DWORD pid{};
            if(FAILED(owner.chain->QueryInterface(IID_PPV_ARGS(&chain)))||FAILED(chain->GetDesc(&desc))||
               !desc.OutputWindow||desc.BufferDesc.Width<640||desc.BufferDesc.Height<360||
               !GetWindowThreadProcessId(desc.OutputWindow,&pid)||pid!=GetCurrentProcessId()||
               !swapchain_bindings.lookup(canonical_identity(chain.Get())).chain)continue;
            ++candidates;selected=chain;
            if(FAILED(owner.queue->QueryInterface(IID_PPV_ARGS(&queue))))queue.Reset();
        }
        if(candidates==1&&queue&&copy.bind(queue.Get(),selected.Get())){identity=canonical_identity(selected.Get());source_queue=canonical_identity(queue.Get());}
    }
    if(copy.state.load()==1&&GetTickCount64()-copy.bound_at>=5000)(void)copy.prepare();
    if(copy.state.load()==6){
        if(!path[0]){
            const auto length=GetModuleFileNameW(own_module,path,MAX_PATH);
            auto slash=length&&length<MAX_PATH?wcsrchr(path,L'\\'):nullptr;
            if(!slash||_snwprintf_s(slash+1,MAX_PATH-size_t(slash+1-path),_TRUNCATE,
                L"startup-menu-buffer-%lu-%llu.ppm",GetCurrentProcessId(),prime_owner_creation_time)<0)path[0]=0;
        }
        if(path[0])saved=copy.save(path);
    }
    const auto state=copy.state.load();
    if(!reported&&(saved||state==5)){
        fprintf(status,"startup_menu_readback state=%u saved=%u hr=%08lx size=%u,%u format=%u queue=%p chain=%p original_buffer=1 stereo_acceptance=0\n",
            state,unsigned(saved),static_cast<unsigned long>(copy.failure),copy.width,copy.height,copy.format,reinterpret_cast<void*>(source_queue),reinterpret_cast<void*>(identity));fflush(status);reported=true;
        // Compare actual current buffers, not the creation-time observer bank:
        // pre-level ResizeBuffers may precede the full transport's anchoring.
        // These temporary references remain inside the existing resize lock.
        if(auto chain=copy.bound_chain()){DXGI_SWAP_CHAIN_DESC desc{};
            if(SUCCEEDED(chain->GetDesc(&desc))&&desc.BufferCount<=16){
                fprintf(status,"startup_menu_actual_source resource=%p index=%u buffer_count=%u\n",
                    reinterpret_cast<void*>(copy.captured_source()?canonical_identity(copy.captured_source()):0),copy.index,desc.BufferCount);
                for(unsigned i=0;i<desc.BufferCount;++i){gears_vr::ComPtr<ID3D12Resource> resource;
                    if(FAILED(chain->GetBuffer(i,IID_PPV_ARGS(&resource))))continue;
                    const auto d=resource->GetDesc();fprintf(status,"startup_menu_current_buffer index=%u resource=%p size=%llu,%u format=%u\n",
                        i,reinterpret_cast<void*>(canonical_identity(resource.Get())),static_cast<unsigned long long>(d.Width),d.Height,unsigned(d.Format));}
                fflush(status);
            }
        }
    }
}
// Original resize already has a bounded mirror-drain boundary. Retire this
// optional copy at the same boundary; never leave an old backbuffer reference.
bool before_resize(IDXGISwapChain* chain) {
    if(!enabled.load())return true;
    std::lock_guard<FrameHookMutex> frame(frame_hook_mutex);std::lock_guard<std::mutex> lock(mutex);
    if(!identity||identity!=canonical_identity(chain))return true;
    retired=true;const auto deadline=GetTickCount64()+1000;
    do {if(copy.release()){identity=0;return true;}Sleep(1);}while(GetTickCount64()<deadline);
    return false;
}
bool finish(FILE* status) {
    if(!enabled.load())return true;
    std::lock_guard<FrameHookMutex> frame(frame_hook_mutex);std::lock_guard<std::mutex> lock(mutex);
    retired=true;const auto deadline=GetTickCount64()+1000;bool clean{};
    do {clean=copy.release();if(clean)break;Sleep(1);}while(GetTickCount64()<deadline);
    fprintf(status,"startup_menu_readback_cleanup restored=%u saved=%u\n",unsigned(clean),unsigned(saved));fflush(status);
    return clean;
}
}
