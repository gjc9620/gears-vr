#pragma once
#include "cover_physx_shape_owner.h"
#include "cover_physx_asset_actors.h"
#include "cover_physx_collider.h"
#include "cover_physx_components.h"

// Only for snapshots already closed by the query's typed C0 guards. This is
// evidence for individual returned shapes, never a complete self inventory,
// world/lifetime fence, negative collision result or movement permission.
namespace gears_cover_physx_self_evidence {
namespace s=gears_cover_physx_query_source;
namespace own=gears_cover_physx_shape_owner;
namespace cx=gears_cover_physx_collider;
namespace parts=gears_cover_physx_components;
namespace asset=gears_cover_physx_asset_actors;
enum Source:unsigned char {none=0,collider=1,pawn_component=2,pawn_asset=4};
inline bool qualified(asset::Snapshot const& bodies,cx::Snapshot const& body,parts::Snapshot const& catalog)noexcept{
    auto const& scope=bodies.query_source;
    return s::same(scope,body.query_source)&&s::same(scope,catalog.query_source)&&
        s::pointer(body.fields.pawn,88)&&parts::consistent(catalog)&&
        catalog.fields.roots[0].actor==body.fields.pawn&&
        bodies.body_count==unsigned(scope.bodies.count)&&bodies.body_count<=asset::capacity;
}
// Invalid/foreign/conflicting evidence always leaves the shape unexcluded.
// Caller qualifies the sources once per result, outside the per-shape_owner loop.
inline unsigned char match(own::Observation const& shape_owner,asset::Snapshot const& bodies,
    cx::Snapshot const& body,parts::Snapshot const& catalog)noexcept{
    auto const& scope=bodies.query_source;
    if(bodies.body_count>asset::capacity||catalog.fields.component_count>parts::capacity||
       catalog.fields.asset_body_count>parts::asset_body_capacity)return none;
    if(shape_owner.kind==own::Kind::unknown||!s::pointer(shape_owner.shape,0x40)||!s::pointer(shape_owner.internal_shape,0xd8)||
       (shape_owner.internal_actor&&!s::pointer(shape_owner.internal_actor,0x18))||shape_owner.scene!=scope.scene||shape_owner.actor_table!=scope.actor_table)return none;
    const auto pawn=body.fields.pawn;
    unsigned char found=none;bool conflict=false;
    auto actor_matches=[&](std::uintptr_t candidate_actor,std::uintptr_t candidate_table,std::uintptr_t candidate_scene,std::uintptr_t candidate_internal){
        return shape_owner.actor==candidate_actor&&shape_owner.actor_table==candidate_table&&shape_owner.scene==candidate_scene&&shape_owner.internal_actor==candidate_internal;};
    if(body.presence==cx::Presence::actor_observed&&body.fields.body_data==shape_owner.actor){
        if(actor_matches(body.fields.body_data,body.actor_table,body.scene,body.internal))found|=collider;
        else conflict=true;
    }
    bool mesh_owned=false;
    for(unsigned n=0;n<catalog.fields.component_count;++n){
        auto const& row=catalog.fields.components[n];auto const& observed=catalog.observations[n];
        if(row.component==scope.source.mesh&&row.owner==pawn)mesh_owned=true;
        if(row.body_data!=shape_owner.actor)continue;
        if(row.owner==pawn&&row.body_owner==row.component&&row.body_owner_actor==pawn&&
           observed.presence==parts::Presence::actor_observed&&
           actor_matches(row.body_data,observed.table,observed.scene,observed.internal))found|=pawn_component;
        else conflict=true;
    }
    // The main asset is explicitly associated with the current Pawn mesh by
    // the existing typed guard. Also require its catalog owner, exact body
    // row, scene and internal actor; array/actor membership alone is insufficient.
    for(unsigned n=0;n<bodies.body_count;++n){auto const& row=bodies.bodies[n];
        if(row.actor!=shape_owner.actor)continue;
        if(mesh_owned&&row.body==scope.members[n]&&row.presence==asset::Presence::actor_observed&&
           actor_matches(row.actor,row.table,row.scene,row.internal))found|=pawn_asset;
        else conflict=true;
    }
    for(unsigned n=0;n<catalog.fields.asset_body_count;++n){
        auto const& row=catalog.fields.asset_bodies[n];auto const& observed=catalog.asset_observations[n];
        if(row.data!=shape_owner.actor)continue;
        bool component_owned=false;
        for(unsigned c=0;c<catalog.fields.component_count;++c){auto const& component=catalog.fields.components[c];
            auto const& component_asset=catalog.fields.assets[c];
            if(component.component==row.owner&&component.owner==pawn&&component_asset.sampled&&
               component_asset.owner==pawn&&n>=component_asset.first_body&&
               n-component_asset.first_body<unsigned(component_asset.array.count))component_owned=true;}
        if(component_owned&&row.owner_actor==pawn&&observed.presence==parts::Presence::actor_observed&&
           actor_matches(row.data,observed.table,observed.scene,observed.internal))found|=pawn_component;
        else conflict=true;
    }
    return conflict?static_cast<unsigned char>(none):found;
}
}
