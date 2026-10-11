#pragma once
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <atomic>
#include <cstring>
#include <stdexcept>
#include <string>

namespace gears_vr {
using Microsoft::WRL::ComPtr;
// One GPU slot. Producer never overwrites it until the consumer's GPU fence
// acknowledges the previous sequence. Both devices must use the same adapter.
struct alignas(8) GpuPacket {
    unsigned magic,version,size,pid,width,height,format;
    LUID adapter;
    volatile LONG ready,stopped,consumer;
    alignas(8) volatile LONG64 sequence;
};
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
class GpuPublisher {
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<IDXGISwapChain3> chain;
    ComPtr<ID3D12Resource> texture,source;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> list;
    ComPtr<ID3D12Fence> ready_fence,done_fence;
    HANDLE map{},shared[3]{};
    GpuPacket* packet{};
    SRWLOCK lock=SRWLOCK_INIT;
    UINT64 serial=0;
    ULONGLONG next_capture=0;
    bool in_flight=false,initialized=false;
    HRESULT error=S_OK;
    bool complete(){return ready_fence && ready_fence->GetCompletedValue()!=~UINT64(0) && ready_fence->GetCompletedValue()>=serial;}
    void detach(){source.Detach();texture.Detach();list.Detach();allocator.Detach();ready_fence.Detach();done_fence.Detach();chain.Detach();queue.Detach();device.Detach();}
public:
    std::atomic<unsigned> frames{0},busy{0};
    std::atomic<unsigned> state{0}; // 0 absent,1 bound,2 ready,3 failed,4 stopped
    GpuPublisher()=default;
    GpuPublisher(GpuPublisher const&)=delete;
    ~GpuPublisher(){close();}
    bool bind(ID3D12CommandQueue* q,IDXGISwapChain* swap){
        if(!TryAcquireSRWLockExclusive(&lock))return false;
        bool ok=false;
        if(state==0 && SUCCEEDED(swap->QueryInterface(IID_PPV_ARGS(&chain))) && SUCCEEDED(q->GetDevice(IID_PPV_ARGS(&device)))){
            queue=q;state=1;ok=true;
        }
        ReleaseSRWLockExclusive(&lock);return ok;
    }
    bool prepare(){
        AcquireSRWLockExclusive(&lock);
        if(state!=1){ReleaseSRWLockExclusive(&lock);return false;}
        bool ok=false;
        try {
            ComPtr<ID3D12Resource> sample;gpu_check(chain->GetBuffer(0,IID_PPV_ARGS(&sample)),"share sample");
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
            map=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(GpuPacket),gpu_name(pid,L"metadata").c_str());
            if(!map || GetLastError()==ERROR_ALREADY_EXISTS)throw std::runtime_error("GPU metadata already exists or cannot be created");
            packet=static_cast<GpuPacket*>(MapViewOfFile(map,FILE_MAP_ALL_ACCESS,0,0,sizeof(GpuPacket)));
            if(!packet)throw std::runtime_error("GPU metadata map failed");
            memset(packet,0,sizeof(*packet));packet->magic=0x47564731;packet->version=1;packet->size=sizeof(*packet);packet->pid=pid;
            packet->width=static_cast<unsigned>(desc.Width);packet->height=desc.Height;packet->format=desc.Format;packet->adapter=device->GetAdapterLuid();
            initialized=true;state=2;InterlockedExchange(&packet->ready,1);ok=true;
        }catch(...){error=E_FAIL;state=3;}
        ReleaseSRWLockExclusive(&lock);return ok;
    }
    void poll(){
        AcquireSRWLockExclusive(&lock);
        if(in_flight && complete()){source.Reset();in_flight=false;}
        ReleaseSRWLockExclusive(&lock);
    }
    // Render callback: no CPU wait, file I/O, allocations of GPU resources or
    // waits on the consumer queue. Busy slot means skip this frame.
    void capture(IDXGISwapChain* swap){
        if(state!=2 || !TryAcquireSRWLockExclusive(&lock))return;
        do {
            if(!InterlockedCompareExchange(&packet->consumer,0,0) || GetTickCount64()<next_capture)break;
            ComPtr<IUnknown> a,b;
            if(FAILED(chain.As(&a)) || FAILED(swap->QueryInterface(IID_PPV_ARGS(&b))) || a.Get()!=b.Get())break;
            if(in_flight && complete()){source.Reset();in_flight=false;}
            auto done=done_fence->GetCompletedValue();
            if(done==~UINT64(0) || ready_fence->GetCompletedValue()==~UINT64(0)){error=DXGI_ERROR_DEVICE_REMOVED;state=3;break;}
            if(in_flight || done<serial){++busy;break;}
            error=chain->GetBuffer(chain->GetCurrentBackBufferIndex(),IID_PPV_ARGS(&source));if(FAILED(error)){state=3;break;}
            auto desc=source->GetDesc();
            if(desc.Width!=packet->width || desc.Height!=packet->height || static_cast<unsigned>(desc.Format)!=packet->format){source.Reset();error=E_INVALIDARG;state=3;break;}
            error=allocator->Reset();if(SUCCEEDED(error))error=list->Reset(allocator.Get(),nullptr);if(FAILED(error)){state=3;break;}
            gpu_barrier(list.Get(),source.Get(),D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_COPY_SOURCE);
            gpu_barrier(list.Get(),texture.Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_DEST);
            list->CopyResource(texture.Get(),source.Get());
            gpu_barrier(list.Get(),texture.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_COMMON);
            gpu_barrier(list.Get(),source.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_PRESENT);
            error=list->Close();if(FAILED(error)){state=3;break;}
            ID3D12CommandList* commands[]={list.Get()};in_flight=true;queue->ExecuteCommandLists(1,commands);
            error=queue->Signal(ready_fence.Get(),++serial);if(FAILED(error)){state=3;break;}
            InterlockedExchange64(&packet->sequence,static_cast<LONG64>(serial));next_capture=GetTickCount64()+16;++frames;
        }while(false);
        ReleaseSRWLockExclusive(&lock);
    }
    HRESULT failure()const{return error;}
    bool drain(unsigned timeout_ms){
        ULONGLONG deadline=GetTickCount64()+timeout_ms;
        do{poll();if(!in_flight)return true;Sleep(1);}while(GetTickCount64()<deadline);
        return false;
    }
    bool close(){
        if(packet && initialized)InterlockedExchange(&packet->stopped,1);
        bool safe=!in_flight || (SUCCEEDED(error) && complete());
        if(!safe)detach();else{source.Reset();texture.Reset();list.Reset();allocator.Reset();ready_fence.Reset();done_fence.Reset();chain.Reset();queue.Reset();device.Reset();}
        if(packet)UnmapViewOfFile(packet);packet=nullptr;if(map)CloseHandle(map);map=nullptr;
        for(auto& handle:shared){if(handle)CloseHandle(handle);handle=nullptr;}initialized=false;state=4;return safe;
    }
};
class GpuReceiver {
    HANDLE map{};GpuPacket* packet{};bool claimed=false;
    ComPtr<ID3D12Fence> ready_fence,done_fence;
    UINT64 pending=0,last=0;
public:
    ComPtr<ID3D12Resource> texture;
    unsigned width{},height{};DXGI_FORMAT format{};DWORD producer_pid{};
    GpuReceiver()=default;GpuReceiver(GpuReceiver const&)=delete;
    ~GpuReceiver(){close();}
    void open(ID3D12Device* device,DWORD pid){
        if(packet)throw std::runtime_error("GPU receiver already open");
        map=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,gpu_name(pid,L"metadata").c_str());
        if(!map)throw std::runtime_error("GPU producer metadata unavailable");
        packet=static_cast<GpuPacket*>(MapViewOfFile(map,FILE_MAP_ALL_ACCESS,0,0,sizeof(GpuPacket)));
        if(!packet || InterlockedCompareExchange(&packet->ready,0,0)!=1 || packet->magic!=0x47564731 || packet->version!=1 || packet->size!=sizeof(*packet) || packet->pid!=pid || InterlockedCompareExchange(&packet->stopped,0,0))throw std::runtime_error("GPU metadata rejected");
        LUID luid=device->GetAdapterLuid();if(memcmp(&luid,&packet->adapter,sizeof(luid)))throw std::runtime_error("GPU adapter differs from producer");
        if(InterlockedCompareExchange(&packet->consumer,static_cast<LONG>(GetCurrentProcessId()),0))throw std::runtime_error("GPU slot already has a consumer");
        claimed=true;producer_pid=pid;
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
        if(!packet || pending || InterlockedCompareExchange(&packet->stopped,0,0))return false;
        auto serial=static_cast<UINT64>(InterlockedCompareExchange64(&packet->sequence,0,0));
        if(!serial || serial<=last)return false;
        if(ready_fence->GetCompletedValue()==~UINT64(0))throw std::runtime_error("Producer device removed");
        gpu_check(queue->Wait(ready_fence.Get(),serial),"wait producer fence");pending=serial;return true;
    }
    void acknowledge(ID3D12CommandQueue* queue){
        if(!pending)throw std::runtime_error("No GPU frame acquired");
        gpu_check(queue->Signal(done_fence.Get(),pending),"acknowledge GPU frame");last=pending;pending=0;
    }
    UINT64 sequence()const{return last;}
    bool stopped()const{return !packet || InterlockedCompareExchange(&packet->stopped,0,0)!=0;}
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
