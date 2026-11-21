#include "weapon_torque_bow_grasp_pose.h"
#include "hand_authored_grasp.h"
#include <cassert>
#include <cstdio>
namespace b=gears_torque_bow_grasp_pose;namespace a=gears_hand_authored_grasp;
int main(){
 std::array<b::Profile,4> poses{};
 for(unsigned part=0;part<4;++part){
  const auto reference=part?b::artist::reference_left:b::artist::reference_right;const auto unchanged=reference;
  assert(b::author(b::Part(part),reference,poses[part]));
  const auto& p=poses[part];a::Request request{};request.reference=reference;request.closed=p.closed;request.trigger_closed=p.trigger_closed;
  request.profile_id=p.id;request.geometry_signature=gears_hand_authored_mount::marcus115_signature;
  request.side=p.side?a::Side::right:a::Side::left;request.mode=p.side?a::Mode::right_main_fingers:a::Mode::left_part;
  request.kind=p.side?gears_hand_object_grasp::Kind::none:part==1?gears_hand_object_grasp::Kind::support:gears_hand_object_grasp::Kind::loading;
  request.has_trigger_endpoint=p.has_trigger_endpoint;assert(a::structural(request));
  a::Binding binding{request.geometry_signature,reference};assert(a::reference_matches(request,binding));
  request.reference[7]=gears_hand_fingers::turn(2,.2);assert(!a::reference_matches(request,binding));
  for(unsigned i=0;i<15;++i)assert(reference[i].x==unchanged[i].x&&reference[i].y==unchanged[i].y&&reference[i].z==unchanged[i].z&&reference[i].w==unchanged[i].w);
 }
 for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j){assert(poses[i].id!=poses[j].id);bool different=false;for(unsigned n=0;n<15;++n){auto a=poses[i].closed[n],b=poses[j].closed[n];different=different||std::abs(a.x-b.x)+std::abs(a.y-b.y)+std::abs(a.z-b.z)+std::abs(a.w-b.w)>1e-7;}assert(different);}
 for(unsigned i=3;i<15;++i){auto x=poses[0].closed[i],y=poses[0].trigger_closed[i];assert(x.x==y.x&&x.y==y.y&&x.z==y.z&&x.w==y.w);}
 b::Profile invalid{};auto reference=b::artist::reference_left;
 reference[2]={0,0,0,0};assert(!b::author(b::Part::arrow_feed,reference,invalid));
 reference=b::artist::reference_left;reference[2]=gears_hand_fingers::turn(2,.2);
 assert(!b::author(b::Part::arrow_feed,reference,invalid)); // unit but wrong MODEL reference
 a::Request request{};auto signature=gears_hand_authored_mount::marcus115_signature;signature[31]^=1;
 assert(!b::request(b::Part::arrow_shaft,signature,b::artist::reference_left,{}, {},false,request));
 assert(b::request(b::Part::main_grip,gears_hand_authored_mount::marcus115_signature,b::artist::reference_right,{}, {},false,request));
 a::Binding binding{request.geometry_signature,b::artist::reference_right};a::Joints released{},pressed{};
 assert(a::rotations(request,binding,0,released)&&a::rotations(request,binding,1,pressed));
 for(unsigned i=3;i<15;++i){auto x=released[i],y=pressed[i];assert(x.x==y.x&&x.y==y.y&&x.z==y.z&&x.w==y.w);}
 gears_weapon::WeaponMeshFollow fixed;std::array<float,16> matrix{0,-.3f,0,0,.3f,0,0,0,0,0,.3f,0,-5,0,0,1};
 assert(fixed.bind({{},{}},matrix));gears_hand_authored_mount::Mount wrist{};
 assert(gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,gears_hand_authored_mount::Side::left,.3,30,wrist));
 gears_vr::Pose grip{};assert(b::support_grip(fixed,.3,.3,30,wrist.calibration,grip));
 gears_vr::Pose recovered{};auto world=grip;world.position=b::am::mul(world.position,30);
 assert(gears_hand_wrist::native_wrist_target(world,wrist.calibration,30,recovered)==gears_hand_wrist::Status::okay);
 const auto expected=b::am::add(gears_vr::Vector{-5,0,0},b::am::mul(b::artist::support_wrist_in_root_raw.position,.3));
 assert(gears_weapon_two_hand::length(b::am::sub(expected,recovered.position))<1e-5);
 assert(!b::support_grip(fixed,.3,.4,30,wrist.calibration,grip));
 gears_weapon::WeaponMeshFollow unbound;assert(!b::support_grip(unbound,.3,.3,30,wrist.calibration,grip));
 std::puts("Bow four private finger profiles satisfy actual V4 structure/reference checks and index-only trigger; skin/palm/wrist/cuff fitting unaccepted.");
}
