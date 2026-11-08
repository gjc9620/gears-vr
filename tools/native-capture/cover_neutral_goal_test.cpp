// Retained61492 geometry/calibration, with an EXPLICIT CPU cover-key fixture.
// Original CT0 source is not relabelled as runtime evidence or a write permit.
#define main ExistingCoverGoalMathMain
#include "body_goal_view_adapter_test.cpp"
#undef main
#include "cover_neutral_goal.h"
namespace ng=gears_cover_neutral_goal;
// Successful Goal values contain four bytes of C++ padding before absolute.
// Compare every named field, while rejection checks still require the actual
// destination object's bytes to remain unchanged.
bool same_converted(a::Output const& x,a::Output const& y){
    auto const& p=x.goal;auto const& q=y.goal;
    return p.scope==q.scope&&p.posture==q.posture&&p.body_yaw_delta==q.body_yaw_delta&&p.target_mode==q.target_mode&&
        distance(p.physical_world,q.physical_world)==0&&distance(p.committed_grip_world,q.committed_grip_world)==0&&
        ng::same(p.head_world,q.head_world)&&ng::same(p.feet_world[0],q.feet_world[0])&&ng::same(p.feet_world[1],q.feet_world[1])&&
        ng::same(b::Pose{p.absolute.neutral_pelvis_world,p.absolute.root_orientation.value},
                 b::Pose{q.absolute.neutral_pelvis_world,q.absolute.root_orientation.value})&&
        x.render.base_view==y.render.base_view&&ng::same(x.render.base_eye_world,y.render.base_eye_world)&&
        v::stereo_same_pose(x.desired_eye,y.desired_eye);
}
int main(int argc,char** argv){
    if(argc!=2)return 2;const std::string folder=argv[1];
    auto frame=std::make_unique<b::Frame>();auto model=std::make_unique<b::Model>();
    auto marker_work=std::make_unique<mount::Workspace>();auto work=std::make_unique<ng::Workspace>();
    mount::Marker marker{};a::core::CoreSnapshot original{};a::Neutral calibrated{};
    C(read(folder+"/source-CT0.bin",&original,sizeof original));
    C(read(folder+"/calibrated-neutral.bin",&calibrated,sizeof calibrated));
    C(read(folder+"/reference.bin",frame->reference.data(),sizeof frame->reference));
    C(read(folder+"/C0-local.bin",frame->local.data(),sizeof frame->local));
    C(read(folder+"/C0-component.bin",&frame->component_to_world,sizeof frame->component_to_world));
    C(original.cover_type==0);frame->units_per_metre=original.units_per_metre;
    C(b::prepare_model(frame->reference,digest,*model));
    C(mount::prepare(*model,*frame,mount::Basis::marcus_mesh_face_forward_y,*marker_work,marker)==mount::Status::okay);
    if(failures)return 1;
    auto source=original;source.flags|=a::core::rig::cover_key_known;
    source.cover_type=2;source.cover_slot=0;source.cover_link=source.world;
    a::Policy policy{};policy.now_ns=int64_t(source.published_ns);policy.allow_sim=true;
    policy.scope=b::Scope::fixed_cover;policy.posture=b::Posture::preserve_c0;
    a::CoverPoseMeasurement measured{};
    C(a::measure_cover_pose(source,*model,*frame,marker,policy,work->fitting.body_work,measured)==a::Status::okay);
    a::CoverPlacement placement=measured.binding;
    const auto theta=std::atan2(-.5956991910934448,.8032075762748718);
    placement.anchor={measured.eye.position,{std::cos(theta),std::sin(theta),0},{-std::sin(theta),std::cos(theta),0}};
    const auto view_theta=-4667.*6.2831853071795864769/65536.;
    placement.view_anchor={measured.eye.position,{std::cos(view_theta),std::sin(view_theta),0},{-std::sin(view_theta),std::cos(view_theta),0}};
    for(unsigned i=0;i<2;++i)placement.feet_world[i]=measured.feet[i];
    a::Output old{};
    C(a::prepare_cover(calibrated,placement,source,*model,*frame,marker,{},policy,old)==a::Status::okay);
    C(old.goal.absolute.neutral_pelvis_world.z<placement.feet_world[0].position.z);
    const auto frame_before=bytes(*frame);const auto source_before=bytes(source);const auto calibrated_before=bytes(calibrated);
    ng::Result result{};
    for(auto posture:{b::Posture::preserve_c0,b::Posture::reference_neutral}){
        policy.posture=posture;
        const auto status=ng::prepare(calibrated,placement,source,*model,*frame,marker,policy,{.25},*work,result);
        if(status!=ng::Status::okay)printf("neutral posture=%u status=%u\n",unsigned(posture),unsigned(status));
        C(status==ng::Status::okay);if(status!=ng::Status::okay)continue;
        C(result.fitting.validation.status==b::Status::solved);
        C(result.neutral.pelvis_world.z==measured.pelvis.position.z);
        C(std::abs(result.fitting.actual_pelvis.z-measured.pelvis.position.z)<.0005);
        C(result.fitting.horizontal_units>0&&result.fitting.horizontal_units<=25.0005);
        C(bytes(result.binding)==bytes(placement));
        C(bytes(result.neutral.feet_world)==bytes(placement.feet_world));
        C(distance(result.fitting.goal.physical_world,{})==0&&distance(result.fitting.goal.committed_grip_world,{})==0);
        const auto saved=result;
        auto tracked=source;
        const v::Vector physical{.025,.015,-.02};
        tracked.stereo.head.position=plus(source.reference.position,rotate(source.reference.orientation,physical));
        tracked.stereo.head.orientation=oracle_multiply(source.reference.orientation,axis(1,.15));
        const auto tracked_before=bytes(tracked);
        ng::Result neutral_after_tracking{};
        C(ng::prepare(calibrated,placement,tracked,*model,*frame,marker,policy,{.25},*work,neutral_after_tracking)==ng::Status::okay);
        C(bytes(neutral_after_tracking)==bytes(saved));
        C(bytes(tracked)==tracked_before);
        a::Output moving{},still{};auto reference_tracking=source.stereo;reference_tracking.head=source.reference;
        C(a::prepare_axes_math(result.neutral,placement.anchor,source,reference_tracking,*model,*frame,marker,{},policy,still)==a::Status::okay);
        const v::CoverOffset grip{.02,.03};
        C(a::prepare_axes_math(result.neutral,placement.anchor,tracked,tracked.stereo,*model,*frame,marker,grip,policy,moving)==a::Status::okay);
        const auto physical_world=map(result.neutral,physical,source.units_per_metre);
        const auto virtual_world=plus(times(placement.anchor.right_world,grip.right*source.units_per_metre),{0,0,grip.up*source.units_per_metre});
        C(distance(moving.goal.physical_world,physical_world)<1e-9);
        C(distance(moving.goal.committed_grip_world,virtual_world)<1e-9);
        // Renderer origins are float; account for that single encoding only.
        C(distance(minus(moving.desired_eye.position,still.desired_eye.position),plus(physical_world,virtual_world))<.001);
        C(bytes(moving.goal.feet_world)==bytes(placement.feet_world));
        C(distance(moving.goal.absolute.neutral_pelvis_world,result.neutral.pelvis_world)==0);
        // Initial placement is not a per-frame conversion of the fitted value:
        // it would apply the independent wall/view pivot correction again.
        a::Output replaced{},frozen_conversion{};
        C(a::prepare_cover(result.neutral,placement,source,*model,*frame,marker,{},policy,replaced)==a::Status::okay);
        C(distance(replaced.goal.absolute.neutral_pelvis_world,result.neutral.pelvis_world)>1e-3);
        C(ng::convert(result,placement,tracked,*model,*frame,marker,grip,policy,frozen_conversion)==ng::Status::okay);
        C(distance(frozen_conversion.goal.absolute.neutral_pelvis_world,result.neutral.pelvis_world)==0);
        C(distance(frozen_conversion.goal.physical_world,moving.goal.physical_world)<1e-9);
        C(distance(frozen_conversion.goal.committed_grip_world,moving.goal.committed_grip_world)<1e-9);
        C(bytes(frozen_conversion.goal.feet_world)==bytes(placement.feet_world));
        C(distance(frozen_conversion.desired_eye.position,moving.desired_eye.position)<1e-9);
        const auto moving_plan=b::plan(*model,*frame,frozen_conversion.goal,work->fitting.body_work,work->fitting.planned);
        if(moving_plan.status!=b::Status::solved)printf("frozen moving posture=%u status=%u bone=%u requested=%.9g available=%.9g head=%.9g\n",unsigned(posture),unsigned(moving_plan.status),moving_plan.failed_bone,moving_plan.requested_distance,moving_plan.available_reach,moving_plan.head_error);
        C(moving_plan.status==b::Status::solved);
        C(bytes(result)==bytes(saved));
        printf("neutral posture=%u shift=%.9g hip=%.9g; tracking+grip applied once\n",unsigned(posture),result.fitting.horizontal_units,result.neutral.pelvis_world.z);
    }
    for(unsigned bad=0;bad<17;++bad){
        auto p=placement;auto s=source;auto pol=policy;auto f=*frame;ng::fit::Limits limit{.25};
        if(bad==0)++p.scene;if(bad==1)++p.publication;if(bad==2)++p.reference;
        if(bad==3)++p.world_generation;if(bad==4)++p.slot;if(bad==5)++p.link.name_number;
        if(bad==6)++p.mesh.name_number;if(bad==7)p.observed_ns-=50000001;
        if(bad==8)p.observed_ns=pol.now_ns+1;if(bad==9)s.flags&=~a::core::rig::cover_key_known;
        if(bad==10)pol.scope=b::Scope::stationary;if(bad==11)p.view_anchor.neutral_eye_world.x+=1;
        if(bad==12)limit.horizontal_metres=0;if(bad==13)f.local[0].scale=.5;
        if(bad==14)f.count=114;if(bad==15)s.published_ns+=1;
        if(bad==16)p.anchor.forward_world.z=.1;
        const auto before=bytes(result);
        C(ng::prepare(calibrated,p,s,*model,f,marker,pol,limit,*work,result)!=ng::Status::okay);
        C(bytes(result)==before);
    }
    const auto before=bytes(result);
    C(ng::prepare(result.neutral,placement,source,*model,*frame,marker,policy,{.25},*work,result)==ng::Status::alias);
    C(bytes(result)==before);
    // Current V3 acquisition on an older immutable Scene association. These
    // are CPU clocks/cohorts; the original retained source file stays CT0.
    namespace src=gears_body_tick_source;
    src::Input input{};input.scene_cache_revision=2;
    const auto now=std::int64_t(source.published_ns)+1000000000;
    input.metadata_begin_ns=now-10000000;input.metadata_end_ns=now-8000000;
    auto& tick=input.observed;tick.parent_scene=source;tick.tick=1;tick.thread=source.native.game_thread;tick.parent_cookie=123;
    tick.begin_ns=now-7000000;tick.end_ns=now-3000000;
    tick.stereo=source.stereo;tick.stereo.sequence+=5;tick.stereo.received_ns=now-4000000;tick.stereo.predicted_time+=1000000000;
    tick.controllers=source.controllers;tick.controllers.sequence=tick.stereo.sequence;
    tick.controllers.predicted_time=tick.stereo.predicted_time;
    tick.controllers.received_ns=now-5000000;tick.controllers.sync_ns=now-6000000;
    tick.continuity.input=tick.controllers;tick.continuity.stream_epoch=source.producer.stream;
    for(unsigned i=0;i<2;++i)tick.continuity.tracking_epoch[i]=source.producer.tracking[i];
    tick.reference_revision=source.native_reference_revision;tick.native_reference=source.native_reference;
    tick.publisher_pid=source.controller_publisher_pid;tick.publisher_start=source.controller_publisher_start;
    auto current_policy=policy;current_policy.now_ns=now;
    auto current_placement=placement;current_placement.observed_ns=input.metadata_end_ns;
    if(src::classify(input,source,now,true)!=src::Verdict::okay)
        printf("V3 fixture hard bits=%x flags=%x producer=%llu publisher=%u\n",src::rejection_detail(input,source,now,true),source.flags,source.producer.stream,source.controller_publisher_pid);
    C(src::classify(input,source,now,true)==src::Verdict::okay);
    C(!a::core::core_snapshot_valid(source,now,a::maximum_age_ns,true));
    const auto input_before=bytes(input);
    a::CoverPoseMeasurement current_measure{};
    C(a::measure_cover_pose(input,*model,*frame,marker,current_policy,work->fitting.body_work,current_measure)==a::Status::okay);
    C(current_measure.binding.observed_ns==input.metadata_end_ns);
    C(current_measure.binding.publication==source.publication_sequence&&current_measure.binding.scene==source.scene_invocation);
    C(distance(current_measure.pelvis.position,measured.pelvis.position)==0);
    ng::Result current_neutral{};
    C(ng::prepare(calibrated,current_placement,input,*model,*frame,marker,current_policy,{.25},*work,current_neutral)==ng::Status::okay);
    C(bytes(current_neutral.neutral)==bytes(result.neutral));
    C(ng::prepare(calibrated,current_placement,source,*model,*frame,marker,current_policy,{.25},*work,current_neutral)!=ng::Status::okay);
    a::Output converted{};
    C(a::prepare_cover(calibrated,current_placement,input,*model,*frame,marker,{},current_policy,converted)==a::Status::okay);
    const auto converted_before=bytes(converted);
    C(a::prepare_cover(calibrated,current_placement,source,*model,*frame,marker,{},current_policy,converted)!=a::Status::okay);
    C(bytes(converted)==converted_before);
    C(bytes(input)==input_before);
    // The latest independent physical input is used; old world_head and the
    // old Scene Stereo never supply a pose to this conversion.
    const v::Vector current_physical{.025,.015,-.02};
    tick.stereo.head.position=plus(source.reference.position,rotate(source.reference.orientation,current_physical));
    tick.stereo.head.orientation=oracle_multiply(source.reference.orientation,axis(1,.15));
    const auto tracked_input_before=bytes(input);
    C(src::classify(input,source,now,true)==src::Verdict::okay);
    C(a::prepare_cover(calibrated,current_placement,input,*model,*frame,marker,{},current_policy,converted)==a::Status::okay);
    a::Neutral placed{};C(a::place_cover_head(calibrated,current_placement,input,*model,*frame,marker,current_policy,placed)==a::Status::okay);
    C(distance(converted.goal.physical_world,map(placed,current_physical,source.units_per_metre))<1e-9);
    ng::Result with_current_head{};
    C(ng::prepare(calibrated,current_placement,input,*model,*frame,marker,current_policy,{.25},*work,with_current_head)==ng::Status::okay);
    C(bytes(with_current_head.neutral)==bytes(result.neutral));
    a::Output current_frozen{},current_math{};const v::CoverOffset held{.02,.03};
    const auto fitted_before=bytes(with_current_head);
    C(ng::convert(with_current_head,current_placement,input,*model,*frame,marker,held,current_policy,current_frozen)==ng::Status::okay);
    C(a::prepare_axes_math(with_current_head.neutral,current_placement.anchor,source,input.observed.stereo,
        *model,*frame,marker,held,current_policy,current_math)==a::Status::okay);
    C(same_converted(current_frozen,current_math));
    C(distance(current_frozen.goal.absolute.neutral_pelvis_world,with_current_head.neutral.pelvis_world)==0);
    C(bytes(with_current_head)==fitted_before&&bytes(input)==tracked_input_before);
    const auto current_plan=b::plan(*model,*frame,current_frozen.goal,work->fitting.body_work,work->fitting.planned);
    if(current_plan.status!=b::Status::solved)printf("frozen current status=%u bone=%u requested=%.9g available=%.9g head=%.9g\n",unsigned(current_plan.status),current_plan.failed_bone,current_plan.requested_distance,current_plan.available_reach,current_plan.head_error);
    C(current_plan.status==b::Status::solved);
    // A later Scene/publication is allowed only with its independently current
    // placement. No old Scene, metadata or tracking clock is rewritten.
    auto later=input;auto later_place=current_placement;
    ++later.observed.parent_scene.publication_sequence;++later.observed.parent_scene.scene_invocation;
    later_place.publication=later.observed.parent_scene.publication_sequence;
    later_place.scene=later.observed.parent_scene.scene_invocation;
    C(src::classify(later,later.observed.parent_scene,now,true)==src::Verdict::okay);
    C(ng::convert(with_current_head,later_place,later,*model,*frame,marker,held,current_policy,current_frozen)==ng::Status::okay);
    C(same_converted(current_frozen,current_math));
    const auto later_before=bytes(current_frozen);
    C(ng::convert(with_current_head,current_placement,later,*model,*frame,marker,held,current_policy,current_frozen)==ng::Status::placement);
    C(bytes(current_frozen)==later_before);
    C(ng::convert(with_current_head,current_placement,source,*model,*frame,marker,held,current_policy,current_frozen)==ng::Status::placement);
    C(bytes(current_frozen)==later_before);
    // Mutate every stable calibration domain independently, keeping the actual
    // current acquisition valid so this tests the frozen-domain comparison.
    for(unsigned bad=0;bad<38;++bad){auto changed=with_current_head;auto& key=changed.basis;
        if(bad==0)++key.native.game_pid;if(bad==1)++key.controller.name_number;
        if(bad==2)key.reference.position.x+=.001;if(bad==3)++key.native_reference.epoch;
        if(bad==4)++key.native_reference.flags;if(bad==5)++key.native_revision;
        if(bad==6)key.source=key.source==v::StereoSource::RuntimeTracked?v::StereoSource::MetaOperatorSimulated:v::StereoSource::RuntimeTracked;if(bad==7)++key.session;
        if(bad==8)++key.space;if(bad==9)++key.observed_stream;
        if(bad==10)++key.observed_reference;if(bad==11)++key.observed_head;
        if(bad==12)++key.producer_stream;if(bad==13)++key.publisher_pid;
        if(bad==14)++key.publisher_start;if(bad==15)key.geometry[0]^=1;
        if(bad==16)key.units+=1;if(bad==17)key.model_scale+=.01;
        if(bad==18)++changed.binding.world.name_number;if(bad==19)++changed.binding.pawn.name_number;
        if(bad==20)++changed.binding.mesh.name_number;if(bad==21)++changed.binding.link.name_number;
        if(bad==22)++changed.binding.world_generation;if(bad==23)++changed.binding.reference;
        if(bad==24)++changed.binding.slot;if(bad==25)++changed.binding.type;
        if(bad==26)changed.binding.anchor.neutral_eye_world.x+=.001;
        if(bad==27)changed.binding.anchor.forward_world.x+=.001;
        if(bad==28)changed.binding.view_anchor.right_world.x+=.001;
        if(bad==29)changed.binding.feet_world[0].position.x+=.001;
        if(bad==30)changed.binding.feet_world[1].orientation.x+=.001;
        if(bad==31)changed.fitting.validation.status=b::Status::goal;
        if(bad==32)changed.neutral.pelvis_world.x+=.001;
        if(bad==33)changed.neutral.root_bone_world_q.x+=.001;
        if(bad==34)changed.neutral.eye_anchor.forward_world.x+=.001;
        if(bad==35)changed.neutral.feet_world[0].position.x+=.001;
        if(bad==36)changed.fitting.goal.physical_world.x=.001;
        if(bad==37)changed.fitting.goal.posture=changed.fitting.goal.posture==b::Posture::preserve_c0?
            b::Posture::reference_neutral:b::Posture::preserve_c0;
        const auto saved_output=bytes(current_frozen);const auto saved_fitted=bytes(changed);
        C(ng::convert(changed,current_placement,input,*model,*frame,marker,held,current_policy,current_frozen)==ng::Status::basis);
        C(bytes(current_frozen)==saved_output&&bytes(changed)==saved_fitted);
    }
    for(unsigned bad=0;bad<15;++bad){auto changed=input;auto p=current_placement;auto pol=current_policy;
        if(bad==0)changed.scene_cache_revision=0;
        if(bad==1)changed.metadata_begin_ns=now-50000001;
        if(bad==2)changed.metadata_end_ns=changed.observed.begin_ns+1;
        if(bad==3)changed.observed.begin_ns=now-40000001;
        if(bad==4)changed.observed.stereo.received_ns=now-40000001;
        if(bad==5)++changed.observed.reference_revision;
        if(bad==6)++changed.observed.publisher_start;
        if(bad==7)++changed.observed.controllers.sequence;
        if(bad==8)++p.scene;if(bad==9)++p.publication;
        if(bad==10)p.observed_ns=input.metadata_begin_ns-1;
        if(bad==11)p.observed_ns=input.metadata_end_ns+1;
        if(bad==12)pol.now_ns=now+50000001;
        if(bad==13)changed.observed.parent_scene.flags&=~a::core::rig::cover_key_known;
        if(bad==14)++changed.observed.parent_scene.cover_slot;
        const auto saved_result=bytes(with_current_head);const auto saved_output=bytes(converted);
        C(ng::prepare(calibrated,p,changed,*model,*frame,marker,pol,{.25},*work,with_current_head)!=ng::Status::okay);
        C(bytes(with_current_head)==saved_result);
        C(a::prepare_cover(calibrated,p,changed,*model,*frame,marker,{},pol,converted)!=a::Status::okay);
        C(bytes(converted)==saved_output);
        const auto saved_frozen_output=bytes(current_frozen);
        C(ng::convert(with_current_head,p,changed,*model,*frame,marker,held,pol,current_frozen)!=ng::Status::okay);
        C(bytes(current_frozen)==saved_frozen_output);
    }
    // Whole-input overlap must be rejected, including tracking outside the
    // parent Core subobject. Only an alias check reads this output reference.
    auto overlap=std::make_unique<src::Input>(input);const auto overlap_before=bytes(*overlap);
    auto& aliased=*reinterpret_cast<a::Output*>(&overlap->observed.stereo);
    C(a::prepare_cover(calibrated,current_placement,*overlap,*model,*frame,marker,{},current_policy,aliased)==a::Status::alias);
    C(bytes(*overlap)==overlap_before);
    C(ng::convert(with_current_head,current_placement,*overlap,*model,*frame,marker,held,current_policy,aliased)==ng::Status::alias);
    C(bytes(*overlap)==overlap_before);
    auto frozen_overlap=with_current_head;const auto frozen_overlap_before=bytes(frozen_overlap);
    auto& frozen_aliased=*reinterpret_cast<a::Output*>(&frozen_overlap);
    C(ng::convert(frozen_overlap,current_placement,input,*model,*frame,marker,held,current_policy,frozen_aliased)==ng::Status::alias);
    C(bytes(frozen_overlap)==frozen_overlap_before);
    C(bytes(input)==tracked_input_before);
    C(bytes(source)==source_before&&bytes(*frame)==frame_before);
    C(bytes(*frame)==frame_before&&bytes(source)==source_before&&bytes(calibrated)==calibrated_before);
    printf("cover_neutral_goal: %u checks, %u failures; retained geometry, CPU cover binding; no runtime permission\n",checks,failures);
    return failures?1:0;
}
