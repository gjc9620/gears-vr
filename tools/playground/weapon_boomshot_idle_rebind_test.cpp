#define main previous_inventory_owner_cases
#include "weapon_boomshot_inventory_owner_test.cpp"
#undef main
namespace {
void new_opening(Normal& x){x.next();++x.input.f.press_token;auto& left=x.input.f.controllers.hands[0];
 ++left.pressed[h::left_control(x.input.f)];++left.released[h::left_control(x.input.f)];x.input.route.reset();x.input.grant(c::BoomshotContact::opening);x.refresh();}
}
int main(){unsigned count{};
 {Normal x(1);assert(x.prepare());const auto id=x.core()->inventory().identity;assert(x.owner.cancel_unstarted());++x.input.f.key.lifecycle;new_opening(x);
  assert(x.prepare()&&x.core()->inventory().identity==id&&native_calls==0&&x.owner.cancel_unstarted());++count;}
 {Normal x(2);confirmed_close(x);assert(x.owner.complete(x.input.f)==c::BoomshotEvent::reload_completed);
  const auto before=lancerreload_inventory::identity(x.input.f);auto* prior=boomshot_sessions::find(before);const auto generation=prior->generation;
  const auto old_binding=prior->core(before)->binding_token();++x.input.f.key.lifecycle;lancerreload_inventory::owned_counts.used=1;hand_q=bone_q=0;new_opening(x);
  assert(x.prepare()&&boomshot_sessions::find(lancerreload_inventory::identity(x.input.f))==prior&&prior->generation==generation&&x.core()->binding_token()>old_binding&&inventory_writer_calls==1&&original_reward_calls==1);
  x.next();assert(x.start()==c::BoomshotEvent::started&&native_calls==2);++count;}
 for(unsigned bad=0;bad<4;++bad){Normal x(3+bad);assert(x.prepare()&&x.owner.cancel_unstarted());new_opening(x);
  if(bad==0)++x.input.f.controllers.input_epoch;if(bad==1)++x.input.f.stereo.space_epoch;if(bad==2)++x.input.f.key.native_started_qpc;if(bad==3)--x.input.f.key.lifecycle;
  x.refresh();assert(!x.prepare()&&!x.owner.pending()&&native_calls==0);++count;}
 {Normal x(7);x.open();assert(x.owner.drain());++x.input.f.key.lifecycle;new_opening(x);
  assert(!x.prepare()&&x.owner.pending()&&x.owner.blocks_fire(x.input.f)&&native_calls==1);++count;}
 {Normal x(8);assert(x.prepare()&&x.owner.cancel_unstarted());++x.input.f.key.lifecycle;bone_q=.5;new_opening(x);
  assert(!x.prepare()&&native_calls==0&&!x.owner.pending());++count;}
 std::printf("Boomshot idle hold rebind: %u cases passed; real owner/session/core, only current closed same-instance monotonic lifecycle may prepare again; binding nonce retained, native/input/space changes and parked debt refused; engine boundaries stubbed\n",count);return 0;
}
