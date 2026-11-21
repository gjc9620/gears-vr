#pragma push_macro("main")
#undef main
#define main previous_normal_owner_cases
#include "weapon_boomshot_reload_owner_test.cpp"
#undef main
#pragma pop_macro("main")
namespace {
// Geometry and raw-reader engine boundary only are synthetic. The ACTUAL owner
// now drives the actual insertion Stroke, maps its opaque raw receipt to real
// typed events and requests q1 itself. No hand-written core insertion receipt.
void insert_current_carrier(Normal& x){
 c::boomshot_insertion::Geometry geometry{};geometry.id=x.owner.plan().steps[1].geometry_id;
 geometry.outward={0,0,-1};geometry.travel_m=.1;geometry.approach_margin_m=.01;geometry.capture_radius_m=.01;geometry.retain_radius_m=.015;
 geometry.capture_angle_degrees=12;geometry.retain_angle_degrees=18;geometry.seat_tolerance_m=.002;geometry.verified=true;
 for(double depth:{.13,.1,.05,0.,0.}){x.next();boomshot_carrier_original::raw_depth=depth;
  const auto result=x.owner.insert(x.input.f,x.input.d,x.input.route.grant(),geometry);
  if(x.core()->stage()==c::BoomshotStage::inventory_commit)assert(result==c::BoomshotEvent::request_inventory);else assert(result==c::BoomshotEvent::none);
 }
 assert(x.core()->stage()==c::BoomshotStage::inventory_commit);
}
void confirmed_insertion(Normal& x){
 x.open();x.shoulder_grant();assert(x.take()==c::BoomshotEvent::grabbed);insert_current_carrier(x);
}
auto commit(Normal& x){return x.owner.commit_inventory(x.input.f,x.input.d,x.input.route.grant());}
void confirmed_close(Normal& x){
 confirmed_insertion(x);assert(commit(x)==c::BoomshotEvent::inventory_committed);
 const auto context=lancerreload_inventory::identity(x.input.f);auto* stream=boomshot_sessions::find(context)->events(context);
 const auto input=[&](){x.next();c::BoomshotOwnerEvents::InputEvent event{};
  assert(stream->input({x.input.f.controllers.sequence,x.input.f.controllers.input_epoch,x.rig.rig.native_reference.epoch,
   x.input.f.controllers.profile_generation,x.input.f.controllers.sync_ns,1250000},clock_value,event)==c::BoomshotOwnerEvents::Result::accepted);return event;};
 auto begin=input();assert(x.core()->observe({begin.stamp(),x.core()->inventory(),true,true,true,true,false,false})==c::BoomshotEvent::none);
 auto end=input();const auto& plan=x.owner.plan();const auto& step=plan.steps[2];
 c::BoomshotMotionReceipt receipt{x.core()->binding_token(),777,plan.plan_id,step.phase_id,step.geometry_id,end.stamp().motion_epoch,
  begin.stamp().sequence,begin.stamp().monotonic_ns,true,true,true,true,true};
 assert(x.core()->complete_mechanism({end.stamp(),x.core()->inventory(),true,true,true,true,false,false},receipt)==c::BoomshotEvent::request_completion);
 bone_q=0;
}
}
int main(){unsigned count{};
 {Normal x(1);confirmed_insertion(x);assert(inventory_writer_calls==0&&lancerreload_inventory::owned_counts.used==1);
  assert(commit(x)==c::BoomshotEvent::inventory_committed&&inventory_writer_calls==1&&visual_seat_calls==1);
  assert(x.core()->inventory().loaded==1&&x.core()->inventory().reserve==1&&x.core()->inventory().revision>1&&x.core()->stage()==c::BoomshotStage::mechanism);
  assert(!boomshot_carrier_original::owner.attached&&x.owner.pending()&&x.owner.blocks_fire(x.input.f));
  x.next();assert(commit(x)==c::BoomshotEvent::rejected&&inventory_writer_calls==1&&visual_seat_calls==1);++count;}
 {Normal x(2);confirmed_insertion(x);normal_fault=20;assert(commit(x)==c::BoomshotEvent::interrupted&&inventory_writer_calls==1&&visual_seat_calls==0);
  assert(lancerreload_inventory::owned_counts.used==1&&lancerreload_inventory::owned_counts.reserve==1&&x.owner.blocks_fire(x.input.f));
  normal_fault=0;x.next();assert(commit(x)==c::BoomshotEvent::rejected&&inventory_writer_calls==1);++count;}
 {Normal x(3);confirmed_insertion(x);normal_fault=21;assert(commit(x)==c::BoomshotEvent::interrupted&&inventory_writer_calls==1&&visual_seat_calls==0);
  assert(x.core()->inventory().loaded==0&&x.core()->inventory().reserve==2&&x.owner.blocks_fire(x.input.f));++count;}
 {Normal x(4);confirmed_insertion(x);normal_fault=22;assert(commit(x)==c::BoomshotEvent::interrupted&&inventory_writer_calls==0&&visual_seat_calls==0&&x.owner.blocks_fire(x.input.f));++count;}
 {Normal x(5);confirmed_insertion(x);normal_fault=24;assert(commit(x)==c::BoomshotEvent::interrupted&&inventory_writer_calls==1&&visual_seat_calls==1);
  assert(x.core()->inventory().loaded==1&&boomshot_carrier_original::owner.attached&&x.owner.blocks_fire(x.input.f));normal_fault=0;x.next();
  assert(commit(x)==c::BoomshotEvent::rejected&&inventory_writer_calls==1&&visual_seat_calls==1);++count;}
 {Normal x(6);x.open();x.shoulder_grant();assert(x.take()==c::BoomshotEvent::grabbed);assert(commit(x)==c::BoomshotEvent::rejected&&inventory_writer_calls==0&&visual_seat_calls==0&&lancerreload_inventory::owned_counts.used==1);++count;}
 {Normal x(7);confirmed_insertion(x);++x.input.f.controllers.input_epoch;assert(commit(x)==c::BoomshotEvent::rejected&&inventory_writer_calls==0&&visual_seat_calls==0&&x.owner.blocks_fire(x.input.f));++count;}
 {Normal x(8);confirmed_close(x);assert(original_exit_calls==0&&original_reward_calls==0);
  assert(x.owner.complete(x.input.f)==c::BoomshotEvent::reload_completed&&!x.owner.pending()&&!x.owner.blocks_fire(x.input.f));
  assert(original_exit_calls==1&&original_reward_calls==1&&!native_reload&&!boomshot_clip::serial&&!boomshot_carrier_original::owner.attached);
  assert(!boomshot_sessions::blocks_fire(x.input.f));x.next();assert(x.owner.complete(x.input.f)==c::BoomshotEvent::rejected&&original_exit_calls==1&&original_reward_calls==1);++count;}
 {Normal x(9);confirmed_close(x);normal_fault=26;assert(x.owner.complete(x.input.f)==c::BoomshotEvent::interrupted&&original_exit_calls==1&&original_reward_calls==0&&x.owner.blocks_fire(x.input.f));
  normal_fault=0;x.next();assert(x.owner.complete(x.input.f)==c::BoomshotEvent::rejected&&original_exit_calls==1&&original_reward_calls==0);++count;}
 {Normal x(10);confirmed_close(x);normal_fault=27;assert(x.owner.complete(x.input.f)==c::BoomshotEvent::interrupted&&original_exit_calls==1&&original_reward_calls==1&&x.owner.blocks_fire(x.input.f));
  normal_fault=0;x.next();assert(x.owner.complete(x.input.f)==c::BoomshotEvent::rejected&&original_reward_calls==1);++count;}
 {Normal x(11);confirmed_close(x);normal_fault=28;assert(x.owner.complete(x.input.f)==c::BoomshotEvent::none&&original_exit_calls==1&&original_reward_calls==1&&x.owner.blocks_fire(x.input.f));
  normal_fault=0;x.next();assert(x.owner.complete(x.input.f)==c::BoomshotEvent::reload_completed&&original_exit_calls==1&&original_reward_calls==1);++count;}
 {Normal x(12);confirmed_close(x);normal_fault=30;assert(x.owner.complete(x.input.f)==c::BoomshotEvent::none&&original_exit_calls==1&&original_reward_calls==0&&x.owner.blocks_fire(x.input.f));
  normal_fault=0;x.next();assert(x.owner.complete(x.input.f)==c::BoomshotEvent::reload_completed&&original_exit_calls==1&&original_reward_calls==1);++count;}
 {Normal x(13);confirmed_insertion(x);assert(commit(x)==c::BoomshotEvent::inventory_committed);assert(x.owner.complete(x.input.f)==c::BoomshotEvent::rejected&&original_exit_calls==0&&original_reward_calls==0&&x.owner.blocks_fire(x.input.f));++count;}
 {Normal x(14);confirmed_close(x);normal_fault=29;assert(x.owner.complete(x.input.f)==c::BoomshotEvent::none&&original_exit_calls==1&&original_reward_calls==1&&x.owner.blocks_fire(x.input.f));
  normal_fault=0;x.next();assert(x.owner.complete(x.input.f)==c::BoomshotEvent::reload_completed&&original_exit_calls==1&&original_reward_calls==1);++count;}
 {Normal x(15);x.open();x.shoulder_grant();assert(x.take()==c::BoomshotEvent::grabbed);x.next();normal_fault=34;
  assert(x.carry()==c::BoomshotEvent::none&&x.core()->stage()==c::BoomshotStage::holding_ammunition&&inventory_writer_calls==0&&boomshot_carrier_original::owner.grabs==1);
  normal_fault=0;x.next();assert(x.carry()==c::BoomshotEvent::none&&boomshot_carrier_original::owner.grabs==1&&native_calls==1);++count;}
 {Normal x(16);confirmed_insertion(x);normal_fault=35;assert(commit(x)==c::BoomshotEvent::inventory_committed&&x.owner.stage()==boomshot_reload_owner::Owner::Stage::seating);
  assert(x.owner.contact(x.input.f)==c::BoomshotContact::none&&!x.owner.service_seating()&&inventory_writer_calls==1&&visual_seat_calls==1&&x.owner.blocks_fire(x.input.f));
  normal_fault=0;x.next();assert(x.owner.service_seating()&&x.owner.stage()==boomshot_reload_owner::Owner::Stage::mechanical&&inventory_writer_calls==1&&visual_seat_calls==1&&x.core()->step_index()==2);++count;}
 std::printf("Boomshot inventory GT owner: %u cases passed; real owner/core/event/request/receipt composition, original writer and component/geometry boundaries stubbed; no live activation\n",count);return 0;
}
