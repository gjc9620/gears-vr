#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include "full_body_c0_planner.h"
#include "body_c0_neutral.h"
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <memory>
#include <string>
#include <vector>
namespace b=gears_full_body_c0;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %d: %s\n",__LINE__,#x);}}while(0)
bool hash(void const* bytes,std::size_t size,b::rig::Signature& out){
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE h{};DWORD needed{},got{};
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return false;
    bool good=BCryptGetProperty(algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&needed),sizeof needed,&got,0)>=0;
    std::vector<unsigned char> object(needed);
    if(good)good=BCryptCreateHash(algorithm,&h,object.data(),needed,nullptr,0,0)>=0;
    if(good)good=BCryptHashData(h,reinterpret_cast<PUCHAR>(const_cast<void*>(bytes)),ULONG(size),0)>=0;
    if(good)good=BCryptFinishHash(h,out.data(),ULONG(out.size()),0)>=0;
    if(h)BCryptDestroyHash(h);BCryptCloseAlgorithmProvider(algorithm,0);return good;
}
bool read(std::string const& path,void* data,std::size_t n){std::ifstream f(path,std::ios::binary);return bool(f.read(static_cast<char*>(data),std::streamsize(n)))&&f.peek()==EOF;}
double error(b::Vec a,b::Vec c){return std::hypot(a.x-c.x,a.y-c.y,a.z-c.z);}
// Independent Pose/quaternion FK, not the planner's Matrix/Workspace FK.
std::array<b::Pose,b::bones> independent(b::Frame const& f,b::Model const& m,b::Output const& out){
    b::ik::Matrix component{};for(unsigned j=0;j<16;++j)component.m[j]=f.component_to_world.values[j];
    b::Pose origin{b::d::position(component),b::d::rotation(component)};std::array<b::Pose,b::bones> world{};
    for(unsigned i=0;i<b::bones;++i){const auto parent=i?world[unsigned(m.parents[i])]:origin;const auto& a=out.local[i];
        const b::Vec local{a.translation[0],a.translation[1],a.translation[2]};const auto offset=gears_vr::transform(gears_vr::rotation_matrix(parent.orientation),local);
        world[i]={b::d::add(parent.position,offset),gears_vr::normalized(gears_vr::multiply(parent.orientation,{a.q[0],a.q[1],a.q[2],a.q[3]}))};}
    return world;
}
int main(int argc,char** argv){
    if(argc!=2&&argc!=3)return 2;
    const std::string folder=argv[1];auto f=std::make_unique<b::Frame>();auto w=std::make_unique<b::Workspace>();b::Model model{};b::Output output{};
    C(read(folder+"/reference.bin",f->reference.data(),sizeof f->reference));C(read(folder+"/C0-local.bin",f->local.data(),sizeof f->local));C(read(folder+"/C0-component.bin",&f->component_to_world,sizeof f->component_to_world));
    C(b::prepare_model(f->reference,hash,model));C(model.signature==b::rig::marcus115_signature);
    if(argc==3&&std::string(argv[2])=="--neutral-stance"){
        namespace a=gears_body_goal_view;namespace n=gears_body_c0_neutral;
        a::marker::Marker marker{};a::marker::Workspace mw{};n::Stored stored{};a::Neutral neutral{};
        const auto original=*f;
        C(a::marker::prepare(model,*f,a::marker::Basis::marcus_mesh_face_forward_y,mw,marker)==a::marker::Status::okay);
        b::Output source{};source.local=f->local;const auto before=independent(*f,model,source);
        a::marker::EyeHmdWorld original_eye{};
        C(a::marker::eye_from_head(marker,model,*f,{before[b::head]},original_eye)==a::marker::Status::okay);
        C(n::capture(model,*f,marker,*w,stored)&&n::move(stored,f->component_to_world,neutral));
        C(error(neutral.eye_anchor.neutral_eye_world,original_eye.pose.position)<.002);
        C(b::d::angle(neutral.root_bone_world_q,before[b::root].orientation)<1e-5);
        for(unsigned s=0;s<2;++s){C(error(neutral.feet_world[s].position,before[b::legs[s][2]].position)<.002);
            C(b::d::angle(neutral.feet_world[s].orientation,before[b::legs[s][2]].orientation)<1e-5);}
        a::marker::HeadBoneWorld head{};
        const auto forward=neutral.eye_anchor.forward_world;
        const a::marker::EyeHmdWorld eye{{neutral.eye_anchor.neutral_eye_world,b::d::yaw(std::atan2(forward.y,forward.x))}};
        C(a::marker::head_from_eye(marker,model,*f,eye,head)==a::marker::Status::okay);
        b::Goal g{};g.scope=b::Scope::stationary;g.posture=b::Posture::reference_neutral;
        g.target_mode=b::BodyTargetMode::world_absolute;g.head_world=head.pose;
        g.absolute={neutral.pelvis_world,{neutral.root_bone_world_q}};
        for(unsigned s=0;s<2;++s)g.feet_world[s]=neutral.feet_world[s];
        const auto report=b::plan(model,*f,g,*w,output);
        std::printf("neutral status=%u pelvis=%.5f,%.5f,%.5f shoulders_z=%.5f,%.5f head_error=%g projection=%g\n",
            unsigned(report.status),g.absolute.neutral_pelvis_world.x,g.absolute.neutral_pelvis_world.y,g.absolute.neutral_pelvis_world.z,
            output.shoulders_world[0].position.z,output.shoulders_world[1].position.z,report.head_error,report.pelvis_projection);
        C(report.status==b::Status::solved);
        if(report.status==b::Status::solved){
            const auto actual=independent(*f,model,output);
            C(std::abs(actual[8].position.z-actual[32].position.z)<.01);
            C(error(actual[b::head].position,head.pose.position)<.002);
            for(unsigned s=0;s<2;++s)C(error(actual[b::legs[s][2]].position,neutral.feet_world[s].position)<.002);
            for(unsigned i=1;i<b::bones;++i){C(!std::memcmp(output.local[i].translation,f->local[i].translation,12));
                C(output.local[i].scale==f->local[i].scale);
                C(std::abs(error(actual[i].position,actual[unsigned(model.parents[i])].position)-
                    error(before[i].position,before[unsigned(model.parents[i])].position))<.002);}
        }
        C(!std::memcmp(&original,f.get(),sizeof original));
        std::printf("neutral stance: %u checks %u failures; actual eye/feet/input unchanged, Game=0\n",checks,failures);
        return failures?1:0;
    }
    if(argc==3&&std::string(argv[2])=="--near-straight-spine"){
        namespace a=gears_body_goal_view;namespace n=gears_body_c0_neutral;
        a::marker::Marker marker{};a::marker::Workspace mw{};n::Stored stored{};a::Neutral neutral{};
        C(a::marker::prepare(model,*f,a::marker::Basis::marcus_mesh_face_forward_y,mw,marker)==a::marker::Status::okay);
        C(n::capture(model,*f,marker,*w,stored)&&n::move(stored,f->component_to_world,neutral));
        a::marker::HeadBoneWorld head{};
        const auto forward=neutral.eye_anchor.forward_world;
        const a::marker::EyeHmdWorld eye{{neutral.eye_anchor.neutral_eye_world,b::d::yaw(std::atan2(forward.y,forward.x))}};
        C(a::marker::head_from_eye(marker,model,*f,eye,head)==a::marker::Status::okay);
        b::Goal goal{};goal.scope=b::Scope::stationary;goal.posture=b::Posture::reference_neutral;
        goal.target_mode=b::BodyTargetMode::world_absolute;goal.head_world=head.pose;
        goal.absolute={neutral.pelvis_world,{neutral.root_bone_world_q}};
        for(unsigned s=0;s<2;++s)goal.feet_world[s]=neutral.feet_world[s];
        C(b::plan(model,*f,goal,*w,output).status==b::Status::solved);
        const auto anchor=w->chain[0];double total{};for(const auto l:w->lengths)total+=l;
        b::Vec base_direction{};C(b::two::detail::unit(b::d::sub(goal.head_world.position,anchor),base_direction));
        const auto original=*f;unsigned attempts{},solved{},iteration_failures{};double max_head{},max_length{};
        for(double pitch:{-.04,-.02,-.005,0.,.005,.02,.04})for(double slack:{.0001,.001,.005,.02,.1}){
            auto g=goal;const auto direction=b::d::rotate({0,std::sin(pitch/2),0,std::cos(pitch/2)},base_direction);
            g.head_world.position=b::d::add(anchor,b::d::mul(direction,total-slack));const auto prior=output;
            ++attempts;const auto result=b::plan(model,*f,g,*w,output);
            if(result.status!=b::Status::solved){
                if(result.status==b::Status::spine_iterations)++iteration_failures;
                if(iteration_failures<=4)std::printf("near-straight failure pitch=%g slack=%g status=%u residual=%g\n",pitch,slack,unsigned(result.status),result.head_error);
                C(!std::memcmp(&prior,&output,sizeof output));continue;
            }
            ++solved;const auto actual=independent(*f,model,output);const auto he=error(actual[b::head].position,g.head_world.position);
            max_head=(std::max)(max_head,he);C(he<.002);
            for(unsigned s=0;s<2;++s)C(error(actual[b::legs[s][2]].position,g.feet_world[s].position)<.002);
            for(unsigned i=1;i<b::bones;++i){const auto delta=std::abs(error(actual[i].position,actual[unsigned(model.parents[i])].position)-
                std::hypot(f->local[i].translation[0],f->local[i].translation[1],f->local[i].translation[2]));
                max_length=(std::max)(max_length,delta);C(delta<.002);
                C(!std::memcmp(output.local[i].translation,f->local[i].translation,12)&&output.local[i].scale==f->local[i].scale);}
        }
        C(!std::memcmp(&original,f.get(),sizeof original));C(solved==attempts);
        std::printf("near-straight spine: solved=%u/%u iteration_failures=%u max_head=%g max_length=%g checks=%u failures=%u Game=0\n",
            solved,attempts,iteration_failures,max_head,max_length,checks,failures);return failures?1:0;
    }
    if(argc==3&&std::string(argv[2])=="--standing-spine-reach"){
        C(b::retain_native_body(model,*f,*w,output).status==b::Status::solved);
        b::Goal g{};g.scope=b::Scope::stationary;g.posture=b::Posture::reference_neutral;
        g.target_mode=b::BodyTargetMode::world_absolute;
        g.absolute.neutral_pelvis_world=output.pelvis_world.position;g.absolute.root_orientation.value=output.root_world.orientation;
        g.head_world=output.head_world;g.head_world.position.z+=12;
        g.head_world.orientation=gears_vr::multiply({0,std::sin(-.325),0,std::cos(-.325)},g.head_world.orientation);
        for(unsigned s=0;s<2;++s)g.feet_world[s]=output.feet_world[s];
        const auto saved_goal=g;
        C(b::plan(model,*f,g,*w,output).status==b::Status::spine_unreachable);
        double projected{};C(b::plan_with_spine_reach(model,*f,g,*w,output,projected).status==b::Status::solved);
        C(projected>0&&std::abs(error(output.head_world.position,g.head_world.position)-projected)<.002);
        C(b::d::angle(output.head_world.orientation,g.head_world.orientation)<1e-4);
        const auto stable=output;const auto original=*f;
        for(double angle:{-.45,0.,.45}){
            *f=original;
            for(unsigned i:{1u,3u,4u,5u,6u,7u,30u,31u,54u}){
                const auto q=gears_vr::multiply(b::d::yaw(angle),{f->local[i].q[0],f->local[i].q[1],f->local[i].q[2],f->local[i].q[3]});
                f->local[i].q[0]=float(q.x);f->local[i].q[1]=float(q.y);f->local[i].q[2]=float(q.z);f->local[i].q[3]=float(q.w);
            }
            C(b::plan_with_spine_reach(model,*f,g,*w,output,projected).status==b::Status::solved);
            for(unsigned s=0;s<2;++s){C(error(output.shoulders_world[s].position,stable.shoulders_world[s].position)<.002);
                C(error(output.feet_world[s].position,g.feet_world[s].position)<.002);}
            const auto actual=independent(*f,model,output);
            for(unsigned i=1;i<b::bones;++i){C(!std::memcmp(output.local[i].translation,f->local[i].translation,12));
                C(output.local[i].scale==f->local[i].scale);
                C(std::abs(error(actual[i].position,actual[unsigned(model.parents[i])].position)-
                    std::hypot(f->local[i].translation[0],f->local[i].translation[1],f->local[i].translation[2]))<.002);}
        }
        C(!std::memcmp(&saved_goal,&g,sizeof g));
        auto exact=g;exact.head_world=stable.head_world;exact.head_world.position.z-=2;
        C(b::plan_with_spine_reach(model,*f,exact,*w,output,projected).status==b::Status::solved&&projected==0);
        auto cover=g;cover.scope=b::Scope::fixed_cover;const auto sentinel=output;
        C(b::plan_with_spine_reach(model,*f,cover,*w,output,projected).status==b::Status::spine_unreachable);
        C(!std::memcmp(&sentinel,&output,sizeof output));
        auto bad=*f;bad.local[11].q[0]=NAN;
        C(b::plan_with_spine_reach(model,bad,g,*w,output,projected).status!=b::Status::solved);
        C(!std::memcmp(&sentinel,&output,sizeof output));
        std::printf("standing spine reach: %u checks %u failures; eye/input unchanged, fixed lengths, Game=0\n",checks,failures);
        return failures?1:0;
    }
    if(argc==3&&std::string(argv[2])=="--leg-roundoff"){
        const auto original=*f;
        for(float bias:{-0.00001f,0.f,0.00001f}){
            *f=original;
            for(unsigned bone:{91u,92u,95u,96u})for(float& q:f->local[bone].q)q*=1.f+bias;
            C(b::retain_native_body(model,*f,*w,output).status==b::Status::solved);
            b::Goal g{};g.head_world=output.head_world;g.body_yaw_delta=.1;
            for(unsigned s=0;s<2;++s)g.feet_world[s]=output.feet_world[s];
            const auto r=b::plan(model,*f,g,*w,output);
            std::printf("leg-roundoff bias=%g status=%u feet=%g,%g length=%g\n",double(bias),unsigned(r.status),r.foot_error[0],r.foot_error[1],r.max_length_error);
            C(r.status==b::Status::solved);
            if(r.status==b::Status::solved)for(unsigned i=1;i<b::bones;++i){
                C(!std::memcmp(output.local[i].translation,f->local[i].translation,12));C(output.local[i].scale==f->local[i].scale);}
        }
        std::printf("leg-roundoff: %u checks %u failures; Game=0\n",checks,failures);return failures?1:0;
    }
    if(argc==3&&std::string(argv[2])=="--animation-scale"){
        const auto original=*f;
        for(float value:{1.f-0x1p-23f,std::nextafter(1.f,0.f),1.f,std::nextafter(1.f,2.f)}){
            *f=original;f->local[3].scale=value;const auto saved=*f;
            C(b::retain_native_body(model,*f,*w,output).status==b::Status::solved);
            C(!std::memcmp(output.local.data(),f->local.data(),sizeof f->local));
            b::Goal g{};g.head_world=output.head_world;
            g.head_world.orientation=gears_vr::multiply({0,std::sin(.1),0,std::cos(.1)},g.head_world.orientation);
            for(unsigned s=0;s<2;++s)g.feet_world[s]=output.feet_world[s];
            C(b::plan(model,*f,g,*w,output).status==b::Status::solved);
            for(unsigned i=0;i<b::bones;++i){C(!std::memcmp(&output.local[i].scale,&f->local[i].scale,4));
                if(i)C(!std::memcmp(output.local[i].translation,f->local[i].translation,12));}
            C(!std::memcmp(&saved,f.get(),sizeof saved));
        }
        for(float value:{std::nextafter(1.f-0x1p-23f,0.f),std::nextafter(std::nextafter(1.f,2.f),2.f),1.01f,0.f,-1.f,INFINITY,NAN}){
            *f=original;f->local[3].scale=value;const auto saved=output;
            C(b::retain_native_body(model,*f,*w,output).status!=b::Status::solved);
            C(!std::memcmp(&saved,&output,sizeof output));
        }
        std::printf("animation scale: %u checks %u failures; source scale/length preserved, Game=0\n",checks,failures);
        return failures?1:0;
    }
    if(argc==3&&std::string(argv[2])=="--native-body-only"){
        C(b::retain_native_body(model,*f,*w,output).status==b::Status::solved);
        C(!std::memcmp(output.local.data(),f->local.data(),sizeof f->local));
        const auto saved=output;auto invalid=*f;invalid.local[11].q[0]=NAN;
        C(b::retain_native_body(model,invalid,*w,output).status!=b::Status::solved);
        C(!std::memcmp(&saved,&output,sizeof output));
        invalid=*f;invalid.reference[114].bytes[12]^=1;
        C(b::retain_native_body(model,invalid,*w,output).status==b::Status::reference);
        C(!std::memcmp(&saved,&output,sizeof output));
        C(b::retain_native_body(model,*f,*w,w->candidate).status==b::Status::alias);
        std::printf("native-body fallback: %u checks %u failures; original body retained, no HMD goal claim\n",checks,failures);
        return failures?1:0;
    }
    auto bad_reference=f->reference;bad_reference[114].bytes[12]^=1;b::Model sentinel=model;C(!b::prepare_model(bad_reference,hash,sentinel));C(!std::memcmp(&sentinel,&model,sizeof model));
    for(unsigned j=0;j<16;++j)w->component.m[j]=f->component_to_world.values[j];std::copy(f->local.begin(),f->local.end(),w->pose.local);C(b::d::rebuild(model,*w));
    b::Goal base{};base.head_world=b::d::pose(w->pose.world[b::head]);for(unsigned s=0;s<2;++s)base.feet_world[s]=b::d::pose(w->pose.world[b::legs[s][2]]);
    const b::Vec pelvis=b::d::position(w->pose.world[1]);
    unsigned plan_number{};
    auto verify=[&](b::Goal const& goal,char const* name){
        auto report=b::plan(model,*f,goal,*w,output);std::printf("case %s status=%u head=%g feet=%g,%g lengths=%g projection=%g iterations=%u distance=%g reach=%g\n",name,unsigned(report.status),report.head_error,report.foot_error[0],report.foot_error[1],report.max_length_error,report.pelvis_projection,report.spine_iterations,report.requested_distance,report.available_reach);
        C(report.status==b::Status::solved);if(report.status!=b::Status::solved)return false;
        const auto world=independent(*f,model,output);C(error(world[b::head].position,goal.head_world.position)<.002);
        for(unsigned s=0;s<2;++s){C(error(world[b::legs[s][2]].position,goal.feet_world[s].position)<.002);C(b::d::angle(world[b::legs[s][2]].orientation,goal.feet_world[s].orientation)<1e-4);C(error(world[b::shoulders[s]].position,output.shoulders_world[s].position)<.002);}
        for(unsigned i=0;i<b::bones;++i){const bool changed=std::memcmp(&output.local[i],&f->local[i],32)!=0;C(bool(output.changed_mask[i/64]&(std::uint64_t(1)<<(i%64)))==changed);C(output.local[i].scale==f->local[i].scale);
            if(i){C(!std::memcmp(output.local[i].translation,f->local[i].translation,12));const auto& t=f->local[i].translation;C(std::abs(error(world[i].position,world[unsigned(model.parents[i])].position)-std::hypot(t[0],t[1],t[2]))<.002);}}
        C((output.changed_mask[1]>>51)==0);
        if(argc==3){const std::string stem=std::string(argv[2])+"/"+std::to_string(++plan_number)+"-"+name;
            std::ofstream atoms(stem+".local32.bin",std::ios::binary);atoms.write(reinterpret_cast<char const*>(output.local.data()),sizeof output.local);C(bool(atoms));
            std::ofstream meta(stem+".json");meta<<std::setprecision(17);
            const auto vec=[&](b::Vec v){meta<<'['<<v.x<<','<<v.y<<','<<v.z<<']';};
            meta<<"{\"case\":\""<<name<<"\",\"bone_count\":115,\"physical_world\":";vec(goal.physical_world);meta<<",\"committed_grip_world\":";vec(goal.committed_grip_world);
            meta<<",\"head_goal\":";vec(goal.head_world.position);meta<<",\"head_result\":";vec(output.head_world.position);meta<<",\"root_result\":";vec(output.root_world.position);meta<<",\"pelvis_result\":";vec(output.pelvis_world.position);
            meta<<",\"shoulders\":[";vec(output.shoulders_world[0].position);meta<<',';vec(output.shoulders_world[1].position);meta<<"],\"changed_mask\":["<<output.changed_mask[0]<<','<<output.changed_mask[1]<<"],\"head_error\":"<<report.head_error<<",\"feet_error\":["<<report.foot_error[0]<<','<<report.foot_error[1]<<"],\"max_length_error\":"<<report.max_length_error<<",\"pelvis_projection\":"<<report.pelvis_projection<<",\"spine_iterations\":"<<report.spine_iterations<<",\"writer_permission\":false}\n";C(bool(meta));}
        return true;
    };
    C(verify(base,"actual-C0-exact-preservation"));C(!std::memcmp(output.local.data(),f->local.data(),sizeof f->local));C(output.changed_mask[0]==0&&output.changed_mask[1]==0);
    const auto identity=output;
    b::Goal rotated=base;rotated.head_world.orientation=gears_vr::multiply({0,std::sin(.2),0,std::cos(.2)},base.head_world.orientation);
    C(verify(rotated,"head-rotation-only"));C(!std::memcmp(&output.local[0],&identity.local[0],32));C(!std::memcmp(&output.local[1],&identity.local[1],32));C(error(output.pelvis_world.position,pelvis)<1e-6);
    C(output.changed_mask[0]==(std::uint64_t(1)<<b::head)&&output.changed_mask[1]==0);
    const b::Vec deltas[]={{0,0,.5},{0,0,-8},{0,5,0},{0,-5,0},{5,0,0},{-5,0,0},{3,3,.5},{-3,-3,-5}};
    const char* labels[]={"up","down","side-plus","side-minus","forward","back","combined-positive","combined-negative"};
    for(unsigned i=0;i<8;++i){auto g=base;g.physical_world=deltas[i];g.head_world.position=b::d::add(base.head_world.position,deltas[i]);C(verify(g,labels[i]));}
    const b::Vec lean[]={{3,0,0},{0,-3,0},{0,0,.5},{2,2,-1}};
    for(unsigned i=0;i<4;++i){auto g=base;g.head_world.position=b::d::add(base.head_world.position,lean[i]);C(verify(g,"independent-spine-3D"));C(!std::memcmp(&output.local[0],&identity.local[0],32));C(!std::memcmp(&output.local[1],&identity.local[1],32));}
    auto yawed=base;yawed.body_yaw_delta=.12;C(verify(yawed,"body-yaw-fixed-feet"));
    auto cover=base;cover.scope=b::Scope::fixed_cover;cover.body_yaw_delta=-.1;cover.physical_world={1,2,-3};cover.committed_grip_world={2,1,-2};cover.head_world.position=b::d::add(base.head_world.position,{3,3,-5});
    C(verify(cover,"cover-physical-plus-committed"));auto held=output;C(verify(cover,"release-keeps-committed-offset"));C(!std::memcmp(held.local.data(),output.local.data(),sizeof output.local));C(held.changed_mask==output.changed_mask);C(error(held.root_world.position,output.root_world.position)==0&&error(held.head_world.position,output.head_world.position)==0);
    auto neutral=base;neutral.posture=b::Posture::reference_neutral;C(verify(neutral,"explicit-reference-neutral"));
    const auto neutral_before=output;neutral.head_world.orientation=rotated.head_world.orientation;C(verify(neutral,"neutral-head-rotation-only"));C(!std::memcmp(&neutral_before.local[0],&output.local[0],32));C(!std::memcmp(&neutral_before.local[1],&output.local[1],32));
    auto projected=base;projected.physical_world={0,0,20};C(verify(projected,"leg-bounded-pelvis-spine-compensation"));C(output.report.pelvis_projection>0);
    b::Vec combined{},unchanged{9,8,7};C(b::motion_world({.01,.02,-.03},{.02,.01,-.02},{1,0,0},{0,1,0},{0,0,1},100,combined));C(error(combined,{3,3,-5})<1e-12);
    C(!b::motion_world({0,0,0},{0,0,0},{2,0,0},{0,1,0},{0,0,1},100,unchanged));C(error(unchanged,{9,8,7})==0);
    auto refuse=[&](b::Frame const& frame,b::Goal const& goal,b::Status expected,char const* name){const auto saved=output;auto r=b::plan(model,frame,goal,*w,output);std::printf("reject %s status=%u distance=%g reach=%g\n",name,unsigned(r.status),r.requested_distance,r.available_reach);C(r.status==expected);C(!std::memcmp(&saved,&output,sizeof output));};
    auto bad=*f;bad.count=114;refuse(bad,base,b::Status::count,"count");bad=*f;bad.reference[114].bytes[12]^=1;refuse(bad,base,b::Status::reference,"full-ref-qT-bit");bad=*f;bad.reference[92].bytes[56]=92;refuse(bad,base,b::Status::reference,"parent-cycle");bad=*f;bad.local[3].q[0]=NAN;refuse(bad,base,b::Status::atom,"nan");bad=*f;bad.local[92].scale=1.01f;refuse(bad,base,b::Status::model_scale,"bone-scale");bad=*f;for(unsigned i=0;i<3;++i)bad.component_to_world.values[i]*=2;refuse(bad,base,b::Status::component,"component-scale");
    auto impossible=base;impossible.head_world.position.z+=100;refuse(*f,impossible,b::Status::spine_unreachable,"head-outside-fixed-length");impossible=base;impossible.feet_world[0].position.x-=500;refuse(*f,impossible,b::Status::feet_incompatible,"separated-foot-anchors");impossible=base;impossible.body_yaw_delta=b::max_body_yaw_delta+1e-6;refuse(*f,impossible,b::Status::goal,"outside-explicit-heading-range");
    auto near_reach=base;near_reach.head_world.position.y+=3;C(verify(near_reach,"actual-C0-positive-side-reachable"));
    // Actual near-reach failure must not prevent an independently composed
    // pair of tracked hands from receiving a valid native-body starting pose.
    C(b::retain_native_body(model,*f,*w,output).status==b::Status::solved);
    for(unsigned i=1;i<b::bones;++i)C(!std::memcmp(output.local[i].translation,f->local[i].translation,12));
    auto invalid=*f;invalid.local[11].q[0]=NAN;auto retained=output;
    C(b::retain_native_body(model,invalid,*w,output).status!=b::Status::solved);
    C(!std::memcmp(&retained,&output,sizeof output));
    auto saved=output;C(b::plan(model,*f,base,*w,w->candidate).status==b::Status::alias);C(!std::memcmp(&saved,&output,sizeof output));
    // Frame equivariance: rotate and translate the component AND all world goals.
    auto world_frame=*f;const double a=.4,c=std::cos(a),s=std::sin(a);auto original=f->component_to_world;
    for(unsigned i=0;i<4;++i){world_frame.component_to_world.values[4*i]=float(original.values[4*i]*c-original.values[4*i+1]*s);world_frame.component_to_world.values[4*i+1]=float(original.values[4*i]*s+original.values[4*i+1]*c);}
    world_frame.component_to_world.values[12]+=20;world_frame.component_to_world.values[13]-=30;
    const auto transform=[&](b::Pose p){p.position={p.position.x*c-p.position.y*s+20,p.position.x*s+p.position.y*c-30,p.position.z};p.orientation=gears_vr::multiply(b::d::yaw(a),p.orientation);return p;};
    auto world_goal=base;world_goal.head_world=transform(base.head_world);for(unsigned i=0;i<2;++i)world_goal.feet_world[i]=transform(base.feet_world[i]);
    C(b::plan(model,world_frame,world_goal,*w,output).status==b::Status::solved);C(error(output.head_world.position,world_goal.head_world.position)<.002);
    std::printf("{\"Model\":%zu,\"Frame\":%zu,\"Goal\":%zu,\"Report\":%zu,\"Output\":%zu,\"Workspace\":%zu,\"Atom\":%zu,\"Matrix64\":%zu,\"Frame_local\":%zu,\"Frame_component\":%zu,\"Output_mask\":%zu,\"Output_report\":%zu}\n",sizeof(b::Model),sizeof(b::Frame),sizeof(b::Goal),sizeof(b::Report),sizeof(b::Output),sizeof(b::Workspace),sizeof(b::Atom),sizeof(b::Matrix64),offsetof(b::Frame,local),offsetof(b::Frame,component_to_world),offsetof(b::Output,changed_mask),offsetof(b::Output,report));
    std::printf("full_body_c0: %u checks, %u failures; actual_C0_fixture=1 fixed_lengths=1 Game=0 writer=0\n",checks,failures);return failures?1:0;
}
