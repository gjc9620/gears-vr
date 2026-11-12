#pragma once
#include "cover_physx_asset_shapes.h"
#include "cover_physx_observed_bounds.h"
// Complete supplied asset's mathematical broad-phase ranges. This consumes
// an owned observation; it neither refreshes it nor certifies a whole self set.
namespace gears_cover_physx_asset_bounds {
namespace a=gears_cover_physx_asset_shapes;
namespace b=gears_cover_physx_observed_bounds;
enum class Code {bounded,alias,capacity,links,geometry};
struct Row {unsigned actor{},member{};std::uintptr_t shape{};b::Bounds bounds{};};
struct Snapshot {unsigned count{};bool has_shapes{};b::Bounds aggregate{};std::array<Row,a::capacity> rows{};};
inline Code convert(a::Snapshot const& source,Snapshot& output)noexcept{
    if(a::s::aliases(&source,sizeof source,&output,sizeof output))return Code::alias;
    const auto& asset=source.assets;
    if(asset.body_count>64||asset.actor_count>64||source.count>a::capacity)return Code::capacity;
    for(unsigned i=0;i<asset.body_count;++i){const auto& body=asset.bodies[i];const auto actor=source.body_actor[i];
      if(body.presence==a::asset::Presence::body_data_field_null){if(body.actor||actor!=~0u)return Code::links;}
      else if(body.presence!=a::asset::Presence::actor_observed||actor>=asset.actor_count||
              !body.actor||body.actor!=asset.actors[actor]||body.scene!=asset.query_source.scene||
              body.internal!=source.actors[actor].membership.body)return Code::links;
    }
    Snapshot next{};unsigned index{};
    for(unsigned i=0;i<asset.actor_count;++i){const auto& actor=source.actors[i];const auto& members=actor.membership;
      if(!asset.actors[i]||members.actor!=asset.actors[i]||members.scene!=asset.query_source.scene||
         actor.first!=index||actor.count!=members.count)return Code::links;
      if(actor.count>a::shapes::capacity||actor.count>source.count-index)return Code::capacity;
      bool referenced=false;for(unsigned j=0;j<asset.body_count;++j)referenced|=source.body_actor[j]==i;
      if(!referenced)return Code::links;
      for(unsigned j=0;j<i;++j)if(asset.actors[j]==asset.actors[i])return Code::links;
      for(unsigned j=0;j<actor.count;++j,++index){const auto& row=source.rows[index];const auto& owner=row.geometry.parameters.owner;
        if(row.actor_index!=i||row.member_ordinal!=j||!owner.shape||!owner.internal_shape||
           !a::detail::member(owner,members,j))return Code::links;
        for(unsigned k=0;k<index;++k){const auto& old=source.rows[k].geometry.parameters.owner;
          if(old.shape==owner.shape||old.internal_shape==owner.internal_shape)return Code::links;}
        auto& out=next.rows[index];out.actor=i;out.member=j;out.shape=owner.shape;
        if(b::observe(row.geometry,out.bounds)!=b::Code::bounded)return Code::geometry;
        if(!index)next.aggregate=out.bounds;
        else for(unsigned axis=0;axis<3;++axis){
          next.aggregate.lower[axis]=std::min(next.aggregate.lower[axis],out.bounds.lower[axis]);
          next.aggregate.upper[axis]=std::max(next.aggregate.upper[axis],out.bounds.upper[axis]);
        }
      }
    }
    if(index!=source.count)return Code::links;
    next.count=index;next.has_shapes=index!=0;output=next;return Code::bounded;
}
}
