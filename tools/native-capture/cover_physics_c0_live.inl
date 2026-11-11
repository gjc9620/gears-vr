#include "cover_physics_c0_live.h"
#include "cover_physx_query.h"
#include "body_source_age_observation.h"
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
thread_local gears_cover_query_guard::Witness query_owner_guard;
thread_local n::ready_wrist::cause::Witness query_source_cause;
bool sphere_probe_requested{};
bool path_probe_requested{};
struct SphereObservation {
    bool attempted{},finished{};q::Code code{q::Code::unavailable};bool raw_hit{};
    uint64_t world{},world_generation{},pawn{},link{},scene{},compose{},sequence{};int slot{-1};unsigned thread{};
    int64_t begin_ns{},end_ns{};q::Sphere shape{};
    QueryAdmission admission{};bool session_entered{};
    cover_physics_query_live::Observation progress{};
    uint64_t original_started_before{},original_returned_before{};
    gears_cover_query_guard::Witness owner_guard{};
    n::ready_wrist::cause::Witness source_cause{};
};
std::mutex sphere_probe_mutex;SphereObservation sphere_probe;
struct PathObservation {
    SphereObservation common{};q::Capsule shape{};q::Vector delta{};
    gears_cover_query_timing::Report timing{};
    gears_body_source_age_observation::Report source_ages{};
    gears_cover_owner_link_observation::Report owner_links{};
    unsigned segments{},hits{},reports{},endpoint_shapes{},endpoint_reports{},self_hits{},endpoint_self{};
};
PathObservation path_probe;
// Large raw packet belongs to this one-shot serialized owner, not its stack.
gears_cover_capsule_path::RawPath path_probe_packet;
bool configure(wchar_t const* ini,FILE* log){
    wchar_t value[8]{};GetPrivateProfileStringW(L"FullBodyC0Native",L"cover_queries",L"0",value,8,ini);
    if(std::wcscmp(value,L"0")&&std::wcscmp(value,L"1"))return false;
    bool requested=!std::wcscmp(value,L"1");
    GetPrivateProfileStringW(L"FullBodyC0Native",L"cover_sphere_probe",L"0",value,8,ini);
    if(std::wcscmp(value,L"0")&&std::wcscmp(value,L"1"))return false;
    sphere_probe_requested=!std::wcscmp(value,L"1");
    GetPrivateProfileStringW(L"FullBodyC0Native",L"cover_path_probe",L"0",value,8,ini);
    if(std::wcscmp(value,L"0")&&std::wcscmp(value,L"1"))return false;
    path_probe_requested=!std::wcscmp(value,L"1");
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
    const auto path_length=GetEnvironmentVariableW(L"GEARSVR_COVER_PATH_PROBE",value,8);
    if(path_length>=8||(path_length&&std::wcscmp(value,L"0")&&std::wcscmp(value,L"1")))return false;
    if(path_length&&!std::wcscmp(value,L"1")){
        wchar_t controlled[8]{};
        if(!simulator_operator||!continuous_session||
           GetEnvironmentVariableW(L"GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY",controlled,8)!=1||std::wcscmp(controlled,L"1"))return false;
        requested=true;path_probe_requested=true;
    }
    if((sphere_probe_requested&&path_probe_requested)||((sphere_probe_requested||path_probe_requested)&&!requested))return false;
    if(path_probe_requested){
        wchar_t controlled[8]{};
        if(!simulator_operator||!continuous_session||
           GetEnvironmentVariableW(L"GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY",controlled,8)!=1||std::wcscmp(controlled,L"1"))return false;
    }
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
    auto const& l=copy.progress.layout;
    fprintf(f,"cover_sphere_probe_layout_step=%u cover_sphere_probe_layout_pass=%u cover_sphere_probe_layout_body=%u cover_sphere_probe_layout_mesh_sources=%u cover_sphere_probe_layout_mesh_identities=%u cover_sphere_probe_layout_instance_sources=%u cover_sphere_probe_layout_instance_identities=%u cover_sphere_probe_layout_offsets=%u,%u,%u,%u,%u ",
        unsigned(l.step),l.pass,l.body_index,l.mesh_sources,l.mesh_identities,l.instance_sources,l.instance_identities,
        l.mesh_asset_offset,l.mesh_instance_offset,l.owner_offset,l.bodies_offset,l.root_offset);
    fprintf(f,"cover_sphere_probe_layout_member_identities=%u cover_sphere_probe_layout_member_capacity=%u ",l.member_identities,unsigned(l.member_capacity));
    PathObservation path_copy{};{std::lock_guard<std::mutex> lock(sphere_probe_mutex);path_copy=path_probe;}
    auto const& p=path_copy.common;
    auto const& ages=path_copy.source_ages;
    auto const& links=path_copy.owner_links;auto const& failed_link=links.first;
    fprintf(f,"cover_path_probe_link_key=%llu,%llu,%llu,%llu,%u cover_path_probe_link_failure=%u,%u,%u,%u,%u,%llu,%llu,%llu,%llu,%llu,%llu,%u ",
        links.key.world,links.key.pawn,links.key.scene,links.key.compose,links.key.thread,
        unsigned(failed_link.observed),unsigned(failed_link.reader),unsigned(failed_link.phase),unsigned(failed_link.branch),failed_link.thread,
        uint64_t(failed_link.actor),uint64_t(failed_link.table),uint64_t(failed_link.expected_scene),uint64_t(failed_link.observed_scene),
        uint64_t(failed_link.internal),uint64_t(failed_link.reverse),unsigned(failed_link.reverse_read));
    fprintf(f,"cover_path_probe_age_key=%llu,%llu,%llu,%llu,%u cover_path_probe_age_calls=%u,%u,%u ",
        ages.key.world,ages.key.pawn,ages.key.scene,ages.key.compose,ages.key.thread,ages.calls[0],ages.calls[1],unsigned(ages.overflow));
    for(unsigned i=0;i<ages.rows.size();++i){auto const& a=ages.rows[i];
        fprintf(f,"cover_path_probe_age_%u=%u,%llu,%llu,%llu,%llu,%llu,%llu,%u,%u,%u,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld ",
            i,unsigned(a.observed),a.tick,a.world,a.pawn,a.scene,a.stereo_sequence,a.controller_sequence,a.thread,a.verdict,unsigned(a.latched),
            a.now,a.metadata_begin,a.metadata_end,a.tracking_begin,a.tracking_end,a.stereo_received,a.controller_received,a.sync,a.capture_begin);}
    fprintf(f,"cover_path_probe_timing_invalid=%llu cover_path_probe_timing_overflow=%llu ",path_copy.timing.invalid,path_copy.timing.overflow);
    for(unsigned i=0;i<unsigned(gears_cover_query_timing::Stage::count);++i){auto const& bin=path_copy.timing.bins[i];
        fprintf(f,"cover_path_probe_timing_%u=%llu,%llu,%llu,%llu ",i,bin.calls,bin.completed,bin.total_ns,bin.max_ns);}
    fprintf(f,"cover_path_probe_requested=%u cover_path_probe_attempted=%u cover_path_probe_finished=%u cover_path_probe_code=%u cover_path_probe_world=%llu cover_path_probe_world_generation=%llu cover_path_probe_pawn=%llu cover_path_probe_link=%llu cover_path_probe_slot=%d cover_path_probe_scene=%llu cover_path_probe_compose=%llu cover_path_probe_sequence=%llu cover_path_probe_thread=%u cover_path_probe_begin_ns=%lld cover_path_probe_end_ns=%lld ",
        unsigned(path_probe_requested),unsigned(p.attempted),unsigned(p.finished),unsigned(p.code),p.world,p.world_generation,p.pawn,p.link,p.slot,p.scene,p.compose,p.sequence,p.thread,p.begin_ns,p.end_ns);
    fprintf(f,"cover_path_probe_capsule=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g cover_path_probe_delta=%.9g,%.9g,%.9g cover_path_probe_segments=%u cover_path_probe_hits=%u cover_path_probe_reports=%u cover_path_probe_endpoint_shapes=%u cover_path_probe_endpoint_reports=%u cover_path_probe_self_hits=%u cover_path_probe_endpoint_self=%u cover_path_probe_max_hits=32 cover_path_probe_max_reports=8 cover_path_probe_diagnostic_geometry=1 cover_path_probe_body_geometry=0 cover_path_probe_movement_permission=0 ",
        double(path_copy.shape.first.x),double(path_copy.shape.first.y),double(path_copy.shape.first.z),double(path_copy.shape.second.x),double(path_copy.shape.second.y),double(path_copy.shape.second.z),double(path_copy.shape.radius),
        double(path_copy.delta.x),double(path_copy.delta.y),double(path_copy.delta.z),path_copy.segments,path_copy.hits,path_copy.reports,path_copy.endpoint_shapes,path_copy.endpoint_reports,path_copy.self_hits,path_copy.endpoint_self);
    fprintf(f,"cover_path_probe_outer_stage=%u cover_path_probe_session_entered=%u cover_path_probe_session_stage=%u cover_path_probe_session_detail=%u cover_path_probe_engine_observed=%u cover_path_probe_engine_stage=%u cover_path_probe_engine_source_code=%u cover_path_probe_engine_exception=%u cover_path_probe_original_started_before=%llu cover_path_probe_original_started_after=%llu cover_path_probe_original_returned_before=%llu cover_path_probe_original_returned_after=%llu cover_path_probe_layout_step=%u cover_path_probe_layout_member_identities=%u cover_path_probe_layout_member_capacity=%u ",
        unsigned(p.admission),unsigned(p.session_entered),unsigned(p.progress.stage),p.progress.detail,unsigned(p.progress.engine.supported),unsigned(p.progress.engine.stage),p.progress.engine.source_code,unsigned(p.progress.engine.exception),
        p.original_started_before,p.progress.engine.original_started,p.original_returned_before,p.progress.engine.original_returned,unsigned(p.progress.layout.step),p.progress.layout.member_identities,unsigned(p.progress.layout.member_capacity));
    fprintf(f,"cover_path_probe_owner_guard=%u,%u,%u,%u,%u cover_path_probe_session_guard=%u,%u,%u,%u,%u ",
        p.owner_guard.calls,p.owner_guard.failed_call,p.owner_guard.stage,p.owner_guard.detail,p.owner_guard.extra,
        p.progress.guard.calls,p.progress.guard.failed_call,p.progress.guard.stage,p.progress.guard.detail,p.progress.guard.extra);
    auto const& cause=p.source_cause;
    fprintf(f,"cover_path_probe_source_age_cause=%u,%llu,%llu,%u,%u,%u,%u ",unsigned(cause.observed),
        uint64_t(cause.key.entry),cause.key.compose,cause.key.thread,cause.key.stage,cause.verdict,cause.causes);
    // The finished row is published under the mutex after the packet write.
    // The one-shot packet is immutable thereafter; no game read occurs here.
    if(p.finished&&p.code==q::Code::queried){
        for(unsigned i=0;i<path_copy.hits;++i){auto const& owner=path_probe_packet.collected.owners[i];
            fprintf(f,"cover_path_probe_sweep_%u=",i);
            for(auto byte:path_probe_packet.collected.raw.hits[i].bytes)fprintf(f,"%02x",unsigned(byte));
            fprintf(f,",%u,%llu,%llu,%llu,%llu,%llu,%u ",unsigned(owner.kind),uint64_t(owner.shape),uint64_t(owner.actor),uint64_t(owner.scene),uint64_t(owner.internal_shape),uint64_t(owner.internal_actor),unsigned(path_probe_packet.collected.self_sources[i]));
        }
        for(unsigned i=0;i<path_copy.endpoint_shapes;++i){auto const& owner=path_probe_packet.endpoint.owners[i];
            fprintf(f,"cover_path_probe_endpoint_%u=%llu,%u,%llu,%llu,%llu,%llu,%llu,%u ",i,
                uint64_t(path_probe_packet.endpoint.raw.shapes[i]),unsigned(owner.kind),uint64_t(owner.shape),uint64_t(owner.actor),uint64_t(owner.scene),uint64_t(owner.internal_shape),uint64_t(owner.internal_actor),unsigned(path_probe_packet.endpoint.self_sources[i]));
        }
    }
}
template<class Shape,class Output,class Invoke> q::Code query(Entry const& entry,Core const& source,Shape const& shape,Output& output,Invoke invoke)noexcept{
    query_admission=QueryAdmission::admission;query_session_entered=false;
    gears_cover_query_guard::Witness owner_witness{};
    n::ready_wrist::cause::Witness cause_witness{};
    struct PublishGuard {gears_cover_query_guard::Witness const& value;n::ready_wrist::cause::Witness const& cause;
        ~PublishGuard(){query_owner_guard=value;query_source_cause=cause;}} publish_guard{owner_witness,cause_witness};
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
        using Gate=gears_cover_query_guard::OwnerStage;
        owner_witness.enter();
        auto reject=[&](Gate stage,unsigned detail=0,unsigned extra=0){return owner_witness.reject(unsigned(stage),detail,extra);};
        gears_cover_pose_phase::Request request{};request.owner=s.context.owner;
        auto reader=[](void* out,void const* address,size_t size){return n::native::read_memory(out,reinterpret_cast<uintptr_t>(address),size);};
        gears_cover_pose_phase::Metadata current{};
        cover_context_live::CurrentKey current_cover{};
        if(!session.enabled())return reject(Gate::session);
        if(!s.captured)return reject(Gate::captured);
        if(s.prepared)return reject(Gate::prepared);
        if(s.kernel.pending())return reject(Gate::pending);
        if(!n::same_entry(entry))return reject(Gate::entry);
        if(std::memcmp(&expected_source,&s.context.source,sizeof source))return reject(Gate::context_source);
        if(std::memcmp(&expected_source,&source,sizeof source))return reject(Gate::input_source);
        {
            n::ready_wrist::cause::Scope observe(path_probe_requested||sphere_probe_requested,
                {reinterpret_cast<uintptr_t>(&entry),entry.invocation,unsigned(entry.thread),unsigned(n::tx::Stage::prepare)},
                unsigned(n::tx::Verdict::source_age_expired),cause_witness);
            if(!s.context.current(n::tx::Stage::prepare))return reject(Gate::context,unsigned(s.context.rejection()),s.context.rejection_detail());
        }
        if(!cover_context_live::sample_current(s.context.actor,current_cover))return reject(Gate::cover_read);
        if(!cover_context_live::same(cover,current_cover))return reject(Gate::cover_match);
        if(!gears_cover_pose_phase::metadata(reader,request,current))return reject(Gate::metadata_read);
        if(!gears_cover_pose_phase::metadata_equal(metadata,current))return reject(Gate::metadata_match);
        if(!gears_cover_pose_phase::metadata_equal(metadata,s.backend.metadata()))return reject(Gate::backend_metadata_match);
        return true;};
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
    GetPrivateProfileStringW(L"FullBodyC0Native",L"cover_path_probe",L"0",value,8,ini);if(std::wcscmp(value,L"0"))return false;
    for(auto name:{L"GEARSVR_COVER_SPHERE_PROBE",L"GEARSVR_COVER_PATH_PROBE"}){
        const auto length=GetEnvironmentVariableW(name,value,8);if(length&&!(length==1&&!std::wcscmp(value,L"0")))return false;}
    return true;}
void request_drain()noexcept{}
bool poll_stop()noexcept{return true;}
unsigned pending_count()noexcept{return 0;}
void summary(FILE*){}
template<class Shape> q::Code do_overlap(Entry const&,Core const&,Shape const&,q::Overlap&)noexcept{return q::Code::unavailable;}
template<class Shape> q::Code do_sweep(Entry const&,Core const&,Shape const&,q::Vector const&,q::Sweep&)noexcept{return q::Code::unavailable;}
template<class Shape,class Output> q::Code do_sweep_all(Entry const&,Core const&,Shape const&,q::Vector const&,q::StreamLimits const&,Output&)noexcept{return q::Code::unavailable;}
#endif
q::Code observe_asset_shapes(Entry const& entry,Core const& core,q::AssetShapeWorkspace& work,
    gears_cover_physx_asset_shapes::Snapshot& output)noexcept{
#if defined(GEARS_COVER_PHYSX_QUERIES) && GEARS_COVER_PHYSX_QUERIES
    if(q::source::aliases(&work,sizeof work,&n::state,sizeof n::state)||q::source::aliases(&work,sizeof work,&entry,sizeof entry)||
       q::source::aliases(&work,sizeof work,&core,sizeof core)||q::source::aliases(&work,sizeof work,&output,sizeof output))return q::Code::request;
    struct ReadOnlyRequest {unsigned version{1};};
    return query(entry,core,ReadOnlyRequest{},output,[&](auto const& actor,auto const& metadata,auto const&,auto read,auto guard,auto& result){
        return session.observe_asset_shapes(actor,metadata,read,guard,work,result);});
#else
    (void)entry;(void)core;(void)work;(void)output;return q::Code::unavailable;
#endif
}
q::Code sweep_capsule_path(Entry const& e,Core const& c,q::Capsule const& shape,q::Vector const& delta,
    gears_cover_capsule_path::Policy const& policy,q::StreamLimits const& limits,gears_cover_capsule_path::RawPath& out)noexcept {
#if defined(GEARS_COVER_PHYSX_QUERIES) && GEARS_COVER_PHYSX_QUERIES
    return query(e,c,shape,out,[&](auto const& actor,auto const& metadata,auto const& geometry,auto read,auto guard,auto& output){
        return session.sweep_capsule_path(actor,metadata,geometry,delta,policy,limits,read,guard,output);});
#else
    (void)e;(void)c;(void)shape;(void)delta;(void)policy;(void)limits;(void)out;return q::Code::unavailable;
#endif
}
template<class Output> q::Code overlap_all_impl(Entry const& e,Core const& c,q::Capsule const& shape,q::StreamLimits const& limits,Output& out)noexcept{
#if defined(GEARS_COVER_PHYSX_QUERIES) && GEARS_COVER_PHYSX_QUERIES
    return query(e,c,shape,out,[&](auto const& actor,auto const& metadata,auto const& geometry,auto read,auto guard,auto& output){
        return session.overlap_all(actor,metadata,geometry,limits,read,guard,output);});
#else
    (void)e;(void)c;(void)shape;(void)limits;(void)out;return q::Code::unavailable;
#endif
}
q::Code overlap_all(Entry const& e,Core const& c,q::Capsule const& shape,q::StreamLimits const& limits,q::AllOverlap& out)noexcept{return overlap_all_impl(e,c,shape,limits,out);}
q::Code overlap_all(Entry const& e,Core const& c,q::Capsule const& shape,q::StreamLimits const& limits,q::AllOverlapOwners& out)noexcept{return overlap_all_impl(e,c,shape,limits,out);}
q::Code overlap(Entry const& e,Core const& c,q::Sphere const& s,q::Overlap& out)noexcept{return do_overlap(e,c,s,out);}
q::Code overlap(Entry const& e,Core const& c,q::Box const& s,q::Overlap& out)noexcept{return do_overlap(e,c,s,out);}
q::Code overlap(Entry const& e,Core const& c,q::Capsule const& s,q::Overlap& out)noexcept{return do_overlap(e,c,s,out);}
void observe_path_once(Entry const& e,Core const& source,q::Vector const& eye)noexcept{
#if defined(GEARS_COVER_PHYSX_QUERIES) && GEARS_COVER_PHYSX_QUERIES
    if(!path_probe_requested||!session.enabled()||GetCurrentThreadId()!=n::owner_thread.load())return;
    try{
        PathObservation row{};auto& common=row.common;
        common.attempted=true;common.world=source.world.address;common.world_generation=source.world_generation;common.pawn=source.pawn.address;
        common.link=source.cover_link.address;common.slot=source.cover_slot;common.scene=source.scene_invocation;
        common.compose=e.invocation;common.sequence=source.stereo.sequence;common.thread=GetCurrentThreadId();common.begin_ns=rig_tracking_live::clock_ns();
        // This centimetre-scale shape is ONLY a bounded ABI/caller control.
        // Current raw cylinder fields are not authenticated body geometry.
        const float centimetre=float(source.units_per_metre*.01);
        row.shape={{eye.x,eye.y,eye.z-centimetre},{eye.x,eye.y,eye.z+centimetre},centimetre};
        row.delta={centimetre,0,0};
        {std::lock_guard<std::mutex> lock(sphere_probe_mutex);if(path_probe.common.attempted)return;path_probe=row;}
        const auto before=session.observation().engine;
        common.original_started_before=before.original_started;common.original_returned_before=before.original_returned;
        gears_cover_query_timing::Scope timing_scope(true,row.timing,+[]()noexcept{return rig_tracking_live::clock_ns();});
        gears_body_source_age_observation::Scope age_scope(true,{common.world,common.pawn,common.scene,common.compose,common.thread},row.source_ages);
        gears_cover_owner_link_observation::Scope link_scope(true,{common.world,common.pawn,common.scene,common.compose,common.thread},row.owner_links);
        query_admission=QueryAdmission::admission;query_session_entered=false;
        if(!q::detail::scalar(centimetre)||centimetre<=0||!q::detail::valid(row.shape)||!q::detail::sweep_motion(row.shape,row.delta))common.code=q::Code::request;
        else common.code=sweep_capsule_path(e,source,row.shape,row.delta,
            gears_cover_capsule_path::Policy{2,double(centimetre)*.001},q::StreamLimits{32,8},path_probe_packet);
        common.admission=query_admission;common.session_entered=query_session_entered;
        common.owner_guard=query_owner_guard;
        common.source_cause=query_source_cause;
        if(common.session_entered)common.progress=session.observation();else common.progress.engine=before;
        if(common.code==q::Code::queried){auto const& packet=path_probe_packet;
            row.segments=packet.plan.count;row.hits=packet.collected.raw.hit_count;row.reports=packet.collected.raw.reports;
            row.endpoint_shapes=packet.endpoint.raw.shape_count;row.endpoint_reports=packet.endpoint.raw.reports;
            for(unsigned i=0;i<row.hits;++i)row.self_hits+=packet.collected.self_sources[i]!=0;
            for(unsigned i=0;i<row.endpoint_shapes;++i)row.endpoint_self+=packet.endpoint.self_sources[i]!=0;
        }
        common.end_ns=rig_tracking_live::clock_ns();common.finished=true;
        {std::lock_guard<std::mutex> lock(sphere_probe_mutex);path_probe=row;}
    }catch(...){/* Preserve attempted, no result, no retry. */}
#else
    (void)e;(void)source;(void)eye;
#endif
}
void observe_sphere_once(Entry const& e,Core const& source,q::Vector const& eye)noexcept{
    // Preserve the existing single C0 measurement entry; modes are exclusive.
    observe_path_once(e,source,eye);
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
