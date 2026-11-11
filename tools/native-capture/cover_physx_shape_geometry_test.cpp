#include "cover_physx_shape_geometry.h"
#include <cstdio>
#include <limits>
namespace g=gears_cover_physx_shape_geometry;
static unsigned checks{},failures{};
#define CHECK(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %d: %s\n",__LINE__,#x);}}while(false)
static bool close(double a,double b){return std::abs(a-b)<2e-5;}
int main(){
    g::w::Snapshot source{};g::Geometry output{},sentinel{};sentinel.radius=123;
    for(unsigned axis=0;axis<3;++axis)for(unsigned n=0;n<128;++n){
      const double angle=double(n)*.037;const float s=float(std::sin(angle/2)),c=float(std::cos(angle/2));
      std::array<float,4> q{};q[axis]=s;q[3]=c;
      CHECK(g::w::math::quaternion(q,{10,-20,30},source.shape_world));
      // Independent Rodrigues rotation, with a positive unit coordinate axis.
      auto rotate=[&](std::array<double,3> v){auto result=v;const unsigned a=(axis+1)%3,b=(axis+2)%3;
        result[a]=std::cos(angle)*v[a]-std::sin(angle)*v[b];result[b]=std::sin(angle)*v[a]+std::cos(angle)*v[b];return result;};
      source.parameters.owner.kind=g::Kind::box;source.parameters.box_dimensions={2,3,4};
      CHECK(g::convert(source,output)==g::Code::converted);
      CHECK(output.half[0]==2&&output.half[1]==3&&output.half[2]==4&&output.centre[0]==10);
      // Each original local +/-half corner, without a second scale conversion.
      for(int x:{-1,1})for(int y:{-1,1})for(int z:{-1,1}){
        const double local[]={double(x)*2,double(y)*3,double(z)*4};double actual[3]{};
        for(unsigned r=0;r<3;++r){actual[r]=output.centre[r];for(unsigned k=0;k<3;++k)actual[r]+=output.orientation[3*r+k]*local[k];}
        const auto expected=rotate({local[0],local[1],local[2]});
        for(unsigned k=0;k<3;++k)CHECK(close(actual[k],output.centre[k]+expected[k]));
      }
      source.parameters.owner.kind=g::Kind::capsule;source.parameters.radius=2;source.parameters.half_segment=3;
      CHECK(g::convert(source,output)==g::Code::converted&&output.radius==2);
      const auto offset=rotate({0,3,0});
      for(unsigned k=0;k<3;++k)CHECK(close(output.endpoint_b[k],output.centre[k]+offset[k]));
      for(unsigned k=0;k<3;++k)CHECK(close(output.endpoint_a[k]+output.endpoint_b[k],2*output.centre[k]));
      source.parameters.owner.kind=g::Kind::sphere;
      CHECK(g::convert(source,output)==g::Code::converted&&output.radius==2&&output.centre[2]==30);
      CHECK((output.endpoint_a==std::array<double,3>{}&&output.half==std::array<double,3>{}));
    }
    source.parameters.owner.kind=g::Kind::capsule;source.parameters.half_segment=0;
    CHECK(g::convert(source,output)==g::Code::converted&&output.endpoint_a==output.endpoint_b);
    for(float bad:{-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::denorm_min()}){
      output=sentinel;source.parameters.radius=bad;CHECK(g::convert(source,output)==g::Code::value&&output.radius==123);
      source.parameters.radius=2;source.parameters.half_segment=bad;CHECK(g::convert(source,output)==g::Code::value&&output.radius==123);
      source.parameters.half_segment=3;source.parameters.owner.kind=g::Kind::box;source.parameters.box_dimensions={2,bad,4};
      CHECK(g::convert(source,output)==g::Code::value&&output.radius==123);source.parameters.owner.kind=g::Kind::capsule;
    }
    output=sentinel;source.parameters.owner.kind=g::Kind::convex;CHECK(g::convert(source,output)==g::Code::unsupported&&output.radius==123);
    source.parameters.owner.kind=g::Kind::sphere;source.shape_world.values[0]=2;
    CHECK(g::convert(source,output)==g::Code::pose&&output.radius==123);
    CHECK(g::convert(source,*reinterpret_cast<g::Geometry*>(&source))==g::Code::alias);
    std::printf("ActiveShapeGeometry checks=%u failures=%u owned PhysX geometry only; no authored correspondence or authority\n",checks,failures);return failures?1:0;
}
