#include "cover_body_query_catalog.h"
#include <cstdio>
#include <memory>
namespace c=gears_cover_body_query_catalog;namespace g=c::g;
int main(){unsigned checks=0,failures=0;
#define C(x) do{++checks;if(!(x)){++failures;printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
 g::names::Mapping map{};map.count=2;map.geometry=g::body::rig::marcus115_signature;map.root_body_index=0;map.root_bone_index=2;
 for(unsigned i=0;i<2;++i){map.rows[i].body_index=i;map.rows[i].bone_index=i+2;map.rows[i].name={int(i+10),0};map.reference_names[i+2]=map.rows[i].name;}
 std::array<g::Inventory,2> inventory{};auto geom=std::make_unique<g::Geometry>();geom->body_count=2;geom->count=6;
 for(unsigned b=0;b<2;++b){inventory[b].count={1,1,1,0};for(unsigned k=0;k<3;++k){auto& s=geom->rows[3*b+k].primitive;
  s.body_index=b;s.bone_index=b+2;s.bone=map.rows[b].name;s.dimensions.kind=static_cast<g::metrics::Kind>(k);
  s.dimensions.radius=1;s.dimensions.box_half_extents={1,2,3};s.shape_world.position={double(b),0,0};s.shape_world.orientation={0,0,0,1};s.segment_start={0,0,0};s.segment_end={0,0,2};}}
 auto work=std::make_unique<c::Workspace>();auto output=std::make_unique<c::Catalog>();
 C(c::prepare(map,inventory.data(),2,*geom,*work,*output)==c::Code::prepared&&output->count==6&&output->body_count==2);
 for(unsigned i=0;i<6;++i)C(output->rows[i].shape.body_index==i/3&&output->rows[i].shape.bone_index==i/3+2&&unsigned(output->rows[i].shape.kind)==i%3);
 const auto saved=std::make_unique<c::Catalog>(*output);
 for(unsigned bad=0;bad<5;++bad){auto changed=std::make_unique<g::Geometry>(*geom);auto counts=inventory;
  if(bad==0)--changed->count;if(bad==1)changed->rows[5].array_index=1;if(bad==2)changed->rows[5].primitive.bone_index=7;
  if(bad==3)changed->rows[5].primitive.segment_end.x=1./3.;if(bad==4)counts[1].count[3]=1;
  C(c::prepare(map,counts.data(),2,*changed,*work,*output)!=c::Code::prepared&&!memcmp(saved.get(),output.get(),sizeof *output));}
 C(c::prepare(map,inventory.data(),2,*geom,*work,work->candidate)==c::Code::alias);
 C(!GetModuleHandleW(L"PhysXCore.dll"));printf("BodyQueryCatalog checks=%u failures=%u supplied inventory only; no world safety/permit\n",checks,failures);return failures?1:0;
}
