#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include "cover_pose_phase.h"
#include "cover_control_code_certificate.h"
#ifndef GEARS_COVER_BODY_PROTOTYPE
#define GEARS_COVER_BODY_PROTOTYPE 1
#endif
#ifndef GEARS_COVER_CONTROL_OBSERVER
#define GEARS_COVER_CONTROL_OBSERVER 1
#endif
namespace p=gears_cover_pose_phase;
unsigned checks{},failures{};
void expect(bool okay,char const* what){++checks;if(!okay){++failures;fprintf(stderr,"FAIL %s\n",what);}}
#define C(x) expect(bool(x),#x)
#if GEARS_COVER_BODY_PROTOTYPE && GEARS_COVER_CONTROL_OBSERVER
namespace gears_anchor {struct Identity {uintptr_t address{},cls{},outer{};unsigned index{},name_index{},number{};std::array<wchar_t,64> name{};};}
namespace weapon_hand_scene_binding {
struct Binding {gears_anchor::Identity world{},pawn{},mesh{};};std::mutex mutex;bool metadata_ok=true;
gears_anchor::Identity node{},root{};
bool metadata_current(){return metadata_ok;}
bool same(gears_anchor::Identity const& a,gears_anchor::Identity const& b){return a.address==b.address&&a.cls==b.cls&&a.outer==b.outer&&a.index==b.index&&a.name_index==b.name_index&&a.number==b.number;}
bool identity(uintptr_t a,gears_anchor::Identity& out,bool,bool){if(a==node.address){out=node;return true;}if(a==root.address){out=root;return true;}return false;}
}
constexpr uintptr_t base=0x140000000,game=0x10000000,mesh=game+0x1000,node=game+0x3000,tree=game+0x2000,vtable=game+0x4000,asset=game+0x5000;
constexpr uintptr_t locals=game+0x8000,spaces=game+0xc000,refs=game+0x10000,desired=game+0x18000;
std::atomic<unsigned> active{};std::atomic<bool> recording{true};bool continuous_session=true,stereo_xr=true,anchor_observer_enabled=true;
LONGLONG session_started_qpc=123;wchar_t directory[MAX_PATH]{};
namespace cover_pose_phase_live {
bool enabled=true,current_ok=true,digest_ok=true;unsigned digest_calls{};
struct Owner {weapon_hand_scene_binding::Binding binding{};p::OwnerKey key{};unsigned thread{};bool valid{};};
struct Scope {Owner owner{};uint64_t invocation{};uintptr_t matrix{};unsigned kind{},caller{},episode{};bool tick{},matching{},started{},arguments{};};
struct Row {bool snapshot_valid{};p::Snapshot snapshot{};};
struct Episode {p::OwnerKey owner{};uint64_t invocation{};unsigned thread{};bool closed{},exception{},nested{},concurrent{};std::array<Row,7> rows{};};
std::mutex mutex;std::array<Episode,8> episodes{};unsigned episode_count=1;
struct Thread {unsigned ticks=1,episode=1;};inline thread_local Thread thread{};
LONGLONG qpc(){LARGE_INTEGER v{};QueryPerformanceCounter(&v);return v.QuadPart;}
p::Identity compact(gears_anchor::Identity const& a){return {a.address,a.cls,a.outer,int32_t(a.index),int32_t(a.name_index),a.number};}
bool same(p::Identity const& a,p::Identity const& b){return a.address==b.address&&a.cls==b.cls&&a.outer==b.outer&&a.internal_index==b.internal_index&&a.name_index==b.name_index&&a.number==b.number;}
bool same(p::OwnerKey const& a,p::OwnerKey const& b){return same(a.world,b.world)&&same(a.pawn,b.pawn)&&same(a.mesh,b.mesh)&&a.session==b.session&&a.world_generation==b.world_generation&&a.scene==b.scene&&a.scene_qpc==b.scene_qpc;}
bool current(Owner const& o){return current_ok&&recording.load()&&o.valid&&o.thread==GetCurrentThreadId();}
bool digest(gears_cover_pose_certificate::Chunk const&){++digest_calls;return digest_ok;}
}
struct Code {uintptr_t address{};std::array<unsigned char,32> bytes{};};std::array<Code,6> code{};
std::array<unsigned char,0x20000> memory{};std::array<p::Matrix,3> output_matrices{};std::array<int32_t,3> affected{{8,10,11}};
uintptr_t fail_read_at{};bool memory_ok=true,change_matrix_on_read{};unsigned game_reads{};bool entry_ok=true;unsigned creates{};bool hook_reject{};
bool span(uintptr_t at,size_t size,uintptr_t start,size_t length){return at>=start&&at-start<=length&&size<=length-(at-start);}
bool safe_copy(void* out,void const* in,size_t size){const auto at=reinterpret_cast<uintptr_t>(in);if(!memory_ok||at==fail_read_at)return false;
    for(auto const& c:code)if(span(at,size,c.address,c.bytes.size())){++game_reads;memcpy(out,c.bytes.data()+at-c.address,size);return true;}
    if(span(at,size,game,memory.size())){++game_reads;memcpy(out,memory.data()+at-game,size);return true;}
    if(span(at,size,reinterpret_cast<uintptr_t>(output_matrices.data()),sizeof output_matrices)||span(at,size,reinterpret_cast<uintptr_t>(affected.data()),sizeof affected)){++game_reads;memcpy(out,in,size);
        if(change_matrix_on_read&&at==reinterpret_cast<uintptr_t>(output_matrices.data())){output_matrices[0].values[13]+=1;change_matrix_on_read=false;}return true;}
    ULONG_PTR low{},high{};GetCurrentThreadStackLimits(&low,&high);if(at>=low&&at<high&&size<=high-at){memcpy(out,in,size);return true;}return false;
}
template<class T>void put(uintptr_t at,T const& value){C(span(at,sizeof value,game,memory.size()));memcpy(memory.data()+at-game,&value,sizeof value);}
constexpr int MH_OK=0;
int MH_CreateHook(void* target,void*,void** original){++creates;if(hook_reject)return 8;for(unsigned i=0;i<3;++i)if(reinterpret_cast<uintptr_t>(target)==code[i].address){*original=reinterpret_cast<void*>(uintptr_t(i+1));return MH_OK;}return 1;}
namespace cover_body_live {using CallPacket=p::CallPacket;extern "C" void GearsCoverRawInvoke(CallPacket*,void*) noexcept(false);}
#include "cover_control_observer_live.inl"
namespace live=cover_control_observer_live;namespace phase=cover_pose_phase_live;
bool live::own_entries(){return entry_ok;}
extern "C" void GearsCoverControlLimbHook(){}extern "C" void GearsCoverControlSingleHook(){}extern "C" void GearsCoverControlWeightHook(){}
phase::Scope scope(uint64_t invocation=900,unsigned episode=1){
    phase::Scope s{};s.kind=0;s.invocation=invocation;s.episode=episode;s.arguments=true;s.owner.valid=true;s.owner.thread=GetCurrentThreadId();
    auto& key=s.owner.key;key.world={game+0x600,1,2,3,4,0};key.pawn={game+0x700,1,2,5,6,0};key.mesh={mesh,1,game+0x700,7,8,0};
    key.world_generation=1;key.session=uint64_t(session_started_qpc);key.scene=99;key.scene_qpc=phase::qpc();
    auto& e=phase::episodes[episode-1];e={};e.owner=key;e.invocation=800;e.thread=GetCurrentThreadId();e.rows[0].snapshot_valid=true;
    auto& request=e.rows[0].snapshot.request;request.owner=key;request.invocation=invocation;request.thread=s.owner.thread;request.qpc=phase::qpc();request.caller_rva=p::compose_caller;request.kind=p::Kind::compose;request.phase=p::Phase::C0;
    phase::episode_count=(std::max)(phase::episode_count,episode);return s;
}
void game_state(unsigned compute_kind=0){
    memory_ok=true;change_matrix_on_read=false;entry_ok=true;phase::current_ok=true;weapon_hand_scene_binding::metadata_ok=true;affected={8,10,11};memory.fill(0);
    auto& id=weapon_hand_scene_binding::node;id={};id.address=node;id.cls=base+0x800;id.outer=tree;id.index=20;id.name_index=30;
    auto& root=weapon_hand_scene_binding::root;root={};root.address=tree;root.cls=base+0x900;root.outer=mesh;root.index=21;root.name_index=31;
    put(mesh+0x230,tree);put(node,vtable);p::Metadata m{};m.asset=asset;m.reference={refs,115,115};m.local={locals,115,115};m.space={spaces,115,115};m.desired={desired,115,115};
    m.component.values[0]=m.component.values[5]=m.component.values[10]=m.component.values[15]=1;m.component.values[12]=100;
    put(mesh+0x218,m.asset);put(mesh+0x29c,m.master);put(mesh+0x248,m.physics_asset);put(mesh+0x250,m.physics_instance);put(asset+0xa4,m.reference);
    put(mesh+0x27c,m.local);put(mesh+0x26c,m.space);put(mesh+0x28c,m.desired);put(mesh+0xa0,m.component);
    constexpr unsigned slots[]={0x230,0x238,0x240,0x248,0x278};const uintptr_t functions[]={code[3].address,code[compute_kind].address,code[4].address,code[5].address,code[2].address};
    for(unsigned i=0;i<5;++i)put(vtable+slots[i],functions[i]);const float alpha=.625f;put(node+0x70,alpha);
}
#include "post_weight_scale_layout.inl"
#include "post_weight_scale_test_cases.inl"

#else
namespace cover_pose_phase_live {struct Scope {};}
namespace weapon_hand_scene_binding {}
#include "cover_control_observer_live.inl"
int main(int argc,char** argv){C(argc==3);if(argc!=3)return 1;
    wchar_t path[MAX_PATH]{};C(swprintf_s(path,L"%S\\post-off-%lu-%llu.ini",argv[2],GetCurrentProcessId(),GetTickCount64())>0);
    C(WritePrivateProfileStringW(L"CoverControlObserver",L"enabled",L"0",path)!=FALSE);
    C(cover_control_observer_live::configure(path,nullptr));
    C(WritePrivateProfileStringW(L"CoverPostWeightScaleObserver",L"enabled",L"1",path)!=FALSE);
    C(!cover_control_observer_live::configure(path,nullptr));
    C(WritePrivateProfileStringW(L"CoverPostWeightScaleObserver",L"enabled",L"invalid",path)!=FALSE);
    C(!cover_control_observer_live::configure(path,nullptr));
    C(WritePrivateProfileStringW(L"CoverPostWeightScaleObserver",L"enabled",L"0",path)!=FALSE);
    C(cover_control_observer_live::configure(path,nullptr));
    C(!cover_control_observer_live::enabled&&!cover_control_observer_live::pending_count());
    printf("PostWeightScaleOff: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
#endif
