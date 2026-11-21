#include "../vr-common/weapon_boomshot_mechanics.h"
#include "../vr-common/weapon_boomshot_owner_events.h"
#include "../vr-common/weapon_hand_participant.h"
#include <cassert>
#include <cstdio>
#include <new>
namespace {
namespace hand=gears_weapon_hand;using namespace gears_weapon;
bool gt=true;
namespace weaponhand {unsigned current_gt(void*)noexcept{return gt?1u:0u;}}
namespace lancerreload_inventory {
// Engine boundary fixture; production uses its existing complete identity type.
struct Identity {
 hand::ObjectIdentity world{},pawn{},weapon{};
 std::uint64_t game_start{},native_start{},registration{},lifecycle{},world_generation{},hold_generation{},session{},space{},input{};
 bool valid()const noexcept{return hand::identity_valid(world)&&hand::identity_valid(pawn)&&hand::identity_valid(weapon)&&
  game_start&&native_start&&registration&&lifecycle&&world_generation&&hold_generation&&session&&space&&input;}
 bool operator==(Identity const& b)const noexcept{return hand::same(world,b.world)&&hand::same(pawn,b.pawn)&&hand::same(weapon,b.weapon)&&
  game_start==b.game_start&&native_start==b.native_start&&registration==b.registration&&lifecycle==b.lifecycle&&world_generation==b.world_generation&&
  hold_generation==b.hold_generation&&session==b.session&&space==b.space&&input==b.input;}
};
Identity current{};
Identity identity(hand::Frame const&)noexcept{return current;}
}
#include "weapon_boomshot_sessions.inl"
using Id=lancerreload_inventory::Identity;namespace sessions=boomshot_sessions;
hand::ObjectIdentity object(std::uint64_t address){return {address*8,0x9000,0xa000,unsigned(address),7,0,1};}
Id identity(){return {object(1),object(2),object(3),10,11,12,13,14,15,16,17,18};}
BoomshotPlan plan(BoomshotIdentity id){BoomshotPlan p{};p.identity=id;p.config_id=1;p.plan_id=1;p.condition=BoomshotCondition::inspection;
 p.count=2;p.current_instance_verified=p.original_order_verified=p.quantity_verified=true;
 p.steps[0]={BoomshotStepKind::move_mechanism,BoomshotPart::barrel,1,1,0,0,false};
 p.steps[1]={BoomshotStepKind::move_mechanism,BoomshotPart::barrel,2,2,0,0,true};return p;}
}
int main(){unsigned count{};auto id=identity();auto* slot=sessions::prepare(id);assert(slot&&sessions::find(id)==slot&&!slot->blocks_fire());
 auto* core=slot->core(id);auto* events=slot->events(id);assert(core&&events);
 assert(core->bind_inspection({slot->identity(),1,1,100000000,true,true},{slot->identity(),1,1,2})&&events->bind(slot->identity(),core->binding_token()));
 BoomshotOwnerEvents::InputEvent first{},next{};
 assert(events->input({1,1,1,1,1000000000,1},1000000000,first)==BoomshotOwnerEvents::Result::accepted);
 auto frame=[&](BoomshotOwnerEvents::InputEvent const& source){return BoomshotFrame{source.stamp(),core->inventory(),true,true,true,true,true,false};};
 core->observe(frame(first));assert(events->input({2,1,1,1,1020000000,2},1020000000,next)==BoomshotOwnerEvents::Result::accepted);
 assert(core->begin_inspection(frame(next),{core->binding_token(),1,plan(slot->identity()),true,true},1)==BoomshotEvent::started);
 assert(slot->blocks_fire());lancerreload_inventory::current=id;assert(sessions::blocks_fire({}));++count;
 for(unsigned changed=0;changed<9;++changed){auto other=id;
  switch(changed){case 0:++other.native_start;break;case 1:++other.registration;break;case 2:++other.lifecycle;break;
   case 3:++other.world_generation;break;case 4:++other.hold_generation;break;case 5:++other.session;break;
   case 6:++other.space;break;case 7:++other.input;break;case 8:++other.lifecycle;++other.hold_generation;++other.input;break;}
  assert(sessions::find(other)==slot&&sessions::prepare(other)==slot&&slot->blocks_fire());
  assert(!slot->core(other)&&!slot->events(other));lancerreload_inventory::current=other;assert(sessions::blocks_fire({}));++count;
 }
 assert(core->interrupt()==BoomshotEvent::interrupted&&slot->blocks_fire()&&!core->reset());++count;
 for(unsigned changed=0;changed<7;++changed){auto other=id;
  switch(changed){case 0:++other.game_start;break;case 1:++other.world.registry_index;break;case 2:++other.pawn.registry_index;break;
   case 3:++other.weapon.registry_index;break;case 4:++other.weapon.name_number;break;case 5:other.weapon.klass+=8;break;case 6:other.weapon.outer+=8;break;}
  auto* separate=sessions::prepare(other);assert(separate&&separate!=slot&&separate->generation!=slot->generation&&!separate->blocks_fire());
  assert(sessions::find(id)==slot&&slot->blocks_fire());++count;
 }
 {gt=false;assert(!sessions::prepare(id)&&!slot->core(id)&&!slot->events(id)&&sessions::blocks_fire({}));gt=true;++count;}
 unsigned added{};
 while(true){auto other=id;other.weapon=object(100+added);auto* made=sessions::prepare(other);if(!made)break;++added;assert(added<=16);}
 assert(added==8&&sessions::find(id)==slot&&slot->blocks_fire());++count;
 std::printf("Boomshot per-actor sessions: %u cases passed; real core remains interrupted across hold/input/native contexts, lookup cannot grant resume, recycled identity separated, bounded exhaustion cannot evict debt; synthetic boundary only\n",count);
}
