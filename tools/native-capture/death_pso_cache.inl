#include "death_pso_cache.h"
namespace death_pso_cache {
#if defined(GEARS_DEATH_PSO_CACHE) && GEARS_DEATH_PSO_CACHE
using namespace gears_death_pso;
using StateFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12PipelineState*);
using BundleFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12GraphicsCommandList*);
StateFn original_set{},original_clear{};BundleFn original_bundle{};
std::array<void*,3> targets{};
Bank bank;Captures captures;
std::array<ID3D12PipelineState*,list_limit> transient{};
struct Cache {ID3D12PipelineState* held{};uint64_t ticket{},world{};bool attempted{},saved{};HRESULT hr=E_PENDING;size_t size{};DWORD error{};wchar_t path[MAX_PATH]{};};
// Permanent holders intentionally have no destructor: bounded to two until process exit.
std::array<Cache,pso_limit> caches{};
struct Receipt {bool present{},known{};unsigned eye{},reason{};uint64_t ticket{},world{},generation{};uintptr_t raw_list{},identity{},pso{},native_view{};long long qpc{},witness{};size_t cache=pso_limit;};
std::array<Receipt,2> receipts{};
uint64_t rejected{},duplicate_draws{},resets{},failed_resets{},clears{},bundles{},untracked_sets{};
bool stopped{};
bool method(uintptr_t object,unsigned slot,void* expected=nullptr){
    uintptr_t table{},address{};HMODULE owner{};
    if(!system_resource_methods(object)||!safe_copy(&table,reinterpret_cast<void*>(object),8)||
       !safe_copy(&address,reinterpret_cast<void*>(table+uintptr_t(slot)*8),8)||!address||
       (expected&&address!=reinterpret_cast<uintptr_t>(expected)))return false;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(address),&owner))return false;
    return owner==GetModuleHandleW(L"D3D12Core.dll")||owner==GetModuleHandleW(L"d3d12.dll");
}
uintptr_t identity(IUnknown* object){
    if(!system_resource_methods(reinterpret_cast<uintptr_t>(object)))return 0;
    __try{return canonical_identity(object);}__except(EXCEPTION_EXECUTE_HANDLER){return 0;}
}
bool hold(ID3D12PipelineState* actual,ID3D12PipelineState*& held,uintptr_t& canonical){
    held=nullptr;canonical=0;
    if(!method(reinterpret_cast<uintptr_t>(actual),8))return false;
    __try{
        if(FAILED(actual->QueryInterface(IID_PPV_ARGS(&held)))||!held)return false;
        if(method(reinterpret_cast<uintptr_t>(held),8))canonical=identity(held);
        if(!canonical){held->Release();held=nullptr;return false;}return true;
    }__except(EXCEPTION_EXECUTE_HANDLER){held=nullptr;canonical=0;return false;}
}
HRESULT cached_blob(ID3D12PipelineState* pso,ID3DBlob** blob){
    if(!method(reinterpret_cast<uintptr_t>(pso),8))return E_INVALIDARG;
    __try{return pso->GetCachedBlob(blob);}__except(EXCEPTION_EXECUTE_HANDLER){return E_FAIL;}
}
void invalidate(uintptr_t id){
    for(size_t i=0;i<bank.lists.size();++i)if(!id||bank.lists[i].identity==id){if(transient[i])transient[i]->Release();transient[i]=nullptr;}
    bank.invalidate(id);
}
bool selected(){
    auto* selected=current_unpossessed_quad_bank();
    return !stopped&&selected&&gears_death_pso::selected(!self_testing&&continuous_session,recording.load(),frame_link_diagnostics,
        consuming_frozen_frame.enabled,consuming_frozen_frame.unpossessed,consuming_frame_ticket,consuming_frozen_frame.id,selected->ordinal);
}
bool list_methods(ID3D12GraphicsCommandList* list){
    const auto raw=reinterpret_cast<uintptr_t>(list);
    return method(raw,25,targets[0])&&method(raw,11,targets[1])&&method(raw,27,targets[2]);
}
void STDMETHODCALLTYPE set(ID3D12GraphicsCommandList* list,ID3D12PipelineState* pso){
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());Scope scope;ngx_mv_compute_capture::SetScope witness(list,pso,caller,scope.world());
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    const auto id=identity(list);invalidate(id);
    ID3D12PipelineState* held{};uintptr_t canonical{};size_t index=list_limit;uint64_t generation{};
    if(scope.world()&&!stereo_publish_bypass&&selected()&&id&&list_methods(list)){
        generation=frame_link.current(id).generation;
        if(generation&&hold(pso,held,canonical)){index=bank.slot(id);if(index==list_limit){held->Release();held=nullptr;invalidate(0);}}
        else ++rejected;
    }else ++untracked_sets;
    const auto revision=bank.invalidations;original_set(list,pso);witness.finish();
    if(held){if(bank.invalidations!=revision)held->Release();
        else {transient[index]=held;bank.witness(index,generation,canonical,now());}}
}
void STDMETHODCALLTYPE clear(ID3D12GraphicsCommandList* list,ID3D12PipelineState* pso){
    Scope scope;ngx_mv_compute_capture::RevocationScope revoke(list,1);std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);const auto id=identity(list);++clears;invalidate(id);
    original_clear(list,pso);invalidate(id); // ClearState's initial PSO is deliberately never inferred.
    revoke.finish();
}
void STDMETHODCALLTYPE bundle(ID3D12GraphicsCommandList* list,ID3D12GraphicsCommandList* child){
    Scope scope;ngx_mv_compute_capture::RevocationScope revoke(list,2);std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);const auto id=identity(list);++bundles;invalidate(id);
    original_bundle(list,child);invalidate(id); // No inference from bundle contents.
    revoke.finish();
}
void reset(ID3D12GraphicsCommandList* list,HRESULT hr){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);++resets;if(FAILED(hr))++failed_resets;invalidate(identity(list));
}
HRESULT reset_passthrough(ID3D12GraphicsCommandList* list,ID3D12CommandAllocator* allocator,ID3D12PipelineState* state){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    const auto hr=death_hdr_pixels::reset_passthrough(list,allocator,state);reset(list,hr);return hr;
}
gears_unpossessed_quad::NativeView native_view(){
    gears_unpossessed_quad::NativeView out;out.renderer=reinterpret_cast<uintptr_t>(consuming_renderer);uintptr_t slot{},context{};
    if(safe_copy(&slot,reinterpret_cast<void*>(__readgsqword(0x58)),8)&&slot&&safe_copy(&context,reinterpret_cast<void*>(slot+0x20),8)&&context&&
        safe_copy(&out.current_view,reinterpret_cast<void*>(context+0x14c0),8))out.read_mask|=1;
    if(out.renderer){if(safe_copy(&out.renderer_views,reinterpret_cast<void*>(out.renderer+0x68),8))out.read_mask|=2;
        if(safe_copy(&out.renderer_count,reinterpret_cast<void*>(out.renderer+0x70),4))out.read_mask|=4;}
    out.eye_valid=out.read_mask==7&&gears_unpossessed_quad::native_eye_index(out.current_view,out.renderer_views,out.renderer_count,out.eye);return out;
}
void draw(ID3D12GraphicsCommandList* list){
    if(!late_quad_candidate.active||late_quad_candidate.caller!=0xd80a7b)return;
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!selected())return;
    const auto view=native_view();if(!view.eye_valid){++rejected;return;}
    auto& r=receipts[view.eye];if(r.present){++duplicate_draws;return;}
    auto* selected=current_unpossessed_quad_bank();r.present=true;r.eye=view.eye;r.ticket=selected->ticket.id;r.world=selected->ticket.world;
    r.raw_list=reinterpret_cast<uintptr_t>(list);r.native_view=view.current_view;r.qpc=now();r.identity=identity(list);
    r.generation=frame_link.current(r.identity).generation;const auto* state=list_methods(list)?bank.current(r.identity,r.generation):nullptr;
    if(!state){r.reason=1;return;}
    r.pso=state->pso;r.witness=state->witness_qpc;
    const auto index=static_cast<size_t>(state-bank.lists.data());
    if(!transient[index]||identity(transient[index])!=r.pso){r.reason=2;return;}
    const auto cache=captures.admit(r.pso);r.cache=cache;if(cache==pso_limit){r.reason=3;return;}
    auto& entry=caches[cache];if(!entry.held){transient[index]->AddRef();entry.held=transient[index];entry.ticket=r.ticket;entry.world=r.world;}
    r.known=true;
}
void poll(){
    for(size_t i=0;i<caches.size();++i){ID3D12PipelineState* pso{};uint64_t ticket{};
        {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);auto& c=caches[i];if(!c.held||c.attempted)continue;c.attempted=true;pso=c.held;ticket=c.ticket;}
        ID3DBlob* blob{};const HRESULT hr=cached_blob(pso,&blob);size_t size{};bool saved{};DWORD error{};wchar_t path[MAX_PATH]{};
        if(SUCCEEDED(hr)&&blob){size=blob->GetBufferSize();if(save_size(size)){
            const auto length=GetModuleFileNameW(own_module,path,MAX_PATH);auto* slash=length&&length<MAX_PATH?wcsrchr(path,L'\\'):nullptr;
            if(slash){*slash=0;wchar_t parent[MAX_PATH]{};wcscpy_s(parent,path);
                if(swprintf_s(path,L"%s\\death-pso-cache-%lu-%llu-%zu-%lld.raw",parent,GetCurrentProcessId(),ticket,i,now())>0){
                    HANDLE file=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
                    if(file!=INVALID_HANDLE_VALUE){DWORD written{};saved=WriteFile(file,blob->GetBufferPointer(),static_cast<DWORD>(size),&written,nullptr)&&written==size&&FlushFileBuffers(file);
                        if(!saved)error=GetLastError();if(!CloseHandle(file)){saved=false;error=GetLastError();}}
                    else error=GetLastError();
                }else error=ERROR_BUFFER_OVERFLOW;
            }else error=ERROR_PATH_NOT_FOUND;
        }else error=size?ERROR_FILE_TOO_LARGE:ERROR_INVALID_DATA;}
        else if(SUCCEEDED(hr))error=ERROR_INVALID_DATA;
        if(blob)blob->Release();
        {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);auto& c=caches[i];c.hr=hr;c.size=size;c.saved=saved;c.error=error;wcscpy_s(c.path,path);}
    }
}
void stop(){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);stopped=true;invalidate(0);}
void print(FILE* file){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    fprintf(file,"death_pso_cache enabled=1 cpu_only=1 gpu_commands_added=0 selection_ordinal=49 list_limit=64 transient_hold_limit=64 permanent_hold_limit=2 blob_save_limit=4194304 cache_internal_allocation_bounded=0 cache_attempts_per_pso=1 opaque_stops_attempt=1 list_overflow=%llu capacity_lost=%u pso_overflow=%llu rejected=%llu duplicate_draws=%llu resets=%llu failed_resets=%llu clears=%llu bundles=%llu untracked_sets=%llu all_mutating_hook_paths=1 inherit_reset_pso=0 infer_bundle_pso=0\n",
        bank.overflow,unsigned(bank.capacity_lost),captures.overflow,rejected,duplicate_draws,resets,failed_resets,clears,bundles,untracked_sets);
    for(auto const& r:receipts)if(r.present)fprintf(file,"death_pso_draw eye=%u ticket=%llu world=%llu list=%llx canonical=%llx generation=%llu native_view=%llx draw_qpc=%lld set_witness_qpc=%lld known=%u reason=%u pso=%llx cache=%zu reason_bits=unknown_generation_or_state:1,held_identity_failed:2,cache_capacity:3\n",
        r.eye,r.ticket,r.world,static_cast<unsigned long long>(r.raw_list),static_cast<unsigned long long>(r.identity),r.generation,static_cast<unsigned long long>(r.native_view),r.qpc,r.witness,unsigned(r.known),r.reason,static_cast<unsigned long long>(r.pso),r.cache);
    for(size_t i=0;i<captures.count;++i){auto const& c=caches[i];fprintf(file,"death_pso_blob index=%zu canonical=%llx ticket=%llu world=%llu attempted=%u hr=%08lx size=%zu saved=%u error=%lu held_until_process=%u path=%ls interpretation=offline_only\n",
        i,static_cast<unsigned long long>(captures.identities[i]),c.ticket,c.world,unsigned(c.attempted),static_cast<unsigned long>(c.hr),c.size,unsigned(c.saved),c.error,unsigned(c.held!=nullptr),c.path);}
}
#else
void draw(ID3D12GraphicsCommandList*){}
void reset(ID3D12GraphicsCommandList*,HRESULT){}
HRESULT reset_passthrough(ID3D12GraphicsCommandList* list,ID3D12CommandAllocator* allocator,ID3D12PipelineState* state){return death_hdr_pixels::reset_passthrough(list,allocator,state);}
void poll(){}void stop(){}
void print(FILE* file){fprintf(file,"death_pso_cache enabled=0\n");}
#endif
}
