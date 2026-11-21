#include "weapon_boomshot_hand_mount.h"
#include "weapon_two_hand_hold.h"
#include <cassert>
#include <cstdio>
#include <limits>
namespace m=gears_boomshot_hand_mount;namespace v=gears_vr;
double error(v::Vector a,v::Vector b){const auto d=m::sub(a,b);return std::sqrt(m::dot(d,d));}
void check_palm(v::Pose const& grip,v::Pose const& part,double part_scale,double model_scale,double units,v::Vector raw,v::Vector normal){
 gears_hand_authored_mount::Mount profile{};assert(gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,gears_hand_authored_mount::Side::left,model_scale,units,profile));
 v::Pose wrist{};assert(gears_hand_wrist::native_wrist_target(grip,profile.calibration,units,wrist)==gears_hand_wrist::Status::okay);
 const auto offset=m::rotate(wrist.orientation,m::mul(m::palm_pad_raw,model_scale));
 assert(error({wrist.position.x+offset.x,wrist.position.y+offset.y,wrist.position.z+offset.z},m::point(part,raw,part_scale))<1e-8);
 const auto y=m::rotate(wrist.orientation,{0,1,0});v::Vector unit_normal{};assert(m::unit(normal,unit_normal));
 assert(error(y,m::rotate(part.orientation,unit_normal))<1e-8);
}
int main(){unsigned count{};const v::Pose root{{10,20,30},{0,0,.7071067811865475,.7071067811865475}};
 for(double gun_scale:{.5,1.,1.5})for(double hand_scale:{.7,1.,1.3})for(double units:{50.,100.,200.}){
  v::Pose support{};assert(m::support(root,gun_scale,hand_scale,units,support));check_palm(support,root,gun_scale,hand_scale,units,m::support_surface_raw,m::support_normal);++count;
 }
 for(bool closing:{false,true}){const v::Pose barrel{gears_weapon::boomshot_barrel::closed_raw,{}};v::Pose first{},moved{};
  assert(m::mechanism(barrel,.75,1.2,100,closing,first));auto shifted=barrel;shifted.position.y-=gears_weapon::boomshot_barrel::travel_raw;
  assert(m::mechanism(shifted,.75,1.2,100,closing,moved));assert(error(m::sub(moved.position,first.position),{0,-gears_weapon::boomshot_barrel::travel_raw,0})<1e-9);
  check_palm(first,barrel,.75,1.2,100,m::sub(m::mechanism_surface_closed_raw,gears_weapon::boomshot_barrel::closed_raw),m::mechanism_normal);++count;
 }
 {v::Pose a{},b{};assert(m::support({},1,.7,100,a)&&m::support({},1,1.3,100,b));assert(error(a.position,b.position)>1);
  v::Pose small{},large{};assert(m::support({},.5,1,100,small)&&m::support({},1.5,1,100,large));
  assert(error(m::sub(large.position,small.position),m::support_surface_raw)<1e-8);++count;}
 {v::Pose relative{},world{{20,45,80},root.orientation};assert(m::relative(root,world,100,relative));
  assert(error(m::point(root,relative.position,100),world.position)<1e-8);++count;}
 for(unsigned reason=0;reason<7;++reason){auto pose=root;double scale=1,body=1,units=100;
  switch(reason){case 0:scale=0;break;case 1:body=0;break;case 2:units=0;break;case 3:scale=11;break;case 4:body=11;break;case 5:units=501;break;default:pose.position.x=std::numeric_limits<double>::quiet_NaN();}
  v::Pose output{{123,456,789},{}};assert(!m::support(pose,scale,body,units,output)&&output.position.x==123);++count;
 }
 {v::Quaternion output{};assert(!m::palm_rotation({0,0,0},{0,1,0},output)&&!m::palm_rotation({0,1,0},{0,1,0},output));++count;}
 std::printf("Boomshot authored left palm mounts: %u cases passed; exact original support/mechanism surfaces, proper tangent frames, current body scale independent of gun scale, inverse Native grip-to-wrist and actual translating barrel; no skin/game fit acceptance\n",count);
}
