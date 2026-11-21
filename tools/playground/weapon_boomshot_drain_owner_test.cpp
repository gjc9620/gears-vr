#pragma push_macro("main")
#undef main
#define main previous_inventory_composition_cases
#include "weapon_boomshot_inventory_owner_test.cpp"
#undef main
#pragma pop_macro("main")
int main(){unsigned count{};using Stage=boomshot_reload_owner::Owner::Stage;
 {Normal x(1);x.open();assert(x.owner.drain()&&x.owner.stage()==Stage::parked&&!x.owner.lifecycle_pending()&&x.owner.pending());
  assert(x.owner.blocks_fire(x.input.f)&&boomshot_sessions::blocks_fire(x.input.f)&&original_exit_calls==1&&inventory_writer_calls==0&&!native_reload&&!boomshot_clip::serial);
  assert(x.core()->stage()==c::BoomshotStage::interrupted&&x.core()->inventory().loaded==0&&x.core()->inventory().reserve==2);
  assert(x.owner.drain()&&original_exit_calls==1&&!x.prepare());++count;}
 {Normal x(2);x.open();x.shoulder_grant();assert(x.take()==c::BoomshotEvent::grabbed);assert(x.owner.drain());
  assert(!boomshot_carrier_original::owner.attached&&boomshot_carrier_original::owner.drops==1&&original_exit_calls==1&&x.core()->held_units()==0&&x.owner.blocks_fire(x.input.f));++count;}
 {Normal x(3);confirmed_insertion(x);normal_fault=20;assert(commit(x)==c::BoomshotEvent::interrupted&&lancerreload_inventory::owned_counts.reserve==1);
  normal_fault=0;assert(x.owner.drain()&&partial_restore_calls==1&&inventory_writer_calls==1&&lancerreload_inventory::owned_counts.reserve==2&&lancerreload_inventory::owned_counts.used==1);
  assert(x.core()->inventory().loaded==0&&x.core()->inventory().reserve==2&&x.owner.blocks_fire(x.input.f));++count;}
 {Normal x(4);confirmed_insertion(x);normal_fault=33;assert(commit(x)==c::BoomshotEvent::interrupted&&inventory_writer_calls==1);
  normal_fault=0;assert(x.owner.drain()&&partial_restore_calls==1&&inventory_writer_calls==1&&visual_seat_calls==1&&x.core()->inventory().loaded==1&&x.core()->inventory().reserve==1&&x.owner.blocks_fire(x.input.f));++count;}
 {Normal x(5);x.open();normal_fault=26;assert(!x.owner.drain()&&original_exit_calls==1&&x.owner.lifecycle_pending());
  normal_fault=0;assert(!x.owner.drain()&&original_exit_calls==1&&x.owner.lifecycle_pending()&&x.owner.blocks_fire(x.input.f));++count;}
 {Normal x(6);x.open();x.shoulder_grant();assert(x.take()==c::BoomshotEvent::grabbed);normal_fault=8;
  assert(!x.owner.drain()&&original_exit_calls==0&&boomshot_carrier_original::owner.attached&&x.owner.lifecycle_pending());++count;}
 {Normal x(7);x.open();x.input.f.left_tracked=0;++x.input.f.controllers.input_epoch;source_failure=1;
  assert(x.owner.drain()&&original_exit_calls==1&&!native_reload&&x.owner.blocks_fire(x.input.f));source_failure=16;++count;}
 {Normal x(8);confirmed_close(x);x.input.f.left_tracked=0;source_failure=1;
  assert(x.owner.drain()&&original_exit_calls==1&&original_reward_calls==1&&!x.owner.lifecycle_pending()&&!x.owner.pending());source_failure=16;++count;}
 {Normal x(9);assert(x.prepare());assert(x.owner.drain()&&!x.owner.pending()&&!x.owner.lifecycle_pending()&&native_calls==0);++count;}
 {Normal x(10);assert(x.prepare());normal_fault=2;x.next();assert(x.start()==c::BoomshotEvent::interrupted);
  assert(!x.owner.drain()&&original_exit_calls==0&&x.owner.lifecycle_pending()&&x.owner.blocks_fire(x.input.f));++count;}
 {Normal x(11);x.open();++lancerreload_state::lease.identity.weapon.registry_index;assert(!x.owner.drain()&&original_exit_calls==0&&x.owner.lifecycle_pending());++count;}
 {Normal x(12);x.open();is_gt=false;assert(!x.owner.drain()&&original_exit_calls==0);is_gt=true;assert(x.owner.drain());++count;}
 {Normal x(13);confirmed_insertion(x);assert(commit(x)==c::BoomshotEvent::inventory_committed);assert(x.owner.drain()&&inventory_writer_calls==1&&partial_restore_calls==0&&original_reward_calls==0&&x.core()->inventory().loaded==1&&x.owner.blocks_fire(x.input.f));++count;}
 {Normal x(14);x.open();normal_fault=30;assert(!x.owner.drain()&&original_exit_calls==1&&x.owner.lifecycle_pending());
  normal_fault=0;assert(x.owner.drain()&&original_exit_calls==1&&!x.owner.lifecycle_pending()&&x.owner.blocks_fire(x.input.f));++count;}
 {Normal x(15);confirmed_insertion(x);normal_fault=35;assert(commit(x)==c::BoomshotEvent::inventory_committed);
  assert(!x.owner.drain()&&original_exit_calls==0&&visual_seat_calls==1);normal_fault=0;
  assert(x.owner.drain()&&original_exit_calls==1&&visual_seat_calls==1&&x.core()->inventory().loaded==1&&x.owner.blocks_fire(x.input.f));++count;}
 std::printf("Boomshot drain GT owner: %u cases passed; actual owner/core/events preserve mechanical debt while original resource boundaries are stubbed; no live dispatcher registration or resume proof\n",count);return 0;
}
