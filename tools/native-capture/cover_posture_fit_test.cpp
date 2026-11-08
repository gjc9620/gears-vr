#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include "cover_posture_fit.h"
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
namespace fit=gears_cover_posture_fit;
namespace body=fit::body;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %d %s\n",__LINE__,#x);}}while(0)
bool read(std::string const& name,void* data,size_t size){std::ifstream f(name,std::ios::binary);return bool(f.read(static_cast<char*>(data),std::streamsize(size)))&&f.peek()==EOF;}
bool hash(void const* data,size_t size,body::rig::Signature& out){
    BCRYPT_ALG_HANDLE alg{};BCRYPT_HASH_HANDLE h{};bool okay=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(okay)okay=BCryptCreateHash(alg,&h,nullptr,0,nullptr,0,0)>=0;
    if(okay)okay=BCryptHashData(h,reinterpret_cast<PUCHAR>(const_cast<void*>(data)),ULONG(size),0)>=0;
    if(okay)okay=BCryptFinishHash(h,out.data(),ULONG(out.size()),0)>=0;
    if(h)BCryptDestroyHash(h);if(alg)BCryptCloseAlgorithmProvider(alg,0);return okay;
}
int main(int argc,char**argv){
    if(argc!=2)return 2;const std::string folder=argv[1];
    auto frame=std::make_unique<body::Frame>();auto model=std::make_unique<body::Model>();
    auto work=std::make_unique<fit::Workspace>();body::Goal goal{};
    C(read(folder+"/reference.bin",frame->reference.data(),sizeof frame->reference));
    C(read(folder+"/C0-local.bin",frame->local.data(),sizeof frame->local));
    C(read(folder+"/C0-component.bin",&frame->component_to_world,sizeof frame->component_to_world));
    C(read(folder+"/fitting-goal.bin",&goal,sizeof goal));
    C(body::prepare_model(frame->reference,hash,*model));if(failures)return 1;
    const auto original=*frame;const auto neutral_goal=goal;const auto measured=goal.absolute.neutral_pelvis_world;
    const auto prior=body::plan(*model,*frame,goal,work->body_work,work->planned);
    C(prior.status==body::Status::spine_unreachable);
    fit::Result result{};double first_shift{};
    for(auto posture:{body::Posture::preserve_c0,body::Posture::reference_neutral}){
        auto candidate=goal;candidate.posture=posture;
        const auto status=fit::fit(*model,*frame,candidate,measured,{.25},*work,result);
        if(status!=fit::Status::okay)std::printf("fit posture=%u status=%u\n",unsigned(posture),unsigned(status));
        C(status==fit::Status::okay);
        if(status==fit::Status::okay){
            C(result.validation.status==body::Status::solved);
            C(result.horizontal_units>0&&result.horizontal_units<=25.0005);
            C(result.goal.absolute.neutral_pelvis_world.z==measured.z);
            C(std::abs(result.actual_pelvis.z-measured.z)<=.0005);
            C(!memcmp(&result.goal.head_world,&goal.head_world,sizeof goal.head_world));
            C(!memcmp(result.goal.feet_world,goal.feet_world,sizeof goal.feet_world));
            C(result.validation.head_error<.001&&result.validation.foot_error[0]<.001&&result.validation.foot_error[1]<.001);
            C(result.iterations<=128);
            if(posture==body::Posture::preserve_c0)first_shift=result.horizontal_units;
            std::printf("fit posture=%u shift=%.9g height=%.9g iterations=%u\n",unsigned(posture),result.horizontal_units,result.actual_pelvis.z,result.iterations);
        }
    }
    for(double heading:{1.5707963267948966,3.1415926535897932,-1.5707963267948966}){
        auto moved=*frame;auto turned=goal;const auto q=body::d::yaw(heading);
        body::Atom atom{};body::d::set_quat(atom,q);atom.scale=1;
        atom.translation[0]=100;atom.translation[1]=-200;atom.translation[2]=20;
        const auto transform=body::d::atom_matrix(atom);
        body::ik::Matrix current{};for(unsigned i=0;i<16;++i)current.m[i]=frame->component_to_world.values[i];
        const auto matrix=body::d::multiply(current,transform);
        for(unsigned i=0;i<16;++i)moved.component_to_world.values[i]=float(matrix.m[i]);
        const auto point=[&](body::Vec p){return body::d::add(body::d::rotate(q,p),body::Vec{100,-200,20});};
        const auto pose=[&](body::Pose p){return body::Pose{point(p.position),body::d::product(q,p.orientation)};};
        turned.head_world=pose(goal.head_world);
        for(unsigned i=0;i<2;++i)turned.feet_world[i]=pose(goal.feet_world[i]);
        turned.absolute.root_orientation.value=body::d::product(q,goal.absolute.root_orientation.value);
        turned.absolute.neutral_pelvis_world=point(measured);
        C(fit::fit(*model,moved,turned,point(measured),{.25},*work,result)==fit::Status::okay);
        C(std::abs(result.horizontal_units-first_shift)<.005);
        C(std::abs(result.actual_pelvis.z-point(measured).z)<.0005);
        C(!memcmp(result.goal.feet_world,turned.feet_world,sizeof turned.feet_world));
    }
    for(unsigned bad=0;bad<9;++bad){
        auto changed=goal;auto source=*frame;auto preferred=measured;fit::Limits limits{.25};
        if(bad==0)limits.horizontal_metres=0;
        if(bad==1)limits.horizontal_metres=.001;
        if(bad==2)changed.physical_world.x=.01;
        if(bad==3)changed.committed_grip_world.z=.01;
        if(bad==4)preferred.z=changed.feet_world[0].position.z-1;
        if(bad==5)source.model_scale=.5;
        if(bad==6)source.count=114;
        if(bad==7)changed.scope=body::Scope::stationary;
        if(bad==8)limits.horizontal_metres=2;
        result.horizontal_units=-123;const auto saved=result;
        C(fit::fit(*model,source,changed,preferred,limits,*work,result)!=fit::Status::okay);
        C(!memcmp(&saved,&result,sizeof result));
    }
    // Already feasible measured C0 stays exactly in place even with zero budget.
    auto& fk=work->body_work;
    for(unsigned i=0;i<16;++i)fk.component.m[i]=frame->component_to_world.values[i];
    std::copy(frame->local.begin(),frame->local.end(),fk.pose.local);C(body::d::rebuild(*model,fk));
    body::Goal identity{};identity.scope=body::Scope::fixed_cover;
    identity.target_mode=body::BodyTargetMode::world_absolute;
    identity.absolute.neutral_pelvis_world=body::d::position(fk.pose.world[body::pelvis]);
    identity.absolute.root_orientation.value=body::d::rotation(fk.pose.world[body::root]);
    identity.head_world=body::d::pose(fk.pose.world[body::head]);
    for(unsigned i=0;i<2;++i)identity.feet_world[i]=body::d::pose(fk.pose.world[body::legs[i][2]]);
    C(fit::fit(*model,*frame,identity,identity.absolute.neutral_pelvis_world,{0},*work,result)==fit::Status::okay);
    C(result.horizontal_units==0&&result.iterations==0);
    result.goal=goal;const auto before_alias=result;
    C(fit::fit(*model,*frame,result.goal,measured,{.25},*work,result)==fit::Status::alias);
    C(!memcmp(&before_alias,&result,sizeof result));
    C(!memcmp(&original,frame.get(),sizeof original)&&!memcmp(&neutral_goal,&goal,sizeof goal));
    std::printf("cover_posture_fit: %u checks, %u failures; retained61492 geometry, neutral goal fixture; no Game/physics permission\n",checks,failures);
    return failures?1:0;
}
