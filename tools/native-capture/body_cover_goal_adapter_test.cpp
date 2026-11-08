// Recorded retail skeleton; Core and cover placements are CPU fixtures.
// No current cover geometry, engine permission or game result is asserted.
#define main ExistingBodyGoalAdapterMain
#include "body_goal_view_adapter_test.cpp"
#undef main
#include "body_c0_neutral.h"
namespace neutral=gears_body_c0_neutral;
int main(int argc,char** argv){
    if(argc!=3)return 2;
    auto frame=std::make_unique<b::Frame>();auto model=std::make_unique<b::Model>();
    auto marker_work=std::make_unique<mount::Workspace>();auto work=std::make_unique<b::Workspace>();
    mount::Marker marker{};a::core::CoreSnapshot source{};
    C(read(argv[1],&source,sizeof source));
    C(read(std::string(argv[2])+"/reference.bin",frame->reference.data(),sizeof frame->reference));
    C(read(std::string(argv[2])+"/C0-local.bin",frame->local.data(),sizeof frame->local));
    C(read(std::string(argv[2])+"/C0-component.bin",&frame->component_to_world,sizeof frame->component_to_world));
    frame->units_per_metre=source.units_per_metre;
    C(b::prepare_model(frame->reference,digest,*model));
    C(mount::prepare(*model,*frame,mount::Basis::designer_authored_ue_actor_axes,*marker_work,marker)==mount::Status::okay);
    neutral::Stored stored{};a::Neutral calibrated{};
    C(neutral::capture(*model,*frame,marker,*work,stored));C(neutral::move(stored,frame->component_to_world,calibrated));
    if(failures)return 1;
    a::Policy policy{};policy.now_ns=int64_t(source.published_ns);policy.allow_sim=true;
    policy.scope=b::Scope::fixed_cover;policy.posture=b::Posture::reference_neutral;
    source.flags|=a::core::rig::cover_key_known;source.cover_link=source.world;source.cover_type=2;source.cover_slot=0;
    source.stereo.head=source.reference;
    a::CoverPlacement placement{};placement.world=source.world;placement.pawn=source.pawn;placement.mesh=source.mesh;
    placement.link=source.cover_link;placement.world_generation=source.world_generation;placement.scene=source.scene_invocation;
    placement.publication=source.publication_sequence;placement.reference=source.reference_generation;
    placement.observed_ns=policy.now_ns;placement.type=2;placement.slot=0;
    a::Output output{};v::CoverOffset committed{.02,.03};unsigned solved{};
    for(double turn:{0.,1.5707963267948966,3.1415926535897932,-1.5707963267948966})for(double drop:{0.,.15,.30}){
        const auto q=axis(2,turn);placement.anchor=calibrated.eye_anchor;
        placement.anchor.forward_world=rotate(q,calibrated.eye_anchor.forward_world);
        placement.anchor.right_world=rotate(q,calibrated.eye_anchor.right_world);
        placement.anchor.neutral_eye_world.z-=drop*source.units_per_metre;
        // The fixture author rotates the complete ground placement. The
        // adapter itself must retain these explicitly chosen foot anchors.
        for(unsigned i=0;i<2;++i){placement.feet_world[i].position=plus(calibrated.eye_anchor.neutral_eye_world,
                rotate(q,minus(calibrated.feet_world[i].position,calibrated.eye_anchor.neutral_eye_world)));
            placement.feet_world[i].orientation=oracle_multiply(q,calibrated.feet_world[i].orientation);}
        a::Neutral placed{};C(a::place_cover(calibrated,placement.anchor,placement.feet_world,placed)==a::Status::okay);
        C(distance(placed.pelvis_world,plus(placement.anchor.neutral_eye_world,
            rotate(q,minus(calibrated.pelvis_world,calibrated.eye_anchor.neutral_eye_world))))<1e-9);
        for(unsigned i=0;i<2;++i)C(distance(placed.feet_world[i].position,placement.feet_world[i].position)==0);
        const auto saved=calibrated;const auto snapshot=source;
        C(a::prepare_cover(calibrated,placement,source,*model,*frame,marker,committed,policy,output)==a::Status::okay);
        C(bytes(saved)==bytes(calibrated)&&bytes(snapshot)==bytes(source));
        C(output.goal.scope==b::Scope::fixed_cover);
        C(std::abs(distance(output.goal.committed_grip_world,times(placement.anchor.right_world,.02*source.units_per_metre))-.03*source.units_per_metre)<1e-9);
        b::Output pose{};const auto report=b::plan(*model,*frame,output.goal,*work,pose);const auto status=report.status;
        if(status!=b::Status::solved)printf("cover plan turn=%.6g status=%u head_error=%.9g\n",turn,unsigned(status),pose.report.head_error);
        C(status==b::Status::solved);if(status==b::Status::solved)++solved;
        const v::Vector physical{.04,-.04,-.03};auto tracked=source;
        tracked.stereo.head.position=plus(source.reference.position,rotate(source.reference.orientation,physical));
        a::Output moving{};C(a::prepare_cover(calibrated,placement,tracked,*model,*frame,marker,committed,policy,moving)==a::Status::okay);
        C(moving.render.base_view==output.render.base_view);
        C(distance(moving.goal.physical_world,map(placed,physical,source.units_per_metre))<1e-9);
        C(distance(moving.goal.committed_grip_world,output.goal.committed_grip_world)==0);
        for(unsigned i=0;i<2;++i)C(distance(moving.goal.feet_world[i].position,placement.feet_world[i].position)==0);
    }
    // No wrong-scene, old geometry, changed avatar or standing context may
    // become a cover goal; a rejection must not alter the caller's output.
    for(unsigned bad=0;bad<12;++bad){auto p=placement;auto s=source;auto pol=policy;const auto before=bytes(output);
        if(bad==0)++p.scene;if(bad==1)++p.publication;if(bad==2)++p.reference;
        if(bad==3)++p.world_generation;if(bad==4)++p.slot;if(bad==5)++p.link.name_number;
        if(bad==6)++p.mesh.name_number;if(bad==7)p.observed_ns=policy.now_ns-50000001;
        if(bad==8)p.observed_ns=policy.now_ns+1;if(bad==9)s.flags&=~a::core::rig::cover_key_known;
        if(bad==10)pol.scope=b::Scope::stationary;if(bad==11)p.anchor.forward_world.z=.1;
        C(a::prepare_cover(calibrated,p,s,*model,*frame,marker,committed,pol,output)!=a::Status::okay);
        C(bytes(output)==before);
    }
    printf("body_cover_goal_adapter: %u checks, %u failures, %u full-body plans; CPU only\n",checks,failures,solved);
    return failures?1:0;
}
