#include "full_body_c0_heading_test.inl"
#include "body_head_authored_mount.h"
namespace authored=gears_body_head_authored;
int main(int argc,char** argv){
    if(argc!=3)return 2;const int old=heading_plan_main(argc,argv);if(old)return old;const unsigned inherited_checks=checks;
    auto frame=std::make_unique<b::Frame>();auto work=std::make_unique<b::Workspace>();auto marker_work=std::make_unique<authored::Workspace>();
    b::Model model{};b::Output output{};authored::Marker marker{};
    C(read(std::string(argv[1])+"/reference.bin",frame->reference.data(),sizeof frame->reference));
    C(read(std::string(argv[1])+"/C0-local.bin",frame->local.data(),sizeof frame->local));
    C(read(std::string(argv[1])+"/C0-component.bin",&frame->component_to_world,sizeof frame->component_to_world));
    C(b::prepare_model(frame->reference,hash,model));C(authored::prepare(model,*frame,authored::Basis::designer_authored_ue_actor_axes,*marker_work,marker)==authored::Status::okay);
    const auto original=*frame;for(unsigned i=0;i<16;++i)work->component.m[i]=frame->component_to_world.values[i];std::copy(frame->local.begin(),frame->local.end(),work->pose.local);C(b::d::rebuild(model,*work));
    // Offline fixture authors choose this initial world anchor ONCE. Production
    // must supply its actor/Cover reference; no automatic reference API is added.
    b::Goal goal{};goal.scope=b::Scope::fixed_cover;goal.target_mode=b::BodyTargetMode::world_absolute;
    goal.absolute.neutral_pelvis_world=b::d::position(work->pose.world[b::pelvis]);
    goal.absolute.root_orientation.value=gears_vr::multiply(b::d::yaw(-.1),b::d::rotation(work->pose.world[b::root]));
    goal.physical_world={1,2,-3};goal.committed_grip_world={2,1,-2};
    const auto displacement=b::d::add(goal.physical_world,goal.committed_grip_world);
    const auto requested_pelvis=b::d::add(goal.absolute.neutral_pelvis_world,displacement);
    authored::HeadBoneWorld authored_head{b::d::pose(work->pose.world[b::head])};authored_head.pose.position=b::d::add(authored_head.pose.position,displacement);
    authored::EyeHmdWorld fixed_eye{};C(authored::eye_from_head(marker,model,*frame,authored_head,fixed_eye)==authored::Status::okay);
    authored::HeadBoneWorld fixed_head{};C(authored::head_from_eye(marker,model,*frame,fixed_eye,fixed_head)==authored::Status::okay);goal.head_world=fixed_head.pose;
    for(unsigned side=0;side<2;++side)goal.feet_world[side]=b::d::pose(work->pose.world[b::legs[side][2]]);
    std::array<unsigned char,sizeof goal> goal_bytes{};std::memcpy(goal_bytes.data(),&goal,sizeof goal);const auto eye_before=fixed_eye;
    b::Output first{};std::array<b::Pose,b::bones> first_world{};unsigned completed{};
    double root_drift{},pelvis_error{},head_error{},feet_error{},length_error{},root_angle{},eye_error{},pelvis_drift{},head_drift{},feet_drift{},projection{};
    for(unsigned iteration=0;iteration<1000;++iteration){const auto input=frame->local;const auto prior=output;
        const auto report=b::plan(model,*frame,goal,*work,output);C(report.status==b::Status::solved);
        if(report.status!=b::Status::solved){C(!std::memcmp(&prior,&output,sizeof output));std::printf("absolute feedback rejected iteration=%u status=%u distance=%g reach=%g\n",iteration,unsigned(report.status),report.requested_distance,report.available_reach);break;}
        ++completed;const auto world=independent(*frame,model,output);
        if(!iteration){first=output;first_world=world;}
        const auto pe=error(world[b::pelvis].position,requested_pelvis),he=error(world[b::head].position,goal.head_world.position),rd=error(world[b::root].position,first_world[b::root].position),pd=error(world[b::pelvis].position,first_world[b::pelvis].position);
        const auto ra=b::d::angle(world[b::root].orientation,goal.absolute.root_orientation.value);
        const auto hd=error(world[b::head].position,first_world[b::head].position);head_drift=(std::max)(head_drift,hd);
        C(pe<.002&&he<.002&&hd<.002&&rd<.001&&pd<.001&&ra<1e-4&&b::d::angle(world[b::head].orientation,goal.head_world.orientation)<1e-4);
        root_drift=(std::max)(root_drift,rd);pelvis_drift=(std::max)(pelvis_drift,pd);pelvis_error=(std::max)(pelvis_error,pe);head_error=(std::max)(head_error,he);root_angle=(std::max)(root_angle,ra);projection=(std::max)(projection,report.pelvis_projection);
        for(unsigned side=0;side<2;++side){const auto e=error(world[b::legs[side][2]].position,goal.feet_world[side].position),fd=error(world[b::legs[side][2]].position,first_world[b::legs[side][2]].position);feet_error=(std::max)(feet_error,e);feet_drift=(std::max)(feet_drift,fd);C(e<.002&&fd<.002&&b::d::angle(world[b::legs[side][2]].orientation,goal.feet_world[side].orientation)<1e-4);}
        for(unsigned i=0;i<b::bones;++i){C(!std::memcmp(&output.local[i].scale,&original.local[i].scale,4)&&(!i||!std::memcmp(output.local[i].translation,original.local[i].translation,12)));
            if(i){const auto& t=original.local[i].translation;const auto delta=std::abs(error(world[i].position,world[unsigned(model.parents[i])].position)-std::hypot(t[0],t[1],t[2]));length_error=(std::max)(length_error,delta);C(delta<.002);}
            const bool changed=std::memcmp(&input[i],&output.local[i],32)!=0;C(bool(output.changed_mask[i/64]&(std::uint64_t(1)<<(i%64)))==changed);}
        authored::EyeHmdWorld reconstructed{};C(authored::eye_from_head(marker,model,*frame,authored::HeadBoneWorld{output.head_world},reconstructed)==authored::Status::okay);
        const auto ee=error(reconstructed.pose.position,fixed_eye.pose.position);eye_error=(std::max)(eye_error,ee);C(ee<.002&&b::d::angle(reconstructed.pose.orientation,fixed_eye.pose.orientation)<1e-4);
        C(!std::memcmp(&goal,goal_bytes.data(),sizeof goal)&&!std::memcmp(&fixed_eye,&eye_before,sizeof fixed_eye)&&!std::memcmp(frame->local.data(),input.data(),sizeof input));
        frame->local=output.local; // The only feedback: simulate cached previous C0.
    }
    C(completed==1000);C(projection==0); // chosen fixture's absolute pelvis is reachable
    for(auto const& pair:{std::pair<char const*,b::Output const*>{"absolute-first",&first},{"absolute-1000",&output}}){std::ofstream f(std::string(argv[2])+"/"+pair.first+".local32.bin",std::ios::binary);f.write(reinterpret_cast<char const*>(pair.second->local.data()),sizeof pair.second->local);C(bool(f));}
    const auto stable_output=output;
    // Real authored Eye pivot, fixed Eye position. This changes neither neutral
    // nor physical/Grip values and does not turn the body Root.
    unsigned rotation_solved{};double rotation_pelvis{};
    for(double yaw:{.05,-.05,.1,-.1}){auto eye=fixed_eye;eye.pose.orientation=gears_vr::multiply(b::d::yaw(yaw),fixed_eye.pose.orientation);
        authored::HeadBoneWorld target{};C(authored::head_from_eye(marker,model,*frame,eye,target)==authored::Status::okay);auto rotated=goal;rotated.head_world=target.pose;const auto prior=output;
        const auto report=b::plan(model,*frame,rotated,*work,output);std::printf("absolute fixed-eye rotation yaw=%g status=%u distance=%g reach=%g\n",yaw,unsigned(report.status),report.requested_distance,report.available_reach);
        C(report.status==b::Status::solved);if(report.status!=b::Status::solved){C(!std::memcmp(&prior,&output,sizeof output));continue;}++rotation_solved;
        const auto p=error(output.pelvis_world.position,stable_output.pelvis_world.position);rotation_pelvis=(std::max)(rotation_pelvis,p);
        C(p<1e-6&&!std::memcmp(&output.local[b::root],&stable_output.local[b::root],32)&&!std::memcmp(&output.local[b::pelvis],&stable_output.local[b::pelvis],32));
        C(!std::memcmp(&rotated.absolute,&goal.absolute,sizeof goal.absolute));
        for(unsigned i=0;i<b::bones;++i)C(!std::memcmp(&output.local[i].scale,&original.local[i].scale,4)&&(!i||!std::memcmp(output.local[i].translation,original.local[i].translation,12)));
    }
    C(rotation_solved==4);
    for(unsigned bad=0;bad<9;++bad){auto g=goal;auto f=*frame;const auto saved=output;b::Status expected=b::Status::goal;
        if(bad==0)g.body_yaw_delta=1e-12;if(bad==1)g.absolute.neutral_pelvis_world.x=NAN;if(bad==2)g.absolute.root_orientation.value={0,0,0,0};
        if(bad==3)g.absolute.root_orientation.value.w=INFINITY;if(bad==4){g.target_mode=static_cast<b::BodyTargetMode>(99);expected=b::Status::policy;}
        if(bad==5)g.physical_world.x=INFINITY;if(bad==6){g.head_world.position.z+=100;expected=b::Status::spine_unreachable;}
        if(bad==7){f.model_scale=2;expected=b::Status::model_scale;}if(bad==8){f.reference[114].bytes[28]^=1;expected=b::Status::reference;}
        C(b::plan(model,f,g,*work,output).status==expected);C(!std::memcmp(&saved,&output,sizeof output));}
    std::ofstream result(std::string(argv[2])+"/absolute-feedback.json");result<<std::setprecision(17);
    result<<"{\"iterations\":"<<completed<<",\"max_root_drift_from_first\":"<<root_drift<<",\"max_pelvis_drift_from_first\":"<<pelvis_drift<<",\"max_head_drift_from_first\":"<<head_drift<<",\"max_feet_drift_from_first\":"<<feet_drift<<",\"max_pelvis_target_error\":"<<pelvis_error<<",\"max_head_error\":"<<head_error<<",\"max_feet_error\":"<<feet_error<<",\"max_length_error\":"<<length_error<<",\"max_root_angle_error\":"<<root_angle<<",\"max_eye_reconstruction_error\":"<<eye_error<<",\"max_projection\":"<<projection<<",\"fixed_eye_rotation_solved\":"<<rotation_solved<<",\"max_rotation_pelvis_change\":"<<rotation_pelvis<<",\"Goal_size\":"<<sizeof(b::Goal)<<",\"BodyTargetMode_offset\":"<<offsetof(b::Goal,target_mode)<<",\"WorldAbsoluteBody_offset\":"<<offsetof(b::Goal,absolute)<<",\"writer_permission\":false}\n";C(bool(result));result.close();
    std::printf("absolute feedback iterations=%u rootDrift=%g pelvisDrift=%g pelvis=%g head=%g feet=%g lengths=%g eye=%g pureRotationPelvis=%g\n",completed,root_drift,pelvis_drift,pelvis_error,head_error,feet_error,length_error,eye_error,rotation_pelvis);
    std::printf("full_body_c0_absolute: %u checks, %u failures; inherited=%u absolute=%u Game=0 writer=0\n",checks,failures,inherited_checks,checks-inherited_checks);return failures?1:0;
}
