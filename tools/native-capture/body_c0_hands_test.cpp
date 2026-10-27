#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include "body_c0_hands.h"
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
namespace a=gears_body_c0_hands;namespace b=a::body;namespace v=gears_vr;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %d: %s\n",__LINE__,#x);}}while(0)
bool read(std::string const& path,void* out,size_t size){std::ifstream f(path,std::ios::binary);return bool(f.read(static_cast<char*>(out),std::streamsize(size)))&&f.peek()==EOF;}
bool digest(void const* bytes,size_t size,b::rig::Signature& out){BCRYPT_ALG_HANDLE alg{};BCRYPT_HASH_HANDLE hash{};
    bool ok=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;if(ok)ok=BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0;
    if(ok)ok=BCryptHashData(hash,reinterpret_cast<PUCHAR>(const_cast<void*>(bytes)),ULONG(size),0)>=0;if(ok)ok=BCryptFinishHash(hash,out.data(),ULONG(out.size()),0)>=0;
    if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);return ok;}
template<class T>auto bytes(T const& x){std::array<unsigned char,sizeof(T)> r{};std::memcpy(r.data(),&x,sizeof x);return r;}
v::Quaternion mul(v::Quaternion p,v::Quaternion q){return {p.w*q.x+p.x*q.w+p.y*q.z-p.z*q.y,p.w*q.y-p.x*q.z+p.y*q.w+p.z*q.x,p.w*q.z+p.x*q.y-p.y*q.x+p.z*q.w,p.w*q.w-p.x*q.x-p.y*q.y-p.z*q.z};}
v::Quaternion inv(v::Quaternion q){return {-q.x,-q.y,-q.z,q.w};}
v::Vector add(v::Vector x,v::Vector y){return {x.x+y.x,x.y+y.y,x.z+y.z};}
v::Vector sub(v::Vector x,v::Vector y){return {x.x-y.x,x.y-y.y,x.z-y.z};}
v::Vector scale(v::Vector x,double k){return {x.x*k,x.y*k,x.z*k};}
v::Vector rotate(v::Quaternion q,v::Vector x){const auto r=mul(mul(q,{x.x,x.y,x.z,0}),inv(q));return {r.x,r.y,r.z};}
double distance(v::Vector x,v::Vector y){const auto d=sub(x,y);return std::hypot(d.x,d.y,d.z);}
double qerror(v::Quaternion x,v::Quaternion y){const double sign=x.x*y.x+x.y*y.y+x.z*y.z+x.w*y.w<0?-1.:1.;return std::abs(x.x-sign*y.x)+std::abs(x.y-sign*y.y)+std::abs(x.z-sign*y.z)+std::abs(x.w-sign*y.w);}
v::Quaternion turn(unsigned axis,double radians){v::Quaternion q{};q.w=std::cos(radians/2);if(axis==0)q.x=std::sin(radians/2);if(axis==1)q.y=std::sin(radians/2);if(axis==2)q.z=std::sin(radians/2);return q;}
// Independent quaternion/vector FK oracle, not the composition Matrix rebuild.
std::array<b::Pose,b::bones> fk(b::Model const& model,b::Frame const& frame,b::Output const& out){
    b::ik::Matrix component{};for(unsigned i=0;i<16;++i)component.m[i]=frame.component_to_world.values[i];
    const b::Pose origin{b::d::position(component),b::d::rotation(component)};std::array<b::Pose,b::bones> world{};
    for(unsigned i=0;i<b::bones;++i){auto const& parent=i?world[unsigned(model.parents[i])]:origin;auto const& local=out.local[i];
        world[i]={add(parent.position,rotate(parent.orientation,{local.translation[0],local.translation[1],local.translation[2]})),
            v::normalized(mul(parent.orientation,{local.q[0],local.q[1],local.q[2],local.q[3]}))};}
    return world;
}
// Explicit HOST INPUT GENERATOR: invert the existing authored mount so the
// synthetic Core world grip asks for a chosen reachable wrist. This is neither
// runtime tracking evidence nor production animated-wrist calibration.
v::Pose synthetic_grip(v::Pose wrist,a::mount::Mount const& mount,double units){
    const auto q=mul(wrist.orientation,inv(mount.calibration.grip_to_wrist_rotation));
    return {sub(wrist.position,rotate(q,scale(mount.calibration.grip_to_wrist_metres,units))),q};
}
int main(int argc,char** argv){
    if(argc!=3)return 2;const std::string fixtures=argv[1];
    auto frame=std::make_unique<b::Frame>();auto model=std::make_unique<b::Model>();auto bw=std::make_unique<b::Workspace>();auto work=std::make_unique<a::Workspace>();
    a::core::CoreSnapshot actual{};C(read(argv[2],&actual,sizeof actual));
    C(read(fixtures+"/reference.bin",frame->reference.data(),sizeof frame->reference));C(read(fixtures+"/C0-local.bin",frame->local.data(),sizeof frame->local));
    C(read(fixtures+"/C0-component.bin",&frame->component_to_world,sizeof frame->component_to_world));frame->units_per_metre=actual.units_per_metre;
    C(b::prepare_model(frame->reference,digest,*model));
    for(unsigned i=0;i<16;++i)bw->component.m[i]=frame->component_to_world.values[i];std::copy(frame->local.begin(),frame->local.end(),bw->pose.local);C(b::d::rebuild(*model,*bw));
    b::Goal goal{};goal.head_world=b::d::pose(bw->pose.world[b::head]);for(unsigned side=0;side<2;++side)goal.feet_world[side]=b::d::pose(bw->pose.world[b::legs[side][2]]);
    goal.body_yaw_delta=-.1;goal.physical_world={1,2,-3};goal.committed_grip_world={2,1,-2};goal.head_world.position=add(goal.head_world.position,{3,3,-5});
    b::Output body_only{};C(b::plan(*model,*frame,goal,*bw,body_only).status==b::Status::solved);
    const auto before=fk(*model,*frame,body_only);const auto model_bytes=bytes(*model);const auto frame_bytes=bytes(*frame);const auto original=bytes(body_only);
    const a::hand::ObjectIdentity asset{0x70000,0x71000,0x72000,18,19,20,1};
    const auto now=static_cast<std::int64_t>(actual.published_ns);const bool sim=actual.stereo.source==v::StereoSource::MetaOperatorSimulated;
    C(a::core::core_snapshot_valid(actual,now,50000000,sim));
    const v::StereoMatrix base=actual.base_view;
    C(model->parents[10]==8&&model->parents[11]==10&&model->parents[34]==32&&model->parents[35]==34);
    C(model->parents[51]==35&&model->parents[52]==51);
    a::core::CoreSnapshot source=actual;
    std::array<v::Pose,2> wrists{};
    auto targets=[&](unsigned variation,a::adapter::MountKind kind=a::adapter::MountKind::free_hand){
        source=actual;
        for(unsigned side=0;side<2;++side){a::mount::Mount mount{};C(a::mount::marcus115(model->signature,static_cast<a::mount::Side>(side),1,source.units_per_metre,mount));
            if(side&&kind==a::adapter::MountKind::held_lancer){gears_weapon_lancer_wrist_mount::Profile held{};C(gears_weapon_lancer_wrist_mount::marcus115(model->signature,1,source.units_per_metre,held));mount=held.mount;}
            const auto shoulder=before[mount.chain[0]].position;const auto from_shoulder=sub(before[mount.chain[2]].position,shoulder);
            wrists[side]=before[mount.chain[2]];wrists[side].position=add(shoulder,scale(from_shoulder,.78+.02*variation));
            wrists[side].position=add(wrists[side].position,{double(variation)*.15,side?-.25:.25,.3});
            wrists[side].orientation=mul(turn((variation+side)%3,.17+.13*variation),wrists[side].orientation);
            if(variation==6)wrists[side].position=before[mount.chain[2]].position; // rotation only
            if(variation==7)wrists[side].orientation=before[mount.chain[2]].orientation; // translation only
            source.world_grip[side]=synthetic_grip(wrists[side],mount,source.units_per_metre);
        }
        C(a::core::core_snapshot_valid(source,now,50000000,sim));
    };
    double max_wrist{},max_angle{},max_length{};
    auto stock_outputs=std::make_unique<std::array<b::Output,8>>();
    for(unsigned selection=0;selection<2;++selection)for(unsigned variation=0;variation<8;++variation){const auto kind=selection?a::adapter::MountKind::held_lancer:a::adapter::MountKind::free_hand;targets(variation,kind);const auto source_bytes=bytes(source);auto combined=body_only;
        const auto result=selection?a::compose(*model,*frame,source,asset,base,now,sim,*work,combined,kind):a::compose(*model,*frame,source,asset,base,now,sim,*work,combined);
        std::printf("mount %u case %u status=%u side=%u hand=%u\n",selection,variation,unsigned(result.status),result.side,unsigned(result.hand_status));C(result.status==a::Status::okay);
        if(result.status!=a::Status::okay)continue;
        if(!selection){(*stock_outputs)[variation]=combined;auto explicit_stock=body_only;C(a::compose(*model,*frame,source,asset,base,now,sim,*work,explicit_stock,a::adapter::MountKind::free_hand).status==a::Status::okay);C(bytes(explicit_stock)==bytes(combined));}
        else{for(unsigned left:{8u,10u,11u})C(bytes(combined.local[left])==bytes((*stock_outputs)[variation].local[left]));C(qerror(wrists[1].orientation,source.world_grip[1].orientation)<1e-12);}
        const auto after=fk(*model,*frame,combined);
        if(selection)C(qerror(after[35].orientation,source.world_grip[1].orientation)<1e-5);
        for(unsigned side=0;side<2;++side){const unsigned shoulder=side?32u:8u,wrist=side?35u:11u;
            const auto e=distance(after[wrist].position,wrists[side].position),q=qerror(after[wrist].orientation,wrists[side].orientation);max_wrist=(std::max)(max_wrist,e);max_angle=(std::max)(max_angle,q);C(e<.002&&q<1e-5);
            C(distance(after[shoulder].position,before[shoulder].position)<1e-6);
            C(qerror(after[shoulder].orientation,combined.shoulders_world[side].orientation)<1e-6);
            if(variation!=6)C(qerror(before[shoulder].orientation,after[shoulder].orientation)>.001);
            C(distance(after[b::legs[side][2]].position,before[b::legs[side][2]].position)<1e-6&&qerror(after[b::legs[side][2]].orientation,before[b::legs[side][2]].orientation)<1e-6);
            C(work->solutions[side].lengths==a::two::LengthResolution::preserved&&!work->solutions[side].length_adjusted);
            C(a::adapter::same_source(work->targets[side],source));
        }
        for(unsigned i:{b::root,b::pelvis,b::head})C(distance(after[i].position,before[i].position)<1e-6&&qerror(after[i].orientation,before[i].orientation)<1e-6);
        C(bytes(combined.root_world)==bytes(body_only.root_world)&&bytes(combined.head_world)==bytes(body_only.head_world)&&bytes(combined.feet_world)==bytes(body_only.feet_world));
        std::array<std::uint64_t,2> changed{};
        for(unsigned i=0;i<b::bones;++i){C(!std::memcmp(combined.local[i].translation,body_only.local[i].translation,12)&&!std::memcmp(&combined.local[i].scale,&body_only.local[i].scale,4));
            if(std::memcmp(&frame->local[i],&combined.local[i],32))changed[i/64]|=std::uint64_t(1)<<(i%64);
            const bool arm=i==8||i==10||i==11||i==32||i==34||i==35;if(!arm)C(!std::memcmp(&combined.local[i],&body_only.local[i],32));
            if(i){const auto p=unsigned(model->parents[i]);const double error=std::abs(distance(after[i].position,after[p].position)-distance(before[i].position,before[p].position));max_length=(std::max)(max_length,error);C(error<.002);}}
        C(changed==combined.changed_mask&&combined.report.max_length_error<.002);
        for(unsigned child:{51u,52u}){const auto parent=unsigned(model->parents[child]);const auto& local=body_only.local[child];
            C(distance(after[child].position,add(after[parent].position,rotate(after[parent].orientation,{local.translation[0],local.translation[1],local.translation[2]})))<1e-9);
            C(qerror(after[child].orientation,v::normalized(mul(after[parent].orientation,{local.q[0],local.q[1],local.q[2],local.q[3]})))<1e-9);C(!std::memcmp(&combined.local[child],&body_only.local[child],32));C(distance(after[child].position,before[child].position)>.01);}
        C(bytes(source)==source_bytes&&bytes(*model)==model_bytes&&bytes(*frame)==frame_bytes&&bytes(body_only)==original);
    }
    // All rejecting paths preserve the entire output, including a left solved
    // scratch arm followed by a right unreachable target.
    for(unsigned failure=0;failure<10;++failure){targets(1);auto combined=body_only;auto rejected_frame=std::make_unique<b::Frame>(*frame);auto rejected_model=std::make_unique<b::Model>(*model);auto render_base=base;auto current=now;
        a::Status expected=a::Status::invalid_core;
        if(failure==0){render_base[12]+=.25f;expected=a::Status::basis;}
        if(failure==1)current+=100000001;
        if(failure==2){rejected_frame->reference[114].bytes[12]^=1;expected=a::Status::reference;}
        if(failure==3){source.world_grip[1].position={1e8,-1e8,1e8};expected=a::Status::unreachable;}
        if(failure==4){source.world_pose_mask&=~gears_rig_tracking::right_grip_pose;expected=a::Status::hand_untracked;}
        if(failure==5){++source.controllers.sequence;expected=a::Status::invalid_core;}
        if(failure==6){combined.local[52].translation[0]+=.2f;expected=a::Status::invalid_body;}
        if(failure==7){rejected_model->parents[10]=9;expected=a::Status::reference;}
        if(failure==8){source.native_reference.flags=gears_rig_tracking::reference_pending;expected=a::Status::invalid_core;}
        if(failure==9){combined.report.max_length_error=NAN;expected=a::Status::invalid_body;}
        const auto sentinel=bytes(combined);const auto source_bytes=bytes(source);const auto rejected_frame_bytes=bytes(*rejected_frame);const auto rejected_model_bytes=bytes(*rejected_model);const auto result=a::compose(*rejected_model,*rejected_frame,source,asset,render_base,current,sim,*work,combined);
        C(result.status==expected&&bytes(combined)==sentinel&&bytes(source)==source_bytes&&bytes(*rejected_frame)==rejected_frame_bytes&&bytes(*rejected_model)==rejected_model_bytes);if(failure==3)C(result.side==1);
    }
    targets(1,a::adapter::MountKind::held_lancer);{auto untouched=body_only;const auto input=bytes(source);const auto result=a::compose(*model,*frame,source,asset,base,now,sim,*work,untouched,static_cast<a::adapter::MountKind>(99));C(result.status==a::Status::binding&&bytes(untouched)==original&&bytes(source)==input);}
    targets(0);work->body_work.candidate=body_only;const auto sentinel=bytes(work->body_work.candidate);
    C(a::compose(*model,*frame,source,asset,base,now,sim,*work,work->body_work.candidate).status==a::Status::alias&&bytes(work->body_work.candidate)==sentinel);
    work->targets[0].source=source;auto combined=body_only;C(a::compose(*model,*frame,work->targets[0].source,asset,base,now,sim,*work,combined).status==a::Status::alias&&bytes(combined)==original);
    C(bytes(*frame)==frame_bytes&&bytes(*model)==model_bytes&&bytes(body_only)==original);
    std::printf("body_c0_hands: %u checks %u failures max_wrist=%g max_q=%g max_segment=%g owned_buffer_only=1 synthetic_grip_input_seam=1 runtime_proved=0 Workspace=%zu\n",checks,failures,max_wrist,max_angle,max_length,sizeof(a::Workspace));
    return failures?1:0;
}
