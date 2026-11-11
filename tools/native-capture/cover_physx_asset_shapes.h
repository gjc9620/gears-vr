#pragma once
#include "cover_physx_asset_actors.h"
#include "cover_physx_actor_shapes.h"
#include "cover_physx_actor_shape_code.h"
#include "cover_physx_shape_world.h"
// Complete supplied PhysicsAsset BodyData actor set, not the whole player/self
// or scene. Every actor member is retained once. Shared BodyData references
// remain explicit; no shape-to-authored-bone assignment is inferred from order.
namespace gears_cover_physx_asset_shapes {
namespace asset=gears_cover_physx_asset_actors;namespace shapes=gears_cover_physx_actor_shapes;
namespace world=gears_cover_physx_shape_world;namespace s=gears_cover_physx_query_source;
constexpr unsigned capacity=256;
struct Actor {shapes::Snapshot membership{};unsigned first{},count{};};
struct Row {unsigned actor_index{},member_ordinal{};world::Snapshot geometry{};};
struct Snapshot {asset::Snapshot assets{};std::array<unsigned,64> body_actor{};std::array<Actor,64> actors{};
    unsigned count{};std::array<Row,capacity> rows{};};
enum class Code {observed,request,alias,busy,guard,module,asset,shapes,world,links,capacity,changed,read};
struct Workspace {Snapshot first{},last{};asset::Workspace asset_work{};asset::Snapshot closing_assets{};
    shapes::Workspace shape_work{};shapes::Snapshot closing_shapes{};world::Workspace world_work{};
    std::atomic<bool> active{};unsigned failed_actor{~0u},failed_member{~0u};
    asset::Code asset_code{};shapes::Code shape_code{};world::Code world_code{};};
inline bool same(Snapshot const& a,Snapshot const& b)noexcept{
    if(a.count>capacity||a.count!=b.count||a.body_actor!=b.body_actor||!asset::same(a.assets,b.assets))return false;
    for(unsigned n=0;n<64;++n)if(a.actors[n].first!=b.actors[n].first||a.actors[n].count!=b.actors[n].count||
       !shapes::same(a.actors[n].membership,b.actors[n].membership))return false;
    for(unsigned n=0;n<a.count;++n)if(a.rows[n].actor_index!=b.rows[n].actor_index||a.rows[n].member_ordinal!=b.rows[n].member_ordinal||
       !world::same(a.rows[n].geometry,b.rows[n].geometry))return false;return true;
}
namespace detail {
template<class Read> bool code_current(std::uintptr_t image_base,Read& read){return gears_cover_physx_actor_shape_code::current(image_base,read)&&world::detail::code_current(image_base,read);}
inline bool member(world::o::Observation const& owner,shapes::Snapshot const& group,unsigned n)noexcept{
    return n<group.count&&owner.shape==group.rows[n].public_shape&&owner.internal_shape==group.rows[n].internal&&
      owner.shape_table==group.rows[n].table&&owner.actor==group.actor&&owner.scene==group.scene&&owner.internal_actor==group.body;
}
template<class Read,class Guard> Code pass(std::uintptr_t image_base,std::uint32_t size,s::Snapshot const& scope,
    Read read,Guard& guard,Workspace& work,Snapshot& out){
    out={};out.body_actor.fill(~0u);
    work.asset_code=asset::observe(image_base,size,scope,read,guard,work.asset_work,out.assets);
    if(work.asset_code!=asset::Code::observed)return Code::asset;
    if(out.assets.body_count>64||out.assets.actor_count>64)return Code::links;
    for(unsigned n=0;n<out.assets.body_count;++n){const auto& body=out.assets.bodies[n];if(!body.actor)continue;
      unsigned matches{};for(unsigned k=0;k<out.assets.actor_count;++k)if(out.assets.actors[k]==body.actor){out.body_actor[n]=k;++matches;}
      if(matches!=1)return Code::links;}
    for(unsigned index=0;index<out.assets.actor_count;++index){work.failed_actor=index;work.failed_member=~0u;auto& actor=out.actors[index];
      auto admitted=[&](){return guard()&&guard(out.assets)&&s::same(scope,out.assets.query_source)&&detail::code_current(image_base,read);};
      work.shape_code=shapes::observe(image_base,size,out.assets.actors[index],scope.scene,read,admitted,work.shape_work,actor.membership);
      if(work.shape_code!=shapes::Code::observed)return Code::shapes;
      actor.first=out.count;actor.count=actor.membership.count;
      if(actor.count>capacity-out.count)return Code::capacity;
      for(unsigned n=0;n<actor.count;++n){work.failed_member=n;auto& row=out.rows[out.count];row.actor_index=index;row.member_ordinal=n;
        world::o::Observation attribution{};
        if(world::o::detail::sample(image_base,size,actor.membership.rows[n].public_shape,scope.scene,read,attribution)!=world::o::Code::observed||
           !member(attribution,actor.membership,n))return Code::links;
        for(unsigned earlier=0;earlier<out.count;++earlier){const auto& old=out.rows[earlier].geometry.parameters.owner;
          if(old.shape==attribution.shape||old.internal_shape==attribution.internal_shape)return Code::links;}
        auto current=[&](world::o::Observation const& owner){return admitted()&&member(owner,actor.membership,n);};
        work.world_code=world::observe(image_base,size,attribution,read,current,work.world_work,row.geometry);
        if(work.world_code!=world::Code::observed)return Code::world;++out.count;
      }
      // Member additions/removals/order changes during geometry collection
      // invalidate the entire actor group, including an initially empty one.
      work.shape_code=shapes::observe(image_base,size,out.assets.actors[index],scope.scene,read,admitted,work.shape_work,work.closing_shapes);
      if(work.shape_code!=shapes::Code::observed||!shapes::same(actor.membership,work.closing_shapes))return Code::changed;
    }
    work.asset_code=asset::observe(image_base,size,scope,read,guard,work.asset_work,work.closing_assets);
    if(work.asset_code!=asset::Code::observed||!asset::same(out.assets,work.closing_assets))return Code::changed;
    return Code::observed;
}
}
template<class Read,class Guard> Code observe(std::uintptr_t image_base,std::uint32_t size,s::Snapshot const& scope,
    Read read,Guard guard,Workspace& work,Snapshot& output)noexcept{
    static_assert(std::is_invocable_r_v<bool,Guard&>&&std::is_invocable_r_v<bool,Guard&,asset::Snapshot const&>,"Full C0/image and typed asset guard required");
    if(!image_base||(image_base&0xffff)||size<0x277490||size>UINTPTR_MAX-image_base||scope.source.thread!=GetCurrentThreadId())return Code::request;
    if(s::aliases(&scope,sizeof scope,&work,sizeof work)||s::aliases(&scope,sizeof scope,&output,sizeof output)||s::aliases(&work,sizeof work,&output,sizeof output))return Code::alias;
    bool inactive=false;if(!work.active.compare_exchange_strong(inactive,true))return Code::busy;
    struct Finish{std::atomic<bool>& active;~Finish(){active=false;}}finish{work.active};const auto key=scope;
    work.failed_actor=work.failed_member=~0u;
    try{
      if(!guard())return Code::guard;if(!detail::code_current(image_base,read))return Code::module;
      auto code=detail::pass(image_base,size,key,read,guard,work,work.first);if(code!=Code::observed)return code;
      if(!guard())return Code::guard;
      code=detail::pass(image_base,size,key,read,guard,work,work.last);if(code!=Code::observed)return code;
      if(!same(work.first,work.last)||!detail::code_current(image_base,read))return Code::changed;
      if(!guard(work.last.assets)||!guard())return Code::guard;
      if(!s::same(scope,key))return Code::changed;
      output=work.last;work.failed_actor=work.failed_member=~0u;return Code::observed;
    }catch(...){return Code::read;}
}
}
