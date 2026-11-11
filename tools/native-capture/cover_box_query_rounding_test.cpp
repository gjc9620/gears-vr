#include "cover_primitive_query.h"
#include <cstdio>
namespace c=gears_cover_primitive_query;namespace b=c::w::body;
int main(){unsigned checks=0,failures=0;
#define C(x) do{++checks;if(!(x)){++failures;if(failures<8)printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
 c::w::Primitive p{};p.body_index=0;p.bone_index=2;p.bone={10,0};p.dimensions.kind=c::w::metrics::Kind::box;p.dimensions.box_half_extents={1,2,3};
 c::Shape out{};
 for(unsigned n=1;n<=512;++n){const double t=double(n)/37.;auto q=b::Quat{.3*std::sin(t),.4*std::cos(t),.5*std::sin(2*t),1};
  const double norm=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);q={q.x/norm,q.y/norm,q.z/norm,q.w/norm};p.shape_world.orientation=q;
  p.shape_world.position={double(n)/7.,-double(n)/11.,double(n)/13.};
  C(c::convert_bounded(p,.001,out)==c::Code::converted);
  for(double expansion:out.box_half_expansion)C(expansion>=0&&expansion<=.001);
  const auto* m=out.box.rotation;const b::Vec x{m[0],m[3],m[6]},y{m[1],m[4],m[7]},z{m[2],m[5],m[8]};
  const auto cross=[](b::Vec a,b::Vec v){return b::Vec{a.y*v.z-a.z*v.y,a.z*v.x-a.x*v.z,a.x*v.y-a.y*v.x};};
  const auto dot=[](b::Vec a,b::Vec v){return a.x*v.x+a.y*v.y+a.z*v.z;};
  const auto ix=cross(y,z),iy=cross(z,x),iz=cross(x,y);const double det=dot(x,ix);C(det>0);
  const double h[]{out.box.half.x,out.box.half.y,out.box.half.z};const b::Vec axes[]{x,y,z},inverse[]{ix,iy,iz};
  for(unsigned corner=0;corner<8;++corner){const b::Vec local{corner&1?1.:-1.,corner&2?2.:-2.,corner&4?3.:-3.};
   const auto point=b::d::add(p.shape_world.position,b::d::rotate(q,local));
   const auto delta=b::d::sub(point,{out.box.centre.x,out.box.centre.y,out.box.centre.z});
   for(unsigned i=0;i<3;++i){C(std::abs(dot(inverse[i],delta)/det)<=h[i]+1e-12);C(std::abs(dot(axes[i],delta))<=h[i]+1e-12);}
  }
 }
 const auto saved=out;C(c::convert_bounded(p,0,out)!=c::Code::converted&&!memcmp(&saved,&out,sizeof out));
 C(c::convert_bounded(p,1e-15,out)!=c::Code::converted&&!memcmp(&saved,&out,sizeof out));
 p.shape_world.orientation={0,0,0,2};C(c::convert_bounded(p,.001,out)==c::Code::invalid&&!memcmp(&saved,&out,sizeof out));
 C(!GetModuleHandleW(L"PhysXCore.dll"));printf("BoxQueryRounding checks=%u failures=%u parameter enclosure only; no Native geometry proof\n",checks,failures);return failures?1:0;
}
