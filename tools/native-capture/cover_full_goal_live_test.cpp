// Real shared consumer/owner/broker and FullGoal store, synthetic native actor
// and neutral provider. This does not certify C0 lifecycle, collision or writes.
#include "../vr-common/cover_body_target.h"
#define main PriorSharedConsumerMain
#include "cover_shared_input_live_test.cpp"
#undef main
#include "cover_full_goal_live.inl"

int main(){
    namespace goal=cover_full_goal_live;
    setup(true);attach();release();
    goal::prepare(&scene,stereo);C(goal::available);
    gears_vr::CoverBodyTarget value{};
    C(goal::read(goal::latest.key,gears_vr::monotonic_ns(),value));
    C(!value.pose_applied&&!value.collision_verified&&!value.same_pose_epoch_verified&&!value.head_bone_goal_valid);
    C(value.tracking.sample.received_ns==stereo.sample.received_ns);

    // A matching sequence cannot substitute a different same-sequence pose.
    stereo.sample.head.position.x+=.1;
    for(auto& eye:stereo.sample.eyes)eye.pose.position.x+=.1;
    goal::prepare(&scene,stereo);C(!goal::available);
    C(!std::strcmp(goal::reason,"input_tracking_binding"));
    C(!goal::read(value.key,gears_vr::monotonic_ns(),value));
    stereo=input::diagnostics.latest.tracking;

    // An explicitly supplied independent view must survive into the target.
    // This view-provider seam is synthetic; the store and target math are real.
    auto& selected=input::diagnostics.latest;
    selected.view_anchor.forward_world={0,1,0};selected.view_anchor.right_world={-1,0,0};
    C(gears_vr::prepare_cover_view(selected.wall_anchor,selected.view_anchor,
        selected.interaction.offset,100,selected.view));
    goal::prepare(&scene,stereo);C(goal::available);
    C(goal::read(goal::latest.key,gears_vr::monotonic_ns(),value));
    C(value.view_anchor.forward_world.y==1&&value.anchor.forward_world.x==1);
    C(value.committed_view.base_view==selected.view.base_view);

    selected.wall_anchor.neutral_eye_world.x+=1;
    goal::prepare(&scene,stereo);C(!goal::available&&!std::strcmp(goal::reason,"input_wall_binding"));
    finish();
    std::printf("cover_full_goal_live: %u checks, %u failures; synthetic provider, no C0/physics permission\n",checks,failures);
    return failures?1:0;
}
