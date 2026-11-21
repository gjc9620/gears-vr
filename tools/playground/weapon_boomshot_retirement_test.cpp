#pragma push_macro("main")
#undef main
#define main previous_drain_compositions
#include "weapon_boomshot_drain_owner_test.cpp"
#undef main
#pragma pop_macro("main")
namespace {
void fresh_opening(Normal& x){
 x.next();++x.input.f.press_token;auto& left=x.input.f.controllers.hands[0];++left.pressed[h::left_control(x.input.f)];++left.released[h::left_control(x.input.f)];
 x.input.route.reset();x.input.grant(c::BoomshotContact::opening);x.refresh();
}
}
int main(){unsigned count{};
 {Normal x(1);x.open();assert(x.owner.drain());const auto prior=lancerreload_inventory::identity(x.input.f);
  auto* old=boomshot_sessions::find(prior);const auto generation=old->generation;const auto old_core_id=x.core()->inventory().identity;
  assert(!x.owner.retire_parked()&&x.owner.pending()&&old->known&&old->blocks_fire());++count;
  retired_actor=prior.weapon;retirement_unreadable=true;assert(!x.owner.retire_parked()&&old->known);retirement_unreadable=false;++count;
  assert(x.owner.retire_parked()&&!x.owner.pending()&&!old->known&&!boomshot_sessions::find(prior)&&original_exit_calls==1);++count;
  ++x.input.f.weapon.registry_index;++x.input.f.key.lifecycle;hand_q=bone_q=0;fresh_opening(x);assert(x.prepare());
  const auto next=lancerreload_inventory::identity(x.input.f);auto* current=boomshot_sessions::find(next);
  assert(current==old&&current->generation>generation&&!(x.core()->inventory().identity==old_core_id));++count;
 }
 {Normal x(2);x.open();retired_actor=x.input.f.weapon;assert(!x.owner.retire_parked()&&original_exit_calls==0&&x.owner.lifecycle_pending()&&x.owner.blocks_fire(x.input.f));++count;}
 {Normal x(3);x.open();normal_fault=26;assert(!x.owner.drain());retired_actor=x.input.f.weapon;
  assert(!x.owner.retire_parked()&&x.owner.lifecycle_pending()&&original_exit_calls==1);++count;}
 {Normal x(4);x.open();assert(x.owner.drain());retired_actor=x.input.f.weapon;is_gt=false;assert(!x.owner.retire_parked());is_gt=true;
  assert(x.owner.retire_parked()&&original_exit_calls==1);++count;}
 {Normal x(5);x.open();assert(x.owner.drain());retired_actor=x.input.f.weapon;
  auto other=lancerreload_inventory::identity(x.input.f);other.weapon=object(0x7000,77);lancerreload_state::lease={other,888,true};native_reload=true;
  assert(x.owner.retire_parked()&&lancerreload_state::lease.acquired&&lancerreload_state::lease.serial==888&&lancerreload_state::lease.identity==other&&original_exit_calls==1);++count;}
 {Normal x(6);auto id=lancerreload_inventory::identity(x.input.f);boomshot_sessions::Slot* safe{};unsigned made{};
  while(true){auto candidate=id;candidate.weapon=object(0x9000+made*0x100,100+made);auto* next=boomshot_sessions::prepare(candidate);if(!next)break;if(!safe)safe=next;++made;assert(made<=16);}
  assert(safe);const auto generation=safe->generation;retired_actor=safe->first.weapon;
  assert(x.prepare()&&native_calls==0&&boomshot_sessions::find(id)==safe&&safe->generation>generation);++count;
 }
 {Normal x(7);retirement_unreadable=true;retired_actor=x.input.f.weapon;
  assert(!x.prepare()&&native_calls==0&&!x.owner.pending());++count;}
 std::printf("Boomshot positive retirement: %u cases passed; actual owner/pool/core, registry death boundary stubbed, resource/foreign-owner guards and monotonic generation; no live activation\n",count);return 0;
}
