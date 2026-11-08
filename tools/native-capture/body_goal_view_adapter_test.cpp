#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include "body_goal_view_adapter.h"
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
namespace a=gears_body_goal_view;namespace b=a::body;namespace v=gears_vr;namespace mount=a::marker;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL line %d: %s\n",__LINE__,#x);}}while(0)
bool read(std::string const& path,void* out,size_t size){std::ifstream f(path,std::ios::binary);return bool(f.read(static_cast<char*>(out),std::streamsize(size)))&&f.peek()==EOF;}
bool digest(void const* bytes,size_t size,b::rig::Signature& out){BCRYPT_ALG_HANDLE alg{};BCRYPT_HASH_HANDLE hash{};
    bool ok=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;if(ok)ok=BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0;
    if(ok)ok=BCryptHashData(hash,reinterpret_cast<PUCHAR>(const_cast<void*>(bytes)),ULONG(size),0)>=0;if(ok)ok=BCryptFinishHash(hash,out.data(),ULONG(out.size()),0)>=0;
    if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);return ok;}
template<class T>auto bytes(T const& x){std::array<unsigned char,sizeof(T)> r{};std::memcpy(r.data(),&x,sizeof x);return r;}
v::Vector plus(v::Vector p,v::Vector q){return {p.x+q.x,p.y+q.y,p.z+q.z};}
v::Vector minus(v::Vector p,v::Vector q){return {p.x-q.x,p.y-q.y,p.z-q.z};}
v::Vector times(v::Vector p,double n){return {p.x*n,p.y*n,p.z*n};}
double distance(v::Vector p,v::Vector q){return std::hypot(p.x-q.x,p.y-q.y,p.z-q.z);}
// Independent quaternion/vector oracle, not rig_tracking::map_vector/quaternion.
v::Quaternion oracle_multiply(v::Quaternion p,v::Quaternion q){return {p.w*q.x+p.x*q.w+p.y*q.z-p.z*q.y,p.w*q.y-p.x*q.z+p.y*q.w+p.z*q.x,p.w*q.z+p.x*q.y-p.y*q.x+p.z*q.w,p.w*q.w-p.x*q.x-p.y*q.y-p.z*q.z};}
v::Vector rotate(v::Quaternion q,v::Vector p){auto r=oracle_multiply(oracle_multiply(q,{p.x,p.y,p.z,0}),{-q.x,-q.y,-q.z,q.w});return {r.x,r.y,r.z};}
v::Quaternion axis(unsigned i,double angle){v::Quaternion q{};q.w=std::cos(angle/2);if(i==0)q.x=std::sin(angle/2);if(i==1)q.y=std::sin(angle/2);if(i==2)q.z=std::sin(angle/2);return q;}
double angle_error(v::Quaternion p,v::Quaternion q){double sign=p.x*q.x+p.y*q.y+p.z*q.z+p.w*q.w<0?-1.:1.;return std::abs(p.x-sign*q.x)+std::abs(p.y-sign*q.y)+std::abs(p.z-sign*q.z)+std::abs(p.w-sign*q.w);}
v::Vector map(a::Neutral const& n,v::Vector xr,double u){return times(plus(plus(times(n.eye_anchor.right_world,xr.x),v::Vector{0,0,xr.y}),times(n.eye_anchor.forward_world,-xr.z)),u);}
void compare(a::Output const& p,a::Output const& q){C(distance(p.desired_eye.position,q.desired_eye.position)<1e-10&&angle_error(p.desired_eye.orientation,q.desired_eye.orientation)<1e-10);C(p.render.base_view==q.render.base_view);C(distance(p.goal.absolute.neutral_pelvis_world,q.goal.absolute.neutral_pelvis_world)==0&&angle_error(p.goal.absolute.root_orientation.value,q.goal.absolute.root_orientation.value)==0);C(distance(p.goal.physical_world,q.goal.physical_world)==0&&distance(p.goal.committed_grip_world,q.goal.committed_grip_world)==0);}
int main(int argc,char** argv){
    if(argc!=3)return 2;auto frame=std::make_unique<b::Frame>();auto model=std::make_unique<b::Model>();auto marker_work=std::make_unique<mount::Workspace>();mount::Marker marker{};a::core::CoreSnapshot actual{};
    C(read(argv[1],&actual,sizeof actual));C(read(std::string(argv[2])+"/reference.bin",frame->reference.data(),sizeof frame->reference));C(read(std::string(argv[2])+"/C0-local.bin",frame->local.data(),sizeof frame->local));
    C(read(std::string(argv[2])+"/C0-component.bin",&frame->component_to_world,sizeof frame->component_to_world));
    frame->units_per_metre=actual.units_per_metre;C(b::prepare_model(frame->reference,digest,*model));C(mount::prepare(*model,*frame,mount::Basis::designer_authored_ue_actor_axes,*marker_work,marker)==mount::Status::okay);
    a::Policy policy{};policy.now_ns=static_cast<int64_t>(actual.published_ns);policy.allow_sim=actual.stereo.source==v::StereoSource::MetaOperatorSimulated;
    C(a::core::core_snapshot_valid(actual,policy.now_ns,a::maximum_age_ns,policy.allow_sim));
    // Upstream-authored fixed world anchor, intentionally unrelated to the old
    // Core camera or animation. No production first-frame calibration exists.
    a::Neutral neutral{};const double yaw=.43;
    neutral.eye_anchor={{1234,-510,196},{std::cos(yaw),std::sin(yaw),0},{-std::sin(yaw),std::cos(yaw),0}};
    neutral.pelvis_world={1230,-500,100};neutral.root_bone_world_q=axis(2,-.6);
    neutral.feet_world[0]={{1235,-510,0},axis(2,-.6)};neutral.feet_world[1]={{1225,-490,0},axis(2,-.6)};
    v::CoverOffset zero{},committed{.07,.03};a::Output from_actual{};const auto actual_bytes=bytes(actual);const auto frame_bytes=bytes(*frame);
    C(a::prepare(neutral,actual,*model,*frame,marker,committed,policy,from_actual)==a::Status::okay);C(bytes(actual)==actual_bytes&&bytes(*frame)==frame_bytes);
    a::core::CoreSnapshot source=actual;source.stereo.head=source.reference;a::Output baseline{};
    C(a::prepare(neutral,source,*model,*frame,marker,zero,policy,baseline)==a::Status::okay);
    C(distance(baseline.goal.physical_world,{})==0&&distance(baseline.goal.committed_grip_world,{})==0);
    C(baseline.goal.target_mode==b::BodyTargetMode::world_absolute&&baseline.goal.body_yaw_delta==0);
    double max_eye_error{},max_axis_error{};
    for(auto relative:{axis(0,.9),axis(1,1.2),axis(1,-1.4),axis(2,.7),oracle_multiply(oracle_multiply(axis(1,.8),axis(0,-.6)),axis(2,.4))}){
        auto moved=source;moved.stereo.head.orientation=oracle_multiply(source.reference.orientation,relative);a::Output out{};
        C(a::prepare(neutral,moved,*model,*frame,marker,zero,policy,out)==a::Status::okay);
        C(distance(out.desired_eye.position,baseline.desired_eye.position)==0&&distance(out.goal.physical_world,{})==0);
        C(distance(out.goal.absolute.neutral_pelvis_world,neutral.pelvis_world)==0&&angle_error(out.goal.absolute.root_orientation.value,neutral.root_bone_world_q)==0);
        C(bytes(out.goal.feet_world)==bytes(neutral.feet_world));
        for(unsigned i=0;i<3;++i){const v::Vector raw=i==0?v::Vector{0,0,-1}:i==1?v::Vector{1,0,0}:v::Vector{0,1,0};const v::Vector ue=i==0?v::Vector{1,0,0}:i==1?v::Vector{0,1,0}:v::Vector{0,0,1};
            const auto expected=map(neutral,rotate(relative,raw),1);const double e=distance(rotate(out.desired_eye.orientation,ue),expected);max_axis_error=(std::max)(max_axis_error,e);C(e<1e-6);}
        mount::EyeHmdWorld reconstructed{};C(mount::eye_from_head(marker,*model,*frame,{out.goal.head_world},reconstructed)==mount::Status::okay);
        double e=distance(reconstructed.pose.position,out.desired_eye.position);max_eye_error=(std::max)(max_eye_error,e);C(e<1e-10&&angle_error(reconstructed.pose.orientation,out.desired_eye.orientation)<1e-10);
        // Low-head pitch rotates around the fixed Eye; it never creates crouch.
        C(distance(plus(out.goal.absolute.neutral_pelvis_world,out.goal.physical_world),neutral.pelvis_world)==0);
    }
    for(v::Vector delta: {v::Vector{.08,0,0},v::Vector{-.08,0,0},v::Vector{0,.06,0},v::Vector{0,-.06,0},v::Vector{0,0,-.09},v::Vector{0,0,.09}}){
        auto moved=source;moved.stereo.head.position=plus(source.reference.position,rotate(source.reference.orientation,delta));a::Output out{};
        C(a::prepare(neutral,moved,*model,*frame,marker,committed,policy,out)==a::Status::okay);
        const auto expected=map(neutral,delta,source.units_per_metre);C(distance(out.goal.physical_world,expected)<1e-10);
        C(distance(out.goal.committed_grip_world,{neutral.eye_anchor.right_world.x*committed.right*source.units_per_metre,neutral.eye_anchor.right_world.y*committed.right*source.units_per_metre,committed.up*source.units_per_metre})<1e-12);
        a::Output virtual_only{};C(a::prepare(neutral,source,*model,*frame,marker,committed,policy,virtual_only)==a::Status::okay);
        C(out.render.base_view==virtual_only.render.base_view);C(distance(minus(out.desired_eye.position,virtual_only.desired_eye.position),expected)<1e-4);
        mount::EyeHmdWorld eye{};C(mount::eye_from_head(marker,*model,*frame,{out.goal.head_world},eye)==mount::Status::okay);C(distance(eye.pose.position,out.desired_eye.position)<1e-10);
    }
    a::Output held{},released{};C(a::prepare(neutral,source,*model,*frame,marker,committed,policy,held)==a::Status::okay);
    const auto retained=committed;C(a::prepare(neutral,source,*model,*frame,marker,retained,policy,released)==a::Status::okay);compare(held,released);
    // Neither an old published world Head nor new animated Local can rebase us.
    auto altered=source;altered.world_head.position={90000,-80000,70000};a::Output unchanged{};
    C(a::prepare(neutral,altered,*model,*frame,marker,committed,policy,unchanged)==a::Status::okay);compare(held,unchanged);
    auto poisoned=std::make_unique<b::Frame>(*frame);for(auto& atom:poisoned->local){atom.q[0]=NAN;atom.translation[0]=INFINITY;atom.scale=0;}
    C(a::prepare(neutral,source,*model,*poisoned,marker,committed,policy,unchanged)==a::Status::okay);compare(held,unchanged);
    // A large world origin retains the actual float renderer base, no FP lift.
    auto distant=neutral;distant.eye_anchor.neutral_eye_world={50000,-20000,1000};C(a::prepare(distant,source,*model,*frame,marker,zero,policy,unchanged)==a::Status::okay);
    C(distance(unchanged.render.base_eye_world,distant.eye_anchor.neutral_eye_world)==0);
    const auto before_far=bytes(unchanged);distant.eye_anchor.neutral_eye_world={1000000,-2000000,3000000};C(a::prepare(distant,source,*model,*frame,marker,zero,policy,unchanged)==a::Status::view);C(bytes(unchanged)==before_far);
    for(unsigned bad=0;bad<13;++bad){auto n=neutral;auto c=source;auto f=std::make_unique<b::Frame>(*frame);auto pol=policy;auto offset=committed;mount::Marker use=marker;auto mm=std::make_unique<b::Model>(*model);
        if(bad==0)pol.now_ns=0;if(bad==1)pol.now_ns=static_cast<int64_t>(c.published_ns+a::maximum_age_ns+1);if(bad==2)c.native_reference.flags=0;if(bad==3)f->units_per_metre+=1;
        if(bad==4)n.eye_anchor.forward_world={};if(bad==5)n.root_bone_world_q={0,0,0,0};if(bad==6)n.pelvis_world.x=NAN;if(bad==7)n.feet_world[1].position.z=INFINITY;
        if(bad==8)offset.up=NAN;if(bad==9)mm->canonical[0]^=1;if(bad==10)f->reference[114].bytes[12]^=1;if(bad==11)use={};if(bad==12)pol.scope=static_cast<b::Scope>(9);
        a::Output sentinel=held;const auto before=bytes(sentinel);C(a::prepare(n,c,*mm,*f,use,offset,pol,sentinel)!=a::Status::okay);C(bytes(sentinel)==before);
    }
    auto alias=std::make_unique<a::core::CoreSnapshot>(source);const auto before=bytes(*alias);auto& bad_out=*reinterpret_cast<a::Output*>(alias.get());C(a::prepare(neutral,*alias,*model,*frame,marker,committed,policy,bad_out)==a::Status::alias);C(bytes(*alias)==before);
    C(bytes(actual)==actual_bytes&&bytes(*frame)==frame_bytes);
    std::printf("BodyGoalViewAdapter: %u checks, %u failures; input Core=%zu bytes + full115 Ref; maxEye=%g maxAxis=%g; no Native/physics permission\n",checks,failures,sizeof(actual),max_eye_error,max_axis_error);return failures?1:0;
}
