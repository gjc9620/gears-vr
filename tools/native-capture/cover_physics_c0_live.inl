#include "cover_physics_c0_live.h"
#include "cover_physx_query.h"
#if defined(GEARS_COVER_PHYSX_QUERIES) && GEARS_COVER_PHYSX_QUERIES
#include "cover_physics_query_live.inl"
#endif
namespace cover_physics_c0_live {
namespace q=gears_cover_physx_query;
using Entry=gears_continuous_hand_entry::Entry;using Core=gears_native_rig_tracking::CoreSnapshot;
#if defined(GEARS_COVER_PHYSX_QUERIES) && GEARS_COVER_PHYSX_QUERIES
namespace n=full_body_c0_native_live;
cover_physics_query_live::Session<> session;
enum class QueryAdmission:unsigned {idle,admission,cover,guard,alias,session,returned};
thread_local QueryAdmission query_admission{};
thread_local bool query_session_entered{};
bool sphere_probe_requested{};
struct SphereObservation {
    bool attempted{},finished{};q::Code code{q::Code::unavailable};bool raw_hit{};
    uint64_t world{},world_generation{},pawn{},link{},scene{},compose{},sequence{};int slot{-1};unsigned thread{};
    int64_t begin_ns{},end_ns{};q::Sphere shape{};
    QueryAdmission admission{};bool session_entered{};
    cover_physics_query_live::Observation progress{};
    uint64_t original_started_before{},original_returned_before{};
};
std::mutex sphere_probe_mutex;SphereObservation sphere_probe;
bool configure(wchar_t const* ini,FILE* log){
    wchar_t value[8]{};GetPrivateProfileStringW(L"FullBodyC0Native",L"cover_queries",L"0",value,8,ini);
    if(std::wcscmp(value,L"0")&&std::wcscmp(value,L"1"))return false;
    bool requested=!std::wcscmp(value,L"1");
    GetPrivateProfileStringW(L"FullBodyC0Native",L"cover_sphere_probe",L"0",value,8,ini);
    if(std::wcscmp(value,L"0")&&std::wcscmp(value,L"1"))return false;
    sphere_probe_requested=!std::wcscmp(value,L"1");
    // Explicit private simulator option; normal profiles remain unchanged.
    // This follows the existing observer-only child environment, not input
    // from a page, game object or received query result.
    const auto length=GetEnvironmentVariableW(L"GEARSVR_COVER_SPHERE_PROBE",value,8);
    if(length>=8||(length&&std::wcscmp(value,L"0")&&std::wcscmp(value,L"1")))return false;
    if(length&&!std::wcscmp(value,L"1")){
        wchar_t controlled[8]{};
        if(!simulator_operator||!continuous_session||
           GetEnvironmentVariableW(L"GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY",controlled,8)!=1||std::wcscmp(controlled,L"1"))return false;
        requested=true;sphere_probe_requested=true;
    }
    if(sphere_probe_requested&&!requested)return false;
    const bool okay=session.start(requested,n::enabled&&continuous_session);
    if(log)fprintf(log,"cover_queries_compiled=1 requested=%u ready=%u raw_observations_only=1 movement_permission=0\n",unsigned(requested),unsigned(session.enabled()));
    return okay;
}
void request_drain()noexcept{session.request_drain();}
bool poll_stop()noexcept{return session.poll_stop();}
unsigned pending_count()noexcept{return session.pending_count();}
void summary(FILE* f){if(!f)return;
    SphereObservation copy{};{std::lock_guard<std::mutex> lock(sphere_probe_mutex);copy=sphere_probe;}
    fprintf(f,"cover_queries_ready=%u pending=%u uncertain=%u raw_observations_only=1 ",unsigned(session.enabled()),session.pending_count(),unsigned(session.uncertain()));
    fprintf(f,"cover_sphere_probe_requested=%u cover_sphere_probe_attempted=%u cover_sphere_probe_finished=%u cover_sphere_probe_code=%u cover_sphere_probe_raw_hit=%u cover_sphere_probe_world=%llu cover_sphere_probe_world_generation=%llu cover_sphere_probe_pawn=%llu cover_sphere_probe_link=%llu cover_sphere_probe_slot=%d cover_sphere_probe_scene=%llu cover_sphere_probe_compose=%llu cover_sphere_probe_sequence=%llu cover_sphere_probe_thread=%u cover_sphere_probe_begin_ns=%lld cover_sphere_probe_end_ns=%lld cover_sphere_probe_sphere=%.9g,%.9g,%.9g,%.9g cover_sphere_probe_movement_permission=0\n",
        unsigned(sphere_probe_requested),unsigned(copy.attempted),unsigned(copy.finished),unsigned(copy.code),unsigned(copy.raw_hit),
        copy.world,copy.world_generation,copy.pawn,copy.link,copy.slot,copy.scene,copy.compose,copy.sequence,copy.thread,copy.begin_ns,copy.end_ns,
        double(copy.shape.centre.x),double(copy.shape.centre.y),double(copy.shape.centre.z),double(copy.shape.radius));
    fprintf(f,"cover_sphere_probe_outer_stage=%u cover_sphere_probe_session_entered=%u cover_sphere_probe_session_stage=%u cover_sphere_probe_session_detail=%u cover_sphere_probe_engine_observed=%u cover_sphere_probe_engine_stage=%u cover_sphere_probe_engine_source_code=%u cover_sphere_probe_engine_exception=%u cover_sphere_probe_original_started_before=%llu cover_sphere_probe_original_started_after=%llu cover_sphere_probe_original_returned_before=%llu cover_sphere_probe_original_returned_after=%llu ",
        unsigned(copy.admission),unsigned(copy.session_entered),unsigned(copy.progress.stage),copy.progress.detail,
        unsigned(copy.progress.engine.supported),unsigned(copy.progress.engine.stage),copy.progress.engine.source_code,unsigned(copy.progress.engine.exception),
        copy.original_started_before,copy.progress.engine.original_started,copy.original_returned_before,copy.progress.engine.original_returned);
}
template<class Shape,class Output,class Invoke> q::Code query(Entry const& entry,Core const& source,Shape const& shape,Output& output,Invoke invoke)noexcept{
    query_admission=QueryAdmission::admission;query_session_entered=false;
    n::Call call;auto& s=n::state;
    if(!session.enabled()||GetCurrentThreadId()!=n::owner_thread.load()||!n::enabled||n::stopping.load()||entry.kind||!entry.selected||
       !s.captured||s.prepared||s.kernel.pending()||!n::same_entry(entry)||!source.cover_type||
       !(source.flags&gears_rig_tracking::cover_key_known)||std::memcmp(&source,&s.context.source,sizeof source))return q::Code::unavailable;
    const auto metadata=s.backend.metadata();
    const Core expected_source=source;
    cover_context_live::CurrentKey cover{};
    query_admission=QueryAdmission::cover;
    if(!cover_context_live::sample_current(s.context.actor,cover)||cover.type!=source.cover_type||cover.slot!=source.cover_slot||
       !gears_weapon_hand::same(rig_tracking_live::object(cover.link),source.cover_link))return q::Code::source;
    auto guard=[&](){
        gears_cover_pose_phase::Request request{};request.owner=s.context.owner;
        auto reader=[](void* out,void const* address,size_t size){return n::native::read_memory(out,reinterpret_cast<uintptr_t>(address),size);};
        gears_cover_pose_phase::Metadata current{};
        cover_context_live::CurrentKey current_cover{};
        return session.enabled()&&s.captured&&!s.prepared&&!s.kernel.pending()&&n::same_entry(entry)&&
            !std::memcmp(&expected_source,&s.context.source,sizeof source)&&!std::memcmp(&expected_source,&source,sizeof source)&&
            s.context.current(n::tx::Stage::prepare)&&cover_context_live::sample_current(s.context.actor,current_cover)&&
            cover_context_live::same(cover,current_cover)&&
            gears_cover_pose_phase::metadata(reader,request,current)&&gears_cover_pose_phase::metadata_equal(metadata,current)&&
            gears_cover_pose_phase::metadata_equal(metadata,s.backend.metadata());};
    query_admission=QueryAdmission::guard;
    if(!guard())return q::Code::source;
    // No binding/stereo/session lock is held across this original operation.
    auto read=[](void* out,void const* address,size_t size){return n::native::read_memory(out,reinterpret_cast<uintptr_t>(address),size);};
    query_admission=QueryAdmission::alias;
    if(q::source::aliases(&output,sizeof output,&s,sizeof s)||q::source::aliases(&output,sizeof output,&entry,sizeof entry)||
       q::source::aliases(&output,sizeof output,&source,sizeof source))return q::Code::request;
    query_admission=QueryAdmission::session;query_session_entered=true;
    const auto code=invoke(s.context.actor,metadata,shape,read,guard,output);
    query_admission=QueryAdmission::returned;
    if(code==q::Code::fault_unknown)request_session_stop(SessionStop::proof_failure);
    return code;
}
template<class Shape> q::Code do_overlap(Entry const& e,Core const& c,Shape const& shape,q::Overlap& out)noexcept{
    return query(e,c,shape,out,[](auto const& actor,auto const& metadata,auto const& geometry,auto read,auto guard,auto& output){return session.overlap(actor,metadata,geometry,read,guard,output);});
}
template<class Shape> q::Code do_sweep(Entry const& e,Core const& c,Shape const& shape,q::Vector const& delta,q::Sweep& out)noexcept{
    return query(e,c,shape,out,[&](auto const& actor,auto const& metadata,auto const& geometry,auto read,auto guard,auto& output){return session.sweep(actor,metadata,geometry,delta,read,guard,output);});
}
template<class Shape,class Output> q::Code do_sweep_all(Entry const& e,Core const& c,Shape const& shape,q::Vector const& delta,q::StreamLimits const& limits,Output& out)noexcept{
    return query(e,c,shape,out,[&](auto const& actor,auto const& metadata,auto const& geometry,auto read,auto guard,auto& output){return session.sweep_all(actor,metadata,geometry,delta,limits,read,guard,output);});
}
#else
bool configure(wchar_t const* ini,FILE*){wchar_t value[8]{};GetPrivateProfileStringW(L"FullBodyC0Native",L"cover_queries",L"0",value,8,ini);if(std::wcscmp(value,L"0"))return false;
    GetPrivateProfileStringW(L"FullBodyC0Native",L"cover_sphere_probe",L"0",value,8,ini);if(std::wcscmp(value,L"0"))return false;
    const auto length=GetEnvironmentVariableW(L"GEARSVR_COVER_SPHERE_PROBE",value,8);return !length||(length==1&&!std::wcscmp(value,L"0"));}
void request_drain()noexcept{}
bool poll_stop()noexcept{return true;}
unsigned pending_count()noexcept{return 0;}
void summary(FILE*){}
template<class Shape> q::Code do_overlap(Entry const&,Core const&,Shape const&,q::Overlap&)noexcept{return q::Code::unavailable;}
template<class Shape> q::Code do_sweep(Entry const&,Core const&,Shape const&,q::Vector const&,q::Sweep&)noexcept{return q::Code::unavailable;}
template<class Shape,class Output> q::Code do_sweep_all(Entry const&,Core const&,Shape const&,q::Vector const&,q::StreamLimits const&,Output&)noexcept{return q::Code::unavailable;}
#endif
q::Code sweep_capsule_path(Entry const& e,Core const& c,q::Capsule const& shape,q::Vector const& delta,
    gears_cover_capsule_path::Policy const& policy,q::StreamLimits const& limits,gears_cover_capsule_path::RawPath& out)noexcept {
#if defined(GEARS_COVER_PHYSX_QUERIES) && GEARS_COVER_PHYSX_QUERIES
    return query(e,c,shape,out,[&](auto const& actor,auto const& metadata,auto const& geometry,auto read,auto guard,auto& output){
        return session.sweep_capsule_path(actor,metadata,geometry,delta,policy,limits,read,guard,output);});
#else
    (void)e;(void)c;(void)shape;(void)delta;(void)policy;(void)limits;(void)out;return q::Code::unavailable;
#endif
}
q::Code overlap(Entry const& e,Core const& c,q::Sphere const& s,q::Overlap& out)noexcept{return do_overlap(e,c,s,out);}
q::Code overlap(Entry const& e,Core const& c,q::Box const& s,q::Overlap& out)noexcept{return do_overlap(e,c,s,out);}
q::Code overlap(Entry const& e,Core const& c,q::Capsule const& s,q::Overlap& out)noexcept{return do_overlap(e,c,s,out);}
void observe_sphere_once(Entry const& e,Core const& source,q::Vector const& eye)noexcept{
#if defined(GEARS_COVER_PHYSX_QUERIES) && GEARS_COVER_PHYSX_QUERIES
    if(!sphere_probe_requested||!session.enabled()||GetCurrentThreadId()!=n::owner_thread.load())return;
    try{
        SphereObservation row{};row.attempted=true;row.world=source.world.address;row.world_generation=source.world_generation;row.pawn=source.pawn.address;
        row.link=source.cover_link.address;row.slot=source.cover_slot;row.scene=source.scene_invocation;
        row.compose=e.invocation;row.sequence=source.stereo.sequence;row.thread=GetCurrentThreadId();
        // One centimetre is a diagnostic query radius, NOT a head collider.
        row.shape={eye,float(source.units_per_metre*.01)};
        row.begin_ns=rig_tracking_live::clock_ns();
        {std::lock_guard<std::mutex> lock(sphere_probe_mutex);if(sphere_probe.attempted)return;sphere_probe=row;}
        // No private lock across the original call. Failure is terminal for
        // this observation too; never silently retry a different source.
        const auto before=session.observation().engine;
        row.original_started_before=before.original_started;row.original_returned_before=before.original_returned;
        q::Overlap result{};row.code=overlap(e,source,row.shape,result);
        row.admission=query_admission;row.session_entered=query_session_entered;
        if(row.session_entered)row.progress=session.observation();
        else {row.progress.engine=before;}// Counts unchanged; do not copy an old Session stage.
        row.raw_hit=row.code==q::Code::queried&&result.raw_hit;
        row.end_ns=rig_tracking_live::clock_ns();row.finished=true;
        {std::lock_guard<std::mutex> lock(sphere_probe_mutex);sphere_probe=row;}
    }catch(...){/* attempted remains visible; never claim a result or retry */}
#else
    (void)e;(void)source;(void)eye;
#endif
}
q::Code sweep(Entry const& e,Core const& c,q::Box const& s,q::Vector const& d,q::Sweep& out)noexcept{return do_sweep(e,c,s,d,out);}
q::Code sweep(Entry const& e,Core const& c,q::Capsule const& s,q::Vector const& d,q::Sweep& out)noexcept{return do_sweep(e,c,s,d,out);}
q::Code sweep_all(Entry const& e,Core const& c,q::Box const& s,q::Vector const& d,q::StreamLimits const& l,q::AllSweep& out)noexcept{return do_sweep_all(e,c,s,d,l,out);}
q::Code sweep_all(Entry const& e,Core const& c,q::Capsule const& s,q::Vector const& d,q::StreamLimits const& l,q::AllSweep& out)noexcept{return do_sweep_all(e,c,s,d,l,out);}
q::Code sweep_all(Entry const& e,Core const& c,q::Box const& s,q::Vector const& d,q::StreamLimits const& l,q::AllSweepOwners& out)noexcept{return do_sweep_all(e,c,s,d,l,out);}
q::Code sweep_all(Entry const& e,Core const& c,q::Capsule const& s,q::Vector const& d,q::StreamLimits const& l,q::AllSweepOwners& out)noexcept{return do_sweep_all(e,c,s,d,l,out);}
}
