// Real shared consumer/owner/broker and FullGoal store, synthetic native actor
// and neutral provider. This does not certify C0 lifecycle, collision or writes.
#include "../vr-common/cover_body_target.h"
#include "cover_c0_target_adapter.h"
#include "cover_c0_committed_adapter.h"
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
    gears_cover_committed::State drag{};
    C(goal::read_committed(drag));
    C(drag.key.sequence==value.key.sequence&&drag.offset.right==value.virtual_offset.right&&
        drag.offset.up==value.virtual_offset.up&&drag.observed_ns==value.observed_ns);
    C(gears_cover_committed::valid(drag,drag.observed_ns));
    // C0 wall correction must survive store re-certification without rotating
    // the continuous view basis. A stale legacy wall must be rejected.
    gears_cover_pose_role::c0_exclusive=true;
    goal::prepare(&scene,stereo);C(!goal::available&&!std::strcmp(goal::reason,"input_wall_binding"));
    const auto legacy_wall=input::diagnostics.latest.wall_anchor;
    input::diagnostics.latest.wall_anchor=input::wall_anchor(cover_neutral_eye_live::eye);
    goal::prepare(&scene,stereo);C(goal::read_committed(drag));
    C(drag.anchor.right_world.x==-drag.view_anchor.right_world.x&&
        drag.anchor.right_world.y==-drag.view_anchor.right_world.y);
    input::diagnostics.latest.wall_anchor=legacy_wall;gears_cover_pose_role::c0_exclusive=false;
    goal::prepare(&scene,stereo);C(goal::available);
    C(goal::set_body_owner(true)&&input::full_body_cover_enabled);
    cover_camera_live::enabled=false;C(!goal::set_body_owner(true)&&!input::full_body_cover_enabled);
    cover_camera_live::enabled=true;C(!goal::set_body_owner(false)&&!input::full_body_cover_enabled);
    // Persistent interaction state may outlive the old target's tracking TTL;
    // its C0 consumer still has to admit new tracking independently.
    C(!goal::read(value.key,value.observed_ns+100000001,value));
    C(goal::read_committed(drag));
    input::enabled=false;drag.observed_ns=123;
    C(!goal::read_committed(drag)&&drag.observed_ns==123);
    input::enabled=true;C(!goal::read_committed(drag));
    goal::prepare(&scene,stereo);C(goal::read_committed(drag));

    // Disabling either provider consumes the stored target. Re-enabling a
    // flag alone must not resurrect it; a fresh valid prepare is required.
    for(unsigned disabled=0;disabled<2;++disabled){
        const auto held_key=value.key;
        if(disabled==0)input::enabled=false;else cover_camera_live::enabled=false;
        goal::prepare(nullptr,stereo);
        C(!goal::available&&!std::strcmp(goal::reason,"disabled"));
        C(!goal::read_committed(drag));
        value.observed_ns=123;
        C(!goal::read(held_key,gears_vr::monotonic_ns(),value)&&value.observed_ns==123);
        input::enabled=true;cover_camera_live::enabled=true;
        C(!goal::read(held_key,gears_vr::monotonic_ns(),value));
        goal::prepare(&scene,stereo);C(goal::available);
        C(goal::read(goal::latest.key,gears_vr::monotonic_ns(),value));
    }

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
    // No stored target means no conversion and no output modification. This
    // instantiates the actual store bridge, without granting a C0 lifecycle.
    namespace c0=gears_cover_c0_target;
    static gears_full_body_c0::Model model{};
    static gears_full_body_c0::Frame frame{};
    gears_cover_neutral_goal::Result frozen{};
    gears_body_goal_view::CoverPlacement placement{};
    gears_native_rig_tracking::CoreSnapshot source{};
    gears_body_head_authored::Marker marker{};
    gears_body_goal_view::Policy policy{};
    c0::Prepared prepared{};prepared.target_observed_ns=123;
    C(goal::prepare_c0(frozen,placement,source,value.key,model,frame,marker,policy,prepared)==c0::Code::unavailable);
    C(prepared.target_observed_ns==123&&prepared.tracking_received_ns==0);
    finish();
    std::printf("cover_full_goal_live: %u checks, %u failures; synthetic provider, no C0/physics permission\n",checks,failures);
    return failures?1:0;
}
