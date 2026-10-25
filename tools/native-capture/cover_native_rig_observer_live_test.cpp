// True CoreSnapshot/header ABI; controlled Rig/Phase/CCTRL/World admission stubs.
// This tests observer composition, NOT the separately tested live Core provider.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <mutex>
#include <new>
#include <stdexcept>
#include <thread>
#include "../vr-common/native_rig_tracking_frame.h"
#include "cover_pose_phase.h"
#ifndef GEARS_COVER_NATIVE_RIG_OBSERVER
#define GEARS_COVER_NATIVE_RIG_OBSERVER 1
#endif
#ifndef GEARS_COVER_BODY_PROTOTYPE
#define GEARS_COVER_BODY_PROTOTYPE 1
#endif
#ifndef GEARS_COVER_CONTROL_OBSERVER
#define GEARS_COVER_CONTROL_OBSERVER 1
#endif
unsigned checks{},failures{};
void check(bool okay,char const* expression){++checks;if(!okay){++failures;fprintf(stderr,"FAIL %s\n",expression);}}
#define C(x) check(bool(x),#x)
namespace gears_anchor {struct Identity {uintptr_t address{},cls{},outer{};unsigned index{},name_index{},number{};};}
namespace weapon_hand_scene_binding {struct Binding {gears_anchor::Identity world{},pawn{},mesh{};uint64_t world_generation{},scene_invocation{};bool context_valid{};};}
namespace p=gears_cover_pose_phase;
namespace cover_pose_phase_live {
bool enabled=true,current_ok=true;bool throw_current{};
struct Owner {weapon_hand_scene_binding::Binding binding{};p::OwnerKey key{};unsigned thread{};bool valid{};};
struct Scope {Owner owner{};uint64_t invocation{};uintptr_t matrix{};unsigned kind{},caller{},episode{};bool tick{},matching{},started{},arguments{};};
struct Thread {unsigned ticks=1,episode=1;};inline thread_local Thread thread{};
p::Identity compact(gears_anchor::Identity const& a){return {a.address,a.cls,a.outer,int32_t(a.index),int32_t(a.name_index),a.number};}
bool same(p::Identity const& a,p::Identity const& b){return a.address==b.address&&a.cls==b.cls&&a.outer==b.outer&&a.internal_index==b.internal_index&&a.name_index==b.name_index&&a.number==b.number;}
bool same(p::OwnerKey const& a,p::OwnerKey const& b){return same(a.world,b.world)&&same(a.pawn,b.pawn)&&same(a.mesh,b.mesh)&&a.session==b.session&&a.world_generation==b.world_generation&&a.scene==b.scene&&a.scene_qpc==b.scene_qpc;}
bool current(Owner const& owner){if(throw_current)throw std::runtime_error("controlled metadata failure");return current_ok&&owner.valid;}
}
namespace cover_control_observer_live {
bool enabled=true,current_ok=true;enum class Failure:unsigned {pending,okay};
struct Node {p::Identity node{};};
struct Row {p::OwnerKey owner{};uint64_t compose{},outer_tick{},ordinal{},compute_ordinal{};int64_t before_qpc{},after_qpc{};
    uintptr_t node_argument{},mesh_argument{},raw_rdx{},result_rax{};unsigned episode{},kind{},thread{};Failure failure{Failure::pending};Node before{},after{};
    bool before_valid{},after_valid{},returned{},exception{},nested{},concurrent{},compose_returned{},compose_exception{};};
struct Compose {cover_pose_phase_live::Scope scope{};uint64_t outer_tick{};bool admitted{},nested{};};
struct Thread {std::array<Compose,8> frames{};unsigned depth=1,overflow{};};inline thread_local Thread thread{};
bool current(Compose const&){return current_ok;}
}
namespace weapon_hand_runtime {
bool enabled=true;
struct Session {unsigned game_pid{},owner_worker_thread{};uint64_t game_start{},native_started_qpc{};};
Session binding{};Session session(){return binding;}void* current(){return &binding;}
}
std::atomic<bool> recording{true};std::atomic<uint64_t> committed_world_generation{7};
bool continuous_session=true,stereo_xr=true,anchor_observer_enabled=true;
int64_t session_started_qpc=123;wchar_t directory[MAX_PATH]{};
bool admission_ok=true,throw_scope{};unsigned leases{},constructed{},destroyed{};
struct Scope {Scope(){if(throw_scope)throw std::runtime_error("controlled admission failure");++leases;++constructed;}
    ~Scope(){--leases;++destroyed;}bool world()const{return admission_ok;}Scope(Scope const&)=delete;Scope& operator=(Scope const&)=delete;};
namespace rig_tracking_live {
namespace core=gears_native_rig_tracking;bool enabled=true,session_ok=true,data_ok=true,end_ok=true;unsigned getters{},begins{},reads{},ends{};
core::CoreSnapshot payload{};
gears_weapon_hand::ObjectIdentity object(gears_anchor::Identity const& a){return {a.address,a.outer,a.cls,a.index,a.name_index,a.number,1};}
bool native_session_current(void*,weapon_hand_runtime::Session const&){return session_ok;}
struct CoreReadRequest {core::NativeSessionKey native{};gears_weapon_hand::ObjectIdentity world{},pawn{},mesh{};uint64_t world_generation{},scene_invocation{};
    int64_t scene_qpc{};uint64_t minimum_publication_sequence{},maximum_source_age_ns{50000000};};
struct TickKey {CoreReadRequest request{};uint64_t outer_tick{};};
struct TickLatch {bool active{},valid{};core::CoreSnapshot snapshot{};TickLatch()=default;TickLatch(TickLatch const&)=delete;TickLatch& operator=(TickLatch const&)=delete;};
bool begin_tick(TickKey const&,TickLatch& latch){++begins;++getters;if(latch.active)return false;latch.active=true;latch.valid=data_ok;latch.snapshot=payload;return latch.valid;}
bool read_tick(TickKey const&,TickLatch& latch,core::CoreSnapshot& out){++reads;out={};if(!latch.active||!latch.valid)return false;if(!data_ok){latch.valid=false;return false;}out=latch.snapshot;return true;}
bool end_tick(TickKey const&,TickLatch& latch){++ends;if(!end_ok||!latch.active)return false;latch.active=latch.valid=false;return true;}
}
#include "cover_native_rig_observer_live.inl"
namespace live=cover_native_rig_observer_live;namespace phase=cover_pose_phase_live;namespace controls=cover_control_observer_live;namespace rig=rig_tracking_live;
void ini_write(wchar_t const* path,wchar_t const* value){C(WritePrivateProfileStringW(L"CoverNativeRigObserver",L"enabled",value,path)!=0);}
#if GEARS_COVER_NATIVE_RIG_OBSERVER && GEARS_COVER_BODY_PROTOTYPE && GEARS_COVER_CONTROL_OBSERVER
phase::Scope setup(){
    live::enabled=true;weapon_hand_runtime::binding={GetCurrentProcessId(),999,456,uint64_t(session_started_qpc)};
    phase::Scope s{};s.kind=2;s.invocation=800;s.episode=1;s.tick=s.matching=s.started=true;s.owner.thread=GetCurrentThreadId();s.owner.valid=true;
    auto& b=s.owner.binding;b.world={0x1000,0x1100,0,1,2,0};b.pawn={0x2000,0x2100,0x1000,3,4,0};b.mesh={0x3000,0x3100,0x2000,5,6,0};
    b.world_generation=7;b.scene_invocation=90;b.context_valid=true;auto& k=s.owner.key;
    k.world=phase::compact(b.world);k.pawn=phase::compact(b.pawn);k.mesh=phase::compact(b.mesh);k.world_generation=7;k.session=123;k.scene=90;k.scene_qpc=100;
    auto& compose=controls::thread.frames[0];compose.scope=s;compose.scope.kind=0;compose.scope.invocation=801;compose.scope.arguments=true;compose.outer_tick=800;compose.admitted=true;
    rig::payload.publication_sequence=777;rig::payload.scene_invocation=90;rig::payload.scene_qpc=100;rig::payload.native={GetCurrentProcessId(),GetCurrentThreadId(),456,123};
    rig::payload.status=gears_rig_tracking::Status::ready;rig::payload.world_generation=7;rig::payload.world=rig::object(b.world);rig::payload.pawn=rig::object(b.pawn);rig::payload.mesh=rig::object(b.mesh);
    rig::payload.flags=0;rig::payload.world_pose_mask=1;rig::payload.world_head.position={1,2,3};return s;
}
controls::Row row(phase::Scope const& s,unsigned ordinal){controls::Row r{};r.owner=s.owner.key;r.compose=801;r.outer_tick=s.invocation;r.ordinal=r.compute_ordinal=ordinal;
    r.episode=s.episode;r.kind=ordinal%2;r.thread=GetCurrentThreadId();r.node_argument=0x4000+ordinal*16;r.mesh_argument=s.owner.key.mesh.address;r.raw_rdx=11;r.result_rax=0xabcdef;
    r.before.node={r.node_argument,0x5000,0x6000,int32_t(10+ordinal),20,0};r.after=r.before;r.before_valid=r.after_valid=r.returned=true;r.failure=controls::Failure::okay;return r;}
void complete(phase::Scope const& s,bool returned=true){live::tick_end(s,returned);C(!live::pending_count()&&leases==0&&constructed==destroyed&&live::ended==1);}
void abi(){
    printf("{\"Header\":%zu,\"Record\":%zu,\"CoreSnapshot\":%zu,\"Header.native\":%zu,\"Header.outer_tick\":%zu,\"Header.scene\":%zu,\"Header.scene_qpc\":%zu,\"Header.world_generation\":%zu,\"Header.flags\":%zu,\"Record.snapshot\":%zu,\"Record.node\":%zu,\"Record.end_bone\":%zu,\"Core.source_view\":%zu,\"Core.base_view\":%zu,\"Core.stereo\":%zu,\"Core.controllers\":%zu}\n",
        sizeof(live::Header),sizeof(live::Record),sizeof(gears_native_rig_tracking::CoreSnapshot),offsetof(live::Header,native),offsetof(live::Header,outer_tick),offsetof(live::Header,scene),offsetof(live::Header,scene_qpc),offsetof(live::Header,world_generation),offsetof(live::Header,flags),offsetof(live::Record,snapshot),offsetof(live::Record,node),offsetof(live::Record,end_bone),offsetof(gears_native_rig_tracking::CoreSnapshot,source_view),offsetof(gears_native_rig_tracking::CoreSnapshot,base_view),offsetof(gears_native_rig_tracking::CoreSnapshot,stereo),offsetof(gears_native_rig_tracking::CoreSnapshot,controllers));
    printf("{\"HeaderOffsets\":{");
#define H(n) printf("\"" #n "\":%zu,",offsetof(live::Header,n))
    H(magic);H(version);H(header_size);H(record_size);H(count);H(capacity);H(read_only);H(writer);H(atomic_verified);H(native);H(outer_tick);H(scene);H(world_generation);H(scene_qpc);H(episode);H(flags);H(failure);
#undef H
    printf("\"reserved\":%zu},\"RecordOffsets\":{",offsetof(live::Header,reserved));
#define R(n) printf("\"" #n "\":%zu,",offsetof(live::Record,n))
    R(outer_tick);R(compose);R(ordinal);R(compute_ordinal);R(node);R(before_qpc);R(after_qpc);R(result_rax);R(end_bone);R(kind);R(thread);R(native_failure);R(flags);R(reserved);
#undef R
    printf("\"snapshot\":%zu}}\n",offsetof(live::Record,snapshot));
}
int run(char const* mode){
    if(!strcmp(mode,"abi")){abi();return 0;}
    if(!strcmp(mode,"configure")){
        wchar_t temp[MAX_PATH]{},ini[MAX_PATH]{};C(GetTempPathW(MAX_PATH,temp)>0);C(GetTempFileNameW(temp,L"crg",0,ini)>0);
        C(!live::enabled&&live::configure(ini,nullptr));ini_write(ini,L"true");C(!live::configure(ini,nullptr));ini_write(ini,L"1");
        bool* gates[]={&phase::enabled,&controls::enabled,&rig::enabled,&weapon_hand_runtime::enabled,&continuous_session,&stereo_xr,&anchor_observer_enabled};
        for(auto gate:gates){*gate=false;C(!live::configure(ini,nullptr)&&!live::enabled);*gate=true;}
        C(live::configure(ini,nullptr)&&live::enabled);C(!live::configure(ini,nullptr));C(DeleteFileW(ini)!=0);return failures?1:0;
    }
    auto s=setup();
    if(!strcmp(mode,"stopped-before")){live::request_drain();live::tick_begin(s);C(!live::selected&&live::pending_count()==0&&leases==0&&rig::begins==0);return failures?1:0;}
    if(!strcmp(mode,"no-data")){
        wchar_t temp[MAX_PATH]{},path[MAX_PATH]{};C(GetTempPathW(MAX_PATH,temp)>0);swprintf_s(directory,L"%sCRIG-empty-%lu-%llu",temp,GetCurrentProcessId(),GetTickCount64());C(CreateDirectoryW(directory,nullptr)!=0);
        swprintf_s(path,L"%s\\core-tick-observation.bin",directory);live::request_drain();live::print(nullptr,true);C(live::file_written&&live::pending_count()==0);
        HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);C(f!=INVALID_HANDLE_VALUE);live::Header h{};DWORD n{};
        C(ReadFile(f,&h,sizeof h,&n,nullptr)&&n==sizeof h&&h.count==0&&(h.flags&live::no_tick)&&h.failure==live::Failure::none);C(GetFileSize(f,nullptr)==sizeof h);C(CloseHandle(f)!=0);C(DeleteFileW(path)!=0);C(RemoveDirectoryW(directory)!=0);return failures?1:0;
    }
    if(!strcmp(mode,"no-event")){s.started=false;live::tick_begin(s);C(!live::selected&&rig::begins==0&&leases==0&&live::pending_count()==0);return failures?1:0;}
    if(!strcmp(mode,"lease-denied"))admission_ok=false;
    if(!strcmp(mode,"scope-exception"))throw_scope=true;
    if(!strcmp(mode,"owner"))s.owner.binding.mesh.number++;
    if(!strcmp(mode,"wrong-episode"))phase::thread.episode++;
    if(!strcmp(mode,"metadata-exception"))phase::throw_current=true;
    if(!strcmp(mode,"latch-unavailable"))rig::data_ok=false;
    live::tick_begin(s);C(live::pending_count()==1&&live::ticks==1);
    if(!strcmp(mode,"lease-denied")||!strcmp(mode,"owner")||!strcmp(mode,"wrong-episode")||!strcmp(mode,"scope-exception")||!strcmp(mode,"metadata-exception")){
        C(live::failed&&rig::begins==0);phase::throw_current=false;complete(s);return failures?1:0;}
    C(leases==1&&rig::begins==1&&rig::getters==1&&live::latch_attempts==1);
    if(!strcmp(mode,"latch-unavailable")){C(live::failed);rig::data_ok=true;live::control(row(s,1));C(rig::reads==0&&live::record_count==0);complete(s);return failures?1:0;}
    if(!strcmp(mode,"normal")){
        for(unsigned i=1;i<=4;++i){rig::payload.publication_sequence++;live::control(row(s,i));C(live::record_count==i&&rig::reads==i&&rig::getters==1);C(!memcmp(&live::records[i-1].snapshot,&live::frame.latch.snapshot,sizeof rig::payload));}
        C(live::read_ok==4&&!live::failed&&live::header.scene==90&&live::frame.key.outer_tick==800&&live::frame.key.request.scene_invocation==90);
        C(live::records[0].snapshot.world_pose_mask==1);live::control(row(s,5));C(live::record_count==4&&rig::reads==4&&live::dropped==1);
    }else if(!strcmp(mode,"stale")){
        rig::data_ok=false;live::control(row(s,1));C(live::record_count==1&&!(live::records[0].flags&1)&&live::failed&&rig::reads==1);
        rig::data_ok=true;live::control(row(s,2));C(rig::reads==1&&live::read_ok==0&&live::read_reject==2);
    }else if(!strcmp(mode,"wrong-row")){
        auto r=row(s,1);r.owner.scene++;live::control(r);C(live::failed&&rig::reads==0&&live::record_count==0);
    }else if(!strcmp(mode,"duplicate")){
        const auto r=row(s,1);live::control(r);live::control(r);C(live::failed&&rig::reads==1&&live::record_count==1);
    }else if(!strcmp(mode,"nested")){
        auto n=s;n.invocation++;n.started=false;phase::thread.ticks=2;live::tick_begin(n);live::tick_end(n,true);C(leases==1&&live::pending_count()==1&&rig::ends==0&&rig::begins==1);
        phase::thread.ticks=1;C(live::failed);
    }else if(!strcmp(mode,"wrong-end")){
        auto other=s;live::tick_end(other,true);C(live::pending_count()==1&&leases==1&&rig::ends==0);C(live::failed);
    }else if(!strcmp(mode,"wrong-end-key")){
        const auto before=s.owner.key;s.owner.key.scene++;live::tick_end(s,true);C(live::pending_count()==1&&leases==1&&rig::ends==0);s.owner.key=before;C(live::failed);
    }else if(!strcmp(mode,"wrong-thread")){
        std::thread wrong([&]{live::tick_end(s,true);live::control(row(s,1));});wrong.join();C(live::pending_count()==1&&leases==1&&rig::reads==0&&rig::ends==0);
    }else if(!strcmp(mode,"drain")){
        live::request_drain();live::control(row(s,1));C(live::pending_count()==1&&leases==1&&rig::reads==0&&live::read_reject==1);
    }else if(!strcmp(mode,"exception")){complete(s,false);C(live::failed&&!(live::header.flags&live::original_returned));return failures?1:0;
    }else if(!strcmp(mode,"end-retain")){
        rig::end_ok=false;live::tick_end(s,true);C(live::pending_count()==1&&leases==1&&live::failed);rig::end_ok=true;
    }else if(strcmp(mode,"file")&&strcmp(mode,"preserve")){fprintf(stderr,"unknown mode\n");return 2;}
    complete(s);C(rig::getters==1);auto later=s;later.invocation++;live::tick_begin(later);C(rig::begins==1&&live::pending_count()==0);
    if(!strcmp(mode,"file")||!strcmp(mode,"preserve")){
        wchar_t temp[MAX_PATH]{};C(GetTempPathW(MAX_PATH,temp)>0);swprintf_s(directory,L"%sCRIG-%lu-%llu",temp,GetCurrentProcessId(),GetTickCount64());C(CreateDirectoryW(directory,nullptr)!=0);
        wchar_t path[MAX_PATH]{};swprintf_s(path,L"%s\\core-tick-observation.bin",directory);const bool preserve=!strcmp(mode,"preserve");
        if(preserve){HANDLE f=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);C(f!=INVALID_HANDLE_VALUE);DWORD n{};C(WriteFile(f,"KEEP",4,&n,nullptr)&&n==4);C(CloseHandle(f)!=0);}
        live::print(nullptr,true);C(!live::file_attempted);live::request_drain();live::print(nullptr,false);C(!live::file_attempted);live::print(nullptr,true);C(live::file_attempted);
        HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);C(f!=INVALID_HANDLE_VALUE);DWORD n{};
        if(preserve){char bytes[4]{};C(!live::file_written&&(live::file_error==ERROR_FILE_EXISTS||live::file_error==ERROR_ALREADY_EXISTS));C(ReadFile(f,bytes,4,&n,nullptr)&&n==4&&!memcmp(bytes,"KEEP",4));}
        else {live::Header h{};C(live::file_written);C(ReadFile(f,&h,sizeof h,&n,nullptr)&&n==sizeof h);C(!memcmp(h.magic,"CRIG0001",8)&&h.header_size==sizeof h&&h.record_size==sizeof(live::Record)&&h.count==0&&h.read_only==1&&h.writer==0&&h.atomic_verified==0);}
        C(CloseHandle(f)!=0);live::print(nullptr,true);C(DeleteFileW(path)!=0);C(RemoveDirectoryW(directory)!=0);
    }
    return failures?1:0;
}
#else
int run(char const*){wchar_t temp[MAX_PATH]{},ini[MAX_PATH]{};C(GetTempPathW(MAX_PATH,temp)>0);C(GetTempFileNameW(temp,L"crg",0,ini)>0);C(live::configure(ini,nullptr)&&!live::enabled);
    ini_write(ini,L"1");C(!live::configure(ini,nullptr));phase::Scope s{};controls::Row r{};live::tick_begin(s);live::control(r);live::tick_end(s,true);live::request_drain();C(live::pending_count()==0);C(DeleteFileW(ini)!=0);return failures?1:0;}
#endif
int main(int argc,char** argv){const char* mode=argc>1?argv[1]:"normal";const auto result=run(mode);printf("cover_native_rig_observer %s: %u checks, %u failures; stubs=Rig/Phase/CCTRL/World real=CoreSnapshotABI\n",mode,checks,failures);return result;}
