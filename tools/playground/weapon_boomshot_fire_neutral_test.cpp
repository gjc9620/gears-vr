#include "../vr-common/weapon_support_grip.h"
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <functional>
#include <limits>
namespace {
using uptr=std::uintptr_t;
bool gt=true,current=true,manual=true,boomer=true,debt{},release{},read_ok=true;
unsigned reads{};std::function<void()> during_read;
namespace weaponhand {unsigned current_gt(void*){return gt?1:0;}}
namespace nativeequipment {bool current(void*,gears_weapon_hand::Frame const&){return ::current;}}
namespace unified_weapon_fire {bool manual_profile(){return manual;}}
namespace playground {bool instance(uptr,wchar_t const*){return boomer;}}
namespace lancerrefire {bool has_initial_debt(){return debt;}bool release_required(){return release;}}
namespace vrfire {
namespace wh=gears_weapon_hand;
namespace lr {struct Game {bool lancer=true,r2_known=true,r2_held{};};}
enum {off=0,running=1,stopping=2};std::atomic<unsigned> mode{running};
struct Input {bool held{},waiting{};bool owns_press()const{return held;}bool awaiting_release()const{return waiting;}}input;
lr::Game actual{};
int actors(wh::Frame const&){return 1;}
bool read_game(int,wh::Frame const*,lr::Game& out){++reads;out=actual;if(during_read)during_read();return read_ok;}
#include "weapon_boomshot_fire_neutral.inl"
}
struct Fixture {
 gears_weapon_hand::Frame frame{};
 Fixture(){gt=current=manual=boomer=read_ok=true;debt=release=false;reads=0;during_read={};
  vrfire::input={};vrfire::actual={};vrfire::mode.store(vrfire::running);frame.controllers.hands[1].trigger_active=1;}
 bool neutral(){return vrfire::boomshot_inspection_neutral(frame);}
};
}
int main(){unsigned cases_count{};
 {Fixture v;assert(v.neutral()&&reads==1);v.frame.controllers.hands[1].trigger=.35f;assert(v.neutral());++cases_count;}
 for(unsigned reason=0;reason<15;++reason){Fixture v;
  switch(reason){
   case 0:gt=false;break;case 1:current=false;break;case 2:manual=false;break;case 3:boomer=false;break;
   case 4:vrfire::mode.store(vrfire::stopping);break;
   case 5:vrfire::input.held=true;break;case 6:vrfire::input.waiting=true;break;
   case 7:debt=true;break;case 8:release=true;break;
   case 9:v.frame.controllers.hands[1].trigger_active=0;break;
   case 10:v.frame.controllers.hands[1].trigger=.3501f;break;
   case 11:v.frame.controllers.hands[1].trigger=-.1f;break;
   case 12:v.frame.controllers.hands[1].trigger=std::numeric_limits<float>::quiet_NaN();break;
   case 13:vrfire::mode.store(vrfire::off);break;
   case 14:v.frame.controllers.hands[1].trigger=std::numeric_limits<float>::infinity();break;
  }
  assert(!v.neutral()&&!reads);++cases_count;
 }
 for(unsigned reason=0;reason<4;++reason){Fixture v;
  if(reason==0)read_ok=false;if(reason==1)vrfire::actual.lancer=false;
  if(reason==2)vrfire::actual.r2_known=false;if(reason==3)vrfire::actual.r2_held=true;
  assert(!v.neutral()&&reads==1);++cases_count;
 }
 for(unsigned reason=0;reason<6;++reason){Fixture v;during_read=[&]{
  switch(reason){case 0:current=false;break;case 1:vrfire::input.held=true;break;
   case 2:vrfire::input.waiting=true;break;case 3:debt=true;break;case 4:release=true;break;case 5:vrfire::mode.store(vrfire::stopping);break;}};
  assert(!v.neutral()&&reads==1);++cases_count;
 }
 std::printf("Boomshot R2 neutral: %u cases passed; native/XR up, owned press/release debt, same-source closing checks; read-only boundary stubs, no input writer\n",cases_count);
}
