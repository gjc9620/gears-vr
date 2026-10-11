#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <bcrypt.h>
#include <intrin.h>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <share.h>
#include "MinHook.h"
#include "gpu_readback.h"
#include "../vr-common/gpu_share.h"

namespace {
using LegacyFn=HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory*,IUnknown*,DXGI_SWAP_CHAIN_DESC*,IDXGISwapChain**);
using HwndFn=HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory2*,IUnknown*,HWND,const DXGI_SWAP_CHAIN_DESC1*,const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*,IDXGIOutput*,IDXGISwapChain1**);
using CompositionFn=HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory2*,IUnknown*,const DXGI_SWAP_CHAIN_DESC1*,IDXGIOutput*,IDXGISwapChain1**);
using PresentFn=HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*,UINT,UINT);
LegacyFn legacy{};HwndFn hwnd_create{};CompositionFn composition{};PresentFn present{};
HMODULE own{};
std::atomic<unsigned> running{},active{},dropped{},present_failures{};
std::atomic<bool> recording{};
SRWLOCK lock=SRWLOCK_INIT;
struct Binding {uintptr_t chain,queue,device,window,caller;unsigned method,width,height,format,queue_type;long long qpc;};
struct Seen {uintptr_t chain,window;unsigned count;};
std::array<Binding,64> bindings{};std::array<Seen,64> seen{};
unsigned binding_count{},seen_count{};
OneShotReadback readback;
gears_vr::GpuPublisher gpu_publisher;
bool gpu_share_enabled{};
bool readback_enabled{};
bool manual_readback{};
std::atomic<bool> readback_requested{};
struct Target {void* address;unsigned char before[32];};
std::array<Target,4> targets{};unsigned target_count{};
struct Scope {Scope(){++active;}~Scope(){--active;}};
uintptr_t identity(IUnknown* object) {
    IUnknown* unknown{};if(!object || FAILED(object->QueryInterface(IID_PPV_ARGS(&unknown))))return 0;
    auto value=reinterpret_cast<uintptr_t>(unknown);unknown->Release();return value;
}
long long now(){LARGE_INTEGER value;QueryPerformanceCounter(&value);return value.QuadPart;}
void remember(unsigned method,IUnknown* input,IDXGISwapChain* chain,void* caller) {
    if(!recording || !input || !chain)return;
    ID3D12CommandQueue* queue{};ID3D12Device* device{};DXGI_SWAP_CHAIN_DESC desc{};
    if(FAILED(input->QueryInterface(IID_PPV_ARGS(&queue))))return;
    Binding record{};record.chain=identity(chain);record.queue=identity(queue);
    record.queue_type=queue->GetDesc().Type;record.method=method;record.caller=reinterpret_cast<uintptr_t>(caller);record.qpc=now();
    if(SUCCEEDED(queue->GetDevice(IID_PPV_ARGS(&device)))){record.device=identity(device);device->Release();}
    if(SUCCEEDED(chain->GetDesc(&desc))){record.window=reinterpret_cast<uintptr_t>(desc.OutputWindow);record.width=desc.BufferDesc.Width;record.height=desc.BufferDesc.Height;record.format=desc.BufferDesc.Format;}
    if((readback_enabled || gpu_share_enabled) && record.queue_type==0 && record.window && record.width>=640){
        DWORD owner{};GetWindowThreadProcessId(reinterpret_cast<HWND>(record.window),&owner);
        if(owner==GetCurrentProcessId()){
            if(readback_enabled)readback.bind(queue,chain);
            if(gpu_share_enabled)gpu_publisher.bind(queue,chain);
        }
    }
    queue->Release();
    if(!TryAcquireSRWLockExclusive(&lock)){++dropped;return;}
    if(binding_count<bindings.size())bindings[binding_count++]=record;else ++dropped;
    ReleaseSRWLockExclusive(&lock);
}
HRESULT STDMETHODCALLTYPE on_legacy(IDXGIFactory* factory,IUnknown* input,DXGI_SWAP_CHAIN_DESC* desc,IDXGISwapChain** out){
    Scope scope;HRESULT result=legacy(factory,input,desc,out);
    if(SUCCEEDED(result) && out)remember(1,input,*out,_ReturnAddress());return result;
}
HRESULT STDMETHODCALLTYPE on_hwnd(IDXGIFactory2* factory,IUnknown* input,HWND window,const DXGI_SWAP_CHAIN_DESC1* desc,const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* full,IDXGIOutput* restrict_output,IDXGISwapChain1** out){
    Scope scope;HRESULT result=hwnd_create(factory,input,window,desc,full,restrict_output,out);
    if(SUCCEEDED(result) && out)remember(2,input,*out,_ReturnAddress());return result;
}
HRESULT STDMETHODCALLTYPE on_composition(IDXGIFactory2* factory,IUnknown* input,const DXGI_SWAP_CHAIN_DESC1* desc,IDXGIOutput* restrict_output,IDXGISwapChain1** out){
    Scope scope;HRESULT result=composition(factory,input,desc,restrict_output,out);
    if(SUCCEEDED(result) && out)remember(3,input,*out,_ReturnAddress());return result;
}
HRESULT STDMETHODCALLTYPE on_present(IDXGISwapChain* chain,UINT interval,UINT flags){
    Scope scope;
    if(recording && !(flags&DXGI_PRESENT_TEST)){
        if(gpu_share_enabled)gpu_publisher.capture(chain);
        if(readback_enabled && (!manual_readback || readback_requested))readback.capture(chain,manual_readback?0:15000);
        uintptr_t id=identity(chain);DXGI_SWAP_CHAIN_DESC desc{};chain->GetDesc(&desc);
        if(TryAcquireSRWLockExclusive(&lock)){
            unsigned i=0;for(;i<seen_count && seen[i].chain!=id;++i){}
            if(i<seen_count)++seen[i].count;
            else if(seen_count<seen.size())seen[seen_count++]={id,reinterpret_cast<uintptr_t>(desc.OutputWindow),1};
            else ++dropped;
            ReleaseSRWLockExclusive(&lock);
        }else ++dropped;
    }
    HRESULT result=present(chain,interval,flags);if(FAILED(result))++present_failures;return result;
}
bool attach(void* target,void* detour,void** original){
    if(target_count>=targets.size())return false;
    Target& item=targets[target_count];item.address=target;
    SIZE_T bytes{};
    if(!ReadProcessMemory(GetCurrentProcess(),target,item.before,32,&bytes) || bytes!=32)return false;
    if(MH_CreateHook(target,detour,original)!=MH_OK || MH_QueueEnableHook(target)!=MH_OK)return false;
    ++target_count;return true;
}
bool setup(){
    IDXGIFactory2* factory{};ID3D12Device* device{};ID3D12CommandQueue* queue{};IDXGISwapChain1* chain{};
    bool ok=false;
    if(SUCCEEDED(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory)))){
        void** vtable=*reinterpret_cast<void***>(factory);
        ok=attach(vtable[10],reinterpret_cast<void*>(on_legacy),reinterpret_cast<void**>(&legacy)) &&
            attach(vtable[15],reinterpret_cast<void*>(on_hwnd),reinterpret_cast<void**>(&hwnd_create)) &&
            attach(vtable[24],reinterpret_cast<void*>(on_composition),reinterpret_cast<void**>(&composition));
        D3D12_COMMAND_QUEUE_DESC queue_desc{};
        if(ok && SUCCEEDED(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))) &&
           SUCCEEDED(device->CreateCommandQueue(&queue_desc,IID_PPV_ARGS(&queue)))){
            DXGI_SWAP_CHAIN_DESC1 desc{};desc.Width=16;desc.Height=16;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count=1;desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.BufferCount=2;
            desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;desc.AlphaMode=DXGI_ALPHA_MODE_PREMULTIPLIED;
            ok=SUCCEEDED(factory->CreateSwapChainForComposition(queue,&desc,nullptr,&chain));
            if(ok)ok=attach((*reinterpret_cast<void***>(chain))[8],reinterpret_cast<void*>(on_present),reinterpret_cast<void**>(&present));
        }else ok=false;
    }
    if(chain)chain->Release();if(queue)queue->Release();if(device)device->Release();if(factory)factory->Release();
    if(ok){recording=true;ok=MH_ApplyQueued()==MH_OK;}
    if(!ok){MH_DisableHook(MH_ALL_HOOKS);recording=false;}return ok;
}
bool valid_game(){
    wchar_t path[32768];GetModuleFileNameW(nullptr,path,32768);
    const wchar_t* name=wcsrchr(path,L'\\');if(!name || _wcsicmp(name+1,L"GOWDE-Steam.exe"))return false;
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);if(file==INVALID_HANDLE_VALUE)return false;
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};
    bool ok=BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0 && BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
    std::array<unsigned char,65536> bytes{};DWORD count{};
    while(ok){if(!ReadFile(file,bytes.data(),static_cast<DWORD>(bytes.size()),&count,nullptr)){ok=false;break;}if(!count)break;ok=BCryptHashData(hash,bytes.data(),count,0)>=0;}
    unsigned char digest[32]{};char hex[65]{};
    if(ok)ok=BCryptFinishHash(hash,digest,32,0)>=0;
    for(unsigned i=0;i<32;++i)sprintf_s(hex+i*2,3,"%02x",digest[i]);
    if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);CloseHandle(file);
    return ok && !strcmp(hex,"bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4");
}
bool stop(){
    bool ok=MH_DisableHook(MH_ALL_HOOKS)==MH_OK;recording=false;
    for(unsigned i=0;i<500 && active;++i)Sleep(10);
    return ok && active==0;
}
DWORD WINAPI worker(void*){
    wchar_t path[MAX_PATH],ini[MAX_PATH];GetModuleFileNameW(own,path,MAX_PATH);*wcsrchr(path,L'\\')=0;
    swprintf_s(ini,L"%s\\binding.ini",path);unsigned seconds=GetPrivateProfileIntW(L"Binding",L"seconds",60,ini);
    readback_enabled=GetPrivateProfileIntW(L"Binding",L"readback",0,ini)==1;
    manual_readback=GetPrivateProfileIntW(L"Binding",L"manual",0,ini)==1;
    gpu_share_enabled=GetPrivateProfileIntW(L"Binding",L"share",0,ini)==1;
    if(gpu_share_enabled && readback_enabled)return 14;
    if(seconds<10 || seconds>180 || !valid_game())return 10;
    wchar_t output[MAX_PATH];swprintf_s(output,L"%s\\binding-%lu.jsonl",path,GetCurrentProcessId());
    FILE* file=_wfsopen(output,L"w",_SH_DENYNO);if(!file)return 11;
    HANDLE request_event{};
    if(readback_enabled && manual_readback){
        wchar_t name[100];swprintf_s(name,L"Local\\GearsGpuReadback-%lu",GetCurrentProcessId());
        request_event=CreateEventW(nullptr,FALSE,FALSE,name);
        if(!request_event){fclose(file);return 13;}
    }
    bool ok=MH_Initialize()==MH_OK && setup();
    fprintf(file,"{\"event\":\"ready\",\"pid\":%lu,\"hooks\":%u,\"ok\":%s}\n",GetCurrentProcessId(),target_count,ok?"true":"false");fflush(file);
    wchar_t image[MAX_PATH];swprintf_s(image,L"%s\\gpu-%lu.ppm",path,GetCurrentProcessId());bool saved=false;
    if(ok){ULONGLONG deadline=GetTickCount64()+seconds*1000ull;while(GetTickCount64()<deadline && !(GetAsyncKeyState(VK_F8)&0x8000)){
        if(request_event && WaitForSingleObject(request_event,0)==WAIT_OBJECT_0)readback_requested=true;
        if(gpu_share_enabled && gpu_publisher.state==1)gpu_publisher.prepare();
        if(gpu_share_enabled)gpu_publisher.poll();
        if(readback_enabled && readback.state==1)readback.prepare();
        if(readback_enabled && !saved && readback.state==6)saved=readback.save(image);
        Sleep(20);
    }}
    bool restored=ok && stop();
    if(request_event)CloseHandle(request_event);
    // A callback that has not returned can still access the capture objects.
    // Keep them resident and fail closed instead of inspecting/freeing them.
    if(active){
        fprintf(file,"{\"event\":\"stopped\",\"restored\":false,\"active\":%u,\"dropped\":%u,\"present_failures\":%u}\n",active.load(),dropped.load(),present_failures.load());
        fclose(file);return 12;
    }
    if(readback_enabled){
        ULONGLONG deadline=GetTickCount64()+5000;
        while(readback.state==6 && !saved && GetTickCount64()<deadline){saved=readback.save(image);if(!saved)Sleep(10);}
        fprintf(file,"{\"event\":\"readback\",\"state\":%u,\"saved\":%s,\"width\":%u,\"height\":%u,\"format\":%u,\"buffer_index\":%u,\"hr\":\"0x%08lx\"}\n",readback.state.load(),saved?"true":"false",readback.width,readback.height,readback.format,readback.index,static_cast<unsigned long>(readback.failure));
    }
    bool released=readback.release();
    if(gpu_share_enabled){
        // close() retains possibly in-flight resources if the fence has not
        // completed. Give the final submission time to finish before closing.
        gpu_publisher.drain(5000);
        auto share_state=gpu_publisher.state.load();auto share_error=gpu_publisher.failure();
        bool share_released=gpu_publisher.close();released=released && share_released;
        fprintf(file,"{\"event\":\"gpu_share\",\"state\":%u,\"published\":%u,\"busy_skips\":%u,\"hr\":\"0x%08lx\",\"resources_released\":%s}\n",share_state,gpu_publisher.frames.load(),gpu_publisher.busy.load(),static_cast<unsigned long>(share_error),share_released?"true":"false");
    }
    fprintf(file,"{\"event\":\"resources\",\"released\":%s}\n",released?"true":"false");
    AcquireSRWLockExclusive(&lock);
    for(unsigned i=0;i<binding_count;++i){const auto& b=bindings[i];
        fprintf(file,"{\"event\":\"binding\",\"method\":%u,\"chain\":\"0x%llx\",\"queue\":\"0x%llx\",\"device\":\"0x%llx\",\"window\":\"0x%llx\",\"caller\":\"0x%llx\",\"queue_type\":%u,\"width\":%u,\"height\":%u,\"format\":%u,\"qpc\":%lld}\n",b.method,b.chain,b.queue,b.device,b.window,b.caller,b.queue_type,b.width,b.height,b.format,b.qpc);}
    for(unsigned i=0;i<seen_count;++i)fprintf(file,"{\"event\":\"present\",\"chain\":\"0x%llx\",\"window\":\"0x%llx\",\"count\":%u}\n",seen[i].chain,seen[i].window,seen[i].count);
    ReleaseSRWLockExclusive(&lock);
    for(unsigned i=0;i<target_count;++i){unsigned char bytes[32]{};SIZE_T read{};bool equal=ReadProcessMemory(GetCurrentProcess(),targets[i].address,bytes,32,&read) && read==32 && !memcmp(bytes,targets[i].before,32);
        fprintf(file,"{\"event\":\"entry\",\"target\":\"0x%llx\",\"restored\":%s,\"before\":\"",reinterpret_cast<uintptr_t>(targets[i].address),equal?"true":"false");for(unsigned j=0;j<32;++j)fprintf(file,"%02x",targets[i].before[j]);fprintf(file,"\"}\n");restored=restored && equal;}
    fprintf(file,"{\"event\":\"stopped\",\"restored\":%s,\"active\":%u,\"dropped\":%u,\"present_failures\":%u}\n",restored?"true":"false",active.load(),dropped.load(),present_failures.load());fclose(file);
    return restored?0:12;
}
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsBindingStart(void*){
    if(running.exchange(1))return 1;
    HANDLE thread=CreateThread(nullptr,0,worker,nullptr,0,nullptr);if(!thread)return 2;CloseHandle(thread);return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsBindingSelfTest(void*){
    if(MH_Initialize()!=MH_OK || !setup())return 20;
    IDXGIFactory2* factory{};ID3D12Device* device{};ID3D12CommandQueue* queue{};IDXGISwapChain1* chain{};
    HWND window=CreateWindowExW(0,L"STATIC",L"Binding self-test",WS_OVERLAPPEDWINDOW,0,0,32,32,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    DWORD result=0;D3D12_COMMAND_QUEUE_DESC q{};
    if(!window || FAILED(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory))) || FAILED(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))) || FAILED(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue))))result=21;
    DXGI_SWAP_CHAIN_DESC1 desc{};desc.Width=16;desc.Height=16;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.BufferCount=2;desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;
    if(!result && FAILED(factory->CreateSwapChainForHwnd(queue,window,&desc,nullptr,nullptr,&chain)))result=22;
    if(!result && (binding_count!=1 || bindings[0].method!=2 || bindings[0].chain!=identity(chain) || bindings[0].queue!=identity(queue) || bindings[0].window!=reinterpret_cast<uintptr_t>(window) || bindings[0].queue_type!=0))result=23;
    if(chain){chain->Release();chain=nullptr;}
    IDXGISwapChain* old_chain{};
    DXGI_SWAP_CHAIN_DESC old_desc{};old_desc.BufferDesc.Width=16;old_desc.BufferDesc.Height=16;old_desc.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    old_desc.SampleDesc.Count=1;old_desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;old_desc.BufferCount=2;old_desc.OutputWindow=window;old_desc.Windowed=TRUE;old_desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;
    if(!result && FAILED(factory->CreateSwapChain(queue,&old_desc,&old_chain)))result=26;
    bool found_legacy=false;
    for(unsigned i=0;i<binding_count;++i)if(bindings[i].method==1 && bindings[i].chain==identity(old_chain) && bindings[i].queue==identity(queue))found_legacy=true;
    if(!result && !found_legacy)result=27;
    if(old_chain){old_chain->Release();old_chain=nullptr;}
    desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;desc.AlphaMode=DXGI_ALPHA_MODE_PREMULTIPLIED;
    if(!result && FAILED(factory->CreateSwapChainForComposition(queue,&desc,nullptr,&chain)))result=28;
    bool found_composition=false;
    for(unsigned i=0;i<binding_count;++i)if(bindings[i].method==3 && bindings[i].chain==identity(chain) && bindings[i].queue==identity(queue))found_composition=true;
    if(!result && !found_composition)result=29;
    if(!stop() && !result)result=24;
    for(unsigned i=0;i<target_count && !result;++i)if(memcmp(targets[i].address,targets[i].before,32))result=25;
    if(chain)chain->Release();if(queue)queue->Release();if(device)device->Release();if(factory)factory->Release();if(window)DestroyWindow(window);return result;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsReadbackSelfTest(void*){
    // Actual GPU clear/copy/fence/map for both the game's packed 10-bit format
    // and the 8-bit test format. Verify every pixel, including row padding.
    IDXGIFactory2* factory{};ID3D12Device* device{};ID3D12CommandQueue* queue{};
    D3D12_COMMAND_QUEUE_DESC q{};
    if(FAILED(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory))) || FAILED(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))) || FAILED(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue))))return 30;
    DWORD result=0;
    for(DXGI_FORMAT format:{DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R10G10B10A2_UNORM}){
        IDXGISwapChain1* chain{};ID3D12Resource* resource{};ID3D12DescriptorHeap* heap{};ID3D12CommandAllocator* allocator{};ID3D12GraphicsCommandList* commands{};
        OneShotReadback shot;
        DXGI_SWAP_CHAIN_DESC1 desc{};desc.Width=19;desc.Height=13;desc.Format=format;desc.SampleDesc.Count=1;desc.BufferCount=2;
        desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;desc.AlphaMode=DXGI_ALPHA_MODE_PREMULTIPLIED;
        D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};heap_desc.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;heap_desc.NumDescriptors=1;
        if(FAILED(factory->CreateSwapChainForComposition(queue,&desc,nullptr,&chain)) || !shot.bind(queue,chain) || !shot.prepare() ||
            FAILED(chain->GetBuffer(0,IID_PPV_ARGS(&resource))) || FAILED(device->CreateDescriptorHeap(&heap_desc,IID_PPV_ARGS(&heap))) ||
            FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))) || FAILED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator,nullptr,IID_PPV_ARGS(&commands))))result=31;
        if(!result){
            device->CreateRenderTargetView(resource,nullptr,heap->GetCPUDescriptorHandleForHeapStart());
            D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition.pResource=resource;
            barrier.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_PRESENT;barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_RENDER_TARGET;
            commands->ResourceBarrier(1,&barrier);float color[]={.25f,.5f,.75f,1.f};commands->ClearRenderTargetView(heap->GetCPUDescriptorHandleForHeapStart(),color,0,nullptr);
            barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_RENDER_TARGET;barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_PRESENT;commands->ResourceBarrier(1,&barrier);
            if(FAILED(commands->Close()))result=32;
            else{ID3D12CommandList* lists[]={commands};queue->ExecuteCommandLists(1,lists);shot.capture(chain,0);}
            std::vector<unsigned char> pixels;bool mapped=false;ULONGLONG deadline=GetTickCount64()+5000;
            while(!result && !mapped && GetTickCount64()<deadline){mapped=shot.pixels(pixels,19,13);if(!mapped)Sleep(1);}
            if(!mapped || pixels.size()!=19*13*3)result=33;
            if(!result)for(size_t i=0;i<pixels.size();i+=3)if(abs(int(pixels[i])-64)>1 || abs(int(pixels[i+1])-128)>1 || abs(int(pixels[i+2])-191)>1){result=34;break;}
        }
        bool released=shot.release();if(!released && !result)result=35;
        // If the GPU did not complete, retain the test objects until process
        // exit too; never release command resources known to be in flight.
        if(released){if(commands)commands->Release();if(allocator)allocator->Release();if(heap)heap->Release();if(resource)resource->Release();if(chain)chain->Release();}
        if(result)break;
    }
    if(queue)queue->Release();if(device)device->Release();if(factory)factory->Release();return result;
}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,void*){if(reason==DLL_PROCESS_ATTACH){own=instance;DisableThreadLibraryCalls(instance);}return TRUE;}
