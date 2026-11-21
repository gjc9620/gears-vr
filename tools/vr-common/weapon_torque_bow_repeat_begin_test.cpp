#include "weapon_torque_bow_driver_fixture.h"
#ifdef _WIN32
#include <windows.h>
#include <cstdlib>
#endif
namespace {
struct DelayedCost:Backend {
 unsigned observations{},reads{};std::int64_t observe_cost{};
 bool observe(h::Frame const& f,h::GripGameState const& game,d::Snapshot& out) noexcept {
  ++observations;time+=observe_cost;
  if(!game.known||!game.main_hand_held||game.busy||game.animation_owns_hands)return false;
  return Backend::observe(f,game,out);
 }
 R press(h::Frame const& f) noexcept {auto r=Backend::press(f);if(r==R::accepted)actual.original={false,false,false,true,false,false,false};return r;}
 bool read_owned(d::Snapshot& out) noexcept {++reads;return Backend::read_owned(out);}
 void original_tick(){assert(owner&&!fenced);actual.original.charging=true;}
};
using D=DriverFixture<DelayedCost>;
void begin(D& f){f.run();f.next();f.trigger(true);f.run();assert(f.backend.owner&&f.driver.stage()==w::TorqueBowStage::charge_begin);}
std::uint64_t waits(D const& f){
 return f.driver.diagnostic().same_frame_begin_waits;
}
void measured_repeat(){
 D f;begin(f);const auto observations=f.backend.observations,reads=f.backend.reads;
 f.backend.time=f.frame.controllers.sync_ns+88000000;f.backend.observe_cost=6000000;f.run();
 assert(f.backend.observations-observations==1&&f.backend.reads==reads&&waits(f)==1);
 assert(!f.backend.cancels&&f.backend.presses==1&&!f.backend.releases&&!f.backend.recorded);
 assert(f.backend.owner&&f.driver.pending()&&f.driver.stage()==w::TorqueBowStage::charge_begin);
 // Real state progress in this same Frame must keep both observations and positive ACK reads.
 f.backend.observe_cost=0;f.backend.original_tick();const auto count=f.backend.observations;f.run();
 assert(f.backend.observations-count==2&&waits(f)==1&&f.backend.reads>reads);
 assert(f.driver.stage()==w::TorqueBowStage::charging&&f.backend.presses==1);
 f.next(1000000);f.trigger(false);f.run();assert(f.backend.releases==1&&f.backend.recorded==1&&!f.driver.pending());
 std::puts("AFTER: same Frame waits without ACK/renewal/replay; state progress keeps full observations and realUP/projectile accounting");
}
void changed_frame_or_packet(){
 for(unsigned kind=0;kind<4;++kind){D f;begin(f);const auto count=f.backend.observations;
  if(kind==0)++f.frame.key.frame;
  if(kind==1)++f.frame.now_ns;
  if(kind==2){++f.frame.controllers.sequence;++f.frame.stereo.sequence;}
  if(kind==3){f.next();f.r3();}
  f.run();assert(f.backend.observations-count==2&&waits(f)==0&&f.backend.presses==1);
  if(kind==3)assert(f.backend.cancels==1&&!f.backend.releases&&!f.backend.recorded);
 }
}
void changed_state_or_tick(){
 for(unsigned kind=0;kind<8;++kind){D f;begin(f);const auto count=f.backend.observations;
  if(kind==0)f.backend.actual.original.pending_end_fire=true;
  if(kind==1)f.backend.actual.original.pending_fire=true;
  if(kind==2)f.backend.actual.original.firing=true;
  if(kind==3)f.backend.actual.original.animation_owns_hands=true;
  if(kind==4)f.backend.actual.original.r2_held=false;
  if(kind==5)++f.backend.actual.game_tick;
  if(kind==6)++f.backend.actual.reserve;
  if(kind==7)f.frame.left_grip_down=1;
  f.run();assert(f.backend.observations-count==2&&waits(f)==0&&f.backend.presses==1);
 }
}
void unchanged_expiry_and_revocation(){
 for(unsigned kind=0;kind<5;++kind){D f;begin(f);
  if(kind==0)f.backend.time=f.frame.controllers.sync_ns+100000001;
  if(kind==1)f.backend.veto=false;
  if(kind==2)f.game.busy=true;
  if(kind==3)f.backend.current=false;
  if(kind==4)++f.backend.hold_generation;
  f.run();assert(!waits(f)&&f.backend.cancels==1&&!f.backend.releases&&!f.backend.recorded);
 }
 D f;begin(f);f.backend.time=f.frame.controllers.sync_ns+80000000;f.run();assert(waits(f)==1);
 f.backend.time=f.frame.controllers.sync_ns+100000001;f.run();
 assert(waits(f)==1&&f.backend.cancels==1&&!f.driver.pending()); // wait did not renew old pose.
 D pre;begin(pre);pre.backend.preparable=false;
 pre.driver.before_original_tick(pre.backend,pre.frame,pre.game);
 assert(waits(pre)==1&&pre.backend.cancels==1&&!pre.backend.releases); // full pre-original prepare remains mandatory.
 D gap;begin(gap);gap.backend.time=gap.frame.controllers.sync_ns+88000000;gap.backend.observe_cost=6000000;gap.run();
 gap.backend.observe_cost=0;gap.backend.original_tick();gap.run();assert(gap.driver.stage()==w::TorqueBowStage::charging);
 gap.next(10000000);gap.trigger(false);gap.run();
 assert(gap.backend.cancels==1&&!gap.backend.releases&&!gap.backend.recorded); // new input gap still exceeds100ms.
 D deadline;begin(deadline);deadline.run();assert(waits(deadline)==1);
 deadline.next(60000000);deadline.run();assert(!deadline.backend.cancels);
 deadline.next(60000000);deadline.run();assert(deadline.backend.cancels==1&&!deadline.driver.pending());
}
}
int main(){
#ifdef _WIN32
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_set_error_mode(_OUT_TO_STDERR);
#endif
 measured_repeat();changed_frame_or_packet();changed_state_or_tick();unchanged_expiry_and_revocation();
 std::puts("same-Frame pending begin: changed Frame/packet/R3, state/game-clock/ammo, hold/current/veto/busy loss and fixed pose/request expiry checks passed; injected backend only");
}
