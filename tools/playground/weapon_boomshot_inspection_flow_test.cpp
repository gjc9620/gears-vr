#define main previous_advance_flow_cases
#include "weapon_boomshot_advance_test.cpp"
#undef main
namespace {
const c::boomshot_insertion::Geometry no_insert_geometry{};
namespace gun=boomshot_inspection_clip;
void inspection_counts(Normal& x,unsigned loaded=1,int reserve=2,bool infinite=false){
 lancerreload_inventory::owned_counts={1,int(1-loaded),reserve,infinite};x.refresh();
}
void inspect_prepare(Normal& x){assert(advance(x,no_insert_geometry)==c::BoomshotEvent::none&&x.owner.stage()==OwnerStage::prepared&&x.core()->inspection_only());}
void inspect_start(Normal& x){inspect_prepare(x);x.next();assert(advance(x,no_insert_geometry)==c::BoomshotEvent::started&&gun::owner.active()&&gun::backend.plays==1&&!native_calls);}
void inspect_open(Normal& x){inspect_start(x);x.next();advance(x,no_insert_geometry);x.next();hand_q=bone_q=.5;advance(x,no_insert_geometry);
 x.next();hand_q=bone_q=1;advance(x,no_insert_geometry);x.next();assert(advance(x,no_insert_geometry)==c::BoomshotEvent::step_completed&&x.core()->step_index()==1&&x.core()->stage()==c::BoomshotStage::mechanism);}
void inspection_only_calls(){assert(!native_calls&&!native_reload&&!inventory_writer_calls&&!visual_seat_calls&&!original_exit_calls&&!original_reward_calls&&!partial_restore_calls&&!boomshot_carrier_original::owner.active());}
void fresh_inspection_grant(Normal& x,c::BoomshotContact purpose){
 x.next();++x.input.f.press_token;auto& left=x.input.f.controllers.hands[0];++left.pressed[h::left_control(x.input.f)];++left.released[h::left_control(x.input.f)];
 x.input.route.reset();x.input.grant(purpose);x.refresh();
}
void inspection_restore(Normal& x,double expected,c::BoomshotContact purpose){
 bone_q=0;fresh_inspection_grant(x,purpose);assert(advance(x,no_insert_geometry)==c::BoomshotEvent::started&&x.owner.stage()==OwnerStage::restoring);
 assert(advance(x,no_insert_geometry)==c::BoomshotEvent::none&&gun::backend.sought_q==expected);
 assert(advance(x,no_insert_geometry)==c::BoomshotEvent::none&&x.owner.stage()==OwnerStage::restoring);
 x.next();bone_q=expected;assert(advance(x,no_insert_geometry)==c::BoomshotEvent::restored&&x.owner.stage()==OwnerStage::mechanical);
 assert(!x.input.route.grant().begin_allowed(x.input.f,x.input.d,purpose,clock_value));
}
}
int main(){unsigned cases{};
 for(unsigned variation=0;variation<3;++variation){Normal x(1+variation);inspection_counts(x,variation==1?0:1,variation==1?0:2,variation==2);
  if(!variation)during_original=[&](){assert(x.owner.stage()==OwnerStage::starting&&x.owner.blocks_fire(x.input.f)&&advance(x,no_insert_geometry)==c::BoomshotEvent::rejected&&!x.owner.drain());};
  const auto before=lancerreload_inventory::owned_counts;inspect_open(x);assert(close_advance(x,no_insert_geometry)==c::BoomshotEvent::inspection_completed);
  assert(!x.owner.pending()&&x.core()->stage()==c::BoomshotStage::ready&&gun::backend.plays==1&&gun::backend.finishes==1&&lancerreload_inventory::owned_counts==before);
  inspection_only_calls();++cases;}
 {Normal x(4);inspection_counts(x);inspect_start(x);x.next();advance(x,no_insert_geometry);x.next();hand_q=bone_q=.5;advance(x,no_insert_geometry);
  const auto original_serial=x.owner.original_serial(),binding=x.core()->binding_token();assert(x.owner.drain()&&x.owner.blocks_fire(x.input.f)&&x.owner.stage()==OwnerStage::parked);
  ++x.input.f.key.lifecycle;inspection_restore(x,.5,c::BoomshotContact::opening);
  assert(x.owner.original_serial()==original_serial&&x.core()->binding_token()==binding&&gun::owner.serial()==2);
  fresh_inspection_grant(x,c::BoomshotContact::opening);hand_q=bone_q=.5;advance(x,no_insert_geometry);x.next();hand_q=bone_q=1;advance(x,no_insert_geometry);x.next();
  assert(advance(x,no_insert_geometry)==c::BoomshotEvent::step_completed);assert(close_advance(x,no_insert_geometry)==c::BoomshotEvent::inspection_completed);inspection_only_calls();++cases;}
 {Normal x(5);inspection_counts(x,0,0);inspect_open(x);closing_grant(x);hand_q=bone_q=1;advance(x,no_insert_geometry);x.next();hand_q=bone_q=.5;advance(x,no_insert_geometry);
  assert(x.owner.drain()&&x.owner.contact(x.input.f)==c::BoomshotContact::closing);++x.input.f.key.lifecycle;inspection_restore(x,.5,c::BoomshotContact::closing);
  fresh_inspection_grant(x,c::BoomshotContact::closing);hand_q=bone_q=.5;advance(x,no_insert_geometry);x.next();hand_q=bone_q=0;advance(x,no_insert_geometry);x.next();
  assert(advance(x,no_insert_geometry)==c::BoomshotEvent::inspection_completed&&gun::backend.plays==2&&gun::backend.finishes==2);inspection_only_calls();++cases;}
 {Normal x(6);inspection_counts(x);inspect_prepare(x);x.next();normal_fault=53;
  assert(advance(x,no_insert_geometry)==c::BoomshotEvent::interrupted&&x.owner.lifecycle_pending()&&gun::backend.plays==1&&!x.owner.drain());
  normal_fault=0;assert(x.owner.drain()&&x.owner.stage()==OwnerStage::parked&&gun::backend.plays==1&&x.owner.blocks_fire(x.input.f));inspection_only_calls();++cases;}
 {Normal x(7);inspection_counts(x);inspect_prepare(x);x.next();normal_fault=51;
  assert(advance(x,no_insert_geometry)==c::BoomshotEvent::rejected&&x.owner.stage()==OwnerStage::prepared&&!gun::owner.active());
  normal_fault=0;x.next();assert(advance(x,no_insert_geometry)==c::BoomshotEvent::started&&gun::owner.serial()==2&&gun::backend.plays==1);inspection_only_calls();++cases;}
 {Normal x(8);inspection_counts(x);inspect_open(x);normal_fault=58;
  assert(close_advance(x,no_insert_geometry)==c::BoomshotEvent::none&&x.owner.stage()==OwnerStage::completing&&gun::backend.finishes==1);
  x.next();assert(advance(x,no_insert_geometry)==c::BoomshotEvent::none&&gun::backend.finishes==1);normal_fault=0;
  source_failure=1;assert(x.owner.complete_owned()==c::BoomshotEvent::inspection_completed&&!x.owner.pending()&&gun::backend.finishes==1);inspection_only_calls();++cases;}
 {Normal x(9);inspection_counts(x);inspect_open(x);normal_fault=58;assert(close_advance(x,no_insert_geometry)==c::BoomshotEvent::none);normal_fault=0;
  lancerreload_inventory::owned_counts.used=1;assert(x.owner.complete_owned()==c::BoomshotEvent::none&&x.owner.blocks_fire(x.input.f));
  inspection_only_calls();++cases;}
 {Normal x(10);inspection_counts(x);lancerreload_state::lease={lancerreload_inventory::identity(x.input.f),888,true};
  assert(advance(x,no_insert_geometry)==c::BoomshotEvent::rejected&&!gun::backend.plays&&lancerreload_state::retains_serial(888));
  assert(!original_exit_calls&&!native_calls);++cases;}
 {Normal x(11);inspection_counts(x);inspect_open(x);assert(x.owner.drain());
  ++x.input.f.weapon.registry_index;fresh_inspection_grant(x,c::BoomshotContact::closing);
  assert(advance(x,no_insert_geometry)==c::BoomshotEvent::rejected&&x.owner.stage()==OwnerStage::parked&&gun::backend.plays==1);inspection_only_calls();++cases;}
 {Normal x(12);inspection_counts(x);source_failure=1;assert(advance(x,no_insert_geometry)==c::BoomshotEvent::rejected&&!x.owner.pending()&&!gun::backend.plays);inspection_only_calls();++cases;}
 {Normal x(13);assert(advance(x,no_insert_geometry)==c::BoomshotEvent::rejected&&!x.owner.pending()&&!gun::backend.plays);inspection_only_calls();++cases;}
 {Normal x(14);inspection_counts(x);inspect_prepare(x);x.next();lancerreload_inventory::owned_counts.used=1;x.refresh();
  assert(advance(x,no_insert_geometry)==c::BoomshotEvent::rejected&&!gun::backend.plays&&x.owner.drain());inspection_only_calls();++cases;}
 {Normal x(15);inspection_counts(x);++x.input.d.lease.id;assert(advance(x,no_insert_geometry)==c::BoomshotEvent::rejected&&!x.owner.pending()&&!gun::backend.plays);inspection_only_calls();++cases;}
 {Normal x(16);inspection_counts(x);inspect_open(x);assert(x.owner.drain()&&x.owner.stage()==OwnerStage::parked&&x.owner.blocks_fire(x.input.f)&&!x.owner.lifecycle_pending());
  retired_actor=x.input.f.weapon;assert(x.owner.retire_parked()&&!x.owner.pending()&&!x.core());inspection_only_calls();++cases;}
 std::printf("Boomshot normal-GT zero-target inspection: %u cases passed; same owner/core/grant/events/RAW Stroke, original gun-only clip lifecycle, unchanged counts/no reward/no native reload, interrupted open/close retain debt and old operation serial, fresh resume + later bone ACK; engine/source/geometry boundaries stubbed, no game proof\n",cases);
}
