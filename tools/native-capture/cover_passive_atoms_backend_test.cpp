// Actual recorded r272 atoms, real Native Backend/CAS/Kernel/readback.
// Owner and original Compose math are Host fixtures, no game permission.
#define main ExistingNativeBackendMain
#include "full_body_c0_native_backend_test.cpp"
#undef main
#include "cover_observed_passive_atoms_test.inl"
int main(int argc,char**argv){
    if(argc!=2)return 2;std::string directory=argv[1];auto frame=std::make_unique<body::Frame>();
    auto work=std::make_unique<body::Workspace>();body::Model model{};body::Output output{};
    C(read_file(directory+"/reference.bin",frame->reference.data(),sizeof frame->reference));
    C(read_file(directory+"/C0-local.bin",frame->local.data(),sizeof frame->local));
    C(read_file(directory+"/C0-component.bin",&frame->component_to_world,sizeof frame->component_to_world));
    recorded_passive_atoms(*frame);C(body::prepare_model(frame->reference,hash,model));
    for(unsigned i=0;i<16;++i)work->component.m[i]=frame->component_to_world.values[i];
    std::copy(frame->local.begin(),frame->local.end(),work->pose.local);C(body::d::rebuild(model,*work));
    body::Goal goal{};goal.scope=body::Scope::fixed_cover;goal.body_yaw_delta=-.1;
    goal.physical_world={1,2,-3};goal.committed_grip_world={2,1,-2};
    goal.head_world=body::d::pose(work->pose.world[55]);goal.head_world.position=body::d::add(goal.head_world.position,{3,3,-5});
    for(unsigned i=0;i<2;++i)goal.feet_world[i]=body::d::pose(work->pose.world[body::legs[i][2]]);
    C(body::plan(model,*frame,goal,*work,output).status==body::Status::solved);
    for(unsigned i:{65u,67u,106u})C(!std::memcmp(&output.local[i],&frame->local[i],sizeof(body::Atom)));
    const auto solved=output;
    // Standing retains its existing unit-quaternion policy.
    auto standing=goal;standing.scope=body::Scope::stationary;
    C(body::plan(model,*frame,standing,*work,output).status==body::Status::atom);
    C(!std::memcmp(&output,&solved,sizeof output));
    auto broken=std::make_unique<body::Frame>(*frame);broken->local[55].q[0]=broken->local[55].q[1]=broken->local[55].q[2]=0;broken->local[55].q[3]=.707106769f;
    C(body::plan(model,*broken,goal,*work,output).status==body::Status::atom);
    C(!std::memcmp(&output,&solved,sizeof output));
    *broken=*frame;broken->local[65].q[0]=.2f;
    C(body::plan(model,*broken,goal,*work,output).status==body::Status::atom);
    C(!gears_cover_passive_atoms::auxiliary(65,55,frame->local[65]));
    auto invalid=frame->local[65];invalid.translation[0]=NAN;C(!gears_cover_passive_atoms::auxiliary(65,56,invalid));
    invalid=frame->local[65];invalid.scale=2;C(!gears_cover_passive_atoms::auxiliary(65,56,invalid));
    // Even equivalent-matrix normalization and scalar-only edits are forbidden.
    for(unsigned defect=0;defect<4;++defect){Fixture f(*frame);tx::Kernel k;tx::Lease l;C(f.capture());auto edited=solved;
        if(defect==0){edited.local[65].q[3]=1;edited.changed_mask[1]|=uint64_t(1)<<1;}
        if(defect==1){edited.local[65].q[3]=.8f;edited.changed_mask[1]|=uint64_t(1)<<1;}
        if(defect==2){edited.local[106].translation[0]+=.01f;edited.changed_mask[1]|=uint64_t(1)<<42;}
        if(defect==3){edited.local[67].scale=1.01f;edited.changed_mask[1]|=uint64_t(1)<<3;}
        C(k.prepare(f.backend,f.backend.stamp(),f.captured,edited,f.desired_copy,l).code==tx::Code::invalid_plan);
        C(!std::memcmp(f.local.data,frame->local.data(),sizeof frame->local));C(k.outstanding_fields()==0);
    }
    for(bool tamper:{false,true}){Fixture f(*frame);tx::Kernel k;tx::Lease l;C(f.capture());const auto key=f.backend.stamp();
        C(k.prepare(f.backend,key,f.captured,solved,f.desired_copy,l).code==tx::Code::prepared);
        C(k.commit(f.backend,l,key).code==tx::Code::committed);const auto serial=k.serial();
        C(k.notify_consumer_entry(f.backend,l,key).code==tx::Code::consumer_marked);f.original_space();
        C(k.finish(f.backend,l,key,tx::End::original_returned).code==tx::Code::scope_closed);
        C(k.completed_consumer(f.backend,key,serial));
        if(tamper){f.atoms()[106].q[3]=.8f;f.original_space();}
        auto observed=std::make_unique<n::ConsumedPose>();observed->observed_qpc=123;const auto before=*observed;
        const auto code=f.backend.read_consumed_pose(k,key,serial,*observed);
        C(code==(tamper?n::ConsumedPoseCode::geometry:n::ConsumedPoseCode::verified));
        if(tamper)C(!std::memcmp(observed.get(),&before,sizeof before));
        else for(unsigned i:{65u,67u,106u})C(!std::memcmp(&observed->local[i],&frame->local[i],sizeof(body::Atom)));
    }
    printf("CoverPassiveAtomsBackend checks=%u failures=%u; recorded atoms, real Kernel/readback, Host owner/original math; Game=0\n",checks,failures);
    return failures?1:0;
}
