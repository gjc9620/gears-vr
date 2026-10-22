#include "weapon_torque_bow_interaction.h"
#include <cassert>
#include <cstdio>

using namespace gears_weapon;
using E=TorqueBowEvent;
using A=TorqueBowAction;
using S=TorqueBowStage;
using O=TorqueBowOutcome;
namespace {
// Synthetic adapter certifications only. None of these counts, parts, ticks,
// step orders or safe-abort receipts represents a measured original instance.
constexpr TorqueBowIdentity id{1,2,3,4,5,6,7,8};
constexpr TorqueBowOriginalState idle{true,false,false,false,false};
constexpr TorqueBowOriginalState charged{false,true,false,true,false};
constexpr TorqueBowOriginalState deferred{false,true,true,false,false};
TorqueBowConfig config() {
    TorqueBowConfig c{};c.identity=id;c.instance_config_serial=17;c.capacity=1;
    c.original_class_verified=c.original_charge_entry_verified=c.original_end_fire_verified=true;
    c.non_firing_abort_verified=c.manual_reload_phase_verified=c.reload_plan_verified=true;
    c.reload_step_count=3;
    c.reload_steps[0]={TorqueBowReloadAction::verified_mechanism,101};
    c.reload_steps[1]={TorqueBowReloadAction::insert_arrow,201};
    c.reload_steps[2]={TorqueBowReloadAction::verified_mechanism,301};return c;
}
struct Fixture {
    TorqueBowInteraction model{};
    std::uint64_t sequence{},original_event{};
    std::int64_t ns{1000000},tick{};
    explicit Fixture(std::uint32_t loaded=0,TorqueBowConfig const& c=config(),std::uint32_t reserve=3) {
        assert(model.bind(c,{1,loaded,reserve}));
    }
    TorqueBowStamp next(std::int64_t at=-1) {
        ++sequence;ns+=1000000;tick=at<0?tick+1:at;return {id,sequence,ns,tick};
    }
    E sample(bool down=false,TorqueBowOriginalState state=idle,
             bool right=true,bool left=true) {
        return model.observe({next(),model.inventory(),state,right,left,true,down});
    }
    E begin(bool known=true,std::uint64_t r3=1,std::uint64_t serial=1) {
        return model.begin_r3({next(1000),r3,serial,{id,serial,known?100:0,known},true,true});
    }
    E step(std::uint64_t component,std::uint64_t arrow,std::uint64_t since,std::int64_t at) {
        return model.complete_step(next(at),{17,1,component,since,arrow,true,true,true,true,true});
    }
    E grab(std::uint64_t serial=11) {
        return model.grab_left_shoulder(next(),{serial,TorqueBowAmmoSource::left_shoulder,true,true});
    }
    TorqueBowRequest take(A expected) {
        TorqueBowRequest r{};assert(model.take_request(r,ns));assert(r.action==expected);
        TorqueBowRequest duplicate{};assert(!model.take_request(duplicate,ns));return r;
    }
    E ack(TorqueBowRequest const& r,TorqueBowOriginalState state,bool accepted=true) {
        auto inventory=model.inventory();
        if(accepted&&r.action==A::complete_original_reload){inventory=r.after;++inventory.revision;}
        return model.acknowledge({next(),r.token,++original_event,accepted,inventory,state,
            accepted?r.outcome:O::none});
    }
    void open() {
        assert(sample()==E::observed);assert(begin()==E::reload_started);
        assert(step(101,0,sequence,1010)==E::step_completed);
    }
    void seat() {
        open();assert(grab()==E::arrow_grabbed);
        assert(step(201,11,sequence,1030)==E::step_completed);
    }
    void charge() {
        assert(sample()==E::observed);
        assert(model.trigger(next(),{1,true,true,true})==E::request_ready);
        const auto r=take(A::begin_original_charge);
        assert(ack(r,charged)==E::charge_started);assert(model.stage()==S::charging);
    }
};
void certified_configuration_only() {
    auto c=config();c.reload_plan_verified=false;c.reload_step_count=0;
    Fixture f(1,c);assert(f.sample()==E::observed);assert(f.begin()==E::rejected);
    c.non_firing_abort_verified=false;Fixture unsafe(1,c);assert(unsafe.sample()==E::observed);
    assert(!unsafe.model.can_request_fire(unsafe.ns,unsafe.model.inventory()));
    assert(unsafe.model.trigger(unsafe.next(),{1,true,true,true})==E::rejected);
    c=config();c.reload_step_count=0;assert(!c.valid());
    c=config();c.reload_steps[2].action=TorqueBowReloadAction::insert_arrow;assert(!c.valid());
    c=config();c.reload_steps[0].component=0;assert(!c.valid());
    c=config();c.identity.space_epoch=0;assert(!c.valid());
    c=config();c.manual_reload_phase_verified=false;assert(!c.valid());
    c=config();c.original_class_verified=false;assert(!c.valid());
    TorqueBowInteraction invalid;assert(!invalid.bind(config(),{1,2,3}));
    assert(!invalid.bind(config(),{0,0,3}));
    // A different certified order with no invented final mechanism is accepted.
    c=config();c.reload_step_count=1;c.reload_steps[0]={TorqueBowReloadAction::insert_arrow,901};
    Fixture direct(0,c);assert(direct.sample()==E::observed);
    assert(direct.begin()==E::reload_started);assert(direct.grab()==E::arrow_grabbed);
    const auto motion_begin=direct.sequence;
    assert(direct.model.complete_step(direct.next(1050),
        {17,1,901,motion_begin,11,true,true,true,true,true})==E::request_ready);
}
void guides_conservation_and_once_only_commit() {
    Fixture f;f.open();auto g=f.model.guide();
    assert(g.part==TorqueBowGuidePart::left_shoulder&&g.component==201);
    assert(g.red==1&&g.green==1&&g.blue==0&&g.alpha==1);
    assert(f.model.total()==3&&f.model.available_reserve()==3);
    assert(f.grab()==E::arrow_grabbed);
    assert(f.model.held_arrows()==1&&f.model.available_reserve()==2&&f.model.total()==3);
    assert(f.model.guide().part==TorqueBowGuidePart::arrow_feed);
    const auto grabbed=f.sequence;
    assert(f.model.release_arrow(f.next(),11)==E::arrow_released);
    assert(f.model.total()==3&&f.model.available_reserve()==3);
    assert(f.grab()==E::rejected); // Retired visual item cannot be reused.
    assert(f.grab(12)==E::arrow_grabbed);
    assert(f.step(201,12,grabbed,1030)==E::rejected); // Old motion cannot seat the replacement.
    assert(f.step(201,12,f.sequence-1,1031)==E::step_completed);
    assert(f.model.held_arrows()==0&&f.model.seated_uncommitted_arrows()==1&&f.model.total()==3);
    g=f.model.guide();assert(g.part==TorqueBowGuidePart::verified_mechanism&&g.component==301);
    assert(f.step(301,0,f.sequence,1040)==E::request_ready);
    assert(f.model.guide().part==TorqueBowGuidePart::none);
    assert(f.model.inventory().loaded==0&&f.model.inventory().reserve==3);
    const auto request=f.take(A::complete_original_reload);
    assert(request.before.loaded==0&&request.before.reserve==3&&request.after.loaded==1&&
        request.after.reserve==2&&request.arrow_serial==12&&request.outcome==O::perfect);
    assert(f.ack(request,idle)==E::perfect);
    assert(f.model.inventory().loaded==1&&f.model.inventory().reserve==2&&f.model.total()==3);
    assert(f.ack(request,idle)==E::rejected);
    assert(!f.model.can_request_fire(f.ns,f.model.inventory())); // Original recovery still required.
    assert(f.sample()==E::observed);assert(f.model.can_request_fire(f.ns,f.model.inventory()));
    auto stale=f.model.inventory();--stale.revision;assert(!f.model.can_request_fire(f.ns,stale));
}
void perfect_deadline_and_unknown_window() {
    for(const auto end: {1050,1100,1101,1300}){
        Fixture f;f.seat();assert(f.step(301,0,f.sequence,end)==E::request_ready);
        const auto r=f.take(A::complete_original_reload);
        assert(r.outcome==(end<=1100?O::perfect:O::ordinary));
        assert(f.ack(r,idle)==(end<=1100?E::perfect:E::ordinary));
    }
    Fixture unknown;assert(unknown.sample()==E::observed);
    assert(unknown.begin(false)==E::reload_started);
    assert(unknown.step(101,0,unknown.sequence,1010)==E::step_completed);
    assert(unknown.grab()==E::arrow_grabbed);
    assert(unknown.step(201,11,unknown.sequence,1020)==E::step_completed);
    assert(unknown.step(301,0,unknown.sequence,1030)==E::request_ready);
    const auto r=unknown.take(A::complete_original_reload);assert(r.outcome==O::ordinary);
    assert(unknown.ack(r,idle)==E::ordinary);
    Fixture wrong;assert(wrong.sample()==E::observed);auto s=wrong.next(1000);
    auto window=TorqueBowPerfectWindow{id,2,100,true};
    assert(wrong.model.begin_r3({s,1,1,window,true,true})==E::rejected);
    Fixture empty(0,config(),0);assert(empty.sample()==E::observed);
    assert(empty.begin()==E::no_ammo);assert(empty.begin()==E::rejected);
    Fixture full(1);assert(full.sample()==E::observed);assert(full.begin()==E::full);
    assert(!full.model.has_pending_request());
}
void reload_gates_and_tracking_fences() {
    Fixture f;f.open();
    assert(f.model.begin_r3({f.next(),2,2,{id,2,100,true},true,true})==E::rejected);
    assert(f.model.grab_left_shoulder(f.next(),{11,TorqueBowAmmoSource::other,true,true})==E::rejected);
    assert(f.model.grab_left_shoulder(f.next(),{11,TorqueBowAmmoSource::left_shoulder,false,true})==E::rejected);
    assert(f.grab()==E::arrow_grabbed);const auto since=f.sequence;
    auto bad=TorqueBowStepReceipt{17,1,201,since,11,true,true,true,true,true};
    bad.continuous=false;assert(f.model.complete_step(f.next(),bad)==E::rejected);
    bad.continuous=true;bad.component=301;assert(f.model.complete_step(f.next(),bad)==E::rejected);
    assert(f.model.tracking_lost()==E::observed);
    assert(f.model.held_arrows()==0&&f.model.available_reserve()==3&&f.model.total()==3);
    assert(f.sample()==E::observed);assert(f.grab()==E::rejected);assert(f.grab(12)==E::arrow_grabbed);
    assert(f.step(201,12,since,1040)==E::rejected);
    assert(f.step(201,12,f.sequence-1,1041)==E::step_completed);
    assert(f.model.tracking_lost()==E::observed);assert(f.model.seated_uncommitted_arrows()==1);
    const auto old=f.sequence;assert(f.sample()==E::observed);
    assert(f.step(301,0,old,1050)==E::rejected);
    assert(f.step(301,0,f.sequence-1,1051)==E::request_ready);
    const auto r=f.take(A::complete_original_reload);
    auto wrong=f.next();++wrong.identity.space_epoch;
    assert(f.model.acknowledge({wrong,r.token,1,true,r.after,idle,r.outcome})==E::rejected);
    assert(f.model.inventory().loaded==0&&f.model.total()==3);
    assert(f.ack(r,idle)==E::perfect);
}
void original_charge_release_and_actual_projectile() {
    Fixture f(1);f.charge();assert(f.model.inventory().loaded==1);
    assert(f.model.trigger(f.next(),{1,true,true,true})==E::rejected);
    assert(f.model.trigger(f.next(),{2,true,true,false})==E::request_ready);
    const auto end=f.take(A::end_original_fire);
    assert(end.requires_right_tracking&&end.before.loaded==end.after.loaded);
    assert(f.ack(end,deferred)==E::release_accepted);
    assert(f.model.stage()==S::await_original&&f.model.inventory().loaded==1);
    assert(f.model.trigger(f.next(),{3,true,true,true})==E::rejected);
    auto after=f.model.inventory();++after.revision;--after.loaded;
    const auto stamp=f.next();
    assert(f.model.record_original_shot({stamp,++f.original_event,11,after,true})==E::shot_recorded);
    assert(f.model.inventory().loaded==0&&f.model.total()==3);
    assert(f.model.record_original_shot({f.next(),f.original_event,11,after,true})==E::rejected);
    assert(f.sample()==E::observed);assert(!f.model.can_request_fire(f.ns,f.model.inventory()));
    Fixture hold_limit(1);hold_limit.charge();auto used=hold_limit.model.inventory();
    ++used.revision;--used.loaded;
    assert(hold_limit.model.record_original_shot({hold_limit.next(9000),
        ++hold_limit.original_event,12,used,true})==E::shot_recorded);
    assert(!hold_limit.model.has_pending_request()); // No synthetic VR release or charge timer.
}
void loss_never_generates_end_fire() {
    Fixture f(1);f.charge();assert(f.model.tracking_lost()==E::abort_required);
    assert(f.model.tracking_lost()==E::abort_required);
    const auto abort=f.take(A::abort_original_charge);assert(!abort.requires_right_tracking);
    assert(f.model.trigger(f.next(),{2,true,true,false})==E::abort_required);
    assert(f.ack(abort,idle)==E::aborted);assert(f.model.inventory().loaded==1);
    assert(f.sample(true)==E::observed);
    assert(f.model.trigger(f.next(),{3,true,true,true})==E::rejected);
    assert(f.sample()==E::observed);
    assert(f.model.trigger(f.next(),{4,true,true,true})==E::request_ready);
    const auto next=f.take(A::begin_original_charge);assert(next.token>abort.token);
    Fixture before_dispatch(1);assert(before_dispatch.sample()==E::observed);
    assert(before_dispatch.model.trigger(before_dispatch.next(),{1,true,true,true})==E::request_ready);
    assert(before_dispatch.model.tracking_lost()==E::aborted);
    assert(!before_dispatch.model.has_pending_request());
    Fixture waiting(1);assert(waiting.sample()==E::observed);
    assert(waiting.model.trigger(waiting.next(),{1,true,true,true})==E::request_ready);
    const auto begin=waiting.take(A::begin_original_charge);
    assert(waiting.model.tracking_lost()==E::abort_required);
    assert(waiting.ack(begin,charged)==E::abort_required);
    waiting.take(A::abort_original_charge);
    Fixture release_queued(1);release_queued.charge();
    assert(release_queued.model.trigger(release_queued.next(),{2,true,true,false})==E::request_ready);
    assert(release_queued.model.tracking_lost()==E::abort_required);
    release_queued.take(A::abort_original_charge);
}
void quick_tap_and_expiry() {
    Fixture f(1);assert(f.sample()==E::observed);
    assert(f.model.trigger(f.next(),{1,true,true,true})==E::request_ready);
    const auto begin=f.take(A::begin_original_charge);
    assert(f.model.trigger(f.next(),{2,true,true,false})==E::release_buffered);
    assert(f.ack(begin,charged)==E::request_ready);f.take(A::end_original_fire);
    Fixture expired(1);expired.charge();
    assert(expired.model.expire_pose(expired.ns+100000001)==E::abort_required);
    expired.take(A::abort_original_charge);
    Fixture stale(1);stale.charge();
    assert(stale.model.trigger(stale.next(),{2,true,true,false})==E::request_ready);
    TorqueBowRequest out{};assert(!stale.model.take_request(out,stale.ns+100000001));
    assert(stale.model.take_request(out,stale.ns+100000001)&&out.action==A::abort_original_charge);
    Fixture gap(1);gap.charge();gap.ns+=100000001;
    assert(gap.sample(true,charged)==E::abort_required);gap.take(A::abort_original_charge);
    Fixture bad_edge(1);bad_edge.charge();
    assert(bad_edge.model.trigger(bad_edge.next(),{2,false,true,false})==E::abort_required);
    bad_edge.take(A::abort_original_charge);
    Fixture late_start(1);assert(late_start.sample()==E::observed);
    assert(late_start.model.trigger(late_start.next(),{1,true,true,true})==E::request_ready);
    const auto delayed=late_start.take(A::begin_original_charge);late_start.ns+=100000001;
    assert(late_start.ack(delayed,charged)==E::abort_required);late_start.take(A::abort_original_charge);
    Fixture stale_tap(1);assert(stale_tap.sample()==E::observed);
    assert(stale_tap.model.trigger(stale_tap.next(),{1,true,true,true})==E::request_ready);
    const auto start=stale_tap.take(A::begin_original_charge);
    assert(stale_tap.model.trigger(stale_tap.next(),{2,true,true,false})==E::release_buffered);
    stale_tap.ns+=100000001;
    assert(stale_tap.ack(start,charged)==E::abort_required);stale_tap.take(A::abort_original_charge);
}
void sequence_clock_and_dispatch_gates() {
    Fixture f(1);f.charge();
    auto s=f.next();s.sequence=3;
    assert(f.model.trigger(s,{2,true,true,false})==E::rejected);
    s=f.next();s.monotonic_ns=4000000;
    assert(f.model.trigger(s,{2,true,true,false})==E::rejected);
    s=f.next();s.game_tick=0;
    assert(f.model.trigger(s,{2,true,true,false})==E::rejected);
    s=f.next();++s.identity.input_epoch;
    assert(f.model.trigger(s,{2,true,true,false})==E::rejected);
    assert(f.model.stage()==S::charging&&!f.model.has_pending_request());
    assert(f.model.trigger(f.next(),{2,true,true,false})==E::request_ready);
    f.take(A::end_original_fire);
    Fixture shared(1);assert(shared.sample()==E::observed);
    assert(shared.model.trigger(shared.next(),{1,true,true,true})==E::request_ready);
    assert(shared.sample(true,charged)==E::observed); // Another source acquired original R2.
    TorqueBowRequest out{};assert(!shared.model.take_request(out,shared.ns));
    assert(!shared.model.has_pending_request());
}
void contradictory_receipts_and_identity_change_fail_closed() {
    Fixture f(1);f.charge();auto changed=f.next();++changed.identity.weapon;
    assert(f.model.observe({changed,f.model.inventory(),charged,true,true,true,true})==E::abort_required);
    const auto abort=f.take(A::abort_original_charge);assert(abort.identity==id);
    assert(f.ack(abort,charged)==E::abort_required);assert(f.model.stage()==S::interrupted);
    assert(!f.model.can_request_fire(f.ns,f.model.inventory()));
    Fixture invalid_shot(1);invalid_shot.charge();auto wrong=invalid_shot.model.inventory();
    ++wrong.revision;--wrong.reserve; // A projectile cannot consume a reserve arrow directly.
    assert(invalid_shot.model.record_original_shot({invalid_shot.next(),
        ++invalid_shot.original_event,21,wrong,true})==E::abort_required);
    assert(invalid_shot.model.inventory().loaded==1&&invalid_shot.model.inventory().reserve==3);
    Fixture safety(1);safety.charge();assert(safety.model.tracking_lost()==E::abort_required);
    auto shot=safety.model.inventory();++shot.revision;--shot.loaded;
    assert(safety.model.record_original_shot({safety.next(),++safety.original_event,22,shot,true})==E::safety_violation);
    assert(safety.model.total()==3); // Account for the real shot, never claim safe recovery.
    const auto stop=safety.take(A::abort_original_charge);
    assert(safety.ack(stop,idle)==E::safety_violation);
    Fixture bad_reload;bad_reload.seat();assert(bad_reload.step(301,0,bad_reload.sequence,1040)==E::request_ready);
    const auto r=bad_reload.take(A::complete_original_reload);
    auto counts=r.after;++counts.revision;++counts.reserve;
    assert(bad_reload.model.acknowledge({bad_reload.next(),r.token,1,true,counts,idle,r.outcome})==E::interrupted);
    assert(bad_reload.model.inventory().loaded==0&&bad_reload.model.total()==3);
    TorqueBowRequest repeat{};assert(!bad_reload.model.take_request(repeat,bad_reload.ns));
}
}
int main() {
    certified_configuration_only();guides_conservation_and_once_only_commit();
    perfect_deadline_and_unknown_window();reload_gates_and_tracking_fences();
    original_charge_release_and_actual_projectile();loss_never_generates_end_fire();
    quick_tap_and_expiry();sequence_clock_and_dispatch_gates();
    contradictory_receipts_and_identity_change_fail_closed();
    std::puts("Torque Bow interaction tests passed (synthetic certified adapter fixtures).");
}
