#include "cover_body_query_catalog.h"
#include <cstdio>
int main(){namespace c=gears_cover_body_query_catalog;namespace g=c::g;unsigned checks{},failed{};
#define C(x) do{++checks;if(!(x)){++failed;printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
 g::names::Mapping mapping{};mapping.count=1;mapping.geometry=g::body::rig::marcus115_signature;mapping.root_bone_index=2;
 mapping.rows[0].bone_index=2;mapping.rows[0].name={10,0};mapping.reference_names[2]={10,0};g::Inventory inventory{};inventory.count={1,0,0,0};
 g::Geometry geometry{};geometry.body_count=geometry.count=1;auto& shape=geometry.rows[0].primitive;
 shape.bone_index=2;shape.bone={10,0};shape.dimensions.kind=g::metrics::Kind::sphere;shape.dimensions.radius=2;shape.shape_world.position={10,20,30};
 c::Workspace work{};c::Catalog output{};C(c::prepare(mapping,&inventory,1,geometry,work,output,.001)==c::Code::prepared);
 const float factor=.02f;const auto& sphere=output.rows[0].shape.sphere;
 C(sphere.centre.x==10.f*factor&&sphere.centre.y==20.f*factor&&sphere.centre.z==30.f*factor);
 C(sphere.radius>=2.f*factor&&sphere.radius<.041f);
 printf("BodyQueryUnits checks=%u failures=%u original game-to-physics boundary only\n",checks,failed);return failed?1:0;
}
