#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <bcrypt.h>
#include <intrin.h>
#include <array>
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cwchar>
#include <cstring>
#include <share.h>
#include <cmath>
#include "MinHook.h"
#include "../vr-common/pose_bridge.h"
#include "source_view_shadow.h"
#include "native_source_view_args.h"
#include "source_eye_args.h"
#include "two_view_renderer_shadow.h"
#include "split_source_views.h"
#include "submitted_view_lifetime.h"
#include "gpu_viewport_stats.h"

namespace {
struct Vec {float x,y,z;};
struct Rot {int pitch,yaw,roll;};
using SceneFn=void*(__fastcall*)(void*,void*,Vec*,Rot*,void*,void*);
using EventFn=void(__fastcall*)(void*,Vec*,Rot*);
using FamilyFn=void(__fastcall*)(void*,void*);
FamilyFn original_family{};
using CloneFn=void*(__fastcall*)(void*,void*);
CloneFn original_clone{};
void* clone_target{};
thread_local void* submitting_family{};
using RendererFn=void*(__fastcall*)(void*,void*,void*,void*);
RendererFn original_renderer{};
void* renderer_target{};
using ConsumeFn=void(__fastcall*)(void*);
using DestroyRendererFn=void*(__fastcall*)(void*);
ConsumeFn original_consume{};
DestroyRendererFn original_destroy_renderer{};
void* consume_target{};void* destroy_renderer_target{};
ConsumeFn original_prepare{};
void* prepare_target{};
thread_local void* consuming_renderer{};
bool family_diagnostics{};
bool shadow_view_enabled{};
std::atomic<unsigned> shadow_attempted{0};
gears_shadow::API shadow_api{};
gears_shadow::Result shadow_result{};
bool constructor_shadow_enabled{};
bool constructor_state_enabled{};
bool constructor_eye_enabled{};
bool renderer_pair_enabled{};
bool submit_pair_enabled{};
bool submit_output_bounds{};
bool submit_mono_control{};
bool submit_mono_independent_state{};
bool submit_duplicate_control{};
std::atomic<uintptr_t> submit_original_state{0};
std::atomic<unsigned> submit_attempted{0},submit_consumed{0},submit_prepared{0},submit_destroyed{0};
std::atomic<uintptr_t> submit_root{0};
std::atomic<unsigned> submit_owned_verified{0},submit_sources_released{0},submit_original_preserved{0};
unsigned submit_source_checks{},submit_rejection{};
std::array<float,4> submit_rectangle{};
unsigned submit_pair_seconds{};
gears_submitted::Lifetime submitted_lifetime;
std::atomic<unsigned> submit_total{0},submit_retained{0},submit_skipped{0};
LONGLONG submit_deadline{},submit_not_before{};
struct RectangleTrace {uintptr_t object{};std::array<float,4> source{};std::array<int,4> target{};
    std::array<float,4> sampling{};std::array<int,2> buffer{};};
std::array<RectangleTrace,10> rectangle_traces{};
std::array<std::atomic<unsigned>,10> rectangle_trace_phases{};
gears_pair_shadow::Result renderer_pair_result{};
thread_local bool unsubmitted_renderer{};
thread_local void* pair_canvas{};
thread_local void* pair_transform{};
std::array<float,32> constructor_eye_matrices{};
unsigned constructor_eye_shifted{},constructor_combined_valid{};
double constructor_combined_relative_error{};
gears_source_args::Constructor source_constructor{};
unsigned constructor_derived_finite{};
double constructor_derived_error{};
using StateFactoryFn=void*(__fastcall*)();
using StateDeleteFn=void*(__fastcall*)(void*,unsigned);
StateFactoryFn state_factory{};
ConsumeFn state_release{};
StateDeleteFn original_state_delete{};
void* state_delete_target{};
uintptr_t state_expected_table{};
bool shadow_state_enabled{};
std::atomic<unsigned> state_attempted{0},state_created{0},state_release_returned{0},state_destroyed{0};
std::atomic<unsigned> state_delete_entered{0},state_original_preserved{0};
std::atomic<uintptr_t> state_temporary{0};
unsigned state_delete_flags{},state_delete_thread{};
uintptr_t state_delete_caller{};
void* family_target{};
bool reservation_ready{};
std::atomic<unsigned> reservation_started{0};
HANDLE reservation_event{};
using PresentFn=HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*,UINT,UINT);
using Present1Fn=HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain1*,UINT,UINT,const DXGI_PRESENT_PARAMETERS*);
using ExecuteFn=void(STDMETHODCALLTYPE*)(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*);
SceneFn original_scene{}; EventFn original_event{};
PresentFn original_present{}; Present1Fn original_present1{};
ExecuteFn original_execute{};
bool gpu_diagnostics{};
bool viewport_diagnostics{};
thread_local bool trial_draw_scope{};
gears_viewport::Tracker viewport_stats;
// Candidate stacks only: resource identities must subsequently match Present buffers.
struct OutputStack {gears_viewport::Target target{};unsigned kind{},count{};std::array<void*,24> frames{};};
std::array<OutputStack,8> output_stacks{};
std::mutex output_stack_mutex;
std::atomic<unsigned> output_stack_attempts{0};
using ResetListFn=HRESULT(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12CommandAllocator*,ID3D12PipelineState*);
using ViewportsFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,const D3D12_VIEWPORT*);
using ScissorsFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,const D3D12_RECT*);
using DrawFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,UINT,UINT,UINT);
using DrawIndexedFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,UINT,UINT,INT,UINT);
using TargetsFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,const D3D12_CPU_DESCRIPTOR_HANDLE*,BOOL,const D3D12_CPU_DESCRIPTOR_HANDLE*);
using CreateRtvFn=void(STDMETHODCALLTYPE*)(ID3D12Device*,ID3D12Resource*,const D3D12_RENDER_TARGET_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
using CopyDescriptorsFn=void(STDMETHODCALLTYPE*)(ID3D12Device*,UINT,const D3D12_CPU_DESCRIPTOR_HANDLE*,const UINT*,UINT,const D3D12_CPU_DESCRIPTOR_HANDLE*,const UINT*,D3D12_DESCRIPTOR_HEAP_TYPE);
using CopySimpleFn=void(STDMETHODCALLTYPE*)(ID3D12Device*,UINT,D3D12_CPU_DESCRIPTOR_HANDLE,D3D12_CPU_DESCRIPTOR_HANDLE,D3D12_DESCRIPTOR_HEAP_TYPE);
ResetListFn original_reset_list{};ViewportsFn original_viewports{};ScissorsFn original_scissors{};
DrawFn original_draw{};DrawIndexedFn original_draw_indexed{};
TargetsFn original_targets{};CreateRtvFn original_create_rtv{};CopyDescriptorsFn original_copy_descriptors{};CopySimpleFn original_copy_simple{};
std::array<void*,9> viewport_targets{};
std::array<void*,3> prime_targets{};
std::atomic<bool> prime_recording{},prime_ready{},prime_handoff{},prime_done{};
std::atomic<unsigned> prime_started{};
std::mutex prime_mutex;
HANDLE prime_event{};
std::array<std::array<unsigned char,32>,3> prime_original_bytes{};
void* execute_target{};
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
    uintptr_t gpu_device,gpu_buffer,gpu_window;
    unsigned queue_type,command_count,buffer_index,buffer_count,width,height,format,swap_flags;
    Vec source_position,translation_delta;
    Rot source_rotation,rotation_delta;
    std::array<float,32> matrices;
    uintptr_t family_address{},family_array{};
    int family_count{},family_capacity{};
    std::array<uintptr_t,4> family_views{};
    uintptr_t clone_source{},clone_owner{};
    std::array<float,32> source_matrices{};
    uintptr_t renderer_input_family{},renderer_storage{};
    int renderer_count{},renderer_capacity{};
    std::array<uintptr_t,4> renderer_sources{};
};
std::array<Record,8192> ring{};
size_t read_index{},write_index{};
unsigned long long emitted[13]{};
void* scene_target{}; void* event_target{}; void* present_target{}; void* present1_target{};
HANDLE stop_event{};
wchar_t directory[MAX_PATH]{};
FILE* diagnostic_status{};
bool safe_copy(void* out,const void* from,size_t bytes) {
    SIZE_T count{};
    return from && ReadProcessMemory(GetCurrentProcess(),from,out,bytes,&count) && count==bytes;
}
void trace_rectangle(unsigned index,uintptr_t view) {
    if(!submit_pair_enabled || !view)return;
    unsigned expected=0;if(!rectangle_trace_phases[index].compare_exchange_strong(expected,1))return;
    auto& trace=rectangle_traces[index];trace.object=view;
    bool read=safe_copy(trace.source.data(),reinterpret_cast<void*>(view+0x48),16) &&
        safe_copy(trace.target.data(),reinterpret_cast<void*>(view+0x58),16) &&
        safe_copy(trace.sampling.data(),reinterpret_cast<void*>(view+0x3d0),16) &&
        safe_copy(trace.buffer.data(),reinterpret_cast<void*>(base+0x1c3b3b8),8);
    rectangle_trace_phases[index]=read?2:3;
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
void inspect_family(Record& record,void* family){
    record.family_address=reinterpret_cast<uintptr_t>(family);
    struct Header{uintptr_t data;int count,capacity;} header{};
    if(!safe_copy(&header,family,sizeof(header)))return;
    record.family_array=header.data;record.family_count=header.count;record.family_capacity=header.capacity;
    // Capacity is allocator slack, not the number of pointers we read. UE
    // arrays can reserve more than sixteen slots while holding one view.
    if(header.count<0 || header.count>4 || header.capacity<header.count || header.capacity>1024 || (header.count && (!header.data || header.data%8)))return;
    if(header.count && !safe_copy(record.family_views.data(),reinterpret_cast<void*>(header.data),size_t(header.count)*8))return;
    record.family_array=header.data;record.family_count=header.count;record.family_capacity=header.capacity;record.valid|=128;
}
void* __fastcall construct_from_snapshot(void* destination,void* source) {
    gears_source_args::Snapshot input{};
    if(!safe_copy(input.bytes.data(),source,input.bytes.size()))return nullptr;
    if(input.value<float>(0x50)<=0 || input.value<float>(0x54)<=0)return nullptr;
    if(constructor_eye_enabled) {
        gears_eye_args::Result shifted{};
        if(!gears_eye_args::shift_right(input,1.f,shifted))return nullptr;
        constructor_eye_shifted=1;
        memcpy(constructor_eye_matrices.data(),input.bytes.data()+0x70,128);
    }
    void* state=constructor_state_enabled?reinterpret_cast<void*>(state_temporary.load()):input.pointer(8);
    if(constructor_state_enabled && (!state || state_created.load()!=1))return nullptr;
    void* result=input.invoke(source_constructor,destination,input.pointer(0),state);
    gears_source_args::Snapshot output{};
    if(result==destination && safe_copy(output.bytes.data(),destination,output.bytes.size())) {
        constructor_derived_finite=1;
        if(constructor_eye_enabled) {
            constructor_combined_valid=1;
            for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col) {
                double expected=0;
                for(unsigned k=0;k<4;++k)expected+=double(input.value<float>(0x70+16*row+4*k))*input.value<float>(0xb0+16*k+4*col);
                float actual=output.value<float>(0x2b0+16*row+4*col);
                if(!std::isfinite(expected) || !std::isfinite(actual))constructor_combined_valid=0;
                else constructor_combined_relative_error=(std::max)(constructor_combined_relative_error,
                    std::fabs(double(actual)-expected)/(1.+std::fabs(expected)));
            }
        }
        // Known initialized matrix blocks; exclude the owned map at +0x140,
        // padding and fields whose initialization has not been established.
        for(size_t offset:{size_t(0x160),size_t(0x1a0),size_t(0x1e0),size_t(0x220),size_t(0x260),size_t(0x2b0),size_t(0x2f0)})
            for(unsigned i=0;i<16;++i) {
                float a=input.value<float>(offset+4*i),b=output.value<float>(offset+4*i);
                if(!std::isfinite(a) || !std::isfinite(b))constructor_derived_finite=0;
                else constructor_derived_error=(std::max)(constructor_derived_error,double(std::fabs(a-b)));
            }
    }
    return result;
}
void try_shadow_state(const Record& record) {
    if(!shadow_state_enabled || constructor_state_enabled || !recording.load() || record.caller!=0x2c8909 ||
       !(record.valid&128) || record.family_count!=1)return;
    unsigned expected=0;if(!state_attempted.compare_exchange_strong(expected,1))return;
    uintptr_t original_state{};
    if(!safe_copy(&original_state,reinterpret_cast<void*>(record.family_views[0]+8),8) || !original_state)return;
    void* temporary=state_factory();uintptr_t table{};
    state_temporary.store(reinterpret_cast<uintptr_t>(temporary));
    if(!temporary || reinterpret_cast<uintptr_t>(temporary)==original_state ||
       !safe_copy(&table,temporary,8) || table!=state_expected_table)return;
    state_created=1;
    // The ordinary virtual release wrapper queues resource release and deferred
    // deletion when required. Do not manually call the destructor/free here.
    state_release(temporary);state_release_returned=1;
    uintptr_t unchanged{};
    if(safe_copy(&unchanged,reinterpret_cast<void*>(record.family_views[0]+8),8) && unchanged==original_state)state_original_preserved=1;
}
void* __fastcall hook_state_delete(void* state,unsigned flags) {
    Scope scope;
    unsigned expected=0;
    const bool observe=reinterpret_cast<uintptr_t>(state)==state_temporary.load() && state_created.load()==1 &&
        state_delete_entered.compare_exchange_strong(expected,1);
    if(observe){state_delete_flags=flags;state_delete_thread=GetCurrentThreadId();state_delete_caller=reinterpret_cast<uintptr_t>(_ReturnAddress())-base;}
    void* result=original_state_delete(state,flags);
    // The deleting wrapper has already released the storage. Only keep numeric
    // identities and completion metadata; never inspect this object afterward.
    if(observe)++state_destroyed;
    return result;
}
bool probe_renderer_pair(void* family,void* view) {
    unsubmitted_renderer=true;
    renderer_pair_result=gears_pair_shadow::run({shadow_api.allocate,original_renderer,original_destroy_renderer,safe_copy},
        family,view,pair_canvas,pair_transform);
    unsubmitted_renderer=false;
    return !renderer_pair_result.retain_inputs;
}
void run_source_shadow(const Record& record,void* family) {
    if(shadow_view_enabled && recording.load() && (record.valid&128) && record.family_count==1) {
        unsigned expected=0;
        if(shadow_attempted.compare_exchange_strong(expected,1)) {
            uintptr_t expected_state=0,original_state=0;
            bool ready=true;
            if(constructor_state_enabled) {
                state_attempted=1;
                ready=safe_copy(&original_state,reinterpret_cast<void*>(record.family_views[0]+8),8) && original_state;
                if(ready) {
                    void* state=state_factory();uintptr_t table{};
                    expected_state=reinterpret_cast<uintptr_t>(state);state_temporary=expected_state;
                    ready=state && expected_state!=original_state && safe_copy(&table,state,8) && table==state_expected_table;
                    if(ready)state_created=1;
                }
            }
            if(ready)shadow_result=gears_shadow::run(shadow_api,family,reinterpret_cast<void*>(record.family_views[0]),expected_state,
                constructor_eye_enabled?constructor_eye_matrices.data():nullptr,constructor_eye_enabled,
                renderer_pair_enabled?probe_renderer_pair:nullptr);
            // Only release the independent state after its view has been safely
            // destroyed, or when no view was allocated. A quarantined view
            // retains its state until process exit; never free beneath it.
            if(constructor_state_enabled && ready && (!shadow_result.temporary || (shadow_result.checks&32))) {
                state_release(reinterpret_cast<void*>(expected_state));state_release_returned=1;
                uintptr_t unchanged{};
                if(safe_copy(&unchanged,reinterpret_cast<void*>(record.family_views[0]+8),8) && unchanged==original_state)state_original_preserved=1;
            }
            shadow_attempted.store(2);
        }
    }
}
void __fastcall hook_family(void* canvas,void* family){
    Scope scope;auto record=start_record(7,canvas,_ReturnAddress());
    inspect_family(record,family);
    if(record.caller==0x2c8909 && record.family_count==1)trace_rectangle(1,record.family_views[0]);
    try_shadow_state(record);
    if(!renderer_pair_enabled && !submit_pair_enabled && record.caller==0x2c8909)run_source_shadow(record,family);
    void* previous=submitting_family;submitting_family=family;
    original_family(canvas,family);
    submitting_family=previous;
    record.duration=now()-record.time;enqueue(record);
}
void* __fastcall hook_clone(void* destination,void* source){
    Scope scope;auto record=start_record(8,destination,_ReturnAddress());
    bool observe=submitting_family!=nullptr && !unsubmitted_renderer;
    if(observe){
        inspect_family(record,submitting_family);
        record.clone_source=reinterpret_cast<uintptr_t>(source);
        if(source && safe_copy(record.source_matrices.data(),reinterpret_cast<char*>(source)+0x70,128))record.valid|=256;
    }
    void* result=original_clone(destination,source);
    if(observe){
        record.result=reinterpret_cast<uintptr_t>(result);
        if(destination && safe_copy(record.matrices.data(),reinterpret_cast<char*>(destination)+0x70,128))record.valid|=512;
        if(safe_copy(&record.clone_owner,destination,8))record.valid|=1024;
        record.duration=now()-record.time;enqueue(record);
    }
    return result;
}
void inspect_completed_renderer(Record& record,void* destination){
    if(!destination)return;
    inspect_family(record,reinterpret_cast<char*>(destination)+8);
    struct Header{uintptr_t data;int count,capacity;} storage{};
    if(safe_copy(&storage,reinterpret_cast<char*>(destination)+0x68,sizeof(storage))){
        record.renderer_storage=storage.data;record.renderer_count=storage.count;record.renderer_capacity=storage.capacity;
        if(storage.data && storage.data%8==0 && storage.count>0 && storage.count<=4 && storage.capacity>=storage.count && storage.capacity<=1024)record.valid|=4096;
    }
    uintptr_t view=record.family_views[0];
    if(view && safe_copy(record.matrices.data(),reinterpret_cast<void*>(view+0x70),128))record.valid|=512;
    if(safe_copy(&record.clone_owner,reinterpret_cast<void*>(view),8))record.valid|=1024;
}
void __fastcall hook_consume(void* renderer){
    Scope scope;auto record=start_record(10,renderer,_ReturnAddress());
    inspect_completed_renderer(record,renderer);
    if(submit_pair_enabled && (submit_pair_seconds?submitted_lifetime.contains(reinterpret_cast<uintptr_t>(renderer)):
        reinterpret_cast<uintptr_t>(renderer)==submit_root.load() && !submit_destroyed.load())){++submit_consumed;record.test_state=77;}
    void* saved=consuming_renderer;consuming_renderer=renderer;
      bool saved_draw_scope=trial_draw_scope;trial_draw_scope=record.test_state==77;
    original_consume(renderer);
      trial_draw_scope=saved_draw_scope;
    consuming_renderer=saved;
    // The original consumer can free renderer. Only our local record is used
    // after it returns; never dereference the game object here.
    record.duration=now()-record.time;enqueue(record);
}
void __fastcall hook_prepare(void* renderer){
    Scope scope;bool observe=renderer && renderer==consuming_renderer;
    auto record=start_record(12,renderer,_ReturnAddress());
    if(observe){
        Record before{};inspect_completed_renderer(before,renderer);
        if(submit_pair_enabled && before.family_count==(submit_mono_control?1:2) && (submit_pair_seconds?submitted_lifetime.contains(reinterpret_cast<uintptr_t>(renderer)):
            reinterpret_cast<uintptr_t>(renderer)==submit_root.load())) {
            trace_rectangle(6,before.family_views[0]);trace_rectangle(7,before.family_views[1]);
        }
        record.clone_source=before.family_views[0];
        record.source_matrices=before.matrices;
        if(before.valid&512)record.valid|=256;
        if(safe_copy(&record.renderer_sources[0],reinterpret_cast<void*>(record.clone_source+8),8))record.valid|=8192;
    }
    original_prepare(renderer);
    if(observe){
        inspect_completed_renderer(record,renderer);
        if(submit_pair_enabled && (submit_pair_seconds?submitted_lifetime.contains(reinterpret_cast<uintptr_t>(renderer)):
            reinterpret_cast<uintptr_t>(renderer)==submit_root.load() && !submit_destroyed.load())) {
            uintptr_t right_state{};
            unsigned index=submit_mono_control?0:1;
            uintptr_t expected_state=submit_mono_control && !submit_mono_independent_state?submit_original_state.load():state_temporary.load();
            if(record.family_count==(submit_mono_control?1:2) && safe_copy(&right_state,reinterpret_cast<void*>(record.family_views[index]+8),8) && right_state==expected_state)++submit_prepared;
            record.test_state=77;
            trace_rectangle(8,record.family_views[0]);trace_rectangle(9,record.family_views[1]);
        }
        if(safe_copy(&record.renderer_sources[1],reinterpret_cast<void*>(record.family_views[0]+8),8))record.valid|=16384;
        record.duration=now()-record.time;enqueue(record);
    }
}
void* __fastcall hook_destroy_renderer(void* renderer){
    Scope scope;auto record=start_record(11,renderer,_ReturnAddress());
    inspect_completed_renderer(record,renderer);
    const bool submitted=submit_pair_enabled && (submit_pair_seconds?submitted_lifetime.contains(reinterpret_cast<uintptr_t>(renderer)):
        submit_attempted.load()==2 && reinterpret_cast<uintptr_t>(renderer)==submit_root.load() && !submit_destroyed.load());
    if(submitted)record.test_state=77;
    void* result=original_destroy_renderer(renderer);
    if(submitted) {
        ++submit_destroyed;
        // The submitted renderer can use its state until destruction returns.
        // Release on this native thread, never on the collection worker.
        bool release=submit_pair_seconds?submitted_lifetime.retire(reinterpret_cast<uintptr_t>(renderer)):true;
        if(release && !submit_retained.load() && submit_sources_released && state_created.load()==1) {
            state_release(reinterpret_cast<void*>(state_temporary.load()));state_release_returned=1;
        }
    }
    record.result=reinterpret_cast<uintptr_t>(result);
    record.duration=now()-record.time;enqueue(record);return result;
}
bool try_submit_pair(void* destination,void* family,void* canvas,void* transform,Record& record,void*& returned) {
    if(submit_pair_seconds) {
        if(submitted_lifetime.is_closed())return false;
        if(!submit_deadline && now()<submit_not_before)return false;
        if(!submit_deadline && !foreground())return false;
        if(submit_deadline && (now()>=submit_deadline || !foreground() || (GetAsyncKeyState(VK_F8)&0x8000))) {
            if(submitted_lifetime.close() && state_created.load() && !submit_retained.load()) {
                state_release(reinterpret_cast<void*>(state_temporary.load()));state_release_returned=1;
            }
            return false;
        }
    } else {
        unsigned expected=0;
        if(!submit_attempted.compare_exchange_strong(expected,1))return false;
    }
    alignas(8) std::array<unsigned char,0x60> original_family_bytes{},split_family{},after_family{};
    Record input{};inspect_family(input,family);
    gears_source_args::Snapshot original_source{},after_source{};
    if(!(input.valid&128) || input.family_count!=1 || !safe_copy(original_family_bytes.data(),family,0x60) ||
       !safe_copy(original_source.bytes.data(),reinterpret_cast<void*>(input.family_views[0]),0x430))return false;
    uintptr_t original_state=reinterpret_cast<uintptr_t>(original_source.pointer(8));
    submit_original_state=original_state;
    trace_rectangle(2,input.family_views[0]);
    memcpy(submit_rectangle.data(),original_source.bytes.data()+0x48,16);
    void* state=reinterpret_cast<void*>(state_temporary.load());
    if(!submit_pair_seconds || !state_created.load()) {
        state_attempted=1;
        state=state_factory();uintptr_t table{};state_temporary=reinterpret_cast<uintptr_t>(state);
        if(!state || state_temporary.load()==original_state || !safe_copy(&table,state,8) || table!=state_expected_table) {
            if(submit_pair_seconds){submit_retained=1;submitted_lifetime.close();}return false;
        }
        state_created=1;
        if(submit_pair_seconds)submit_deadline=now()+static_cast<LONGLONG>(submit_pair_seconds)*qpc_frequency;
    }
    gears_split_sources::API api{shadow_api.allocate,source_constructor,shadow_api.destroy,shadow_api.free,safe_copy};
    auto sources=gears_split_sources::create(api,family,reinterpret_cast<void*>(input.family_views[0]),state,submit_output_bounds,submit_mono_control || submit_duplicate_control);
    submit_source_checks=sources.checks;submit_rejection=sources.rejection;
    if(sources.checks!=63) {
        bool no_views=!sources.views[0] && !sources.views[1];
        bool cleaned=no_views || gears_split_sources::release(api,sources);
        if(submit_pair_seconds) {
            if(!cleaned)submit_retained=1;
            if(submitted_lifetime.close() && cleaned){state_release(state);state_release_returned=1;}
        } else if(cleaned){state_release(state);state_release_returned=1;}
        state_original_preserved=safe_copy(after_source.bytes.data(),reinterpret_cast<void*>(input.family_views[0]),0x430) && after_source.bytes==original_source.bytes;
        submit_attempted=3; // Publish a completed rejection, never claim submission.
        return false;
    }
    trace_rectangle(3,reinterpret_cast<uintptr_t>(sources.views[0]));trace_rectangle(4,reinterpret_cast<uintptr_t>(sources.views[1]));
    if(submit_pair_seconds && !submitted_lifetime.begin(reinterpret_cast<uintptr_t>(destination))) {
        if(!gears_split_sources::release(api,sources))submit_retained=1;
        ++submit_skipped;return false;
    }
    split_family=original_family_bytes;
    uintptr_t pointers[]={reinterpret_cast<uintptr_t>(sources.views[0]),reinterpret_cast<uintptr_t>(sources.views[1])};
      if(submit_mono_independent_state)pointers[0]=pointers[1];
    int count=submit_mono_control?1:2;
    gears_shadow::Header pair{reinterpret_cast<uintptr_t>(pointers),count,count};memcpy(split_family.data(),&pair,sizeof(pair));
    record.renderer_input_family=reinterpret_cast<uintptr_t>(split_family.data());
    record.renderer_sources={pointers[0],pointers[1],0,0};record.clone_source=pointers[0];
    memcpy(record.source_matrices.data(),sources.inputs[0].bytes.data()+0x70,128);
    submit_root=reinterpret_cast<uintptr_t>(destination);
    void* previous=submitting_family;submitting_family=split_family.data();
    returned=original_renderer(destination,split_family.data(),canvas,transform);
    submitting_family=previous;
    Record built{};inspect_completed_renderer(built,destination);
    trace_rectangle(5,built.family_views[0]);
    bool verified=returned==destination && built.family_count==count && built.renderer_count==count &&
        built.family_views[0]==built.renderer_storage && (count==1 || built.family_views[1]==built.renderer_storage+0xd30);
    for(unsigned i=0;verified && i<static_cast<unsigned>(count);++i) {
        uintptr_t owner{},associated{};unsigned index{};std::array<unsigned char,128> matrix{};
        std::array<unsigned char,16> rectangle{};
        unsigned source_index=submit_mono_independent_state?1:i;
        verified=safe_copy(&owner,reinterpret_cast<void*>(built.family_views[i]),8) && owner==submit_root.load()+8 &&
            safe_copy(&associated,reinterpret_cast<void*>(built.family_views[i]+8),8) && associated==(source_index?state_temporary.load():original_state) &&
            safe_copy(&index,reinterpret_cast<void*>(built.family_views[i]+0xd24),4) && index==i &&
            safe_copy(matrix.data(),reinterpret_cast<void*>(built.family_views[i]+0x70),128) &&
            !memcmp(matrix.data(),sources.inputs[source_index].bytes.data()+0x70,128) &&
            safe_copy(rectangle.data(),reinterpret_cast<void*>(built.family_views[i]+0x48),16) &&
            !memcmp(rectangle.data(),sources.inputs[source_index].bytes.data()+0x48,16);
    }
    bool source_released=gears_split_sources::release(api,sources);
    bool original_preserved=safe_copy(after_family.data(),family,0x60) && after_family==original_family_bytes &&
        safe_copy(after_source.bytes.data(),reinterpret_cast<void*>(input.family_views[0]),0x430) && after_source.bytes==original_source.bytes;
    bool first=submit_total.load()==0;
    submit_owned_verified=first?verified:submit_owned_verified && verified;
    submit_sources_released=first?source_released:submit_sources_released && source_released;
    submit_original_preserved=first?original_preserved:submit_original_preserved && original_preserved;
    if(!source_released)submit_retained=1;
    ++submit_total;
    state_original_preserved=submit_original_preserved.load();
    submit_attempted=2;
    if(submit_pair_seconds && (!verified || !source_released || !original_preserved)) {
        // Stop producing immediately; the registered renderer still owns its
        // copies and must retire through the native destruction path.
        submitted_lifetime.close();
    }
    return true;
}
void* __fastcall hook_renderer(void* destination,void* family,void* canvas,void* transform){
    Scope scope;auto record=start_record(9,destination,_ReturnAddress());
    bool observe=submitting_family!=nullptr;
    if(observe){
        Record input{};inspect_family(input,family);
        if(renderer_pair_enabled && family==submitting_family && record.caller==0x3a1595) {
            pair_canvas=canvas;pair_transform=transform;
            run_source_shadow(input,family);
            pair_canvas=nullptr;pair_transform=nullptr;
        }
        record.renderer_input_family=reinterpret_cast<uintptr_t>(family);
        record.renderer_sources=input.family_views;
        if(input.valid&128)record.valid|=2048;
        record.clone_source=input.family_views[0];
        if(record.clone_source && safe_copy(record.source_matrices.data(),reinterpret_cast<void*>(record.clone_source+0x70),128))record.valid|=256;
    }
    void* result=nullptr;
    bool handled=submit_pair_enabled && observe && family==submitting_family && record.caller==0x3a1595 && recording.load() &&
        try_submit_pair(destination,family,canvas,transform,record,result);
    if(!handled)result=original_renderer(destination,family,canvas,transform);
    if(handled)record.test_state=77;
    if(observe){
        record.result=reinterpret_cast<uintptr_t>(result);
        inspect_completed_renderer(record,destination);
        record.duration=now()-record.time;enqueue(record);
    }
    return result;
}
void* __fastcall hook_scene(void* local,void* family,Vec* position,Rot* rotation,void* viewport,void* drawer) {
    Scope scope;auto record=start_record(1,local,_ReturnAddress());
    void* saved_local=scene_local;Rot* saved_rotation=scene_rotation;int saved_delta=scene_delta;
    unsigned saved_phase=scene_phase;Vec saved_translation=scene_translation;Rot saved_rotation_delta=scene_rotation_delta;
    scene_local=local;scene_rotation=rotation;scene_delta=0;
    scene_phase=0;scene_translation={};scene_rotation_delta={};
    void* result=original_scene(local,family,position,rotation,viewport,drawer);
    if(record.object==expected_local)trace_rectangle(0,reinterpret_cast<uintptr_t>(result));
    if(family_diagnostics)inspect_family(record,family);
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
void inspect_swapchain(Record& record,IDXGISwapChain* chain) {
    if(!gpu_diagnostics || !recording.load())return;
    DXGI_SWAP_CHAIN_DESC desc{};
    if(SUCCEEDED(chain->GetDesc(&desc))) {
        record.width=desc.BufferDesc.Width;record.height=desc.BufferDesc.Height;
        record.format=desc.BufferDesc.Format;record.buffer_count=desc.BufferCount;record.swap_flags=desc.Flags;
        record.gpu_window=reinterpret_cast<uintptr_t>(desc.OutputWindow);
    }
    ID3D12Device* device{};
    if(SUCCEEDED(chain->GetDevice(IID_PPV_ARGS(&device)))) {
        record.gpu_device=reinterpret_cast<uintptr_t>(device);device->Release();record.valid|=128;
    }
    IDXGISwapChain3* current{};
    if(SUCCEEDED(chain->QueryInterface(IID_PPV_ARGS(&current)))) {
        record.buffer_index=current->GetCurrentBackBufferIndex();current->Release();
        ID3D12Resource* buffer{};
        if(SUCCEEDED(chain->GetBuffer(record.buffer_index,IID_PPV_ARGS(&buffer)))) {
            record.gpu_buffer=reinterpret_cast<uintptr_t>(buffer);buffer->Release();record.valid|=256;
        }
    }
}
void STDMETHODCALLTYPE hook_execute(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList* const* lists) {
    Scope scope;auto record=start_record(5,queue,_ReturnAddress());
    if(gpu_diagnostics && recording.load()) {
        record.queue_type=queue->GetDesc().Type;record.command_count=count;
        ID3D12Device* device{};
        if(SUCCEEDED(queue->GetDevice(IID_PPV_ARGS(&device)))) {
            record.gpu_device=reinterpret_cast<uintptr_t>(device);device->Release();record.valid|=128;
        }
    }
    original_execute(queue,count,lists);record.duration=now()-record.time;enqueue(record);
}
HRESULT STDMETHODCALLTYPE hook_present(IDXGISwapChain* chain,UINT interval,UINT flags) {
    Scope scope;auto record=start_record(3,chain,_ReturnAddress());
    inspect_swapchain(record,chain);
    HRESULT result=original_present(chain,interval,flags);
    record.duration=now()-record.time;record.result=static_cast<unsigned long>(result);
    if(!(flags&DXGI_PRESENT_TEST)) record.valid|=32;
    enqueue(record);return result;
}
HRESULT STDMETHODCALLTYPE hook_present1(IDXGISwapChain1* chain,UINT interval,UINT flags,const DXGI_PRESENT_PARAMETERS* params) {
    Scope scope;auto record=start_record(4,chain,_ReturnAddress());
    inspect_swapchain(record,chain);
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
            fprintf(file,",%u,%llu,0x%llx,0x%llx,0x%llx,%u,%u,%u,%u,%u,%u,%u,%u",r.pose_status,r.pose_sequence,
                static_cast<unsigned long long>(r.gpu_device),static_cast<unsigned long long>(r.gpu_buffer),static_cast<unsigned long long>(r.gpu_window),
                r.queue_type,r.command_count,r.buffer_index,r.buffer_count,r.width,r.height,r.format,r.swap_flags);
            fprintf(file,",0x%llx,0x%llx,%d,%d",static_cast<unsigned long long>(r.family_address),static_cast<unsigned long long>(r.family_array),r.family_count,r.family_capacity);
            for(auto view:r.family_views)fprintf(file,",0x%llx",static_cast<unsigned long long>(view));
            fprintf(file,",0x%llx,0x%llx",static_cast<unsigned long long>(r.clone_source),static_cast<unsigned long long>(r.clone_owner));
            for(float value:r.source_matrices)fprintf(file,",%.9g",value);
            fprintf(file,",0x%llx,0x%llx,%d,%d",static_cast<unsigned long long>(r.renderer_input_family),static_cast<unsigned long long>(r.renderer_storage),r.renderer_count,r.renderer_capacity);
            for(auto view:r.renderer_sources)fprintf(file,",0x%llx",static_cast<unsigned long long>(view));fprintf(file,"\n");
        }
        fflush(file);
    }
}
bool install(void* target,void* detour,void** original) {
    MH_STATUS created=MH_CreateHook(target,detour,original);
    if(created==MH_ERROR_ALREADY_CREATED && *original &&
         ((reservation_ready && (target==scene_target || target==event_target || target==family_target || target==clone_target || target==renderer_target || target==consume_target || target==destroy_renderer_target || target==prepare_target || target==state_delete_target)) ||
          (prime_ready.load() && std::find(prime_targets.begin(),prime_targets.end(),target)!=prime_targets.end())))created=MH_OK;
    MH_STATUS queued=created==MH_OK?MH_QueueEnableHook(target):MH_ERROR_NOT_CREATED;
    if(diagnostic_status)fprintf(diagnostic_status,"hook_target=%p create=%s queue=%s\n",target,MH_StatusToString(created),MH_StatusToString(queued));
    return created==MH_OK && queued==MH_OK;
}
HRESULT STDMETHODCALLTYPE hook_reset_list(ID3D12GraphicsCommandList* list,ID3D12CommandAllocator* allocator,ID3D12PipelineState* state) {
    Scope scope;HRESULT result=original_reset_list(list,allocator,state);
    if(recording.load()&&SUCCEEDED(result))viewport_stats.reset(reinterpret_cast<uintptr_t>(list));
    return result;
}
void STDMETHODCALLTYPE hook_viewports(ID3D12GraphicsCommandList* list,UINT count,const D3D12_VIEWPORT* values) {
    Scope scope;
    if(recording.load()) {
        D3D12_VIEWPORT value{};unsigned category=gears_viewport::unknown;
        if(count==1&&safe_copy(&value,values,sizeof(value)))category=gears_viewport::classify(value.TopLeftX,value.TopLeftY,value.Width,value.Height);
        else if(count>1)category=gears_viewport::other;
        viewport_stats.set(reinterpret_cast<uintptr_t>(list),category,false,
            {value.TopLeftX,value.TopLeftY,value.Width,value.Height});
    }
    original_viewports(list,count,values);
}
void STDMETHODCALLTYPE hook_scissors(ID3D12GraphicsCommandList* list,UINT count,const D3D12_RECT* values) {
    Scope scope;
    if(recording.load()) {
        D3D12_RECT value{};unsigned category=gears_viewport::unknown;
        if(count==1&&safe_copy(&value,values,sizeof(value)))category=gears_viewport::classify(static_cast<float>(value.left),static_cast<float>(value.top),
            static_cast<float>(static_cast<int64_t>(value.right)-value.left),static_cast<float>(static_cast<int64_t>(value.bottom)-value.top));
        else if(count>1)category=gears_viewport::other;
        viewport_stats.set(reinterpret_cast<uintptr_t>(list),category,true,
            {static_cast<float>(value.left),static_cast<float>(value.top),
             static_cast<float>(static_cast<int64_t>(value.right)-value.left),
             static_cast<float>(static_cast<int64_t>(value.bottom)-value.top)});
    }
    original_scissors(list,count,values);
}
void observe_output_stack(ID3D12GraphicsCommandList* list,unsigned kind) {
    if(!trial_draw_scope||output_stack_attempts.load()>=64)return;
    auto binding=viewport_stats.binding(reinterpret_cast<uintptr_t>(list));
    if(binding.viewport!=gears_viewport::full||binding.targets!=1||!binding.target.resource||
       binding.target.width!=1920||binding.target.height!=1080||binding.target.format!=24)return;
    unsigned attempt=output_stack_attempts.load();
    do {if(attempt>=64)return;}while(!output_stack_attempts.compare_exchange_weak(attempt,attempt+1));
    OutputStack value{};value.target=binding.target;value.kind=kind;
    value.count=CaptureStackBackTrace(0,static_cast<DWORD>(value.frames.size()),value.frames.data(),nullptr);
    if(!value.count)return;
    std::lock_guard<std::mutex> lock(output_stack_mutex);
    for(auto& entry:output_stacks){
        if(!entry.count){entry=value;return;}
        if(entry.target==value.target&&entry.kind==value.kind&&entry.count==value.count&&entry.frames==value.frames)return;
    }
}
void STDMETHODCALLTYPE hook_draw(ID3D12GraphicsCommandList* list,UINT vertices,UINT instances,UINT start,UINT first_instance) {
    Scope scope;if(recording.load())viewport_stats.draw(reinterpret_cast<uintptr_t>(list),trial_draw_scope?1:0);
    if(recording.load())observe_output_stack(list,0);
    original_draw(list,vertices,instances,start,first_instance);
}
void STDMETHODCALLTYPE hook_draw_indexed(ID3D12GraphicsCommandList* list,UINT indices,UINT instances,UINT start,INT vertex,UINT first_instance) {
    Scope scope;if(recording.load())viewport_stats.draw(reinterpret_cast<uintptr_t>(list),trial_draw_scope?1:0);
    if(recording.load())observe_output_stack(list,1);
    original_draw_indexed(list,indices,instances,start,vertex,first_instance);
}
void STDMETHODCALLTYPE hook_targets(ID3D12GraphicsCommandList* list,UINT count,const D3D12_CPU_DESCRIPTOR_HANDLE* handles,BOOL single,const D3D12_CPU_DESCRIPTOR_HANDLE* depth) {
    Scope scope;
    if(recording.load()){D3D12_CPU_DESCRIPTOR_HANDLE first{};
        if(count)safe_copy(&first,handles,sizeof(first));
        viewport_stats.bind(reinterpret_cast<uintptr_t>(list),count,first.ptr);}
    original_targets(list,count,handles,single,depth);
}
void STDMETHODCALLTYPE hook_create_rtv(ID3D12Device* device,ID3D12Resource* resource,const D3D12_RENDER_TARGET_VIEW_DESC* view,D3D12_CPU_DESCRIPTOR_HANDLE handle) {
    Scope scope;original_create_rtv(device,resource,view,handle);
    if(recording.load()||prime_recording.load()){
        gears_viewport::Target target{};D3D12_RENDER_TARGET_VIEW_DESC description{};
        bool valid=!view||safe_copy(&description,view,sizeof(description));
        unsigned mip=0;
        if(view&&valid)switch(description.ViewDimension){
            case D3D12_RTV_DIMENSION_TEXTURE1D:mip=description.Texture1D.MipSlice;break;
            case D3D12_RTV_DIMENSION_TEXTURE1DARRAY:mip=description.Texture1DArray.MipSlice;break;
            case D3D12_RTV_DIMENSION_TEXTURE2D:mip=description.Texture2D.MipSlice;break;
            case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:mip=description.Texture2DArray.MipSlice;break;
            case D3D12_RTV_DIMENSION_TEXTURE3D:mip=description.Texture3D.MipSlice;break;
            case D3D12_RTV_DIMENSION_TEXTURE2DMS:case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY:break;
            default:valid=false;
        }
        if(resource&&valid){auto desc=resource->GetDesc();
            if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER&&mip<desc.MipLevels&&mip<32)
                target={reinterpret_cast<uintptr_t>(resource),std::max<uint64_t>(1,desc.Width>>mip),
                    std::max<UINT>(1,desc.Height>>mip),static_cast<unsigned>(view?description.Format:desc.Format),mip};}
        viewport_stats.create(handle.ptr,target);
    }
}
bool flatten_descriptors(UINT count,const D3D12_CPU_DESCRIPTOR_HANDLE* starts,const UINT* sizes,UINT stride,std::array<uintptr_t,128>& output,unsigned& total) {
    total=0;if(count>output.size())return false;
    for(UINT i=0;i<count;++i){D3D12_CPU_DESCRIPTOR_HANDLE start{};UINT size=1;
        if(!safe_copy(&start,starts+i,sizeof(start))||(sizes&&!safe_copy(&size,sizes+i,sizeof(size)))||size>output.size()-total)return false;
        for(UINT j=0;j<size;++j){uint64_t offset=static_cast<uint64_t>(j)*stride;
            if(offset>UINTPTR_MAX-start.ptr)return false;output[total++]=start.ptr+static_cast<uintptr_t>(offset);}}
    return true;
}
void STDMETHODCALLTYPE hook_copy_descriptors(ID3D12Device* device,UINT destinations,const D3D12_CPU_DESCRIPTOR_HANDLE* destination_starts,const UINT* destination_sizes,
    UINT sources,const D3D12_CPU_DESCRIPTOR_HANDLE* source_starts,const UINT* source_sizes,D3D12_DESCRIPTOR_HEAP_TYPE type) {
    Scope scope;original_copy_descriptors(device,destinations,destination_starts,destination_sizes,sources,source_starts,source_sizes,type);
    if((recording.load()||prime_recording.load())&&type==D3D12_DESCRIPTOR_HEAP_TYPE_RTV){std::array<uintptr_t,128> dest{},src{};unsigned dc{},sc{};
        UINT stride=device->GetDescriptorHandleIncrementSize(type);
        if(flatten_descriptors(destinations,destination_starts,destination_sizes,stride,dest,dc)&&
           flatten_descriptors(sources,source_starts,source_sizes,stride,src,sc)&&dc==sc)viewport_stats.copy(dest.data(),src.data(),dc);
        else viewport_stats.reject_copy();}
}
void STDMETHODCALLTYPE hook_copy_simple(ID3D12Device* device,UINT count,D3D12_CPU_DESCRIPTOR_HANDLE dest,D3D12_CPU_DESCRIPTOR_HANDLE src,D3D12_DESCRIPTOR_HEAP_TYPE type) {
    Scope scope;original_copy_simple(device,count,dest,src,type);
    if((recording.load()||prime_recording.load())&&type==D3D12_DESCRIPTOR_HEAP_TYPE_RTV){std::array<uintptr_t,128> destinations{},sources{};unsigned dc{},sc{};
        UINT stride=device->GetDescriptorHandleIncrementSize(type);
        if(flatten_descriptors(1,&dest,&count,stride,destinations,dc)&&flatten_descriptors(1,&src,&count,stride,sources,sc))
            viewport_stats.copy(destinations.data(),sources.data(),dc);
        else viewport_stats.reject_copy();}
}
bool viewport_hooks(ID3D12Device* device) {
    ID3D12CommandAllocator* allocator{};ID3D12GraphicsCommandList* list{};bool success=false;
    if(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)))&&
       SUCCEEDED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator,nullptr,IID_PPV_ARGS(&list)))) {
        void** table=*reinterpret_cast<void***>(list);
        void** device_table=*reinterpret_cast<void***>(device);
        viewport_targets={table[10],table[21],table[22],table[12],table[13],table[46],device_table[20],device_table[23],device_table[24]};
        success=install(viewport_targets[0],reinterpret_cast<void*>(hook_reset_list),reinterpret_cast<void**>(&original_reset_list));
        success=install(viewport_targets[1],reinterpret_cast<void*>(hook_viewports),reinterpret_cast<void**>(&original_viewports))&&success;
        success=install(viewport_targets[2],reinterpret_cast<void*>(hook_scissors),reinterpret_cast<void**>(&original_scissors))&&success;
        success=install(viewport_targets[3],reinterpret_cast<void*>(hook_draw),reinterpret_cast<void**>(&original_draw))&&success;
        success=install(viewport_targets[4],reinterpret_cast<void*>(hook_draw_indexed),reinterpret_cast<void**>(&original_draw_indexed))&&success;
        success=install(viewport_targets[5],reinterpret_cast<void*>(hook_targets),reinterpret_cast<void**>(&original_targets))&&success;
        success=install(viewport_targets[6],reinterpret_cast<void*>(hook_create_rtv),reinterpret_cast<void**>(&original_create_rtv))&&success;
        success=install(viewport_targets[7],reinterpret_cast<void*>(hook_copy_descriptors),reinterpret_cast<void**>(&original_copy_descriptors))&&success;
        success=install(viewport_targets[8],reinterpret_cast<void*>(hook_copy_simple),reinterpret_cast<void**>(&original_copy_simple))&&success;
        list->Close();
    }
    if(list)list->Release();if(allocator)allocator->Release();return success;
}
bool present_hooks() {
    IDXGIFactory2* factory{};ID3D12Device* device{};ID3D12CommandQueue* queue{};IDXGISwapChain1* chain{};
    bool success=false;
    if(SUCCEEDED(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory))) &&
       SUCCEEDED(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)))) {
        D3D12_COMMAND_QUEUE_DESC q{};
        if(SUCCEEDED(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)))) {
            bool execute_ok=true;
              if(viewport_diagnostics)execute_ok=viewport_hooks(device);
            if(gpu_diagnostics) {
                execute_target=(*reinterpret_cast<void***>(queue))[10];
                  execute_ok=install(execute_target,reinterpret_cast<void*>(hook_execute),reinterpret_cast<void**>(&original_execute))&&execute_ok;
            }
            DXGI_SWAP_CHAIN_DESC1 desc{};desc.Width=16;desc.Height=16;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count=1;desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.BufferCount=2;
            desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;desc.AlphaMode=DXGI_ALPHA_MODE_PREMULTIPLIED;
            if(SUCCEEDED(factory->CreateSwapChainForComposition(queue,&desc,nullptr,&chain))) {
                void** table=*reinterpret_cast<void***>(chain);
                present_target=table[8];present1_target=table[22];
                success=install(present_target,reinterpret_cast<void*>(hook_present),reinterpret_cast<void**>(&original_present));
                if(present1_target!=present_target)
                    success=install(present1_target,reinterpret_cast<void*>(hook_present1),reinterpret_cast<void**>(&original_present1)) && success;
                success=success && execute_ok;
            }
        }
    }
    if(chain) chain->Release();if(queue) queue->Release();if(device) device->Release();if(factory) factory->Release();
    return success;
}
DWORD WINAPI target_prime_watchdog(void* argument) {
    FILE* status=static_cast<FILE*>(argument);
    WaitForSingleObject(prime_event,180000);
    std::lock_guard<std::mutex> lock(prime_mutex);
    if(prime_handoff.load()){
        auto stats=viewport_stats.snapshot();
        fprintf(status,"handed_off creates=%llu copies=%llu evictions=%llu\n",stats.descriptor_creates,stats.descriptor_copies,stats.descriptor_evictions);
    }else{
        prime_recording=false;bool disabled=true;
        for(auto target:prime_targets){auto result=MH_DisableHook(target);disabled=disabled&&(result==MH_OK||result==MH_ERROR_DISABLED);}
        ULONGLONG deadline=GetTickCount64()+1000;while(active.load()&&GetTickCount64()<deadline)Sleep(1);
        bool restored=true;std::array<unsigned char,32> bytes{};
        for(unsigned i=0;i<prime_targets.size();++i)
            restored=restored&&safe_copy(bytes.data(),prime_targets[i],bytes.size())&&!memcmp(bytes.data(),prime_original_bytes[i].data(),bytes.size());
        fprintf(status,"stopped disable_status=%u active=%u entries_match=%u\n",disabled?0:1,active.load(),restored?1:0);
    }
    fclose(status);prime_done=true;return 0;
}
DWORD begin_target_prime() {
    std::lock_guard<std::mutex> lock(prime_mutex);
    if(prime_started.exchange(1)||started.load())return ERROR_ALREADY_EXISTS;
    wchar_t path[MAX_PATH];GetModuleFileNameW(own_module,path,MAX_PATH);auto slash=wcsrchr(path,L'\\');
    if(!slash)return ERROR_BAD_PATHNAME;
    wcscpy_s(slash+1,MAX_PATH-static_cast<size_t>(slash+1-path),L"target-prime-status.txt");
    FILE* status=_wfsopen(path,L"w",_SH_DENYWR);if(!status)return ERROR_WRITE_FAULT;
    ID3D12Device* device{};
    if(FAILED(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)))){fclose(status);return ERROR_NOT_SUPPORTED;}
    auto table=*reinterpret_cast<void***>(device);prime_targets={table[20],table[23],table[24]};device->Release();
    std::array<void*,3> detours{reinterpret_cast<void*>(hook_create_rtv),reinterpret_cast<void*>(hook_copy_descriptors),reinterpret_cast<void*>(hook_copy_simple)};
    std::array<void**,3> originals{reinterpret_cast<void**>(&original_create_rtv),reinterpret_cast<void**>(&original_copy_descriptors),reinterpret_cast<void**>(&original_copy_simple)};
    bool ok=true;
    for(unsigned i=0;i<prime_targets.size();++i){
        if(!safe_copy(prime_original_bytes[i].data(),prime_targets[i],32)){ok=false;break;}
        auto created=MH_CreateHook(prime_targets[i],detours[i],originals[i]);
        auto queued=created==MH_OK?MH_QueueEnableHook(prime_targets[i]):MH_ERROR_NOT_CREATED;
        fprintf(status,"hook_target=%p create=%s queue=%s\n",prime_targets[i],MH_StatusToString(created),MH_StatusToString(queued));
        if(created!=MH_OK||queued!=MH_OK){ok=false;break;}
    }
    wchar_t name[128];swprintf_s(name,L"Local\\GearsVR.Capture.TargetPrime.%lu",GetCurrentProcessId());
    if(ok)prime_event=CreateEventW(nullptr,TRUE,FALSE,name);
    ok=ok&&prime_event;
    if(ok){prime_recording=true;ok=MH_ApplyQueued()==MH_OK;}
    HANDLE thread{};
    if(ok){thread=CreateThread(nullptr,0,target_prime_watchdog,status,0,nullptr);ok=thread!=nullptr;}
    if(!ok){prime_recording=false;for(auto target:prime_targets)MH_DisableHook(target);fprintf(status,"prime_setup_failed\n");fclose(status);return ERROR_INVALID_STATE;}
    fprintf(status,"capturing pid=%lu seconds=180 native_hooks_enabled=0 descriptor_hooks_enabled=3\n",GetCurrentProcessId());fflush(status);
    prime_ready=true;CloseHandle(thread);return 0;
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
    gpu_diagnostics=GetPrivateProfileIntW(L"Capture",L"gpu_diagnostics",0,ini)==1;
      viewport_diagnostics=GetPrivateProfileIntW(L"Capture",L"viewport_diagnostics",0,ini)==1;
      if(viewport_diagnostics&&!gpu_diagnostics){fclose(status);return 4;}
    family_diagnostics=GetPrivateProfileIntW(L"Capture",L"family_diagnostics",0,ini)==1;
    shadow_view_enabled=GetPrivateProfileIntW(L"Capture",L"shadow_view",0,ini)==1;
    constructor_shadow_enabled=GetPrivateProfileIntW(L"Capture",L"shadow_constructor",0,ini)==1;
    constructor_state_enabled=GetPrivateProfileIntW(L"Capture",L"constructor_state",0,ini)==1;
    constructor_eye_enabled=GetPrivateProfileIntW(L"Capture",L"constructor_eye",0,ini)==1;
    renderer_pair_enabled=GetPrivateProfileIntW(L"Capture",L"renderer_pair",0,ini)==1;
    submit_pair_enabled=GetPrivateProfileIntW(L"Capture",L"submit_pair",0,ini)==1;
    submit_output_bounds=GetPrivateProfileIntW(L"Capture",L"submit_output_bounds",0,ini)==1;
    submit_mono_control=GetPrivateProfileIntW(L"Capture",L"submit_mono_control",0,ini)==1;
    submit_mono_independent_state=GetPrivateProfileIntW(L"Capture",L"submit_mono_independent_state",0,ini)==1;
    if(submit_mono_independent_state && !submit_mono_control)return 4;
    if(submit_mono_control && submit_output_bounds)return 4;
    submit_duplicate_control=GetPrivateProfileIntW(L"Capture",L"submit_duplicate_control",0,ini)==1;
    if(submit_duplicate_control && (submit_mono_control || submit_output_bounds))return 4;
    submit_pair_seconds=GetPrivateProfileIntW(L"Capture",L"submit_pair_seconds",0,ini);
    if(submit_pair_seconds>60)return 4;
    if(submit_pair_enabled && (!constructor_eye_enabled || renderer_pair_enabled)){fprintf(status,"rejected: submit pair requires exclusive shifted-eye mode\n");fclose(status);return 3;}
    if(renderer_pair_enabled && !constructor_eye_enabled){fprintf(status,"rejected: renderer pair requires independent shifted eye\n");fclose(status);return 3;}
    if(constructor_eye_enabled && !constructor_state_enabled){fprintf(status,"rejected: eye requires independent constructor state\n");fclose(status);return 3;}
    if(constructor_state_enabled && !constructor_shadow_enabled){fprintf(status,"rejected: independent constructor state requires full constructor mode\n");fclose(status);return 3;}
    if(constructor_shadow_enabled && !shadow_view_enabled){fprintf(status,"rejected: constructor requires shadow view mode\n");fclose(status);return 3;}
    shadow_state_enabled=GetPrivateProfileIntW(L"Capture",L"shadow_state",0,ini)==1;
    if(shadow_state_enabled && (!family_diagnostics || (shadow_view_enabled && !constructor_state_enabled))){fprintf(status,"rejected: shadow state requires exclusive family diagnostics\n");fclose(status);return 3;}
    if(constructor_state_enabled && !shadow_state_enabled){fprintf(status,"rejected: independent constructor state requires lifetime observation\n");fclose(status);return 3;}
    if(shadow_view_enabled && !family_diagnostics){fprintf(status,"rejected: shadow copy requires family diagnostics\n");fclose(status);return 3;}
    unsigned observer_overlap=viewport_diagnostics&&submit_pair_enabled&&gpu_diagnostics&&family_diagnostics?1:0;
    if(int(wobble_enabled)+int(sweep_enabled)+int(live_enabled)+int(gpu_diagnostics)+int(family_diagnostics)-observer_overlap>1) {fprintf(status,"rejected: mutually exclusive capture modes\n");fclose(status);return 3;}
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
    if(family_diagnostics){
        family_target=reinterpret_cast<void*>(base+0x3a148c);
        const unsigned char signature[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x18,0x55};
        if(!safe_copy(bytes,family_target,sizeof(signature)) || memcmp(bytes,signature,sizeof(signature))){fprintf(status,"rejected: family candidate prologue mismatch\n");fclose(status);return 4;}
        clone_target=reinterpret_cast<void*>(base+0x193d50);
        const unsigned char clone_signature[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x10,0x48,0x89,0x68,0x18,0x48,0x89,0x70,0x20};
        if(!safe_copy(bytes,clone_target,sizeof(clone_signature)) || memcmp(bytes,clone_signature,sizeof(clone_signature))){fprintf(status,"rejected: clone candidate prologue mismatch\n");fclose(status);return 4;}
        renderer_target=reinterpret_cast<void*>(base+0x116250);
        const unsigned char signature_renderer[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x18,0x48,0x89,0x70,0x20,0x57,0x41,0x54};
        if(!safe_copy(bytes,renderer_target,sizeof(signature_renderer)) || memcmp(bytes,signature_renderer,sizeof(signature_renderer))){fprintf(status,"rejected: renderer prologue mismatch\n");fclose(status);return 4;}
        consume_target=reinterpret_cast<void*>(base+0x15a80c);destroy_renderer_target=reinterpret_cast<void*>(base+0x15a974);
        const unsigned char signature_consume[]={0x48,0x89,0x5c,0x24,0x10,0x55,0x48,0x8b,0xec,0x48,0x83,0xec,0x70};
        const unsigned char signature_destroy[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xd9};
        if(!safe_copy(bytes,consume_target,sizeof(signature_consume)) || memcmp(bytes,signature_consume,sizeof(signature_consume)) ||
           !safe_copy(bytes,destroy_renderer_target,sizeof(signature_destroy)) || memcmp(bytes,signature_destroy,sizeof(signature_destroy))){fprintf(status,"rejected: consume/destruction prologue mismatch\n");fclose(status);return 4;}
        prepare_target=reinterpret_cast<void*>(base+0x1fc060);
        const unsigned char signature_prepare[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x10,0x48,0x89,0x70,0x18,0x57,0x41,0x54};
        if(!safe_copy(bytes,prepare_target,sizeof(signature_prepare)) || memcmp(bytes,signature_prepare,sizeof(signature_prepare))){fprintf(status,"rejected: preparation prologue mismatch\n");fclose(status);return 4;}
    }
    if(shadow_view_enabled) {
        struct Gate {uintptr_t rva;unsigned char bytes[16];unsigned size;};
        const Gate gates[]={
            {0x1c03c8,{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57},16},
            {0x194454,{0x48,0x89,0x5c,0x24,0x18,0x48,0x89,0x4c,0x24,0x08,0x56,0x57,0x41,0x56,0x48,0x83},16},
            {0x15ca7c,{0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0xf9,0x48,0x8b,0x89},16},
            {0x53ea50,{0xe9,0xab,0xe8,0xb9,0xff},5}};
        for(const auto& gate:gates)if(!safe_copy(bytes,reinterpret_cast<void*>(base+gate.rva),gate.size) || memcmp(bytes,gate.bytes,gate.size)) {
            fprintf(status,"rejected: source copy lifetime signature mismatch rva=0x%llx\n",static_cast<unsigned long long>(gate.rva));fclose(status);return 4;
        }
        shadow_api={reinterpret_cast<gears_shadow::Allocate>(base+0x1c03c8),reinterpret_cast<gears_shadow::Copy>(base+0x194454),
            reinterpret_cast<gears_shadow::Destroy>(base+0x15ca7c),reinterpret_cast<gears_shadow::Free>(base+0x53ea50),safe_copy};
        if(constructor_shadow_enabled) {
            const unsigned char signature[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x10,0x48,0x89,0x70,0x18,0x48,0x89,0x78,0x20};
            source_constructor=reinterpret_cast<gears_source_args::Constructor>(base+0x15e428);
            if(!safe_copy(bytes,reinterpret_cast<void*>(source_constructor),sizeof(signature)) || memcmp(bytes,signature,sizeof(signature))) {
                fprintf(status,"rejected: full source constructor signature mismatch\n");fclose(status);return 4;
            }
            shadow_api.copy=construct_from_snapshot;
        }
    }
    if(shadow_state_enabled) {
        const unsigned char factory_bytes[]={0x48,0x83,0xec,0x28,0xba,0x08,0,0,0,0xb9,0x80,0x02,0,0};
        const unsigned char release_bytes[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xc1,0x48,0x8d,0x51,0x1c,0x48,0x8b,0xd9};
        const unsigned char delete_bytes[]={0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x8b,0xda,0x48,0x8b,0xf9};
        state_factory=reinterpret_cast<StateFactoryFn>(base+0x33bdd4);state_release=reinterpret_cast<ConsumeFn>(base+0x4ce80c);
        state_delete_target=reinterpret_cast<void*>(base+0x33bc70);state_expected_table=base+0x1353918;
        if(!safe_copy(bytes,reinterpret_cast<void*>(state_factory),sizeof(factory_bytes)) || memcmp(bytes,factory_bytes,sizeof(factory_bytes)) ||
           !safe_copy(bytes,reinterpret_cast<void*>(state_release),sizeof(release_bytes)) || memcmp(bytes,release_bytes,sizeof(release_bytes)) ||
           !safe_copy(bytes,state_delete_target,sizeof(delete_bytes)) || memcmp(bytes,delete_bytes,sizeof(delete_bytes))) {
            fprintf(status,"rejected: view state lifetime signature mismatch\n");fclose(status);return 4;
        }
    }
    FILE* file{};_wfopen_s(&file,csv,L"w");
    if(!file) {fclose(status);return 5;}
    fprintf(file,"kind,thread,focused,valid,qpc,duration_qpc,object,result,caller_rva,x,y,z,pitch,yaw,roll,camera_timestamp,source_yaw,yaw_delta,cache_yaw,test_state");
    for(unsigned i=0;i<32;++i) fprintf(file,",matrix%u",i);
    fprintf(file,",pose_phase,source_x,source_y,source_z,source_pitch,source_rot_yaw,source_roll,delta_x,delta_y,delta_z,delta_pitch,delta_rot_yaw,delta_roll,pose_status,pose_sequence,gpu_device,gpu_buffer,gpu_window,queue_type,command_count,buffer_index,buffer_count,width,height,format,swap_flags,family_address,family_array,family_count,family_capacity,family_view0,family_view1,family_view2,family_view3,clone_source,clone_owner");
    for(unsigned i=0;i<32;++i)fprintf(file,",source_matrix%u",i);
    fprintf(file,",renderer_input_family,renderer_storage,renderer_count,renderer_capacity,renderer_source0,renderer_source1,renderer_source2,renderer_source3\n");fflush(file);
    LARGE_INTEGER frequency;QueryPerformanceFrequency(&frequency);
    qpc_frequency=frequency.QuadPart;
    fprintf(status,"qpc_frequency=%lld\n",frequency.QuadPart);
    fprintf(status,"wobble_enabled=%d amplitude_degrees=1.5 baseline_seconds=3 wave_seconds=12\n",wobble_enabled);
    fprintf(status,"sweep_enabled=%d rotation_degrees=1.5 translation_units=2 baseline_seconds=3 axis_seconds=4 total_seconds=27\n",sweep_enabled);
    fprintf(status,"live_head_enabled=%d rotation_limit_degrees=5 translation_limit_units=2 max_age_ms=100 F8=abort F9=recenter\n",live_enabled);
    fprintf(status,"gpu_diagnostics=%d\n",gpu_diagnostics);
    fprintf(status,"family_diagnostics=%d candidate_rva=0x3a148c\n",family_diagnostics);
    MH_STATUS init=MH_Initialize();
    fprintf(status,"minhook_initialize=%s\n",MH_StatusToString(init));
    bool ok=(init==MH_OK || (reservation_ready && init==MH_ERROR_ALREADY_INITIALIZED)) && install(scene_target,reinterpret_cast<void*>(hook_scene),reinterpret_cast<void**>(&original_scene))
        && install(event_target,reinterpret_cast<void*>(hook_event),reinterpret_cast<void**>(&original_event));
    if(ok && family_diagnostics)ok=install(family_target,reinterpret_cast<void*>(hook_family),reinterpret_cast<void**>(&original_family));
    if(ok && family_diagnostics)ok=install(clone_target,reinterpret_cast<void*>(hook_clone),reinterpret_cast<void**>(&original_clone));
    if(ok && family_diagnostics)ok=install(renderer_target,reinterpret_cast<void*>(hook_renderer),reinterpret_cast<void**>(&original_renderer));
    if(ok && family_diagnostics)ok=install(consume_target,reinterpret_cast<void*>(hook_consume),reinterpret_cast<void**>(&original_consume)) &&
        install(destroy_renderer_target,reinterpret_cast<void*>(hook_destroy_renderer),reinterpret_cast<void**>(&original_destroy_renderer));
    if(ok && family_diagnostics)ok=install(prepare_target,reinterpret_cast<void*>(hook_prepare),reinterpret_cast<void**>(&original_prepare));
    if(ok && shadow_state_enabled)ok=install(state_delete_target,reinterpret_cast<void*>(hook_state_delete),reinterpret_cast<void**>(&original_state_delete));
    bool graphics=ok && present_hooks();
    if(gpu_diagnostics && !graphics)ok=false;
    fprintf(status,"present_hooks_created=%d\n",graphics);
    if(ok) {
        std::lock_guard<std::mutex> lock(prime_mutex);
        if(prime_ready.load()){prime_handoff=true;SetEvent(prime_event);}
        submit_not_before=now()+3*qpc_frequency;recording=true;prime_recording=false;
        MH_STATUS applied=MH_ApplyQueued();fprintf(status,"minhook_apply=%s\n",MH_StatusToString(applied));ok=applied==MH_OK;
    }
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
    fprintf(status,"stopped disable_status=%d active=%u dropped=%llu scene=%llu event=%llu present=%llu present1=%llu execute=%llu\n",
        disabled,active.load(),dropped.load(),emitted[1],emitted[2],emitted[3],emitted[4],emitted[5]);
    fprintf(status,"wobble_final_state=%u\n",wobble_state.load());
    fprintf(status,"family_calls=%llu\n",emitted[7]);
    fprintf(status,"renderer_consume_calls=%llu renderer_destroy_calls=%llu\n",emitted[10],emitted[11]);
      fprintf(status,"renderer_prepare_calls=%llu\n",emitted[12]);
      if(viewport_diagnostics) {
          auto stats=viewport_stats.snapshot();
          fprintf(status,"gpu_viewport resets=%llu viewports=%llu scissors=%llu overflow=%llu untracked_draws=%llu\n",
              stats.resets,stats.viewports,stats.scissors,stats.overflow,stats.untracked_draws);
          for(unsigned phase=0;phase<3;++phase)for(unsigned viewport=0;viewport<5;++viewport)for(unsigned scissor=0;scissor<5;++scissor)
              if(stats.draws[phase][viewport][scissor])fprintf(status,"gpu_draw phase=%u viewport=%u scissor=%u calls=%llu\n",phase,viewport,scissor,stats.draws[phase][viewport][scissor]);
          fprintf(status,"gpu_rect_overflow=%llu\n",stats.rect_overflow);
          for(const auto& entry:stats.trial_rects)if(entry.calls)
              fprintf(status,"gpu_trial_rect viewport=(%.9g,%.9g,%.9g,%.9g) scissor=(%.9g,%.9g,%.9g,%.9g) calls=%llu\n",
                  entry.viewport.x,entry.viewport.y,entry.viewport.width,entry.viewport.height,
                  entry.scissor.x,entry.scissor.y,entry.scissor.width,entry.scissor.height,entry.calls);
          fprintf(status,"gpu_targets binds=%llu creates=%llu copies=%llu evictions=%llu rejected_copies=%llu histogram_overflow=%llu\n",
              stats.target_binds,stats.descriptor_creates,stats.descriptor_copies,stats.descriptor_evictions,stats.copy_rejected,stats.target_overflow);
          for(const auto& entry:stats.trial_targets)if(entry.calls)
              fprintf(status,"gpu_trial_target viewport=%u targets=%u resource=%llx width=%llu height=%u format=%u mip=%u calls=%llu\n",
                  entry.viewport,entry.targets,static_cast<unsigned long long>(entry.target.resource),entry.target.width,
                  entry.target.height,entry.target.format,entry.target.mip,entry.calls);
          fprintf(status,"gpu_output_stack_attempts=%u limit=64 slots=8 frames=24\n",output_stack_attempts.load());
          for(unsigned i=0;i<output_stacks.size();++i){const auto& entry=output_stacks[i];if(!entry.count)continue;
              fprintf(status,"gpu_output_stack index=%u resource=%llx kind=%u count=%u\n",i,
                  static_cast<unsigned long long>(entry.target.resource),entry.kind,entry.count);
              for(unsigned j=0;j<entry.count;++j){HMODULE module{};wchar_t path[MAX_PATH]{};
                  if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                     reinterpret_cast<LPCWSTR>(entry.frames[j]),&module))GetModuleFileNameW(module,path,MAX_PATH);
                  const wchar_t* name=wcsrchr(path,L'\\');name=name?name+1:path;
                  fprintf(status,"gpu_output_frame index=%u frame=%u address=%llx module=%ls rva=%llx\n",i,j,
                      static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(entry.frames[j])),name,
                      static_cast<unsigned long long>(module?reinterpret_cast<uintptr_t>(entry.frames[j])-reinterpret_cast<uintptr_t>(module):0));
              }}
      }
    unsigned shadow_state=shadow_attempted.load();
    const auto completed_shadow=shadow_state==2?shadow_result:gears_shadow::Result{};
    fprintf(status,"shadow_view_enabled=%d attempted=%u stage=%u checks=%u nonempty_arrays=%u source=0x%llx temporary=0x%llx\n",
        shadow_view_enabled,shadow_state,completed_shadow.stage,completed_shadow.checks,completed_shadow.nonempty_arrays,
        static_cast<unsigned long long>(completed_shadow.source),static_cast<unsigned long long>(completed_shadow.temporary));
    fprintf(status,"constructor_shadow_enabled=%d derived_finite=%u derived_max_error=%.9g\n",constructor_shadow_enabled,
        shadow_state==2?constructor_derived_finite:0,shadow_state==2?constructor_derived_error:0.);
    fprintf(status,"constructor_state_enabled=%d\n",constructor_state_enabled);
    const auto pair_result=shadow_state==2?renderer_pair_result:gears_pair_shadow::Result{};
    fprintf(status,"renderer_pair_enabled=%d checks=%u renderer=0x%llx retained=%d\n",renderer_pair_enabled,pair_result.checks,
        static_cast<unsigned long long>(pair_result.renderer),pair_result.retain_inputs);
    const unsigned submit_phase=submit_attempted.load();
    fprintf(status,"submit_pair_enabled=%d attempted=%u root=0x%llx owned=%u sources_released=%u original_preserved=%u consumed=%u prepared=%u destroyed=%u\n",
        submit_pair_enabled,submit_phase,static_cast<unsigned long long>(submit_root.load()),
        submit_phase==2?submit_owned_verified.load():0,submit_phase==2?submit_sources_released.load():0,submit_phase==2?submit_original_preserved.load():0,
        submit_consumed.load(),submit_prepared.load(),submit_destroyed.load());
    fprintf(status,"submit_inputs checks=%u rejection=%u rectangle=%.9g,%.9g,%.9g,%.9g\n",
        submit_phase>=2?submit_source_checks:0,submit_phase>=2?submit_rejection:0,
        submit_phase>=2?submit_rectangle[0]:0.,submit_phase>=2?submit_rectangle[1]:0.,
        submit_phase>=2?submit_rectangle[2]:0.,submit_phase>=2?submit_rectangle[3]:0.);
    fprintf(status,"submit_lifetime seconds=%u total=%u closed=%d pending=%u retained=%u skipped=%u\n",
        submit_pair_seconds,submit_total.load(),submitted_lifetime.is_closed(),submitted_lifetime.pending(),submit_retained.load(),submit_skipped.load());
    fprintf(status,"submit_output_bounds=%d\n",submit_output_bounds);
    fprintf(status,"submit_view_count=%u submit_event_marker=77\n",submit_mono_control?1:2);
    fprintf(status,"submit_duplicate_control=%d\n",submit_duplicate_control);
    fprintf(status,"submit_mono_independent_state=%d\n",submit_mono_independent_state);
    for(unsigned i=0;i<rectangle_traces.size();++i) {
        unsigned phase=rectangle_trace_phases[i].load();auto trace=phase==2?rectangle_traces[i]:RectangleTrace{};
        fprintf(status,"rectangle_trace index=%u phase=%u object=0x%llx source=%.9g,%.9g,%.9g,%.9g target=%d,%d,%d,%d\n",
            i,phase,static_cast<unsigned long long>(trace.object),trace.source[0],trace.source[1],trace.source[2],trace.source[3],
            trace.target[0],trace.target[1],trace.target[2],trace.target[3]);
        fprintf(status,"sampling_trace index=%u phase=%u uv=%.9g,%.9g,%.9g,%.9g buffer=%d,%d\n",
            i,phase,trace.sampling[0],trace.sampling[1],trace.sampling[2],trace.sampling[3],trace.buffer[0],trace.buffer[1]);
    }
    fprintf(status,"constructor_eye_enabled=%d shifted=%u combined_valid=%u combined_relative_error=%.9g\n",constructor_eye_enabled,
        shadow_state==2?constructor_eye_shifted:0,shadow_state==2?constructor_combined_valid:0,
        shadow_state==2?constructor_combined_relative_error:0.);
    unsigned state_done=state_destroyed.load();
    fprintf(status,"shadow_state_enabled=%d attempted=%u created=%u release_returned=%u destroyed=%u original_preserved=%u flags=%u thread=%u caller=0x%llx temporary=0x%llx\n",
        shadow_state_enabled,state_attempted.load(),state_created.load(),state_release_returned.load(),state_done,state_original_preserved.load(),
        state_done?state_delete_flags:0,state_done?state_delete_thread:0,
        static_cast<unsigned long long>(state_done?state_delete_caller:0),static_cast<unsigned long long>(state_temporary.load()));
    diagnostic_status=nullptr;fclose(file);fclose(status);if(stop_event) CloseHandle(stop_event);stop_event=nullptr;
    return disabled==MH_OK?0:7;
}
// Test real trampoline execution and six-argument forwarding before injection.
__declspec(noinline) void fake_event(void* pc,Vec* p,Rot* r) {p->x+=static_cast<float>(reinterpret_cast<uintptr_t>(pc));r->yaw+=7;}
struct FakeView {char prefix[0x70];float matrices[32];};
FakeView fake_view{};
unsigned clone_forward_count{};
unsigned renderer_forward_count{};
unsigned consume_forward_count{},destroy_forward_count{};
__declspec(noinline) void* fake_destroy_renderer(void* renderer){
    ++destroy_forward_count;VirtualFree(renderer,0,MEM_RELEASE);return renderer;
}
__declspec(noinline) void fake_consume(void* renderer){
    fake_destroy_renderer(renderer);++consume_forward_count;
}
FakeView fake_rendered_view{};
uintptr_t fake_rendered_pointer{};
unsigned prepare_forward_count{};
__declspec(noinline) void fake_prepare(void* renderer){
    ++prepare_forward_count;
    uintptr_t view{};memcpy(&view,reinterpret_cast<char*>(renderer)+0x68,8);
    float x=.125f,y=-.25f;uintptr_t state=0x2222;
    memcpy(reinterpret_cast<void*>(view+0xd0),&x,4);memcpy(reinterpret_cast<void*>(view+0xd4),&y,4);
    memcpy(reinterpret_cast<void*>(view+8),&state,8);
}
__declspec(noinline) void* fake_renderer(void* destination,void* family,void* canvas,void* transform){
    renderer_forward_count+=canvas==reinterpret_cast<void*>(5) && transform==reinterpret_cast<void*>(6)?1:100;
    uintptr_t array{};memcpy(&array,family,8);uintptr_t source{};memcpy(&source,reinterpret_cast<void*>(array),8);
    memcpy(&fake_rendered_view,reinterpret_cast<void*>(source),sizeof(FakeView));
    uintptr_t owner=reinterpret_cast<uintptr_t>(destination)+8;memcpy(fake_rendered_view.prefix,&owner,8);
    fake_rendered_pointer=reinterpret_cast<uintptr_t>(&fake_rendered_view);
    struct Header{uintptr_t data;int count,capacity;} pointers{reinterpret_cast<uintptr_t>(&fake_rendered_pointer),1,1},storage{fake_rendered_pointer,1,1};
    memcpy(reinterpret_cast<char*>(destination)+8,&pointers,16);memcpy(reinterpret_cast<char*>(destination)+0x68,&storage,16);
    return destination;
}
__declspec(noinline) void* fake_clone(void* destination,void* source){
    ++clone_forward_count;memcpy(destination,source,sizeof(FakeView));return destination;
}
unsigned family_forward_count{};
void* expected_test_family{};
__declspec(noinline) void fake_family(void* canvas,void* family){
    if(canvas==reinterpret_cast<void*>(1) && family==expected_test_family)++family_forward_count;
    else family_forward_count+=100;
}
bool arguments_good{};
unsigned fake_state_factory_calls{},fake_state_release_calls{},fake_state_delete_calls{};
void* fake_state_pending{};
__declspec(noinline) void* fake_state_factory() {
    ++fake_state_factory_calls;
    void* value=VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(value)memcpy(value,&state_expected_table,8);return value;
}
__declspec(noinline) void fake_state_release(void* value) {++fake_state_release_calls;fake_state_pending=value;}
__declspec(noinline) void* fake_state_delete(void* value,unsigned flags) {
    ++fake_state_delete_calls;if(flags&1)VirtualFree(value,0,MEM_RELEASE);return value;
}
DWORD WINAPI fake_state_queue(void* value) {
    StateDeleteFn volatile invoke=fake_state_delete;return invoke(value,1)==value?0:1;
}
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
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureReserve(void*) {
    // Create trampolines before level loading fills nearby address space.
    // MH_CreateHook alone does not patch or enable the game entry points.
    if(reservation_started.exchange(1) || started.load())return ERROR_ALREADY_EXISTS;
    if(!hash_matches())return ERROR_REVISION_MISMATCH;
    base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    scene_target=reinterpret_cast<void*>(base+0x15cf08);event_target=reinterpret_cast<void*>(base+0x15c968);family_target=reinterpret_cast<void*>(base+0x3a148c);
    clone_target=reinterpret_cast<void*>(base+0x193d50);
    renderer_target=reinterpret_cast<void*>(base+0x116250);
    consume_target=reinterpret_cast<void*>(base+0x15a80c);destroy_renderer_target=reinterpret_cast<void*>(base+0x15a974);
    const unsigned char a[]={0x48,0x8b,0xc4,0x53,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57};
    const unsigned char b[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x08,0x48,0x89,0x68,0x10,0x48,0x89,0x70,0x18};
    const unsigned char c[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x18,0x55};
    const unsigned char d[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x10,0x48,0x89,0x68,0x18,0x48,0x89,0x70,0x20};
    const unsigned char e[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x18,0x48,0x89,0x70,0x20,0x57,0x41,0x54};
    const unsigned char f[]={0x48,0x89,0x5c,0x24,0x10,0x55,0x48,0x8b,0xec,0x48,0x83,0xec,0x70};
    prepare_target=reinterpret_cast<void*>(base+0x1fc060);
    const unsigned char h[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x10,0x48,0x89,0x70,0x18,0x57,0x41,0x54};
    const unsigned char g[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xd9};
    state_delete_target=reinterpret_cast<void*>(base+0x33bc70);
    const unsigned char j[]={0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x8b,0xda,0x48,0x8b,0xf9};
    unsigned char read[16];
    if(!safe_copy(read,scene_target,sizeof(a)) || memcmp(read,a,sizeof(a)) ||
       !safe_copy(read,event_target,sizeof(b)) || memcmp(read,b,sizeof(b)) ||
       !safe_copy(read,family_target,sizeof(c)) || memcmp(read,c,sizeof(c)) ||
       !safe_copy(read,clone_target,sizeof(d)) || memcmp(read,d,sizeof(d)) ||
       !safe_copy(read,renderer_target,sizeof(e)) || memcmp(read,e,sizeof(e)) ||
       !safe_copy(read,consume_target,sizeof(f)) || memcmp(read,f,sizeof(f)) ||
       !safe_copy(read,destroy_renderer_target,sizeof(g)) || memcmp(read,g,sizeof(g)) ||
       !safe_copy(read,prepare_target,sizeof(h)) || memcmp(read,h,sizeof(h)) ||
       !safe_copy(read,state_delete_target,sizeof(j)) || memcmp(read,j,sizeof(j)))return ERROR_INVALID_DATA;
    if(MH_Initialize()!=MH_OK)return ERROR_INVALID_STATE;
    if(MH_CreateHook(scene_target,reinterpret_cast<void*>(hook_scene),reinterpret_cast<void**>(&original_scene))!=MH_OK ||
       MH_CreateHook(event_target,reinterpret_cast<void*>(hook_event),reinterpret_cast<void**>(&original_event))!=MH_OK ||
       MH_CreateHook(family_target,reinterpret_cast<void*>(hook_family),reinterpret_cast<void**>(&original_family))!=MH_OK ||
       MH_CreateHook(clone_target,reinterpret_cast<void*>(hook_clone),reinterpret_cast<void**>(&original_clone))!=MH_OK ||
       MH_CreateHook(renderer_target,reinterpret_cast<void*>(hook_renderer),reinterpret_cast<void**>(&original_renderer))!=MH_OK ||
       MH_CreateHook(consume_target,reinterpret_cast<void*>(hook_consume),reinterpret_cast<void**>(&original_consume))!=MH_OK ||
       MH_CreateHook(destroy_renderer_target,reinterpret_cast<void*>(hook_destroy_renderer),reinterpret_cast<void**>(&original_destroy_renderer))!=MH_OK ||
       MH_CreateHook(prepare_target,reinterpret_cast<void*>(hook_prepare),reinterpret_cast<void**>(&original_prepare))!=MH_OK ||
       MH_CreateHook(state_delete_target,reinterpret_cast<void*>(hook_state_delete),reinterpret_cast<void**>(&original_state_delete))!=MH_OK)return ERROR_NOT_ENOUGH_MEMORY;
    // Independently re-read the entries to confirm reservation left them alone.
    if(!safe_copy(read,scene_target,sizeof(a)) || memcmp(read,a,sizeof(a)) ||
       !safe_copy(read,event_target,sizeof(b)) || memcmp(read,b,sizeof(b)) ||
       !safe_copy(read,family_target,sizeof(c)) || memcmp(read,c,sizeof(c)) ||
       !safe_copy(read,clone_target,sizeof(d)) || memcmp(read,d,sizeof(d)) ||
       !safe_copy(read,renderer_target,sizeof(e)) || memcmp(read,e,sizeof(e)) ||
       !safe_copy(read,consume_target,sizeof(f)) || memcmp(read,f,sizeof(f)) ||
       !safe_copy(read,destroy_renderer_target,sizeof(g)) || memcmp(read,g,sizeof(g)) ||
       !safe_copy(read,prepare_target,sizeof(h)) || memcmp(read,h,sizeof(h)) ||
       !safe_copy(read,state_delete_target,sizeof(j)) || memcmp(read,j,sizeof(j)))return ERROR_INVALID_DATA;
    wchar_t name[128];swprintf_s(name,L"Local\\GearsVR.Capture.Reserved.%lu",GetCurrentProcessId());
    reservation_event=CreateEventW(nullptr,TRUE,TRUE,name);
    if(!reservation_event)return GetLastError();reservation_ready=true;return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureTargetPrime(void*) {
    if(!reservation_ready||started.load())return ERROR_INVALID_STATE;
    return begin_target_prime();
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureFamilyCheck(void*) {
    uintptr_t views[]={reinterpret_cast<uintptr_t>(&fake_view),reinterpret_cast<uintptr_t>(&fake_view)+16};
    struct Header{uintptr_t data;int count,capacity;} header{reinterpret_cast<uintptr_t>(views),2,2};
    expected_test_family=&header;unsigned char before[32],after[32];
    if(!safe_copy(before,reinterpret_cast<void*>(fake_family),32) || MH_Initialize()!=MH_OK)return 70;
    if(!install(reinterpret_cast<void*>(fake_family),reinterpret_cast<void*>(hook_family),reinterpret_cast<void**>(&original_family)))return 71;
    recording=true;if(MH_ApplyQueued()!=MH_OK)return 72;
    FamilyFn volatile invoke=fake_family;invoke(reinterpret_cast<void*>(1),&header);
    recording=false;if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK)return 73;
    if(!safe_copy(after,reinterpret_cast<void*>(fake_family),32) || memcmp(before,after,32) || active.load() || family_forward_count!=1 || write_index!=1)return 74;
    const auto& record=ring[0];
    if(record.kind!=7 || !(record.valid&128) || record.family_count!=2 || record.family_views[0]!=views[0] || record.family_views[1]!=views[1] || record.family_address!=reinterpret_cast<uintptr_t>(&header))return 75;
    Record invalid{};header.count=5;header.capacity=5;inspect_family(invalid,&header);if(invalid.valid&128)return 76;
    header.count=2;header.capacity=1;inspect_family(invalid,&header);if(invalid.valid&128)return 77;
    header.count=1;header.capacity=1;header.data=8;inspect_family(invalid,&header);if(invalid.valid&128)return 78;
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureCloneCheck(void*) {
    FakeView source{},destination{};uintptr_t owner=0x12345678;
    memcpy(source.prefix,&owner,8);for(unsigned i=0;i<32;++i)source.matrices[i]=float(i)+.25f;
    uintptr_t views[]={reinterpret_cast<uintptr_t>(&source)};
    struct Header{uintptr_t data;int count,capacity;} header{reinterpret_cast<uintptr_t>(views),1,17};
    unsigned char before[32],after[32];
    if(!safe_copy(before,reinterpret_cast<void*>(fake_clone),32) || MH_Initialize()!=MH_OK)return 80;
    if(!install(reinterpret_cast<void*>(fake_clone),reinterpret_cast<void*>(hook_clone),reinterpret_cast<void**>(&original_clone)))return 81;
    recording=true;if(MH_ApplyQueued()!=MH_OK)return 82;
    submitting_family=&header;CloneFn volatile invoke=fake_clone;void* result=invoke(&destination,&source);submitting_family=nullptr;
    // Unrelated calls must forward but must not generate a family observation.
    invoke(&destination,&source);
    recording=false;if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK)return 83;
    if(!safe_copy(after,reinterpret_cast<void*>(fake_clone),32) || memcmp(before,after,32) || active.load() || clone_forward_count!=2 || write_index!=1)return 84;
    const auto& record=ring[0];
    if(result!=&destination || record.kind!=8 || record.object!=reinterpret_cast<uintptr_t>(&destination) || record.result!=record.object || record.clone_source!=views[0] || record.clone_owner!=owner || record.valid!=(128|256|512|1024))return 85;
    if(memcmp(record.matrices.data(),source.matrices,128) || memcmp(record.source_matrices.data(),source.matrices,128) || memcmp(&source,&destination,sizeof(source)))return 86;
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureRendererCheck(void*) {
    FakeView source{};for(unsigned i=0;i<32;++i)source.matrices[i]=float(i)-.125f;
    uintptr_t views[]={reinterpret_cast<uintptr_t>(&source)};
    struct Header{uintptr_t data;int count,capacity;} header{reinterpret_cast<uintptr_t>(views),1,17};
    alignas(8) unsigned char destination[0x140]{};unsigned char before[32],after[32];
    if(!safe_copy(before,reinterpret_cast<void*>(fake_renderer),32) || MH_Initialize()!=MH_OK)return 90;
    if(!install(reinterpret_cast<void*>(fake_renderer),reinterpret_cast<void*>(hook_renderer),reinterpret_cast<void**>(&original_renderer)))return 91;
    recording=true;if(MH_ApplyQueued()!=MH_OK)return 92;
    submitting_family=&header;RendererFn volatile invoke=fake_renderer;
    void* result=invoke(destination,&header,reinterpret_cast<void*>(5),reinterpret_cast<void*>(6));submitting_family=nullptr;
    invoke(destination,&header,reinterpret_cast<void*>(5),reinterpret_cast<void*>(6));
    recording=false;if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK)return 93;
    if(!safe_copy(after,reinterpret_cast<void*>(fake_renderer),32) || memcmp(before,after,32) || active.load() || renderer_forward_count!=2 || write_index!=1)return 94;
    const auto& r=ring[0];uintptr_t owner=reinterpret_cast<uintptr_t>(destination)+8;
    if(result!=destination || r.kind!=9 || r.result!=r.object || r.valid!=(128|256|512|1024|2048|4096) || r.renderer_input_family!=reinterpret_cast<uintptr_t>(&header) || r.renderer_sources[0]!=views[0] || r.family_views[0]!=fake_rendered_pointer || r.renderer_storage!=fake_rendered_pointer || r.renderer_count!=1 || r.clone_owner!=owner || r.family_address!=owner)return 95;
    if(memcmp(r.matrices.data(),source.matrices,128) || memcmp(r.source_matrices.data(),source.matrices,128))return 96;
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureLifetimeCheck(void*) {
    FakeView source{};for(unsigned i=0;i<32;++i)source.matrices[i]=float(i)+.5f;
    uintptr_t views[]={reinterpret_cast<uintptr_t>(&source)};
    struct Header{uintptr_t data;int count,capacity;} header{reinterpret_cast<uintptr_t>(views),1,17};
    void* destination=VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!destination)return 100;
    fake_renderer(destination,&header,reinterpret_cast<void*>(5),reinterpret_cast<void*>(6));
    unsigned char consume_before[32],destroy_before[32],after[32];
    if(!safe_copy(consume_before,reinterpret_cast<void*>(fake_consume),32) || !safe_copy(destroy_before,reinterpret_cast<void*>(fake_destroy_renderer),32) || MH_Initialize()!=MH_OK)return 101;
    if(!install(reinterpret_cast<void*>(fake_consume),reinterpret_cast<void*>(hook_consume),reinterpret_cast<void**>(&original_consume)) ||
       !install(reinterpret_cast<void*>(fake_destroy_renderer),reinterpret_cast<void*>(hook_destroy_renderer),reinterpret_cast<void**>(&original_destroy_renderer)))return 102;
    recording=true;if(MH_ApplyQueued()!=MH_OK)return 103;
    ConsumeFn volatile invoke=fake_consume;invoke(destination);
    recording=false;if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK)return 104;
    if(!safe_copy(after,reinterpret_cast<void*>(fake_consume),32) || memcmp(after,consume_before,32) || !safe_copy(after,reinterpret_cast<void*>(fake_destroy_renderer),32) || memcmp(after,destroy_before,32) || active.load())return 105;
    if(consume_forward_count!=1 || destroy_forward_count!=1 || write_index!=2 || ring[0].kind!=11 || ring[1].kind!=10)return 106;
    // The original really releases the page. Observation must be pre-call.
    if(safe_copy(after,destination,8))return 107;
    for(unsigned i=0;i<2;++i){const auto& r=ring[i];
        if(r.object!=reinterpret_cast<uintptr_t>(destination) || r.valid!=(128|512|1024|4096) || r.family_views[0]!=fake_rendered_pointer || r.clone_owner!=reinterpret_cast<uintptr_t>(destination)+8 || memcmp(r.matrices.data(),source.matrices,128))return 108;
    }
    if(ring[0].result!=reinterpret_cast<uintptr_t>(destination))return 109;
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCapturePrepareCheck(void*) {
    FakeView source{};for(unsigned i=0;i<32;++i)source.matrices[i]=float(i)+.5f;
    uintptr_t initial_state=0x1111;memcpy(source.prefix+8,&initial_state,8);
    uintptr_t views[]={reinterpret_cast<uintptr_t>(&source)};
    struct Header{uintptr_t data;int count,capacity;} header{reinterpret_cast<uintptr_t>(views),1,17};
    alignas(8) unsigned char destination[0x140]{};fake_renderer(destination,&header,reinterpret_cast<void*>(5),reinterpret_cast<void*>(6));
    unsigned char before[32],after[32];
    if(!safe_copy(before,reinterpret_cast<void*>(fake_prepare),32) || MH_Initialize()!=MH_OK)return 110;
    if(!install(reinterpret_cast<void*>(fake_prepare),reinterpret_cast<void*>(hook_prepare),reinterpret_cast<void**>(&original_prepare)))return 111;
    recording=true;if(MH_ApplyQueued()!=MH_OK)return 112;
    consuming_renderer=destination;ConsumeFn volatile invoke=fake_prepare;invoke(destination);consuming_renderer=nullptr;
    invoke(destination);recording=false;if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK)return 113;
    if(!safe_copy(after,reinterpret_cast<void*>(fake_prepare),32) || memcmp(before,after,32) || active.load() || prepare_forward_count!=2 || write_index!=1)return 114;
    const auto& r=ring[0];
    if(r.kind!=12 || r.object!=reinterpret_cast<uintptr_t>(destination) || r.clone_source!=r.family_views[0] || r.valid!=(128|256|512|1024|4096|8192|16384) || r.renderer_sources[0]!=0x1111 || r.renderer_sources[1]!=0x2222)return 115;
    if(memcmp(r.source_matrices.data(),source.matrices,128) || r.matrices[24]!=.125f || r.matrices[25]!=-.25f)return 116;
    for(unsigned i=0;i<32;++i)if(i!=24 && i!=25 && r.matrices[i]!=source.matrices[i])return 117;
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureStateCheck(void*) {
    FakeView source{};uintptr_t prior=0x1111;memcpy(source.prefix+8,&prior,8);
    Record record{};record.caller=0x2c8909;record.valid=128;record.family_count=1;
    record.family_views[0]=reinterpret_cast<uintptr_t>(&source);
    state_factory=fake_state_factory;state_release=fake_state_release;state_expected_table=0x12345678;
    unsigned char before[32],after[32];
    if(!safe_copy(before,reinterpret_cast<void*>(fake_state_delete),32) || MH_Initialize()!=MH_OK)return 120;
    if(!install(reinterpret_cast<void*>(fake_state_delete),reinterpret_cast<void*>(hook_state_delete),reinterpret_cast<void**>(&original_state_delete)))return 121;
    recording=true;shadow_state_enabled=true;if(MH_ApplyQueued()!=MH_OK)return 122;
    // Wrong context/count must not allocate; the eligible context acts once.
    record.family_count=2;try_shadow_state(record);record.family_count=1;
    record.caller=0;try_shadow_state(record);record.caller=0x2c8909;
    try_shadow_state(record);try_shadow_state(record);
    if(fake_state_factory_calls!=1 || fake_state_release_calls!=1 || state_created!=1 || state_release_returned!=1 || state_original_preserved!=1 || state_destroyed || !fake_state_pending)return 123;
    HANDLE thread=CreateThread(nullptr,0,fake_state_queue,fake_state_pending,0,nullptr);if(!thread)return 124;
    DWORD waited=WaitForSingleObject(thread,5000),exit_code{};GetExitCodeThread(thread,&exit_code);CloseHandle(thread);
    if(waited!=WAIT_OBJECT_0 || exit_code || fake_state_delete_calls!=1 || state_destroyed!=1 || state_delete_flags!=1 || state_delete_thread==GetCurrentThreadId())return 125;
    if(safe_copy(after,fake_state_pending,8))return 126;
    recording=false;shadow_state_enabled=false;
    if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK || !safe_copy(after,reinterpret_cast<void*>(fake_state_delete),32) || memcmp(before,after,32) || active.load())return 127;
    return 0;
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
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureGPUCheck(void*) {
    gpu_diagnostics=true;
    auto initialized=MH_Initialize();
    if((initialized!=MH_OK&&!(prime_ready.load()&&initialized==MH_ERROR_ALREADY_INITIALIZED))||!present_hooks())return 50;
    unsigned char before[32],after[32];
    if(!safe_copy(before,execute_target,32))return 51;
    std::array<std::array<unsigned char,32>,9> viewport_before{};
    if(viewport_diagnostics)for(unsigned i=0;i<viewport_targets.size();++i){
        auto primed=std::find(prime_targets.begin(),prime_targets.end(),viewport_targets[i]);
        if(prime_ready.load()&&primed!=prime_targets.end())viewport_before[i]=prime_original_bytes[static_cast<size_t>(primed-prime_targets.begin())];
        else if(!safe_copy(viewport_before[i].data(),viewport_targets[i],32))return 58;
    }
    ID3D12Device* device{};ID3D12CommandQueue* queue{};ID3D12CommandAllocator* allocator{};
    ID3D12GraphicsCommandList* list{};ID3D12Fence* fence{};
    ID3D12Resource* target_texture{};ID3D12DescriptorHeap* target_heap{};
    DWORD result=0;HANDLE event{};D3D12_COMMAND_QUEUE_DESC desc{};
    if(FAILED(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))) ||
       FAILED(device->CreateCommandQueue(&desc,IID_PPV_ARGS(&queue))) ||
       FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))) ||
       FAILED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator,nullptr,IID_PPV_ARGS(&list))) ||
       FAILED(list->Close()) || FAILED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence))))result=52;
    if(!result) {
        {std::lock_guard<std::mutex> lock(prime_mutex);
            if(prime_ready.load()){prime_handoff=true;SetEvent(prime_event);}
            recording=true;prime_recording=false;if(MH_ApplyQueued()!=MH_OK)result=53;}
        if(!result&&viewport_diagnostics) {
            if(FAILED(list->Reset(allocator,nullptr)))result=59;
            else {
                D3D12_VIEWPORT viewport{960,0,960,1080,0,1};D3D12_RECT scissor{960,0,1920,1080};
                list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
                D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
                D3D12_RESOURCE_DESC texture{};texture.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;texture.Width=1920;texture.Height=1080;
                texture.DepthOrArraySize=1;texture.MipLevels=1;texture.Format=DXGI_FORMAT_R8G8B8A8_UNORM;texture.SampleDesc.Count=1;texture.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
                D3D12_DESCRIPTOR_HEAP_DESC handles{};handles.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;handles.NumDescriptors=3;
                if(FAILED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&texture,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&target_texture)))||
                   FAILED(device->CreateDescriptorHeap(&handles,IID_PPV_ARGS(&target_heap))))result=63;
                if(!result){auto first=target_heap->GetCPUDescriptorHandleForHeapStart();auto second=first,third=first;
                    UINT stride=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);second.ptr+=stride;third.ptr+=2*stride;
                    device->CreateRenderTargetView(target_texture,nullptr,first);
                    device->CopyDescriptorsSimple(1,second,first,D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
                    device->CopyDescriptors(1,&third,nullptr,1,&second,nullptr,D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
                    list->OMSetRenderTargets(1,&third,FALSE,nullptr);
                    auto mapped=viewport_stats.lookup(third.ptr);
                    if(mapped.resource!=reinterpret_cast<uintptr_t>(target_texture)||mapped.width!=1920||mapped.height!=1080)result=64;
                }
                if(FAILED(list->Close()))result=60;
                auto stats=viewport_stats.snapshot();
                if(stats.resets!=1||stats.viewports!=1||stats.scissors!=1||stats.overflow||stats.target_binds!=1||stats.descriptor_creates!=1||stats.descriptor_copies!=2||stats.copy_rejected)result=61;
            }
        }
        if(!result) {
            ID3D12CommandList* lists[]={list};queue->ExecuteCommandLists(1,lists);
            event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
            if(!event || FAILED(queue->Signal(fence,1)) || FAILED(fence->SetEventOnCompletion(1,event)) ||
               WaitForSingleObject(event,5000)!=WAIT_OBJECT_0)result=54;
            if(!result && (write_index!=1 || ring[0].kind!=5 || ring[0].object!=reinterpret_cast<uintptr_t>(queue) ||
               ring[0].gpu_device!=reinterpret_cast<uintptr_t>(device) || ring[0].queue_type!=D3D12_COMMAND_LIST_TYPE_DIRECT ||
               ring[0].command_count!=1 || !(ring[0].valid&128)))result=55;
        }
    }
    if(MH_DisableHook(MH_ALL_HOOKS)!=MH_OK && !result)result=56;
    recording=false;
    if(!result && (!safe_copy(after,execute_target,32) || memcmp(before,after,32)))result=57;
    if(!result&&viewport_diagnostics)for(unsigned i=0;i<viewport_targets.size();++i)
        if(!safe_copy(after,viewport_targets[i],32)||memcmp(after,viewport_before[i].data(),32))result=62;
    if(target_heap)target_heap->Release();if(target_texture)target_texture->Release();
    if(event)CloseHandle(event);if(fence)fence->Release();if(list)list->Release();
    if(allocator)allocator->Release();if(queue)queue->Release();if(device)device->Release();
    return result;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureViewportCheck(void*) {
    viewport_diagnostics=true;return GearsCaptureGPUCheck(nullptr);
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCapturePrimeCheck(void*) {
    if(MH_Initialize()!=MH_OK||begin_target_prime())return 65;
    ID3D12Device* device{};ID3D12Resource* texture{};ID3D12DescriptorHeap* descriptors{};DWORD result=0;
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC description{};description.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;description.Width=32;description.Height=16;
    description.DepthOrArraySize=1;description.MipLevels=2;description.Format=DXGI_FORMAT_R8G8B8A8_UNORM;description.SampleDesc.Count=1;description.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    D3D12_DESCRIPTOR_HEAP_DESC handles{};handles.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;handles.NumDescriptors=1;
    if(FAILED(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)))||
       FAILED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&description,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&texture)))||
       FAILED(device->CreateDescriptorHeap(&handles,IID_PPV_ARGS(&descriptors))))result=66;
    if(!result){auto handle=descriptors->GetCPUDescriptorHandleForHeapStart();D3D12_RENDER_TARGET_VIEW_DESC view{};
        view.Format=description.Format;view.ViewDimension=D3D12_RTV_DIMENSION_TEXTURE2D;view.Texture2D.MipSlice=1;
        device->CreateRenderTargetView(texture,&view,handle);auto mapped=viewport_stats.lookup(handle.ptr);
        if(mapped.resource!=reinterpret_cast<uintptr_t>(texture)||mapped.width!=16||mapped.height!=8||mapped.mip!=1)result=67;
    }
    SetEvent(prime_event);ULONGLONG deadline=GetTickCount64()+5000;
    while(!prime_done.load()&&GetTickCount64()<deadline)Sleep(1);
    if(!prime_done.load()||prime_recording.load()||active.load())result=68;
    std::array<unsigned char,32> bytes{};
    for(unsigned i=0;i<prime_targets.size();++i)
        if(!safe_copy(bytes.data(),prime_targets[i],bytes.size())||memcmp(bytes.data(),prime_original_bytes[i].data(),bytes.size()))result=69;
    if(descriptors)descriptors->Release();if(texture)texture->Release();if(device)device->Release();return result;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCapturePrimeHandoffCheck(void*) {
    if(MH_Initialize()!=MH_OK||begin_target_prime())return 65;
    viewport_diagnostics=true;DWORD result=GearsCaptureGPUCheck(nullptr);
    if(!prime_handoff.load())SetEvent(prime_event);
    ULONGLONG deadline=GetTickCount64()+5000;while(!prime_done.load()&&GetTickCount64()<deadline)Sleep(1);
    if(!prime_done.load()||prime_recording.load())result=68;
    return result;
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,void*) {
    // Keep CRT thread notifications enabled for the statically linked runtime.
    if(reason==DLL_PROCESS_ATTACH) own_module=module;
    return TRUE;
}
