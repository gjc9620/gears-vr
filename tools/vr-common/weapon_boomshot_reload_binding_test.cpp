#define main previous_core_regressions
#include "weapon_boomshot_interaction_test.cpp"
#undef main
#include <cassert>
#include <cstdio>
int main(){assert(previous_core_regressions()==0);unsigned count{};
 for(unsigned bad=0;bad<10;++bad){auto c=boom_config().reload();
  switch(bad){case 0:c.identity={};break;case 1:c.config_id=0;break;case 2:c.ammunition_type=0;break;
   case 3:c.ammunition_kind=BoomshotAmmoKind::projectile;break;case 4:c.capacity=2;break;
   case 5:c.maximum_sample_gap_ns=0;break;case 6:c.maximum_sample_gap_ns=100000001;break;
   case 7:c.current_instance_verified=false;break;case 8:c.original_class_verified=false;break;case 9:c.ammunition_kind=BoomshotAmmoKind::unknown;break;}
  BoomshotInteraction w;assert(!w.bind_reload(c,{boom_id,1,0,11}));++count;
 }
 for(bool infinite:{false,true}){BoomshotInteraction w;assert(w.bind_reload(boom_config().reload(),{boom_id,1,0,infinite?0u:11u,infinite}));
  auto s=boom_start(w.binding_token());s.perfect={boom_id,5,1,10,true};assert(w.observe(live_frame(w,1))==BoomshotEvent::motion_reset);
  assert(w.reload_input_current(live_frame(w,2)));auto wrong=live_frame(w,2);++wrong.stamp.motion_epoch;assert(!w.reload_input_current(wrong));
  assert(w.begin_reload(live_frame(w,2),s,1,ReloadIntentSource::mechanism_open)==BoomshotEvent::started);
  assert(w.complete_mechanism(live_frame(w,3),boom_motion(w,2))==BoomshotEvent::step_completed);
  assert(w.grab_left_shoulder(live_frame(w,4),boom_carrier())==BoomshotEvent::grabbed);
  assert(w.insert_ammunition(live_frame(w,5),{boom_motion(w,4),100,true,true,true})==BoomshotEvent::request_inventory);
  BoomshotInteraction::ReloadRequest q{};assert(w.take_reload_request(q));
  assert(w.acknowledge_inventory(boom_ack(w,q,6))==BoomshotEvent::inventory_committed);
  assert(w.complete_mechanism(live_frame(w,7),boom_motion(w,6))==BoomshotEvent::request_completion&&w.take_reload_request(q)&&q.result==ReloadTxnResult::perfect);
  assert(w.acknowledge_completion(boom_ack(w,q,8))==BoomshotEvent::reload_completed&&w.inventory().loaded==1&&w.inventory().reserve==(infinite?0u:10u));
  w.observe(live_frame(w,9));auto fire=live_frame(w,10);fire.original_can_fire=true;
  assert(w.request_original_fire(fire,1)==BoomshotEvent::rejected&&!w.can_request_fire());BoomshotFireRequest shot{};assert(!w.take_fire_request(shot));++count;
 }
 {BoomshotInteraction w;auto c=boom_config();c.original_fire_semantics_verified=false;assert(!w.bind(c,{boom_id,1,0,11})&&w.bind_reload(c.reload(),{boom_id,1,0,11}));++count;}
 std::printf("Boomshot original reload binding: %u cases plus original core regressions passed; full one-carrier inventory/close/perfect flow, no projectile authority, existing full-fire certificate unchanged; synthetic only\n",count);return 0;
}
