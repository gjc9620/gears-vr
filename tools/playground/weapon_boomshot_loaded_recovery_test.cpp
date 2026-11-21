#pragma push_macro("main")
#undef main
#define main previous_empty_recovery_cases
#include "weapon_boomshot_resume_owner_test.cpp"
#undef main
#pragma pop_macro("main")
namespace {
namespace gun=boomshot_inspection_clip;
void closing_grant(Normal& x){x.next();++x.input.f.press_token;auto& left=x.input.f.controllers.hands[0];
 ++left.pressed[h::left_control(x.input.f)];++left.released[h::left_control(x.input.f)];x.input.route.reset();x.input.grant(c::BoomshotContact::closing);x.refresh();}
void loaded_park(Normal& x){confirmed_insertion(x);assert(commit(x)==c::BoomshotEvent::inventory_committed);x.next();
 assert(x.owner.drain()&&x.owner.stage()==OwnerStage::parked&&x.core()->step_index()==2&&x.core()->inventory().loaded==1);}
auto resume_loaded(Normal& x){return x.owner.resume_loaded(x.input.f,x.input.d,x.input.route.grant());}
void loaded_start(Normal& x){bone_q=0;closing_grant(x);assert(resume_loaded(x)==c::BoomshotEvent::started);}
void loaded_restore(Normal& x,double q=1){loaded_start(x);const auto seeks=gun::backend.seeks;
 assert(service_resume(x)==c::BoomshotEvent::none&&gun::backend.seeks==seeks+1);x.next();bone_q=q;
 assert(service_resume(x)==c::BoomshotEvent::restored&&x.core()->stage()==c::BoomshotStage::mechanism);}
void physical_close(Normal& x,double from=1){closing_grant(x);hand_q=bone_q=from;const auto began=x.move();
 assert(began==c::BoomshotEvent::motion_reset||began==c::BoomshotEvent::none);assert(boomshot_barrel::stroke.active());
 x.next();hand_q=bone_q=from*.5;assert(x.move()==c::BoomshotEvent::none);x.next();hand_q=bone_q=0;assert(x.move()==c::BoomshotEvent::none);
 x.next();assert(x.move()==c::BoomshotEvent::request_completion&&x.core()->stage()==c::BoomshotStage::completion_commit);}
}
int main(){unsigned count{};
 {Normal x(1);lancerreload_inventory::owned_counts.reserve=1;x.refresh();loaded_park(x);const auto id=x.core()->inventory().identity;const auto binding=x.core()->binding_token();
  ++x.input.f.key.lifecycle;loaded_restore(x);assert(x.core()->inventory().identity==id&&x.core()->binding_token()==binding&&x.owner.original_serial()==777&&native_calls==1&&!native_reload);
  assert(x.core()->inventory().loaded==1&&x.core()->inventory().reserve==0&&inventory_writer_calls==1&&visual_seat_calls==1&&gun::backend.plays==1&&gun::backend.stops==1);
  assert(!x.input.route.grant().begin_allowed(x.input.f,x.input.d,c::BoomshotContact::closing,clock_value));physical_close(x);
  assert(x.owner.complete(x.input.f)==c::BoomshotEvent::reload_completed&&gun::backend.finishes==1&&original_exit_calls==1&&original_reward_calls==1&&inventory_writer_calls==1&&!x.owner.pending());++count;}
 {Normal x(2);loaded_park(x);loaded_restore(x);closing_grant(x);hand_q=bone_q=1;x.move();x.next();hand_q=bone_q=.5;x.move();
  assert(x.owner.drain()&&gun::backend.finishes==1&&x.owner.blocks_fire(x.input.f));loaded_restore(x,.5);
  assert(gun::owner.serial()==2&&gun::backend.plays==2&&gun::backend.seeks==2&&std::abs(gun::backend.sought_q-.5)<1e-8&&native_calls==1);
  physical_close(x,.5);assert(x.owner.complete(x.input.f)==c::BoomshotEvent::reload_completed&&inventory_writer_calls==1&&original_exit_calls==1&&gun::backend.finishes==2);++count;}
 {Normal x(3);loaded_park(x);const auto old=lancerreload_inventory::identity(x.input.f);++x.input.f.key.lifecycle;bone_q=0;closing_grant(x);normal_fault=51;
  assert(resume_loaded(x)==c::BoomshotEvent::rejected&&x.owner.stage()==OwnerStage::parked&&!gun::owner.active()&&gun::backend.plays==0);
  auto* slot=boomshot_sessions::find(old);assert(slot->core(old)&&!slot->core(lancerreload_inventory::identity(x.input.f)));
  normal_fault=0;loaded_restore(x);assert(gun::owner.serial()==2&&native_calls==1&&inventory_writer_calls==1);++count;}
 {Normal x(4);loaded_park(x);bone_q=0;closing_grant(x);normal_fault=52;
  during_original=[&](){assert(x.owner.stage()==OwnerStage::restoring&&gun::owner.active()&&x.owner.blocks_fire(x.input.f)&&resume_loaded(x)==c::BoomshotEvent::rejected);};
  assert(resume_loaded(x)==c::BoomshotEvent::started&&gun::backend.plays==1&&gun::backend.stops==1&&native_calls==1);
  normal_fault=0;assert(service_resume(x)==c::BoomshotEvent::none);x.next();bone_q=1;assert(service_resume(x)==c::BoomshotEvent::restored&&gun::backend.plays==1);++count;}
 {Normal x(5);loaded_park(x);bone_q=0;closing_grant(x);normal_fault=53;
  assert(resume_loaded(x)==c::BoomshotEvent::interrupted&&gun::backend.plays==1&&gun::backend.stops==0&&x.owner.lifecycle_pending());
  assert(!x.owner.drain()&&gun::backend.plays==1);normal_fault=0;assert(x.owner.drain()&&gun::backend.stops==1&&gun::backend.finishes==1&&native_calls==1);++count;}
 {Normal x(6);loaded_park(x);bone_q=0;closing_grant(x);normal_fault=54;
  assert(resume_loaded(x)==c::BoomshotEvent::interrupted&&gun::backend.plays==1&&gun::backend.stops==1);
  normal_fault=0;assert(x.owner.drain()&&gun::backend.stops==1&&gun::backend.finishes==1&&inventory_writer_calls==1);++count;}
 {Normal x(7);loaded_park(x);bone_q=0;closing_grant(x);normal_fault=55;
  assert(resume_loaded(x)==c::BoomshotEvent::started);for(unsigned i=0;i<8;++i){x.next();assert(service_resume(x)==c::BoomshotEvent::none&&gun::backend.seeks==0);}
  normal_fault=0;assert(service_resume(x)==c::BoomshotEvent::none);x.next();bone_q=1;
  assert(service_resume(x)==c::BoomshotEvent::restored&&gun::backend.plays==1&&gun::backend.stops==1&&native_calls==1);++count;}
 {Normal x(8);loaded_park(x);loaded_start(x);normal_fault=56;
  assert(service_resume(x)==c::BoomshotEvent::interrupted&&gun::backend.seeks==1);assert(service_resume(x)==c::BoomshotEvent::rejected&&gun::backend.seeks==1);
  normal_fault=0;assert(x.owner.drain());loaded_restore(x);assert(gun::backend.plays==2&&gun::backend.seeks==2&&std::abs(gun::backend.sought_q-1)<1e-8&&native_calls==1);++count;}
 {Normal x(9);loaded_park(x);loaded_restore(x);physical_close(x);normal_fault=57;
  assert(x.owner.complete(x.input.f)==c::BoomshotEvent::none&&gun::backend.finishes==1&&original_reward_calls==0);
  normal_fault=0;for(unsigned i=0;i<3;++i){x.next();assert(x.owner.complete(x.input.f)==c::BoomshotEvent::none&&gun::backend.finishes==1&&original_reward_calls==0&&x.owner.blocks_fire(x.input.f));}
  assert(original_exit_calls==1&&inventory_writer_calls==1&&x.owner.lifecycle_pending());++count;}
 {Normal x(10);loaded_park(x);loaded_restore(x);physical_close(x);normal_fault=58;
  assert(x.owner.complete(x.input.f)==c::BoomshotEvent::none&&gun::backend.finishes==1&&original_reward_calls==0);
  normal_fault=0;x.next();assert(x.owner.complete(x.input.f)==c::BoomshotEvent::reload_completed&&gun::backend.finishes==1&&original_reward_calls==1&&original_exit_calls==1);++count;}
 {Normal x(11);loaded_park(x);loaded_start(x);assert(service_resume(x)==c::BoomshotEvent::none);x.next();
  assert(service_resume(x)==c::BoomshotEvent::none&&gun::backend.seeks==1&&x.core()->stage()==c::BoomshotStage::interrupted);bone_q=1;
  assert(service_resume(x)==c::BoomshotEvent::restored&&gun::backend.seeks==1);++count;}
 {Normal x(12);loaded_park(x);loaded_start(x);++gun::backend.curve.content;
  assert(service_resume(x)==c::BoomshotEvent::interrupted&&gun::backend.seeks==0&&x.owner.blocks_fire(x.input.f));++count;}
 {Normal x(13);loaded_park(x);bone_q=0;closing_grant(x);fire_neutral=false;
  assert(resume_loaded(x)==c::BoomshotEvent::rejected&&gun::backend.plays==0&&native_calls==1);fire_neutral=true;
  ++lancerreload_inventory::owned_counts.reserve;x.refresh();assert(resume_loaded(x)==c::BoomshotEvent::rejected&&gun::backend.plays==0&&native_calls==1&&x.owner.blocks_fire(x.input.f));++count;}
 {Normal x(14);loaded_park(x);original_game_time=6.25f;loaded_restore(x);physical_close(x);
  assert(x.owner.complete(x.input.f)==c::BoomshotEvent::reload_completed&&original_reward_calls==0&&native_calls==1&&inventory_writer_calls==1);++count;}
 {Normal x(15);confirmed_insertion(x);normal_fault=33;assert(commit(x)==c::BoomshotEvent::interrupted);normal_fault=0;x.next();
  assert(x.owner.drain()&&x.core()->inventory().loaded==1&&visual_seat_calls==1&&original_exit_calls==1);
  bone_q=.5;closing_grant(x);assert(resume_loaded(x)==c::BoomshotEvent::started);
  assert(service_resume(x)==c::BoomshotEvent::none&&gun::backend.seeks==0);x.next();bone_q=0;assert(service_resume(x)==c::BoomshotEvent::none&&gun::backend.seeks==1);++count;}
 {Normal x(16);loaded_park(x);loaded_start(x);++gun::backend.identity.weapon.registry_index;
  assert(service_resume(x)==c::BoomshotEvent::rejected&&!x.owner.drain()&&gun::backend.finishes==0&&gun::backend.plays==1&&native_calls==1&&x.owner.lifecycle_pending());++count;}
 std::printf("Boomshot loaded closing recovery: %u cases passed; actual reload owner plus original gun-clip owner lifecycle/core/grant/events/Stroke, q1 retained, no second ForceReload/writer/seat/exit, later bone ACK, physical close and original4s; engine/curve/component/geometry boundaries stubbed, no live activation\n",count);return 0;
}
