#include "weapon_torque_bow_reload_fixture.h"
namespace {
using ReloadF=DriverFixture<ReloadBackend>;
void load(ReloadF& f,std::int64_t at=3500){
 assert(f.run()==d::Status::baseline);f.next();f.run();h::Decision decision{};decision.new_grant=true;
 assert(f.driver.begin_reload(f.backend,f.frame,f.game,decision)==d::Status::owned);
 assert(f.driver.stage()==w::TorqueBowStage::reload_steps&&f.driver.interaction().held_arrow_serial()==f.backend.arrow_id);
 const auto begin=f.frame.controllers.sequence;f.next();f.backend.actual.game_tick=at;f.run();
 d::ReloadMotion m{1,f.backend.serial,201,f.backend.arrow_id,begin,true,true,true,true,true};
 assert(f.driver.complete_reload_motion(f.backend,f.frame,f.game,m));
 assert(!f.driver.pending()&&!f.backend.reload_owned()&&f.backend.completions==1&&f.backend.retirements==1);
 assert(f.driver.inventory().loaded==1&&f.driver.inventory().reserve==4&&f.driver.interaction().last_loaded_token());
 assert(!f.driver.complete_reload_motion(f.backend,f.frame,f.game,m));
}

d::ReloadMotion motion(ReloadF const& f,std::uint64_t begin){
 d::ReloadMotion m{};m.instance_config_serial=1;m.original_reload_serial=f.backend.serial;m.component=201;
 m.arrow_serial=f.backend.arrow_id;m.begin_input_sequence=begin;
 m.geometry_verified=m.aligned=m.continuous=m.complete=m.tracked_left_hand=true;return m;
}
void delayed_acceptance_and_completion(){
 ReloadF f;f.backend.actual.loaded=0;f.run();f.next();f.run();h::Decision decision{};decision.new_grant=true;
 f.backend.accept_wait=true;assert(f.driver.begin_reload(f.backend,f.frame,f.game,decision)==d::Status::owned);
 for(int i=0;i<3;++i){f.next();f.backend.actual.game_tick+=500;f.run();assert(f.backend.starts==1&&f.driver.pending()&&f.driver.inventory().loaded==0);}
 f.backend.accept_wait=false;f.next();f.run();assert(f.driver.stage()==w::TorqueBowStage::reload_steps);const auto begin=f.frame.controllers.sequence;
 f.next();f.backend.actual.game_tick=6001;f.run();auto m=motion(f,begin);
 auto bad=m;std::swap(bad.arrow_serial,bad.begin_input_sequence);assert(!f.driver.complete_reload_motion(f.backend,f.frame,f.game,bad));
 f.backend.receipt_wait=true;assert(f.driver.complete_reload_motion(f.backend,f.frame,f.game,m));assert(f.backend.completions==1&&f.backend.retirements==0&&f.driver.pending());
 assert(f.backend.request.outcome==w::TorqueBowOutcome::ordinary); // original1000, not delayed acceptance2500
 for(int i=0;i<3;++i){f.next();f.run();assert(f.backend.completions==1&&f.driver.inventory().loaded==0&&f.backend.actual.loaded==1);}
 f.backend.receipt_wait=false;f.next();f.run();assert(f.backend.retirements==1&&!f.driver.pending()&&f.driver.inventory().loaded==1&&f.driver.inventory().reserve==4);
 f.next();f.run();assert(f.backend.completions==1&&f.backend.retirements==1);
}
void held_reload_requires_release(){
 ReloadF f;f.backend.actual.loaded=0;f.frame.controllers.hands[1].trigger=1;load(f);
 for(int i=0;i<3;++i){f.next();f.run();assert(!f.backend.presses&&!f.backend.recorded);}
 f.next();f.trigger(false);f.run();f.next();f.trigger(true);f.run();assert(f.backend.presses==1&&f.backend.owner);
 f.next();f.trigger(false);f.run();assert(f.backend.recorded==1&&f.driver.inventory().loaded==0);
}
void completion_rejection_keeps_responsibility(){
 ReloadF f;f.backend.actual.loaded=0;f.run();f.next();f.run();h::Decision decision{};decision.new_grant=true;
 assert(f.driver.begin_reload(f.backend,f.frame,f.game,decision)==d::Status::owned);const auto begin=f.frame.controllers.sequence;
 f.next();f.run();f.backend.permit=false;auto m=motion(f,begin);assert(f.driver.complete_reload_motion(f.backend,f.frame,f.game,m));
 assert(f.driver.pending()&&f.backend.reload_owned()&&f.backend.actual.loaded==0&&f.backend.actual.reserve==5&&!f.backend.completions&&!f.backend.presses&&!f.backend.recorded);
 assert(!f.driver.reset_drained(f.backend)); // No fabricated original rejection receipt.
}
void full_chain(){ReloadF f;f.backend.actual.loaded=0;load(f);
 f.next();f.run();f.next();f.trigger(true);f.run();assert(f.backend.owner&&f.backend.presses==1);
 f.next();f.trigger(false);f.run();assert(f.backend.recorded==1&&f.backend.releases==1&&!f.backend.owner&&f.driver.inventory().loaded==0&&f.driver.inventory().reserve==4);
 // A second real reload on this same driver, then R3 cancellation and release rearm.
 f.next();f.run();h::Decision d{};d.new_grant=true;++f.backend.grip;assert(f.driver.begin_reload(f.backend,f.frame,f.game,d)==d::Status::owned);
 const auto begin=f.frame.controllers.sequence;f.next();f.run();d::ReloadMotion m{1,f.backend.serial,201,f.backend.arrow_id,begin,true,true,true,true,true};assert(f.driver.complete_reload_motion(f.backend,f.frame,f.game,m));
 assert(f.driver.inventory().loaded==1&&f.driver.inventory().reserve==3);f.next();f.run();f.next();f.trigger(true);f.run();assert(f.backend.presses==2);
 f.next();f.r3();f.run();assert(f.backend.cancels==1&&f.backend.recorded==1&&f.backend.releases==1&&f.driver.inventory().loaded==1);
 f.next();f.run();assert(f.backend.presses==2);f.next();f.trigger(false);f.run();f.next();f.trigger(true);f.run();assert(f.backend.presses==3);
 f.next();f.trigger(false);f.run();assert(f.backend.recorded==2&&f.backend.releases==2&&f.driver.inventory().loaded==0&&f.driver.inventory().reserve==3);
}
}
int main(){
#ifdef _WIN32
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
 full_chain();delayed_acceptance_and_completion();held_reload_requires_release();completion_rejection_keeps_responsibility();std::puts("Production Bow driver: composed load/fire/reload/R3/held-no-restart/realUP/newfire, delayed original acceptance/start clock, delayed completion accounting, swapped motion identity refusal and undispatched completion responsibility passed; injected original backend only, not game acceptance.");}