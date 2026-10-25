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
bool memory_ok=true,change_matrix_on_read{};unsigned game_reads{};bool entry_ok=true;unsigned creates{};
bool span(uintptr_t at,size_t size,uintptr_t start,size_t length){return at>=start&&at-start<=length&&size<=length-(at-start);}
bool safe_copy(void* out,void const* in,size_t size){const auto at=reinterpret_cast<uintptr_t>(in);if(!memory_ok)return false;
    for(auto const& c:code)if(span(at,size,c.address,c.bytes.size())){++game_reads;memcpy(out,c.bytes.data()+at-c.address,size);return true;}
    if(span(at,size,game,memory.size())){++game_reads;memcpy(out,memory.data()+at-game,size);return true;}
    if(span(at,size,reinterpret_cast<uintptr_t>(output_matrices.data()),sizeof output_matrices)||span(at,size,reinterpret_cast<uintptr_t>(affected.data()),sizeof affected)){++game_reads;memcpy(out,in,size);
        if(change_matrix_on_read&&at==reinterpret_cast<uintptr_t>(output_matrices.data())){output_matrices[0].values[13]+=1;change_matrix_on_read=false;}return true;}
    ULONG_PTR low{},high{};GetCurrentThreadStackLimits(&low,&high);if(at>=low&&at<high&&size<=high-at){memcpy(out,in,size);return true;}return false;
}
template<class T>void put(uintptr_t at,T const& value){C(span(at,sizeof value,game,memory.size()));memcpy(memory.data()+at-game,&value,sizeof value);}
constexpr int MH_OK=0;
int MH_CreateHook(void* target,void*,void** original){++creates;for(unsigned i=0;i<3;++i)if(reinterpret_cast<uintptr_t>(target)==code[i].address){*original=reinterpret_cast<void*>(uintptr_t(i+1));return MH_OK;}return 1;}
namespace cover_body_live {using CallPacket=p::CallPacket;extern "C" void GearsCoverRawInvoke(CallPacket*,void*) noexcept(false);}
#include "cover_control_observer_live.inl"
namespace live=cover_control_observer_live;namespace phase=cover_pose_phase_live;
bool live::own_entries(){return entry_ok;}
extern "C" void GearsCoverControlLimbHook(){}extern "C" void GearsCoverControlSingleHook(){}extern "C" void GearsCoverControlWeightHook(){}
#if GEARS_COVER_CONTROL_OBSERVER && GEARS_COVER_BODY_PROTOTYPE
enum class Mode {normal,node_changed,header_changed,count,nan,affected_changed,alias_data,payload_changed,weight_nan,exception_seh,exception_cpp,nested,concurrent,read_failed,stop_overlap,block};Mode mode{};
std::atomic<unsigned> original_calls{};constexpr DWORD control_exception=0xe042c099;struct CppException {unsigned value;};
std::mutex wait_mutex;std::condition_variable wait_cv;bool entered{},allow_return{};
void invoke(unsigned kind,uintptr_t caller=0,uintptr_t node_value=node,bool bad_pointer=false,int end=11,uintptr_t receiver=mesh,bool bad_rsp=false){
    alignas(16) unsigned char stack[0x120]{};const auto entry=reinterpret_cast<uintptr_t>(stack)+8;
    const unsigned n=kind==1?1:3;p::ArrayHeader out{},indices{reinterpret_cast<uintptr_t>(affected.data()),int32_t(n),int32_t(n)};
    memcpy(reinterpret_cast<void*>(entry+0x80),&out,16);memcpy(reinterpret_cast<void*>(entry+0x90),&indices,16);
    p::CallPacket packet{};packet.gpr[1]=node_value;packet.gpr[2]=static_cast<uint32_t>(end);packet.gpr[3]=receiver;packet.gpr[4]=entry+0x80+unsigned(bad_pointer);
    if(!caller)caller=base+(kind==2?0x12911f:0x1290e5);
    live::GearsCoverControlInvokeProtected(&packet,kind,caller,bad_rsp?0:entry);C(packet.gpr[0]==0x123456789abcdef0ull);
}
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
    put(mesh+0x230,tree);put(node,vtable);p::Metadata m{};m.asset=asset;m.reference={refs,16,16};m.local={locals,16,16};m.space={spaces,16,16};m.desired={desired,16,16};
    m.component.values[0]=m.component.values[5]=m.component.values[10]=m.component.values[15]=1;m.component.values[12]=100;
    put(mesh+0x218,m.asset);put(mesh+0x29c,m.master);put(mesh+0x248,m.physics_asset);put(mesh+0x250,m.physics_instance);put(asset+0xa4,m.reference);
    put(mesh+0x27c,m.local);put(mesh+0x26c,m.space);put(mesh+0x28c,m.desired);put(mesh+0xa0,m.component);
    constexpr unsigned slots[]={0x230,0x238,0x240,0x248,0x278};const uintptr_t functions[]={code[3].address,code[compute_kind].address,code[4].address,code[5].address,code[2].address};
    for(unsigned i=0;i<5;++i)put(vtable+slots[i],functions[i]);const float alpha=.625f;put(node+0x70,alpha);
}
extern "C" void cover_body_live::GearsCoverRawInvoke(CallPacket* packet,void* original) noexcept(false){
    ++original_calls;C(active.load()>0&&live::in_flight.load()>0);const auto kind=unsigned(reinterpret_cast<uintptr_t>(original)-1);
    if(mode==Mode::block){std::unique_lock<std::mutex> lock(wait_mutex);entered=true;wait_cv.notify_all();C(wait_cv.wait_for(lock,std::chrono::seconds(5),[]{return allow_return;}));}
    if(mode==Mode::exception_seh)RaiseException(control_exception,0,0,nullptr);
    if(mode==Mode::exception_cpp)throw CppException{0x9234};
    if(kind<2){if(mode==Mode::nested){const auto old=mode;mode=Mode::normal;invoke(1);mode=old;}
        if(mode==Mode::concurrent){phase::episodes[0].concurrent=true;std::thread other([]{invoke(2);});other.join();}
        const unsigned n=kind?1:3;for(unsigned i=0;i<n;++i){auto& m=output_matrices[i];m={};m.values[0]=m.values[5]=m.values[10]=m.values[15]=1;m.values[12]=float(10+i);}
        p::ArrayHeader result{reinterpret_cast<uintptr_t>(output_matrices.data()),int32_t(n),int32_t(n)};
        if(mode==Mode::count)result.count=2;if(mode==Mode::nan)output_matrices[0].values[0]=(std::numeric_limits<float>::quiet_NaN)();
        if(mode==Mode::alias_data)result.data=locals;
        if(mode==Mode::payload_changed)change_matrix_on_read=true;
        if(mode==Mode::affected_changed)affected[0]=7;
        memcpy(reinterpret_cast<void*>(packet->gpr[4]),&result,16);
    }else {const float weight=mode==Mode::weight_nan?(std::numeric_limits<float>::quiet_NaN)():.625f;memcpy(packet->xmm[0].data(),&weight,4);}
    if(mode==Mode::node_changed)++weapon_hand_scene_binding::node.number;
    if(mode==Mode::header_changed){p::ArrayHeader h{locals,16,17};put(mesh+0x27c,h);}
    if(mode==Mode::read_failed)memory_ok=false;
    if(mode==Mode::stop_overlap)live::request_drain();
    packet->gpr[0]=0x123456789abcdef0ull;
}
bool catch_seh(){__try{invoke(0);return false;}__except(GetExceptionCode()==control_exception?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return true;}}
void collect_cases(){
    // Hooks always forward outside an admitted event without touching Game.
    game_state();const auto reads=game_reads;const auto originals_before=original_calls.load();invoke(0);C(game_reads==reads&&original_calls==originals_before+1&&!live::pending_count());
    auto s=scope();
    for(unsigned gate=0;gate<8;++gate){auto denied=s;
        if(gate==0)denied.arguments=false;if(gate==1)phase::episodes[0].rows[0].snapshot_valid=false;
        if(gate==2)denied.owner.thread+=1;if(gate==3)phase::thread.ticks=2;if(gate==4)phase::current_ok=false;
        if(gate==5)phase::episodes[0].rows[0].snapshot.request.owner.mesh.address+=8;
        if(gate==6)phase::episodes[0].rows[0].snapshot.request.caller_rva=0x12345;
        if(gate==7)phase::thread.episode=0;
        const auto no_read=game_reads;live::compose_begin(denied);invoke(0);C(game_reads==no_read&&live::row_count==0);live::compose_end(denied,true);C(!live::pending_count());
        phase::episodes[0].rows[0].snapshot_valid=true;phase::episodes[0].rows[0].snapshot.request.owner=s.owner.key;phase::episodes[0].rows[0].snapshot.request.caller_rva=p::compose_caller;
        phase::thread.ticks=1;phase::thread.episode=1;phase::current_ok=true;
    }
    live::compose_begin(s);C(live::pending_count()==1);
    game_state();mode=Mode::normal;invoke(0);C(live::row_count==1&&live::rows[0].before_valid&&live::rows[0].after_valid&&live::rows[0].returned);
    C(live::rows[0].affected==affected&&live::rows[0].out_after.count==3&&live::rows[0].matrices[2].values[12]==12);
    C(live::rows[0].before.methods[1].current_prefix[0]==0xe9); // Actual patched bytes, not original certificate bytes.
    invoke(2);C(live::row_count==2&&live::rows[1].kind==2&&live::rows[1].after_valid&&live::rows[1].linked_row==1&&live::rows[1].compute_ordinal==1);
    C(live::rows[1].weight==.625f&&live::rows[1].weight_bits==0x3f200000);
    C(live::rows[0].weight_bits==0&&!live::rows[0].weight_finite); // Compute XMM0 is raw, not future weight.
    game_state(1);invoke(1);C(live::rows[2].after_valid&&live::rows[2].out_after.count==1&&live::rows[2].affected[0]==8);
    invoke(2);C(live::rows[3].linked_row==3&&live::rows[3].after_valid);
    auto n=live::row_count;invoke(2,base+0x12911f,node+8);C(live::rows[n].failure==live::Failure::prior_compute);
    game_state();n=live::row_count;const auto before_bad=game_reads;invoke(0,base+0x12345);C(live::rows[n].failure==live::Failure::caller&&game_reads==before_bad);
    n=live::row_count;invoke(0,0,node,true);C(live::rows[n].failure==live::Failure::arguments);
    n=live::row_count;invoke(0,0,node,false,999);C(live::rows[n].failure==live::Failure::arguments);
    n=live::row_count;invoke(0,0,node,false,11,mesh+8);C(live::rows[n].failure==live::Failure::arguments);
    n=live::row_count;const auto stack_reads=game_reads;invoke(0,0,node,false,11,mesh,true);C(live::rows[n].failure==live::Failure::stack&&game_reads==stack_reads);
    for(auto selected:{Mode::node_changed,Mode::header_changed,Mode::count,Mode::nan,Mode::affected_changed,Mode::alias_data,Mode::payload_changed,Mode::read_failed}){
        game_state();mode=selected;n=live::row_count;invoke(0);C(live::row_count==n+1&&live::rows[n].before_valid&&!live::rows[n].after_valid&&live::rows[n].returned);}
    game_state();mode=Mode::normal;invoke(0);++weapon_hand_scene_binding::node.number;n=live::row_count;invoke(2);C(live::rows[n].failure==live::Failure::prior_compute&&!live::rows[n].after_valid);
    game_state();invoke(0);mode=Mode::weight_nan;n=live::row_count;invoke(2);C(!live::rows[n].weight_finite&&live::rows[n].failure==live::Failure::shape);
    game_state();mode=Mode::exception_seh;n=live::row_count;C(catch_seh());C(live::rows[n].exception&&!live::rows[n].returned&&live::rows[n].failure==live::Failure::exception);C(active==0&&live::in_flight==0&&live::pending_count()==1);
    game_state();mode=Mode::exception_cpp;n=live::row_count;bool caught{};try{invoke(0);}catch(CppException const& e){caught=e.value==0x9234;}
    C(caught&&live::rows[n].exception&&active==0&&live::in_flight==0);
    game_state();mode=Mode::nested;n=live::row_count;const auto old_originals=original_calls.load();invoke(0);
    C(original_calls==old_originals+2&&live::rows[n].nested&&!live::rows[n].after_valid&&live::row_count==n+1);
    game_state();mode=Mode::normal;
    for(unsigned attempt=0;attempt<live::row_limit&&live::row_count<live::row_limit;++attempt)invoke(0);
    C(live::row_count==live::row_limit);
    const auto full_reads=game_reads;invoke(0);C(live::row_count==128&&live::dropped>0&&game_reads==full_reads);
    auto nested_scope=s;nested_scope.invocation=901;live::compose_begin(nested_scope);const auto no_reads=game_reads;invoke(0);C(game_reads==no_reads);
    live::compose_end(nested_scope,true);C(live::pending_count()==1);
    phase::episodes[0].concurrent=true;auto wrong=s;wrong.invocation=999;live::compose_end(wrong,true);C(live::pending_count()==1&&live::failed.load());
    live::compose_end(s,true);C(!live::pending_count()&&active==0);C(live::rows[0].concurrent&&!live::rows[0].after_valid);
    // A second event is not inherited, nor relabelled as the first.
    auto second=scope(902,2);phase::thread.episode=2;live::compose_begin(second);const auto prior=game_reads;invoke(0);C(game_reads==prior&&live::row_count==128);live::compose_end(second,true);phase::thread.episode=1;C(!live::pending_count());
    // Residency covers even unadmitted originals and stop overlap.
    game_state();mode=Mode::block;entered=allow_return=false;std::thread pending([]{invoke(2);});
    {std::unique_lock<std::mutex> lock(wait_mutex);C(wait_cv.wait_for(lock,std::chrono::seconds(5),[]{return entered;}));}
    live::request_drain();C(live::pending_count()==1);live::print(nullptr,true);C(!live::file_attempted.load());
    {std::lock_guard<std::mutex> lock(wait_mutex);allow_return=true;}wait_cv.notify_all();pending.join();C(!live::pending_count()&&active==0);
}
#endif
int wmain(int argc,wchar_t** argv){C(argc==2);if(argc!=2)return 1;
    C(swprintf_s(directory,L"%s\\case-%lu-%llu",argv[1],GetCurrentProcessId(),GetTickCount64())>0);const bool made=CreateDirectoryW(directory,nullptr)!=FALSE;C(made);if(!made)return 1;
    wchar_t ini[MAX_PATH]{};swprintf_s(ini,L"%s\\observer.ini",directory);
    C(WritePrivateProfileStringW(L"CoverControlObserver",L"enabled",L"0",ini));C(live::configure(ini,nullptr)&&live::prepare()&&live::reserve());
    C(WritePrivateProfileStringW(L"CoverControlObserver",L"enabled",L"invalid",ini));C(!live::configure(ini,nullptr));
    C(WritePrivateProfileStringW(L"CoverControlObserver",L"enabled",L"1",ini));
#if GEARS_COVER_CONTROL_OBSERVER && GEARS_COVER_BODY_PROTOTYPE
    for(unsigned i=0;i<3;++i){code[i].address=base+gears_cover_control_certificate::hooks[i].rva;code[i].bytes=gears_cover_control_certificate::hooks[i].prefix;}
    for(unsigned i=3;i<6;++i){code[i].address=base+0x700000+(i-3)*0x100;code[i].bytes.fill(0xcc);}
    C(live::configure(ini,nullptr));phase::digest_ok=false;C(!live::prepare()&&!creates);phase::digest_ok=true;live::failed=false;
    const bool hooks_ready=live::reserve()&&creates==3&&live::reserved_mask==7;C(hooks_ready);if(!hooks_ready)return 1;
    C(phase::digest_calls==11);C(live::reserved_target(live::targets[1]));
    for(unsigned i=0;i<3;++i)code[i].bytes[0]=0xe9; // Synthetic owned entry patch, never executed.
    collect_cases();live::print(nullptr,true);C(live::file_written.load());
    wchar_t path[MAX_PATH]{};swprintf_s(path,L"%s\\control-node-phases.bin",directory);HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    C(file!=INVALID_HANDLE_VALUE);live::Header h{};DWORD n{};if(file!=INVALID_HANDLE_VALUE){C(ReadFile(file,&h,sizeof h,&n,nullptr)&&n==sizeof h);C(CloseHandle(file)!=FALSE);}
    C(!memcmp(h.magic,"CCTRL001",8)&&h.row_size==sizeof(live::Row)&&h.count==128&&h.read_only==1&&!h.atomic_verified&&!h.writer);
    live::print(nullptr,true);C(live::file_written.load());
    printf("cover_control_observer_live: %u checks, %u failures; Row=%zu Header=%zu synthetic only/no native authority\n",checks,failures,sizeof(live::Row),sizeof(live::Header));
#else
    C(!live::configure(ini,nullptr));C(!live::enabled&&!live::pending_count());phase::Scope s{};live::compose_begin(s);live::compose_end(s,true);live::request_drain();C(!live::pending_count());
    printf("cover_control_observer_disabled: %u checks, %u failures\n",checks,failures);
#endif
    return failures?1:0;
}
