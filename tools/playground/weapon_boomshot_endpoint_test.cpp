#include "../vr-common/weapon_boomshot_guide.h"
#define main previous_endpoint_skin_cases
#include "weapon_boomshot_skin_test.cpp"
#undef main
namespace {
FILE* log_file{};
namespace boomshot_endpoint_sound {
enum class Cue {opened,seated,closed};unsigned calls{};bool returned=true;
bool play(h::Frame const&,Cue,std::uint64_t,std::int64_t)noexcept{++calls;return returned;}
}
#include "weapon_boomshot_endpoint.inl"
namespace endpoint=boomshot_endpoint;using End=endpoint::Kind;using Receipt=endpoint::Receipt;
namespace boomshot_guide {
bool current(h::Frame const& f)noexcept{return nativeequipment::current(nullptr,f);}
bool draw(h::Frame const&,gears_boomshot_guide::Shape const&,std::int64_t)noexcept;
}
namespace boomshot_seating {bool read(h::Frame const&,c::boomshot_insertion::Geometry& out)noexcept{out=insertion_fixture();return true;}}
#define BOOMSHOT_GUIDE_TRANSPORT_BOUNDARY_FIXTURE 1
#include "weapon_boomshot_guide.inl"
#undef BOOMSHOT_GUIDE_TRANSPORT_BOUNDARY_FIXTURE
namespace boomshot_guide {
unsigned drawn_calls{};guide::Shape drawn_shape{};
bool draw(h::Frame const& f,guide::Shape const& shape,std::int64_t)noexcept{++drawn_calls;drawn_shape=shape;drawn_source=f.key;drawn_known=true;return true;}
}
void reset_endpoint(Normal& x){reset_skin(x);endpoint::display={};endpoint::consumed_ordinal=0;boomshot_endpoint_sound::calls=0;boomshot_endpoint_sound::returned=true;}
void discard_receipt(){Receipt receipt{};assert(boomshot_reload_owner::owner.take_endpoint(receipt));}
void insert_endpoint(Normal& x){for(double depth:{.13,.1,.05,0.,0.}){x.next();boomshot_carrier_original::raw_depth=depth;skin_advance(x);}}
void close_endpoint(Normal& x){closing_grant(x);hand_q=bone_q=1;skin_advance(x);x.next();hand_q=bone_q=.5;skin_advance(x);
 x.next();hand_q=bone_q=0;skin_advance(x);x.next();skin_advance(x);}
}
int main(){unsigned cases{};
 {Normal x(1);reset_endpoint(x);Receipt receipt{};assert(!boomshot_reload_owner::owner.take_endpoint(receipt));skin_open(x);
  assert(boomshot_reload_owner::owner.take_endpoint(receipt)&&receipt.kind==End::opened&&receipt.ordinal==1&&receipt.serial==777&&
   receipt.raw_sequence==x.input.f.controllers.sequence&&receipt.raw_sync_ns==x.input.f.controllers.sync_ns&&receipt.identity==lancerreload_inventory::identity(x.input.f));
  assert(!boomshot_reload_owner::owner.take_endpoint(receipt));x.next();skin_advance(x);assert(!boomshot_reload_owner::owner.take_endpoint(receipt));++cases;}
 {Normal x(2);reset_endpoint(x);skin_take(x);discard_receipt();insert_endpoint(x);Receipt receipt{};
  assert(boomshot_reload_owner::owner.take_endpoint(receipt)&&receipt.kind==End::seated&&receipt.ordinal==2&&inventory_writer_calls==1);
  close_endpoint(x);assert(boomshot_reload_owner::owner.take_endpoint(receipt)&&receipt.kind==End::closed&&receipt.ordinal==3);
  assert(!boomshot_reload_owner::owner.take_endpoint(receipt)&&original_exit_calls==1&&original_reward_calls==1);++cases;}
 {Normal x(3);reset_endpoint(x);skin_take(x);discard_receipt();normal_fault=35;insert_endpoint(x);Receipt receipt{};
  assert(inventory_writer_calls==1&&!boomshot_reload_owner::owner.take_endpoint(receipt)&&boomshot_reload_owner::owner.stage()==OwnerStage::seating);
  assert(!boomshot_reload_owner::owner.service_seating());normal_fault=0;x.next();assert(boomshot_reload_owner::owner.service_seating());
  assert(boomshot_reload_owner::owner.take_endpoint(receipt)&&receipt.kind==End::seated&&receipt.raw_sequence<x.input.f.controllers.sequence);
  assert(!boomshot_reload_owner::owner.service_seating()&&!boomshot_reload_owner::owner.take_endpoint(receipt)&&inventory_writer_calls==1);++cases;}
 {Normal x(4);reset_endpoint(x);skin_open(x);boomshot_endpoint_sound::returned=false;endpoint::sample(x.input.f);End kind{};assert(endpoint::view(x.input.f,kind)&&kind==End::opened&&endpoint::consumed_ordinal==1&&boomshot_endpoint_sound::calls==1);
  for(unsigned i=0;i<5;++i)x.next();assert(endpoint::view(x.input.f,kind)); // Cosmetic only, NOT renewed input/receipt.
  for(unsigned i=0;i<13;++i)x.next();assert(!endpoint::view(x.input.f,kind));endpoint::sample(x.input.f);assert(!endpoint::view(x.input.f,kind)&&boomshot_endpoint_sound::calls==1);++cases;}
 {Normal x(5);reset_endpoint(x);skin_open(x);for(unsigned i=0;i<3;++i)x.next();endpoint::sample(x.input.f);End kind{};
  assert(endpoint::consumed_ordinal==1&&!endpoint::view(x.input.f,kind)&&!boomshot_endpoint_sound::calls);++cases;}
 for(unsigned changed=0;changed<6;++changed){Normal x(6+changed);reset_endpoint(x);skin_open(x);const auto saved=x.input.f;
  switch(changed){case 0:++x.input.f.key.lifecycle;break;case 1:++x.input.f.controllers.profile_generation;break;case 2:++x.input.f.reference_generation;break;
   case 3:++x.input.f.weapon.name_number;break;case 4:--x.input.f.key.frame;break;case 5:x.input.f.left_tracked=0;break;}
  endpoint::sample(x.input.f);End kind{};assert(!endpoint::view(x.input.f,kind)&&endpoint::consumed_ordinal==1&&!boomshot_endpoint_sound::calls);
  x.input.f=saved;endpoint::sample(x.input.f);assert(!endpoint::view(x.input.f,kind));++cases;}
 {Normal x(12);reset_endpoint(x);skin_open(x);endpoint::lost();endpoint::sample(x.input.f);End kind{};
  assert(!endpoint::view(x.input.f,kind)&&!endpoint::consumed_ordinal);++cases;}
 {Normal x(13);reset_endpoint(x);skin_open(x);Receipt receipt{};is_gt=false;assert(!boomshot_reload_owner::owner.take_endpoint(receipt));is_gt=true;
  assert(boomshot_reload_owner::owner.take_endpoint(receipt)&&receipt.kind==End::opened);++cases;}
 {Normal x(14);reset_endpoint(x);assert(skin_advance(x)==c::BoomshotEvent::none);x.next();skin_advance(x);x.next();skin_advance(x);
  x.next();hand_q=1;bone_q=.5;skin_advance(x);Receipt receipt{};assert(!boomshot_reload_owner::owner.take_endpoint(receipt));++cases;}
 {Normal x(15);reset_endpoint(x);skin_open(x);endpoint::sample(x.input.f);auto game=h::GripGameState{};
  game.known=game.main_hand_held=true;game.kind=h::WeaponKind::firearm;boomshot_guide::drawn_known=false;boomshot_guide::drawn_calls=0;
  boomshot_guide::tick(x.input.f,game,x.input.d,x.input.route.grant());
  assert(boomshot_guide::drawn_calls==1&&boomshot_guide::drawn_shape.count==2&&boomshot_guide::drawn_shape.lines[0].red==40);
  x.next();++x.input.f.controllers.hands[0].released[gears_vr::control_squeeze];neutral(x);refresh_skin(x);
  boomshot_guide::tick(x.input.f,game,x.input.d,x.input.route.grant());
  assert(boomshot_guide::drawn_calls==2&&boomshot_guide::drawn_shape.count==16&&boomshot_guide::drawn_shape.lines[0].red==255&&
   boomshot_guide::drawn_shape.lines[14].red==40);++cases;}
 {Normal x(16);reset_endpoint(x);lancerreload_inventory::owned_counts={1,1,0,false};x.refresh();skin_open(x);Receipt receipt{};
  assert(boomshot_reload_owner::owner.take_endpoint(receipt)&&receipt.kind==End::opened);close_endpoint(x);
  assert(boomshot_reload_owner::owner.take_endpoint(receipt)&&receipt.kind==End::closed&&!native_calls&&!inventory_writer_calls&&!original_reward_calls);
  assert(!boomshot_reload_owner::owner.take_endpoint(receipt));++cases;}
 std::printf("Boomshot actual endpoint receipt/result: %u cases passed; sole owner later-bone ACK, q1 distinct from original visual-seat ACK, physical close, destructive once-only delivery, scope/age/loss/high-water, no target-as-completion; engine/source/geometry boundaries stubbed, no actual result pixels/audio proof\n",cases);
}
