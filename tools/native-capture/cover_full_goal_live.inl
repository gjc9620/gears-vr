// Complete input goal for the later pose solver. Scene callers hold
// stereo_mutex; the independent store never holds a lock across native code.
namespace cover_full_goal_live {
std::mutex mutex;
gears_vr::CoverBodyTarget latest{};bool available{};
uint64_t attempts{},accepted{},rejected{};
const char* reason="inactive";
void clear(char const* why){std::lock_guard<std::mutex> lock(mutex);available=false;reason=why;}
void prepare(gears_anchor::Receipt const* scene,gears_vr::StereoResult const& stereo){
    if(!cover_input_live::enabled||!cover_camera_live::enabled)return;
    ++attempts;
    try{
        auto const& diagnostics=cover_input_live::diagnostics;auto const& result=diagnostics.latest;
        if(!scene||scene->thread!=GetCurrentThreadId()||diagnostics.frame!=scene->invocation||
           diagnostics.sequence!=stereo.sample.sequence||diagnostics.reference!=stereo.reference_generation||!result.view_valid){
            ++rejected;clear("input_or_scene");return;
        }
        cover_neutral_eye_live::EyeSample eye{};
        if(!cover_neutral_eye_live::sample(scene,eye)) {++rejected;clear("current_eye_binding");return;}
        gears_vr::CoverBodyTargetKey key{};key.epochs=cover_input_live::epochs;
        key.world=eye.current_world;key.world_generation=eye.world_generation;key.pawn=eye.pawn.address;
        key.mesh=eye.mesh.address;key.link=eye.link.address;key.slot=eye.slot;key.thread=scene->thread;key.scene=scene->invocation;
        key.xr_session=stereo.sample.session;key.xr_space=stereo.sample.space_epoch;key.reference=stereo.reference_generation;key.sequence=stereo.sample.sequence;
        gears_vr::CoverViewAnchor anchor{eye.neutral_eye_world,cover_camera_live::facing.forward,cover_camera_live::facing.right};
        gears_vr::CoverBodyTarget goal{};
        if(!gears_vr::prepare_cover_body_target(key,key,anchor,result.view,result.interaction.offset,stereo,
            gears_vr::monotonic_ns(),stereo_units_per_metre,goal,simulator_operator)){
            ++rejected;clear("goal_inputs");return;
        }
        {std::lock_guard<std::mutex> lock(mutex);latest=goal;available=true;++accepted;reason="ready";}
    }catch(...){++rejected;clear("exception");}
}
bool read(gears_vr::CoverBodyTargetKey const& key,std::int64_t stamp,gears_vr::CoverBodyTarget& out){
    std::lock_guard<std::mutex> lock(mutex);
    if(!available||!gears_vr::cover_body_target_current(latest,key,stamp))return false;
    out=latest;return true;
}
void summary(FILE* file){
    std::lock_guard<std::mutex> lock(mutex);
    fprintf(file,"cover_full_goal_stored=%u cover_full_goal_attempts=%llu cover_full_goal_accepted=%llu cover_full_goal_rejected=%llu cover_full_goal_reason=%s cover_full_goal_frame=%llu cover_full_goal_sequence=%llu cover_full_goal_reference=%llu cover_full_goal_world=%llu cover_full_goal_right=%.9g cover_full_goal_up=%.9g cover_full_goal_forward=%.9g cover_full_goal_xyz=%.9g,%.9g,%.9g cover_full_goal_pose_applied=0 cover_full_goal_collision_verified=0 ",
        unsigned(available),attempts,accepted,rejected,reason,latest.key.scene,latest.key.sequence,latest.key.reference,latest.key.world_generation,
        latest.desired_right_metres,latest.desired_up_metres,latest.desired_forward_metres,latest.desired_eye_world.x,latest.desired_eye_world.y,latest.desired_eye_world.z);
}
} // namespace cover_full_goal_live
