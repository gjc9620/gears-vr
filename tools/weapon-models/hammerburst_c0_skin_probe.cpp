// Offline owned-buffer probe for Hammerburst hand/forearm surface fitting.
// Calls the existing C0 body/hand composers; no engine read, hook, clock query,
// controller publication or game/Native write exists here. The dummy identity
// and CoreSnapshot are an explicit GEOMETRY-ONLY host seam, never admission.
//
// CLI: fixture-directory input.bin output.bin [--inward-one-cm]
// Input80 doubles: observed HB right Grip xyz/xyzw; mesh xyz/UE PitchYawRoll
// integer units; authored left wrist xyz/xyzw in mesh;15 local xyzw joints.
// Output833 doubles: constructed gun, component, desired wrist, actual wrist
// poses (xyz/xyzw), followed by115 complete posed bone xyz/xyzw records.
// The optional1cm movement is a constructed reach counterexample, not a fix
// or a claim about any current runtime/physical tracking sample.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include "tools/native-capture/body_c0_hands.h"
#include "tools/vr-common/rig_tracking_math.h"
#include <fstream>
#include <memory>
#include <cstdio>
#include <vector>
namespace a=gears_body_c0_hands;namespace b=a::body;namespace v=gears_vr;
bool read(char const* path,void* out,size_t n){std::ifstream f(path,std::ios::binary);return bool(f.read(static_cast<char*>(out),std::streamsize(n)))&&f.peek()==EOF;}
bool digest(void const* bytes,size_t n,b::rig::Signature& out){BCRYPT_ALG_HANDLE alg{};BCRYPT_HASH_HANDLE h{};bool ok=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;if(ok)ok=BCryptCreateHash(alg,&h,nullptr,0,nullptr,0,0)>=0;if(ok)ok=BCryptHashData(h,(PUCHAR)bytes,ULONG(n),0)>=0;if(ok)ok=BCryptFinishHash(h,out.data(),ULONG(out.size()),0)>=0;if(h)BCryptDestroyHash(h);if(alg)BCryptCloseAlgorithmProvider(alg,0);return ok;}
v::Vector plus(v::Vector x,v::Vector y){return {x.x+y.x,x.y+y.y,x.z+y.z};}
v::Vector minus(v::Vector x,v::Vector y){return {x.x-y.x,x.y-y.y,x.z-y.z};}
v::Vector rotate(v::Quaternion q,v::Vector p){return v::transform(v::rotation_matrix(q),p);}
v::Pose multiply(v::Pose const& p,v::Pose const& q){return {plus(p.position,rotate(p.orientation,q.position)),v::normalized(v::multiply(p.orientation,q.orientation))};}
v::Pose inverse(v::Pose p){auto q=v::inverse(p.orientation);return {rotate(q,{-p.position.x,-p.position.y,-p.position.z}),q};}
v::Pose grip(v::Pose wrist,a::mount::Mount const& m){const auto q=v::multiply(wrist.orientation,v::inverse(m.calibration.grip_to_wrist_rotation));const auto t=m.calibration.grip_to_wrist_metres;return {minus(wrist.position,rotate(q,{t.x*100,t.y*100,t.z*100})),q};}
v::Pose pose(double const* p){return {{p[0],p[1],p[2]},{p[3],p[4],p[5],p[6]}};}
void push(std::vector<double>& out,v::Pose p){for(double x:{p.position.x,p.position.y,p.position.z,p.orientation.x,p.orientation.y,p.orientation.z,p.orientation.w})out.push_back(x);}
int main(int argc,char** argv){
 if(argc!=4&&argc!=5)return 2;const std::string fixtures=argv[1];double data[80]{};if(!read(argv[2],data,sizeof data))return 3;
 auto frame=std::make_unique<b::Frame>();auto model=std::make_unique<b::Model>();auto bw=std::make_unique<b::Workspace>();auto work=std::make_unique<a::Workspace>();
 if(!read((fixtures+"/reference.bin").c_str(),frame->reference.data(),sizeof frame->reference)||!read((fixtures+"/C0-local.bin").c_str(),frame->local.data(),sizeof frame->local)||!read((fixtures+"/C0-component.bin").c_str(),&frame->component_to_world,sizeof frame->component_to_world))return 4;
 if(!b::prepare_model(frame->reference,digest,*model))return 5;
 for(unsigned i=0;i<16;++i)bw->component.m[i]=frame->component_to_world.values[i];std::copy(frame->local.begin(),frame->local.end(),bw->pose.local);if(!b::d::rebuild(*model,*bw))return 6;
 b::Goal goal{};goal.head_world=b::d::pose(bw->pose.world[b::head]);for(unsigned i=0;i<2;++i)goal.feet_world[i]=b::d::pose(bw->pose.world[b::legs[i][2]]);
 b::Output body{};if(b::plan(*model,*frame,goal,*bw,body).status!=b::Status::solved)return 7;
 const auto right_wrist=b::d::pose(bw->pose.world[35]);a::mount::Mount right_mount{},left_mount{};
 if(!a::mount::marcus115(model->signature,a::mount::Side::right,1,100,right_mount)||!a::mount::marcus115(model->signature,a::mount::Side::left,1,100,left_mount))return 8;
 const auto measured_grip=pose(data);const double k=6.283185307179586/65536.;v::Quaternion measured_mesh_q{};
 if(!v::rig_tracking::detail::quaternion(v::ue_matrix({data[10]*k,data[11]*k,data[12]*k}),measured_mesh_q))return 9;
 const v::Pose measured_mesh{{data[7],data[8],data[9]},measured_mesh_q};const auto relative_mesh=multiply(inverse(measured_grip),measured_mesh);
 auto right_grip=grip(right_wrist,right_mount);if(argc==5){if(std::string(argv[4])!="--inward-one-cm")return 12;const auto candidate=multiply(multiply(right_grip,relative_mesh),pose(data+13));auto move=minus(b::d::position(bw->pose.world[8]),candidate.position);const auto length=std::hypot(std::hypot(move.x,move.y),move.z);if(length<1e-6)return 13;right_grip.position=plus(right_grip.position,{move.x/length,move.y/length,move.z/length});}const auto gun=multiply(right_grip,relative_mesh);const auto desired=multiply(gun,pose(data+13));const auto left_grip=grip(desired,left_mount);
 work->left_visual.kind=gears_hand_object_grasp::Kind::magazine;work->left_visual.grip=left_grip;work->left_visual.weight=1;work->left_visual.preset=gears_hand_object_grasp::Preset::authored;work->left_visual.has_authored_pose=true;
 for(unsigned i=0;i<15;++i)work->left_visual.authored_pose[i]={data[20+i*4],data[21+i*4],data[22+i*4],data[23+i*4]};
 work->finger_closure[0]=1;a::core::CoreSnapshot source{};source.units_per_metre=100;for(unsigned i=0;i<4;++i)source.base_view[i*5]=1;
 const auto render_base=source.base_view;
 const a::hand::ObjectIdentity asset{0x70000,0x71000,0x72000,18,19,20,1};b::Output result{};a::Result status{};
 for(unsigned iteration=0;iteration<4;++iteration){result=body;auto prepare=[&](a::adapter::ModelBinding const&,unsigned side){if(side)return a::adapter::Status::hand_untracked;work->computed_wrist[0]=desired;work->eye_targets[0].grip_world=left_grip;return a::adapter::Status::okay;};
  status=a::compose_common(*model,*frame,source,asset,render_base,1000000000+std::int64_t(iteration)*60000000,true,*work,result,a::adapter::MountKind::free_hand,prepare,true);
  if(status.status!=a::Status::okay){std::printf("{\"geometry_only\":true,\"compose_status\":%u,\"side\":%u,\"hand_status\":%u}\n",unsigned(status.status),status.side,unsigned(status.hand_status));return 10;}
 }
 std::vector<double> out;push(out,gun);push(out,b::d::pose(bw->component));push(out,desired);push(out,b::d::pose(work->body_work.pose.world[11]));for(unsigned i=0;i<115;++i)push(out,b::d::pose(work->body_work.pose.world[i]));std::ofstream stream(argv[3],std::ios::binary);stream.write(reinterpret_cast<char const*>(out.data()),std::streamsize(out.size()*sizeof(double)));if(!stream)return 11;
 const auto delta=minus(desired.position,b::d::position(work->body_work.pose.world[11]));const double error=std::hypot(std::hypot(delta.x,delta.y),delta.z);
 std::printf("{\"geometry_only\":true,\"compose_status\":0,\"full_bones\":115,\"saved_fixture_only\":true,\"synthetic_targets\":true,\"game_started\":false,\"solved_mask\":%u,\"left_visual_applied\":%s,\"desired_wrist_error_raw\":%.12g,\"max_length_error_raw\":%.12g,\"doubles\":%zu}\n",work->solved_mask,work->left_visual_applied?"true":"false",error,result.report.max_length_error,out.size());return 0;
}
