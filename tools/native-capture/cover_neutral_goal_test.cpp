// Retained61492 geometry/calibration, with an EXPLICIT CPU cover-key fixture.
// Original CT0 source is not relabelled as runtime evidence or a write permit.
#define main ExistingCoverGoalMathMain
#include "body_goal_view_adapter_test.cpp"
#undef main
#include "cover_neutral_goal.h"
namespace ng=gears_cover_neutral_goal;
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
    C(bytes(*frame)==frame_before&&bytes(source)==source_before&&bytes(calibrated)==calibrated_before);
    printf("cover_neutral_goal: %u checks, %u failures; retained geometry, CPU cover binding; no runtime permission\n",checks,failures);
    return failures?1:0;
}
