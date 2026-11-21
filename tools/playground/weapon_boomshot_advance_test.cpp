#pragma push_macro("main")
#undef main
#define main previous_loaded_recovery_cases
#include "weapon_boomshot_loaded_recovery_test.cpp"
#undef main
#pragma pop_macro("main")
namespace {
c::boomshot_insertion::Geometry insertion_fixture(){
 c::boomshot_insertion::Geometry g{};g.id=444;g.outward={0,0,-1};g.travel_m=.1;g.approach_margin_m=.01;
 g.capture_radius_m=.01;g.retain_radius_m=.015;g.capture_angle_degrees=12;g.retain_angle_degrees=18;g.seat_tolerance_m=.002;g.verified=true;return g;
}
auto advance(Normal& x,c::boomshot_insertion::Geometry const& g){return x.owner.advance(x.input.f,x.input.d,x.input.route.grant(),g);}
void open_advance(Normal& x,c::boomshot_insertion::Geometry const& g){
 assert(advance(x,g)==c::BoomshotEvent::none&&x.owner.stage()==OwnerStage::prepared);x.next();assert(advance(x,g)==c::BoomshotEvent::started);
 x.next();advance(x,g);x.next();hand_q=bone_q=.5;advance(x,g);x.next();hand_q=bone_q=1;advance(x,g);x.next();
 assert(advance(x,g)==c::BoomshotEvent::step_completed&&x.core()->stage()==c::BoomshotStage::take_ammunition);
}
void take_advance(Normal& x,c::boomshot_insertion::Geometry const& g){x.shoulder_grant();assert(advance(x,g)==c::BoomshotEvent::grabbed);}
void insert_advance(Normal& x,c::boomshot_insertion::Geometry const& g){
 for(double depth:{.13,.1,.05,0.,0.}){x.next();boomshot_carrier_original::raw_depth=depth;
  const auto result=advance(x,g);if(x.core()->inventory().loaded)assert(result==c::BoomshotEvent::inventory_committed);else assert(result==c::BoomshotEvent::none);}
 assert(x.core()->inventory().loaded==1&&inventory_writer_calls==1&&visual_seat_calls==1);x.refresh();
}
auto close_advance(Normal& x,c::boomshot_insertion::Geometry const& g){closing_grant(x);hand_q=bone_q=1;const auto first=advance(x,g);
 if(!boomshot_barrel::stroke.active())std::fprintf(stderr,"close instance=%u first=%u owner=%u core=%u fault=%u\n",x.instance,unsigned(first),unsigned(x.owner.stage()),unsigned(x.core()->stage()),normal_fault);
 assert(boomshot_barrel::stroke.active());x.next();hand_q=bone_q=.5;assert(advance(x,g)==c::BoomshotEvent::none);
 x.next();hand_q=bone_q=0;assert(advance(x,g)==c::BoomshotEvent::none);x.next();return advance(x,g);}
}
int main(){unsigned count{};const auto geometry=insertion_fixture();const c::boomshot_insertion::Geometry absent{};
 for(unsigned infinite=0;infinite<2;++infinite){Normal x(1+infinite);if(infinite){lancerreload_inventory::owned_counts.reserve=0;lancerreload_inventory::owned_counts.infinite=true;x.refresh();}
  open_advance(x,geometry);take_advance(x,geometry);insert_advance(x,geometry);
  assert(close_advance(x,geometry)==c::BoomshotEvent::reload_completed&&!x.owner.pending()&&original_exit_calls==1&&original_reward_calls==1&&native_calls==1);
  assert(x.core()&&x.core()->stage()==c::BoomshotStage::ready&&inventory_writer_calls==1&&visual_seat_calls==1);++count;}
 {Normal x(3);assert(advance(x,absent)==c::BoomshotEvent::rejected&&!x.owner.pending()&&native_calls==0);++count;}
 {Normal x(4);assert(advance(x,geometry)==c::BoomshotEvent::none);x.next();assert(advance(x,absent)==c::BoomshotEvent::rejected&&!x.owner.pending()&&native_calls==0);++count;}
 {Normal x(5);open_advance(x,geometry);take_advance(x,geometry);x.next();x.input.f.left_grip_down=0;x.input.f.controllers.hands[0].squeeze=x.input.f.controllers.hands[0].trigger=0;x.refresh();
  assert(advance(x,absent)==c::BoomshotEvent::released&&x.core()->held_units()==0&&!boomshot_carrier_original::owner.active()&&inventory_writer_calls==0&&x.core()->inventory().reserve==2);++count;}
 {Normal x(6);open_advance(x,geometry);take_advance(x,geometry);normal_fault=34;
  for(unsigned i=0;i<3;++i){x.next();assert(advance(x,geometry)==c::BoomshotEvent::none&&x.core()->stage()==c::BoomshotStage::holding_ammunition&&inventory_writer_calls==0);}
  normal_fault=0;insert_advance(x,geometry);assert(native_calls==1&&boomshot_carrier_original::owner.grabs==1);++count;}
 {Normal x(7);open_advance(x,geometry);take_advance(x,geometry);normal_fault=35;insert_advance(x,geometry);
  assert(x.owner.stage()==OwnerStage::seating);x.next();assert(advance(x,geometry)==c::BoomshotEvent::none&&visual_seat_calls==1&&inventory_writer_calls==1);
  normal_fault=0;x.next();assert(advance(x,geometry)==c::BoomshotEvent::none&&x.owner.stage()==OwnerStage::mechanical);
  assert(close_advance(x,geometry)==c::BoomshotEvent::reload_completed&&visual_seat_calls==1&&original_exit_calls==1);++count;}
 {Normal x(8);open_advance(x,geometry);take_advance(x,geometry);insert_advance(x,geometry);normal_fault=28;
  assert(close_advance(x,geometry)==c::BoomshotEvent::none&&x.owner.stage()==OwnerStage::completing&&original_reward_calls==1);
  normal_fault=0;x.next();assert(advance(x,absent)==c::BoomshotEvent::reload_completed&&original_reward_calls==1&&original_exit_calls==1);++count;}
 {Normal x(9);assert(advance(x,geometry)==c::BoomshotEvent::none);x.next();normal_fault=2;
  assert(advance(x,geometry)==c::BoomshotEvent::interrupted&&native_calls==1);normal_fault=0;x.next();
  assert(advance(x,absent)==c::BoomshotEvent::interrupted&&native_calls==1&&original_exit_calls==0&&x.owner.lifecycle_pending());++count;}
 {Normal x(10);open_advance(x,geometry);take_advance(x,geometry);insert_advance(x,geometry);x.next();assert(x.owner.drain());
  bone_q=0;closing_grant(x);assert(advance(x,absent)==c::BoomshotEvent::started&&native_calls==1);
  assert(advance(x,absent)==c::BoomshotEvent::none&&gun::backend.seeks==1);x.next();bone_q=1;
  assert(advance(x,absent)==c::BoomshotEvent::restored);assert(close_advance(x,absent)==c::BoomshotEvent::reload_completed&&native_calls==1&&inventory_writer_calls==1);++count;}
 {Normal x(11);open_advance(x,geometry);take_advance(x,geometry);insert_advance(x,geometry);
  for(unsigned i=0;i<3;++i)advance(x,geometry);assert(inventory_writer_calls==1&&visual_seat_calls==1&&native_calls==1);
  assert(close_advance(x,geometry)==c::BoomshotEvent::reload_completed);advance(x,geometry);
  assert(original_reward_calls==1&&original_exit_calls==1&&native_calls==1&&inventory_writer_calls==1);++count;}
 {Normal x(12);assert(advance(x,geometry)==c::BoomshotEvent::none);x.next();
  during_original=[&](){assert(x.owner.stage()==OwnerStage::starting&&advance(x,geometry)==c::BoomshotEvent::rejected&&x.owner.blocks_fire(x.input.f));};
  assert(advance(x,geometry)==c::BoomshotEvent::started&&native_calls==1);++count;}
 std::printf("Boomshot sole GT advance: %u cases passed; actual owner/core/grant/events/raw open-insert-close Stroke, one input consumer per phase, same-GT once-only original q1/completion, finite/infinite, deferred attach/seat/reward, unknown start and loaded resume; engine/Rig/component/geometry boundaries stubbed, live certificate/Native dispatch unregistered\n",count);return 0;
}
