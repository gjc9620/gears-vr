#include "weapon_troika_interaction.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <type_traits>

using namespace gears_weapon;
using E=TroikaEvent;using A=TroikaAction;using P=TroikaPhase;using H=TroikaHands;using I=TroikaInput;
static_assert(!std::is_copy_constructible<TroikaInteraction>::value&&
    !std::is_move_constructible<TroikaInteraction>::value,"One native input owner per binding");
namespace {
unsigned checks{};
void check(bool good,char const* expression,int line) {
    ++checks;if(!good){std::fprintf(stderr,"line %d: %s\n",line,expression);std::exit(1);}
}
#define CHECK(expression) check(bool(expression),#expression,__LINE__)
constexpr TroikaIdentity identity{1,2,3,4,5,6,7,8,9,10,11,12};
// Synthetic certificates, transforms, limits, resources and times only. These
// tests establish NO retail Troika animation, ammunition, heat or reload rule.
TroikaConfig config(bool mechanical=false) {
    TroikaConfig c{};c.identity=identity;c.binding=20;c.instance_config=21;c.pose_mapping=22;
    c.resource_rule=23;c.left_handle=31;c.right_handle=32;c.left_muzzle=41;c.right_muzzle=42;
    c.fixed_pivot_world_m={3,4,5};c.left_handle_local_m.position={-0.2,0,0};
    c.right_handle_local_m.position={0.2,0,0};c.minimum_yaw_rad=c.minimum_pitch_rad=-1;
    c.maximum_yaw_rad=c.maximum_pitch_rad=1;c.grip_distance_m=0.02;c.grip_angle_rad=0.1;
    c.anchor_tolerance_m=0.001;c.maximum_pose_age_ns=1000;
    c.current_instance_verified=c.fixed_turret_verified=c.geometry_verified=true;
    c.original_mount_verified=c.original_unmount_verified=c.original_aim_verified=true;
    c.original_fire_verified=c.non_firing_stop_verified=c.original_effects_preserved=true;
    c.resource_rules_verified=c.heat_rules_verified=c.reload_rules_verified=true;
    if(mechanical){c.manual_mechanical_plan_verified=true;c.mechanical_plan=70;c.step_count=2;
        c.steps[0]={71,72,73};c.steps[1]={81,82,83};}
    return c;
}
struct Fixture {
    TroikaInteraction model{};TroikaConfig certificate{};TroikaOriginalState original{};
    TroikaResources resources{1,23,500,10};
    std::uint64_t sequence{},ticket{},event{},edge_serial{},shot_serial{},motion_serial{};
    std::int64_t ns{100};bool trigger{},interact{};
    TroikaPose pose{};
    explicit Fixture(TroikaConfig c=config(),P phase=P::mounted):certificate(c) {
        original.rule=c.resource_rule;original.phase=phase;original.driver_is_this_pawn=phase!=P::detached;
        original.current_instance_verified=original.input_allowed=original.can_mount=original.can_unmount=true;
        original.can_fire=original.resources_allow_fire=original.heat_allows_fire=true;
        pose.mapping=c.pose_mapping;pose.turret_world_m.position=c.fixed_pivot_world_m;
        pose.left_hand_world_m=troika_compose(pose.turret_world_m,c.left_handle_local_m);
        pose.right_hand_world_m=troika_compose(pose.turret_world_m,c.right_handle_local_m);
        pose.muzzles={TroikaMuzzle{c.left_muzzle,{{2,4,6},{0,0,1}}},
            TroikaMuzzle{c.right_muzzle,{{4,4,6},{0,0,1}}}};
        pose.left_tracked=pose.right_tracked=pose.left_grip_down=pose.right_grip_down=true;
        pose.original_pivot_and_constraints_verified=pose.muzzle_axis_relation_verified=pose.mount_contact_verified=true;
        CHECK(model.bind(c,resources));CHECK(model.observe(sample())==E::hands_restored);
    }
    TroikaStamp stamp(){return {identity,certificate.binding,++sequence,++ns};}
    TroikaSample sample() {
        ++original.revision;++ticket;auto s=stamp();pose.frame_ticket=ticket;pose.measured_ns=ns;
        return {s,original,resources,pose,trigger,interact,false,0};
    }
    E observe(){return model.observe(sample());}
    E edge(I input,bool down) {
        if(input==I::fire)trigger=down;else interact=down;
        observe();return model.edge(stamp(),{++edge_serial,input,down,true});
    }
    TroikaRequest take(){TroikaRequest r{};CHECK(model.take_request(r,ns));return r;}
    TroikaReceipt receipt(TroikaRequest const& r,bool accepted=true) {
        if(accepted){
            switch(r.kind){
                case A::press_original_fire:original.fire_input_down=true;break;
                case A::stop_original_fire:original.fire_input_down=false;break;
                case A::mount_original:original.phase=P::entering;original.driver_is_this_pawn=true;
                    original.animation_owns_hands=true;original.animation_serial=1000+r.token;break;
                case A::unmount_original:original.phase=P::leaving;original.animation_owns_hands=true;
                    original.animation_serial=1000+r.token;break;
                default:break;
            }
        }
        ++original.revision;return {stamp(),r.token,certificate.instance_config,++event,original,resources,accepted,false,false};
    }
    E ack(TroikaRequest const& r,bool accepted=true){return model.acknowledge(receipt(r,accepted));}
    TroikaRequest fire(){CHECK(edge(I::fire,true)==E::request_ready);auto r=take();
        CHECK(r.kind==A::press_original_fire);CHECK(ack(r)==E::original_accepted);CHECK(model.owns_fire());return r;}
    TroikaShotReceipt shot(TroikaRequest const& r,std::uint64_t muzzle) {
        --resources.ammunition;++resources.revision;resources.heat+=2;
        return {stamp(),certificate.instance_config,r.fire_cycle,++shot_serial,muzzle,++event,
            {{9,8,7},{0,1,0}},resources,true,true,true,true,true};
    }
    TroikaAnimationReceipt animation(TroikaRequest const& r,P target) {
        original.phase=target;original.driver_is_this_pawn=target!=P::detached;
        original.animation_owns_hands=false;++original.revision;
        return {stamp(),certificate.instance_config,r.token,original.animation_serial,++event,original,true,true};
    }
    TroikaMechanicalStart service() {
        original.servicing=true;++original.service_serial;observe();
        TroikaMechanicalStart r{certificate.instance_config,certificate.mechanical_plan,
            original.service_serial,++event,true,true};
        CHECK(model.begin_mechanical(stamp(),r)==E::mechanical_started);return r;
    }
    TroikaMechanicalMotion motion(TroikaMechanicalStart const& start) {
        auto g=model.guide(ns);return {certificate.instance_config,start.plan,start.service_serial,
            g.component,g.action,g.motion_spec,++motion_serial,sequence,ticket,true,true,true,true};
    }
};

void geometry_and_certificates() {
    auto c=config();CHECK(c.valid());c.right_muzzle=c.left_muzzle;CHECK(!c.valid());
    c=config();c.right_handle_local_m=c.left_handle_local_m;CHECK(!c.valid());
    c=config();c.grip_distance_m=std::numeric_limits<double>::quiet_NaN();CHECK(!c.valid());
    c=config();c.identity.turret_generation=0;CHECK(!c.valid());
    c=config();c.identity.profile_generation=0;CHECK(!c.valid());
    c=config(true);c.steps[1].motion_spec=0;CHECK(!c.valid());
    c=config();c.step_count=1;CHECK(!c.valid());
    const double q=std::sqrt(0.5);
    TroikaTransform t{{2,3,4},{0,0,q,q}},local{{1,0,0},{0,0,0,1}};
    auto world=troika_compose(t,local);CHECK(std::abs(world.position.x-2)<1e-9);
    CHECK(std::abs(world.position.y-4)<1e-9);CHECK(std::abs(world.position.z-4)<1e-9);
    CHECK(troika_grip_matches(t,local,world,0.001,0.01));
    world.rotation={0,0,-q,-q};CHECK(troika_grip_matches(t,local,world,0.001,0.01));
    world.rotation={0,0,0,1};CHECK(!troika_grip_matches(t,local,world,0.001,0.01));
    for(unsigned gate=0;gate<7;++gate){
        c=config();switch(gate){case 0:c.original_fire_verified=false;break;
            case 1:c.non_firing_stop_verified=false;break;case 2:c.original_effects_preserved=false;break;
            case 3:c.resource_rules_verified=false;break;case 4:c.heat_rules_verified=false;break;
            case 5:c.reload_rules_verified=false;break;default:c.original_aim_verified=false;break;}
        Fixture f(c);CHECK(f.edge(I::fire,true)==E::rejected);CHECK(!f.model.has_pending_request());
    }
    Fixture passive;CHECK(passive.model.begin_mechanical(passive.stamp(),{})==E::unsupported_mechanism);
    CHECK(!passive.model.guide(passive.ns).component);
    CHECK(!passive.model.bind(config(),passive.resources));
}
void input_and_identity() {
    Fixture f;auto s=f.stamp();auto bad=s;bad.identity.turret_generation++;
    CHECK(f.model.edge(bad,{1,I::fire,true,true})==E::rejected);
    auto sample=f.sample();sample.stamp.identity.weapon_generation++;
    CHECK(f.model.observe(sample)==E::rejected);
    sample=f.sample();sample.stamp.binding++;CHECK(f.model.observe(sample)==E::rejected);
    sample=f.sample();sample.stamp.identity.space_epoch++;CHECK(f.model.observe(sample)==E::rejected);
    sample=f.sample();sample.stamp.identity.profile_generation++;CHECK(f.model.observe(sample)==E::rejected);
    sample=f.sample();sample.stamp.monotonic_ns=1;CHECK(f.model.observe(sample)==E::rejected);
    CHECK(f.model.edge(f.stamp(),{1,I::fire,true,true})==E::rejected); // level is still released
    f.trigger=true;f.observe();CHECK(f.model.edge(f.stamp(),{1,I::fire,true,false})==E::rejected);
    CHECK(f.model.edge(f.stamp(),{1,I::fire,true,true})==E::request_ready);
    CHECK(f.model.edge(f.stamp(),{2,I::fire,true,true})==E::rejected);
    auto r=f.take();TroikaRequest other{};CHECK(!f.model.take_request(other,f.ns));
    auto ack=f.receipt(r);auto wrong=ack;wrong.token++;CHECK(f.model.acknowledge(wrong)==E::rejected);
    wrong=ack;wrong.instance_config++;CHECK(f.model.acknowledge(wrong)==E::rejected);
    CHECK(f.model.acknowledge(ack)==E::original_accepted);
    CHECK(f.model.acknowledge(ack)==E::rejected);
    f.trigger=false;CHECK(f.observe()==E::request_ready);auto stop=f.take();
    CHECK(stop.kind==A::stop_original_fire);CHECK(f.ack(stop)==E::original_accepted);CHECK(!f.model.owns_fire());
}
void fixed_mount_and_original_animation() {
    Fixture f(config(),P::detached);CHECK(f.edge(I::fire,true)==E::rejected);f.edge(I::fire,false);
    CHECK(f.edge(I::interact,true)==E::request_ready);auto mount=f.take();CHECK(mount.kind==A::mount_original);
    CHECK(f.ack(mount)==E::original_accepted);CHECK(f.model.hands()==H::original_animation);
    CHECK(f.model.update_aim(f.stamp())==E::rejected);CHECK(!f.model.settled());
    // Merely observing a final phase cannot take the hands from original animation.
    f.original.phase=P::mounted;f.original.animation_owns_hands=false;f.observe();
    CHECK(f.model.hands()==H::original_animation);
    auto done=f.animation(mount,P::mounted);auto invalid=done;invalid.animation_serial++;
    CHECK(f.model.animation_finished(invalid)==E::rejected);invalid=done;invalid.original_animation_completed=false;
    CHECK(f.model.animation_finished(invalid)==E::rejected);
    CHECK(f.model.animation_finished(done)==E::observed);CHECK(f.model.hands()==H::await_tracking);
    auto replay=f.sample();replay.original.animation_owns_hands=true;
    CHECK(f.model.observe(replay)==E::rejected); // completed animation lease cannot be reacquired by a stale snapshot
    f.trigger=true;CHECK(f.observe()==E::hands_restored);CHECK(f.edge(I::fire,true)==E::rejected);
    f.edge(I::fire,false);auto fire=f.fire();CHECK(fire.fire_cycle);
    f.interact=false;f.observe();f.edge(I::interact,false);
    CHECK(f.edge(I::interact,true)==E::request_ready);auto stop=f.take();CHECK(stop.kind==A::stop_original_fire);
    CHECK(f.ack(stop)==E::request_ready);auto exit=f.take();CHECK(exit.kind==A::unmount_original);
    CHECK(f.ack(exit)==E::original_accepted);CHECK(f.model.hands()==H::original_animation);
    done=f.animation(exit,P::mounted);CHECK(f.model.animation_finished(done)==E::rejected);
    done=f.animation(exit,P::detached);CHECK(f.model.animation_finished(done)==E::observed);
    f.observe();CHECK(f.model.settled());CHECK(f.model.update_aim(f.stamp())==E::rejected);
    Fixture rejected(config(),P::detached);CHECK(rejected.edge(I::interact,true)==E::request_ready);
    mount=rejected.take();CHECK(rejected.ack(mount,false)==E::original_rejected);CHECK(rejected.model.settled());
    Fixture malformed(config(),P::detached);malformed.edge(I::interact,true);mount=malformed.take();
    auto receipt=malformed.receipt(mount);receipt.original.animation_owns_hands=false;
    CHECK(malformed.model.acknowledge(receipt)==E::safety_violation);CHECK(!malformed.model.settled());
}
void pose_and_original_gates() {
    for(unsigned gate=0;gate<13;++gate){
        Fixture f;
        switch(gate){
            case 0:f.pose.left_hand_world_m.position.x+=0.1;break;
            case 1:f.pose.right_hand_world_m.rotation={0,0,1,0};break;
            case 2:f.pose.left_grip_down=false;break;case 3:f.pose.right_grip_down=false;break;
            case 4:f.pose.turret_world_m.position.x+=0.1;break;
            case 5:f.pose.yaw_rad=1.01;break;case 6:f.pose.pitch_rad=-1.01;break;
            case 7:f.pose.left_tracked=false;break;case 8:f.pose.muzzles[0].identity=f.certificate.right_muzzle;break;
            case 9:f.pose.muzzles[1].ray.direction={0,0,0};break;
            case 10:f.original.heat_allows_fire=false;break;case 11:f.original.resources_allow_fire=false;break;
            default:f.original.can_fire=false;break;
        }
        CHECK(f.edge(I::fire,true)==E::rejected);CHECK(!f.model.has_pending_request());
    }
    Fixture f;CHECK(f.model.update_aim(f.stamp())==E::request_ready);auto aim=f.take();
    CHECK(aim.kind==A::aim_original&&aim.muzzles[0].identity!=aim.muzzles[1].identity);
    CHECK(f.ack(aim)==E::original_accepted);
    // HMD is absent from this API; only the certified turret/hand pose enters aim.
    f.pose.yaw_rad=0.5;f.observe();CHECK(f.model.update_aim(f.stamp())==E::request_ready);
    aim=f.take();CHECK(aim.yaw_rad==0.5);CHECK(f.ack(aim)==E::original_accepted);
    f.original.can_fire=false;f.observe();CHECK(f.model.update_aim(f.stamp())==E::request_ready);
    aim=f.take();CHECK(f.ack(aim)==E::original_accepted); // aiming during a native fire gate is allowed
    Fixture stale;CHECK(stale.edge(I::fire,true)==E::request_ready);TroikaRequest out{};
    CHECK(!stale.model.take_request(out,stale.ns+1001));CHECK(!stale.model.has_pending_request());
    Fixture revised;CHECK(revised.edge(I::fire,true)==E::request_ready);revised.observe();
    CHECK(!revised.model.take_request(out,revised.ns));
    Fixture dispatch;dispatch.edge(I::fire,true);CHECK(dispatch.model.take_request(out,dispatch.ns+10));
    auto ack=dispatch.receipt(out);CHECK(dispatch.model.acknowledge(ack)==E::rejected); // before actual dispatch time
    dispatch.ns+=10;ack=dispatch.receipt(out);CHECK(dispatch.model.acknowledge(ack)==E::original_accepted);
}
void interruption_and_native_fire_lease() {
    Fixture queued;queued.edge(I::fire,true);CHECK(queued.model.interrupt()==E::interrupted);
    CHECK(!queued.model.has_pending_request());CHECK(queued.model.settled());
    Fixture flight;flight.edge(I::fire,true);auto press=flight.take();
    CHECK(flight.model.interrupt()==E::stop_required);CHECK(!flight.model.settled());
    CHECK(flight.ack(press)==E::request_ready);CHECK(flight.model.owns_fire());auto stop=flight.take();
    CHECK(stop.kind==A::stop_original_fire);CHECK(flight.ack(stop,false)==E::request_ready);
    CHECK(flight.model.owns_fire());stop=flight.take();CHECK(flight.ack(stop)==E::interrupted);
    CHECK(flight.model.settled());CHECK(flight.model.interrupted());
    Fixture lost;lost.fire();lost.pose.left_tracked=false;CHECK(lost.observe()==E::request_ready);
    stop=lost.take();CHECK(stop.kind==A::stop_original_fire);CHECK(lost.ack(stop)==E::original_accepted);
    lost.pose.left_tracked=true;CHECK(lost.observe()==E::hands_restored);
    CHECK(lost.edge(I::fire,true)==E::rejected);lost.edge(I::fire,false);lost.fire();
    Fixture expired;expired.fire();expired.ns+=1001;CHECK(expired.model.poll(expired.stamp())==E::request_ready);
    stop=expired.take();CHECK(stop.kind==A::stop_original_fire);CHECK(expired.ack(stop)==E::original_accepted);
    Fixture malformed;malformed.edge(I::fire,true);press=malformed.take();auto ack=malformed.receipt(press);
    ack.resources.ammunition--; // uncertified resource delta after an accepted native press
    CHECK(malformed.model.acknowledge(ack)==E::safety_violation);CHECK(malformed.model.owns_fire());
    stop=malformed.take();CHECK(stop.kind==A::stop_original_fire);CHECK(malformed.ack(stop)==E::interrupted);
    Fixture anim;anim.fire();anim.original.animation_owns_hands=true;anim.original.animation_serial=888;
    CHECK(anim.observe()==E::request_ready);CHECK(anim.model.hands()==H::original_animation);
    stop=anim.take();CHECK(anim.ack(stop)==E::original_accepted);CHECK(anim.model.hands()==H::original_animation);
    // Recovery or a repress before the in-flight native press receipt does not
    // undo a loss/release cancellation. The late acquired lease must be stopped.
    Fixture recovered;recovered.edge(I::fire,true);press=recovered.take();
    recovered.pose.left_tracked=false;CHECK(recovered.observe()==E::stop_required);
    recovered.pose.left_tracked=true;CHECK(recovered.observe()==E::stop_required);
    CHECK(recovered.ack(press)==E::request_ready);stop=recovered.take();
    CHECK(stop.kind==A::stop_original_fire);CHECK(recovered.ack(stop)==E::original_accepted);
    Fixture repress;repress.edge(I::fire,true);press=repress.take();
    repress.trigger=false;CHECK(repress.observe()==E::stop_required);repress.trigger=true;repress.observe();
    CHECK(repress.ack(press)==E::request_ready);stop=repress.take();CHECK(stop.kind==A::stop_original_fire);
    CHECK(repress.ack(stop)==E::original_accepted);
    Fixture driver;driver.fire();driver.original.driver_is_this_pawn=false;
    CHECK(driver.observe()==E::request_ready);stop=driver.take();CHECK(stop.kind==A::stop_original_fire);
    CHECK(driver.ack(stop)==E::original_accepted);CHECK(driver.model.update_aim(driver.stamp())==E::rejected);
}
void shot_identity_and_resource_receipts() {
    Fixture f;auto fire=f.fire();auto shot=f.shot(fire,f.certificate.right_muzzle);
    auto bad=shot;bad.muzzle=999;CHECK(f.model.record_original_shot(bad)==E::rejected);
    bad=shot;bad.original_ray_verified=false;CHECK(f.model.record_original_shot(bad)==E::rejected);
    bad=shot;bad.original_damage_spread_recoil_effects_preserved=false;
    CHECK(f.model.record_original_shot(bad)==E::rejected);
    bad=shot;bad.original_resource_change_verified=false;CHECK(f.model.record_original_shot(bad)==E::rejected);
    bad=shot;bad.fire_cycle++;CHECK(f.model.record_original_shot(bad)==E::rejected);
    CHECK(f.model.record_original_shot(shot)==E::shot_recorded);
    CHECK(f.model.resources().ammunition==499&&f.model.resources().heat==12);
    CHECK(f.model.last_shot().muzzle==f.certificate.right_muzzle);
    CHECK(f.model.last_shot().authoritative_firing_ray.direction.y==1); // original spread/ray, not animation bone axis
    CHECK(f.model.record_original_shot(shot)==E::rejected);
    // Original may choose the SAME muzzle twice. No assumed L/R alternating rule.
    shot=f.shot(fire,f.certificate.right_muzzle);CHECK(f.model.record_original_shot(shot)==E::shot_recorded);
    shot=f.shot(fire,f.certificate.left_muzzle);CHECK(f.model.record_original_shot(shot)==E::shot_recorded);
    f.trigger=false;f.observe();auto stop=f.take();CHECK(f.ack(stop)==E::original_accepted);
    shot=f.shot(fire,f.certificate.left_muzzle);CHECK(f.model.record_original_shot(shot)==E::shot_recorded); // native late effect
    auto sample=f.sample();sample.resources.heat=0;++sample.resources.revision;
    CHECK(f.model.observe(sample)==E::rejected);sample.original_resource_change_verified=true;sample.resource_event_serial=50;
    CHECK(f.model.observe(sample)==E::observed);f.resources=sample.resources;
    sample=f.sample();sample.resource_event_serial=1;CHECK(f.model.observe(sample)==E::observed);
    sample=f.sample();sample.resources.heat=1;++sample.resources.revision;
    sample.original_resource_change_verified=true;sample.resource_event_serial=2;
    CHECK(f.model.observe(sample)==E::rejected); // old resource evidence cannot be made fresh by a lower no-change serial
}
void certified_mechanical_guidance() {
    Fixture f(config(true));auto start=f.service();auto g=f.model.guide(f.ns);
    CHECK(g.component==71&&g.action==72&&g.motion_spec==73&&g.step==0);
    auto motion=f.motion(start);auto bad=motion;bad.component=81;
    CHECK(f.model.mechanical_motion(f.stamp(),bad)==E::rejected);
    bad=motion;bad.complete=false;CHECK(f.model.mechanical_motion(f.stamp(),bad)==E::rejected);
    bad=motion;bad.continuous=false;CHECK(f.model.mechanical_motion(f.stamp(),bad)==E::rejected);
    bad=motion;bad.motion_begin_sequence=1;CHECK(f.model.mechanical_motion(f.stamp(),bad)==E::rejected);
    CHECK(f.model.mechanical_motion(f.stamp(),motion)==E::request_ready);auto request=f.take();
    CHECK(request.kind==A::mechanical_original&&request.component==71&&request.mechanical_plan==70);
    CHECK(f.model.guide(f.ns).component==71);CHECK(f.ack(request)==E::original_accepted);
    CHECK(f.model.guide(f.ns).component==81);
    CHECK(f.model.mechanical_motion(f.stamp(),motion)==E::rejected);
    motion=f.motion(start);CHECK(f.model.mechanical_motion(f.stamp(),motion)==E::request_ready);request=f.take();
    f.original.servicing=false;auto ack=f.receipt(request);ack.mechanical_completion_verified=true;
    ack.resources.ammunition+=7;++ack.resources.revision;ack.original_resource_change_verified=true;
    CHECK(f.model.acknowledge(ack)==E::original_accepted);CHECK(!f.model.guide(f.ns).component);
    CHECK(f.model.resources().ammunition==507); // exact original receipt; no presumed clip size or refill count
    Fixture lost(config(true));start=lost.service();lost.pose.left_tracked=false;lost.observe();
    CHECK(!lost.model.guide(lost.ns).component);lost.pose.left_tracked=true;lost.observe();
    CHECK(!lost.model.guide(lost.ns).component);
    CHECK(lost.model.begin_mechanical(lost.stamp(),start)==E::rejected); // interrupted mechanical motion is never replayed
    Fixture input(config(true));input.service();input.original.input_allowed=false;input.observe();
    CHECK(!input.model.guide(input.ns).component);
}
} // namespace
int main() {
    geometry_and_certificates();input_and_identity();fixed_mount_and_original_animation();
    pose_and_original_gates();interruption_and_native_fire_lease();shot_identity_and_resource_receipts();
    certified_mechanical_guidance();
    std::printf("Troika interaction: %u synthetic checks passed; game/retail/headset wiring pending.\n",checks);
}
