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
 C(!GetModuleHandleW(L"PhysXCore.dll"));printf("PrimitiveQuery checks=%u failures=%u exact owned conversion only; no query/permission\n",checks,failures);return failures?1:0;
}
