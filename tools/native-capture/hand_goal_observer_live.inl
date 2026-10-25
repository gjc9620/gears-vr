// Global include hand_core_target_adapter.h first. Include after CoreObserver.
// Event-only planning from its SAME accepted latch copy, before its record lock.
// No Game/node/Local/Space/input write, engine call, bridge or additional getter.
namespace cover_control_observer_live {struct Row;}
namespace hand_goal_observer_live {
bool enabled{};
#if defined(GEARS_HAND_GOAL_OBSERVER) && GEARS_HAND_GOAL_OBSERVER && defined(GEARS_COVER_NATIVE_RIG_OBSERVER) && GEARS_COVER_NATIVE_RIG_OBSERVER && defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE && defined(GEARS_COVER_CONTROL_OBSERVER) && GEARS_COVER_CONTROL_OBSERVER
namespace adapter=gears_hand_core_target;namespace mount=gears_hand_authored_mount;namespace math=gears_cover_control_output_pose;
namespace core=gears_native_rig_tracking;namespace rig=rig_tracking_live;namespace native=cover_native_rig_observer_live;
namespace phase=cover_pose_phase_live;namespace controls=cover_control_observer_live;namespace pose=gears_cover_pose_phase;namespace binding=weapon_hand_scene_binding;
constexpr unsigned capacity=4;constexpr uint64_t maximum_age_ns=50000000;
using LengthMode=gears_hand_two_bone::LengthMode;LengthMode length_mode=LengthMode::preserve;
enum class Failure:unsigned {none,stopped,context,source_changed,c0,metadata,identity,geometry,scale,role,preparation,math,alias,exception};
struct Header {
    char magic[8]{};unsigned version{},header_size{},record_size{},count{},capacity{},read_only{},writer{},atomic_verified{};
    core::NativeSessionKey native{};uint64_t outer_tick{},scene{},world_generation{};int64_t scene_qpc{};
    unsigned episode{},mode{},failed{},reserved{};mount::Signature profile_signature{};char profile_id[48]{};
};
struct Record {
    uint64_t outer_tick{},compose{},ordinal{},node{};int64_t observed_ns{},before_qpc{},after_qpc{};
    unsigned episode{},kind{},side{unsigned(mount::Side::unknown)},hand_bone{},mode{},original_count{},flags{};Failure failure{Failure::context};
    adapter::Status prepare_status{adapter::Status::invalid_core},math_status{adapter::Status::invalid_target};
    gears_weapon_hand::ObjectIdentity asset{};mount::Signature geometry_signature{};gears_vr::Pose wrist_world{};
    math::Matrix64 original[3]{},planned[3]{},component_to_world{};core::CoreSnapshot source{};
};
static_assert(std::is_standard_layout_v<Header>&&std::is_trivially_copyable_v<Header>&&std::is_standard_layout_v<Record>&&std::is_trivially_copyable_v<Record>);
std::mutex mutex;Header header{};std::array<Record,capacity> records{};unsigned record_count{};
std::atomic<bool> stopping{},failed{},file_attempted{},file_written{};std::atomic<DWORD> file_error{};
std::atomic<unsigned> in_flight{};std::atomic<uint64_t> observations{},planned{},rejected{},dropped{},preparations[2]{};
struct Hand {adapter::ModelBinding binding{};adapter::Target target{};adapter::Status status{adapter::Status::invalid_target};bool attempted{},seen[2]{};};
struct Workspace {
    core::CoreSnapshot source{};pose::OwnerKey owner{};uint64_t outer_tick{},compose{},ordinal{};unsigned episode{};
    pose::Request c0_request{};pose::Metadata c0_metadata{},live_metadata{};
    std::array<mount::ReferenceRecord68,115> reference{};mount::CanonicalGeometry canonical{};mount::Signature signature{};
    gears_anchor::Identity asset{};Hand hands[2]{};adapter::LimbData limb{};adapter::SingleData single{};adapter::Workspace math_work{};Record record{};
    bool have_source{},have_c0{},have_model{},sticky{};Failure sticky_reason{Failure::none};
    Workspace()=default;Workspace(Workspace const&)=delete;Workspace& operator=(Workspace const&)=delete;
};
inline thread_local Workspace workspace{};
struct Call {Call(){++in_flight;}~Call(){--in_flight;}Call(Call const&)=delete;Call& operator=(Call const&)=delete;};
bool passive(){return !cover_body_live::enabled&&!cover_input_live::enabled&&!weapon_hand_runtime::detail::engine_restore_enabled;}
bool current(controls::Row const& row,core::CoreSnapshot const& source){
    auto const& f=native::frame;
    if(stopping.load()||!enabled||!passive()||!native::enabled||native::stopping.load()||!f.active||f.sticky||!f.attempted||!f.lease||!f.lease->world()||
       row.kind>1||row.failure!=controls::Failure::okay||!row.before_valid||!row.after_valid||!row.returned||row.exception||row.nested||row.concurrent||row.compose_exception||
       row.episode!=f.episode||row.outer_tick!=f.invocation||row.thread!=GetCurrentThreadId()||row.thread!=f.owner.thread||row.mesh_argument!=f.owner.key.mesh.address||
       !row.compose||!row.ordinal||row.compute_ordinal!=row.ordinal||!phase::same(row.owner,f.owner.key)||
       !rig::owner_matches(f.key.request,source)||!native::current_owner(f.owner)||controls::thread.depth!=1||controls::thread.overflow)return false;
    auto const& c=controls::thread.frames[0];
    return c.admitted&&!c.nested&&c.scope.kind==0&&c.scope.arguments&&c.scope.invocation==row.compose&&c.outer_tick==row.outer_tick&&
        c.scope.episode==row.episode&&phase::same(c.scope.owner.key,row.owner)&&controls::current(c)&&
        row.node_argument==row.before.node.address&&row.node_argument==row.after.node.address&&phase::same(row.before.node,row.after.node);
}
bool copy_c0(controls::Row const& row){
    auto& w=workspace;std::lock_guard<std::mutex> lock(phase::mutex);
    if(!row.episode||row.episode>phase::episode_count)return false;auto const& e=phase::episodes[row.episode-1];auto const& c0=e.rows[0];auto const& s=c0.snapshot;
    if(!c0.snapshot_valid||e.closed||e.exception||e.nested||e.concurrent||e.invocation!=row.outer_tick||e.thread!=row.thread||!phase::same(e.owner,row.owner)||
       !pose::request_valid(s.request)||s.request.kind!=pose::Kind::compose||s.request.phase!=pose::Phase::C0||s.request.invocation!=row.compose||s.request.thread!=row.thread||
       !phase::same(s.request.owner,row.owner)||s.metadata.master||s.metadata.reference.count!=115||!pose::header_valid(s.metadata.reference,68)||
       s.metadata.asset!=row.before.metadata.asset||s.metadata.asset!=row.after.metadata.asset||row.before.metadata.master||row.after.metadata.master||
       !pose::header_equal(s.metadata.reference,row.before.metadata.reference)||!pose::header_equal(s.metadata.reference,row.after.metadata.reference))return false;
    if(w.have_c0)return !memcmp(&w.c0_request,&s.request,sizeof(s.request))&&
        w.c0_metadata.asset==s.metadata.asset&&pose::header_equal(w.c0_metadata.reference,s.metadata.reference)&&
        !memcmp(w.reference.data(),s.reference.data(),sizeof(w.reference));
    w.c0_request=s.request;w.c0_metadata=s.metadata;memcpy(w.reference.data(),s.reference.data(),sizeof(w.reference));w.have_c0=true;return true;
}
bool live_metadata(controls::Row const& row){
    auto& w=workspace;auto read=[](void* out,void const* in,size_t n){return safe_copy(out,in,n);};gears_anchor::Identity first{},last{};
    std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);
    if(!lock.owns_lock()||!binding::metadata_current()||!binding::identity(w.c0_metadata.asset,first,true)||
       !pose::metadata(read,w.c0_request,w.live_metadata)||w.live_metadata.master||
       !controls::same_metadata(w.live_metadata,row.before.metadata)||!controls::same_metadata(w.live_metadata,row.after.metadata)||
       !pose::header_equal(w.live_metadata.reference,w.c0_metadata.reference)||w.live_metadata.reference.count!=115||
       pose::repeat(read,w.reference.data(),w.live_metadata.reference.data,sizeof(w.reference))!=pose::Reject::none||
       !binding::identity(w.c0_metadata.asset,last,true)||!binding::same(first,last)||!binding::metadata_current()||
       (w.have_model&&!binding::same(w.asset,first)))return false;
    if(!w.have_model)w.asset=first;return true;
}
bool geometry(){
    auto& w=workspace;if(!mount::canonical_geometry(w.reference.data(),w.reference.size(),w.canonical))return false;
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};bool okay=BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(okay)okay=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
    if(okay)okay=BCryptHashData(hash,w.canonical.data(),DWORD(w.canonical.size()),0)>=0;
    if(okay)okay=BCryptFinishHash(hash,w.signature.data(),DWORD(w.signature.size()),0)>=0;
    if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);return okay&&w.signature==mount::marcus115_signature;
}
void sticky(Failure reason){if(!workspace.sticky)workspace.sticky_reason=reason;workspace.sticky=true;failed=true;workspace.record.failure=workspace.sticky_reason;}
void commit(){std::lock_guard<std::mutex> lock(mutex);if(record_count<capacity)records[record_count++]=workspace.record;else ++dropped;}
void observe_impl(controls::Row const& row,core::CoreSnapshot const& source){
    if(!enabled||row.kind>1)return;++observations;auto& w=workspace;
    {std::lock_guard<std::mutex> lock(mutex);if(record_count==capacity){++dropped;return;}}
    using namespace math::ik::detail;const Range ranges[]={range(&row,sizeof(row)),range(&source,sizeof(source)),range(&w,sizeof(w))};
    if(!disjoint(ranges,3)){sticky(Failure::alias);++rejected;return;} // Do not overwrite an aliased source to manufacture a diagnostic.
    w.record={};auto& r=w.record;r.outer_tick=row.outer_tick;r.compose=row.compose;r.ordinal=row.ordinal;r.node=row.node_argument;
    r.before_qpc=row.before_qpc;r.after_qpc=row.after_qpc;r.observed_ns=rig::clock_ns();r.episode=row.episode;r.kind=row.kind;r.mode=unsigned(length_mode);
    memcpy(&r.source,&source,sizeof(source));r.original_count=row.kind?1u:3u;memcpy(r.original,row.matrices.data(),r.original_count*sizeof(math::Matrix64));
    memcpy(&r.component_to_world,&row.before.metadata.component,sizeof(math::Matrix64));
    if(w.sticky){r.failure=w.sticky_reason;++rejected;commit();return;}
    if(!current(row,source)){sticky(stopping.load()?Failure::stopped:Failure::context);++rejected;commit();return;}
    if(w.have_source){
        if(!phase::same(w.owner,row.owner)||w.outer_tick!=row.outer_tick||w.compose!=row.compose||w.episode!=row.episode||
           memcmp(&w.source,&source,sizeof(source))||row.ordinal<=w.ordinal){sticky(Failure::source_changed);++rejected;commit();return;}
    }else{memcpy(&w.source,&source,sizeof(source));w.owner=row.owner;w.outer_tick=row.outer_tick;w.compose=row.compose;w.episode=row.episode;w.have_source=true;
        std::lock_guard<std::mutex> lock(mutex);header.native=source.native;header.outer_tick=row.outer_tick;header.scene=source.scene_invocation;header.scene_qpc=source.scene_qpc;header.world_generation=source.world_generation;header.episode=row.episode;}
    w.ordinal=row.ordinal;
    if(!copy_c0(row)){sticky(Failure::c0);++rejected;commit();return;}
    if(!live_metadata(row)){sticky(Failure::metadata);++rejected;commit();return;}
    if(!math::detail::rigid(math::detail::unpack(r.component_to_world))){sticky(Failure::scale);++rejected;commit();return;}
    if(!w.have_model){if(!geometry()){r.geometry_signature=w.signature;sticky(Failure::geometry);++rejected;commit();return;}w.have_model=true;}
    r.asset=rig::object(w.asset);r.geometry_signature=w.signature;
    const auto end=static_cast<uint32_t>(row.raw_rdx);const unsigned side=end==11?0u:end==35?1u:2u;
    if(side>1){sticky(Failure::role);++rejected;commit();return;}r.side=side;r.hand_bone=end;auto& hand=w.hands[side];
    if(!hand.attempted){hand.attempted=true;++preparations[side];auto& b=hand.binding;b.native=source.native;b.world=source.world;b.pawn=source.pawn;b.mesh=source.mesh;b.asset=r.asset;
        b.world_generation=source.world_generation;b.binding_generation=source.binding_generation;b.model=mount::Model::marcus115;b.geometry_signature=w.signature;b.bone_count=115;b.model_to_world_scale=1;
        if(mount::marcus115(w.signature,static_cast<adapter::Side>(side),1,source.units_per_metre,b.authored_mount))
            hand.status=adapter::prepare_hand(w.source,b,static_cast<adapter::Side>(side),r.observed_ns,maximum_age_ns,simulator_operator,{false},hand.target);
        else hand.status=adapter::Status::unknown_mount;
    }
    r.prepare_status=hand.status;
    if(hand.status!=adapter::Status::okay){r.failure=Failure::preparation;++rejected;commit();return;}
    const auto& profile=hand.binding.authored_mount;bool affected=row.affected_before.count==(row.kind?1:3);
    for(unsigned i=0;affected&&i<r.original_count;++i)affected=uint32_t(row.affected[i])==(row.kind?profile.hand_bone:profile.chain[i]);
    if(!affected||hand.seen[row.kind]||!adapter::same_source(hand.target,source)){sticky(Failure::role);++rejected;commit();return;}hand.seen[row.kind]=true;
    r.wrist_world=hand.target.wrist_world;r.flags=1;
    const auto fresh=adapter::detail::source_valid(source,static_cast<adapter::Side>(side),r.observed_ns,maximum_age_ns,simulator_operator,{false});
    if(fresh!=adapter::Status::okay){r.prepare_status=fresh;r.failure=Failure::preparation;++rejected;commit();return;}
    if(row.kind==0){w.limb={};w.limb.asset=r.asset;w.limb.bone_count=115;w.limb.affected_count=3;w.limb.affected=profile.chain;
        memcpy(w.limb.component,r.original,sizeof(w.limb.component));w.limb.component_to_world=r.component_to_world;
        r.math_status=adapter::solve_limb(hand.target,w.limb,length_mode,w.math_work,r.planned);
    }else{w.single={};w.single.asset=r.asset;w.single.bone_count=115;w.single.affected_count=1;w.single.affected=profile.hand_bone;
        w.single.component=r.original[0];w.single.component_to_world=r.component_to_world;r.math_status=adapter::replace_wrist_rotation(hand.target,w.single,w.math_work,r.planned);r.flags|=4;}
    if(!live_metadata(row)||!copy_c0(row)||!current(row,source)||!adapter::same_source(hand.target,source)||
       adapter::detail::source_valid(source,static_cast<adapter::Side>(side),rig::clock_ns(),maximum_age_ns,simulator_operator,{false})!=adapter::Status::okay){
        memset(r.planned,0,sizeof(r.planned));sticky(Failure::metadata);++rejected;commit();return;}
    if(r.math_status==adapter::Status::okay){r.failure=Failure::none;r.flags|=2;++planned;}else{sticky(Failure::math);++rejected;}
    commit();
}
void observe(controls::Row const& row,core::CoreSnapshot const& source) noexcept {Call call;try{observe_impl(row,source);}catch(...){if(!workspace.sticky)workspace.sticky_reason=Failure::exception;failed=true;workspace.sticky=true;++rejected;}}
bool configure(wchar_t const* ini,FILE* file){wchar_t value[16]{},mode[16]{};GetPrivateProfileStringW(L"HandGoalObserver",L"enabled",L"0",value,16,ini);GetPrivateProfileStringW(L"HandGoalObserver",L"length_mode",L"preserve",mode,16,ini);
    if((wcscmp(value,L"0")&&wcscmp(value,L"1"))||(wcscmp(mode,L"preserve")&&wcscmp(mode,L"adaptive")))return false;
    if(!wcscmp(value,L"1")){if(enabled||stopping.load()||!native::enabled||!phase::enabled||!controls::enabled||!rig::enabled||!weapon_hand_runtime::enabled||!continuous_session||!stereo_xr||!anchor_observer_enabled||!passive())return false;
        length_mode=!wcscmp(mode,L"adaptive")?LengthMode::adaptive:LengthMode::preserve;enabled=true;}
    if(file)fprintf(file,"hand_goal_enabled=%u read_only=1 writer=0 authored_profile=marcus115 length_mode=%u single_rotation_only=1\n",unsigned(enabled),unsigned(length_mode));return true;
}
void request_drain(){stopping=true;}unsigned pending_count(){return in_flight.load();}
void summary(FILE* file){if(file){unsigned count{};{std::lock_guard<std::mutex> lock(mutex);count=record_count;}
    fprintf(file,"hand_goal_enabled=%u hand_goal_observations=%llu hand_goal_prepare_left=%llu hand_goal_prepare_right=%llu hand_goal_planned=%llu hand_goal_rejected=%llu hand_goal_records=%u hand_goal_dropped=%llu hand_goal_failed=%u hand_goal_pending=%u hand_goal_file_written=%u hand_goal_file_error=%lu ",unsigned(enabled),static_cast<unsigned long long>(observations.load()),static_cast<unsigned long long>(preparations[0].load()),static_cast<unsigned long long>(preparations[1].load()),static_cast<unsigned long long>(planned.load()),static_cast<unsigned long long>(rejected.load()),count,static_cast<unsigned long long>(dropped.load()),unsigned(failed.load()),pending_count(),unsigned(file_written.load()),file_error.load());}}
void print(FILE* file,bool disabled){
    if(enabled&&disabled&&stopping.load()&&!pending_count()&&!file_attempted.exchange(true)){
        std::lock_guard<std::mutex> lock(mutex);memcpy(header.magic,"HGOAL001",8);header.version=1;header.header_size=sizeof(Header);header.record_size=sizeof(Record);header.count=record_count;header.capacity=capacity;
        header.read_only=1;header.mode=unsigned(length_mode);header.failed=unsigned(failed.load());header.profile_signature=mount::marcus115_signature;memcpy(header.profile_id,mount::profile_id,sizeof(mount::profile_id));wchar_t path[MAX_PATH]{};
        if(directory[0]&&swprintf_s(path,L"%s\\hand-target-plans.bin",directory)>0){HANDLE out=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
            if(out==INVALID_HANDLE_VALUE)file_error=GetLastError();else{DWORD written{};bool okay=WriteFile(out,&header,sizeof header,&written,nullptr)&&written==sizeof header;
                if(!okay)file_error=GetLastError()?GetLastError():ERROR_WRITE_FAULT;const auto bytes=DWORD(record_count*sizeof(Record));
                if(okay&&bytes){okay=WriteFile(out,records.data(),bytes,&written,nullptr)&&written==bytes;if(!okay)file_error=GetLastError()?GetLastError():ERROR_WRITE_FAULT;}
                if(!CloseHandle(out)){file_error=GetLastError();okay=false;}file_written=okay;}
        }else file_error=ERROR_INVALID_NAME;
    }summary(file);if(file)fprintf(file,"read_only=1 writer=0 atomic_verified=0 single_translation_target_unproven=1\n");
}
#else
bool configure(wchar_t const* ini,FILE*){wchar_t value[16]{};GetPrivateProfileStringW(L"HandGoalObserver",L"enabled",L"0",value,16,ini);return !wcscmp(value,L"0");}
void observe(cover_control_observer_live::Row const&,gears_native_rig_tracking::CoreSnapshot const&) noexcept {}void request_drain(){}unsigned pending_count(){return 0;}
void summary(FILE* file){if(file)fprintf(file,"hand_goal_enabled=0 hand_goal_compiled=0 ");}void print(FILE* file,bool){summary(file);}
#endif
} // namespace hand_goal_observer_live
