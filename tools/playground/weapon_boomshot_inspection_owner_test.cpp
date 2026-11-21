#include "../vr-common/weapon_support_grip.h"
#include "../vr-common/weapon_boomshot_animation_tracks.h"
#include <cassert>
#include <cstdio>
#include <functional>
namespace boomshot_inspection_clip {
#include "weapon_boomshot_inspection_owner.inl"
}
namespace {
namespace clip=boomshot_inspection_clip;
struct Backend {
    struct Prepared{};
    bool admit=true,journal_allowed=true,capture_allowed=true,controls_allowed=true,context=true;
    bool active{},playing{},stop_effect=true,stop_dispatched{},finish_returned=true,finish_dispatched{},retired{};
    clip::Dispatch play_result=clip::Dispatch::returned,stop_result=clip::Dispatch::returned;
    unsigned prepared{},journals{},plays{},captures{},stops{},seeks{},finishes{},dropped{},selected=100,stopped_node{};
    std::function<void()> during_play;
    bool prepare(gears_weapon_hand::Frame const&,Prepared&){++prepared;return admit;}
    bool journal(gears_weapon_hand::Frame const&,std::uint64_t,Prepared const&){
        ++journals;if(active||!journal_allowed)return false;active=true;return true;
    }
    clip::Dispatch play(gears_weapon_hand::Frame const&,Prepared const&){
        if(play_result==clip::Dispatch::not_dispatched)return play_result;
        ++plays;if(during_play)during_play();selected=200;playing=true;return play_result;
    }
    bool drop_unstarted(){++dropped;active=false;return true;}
    bool capture(){++captures;return active&&context&&capture_allowed&&selected==200;}
    clip::Dispatch stop(){
        if(stop_result==clip::Dispatch::not_dispatched)return stop_result;
        ++stops;stopped_node=selected;stop_dispatched=true;if(stop_effect)playing=false;return stop_result;
    }
    bool stopped(){return active&&context&&stop_dispatched&&!playing;}
    bool controls(gears_weapon_hand::Frame const&){return controls_allowed&&context;}
    bool seek(gears_weapon_hand::Frame const&,double,gears_weapon::boomshot_barrel::Direction){
        if(!controls_allowed||!context)return false;++seeks;return true;
    }
    bool finish_started() const{return finish_dispatched;}
    bool retire_if_replaced(){if(!active||!retired)return false;active=false;return true;}
    bool finish(){
        if(!finish_dispatched){++finishes;finish_dispatched=true;}
        if(!finish_returned||!context)return false;active=false;playing=false;return true;
    }
};
struct Fixture {Backend backend;clip::Owner<Backend> owner{backend};gears_weapon_hand::Frame frame{};};
}
int main(){using R=clip::Result;using D=clip::Dispatch;using Direction=gears_weapon::boomshot_barrel::Direction;unsigned cases_count{};
 {Fixture v;v.backend.during_play=[&]{assert(v.owner.active()&&v.owner.serial()==7);assert(v.owner.begin(v.frame,8)==R::rejected);
   assert(v.owner.service(v.frame)==R::recovery_required&&!v.owner.drain());assert(!v.owner.seek(v.frame,.5,Direction::opening));};
  assert(v.owner.begin(v.frame,7)==R::ready&&v.backend.plays==1&&v.backend.stops==1&&v.backend.stopped_node==200);
  assert(v.owner.seek(v.frame,.5,Direction::opening)&&v.backend.seeks==1);
  assert(v.owner.drain()&&!v.owner.active()&&v.backend.finishes==1);
  assert(v.owner.drain()&&v.backend.finishes==1&&!v.owner.seek(v.frame,0,Direction::closing));
  const auto count=v.backend.prepared;assert(v.owner.begin(v.frame,7)==R::rejected&&v.backend.prepared==count);++cases_count;}
 for(unsigned reason=0;reason<2;++reason){Fixture v;if(reason)v.backend.journal_allowed=false;else v.backend.admit=false;
  assert(v.owner.begin(v.frame,7)==R::rejected&&!v.owner.active()&&!v.backend.plays&&!v.backend.stops);++cases_count;}
 {Fixture v;v.backend.play_result=D::not_dispatched;
  assert(v.owner.begin(v.frame,7)==R::rejected&&!v.owner.active()&&v.backend.dropped==1&&!v.backend.plays&&!v.backend.stops);
  assert(v.owner.begin(v.frame,7)==R::rejected);++cases_count;}
 {Fixture v;v.backend.capture_allowed=false;assert(v.owner.begin(v.frame,7)==R::recovery_required&&v.owner.active());
  for(unsigned i=0;i<8;++i)assert(v.owner.service(v.frame)==R::recovery_required);
  assert(v.backend.plays==1&&!v.backend.stops);v.backend.capture_allowed=true;
  assert(v.owner.service(v.frame)==R::ready&&v.backend.plays==1&&v.backend.stops==1);++cases_count;}
 {Fixture v;v.backend.stop_result=D::not_dispatched;assert(v.owner.begin(v.frame,7)==R::recovery_required&&!v.backend.stops);
  v.backend.stop_result=D::returned;assert(v.owner.service(v.frame)==R::ready&&v.backend.stops==1&&v.backend.plays==1);++cases_count;}
 {Fixture v;v.backend.stop_result=D::unknown;v.backend.stop_effect=false;
  assert(v.owner.begin(v.frame,7)==R::recovery_required&&v.backend.stops==1);
  for(unsigned i=0;i<8;++i)assert(v.owner.service(v.frame)==R::recovery_required);
  assert(v.backend.stops==1&&v.backend.plays==1);v.backend.playing=false;
  assert(v.owner.service(v.frame)==R::ready&&v.backend.stops==1);++cases_count;}
 {Fixture v;v.backend.play_result=D::unknown;assert(v.owner.begin(v.frame,7)==R::ready&&v.backend.plays==1&&v.backend.stops==1);++cases_count;}
 {Fixture v;v.backend.controls_allowed=false;
  assert(v.owner.begin(v.frame,7)==R::pending&&v.backend.stops==1);
  assert(!v.owner.seek(v.frame,.2,Direction::opening));v.backend.controls_allowed=true;
  assert(v.owner.service(v.frame)==R::ready&&v.backend.plays==1&&v.backend.stops==1);++cases_count;}
 {Fixture v;v.backend.capture_allowed=false;assert(v.owner.begin(v.frame,7)==R::recovery_required);
  assert(!v.owner.drain()&&v.owner.active()&&!v.backend.finishes);v.backend.capture_allowed=true;
  assert(v.owner.drain()&&!v.owner.active()&&v.backend.stops==1&&v.backend.finishes==1);++cases_count;}
 {Fixture v;assert(v.owner.begin(v.frame,7)==R::ready);v.backend.finish_returned=false;
  for(unsigned i=0;i<8;++i)assert(!v.owner.drain()&&v.owner.active());
  assert(v.backend.finishes==1&&v.backend.stops==1&&v.backend.plays==1);
  assert(!v.owner.seek(v.frame,0,Direction::closing));++cases_count;}
 {Fixture v;v.backend.active=true;assert(v.owner.begin(v.frame,7)==R::rejected&&!v.owner.active()&&!v.backend.plays);++cases_count;}
 {Fixture v;assert(v.owner.begin(v.frame,7)==R::ready);v.backend.context=false;
  assert(v.owner.service(v.frame)==R::recovery_required&&!v.owner.drain()&&v.owner.active()&&v.backend.stops==1);
  v.backend.context=true;assert(v.owner.drain()&&!v.owner.active());++cases_count;}
 {Fixture v;assert(v.owner.begin(v.frame,7)==R::ready);v.backend.finish_returned=false;assert(!v.owner.drain());
  const auto captured=v.backend.captures;v.backend.capture_allowed=false;v.backend.finish_returned=true;
  assert(v.owner.drain()&&!v.owner.active()&&v.backend.captures==captured&&v.backend.finishes==1);++cases_count;}
 {Fixture v;assert(v.owner.begin(v.frame,7)==R::ready);v.backend.context=false;
  assert(!v.owner.drain()&&v.owner.active());v.backend.retired=true;
  assert(v.owner.drain()&&!v.owner.active()&&!v.backend.finishes&&v.backend.stops==1);++cases_count;}
 std::printf("Boomshot inspection owner: %u lifecycle cases passed; post-Play node, journal-before-call, one dispatch, unknown debt, fresh seeks and exact handback; injected engine boundary only\n",cases_count);
}
