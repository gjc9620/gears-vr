#include "cover_primitive_query.h"
#include <cstdio>
namespace c=gears_cover_primitive_query;namespace w=c::w;
int main(){unsigned checks=0,failures=0;
#define C(x) do{++checks;if(!(x)){++failures;printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
 w::Primitive p{};p.body_index=2;p.bone_index=3;p.bone={4,0};c::Shape out{};
 p.dimensions.kind=w::metrics::Kind::sphere;p.dimensions.radius=2;p.shape_world.position={10,20,30};
 C(c::convert(p,out)==c::Code::converted&&out.sphere.centre.x==10&&out.sphere.radius==2);
 p.dimensions.kind=w::metrics::Kind::box;p.dimensions.box_half_extents={1,2,3};p.shape_world.orientation={0,0,0,1};
 C(c::convert(p,out)==c::Code::converted&&out.box.rotation[0]==1&&out.box.rotation[4]==1&&out.box.half.y==2);
 p.dimensions.kind=w::metrics::Kind::sphyl;p.segment_start={1,2,3};p.segment_end={1,2,7};
 C(c::convert(p,out)==c::Code::converted&&out.capsule.first.z==3&&out.capsule.second.z==7&&out.capsule.radius==2);
 C(out.body_index==2&&out.bone_index==3&&out.bone.index==4);
 const auto saved=out;
 p.segment_start.x=1./3.;C(c::convert(p,out)==c::Code::precision&&!memcmp(&saved,&out,sizeof out));
 p.dimensions.kind=w::metrics::Kind::convex;C(c::convert(p,out)==c::Code::unsupported&&!memcmp(&saved,&out,sizeof out));
 p.dimensions.kind=w::metrics::Kind::sphere;p.dimensions.radius=-1;C(c::convert(p,out)==c::Code::invalid&&!memcmp(&saved,&out,sizeof out));
 p.dimensions.radius=1;p.shape_world.position.x=1./3.;C(c::convert(p,out)==c::Code::precision&&!memcmp(&saved,&out,sizeof out));
 p.dimensions.kind=w::metrics::Kind::box;p.shape_world.position={};p.shape_world.orientation={0,0,0,2};
 C(c::convert(p,out)==c::Code::invalid&&!memcmp(&saved,&out,sizeof out));
 p.shape_world.orientation={0,0,1,0};
 C(c::convert(p,out)==c::Code::converted&&out.box.rotation[0]==-1&&out.box.rotation[4]==-1&&out.box.rotation[8]==1);
 const auto turned=out;p.bone_index=w::body::bones;
 C(c::convert(p,out)==c::Code::invalid&&!memcmp(&turned,&out,sizeof out));

 p.bone_index=3;p.dimensions.kind=w::metrics::Kind::sphere;p.dimensions.radius=1;
 for(unsigned i=1;i<=1024;++i){p.shape_world.position={double(i)/7.,-double(i)/11.,double(i)/13.};
  C(c::convert_bounded(p,.001,out)==c::Code::converted);
  const double dx=p.shape_world.position.x-out.sphere.centre.x,dy=p.shape_world.position.y-out.sphere.centre.y,dz=p.shape_world.position.z-out.sphere.centre.z;
  C(double(out.sphere.radius)>=1+std::sqrt(dx*dx+dy*dy+dz*dz)&&out.radius_expansion<=.001);
 }
 const auto bounded=out;C(c::convert_bounded(p,0,out)==c::Code::precision&&!memcmp(&out,&bounded,sizeof out));
 C(c::convert_bounded(p,-1,out)==c::Code::invalid&&!memcmp(&out,&bounded,sizeof out));
 p.dimensions.kind=w::metrics::Kind::sphyl;p.segment_start={1./3.,2./7.,3};p.segment_end={1./5.,2./9.,7};
 C(c::convert_bounded(p,.001,out)==c::Code::converted&&out.radius_expansion>0);
 const auto capped=out;C(c::convert_bounded(p,out.radius_expansion/2,out)==c::Code::precision&&!memcmp(&out,&capped,sizeof out));

 C(c::convert_bounded(p,.001,out)==c::Code::converted);
 for(unsigned i=0;i<=64;++i){const double t=double(i)/64.;
  const auto original=w::body::d::add(w::body::d::mul(p.segment_start,1-t),w::body::d::mul(p.segment_end,t));
  const w::Vec first{out.capsule.first.x,out.capsule.first.y,out.capsule.first.z},second{out.capsule.second.x,out.capsule.second.y,out.capsule.second.z};
  const auto rounded=w::body::d::add(w::body::d::mul(first,1-t),w::body::d::mul(second,t));
  C(w::body::d::length(w::body::d::sub(original,rounded))+p.dimensions.radius<=out.capsule.radius);
 }
 unsigned control{};C(_controlfp_s(&control,0,0)==0);unsigned prior=control;C(_controlfp_s(&control,_RC_DOWN,_MCW_RC)==0);
 const auto mode_saved=out;C(c::convert_bounded(p,.001,out)==c::Code::precision&&!memcmp(&out,&mode_saved,sizeof out));
 C(_controlfp_s(&control,prior&_MCW_RC,_MCW_RC)==0);
 C(!GetModuleHandleW(L"PhysXCore.dll"));printf("PrimitiveQuery checks=%u failures=%u bounded owned conversion only; no query/permission\n",checks,failures);return failures?1:0;
}
