#include "weapon_hammer_of_dawn_interaction.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <type_traits>

using namespace gears_weapon;
using E=HammerOfDawnEvent;
using A=HammerOfDawnAction;
using I=HammerOfDawnInput;
using P=HammerOfDawnPhase;
using H=HammerOfDawnHands;
static_assert(!std::is_copy_constructible<HammerOfDawnInteraction>::value&&
    !std::is_move_constructible<HammerOfDawnInteraction>::value,"One owner per original input lease");
namespace {
unsigned checks{};
void check(bool good,char const* expression,int line) {
    ++checks;
    if(!good){std::fprintf(stderr,"line %d: %s\n",line,expression);std::exit(1);}
}
#define CHECK(expression) check(bool(expression),#expression,__LINE__)
constexpr HammerOfDawnIdentity id{11,12,13,14,15,16,17,18,19};
// All certificates, resource quantities, game phases and clocks below are
// synthetic. These tests establish no actual Hammer of Dawn runtime rules.
HammerOfDawnConfig config() {
    HammerOfDawnConfig c{};c.identity=id;c.binding=20;c.instance_config=21;
    c.pose_mapping=22;c.resource_rule=23;c.maximum_pose_age_ns=1000;
    c.resources=HammerOfDawnResourceRule::original_managed;
    c.current_instance_verified=c.original_class_verified=c.original_l_hold_semantics_verified=true;
    c.original_r_semantics_verified=c.original_release_semantics_verified=true;
    c.non_firing_cancel_verified=c.original_conditions_and_target_check_verified=true;
    c.original_effects_and_recoil_preserved=c.no_regular_reload_verified=true;
    return c;
}
struct Fixture {
    HammerOfDawnInteraction model{};
    HammerOfDawnConfig certificate{};
    HammerOfDawnOriginalState original{};
    HammerOfDawnResources resources{1,23,777};
    std::uint64_t sequence{},ticket{},original_event{},edge_serial{},effect_serial{};
    std::int64_t ns{100};
    bool aim_down{},fire_down{};
    explicit Fixture(HammerOfDawnConfig c=config()):certificate(c) {
        if(c.resources==HammerOfDawnResourceRule::no_consumable)resources.units=0;
        original.revision=1;original.phase=P::idle;
        original.current_instance_verified=original.equipped=original.ordinary_player=true;
        original.input_allowed=original.manager_enabled=original.conditions_allow_use=true;
        original.resources_allow_use=true;
        CHECK(model.bind(certificate,resources));
        CHECK(model.observe(sample())==E::hands_restored);
    }
    HammerOfDawnStamp stamp() {
        return {id,certificate.binding,++sequence,ticket,++ns};
    }
    HammerOfDawnSample sample() {
        ++ticket;++original.revision;
        HammerOfDawnSample s{};s.stamp=stamp();s.original=original;s.resources=resources;
        s.pose.ray={{1,2,3},{0,0,-1}};
        s.pose.frame_ticket=ticket;s.pose.mapping=certificate.pose_mapping;
        s.pose.measured_ns=ns;s.pose.right_tracked=s.pose.left_tracked=true;
        s.pose.right_weapon_held=s.pose.origin_and_axis_from_weapon=s.pose.grip_to_indicator_clear=true;
        s.target={ticket,ticket+100,ticket,true,true,true,true};
        s.left_trigger_down=aim_down;s.right_trigger_down=fire_down;return s;
    }
    E edge(I input,bool down) {
        if(input==I::aim)aim_down=down;else fire_down=down;
        model.observe(sample());
        return model.edge(stamp(),{++edge_serial,input,down,true});
    }
    HammerOfDawnRequest take() {
        HammerOfDawnRequest r{};CHECK(model.take_request(r,ns));return r;
    }
    HammerOfDawnReceipt receipt(HammerOfDawnRequest const& request,bool accepted=true) {
        if(accepted){
            switch(request.action){
                case A::press_original_aim:original.aim_input_down=original.aiming=true;break;
                case A::release_original_aim:original.aim_input_down=original.aiming=false;break;
                case A::press_original_fire:original.fire_input_down=true;original.phase=P::warming;break;
                case A::release_original_fire:original.fire_input_down=false;original.phase=P::idle;break;
                case A::cancel_original_inputs:
                    if(request.owned_aim)original.aim_input_down=original.aiming=false;
                    if(request.owned_fire){original.fire_input_down=false;original.phase=P::idle;}
                    break;
                default:break;
            }
        }
        ++original.revision;
        HammerOfDawnReceipt r{stamp(),request.token,certificate.instance_config,++original_event,
            original,resources,accepted,false};
        r.stop={id.weapon,id.weapon_generation,request.use_cycle,r.original_event_serial,
            true,true,true,true,true,true,true};
        return r;
    }
    E ack(HammerOfDawnRequest const& request,bool accepted=true) {
        return model.acknowledge(receipt(request,accepted));
    }
    HammerOfDawnRequest fire() {
        CHECK(edge(I::fire,true)==E::request_ready);auto r=take();
        CHECK(r.action==A::press_original_fire);CHECK(ack(r)==E::original_accepted);
        return r;
    }
    HammerOfDawnEffectReceipt effect(std::uint64_t cycle) {
        return {stamp(),certificate.instance_config,cycle,++original_event,++effect_serial,
            resources,true,true,false};
    }
};

void binding_and_unverified_rules() {
    auto c=config();HammerOfDawnInteraction empty;
    CHECK(!empty.bind(c,{0,23,777}));c.identity.weapon_generation=0;
    CHECK(!empty.bind(c,{1,23,777}));c=config();c.binding=0;
    CHECK(!empty.bind(c,{1,23,777}));c=config();c.maximum_pose_age_ns=0;
    CHECK(!empty.bind(c,{1,23,777}));c=config();c.original_class_verified=false;
    CHECK(!empty.bind(c,{1,23,777}));c=config();
    CHECK(!empty.bind(c,{1,24,777}));CHECK(empty.bind(c,{1,23,777}));
    CHECK(!empty.bind(c,{1,23,777}));
    for(unsigned n=0;n<7;++n){
        c=config();
        switch(n){
            case 0:c.original_r_semantics_verified=false;break;
            case 1:c.original_release_semantics_verified=false;break;
            case 2:c.non_firing_cancel_verified=false;break;
            case 3:c.original_conditions_and_target_check_verified=false;break;
            case 4:c.original_effects_and_recoil_preserved=false;break;
            case 5:c.resources=HammerOfDawnResourceRule::unverified;break;
            case 6:c.original_l_hold_semantics_verified=false;break;
            default:break;
        }
        Fixture f(c);
        CHECK(f.edge(n==6?I::aim:I::fire,true)==E::rejected);
        CHECK(!f.model.has_pending_request());CHECK(f.model.resources().units==777);
    }
    c=config();c.resources=HammerOfDawnResourceRule::no_consumable;
    HammerOfDawnInteraction no_ammo;CHECK(!no_ammo.bind(c,{1,23,1}));
    Fixture unlimited(c);auto request=unlimited.fire();
    auto effect=unlimited.effect(request.use_cycle);
    CHECK(unlimited.model.record_original_effect(effect)==E::original_effect_recorded);
    CHECK(unlimited.model.resources().units==0);
}
void cold_single_hand_and_invalid_target() {
    Fixture f;HammerOfDawnInteraction cold;
    CHECK(cold.bind(f.certificate,f.resources));
    auto s=f.sample();s.pose.left_tracked=false;
    CHECK(cold.observe(s)==E::hands_restored);
    f.fire_down=true;s=f.sample();s.pose.left_tracked=false;
    cold.observe(s);
    CHECK(cold.edge(f.stamp(),{1,I::fire,true,true})==E::request_ready);
    Fixture active;active.fire();s=active.sample();
    s.target.original_target_valid=false;s.target.occlusion_clear=false;
    CHECK(active.model.observe(s)==E::observed);
    CHECK(active.model.owns_fire()&&!active.model.has_pending_request());
    CHECK(active.model.update_designation(active.stamp())==E::request_ready);
    auto query=active.take();CHECK(query.action==A::update_original_designation);
    CHECK(active.ack(query)==E::original_accepted);
    s=active.sample();CHECK(active.model.observe(s)==E::observed);
    CHECK(active.model.owns_fire());
}
void complete_stop_readback() {
    for(unsigned n=0;n<12;++n){
        Fixture f;f.fire();CHECK(f.edge(I::fire,false)==E::request_ready);
        const auto end=f.take();auto r=f.receipt(end);
        switch(n){
        case 0:r.stop.weapon++;break;
        case 1:r.stop.weapon_generation++;break;
        case 2:r.stop.use_cycle++;break;
        case 3:r.stop.original_event_serial++;break;
        case 4:r.stop.verified=false;break;
        case 5:r.stop.target_withdrawn=false;break;
        case 6:r.stop.beam_inactive=false;break;
        case 7:r.stop.laser_inactive=false;break;
        case 8:r.stop.valid_target_emitter_inactive=false;break;
        case 9:r.stop.loop_sound_stopped=false;break;
        case 10:r.stop.pending_fire_cleared=false;break;
        case 11:r.stop={};break;
        }
        CHECK(f.model.acknowledge(r)==E::cancel_required);
        CHECK(f.model.owns_fire()&&f.model.interrupted());
        auto cancel=f.take();CHECK(cancel.action==A::cancel_original_inputs);
        CHECK(f.ack(cancel)==E::cancelled);CHECK(!f.model.owns_fire());
    }
}
void original_gates_and_pose() {
    for(unsigned n=0;n<14;++n){
        Fixture f;f.fire_down=true;auto s=f.sample();
        switch(n){
            case 0:s.original.equipped=false;break;
            case 1:s.original.ordinary_player=false;break;
            case 2:s.original.input_allowed=false;break;
            case 3:s.original.manager_enabled=false;break;
            case 4:s.original.conditions_allow_use=false;break;
            case 5:s.original.resources_allow_use=false;break;
            case 6:s.original.phase=P::cooldown;break;
            case 7:s.target.original_conditions_passed=false;break;
            case 8:s.target.original_target_valid=false;break;
            case 9:s.target.occlusion_clear=false;break;
            case 10:s.target.checked_from_same_weapon_ray=false;break;
            case 11:s.target.frame_ticket=0;break;
            case 12:s.original.fire_input_down=true;break;
            case 13:s.original.aim_input_down=true;break;
            default:break;
        }
        f.model.observe(s);
        const bool original_negative=n==7||n==8||n==9;
        CHECK(f.model.edge(f.stamp(),{1,I::fire,true,true})==
            (original_negative?E::request_ready:E::rejected));
        CHECK(f.model.has_pending_request()==original_negative);CHECK(f.model.resources().units==777);
    }
    for(unsigned n=0;n<10;++n){
        Fixture f;f.fire_down=true;auto s=f.sample();
        switch(n){
            case 0:s.pose.right_tracked=false;break;
            case 1:s.pose.right_weapon_held=false;break;
            case 2:s.pose.origin_and_axis_from_weapon=false;break;
            case 3:s.pose.grip_to_indicator_clear=false;break;
            case 4:s.pose.mapping=0;break;
            case 5:s.pose.frame_ticket=0;break;
            case 6:s.pose.measured_ns=s.stamp.monotonic_ns+1;break;
            case 7:s.pose.ray.origin.x=std::numeric_limits<double>::quiet_NaN();break;
            case 8:s.pose.ray.direction.z=-2;break;
            case 9:s.pose.ray.direction.x=std::numeric_limits<double>::infinity();break;
            default:break;
        }
        f.model.observe(s);f.model.edge(f.stamp(),{1,I::fire,true,true});
        CHECK(!f.model.has_pending_request());CHECK(!f.model.owns_fire());
    }
}
void aim_use_and_original_effects() {
    Fixture f;CHECK(f.edge(I::aim,true)==E::request_ready);
    auto a=f.take();CHECK(a.action==A::press_original_aim);
    CHECK(f.ack(a)==E::original_accepted);CHECK(f.model.owns_aim());
    auto r=f.fire();CHECK(r.ray.origin.x==1&&r.ray.direction.z==-1);
    CHECK(r.requires_pose&&r.target_certificate&&r.instance_config==21);
    CHECK(f.model.original_phase()==P::warming);CHECK(f.model.resources().units==777);
    HammerOfDawnRequest duplicate{};CHECK(!f.model.take_request(duplicate,f.ns));
    // Passing time cannot synthesize an illumination, resource debit or attack.
    f.ns+=100;CHECK(f.model.observe(f.sample())==E::observed);
    CHECK(f.model.original_phase()==P::warming);CHECK(!f.model.has_pending_request());
    f.original.phase=P::illuminating;CHECK(f.model.observe(f.sample())==E::observed);
    CHECK(f.model.original_phase()==P::illuminating);
    auto effect=f.effect(r.use_cycle);effect.resources={2,23,754};
    CHECK(f.model.record_original_effect(effect)==E::cancel_required); // No resource proof.
    CHECK(f.model.interrupted());
    CHECK(f.model.resources().units==777);

    Fixture g;auto use=g.fire();auto real=g.effect(use.use_cycle);
    real.resources={2,23,754};real.original_resource_change_verified=true;
    CHECK(g.model.record_original_effect(real)==E::original_effect_recorded);
    g.resources=real.resources;
    CHECK(g.model.resources().units==754);
    real.stamp=g.stamp();CHECK(g.model.record_original_effect(real)==E::rejected);
    real=g.effect(use.use_cycle+1);CHECK(g.model.record_original_effect(real)==E::rejected);
    real=g.effect(use.use_cycle);real.original_effects_and_recoil_preserved=false;
    CHECK(g.model.record_original_effect(real)==E::rejected);
    CHECK(g.edge(I::fire,false)==E::request_ready);auto end=g.take();
    CHECK(end.action==A::release_original_fire&&end.requires_pose);
    CHECK(g.ack(end)==E::original_accepted);CHECK(!g.model.owns_fire());
    // A late certified original effect still belongs to the dispatched cycle.
    real=g.effect(use.use_cycle);CHECK(g.model.record_original_effect(real)==E::original_effect_recorded);
    CHECK(g.model.observe(g.sample())==E::observed);
    CHECK(g.edge(I::fire,true)==E::request_ready);
}
void designation_and_ticket_leases() {
    Fixture f;f.fire();auto s=f.sample();s.pose.ray={{10,20,30},{1,0,0}};
    CHECK(f.model.observe(s)==E::observed);
    CHECK(f.model.update_designation(f.stamp())==E::request_ready);auto update=f.take();
    CHECK(update.action==A::update_original_designation);
    CHECK(update.ray.origin.x==10&&update.ray.origin.y==20&&update.ray.direction.x==1);
    CHECK(f.ack(update)==E::original_accepted);
    CHECK(f.model.update_designation(f.stamp())==E::rejected); // One request per pose.
    s=f.sample();s.target.original_check_serial=1;f.model.observe(s);
    CHECK(f.take().action==A::cancel_original_inputs); // Stale target proof cannot track.

    Fixture stale;CHECK(stale.edge(I::fire,true)==E::request_ready);
    HammerOfDawnRequest request{};
    CHECK(!stale.model.take_request(request,stale.ns+1001));
    CHECK(!stale.model.has_pending_request()&&!stale.model.owns_fire());
    CHECK(stale.model.resources().units==777);
    stale.model.observe(stale.sample());
    CHECK(stale.model.edge(stale.stamp(),{2,I::fire,true,true})==E::rejected);
    CHECK(stale.edge(I::fire,false)==E::observed);
    CHECK(stale.edge(I::fire,true)==E::request_ready);

    Fixture replaced;CHECK(replaced.edge(I::fire,true)==E::request_ready);
    replaced.model.observe(replaced.sample());
    CHECK(!replaced.model.take_request(request,replaced.ns)); // Old ray ticket cannot dispatch.
}
void tracking_cancellation_and_late_receipts() {
    Fixture f;auto use=f.fire();f.ns+=1001;
    CHECK(f.model.expire_pose(f.ns)==E::cancel_required);
    auto cancel=f.take();CHECK(cancel.action==A::cancel_original_inputs&&!cancel.requires_pose);
    CHECK(cancel.owned_fire&&!cancel.owned_aim);CHECK(f.ack(cancel)==E::cancelled);
    CHECK(!f.model.owns_fire());CHECK(f.model.resources().units==777);
    auto late=f.effect(use.use_cycle);
    CHECK(f.model.record_original_effect(late)==E::safety_violation);
    CHECK(f.model.interrupted());

    Fixture pending;CHECK(pending.edge(I::fire,true)==E::request_ready);auto press=pending.take();
    pending.ns+=1001;CHECK(pending.model.expire_pose(pending.ns)==E::cancel_required);
    CHECK(!pending.model.take_request(cancel,pending.ns)); // Wait for dispatched call receipt.
    CHECK(pending.ack(press)==E::cancel_required);
    cancel=pending.take();CHECK(cancel.action==A::cancel_original_inputs);
    CHECK(pending.ack(cancel)==E::cancelled);
    pending.model.observe(pending.sample());
    CHECK(pending.model.edge(pending.stamp(),{2,I::fire,true,true})==E::rejected);
    pending.edge(I::fire,false);CHECK(pending.edge(I::fire,true)==E::request_ready);

    Fixture release;CHECK(release.edge(I::fire,true)==E::request_ready);press=release.take();
    CHECK(release.edge(I::fire,false)==E::release_deferred);
    CHECK(release.ack(press)==E::request_ready);auto end=release.take();
    CHECK(end.action==A::release_original_fire);CHECK(release.ack(end)==E::original_accepted);
    CHECK(!release.model.owns_fire());

    Fixture no_dispatch;CHECK(no_dispatch.edge(I::fire,true)==E::request_ready);
    CHECK(no_dispatch.edge(I::fire,false)==E::input_released);
    CHECK(!no_dispatch.model.has_pending_request());
    CHECK(no_dispatch.edge(I::fire,true)==E::request_ready);auto next=no_dispatch.take();
    CHECK(no_dispatch.ack(next)==E::original_accepted);
    CHECK(!no_dispatch.model.has_pending_request());
}
void animation_and_identity() {
    Fixture f;f.fire();f.original.animation_owns_hands=true;
    CHECK(f.model.observe(f.sample())==E::cancel_required);
    CHECK(f.model.hands()==H::original_animation);auto cancel=f.take();
    CHECK(f.ack(cancel)==E::cancelled);CHECK(f.model.hands()==H::original_animation);
    f.model.expire_pose(f.ns+2000);CHECK(f.model.hands()==H::original_animation);
    f.original.animation_owns_hands=false;
    auto s=f.sample();s.pose.left_tracked=false;
    CHECK(f.model.observe(s)==E::hands_restored);
    CHECK(f.model.observe(f.sample())==E::observed);
    CHECK(f.model.hands()==H::vr);
    CHECK(f.model.edge(f.stamp(),{2,I::fire,true,true})==E::rejected);
    f.edge(I::fire,false);CHECK(f.edge(I::fire,true)==E::request_ready);

    for(unsigned n=0;n<10;++n){
        Fixture g;g.fire();s=g.sample();
        switch(n){
            case 0:++s.stamp.identity.world;break;
            case 1:++s.stamp.identity.world_generation;break;
            case 2:++s.stamp.identity.pawn;break;
            case 3:++s.stamp.identity.controller;break;
            case 4:++s.stamp.identity.weapon;break;
            case 5:++s.stamp.identity.weapon_generation;break;
            case 6:++s.stamp.identity.xr_session;break;
            case 7:++s.stamp.identity.space_epoch;break;
            case 8:++s.stamp.identity.input_epoch;break;
            case 9:++s.stamp.binding;break;
            default:break;
        }
        g.model.observe(s);CHECK(g.model.interrupted());cancel=g.take();
        CHECK(cancel.action==A::cancel_original_inputs&&cancel.stamp.identity==id);
        CHECK(g.ack(cancel)==E::cancelled);
    }
}
void receipts_reload_and_conflicts() {
    Fixture f;CHECK(f.model.begin_r3(f.stamp(),1)==E::unsupported_reload);
    CHECK(f.model.begin_r3(f.stamp(),1)==E::rejected);
    CHECK(!f.model.has_pending_request()&&f.model.resources().units==777);
    auto c=config();c.no_regular_reload_verified=false;Fixture unknown(c);
    CHECK(unknown.model.begin_r3(unknown.stamp(),1)==E::unverified_reload);
    CHECK(!unknown.model.has_pending_request());

    Fixture r;CHECK(r.edge(I::fire,true)==E::request_ready);auto press=r.take();auto receipt=r.receipt(press);
    auto bad=receipt;bad.request_token++;
    CHECK(r.model.acknowledge(bad)==E::rejected);
    bad=receipt;bad.instance_config++;CHECK(r.model.acknowledge(bad)==E::rejected);
    bad=receipt;bad.stamp.identity.weapon_generation++;
    CHECK(r.model.acknowledge(bad)==E::rejected);
    CHECK(r.model.acknowledge(receipt)==E::original_accepted);
    receipt.stamp=r.stamp();CHECK(r.model.acknowledge(receipt)==E::rejected);
    auto sample=r.sample();sample.resources.units--;
    r.model.observe(sample);CHECK(r.model.interrupted());CHECK(r.model.resources().units==777);

    Fixture rejected;CHECK(rejected.edge(I::fire,true)==E::request_ready);press=rejected.take();
    CHECK(rejected.ack(press,false)==E::original_rejected);CHECK(!rejected.model.owns_fire());
    CHECK(rejected.model.edge(rejected.stamp(),{2,I::fire,true,true})==E::rejected);

    Fixture gap;gap.fire();sample=gap.sample();sample.stamp.monotonic_ns+=1001;
    gap.ns=sample.stamp.monotonic_ns;sample.pose.measured_ns=gap.ns;
    CHECK(gap.model.observe(sample)==E::cancel_required);CHECK(gap.take().action==A::cancel_original_inputs);

    Fixture external;external.fire();external.original.aim_input_down=true;
    external.ns+=1001;external.model.expire_pose(external.ns);auto cancel=external.take();
    CHECK(!cancel.owned_aim&&cancel.owned_fire);
    CHECK(external.ack(cancel)==E::cancelled); // Never clear another source's L input.
    CHECK(external.original.aim_input_down);
}
void original_resource_and_abort_boundaries() {
    // Resource units have no locally assumed cost or ammunition interpretation.
    Fixture zero;zero.resources.units=0; // A snapshot cannot make this change.
    HammerOfDawnInteraction native;
    CHECK(native.bind(zero.certificate,zero.resources));
    auto s=zero.sample();CHECK(native.observe(s)==E::hands_restored);
    zero.fire_down=true;s=zero.sample();native.observe(s);
    CHECK(native.edge(zero.stamp(),{1,I::fire,true,true})==E::request_ready);

    Fixture paid;CHECK(paid.edge(I::fire,true)==E::request_ready);auto press=paid.take();
    auto receipt=paid.receipt(press);receipt.resources={2,23,700};
    receipt.original_resource_change_verified=true;
    CHECK(paid.model.acknowledge(receipt)==E::original_accepted);
    CHECK(paid.model.resources().units==700);paid.resources=receipt.resources;
    // A real effect inside the original press call can arrive before its ack.
    Fixture early;CHECK(early.edge(I::fire,true)==E::request_ready);press=early.take();
    auto effect=early.effect(press.use_cycle);
    CHECK(early.model.record_original_effect(effect)==E::original_effect_recorded);
    CHECK(early.ack(press)==E::original_accepted);
    early.ns+=1001;effect=early.effect(press.use_cycle);
    CHECK(early.model.record_original_effect(effect)==E::safety_violation);
    CHECK(early.model.interrupted());CHECK(early.take().action==A::cancel_original_inputs);

    Fixture refused;refused.fire();refused.ns+=1001;refused.model.expire_pose(refused.ns);
    auto cancel=refused.take();
    CHECK(refused.ack(cancel,false)==E::cancel_required);
    CHECK(refused.model.owns_fire()&&refused.model.interrupted());
    // A claimed cancellation with a live original beam cannot erase ownership.
    Fixture live;live.fire();live.ns+=1001;live.model.expire_pose(live.ns);cancel=live.take();
    receipt=live.receipt(cancel);receipt.original.phase=P::illuminating;
    CHECK(live.model.acknowledge(receipt)==E::cancel_required);
    CHECK(live.model.owns_fire()&&live.model.interrupted());

    Fixture one_hand;s=one_hand.sample();s.pose.left_tracked=false;
    one_hand.model.observe(s);one_hand.fire_down=true;s=one_hand.sample();s.pose.left_tracked=false;
    one_hand.model.observe(s);
    CHECK(one_hand.model.edge(one_hand.stamp(),{1,I::fire,true,true})==E::request_ready);
    Fixture no_left;no_left.aim_down=true;s=no_left.sample();s.pose.left_tracked=false;
    no_left.model.observe(s);no_left.model.edge(no_left.stamp(),{1,I::aim,true,true});
    CHECK(!no_left.model.has_pending_request());

    Fixture held;HammerOfDawnInteraction initial;
    CHECK(initial.bind(held.certificate,held.resources));held.fire_down=true;
    s=held.sample();initial.observe(s);
    CHECK(initial.edge(held.stamp(),{1,I::fire,true,true})==E::rejected);
    Fixture discontinuity;discontinuity.fire();discontinuity.fire_down=false;
    discontinuity.model.observe(discontinuity.sample());
    CHECK(discontinuity.model.edge(discontinuity.stamp(),{2,I::fire,false,false})==E::cancel_required);
    CHECK(discontinuity.take().action==A::cancel_original_inputs);
}
}
int main() {
    binding_and_unverified_rules();cold_single_hand_and_invalid_target();complete_stop_readback();original_gates_and_pose();aim_use_and_original_effects();
    designation_and_ticket_leases();tracking_cancellation_and_late_receipts();
    animation_and_identity();receipts_reload_and_conflicts();original_resource_and_abort_boundaries();
    std::printf("Hammer of Dawn interaction: %u checks passed (synthetic only)\n",checks);
}
