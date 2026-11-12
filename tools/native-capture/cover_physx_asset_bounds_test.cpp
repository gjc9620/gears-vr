#include "cover_physx_asset_bounds.h"
#include <memory>
#include <cstdio>
namespace g=gears_cover_physx_asset_bounds;
static unsigned checks{},failures{};
#define CHECK(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %d %s\n",__LINE__,#x);}}while(false)
static void fill(g::a::Snapshot& s,unsigned actors,unsigned shapes){
  s={};s.body_actor.fill(~0u);s.assets.query_source.scene=0x10000;s.assets.body_count=s.assets.actor_count=actors;
  for(unsigned i=0;i<actors;++i){
    const std::uintptr_t actor=0x20000+i*0x1000,body=0x100000+i*0x1000;
    s.assets.actors[i]=actor;s.body_actor[i]=i;auto& b=s.assets.bodies[i];b.presence=g::a::asset::Presence::actor_observed;
    b.body=0x1000000+i*0x1000;b.actor=actor;b.internal=body;b.scene=s.assets.query_source.scene;
    auto& group=s.actors[i];group.first=s.count;group.count=shapes;group.membership.actor=actor;group.membership.body=body;group.membership.scene=b.scene;group.membership.count=shapes;
    for(unsigned j=0;j<shapes;++j){const auto index=s.count++;auto& row=s.rows[index];row.actor_index=i;row.member_ordinal=j;
      auto& w=row.geometry;auto& owner=w.parameters.owner;owner.actor=actor;owner.internal_actor=body;owner.scene=b.scene;
      owner.shape=0x2000000+index*0x100;owner.internal_shape=0x3000000+index*0x100;owner.shape_table=0x4000000;
      owner.kind=j%3==0?g::b::Kind::box:j%3==1?g::b::Kind::capsule:g::b::Kind::sphere;
      auto& m=group.membership.rows[j];m.public_shape=owner.shape;m.internal=owner.internal_shape;m.table=owner.shape_table;
      w.parameters.stored_quaternion={0,0,0,1};w.parameters.stored_position={float(index)*2,0,0};w.parameters.box_dimensions={2,3,4};w.parameters.radius=1;w.parameters.half_segment=2;
      w.body_pose_present=true;w.body_quaternion={0,0,0,1};w.body_position={0,20,0};g::b::p::Pose local{},world{},a{};
      g::b::p::quaternion(w.parameters.stored_quaternion,w.parameters.stored_position,local);g::b::p::quaternion(w.body_quaternion,w.body_position,a);g::b::p::compose(a,local,world);
      w.actor_world=a.matrix;w.actor_gram_error=a.gram_error;w.shape_world=world.matrix;w.shape_gram_error=world.gram_error;
    }
  }
}
int main(){
 auto base=std::make_unique<g::a::Snapshot>();auto bad=std::make_unique<g::a::Snapshot>();g::Snapshot out{},sentinel{};sentinel.count=123;
 for(unsigned actors:{0u,1u,2u,64u})for(unsigned shapes:{0u,1u,4u}){
   fill(*base,actors,shapes);CHECK(g::convert(*base,out)==g::Code::bounded);CHECK(out.count==actors*shapes&&out.has_shapes==(out.count!=0));
   for(unsigned i=0;i<out.count;++i){CHECK(out.rows[i].shape==base->rows[i].geometry.parameters.owner.shape);
     for(unsigned axis=0;axis<3;++axis){CHECK(out.aggregate.lower[axis]<=out.rows[i].bounds.lower[axis]);CHECK(out.aggregate.upper[axis]>=out.rows[i].bounds.upper[axis]);}}
 }
 fill(*base,2,3);
 auto reject=[&](auto edit,g::Code expected){*bad=*base;edit(*bad);out=sentinel;CHECK(g::convert(*bad,out)==expected&&out.count==123);};
 reject([](auto& s){s.count=257;},g::Code::capacity);reject([](auto& s){s.assets.body_count=65;},g::Code::capacity);reject([](auto& s){s.assets.actor_count=65;},g::Code::capacity);
 reject([](auto& s){s.actors[1].count=65;s.actors[1].membership.count=65;},g::Code::capacity);
 reject([](auto& s){s.actors[1].count=4;s.actors[1].membership.count=4;},g::Code::capacity);
 reject([](auto& s){s.count=7;},g::Code::links);reject([](auto& s){s.actors[1].first=0;},g::Code::links);
 reject([](auto& s){s.body_actor[1]=0;},g::Code::links);reject([](auto& s){s.assets.bodies[1].internal=0;},g::Code::links);
 reject([](auto& s){s.assets.bodies[1].presence=g::a::asset::Presence::not_sampled;},g::Code::links);
 reject([](auto& s){s.assets.actor_count=1;},g::Code::links);
 reject([](auto& s){s.rows[5].actor_index=0;},g::Code::links);reject([](auto& s){s.rows[5].member_ordinal=0;},g::Code::links);
 reject([](auto& s){s.rows[5].geometry.parameters.owner.scene=0;},g::Code::links);
 reject([](auto& s){s.rows[5].geometry.parameters.owner.internal_shape=0;},g::Code::links);
 reject([](auto& s){s.rows[5].geometry.parameters.owner.shape=s.rows[0].geometry.parameters.owner.shape;s.actors[1].membership.rows[2].public_shape=s.rows[0].geometry.parameters.owner.shape;},g::Code::links);
 reject([](auto& s){s.rows[5].geometry.shape_world.values[9]+=1;},g::Code::geometry);
 reject([](auto& s){s.rows[5].geometry.parameters.radius=-1;},g::Code::geometry);
 // Explicit shared-body mapping, a null BodyData, and a referenced empty actor.
 base->assets.body_count=4;base->assets.bodies[2]=base->assets.bodies[0];base->body_actor[2]=0;
 base->assets.bodies[3].presence=g::a::asset::Presence::body_data_field_null;base->body_actor[3]=~0u;
 CHECK(g::convert(*base,out)==g::Code::bounded&&out.count==6);
 reject([](auto& s){s.body_actor[3]=0;},g::Code::links);
 fill(*base,2,0);CHECK(g::convert(*base,out)==g::Code::bounded&&!out.has_shapes);
 CHECK(g::convert(*base,*reinterpret_cast<g::Snapshot*>(base.get()))==g::Code::alias);
 std::printf("AssetBounds checks=%u failures=%u complete supplied mathematical bounds; no live/self/world authority\n",checks,failures);return failures?1:0;
}
