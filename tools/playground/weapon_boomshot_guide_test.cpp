#include "../vr-common/weapon_boomshot_guide.h"
#define main previous_skin_selection_cases
#include "weapon_boomshot_skin_test.cpp"
#undef main
namespace {
FILE* log_file{};
namespace boomshot_guide {
bool current(h::Frame const& f)noexcept{return nativeequipment::current(nullptr,f);}
bool draw(h::Frame const&,gears_boomshot_guide::Shape const&,std::int64_t)noexcept;
}
namespace boomshot_seating {bool read(h::Frame const&,c::boomshot_insertion::Geometry& out)noexcept{out=insertion_fixture();return true;}}
namespace boomshot_endpoint {enum class Kind{none,opened,seated,closed};bool view(h::Frame const&,Kind&)noexcept{return false;}}
#define BOOMSHOT_GUIDE_TRANSPORT_BOUNDARY_FIXTURE 1
#include "weapon_boomshot_guide.inl"
#undef BOOMSHOT_GUIDE_TRANSPORT_BOUNDARY_FIXTURE
namespace boomshot_guide {
unsigned draw_calls{};guide::Shape drawn{};
bool draw(h::Frame const& f,guide::Shape const& shape,std::int64_t)noexcept{++draw_calls;drawn=shape;drawn_source=f.key;drawn_known=true;return true;}
}
namespace cue=boomshot_guide;namespace guide=gears_boomshot_guide;
h::GripGameState guide_game(){h::GripGameState game{};game.known=game.main_hand_held=true;game.kind=h::WeaponKind::firearm;return game;}
bool view(Normal& x,guide::View& out,c::boomshot_insertion::Geometry geometry=insertion_fixture()){
 refresh_skin(x);return cue::select(x.input.f,guide_game(),x.input.d,x.input.route.grant(),boomshot_skin_source::supplied,geometry,out);
}
}
int main(){unsigned cases{};guide::View hint{};
 {Normal x(1);reset_skin(x);neutral(x);assert(view(x,hint)&&hint.step==guide::Step::opening);++cases;}
 {Normal x(2);reset_skin(x);skin_open(x);assert(!view(x,hint));neutral(x);assert(view(x,hint)&&hint.step==guide::Step::shoulder);++cases;}
 {Normal x(3);reset_skin(x);skin_take(x);assert(view(x,hint)&&hint.step==guide::Step::entry&&!hint.entered);
  x.next();boomshot_carrier_original::raw_depth=.13;skin_advance(x);x.next();boomshot_carrier_original::raw_depth=.1;skin_advance(x);
  assert(view(x,hint)&&hint.step==guide::Step::entry&&hint.entered);++cases;}
 {Normal x(4);reset_skin(x);skin_take(x);for(double depth:{.13,.1,.05,0.,0.}){x.next();boomshot_carrier_original::raw_depth=depth;skin_advance(x);}
  assert(!view(x,hint));closing_grant(x);assert(view(x,hint)&&hint.step==guide::Step::closing&&inventory_writer_calls==1);++cases;}
 {Normal x(5);reset_skin(x);neutral(x);x.input.d.lease={h::LeaseKind::cover,0,x.input.f.key.lifecycle,1};assert(!view(x,hint));++cases;}
 {Normal x(6);reset_skin(x);neutral(x);refresh_skin(x);auto game=guide_game();game.object_incumbents[1]={h::LeaseKind::other,0,x.input.f.key.lifecycle,1};
  assert(!cue::select(x.input.f,game,x.input.d,x.input.route.grant(),boomshot_skin_source::supplied,insertion_fixture(),hint));++cases;}
 {Normal x(7);reset_skin(x);neutral(x);assert(!view(x,hint,{}));lancerreload_inventory::owned_counts.used=0;x.refresh();
  assert(view(x,hint,{})&&hint.step==guide::Step::opening);lancerreload_inventory::owned_counts={1,1,0,false};x.refresh();
  assert(view(x,hint,{})&&hint.step==guide::Step::opening);++cases;}
 {Normal x(8);reset_skin(x);neutral(x);bone_q=.5;assert(!view(x,hint));++cases;}
 {Normal x(9);reset_skin(x);neutral(x);source_failure=5;assert(!view(x,hint));++cases;}
 {Normal x(10);reset_skin(x);neutral(x);refresh_skin(x);cue::drawn_known=false;cue::draw_calls=0;
  cue::tick(x.input.f,guide_game(),x.input.d,x.input.route.grant());assert(cue::draw_calls==1&&cue::drawn.count==17);
  cue::tick(x.input.f,guide_game(),x.input.d,x.input.route.grant());assert(cue::draw_calls==1);
  x.next();neutral(x);skin_source_known=false;cue::tick(x.input.f,guide_game(),x.input.d,x.input.route.grant());
  assert(cue::draw_calls==1&&!native_calls&&!inventory_writer_calls);++cases;}
 {Normal x(11);reset_skin(x);neutral(x);for(auto status:{h::DecisionStatus::invalid,h::DecisionStatus::ambiguous,h::DecisionStatus::inactive,h::DecisionStatus::needs_release}){
   x.input.d.status=status;assert(!view(x,hint));}++cases;}
 std::printf("Boomshot actual owner guide selection: %u cases passed; open/shoulder/current carrier-entry/close, release-required and foreign holders, inspection-vs-reload source requirements, per-frame transient dispatch; current Source/Engine draw boundary stubbed, no actual yellow pixel proof\n",cases);
}
