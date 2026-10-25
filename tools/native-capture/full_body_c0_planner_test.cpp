#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include "full_body_c0_planner.h"
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
    auto bad=*f;bad.count=114;refuse(bad,base,b::Status::count,"count");bad=*f;bad.reference[114].bytes[12]^=1;refuse(bad,base,b::Status::reference,"full-ref-qT-bit");bad=*f;bad.reference[92].bytes[56]=92;refuse(bad,base,b::Status::reference,"parent-cycle");bad=*f;bad.local[3].q[0]=NAN;refuse(bad,base,b::Status::atom,"nan");bad=*f;bad.local[92].scale=1.01f;refuse(bad,base,b::Status::model_scale,"bone-scale");bad=*f;bad.component_to_world.values[0]*=2;refuse(bad,base,b::Status::component,"component-scale");
    auto impossible=base;impossible.head_world.position.z+=100;refuse(*f,impossible,b::Status::spine_unreachable,"head-outside-fixed-length");impossible=base;impossible.feet_world[0].position.x-=500;refuse(*f,impossible,b::Status::feet_incompatible,"separated-foot-anchors");impossible=base;impossible.body_yaw_delta=2;refuse(*f,impossible,b::Status::goal,"outside-stationary-yaw-scope");
    impossible=base;impossible.head_world.position.y+=3;refuse(*f,impossible,b::Status::spine_unreachable,"actual-C0-positive-side-near-reach-boundary");
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
