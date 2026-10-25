#define main legacy_plan_main
#include "full_body_c0_planner_test.cpp"
#undef main
int heading_plan_main(int argc,char** argv){
    if(argc!=3)return 2;
    const int legacy=legacy_plan_main(argc,argv);if(legacy)return legacy;
    const unsigned legacy_checks=checks;
    auto frame=std::make_unique<b::Frame>();auto work=std::make_unique<b::Workspace>();b::Model model{};b::Output output{};
    C(read(std::string(argv[1])+"/reference.bin",frame->reference.data(),sizeof frame->reference));
    C(read(std::string(argv[1])+"/C0-local.bin",frame->local.data(),sizeof frame->local));
    C(read(std::string(argv[1])+"/C0-component.bin",&frame->component_to_world,sizeof frame->component_to_world));
    C(b::prepare_model(frame->reference,hash,model));
    for(unsigned i=0;i<16;++i)work->component.m[i]=frame->component_to_world.values[i];std::copy(frame->local.begin(),frame->local.end(),work->pose.local);C(b::d::rebuild(model,*work));
    const auto pivot=b::d::position(work->pose.world[b::pelvis]);const auto root_original=b::d::pose(work->pose.world[b::root]);
    b::Goal base{};base.scope=b::Scope::fixed_cover;base.head_world=b::d::pose(work->pose.world[b::head]);
    for(unsigned s=0;s<2;++s)base.feet_world[s]=b::d::pose(work->pose.world[b::legs[s][2]]);
    C(b::plan(model,*frame,base,*work,output).status==b::Status::solved);const auto identity=output;
    std::array<unsigned char,sizeof(b::Frame)> original_frame{};std::memcpy(original_frame.data(),frame.get(),sizeof *frame);
    std::ofstream rows(std::string(argv[2])+"/heading-cases.jsonl");rows<<std::setprecision(17);
    auto turn=[&](b::Pose p,double yaw){const double c=std::cos(yaw),s=std::sin(yaw);const auto v=b::d::sub(p.position,pivot);
        p.position={pivot.x+c*v.x-s*v.y,pivot.y+s*v.x+c*v.y,pivot.z+v.z};
        p.orientation=gears_vr::multiply(b::d::yaw(yaw),p.orientation);return p;};
    unsigned new_plans{},fixed_rejections{},fixed_solutions{};double maximum_head{},maximum_feet{},maximum_lengths{},maximum_pelvis{};
    auto evaluate=[&](b::Goal const& goal,char const* name,bool require_solution){
        const auto saved=output;const auto copied_goal=goal;const auto report=b::plan(model,*frame,goal,*work,output);
        const bool solved=report.status==b::Status::solved;bool untouched=!std::memcmp(&saved,&output,sizeof output);
        if(require_solution)C(solved);else if(solved)++fixed_solutions;else ++fixed_rejections;
        C(!std::memcmp(&goal,&copied_goal,sizeof goal));C(!std::memcmp(frame.get(),original_frame.data(),sizeof *frame));
        if(!solved){C(untouched);C(report.status!=b::Status::goal);if(!require_solution)C(report.status==b::Status::spine_unreachable&&report.requested_distance>report.available_reach);}
        else{
            ++new_plans;const auto world=independent(*frame,model,output);double feet{},lengths{};
            C(error(world[b::head].position,goal.head_world.position)<.002);C(b::d::angle(world[b::head].orientation,goal.head_world.orientation)<1e-4);
            for(unsigned s=0;s<2;++s){feet=(std::max)(feet,error(world[b::legs[s][2]].position,goal.feet_world[s].position));C(error(world[b::legs[s][2]].position,goal.feet_world[s].position)<.002);C(b::d::angle(world[b::legs[s][2]].orientation,goal.feet_world[s].orientation)<1e-4);}
            for(unsigned i=0;i<b::bones;++i){C(!std::memcmp(&output.local[i].scale,&frame->local[i].scale,4));
                const bool changed=std::memcmp(&output.local[i],&frame->local[i],32)!=0;C(bool(output.changed_mask[i/64]&(std::uint64_t(1)<<(i%64)))==changed);
                if(i){C(!std::memcmp(output.local[i].translation,frame->local[i].translation,12));const auto& t=frame->local[i].translation;
                    const auto delta=std::abs(error(world[i].position,world[unsigned(model.parents[i])].position)-std::hypot(t[0],t[1],t[2]));lengths=(std::max)(lengths,delta);C(delta<.002);}}
            maximum_head=(std::max)(maximum_head,error(world[b::head].position,goal.head_world.position));maximum_feet=(std::max)(maximum_feet,feet);maximum_lengths=(std::max)(maximum_lengths,lengths);
            if(require_solution){const auto expected=gears_vr::multiply(b::d::yaw(goal.body_yaw_delta),root_original.orientation);C(b::d::angle(world[b::root].orientation,expected)<1e-4);C(error(world[b::pelvis].position,pivot)<.002);maximum_pelvis=(std::max)(maximum_pelvis,error(world[b::pelvis].position,pivot));}
            const auto namebase=std::string(argv[2])+"/heading-"+std::to_string(new_plans)+".local32.bin";std::ofstream atoms(namebase,std::ios::binary);atoms.write(reinterpret_cast<char const*>(output.local.data()),sizeof output.local);C(bool(atoms));
        }
        std::printf("heading %s yaw=%.17g status=%u head=%g feet=%g,%g length=%g projection=%g distance=%g reach=%g\n",name,goal.body_yaw_delta,unsigned(report.status),report.head_error,report.foot_error[0],report.foot_error[1],report.max_length_error,report.pelvis_projection,report.requested_distance,report.available_reach);
        rows<<"{\"case\":\""<<name<<"\",\"yaw\":"<<goal.body_yaw_delta<<",\"status\":"<<unsigned(report.status)<<",\"solved\":"<<(solved?"true":"false")<<",\"output_unchanged\":"<<(untouched?"true":"false")<<",\"head_error\":"<<report.head_error<<",\"foot_error\":["<<report.foot_error[0]<<','<<report.foot_error[1]<<"],\"length_error\":"<<report.max_length_error<<",\"pelvis_projection\":"<<report.pelvis_projection<<",\"requested_distance\":"<<report.requested_distance<<",\"available_reach\":"<<report.available_reach<<",\"foot_anchors_explicitly_rotated\":"<<(require_solution?"true":"false")<<",\"native_permission\":false}\n";
        return solved;
    };
    const double angles[]={b::max_body_yaw_delta/2,-b::max_body_yaw_delta/2,b::max_body_yaw_delta,-b::max_body_yaw_delta,b::max_body_yaw_delta-1e-6,-b::max_body_yaw_delta+1e-6};
    for(double yaw:angles){auto goal=base;goal.body_yaw_delta=yaw;goal.head_world=turn(base.head_world,yaw);for(unsigned s=0;s<2;++s)goal.feet_world[s]=turn(base.feet_world[s],yaw);evaluate(goal,"explicit-rotated-foot-anchors",true);}
    // Same heading/head target with ORIGINAL explicit feet: do not move or
    // replace either foot. Report actual geometric acceptance/rejection.
    for(double yaw:angles){auto goal=base;goal.body_yaw_delta=yaw;goal.head_world=turn(base.head_world,yaw);evaluate(goal,"original-fixed-foot-anchors",false);}
    C(fixed_solutions==1&&fixed_rejections==5);
    for(double yaw:angles){auto goal=base;goal.head_world.orientation=gears_vr::multiply(b::d::yaw(yaw),base.head_world.orientation);const auto r=b::plan(model,*frame,goal,*work,output);
        C(r.status==b::Status::solved);C(!std::memcmp(&output.local[b::root],&identity.local[b::root],32));C(!std::memcmp(&output.local[b::pelvis],&identity.local[b::pelvis],32));
        C(output.changed_mask[0]==(std::uint64_t(1)<<b::head)&&output.changed_mask[1]==0);C(error(output.pelvis_world.position,pivot)<1e-6);
        for(unsigned i=0;i<b::bones;++i)C(!std::memcmp(&output.local[i].scale,&frame->local[i].scale,4)&&(!i||!std::memcmp(output.local[i].translation,frame->local[i].translation,12)));}
    for(double yaw:{b::max_body_yaw_delta+1e-8,-b::max_body_yaw_delta-1e-8,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){
        auto goal=base;goal.body_yaw_delta=yaw;const auto saved=output;C(b::plan(model,*frame,goal,*work,output).status==b::Status::goal);C(!std::memcmp(&saved,&output,sizeof output));}
    C(bool(rows));rows.close();
    std::printf("{\"legacy_checks\":%u,\"heading_checks\":%u,\"new_solved_plans\":%u,\"fixed_foot_solutions\":%u,\"fixed_foot_rejections\":%u,\"max_head\":%.17g,\"max_feet\":%.17g,\"max_length\":%.17g,\"max_pelvis_rotated_anchors\":%.17g}\n",legacy_checks,checks-legacy_checks,new_plans,fixed_solutions,fixed_rejections,maximum_head,maximum_feet,maximum_lengths,maximum_pelvis);
    std::printf("full_body_c0_heading: %u checks, %u failures; Game=0 writer=0 lengths_fixed=1\n",checks,failures);return failures?1:0;
}
