#pragma once
#include "death_constant_observer.h"
namespace gears_death_dof {
using gears_death_constants::Float4;using gears_death_constants::Descriptor;
using gears_death_constants::finite;using gears_death_constants::equal;
constexpr unsigned limit=256;
enum Reject:unsigned {accepted,configuration,ticket,world,caller,index,view,descriptor,source,owner,tls,duplicate,capacity,reject_count};
struct Gate {bool enabled{},recording{},diagnostics{},frozen{},unpossessed{},native_valid{};
    uint64_t ticket{},frozen_ticket{},world{},frozen_world{};uintptr_t caller{},owner{};unsigned index{},eye{};};
inline Reject gate(Gate const& g,Descriptor const& d,Float4 const& value){
    if(!g.enabled||!g.recording||!g.diagnostics||!g.frozen||!g.unpossessed)return configuration;
    if(!g.ticket||g.ticket!=g.frozen_ticket)return ticket;
    if(!g.world||g.world!=g.frozen_world)return world;
    if(g.caller!=0xd803bf)return caller;if(g.index)return index;
    if(!g.native_valid||g.eye>1)return view;
    if(d!=Descriptor{0,192,16})return descriptor;
    if(!finite(value)||value[0]!=1.0f)return source;
    if(!g.owner)return owner;return accepted;
}
inline uintptr_t descriptor_owner(uintptr_t address){return address>=0xfe&&address%2==0?address-0xfe:0;}
inline Float4 forwarded(Float4 const& original,bool right){auto out=original;if(right)out[0]=0.0f;return out;}
struct Ticket {uint64_t world{},id{},duplicates{};};
struct Recorder {
    uint64_t bound_world{};uintptr_t bound_owner{};unsigned count{};std::array<Ticket,limit> tickets{};
    uint64_t duplicates{},overflow{};
    bool bind(uint64_t world,uintptr_t owner){if(!world||!owner)return false;
        if(!bound_world){bound_world=world;bound_owner=owner;}return bound_world==world&&bound_owner==owner;}
    Ticket* find(uint64_t world,uint64_t id){for(unsigned i=0;i<count;++i)if(tickets[i].world==world&&tickets[i].id==id)return &tickets[i];return nullptr;}
    Reject admit(uint64_t world,uint64_t id,unsigned& slot){
        if(auto* row=find(world,id)){++row->duplicates;++duplicates;return duplicate;}
        if(count==limit){++overflow;return capacity;}slot=count;tickets[count++]={world,id,0};return accepted;
    }
};
}
