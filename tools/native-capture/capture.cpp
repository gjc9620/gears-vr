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
#include <cmath>
#include "MinHook.h"
#include "../vr-common/pose_bridge.h"

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
uintptr_t expected_camera{},camera_yaw{};
bool wobble_enabled{},self_testing{};
bool sweep_enabled{};
bool live_enabled{};
gears_vr::PoseBridge live_bridge;
gears_vr::PoseGate live_gate;
SRWLOCK live_lock=SRWLOCK_INIT;
thread_local unsigned live_status{};
thread_local unsigned long long live_sequence{};
std::atomic<bool> live_aborted{false};
double selftest_elapsed=4;
uintptr_t allowed_caller=0x15d0ca;
long long qpc_frequency{};
std::atomic<long long> wobble_start{0};
std::atomic<unsigned> wobble_state{0}; // 0 waiting, 1 running, 2 complete, 3 aborted
thread_local void* scene_local{};
thread_local Rot* scene_rotation{};
thread_local int scene_delta{};
thread_local unsigned scene_phase{};
thread_local Vec scene_translation{};
thread_local Rot scene_rotation_delta{};
thread_local float observed_stamp{};
thread_local bool stamp_known{};
std::atomic<unsigned> active{0},started{0};
std::atomic<unsigned long long> dropped{0};
std::atomic<bool> recording{false};
SRWLOCK queue_lock=SRWLOCK_INIT;
struct Record {
    unsigned kind,thread,focused,valid;
    long long time,duration;
    uintptr_t object,result,caller;
    Vec position; Rot rotation; float camera_time;
    int source_yaw,yaw_delta,cache_yaw;
    unsigned test_state;
    unsigned pose_phase;
    unsigned pose_status;
    unsigned long long pose_sequence;
    Vec source_position,translation_delta;
    Rot source_rotation,rotation_delta;
    std::array<float,32> matrices;
};
std::array<Record,8192> ring{};
size_t read_index{},write_index{};
unsigned long long emitted[5]{};
void* scene_target{}; void* event_target{}; void* present_target{}; void* present1_target{};
HANDLE stop_event{};
wchar_t directory[MAX_PATH]{};
FILE* diagnostic_status{};
bool safe_copy(void* out,const void* from,size_t bytes) {
    SIZE_T count{};
    return from && ReadProcessMemory(GetCurrentProcess(),from,out,bytes,&count) && count==bytes;
}
long long now() {LARGE_INTEGER value;QueryPerformanceCounter(&value);return value.QuadPart;}
unsigned foreground() {
    DWORD pid{};GetWindowThreadProcessId(GetForegroundWindow(),&pid);return pid==GetCurrentProcessId();
}
struct Scope {Scope(){++active;} ~Scope(){--active;}};
int offset_at(double seconds) {
    if(seconds<3 || seconds>=15) return 0;
    return static_cast<int>(std::lround((1.5*65536.0/360.0)*std::sin((seconds-3)*6.283185307179586/4.0)));
}
void apply_wobble(Record& record,void* pc,Rot* rotation) {
    Rot raw{};
    if(!safe_copy(&raw,rotation,sizeof(raw))) return;
    record.source_yaw=raw.yaw;
    if(!wobble_enabled || !recording.load() || reinterpret_cast<uintptr_t>(pc)!=expected_pc ||
       reinterpret_cast<uintptr_t>(scene_local)!=expected_local || scene_rotation!=rotation || record.caller!=allowed_caller) return;
    unsigned state=wobble_state.load();
    if(state>=2) {record.test_state=state;return;}
    float stamp{};
    bool readable=camera_cache && safe_copy(&stamp,reinterpret_cast<void*>(camera_cache),4);
    bool advancing=readable && stamp_known && stamp!=observed_stamp;
    if(readable) {observed_stamp=stamp;stamp_known=true;}
    if(!self_testing && !record.focused) {
        if(state==1) wobble_state=3;
        record.test_state=wobble_state.load();return;
    }
    if(!self_testing) {
        uintptr_t linked{};
        if(!safe_copy(&linked,reinterpret_cast<void*>(expected_local+0x68),8) || linked!=expected_pc ||
           !safe_copy(&linked,reinterpret_cast<void*>(expected_pc+0x37c),8) || linked!=expected_camera) {
            wobble_state=3;record.test_state=3;return;
        }
    }
    long long origin=wobble_start.load();
    if(!origin && (self_testing || advancing)) {
        long long zero=0;
        wobble_start.compare_exchange_strong(zero,record.time);
        origin=wobble_start.load();wobble_state=1;
    }
    if(!origin) return;
    double elapsed=self_testing?4.0:static_cast<double>(record.time-origin)/qpc_frequency;
    if(elapsed>=15) {wobble_state=2;record.test_state=2;return;}
    int delta=offset_at(elapsed);
    if(delta) {
        unsigned bits=static_cast<unsigned>(raw.yaw)+static_cast<unsigned>(delta);
        int yaw{};memcpy(&yaw,&bits,4);
        SIZE_T written{};
        if(WriteProcessMemory(GetCurrentProcess(),reinterpret_cast<char*>(rotation)+4,&yaw,4,&written) && written==4) {
            record.yaw_delta=delta;record.valid|=64;scene_delta=delta;
        } else wobble_state=3;
    }
    record.test_state=wobble_state.load();
}
void apply_sweep(Record& record,void* pc,Vec* position,Rot* rotation) {
    Vec source_position{};Rot source_rotation{};
    if(!safe_copy(&source_position,position,sizeof(Vec)) || !safe_copy(&source_rotation,rotation,sizeof(Rot))) return;
    record.source_position=source_position;record.source_rotation=source_rotation;
    if(!sweep_enabled || !recording.load() || reinterpret_cast<uintptr_t>(pc)!=expected_pc ||
       reinterpret_cast<uintptr_t>(scene_local)!=expected_local || scene_rotation!=rotation || record.caller!=allowed_caller) return;
    unsigned state=wobble_state.load();
    if(state>=2) {record.test_state=state;record.pose_phase=state==2?7:0;scene_phase=record.pose_phase;return;}
    float stamp{};
    bool readable=camera_cache && safe_copy(&stamp,reinterpret_cast<void*>(camera_cache),4);
    bool advancing=readable && stamp_known && stamp!=observed_stamp;
    if(readable) {observed_stamp=stamp;stamp_known=true;}
    if(!self_testing && !record.focused) {
        if(state==1)wobble_state=3;
        record.test_state=wobble_state.load();return;
    }
    if(!self_testing) {
        uintptr_t linked{};
        if(!safe_copy(&linked,reinterpret_cast<void*>(expected_local+0x68),8) || linked!=expected_pc ||
           !safe_copy(&linked,reinterpret_cast<void*>(expected_pc+0x37c),8) || linked!=expected_camera) {
            wobble_state=3;record.test_state=3;return;
        }
    }
    long long origin=wobble_start.load();
    if(!origin && (self_testing || advancing)) {
        long long zero=0;wobble_start.compare_exchange_strong(zero,record.time);
        origin=wobble_start.load();wobble_state=1;
    }
    if(!origin)return;
    double elapsed=self_testing?selftest_elapsed:static_cast<double>(record.time-origin)/qpc_frequency;
    if(elapsed>=27) {wobble_state=2;record.test_state=2;record.pose_phase=7;scene_phase=7;return;}
    record.test_state=1;
    if(elapsed<3)return;
    unsigned phase=1+static_cast<unsigned>((elapsed-3)/4);
    double wave=std::sin((elapsed-3-(phase-1)*4)*6.283185307179586/4);
    int angle=static_cast<int>(std::lround(wave*1.5*65536.0/360.0));
    Vec delta{};Rot delta_rotation{};
    if(phase==1)delta_rotation.pitch=angle;
    if(phase==2)delta_rotation.yaw=angle;
    if(phase==3)delta_rotation.roll=angle;
    double yaw=source_rotation.yaw*6.283185307179586/65536.0;
    if(phase==4) {delta.x=static_cast<float>(2*wave*std::cos(yaw));delta.y=static_cast<float>(2*wave*std::sin(yaw));}
    if(phase==5) {delta.x=static_cast<float>(-2*wave*std::sin(yaw));delta.y=static_cast<float>(2*wave*std::cos(yaw));}
    if(phase==6)delta.z=static_cast<float>(2*wave);
    Vec output_position{source_position.x+delta.x,source_position.y+delta.y,source_position.z+delta.z};
    Rot output_rotation{source_rotation.pitch+delta_rotation.pitch,source_rotation.yaw+delta_rotation.yaw,source_rotation.roll+delta_rotation.roll};
    SIZE_T written{};
    if(!WriteProcessMemory(GetCurrentProcess(),position,&output_position,sizeof(Vec),&written) || written!=sizeof(Vec) ||
       !WriteProcessMemory(GetCurrentProcess(),rotation,&output_rotation,sizeof(Rot),&written) || written!=sizeof(Rot)) {
        wobble_state=3;record.test_state=3;return;
    }
    record.pose_phase=phase;record.translation_delta=delta;record.rotation_delta=delta_rotation;
    record.source_yaw=source_rotation.yaw;record.yaw_delta=delta_rotation.yaw;
    scene_phase=phase;scene_translation=delta;scene_rotation_delta=delta_rotation;scene_delta=delta_rotation.yaw;
    if(angle || delta.x || delta.y || delta.z)record.valid|=64;
}
Record start_record(unsigned kind,void* object,void* caller) {
    Record r{};r.kind=kind;r.thread=GetCurrentThreadId();r.time=now();r.focused=foreground();
    r.object=reinterpret_cast<uintptr_t>(object);r.caller=reinterpret_cast<uintptr_t>(caller)-base;
    return r;
}
void apply_live(Record& record,void* pc,Vec* position,Rot* rotation) {
    if(!live_enabled || !recording.load() || reinterpret_cast<uintptr_t>(pc)!=expected_pc ||
       reinterpret_cast<uintptr_t>(scene_local)!=expected_local || scene_rotation!=rotation || record.caller!=allowed_caller)return;
    if(!TryAcquireSRWLockExclusive(&live_lock))return;
    struct Unlock {~Unlock(){ReleaseSRWLockExclusive(&live_lock);}} unlock;
    Vec p{};Rot r{};
    if(!safe_copy(&p,position,sizeof(p)) || !safe_copy(&r,rotation,sizeof(r)))return;
    record.source_position=p;record.source_rotation=r;
    uintptr_t link{};
    bool linked=self_testing || (safe_copy(&link,reinterpret_cast<void*>(expected_local+0x68),8) && link==expected_pc &&
        safe_copy(&link,reinterpret_cast<void*>(expected_pc+0x37c),8) && link==expected_camera);
    if(GetAsyncKeyState(VK_F8)&0x8000)live_aborted=true;
    if(GetAsyncKeyState(VK_F9)&0x8000)live_gate.recenter();
    if(live_aborted)live_gate.arm(false);
    gears_vr::PoseSample sample{};live_bridge.read(sample);
    const double unit=6.283185307179586/65536;
    gears_vr::Viewpoint clean{{p.x,p.y,p.z},{r.pitch*unit,r.yaw*unit,r.roll*unit}};
    auto result=live_gate.update(clean,sample,gears_vr::monotonic_ns(),(record.focused || self_testing) && linked);
    record.pose_status=live_status=static_cast<unsigned>(result.status);
    record.pose_sequence=live_sequence=sample.sequence;
    if(result.status!=gears_vr::PoseStatus::applied)return;
    auto output=gears_vr::bounded_view(clean,result.view);
    Vec next_p{static_cast<float>(output.position.x),static_cast<float>(output.position.y),static_cast<float>(output.position.z)};
    Rot delta{gears_vr::to_rotator_units(output.rotation.pitch-clean.rotation.pitch),
        gears_vr::to_rotator_units(output.rotation.yaw-clean.rotation.yaw),gears_vr::to_rotator_units(output.rotation.roll-clean.rotation.roll)};
    auto add=[](int a,int b){unsigned bits=static_cast<unsigned>(a)+static_cast<unsigned>(b);int value;memcpy(&value,&bits,4);return value;};
    Rot next_r{add(r.pitch,delta.pitch),add(r.yaw,delta.yaw),add(r.roll,delta.roll)};
    SIZE_T written{};
    if(!WriteProcessMemory(GetCurrentProcess(),position,&next_p,sizeof(next_p),&written) || written!=sizeof(next_p) ||
       !WriteProcessMemory(GetCurrentProcess(),rotation,&next_r,sizeof(next_r),&written) || written!=sizeof(next_r)) {
        live_aborted=true;return;
    }
    scene_translation=record.translation_delta={next_p.x-p.x,next_p.y-p.y,next_p.z-p.z};
    scene_rotation_delta=record.rotation_delta=delta;
    record.source_yaw=r.yaw;record.yaw_delta=scene_delta=delta.yaw;
    record.pose_phase=scene_phase=8;
    record.valid|=64;
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
    void* saved_local=scene_local;Rot* saved_rotation=scene_rotation;int saved_delta=scene_delta;
    unsigned saved_phase=scene_phase;Vec saved_translation=scene_translation;Rot saved_rotation_delta=scene_rotation_delta;
    scene_local=local;scene_rotation=rotation;scene_delta=0;
    scene_phase=0;scene_translation={};scene_rotation_delta={};
    void* result=original_scene(local,family,position,rotation,viewport,drawer);
    record.duration=now()-record.time;record.result=reinterpret_cast<uintptr_t>(result);
    if(safe_copy(&record.position,position,sizeof(Vec))) record.valid|=1;
    if(safe_copy(&record.rotation,rotation,sizeof(Rot))) record.valid|=2;
    if(result && safe_copy(record.matrices.data(),reinterpret_cast<char*>(result)+0x70,128)) record.valid|=4;
    if(camera_cache && safe_copy(&record.camera_time,reinterpret_cast<void*>(camera_cache),4)) record.valid|=8;
    if(record.object==expected_local) record.valid|=16;
    record.yaw_delta=scene_delta;record.source_yaw=record.rotation.yaw-scene_delta;record.test_state=wobble_state.load();
    if(scene_delta) record.valid|=64;
    if(camera_yaw) safe_copy(&record.cache_yaw,reinterpret_cast<void*>(camera_yaw),4);
    record.pose_phase=scene_phase;record.translation_delta=scene_translation;record.rotation_delta=scene_rotation_delta;
    if(live_enabled){record.pose_status=live_status;record.pose_sequence=live_sequence;}
    record.source_position={record.position.x-scene_translation.x,record.position.y-scene_translation.y,record.position.z-scene_translation.z};
    record.source_rotation={record.rotation.pitch-scene_rotation_delta.pitch,record.rotation.yaw-scene_rotation_delta.yaw,record.rotation.roll-scene_rotation_delta.roll};
    if(scene_rotation_delta.pitch || scene_rotation_delta.yaw || scene_rotation_delta.roll ||
       scene_translation.x || scene_translation.y || scene_translation.z)record.valid|=64;
    scene_local=saved_local;scene_rotation=saved_rotation;scene_delta=saved_delta;
    scene_phase=saved_phase;scene_translation=saved_translation;scene_rotation_delta=saved_rotation_delta;
    enqueue(record);return result;
}
void __fastcall hook_event(void* pc,Vec* position,Rot* rotation) {
    Scope scope;auto record=start_record(2,pc,_ReturnAddress());
    original_event(pc,position,rotation);
    record.duration=now()-record.time;
    apply_wobble(record,pc,rotation);
    apply_sweep(record,pc,position,rotation);
    apply_live(record,pc,position,rotation);
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
            fprintf(file,",%d,%d,%d,%u",r.source_yaw,r.yaw_delta,r.cache_yaw,r.test_state);
            for(float value:r.matrices) fprintf(file,",%.9g",value);
            fprintf(file,",%u,%.9g,%.9g,%.9g,%d,%d,%d,%.9g,%.9g,%.9g,%d,%d,%d",
                r.pose_phase,r.source_position.x,r.source_position.y,r.source_position.z,
                r.source_rotation.pitch,r.source_rotation.yaw,r.source_rotation.roll,
                r.translation_delta.x,r.translation_delta.y,r.translation_delta.z,
                r.rotation_delta.pitch,r.rotation_delta.yaw,r.rotation_delta.roll);
            fprintf(file,",%u,%llu\n",r.pose_status,r.pose_sequence);
        }
        fflush(file);
    }
}
bool install(void* target,void* detour,void** original) {
    MH_STATUS created=MH_CreateHook(target,detour,original);
    MH_STATUS queued=created==MH_OK?MH_QueueEnableHook(target):MH_ERROR_NOT_CREATED;
    if(diagnostic_status)fprintf(diagnostic_status,"hook_target=%p create=%s queue=%s\n",target,MH_StatusToString(created),MH_StatusToString(queued));
    return created==MH_OK && queued==MH_OK;
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
    diagnostic_status=status;
    if(!hash_matches() || ini_address(ini,L"pid")!=GetCurrentProcessId()) {
        fprintf(status,"rejected: executable hash/name or PID mismatch\n");fclose(status);return 2;
    }
    base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    expected_local=ini_address(ini,L"local_player");expected_pc=ini_address(ini,L"controller");
    camera_cache=ini_address(ini,L"timestamp");
    expected_camera=ini_address(ini,L"camera");camera_yaw=ini_address(ini,L"camera_yaw");
    wobble_enabled=GetPrivateProfileIntW(L"Capture",L"wobble",0,ini)==1;
    sweep_enabled=GetPrivateProfileIntW(L"Capture",L"sweep",0,ini)==1;
    live_enabled=GetPrivateProfileIntW(L"Capture",L"live_head",0,ini)==1;
    if(int(wobble_enabled)+int(sweep_enabled)+int(live_enabled)>1) {fprintf(status,"rejected: mutually exclusive camera tests\n");fclose(status);return 3;}
    if(live_enabled) {
        if(!live_bridge.open(GetCurrentProcessId(),false)){fprintf(status,"rejected: pose producer unavailable\n");fclose(status);return 3;}
        live_gate.arm(true);
    }
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
    fprintf(file,"kind,thread,focused,valid,qpc,duration_qpc,object,result,caller_rva,x,y,z,pitch,yaw,roll,camera_timestamp,source_yaw,yaw_delta,cache_yaw,test_state");
    for(unsigned i=0;i<32;++i) fprintf(file,",matrix%u",i);
    fprintf(file,",pose_phase,source_x,source_y,source_z,source_pitch,source_rot_yaw,source_roll,delta_x,delta_y,delta_z,delta_pitch,delta_rot_yaw,delta_roll,pose_status,pose_sequence\n");fflush(file);
    LARGE_INTEGER frequency;QueryPerformanceFrequency(&frequency);
    qpc_frequency=frequency.QuadPart;
    fprintf(status,"qpc_frequency=%lld\n",frequency.QuadPart);
    fprintf(status,"wobble_enabled=%d amplitude_degrees=1.5 baseline_seconds=3 wave_seconds=12\n",wobble_enabled);
    fprintf(status,"sweep_enabled=%d rotation_degrees=1.5 translation_units=2 baseline_seconds=3 axis_seconds=4 total_seconds=27\n",sweep_enabled);
    fprintf(status,"live_head_enabled=%d rotation_limit_degrees=5 translation_limit_units=2 max_age_ms=100 F8=abort F9=recenter\n",live_enabled);
    MH_STATUS init=MH_Initialize();
    fprintf(status,"minhook_initialize=%s\n",MH_StatusToString(init));
    bool ok=init==MH_OK && install(scene_target,reinterpret_cast<void*>(hook_scene),reinterpret_cast<void**>(&original_scene))
        && install(event_target,reinterpret_cast<void*>(hook_event),reinterpret_cast<void**>(&original_event));
    bool graphics=ok && present_hooks();
    fprintf(status,"present_hooks_created=%d\n",graphics);
    if(ok) {recording=true;MH_STATUS applied=MH_ApplyQueued();fprintf(status,"minhook_apply=%s\n",MH_StatusToString(applied));ok=applied==MH_OK;}
    if(!ok) {
        recording=false;MH_STATUS disabled=MH_DisableHook(MH_ALL_HOOKS);
        fprintf(status,"hook installation failed disable_status=%d active=%u\n",disabled,active.load());
        diagnostic_status=nullptr;fclose(file);fclose(status);return 6;
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
    fprintf(status,"wobble_final_state=%u\n",wobble_state.load());
    diagnostic_status=nullptr;fclose(file);fclose(status);if(stop_event) CloseHandle(stop_event);stop_event=nullptr;
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
    // Apply only through a matching scene context/caller; a standalone event
    // must retain its original yaw, and disabling hooks must restore behavior.
    allowed_caller=ring[0].caller;self_testing=true;wobble_enabled=true;recording=true;
    if(MH_QueueEnableHook(reinterpret_cast<void*>(fake_scene))!=MH_OK ||
       MH_QueueEnableHook(reinterpret_cast<void*>(fake_event))!=MH_OK || MH_ApplyQueued()!=MH_OK) return 19;
    Vec p3{1,2,3};Rot r3{4,5,6};
    invoke(reinterpret_cast<void*>(1),reinterpret_cast<void*>(2),&p3,&r3,reinterpret_cast<void*>(3),reinterpret_cast<void*>(4));
    if(r3.yaw!=12+offset_at(4) || p3.x!=6 || ring[3].yaw_delta!=offset_at(4)) return 20;
    Vec p4{1,2,3};Rot r4{4,5,6};fake_event(reinterpret_cast<void*>(5),&p4,&r4);
    if(r4.yaw!=12 || ring[4].yaw_delta!=0) return 21;
    recording=false;if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK) return 22;
    if(offset_at(2)!=0 || offset_at(15)!=0 || offset_at(4)<=0 || offset_at(6)>=0) return 23;
    wobble_enabled=false;sweep_enabled=true;recording=true;wobble_state=0;wobble_start=0;
    if(MH_QueueEnableHook(reinterpret_cast<void*>(fake_scene))!=MH_OK ||
       MH_QueueEnableHook(reinterpret_cast<void*>(fake_event))!=MH_OK || MH_ApplyQueued()!=MH_OK)return 24;
    for(unsigned phase=1;phase<=6;++phase) {
        selftest_elapsed=4+(phase-1)*4;Vec test_p{1,2,3};Rot test_r{4,5,6};
        invoke(reinterpret_cast<void*>(1),reinterpret_cast<void*>(2),&test_p,&test_r,reinterpret_cast<void*>(3),reinterpret_cast<void*>(4));
        const auto& record=ring[(write_index-1)%ring.size()];
        if(record.pose_phase!=phase)return 25;
        if(phase<=3) {
            if(test_r.pitch!=4+(phase==1?offset_at(4):0) || test_r.yaw!=12+(phase==2?offset_at(4):0) ||
               test_r.roll!=6+(phase==3?offset_at(4):0))return 26;
        } else if(std::fabs(std::sqrt(record.translation_delta.x*record.translation_delta.x+
                       record.translation_delta.y*record.translation_delta.y+record.translation_delta.z*record.translation_delta.z)-2)>0.001)return 27;
    }
    selftest_elapsed=28;Vec end_p{1,2,3};Rot end_r{4,5,6};
    invoke(reinterpret_cast<void*>(1),reinterpret_cast<void*>(2),&end_p,&end_r,reinterpret_cast<void*>(3),reinterpret_cast<void*>(4));
    if(wobble_state!=2 || end_p.x!=6 || end_r.pitch!=4 || end_r.yaw!=12 || end_r.roll!=6)return 28;
    recording=false;if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK)return 29;
    // Real shared-memory producer/consumer through the hooked matching caller.
    sweep_enabled=false;live_enabled=true;live_gate.arm(true);
    gears_vr::PoseBridge producer,duplicate;
    if(!producer.open(GetCurrentProcessId(),true) || duplicate.open(GetCurrentProcessId(),true) ||
       !live_bridge.open(GetCurrentProcessId(),false))return 30;
    gears_vr::PoseSample sample{{{0,0,0},{0,0,0,1}},1,gears_vr::monotonic_ns(),15,true};
    if(!producer.publish(sample))return 31;
    recording=true;
    if(MH_QueueEnableHook(reinterpret_cast<void*>(fake_scene))!=MH_OK ||
       MH_QueueEnableHook(reinterpret_cast<void*>(fake_event))!=MH_OK || MH_ApplyQueued()!=MH_OK)return 32;
    Vec live_p{1,2,3};Rot live_r{4,5,6};
    invoke(reinterpret_cast<void*>(1),reinterpret_cast<void*>(2),&live_p,&live_r,reinterpret_cast<void*>(3),reinterpret_cast<void*>(4));
    if(live_p.x!=6 || live_r.yaw!=12 || live_status!=static_cast<unsigned>(gears_vr::PoseStatus::recentered))return 33;
    sample.sequence++;sample.received_ns=gears_vr::monotonic_ns();sample.head.position.x=.2;
    sample.head.orientation={0,std::sin(.2),0,std::cos(.2)};
    if(!producer.publish(sample))return 34;
    Vec moved_p{1,2,3};Rot moved_r{4,5,6};
    invoke(reinterpret_cast<void*>(1),reinterpret_cast<void*>(2),&moved_p,&moved_r,reinterpret_cast<void*>(3),reinterpret_cast<void*>(4));
    const auto& live_record=ring[(write_index-1)%ring.size()];
    double length=std::sqrt(live_record.translation_delta.x*live_record.translation_delta.x+
        live_record.translation_delta.y*live_record.translation_delta.y+live_record.translation_delta.z*live_record.translation_delta.z);
    if(live_status!=static_cast<unsigned>(gears_vr::PoseStatus::applied) || length<1.99 || length>2.001 ||
       std::abs(live_record.rotation_delta.yaw)>911 || std::abs(live_record.rotation_delta.yaw)<909)return 35;
    sample.sequence++;sample.received_ns=gears_vr::monotonic_ns()-200000000;
    if(!producer.publish(sample))return 36;
    Vec stale_p{1,2,3};Rot stale_r{4,5,6};
    invoke(reinterpret_cast<void*>(1),reinterpret_cast<void*>(2),&stale_p,&stale_r,reinterpret_cast<void*>(3),reinterpret_cast<void*>(4));
    if(stale_p.x!=6 || stale_p.y!=2 || stale_r.yaw!=12 ||
       live_status!=static_cast<unsigned>(gears_vr::PoseStatus::stale))return 37;
    recording=false;live_enabled=false;live_bridge.close();
    if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK)return 38;
    return 0;
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,void*) {
    // Keep CRT thread notifications enabled for the statically linked runtime.
    if(reason==DLL_PROCESS_ATTACH) own_module=module;
    return TRUE;
}
