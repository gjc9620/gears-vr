#include "weapon_torque_bow_interaction.h"
#include <cassert>
#include <cstdio>
using namespace gears_weapon;
using E=TorqueBowEvent;using A=TorqueBowAction;using S=TorqueBowStage;using O=TorqueBowOutcome;
namespace {
// Synthetic adapter receipts. No fixture is retail ABI or physical evidence.
constexpr TorqueBowIdentity id{1,2,3,4,5,6,7,8};
constexpr TorqueBowOriginalState idle{true,false,false,false,false};
constexpr TorqueBowOriginalState charged{false,true,false,true,false};
constexpr TorqueBowOriginalState deferred{false,true,true,false,false};
TorqueBowConfig config(){
    TorqueBowConfig c{};c.identity=id;c.instance_config_serial=17;c.capacity=1;
    c.game_ticks_per_second=1000;
    c.original_class_verified=c.original_charge_entry_verified=c.original_end_fire_verified=true;
    c.non_firing_abort_verified=c.manual_reload_phase_verified=c.reload_plan_verified=true;
    c.reload_step_count=1;c.reload_steps[0]={TorqueBowReloadAction::insert_arrow,201};return c;
}
struct F {
    TorqueBowInteraction m;std::uint64_t seq{},events{};std::int64_t ns{1000000},tick{};
    explicit F(unsigned loaded=0,unsigned reserve=1){assert(m.bind(config(),{1,loaded,reserve}));}
    TorqueBowStamp next(std::int64_t at=-1){++seq;ns+=1000000;if(at>=0)tick=at;return {id,seq,ns,tick};}
    E sample(bool down=false,TorqueBowOriginalState state=idle,bool right=true,bool left=true){
        const auto stamp=next();return m.observe({stamp,m.inventory(),state,right,left,true,down,stamp.monotonic_ns});
    }
    E begin(){const auto s=next(tick<1000?1000:tick);return m.begin_left_shoulder({s,1,1,true,true,true,true,s.game_tick},
        {11,TorqueBowAmmoSource::left_shoulder,true,true,1,true,true});}
    E seat(std::int64_t at){auto start=seq;return m.complete_step(next(at),{17,1,201,start,11,true,true,true,true,true});}
    E edge(std::uint64_t n,bool down){return m.trigger(next(),{n,true,true,down});}
    E cancel(std::uint64_t n=1){return m.cancel_charge(next(),{n,true,true,true});}
    TorqueBowRequest take(A a){TorqueBowRequest r{};assert(m.take_request(r,ns));assert(r.action==a);
        TorqueBowRequest twice{};assert(!m.take_request(twice,ns));return r;}
    E ack(TorqueBowRequest const& r,TorqueBowOriginalState state=idle){
        auto i=m.inventory();if(r.action==A::complete_original_reload){i=r.after;++i.revision;}
        return m.acknowledge({next(),r.token,++events,true,i,state,r.outcome});
    }
    void charge(){assert(sample()==E::observed);assert(edge(1,true)==E::request_ready);
        auto r=take(A::begin_original_charge);assert(ack(r,charged)==E::charge_started);}
};
void reload_deadline(){
    for(auto at:{1000,4999,5000,5001,9000}){
        F f;assert(f.sample()==E::observed);assert(f.begin()==E::reload_started);
        assert(f.m.held_arrows()==1&&f.m.available_reserve()==0&&f.m.total()==1);
        assert(f.m.identity()==id&&f.m.held_arrow_serial()==11);
        assert(f.seat(at)==E::request_ready);auto r=f.take(A::complete_original_reload);
        assert(!f.m.last_loaded_token());
        assert(!f.m.held_arrow_serial()); // seated arrow no longer follows the hand
        assert(r.outcome==(at<=5000?O::perfect:O::ordinary));
        assert(f.ack(r)==(at<=5000?E::perfect:E::ordinary));
        assert(f.m.last_loaded_token()==r.token);
        assert(f.m.inventory().loaded==1&&f.m.inventory().reserve==0&&f.m.total()==1);
        assert(f.m.acknowledge({f.next(),r.token,++f.events,true,r.after,idle,r.outcome})==E::rejected);
    }
    F pause;pause.sample();pause.begin();
    for(int n=0;n<20;++n)assert(pause.sample()==E::observed); // clock paused, input remains fresh
    assert(pause.seat(1000)==E::request_ready);assert(pause.take(A::complete_original_reload).outcome==O::perfect);
}
void accepted_original_clock(){
    for(auto finish:{5000,5001}){
        F delayed;assert(delayed.sample()==E::observed);
        const auto receipt=delayed.next(1700);
        assert(delayed.m.begin_left_shoulder({receipt,1,1,true,true,true,true,1000},
            {11,TorqueBowAmmoSource::left_shoulder,true,true,1,true,true})==E::reload_started);
        assert(delayed.seat(finish)==E::request_ready);
        assert(delayed.take(A::complete_original_reload).outcome==(finish==5000?O::perfect:O::ordinary));
    }
    F zero;zero.sample();const auto receipt=zero.next(1400);
    const TorqueBowArrowReceipt arrow{11,TorqueBowAmmoSource::left_shoulder,true,true,1,true,true};
    assert(zero.m.begin_left_shoulder({receipt,1,1,true,true,true,true},arrow)==E::rejected);
    assert(zero.m.begin_left_shoulder({receipt,1,1,true,true,true,true,-1},arrow)==E::rejected);
    assert(zero.m.begin_left_shoulder({receipt,1,1,true,true,true,true,1401},arrow)==E::rejected);
    assert(zero.m.begin_left_shoulder({receipt,1,1,true,true,true,true,0},arrow)==E::reload_started);
    assert(zero.seat(4000)==E::request_ready);
    assert(zero.take(A::complete_original_reload).outcome==O::perfect);
    F already_reloading;assert(already_reloading.sample(false,{false})==E::observed);
    const auto accepted=already_reloading.next(1700);
    assert(already_reloading.m.begin_left_shoulder({accepted,1,1,true,true,true,true,1000},arrow)==E::reload_started);
    F not_active(1);assert(not_active.sample(false,{false})==E::observed);
    assert(not_active.edge(1,true)==E::rejected); // Reload receipt does not relax new Charge admission.
    F unsafe;unsafe.sample(false,{false,true});
    assert(unsafe.m.begin_left_shoulder({unsafe.next(1700),1,1,true,true,true,true,1000},arrow)==E::rejected);
}
void reservation_and_regrip(){
    F f;f.sample();assert(f.begin()==E::reload_started);auto before=f.m.inventory();
    assert(f.m.release_arrow(f.next(2500),11)==E::arrow_released);
    assert(!f.m.held_arrow_serial());
    assert(f.m.inventory().reserve==before.reserve&&f.m.available_reserve()==1);
    assert(f.begin()==E::rejected); // no restart/old clock rollback
    assert(f.m.grab_left_shoulder(f.next(),{12,TorqueBowAmmoSource::left_shoulder,true,true,2,true,true})==E::arrow_grabbed);
    assert(f.m.held_arrow_serial()==12);
    auto start=f.seq;assert(f.m.complete_step(f.next(5001),{17,1,201,start,12,true,true,true,true,true})==E::request_ready);
    assert(f.take(A::complete_original_reload).outcome==O::ordinary);
    F empty(0,0);empty.sample();assert(empty.begin()==E::no_ammo);assert(empty.m.held_arrows()==0);
    F full(1,1);full.sample();assert(full.begin()==E::full);assert(full.m.held_arrows()==0);
    F bad;bad.sample();auto s=bad.next(1000);
    assert(bad.m.begin_left_shoulder({s,1,1,true,true,false,true,s.game_tick},{11,TorqueBowAmmoSource::left_shoulder,true,true,1,true,true})==E::rejected);
    assert(bad.m.begin_left_shoulder({s,1,1,true,true,true,true,s.game_tick},{11,TorqueBowAmmoSource::other,true,true,1,true,true})==E::rejected);
    assert(bad.m.begin_left_shoulder({s,1,1,true,true,true,true,s.game_tick},{11,TorqueBowAmmoSource::left_shoulder,true,true,1,true,true})==E::reload_started);
    start=bad.seq;
    assert(bad.m.complete_step(bad.next(1200),{17,1,201,start,11,true,false,true,true,true})==E::rejected);
    assert(bad.m.complete_step(bad.next(1201),{17,1,201,start,11,true,true,false,true,true})==E::rejected);
    assert(bad.m.complete_step(bad.next(1202),{17,1,201,start,11,true,true,true,false,true})==E::rejected);
    assert(bad.m.tracking_lost()==E::observed);assert(bad.m.held_arrows()==0);
}
void cancellation(){
    F f(1);f.charge();assert(f.cancel()==E::abort_required);
    assert(f.cancel()==E::rejected);assert(!f.m.last_confirmed_cancel_edge());auto stop=f.take(A::abort_original_charge);
    assert(f.ack(stop)==E::aborted);assert(f.m.inventory().loaded==1);
    assert(f.m.last_confirmed_cancel_edge()==1);
    assert(f.sample(true)==E::observed);assert(f.edge(2,true)==E::rejected);
    assert(f.sample(false)==E::observed);assert(f.edge(3,true)==E::rejected); // level alone cannot rearm
    assert(f.edge(4,false)==E::observed);assert(f.edge(5,true)==E::request_ready);
    assert(f.take(A::begin_original_charge).before.loaded==1);
    F early(1);early.sample();assert(early.edge(1,true)==E::request_ready);
    assert(early.cancel()==E::aborted);assert(!early.m.has_pending_request());
    assert(!early.m.last_confirmed_cancel_edge()); // No original Charge was dispatched.
    assert(early.edge(2,false)==E::observed);assert(early.edge(3,true)==E::request_ready);
    F inflight(1);inflight.sample();inflight.edge(1,true);auto begin=inflight.take(A::begin_original_charge);
    assert(inflight.cancel()==E::abort_required);assert(inflight.ack(begin,charged)==E::abort_required);
    auto abort=inflight.take(A::abort_original_charge);assert(inflight.ack(abort)==E::aborted);
    F pending(1);pending.charge();assert(pending.edge(2,false)==E::request_ready);
    auto release=pending.take(A::end_original_fire);assert(pending.ack(release,deferred)==E::release_accepted);
    assert(pending.cancel()==E::abort_required);auto clear=pending.take(A::abort_original_charge);
    assert(pending.ack(clear)==E::aborted);assert(pending.m.inventory().loaded==1);
    assert(pending.m.last_confirmed_cancel_edge()==1);
    F canceled_loss(1);canceled_loss.charge();canceled_loss.cancel();canceled_loss.m.tracking_lost();
    auto lost_stop=canceled_loss.take(A::abort_original_charge);assert(canceled_loss.ack(lost_stop)==E::aborted);
    assert(!canceled_loss.m.last_confirmed_cancel_edge());
    F race(1);race.charge();race.edge(2,false);auto end=race.take(A::end_original_fire);
    assert(race.cancel()==E::abort_required);assert(race.ack(end,deferred)==E::abort_required);
    assert(race.take(A::abort_original_charge).before.loaded==1);
}
void release_shot_and_safety(){
    F shot(1);shot.charge();assert(shot.edge(2,false)==E::request_ready);
    auto end=shot.take(A::end_original_fire);assert(shot.ack(end,deferred)==E::release_accepted);
    auto ammo=shot.m.inventory();++ammo.revision;--ammo.loaded;
    assert(shot.m.record_original_shot({shot.next(),++shot.events,1,ammo,true})==E::shot_recorded);
    assert(shot.m.last_original_projectile_serial()==1&&!shot.m.last_confirmed_cancel_edge());
    assert(shot.cancel()==E::rejected);assert(shot.m.inventory().loaded==0);
    F tired(1);tired.charge();ammo=tired.m.inventory();++ammo.revision;--ammo.loaded;
    assert(tired.m.record_original_shot({tired.next(),++tired.events,1,ammo,true})==E::shot_recorded);
    assert(tired.edge(2,false)==E::request_ready);auto tired_up=tired.take(A::end_original_fire);
    assert(tired.ack(tired_up)==E::release_accepted);assert(tired.sample()==E::observed);
    assert(tired.m.stage()==S::ready&&tired.m.inventory().loaded==0); // No reload/refund/re-fire.
    F loss(1);loss.charge();assert(loss.m.tracking_lost()==E::abort_required);
    auto abort=loss.take(A::abort_original_charge);assert(loss.ack(abort)==E::aborted);
    assert(!loss.m.last_confirmed_cancel_edge());
    F stale(1);stale.charge();assert(stale.m.expire_pose(stale.ns+100000001)==E::abort_required);
    stale.take(A::abort_original_charge);
    F identity(1);identity.charge();auto s=identity.next();++s.identity.space_epoch;
    assert(identity.m.observe({s,identity.m.inventory(),charged,true,true,true,true,s.monotonic_ns})==E::abort_required);
    auto stop=identity.take(A::abort_original_charge);assert(identity.ack(stop)==E::safety_violation);
    F late(1);late.charge();late.m.tracking_lost();ammo=late.m.inventory();++ammo.revision;--ammo.loaded;
    assert(late.m.record_original_shot({late.next(),++late.events,1,ammo,true})==E::safety_violation);
    assert(late.m.inventory().loaded==0); // real projectile is never refunded
}
void charge_receipt_boundaries(){
 F tap(1);tap.sample();assert(tap.edge(1,true)==E::request_ready);
 assert(tap.edge(2,false)==E::release_buffered);auto begin=tap.take(A::begin_original_charge);
 assert(tap.ack(begin,charged)==E::request_ready);tap.take(A::end_original_fire);
 F late(1);late.sample();late.edge(1,true);auto pending=late.take(A::begin_original_charge);
 late.edge(2,false);late.ns+=100000001;
 assert(late.ack(pending,charged)==E::abort_required);late.take(A::abort_original_charge);
 F invalid(1);invalid.charge();invalid.cancel();auto stop=invalid.take(A::abort_original_charge);
 auto still_pending=idle;still_pending.pending_fire=true;
 assert(invalid.ack(stop,still_pending)==E::abort_required);assert(invalid.m.stage()==S::interrupted);
    assert(!invalid.m.last_confirmed_cancel_edge());
 F wrong(1);wrong.sample();wrong.edge(1,true);auto start=wrong.take(A::begin_original_charge);
 auto receipt=TorqueBowActionReceipt{wrong.next(),start.token+1,1,true,wrong.m.inventory(),charged,O::none};
 assert(wrong.m.acknowledge(receipt)==E::rejected);assert(wrong.m.has_pending_request());
 receipt.request_token=start.token;receipt.stamp=wrong.next();assert(wrong.m.acknowledge(receipt)==E::charge_started);
}
void certification(){
    auto c=config();c.game_ticks_per_second=0;assert(!c.valid());
    c=config();c.reload_step_count=2;assert(!c.valid());
    c=config();c.capacity=2;assert(!c.valid());
    c=config();c.non_firing_abort_verified=false;TorqueBowInteraction m;assert(m.bind(c,{1,1,1}));
    assert(m.observe({{id,1,1,0},{1,1,1},idle,true,true,true,false,1})==E::observed);
    assert(!m.can_request_fire(1,m.inventory()));
}
void original_sync_deadline(){
    // The same old source is current at processing, but the next command does
    // not acquire a new100ms deadline from the GT processing timestamp.
    for(auto age:{99000000LL,100000000LL}){
        F f(1);f.ns=200000000;const auto s=f.next();
        assert(f.m.observe({s,f.m.inventory(),idle,true,true,true,false,s.monotonic_ns-age})==E::observed);
        const auto event=f.edge(1,true);
        assert(event==(age==99000000?E::request_ready:E::observed));
        assert(f.m.has_pending_request()==(age==99000000));
    }
    for(unsigned bad=0;bad<4;++bad){
        F f(1);f.ns=200000000;auto s=f.next();
        const auto sync=bad==0?0:bad==1?s.monotonic_ns+1:bad==2?s.monotonic_ns-100000001:s.monotonic_ns;
        assert(f.m.observe({s,f.m.inventory(),idle,true,true,true,false,sync})==E::observed);
        if(bad==3){s=f.next();assert(f.m.observe({s,f.m.inventory(),idle,true,true,true,false,sync-1})==E::observed);}
        assert(f.edge(1,true)==E::observed&&!f.m.has_pending_request());
    }
    F charging(1);charging.charge();auto s=charging.next();
    assert(charging.m.observe({s,charging.m.inventory(),charged,true,true,true,true,s.monotonic_ns+1})==E::abort_required);
    charging.take(A::abort_original_charge); // Invalid tracking is never EndFire.
}
void inflight_begin_and_rearm(){
    F uncalled(1);uncalled.sample();uncalled.edge(1,true);
    auto ammo=uncalled.m.inventory();++ammo.revision;--ammo.loaded;
    assert(uncalled.m.record_original_shot({uncalled.next(),2,1,ammo,true})==E::rejected);
    assert(uncalled.m.inventory().loaded==1); // No root was dispatched.
    F called(1);called.sample();called.edge(1,true);auto begin=called.take(A::begin_original_charge);
    ammo=called.m.inventory();++ammo.revision;--ammo.loaded;
    assert(called.m.record_original_shot({called.next(),2,1,ammo,true})==E::shot_recorded);
    assert(called.m.acknowledge({called.next(),begin.token,3,false,called.m.inventory(),idle,{}})==E::rejected);
    assert(called.m.inventory().loaded==0&&called.m.original_shot_recorded());
    F canceled(1);canceled.charge();canceled.cancel();auto stop=canceled.take(A::abort_original_charge);
    assert(canceled.ack(stop)==E::aborted);canceled.sample();
    assert(canceled.m.observe_trigger_up(canceled.next(),false)==E::rejected);
    assert(canceled.m.observe_trigger_up(canceled.next(),true)==E::observed);
    assert(canceled.edge(2,true)==E::request_ready);
    auto next=canceled.take(A::begin_original_charge);assert(next.action==A::begin_original_charge);
    F held(1);held.charge();assert(held.m.observe_trigger_up(held.next(),true)==E::rejected);
    assert(!held.m.has_pending_request()); // An UP level cannot emit an original release.
}
}
int main(){reload_deadline();accepted_original_clock();reservation_and_regrip();cancellation();release_shot_and_safety();charge_receipt_boundaries();certification();original_sync_deadline();inflight_begin_and_rearm();
    std::puts("Torque Bow interaction regression passed; synthetic adapter receipts only.");}
