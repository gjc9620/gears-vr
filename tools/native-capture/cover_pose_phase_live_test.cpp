#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#define main pose_core_main
#include "cover_pose_phase_test.cpp"
#undef main
namespace gears_anchor {
struct Identity {uintptr_t address{},cls{},outer{};unsigned index{},name_index{},number{};};
struct Receipt {uint64_t invocation{};unsigned thread{};LONGLONG scene_qpc{};};
}
namespace gears_vr {struct StereoResult {struct Sample {uint64_t session{1};}sample;explicit operator bool() const{return true;}};}
std::atomic<unsigned> active{};std::atomic<bool> recording{true};std::atomic<uint64_t> committed_world_generation{3};
bool continuous_session=true,stereo_xr=true,anchor_observer_enabled=true;LONGLONG session_started_qpc=4;wchar_t directory[MAX_PATH]{};
namespace weapon_hand_scene_binding {
struct Binding {gears_anchor::Identity world{},pawn{},mesh{};uint64_t world_generation{3};bool context_valid{true};};
std::mutex mutex;Binding actor;bool metadata_ok=true;
bool same(gears_anchor::Identity const& a,gears_anchor::Identity const& b){return !memcmp(&a,&b,sizeof(a));}
bool metadata_current(){return metadata_ok;}
bool identity(uintptr_t address,gears_anchor::Identity& out,bool){for(auto p:{&actor.world,&actor.pawn,&actor.mesh})if(p->address==address){out=*p;return true;}return false;}
bool sample(gears_anchor::Receipt const*,Binding& out){out=actor;return metadata_ok;}
}
struct Code {uintptr_t rva{};std::vector<unsigned char> bytes;};std::vector<Code> code;
Fixture* fixture{};
bool safe_copy(void* out,void const* in,size_t count){const auto address=reinterpret_cast<uintptr_t>(in);
    for(auto const& c:code)if(address>=base+c.rva&&address-base-c.rva<=c.bytes.size()&&count<=c.bytes.size()-(address-base-c.rva)){memcpy(out,c.bytes.data()+address-base-c.rva,count);return true;}
    return fixture&&(*fixture)(out,in,count);
}
constexpr int MH_OK=0;unsigned creates{};
int MH_CreateHook(void* target,void*,void** original){++creates;const auto rva=reinterpret_cast<uintptr_t>(target)-base;
    *original=reinterpret_cast<void*>(uintptr_t(rva==p::compose_rva?1:rva==p::physics_rva?2:3));return MH_OK;}
namespace cover_body_live {using CallPacket=p::CallPacket;extern "C" void GearsCoverRawInvoke(CallPacket*,void*) noexcept(false);}
#include "cover_pose_phase_live.inl"
namespace live=cover_pose_phase_live;
bool entry_ok=true;bool live::own_entries(){return entry_ok;}
extern "C" void GearsCoverPoseComposeHook(){}extern "C" void GearsCoverPosePhysicsHook(){}extern "C" void GearsCoverPoseTickHook(){}
enum class Mode {normal,missing,exception,changed,duplicate,concurrent,bad_args};Mode mode{};
unsigned originals{};constexpr uint64_t sentinel=0xfedcba9876543210ull;constexpr DWORD pose_exception=0xe042c090;
void invoke(unsigned kind,uintptr_t ra,uintptr_t receiver=mesh){p::CallPacket packet{};packet.gpr[1]=receiver;
    packet.gpr[2]=kind?mesh+0xa0:mesh+0x27c;packet.gpr[3]=mesh+0x28c;
    if(mode==Mode::bad_args&&kind==0)++packet.gpr[2];live::GearsCoverPoseInvokeProtected(&packet,kind,base+ra);C(packet.gpr[0]==sentinel);}
extern "C" void cover_body_live::GearsCoverRawInvoke(CallPacket* packet,void* original) noexcept(false){
    ++originals;C(active.load()>0&&live::in_flight.load()>0);const auto kind=reinterpret_cast<uintptr_t>(original);
    if(kind==3){if(mode==Mode::exception)RaiseException(pose_exception,0,0,nullptr);
        if(mode==Mode::concurrent){std::thread other([]{invoke(0,p::compose_caller);});other.join();}
        invoke(0,p::compose_caller,packet->gpr[1]);if(mode==Mode::duplicate)invoke(0,p::compose_caller,packet->gpr[1]);
        if(mode!=Mode::missing){invoke(1,p::physics_main_caller,packet->gpr[1]);invoke(1,p::physics_late_caller,packet->gpr[1]);}
    }else if(kind==1&&mode==Mode::changed)++weapon_hand_scene_binding::actor.mesh.number;
    else if(kind==2)packet->gpr[2]=1; // Production must freeze physics entry RDX.
    packet->gpr[0]=sentinel;
}
unsigned catch_original(){__try{invoke(2,0x123456);return 0;}__except(GetExceptionCode()==pose_exception?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return GetExceptionCode();}}
void publish(){auto& a=weapon_hand_scene_binding::actor;a.world={base+0x700,1,2,3,4,0};a.pawn={base+0x800,1,2,4,5,0};a.mesh={mesh,1,2,5,6,0};
    gears_anchor::Receipt scene{};scene.invocation=5;scene.thread=GetCurrentThreadId();scene.scene_qpc=live::qpc();live::scene(&scene,{});}
void round(Mode value){publish();mode=value;C(SetEvent(live::capture_event)!=FALSE);invoke(2,0x123456);C(!active.load()&&!live::pending_count()&&!live::thread.ticks);}
int wmain(int argc,wchar_t** argv){C(argc==3);if(argc!=3)return 1;FILE* file{};C(_wfopen_s(&file,argv[1],L"rb")==0&&file);
    while(file){uint64_t rva{};unsigned size{};if(fread(&rva,8,1,file)!=1)break;C(fread(&size,4,1,file)==1);Code c;c.rva=uintptr_t(rva);c.bytes.resize(size);C(fread(c.bytes.data(),1,size,file)==size);code.push_back(c);}if(file)fclose(file);C(code.size()==8);
    // Keep CREATE_NEW output per invocation, including repeated CTest runs.
    C(swprintf_s(directory,L"%s\\case-%lu-%llu",argv[2],GetCurrentProcessId(),GetTickCount64())>0);
    C(CreateDirectoryW(directory,nullptr)!=FALSE);
    Fixture fake;fixture=&fake;fake.init();wchar_t ini[MAX_PATH]{};swprintf_s(ini,L"%s\\observer.ini",directory);
    C(WritePrivateProfileStringW(L"CoverPosePhaseObserver",L"enabled",L"0",ini));C(live::configure(ini,nullptr)&&live::reserve()&&!creates);
    C(WritePrivateProfileStringW(L"CoverPosePhaseObserver",L"enabled",L"1",ini));C(live::configure(ini,nullptr));
    code[7].bytes[5]^=1;C(!live::prepare()&&!creates);code[7].bytes[5]^=1;live::failed=false;
    C(live::reserve()&&creates==3&&live::certified);publish();mode=Mode::normal;
    C(SetEvent(live::capture_event));invoke(2,0x123456,mesh+8);C(live::episode_count==0&&!active.load());ResetEvent(live::capture_event);
    auto owner=live::selected;live::selected.key.scene_qpc=1;C(SetEvent(live::capture_event));invoke(2,0x123456);C(live::episode_count==0);ResetEvent(live::capture_event);live::selected=owner;
    C(SetEvent(live::capture_event));p::CallPacket no_ra{};no_ra.gpr[1]=mesh;live::GearsCoverPoseInvokeProtected(&no_ra,2,0);C(live::episode_count==0&&!active.load());ResetEvent(live::capture_event);
    round(Mode::normal);C(live::episodes[0].accepted==0x7f&&live::episodes[0].closed);C(live::episodes[0].rows[3].snapshot.request.physics_matrix==mesh+0xa0);
    round(Mode::missing);C(live::episodes[1].seen==0x43&&live::episodes[1].accepted==0x43);
    publish();mode=Mode::exception;C(SetEvent(live::capture_event));C(catch_original()==pose_exception);C(!active.load()&&!live::pending_count()&&!live::thread.ticks&&live::episodes[2].exception);
    round(Mode::changed);C(live::episodes[3].accepted!=0x7f&&!live::episodes[3].rows[1].snapshot_valid);
    round(Mode::duplicate);C(live::episodes[4].duplicate&&live::episodes[4].rows[0].attempts==2);
    round(Mode::concurrent);C(live::episodes[5].concurrent);
    round(Mode::bad_args);C(!live::episodes[6].rows[0].snapshot_valid&&!live::episodes[6].rows[1].snapshot_valid);
    round(Mode::normal);round(Mode::normal);C(live::episode_count==8);
    live::request_drain();live::print(nullptr,true);C(live::file_written.load()&&live::capture_event==nullptr);
    std::printf("cover_pose_phase_live: %u checks, %u failures; production composition, no game\n",checks,failures);return failures?1:0;}
