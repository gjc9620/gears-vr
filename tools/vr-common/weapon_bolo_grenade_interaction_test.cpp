#include "weapon_bolo_grenade_interaction.h"
#include "weapon_equipment_model.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <type_traits>

using namespace gears_weapon;
using E=BoloEvent;
using S=BoloStage;
using A=BoloAction;
static_assert(!std::is_copy_constructible<BoloGrenadeInteraction>::value&&
              !std::is_copy_assignable<BoloGrenadeInteraction>::value,"A transaction has one owner");
namespace {
unsigned checks{};
void check(bool value,char const* expression,int line){
    ++checks;
    if(!value){std::fprintf(stderr,"line %d: %s\n",line,expression);std::exit(1);}
}
#define CHECK(x) check(bool(x),#x,__LINE__)
bool near(double a,double b,double tolerance=1e-6){return std::abs(a-b)<tolerance;}
constexpr BoloIdentity identity{11,12,13,14,15,16,17,18,19};

// Synthetic instance certificates only. These timings, IDs and fuse choices
// are not claims about retail Bolo semantics or a headset acceptance result.
BoloInstanceConfig config(BoloFuseOwner fuse=BoloFuseOwner::original_projectile){
    BoloInstanceConfig c{};c.identity=identity;c.config_serial=21;c.original_projectile_type=22;
    c.grip_geometry_serial=23;c.release_rule_serial=24;c.fuse_rule_serial=25;
    c.prepare_entry=26;c.launch_entry=27;c.safety_entry=28;
    c.original_no_ammo_cue=29;c.audio_rule_serial=30;
    c.release_input=BoloReleaseInput::right_trigger_release_after_press;c.fuse_owner=fuse;
    c.current_instance_verified=c.original_frag_classes_verified=c.grip_geometry_verified=
        c.release_input_verified=c.fuse_rule_verified=c.original_prepare_no_ammo_change_verified=
        c.original_launch_once_verified=c.original_initial_pose_velocity_override_verified=
        c.original_effects_preserved_verified=c.non_throwing_safety_entry_verified=
        c.original_gravity_and_units_verified=c.original_no_ammo_cue_verified=true;
    return c;
}
struct Fixture {
    BoloGrenadeInteraction model{};
    BoloInstanceConfig certificate{};
    std::uint64_t sequence{},hand_sequence{},edge{},native_event{},preparation{100};
    std::uint64_t epoch{1},reference{1},equipment_generation{};
    std::int64_t ns{2000000000},game_tick{100};
    bool trigger_down{};
    explicit Fixture(std::uint32_t count=3,BoloFuseOwner fuse=BoloFuseOwner::original_projectile,
                     Vector offset={},gears_vr::StereoSource source=gears_vr::StereoSource::RuntimeTracked,
                     BoloReleaseInput rule=BoloReleaseInput::right_trigger_release_after_press){
        certificate=config(fuse);certificate.centre_in_right_grip_metres=offset;
        certificate.release_input=rule;
        if(rule==BoloReleaseInput::right_grip_release_after_hold)certificate.motion.virtual_mass_kg=2.;
        certificate.input_source=source;
        CHECK(model.bind(certificate,{1,count,true}));
    }
    BoloStamp stamp(std::int64_t delta=1000000){
        ns+=delta;++game_tick;return {certificate.identity,model.binding_token(),++sequence,epoch,ns,game_tick};
    }
    BoloFrame frame(Vector position={0,1.5,0},std::int64_t delta=10000000){
        BoloFrame f{};f.stamp=stamp(delta);f.inventory=model.inventory();
        f.hands.source=f.stereo.source=certificate.input_source;
        f.hands.session=f.stereo.session=certificate.identity.xr_session;
        f.hands.space_epoch=f.stereo.space_epoch=certificate.identity.space_epoch;
        f.hands.input_epoch=certificate.identity.input_epoch;
        f.hands.profile_generation=certificate.identity.profile_generation;
        f.hands.sequence=f.stereo.sequence=++hand_sequence;
        f.hands.predicted_time=f.stereo.predicted_time=ns+8000000000LL;
        f.hands.received_ns=ns-1000;f.hands.sync_ns=ns-2000;
        f.hands.focused=f.stereo.focused=1;
        f.hands.hands[1].grip_active=1;f.hands.hands[1].grip_flags=15;
        f.hands.hands[1].grip={position,{0,0,0,1}};
        f.hands.hands[1].trigger_active=f.hands.hands[1].squeeze_active=1;
        f.hands.hands[1].trigger=trigger_down?1.f:0.f;
        f.mapping.motion_epoch=epoch;f.mapping.reference_generation=reference;
        f.mapping.frozen_reference={{0,1.65,0},{0,0,0,1}};
        f.mapping.game_base={{100,20,30},{0,0,0}};f.mapping.units_per_metre=100;
        f.mapping.pawn_velocity_world_units_per_second={50,-20,0};f.mapping.current_instance_verified=true;
        f.right_pocket_weapon=certificate.identity.weapon;
        f.equipment_hold_generation=equipment_generation;
        f.original_equipped=f.original_interaction_allowed=true;return f;
    }
    BoloInput input(BoloFrame& frame,BoloInputKind kind,BodySlot slot=BodySlot::none){
        if(kind==BoloInputKind::right_trigger_press)trigger_down=true;
        if(kind==BoloInputKind::right_trigger_release)trigger_down=false;
        frame.hands.hands[1].trigger=trigger_down?1.f:0.f;
        BoloInput command{};command.kind=kind;command.edge_serial=++edge;
        command.hand_sequence=frame.hands.sequence;command.input_epoch=frame.hands.input_epoch;
        command.sync_ns=frame.hands.sync_ns;command.slot=slot;command.continuous_input=true;return command;
    }
    BoloDecision step(BoloInputKind kind,BodySlot slot=BodySlot::none){
        auto f=frame();const auto command=input(f,kind,slot);return model.update(f,command);
    }
    void grab(){
        CHECK(step(BoloInputKind::right_grip_press,BodySlot::grenade).event==E::grabbed);
        CHECK(model.reserved_count()==1&&model.inventory().count>0);
    }
    BoloRequest take(A action){
        const auto r=model.take_request();CHECK(r&&r->action==action);
        CHECK(!model.take_request());return *r;
    }
    BoloOriginalReceipt receipt(BoloRequest const& request,bool accepted=true,bool safety_projectile=false){
        BoloOriginalReceipt r{};r.stamp=stamp();r.request_token=request.token;
        r.original_event_serial=++native_event;r.executed_ns=request.authored_ns+1;
        r.accepted=accepted;r.inventory=model.inventory();r.fuse=model.original_fuse();
        r.original_preparation_serial=request.original_preparation_serial;
        if(!accepted)return r;
        r.original_effect_applied_once=r.original_fuse_preserved=r.original_effects_preserved=true;
        if(request.action==A::prepare_original){
            r.original_preparation_serial=++preparation;
            if(certificate.fuse_owner==BoloFuseOwner::original_preparation)
                r.fuse={700,request.authored_game_tick,true};
        }else if(request.action==A::launch_original||safety_projectile){
            r.original_projectiles_spawned=1;r.original_projectile=800+native_event;
            r.original_projectile_type=certificate.original_projectile_type;
            ++r.inventory.revision;--r.inventory.count;
            r.tracked_initial_pose_and_velocity_applied=request.action==A::launch_original;
            if(!r.fuse.active)r.fuse={701,request.authored_game_tick,true};
        }
        r.native_safety_complete=request.action==A::preserve_original_on_interrupt;return r;
    }
    void prepare(){
        CHECK(step(BoloInputKind::right_trigger_press).event==E::request_ready);
        const auto request=take(A::prepare_original);
        CHECK(request.before.count==model.inventory().count&&request.expected_count==request.before.count);
        CHECK(model.acknowledge(receipt(request))==E::prepared);
    }
    BoloRequest throw_ready(Vector velocity={4,2,-1},bool rotating=false){
        grab();prepare();
        for(unsigned i=0;i<12;++i){
            const double t=.01*double(i);
            auto f=frame(rotating?Vector{0,1.5,0}:sum(Vector{0,1.5,0},scaled(velocity,t)));
            // Changing HMD direction cannot determine release direction.
            f.stereo.head.orientation={0,std::sin(t),0,std::cos(t)};
            if(rotating){const auto a=.1*double(i);f.hands.hands[1].grip.orientation={0,0,std::sin(a/2),std::cos(a/2)};}
            const auto command=i==11?input(f,BoloInputKind::right_trigger_release):BoloInput{};
            const auto result=model.update(f,command);
            CHECK(result.event==(i==11?E::request_ready:E::observed));
        }
        return take(A::launch_original);
    }
};

void certificate_gates(){
    BoloGrenadeInteraction m;
    CHECK(!m.bind({},{}));
    auto c=config();CHECK(c.valid());
    auto unknown=c;unknown.release_input=BoloReleaseInput::unknown;CHECK(!m.bind(unknown,{1,2,true}));
    unknown=c;unknown.fuse_owner=BoloFuseOwner::unknown;CHECK(!m.bind(unknown,{1,2,true}));
    bool BoloInstanceConfig::* const gates[]={
        &BoloInstanceConfig::current_instance_verified,&BoloInstanceConfig::original_frag_classes_verified,
        &BoloInstanceConfig::grip_geometry_verified,&BoloInstanceConfig::release_input_verified,
        &BoloInstanceConfig::fuse_rule_verified,&BoloInstanceConfig::original_prepare_no_ammo_change_verified,
        &BoloInstanceConfig::original_launch_once_verified,&BoloInstanceConfig::original_initial_pose_velocity_override_verified,
        &BoloInstanceConfig::original_effects_preserved_verified,&BoloInstanceConfig::non_throwing_safety_entry_verified,
        &BoloInstanceConfig::original_gravity_and_units_verified,&BoloInstanceConfig::original_no_ammo_cue_verified};
    for(auto gate:gates){auto broken=c;broken.*gate=false;CHECK(!m.bind(broken,{1,2,true}));}
    CHECK(!m.bind(c,{1,2,false}));CHECK(!m.bind(c,{0,2,true}));
    unknown=c;unknown.original_no_ammo_cue=0;CHECK(!m.bind(unknown,{1,2,true}));
    unknown=c;unknown.audio_rule_serial=0;CHECK(!m.bind(unknown,{1,2,true}));
    c.centre_in_right_grip_metres={1,0,0};CHECK(!m.bind(c,{1,2,true}));
    c=config();c.motion.maximum_speed_mps=std::numeric_limits<double>::quiet_NaN();
    CHECK(!m.bind(c,{1,2,true}));
}
void grab_and_empty(){
    Fixture empty(0);
    auto frame=empty.frame();const auto press=empty.input(frame,BoloInputKind::right_grip_press,BodySlot::grenade);
    auto result=empty.model.update(frame,press);
    CHECK(result.event==E::no_ammo&&result.request_original_no_ammo_sound);
    const auto first_audio=result;
    CHECK(result.audio_token&&result.original_cue==empty.certificate.original_no_ammo_cue&&
          result.audio_for(identity,empty.model.binding_token()));
    auto other=identity;++other.weapon;CHECK(!result.audio_for(other,empty.model.binding_token()));
    CHECK(!result.audio_for(identity,empty.model.binding_token()+1));
    CHECK(empty.model.reserved_count()==0&&empty.model.inventory().count==0&&!empty.model.take_request());
    result=empty.model.update(empty.frame(),press);CHECK(!result.request_original_no_ammo_sound);
    result=empty.model.update(empty.frame());CHECK(!result.request_original_no_ammo_sound);
    result=empty.step(BoloInputKind::right_grip_press,BodySlot::grenade);
    CHECK(result.event==E::no_ammo&&result.request_original_no_ammo_sound);
    CHECK(result.audio_token>first_audio.audio_token);
    CHECK(empty.model.bind(empty.certificate,{1,0,true}));
    CHECK(!first_audio.audio_for(identity,empty.model.binding_token())&&
          !result.audio_for(identity,empty.model.binding_token()));
    Fixture f;
    frame=f.frame();auto wrong=f.input(frame,BoloInputKind::right_grip_press,BodySlot::grenade);wrong.hand=0;
    CHECK(f.model.update(frame,wrong).event==E::rejected);
    CHECK(f.step(BoloInputKind::right_grip_press,BodySlot::pistol).event==E::rejected);
    frame=f.frame();wrong=f.input(frame,BoloInputKind::right_grip_press,BodySlot::grenade);wrong.continuous_input=false;
    CHECK(f.model.update(frame,wrong).event==E::rejected&&f.model.reserved_count()==0);
    f.grab();CHECK(f.model.suppress_original_trajectory());
    CHECK(f.model.update(f.frame()).event==E::observed); // Grip release does not throw/stow.
    CHECK(!f.model.take_request()&&f.model.reserved_count()==1);
    CHECK(f.step(BoloInputKind::right_grip_press,BodySlot::grenade).event==E::stowed);
    CHECK(f.model.inventory().count==3&&!f.model.suppress_original_trajectory());
    auto bad=f.frame();bad.original_equipped=false;
    CHECK(f.model.update(bad,f.input(bad,BoloInputKind::right_grip_press,BodySlot::grenade)).event==E::interrupted);
    CHECK(f.model.reserved_count()==0);
}
void real_motion_and_one_original_commit(){
    Fixture f;
    const auto request=f.throw_ready();
    CHECK(near(request.tracked.velocity_mps.x,4)&&near(request.tracked.velocity_mps.y,2)&&
          near(request.tracked.velocity_mps.z,-1));
    CHECK(near(request.tracked.mass_kg,3)&&near(request.tracked.impulse_kg_mps.x,12));
    CHECK(near(request.launch.position.x,111)&&near(request.launch.position.y,64)&&near(request.launch.position.z,37));
    CHECK(near(request.launch.velocity.x,150)&&near(request.launch.velocity.y,380)&&near(request.launch.velocity.z,200));
    CHECK(near(request.launch.impulse.y,1140)&&near(request.launch.gravity.z,-981));
    CHECK(request.before.count==3&&request.expected_count==2&&f.model.inventory().count==3);
    CHECK(request.executable_at(request.authored_ns,identity,f.model.binding_token(),f.epoch,f.reference));
    CHECK(!request.executable_at(request.valid_until_ns+1,identity,f.model.binding_token(),f.epoch,f.reference));
    CHECK(!request.executable_at(request.authored_ns,identity,f.model.binding_token(),f.epoch+1,f.reference));
    CHECK(!request.executable_at(request.authored_ns,identity,f.model.binding_token(),f.epoch,f.reference+1));
    auto changed=identity;++changed.weapon;
    CHECK(!request.executable_at(request.authored_ns,changed,f.model.binding_token(),f.epoch,f.reference));
    CHECK(!f.model.take_request());
    CHECK(f.step(BoloInputKind::right_trigger_release).event==E::observed);
    auto receipt=f.receipt(request);CHECK(f.model.acknowledge(receipt)==E::thrown);
    CHECK(f.model.inventory().count==2&&f.model.reserved_count()==0&&f.model.stage()==S::idle);
    CHECK(f.model.acknowledge(receipt)==E::rejected&&f.model.inventory().count==2);
    CHECK(f.step(BoloInputKind::right_trigger_release).event==E::rejected);
    for(auto velocity:{Vector{0,5,-2},Vector{0,0,-5},Vector{0,-2,-4}}){
        Fixture direction;const auto launch=direction.throw_ready(velocity);
        CHECK(near(launch.tracked.velocity_mps.x,velocity.x)&&near(launch.tracked.velocity_mps.y,velocity.y)&&
              near(launch.tracked.velocity_mps.z,velocity.z));
    }
    Fixture wrist(3,BoloFuseOwner::original_projectile,{.2,0,0});
    const auto launch=wrist.throw_ready({},true);
    CHECK(launch.tracked.velocity_mps.y>1&&launch.tracked.release_position.y>1.65);
}
void receipt_validation_and_old_callbacks(){
    Fixture f;const auto request=f.throw_ready();
    auto valid=f.receipt(request);
    auto broken=valid;broken.inventory.count=1;
    CHECK(f.model.acknowledge(broken)==E::safety_violation&&f.model.inventory().count==3);
    broken=valid;broken.original_projectiles_spawned=2;CHECK(f.model.acknowledge(broken)==E::safety_violation);
    broken=valid;broken.inventory.revision=request.before.revision;CHECK(f.model.acknowledge(broken)==E::safety_violation);
    broken=valid;broken.original_effects_preserved=false;CHECK(f.model.acknowledge(broken)==E::safety_violation);
    broken=valid;broken.tracked_initial_pose_and_velocity_applied=false;CHECK(f.model.acknowledge(broken)==E::safety_violation);
    broken=valid;++broken.original_projectile_type;CHECK(f.model.acknowledge(broken)==E::safety_violation);
    broken=valid;broken.executed_ns=request.valid_until_ns+1;CHECK(f.model.acknowledge(broken)==E::safety_violation);
    broken=valid;--broken.stamp.binding;CHECK(f.model.acknowledge(broken)==E::rejected);
    broken=valid;++broken.request_token;broken.stamp.sequence+=10000;broken.stamp.monotonic_ns+=1000000000;
    CHECK(f.model.acknowledge(broken)==E::rejected);
    CHECK(f.model.acknowledge(valid)==E::thrown); // Bad callback did not poison sequence.
    const auto old_binding=f.model.binding_token();
    CHECK(f.model.bind(f.certificate,{10,2,true})&&f.model.binding_token()!=old_binding);
    broken=valid;broken.stamp.sequence+=20000;broken.stamp.monotonic_ns+=2000000000;
    CHECK(f.model.acknowledge(broken)==E::rejected&&f.model.inventory().count==2);
    f.grab();f.prepare();
    CHECK(f.model.acknowledge(valid)==E::rejected&&f.model.stage()==S::prepared);
    CHECK(!f.model.bind(f.certificate,{10,2,true}));
}
void interruption_and_native_fuse(){
    Fixture f(3,BoloFuseOwner::original_preparation);f.grab();f.prepare();
    const auto old_fuse=f.model.original_fuse();CHECK(old_fuse.active);
    for(unsigned i=0;i<8;++i)CHECK(f.model.update(f.frame({.04*double(i),1.5,0})).event==E::observed);
    auto lost=f.frame({.4,1.5,0});lost.hands.hands[1].grip_flags=0;
    CHECK(f.model.update(lost,f.input(lost,BoloInputKind::right_trigger_release)).event==E::safety_required);
    CHECK(f.model.original_fuse()==old_fuse&&f.model.inventory().count==3);
    const auto safety=f.take(A::preserve_original_on_interrupt);
    CHECK(safety.fuse==old_fuse&&safety.tracked.predicted_time==0);
    CHECK(f.model.cancel()==E::interrupted&&!f.model.take_request());
    auto bad=f.receipt(safety);CHECK(f.model.acknowledge(bad)==E::safety_violation); // Active fuse cannot be reclaimed free.
    auto native=f.receipt(safety,true,true);bad=native;++bad.fuse.started_game_tick;
    CHECK(f.model.acknowledge(bad)==E::safety_violation);
    bad=native;++bad.fuse.original_serial;CHECK(f.model.acknowledge(bad)==E::safety_violation);
    CHECK(f.model.acknowledge(native)==E::safety_completed);
    CHECK(f.model.inventory().count==2&&f.model.reserved_count()==0&&f.model.stage()==S::interrupted);
    CHECK(f.model.acknowledge(native)==E::rejected&&f.model.inventory().count==2);

    Fixture unarmed;unarmed.grab();unarmed.prepare();
    CHECK(unarmed.model.cancel()==E::safety_required);
    const auto abort=unarmed.take(A::preserve_original_on_interrupt);
    CHECK(unarmed.model.acknowledge(unarmed.receipt(abort))==E::safety_completed);
    CHECK(unarmed.model.inventory().count==3&&unarmed.model.reserved_count()==0);

    Fixture failure;failure.grab();failure.prepare();CHECK(failure.model.cancel()==E::safety_required);
    const auto obligation=failure.take(A::preserve_original_on_interrupt);
    CHECK(failure.model.acknowledge(failure.receipt(obligation,false))==E::safety_violation);
    CHECK(!failure.model.bind(failure.certificate,{1,3,true}));
    CHECK(failure.model.cancel()==E::interrupted&&!failure.model.take_request());
}
void late_release_only_cleanup(){
    Fixture f;f.grab();f.prepare();CHECK(f.model.cancel()==E::safety_required);
    const auto stop=f.take(A::preserve_original_on_interrupt);
    f.ns+=500000000;
    auto receipt=f.receipt(stop);receipt.executed_ns=f.ns-1;
    CHECK(receipt.executed_ns>stop.valid_until_ns);
    auto broken=receipt;broken.native_safety_complete=false;
    CHECK(f.model.acknowledge(broken)==E::safety_violation);
    broken=receipt;broken.executed_ns=receipt.stamp.monotonic_ns+1;
    CHECK(f.model.acknowledge(broken)==E::safety_violation);
    CHECK(f.model.acknowledge(receipt)==E::safety_completed);
    CHECK(f.model.inventory().count==3&&f.model.reserved_count()==0&&f.model.stage()==S::interrupted);
    // An expired actual throw cannot borrow the release-only exception.
    Fixture launch;const auto q=launch.throw_ready();launch.ns+=500000000;
    auto late=launch.receipt(q);late.executed_ns=launch.ns-1;
    CHECK(launch.model.acknowledge(late)==E::safety_violation);
    CHECK(launch.model.inventory().count==3);
    // Nor can a live original fuse be made to disappear after expiry.
    Fixture lit(3,BoloFuseOwner::original_preparation);lit.grab();lit.prepare();
    CHECK(lit.model.cancel()==E::safety_required);const auto s=lit.take(A::preserve_original_on_interrupt);
    lit.ns+=500000000;auto live=lit.receipt(s,true,true);live.executed_ns=lit.ns-1;
    CHECK(lit.model.acknowledge(live)==E::safety_violation&&lit.model.inventory().count==3);
}
void ambiguous_and_undispatched_operations(){
    Fixture undelivered;
    undelivered.grab();
    CHECK(undelivered.step(BoloInputKind::right_trigger_press).event==E::request_ready);
    CHECK(undelivered.model.cancel()==E::interrupted&&!undelivered.model.take_request());
    CHECK(undelivered.model.inventory().count==3&&undelivered.model.reserved_count()==0);

    Fixture pending(3,BoloFuseOwner::original_preparation);pending.grab();
    CHECK(pending.step(BoloInputKind::right_trigger_press).event==E::request_ready);
    const auto prepare=pending.take(A::prepare_original);
    CHECK(pending.model.cancel()==E::interrupted&&!pending.model.take_request());
    CHECK(pending.model.acknowledge(pending.receipt(prepare))==E::safety_required);
    const auto safety=pending.take(A::preserve_original_on_interrupt);
    CHECK(safety.token!=prepare.token&&safety.fuse.active);
    CHECK(pending.model.acknowledge(pending.receipt(safety,true,true))==E::safety_completed);

    Fixture late;const auto launch=late.throw_ready();
    CHECK(late.model.cancel()==E::interrupted&&!late.model.take_request());
    // It executed while valid, but its receipt arrives after lost tracking.
    late.ns+=200000000;
    CHECK(late.model.acknowledge(late.receipt(launch))==E::thrown);
    CHECK(late.model.inventory().count==2&&late.model.stage()==S::interrupted);

    Fixture denied;const auto rejected=denied.throw_ready();
    CHECK(denied.model.acknowledge(denied.receipt(rejected,false))==E::safety_required);
    CHECK(denied.model.inventory().count==3);
    const auto cleanup=denied.take(A::preserve_original_on_interrupt);
    CHECK(denied.model.acknowledge(denied.receipt(cleanup))==E::safety_completed);

    // Revoke a launch that has not yet been handed to the adapter.
    Fixture revoked;revoked.grab();revoked.prepare();
    for(unsigned i=0;i<8;++i){
        auto frame=revoked.frame({.04*double(i),1.5,0});
        const auto command=i==7?revoked.input(frame,BoloInputKind::right_trigger_release):BoloInput{};
        CHECK(revoked.model.update(frame,command).event==
              (i==7?E::request_ready:E::observed));
    }
    CHECK(revoked.model.cancel()==E::safety_required);
    const auto only=revoked.take(A::preserve_original_on_interrupt);
    CHECK(only.tracked.predicted_time==0&&revoked.model.inventory().count==3);

    Fixture early;early.grab();
    CHECK(early.step(BoloInputKind::right_trigger_press).event==E::request_ready);
    const auto unresolved=early.take(A::prepare_original);
    auto early_frame=early.frame();const auto release=early.input(early_frame,BoloInputKind::right_trigger_release);
    CHECK(early.model.update(early_frame,release).event==E::interrupted);
    CHECK(early.model.acknowledge(early.receipt(unresolved))==E::safety_required);
    CHECK(early.model.pending_request().action==A::preserve_original_on_interrupt);

    Fixture boundary;boundary.grab();
    ++boundary.epoch;
    auto changed=boundary.frame();const auto press=boundary.input(changed,BoloInputKind::right_trigger_press);
    CHECK(boundary.model.update(changed,press).event==E::motion_reset);
    CHECK(boundary.model.update(boundary.frame(),press).event==E::rejected);
    CHECK(boundary.model.stage()==S::held&&!boundary.model.take_request());
    changed=boundary.frame();auto recovery=boundary.input(changed,BoloInputKind::right_trigger_press);recovery.continuous_input=false;
    CHECK(boundary.model.update(changed,recovery).event==E::rejected);
    recovery.continuous_input=true;
    CHECK(boundary.model.update(boundary.frame(),recovery).event==E::rejected&&!boundary.model.take_request());

    Fixture refused;refused.grab();
    CHECK(refused.step(BoloInputKind::right_trigger_press).event==E::request_ready);
    const auto no_prepare=refused.take(A::prepare_original);
    CHECK(refused.model.acknowledge(refused.receipt(no_prepare,false))==E::interrupted);
    CHECK(refused.model.reserved_count()==0&&refused.model.inventory().count==3&&!refused.model.take_request());
}
void bad_motion_never_throws(){
    // These are separate interrupted preparations: no old trajectory survives.
    for(unsigned mode=0;mode<14;++mode){
        Fixture f;f.grab();f.prepare();
        for(unsigned i=0;i<8;++i)CHECK(f.model.update(f.frame({.04*double(i),1.5,0})).event==E::observed);
        auto frame=f.frame({.36,1.5,0});
        switch(mode){
            case 0:frame.hands.hands[1].grip_active=0;break;
            case 1:++frame.stamp.motion_epoch;++frame.mapping.motion_epoch;break; // Snap.
            case 2:++frame.mapping.reference_generation;break; // Recenter.
            case 3:frame.mapping.frozen_reference.position.x=.5;break; // Unannounced change.
            case 4:frame.original_equipped=false;break;
            case 5:frame.original_interaction_allowed=false;break; // Pause/death.
            case 6:frame.original_owns_hands=true;break;
            case 7:++frame.hands.input_epoch;break;
            case 8:++frame.hands.session;break;
            case 9:++frame.hands.space_epoch;break;
            case 10:frame.hands.received_ns-=101000000;break;
            case 11:frame.mapping.current_instance_verified=false;break;
            case 12:++frame.hands.profile_generation;break;
            case 13:frame.mapping.units_per_metre=110;break;
        }
        const auto result=f.model.update(frame,f.input(frame,BoloInputKind::right_trigger_release));
        CHECK(result.event==E::safety_required);
        CHECK(f.model.last_boundary_flags()==(mode==1?1u:mode==2?2u:mode==3?4u:mode==13?8u:0u));
        CHECK(f.model.pending_request().action==A::preserve_original_on_interrupt&&f.model.inventory().count==3);
        const auto request=f.take(A::preserve_original_on_interrupt);
        CHECK(f.model.acknowledge(f.receipt(request))==E::safety_completed);
        CHECK(f.step(BoloInputKind::right_trigger_release).event==E::rejected);
        CHECK(!f.model.take_request()&&f.model.inventory().count==3);
    }
    // Deliberate stationary release is valid and covered separately below.
    for(unsigned mode:{0u,2u,3u}){
        Fixture f;f.grab();f.prepare();
        const unsigned count=mode==0?2u:8u;
        for(unsigned i=0;i<count;++i){
            auto point=Vector{.04*double(i),1.5,0};
            if(mode==2&&i==count-1)point.x=4; // Tracking teleport on release.
            auto frame=f.frame(point,mode==3&&i==count-1?70000000:10000000);
            const auto result=f.model.update(frame,i==count-1?f.input(frame,BoloInputKind::right_trigger_release):BoloInput{});
            if(i==count-1){CHECK(result.event==E::safety_required);if(mode==3)CHECK(f.model.last_boundary_flags()==16u);}
            else CHECK(result.event==E::observed);
        }
        CHECK(f.model.pending_request().action==A::preserve_original_on_interrupt&&f.model.inventory().count==3);
    }
    Fixture broken_inventory;broken_inventory.grab();broken_inventory.prepare();
    auto unexpected=broken_inventory.frame();--unexpected.inventory.count;++unexpected.inventory.revision;
    CHECK(broken_inventory.model.update(unexpected).event==E::safety_required);
    CHECK(broken_inventory.model.inventory().count==3); // No invented reconciliation.
}
void input_provenance_and_verified_press(){
    for(unsigned mode=0;mode<7;++mode){
        Fixture idle;
        auto frame=idle.frame();auto command=idle.input(frame,BoloInputKind::right_grip_press,BodySlot::grenade);
        switch(mode){
            case 0:--command.hand_sequence;break;
            case 1:++command.input_epoch;break;
            case 2:--command.sync_ns;break;
            case 3:command.continuous_input=false;break;
            case 4:command.hand_sequence=command.input_epoch=0;command.sync_ns=0;break;
            case 5:command.edge_serial=0;break;
            case 6:command.hand=0;break;
        }
        const auto decision=idle.model.update(frame,command);
        CHECK(decision.event==E::rejected&&!decision.request_original_no_ammo_sound&&!decision.audio_token);
        CHECK(idle.model.stage()==S::idle&&idle.model.reserved_count()==0&&!idle.model.take_request());
    }
    for(unsigned mode=0;mode<13;++mode){
        Fixture prepared;prepared.grab();prepared.prepare();
        for(unsigned i=0;i<8;++i)CHECK(prepared.model.update(prepared.frame({.04*double(i),1.5,0})).event==E::observed);
        auto frame=prepared.frame({.36,1.5,0});
        auto release=prepared.input(frame,BoloInputKind::right_trigger_release);
        switch(mode){
            case 0:--release.hand_sequence;break;
            case 1:++release.input_epoch;break;
            case 2:--release.sync_ns;break;
            case 3:release.continuous_input=false;break;
            case 4:frame.hands.hands[1].trigger_active=0;break;
            case 5:frame.hands.hands[1].trigger=.8f;break;
            case 6:frame.hands.sync_ns-=70000000;release.sync_ns=frame.hands.sync_ns;break;
            case 7:frame.hands.sync_ns-=10000000;release.sync_ns=frame.hands.sync_ns;break;
            case 8:frame.hands.predicted_time-=10000000;frame.stereo.predicted_time=frame.hands.predicted_time;break;
            case 9:release.hand_sequence=release.input_epoch=0;release.sync_ns=0;break;
            case 10:release.edge_serial=0;break;
            case 11:release.edge_serial=1;break;
            case 12:release={};break; // Low trigger without a verified release edge.
        }
        CHECK(prepared.model.update(frame,release).event==E::safety_required);
        CHECK(prepared.model.inventory().count==3&&prepared.model.pending_request().action==A::preserve_original_on_interrupt);
        const auto safety=prepared.take(A::preserve_original_on_interrupt);
        CHECK(safety.tracked.predicted_time==0);
        CHECK(prepared.model.acknowledge(prepared.receipt(safety))==E::safety_completed);
        CHECK(!prepared.model.take_request()&&prepared.model.inventory().count==3);
    }
    for(unsigned mode=0;mode<5;++mode){
        Fixture held;held.grab();auto frame=held.frame();
        auto press=held.input(frame,BoloInputKind::right_trigger_press);
        switch(mode){
            case 0:--press.hand_sequence;break;
            case 1:++press.input_epoch;break;
            case 2:--press.sync_ns;break;
            case 3:frame.hands.hands[1].trigger_active=0;frame.hands.hands[1].trigger=0;break;
            case 4:frame.hands.hands[1].trigger=.2f;break;
        }
        CHECK(held.model.update(frame,press).event==E::rejected&&!held.model.take_request());
        CHECK(held.step(BoloInputKind::right_trigger_release).event==E::observed);
        CHECK(held.model.stage()==S::held&&held.model.inventory().count==3&&!held.model.take_request());
    }
    // A press certificate from a previous local hold cannot prepare the new one.
    Fixture new_hold;new_hold.grab();
    auto frame=new_hold.frame();const auto old_press=new_hold.input(frame,BoloInputKind::right_trigger_press);
    CHECK(new_hold.step(BoloInputKind::right_grip_press,BodySlot::grenade).event==E::stowed);
    new_hold.grab();
    CHECK(new_hold.model.update(new_hold.frame(),old_press).event==E::rejected);
    CHECK(new_hold.step(BoloInputKind::right_trigger_release).event==E::observed&&!new_hold.model.take_request());
}
void existing_equipment_hold(){
    Fixture f;f.equipment_generation=500;
    auto first=f.frame();CHECK(f.model.adopt_equipment_hold(first).event==E::grabbed);
    CHECK(f.edge==0&&f.model.stage()==S::held&&f.model.reserved_count()==1);
    CHECK(f.model.inventory().count==3&&!f.model.take_request());
    CHECK(f.model.adopt_equipment_hold(first).event==E::rejected); // No replay/reacquisition.
    CHECK(f.step(BoloInputKind::right_grip_press,BodySlot::grenade).event==E::observed);
    CHECK(f.model.stage()==S::held&&f.model.inventory().count==3); // Sole equipment arbiter.
    f.prepare();
    for(unsigned i=0;i<12;++i){
        auto frame=f.frame({.04*double(i),1.5,.01*double(i)});
        const auto input=i==11?f.input(frame,BoloInputKind::right_trigger_release):BoloInput{};
        CHECK(f.model.update(frame,input).event==(i==11?E::request_ready:E::observed));
    }
    const auto request=f.take(A::launch_original);const auto receipt=f.receipt(request);
    CHECK(f.model.acknowledge(receipt)==E::thrown&&f.model.inventory().count==2);
    CHECK(f.model.acknowledge(receipt)==E::rejected&&f.model.inventory().count==2);
    // No new pocket Grip is required for the original newly re-equipped one.
    CHECK(f.model.adopt_equipment_hold(f.frame()).event==E::grabbed);
    CHECK(f.model.inventory().count==2&&!f.model.take_request());
    for(unsigned mode=0;mode<2;++mode)for(unsigned bad=0;bad<7;++bad){
        Fixture q(3,BoloFuseOwner::original_projectile,{},gears_vr::StereoSource::RuntimeTracked,
            mode?BoloReleaseInput::right_grip_release_after_hold:BoloReleaseInput::right_trigger_release_after_press);
        auto frame=q.frame();frame.equipment_hold_generation=500;
        if(mode){frame.hands.hands[1].squeeze=1;frame.hands.hands[1].pressed[gears_vr::control_squeeze]=1;}
        switch(bad){case 0:frame.equipment_hold_generation=0;break;
        case 1:frame.original_equipped=false;break;case 2:frame.original_interaction_allowed=false;break;
        case 3:frame.original_owns_hands=true;break;case 4:++frame.right_pocket_weapon;break;
        case 5:++frame.hands.input_epoch;break;case 6:frame.hands.hands[1].grip_active=0;break;}
        const auto adoption=q.model.adopt_equipment_hold(frame);
        CHECK(adoption.event==(bad==0?E::rejected:E::interrupted));
        CHECK(q.model.stage()==(bad==0?S::idle:S::interrupted));
        CHECK(q.model.reserved_count()==0&&!q.model.take_request()&&q.model.inventory().count==3);
        if(bad!=0)CHECK(q.model.adopt_equipment_hold(q.frame()).event==E::rejected&&!q.model.take_request());
    }
    {Fixture q(0);q.equipment_generation=500;
        CHECK(q.model.adopt_equipment_hold(q.frame()).event==E::rejected);
        CHECK(q.model.reserved_count()==0&&!q.model.take_request());}
    {Fixture q;q.equipment_generation=500;CHECK(q.model.adopt_equipment_hold(q.frame()).event==E::grabbed);
        ++q.equipment_generation;CHECK(q.model.update(q.frame()).event==E::interrupted);
        CHECK(!q.model.take_request()&&q.model.inventory().count==3);}
    {Fixture q;q.equipment_generation=500;CHECK(q.model.adopt_equipment_hold(q.frame()).event==E::grabbed);q.prepare();
        ++q.equipment_generation;CHECK(q.model.update(q.frame()).event==E::safety_required);
        const auto safety=q.take(A::preserve_original_on_interrupt);
        CHECK(q.model.acknowledge(q.receipt(safety))==E::safety_completed&&q.model.inventory().count==3);}
}
void gentle_and_stationary_release(){
    for(const auto velocity:{Vector{0,0,0},Vector{.1,0,0}}){
        Fixture f;const auto request=f.throw_ready(velocity);
        CHECK(near(request.tracked.velocity_mps.x,velocity.x));
        CHECK(near(request.tracked.velocity_mps.y,0)&&near(request.tracked.velocity_mps.z,0));
        CHECK(f.model.acknowledge(f.receipt(request))==E::thrown);
        CHECK(f.model.inventory().count==2&&!f.model.take_request());
    }
}
void explicit_provenance_profile(){
    using Source=gears_vr::StereoSource;
    Fixture simulator(3,BoloFuseOwner::original_projectile,{},Source::MetaOperatorSimulated);
    const auto request=simulator.throw_ready();
    CHECK(simulator.model.acknowledge(simulator.receipt(request))==E::thrown);
    for(const auto configured:{Source::RuntimeTracked,Source::MetaOperatorSimulated}){
        Fixture q(3,BoloFuseOwner::original_projectile,{},configured);q.grab();q.prepare();
        auto frame=q.frame();frame.hands.source=frame.stereo.source=configured==Source::RuntimeTracked?
            Source::MetaOperatorSimulated:Source::RuntimeTracked;
        CHECK(q.model.update(frame).event==E::safety_required);
        CHECK(q.model.inventory().count==3&&q.model.pending_request().action==A::preserve_original_on_interrupt);
    }
    BoloGrenadeInteraction q;auto c=config();c.input_source=static_cast<Source>(2);
    CHECK(!q.bind(c,{1,3,true}));
}
void original_ring_and_trigger_release(){
    using Observe=BoloTriggerObservation;
    for(unsigned scenario=0;scenario<11;++scenario){
        Fixture q;q.equipment_generation=42;BoloTriggerObserver observer;
        gears_hand_continuity::MotionHistory history{};
        const auto raw=[&](double x,bool down,std::uint32_t pressed,std::uint32_t released){
            auto f=q.frame({x,1.5,0});auto& right=f.hands.hands[1];right.trigger=down?1.f:0.f;
            right.pressed[gears_vr::control_trigger]=pressed;right.released[gears_vr::control_trigger]=released;
            if(history.count==gears_hand_continuity::motion_history_capacity){
                for(unsigned i=1;i<history.count;++i)history.frames[i-1]=history.frames[i];--history.count;}
            auto& sample=history.frames[history.count++];sample.input=f.hands;
            sample.stream_epoch=11;sample.tracking_epoch[0]=21;sample.tracking_epoch[1]=22;
            f.motion_history=history;return f;
        };
        auto baseline=raw(0,false,0,0);BoloInput input{};
        CHECK(observer.observe(baseline,input)==Observe::baseline&&input.kind==BoloInputKind::none);
        CHECK(q.model.adopt_equipment_hold(baseline).event==E::grabbed);
        auto press=raw(0,true,1,0);
        CHECK(observer.observe(press,input)==Observe::edge&&input.kind==BoloInputKind::right_trigger_press);
        CHECK(q.model.update(press,input).event==E::request_ready);
        const auto prepared=q.take(A::prepare_original);
        CHECK(prepared.original_input_sync_ns==press.hands.sync_ns);
        CHECK(q.model.acknowledge(q.receipt(prepared))==E::prepared);
        BoloFrame latest{},released{};
        for(unsigned i=0;i<8;++i){latest=raw(.02*double(i),i<6,1,i<6?0:1);if(i==6)released=latest;}
        CHECK(latest.stamp.monotonic_ns-press.stamp.monotonic_ns>60000000);
        if(scenario==1)++latest.motion_history.frames[7].input.sync_ns;
        if(scenario==2)latest.motion_history={};
        if(scenario==3)++latest.motion_history.frames[3].tracking_epoch[1];
        if(scenario==4){latest.hands.hands[1].pressed[gears_vr::control_trigger]=2;
            latest.hands.hands[1].trigger=1;latest.motion_history.frames[7].input=latest.hands;}
        if(scenario==5){++latest.hands.input_epoch;latest.motion_history.frames[7].input=latest.hands;}
        if(scenario==6){++latest.mapping.reference_generation;}
        if(scenario==7){latest.hands.hands[1].pressed[gears_vr::control_trigger]=0;
            latest.hands.hands[1].released[gears_vr::control_trigger]=0;latest.motion_history.frames[7].input=latest.hands;}
        if(scenario==8){for(unsigned i=0;i<latest.motion_history.count;++i){
            ++latest.motion_history.frames[i].stream_epoch;latest.motion_history.frames[i].tracking_epoch[1]++;}}
        if(scenario==9)for(unsigned i=0;i<latest.motion_history.count;++i)++latest.motion_history.frames[i].stream_epoch;
        if(scenario==10)for(unsigned i=0;i<latest.motion_history.count;++i)++latest.motion_history.frames[i].tracking_epoch[1];
        const auto observed=observer.observe(latest,input);
        if(scenario){
            // No malformed/missing ring, new trigger cycle or reference may
            // turn a poll into a release. The model must require safety.
            CHECK(q.model.update(latest,input).event==E::safety_required);
            if(scenario==6||scenario>=8)CHECK(q.model.last_boundary_flags()==(scenario==6?2u:scenario==8?96u:scenario==9?32u:64u));
            CHECK(q.model.inventory().count==3&&q.model.pending_request().action==A::preserve_original_on_interrupt);
            continue;
        }
        CHECK(observed==Observe::edge&&input.kind==BoloInputKind::right_trigger_release);
        CHECK(input.hand_sequence==released.hands.sequence&&input.hand_sequence<latest.hands.sequence);
        CHECK(input.sync_ns==released.hands.sync_ns);
        CHECK(q.model.update(latest,input).event==E::request_ready);
        const auto request=q.take(A::launch_original);
        CHECK(near(request.tracked.velocity_mps.x,2)&&near(request.tracked.release_position.x,.12));
        CHECK(request.tracked.predicted_time==released.hands.predicted_time);
        CHECK(request.original_input_sync_ns==released.hands.sync_ns&&
            request.valid_until_ns==released.hands.sync_ns+q.certificate.request_lease_ns);
        CHECK(q.model.inventory().count==3&&q.model.acknowledge(q.receipt(request))==E::thrown);
        CHECK(q.model.inventory().count==2);
        CHECK(observer.observe(latest,input)==Observe::duplicate&&input.kind==BoloInputKind::none);
    }
}
void grip_hold_release_owner(){
    constexpr auto rule=BoloReleaseInput::right_grip_release_after_hold;
    using Observe=BoloTriggerObservation;
    for(unsigned scenario=0;scenario<13;++scenario){
        Fixture q(3,BoloFuseOwner::original_projectile,{},gears_vr::StereoSource::RuntimeTracked,rule);
        q.equipment_generation=42;BoloTriggerObserver observer,diagnosed;
        BoloTriggerDiagnostic diagnostic{};BoloInput diagnosed_input{};
        gears_hand_continuity::MotionHistory history{};
        const auto raw=[&](double x,bool down,std::uint32_t pressed,std::uint32_t released){
            auto f=q.frame({x,1.5,0});auto& right=f.hands.hands[1];right.squeeze=down?1.f:0.f;
            right.pressed[gears_vr::control_squeeze]=pressed;right.released[gears_vr::control_squeeze]=released;
            // Trigger is deliberately up throughout: no trigger preparation.
            if(history.count==gears_hand_continuity::motion_history_capacity){
                for(unsigned i=1;i<history.count;++i)history.frames[i-1]=history.frames[i];--history.count;
            }
            auto& row=history.frames[history.count++];row.input=f.hands;
            row.stream_epoch=11;row.tracking_epoch[0]=21;row.tracking_epoch[1]=22;
            f.motion_history=history;return f;
        };
        auto baseline=raw(0,true,1,0);BoloInput input{};
        CHECK(observer.observe(baseline,input,rule)==Observe::baseline);
        CHECK(diagnosed.observe(baseline,diagnosed_input,rule,&diagnostic)==Observe::baseline);
        if(scenario==1||scenario==2||scenario==3){
            if(scenario==1)baseline.hands.hands[1].squeeze=0;
            if(scenario==2)baseline.hands.hands[1].squeeze_active=0;
            if(scenario==3)baseline.hands.hands[1].pressed[gears_vr::control_squeeze]=0;
            CHECK(q.model.adopt_equipment_hold(baseline).event==E::rejected);
            CHECK(!q.model.take_request()&&q.model.reserved_count()==0);continue;
        }
        CHECK(q.model.adopt_equipment_hold(baseline).event==E::request_ready);
        const auto prepare=q.take(A::prepare_original);
        CHECK(prepare.before.count==3&&prepare.expected_count==3);
        CHECK(q.model.acknowledge(q.receipt(prepare))==E::prepared);
        BoloFrame latest{},released{};
        for(unsigned i=0;i<8;++i){
            // Scenario 12 is a deliberate stationary release, not bad tracking.
            latest=raw(scenario==12?0:.02*double(i),i<6,1,i<6?0:1);
            if(i==6)released=latest;
        }
        if(scenario==4){latest.hands.hands[1].pressed[gears_vr::control_squeeze]=2;
            latest.hands.hands[1].squeeze=1;latest.motion_history.frames[7].input=latest.hands;}
        if(scenario==5){latest.hands.hands[1].squeeze_active=0;latest.motion_history.frames[7].input=latest.hands;}
        if(scenario==6){latest.motion_history={};}
        if(scenario==7){++latest.mapping.reference_generation;}
        if(scenario==8){++latest.equipment_hold_generation;}
        if(scenario==9){++latest.hands.input_epoch;latest.motion_history.frames[7].input=latest.hands;}
        if(scenario==10){latest.original_interaction_allowed=false;}
        if(scenario==11){latest.motion_history.frames[4].tracking_epoch[1]++;}
        const auto seen=observer.observe(latest,input,rule);
        // Diagnostics preserve admission and the exact producer edge. They
        // retain the prior evidence even when the rejection clears the reader.
        CHECK(diagnosed.observe(latest,diagnosed_input,rule,&diagnostic)==seen);
        CHECK(diagnosed_input.kind==input.kind&&diagnosed_input.edge_serial==input.edge_serial&&
              diagnosed_input.hand_sequence==input.hand_sequence&&diagnosed_input.sync_ns==input.sync_ns);
        CHECK(diagnostic.known&&diagnostic.sequence==baseline.hands.sequence&&diagnostic.sync_ns==baseline.hands.sync_ns);
        if(scenario==6)CHECK(diagnostic.reason==1);
        if(scenario==11)CHECK(diagnostic.reason==3);
        const auto decision=q.model.update(latest,seen==Observe::edge?input:BoloInput{});
        if(scenario>=4&&scenario<=11){
            CHECK(decision.event==E::safety_required);
            CHECK(q.model.pending_request().action==A::preserve_original_on_interrupt&&q.model.inventory().count==3);
            continue;
        }
        CHECK(seen==Observe::edge&&input.kind==BoloInputKind::right_grip_release);
        CHECK(input.hand_sequence==released.hands.sequence&&input.sync_ns==released.hands.sync_ns);
        CHECK(decision.event==E::request_ready);
        const auto launch=q.take(A::launch_original);
        const double speed=scenario==12?0:2;
        CHECK(near(launch.tracked.velocity_mps.x,speed)&&near(launch.tracked.mass_kg,2));
        CHECK(near(launch.tracked.impulse_kg_mps.x,2*speed));
        CHECK(near(launch.tracked.release_position.x,scenario==12?0:.12));
        CHECK(launch.original_input_sync_ns==released.hands.sync_ns);
        const auto receipt=q.receipt(launch);
        CHECK(q.model.acknowledge(receipt)==E::thrown&&q.model.inventory().count==2);
        CHECK(q.model.acknowledge(receipt)==E::rejected&&q.model.inventory().count==2);
        CHECK(observer.observe(latest,input,rule)==Observe::duplicate);
        CHECK(diagnosed.observe(latest,diagnosed_input,rule,&diagnostic)==Observe::duplicate);
        // Automatic original replenishment cannot re-grasp an open hand.
        auto next=q.frame();next.equipment_hold_generation=42;
        CHECK(q.model.adopt_equipment_hold(next).event==E::rejected&&!q.model.take_request());
    }
}
void exact_throwable_hold_release(){
    EquipmentSnapshot game{};game.world=11;game.pawn=12;game.current_weapon=14;
    game.weapon_in_slot[1]=14;game.weapon_in_slot[2]=15;game.can_switch=true;
    game.throwable_count_known=true;game.throwable_count=3;
    WeaponEquipmentModel equipment;
    CHECK(equipment.tap(BodySlot::grenade,game).event==EquipmentEvent::request_unholster);
    const auto hold=equipment.hold_generation();CHECK(hold&&equipment.held());
    CHECK(!equipment.release_throwable(11,12,14,hold+1));
    CHECK(!equipment.release_throwable(11,12,15,hold));
    CHECK(!equipment.release_throwable(11,13,14,hold));
    CHECK(equipment.held()&&equipment.release_throwable(11,12,14,hold));
    CHECK(!equipment.held()&&!equipment.release_throwable(11,12,14,hold));
    CHECK(equipment.tap(BodySlot::grenade,game).event==EquipmentEvent::request_unholster);
    CHECK(equipment.hold_generation()!=hold&&!equipment.release_throwable(11,12,14,hold));
    equipment.reset();game.current_weapon=15;
    CHECK(equipment.tap(BodySlot::weapon_2,game).event==EquipmentEvent::request_unholster);
    CHECK(!equipment.release_throwable(11,12,15,equipment.hold_generation())&&equipment.held());
}
void external_original_debit_after_stopped_preparation(){
    constexpr auto rule=BoloReleaseInput::right_grip_release_after_hold;
    for(unsigned scenario=0;scenario<23;++scenario){
        Fixture q(4,scenario==21?BoloFuseOwner::original_preparation:BoloFuseOwner::original_projectile,
                  {},gears_vr::StereoSource::RuntimeTracked,rule);
        q.equipment_generation=41;
        auto held=q.frame();held.hands.hands[1].squeeze=1;
        held.hands.hands[1].pressed[gears_vr::control_squeeze]=1;
        CHECK(q.model.adopt_equipment_hold(held).event==E::request_ready);
        const auto prepare=q.take(A::prepare_original);
        CHECK(q.model.acknowledge(q.receipt(prepare))==E::prepared);
        // Reproduce15368: the ORIGINAL independent melee consumes one item.
        auto changed=q.frame();changed.inventory={2,3,true};
        CHECK(q.model.update(changed).event==E::safety_required);
        const auto stop=q.take(A::preserve_original_on_interrupt);
        auto r=q.receipt(stop);r.inventory={2,3,true};
        // This is never a successful ordinary unchanged-inventory receipt.
        CHECK(q.model.acknowledge(r)==E::safety_violation);
        CHECK(q.model.inventory().count==4&&q.model.pending_request().token==stop.token);
        switch(scenario){
            case 1:++r.request_token;break;
            case 2:++r.stamp.binding;break;
            case 3:++r.stamp.identity.weapon;break;
            case 4:++r.stamp.identity.world;break;
            case 5:r.stamp.sequence=changed.stamp.sequence;break;
            case 6:r.original_event_serial=0;break;
            case 7:r.accepted=false;break;
            case 8:r.original_effect_applied_once=false;break;
            case 9:r.native_safety_complete=false;break;
            case 10:r.original_fuse_preserved=false;break;
            case 11:r.original_effects_preserved=false;break;
            case 12:++r.original_preparation_serial;break;
            case 13:r.fuse={700,r.stamp.game_tick,true};break;
            case 14:r.original_projectiles_spawned=1;break;
            case 15:r.original_projectile=800;break;
            case 16:r.tracked_initial_pose_and_velocity_applied=true;break;
            case 17:r.inventory.revision=1;break;
            case 18:r.inventory.count=4;break;
            case 19:r.inventory.count=2;break;
            case 20:r.executed_ns=stop.authored_ns-1;break;
            case 22:r.executed_ns=r.stamp.monotonic_ns+1;break;
            default:break;
        }
        CHECK(q.model.retire_stopped_after_external_debit(r)==(scenario==0));
        if(scenario){
            CHECK(q.model.inventory().count==4&&q.model.pending_request().token==stop.token);
            CHECK(q.model.reserved_count()==1);continue;
        }
        CHECK(q.model.inventory().count==3&&q.model.stage()==S::interrupted);
        CHECK(!q.model.reserved_count()&&!q.model.pending_request().token&&!q.model.take_request());
        CHECK(!q.model.retire_stopped_after_external_debit(r));
        auto still_held=q.frame();still_held.equipment_hold_generation=41;
        still_held.hands.hands[1].squeeze=1;
        still_held.hands.hands[1].pressed[gears_vr::control_squeeze]=1;
        CHECK(q.model.adopt_equipment_hold(still_held).event==E::rejected&&!q.model.take_request());
    }
    // A dispatched throw remains ambiguous and cannot borrow this retirement.
    Fixture launch;const auto request=launch.throw_ready();
    auto r=launch.receipt(request);r.original_projectiles_spawned=0;r.original_projectile=0;
    r.tracked_initial_pose_and_velocity_applied=false;r.fuse={};r.native_safety_complete=true;
    CHECK(!launch.model.retire_stopped_after_external_debit(r));
    CHECK(launch.model.pending_request().token==request.token&&launch.model.inventory().count==3);
}
}
void pitch_continuity(){
    using Observe=BoloTriggerObservation;
    constexpr auto rule=BoloReleaseInput::right_grip_release_after_hold;
    enum Case {recorded,left_only,right_only,left_yaw,neutral,expired,roll,cap,rate,identity_change,reference_change,motion_change,units_change,stream_change,tracking_change};
    for(unsigned scenario=0;scenario<15;++scenario){
        Fixture q(3,BoloFuseOwner::original_projectile,{},gears_vr::StereoSource::RuntimeTracked,rule);
        q.equipment_generation=42;BoloTriggerObserver observer;
        gears_hand_continuity::MotionHistory history{};
        const bool initial_left=scenario!=neutral&&scenario!=right_only;
        const bool initial_right=scenario==recorded||scenario==right_only||scenario>=roll;
        const double old_yaw=.229809513;
        auto raw=[&](std::int64_t delta,bool left,bool right,bool down,double pitch,double yaw){
            auto f=q.frame({0,1.5,0},delta);
            f.mapping.game_base.rotation={pitch,yaw,0};
            auto& hand=f.hands.hands[1];hand.trigger_active=0;
            hand.squeeze=down?1.f:0.f;
            hand.pressed[gears_vr::control_squeeze]=1;
            hand.released[gears_vr::control_squeeze]=down?0:1;
            auto& l=f.hands.hands[0];l.stick_active=1;l.stick_x=0;l.stick_y=left?.55f:0.f;
            hand.stick_active=1;hand.stick_x=right?.42f:0.f;hand.stick_y=0;
            if(history.count==gears_hand_continuity::motion_history_capacity){
                for(unsigned i=1;i<history.count;++i)history.frames[i-1]=history.frames[i];
                --history.count;
            }
            auto& row=history.frames[history.count++];row.input=f.hands;
            row.stream_epoch=11;row.tracking_epoch[0]=21;row.tracking_epoch[1]=22;
            f.motion_history=history;return f;
        };
        auto held=raw(1000000,initial_left,initial_right,true,0,old_yaw);BoloInput edge{};
        CHECK(observer.observe(held,edge,rule)==Observe::baseline);
        CHECK(q.model.adopt_equipment_hold(held).event==E::request_ready);
        const auto prepare=q.take(A::prepare_original);
        CHECK(q.model.acknowledge(q.receipt(prepare))==E::prepared);
        auto baseline=raw(1000000,initial_left,initial_right,true,0,old_yaw);
        CHECK(q.model.update(baseline).event==E::observed);
        const auto baseline_ns=baseline.stamp.monotonic_ns;
        const auto old_base=baseline.mapping.game_base;
        // Raw producer advances normally. The consumer observes only the last
        // frame: no old row, sequence, counter or timestamp is ever rewritten.
        if(scenario==expired){
            for(unsigned i=0;i<8;++i)raw(5000000,false,false,true,0,old_yaw);
        }else if(scenario!=rate){
            for(unsigned i=0;i<5;++i)raw(10000000,initial_left,initial_right,true,0,old_yaw);
        }
        const bool final_left=scenario==expired?false:initial_left;
        const bool final_right=scenario==expired?false:initial_right;
        const auto final_delta=scenario==rate?1000000:11223400;
        double pitch=-.00872451684;
        double yaw=(scenario==left_only||scenario==neutral||scenario==expired)?old_yaw:.233165114;
        if(scenario==cap){pitch=.16;yaw=old_yaw+.16;}
        auto changed=raw(final_delta,final_left,final_right,true,pitch,yaw);
        if(scenario==roll)changed.mapping.game_base.rotation.roll=.001;
        if(scenario==identity_change)++changed.stamp.identity.weapon;
        if(scenario==reference_change)++changed.mapping.reference_generation;
        if(scenario==motion_change)++changed.stamp.motion_epoch;
        if(scenario==units_change)++changed.mapping.units_per_metre;
        if(scenario==stream_change||scenario==tracking_change){
            // Deliberate negative cohort fixture, not a positive-history repair.
            for(unsigned i=0;i<changed.motion_history.count;++i){
                if(scenario==stream_change)++changed.motion_history.frames[i].stream_epoch;
                else ++changed.motion_history.frames[i].tracking_epoch[1];
            }
        }
        CHECK(changed.stamp.monotonic_ns==q.ns);
        CHECK(changed.hands.sync_ns==q.ns-2000);
        if(scenario!=expired&&scenario!=rate)CHECK(changed.stamp.monotonic_ns-baseline_ns==61223400);
        const auto input_seen=observer.observe(changed,edge,rule);
        if(scenario<identity_change&&input_seen!=Observe::quiet)std::fprintf(stderr,"pitch observer case=%u seen=%u age=%lld\n",scenario,unsigned(input_seen),static_cast<long long>(changed.stamp.monotonic_ns-changed.hands.sync_ns));
        if(scenario<identity_change)CHECK(input_seen==Observe::quiet);
        const auto result=q.model.update(changed);
        if(scenario>=left_yaw){
            if(result.event!=E::safety_required)std::fprintf(stderr,"pitch case %u event=%u refusal=%u boundary=%u\n",scenario,unsigned(result.event),q.model.last_update_refusal(),q.model.last_boundary_flags());
            CHECK(result.event==E::safety_required&&q.model.inventory().count==3);
            if(scenario<=rate)CHECK(q.model.last_update_refusal()==10&&(q.model.last_boundary_flags()&128));
            CHECK(q.model.pending_request().action!=A::launch_original);
            continue;
        }
        if(result.event!=E::observed)std::fprintf(stderr,"pitch positive %u event=%u refusal=%u boundary=%u\n",scenario,unsigned(result.event),q.model.last_update_refusal(),q.model.last_boundary_flags());
        CHECK(result.event==E::observed);
        const auto rotation=q.model.last_rotation_observation();
        CHECK(rotation.pitch_changed&&rotation.pitch_continuous);
        CHECK(rotation.consumer_gap_ns==61223400);
        CHECK(rotation.rotation_magnitude<.2&&rotation.rotation_magnitude<=8.*double(rotation.consumer_gap_ns)/1e9);
        if(scenario==left_only)CHECK(!rotation.yaw_changed&&!rotation.episode_after);
        else CHECK(rotation.yaw_changed&&rotation.continuous&&rotation.episode_after);
        auto released=raw(1000000,final_left,final_right,false,pitch,yaw);
        CHECK(observer.observe(released,edge,rule)==Observe::edge&&edge.kind==BoloInputKind::right_grip_release);
        const auto decision=q.model.update(released,edge);
        if(decision.event!=E::request_ready)std::fprintf(stderr,"pitch release %u event=%u refusal=%u\n",scenario,unsigned(decision.event),q.model.last_update_refusal());
        CHECK(decision.event==E::request_ready);
        const auto launch=q.take(A::launch_original);
        CHECK(launch.original_input_sync_ns==released.hands.sync_ns);
        GameGrenadeLaunch current{},previous{};
        CHECK(grenade_world_launch(launch.tracked,released.mapping.frozen_reference,released.mapping.game_base,released.mapping.units_per_metre,current,released.mapping.pawn_velocity_world_units_per_second));
        CHECK(grenade_world_launch(launch.tracked,released.mapping.frozen_reference,old_base,released.mapping.units_per_metre,previous,released.mapping.pawn_velocity_world_units_per_second));
        CHECK(near(launch.launch.position.x,current.position.x)&&near(launch.launch.position.y,current.position.y)&&near(launch.launch.position.z,current.position.z));
        CHECK(!near(current.position.x,previous.position.x)||!near(current.position.y,previous.position.y)||!near(current.position.z,previous.position.z));
        CHECK(q.model.acknowledge(q.receipt(launch))==E::thrown&&q.model.inventory().count==2);
        CHECK(!q.model.take_request());
    }
}

void translated_body_origin_is_not_hand_velocity(){
    // The production adapter authenticates the current rig base each sample.
    // Translation is an output origin, not XR motion data. Artificial cover
    // translation must not be fitted as speed or added as a second base speed.
    // The current authenticated Game frame maps the actual raw release pose,
    // including a continuous release observed one input sample later. No past
    // Game origin is fabricated or required by the original interaction contract.
    for(unsigned scenario=0;scenario<8;++scenario){
        Fixture f;f.certificate.release_input=BoloReleaseInput::right_grip_release_after_hold;
        f.certificate.motion.virtual_mass_kg=2.;
        CHECK(f.model.bind(f.certificate,{1,3,true}));
        BoloTriggerObserver observer;gears_hand_continuity::MotionHistory history{};
        const auto raw=[&](unsigned step,bool down){
            auto frame=f.frame({scenario==1?.02*step:0.,1.5,0});frame.equipment_hold_generation=42;
            frame.mapping.game_base.position.x=100.+(scenario==1?.5:5.)*(scenario==7&&step>=7?7:step);
            auto& hand=frame.hands.hands[1];hand.trigger_active=0;hand.squeeze=down?1.f:0.f;
            hand.pressed[gears_vr::control_squeeze]=1;hand.released[gears_vr::control_squeeze]=down?0:1;
            if(history.count==gears_hand_continuity::motion_history_capacity){
                for(unsigned i=1;i<history.count;++i)history.frames[i-1]=history.frames[i];--history.count;}
            auto& row=history.frames[history.count++];row.input=frame.hands;
            row.stream_epoch=11;row.tracking_epoch[0]=21;row.tracking_epoch[1]=22;
            frame.motion_history=history;return frame;
        };
        auto held=raw(0,true);BoloInput edge{};
        CHECK(observer.observe(held,edge,f.certificate.release_input)==BoloTriggerObservation::baseline);
        CHECK(f.model.adopt_equipment_hold(held).event==E::request_ready);
        const auto preparation=f.take(A::prepare_original);
        CHECK(f.model.acknowledge(f.receipt(preparation))==E::prepared);
        for(unsigned step=1;step<=8;++step){
            auto frame=raw(step,step<8);
            if(scenario==2)frame.mapping.frozen_reference.position.x+=.01;
            if(scenario==3)frame.mapping.units_per_metre+=1.;
            if(scenario==4)frame.mapping.game_base.rotation.yaw+=.2;
            if(scenario==5)frame.mapping.game_base.position.x=std::numeric_limits<double>::infinity();
            if((scenario==6||scenario==7)&&step==8)frame=raw(9,false);
            const auto observed=observer.observe(frame,edge,f.certificate.release_input);
            const auto decision=f.model.update(frame,observed==BoloTriggerObservation::edge?edge:BoloInput{});
            if(scenario>=2&&scenario<=5){
                CHECK(decision.event==E::safety_required&&f.model.inventory().count==3);
                CHECK(f.model.pending_request().action==A::preserve_original_on_interrupt);
                break;
            }
            CHECK(decision.event==(step==8?E::request_ready:E::observed));
        }
        if(scenario>=2&&scenario<=5)continue;
        const auto launch=f.take(A::launch_original);
        CHECK(near(launch.tracked.velocity_mps.x,scenario==1?2.:0.));
        CHECK(near(launch.tracked.velocity_mps.y,0.)&&near(launch.tracked.velocity_mps.z,0.));
        CHECK(near(launch.launch.position.x,scenario==1?104.:scenario==7?135.:scenario==6?145.:140.));
        CHECK(near(launch.launch.position.y,scenario==1?36.:20.)&&near(launch.launch.position.z,15.));
        CHECK(near(launch.launch.velocity.x,50.)&&near(launch.launch.velocity.y,scenario==1?180.:-20.)&&near(launch.launch.velocity.z,0.));
        CHECK(f.model.acknowledge(f.receipt(launch))==E::thrown&&f.model.inventory().count==2);
    }
}
void delayed_first_hold_waits_for_fresh_authority(){
    constexpr auto rule=BoloReleaseInput::right_grip_release_after_hold;
    for(unsigned scenario=0;scenario<11;++scenario){
        Fixture f(3,BoloFuseOwner::original_projectile,{},gears_vr::StereoSource::RuntimeTracked,rule);
        f.equipment_generation=42;
        const auto held=[&](){auto row=f.frame();row.hands.hands[1].squeeze=1;
            row.hands.hands[1].pressed[gears_vr::control_squeeze]=1;return row;};
        auto delayed=held();delayed.hands.sync_ns=delayed.stamp.monotonic_ns-64425500;
        if(scenario==2)delayed.hands.focused=0;
        if(scenario==3)delayed.hands.hands[1].grip_flags=0;
        if(scenario==4)++delayed.hands.input_epoch;
        if(scenario==5)delayed.mapping.current_instance_verified=false;
        if(scenario==6)delayed.original_owns_hands=true;
        if(scenario==7)delayed.inventory.count=2;
        if(scenario==8)delayed.hands.sync_ns=delayed.stamp.monotonic_ns-100000001;
        if(scenario==9)delayed.hands.sync_ns=delayed.stamp.monotonic_ns+1;
        if(scenario==10){
            auto fresh=held();CHECK(f.model.adopt_equipment_hold(fresh).event==E::request_ready);
            const auto preparation=f.take(A::prepare_original);
            CHECK(f.model.acknowledge(f.receipt(preparation))==E::prepared);
            delayed=held();delayed.hands.sync_ns=delayed.stamp.monotonic_ns-64425500;
            CHECK(f.model.update(delayed).event==E::safety_required);
            CHECK(f.model.pending_request().action==A::preserve_original_on_interrupt);
            CHECK(f.model.adopt_equipment_hold(held()).event==E::rejected);continue;
        }
        const auto decision=f.model.adopt_equipment_hold(delayed);
        CHECK(f.model.inventory().count==3&&!f.model.take_request()&&!f.model.reserved_count());
        if(scenario>=2){
            if(decision.event!=E::interrupted)std::fprintf(stderr,"deferred hold scenario %u event %u\n",scenario,unsigned(decision.event));
            CHECK(decision.event==E::interrupted&&f.model.stage()==S::interrupted);
            CHECK(f.model.adopt_equipment_hold(held()).event==E::rejected);continue;
        }
        CHECK(decision.event==E::rejected&&f.model.stage()==S::idle);
        // Re-reading the delayed cohort cannot issue or renew preparation.
        CHECK(f.model.adopt_equipment_hold(delayed).event==E::rejected&&!f.model.take_request());
        auto fresh=held();
        if(scenario==1){fresh.hands.hands[1].squeeze=0;fresh.hands.hands[1].released[gears_vr::control_squeeze]=1;}
        CHECK(f.model.adopt_equipment_hold(fresh).event==(scenario==1?E::rejected:E::request_ready));
        if(scenario==1)CHECK(!f.model.take_request()&&!f.model.reserved_count());
        else {const auto preparation=f.take(A::prepare_original);
            CHECK(preparation.original_input_sync_ns==fresh.hands.sync_ns);
            CHECK(f.model.acknowledge(f.receipt(preparation))==E::prepared);}
    }
}

int main(){
    pitch_continuity();
    delayed_first_hold_waits_for_fresh_authority();
    translated_body_origin_is_not_hand_velocity();
    certificate_gates();grab_and_empty();real_motion_and_one_original_commit();
    receipt_validation_and_old_callbacks();interruption_and_native_fuse();
    ambiguous_and_undispatched_operations();bad_motion_never_throws();
    input_provenance_and_verified_press();
    existing_equipment_hold();
    gentle_and_stationary_release();
    explicit_provenance_profile();
    original_ring_and_trigger_release();
    grip_hold_release_owner();
    exact_throwable_hold_release();
    late_release_only_cleanup();
    external_original_debit_after_stopped_preparation();
    std::printf("Bolo grab/fuse/original-once hand-trajectory checks passed (%u checks)\n",checks);
}
