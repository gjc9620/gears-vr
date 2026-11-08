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
void shoulder_policy_cases(){
    namespace sh=gears_hand_shoulder;namespace two=gears_hand_two_bone;
    for(int side:{-1,1})for(double reach:{30.,38.}){
        sh::Input in{};in.clavicle.position={0,double(side)*6,0};in.up={0,0,1};in.outward={0,double(side),0};
        auto& arm=in.arm;arm.shoulder.position={0,double(side)*18.406,0};
        arm.elbow.position=add(arm.shoulder.position,{0,0,-24.63});arm.wrist.position=add(arm.elbow.position,{0,0,-22.365});
        // The saved forward-neutral wrist lies about38UE in front of the
        // shoulder, not30: the eye is already forward of the torso. Keep a
        // closer30UE boundary too; bounded clavicles cannot make it identical.
        arm.target.position=add(arm.shoulder.position,{reach,0,6});
        arm.authored={true,add(arm.shoulder.position,{0,.35*side,-1})};arm.share_forearm_twist=true;
        arm.preferred_wrist_bend=.3490658503988659;arm.max_wrist_pole_turn=1.0471975511965977;
        const auto original=bytes(in);two::Solution base{};C(two::solve(arm,base)==two::Status::solved);
        sh::Solution result{};C(sh::solve(in,result)==two::Status::solved);
        const auto bend=[](two::Solution const& s){return sh::detail::bend(s,two::detail::rotated(s.wrist.orientation,{1,0,0}));};
        C(bend(base)-result.wrist_bend>.17&&result.adjusted);
        if(reach==38)C(result.wrist_bend<.48);
        C(distance(result.arm.wrist.position,arm.target.position)<1e-12&&qerror(result.arm.wrist.orientation,arm.target.orientation)<1e-12);
        C(distance(result.clavicle.position,in.clavicle.position)==0&&result.clavicle_turn<=sh::detail::maximum_turn+1e-12);
        C(std::abs(distance(result.clavicle.position,result.arm.shoulder.position)-12.406)<1e-10);
        C(std::abs(distance(result.arm.shoulder.position,result.arm.elbow.position)-24.63)<1e-10&&
          std::abs(distance(result.arm.elbow.position,result.arm.wrist.position)-22.365)<1e-10);
        C(result.arm.elbow.position.z<=(std::max)(result.arm.shoulder.position.z,arm.target.position.z)+2);
        C(bytes(in)==original);
        const auto R=mul(turn(0,.7),turn(2,-1.1));const v::Vector T{123,-59,37};auto moved=in;
        const auto pose=[&](v::Pose p){return v::Pose{add(T,rotate(R,p.position)),mul(R,p.orientation)};};
        moved.clavicle=pose(in.clavicle);moved.up=rotate(R,in.up);moved.outward=rotate(R,in.outward);
        moved.arm.shoulder=pose(arm.shoulder);moved.arm.elbow=pose(arm.elbow);moved.arm.wrist=pose(arm.wrist);moved.arm.target=pose(arm.target);
        moved.arm.authored.point=add(T,rotate(R,arm.authored.point));sh::Solution rigid{};C(sh::solve(moved,rigid)==two::Status::solved);
        C(distance(rigid.arm.shoulder.position,pose(result.arm.shoulder).position)<1e-7&&
          distance(rigid.arm.elbow.position,pose(result.arm.elbow).position)<1e-7&&qerror(rigid.clavicle.orientation,pose(result.clavicle).orientation)<1e-7);
        for(unsigned failure=0;failure<4;++failure){auto bad=in;
            if(failure==0)bad.up={};if(failure==1)bad.clavicle.orientation={0,0,0,0};
            if(failure==2)bad.units_per_metre=NAN;if(failure==3)bad.arm.lengths.mode=two::LengthMode::adaptive;
            const auto before=bytes(result);C(sh::solve(bad,result)!=two::Status::solved&&bytes(result)==before);}
        // A smooth 1mm movement must not select centimetre-scale joint jumps.
        auto prior=result;double max_step{};
        for(unsigned n=1;n<=60;++n){auto next=in;next.arm.target.position.x+=double(n)*.1;sh::Solution current{};
            C(sh::solve(next,current)==two::Status::solved);max_step=(std::max)(max_step,distance(prior.arm.shoulder.position,current.arm.shoulder.position));prior=current;}
        C(max_step<.8);
        printf("shoulder-policy side=%d reach=%g base_bend=%g selected_bend=%g displacement=%g max_joint_step=%g\n",side,reach,bend(base)*180/3.141592653589793,result.wrist_bend*180/3.141592653589793,result.shoulder_displacement,max_step);
    }
}
int main(int argc,char** argv){
    shoulder_policy_cases();
    {
        // A current object pose changes smoothly on attach, release and a
        // different part. Only the index responds to a held rifle's trigger.
        gears_hand_fingers::PoseBlend blend;const v::Quaternion open{0,0,0,1};v::Quaternion held{},out{};
        C(gears_hand_fingers::rotation(0,0,1,open,held,gears_hand_fingers::Shape::loading));
        C(blend.begin(1000000000)&&blend.joint(0,open,held,out)&&qerror(out,open)<1e-12);
        auto prior=out;
        for(unsigned tick=1;tick<=10;++tick){C(blend.begin(1000000000+tick*20000000ll));
            C(blend.joint(0,open,tick<=5?held:open,out));
            C(b::d::angle(prior,out)<=.401);prior=out;}
        C(qerror(out,open)<1e-12);
        for(unsigned n=0;n<15;++n){v::Quaternion aim{},fire{};
            C(gears_hand_fingers::rotation(1,n,1,open,aim,gears_hand_fingers::Shape::weapon,1,0));
            C(gears_hand_fingers::rotation(1,n,1,open,fire,gears_hand_fingers::Shape::weapon,1,1));
            C(n<3?qerror(aim,fire)>.01:qerror(aim,fire)<1e-12);}
        // Frag's retained handle grasp is distinct from firearm trigger
        // articulation. A trigger release never opens the cosmetic grasp.
        for(unsigned n=0;n<15;++n){v::Quaternion held0{},held1{},rifle{};
            C(gears_hand_fingers::rotation(1,n,1,open,held0,gears_hand_fingers::Shape::grenade,1,0));
            C(gears_hand_fingers::rotation(1,n,1,open,held1,gears_hand_fingers::Shape::grenade,1,1));
            C(gears_hand_fingers::rotation(1,n,1,open,rifle,gears_hand_fingers::Shape::weapon));
            C(qerror(held0,held1)<1e-12);if(n<3)C(qerror(held0,rifle)>.01);}
    }
    #if defined(GEARS_C0_CONTEXT_GEOMETRY_ONLY)
    if(argc!=2)return 2;
    #else
    if(argc!=3)return 2;
    #endif
    const std::string fixtures=argv[1];
    auto frame=std::make_unique<b::Frame>();auto model=std::make_unique<b::Model>();auto bw=std::make_unique<b::Workspace>();auto work=std::make_unique<a::Workspace>();
    a::core::CoreSnapshot actual{};
    #if defined(GEARS_C0_CONTEXT_GEOMETRY_ONLY)
    // Explicit geometry-only seam. No old-version Core bytes are relabelled
    // as a current acquisition; legacy source/age/ABI tests remain separate.
    actual.units_per_metre=100;actual.published_ns=1000000000;
    for(unsigned i=0;i<4;++i)actual.base_view[i*5]=1;
    #else
    C(read(argv[2],&actual,sizeof actual));
    #endif
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
    #if !defined(GEARS_C0_CONTEXT_GEOMETRY_ONLY)
    C(a::core::core_snapshot_valid(actual,now,50000000,sim));
    #endif
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
        #if !defined(GEARS_C0_CONTEXT_GEOMETRY_ONLY)
        C(a::core::core_snapshot_valid(source,now,50000000,sim));
        #endif
    };
    double max_wrist{},max_angle{},max_length{};
    #if !defined(GEARS_C0_CONTEXT_GEOMETRY_ONLY)
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
    #endif
    targets(1,a::adapter::MountKind::held_lancer);{auto untouched=body_only;const auto input=bytes(source);const auto result=a::compose(*model,*frame,source,asset,base,now,sim,*work,untouched,static_cast<a::adapter::MountKind>(99));C(result.status==a::Status::binding&&bytes(untouched)==original&&bytes(source)==input);}
    // Exercise the actual independent C0 composition with both clavicles,
    // then check it with the separate quaternion FK oracle. Legacy cases above
    // retain their exact non-clavicle contract.
    {
        auto independent=body_only;std::array<v::Pose,2> expected{};
        const auto result=a::compose_common(*model,*frame,actual,asset,base,now,sim,*work,independent,
            a::adapter::MountKind::held_lancer,[&](a::adapter::ModelBinding const& binding,unsigned side){
                const auto S=before[binding.authored_mount.chain[0]];const auto up=v::Vector{0,0,1};
                auto outward=sub(before[side?32:8].position,before[side?8:32].position);outward=scale(outward,1/distance(outward,{}));
                auto forward=side?a::two::detail::cross(outward,up):a::two::detail::cross(up,outward);forward=scale(forward,1/distance(forward,{}));
                a::two::Quaternion swing{};C(a::two::detail::swing(rotate(S.orientation,{1,0,0}),forward,up,S.orientation,swing));
                expected[side]={add(S.position,add(scale(forward,30),scale(up,6))),mul(swing,S.orientation)};
                work->computed_wrist[side]=expected[side];return a::adapter::Status::okay;
            },true);
        C(result.status==a::Status::okay&&work->solved_mask==3);
        if(result.status==a::Status::okay){const auto after=fk(*model,*frame,independent);
            for(unsigned side=0;side<2;++side){const unsigned clavicle=side?30:6,wrist=side?35:11;
                C(distance(after[wrist].position,expected[side].position)<.002&&qerror(after[wrist].orientation,expected[side].orientation)<1e-5);
                C(distance(after[clavicle].position,before[clavicle].position)<1e-6);
                C(qerror(after[clavicle].orientation,work->shoulder_solutions[side].clavicle.orientation)<1e-5);
                const double shoulder_error=distance(after[b::shoulders[side]].position,independent.shoulders_world[side].position);
                // Independent normalized quaternion FK versus float row
                // matrices accumulates up to4e-5UE on this real fixture.
                printf("shoulder-FK side=%u error=%g\n",side,shoulder_error);C(shoulder_error<1e-4);}
            for(unsigned i:{b::root,b::pelvis,b::head,93u,97u})C(distance(after[i].position,before[i].position)<1e-6&&qerror(after[i].orientation,before[i].orientation)<1e-6);
            for(unsigned i=0;i<b::bones;++i)C(!std::memcmp(independent.local[i].translation,body_only.local[i].translation,12)&&independent.local[i].scale==body_only.local[i].scale);
            for(unsigned i=12;i<27;++i)C(!std::memcmp(independent.local[i].q,model->reference[i].q,16));
            // First fresh skin tick starts at the authored open reference,
            // then blends into the actual held-gun profile, not stock recoil.
            for(unsigned i=36;i<51;++i)C(!std::memcmp(independent.local[i].q,model->reference[i].q,16));
            for(unsigned i:{27u,28u,51u,52u})C(!std::memcmp(&independent.local[i],&body_only.local[i],32));
            std::copy(body_only.local.begin(),body_only.local.end(),bw->pose.local);const auto original_fingers=bytes(bw->pose.local);
            C(a::detail::fingers(*model,*bw,0,0,{})&&bytes(bw->pose.local)==original_fingers);
        }
    }
    // The contact profile changes only the visible left arm/fingers. Check
    // it with real retail geometry and the independent FK/length oracle;
    // the unsnapped target and source remain byte-identical for interaction.
    const auto physical_targets=std::array<v::Pose,2>{{work->computed_wrist[0],work->computed_wrist[1]}};
    {
        // Grip-release after gun pickup must keep a closed main hand; firing
        // changes the index only. Reach, weapon helpers and input are intact.
        work->finger_closure[1]=0;work->finger_pinch[1]=0;
        auto held=body_only;
        for(unsigned tick=1;tick<=5;++tick){held=body_only;
            C(a::compose_common(*model,*frame,actual,asset,base,now+tick*40000000ll,sim,*work,held,
                a::adapter::MountKind::held_lancer,[&](auto const&,unsigned side){work->computed_wrist[side]=physical_targets[side];return a::adapter::Status::okay;},true).status==a::Status::okay);}
        for(unsigned n=0;n<15;++n){const auto& ref=model->reference[36+n].q;v::Quaternion expected{};
            C(gears_hand_fingers::rotation(1,n,1,{ref[0],ref[1],ref[2],ref[3]},expected,gears_hand_fingers::Shape::weapon));
            const auto& q=held.local[36+n].q;C(qerror({q[0],q[1],q[2],q[3]},expected)<1e-5);}
        for(unsigned i:{27u,28u,51u,52u})C(!std::memcmp(&held.local[i],&body_only.local[i],32));
        for(unsigned i=0;i<b::bones;++i)C(!std::memcmp(held.local[i].translation,body_only.local[i].translation,12)&&held.local[i].scale==body_only.local[i].scale);
        work->finger_blend[0]={};work->finger_blend[1]={};
    }
    {
        // The actual compose path must select Frag fingers, rather than just
        // define an unused shape. Retain the supplied wrist target/lengths.
        const auto source_bytes=bytes(actual);auto held=body_only;
        for(unsigned tick=1;tick<=5;++tick){held=body_only;
            C(a::compose_common(*model,*frame,actual,asset,base,now+tick*40000000ll,sim,*work,held,
                a::adapter::MountKind::held_frag,[&](auto const& binding,unsigned side){
                    C(binding.mount_kind==(side?a::adapter::MountKind::held_frag:a::adapter::MountKind::free_hand));
                    work->computed_wrist[side]=physical_targets[side];return a::adapter::Status::okay;
                },true).status==a::Status::okay);}
        for(unsigned n=0;n<15;++n){const auto& ref=model->reference[36+n].q;v::Quaternion expected{};
            C(gears_hand_fingers::rotation(1,n,1,{ref[0],ref[1],ref[2],ref[3]},expected,gears_hand_fingers::Shape::grenade));
            const auto& q=held.local[36+n].q;C(qerror({q[0],q[1],q[2],q[3]},expected)<1e-5);}
        C(bytes(actual)==source_bytes);
        for(unsigned i=0;i<b::bones;++i)C(!std::memcmp(held.local[i].translation,body_only.local[i].translation,12)&&held.local[i].scale==body_only.local[i].scale);
        work->finger_blend[0]={};work->finger_blend[1]={};
    }
    std::array<v::Pose,2> physical_reach{};
    for(unsigned kind=1;kind<=3;++kind){
        auto adjusted=body_only;auto const& calibration=work->bindings[0].authored_mount.calibration;
        const auto q=mul(physical_targets[0].orientation,inv(calibration.grip_to_wrist_rotation));
        const auto d=rotate(q,scale(calibration.grip_to_wrist_metres,frame->units_per_metre));
        work->eye_targets[0].grip_world={sub(physical_targets[0].position,d),q};
        work->left_visual={static_cast<gears_hand_object_grasp::Kind>(kind),
            {add(work->eye_targets[0].grip_world.position,{.5*kind,0,0}),mul(turn(2,.2*kind),q)},1};
        work->left_blend={};work->left_blend.kind=work->left_visual.kind;work->left_blend.weight=1;work->left_blend.last_ns=now-20000000;
        work->finger_closure[0]=1;
        const auto observed_before=bytes(actual);const auto target_before=bytes(work->eye_targets[0].grip_world);
        const auto result=a::compose_common(*model,*frame,actual,asset,base,now,sim,*work,adjusted,
            a::adapter::MountKind::held_lancer,[&](auto const&,unsigned side){work->computed_wrist[side]=physical_targets[side];return a::adapter::Status::okay;},true);
        C(result.status==a::Status::okay&&work->left_visual_applied&&work->solved_mask==3);
        C(bytes(actual)==observed_before&&bytes(work->eye_targets[0].grip_world)==target_before);
        C(bytes(work->computed_wrist[0])==bytes(physical_targets[0]));
        if(kind==1)physical_reach={{work->physical_wrist[0],work->physical_wrist[1]}};
        else for(unsigned side=0;side<2;++side)C(bytes(work->physical_wrist[side])==bytes(physical_reach[side]));
        C(qerror(work->physical_wrist[0].orientation,physical_targets[0].orientation)<1e-6);
        const auto& physical_input=work->inputs[0];
        const auto physical_limit=distance(physical_input.shoulder.position,physical_input.elbow.position)+
            distance(physical_input.elbow.position,physical_input.wrist.position);
        C(distance(work->physical_wrist[0].position,physical_input.shoulder.position)<=physical_limit);
        if(result.status==a::Status::okay){const auto world=fk(*model,*frame,adjusted);
            C(qerror(world[11].orientation,physical_targets[0].orientation)>.1);
            C(qerror(world[35].orientation,physical_targets[1].orientation)<1e-5);
            for(unsigned i=0;i<b::bones;++i)C(!std::memcmp(adjusted.local[i].translation,body_only.local[i].translation,12)&&adjusted.local[i].scale==body_only.local[i].scale);
            for(unsigned i:{b::root,b::pelvis,b::head,93u,97u})C(distance(world[i].position,before[i].position)<1e-6&&qerror(world[i].orientation,before[i].orientation)<1e-6);
        }
    }
    // Exercise the actual support request through the SAME two-arm C0 writer.
    // Relative support geometry must never be mistaken for a world position;
    // steering the left physical grip changes the right skin orientation while
    // keeping both authenticated observations and all bone lengths unchanged.
    {
        const auto right=synthetic_grip(physical_targets[1],work->bindings[1].authored_mount,frame->units_per_metre);
        const auto left=synthetic_grip(physical_targets[0],work->bindings[0].authored_mount,frame->units_per_metre);
        const v::Pose geometry{scale(rotate(inv(right.orientation),sub(left.position,right.position)),1/frame->units_per_metre),
            mul(inv(right.orientation),left.orientation)};
        C(gears_weapon_two_hand::geometry(geometry));
        work->left_visual={gears_hand_object_grasp::Kind::support,geometry,1};
        work->left_blend={};work->right_blend={};
        for(auto* blend:{&work->left_blend,&work->right_blend}){blend->kind=gears_hand_object_grasp::Kind::support;blend->weight=1;blend->last_ns=now-20000000;}
        auto moved_left=left;moved_left.position=add(left.position,{1,0,1});
        gears_weapon_two_hand::Solution expected{};C(gears_weapon_two_hand::solve(right,moved_left,geometry,frame->units_per_metre,expected));
        C(distance(expected.right.position,right.position)<1e-10&&qerror(expected.right.orientation,right.orientation)>.001);
        std::array<v::Pose,2> expected_wrist{};
        for(unsigned side=0;side<2;++side){const auto grip=side?expected.right:expected.left;auto const& calibration=work->bindings[side].authored_mount.calibration;
            expected_wrist[side]={add(grip.position,rotate(grip.orientation,scale(calibration.grip_to_wrist_metres,frame->units_per_metre))),
                mul(grip.orientation,calibration.grip_to_wrist_rotation)};}
        const auto input_before=bytes(actual);auto adjusted=body_only;
        const auto prepare=[&](auto const&,unsigned side){
            work->computed_wrist[side]=physical_targets[side];work->eye_targets[side].grip_world=side?right:moved_left;
            return a::adapter::Status::okay;
        };
        const auto result=a::compose_common(*model,*frame,actual,asset,base,now,sim,*work,adjusted,a::adapter::MountKind::held_lancer,prepare,true);
        C(result.status==a::Status::okay&&work->left_visual_applied&&work->solved_mask==3);
        C(bytes(actual)==input_before&&bytes(work->eye_targets[0].grip_world)==bytes(moved_left)&&bytes(work->eye_targets[1].grip_world)==bytes(right));
        const auto world=fk(*model,*frame,adjusted);
        for(unsigned side=0;side<2;++side){const unsigned wrist=side?35:11;
            C(bytes(work->computed_wrist[side])==bytes(physical_targets[side]));
            C(distance(world[wrist].position,expected_wrist[side].position)<.002&&qerror(world[wrist].orientation,expected_wrist[side].orientation)<1e-5);}
        for(unsigned i=0;i<b::bones;++i)C(!std::memcmp(adjusted.local[i].translation,body_only.local[i].translation,12)&&adjusted.local[i].scale==body_only.local[i].scale);
        // Proximity handguard attachment is a WORLD cosmetic socket. Unlike
        // active support it must not rotate the right hand/gun before Grip.
        {
            auto socket=left;socket.position.x+=1;
            work->left_visual={gears_hand_object_grasp::Kind::support,socket,1,true};
            work->left_blend={};work->right_blend={};
            work->left_blend.kind=gears_hand_object_grasp::Kind::support;
            work->left_blend.weight=1;work->left_blend.last_ns=now-20000000;
            adjusted=body_only;
            C(a::compose_common(*model,*frame,actual,asset,base,now,sim,*work,adjusted,
                a::adapter::MountKind::held_lancer,prepare,true).status==a::Status::okay);
            const auto passive=fk(*model,*frame,adjusted);
            C(work->left_visual_applied&&work->right_blend.weight==0);
            C(distance(passive[35].position,physical_targets[1].position)<.002&&
                qerror(passive[35].orientation,physical_targets[1].orientation)<1e-5);
            C(bytes(actual)==input_before&&bytes(work->eye_targets[0].grip_world)==bytes(moved_left));
            for(unsigned i=0;i<b::bones;++i)C(!std::memcmp(adjusted.local[i].translation,body_only.local[i].translation,12)&&
                adjusted.local[i].scale==body_only.local[i].scale);
        }
        // A release decays the skin-only fitting on fresh physical input, then
        // returns both hands to the independently tracked poses.
        work->left_visual={};
        for(unsigned tick=1;tick<=4;++tick){adjusted=body_only;C(a::compose_common(*model,*frame,actual,asset,base,now+tick*50000000ll,sim,*work,adjusted,
            a::adapter::MountKind::held_lancer,prepare,true).status==a::Status::okay);}
        C(work->left_blend.weight==0&&work->right_blend.weight==0);
        const auto released=fk(*model,*frame,adjusted);
        for(unsigned side=0;side<2;++side)C(distance(released[side?35:11].position,physical_targets[side].position)<.002&&
            qerror(released[side?35:11].orientation,physical_targets[side].orientation)<1e-5);
    }
    work->left_visual={};work->left_blend={};work->right_blend={};work->finger_closure[0]=0;
    targets(0);work->body_work.candidate=body_only;const auto sentinel=bytes(work->body_work.candidate);
    C(a::compose(*model,*frame,source,asset,base,now,sim,*work,work->body_work.candidate).status==a::Status::alias&&bytes(work->body_work.candidate)==sentinel);
    work->targets[0].source=source;auto combined=body_only;C(a::compose(*model,*frame,work->targets[0].source,asset,base,now,sim,*work,combined).status==a::Status::alias&&bytes(combined)==original);
    C(bytes(*frame)==frame_bytes&&bytes(*model)==model_bytes&&bytes(body_only)==original);
    // The new free grasp must close at the MCP even when the engine supplies
    // a completely open or unrelated support animation. Independent FK checks
    // geometry, stable wrist/other bones and every original bone length.
    for(unsigned side=0;side<2;++side){
        const unsigned first=side?36:12,wrist=side?35:11;
        auto opened=body_only;
        for(unsigned i=first;i<first+15;++i)std::memcpy(opened.local[i].q,model->reference[i].q,16);
        const auto open_fk=fk(*model,*frame,opened);double previous_mcp_y[4]{};
        for(unsigned finger=0;finger<4;++finger){const auto local=rotate(inv(open_fk[wrist].orientation),sub(open_fk[first+finger*3+1].position,open_fk[wrist].position));previous_mcp_y[finger]=local.y;}
        for(double amount:{0.,.25,.5,.75,1.}){
            std::copy(opened.local.begin(),opened.local.end(),bw->pose.local);
            C(a::detail::fingers(*model,*bw,side,amount,{{UINT64_MAX,UINT64_MAX}}));
            auto closed=opened;std::copy_n(bw->pose.local,b::bones,closed.local.begin());
            const auto closed_fk=fk(*model,*frame,closed);
            C(distance(closed_fk[wrist].position,open_fk[wrist].position)<1e-8&&qerror(closed_fk[wrist].orientation,open_fk[wrist].orientation)<1e-8);
            for(unsigned i=0;i<b::bones;++i){
                C(!std::memcmp(closed.local[i].translation,opened.local[i].translation,12)&&closed.local[i].scale==opened.local[i].scale);
                if(i<first||i>=first+15)C(!std::memcmp(&closed.local[i],&opened.local[i],32));
                if(i)C(std::abs(distance(closed_fk[i].position,closed_fk[unsigned(model->parents[i])].position)-
                    distance(open_fk[i].position,open_fk[unsigned(model->parents[i])].position))<.002);
            }
            for(unsigned finger=0;finger<4;++finger){const auto local=rotate(inv(closed_fk[wrist].orientation),sub(closed_fk[first+finger*3+1].position,closed_fk[wrist].position));
                C(local.y<=previous_mcp_y[finger]+1e-5);previous_mcp_y[finger]=local.y;
                if(amount==1){const auto before_local=rotate(inv(open_fk[wrist].orientation),sub(open_fk[first+finger*3+1].position,open_fk[wrist].position));
                    C(before_local.y-local.y>1.5);}
            }
            if(amount==1)for(unsigned finger=0;finger<5;++finger){const auto local=rotate(inv(closed_fk[wrist].orientation),sub(closed_fk[first+finger*3+2].position,closed_fk[wrist].position));
                printf("finger-grasp side=%u finger=%u distal_wrist_xyz=%g,%g,%g\n",side,finger,local.x,local.y,local.z);}
        }
        // LOD-unowned fingers remain byte-identical, also at full squeeze.
        std::copy(opened.local.begin(),opened.local.end(),bw->pose.local);const auto unowned=bytes(bw->pose.local);
        C(a::detail::fingers(*model,*bw,side,1,{})&&bytes(bw->pose.local)==unowned);
    }
    // Context transitions begin at the same ordinary squeeze silhouette and
    // move continuously through unit joint rotations; no instant shape swap.
    for(unsigned kind=1;kind<=3;++kind)for(unsigned n=0;n<15;++n){
        const auto& raw=model->reference[12+n].q;
        const v::Quaternion reference{raw[0],raw[1],raw[2],raw[3]};
        v::Quaternion ordinary{},previous{};
        C(gears_hand_fingers::rotation(0,n,1,reference,ordinary));
        for(unsigned step=0;step<=20;++step){v::Quaternion current{};
            C(gears_hand_fingers::rotation(0,n,1,reference,current,
                static_cast<gears_hand_fingers::Shape>(kind),step/20.));
            C(std::abs(current.x*current.x+current.y*current.y+current.z*current.z+current.w*current.w-1)<1e-10);
            if(step==0)C(qerror(current,ordinary)<1e-8);
            else C(qerror(current,previous)<.12);
            previous=current;
        }
    }
    {v::Quaternion sentinel_q{1,2,3,4};const auto before_q=bytes(sentinel_q);
        C(!gears_hand_fingers::rotation(2,0,1,{0,0,0,1},sentinel_q)&&bytes(sentinel_q)==before_q);
        C(!gears_hand_fingers::rotation(0,15,1,{0,0,0,1},sentinel_q)&&bytes(sentinel_q)==before_q);
        C(!gears_hand_fingers::rotation(0,0,NAN,{0,0,0,1},sentinel_q)&&bytes(sentinel_q)==before_q);
        C(!gears_hand_fingers::rotation(0,0,1,{0,0,0,0},sentinel_q)&&bytes(sentinel_q)==before_q);
        for(double weight:{-.01,1.01,double(NAN)})C(!gears_hand_fingers::rotation(0,0,1,{0,0,0,1},sentinel_q,
            gears_hand_fingers::Shape::starter,weight)&&bytes(sentinel_q)==before_q);}
    std::printf("body_c0_hands: %u checks %u failures max_wrist=%g max_q=%g max_segment=%g owned_buffer_only=1 synthetic_grip_input_seam=1 runtime_proved=0 Workspace=%zu\n",checks,failures,max_wrist,max_angle,max_length,sizeof(a::Workspace));
    return failures?1:0;
}
