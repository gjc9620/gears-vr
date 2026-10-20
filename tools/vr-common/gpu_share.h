#pragma once
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <atomic>
#include <cstring>
#include <stdexcept>
#include <string>
#include "stereo_image_ticket.h"

namespace gears_vr {
using Microsoft::WRL::ComPtr;
inline constexpr unsigned gpu_packet_version=7;
struct GpuOpeningDeferred:std::runtime_error {GpuOpeningDeferred():std::runtime_error("GPU source is being reconfigured"){};};
// One GPU slot. Producer never overwrites it until the consumer's GPU fence
// acknowledges the previous sequence. Both devices must use the same adapter.
struct alignas(8) GpuPacket {
    unsigned magic,version,size,pid,width,height,format,allow_simulated;
    LUID adapter;
    volatile LONG ready,stopped,consumer;
    alignas(8) volatile LONG64 sequence;
    std::uint64_t generation;
    StereoImageTicket ticket;
    volatile LONG world_revision,world_quiescing;
    alignas(8) volatile LONG64 world_generation,world_minimum_sample_ns;
};
struct WorldState {bool valid{};std::uint64_t generation{};bool quiescing{};std::int64_t minimum_sample_ns{};};
inline bool gpu_display_identity_matches(GpuPacket const& p,unsigned pid,std::uint64_t generation,
                                         unsigned width,unsigned height,unsigned format,bool simulated){
    return p.ready==1&&p.stopped==0&&p.magic==0x47564731&&p.version==gpu_packet_version&&p.size==sizeof(GpuPacket)&&
        p.pid==pid&&pid&&p.generation==generation&&generation&&p.allow_simulated==(simulated?1u:0u)&&
        p.width==width&&p.height==height&&p.format==format;
}
inline bool gpu_world_image_current(WorldState const& world,std::uint64_t acquired_generation,std::int64_t sample_ns){
    return world.valid&&!world.quiescing&&world.generation&&acquired_generation==world.generation&&sample_ns>world.minimum_sample_ns;
}
// Bounded seqlock snapshot fails closed during a concurrent world commit.
inline WorldState gpu_world_state(GpuPacket* packet){
    if(!packet)return {};
    for(unsigned attempt=0;attempt<4;++attempt){
        const LONG revision=InterlockedCompareExchange(&packet->world_revision,0,0);if(revision&1)continue;
        WorldState out{};out.generation=static_cast<std::uint64_t>(InterlockedCompareExchange64(&packet->world_generation,0,0));
        const LONG quiescing=InterlockedCompareExchange(&packet->world_quiescing,0,0);out.quiescing=quiescing!=0;
        out.minimum_sample_ns=InterlockedCompareExchange64(&packet->world_minimum_sample_ns,0,0);
        if(revision!=InterlockedCompareExchange(&packet->world_revision,0,0))continue;
        out.valid=packet->magic==0x47564731&&packet->version==gpu_packet_version&&packet->size==sizeof(*packet)&&out.generation&&out.minimum_sample_ns>=0&&(quiescing==0||quiescing==1);
        return out;
    }
    return {};
}
inline std::wstring gpu_name(DWORD pid,const wchar_t* suffix){
    wchar_t name[128];swprintf_s(name,L"Local\\GearsVR.Gpu.%lu.%s",pid,suffix);return name;
}
inline void gpu_check(HRESULT result,const char* action){
    if(FAILED(result))throw std::runtime_error(std::string(action)+": "+std::to_string(result));
}
inline void gpu_barrier(ID3D12GraphicsCommandList* list,ID3D12Resource* resource,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){
    D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=resource;
    b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;b.Transition.StateBefore=before;b.Transition.StateAfter=after;list->ResourceBarrier(1,&b);
}
enum class GpuCaptureReason:unsigned {not_attempted,published,not_ready,lock_busy,metadata_invalid,
    ticket_invalid,source_changed,no_consumer,rate_limited,chain_mismatch,device_removed,
    producer_in_flight,consumer_not_done,api_failure,native_stopped,native_deadline,owned_binding_mismatch,world_quiescing};
enum GpuCaptureValid:unsigned {capture_state_valid=1,capture_serial_valid=2,capture_consumer_valid=4,
    capture_ready_valid=8,capture_done_valid=16,capture_hresult_valid=32};
struct GpuCaptureTrace {GpuCaptureReason reason{GpuCaptureReason::not_attempted};unsigned valid{},state{};
    UINT64 serial_before{},serial_after{},ready_completed{},done_completed{};LONG consumer{};HRESULT result{};};
// A nonblocking observation, not an admission lock or permission to overwrite.
// World handoff must stop new publications and recheck this snapshot at commit.
struct GpuIdleSnapshot {
    bool valid{},in_flight{},reconfiguring{};
    unsigned state{};LONG consumer{};HRESULT result{};
    std::uint64_t generation{},serial{},ready_completed{},done_completed{};
};
inline bool gpu_idle_complete(GpuIdleSnapshot const& s){
    return s.valid&&s.state==2&&!s.in_flight&&!s.reconfiguring&&SUCCEEDED(s.result)&&s.generation&&s.consumer>=0&&
        s.ready_completed!=~std::uint64_t(0)&&s.done_completed!=~std::uint64_t(0)&&
        s.ready_completed>=s.serial&&s.done_completed>=s.serial;
}
inline const char* gpu_capture_reason_name(GpuCaptureReason reason){
    static const char* names[]={"not_attempted","published","not_ready","lock_busy","metadata_invalid","ticket_invalid",
        "source_changed","no_consumer","rate_limited","chain_mismatch","device_removed","producer_in_flight",
        "consumer_not_done","api_failure","native_stopped","native_deadline","owned_binding_mismatch","world_quiescing"};
    auto i=static_cast<unsigned>(reason);return i<std::size(names)?names[i]:"unknown";
}
class GpuPublisher {
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<IDXGISwapChain3> chain;
    ComPtr<ID3D12Resource> texture,source;
    ComPtr<ID3D12Resource> bound_resource;
    ComPtr<IUnknown> bound_identity,bound_device_identity;
    D3D12_RESOURCE_DESC bound_desc{};
    UINT64 bound_generation{};
    bool owned_binding_=false,owned_detached_=false;
    UINT64 retired_owned_world_{},retired_owned_ticket_{};
    D3D12_RESOURCE_STATES bound_idle=D3D12_RESOURCE_STATE_COMMON;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> list;
    ComPtr<ID3D12Fence> ready_fence,done_fence;
    HANDLE map{},shared[3]{};
    GpuPacket* packet{};
    SRWLOCK lock=SRWLOCK_INIT;
    UINT64 serial=0;
    ULONGLONG next_capture=0;
    bool in_flight=false,initialized=false,allow_simulated_=false;
    bool resize_active=false,rebuild_claimed_=false;
    std::uint64_t generation_=0,world_generation_=1;
    std::int64_t world_minimum_sample_ns_=0;
    bool world_quiescing_=false;
    HRESULT error=S_OK;
    bool complete(){return ready_fence && ready_fence->GetCompletedValue()!=~UINT64(0) && ready_fence->GetCompletedValue()>=serial;}
    void detach(){source.Detach();texture.Detach();list.Detach();allocator.Detach();ready_fence.Detach();done_fence.Detach();chain.Detach();queue.Detach();device.Detach();
        bound_resource.Detach();bound_identity.Detach();bound_device_identity.Detach();}
    static bool owned_desc_supported(D3D12_RESOURCE_DESC const& d,D3D12_RESOURCE_STATES idle) {
        if(d.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||!d.Width||d.Width%2||d.Width>8192||!d.Height||d.Height>8192||
           d.SampleDesc.Count!=1||d.MipLevels!=1||d.DepthOrArraySize!=1||
           (d.Format!=DXGI_FORMAT_R10G10B10A2_UNORM&&d.Format!=DXGI_FORMAT_R8G8B8A8_UNORM&&d.Format!=DXGI_FORMAT_B8G8R8A8_UNORM))return false;
        if(idle==D3D12_RESOURCE_STATE_COMMON)return true;
        if(idle==D3D12_RESOURCE_STATE_RENDER_TARGET)return (d.Flags&D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)!=0;
        return idle==D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE&&(d.Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)==0;
    }
    static bool same_desc(D3D12_RESOURCE_DESC const& a,D3D12_RESOURCE_DESC const& b) {
        return a.Dimension==b.Dimension&&a.Alignment==b.Alignment&&a.Width==b.Width&&a.Height==b.Height&&
            a.DepthOrArraySize==b.DepthOrArraySize&&a.MipLevels==b.MipLevels&&a.Format==b.Format&&
            a.SampleDesc.Count==b.SampleDesc.Count&&a.SampleDesc.Quality==b.SampleDesc.Quality&&a.Layout==b.Layout&&a.Flags==b.Flags;
    }
    bool owned_matches(ID3D12Resource* resource,UINT64 generation,D3D12_RESOURCE_STATES idle) const {
        if(!bound_resource||!resource||generation!=bound_generation||idle!=bound_idle)return false;
        ComPtr<IUnknown> identity,resource_device_identity;ComPtr<ID3D12Device> resource_device;
        return SUCCEEDED(resource->QueryInterface(IID_PPV_ARGS(&identity)))&&identity.Get()==bound_identity.Get()&&
            SUCCEEDED(resource->GetDevice(IID_PPV_ARGS(&resource_device)))&&SUCCEEDED(resource_device.As(&resource_device_identity))&&
            resource_device_identity.Get()==bound_device_identity.Get()&&same_desc(resource->GetDesc(),bound_desc);
    }
public:
    std::atomic<unsigned> frames{0},busy{0},rate_limited{0};
    std::atomic<unsigned> state{0}; // 0 absent,1 bound,2 ready,3 failed,4 stopped,5 resizing
    GpuPublisher()=default;
    GpuPublisher(GpuPublisher const&)=delete;
    ~GpuPublisher(){close();}
    bool bind(ID3D12CommandQueue* q,IDXGISwapChain* swap,bool allow_sim=false){
        if(!TryAcquireSRWLockExclusive(&lock))return false;
        bool ok=false;
        if(state==0 && SUCCEEDED(swap->QueryInterface(IID_PPV_ARGS(&chain))) && SUCCEEDED(q->GetDevice(IID_PPV_ARGS(&device)))){
            queue=q;allow_simulated_=allow_sim;state=1;ok=true;
        }
        ReleaseSRWLockExclusive(&lock);return ok;
    }
    // The caller certifies actual RHI state and same-queue render/copy ordering.
    // D3D12 cannot query a resource's current state. COMMON/PRESENT both have
    // value zero; accepting it is not proof that an arbitrary resource is idle.
    // Rebinding requires explicit source retirement and a committed later world;
    // it keeps the same shared slot, DIRECT queue, device and resource descriptor.
    bool bind_resource(ID3D12CommandQueue* q,ID3D12Resource* resource,UINT64 generation,
                      D3D12_RESOURCE_STATES idle,bool allow_sim=false) {
        if(!q||!resource||!generation||!TryAcquireSRWLockExclusive(&lock))return false;
        bool ok=false;
        const bool first=state==0;
        const bool rebind=state==2&&owned_binding_&&owned_detached_&&!bound_resource&&!source&&
            !world_quiescing_&&world_generation_>retired_owned_world_&&generation>bound_generation&&
            allow_sim==allow_simulated_&&gpu_idle_complete(idle_locked());
        if((first||rebind)&&q->GetDesc().Type==D3D12_COMMAND_LIST_TYPE_DIRECT) {
            ComPtr<ID3D12Device> q_device,r_device;ComPtr<IUnknown> q_identity,r_identity,identity;
            auto desc=resource->GetDesc();
            if(owned_desc_supported(desc,idle)&&SUCCEEDED(q->GetDevice(IID_PPV_ARGS(&q_device)))&&
               SUCCEEDED(resource->GetDevice(IID_PPV_ARGS(&r_device)))&&SUCCEEDED(q_device.As(&q_identity))&&
               SUCCEEDED(r_device.As(&r_identity))&&q_identity.Get()==r_identity.Get()&&
               SUCCEEDED(resource->QueryInterface(IID_PPV_ARGS(&identity)))) {
                ComPtr<IUnknown> old_queue,new_queue;
                const bool same_binding=first||(idle==bound_idle&&same_desc(desc,bound_desc)&&q_identity.Get()==bound_device_identity.Get()&&
                    SUCCEEDED(queue.As(&old_queue))&&SUCCEEDED(q->QueryInterface(IID_PPV_ARGS(&new_queue)))&&old_queue.Get()==new_queue.Get());
                if(same_binding){
                    device=q_device;queue=q;bound_resource=resource;bound_identity=identity;bound_device_identity=q_identity;
                    bound_desc=desc;bound_generation=generation;bound_idle=idle;allow_simulated_=allow_sim;
                    owned_binding_=true;owned_detached_=false;if(first)state=1;ok=true;
                }
            }
        }
        ReleaseSRWLockExclusive(&lock);return ok;
    }
    bool prepare(){
        AcquireSRWLockExclusive(&lock);
        if(state!=1||resize_active){ReleaseSRWLockExclusive(&lock);return false;}
        bool ok=false;
        try {
            ComPtr<ID3D12Resource> sample;
            if(bound_resource){if(!owned_matches(bound_resource.Get(),bound_generation,bound_idle))throw std::runtime_error("Owned binding changed");sample=bound_resource;}
            else gpu_check(chain->GetBuffer(0,IID_PPV_ARGS(&sample)),"share sample");
            auto desc=sample->GetDesc();
            if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D || !desc.Width || desc.Width>8192 || !desc.Height || desc.Height>8192 || desc.SampleDesc.Count!=1 || desc.MipLevels!=1 || desc.DepthOrArraySize!=1 ||
               (desc.Format!=DXGI_FORMAT_R10G10B10A2_UNORM && desc.Format!=DXGI_FORMAT_R8G8B8A8_UNORM && desc.Format!=DXGI_FORMAT_B8G8R8A8_UNORM))throw std::runtime_error("Unsupported shared image");
            desc.Flags=D3D12_RESOURCE_FLAG_NONE;
            D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
            gpu_check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_SHARED,&desc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&texture)),"shared texture");
            gpu_check(device->CreateFence(0,D3D12_FENCE_FLAG_SHARED,IID_PPV_ARGS(&ready_fence)),"ready fence");
            gpu_check(device->CreateFence(0,D3D12_FENCE_FLAG_SHARED,IID_PPV_ARGS(&done_fence)),"done fence");
            gpu_check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"share allocator");
            gpu_check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)),"share commands");gpu_check(list->Close(),"share initial close");
            DWORD pid=GetCurrentProcessId();
            ID3D12DeviceChild* objects[]={texture.Get(),ready_fence.Get(),done_fence.Get()};const wchar_t* names[]={L"texture",L"ready",L"done"};
            for(unsigned i=0;i<3;++i)gpu_check(device->CreateSharedHandle(objects[i],nullptr,GENERIC_ALL,gpu_name(pid,names[i]).c_str(),&shared[i]),"create shared handle");
            if(!map){
                map=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(GpuPacket),gpu_name(pid,L"metadata").c_str());
                if(!map || GetLastError()==ERROR_ALREADY_EXISTS)throw std::runtime_error("GPU metadata already exists or cannot be created");
                packet=static_cast<GpuPacket*>(MapViewOfFile(map,FILE_MAP_ALL_ACCESS,0,0,sizeof(GpuPacket)));
                if(!packet)throw std::runtime_error("GPU metadata map failed");
            }
            if(generation_==~std::uint64_t(0))throw std::runtime_error("GPU generation exhausted");
            memset(packet,0,sizeof(*packet));packet->magic=0x47564731;packet->version=gpu_packet_version;packet->size=sizeof(*packet);packet->pid=pid;packet->ticket=StereoImageTicket{};
            packet->generation=++generation_;serial=0;
            packet->world_generation=static_cast<LONG64>(world_generation_);packet->world_minimum_sample_ns=world_minimum_sample_ns_;packet->world_quiescing=world_quiescing_?1:0;
            packet->allow_simulated=allow_simulated_?1:0;
            packet->width=static_cast<unsigned>(desc.Width);packet->height=desc.Height;packet->format=desc.Format;packet->adapter=device->GetAdapterLuid();
            initialized=true;state=2;InterlockedExchange(&packet->ready,1);rebuild_claimed_=false;ok=true;
        }catch(...){error=E_FAIL;state=3;}
        ReleaseSRWLockExclusive(&lock);return ok;
    }
    void poll(){
        AcquireSRWLockExclusive(&lock);
        if(in_flight && complete()){source.Reset();in_flight=false;}
        if(state==5&&!resize_active&&!in_flight&&
           (rebuild_claimed_||!packet||InterlockedCompareExchange(&packet->consumer,-1,0)==0)){
            // A second resize can occur after retirement but before prepare.
            // Keep our reservation across that boundary; never adopt an
            // unexplained -1 from shared memory as an owned reservation.
            rebuild_claimed_=packet!=nullptr;
            // ready==2 keeps new receivers out. The previous consumer releases
            // its claim only after its GPU work and eye swapchains are retired.
            // The final slot may never have been acquired. After explicit
            // consumer retirement it needs no acknowledge; producer completion
            // plus the released consumer claim proves both queues are done.
            // -1 reserves the slot against a receiver that observed ready==1
            // immediately before begin_resize. prepare publishes consumer=0 last.
            source.Reset();texture.Reset();list.Reset();allocator.Reset();ready_fence.Reset();done_fence.Reset();
            for(auto& handle:shared){if(handle)CloseHandle(handle);handle=nullptr;}
            initialized=false;state=1;
        }
        ReleaseSRWLockExclusive(&lock);
    }
    // Called around the actual DXGI resize, outside ordinary Present work.
    // Shared metadata survives; no old image may be admitted while ready==2.
    bool begin_resize(IDXGISwapChain* swap){
        AcquireSRWLockExclusive(&lock);bool ok=false;
        ComPtr<IDXGISwapChain3> candidate;
        if(!owned_binding_&&!resize_active&&state!=3&&state!=4&&swap&&
           (state==0||(SUCCEEDED(swap->QueryInterface(IID_PPV_ARGS(&candidate)))&&candidate.Get()==chain.Get()))){
            resize_active=true;ok=true;
            if(state!=0){state=5;if(packet)InterlockedExchange(&packet->ready,2);}
        }
        ReleaseSRWLockExclusive(&lock);return ok;
    }
    void end_resize(){AcquireSRWLockExclusive(&lock);resize_active=false;ReleaseSRWLockExclusive(&lock);}
    // Render callback: no CPU wait, file I/O, allocations of GPU resources or
    // waits on the consumer queue. Busy slot means skip this frame.
    bool capture(IDXGISwapChain* swap,const StereoImageTicket* ticket=nullptr,GpuCaptureTrace* traceout=nullptr){
        return capture_impl(swap,nullptr,0,D3D12_RESOURCE_STATE_PRESENT,ticket,traceout);
    }
    bool capture_resource(ID3D12Resource* resource,UINT64 generation,D3D12_RESOURCE_STATES idle,
                          StereoImageTicket const& ticket,GpuCaptureTrace* traceout=nullptr) {
        return capture_impl(nullptr,resource,generation,idle,&ticket,traceout);
    }
private:
    bool capture_impl(IDXGISwapChain* swap,ID3D12Resource* resource,UINT64 generation,D3D12_RESOURCE_STATES idle,
                      const StereoImageTicket* ticket,GpuCaptureTrace* traceout){
        if(traceout){*traceout={};traceout->state=state.load();traceout->valid=capture_state_valid;}
        if(state!=2){if(traceout)traceout->reason=GpuCaptureReason::not_ready;return false;}
        if(!TryAcquireSRWLockExclusive(&lock)){if(traceout)traceout->reason=GpuCaptureReason::lock_busy;return false;}
        bool published=false;
        if(traceout){traceout->state=state.load();traceout->serial_before=traceout->serial_after=serial;traceout->valid|=capture_serial_valid;}
        auto reason=[&](GpuCaptureReason why){if(traceout)traceout->reason=why;};
        auto failure=[&](HRESULT hr){if(traceout){traceout->result=hr;traceout->valid|=capture_hresult_valid;}reason(GpuCaptureReason::api_failure);};
        do {
            if(state!=2){reason(GpuCaptureReason::not_ready);break;} // mark_stopped may win before lock.
            if(packet->magic!=0x47564731 || packet->version!=gpu_packet_version || packet->size!=sizeof(*packet) || packet->pid!=GetCurrentProcessId() || packet->allow_simulated!=(allow_simulated_?1u:0u)){reason(GpuCaptureReason::metadata_invalid);break;}
            if(world_quiescing_){reason(GpuCaptureReason::world_quiescing);break;}
            if(owned_detached_){reason(GpuCaptureReason::owned_binding_mismatch);break;}
            if(bound_resource){
                if(swap||!owned_matches(resource,generation,idle)||packet->width!=bound_desc.Width||packet->height!=bound_desc.Height||
                   packet->format!=static_cast<unsigned>(bound_desc.Format)){reason(GpuCaptureReason::owned_binding_mismatch);break;}
            }else if(resource||generation||!swap){reason(GpuCaptureReason::owned_binding_mismatch);break;}
            // Copy the caller's frozen ticket once. Validation never selects a
            // newer pose or changes the metadata of a still-owned GPU slot.
            StereoImageTicket frozen=ticket?*ticket:StereoImageTicket{};
            if(allow_simulated_&&!ticket){reason(GpuCaptureReason::ticket_invalid);break;}
            if(ticket && !stereo_image_ticket_valid(frozen,packet->width,packet->height,allow_simulated_)){reason(GpuCaptureReason::ticket_invalid);break;}
            if(owned_binding_&&retired_owned_world_&&(!ticket||frozen.id<=retired_owned_ticket_||
                frozen.sample.received_ns<=world_minimum_sample_ns_)){reason(GpuCaptureReason::ticket_invalid);break;}
            if(serial&&frozen.id&&packet->ticket.id&&frozen.sample.source!=packet->ticket.sample.source){reason(GpuCaptureReason::source_changed);break;}
            auto consumer=InterlockedCompareExchange(&packet->consumer,0,0);
            if(traceout){traceout->consumer=consumer;traceout->valid|=capture_consumer_valid;}
            if(!consumer){reason(GpuCaptureReason::no_consumer);break;}
            // Legacy diagnostic quad images keep their fixed rate cap. Frozen
            // stereo tickets use the consumer ack and GPU fences as the only
            // pacing limits, preserving the nonblocking single-slot policy.
            if(!ticket&&GetTickCount64()<next_capture){++rate_limited;reason(GpuCaptureReason::rate_limited);break;}
            if(!bound_resource){ComPtr<IUnknown> a,b;
                auto identity_result=chain.As(&a);if(FAILED(identity_result)){reason(GpuCaptureReason::chain_mismatch);break;}
                identity_result=swap->QueryInterface(IID_PPV_ARGS(&b));if(FAILED(identity_result)||a.Get()!=b.Get()){reason(GpuCaptureReason::chain_mismatch);break;}}
            if(in_flight && complete()){source.Reset();in_flight=false;}
            auto done=done_fence->GetCompletedValue();
            if(traceout){traceout->done_completed=done;traceout->valid|=capture_done_valid;}
            if(done==~UINT64(0)){error=DXGI_ERROR_DEVICE_REMOVED;state=3;reason(GpuCaptureReason::device_removed);break;}
            auto ready=ready_fence->GetCompletedValue();if(traceout){traceout->ready_completed=ready;traceout->valid|=capture_ready_valid;}
            if(ready==~UINT64(0)){error=DXGI_ERROR_DEVICE_REMOVED;state=3;reason(GpuCaptureReason::device_removed);break;}
            if(in_flight || done<serial){++busy;reason(in_flight?GpuCaptureReason::producer_in_flight:GpuCaptureReason::consumer_not_done);break;}
            if(bound_resource)source=bound_resource;
            else {error=chain->GetBuffer(chain->GetCurrentBackBufferIndex(),IID_PPV_ARGS(&source));if(FAILED(error)){state=3;failure(error);break;}}
            auto desc=source->GetDesc();
            if(desc.Width!=packet->width || desc.Height!=packet->height || static_cast<unsigned>(desc.Format)!=packet->format){source.Reset();error=E_INVALIDARG;state=3;failure(error);break;}
            error=allocator->Reset();if(SUCCEEDED(error))error=list->Reset(allocator.Get(),nullptr);if(FAILED(error)){state=3;failure(error);break;}
            const auto source_idle=bound_resource?bound_idle:D3D12_RESOURCE_STATE_PRESENT;
            gpu_barrier(list.Get(),source.Get(),source_idle,D3D12_RESOURCE_STATE_COPY_SOURCE);
            gpu_barrier(list.Get(),texture.Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_DEST);
            list->CopyResource(texture.Get(),source.Get());
            gpu_barrier(list.Get(),texture.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_COMMON);
            gpu_barrier(list.Get(),source.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,source_idle);
            error=list->Close();if(FAILED(error)){state=3;failure(error);break;}
            ID3D12CommandList* commands[]={list.Get()};in_flight=true;queue->ExecuteCommandLists(1,commands);
            error=queue->Signal(ready_fence.Get(),++serial);if(FAILED(error)){state=3;failure(error);break;}
            packet->ticket=frozen;
            InterlockedExchange64(&packet->sequence,static_cast<LONG64>(serial));next_capture=GetTickCount64()+16;++frames;
            published=true;reason(GpuCaptureReason::published);
        }while(false);
        if(traceout){traceout->serial_after=serial;traceout->state=state.load();}
        ReleaseSRWLockExclusive(&lock);
        return published;
    }
public:
    HRESULT failure()const{return error;}
private:
    GpuIdleSnapshot idle_locked() const {
        GpuIdleSnapshot candidate{};candidate.state=state.load();candidate.result=error;candidate.in_flight=in_flight;
        candidate.reconfiguring=resize_active||rebuild_claimed_;candidate.generation=generation_;candidate.serial=serial;
        if(initialized&&packet&&ready_fence&&done_fence){
            candidate.consumer=InterlockedCompareExchange(&packet->consumer,0,0);
            candidate.ready_completed=ready_fence->GetCompletedValue();candidate.done_completed=done_fence->GetCompletedValue();candidate.valid=packet->generation==generation_;
        }
        return candidate;
    }
public:
    bool try_idle(GpuIdleSnapshot& out){
        if(!TryAcquireSRWLockExclusive(&lock))return false;
        out=idle_locked();const bool idle=gpu_idle_complete(out);ReleaseSRWLockExclusive(&lock);return idle;
    }
    // Release only this publisher's source references. The shared image/fences
    // remain available to XR. Caller separately drains native owner/other GPU
    // users; this method proves only the producer/consumer shared-slot lifetime.
    // poll() must first observe producer completion; never fabricate an ACK.
    bool retire_owned_source(std::uint64_t expected_world,UINT64 expected_resource_generation){
        if(!TryAcquireSRWLockExclusive(&lock))return false;
        const bool identity=state==2&&world_quiescing_&&expected_world&&expected_world==world_generation_&&
            owned_binding_&&expected_resource_generation==bound_generation&&gpu_idle_complete(idle_locked());
        const bool ok=identity&&((owned_detached_&&!source&&!bound_resource&&!bound_identity&&retired_owned_world_==expected_world)||
            (!owned_detached_&&bound_resource&&owned_matches(bound_resource.Get(),bound_generation,bound_idle)));
        if(ok&&!owned_detached_){
            retired_owned_world_=world_generation_;retired_owned_ticket_=packet->ticket.id;
            source.Reset();bound_resource.Reset();bound_identity.Reset();owned_detached_=true;
        }
        ReleaseSRWLockExclusive(&lock);return ok;
    }
    // Admission barrier shares capture's lock. A failed commit never reopens it.
    bool begin_world_rebind(std::uint64_t expected){
        if(!TryAcquireSRWLockExclusive(&lock))return false;
        const bool ok=expected&&expected==world_generation_&&initialized&&packet&&state==2&&!resize_active&&!rebuild_claimed_;
        if(ok){InterlockedIncrement(&packet->world_revision);world_quiescing_=true;InterlockedExchange(&packet->world_quiescing,1);InterlockedIncrement(&packet->world_revision);}
        ReleaseSRWLockExclusive(&lock);return ok;
    }
    // Both queues must finish the existing slot; resource identity and serial
    // survive this handoff. Caller polls ordinary producer completion first.
    template<class Commit> bool commit_world_rebind(std::uint64_t expected,std::uint64_t next,std::int64_t minimum_ns,Commit commit){
        if(!TryAcquireSRWLockExclusive(&lock))return false;
        const bool ready=world_quiescing_&&expected&&expected==world_generation_&&expected!=~std::uint64_t(0)&&next==expected+1&&minimum_ns>world_minimum_sample_ns_&&gpu_idle_complete(idle_locked());
        // The caller's bounded memory commit runs before making the new world
        // visible to XR. It must not reenter this publisher or invoke GPU APIs.
        const bool ok=ready&&commit();
        if(ok){
            InterlockedIncrement(&packet->world_revision);world_generation_=next;world_minimum_sample_ns_=minimum_ns;
            InterlockedExchange64(&packet->world_generation,static_cast<LONG64>(next));InterlockedExchange64(&packet->world_minimum_sample_ns,minimum_ns);
            world_quiescing_=false;InterlockedExchange(&packet->world_quiescing,0);InterlockedIncrement(&packet->world_revision);
        }
        ReleaseSRWLockExclusive(&lock);return ok;
    }
    bool commit_world_rebind(std::uint64_t expected,std::uint64_t next,std::int64_t minimum_ns){
        return commit_world_rebind(expected,next,minimum_ns,[](){return true;});
    }
    // Notify the receiver and prevent new submissions. Keep all resources and
    // shared handles alive until callbacks drain and the producer fence is safe.
    void mark_stopped(){
        AcquireSRWLockExclusive(&lock);
        if(packet&&initialized)InterlockedExchange(&packet->stopped,1);
        state=4;
        ReleaseSRWLockExclusive(&lock);
    }
    bool drain(unsigned timeout_ms){
        ULONGLONG deadline=GetTickCount64()+timeout_ms;
        do{poll();if(!in_flight)return true;Sleep(1);}while(GetTickCount64()<deadline);
        return false;
    }
    bool close(){
        if(packet && initialized)InterlockedExchange(&packet->stopped,1);
        bool safe=!in_flight || (SUCCEEDED(error) && complete());
        if(!safe)detach();else{source.Reset();texture.Reset();list.Reset();allocator.Reset();ready_fence.Reset();done_fence.Reset();chain.Reset();queue.Reset();device.Reset();
            bound_resource.Reset();bound_identity.Reset();bound_device_identity.Reset();}
        if(packet)UnmapViewOfFile(packet);packet=nullptr;if(map)CloseHandle(map);map=nullptr;
        for(auto& handle:shared){if(handle)CloseHandle(handle);handle=nullptr;}initialized=false;rebuild_claimed_=false;state=4;return safe;
    }
};
class GpuReceiver {
    HANDLE map{};GpuPacket* packet{};bool claimed=false,allow_simulated_=false;
    ComPtr<ID3D12Fence> ready_fence,done_fence;
    UINT64 pending=0,last=0;
    std::uint64_t generation_{},acquired_world_generation_{};
    StereoImageTicket ticket_{};
public:
    ComPtr<ID3D12Resource> texture;
    unsigned width{},height{};DXGI_FORMAT format{};DWORD producer_pid{};
    GpuReceiver()=default;GpuReceiver(GpuReceiver const&)=delete;
    ~GpuReceiver(){close();}
    void open(ID3D12Device* device,DWORD pid,bool allow_sim=false){
        if(packet)throw std::runtime_error("GPU receiver already open");
        allow_simulated_=allow_sim;
        map=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,gpu_name(pid,L"metadata").c_str());
        if(!map)throw std::runtime_error("GPU producer metadata unavailable");
        packet=static_cast<GpuPacket*>(MapViewOfFile(map,FILE_MAP_ALL_ACCESS,0,0,sizeof(GpuPacket)));
        if(!packet)throw std::runtime_error("GPU metadata rejected");
        if(InterlockedCompareExchange(&packet->ready,0,0)!=1)throw GpuOpeningDeferred();
        if(packet->magic!=0x47564731 || packet->version!=gpu_packet_version || packet->size!=sizeof(*packet) || packet->pid!=pid || packet->allow_simulated!=(allow_simulated_?1u:0u) || InterlockedCompareExchange(&packet->stopped,0,0))throw std::runtime_error("GPU metadata rejected");
        LUID luid=device->GetAdapterLuid();if(memcmp(&luid,&packet->adapter,sizeof(luid)))throw std::runtime_error("GPU adapter differs from producer");
        generation_=packet->generation;
        const auto prior=InterlockedCompareExchange(&packet->consumer,static_cast<LONG>(GetCurrentProcessId()),0);
        if(prior==-1)throw GpuOpeningDeferred();
        if(prior)throw std::runtime_error("GPU slot already has a consumer");
        if(InterlockedCompareExchange(&packet->ready,0,0)!=1||generation_!=packet->generation){
            InterlockedCompareExchange(&packet->consumer,0,static_cast<LONG>(GetCurrentProcessId()));throw GpuOpeningDeferred();
        }
        claimed=true;producer_pid=pid;
        if(!generation_)throw std::runtime_error("GPU generation missing");
        auto open_object=[&](const wchar_t* name,REFIID iid,void** out){
            HANDLE handle{};gpu_check(device->OpenSharedHandleByName(gpu_name(pid,name).c_str(),GENERIC_ALL,&handle),"open GPU handle name");
            HRESULT result=device->OpenSharedHandle(handle,iid,out);CloseHandle(handle);gpu_check(result,"open GPU object");
        };
        open_object(L"texture",__uuidof(ID3D12Resource),reinterpret_cast<void**>(texture.GetAddressOf()));
        open_object(L"ready",__uuidof(ID3D12Fence),reinterpret_cast<void**>(ready_fence.GetAddressOf()));
        open_object(L"done",__uuidof(ID3D12Fence),reinterpret_cast<void**>(done_fence.GetAddressOf()));
        auto desc=texture->GetDesc();width=packet->width;height=packet->height;format=static_cast<DXGI_FORMAT>(packet->format);
        if(desc.Width!=width || desc.Height!=height || desc.Format!=format || !width || !height)throw std::runtime_error("Shared texture metadata differs");
    }
    bool acquire(ID3D12CommandQueue* queue){
        if(!packet || pending || InterlockedCompareExchange(&packet->stopped,0,0)||InterlockedCompareExchange(&packet->ready,0,0)!=1)return false;
        if(packet->magic!=0x47564731 || packet->version!=gpu_packet_version || packet->size!=sizeof(*packet) || packet->allow_simulated!=(allow_simulated_?1u:0u) ||
           packet->pid!=producer_pid || packet->generation!=generation_ || packet->width!=width || packet->height!=height || packet->format!=static_cast<unsigned>(format))
            throw std::runtime_error("GPU metadata changed or version rejected");
        auto serial=static_cast<UINT64>(InterlockedCompareExchange64(&packet->sequence,0,0));
        if(!serial || serial<=last)return false;
        // Until acknowledge's GPU fence completes the producer cannot reuse
        // this slot. The interlocked sequence publishes the preceding ticket.
        StereoImageTicket frozen=packet->ticket;
        if(frozen.id?!stereo_image_ticket_valid(frozen,width,height,allow_simulated_):(allow_simulated_||!stereo_image_ticket_empty(frozen)))throw std::runtime_error("GPU stereo ticket rejected");
        if(ticket_.id&&frozen.id&&ticket_.sample.source!=frozen.sample.source)throw std::runtime_error("GPU stereo source changed");
        if(ready_fence->GetCompletedValue()==~UINT64(0))throw std::runtime_error("Producer device removed");
        const auto world=world_state();if(!world.valid)return false;
        gpu_check(queue->Wait(ready_fence.Get(),serial),"wait producer fence");ticket_=frozen;pending=serial;acquired_world_generation_=world.generation;return true;
    }
    void acknowledge(ID3D12CommandQueue* queue){
        if(!pending)throw std::runtime_error("No GPU frame acquired");
        gpu_check(queue->Signal(done_fence.Get(),pending),"acknowledge GPU frame");last=pending;pending=0;
    }
    UINT64 sequence()const{return last;}
    // Private immutable snapshot of the acquired image; ack keeps it available
    // for reuse/reprojection. Replaced only by the next successful acquire.
    StereoImageTicket const& ticket()const{return ticket_;}
    bool stopped()const{return !packet || InterlockedCompareExchange(&packet->stopped,0,0)!=0;}
    bool reconfiguring()const{return packet&&!stopped()&&InterlockedCompareExchange(&packet->ready,0,0)==2;}
    std::uint64_t generation()const{return generation_;}
    WorldState world_state()const{return gpu_world_state(packet);}
    // Read-only authority for an application-owned wait card. Mirrors the
    // opened receiver identity checks without acquiring/acknowledging a slot.
    bool display_identity_valid()const{
        return packet&&claimed&&!stopped()&&InterlockedCompareExchange(&packet->ready,0,0)==1&&
            gpu_display_identity_matches(*packet,producer_pid,generation_,width,height,static_cast<unsigned>(format),allow_simulated_);
    }
    std::uint64_t acquired_world_generation()const{return acquired_world_generation_;}
    bool release_for_resize(){
        if(!reconfiguring()||pending||!claimed)return false;
        const auto done=done_fence?done_fence->GetCompletedValue():0;
        if(last&&(done==~UINT64(0)||done<last))return false;
        texture.Reset();ready_fence.Reset();done_fence.Reset();
        // Only this explicit, drained handoff permits another consumer to open.
        InterlockedCompareExchange(&packet->consumer,0,static_cast<LONG>(GetCurrentProcessId()));
        UnmapViewOfFile(packet);packet=nullptr;CloseHandle(map);map=nullptr;claimed=false;
        last=pending=0;ticket_={};generation_=acquired_world_generation_=0;producer_pid=0;
        return true;
    }
    void close(){
        // Keep the single-consumer claim until producer exit. A replacement
        // consumer must not race commands from the previous process.
        bool safe=!pending && (!last || (done_fence && done_fence->GetCompletedValue()!=~UINT64(0) && done_fence->GetCompletedValue()>=last));
        if(safe){texture.Reset();ready_fence.Reset();done_fence.Reset();}
        else{texture.Detach();ready_fence.Detach();done_fence.Detach();}
        if(packet)UnmapViewOfFile(packet);packet=nullptr;if(map)CloseHandle(map);map=nullptr;claimed=false;
    }
};
}
