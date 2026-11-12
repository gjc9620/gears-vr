#include "cover_physx_observed_bounds.h"
#include <cfenv>
#include <cstdio>
#include <limits>
namespace b=gears_cover_physx_observed_bounds;
static unsigned checks{},failures{};
#define CHECK(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %d %s\n",__LINE__,#x);}}while(false)
static bool contains(b::Bounds const& a,unsigned axis,long double x){return a.lower[axis]<=x&&x<=a.upper[axis];}
int main(){
  b::Bounds out{},sentinel{};sentinel.lower={123,456,789};
  for(unsigned axis=0;axis<3;++axis)for(unsigned n=0;n<192;++n){
    const double angle=n*.037,norm=1+(int(n%7)-3)*.00001;
    b::w::Snapshot s{};auto& q=s.parameters.stored_quaternion;q[axis]=float(std::sin(angle/2)*norm);q[3]=float(std::cos(angle/2)*norm);
    s.parameters.stored_position={n%2?1e20f:10.f,-20,30};
    b::p::Pose pose{};CHECK(b::p::quaternion(q,s.parameters.stored_position,pose));
    s.shape_world=pose.matrix;s.shape_gram_error=pose.gram_error;
    s.parameters.box_dimensions={2,3,4};s.parameters.radius=2;s.parameters.half_segment=3;
    for(auto kind:{b::Kind::box,b::Kind::capsule,b::Kind::sphere}){
      s.parameters.owner.kind=kind;CHECK(b::observe(s,out)==b::Code::bounded);
      const auto* m=s.shape_world.values;
      // Independent long-double enumeration of all box corners, or capsule
      // segment endpoints plus the two extremal sphere points on each axis.
      for(unsigned r=0;r<3;++r){
        if(kind==b::Kind::box){for(int x:{-1,1})for(int y:{-1,1})for(int z:{-1,1}){
          const long double p=(long double)m[9+r]+(long double)m[3*r]*x*2+(long double)m[3*r+1]*y*3+(long double)m[3*r+2]*z*4;
          CHECK(contains(out,r,p));
        }}else for(int side:{-1,1})for(int end:{-1,1}){
          const long double p=(long double)m[9+r]+side*2+(kind==b::Kind::capsule?(long double)m[3*r+1]*end*3:0);
          CHECK(contains(out,r,p));
        }
      }
    }
    auto reject=[&](){out=sentinel;CHECK(b::observe(s,out)==b::Code::pose&&out.lower==sentinel.lower);};
    s.shape_world.values[0]=std::nextafter(s.shape_world.values[0],2.f);reject();s.shape_world=pose.matrix;
    s.shape_gram_error=0;reject();s.shape_gram_error=pose.gram_error;
    s.body_pose_present=true;reject();
    s.parameters.owner.internal_actor=1;s.body_quaternion={0,0,0,1};s.body_position={2,3,4};
    b::p::Pose actor{},world{};CHECK(b::p::quaternion(s.body_quaternion,s.body_position,actor));CHECK(b::p::compose(actor,pose,world));
    s.actor_world=actor.matrix;s.actor_gram_error=actor.gram_error;s.shape_world=world.matrix;s.shape_gram_error=world.gram_error;
    CHECK(b::observe(s,out)==b::Code::bounded);s.actor_gram_error=0;reject();
  }
  b::p::Pose pose{};CHECK(b::p::quaternion({0,0,0,1},{0,0,0},pose));
  for(float bad:{0.f,-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::denorm_min()}){
    out=sentinel;CHECK(b::primitive(pose,b::Kind::box,{1,bad,1},1,1,out)==b::Code::value&&out.lower==sentinel.lower);
    CHECK(b::primitive(pose,b::Kind::sphere,{},bad,1,out)==b::Code::value&&out.lower==sentinel.lower);
  }
  CHECK(b::primitive(pose,b::Kind::capsule,{},1,0,out)==b::Code::bounded);
  out=sentinel;CHECK(b::primitive(pose,b::Kind::capsule,{},1,-1,out)==b::Code::value&&out.lower==sentinel.lower);
  CHECK(b::primitive(pose,b::Kind::convex,{},1,1,out)==b::Code::unsupported&&out.lower==sentinel.lower);
  pose.matrix.values[0]=-1;pose.gram_error=b::p::gram_bound(pose.matrix);
  CHECK(b::primitive(pose,b::Kind::box,{1,1,1},1,1,out)==b::Code::pose&&out.lower==sentinel.lower);
  CHECK(b::p::quaternion({0,0,0,1},{0,0,0},pose));
  const int mode=std::fegetround();CHECK(std::fesetround(FE_UPWARD)==0);
  CHECK(b::primitive(pose,b::Kind::sphere,{},1,1,out)==b::Code::rounding&&out.lower==sentinel.lower);CHECK(std::fesetround(mode)==0);
  b::w::Snapshot s{};CHECK(b::observe(s,*reinterpret_cast<b::Bounds*>(&s))==b::Code::alias);
  std::printf("ObservedBounds checks=%u failures=%u mathematical primitive enclosure only; no live/query authority\n",checks,failures);
  return failures?1:0;
}
