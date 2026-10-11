#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <bcrypt.h>
#include <intrin.h>
#include <array>
#include <atomic>
#include <cstdio>
#include <cwchar>
#include <cstring>
#include "MinHook.h"

namespace {
struct Vec {float x,y,z;};
struct Rot {int pitch,yaw,roll;};
using SceneFn=void*(__fastcall*)(void*,void*,Vec*,Rot*,void*,void*);
using EventFn=void(__fastcall*)(void*,Vec*,Rot*);
using PresentFn=HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*,UINT,UINT);
using Present1Fn=HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain1*,UINT,UINT,const DXGI_PRESENT_PARAMETERS*);
SceneFn original_scene{}; EventFn original_event{};
PresentFn original_present{}; Present1Fn original_present1{};
HMODULE own_module{};
uintptr_t base{},expected_local{},expected_pc{},camera_cache{};
std::atomic<unsigned> active{0},started{0};
std::atomic<unsigned long long> dropped{0};
std::atomic<bool> recording{false};
SRWLOCK queue_lock=SRWLOCK_INIT;
struct Record {
    unsigned kind,thread,focused,valid;
    long long time,duration;
    uintptr_t object,result,caller;
    Vec position; Rot rotation; float camera_time;
    std::array<float,32> matrices;
};
std::array<Record,8192> ring{};
size_t read_index{},write_index{};
unsigned long long emitted[5]{};
void* scene_target{}; void* event_target{}; void* present_target{}; void* present1_target{};
HANDLE stop_event{};
wchar_t directory[MAX_PATH]{};
bool safe_copy(void* out,const void* from,size_t bytes) {
    SIZE_T count{};
    return from && ReadProcessMemory(GetCurrentProcess(),from,out,bytes,&count) && count==bytes;
}
long long now() {LARGE_INTEGER value;QueryPerformanceCounter(&value);return value.QuadPart;}
unsigned foreground() {
    DWORD pid{};GetWindowThreadProcessId(GetForegroundWindow(),&pid);return pid==GetCurrentProcessId();
}
struct Scope {Scope(){++active;} ~Scope(){--active;}};
Record start_record(unsigned kind,void* object,void* caller) {
    Record r{};r.kind=kind;r.thread=GetCurrentThreadId();r.time=now();r.focused=foreground();
    r.object=reinterpret_cast<uintptr_t>(object);r.caller=reinterpret_cast<uintptr_t>(caller)-base;
    return r;
}
void enqueue(const Record& value) {
    if(!recording.load(std::memory_order_relaxed)) return;
    if(!TryAcquireSRWLockExclusive(&queue_lock)) {++dropped;return;}
    if(write_index-read_index<ring.size()) ring[write_index++%ring.size()]=value;
    else ++dropped;
    ReleaseSRWLockExclusive(&queue_lock);
}
void* __fastcall hook_scene(void* local,void* family,Vec* position,Rot* rotation,void* viewport,void* drawer) {
    Scope scope;auto record=start_record(1,local,_ReturnAddress());
    void* result=original_scene(local,family,position,rotation,viewport,drawer);
    record.duration=now()-record.time;record.result=reinterpret_cast<uintptr_t>(result);
    if(safe_copy(&record.position,position,sizeof(Vec))) record.valid|=1;
    if(safe_copy(&record.rotation,rotation,sizeof(Rot))) record.valid|=2;
    if(result && safe_copy(record.matrices.data(),reinterpret_cast<char*>(result)+0x70,128)) record.valid|=4;
    if(camera_cache && safe_copy(&record.camera_time,reinterpret_cast<void*>(camera_cache),4)) record.valid|=8;
    if(record.object==expected_local) record.valid|=16;
    enqueue(record);return result;
}
void __fastcall hook_event(void* pc,Vec* position,Rot* rotation) {
    Scope scope;auto record=start_record(2,pc,_ReturnAddress());
    original_event(pc,position,rotation);
    record.duration=now()-record.time;
    if(safe_copy(&record.position,position,sizeof(Vec))) record.valid|=1;
    if(safe_copy(&record.rotation,rotation,sizeof(Rot))) record.valid|=2;
    if(record.object==expected_pc) record.valid|=16;
    enqueue(record);
}
HRESULT STDMETHODCALLTYPE hook_present(IDXGISwapChain* chain,UINT interval,UINT flags) {
    Scope scope;auto record=start_record(3,chain,_ReturnAddress());
    HRESULT result=original_present(chain,interval,flags);
    record.duration=now()-record.time;record.result=static_cast<unsigned long>(result);
    if(!(flags&DXGI_PRESENT_TEST)) record.valid|=32;
    enqueue(record);return result;
}
HRESULT STDMETHODCALLTYPE hook_present1(IDXGISwapChain1* chain,UINT interval,UINT flags,const DXGI_PRESENT_PARAMETERS* params) {
    Scope scope;auto record=start_record(4,chain,_ReturnAddress());
    HRESULT result=original_present1(chain,interval,flags,params);
    record.duration=now()-record.time;record.result=static_cast<unsigned long>(result);
    if(!(flags&DXGI_PRESENT_TEST)) record.valid|=32;
    enqueue(record);return result;
}
bool hash_matches() {
    wchar_t path[MAX_PATH];GetModuleFileNameW(nullptr,path,MAX_PATH);
    const wchar_t* name=wcsrchr(path,L'\\');
    if(!name || _wcsicmp(name+1,L"GOWDE-Steam.exe")) return false;
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
    if(file==INVALID_HANDLE_VALUE) return false;
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};
    bool ok=BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(ok) ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
    std::array<unsigned char,65536> buffer;DWORD bytes{};
    if(ok) {
        for(;;) {
            if(!ReadFile(file,buffer.data(),static_cast<DWORD>(buffer.size()),&bytes,nullptr)) {ok=false;break;}
            if(!bytes) break;
            if(BCryptHashData(hash,buffer.data(),bytes,0)<0) {ok=false;break;}
        }
    }
    unsigned char digest[32]{};char hex[65]{};
    if(ok) ok=BCryptFinishHash(hash,digest,32,0)>=0;
    for(unsigned i=0;i<32;++i) sprintf_s(hex+2*i,3,"%02x",digest[i]);
    if(hash) BCryptDestroyHash(hash);
    if(algorithm) BCryptCloseAlgorithmProvider(algorithm,0);
    CloseHandle(file);
    return ok && !strcmp(hex,"bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4");
}
void path_for(wchar_t* result,const wchar_t* filename) {swprintf_s(result,MAX_PATH,L"%s\\%s",directory,filename);}
uintptr_t ini_address(const wchar_t* ini,const wchar_t* key) {
    wchar_t value[64];GetPrivateProfileStringW(L"Capture",key,L"0",value,64,ini);
    return _wcstoui64(value,nullptr,0);
}
void flush(FILE* file) {
    std::array<Record,128> batch;
    for(;;) {
        AcquireSRWLockExclusive(&queue_lock);
        size_t count=0;
        while(read_index<write_index && count<batch.size()) batch[count++]=ring[read_index++%ring.size()];
        ReleaseSRWLockExclusive(&queue_lock);
        if(!count) break;
        for(size_t i=0;i<count;++i) {
            const auto& r=batch[i];++emitted[r.kind];
            fprintf(file,"%u,%u,%u,%u,%lld,%lld,0x%llx,0x%llx,0x%llx,%.9g,%.9g,%.9g,%d,%d,%d,%.9g",
                r.kind,r.thread,r.focused,r.valid,r.time,r.duration,
                static_cast<unsigned long long>(r.object),static_cast<unsigned long long>(r.result),
                static_cast<unsigned long long>(r.caller),r.position.x,r.position.y,r.position.z,
                r.rotation.pitch,r.rotation.yaw,r.rotation.roll,r.camera_time);
            for(float value:r.matrices) fprintf(file,",%.9g",value);
            fputc('\n',file);
        }
        fflush(file);
    }
}
bool install(void* target,void* detour,void** original) {
    return MH_CreateHook(target,detour,original)==MH_OK && MH_QueueEnableHook(target)==MH_OK;
}
bool present_hooks() {
    IDXGIFactory2* factory{};ID3D12Device* device{};ID3D12CommandQueue* queue{};IDXGISwapChain1* chain{};
    bool success=false;
    if(SUCCEEDED(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory))) &&
       SUCCEEDED(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)))) {
        D3D12_COMMAND_QUEUE_DESC q{};
        if(SUCCEEDED(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)))) {
            DXGI_SWAP_CHAIN_DESC1 desc{};desc.Width=16;desc.Height=16;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count=1;desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.BufferCount=2;
            desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;desc.AlphaMode=DXGI_ALPHA_MODE_PREMULTIPLIED;
            if(SUCCEEDED(factory->CreateSwapChainForComposition(queue,&desc,nullptr,&chain))) {
                void** table=*reinterpret_cast<void***>(chain);
                present_target=table[8];present1_target=table[22];
                success=install(present_target,reinterpret_cast<void*>(hook_present),reinterpret_cast<void**>(&original_present));
                if(present1_target!=present_target)
                    success=install(present1_target,reinterpret_cast<void*>(hook_present1),reinterpret_cast<void**>(&original_present1)) && success;
            }
        }
    }
    if(chain) chain->Release();if(queue) queue->Release();if(device) device->Release();if(factory) factory->Release();
    return success;
}
DWORD WINAPI worker(void*) {
    wchar_t ini[MAX_PATH],csv[MAX_PATH],status_path[MAX_PATH];
    path_for(ini,L"capture.ini");path_for(csv,L"capture.csv");path_for(status_path,L"capture-status.txt");
    FILE* status{};_wfopen_s(&status,status_path,L"w");
    if(!status) return 1;
    if(!hash_matches() || ini_address(ini,L"pid")!=GetCurrentProcessId()) {
        fprintf(status,"rejected: executable hash/name or PID mismatch\n");fclose(status);return 2;
    }
    base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    expected_local=ini_address(ini,L"local_player");expected_pc=ini_address(ini,L"controller");
    camera_cache=ini_address(ini,L"timestamp");
    uintptr_t linked{};
    if(!expected_local || !expected_pc || !safe_copy(&linked,reinterpret_cast<void*>(expected_local+0x68),8) || linked!=expected_pc) {
        fprintf(status,"rejected: local player/controller link mismatch\n");fclose(status);return 3;
    }
    scene_target=reinterpret_cast<void*>(base+0x15cf08);event_target=reinterpret_cast<void*>(base+0x15c968);
    const unsigned char scene_bytes[]={0x48,0x8b,0xc4,0x53,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57};
    const unsigned char event_bytes[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x08,0x48,0x89,0x68,0x10,0x48,0x89,0x70,0x18};
    unsigned char bytes[16]{};
    if(!safe_copy(bytes,scene_target,sizeof(scene_bytes)) || memcmp(bytes,scene_bytes,sizeof(scene_bytes)) ||
       !safe_copy(bytes,event_target,sizeof(event_bytes)) || memcmp(bytes,event_bytes,sizeof(event_bytes))) {
        fprintf(status,"rejected: candidate prologue mismatch\n");fclose(status);return 4;
    }
    FILE* file{};_wfopen_s(&file,csv,L"w");
    if(!file) {fclose(status);return 5;}
    fprintf(file,"kind,thread,focused,valid,qpc,duration_qpc,object,result,caller_rva,x,y,z,pitch,yaw,roll,camera_timestamp");
    for(unsigned i=0;i<32;++i) fprintf(file,",matrix%u",i);fputc('\n',file);fflush(file);
    LARGE_INTEGER frequency;QueryPerformanceFrequency(&frequency);
    fprintf(status,"qpc_frequency=%lld\n",frequency.QuadPart);
    MH_STATUS init=MH_Initialize();
    bool ok=init==MH_OK && install(scene_target,reinterpret_cast<void*>(hook_scene),reinterpret_cast<void**>(&original_scene))
        && install(event_target,reinterpret_cast<void*>(hook_event),reinterpret_cast<void**>(&original_event));
    bool graphics=ok && present_hooks();
    fprintf(status,"present_hooks_created=%d\n",graphics);
    if(ok) {recording=true;ok=MH_ApplyQueued()==MH_OK;}
    if(!ok) {
        recording=false;MH_DisableHook(MH_ALL_HOOKS);
        fprintf(status,"hook installation failed\n");fclose(file);fclose(status);return 6;
    }
    wchar_t name[128];swprintf_s(name,L"Local\\GearsVR.Capture.Stop.%lu",GetCurrentProcessId());
    stop_event=CreateEventW(nullptr,TRUE,FALSE,name);
    const unsigned seconds=GetPrivateProfileIntW(L"Capture",L"seconds",90,ini);
    fprintf(status,"capturing pid=%lu seconds=%u\n",GetCurrentProcessId(),seconds);fflush(status);
    const ULONGLONG deadline=GetTickCount64()+static_cast<ULONGLONG>(seconds)*1000;
    while(GetTickCount64()<deadline && (!stop_event || WaitForSingleObject(stop_event,20)!=WAIT_OBJECT_0)) {flush(file);if(!stop_event) Sleep(20);}
    // Disable entry patches; leave the DLL and trampolines resident so delayed
    // in-flight execution can never jump into freed code.
    MH_STATUS disabled=MH_DisableHook(MH_ALL_HOOKS);
    recording=false;
    for(unsigned i=0;i<100 && active.load();++i) Sleep(10);
    flush(file);
    fprintf(status,"stopped disable_status=%d active=%u dropped=%llu scene=%llu event=%llu present=%llu present1=%llu\n",
        disabled,active.load(),dropped.load(),emitted[1],emitted[2],emitted[3],emitted[4]);
    fclose(file);fclose(status);if(stop_event) CloseHandle(stop_event);stop_event=nullptr;
    return disabled==MH_OK?0:7;
}
// Test real trampoline execution and six-argument forwarding before injection.
__declspec(noinline) void fake_event(void* pc,Vec* p,Rot* r) {p->x+=static_cast<float>(reinterpret_cast<uintptr_t>(pc));r->yaw+=7;}
struct FakeView {char prefix[0x70];float matrices[32];};
FakeView fake_view{};
bool arguments_good{};
__declspec(noinline) void* fake_scene(void* lp,void* family,Vec* p,Rot* r,void* vp,void* drawer) {
    arguments_good=lp==reinterpret_cast<void*>(1) && family==reinterpret_cast<void*>(2) && vp==reinterpret_cast<void*>(3) && drawer==reinterpret_cast<void*>(4);
    fake_event(reinterpret_cast<void*>(5),p,r);
    for(int i=0;i<32;++i) fake_view.matrices[i]=static_cast<float>(i+10);
    return &fake_view;
}
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureStart(void*) {
    if(started.exchange(1)) return ERROR_ALREADY_EXISTS;
    wchar_t path[MAX_PATH];GetModuleFileNameW(own_module,path,MAX_PATH);
    wchar_t* slash=wcsrchr(path,L'\\');if(!slash) return ERROR_BAD_PATHNAME;*slash=0;wcscpy_s(directory,path);
    HANDLE thread=CreateThread(nullptr,0,worker,nullptr,0,nullptr);
    if(!thread) return GetLastError();CloseHandle(thread);return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureSelfTest(void*) {
    base=0;expected_local=1;expected_pc=5;
    unsigned char scene_before[32],event_before[32];
    if(!safe_copy(scene_before,reinterpret_cast<void*>(fake_scene),32) ||
       !safe_copy(event_before,reinterpret_cast<void*>(fake_event),32)) return 9;
    if(MH_Initialize()!=MH_OK) return 10;
    if(!install(reinterpret_cast<void*>(fake_scene),reinterpret_cast<void*>(hook_scene),reinterpret_cast<void**>(&original_scene)) ||
       !install(reinterpret_cast<void*>(fake_event),reinterpret_cast<void*>(hook_event),reinterpret_cast<void**>(&original_event))) return 11;
    recording=true;if(MH_ApplyQueued()!=MH_OK) return 12;
    Vec p{1,2,3};Rot rot{4,5,6};
    auto invoke=reinterpret_cast<SceneFn>(reinterpret_cast<void*>(fake_scene));
    void* result=invoke(reinterpret_cast<void*>(1),reinterpret_cast<void*>(2),&p,&rot,reinterpret_cast<void*>(3),reinterpret_cast<void*>(4));
    recording=false;if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK) return 13;
    if(!arguments_good || result!=&fake_view || p.x!=6 || p.y!=2 || p.z!=3 || rot.yaw!=12) return 14;
    if(write_index!=2 || dropped.load()!=0 || ring[0].kind!=2 || ring[1].kind!=1 || ring[1].valid!=23) return 15;
    for(int i=0;i<32;++i) if(ring[1].matrices[i]!=i+10) return 16;
    unsigned char after[32];
    if(!safe_copy(after,reinterpret_cast<void*>(fake_scene),32) || memcmp(after,scene_before,32) ||
       !safe_copy(after,reinterpret_cast<void*>(fake_event),32) || memcmp(after,event_before,32)) return 17;
    Vec p2{1,2,3};Rot r2{4,5,6};
    if(invoke(reinterpret_cast<void*>(1),reinterpret_cast<void*>(2),&p2,&r2,reinterpret_cast<void*>(3),reinterpret_cast<void*>(4))!=&fake_view ||
       p2.x!=6 || r2.yaw!=12 || write_index!=2) return 18;
    return 0;
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,void*) {
    // Keep CRT thread notifications enabled for the statically linked runtime.
    if(reason==DLL_PROCESS_ATTACH) own_module=module;
    return TRUE;
}
