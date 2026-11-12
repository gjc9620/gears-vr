#include "cover_physics_c0_live.h"
#include "cover_physx_query.h"
#include "cover_probe_geometry.h"
#include "body_source_age_observation.h"
#if defined(GEARS_COVER_PHYSX_QUERIES) && GEARS_COVER_PHYSX_QUERIES
#include "cover_physics_query_live.inl"
#include "cover_physx_asset_bounds.h"
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
bool shape_snapshot_requested{};
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
struct ShapeObservation {SphereObservation common{};unsigned bodies{},actors{},shapes{};
    gears_cover_physx_asset_shapes::Code detail{gears_cover_physx_asset_shapes::Code::request};
    bool bounds_attempted{};gears_cover_physx_asset_bounds::Code bounds_code{gears_cover_physx_asset_bounds::Code::links};
    int64_t bounds_begin_ns{},bounds_end_ns{};};
ShapeObservation shape_snapshot;
q::AssetShapeWorkspace shape_snapshot_work;
gears_cover_physx_asset_shapes::Snapshot shape_snapshot_packet;
gears_cover_physx_asset_bounds::Snapshot shape_snapshot_bounds;
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
    GetPrivateProfileStringW(L"FullBodyC0Native",L"cover_shape_snapshot",L"0",value,8,ini);
    if(std::wcscmp(value,L"0")&&std::wcscmp(value,L"1"))return false;
    shape_snapshot_requested=!std::wcscmp(value,L"1");
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
    const auto shape_length=GetEnvironmentVariableW(L"GEARSVR_COVER_SHAPE_SNAPSHOT",value,8);
    if(shape_length>=8||(shape_length&&std::wcscmp(value,L"0")&&std::wcscmp(value,L"1")))return false;
    if(shape_length&&!std::wcscmp(value,L"1")){requested=true;shape_snapshot_requested=true;}
    const unsigned modes=unsigned(sphere_probe_requested)+unsigned(path_probe_requested)+unsigned(shape_snapshot_requested);
    if(modes>1||(modes&&!requested))return false;
    if(path_probe_requested||shape_snapshot_requested){
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
    fprintf(f,"cover_probe_geometry_units=physx cover_probe_game_to_physics=%.9g\n",double(gears_cover_physx_units::game_to_physics));
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
    std::lock_guard<std::mutex> shape_lock(sphere_probe_mutex);auto const& r=shape_snapshot;auto const& c=r.common;
    fprintf(f,"\ncover_shape_snapshot_requested=%u attempted=%u finished=%u code=%u detail=%u bodies=%u actors=%u shapes=%u units=physx movement_permission=0 original_started=%llu,%llu original_returned=%llu,%llu world=%llu pawn=%llu scene=%llu compose=%llu sequence=%llu thread=%u admission=%u session_entered=%u session_stage=%u session_detail=%u begin_ns=%lld end_ns=%lld\n",
        unsigned(shape_snapshot_requested),unsigned(c.attempted),unsigned(c.finished),unsigned(c.code),unsigned(r.detail),r.bodies,r.actors,r.shapes,
        c.original_started_before,c.progress.engine.original_started,c.original_returned_before,c.progress.engine.original_returned,
        c.world,c.pawn,c.scene,c.compose,c.sequence,c.thread,unsigned(c.admission),unsigned(c.session_entered),unsigned(c.progress.stage),c.progress.detail,c.begin_ns,c.end_ns);
    fprintf(f,"cover_shape_snapshot_owner_guard=%u,%u,%u,%u,%u session_guard=%u,%u,%u,%u,%u source_cause=%u,%llu,%llu,%u,%u,%u,%u\n",
        c.owner_guard.calls,c.owner_guard.failed_call,c.owner_guard.stage,c.owner_guard.detail,c.owner_guard.extra,
        c.progress.guard.calls,c.progress.guard.failed_call,c.progress.guard.stage,c.progress.guard.detail,c.progress.guard.extra,
        unsigned(c.source_cause.observed),uint64_t(c.source_cause.key.entry),c.source_cause.key.compose,c.source_cause.key.thread,c.source_cause.key.stage,c.source_cause.verdict,c.source_cause.causes);
    fprintf(f,"cover_shape_snapshot_full_rechecks=%u cover_shape_snapshot_borrowed_guards=%u\n",c.progress.physics_rechecks,c.progress.asset_read_guards);
    if(c.finished&&shape_snapshot_work.detail==gears_cover_physx_asset_shapes::Code::observed){auto const& candidate=shape_snapshot_work.candidate;
        fprintf(f,"cover_shape_snapshot_candidate_counts=%u,%u,%u cover_shape_snapshot_candidate_requires_close=1\n",candidate.assets.body_count,candidate.assets.actor_count,candidate.count);
    }
    if(c.finished&&c.code==q::Code::queried)for(unsigned i=0;i<r.bodies;++i){auto const& b=shape_snapshot_packet.assets.bodies[i];
        fprintf(f,"cover_shape_snapshot_body_mapping=%u,%u,%u,%llu,%llu,%llu,%llu,%llu\n",i,shape_snapshot_packet.body_actor[i],unsigned(b.presence),uint64_t(b.body),uint64_t(b.actor),uint64_t(b.table),uint64_t(b.scene),uint64_t(b.internal));
    }
    // Only immutable owned scratch after this one-shot finishes. Partial
    // scratch is diagnostic, never a published inventory or permission.
    if(c.finished){auto const& w=shape_snapshot_work.collection;auto const& sw=w.shape_work;auto const& a=sw.first;
        fprintf(f,"cover_shape_snapshot_nested=%u,%u,%u cover_shape_snapshot_failed=%u,%u cover_shape_snapshot_progress=%u,%u,%u,%u cover_shape_snapshot_partial_unpublished=1 cover_shape_snapshot_partial_actor=%llu,%llu,%llu,%llu,%llu,%u,%u cover_shape_snapshot_partial_arrays=%llu,%llu,%llu,%llu,%llu,%llu\n",
            unsigned(w.asset_code),unsigned(w.shape_code),unsigned(w.world_code),w.failed_actor,w.failed_member,
            unsigned(sw.first_progress.stage),sw.first_progress.member,unsigned(sw.last_progress.stage),sw.last_progress.member,
            uint64_t(a.actor),uint64_t(a.scene),uint64_t(a.body),uint64_t(a.root),uint64_t(a.root_table),a.type,a.count,
            uint64_t(a.internal_array[0]),uint64_t(a.internal_array[1]),uint64_t(a.internal_array[2]),uint64_t(a.public_array[0]),uint64_t(a.public_array[1]),uint64_t(a.public_array[2]));
    }
    if(c.finished){auto const& w=shape_snapshot_work.collection.world_work;
        for(unsigned pass=0;pass<2;++pass){auto const& d=pass?w.last_diagnostic:w.first_diagnostic;auto const& a=d.partial;
            fprintf(f,"cover_shape_snapshot_world_pass=%u cover_shape_snapshot_world_stage=%u cover_shape_snapshot_parameter_code=%u cover_shape_snapshot_pose_unpublished=1 cover_shape_snapshot_body=%u,%llu cover_shape_snapshot_stored_p=",pass,unsigned(d.stage),unsigned(w.parameter_code),unsigned(a.body_pose_present),uint64_t(a.body_table));
            for(unsigned k=0;k<3;++k)fprintf(f,"%s%.9g",k?",":"",double(a.parameters.stored_position[k]));
            fprintf(f," cover_shape_snapshot_stored_q=");for(unsigned k=0;k<4;++k)fprintf(f,"%s%.9g",k?",":"",double(a.parameters.stored_quaternion[k]));
            fprintf(f," cover_shape_snapshot_body_p=");for(unsigned k=0;k<3;++k)fprintf(f,"%s%.9g",k?",":"",double(a.body_position[k]));
            fprintf(f," cover_shape_snapshot_body_q=");for(unsigned k=0;k<4;++k)fprintf(f,"%s%.9g",k?",":"",double(a.body_quaternion[k]));fputc('\n',f);
        }
    }
    if(c.finished&&c.code==q::Code::queried)for(unsigned i=0;i<r.shapes;++i){auto const& row=shape_snapshot_packet.rows[i];auto const& g=row.geometry;auto const& x=g.parameters;auto const& owner=x.owner;
        fprintf(f,"cover_shape_snapshot_row=%u,%u,%u,%u,%llu,%llu,%llu,%llu flags=%u local_p=%.9g,%.9g,%.9g local_q=%.9g,%.9g,%.9g,%.9g sizes=%.9g,%.9g,%.9g,%.9g,%.9g world=",
            i,row.actor_index,row.member_ordinal,unsigned(owner.kind),uint64_t(owner.shape),uint64_t(owner.actor),uint64_t(owner.internal_shape),uint64_t(owner.internal_actor),x.flags,
            double(x.stored_position[0]),double(x.stored_position[1]),double(x.stored_position[2]),double(x.stored_quaternion[0]),double(x.stored_quaternion[1]),double(x.stored_quaternion[2]),double(x.stored_quaternion[3]),
            double(x.box_dimensions[0]),double(x.box_dimensions[1]),double(x.box_dimensions[2]),double(x.radius),double(x.half_segment));
        for(unsigned k=0;k<12;++k)fprintf(f,"%s%.9g",k?",":"",double(g.shape_world.values[k]));fprintf(f," observed_gram_error=%.17g,%.17g rigid_query_permission=0\n",g.actor_gram_error,g.shape_gram_error);
    }
    fprintf(f,"cover_shape_bounds_attempted=%u code=%u begin_ns=%lld end_ns=%lld derived_after_close=1 mathematical_only=1 query_permission=0 movement_permission=0\n",
        unsigned(r.bounds_attempted),unsigned(r.bounds_code),r.bounds_begin_ns,r.bounds_end_ns);
    if(c.finished&&c.code==q::Code::queried&&r.bounds_attempted&&r.bounds_code==gears_cover_physx_asset_bounds::Code::bounded){
        auto const& packet=shape_snapshot_bounds;
        fprintf(f,"cover_shape_bounds_count=%u has_shapes=%u\n",packet.count,unsigned(packet.has_shapes));
        for(unsigned i=0;i<packet.count;++i){auto const& row=packet.rows[i];
            fprintf(f,"cover_shape_bounds_row=%u,%u,%u,%llu lower=%.17g,%.17g,%.17g upper=%.17g,%.17g,%.17g\n",i,row.actor,row.member,uint64_t(row.shape),
                row.bounds.lower[0],row.bounds.lower[1],row.bounds.lower[2],row.bounds.upper[0],row.bounds.upper[1],row.bounds.upper[2]);
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
            n::ready_wrist::cause::Scope observe(path_probe_requested||sphere_probe_requested||shape_snapshot_requested,
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
    GetPrivateProfileStringW(L"FullBodyC0Native",L"cover_shape_snapshot",L"0",value,8,ini);if(std::wcscmp(value,L"0"))return false;
    for(auto name:{L"GEARSVR_COVER_SPHERE_PROBE",L"GEARSVR_COVER_PATH_PROBE",L"GEARSVR_COVER_SHAPE_SNAPSHOT"}){
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
        gears_cover_probe_geometry::Path geometry{};
        const auto geometry_code=gears_cover_probe_geometry::path(eye,source.units_per_metre,geometry);
        row.shape=geometry.capsule;row.delta=geometry.delta;
        {std::lock_guard<std::mutex> lock(sphere_probe_mutex);if(path_probe.common.attempted)return;path_probe=row;}
        const auto before=session.observation().engine;
        common.original_started_before=before.original_started;common.original_returned_before=before.original_returned;
        gears_cover_query_timing::Scope timing_scope(true,row.timing,+[]()noexcept{return rig_tracking_live::clock_ns();});
        gears_body_source_age_observation::Scope age_scope(true,{common.world,common.pawn,common.scene,common.compose,common.thread},row.source_ages);
        gears_cover_owner_link_observation::Scope link_scope(true,{common.world,common.pawn,common.scene,common.compose,common.thread},row.owner_links);
        query_admission=QueryAdmission::admission;query_session_entered=false;
        if(geometry_code!=gears_cover_probe_geometry::Code::prepared)common.code=q::Code::request;
        else common.code=sweep_capsule_path(e,source,row.shape,row.delta,
            gears_cover_capsule_path::Policy{2,geometry.epsilon},q::StreamLimits{32,8},path_probe_packet);
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
void observe_shapes_once(Entry const& e,Core const& source)noexcept{
#if defined(GEARS_COVER_PHYSX_QUERIES) && GEARS_COVER_PHYSX_QUERIES
    if(!shape_snapshot_requested||!session.enabled()||GetCurrentThreadId()!=n::owner_thread.load())return;
    try{ShapeObservation row{};auto& common=row.common;common.attempted=true;common.world=source.world.address;common.world_generation=source.world_generation;common.pawn=source.pawn.address;
        common.link=source.cover_link.address;common.slot=source.cover_slot;common.scene=source.scene_invocation;common.compose=e.invocation;common.sequence=source.stereo.sequence;common.thread=GetCurrentThreadId();common.begin_ns=rig_tracking_live::clock_ns();
        {std::lock_guard<std::mutex> lock(sphere_probe_mutex);if(shape_snapshot.common.attempted)return;shape_snapshot=row;}
        const auto before=session.observation().engine;common.original_started_before=before.original_started;common.original_returned_before=before.original_returned;
        shape_snapshot_work.detail=gears_cover_physx_asset_shapes::Code::request;
        common.code=observe_asset_shapes(e,source,shape_snapshot_work,shape_snapshot_packet);row.detail=shape_snapshot_work.detail;
        common.admission=query_admission;common.session_entered=query_session_entered;common.owner_guard=query_owner_guard;common.source_cause=query_source_cause;
        if(common.session_entered)common.progress=session.observation();else common.progress.engine=before;
        if(common.code==q::Code::queried){row.bodies=shape_snapshot_packet.assets.body_count;row.actors=shape_snapshot_packet.assets.actor_count;row.shapes=shape_snapshot_packet.count;
            // Derive from the fully closed owned packet. No borrowed live proof,
            // source-clock renewal, original call or permission follows this.
            row.bounds_attempted=true;row.bounds_begin_ns=rig_tracking_live::clock_ns();
            row.bounds_code=gears_cover_physx_asset_bounds::convert(shape_snapshot_packet,shape_snapshot_bounds);
            row.bounds_end_ns=rig_tracking_live::clock_ns();
        }
        common.end_ns=rig_tracking_live::clock_ns();common.finished=true;{std::lock_guard<std::mutex> lock(sphere_probe_mutex);shape_snapshot=row;}
    }catch(...){/* Preserve attempted, no result, no retry. */}
#else
    (void)e;(void)source;
#endif
}
void observe_sphere_once(Entry const& e,Core const& source,q::Vector const& eye)noexcept{
    // Preserve the existing single C0 measurement entry; modes are exclusive.
    observe_shapes_once(e,source);observe_path_once(e,source,eye);
#if defined(GEARS_COVER_PHYSX_QUERIES) && GEARS_COVER_PHYSX_QUERIES
    if(!sphere_probe_requested||!session.enabled()||GetCurrentThreadId()!=n::owner_thread.load())return;
    try{
        SphereObservation row{};row.attempted=true;row.world=source.world.address;row.world_generation=source.world_generation;row.pawn=source.pawn.address;
        row.link=source.cover_link.address;row.slot=source.cover_slot;row.scene=source.scene_invocation;
        row.compose=e.invocation;row.sequence=source.stereo.sequence;row.thread=GetCurrentThreadId();
        // One centimetre is a diagnostic query radius, NOT a head collider.
        const auto geometry_code=gears_cover_probe_geometry::sphere(eye,source.units_per_metre,row.shape);
        row.begin_ns=rig_tracking_live::clock_ns();
        {std::lock_guard<std::mutex> lock(sphere_probe_mutex);if(sphere_probe.attempted)return;sphere_probe=row;}
        // No private lock across the original call. Failure is terminal for
        // this observation too; never silently retry a different source.
        const auto before=session.observation().engine;
        row.original_started_before=before.original_started;row.original_returned_before=before.original_returned;
        query_admission=QueryAdmission::admission;query_session_entered=false;
        q::Overlap result{};row.code=geometry_code==gears_cover_probe_geometry::Code::prepared?
            overlap(e,source,row.shape,result):q::Code::request;
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
