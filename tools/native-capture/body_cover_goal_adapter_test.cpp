// Recorded retail skeleton; Core and cover placements are CPU fixtures.
// No current cover geometry, engine permission or game result is asserted.
#define main ExistingBodyGoalAdapterMain
#include "body_goal_view_adapter_test.cpp"
#undef main
#include "body_c0_neutral.h"
namespace neutral=gears_body_c0_neutral;
int main(int argc,char** argv){
    const bool measurement_only=argc==5&&std::string(argv[4])=="--measurement-only";
    const bool marcus=(argc==4||measurement_only)&&std::string(argv[3])=="--marcus";
    if(argc!=3&&!marcus)return 2;
    auto frame=std::make_unique<b::Frame>();auto model=std::make_unique<b::Model>();
    auto marker_work=std::make_unique<mount::Workspace>();auto work=std::make_unique<b::Workspace>();
    mount::Marker marker{};a::core::CoreSnapshot source{};
    C(read(argv[1],&source,sizeof source));
    C(read(std::string(argv[2])+"/reference.bin",frame->reference.data(),sizeof frame->reference));
    C(read(std::string(argv[2])+"/C0-local.bin",frame->local.data(),sizeof frame->local));
    C(read(std::string(argv[2])+"/C0-component.bin",&frame->component_to_world,sizeof frame->component_to_world));
    frame->units_per_metre=source.units_per_metre;
    C(b::prepare_model(frame->reference,digest,*model));
    C(mount::prepare(*model,*frame,marcus?mount::Basis::marcus_mesh_face_forward_y:
        mount::Basis::designer_authored_ue_actor_axes,*marker_work,marker)==mount::Status::okay);
    neutral::Stored stored{};a::Neutral calibrated{};
    C(neutral::capture(*model,*frame,marker,*work,stored));C(neutral::move(stored,frame->component_to_world,calibrated));
    if(failures)return 1;
    a::Policy policy{};policy.now_ns=int64_t(source.published_ns);policy.allow_sim=true;
    policy.scope=b::Scope::fixed_cover;policy.posture=b::Posture::reference_neutral;
    source.flags|=a::core::rig::cover_key_known;source.cover_link=source.world;source.cover_type=2;source.cover_slot=0;
    source.stereo.head=source.reference;
    a::CoverPoseMeasurement measured{};const auto frame_before=bytes(*frame);
    C(a::measure_cover_pose(source,*model,*frame,marker,policy,*work,measured)==a::Status::okay);
    C(bytes(*frame)==frame_before);
    C(measured.binding.publication==source.publication_sequence&&measured.binding.scene==source.scene_invocation&&
      measured.binding.observed_ns==int64_t(source.published_ns));
    if(!measurement_only)for(unsigned i=0;i<2;++i)C(distance(measured.feet[i].position,calibrated.feet_world[i].position)<1e-8);
    auto head_changed=source;head_changed.stereo.head.position.x+=.1;head_changed.world_head.position.z+=100;
    a::CoverPoseMeasurement same_pose{};
    C(a::measure_cover_pose(head_changed,*model,*frame,marker,policy,*work,same_pose)==a::Status::okay);
    C(bytes(measured)==bytes(same_pose)); // Raw C0 measurement must not reapply HMD/camera.
    auto invalid_source=source;invalid_source.flags&=~a::core::rig::cover_key_known;
    const auto measurement_before=bytes(measured);
    C(a::measure_cover_pose(invalid_source,*model,*frame,marker,policy,*work,measured)!=a::Status::okay);
    C(bytes(measured)==measurement_before);
    // Keep the same rejection predicates and output atomicity, but expose
    // which original-pose shape failed in the live measure status.
    auto broken=std::make_unique<b::Frame>(*frame);
    broken->component_to_world.values[0]*=2;broken->component_to_world.values[1]*=2;
    broken->component_to_world.values[2]*=2;
    C(a::measure_cover_pose(source,*model,*broken,marker,policy,*work,measured)==a::Status::cover_component);
    C(bytes(measured)==measurement_before);
    *broken=*frame;broken->local[b::head].scale=1.01f;
    C(a::measure_cover_pose(source,*model,*broken,marker,policy,*work,measured)==a::Status::cover_scale);
    C(bytes(measured)==measurement_before);
    *broken=*frame;broken->local[b::head].q[0]=NAN;
    C(a::measure_cover_pose(source,*model,*broken,marker,policy,*work,measured)==a::Status::cover_atom);
    C(bytes(measured)==measurement_before);
    if(measurement_only){printf("body_cover_measurement: %u checks, %u failures; recorded retail skeleton, Host Core; Game=0\n",checks,failures);return failures?1:0;}
    a::CoverPlacement placement{};placement.world=source.world;placement.pawn=source.pawn;placement.mesh=source.mesh;
    placement.link=source.cover_link;placement.world_generation=source.world_generation;placement.scene=source.scene_invocation;
    placement.publication=source.publication_sequence;placement.reference=source.reference_generation;
    placement.observed_ns=policy.now_ns;placement.type=2;placement.slot=0;
    a::marker::HeadBoneWorld calibrated_head{};
    C(a::marker::head_from_eye(marker,*model,*frame,
        {{calibrated.eye_anchor.neutral_eye_world,axis(2,std::atan2(calibrated.eye_anchor.forward_world.y,
                                                                calibrated.eye_anchor.forward_world.x))}},calibrated_head)==a::marker::Status::okay);
    a::Output output{};v::CoverOffset committed{.02,.03};unsigned solved{};
    for(double turn:{0.,1.5707963267948966,3.1415926535897932,-1.5707963267948966})for(double drop:{0.,.15,.30}){
        const auto q=axis(2,turn);placement.anchor=calibrated.eye_anchor;
        placement.anchor.forward_world=rotate(q,calibrated.eye_anchor.forward_world);
        placement.anchor.right_world=rotate(q,calibrated.eye_anchor.right_world);
        placement.anchor.neutral_eye_world.z-=drop*source.units_per_metre;
        placement.view_anchor=calibrated.eye_anchor;
        placement.view_anchor.neutral_eye_world=placement.anchor.neutral_eye_world;
        // Explicit ground placement turns around the neutral HEAD pivot,
        // while the view/eye heading stays continuous. Eye lowering does not
        // move the ground. The adapter may not silently move these feet.
        const auto head_pivot=plus(calibrated_head.pose.position,
            minus(placement.anchor.neutral_eye_world,calibrated.eye_anchor.neutral_eye_world));
        auto ground_pivot=head_pivot;ground_pivot.z=calibrated_head.pose.position.z;
        for(unsigned i=0;i<2;++i){placement.feet_world[i].position=plus(ground_pivot,
                rotate(q,minus(calibrated.feet_world[i].position,calibrated_head.pose.position)));
            placement.feet_world[i].orientation=oracle_multiply(q,calibrated.feet_world[i].orientation);}
        a::Neutral placed{};C(a::place_cover(calibrated,placement.anchor,placement.view_anchor,placement.feet_world,placed)==a::Status::okay);
        C(distance(placed.pelvis_world,plus(placement.anchor.neutral_eye_world,
            rotate(q,minus(calibrated.pelvis_world,calibrated.eye_anchor.neutral_eye_world))))<1e-9);
        for(unsigned i=0;i<2;++i)C(distance(placed.feet_world[i].position,placement.feet_world[i].position)==0);
        const auto saved=calibrated;const auto snapshot=source;
        C(a::prepare_cover(calibrated,placement,source,*model,*frame,marker,committed,policy,output)==a::Status::okay);
        C(distance(output.goal.absolute.neutral_pelvis_world,
            plus(head_pivot,rotate(q,minus(calibrated.pelvis_world,calibrated_head.pose.position))))<1e-5);
        a::Output unchanged_view{};
        C(a::prepare(calibrated,source,*model,*frame,marker,{},policy,unchanged_view)==a::Status::okay);
        // A wall-facing body is not permission to turn the physical view.
        C(distance(rotate(output.desired_eye.orientation,{1,0,0}),
                   rotate(unchanged_view.desired_eye.orientation,{1,0,0}))<1e-6);
        C(bytes(saved)==bytes(calibrated)&&bytes(snapshot)==bytes(source));
        C(output.goal.scope==b::Scope::fixed_cover);
        C(std::abs(distance(output.goal.committed_grip_world,times(placement.anchor.right_world,.02*source.units_per_metre))-.03*source.units_per_metre)<1e-9);
        b::Output pose{};const auto report=b::plan(*model,*frame,output.goal,*work,pose);const auto status=report.status;
        if(status!=b::Status::solved)printf("cover plan turn=%.6g drop=%.6g status=%u distance=%.12g reach=%.12g head_error=%.9g\n",turn,drop,unsigned(status),report.requested_distance,report.available_reach,pose.report.head_error);
        C(status==b::Status::solved);if(status==b::Status::solved)++solved;
        const v::Vector physical{.04,-.04,-.03};auto tracked=source;
        tracked.stereo.head.position=plus(source.reference.position,rotate(source.reference.orientation,physical));
        a::Output moving{};C(a::prepare_cover(calibrated,placement,tracked,*model,*frame,marker,committed,policy,moving)==a::Status::okay);
        C(moving.render.base_view==output.render.base_view);
        C(distance(moving.goal.physical_world,map(placed,physical,source.units_per_metre))<1e-9);
        C(distance(moving.goal.committed_grip_world,output.goal.committed_grip_world)==0);
        auto looking=tracked;looking.stereo.head.orientation=oracle_multiply(source.reference.orientation,axis(1,.3));
        a::Output looked{};C(a::prepare_cover(calibrated,placement,looking,*model,*frame,marker,committed,policy,looked)==a::Status::okay);
        C(angle_error(looked.goal.absolute.root_orientation.value,output.goal.absolute.root_orientation.value)<1e-10);
        C(distance(looked.goal.absolute.neutral_pelvis_world,output.goal.absolute.neutral_pelvis_world)==0);
        C(distance(looked.goal.physical_world,moving.goal.physical_world)==0);
        C(distance(rotate(looked.desired_eye.orientation,{1,0,0}),rotate(moving.desired_eye.orientation,{1,0,0}))>.1);
        for(unsigned i=0;i<2;++i)C(distance(moving.goal.feet_world[i].position,placement.feet_world[i].position)==0);
    }
    // Explicitly keeping the OLD eye-pivot feet is not silently repaired.
    // Conversion grants no reach permission: the unchanged fixed-length
    // planner still rejects this incompatible requested body placement.
    auto bad_feet=placement;const auto quarter=axis(2,1.5707963267948966);
    bad_feet.anchor=calibrated.eye_anchor;bad_feet.view_anchor=calibrated.eye_anchor;
    bad_feet.anchor.forward_world=rotate(quarter,calibrated.eye_anchor.forward_world);
    bad_feet.anchor.right_world=rotate(quarter,calibrated.eye_anchor.right_world);
    for(unsigned i=0;i<2;++i){bad_feet.feet_world[i].position=plus(calibrated.eye_anchor.neutral_eye_world,
        rotate(quarter,minus(calibrated.feet_world[i].position,calibrated.eye_anchor.neutral_eye_world)));
        bad_feet.feet_world[i].orientation=oracle_multiply(quarter,calibrated.feet_world[i].orientation);}
    a::Output rejected_pose{};C(a::prepare_cover(calibrated,bad_feet,source,*model,*frame,marker,{},policy,rejected_pose)==a::Status::okay);
    b::Output rejected{};C(b::plan(*model,*frame,rejected_pose.goal,*work,rejected).status==b::Status::spine_unreachable);
    for(unsigned i=0;i<2;++i)C(distance(rejected_pose.goal.feet_world[i].position,bad_feet.feet_world[i].position)==0);
    // No wrong-scene, old geometry, changed avatar or standing context may
    // become a cover goal; a rejection must not alter the caller's output.
    for(unsigned bad=0;bad<14;++bad){auto p=placement;auto s=source;auto pol=policy;const auto before=bytes(output);
        if(bad==0)++p.scene;if(bad==1)++p.publication;if(bad==2)++p.reference;
        if(bad==3)++p.world_generation;if(bad==4)++p.slot;if(bad==5)++p.link.name_number;
        if(bad==6)++p.mesh.name_number;if(bad==7)p.observed_ns=policy.now_ns-50000001;
        if(bad==8)p.observed_ns=policy.now_ns+1;if(bad==9)s.flags&=~a::core::rig::cover_key_known;
        if(bad==10)pol.scope=b::Scope::stationary;if(bad==11)p.anchor.forward_world.z=.1;
        if(bad==12)p.view_anchor={};if(bad==13)p.view_anchor.neutral_eye_world.x+=1;
        C(a::prepare_cover(calibrated,p,s,*model,*frame,marker,committed,pol,output)!=a::Status::okay);
        C(bytes(output)==before);
    }
    printf("body_cover_goal_adapter: %u checks, %u failures, %u full-body plans; marker=%s; CPU only\n",checks,failures,solved,marcus?"Marcus":"designer");
    return failures?1:0;
}
