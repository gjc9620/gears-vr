// Saved actual r276 source/plan/original-return Local+SpaceBases. Real
// Backend/CAS/Kernel; fresh native owner is a Host fixture, not current game.
#define main ExistingNativeBackendMain
#include "full_body_c0_native_backend_test.cpp"
#undef main
int main(int argc,char** argv){
    if(argc!=2)return 2;std::string directory=argv[1];auto source=std::make_unique<body::Frame>();
    auto plan=std::make_unique<body::Output>();auto actual=std::make_unique<n::ConsumedPose>();
    C(read_file(directory+"/cover-first-consumption-source.bin",source.get(),sizeof *source));
    C(read_file(directory+"/cover-first-consumption-output.bin",plan.get(),sizeof *plan));
    C(read_file(directory+"/cover-first-consumption-observed.bin",actual.get(),sizeof *actual));
    if(failures)return 1;
    for(unsigned defect=0;defect<9;++defect){
        Fixture f(*source);tx::Kernel k;tx::Lease l;C(f.capture());const auto key=f.backend.stamp();auto candidate=*plan;
        if(defect==1||defect==2){const unsigned bone=defect==1?8u:32u;
            for(auto& q:candidate.local[bone].q)q=-q;candidate.changed_mask[bone/64]|=uint64_t(1)<<(bone%64);
        } // Equivalent rotation, but now actually C0-written; original cannot replace it.
        C(k.prepare(f.backend,key,f.captured,candidate,f.desired_copy,l).code==tx::Code::prepared);
        C(k.commit(f.backend,l,key).code==tx::Code::committed);const auto serial=k.serial();
        C(k.notify_consumer_entry(f.backend,l,key).code==tx::Code::consumer_marked);
        std::memcpy(f.local.data,actual->local.data(),sizeof actual->local);
        std::memcpy(f.space.data,actual->space.data(),sizeof actual->space);
        C(k.finish(f.backend,l,key,tx::End::original_returned).code==tx::Code::scope_closed);
        C(k.completed_consumer(f.backend,key,serial));auto read=std::make_unique<n::ConsumedPose>();read->observed_qpc=123;
        if(defect==3)reinterpret_cast<body::Matrix64*>(f.space.data)[11].values[15]=1.00001f;
        if(defect==4)reinterpret_cast<body::Matrix64*>(f.space.data)[11].values[3]=1e-8f;
        if(defect==5)reinterpret_cast<body::Matrix64*>(f.space.data)[11].values[15]=NAN;
        if(defect==6){auto& head=f.atoms()[55];body::Quat q{head.q[0],head.q[1],head.q[2],head.q[3]};
            C(body::d::set_quat(head,body::d::product(q,body::d::yaw(.02))));f.original_space();}
        if(defect==7){auto& foot=f.atoms()[93];body::Quat q{foot.q[0],foot.q[1],foot.q[2],foot.q[3]};
            C(body::d::set_quat(foot,body::d::product(q,body::d::yaw(.02))));f.original_space();}
        if(defect==8){f.atoms()[8].translation[0]+=.01f;f.original_space();}
        const auto before=*read;
        if(!defect){
            C(f.backend.read_consumed_pose(k,key,serial,*read)==n::ConsumedPoseCode::target);
            C(!std::memcmp(read.get(),&before,sizeof before)); // all-planned remains strict.
        }
        const auto code=f.backend.read_consumed_pose(k,key,serial,*read,n::ConsumedPosePolicy::cover_unowned_shoulders);
        printf("recorded actual body defect=%u read=%u\n",defect,unsigned(code));
        if(!defect){C(code==n::ConsumedPoseCode::verified);
            C(!std::memcmp(read->space.data(),actual->space.data(),sizeof actual->space));
            C(!std::memcmp(read->local.data(),actual->local.data(),sizeof actual->local));
            C(read->max_position_error<.002&&read->max_angle_error<1e-4);
        }else{C(code!=n::ConsumedPoseCode::verified);C(!std::memcmp(read.get(),&before,sizeof before));}
    }
    printf("CoverRecordedConsumption checks=%u failures=%u; real saved geometry, Host identity; Game=0\n",checks,failures);
    return failures?1:0;
}
