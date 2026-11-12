// Complete input goal for the later pose solver. Scene callers hold
// stereo_mutex; the independent store never holds a lock across native code.
namespace cover_full_goal_live {
std::mutex mutex;
gears_vr::CoverBodyTarget latest{};bool available{};
gears_cover_committed::State committed{};bool committed_available{};
uint64_t attempts{},accepted{},rejected{};
const char* reason="inactive";
void clear(char const* why){std::lock_guard<std::mutex> lock(mutex);available=committed_available=false;reason=why;}
gears_weapon_hand::ObjectIdentity committed_identity(gears_anchor::Identity const& id){
    return {id.address,id.outer,id.cls,id.index,id.name_index,id.number,1};
}
void prepare(gears_anchor::Receipt const* scene_receipt,gears_vr::StereoResult const& selected_stereo){
    if(!cover_input_live::enabled||!cover_camera_live::enabled){clear("disabled");return;}
    ++attempts;
    try{
        auto const& diagnostics=cover_input_live::diagnostics;auto const& result=diagnostics.latest;
        if(!scene_receipt||scene_receipt->thread!=GetCurrentThreadId()||diagnostics.frame!=scene_receipt->invocation||
           diagnostics.sequence!=selected_stereo.sample.sequence||diagnostics.reference!=selected_stereo.reference_generation||!result.view_valid){
            ++rejected;clear("input_or_scene");return;
        }
        auto const& tracking=result.tracking;
        if(tracking.status!=selected_stereo.status||tracking.reference_generation!=selected_stereo.reference_generation||
           tracking.recentered!=selected_stereo.recentered||tracking.heading_deferred!=selected_stereo.heading_deferred||
           !gears_vr::stereo_same_pose(tracking.reference,selected_stereo.reference)||
           !gears_vr::stereo_same(tracking.sample,selected_stereo.sample)){
            ++rejected;clear("input_tracking_binding");return;
        }
        cover_neutral_eye_live::EyeSample eye{};
        if(!cover_neutral_eye_live::sample(scene_receipt,eye)) {++rejected;clear("current_eye_binding");return;}
        gears_vr::CoverBodyTargetKey key{};key.epochs=cover_input_live::epochs;
        key.world=eye.current_world;key.world_generation=eye.world_generation;key.pawn=eye.pawn.address;
        key.mesh=eye.mesh.address;key.link=eye.link.address;key.slot=eye.slot;key.thread=scene_receipt->thread;key.scene=scene_receipt->invocation;
        key.xr_session=selected_stereo.sample.session;key.xr_space=selected_stereo.sample.space_epoch;key.reference=selected_stereo.reference_generation;key.sequence=selected_stereo.sample.sequence;
        const auto anchor=cover_input_live::wall_anchor(eye);
        const auto& frozen=result.wall_anchor;
        if(frozen.neutral_eye_world.x!=anchor.neutral_eye_world.x||frozen.neutral_eye_world.y!=anchor.neutral_eye_world.y||
           frozen.neutral_eye_world.z!=anchor.neutral_eye_world.z||frozen.forward_world.x!=anchor.forward_world.x||
           frozen.forward_world.y!=anchor.forward_world.y||frozen.forward_world.z!=anchor.forward_world.z||
           frozen.right_world.x!=anchor.right_world.x||frozen.right_world.y!=anchor.right_world.y||
           frozen.right_world.z!=anchor.right_world.z){++rejected;clear("input_wall_binding");return;}
        gears_vr::CoverBodyTarget goal{};
        if(!gears_vr::prepare_cover_body_target(key,key,frozen,result.view_anchor,result.view,result.interaction.offset,tracking,
            gears_vr::monotonic_ns(),stereo_units_per_metre,goal,simulator_operator)){
            ++rejected;clear("goal_inputs");return;
        }
        gears_cover_committed::State drag{};drag.key=key;
        drag.pawn=committed_identity(eye.pawn);drag.mesh=committed_identity(eye.mesh);drag.link=committed_identity(eye.link);
        drag.anchor=frozen;drag.view_anchor=result.view_anchor;drag.offset=goal.virtual_offset;
        drag.range=cover_input_live::last_grab_range;drag.reference=tracking.reference;
        drag.tracking_source=tracking.sample.source;drag.units_per_metre=goal.units_per_metre;
        drag.type=eye.native_ct;drag.observed_ns=goal.observed_ns;
        const bool drag_valid=result.interaction.offset_certified&&gears_cover_committed::valid(drag,goal.observed_ns);
        {std::lock_guard<std::mutex> lock(mutex);latest=goal;available=true;committed=drag;
            committed_available=drag_valid;++accepted;reason="ready";}
    }catch(...){++rejected;clear("exception");}
}
bool read(gears_vr::CoverBodyTargetKey const& key,std::int64_t stamp,gears_vr::CoverBodyTarget& out){
    std::lock_guard<std::mutex> lock(mutex);
    if(!available||!gears_vr::cover_body_target_current(latest,key,stamp))return false;
    out=latest;return true;
}
// Copies persistent virtual interaction state only. This does NOT refresh or
// return any tracking receipt. C0 must admit its own current Core/V3 source.
bool read_committed(gears_cover_committed::State& out){
    std::lock_guard<std::mutex> lock(mutex);
    if(!cover_input_live::enabled||!cover_camera_live::enabled){committed_available=false;return false;}
    if(!committed_available)return false;
    out=committed;return true;
}
bool set_body_owner(bool active){
    const bool selected=active&&cover_input_live::enabled&&cover_camera_live::enabled;
    cover_input_live::full_body_cover_enabled=selected;return selected;
}
// Entry point for the existing owner with its current C0 source and independently
// fitted placement. The store lock ends before conversion or any Native call.
// A prepared value is not a body commit, collision result or view publication.
template<class Source> gears_cover_c0_target::Code prepare_c0(
    gears_cover_neutral_goal::Result const& frozen,gears_body_goal_view::CoverPlacement const& placement,
    Source const& source,gears_vr::CoverBodyTargetKey const& key,
    gears_full_body_c0::Model const& model,gears_full_body_c0::Frame const& frame,
    gears_body_head_authored::Marker const& marker,gears_body_goal_view::Policy const& policy,
    gears_cover_c0_target::Prepared& out){
    gears_vr::CoverBodyTarget target{};
    if(!read(key,policy.now_ns,target))return gears_cover_c0_target::Code::unavailable;
    return gears_cover_c0_target::prepare(frozen,placement,source,key,target,model,frame,marker,policy,out);
}
void summary(FILE* file){
    std::lock_guard<std::mutex> lock(mutex);
    fprintf(file,"cover_full_goal_stored=%u cover_full_goal_attempts=%llu cover_full_goal_accepted=%llu cover_full_goal_rejected=%llu cover_full_goal_reason=%s cover_full_goal_frame=%llu cover_full_goal_sequence=%llu cover_full_goal_reference=%llu cover_full_goal_world=%llu cover_full_goal_right=%.9g cover_full_goal_up=%.9g cover_full_goal_forward=%.9g cover_full_goal_xyz=%.9g,%.9g,%.9g cover_full_goal_pose_applied=0 cover_full_goal_collision_verified=0 ",
        unsigned(available),attempts,accepted,rejected,reason,latest.key.scene,latest.key.sequence,latest.key.reference,latest.key.world_generation,
        latest.desired_right_metres,latest.desired_up_metres,latest.desired_forward_metres,latest.desired_eye_world.x,latest.desired_eye_world.y,latest.desired_eye_world.z);
}
} // namespace cover_full_goal_live
