#pragma push_macro("main")
#undef main
#define main previous_retirement_cases
#include "weapon_boomshot_retirement_test.cpp"
#undef main
#pragma pop_macro("main")
namespace {
using OwnerStage=boomshot_reload_owner::Owner::Stage;
void half_open(Normal& x){assert(x.prepare());x.next();assert(x.start()==c::BoomshotEvent::started);x.next();x.move();
 x.next();hand_q=bone_q=.5;x.move();assert(x.owner.drain()&&x.owner.stage()==OwnerStage::parked);}
void fresh_restore_grant(Normal& x){
 if(x.owner.contact(x.input.f)==c::BoomshotContact::take_carrier)x.shoulder_grant();else fresh_opening(x);
}
auto resume_empty(Normal& x){return x.owner.resume_empty(x.input.f,x.input.d,x.input.route.grant());}
auto service_resume(Normal& x){return x.owner.service_resume(x.input.f,x.input.d,x.input.route.grant());}
void reacquire(Normal& x){bone_q=0;fresh_restore_grant(x);assert(resume_empty(x)==c::BoomshotEvent::started);}
void restore(Normal& x,double q){reacquire(x);assert(service_resume(x)==c::BoomshotEvent::none&&boomshot_clip::seeks==1);
 x.next();bone_q=q;assert(service_resume(x)==c::BoomshotEvent::restored&&x.owner.stage()==OwnerStage::mechanical);}
void fresh_take(Normal& x){x.shoulder_grant();auto result=x.take();
 if(result==c::BoomshotEvent::motion_reset){x.next();result=x.take();}assert(result==c::BoomshotEvent::grabbed);}
void request_close_at(Normal& x,float time){
 original_game_time=time;const auto id=lancerreload_inventory::identity(x.input.f);auto* stream=boomshot_sessions::find(id)->events(id);
 const auto input=[&](){x.next();c::BoomshotOwnerEvents::InputEvent event{};
  assert(stream->input({x.input.f.controllers.sequence,x.input.f.controllers.input_epoch,x.rig.rig.native_reference.epoch,
   x.input.f.controllers.profile_generation,x.input.f.controllers.sync_ns,std::int64_t(double(time)*1000000.)},clock_value,event)==c::BoomshotOwnerEvents::Result::accepted);return event;};
 const auto begin=input();x.core()->observe({begin.stamp(),x.core()->inventory(),true,true,true,true,false,false});
 const auto end=input();const auto& plan=x.owner.plan();const auto& step=plan.steps[2];
 const c::BoomshotMotionReceipt receipt{x.core()->binding_token(),777,plan.plan_id,step.phase_id,step.geometry_id,end.stamp().motion_epoch,
  begin.stamp().sequence,begin.stamp().monotonic_ns,true,true,true,true,true};
 assert(x.core()->complete_mechanism({end.stamp(),x.core()->inventory(),true,true,true,true,false,false},receipt)==c::BoomshotEvent::request_completion);bone_q=0;
}
}
int main(){unsigned count{};
 {Normal x(1);half_open(x);const auto id=x.core()->inventory().identity;const auto binding=x.core()->binding_token();
  const auto before=lancerreload_state::lease;assert(!before.acquired);reacquire(x);
  assert(service_resume(x)==c::BoomshotEvent::none&&boomshot_clip::seeks==1&&std::abs(boomshot_clip::sought_q-.5)<1e-8);
  assert(service_resume(x)==c::BoomshotEvent::none&&boomshot_clip::seeks==1);x.next();
  assert(service_resume(x)==c::BoomshotEvent::none&&x.core()->stage()==c::BoomshotStage::interrupted);bone_q=.5;
  assert(service_resume(x)==c::BoomshotEvent::restored&&x.core()->inventory().identity==id&&x.core()->binding_token()==binding&&native_calls==2);
  assert(!x.input.route.grant().begin_allowed(x.input.f,x.input.d,c::BoomshotContact::opening,clock_value)&&inventory_writer_calls==0&&x.owner.blocks_fire(x.input.f));++count;}
 {Normal x(2);x.open();const auto id=x.core()->inventory().identity;assert(x.owner.drain());++x.input.f.key.lifecycle;restore(x,1);
  assert(x.core()->inventory().identity==id&&x.core()->stage()==c::BoomshotStage::take_ammunition&&x.core()->inventory().loaded==0);
  assert(x.take()==c::BoomshotEvent::rejected&&boomshot_carrier_original::owner.grabs==0);fresh_take(x);++count;}
 {Normal x(3);half_open(x);x.next();assert(resume_empty(x)==c::BoomshotEvent::rejected&&native_calls==1&&x.owner.stage()==OwnerStage::parked);++count;}
 {Normal x(4);half_open(x);fresh_opening(x);const auto old=x.input.f;
  for(unsigned bad=0;bad<7;++bad){x.input.f=old;
   if(bad==0)++x.input.f.controllers.input_epoch;if(bad==1)++x.input.f.stereo.space_epoch;if(bad==2)++x.input.f.stereo.session;
   if(bad==3)++x.input.f.key.native_started_qpc;if(bad==4)++x.input.f.key.world_generation;
   if(bad==5)++x.input.f.weapon.registry_index;if(bad==6)--x.input.f.key.lifecycle;
   assert(resume_empty(x)==c::BoomshotEvent::rejected&&native_calls==1&&x.owner.stage()==OwnerStage::parked);}
  x.input.f=old;++count;}
 {Normal x(5);half_open(x);fresh_opening(x);++lancerreload_inventory::owned_counts.reserve;
  assert(resume_empty(x)==c::BoomshotEvent::rejected&&native_calls==1&&inventory_writer_calls==0&&x.owner.blocks_fire(x.input.f));++count;}
 {Normal x(6);confirmed_insertion(x);assert(commit(x)==c::BoomshotEvent::inventory_committed&&x.owner.drain());fresh_opening(x);
  assert(resume_empty(x)==c::BoomshotEvent::rejected&&native_calls==1&&inventory_writer_calls==1&&x.core()->inventory().loaded==1&&x.owner.blocks_fire(x.input.f));++count;}
 {Normal x(7);half_open(x);const auto old=lancerreload_inventory::identity(x.input.f);++x.input.f.key.lifecycle;bone_q=0;fresh_opening(x);normal_fault=1;
  assert(resume_empty(x)==c::BoomshotEvent::rejected&&native_calls==2&&x.owner.original_serial()==777&&x.owner.stage()==OwnerStage::parked);
  auto* slot=boomshot_sessions::find(old);assert(slot&&slot->core(old)&&!slot->core(lancerreload_inventory::identity(x.input.f)));
  normal_fault=0;fresh_opening(x);assert(resume_empty(x)==c::BoomshotEvent::started&&native_calls==3);++count;}
 {Normal x(8);half_open(x);bone_q=0;fresh_opening(x);normal_fault=2;
  assert(resume_empty(x)==c::BoomshotEvent::interrupted&&x.owner.stage()==OwnerStage::recovery&&native_calls==2);
  normal_fault=0;x.next();assert(resume_empty(x)==c::BoomshotEvent::rejected&&!x.owner.drain()&&native_calls==2&&x.owner.blocks_fire(x.input.f));++count;}
 {Normal x(9);half_open(x);bone_q=0;fresh_opening(x);normal_fault=38;
  assert(resume_empty(x)==c::BoomshotEvent::interrupted&&native_calls==2&&x.owner.lifecycle_pending());normal_fault=0;
  assert(x.owner.drain()&&original_exit_calls==2&&x.owner.blocks_fire(x.input.f));reacquire(x);
  assert(lancerreload_state::lease.reward_start==1.25f&&native_calls==3);++count;}
 {Normal x(10);half_open(x);reacquire(x);normal_fault=36;assert(service_resume(x)==c::BoomshotEvent::interrupted&&boomshot_clip::seeks==1);
  assert(service_resume(x)==c::BoomshotEvent::rejected&&boomshot_clip::seeks==1);normal_fault=0;assert(x.owner.drain());
  reacquire(x);assert(service_resume(x)==c::BoomshotEvent::none&&boomshot_clip::seeks==2&&std::abs(boomshot_clip::sought_q-.5)<1e-8);++count;}
 {Normal x(11);half_open(x);reacquire(x);normal_fault=37;
  assert(service_resume(x)==c::BoomshotEvent::none);for(unsigned i=0;i<10;++i){x.next();assert(service_resume(x)==c::BoomshotEvent::none&&boomshot_clip::seeks==0);}
  normal_fault=0;assert(service_resume(x)==c::BoomshotEvent::none&&boomshot_clip::seeks==1);x.next();bone_q=.5;
  assert(service_resume(x)==c::BoomshotEvent::restored&&native_calls==2&&lancerreload_state::lease.reward_start==1.25f);++count;}
 {Normal x(12);half_open(x);reacquire(x);++boomshot_clip::curve.content;
  assert(service_resume(x)==c::BoomshotEvent::interrupted&&boomshot_clip::seeks==0&&x.owner.blocks_fire(x.input.f));++count;}
 {Normal x(13);x.open();x.shoulder_grant();assert(x.take()==c::BoomshotEvent::grabbed);insert_current_carrier(x);normal_fault=20;
  assert(commit(x)==c::BoomshotEvent::interrupted);normal_fault=0;assert(x.owner.drain()&&inventory_writer_calls==1&&x.core()->inventory().loaded==0);
  restore(x,1);fresh_take(x);insert_current_carrier(x);
  assert(commit(x)==c::BoomshotEvent::inventory_committed&&inventory_writer_calls==2&&x.core()->inventory().loaded==1&&x.core()->inventory().reserve==1);++count;}
 for(unsigned boundary=0;boundary<2;++boundary){Normal x(14+boundary);x.open();const auto began=lancerreload_state::lease;assert(x.owner.drain());
  original_game_time=4.25f;restore(x,1);assert(lancerreload_state::lease.reward_start==began.reward_start&&lancerreload_state::lease.accepted_begin_ms==began.accepted_begin_ms&&lancerreload_state::lease.perfect_end_ms==began.perfect_end_ms);
  fresh_take(x);insert_current_carrier(x);assert(commit(x)==c::BoomshotEvent::inventory_committed);
  // TimeSeconds is float; 5.250001f still truncates to the inclusive microtick.
  // This adjacent representable value reaches the first outside microtick.
  request_close_at(x,boundary?5.250002f:5.25f);assert(x.owner.complete(x.input.f)==c::BoomshotEvent::reload_completed);
  if(original_reward_calls!=(boundary?0u:1u))std::fprintf(stderr,"boundary=%u reward=%u calls=%u writers=%u time=%.9g\n",boundary,original_reward_calls,native_calls,inventory_writer_calls,double(original_game_time));
  assert(original_reward_calls==(boundary?0u:1u)&&native_calls==2&&inventory_writer_calls==1&&!x.owner.pending());++count;}
 {Normal x(16);half_open(x);bone_q=0;fresh_restore_grant(x);normal_fault=4;
  assert(resume_empty(x)==c::BoomshotEvent::started);normal_fault=0;x.next();bone_q=.5;
  assert(service_resume(x)==c::BoomshotEvent::none&&boomshot_clip::seeks==0);x.next();bone_q=0;
  assert(service_resume(x)==c::BoomshotEvent::none&&boomshot_clip::seeks==1);x.next();bone_q=.5;
  assert(service_resume(x)==c::BoomshotEvent::restored&&native_calls==2&&lancerreload_state::lease.reward_start==1.25f);++count;}
 std::printf("Boomshot pre-q1 owner recovery: %u cases passed; actual owner/core/grant/events/accounting, later bone ACK/seek once, exact hold-only rebind, no q1 replay, old4s inclusive boundary; engine/curve/component geometry boundaries stubbed; not live activation\n",count);return 0;
}
