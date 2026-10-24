// Include after weapon_hand_scene_binding and cover_body_live. Observation only.
#include "cover_pose_phase.h"
#include "cover_pose_code_certificate.h"
#include <bcrypt.h>
namespace cover_pose_phase_live {
namespace pose=gears_cover_pose_phase;namespace certificate=gears_cover_pose_certificate;
namespace binding=weapon_hand_scene_binding;
bool enabled{},reserved{},certified{};
std::array<void*,3> targets{},originals{};
bool own_entries(); // Root: manifest-ready + read_attested_hook_entry of all 3 prefixes.
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
constexpr unsigned episode_limit=8,row_limit=7;
enum class Failure:unsigned {missing,okay,owner,stale,caller,arguments,read,duplicate,exception};
struct Owner {binding::Binding binding{};pose::OwnerKey key{};unsigned thread{};bool valid{};};
struct Row {unsigned attempts{};Failure failure{Failure::missing};pose::Reject reject{};bool snapshot_valid{};pose::Snapshot snapshot{};};
struct Episode {pose::OwnerKey owner{};uint64_t invocation{};unsigned thread{},seen{},accepted{};bool closed{},exception{},concurrent{},nested{},duplicate{};std::array<Row,row_limit> rows{};};
std::mutex mutex;Owner selected{};std::array<Episode,episode_limit> episodes{};unsigned episode_count{};
std::atomic<unsigned> in_flight{},matching_ticks{},capturing{};
std::atomic<bool> stopping{},failed{};std::atomic<uint64_t> serial{1},calls{},rejected{},duplicates{},changed{},stale{},no_caller{};
HANDLE capture_event{};std::atomic<bool> file_attempted{},file_written{};std::atomic<DWORD> file_error{};
struct Thread {unsigned ticks{},episode{};};inline thread_local Thread thread{};
inline thread_local pose::Snapshot scratch{},output{};
struct Scope {Owner owner{};uint64_t invocation{};uintptr_t matrix{};unsigned kind{},caller{},episode{};bool tick{},matching{},started{},arguments{};};
static_assert(std::is_trivially_copyable_v<Scope>&&std::is_trivially_destructible_v<Scope>);
LONGLONG qpc(){LARGE_INTEGER v{};return QueryPerformanceCounter(&v)?v.QuadPart:0;}
bool fresh(pose::OwnerKey const& key){LARGE_INTEGER f{};const auto t=qpc();return QueryPerformanceFrequency(&f)&&f.QuadPart>0&&key.scene_qpc>0&&t>=key.scene_qpc&&t-key.scene_qpc<=f.QuadPart/10;}
pose::Identity compact(gears_anchor::Identity const& id){return {id.address,id.cls,id.outer,int32_t(id.index),int32_t(id.name_index),id.number};}
bool same(pose::Identity const& a,pose::Identity const& b){return a.address==b.address&&a.cls==b.cls&&a.outer==b.outer&&a.internal_index==b.internal_index&&a.name_index==b.name_index&&a.number==b.number;}
bool same(pose::OwnerKey const& a,pose::OwnerKey const& b){return same(a.world,b.world)&&same(a.pawn,b.pawn)&&same(a.mesh,b.mesh)&&a.world_generation==b.world_generation&&a.session==b.session&&a.scene==b.scene&&a.scene_qpc==b.scene_qpc;}
Owner owner_copy(){std::lock_guard<std::mutex> lock(mutex);return selected;}
bool current(Owner const& o){
    if(!o.valid||!recording.load()||stopping.load()||o.thread!=GetCurrentThreadId()||o.key.session!=uint64_t(session_started_qpc)||!own_entries())return false;
    const auto copy=owner_copy();if(!copy.valid||!same(copy.key,o.key))return false;
    if(!fresh(o.key)){++stale;return false;}
    std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);if(!lock.owns_lock()||!binding::metadata_current()||o.key.world_generation!=committed_world_generation.load())return false;
    gears_anchor::Identity w{},p{},m{};
    return binding::identity(o.key.world.address,w,true)&&binding::identity(o.key.pawn.address,p,true)&&binding::identity(o.key.mesh.address,m,true)&&
        binding::same(w,o.binding.world)&&binding::same(p,o.binding.pawn)&&binding::same(m,o.binding.mesh)&&binding::metadata_current()&&o.key.world_generation==committed_world_generation.load();
}
bool digest(certificate::Chunk const& c){
    if(!base||!c.size||c.size>65536||base>UINTPTR_MAX-c.rva||base+c.rva>UINTPTR_MAX-c.size)return false;
    BCRYPT_ALG_HANDLE a{};BCRYPT_HASH_HANDLE h{};bool okay=BCryptOpenAlgorithmProvider(&a,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(okay)okay=BCryptCreateHash(a,&h,nullptr,0,nullptr,0,0)>=0;
    std::array<unsigned char,1024> bytes{};unsigned char value[32]{};
    for(unsigned at=0;okay&&at<c.size;){const auto n=(std::min)(unsigned(bytes.size()),c.size-at);
        okay=safe_copy(bytes.data(),reinterpret_cast<void const*>(base+c.rva+at),n)&&BCryptHashData(h,bytes.data(),n,0)>=0;at+=n;}
    if(okay)okay=BCryptFinishHash(h,value,sizeof value,0)>=0;
    if(h)BCryptDestroyHash(h);if(a)BCryptCloseAlgorithmProvider(a,0);
    constexpr char hex[]="0123456789abcdef";if(okay)for(unsigned i=0;i<32;++i)if(c.sha256[i*2]!=hex[value[i]>>4]||c.sha256[i*2+1]!=hex[value[i]&15])return false;return okay;
}
bool prepare(){if(!enabled)return true;if(certified||reserved)return certified;
    for(auto const& c:certificate::chunks)if(!digest(c)||!digest(c)){failed=true;return false;}certified=true;return true;}
extern "C" void GearsCoverPoseComposeHook();extern "C" void GearsCoverPosePhysicsHook();extern "C" void GearsCoverPoseTickHook();
bool reserve(){if(!enabled)return true;if(reserved||!prepare())return false;
    const std::array<void*,3> detours{{reinterpret_cast<void*>(GearsCoverPoseComposeHook),reinterpret_cast<void*>(GearsCoverPosePhysicsHook),reinterpret_cast<void*>(GearsCoverPoseTickHook)}};
    for(unsigned i=0;i<3;++i){targets[i]=reinterpret_cast<void*>(base+certificate::hooks[i].rva);std::array<unsigned char,32> before{},after{};
        if(!safe_copy(before.data(),targets[i],32)||before!=certificate::hooks[i].prefix||MH_CreateHook(targets[i],detours[i],&originals[i])!=MH_OK||!safe_copy(after.data(),targets[i],32)||before!=after){failed=true;return false;}}
    reserved=true;return true;
}
bool reserved_target(void* target){return reserved&&(target==targets[0]||target==targets[1]||target==targets[2]);}
bool configure(wchar_t const* ini,FILE* file){const auto value=GetPrivateProfileIntW(L"CoverPosePhaseObserver",L"enabled",0,ini);
    if(value>1||(value&&(!continuous_session||!stereo_xr||!anchor_observer_enabled||enabled)))return false;
    if(value){wchar_t name[128]{};swprintf_s(name,L"Local\\GearsVR.CoverPoseCapture.%lu",GetCurrentProcessId());SetLastError(0);
        capture_event=CreateEventW(nullptr,FALSE,FALSE,name);const auto error=GetLastError();
        if(!capture_event||error==ERROR_ALREADY_EXISTS){if(capture_event)CloseHandle(capture_event);capture_event=nullptr;return false;}enabled=true;}
    if(file)fprintf(file,"cover_pose_phase enabled=%u read_only=1 episodes_max=8 snapshots_max=56 event_arms_next_tick=1 atomic_snapshot_verified=0 final_publish_verified=0\n",unsigned(enabled));return true;
}
void scene(gears_anchor::Receipt const* scene,gears_vr::StereoResult const& stereo){
    if(!enabled||stopping.load())return;Owner next{};
    if(scene&&stereo&&stereo.sample.session&&session_started_qpc>0&&scene->thread==GetCurrentThreadId()&&binding::sample(scene,next.binding)){
        next.key.world=compact(next.binding.world);next.key.pawn=compact(next.binding.pawn);next.key.mesh=compact(next.binding.mesh);
        next.key.world_generation=next.binding.world_generation;next.key.session=uint64_t(session_started_qpc);next.key.scene=scene->invocation;next.key.scene_qpc=scene->scene_qpc;next.thread=scene->thread;
        next.valid=next.binding.context_valid&&next.key.world.address&&next.key.pawn.address&&next.key.mesh.address&&fresh(next.key);
    }
    std::lock_guard<std::mutex> lock(mutex);selected=next;
}
void mark_concurrent(){const auto index=capturing.load();if(index){std::lock_guard<std::mutex> lock(mutex);episodes[index-1].concurrent=true;}}
void sample(Scope const& scope,unsigned slot,pose::Phase phase,bool args=true){
    if(!scope.episode)return;auto& e=episodes[scope.episode-1];
    {std::lock_guard<std::mutex> lock(mutex);auto& row=e.rows[slot];if(row.attempts++){++duplicates;e.duplicate=true;return;}e.seen|=1u<<slot;}
    pose::Request request{};request.owner=scope.owner.key;request.invocation=scope.invocation;request.qpc=qpc();request.thread=GetCurrentThreadId();
    request.caller_rva=scope.caller;request.kind=static_cast<pose::Kind>(scope.kind);request.phase=phase;request.physics_matrix=scope.matrix;
    Failure reason=Failure::okay;pose::Reject reject=pose::Reject::none;bool valid=false;
    if(!args)reason=Failure::arguments;
    else if(!pose::caller_allowed(request.kind,request.caller_rva))reason=Failure::caller;
    else if(!current(scope.owner)){reason=fresh(scope.owner.key)?Failure::owner:Failure::stale;++changed;}
    else {auto read=[](void* out,void const* in,size_t size){return safe_copy(out,in,size);};reject=pose::capture(read,request,scratch,output);
        if(reject!=pose::Reject::none)reason=Failure::read;
        else if(!current(scope.owner)){reason=Failure::owner;++changed;}else valid=true;}
    {std::lock_guard<std::mutex> lock(mutex);auto& row=e.rows[slot];row.failure=reason;row.reject=reject;row.snapshot_valid=valid;
        if(valid){row.snapshot=output;e.accepted|=1u<<slot;}
        else if(reason==Failure::read)row.snapshot=scratch;
        else row.snapshot.request=request;}
}
void begin(pose::CallPacket const& packet,unsigned kind,uintptr_t ra,Scope& scope){
    scope.kind=kind;scope.invocation=serial.fetch_add(1);if(!scope.invocation){failed=true;stopping=true;return;}
    if(kind==2){scope.tick=true;++thread.ticks;if(thread.ticks!=1){const auto old=thread.episode;if(old){std::lock_guard<std::mutex> lock(mutex);episodes[old-1].nested=true;}return;}}
    if(!enabled||stopping.load()||kind>2)return;
    auto owner=owner_copy();if(!owner.valid||packet.gpr[1]!=owner.key.mesh.address)return;
    if(kind==2){scope.matching=true;if(matching_ticks.fetch_add(1))mark_concurrent();}
    else if(!thread.episode){mark_concurrent();return;}
    if(ra<=base||ra-base>UINT32_MAX){++no_caller;return;}scope.caller=unsigned(ra-base);
    if(!pose::caller_allowed(static_cast<pose::Kind>(kind),scope.caller)||!current(owner)){++rejected;return;}
    if(kind==2){
        if(capturing.load()||!capture_event||WaitForSingleObject(capture_event,0)!=WAIT_OBJECT_0)return;
        std::lock_guard<std::mutex> lock(mutex);if(episode_count==episode_limit)return;
        auto& e=episodes[episode_count];e.owner=owner.key;e.invocation=scope.invocation;e.thread=GetCurrentThreadId();e.concurrent=matching_ticks.load()>1;
        scope.episode=++episode_count;scope.started=true;thread.episode=scope.episode;capturing.store(scope.episode);scope.owner=owner;return;
    }
    if(thread.ticks!=1)return;scope.episode=thread.episode;scope.owner=owner;
    {std::lock_guard<std::mutex> lock(mutex);if(!same(episodes[scope.episode-1].owner,owner.key)){episodes[scope.episode-1].nested=true;scope.episode=0;++changed;return;}}
    if(kind==0){uintptr_t local_header{},desired_header{};scope.arguments=pose::add(owner.key.mesh.address,0x27c,local_header)&&pose::add(owner.key.mesh.address,0x28c,desired_header)&&packet.gpr[2]==local_header&&packet.gpr[3]==desired_header;
        sample(scope,0,pose::Phase::C0,scope.arguments);}
    else {scope.matrix=packet.gpr[2];sample(scope,scope.caller==pose::physics_main_caller?2u:4u,pose::Phase::P0);}
}
void finish(Scope const& scope,bool returned){
    if(!scope.episode)return;if(!returned){std::lock_guard<std::mutex> lock(mutex);episodes[scope.episode-1].exception=true;return;}
    if(scope.kind==2&&scope.started)sample(scope,6,pose::Phase::L1);
    else if(scope.kind==0)sample(scope,1,pose::Phase::C1,scope.arguments);
    else if(scope.kind==1)sample(scope,scope.caller==pose::physics_main_caller?3u:5u,pose::Phase::P1);
}
void release(Scope const& scope){
    if(scope.started){std::lock_guard<std::mutex> lock(mutex);episodes[scope.episode-1].closed=true;thread.episode=0;capturing.store(0);}
    if(scope.matching)--matching_ticks;if(scope.tick){if(thread.ticks)--thread.ticks;else failed=true;}
}
void begin_guarded(pose::CallPacket const& packet,unsigned kind,uintptr_t ra,Scope& scope){try{begin(packet,kind,ra,scope);}catch(...){failed=true;stopping=true;}}
void finish_guarded(Scope const& scope,bool returned){try{finish(scope,returned);}catch(...){failed=true;stopping=true;}}
void release_guarded(Scope const& scope){try{release(scope);}catch(...){failed=true;stopping=true;}}
extern "C" __declspec(noinline) void GearsCoverPoseInvokeProtected(pose::CallPacket* packet,unsigned kind,uintptr_t ra) noexcept(false){
    Scope scope{};bool returned{};++active;++in_flight;++calls;
    __try{begin_guarded(*packet,kind,ra,scope);
        if(kind<3)cover_body_live::GearsCoverRawInvoke(reinterpret_cast<cover_body_live::CallPacket*>(packet),originals[kind]);else failed=true;
        returned=true;
    }__finally{__try{finish_guarded(scope,returned&&!AbnormalTermination());}__finally{release_guarded(scope);--in_flight;--active;}}
}
void request_drain(){stopping=true;}
unsigned pending_count(){return in_flight.load()+unsigned(capturing.load()!=0);}
void summary(FILE* file){std::lock_guard<std::mutex> lock(mutex);if(file)fprintf(file,"cover_pose_enabled=%u cover_pose_certified=%u cover_pose_reserved=%u cover_pose_calls=%llu cover_pose_inflight=%u cover_pose_episodes=%u cover_pose_rejected=%llu cover_pose_duplicates=%llu cover_pose_changed=%llu cover_pose_stale=%llu cover_pose_no_caller=%llu cover_pose_failed=%u cover_pose_file_written=%u cover_pose_file_error=%lu ",unsigned(enabled),unsigned(certified),unsigned(reserved),calls.load(),in_flight.load(),episode_count,rejected.load(),duplicates.load(),changed.load(),stale.load(),no_caller.load(),unsigned(failed.load()),unsigned(file_written.load()),file_error.load());}
void print(FILE* file,bool disabled){
    if(enabled&&disabled&&stopping.load()&&!pending_count()&&!file_attempted.exchange(true)){wchar_t path[MAX_PATH]{};
        struct Header {char magic[8];unsigned version,snapshot_size,row_size,episode_size,count;uint64_t session;unsigned read_only,atomic_verified,final_publish_verified;};
        Header header{{'C','P','O','S','E','0','0','1'},1,sizeof(pose::Snapshot),sizeof(Row),sizeof(Episode),episode_count,uint64_t(session_started_qpc),1,0,0};
        if(directory[0]&&swprintf_s(path,L"%s\\cover-pose-phases.bin",directory)>0){HANDLE out=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
            if(out!=INVALID_HANDLE_VALUE){DWORD written{};bool okay=WriteFile(out,&header,sizeof header,&written,nullptr)&&written==sizeof header;
                const auto bytes=DWORD(sizeof(Episode)*episode_count);if(okay&&bytes)okay=WriteFile(out,episodes.data(),bytes,&written,nullptr)&&written==bytes;
                if(!okay)file_error=GetLastError();CloseHandle(out);file_written=okay;
            }else file_error=GetLastError();}else file_error=ERROR_INVALID_NAME;
        if(capture_event){CloseHandle(capture_event);capture_event=nullptr;}
    }
    summary(file);if(!file)return;fprintf(file,"read_only=1 atomic_snapshot_verified=0 final_publish_verified=0\n");
    std::lock_guard<std::mutex> lock(mutex);for(unsigned i=0;i<episode_count;++i){auto const& e=episodes[i];fprintf(file,"cover_pose_episode index=%u invocation=%llu thread=%u seen=0x%x accepted=0x%x missing=0x%x closed=%u exception=%u concurrent=%u nested=%u duplicate=%u\n",i,e.invocation,e.thread,e.seen,e.accepted,0x7fu&~e.seen,unsigned(e.closed),unsigned(e.exception),unsigned(e.concurrent),unsigned(e.nested),unsigned(e.duplicate));}
}
#else
bool prepare(){return true;}bool reserve(){return true;}bool reserved_target(void*){return false;}
bool configure(wchar_t const* ini,FILE*){return GetPrivateProfileIntW(L"CoverPosePhaseObserver",L"enabled",0,ini)==0;}
void scene(gears_anchor::Receipt const*,gears_vr::StereoResult const&){}void request_drain(){}unsigned pending_count(){return 0;}
void summary(FILE* file){if(file)fprintf(file,"cover_pose_enabled=0 cover_pose_compiled=0 ");}void print(FILE* file,bool){summary(file);}
#endif
} // namespace cover_pose_phase_live
