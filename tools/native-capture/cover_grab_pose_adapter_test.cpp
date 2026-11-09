#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include "cover_grab_pose_adapter.h"
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
namespace a=gears_cover_grab_pose_adapter;namespace b=a::body;namespace g=a::grab;namespace v=gears_vr;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %d: %s\n",__LINE__,#x);}}while(0)
bool read(std::string const& p,void* out,size_t n){std::ifstream f(p,std::ios::binary);return bool(f.read(static_cast<char*>(out),std::streamsize(n)))&&f.peek()==EOF;}
bool digest(void const* p,size_t n,b::rig::Signature& out){BCRYPT_ALG_HANDLE alg{};BCRYPT_HASH_HANDLE h{};
    bool ok=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(ok)ok=BCryptCreateHash(alg,&h,nullptr,0,nullptr,0,0)>=0;
    if(ok)ok=BCryptHashData(h,reinterpret_cast<PUCHAR>(const_cast<void*>(p)),ULONG(n),0)>=0;
    if(ok)ok=BCryptFinishHash(h,out.data(),ULONG(out.size()),0)>=0;
    if(h)BCryptDestroyHash(h);if(alg)BCryptCloseAlgorithmProvider(alg,0);return ok;}
struct Fixture {
    b::Model model{};b::Frame frame{};b::Output body{};b::Workspace fk{};a::Workspace work{};
    g::Frame input{};g::Contact contact{};g::Profile profile{};
    std::array<std::uint64_t,2> available{{UINT64_MAX,(std::uint64_t(1)<<(b::bones-64))-1}};
    bool initialize(std::string const& path){
        if(!read(path+"/reference.bin",frame.reference.data(),sizeof frame.reference)||
           !read(path+"/C0-local.bin",frame.local.data(),sizeof frame.local)||
           !read(path+"/C0-component.bin",&frame.component_to_world,sizeof frame.component_to_world)||
           !b::prepare_model(frame.reference,digest,model))return false;
        for(unsigned i=0;i<16;++i)fk.component.m[i]=frame.component_to_world.values[i];
        std::copy(frame.local.begin(),frame.local.end(),fk.pose.local);if(!b::d::rebuild(model,fk))return false;
        body.local=frame.local;body.root_world=b::d::pose(fk.pose.world[b::root]);
        body.pelvis_world=b::d::pose(fk.pose.world[b::pelvis]);body.head_world=b::d::pose(fk.pose.world[b::head]);
        for(unsigned i=0;i<2;++i){body.feet_world[i]=b::d::pose(fk.pose.world[b::legs[i][2]]);body.shoulders_world[i]=b::d::pose(fk.pose.world[b::shoulders[i]]);}
        profile.id=23;profile.kind=v::CoverEdgeKind::left;profile.geometry_signature=model.signature;
        profile.acquire_metres=.1;profile.retain_metres=.16;profile.maximum_wrist_turn=1.;
        for(unsigned n=0;n<15;++n){const auto& q=model.reference[12+n].q;
            profile.approach[n]={q[0],q[1],q[2],q[3]};
            profile.held[n]=v::normalized(v::multiply(profile.approach[n],{0,0,-std::sin(.05),std::cos(.05)}));}
        input.expected={{1,2,3,4,5,6,7},42,200,3,profile.id};contact.key=input.expected;
        contact.observed_ns=990000000;input.now_ns=1000000000;input.model_scale=1;input.units_per_metre=100;
        input.actual_geometry_signature=model.signature;input.mode=g::Mode::held;
        input.cover.sequence=3;input.cover.observed_ns=990000000;input.cover.epochs=input.expected.epochs;
        input.cover.cover_valid=input.cover.interaction_enabled=true;input.cover.edge={42,profile.kind,true,true};
        input.hand.sequence=4;input.hand.observed_ns=990000000;input.hand.session_epoch=5;input.hand.space_epoch=6;
        input.hand.input_active=input.hand.position_valid=input.hand.position_tracked=input.hand.lgrip_down=true;
        input.ownership={v::CoverLeftHandOwner::cover,9};input.interaction.offset_certified=input.interaction.owns_left_hand=true;
        input.interaction.phase=v::CoverPhase::grabbing;input.interaction.request_id=9;input.interaction.edge_id=42;
        target(b::d::pose(fk.pose.world[11]));return true;
    }
    void target(v::Pose pose){contact.edge_world=pose;g::mount::Mount m{};
        C(g::mount::marcus115(model.signature,g::mount::Side::left,1,100,m));
        const auto q=v::multiply(pose.orientation,v::inverse(m.calibration.grip_to_wrist_rotation));
        const auto d=v::transform(v::rotation_matrix(q),m.calibration.grip_to_wrist_metres);
        input.physical_grip_world={{pose.position.x-d.x*100,pose.position.y-d.y*100,pose.position.z-d.z*100},q};}
    a::Result run(){return a::compose(model,frame,input,contact,profile,available,work,body);}
};
int main(int argc,char** argv){if(argc!=2)return 2;const std::string path=argv[1];
    for(auto kind:{v::CoverEdgeKind::left,v::CoverEdgeKind::right,v::CoverEdgeKind::top}){
        auto x=std::make_unique<Fixture>();C(x->initialize(path));x->profile.kind=kind;x->input.cover.edge.kind=kind;
        auto target=x->contact.edge_world;
        target.position.x+=kind==v::CoverEdgeKind::left?.75:0;
        target.position.y+=kind==v::CoverEdgeKind::right?-.75:0;
        target.position.z+=kind==v::CoverEdgeKind::top?1.:0;
        target.orientation=v::normalized(v::multiply(target.orientation,{0,0,std::sin(.025),std::cos(.025)}));
        x->target(target);
        const auto before=x->body;C(x->run().status==a::Status::prepared);
        for(unsigned i=0;i<b::bones;++i){C(!std::memcmp(before.local[i].translation,x->body.local[i].translation,12));
            C(before.local[i].scale==x->body.local[i].scale);
            if(i!=8&&i!=9&&i!=10&&i!=11&&(i<12||i>=27))C(!std::memcmp(&before.local[i],&x->body.local[i],sizeof(b::Atom)));}
        for(unsigned n=0;n<15;++n){const auto& q=x->body.local[12+n].q;
            C(b::d::angle({q[0],q[1],q[2],q[3]},x->profile.held[n])<1e-4);}
        // Quaternion/vector FK independent of the adapter's Matrix rebuild.
        std::array<v::Pose,b::bones> world{};
        const v::Pose component{b::d::position(x->fk.component),b::d::rotation(x->fk.component)};
        for(unsigned i=0;i<b::bones;++i){const auto p=i?world[unsigned(x->model.parents[i])]:component;
            const auto& local=x->body.local[i];const auto d=v::transform(v::rotation_matrix(p.orientation),
                {local.translation[0],local.translation[1],local.translation[2]});
            world[i]={{p.position.x+d.x,p.position.y+d.y,p.position.z+d.z},
                v::normalized(v::multiply(p.orientation,{local.q[0],local.q[1],local.q[2],local.q[3]}))};}
        C(b::d::same_position(world[11].position,target.position,.002));
        C(b::d::angle(world[11].orientation,target.orientation)<1e-4);
        C(x->body.report.max_length_error<=.002);
        // Release is inactive and leaves this owned candidate unchanged. The
        // current physical solve/release blend remains the caller's job.
        const auto held=x->body;x->input.hand.lgrip_down=false;
        C(x->run().status==a::Status::inactive);C(!std::memcmp(&held,&x->body,sizeof held));
    }
    for(unsigned defect=0;defect<8;++defect){auto x=std::make_unique<Fixture>();C(x->initialize(path));
        switch(defect){case 0:x->input.ownership.owner=v::CoverLeftHandOwner::other;break;
        case 1:x->contact.key.shape_generation++;break;case 2:x->input.hand.observed_ns=1;break;
        case 3:x->body.head_world.position.z+=1;break;case 4:x->frame.reference[100].bytes[12]^=1;break;
        case 5:x->body.report.status=b::Status::residual;break;case 6:x->profile.held[3]={0,0,0,0};break;
        default:{auto p=x->contact.edge_world;p.position.x+=200;x->target(p);break;}}
        const auto before=x->body;const auto result=x->run();C(result.status!=a::Status::prepared);
        if(defect==7)C(result.status==a::Status::unreachable);
        C(!std::memcmp(&before,&x->body,sizeof before));
    }
    // Every written joint, its root/torso ancestors, and an empty or malformed
    // snapshot must reject without partially replacing the candidate.
    for(unsigned bone=0;bone<27;++bone){auto x=std::make_unique<Fixture>();C(x->initialize(path));
        x->available[bone/64]&=~(std::uint64_t(1)<<(bone%64));const auto before=x->body;
        // Some reference bones in0..7 are siblings, not left-arm ancestors.
        bool required=bone>=8;
        for(unsigned at=8;at;at=unsigned(x->model.parents[at]))required=required||at==bone;
        required=required||bone==0;
        C(x->run().status==(required?a::Status::lod:a::Status::prepared));
        if(required)C(!std::memcmp(&before,&x->body,sizeof before));
    }
    for(unsigned defect=0;defect<2;++defect){auto x=std::make_unique<Fixture>();C(x->initialize(path));
        if(defect)x->available[1]|=std::uint64_t(1)<<63;else x->available={};
        const auto before=x->body;C(x->run().status==a::Status::lod);C(!std::memcmp(&before,&x->body,sizeof before));}
    {auto x=std::make_unique<Fixture>();C(x->initialize(path));
        const auto baseline=x->body;gears_cover_grab_transition::State state{};
        const auto delta=b::d::sub(baseline.shoulders_world[0].position,x->contact.edge_world.position);
        const double distance=b::d::length(delta);
        x->profile.wrist_in_edge.position=v::transform(v::transpose(v::rotation_matrix(x->contact.edge_world.orientation)),
            {delta.x/distance,delta.y/distance,delta.z/distance});
        const auto step=[&](){x->body=baseline;
            x->input.cover.observed_ns=x->input.hand.observed_ns=x->contact.observed_ns=x->input.now_ns;
            return a::compose_transition(x->model,x->frame,x->input,x->contact,x->profile,x->available,state,x->work,x->body);};
        for(unsigned n=0;n<4;++n){C(step().status==a::Status::prepared);x->input.now_ns+=40000000;}
        C(state.weight==1);C(x->body.report.max_length_error<=.002);
        x->input.mode=g::Mode::release;x->input.hand.lgrip_down=false;x->input.ownership={};
        x->input.interaction.owns_left_hand=false;
        for(unsigned n=0;n<12;++n){const auto r=step();C(r.status==a::Status::prepared||r.status==a::Status::inactive);x->input.now_ns+=40000000;}
        C(!state.known);C(!std::memcmp(&baseline,&x->body,sizeof baseline));
        x->input.mode=g::Mode::held;x->input.hand.lgrip_down=true;x->input.ownership={v::CoverLeftHandOwner::cover,9};x->input.interaction.owns_left_hand=true;
        C(step().status==a::Status::prepared);x->input.now_ns+=40000000;
        C(step().status==a::Status::prepared);C(state.weight>0);
        x->available={};x->input.now_ns+=40000000;
        C(step().status==a::Status::lod);C(!state.known);C(!std::memcmp(&baseline,&x->body,sizeof baseline));
    }
    std::printf("CoverGrabPoseAdapterTest checks=%u failures=%u\n",checks,failures);return failures?1:0;
}
