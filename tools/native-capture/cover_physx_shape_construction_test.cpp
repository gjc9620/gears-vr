#include "cover_physx_shape_construction.h"
#include <cstdio>
#include <limits>
namespace c=gears_cover_physx_shape_construction;
static unsigned checks{},failures{};
#define CHECK(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %d %s\n",__LINE__,#x);}}while(false)
using V=std::array<double,3>;
static V rotate(unsigned axis,double a,V v){const auto x=(axis+1)%3,y=(axis+2)%3;V next=v;
 next[x]=std::cos(a)*v[x]-std::sin(a)*v[y];next[y]=std::sin(a)*v[x]+std::cos(a)*v[y];return next;}
static c::Matrix matrix(unsigned axis,double a,std::array<float,3> position){c::Matrix m{};
 for(unsigned column=0;column<3;++column){V v{};v[column]=1;v=rotate(axis,a,v);for(unsigned r=0;r<3;++r)m.values[3*r+column]=float(v[r]);}
 for(unsigned k=0;k<3;++k)m.values[9+k]=position[k];return m;}
static V quaternion_rotate(std::array<float,4> q,V v){V u{q[0],q[1],q[2]};
 V cross{u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]};
 V twice{u[1]*cross[2]-u[2]*cross[1],u[2]*cross[0]-u[0]*cross[2],u[0]*cross[1]-u[1]*cross[0]};
 for(unsigned k=0;k<3;++k)v[k]+=2*q[3]*cross[k]+2*twice[k];return v;}
int main(){c::creation::Descriptor descriptor{};c::Stored output{},sentinel{};sentinel.radius=123;
 for(unsigned axis=0;axis<3;++axis)for(unsigned n=0;n<128;++n){const double angle=n*.037,parent_angle=n*.021;
  descriptor.local=matrix(axis,angle,{1,2,3});descriptor.half={2,3,4};descriptor.radius=2;descriptor.full_segment_length=6;
  const auto actor_pose=matrix((axis+1)%3,parent_angle,{10,-20,30});
  for(auto placement:{c::Placement::dynamic_local,c::Placement::static_world})for(auto kind:{c::Kind::sphere,c::Kind::box,c::Kind::capsule}){
   descriptor.kind=kind;CHECK(c::convert(descriptor,placement,actor_pose,output)==c::Code::converted);
   CHECK(kind==c::Kind::box?output.half==descriptor.half:output.radius==2);
   if(kind==c::Kind::capsule)CHECK(output.half_segment==3);
   for(V point:{V{0,0,0},V{2,-3,4},V{0,3,0},V{0,-3,0}}){auto expected=rotate(axis,angle,point);
    for(unsigned k=0;k<3;++k)expected[k]+=k+1;
    if(placement==c::Placement::static_world){expected=rotate((axis+1)%3,parent_angle,expected);for(unsigned k=0;k<3;++k)expected[k]+=actor_pose.values[9+k];}
    auto actual=quaternion_rotate(output.quaternion,point);for(unsigned k=0;k<3;++k){actual[k]+=output.position[k];CHECK(std::abs(actual[k]-expected[k])<3e-5);}
   }
  }
 }
 descriptor.kind=c::Kind::capsule;descriptor.local=matrix(0,0,{1,2,3});auto actor_pose=matrix(0,0,{10,-20,30});
 descriptor.full_segment_length=0;CHECK(c::convert(descriptor,c::Placement::dynamic_local,actor_pose,output)==c::Code::converted&&output.half_segment==0);
 for(float bad:{-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::denorm_min(),std::numeric_limits<float>::min()}){
  descriptor.full_segment_length=bad;output=sentinel;CHECK(c::convert(descriptor,c::Placement::dynamic_local,actor_pose,output)==c::Code::value&&output.radius==123);}
 descriptor.full_segment_length=6;descriptor.local.values[0]=2;CHECK(c::convert(descriptor,c::Placement::dynamic_local,actor_pose,output)==c::Code::pose&&output.radius==123);
 descriptor.local=matrix(0,0,{1,2,3});actor_pose.values[0]=2;
 CHECK(c::convert(descriptor,c::Placement::static_world,actor_pose,output)==c::Code::pose&&output.radius==123);
 CHECK(c::convert(descriptor,c::Placement::dynamic_local,actor_pose,output)==c::Code::converted);
 output=sentinel;descriptor.kind=c::Kind::convex;CHECK(c::convert(descriptor,c::Placement::dynamic_local,actor_pose,output)==c::Code::request&&output.radius==123);
 descriptor.kind=c::Kind::capsule;CHECK(c::convert(descriptor,c::Placement(99),actor_pose,output)==c::Code::request);
 descriptor.kind=c::Kind::sphere;descriptor.radius=std::numeric_limits<float>::quiet_NaN();CHECK(c::convert(descriptor,c::Placement::dynamic_local,actor_pose,output)==c::Code::value&&output.radius==123);
 descriptor.radius=2;descriptor.kind=c::Kind::box;descriptor.half[1]=0;CHECK(c::convert(descriptor,c::Placement::dynamic_local,actor_pose,output)==c::Code::value&&output.radius==123);descriptor.half[1]=3;
 descriptor.kind=c::Kind::capsule;
 CHECK(c::convert(descriptor,c::Placement::dynamic_local,actor_pose,*reinterpret_cast<c::Stored*>(&descriptor))==c::Code::alias);
 const auto old_round=std::fegetround();CHECK(std::fesetround(FE_UPWARD)==0);const auto rounding=c::convert(descriptor,c::Placement::dynamic_local,actor_pose,output);
 CHECK(std::fesetround(old_round)==0);CHECK(rounding==c::Code::rounding&&output.radius==123);
 std::printf("ShapeConstruction checks=%u failures=%u original stored representation math only; no active identity\n",checks,failures);return failures?1:0;
}
