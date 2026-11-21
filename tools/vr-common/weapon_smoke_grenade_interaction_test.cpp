#include "weapon_smoke_grenade_interaction.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <type_traits>

using namespace gears_weapon;
using E=SmokeEvent;
using S=SmokeStage;
using A=SmokeAction;
static_assert(!std::is_copy_constructible<SmokeGrenadeInteraction>::value&&
              !std::is_copy_assignable<SmokeGrenadeInteraction>::value,"A transaction has one owner");
namespace {
unsigned checks{};
void check(bool value,char const* expression,int line){
    ++checks;
    if(!value){std::fprintf(stderr,"line %d: %s\n",line,expression);std::exit(1);}
}
#define CHECK(x) check(bool(x),#x,__LINE__)
bool near(double a,double b,double tolerance=1e-6){return std::abs(a-b)<tolerance;}
constexpr SmokeIdentity identity{11,12,13,14,15,16,17,18,19};

// Synthetic instance certificates only. These timings, IDs and fuse choices
// are not claims about retail Smoke semantics or a headset acceptance result.
SmokeInstanceConfig config(SmokeFuseOwner fuse=SmokeFuseOwner::original_projectile){
    SmokeInstanceConfig c{};c.identity=identity;c.config_serial=21;c.original_projectile_type=22;
    c.original_weapon_type=29;c.original_ammo_type=30;c.original_emitter_type=31;
    c.original_no_ammo_cue=32;c.audio_rule_serial=33;c.motion.gravity_mps2=9.81;
    c.grip_geometry_serial=23;c.release_rule_serial=24;c.fuse_rule_serial=25;
    c.prepare_entry=26;c.launch_entry=27;c.safety_entry=28;
    c.release_input=SmokeReleaseInput::right_trigger_release_after_press;c.fuse_owner=fuse;
    c.current_instance_verified=c.original_smoke_classes_verified=c.grip_geometry_verified=
        c.release_input_verified=c.fuse_rule_verified=c.original_prepare_no_ammo_change_verified=
        c.original_launch_once_verified=c.original_initial_pose_velocity_override_verified=
        c.original_effects_preserved_verified=c.non_throwing_safety_entry_verified=
        c.original_gravity_and_units_verified=c.original_smoke_emitter_path_verified=
        c.original_throw_and_smoke_audio_verified=c.original_no_ammo_cue_verified=true;
    return c;
}
struct Fixture {
    SmokeGrenadeInteraction model{};
    SmokeInstanceConfig certificate{};
    std::uint64_t sequence{},hand_sequence{},edge{},native_event{},preparation{100};
    std::uint64_t epoch{1},reference{1};
    std::int64_t ns{2000000000},game_tick{100};
    bool trigger_down{};
    explicit Fixture(std::uint32_t count=3,SmokeFuseOwner fuse=SmokeFuseOwner::original_projectile,
                     Vector offset={}){
        certificate=config(fuse);certificate.centre_in_right_grip_metres=offset;
        CHECK(model.bind(certificate,{1,count,true}));
    }
    SmokeStamp stamp(std::int64_t delta=1000000){
        ns+=delta;++game_tick;return {certificate.identity,model.binding_token(),++sequence,epoch,ns,game_tick};
    }
    SmokeFrame frame(Vector position={0,1.5,0},std::int64_t delta=10000000){
        SmokeFrame f{};f.stamp=stamp(delta);f.inventory=model.inventory();
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
        f.weapon_type=certificate.original_weapon_type;f.projectile_type=certificate.original_projectile_type;
        f.ammo_type=certificate.original_ammo_type;f.emitter_type=certificate.original_emitter_type;
        f.original_equipped=f.original_interaction_allowed=true;return f;
    }
    SmokeInput input(SmokeInputKind kind,BodySlot slot=BodySlot::none){
        SmokeInput i{};i.kind=kind;i.edge_serial=++edge;i.hand=1;i.slot=slot;
        i.input_epoch=certificate.identity.input_epoch;i.continuous_input=true;return i;
    }
    SmokeInput authenticated_input(SmokeFrame const& f,SmokeInputKind kind,BodySlot slot=BodySlot::none){
        auto i=input(kind,slot);i.hand_sequence=f.hands.sequence;i.sync_ns=f.hands.sync_ns;return i;
    }
    SmokeDecision update(SmokeFrame f,SmokeInput i={}){
        if(i.kind!=SmokeInputKind::none){
            if(!i.hand_sequence)i.hand_sequence=f.hands.sequence;
            if(!i.sync_ns)i.sync_ns=f.hands.sync_ns;
            if(i.kind==SmokeInputKind::right_trigger_press)trigger_down=true;
            if(i.kind==SmokeInputKind::right_trigger_release)trigger_down=false;
            f.hands.hands[1].trigger=trigger_down?1.f:0.f;
        }
        return model.update(f,i);
    }
    void grab(){
        CHECK(update(frame(),input(SmokeInputKind::right_grip_press,BodySlot::grenade)).event==E::grabbed);
        CHECK(model.reserved_count()==1&&model.inventory().count>0);
    }
    SmokeRequest take(A action){
        const auto r=model.take_request();CHECK(r&&r->action==action);
        CHECK(!model.take_request());return *r;
    }
    SmokeOriginalReceipt receipt(SmokeRequest const& request,bool accepted=true,bool safety_projectile=false){
        SmokeOriginalReceipt r{};r.stamp=stamp();r.request_token=request.token;
        r.config_serial=request.config_serial;r.original_entry=request.original_entry;
        r.original_ammo_type=request.original_ammo_type;r.original_emitter_type=request.original_emitter_type;
        r.executed_motion_epoch=request.motion_epoch;r.executed_reference_generation=request.reference_generation;
        r.original_event_serial=++native_event;r.executed_ns=request.authored_ns+1;
        r.accepted=accepted;r.inventory=model.inventory();r.fuse=model.original_fuse();
        r.original_preparation_serial=request.original_preparation_serial;
        if(!accepted)return r;
        r.original_effect_applied_once=r.original_fuse_preserved=r.original_effects_preserved=true;
        r.original_smoke_emitter_path_preserved=r.original_audio_path_preserved=true;
        if(request.action==A::prepare_original){
            r.original_preparation_serial=++preparation;
            if(certificate.fuse_owner==SmokeFuseOwner::original_preparation)
                r.fuse={700,request.authored_game_tick,true};
        }else if(request.action==A::launch_original||safety_projectile){
            r.original_projectiles_spawned=1;r.original_projectile=800+native_event;
            r.original_ammo_consumptions=1;
            r.original_projectile_type=certificate.original_projectile_type;
            ++r.inventory.revision;--r.inventory.count;
            r.tracked_initial_pose_and_velocity_applied=request.action==A::launch_original;
            if(!r.fuse.active)r.fuse={701,request.authored_game_tick,true};
        }
        r.native_safety_complete=request.action==A::preserve_original_on_interrupt;
        r.native_non_throwing_consequence=r.native_safety_complete&&safety_projectile;return r;
    }
    void prepare(){
        CHECK(update(frame(),input(SmokeInputKind::right_trigger_press)).event==E::request_ready);
        const auto request=take(A::prepare_original);
        CHECK(request.before.count==model.inventory().count&&request.expected_count==request.before.count);
        CHECK(model.acknowledge(receipt(request))==E::prepared);
    }
    SmokeRequest throw_ready(Vector velocity={4,2,-1},bool rotating=false){
        grab();prepare();
        for(unsigned i=0;i<12;++i){
            const double t=.01*double(i);
            auto f=frame(rotating?Vector{0,1.5,0}:sum(Vector{0,1.5,0},scaled(velocity,t)));
            // Changing HMD direction cannot determine release direction.
            f.stereo.head.orientation={0,std::sin(t),0,std::cos(t)};
            if(rotating){const auto a=.1*double(i);f.hands.hands[1].grip.orientation={0,0,std::sin(a/2),std::cos(a/2)};}
            const auto command=i==11?input(SmokeInputKind::right_trigger_release):SmokeInput{};
            const auto result=update(f,command);
            CHECK(result.event==(i==11?E::request_ready:E::observed));
        }
        return take(A::launch_original);
    }
};

void certificate_gates(){
    SmokeGrenadeInteraction m;
    CHECK(!m.bind({},{}));
    auto c=config();CHECK(c.valid());
    auto unknown=c;unknown.release_input=SmokeReleaseInput::unknown;CHECK(!m.bind(unknown,{1,2,true}));
    unknown=c;unknown.fuse_owner=SmokeFuseOwner::unknown;CHECK(!m.bind(unknown,{1,2,true}));
    bool SmokeInstanceConfig::* const gates[]={
        &SmokeInstanceConfig::current_instance_verified,&SmokeInstanceConfig::original_smoke_classes_verified,
        &SmokeInstanceConfig::grip_geometry_verified,&SmokeInstanceConfig::release_input_verified,
        &SmokeInstanceConfig::fuse_rule_verified,&SmokeInstanceConfig::original_prepare_no_ammo_change_verified,
        &SmokeInstanceConfig::original_launch_once_verified,&SmokeInstanceConfig::original_initial_pose_velocity_override_verified,
        &SmokeInstanceConfig::original_effects_preserved_verified,&SmokeInstanceConfig::non_throwing_safety_entry_verified,
        &SmokeInstanceConfig::original_gravity_and_units_verified,&SmokeInstanceConfig::original_smoke_emitter_path_verified,
        &SmokeInstanceConfig::original_throw_and_smoke_audio_verified,&SmokeInstanceConfig::original_no_ammo_cue_verified};
    for(auto gate:gates){auto broken=c;broken.*gate=false;CHECK(!m.bind(broken,{1,2,true}));}
    std::uint64_t SmokeInstanceConfig::* const resolved[]={
        &SmokeInstanceConfig::config_serial,&SmokeInstanceConfig::original_weapon_type,
        &SmokeInstanceConfig::original_projectile_type,&SmokeInstanceConfig::original_ammo_type,
        &SmokeInstanceConfig::original_emitter_type,&SmokeInstanceConfig::original_no_ammo_cue,
        &SmokeInstanceConfig::audio_rule_serial,&SmokeInstanceConfig::grip_geometry_serial,
        &SmokeInstanceConfig::release_rule_serial,&SmokeInstanceConfig::fuse_rule_serial,
        &SmokeInstanceConfig::prepare_entry,&SmokeInstanceConfig::launch_entry,&SmokeInstanceConfig::safety_entry};
    for(auto field:resolved){auto broken=c;broken.*field=0;CHECK(!m.bind(broken,{1,2,true}));}
    auto no_original_gravity=c;no_original_gravity.motion.gravity_mps2=0;
    CHECK(!m.bind(no_original_gravity,{1,2,true}));
    CHECK(!m.bind(c,{1,2,false}));CHECK(!m.bind(c,{0,2,true}));
    c.centre_in_right_grip_metres={1,0,0};CHECK(!m.bind(c,{1,2,true}));
    c=config();c.motion.maximum_speed_mps=std::numeric_limits<double>::quiet_NaN();
    CHECK(!m.bind(c,{1,2,true}));
}
void grab_and_empty(){
    Fixture empty(0);
    const auto press=empty.input(SmokeInputKind::right_grip_press,BodySlot::grenade);
    auto result=empty.update(empty.frame(),press);
    CHECK(result.event==E::no_ammo&&result.request_original_no_ammo_sound);
    CHECK(result.audio_token&&result.original_cue==empty.certificate.original_no_ammo_cue);
    CHECK(result.audio_for(identity,empty.model.binding_token()));
    CHECK(!result.audio_for(identity,empty.model.binding_token()+1));
    auto other=identity;++other.weapon;CHECK(!result.audio_for(other,empty.model.binding_token()));
    const auto first_audio=result.audio_token;
    CHECK(empty.model.reserved_count()==0&&empty.model.inventory().count==0&&!empty.model.take_request());
    result=empty.update(empty.frame(),press);CHECK(!result.request_original_no_ammo_sound);
    result=empty.update(empty.frame());CHECK(!result.request_original_no_ammo_sound);
    result=empty.update(empty.frame(),empty.input(SmokeInputKind::right_grip_press,BodySlot::grenade));
    CHECK(result.event==E::no_ammo&&result.request_original_no_ammo_sound);
    CHECK(result.audio_token!=first_audio&&empty.model.available_count()==0);
    Fixture f;
    auto wrong=f.input(SmokeInputKind::right_grip_press,BodySlot::grenade);wrong.hand=0;
    CHECK(f.update(f.frame(),wrong).event==E::rejected);
    CHECK(f.update(f.frame(),f.input(SmokeInputKind::right_grip_press,BodySlot::pistol)).event==E::rejected);
    wrong=f.input(SmokeInputKind::right_grip_press,BodySlot::grenade);wrong.continuous_input=false;
    CHECK(f.update(f.frame(),wrong).event==E::rejected&&f.model.reserved_count()==0);
    f.grab();CHECK(f.model.suppress_original_trajectory());
    CHECK(f.model.available_count()+f.model.reserved_count()==f.model.inventory().count);
    CHECK(f.update(f.frame()).event==E::observed); // Grip release does not throw/stow.
    CHECK(!f.model.take_request()&&f.model.reserved_count()==1);
    CHECK(f.update(f.frame(),f.input(SmokeInputKind::right_grip_press,BodySlot::grenade)).event==E::stowed);
    CHECK(f.model.inventory().count==3&&!f.model.suppress_original_trajectory());
    auto bad=f.frame();bad.original_equipped=false;
    CHECK(f.update(bad,f.input(SmokeInputKind::right_grip_press,BodySlot::grenade)).event==E::interrupted);
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
    CHECK(f.update(f.frame(),f.input(SmokeInputKind::right_trigger_release)).event==E::observed);
    auto receipt=f.receipt(request);CHECK(f.model.acknowledge(receipt)==E::thrown);
    CHECK(f.model.inventory().count==2&&f.model.reserved_count()==0&&f.model.stage()==S::idle);
    CHECK(f.model.acknowledge(receipt)==E::rejected&&f.model.inventory().count==2);
    CHECK(f.update(f.frame(),f.input(SmokeInputKind::right_trigger_release)).event==E::rejected);
    for(auto velocity:{Vector{0,5,-2},Vector{0,0,-5},Vector{0,-2,-4}}){
        Fixture direction;const auto launch=direction.throw_ready(velocity);
        CHECK(near(launch.tracked.velocity_mps.x,velocity.x)&&near(launch.tracked.velocity_mps.y,velocity.y)&&
              near(launch.tracked.velocity_mps.z,velocity.z));
    }
    Fixture wrist(3,SmokeFuseOwner::original_projectile,{.2,0,0});
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
    Fixture f(3,SmokeFuseOwner::original_preparation);f.grab();f.prepare();
    const auto old_fuse=f.model.original_fuse();CHECK(old_fuse.active);
    for(unsigned i=0;i<8;++i)CHECK(f.update(f.frame({.04*double(i),1.5,0})).event==E::observed);
    auto lost=f.frame({.4,1.5,0});lost.hands.hands[1].grip_flags=0;
    CHECK(f.update(lost,f.input(SmokeInputKind::right_trigger_release)).event==E::safety_required);
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
void ambiguous_and_undispatched_operations(){
    Fixture undelivered;
    undelivered.grab();
    CHECK(undelivered.update(undelivered.frame(),undelivered.input(SmokeInputKind::right_trigger_press)).event==E::request_ready);
    CHECK(undelivered.model.cancel()==E::interrupted&&!undelivered.model.take_request());
    CHECK(undelivered.model.inventory().count==3&&undelivered.model.reserved_count()==0);

    Fixture pending(3,SmokeFuseOwner::original_preparation);pending.grab();
    CHECK(pending.update(pending.frame(),pending.input(SmokeInputKind::right_trigger_press)).event==E::request_ready);
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
        const auto command=i==7?revoked.input(SmokeInputKind::right_trigger_release):SmokeInput{};
        CHECK(revoked.update(revoked.frame({.04*double(i),1.5,0}),command).event==
              (i==7?E::request_ready:E::observed));
    }
    CHECK(revoked.model.cancel()==E::safety_required);
    const auto only=revoked.take(A::preserve_original_on_interrupt);
    CHECK(only.tracked.predicted_time==0&&revoked.model.inventory().count==3);

    Fixture early;early.grab();
    CHECK(early.update(early.frame(),early.input(SmokeInputKind::right_trigger_press)).event==E::request_ready);
    const auto unresolved=early.take(A::prepare_original);
    const auto release=early.input(SmokeInputKind::right_trigger_release);
    CHECK(early.update(early.frame(),release).event==E::interrupted);
    CHECK(early.model.acknowledge(early.receipt(unresolved))==E::safety_required);
    CHECK(early.model.pending_request().action==A::preserve_original_on_interrupt);

    Fixture boundary;boundary.grab();
    const auto press=boundary.input(SmokeInputKind::right_trigger_press);
    ++boundary.epoch;
    CHECK(boundary.update(boundary.frame(),press).event==E::motion_reset);
    CHECK(boundary.update(boundary.frame(),press).event==E::rejected);
    CHECK(boundary.model.stage()==S::held&&!boundary.model.take_request());
    auto recovery=boundary.input(SmokeInputKind::right_trigger_press);recovery.continuous_input=false;
    CHECK(boundary.update(boundary.frame(),recovery).event==E::rejected);
    recovery.continuous_input=true;
    CHECK(boundary.update(boundary.frame(),recovery).event==E::rejected&&!boundary.model.take_request());

    Fixture refused;refused.grab();
    CHECK(refused.update(refused.frame(),refused.input(SmokeInputKind::right_trigger_press)).event==E::request_ready);
    const auto no_prepare=refused.take(A::prepare_original);
    CHECK(refused.model.acknowledge(refused.receipt(no_prepare,false))==E::interrupted);
    CHECK(refused.model.reserved_count()==0&&refused.model.inventory().count==3&&!refused.model.take_request());
}
void bad_motion_never_throws(){
    // These are separate interrupted preparations: no old trajectory survives.
    for(unsigned mode=0;mode<14;++mode){
        Fixture f;f.grab();f.prepare();
        for(unsigned i=0;i<8;++i)CHECK(f.update(f.frame({.04*double(i),1.5,0})).event==E::observed);
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
        const auto result=f.update(frame,f.input(SmokeInputKind::right_trigger_release));
        CHECK(result.event==E::safety_required);
        CHECK(f.model.pending_request().action==A::preserve_original_on_interrupt&&f.model.inventory().count==3);
        const auto request=f.take(A::preserve_original_on_interrupt);
        CHECK(f.model.acknowledge(f.receipt(request))==E::safety_completed);
        CHECK(f.update(f.frame(),f.input(SmokeInputKind::right_trigger_release)).event==E::rejected);
        CHECK(!f.model.take_request()&&f.model.inventory().count==3);
    }
    // Shared tracker permits deliberate stationary release.
    for(unsigned mode:{0u,2u,3u}){
        Fixture f;f.grab();f.prepare();
        const unsigned count=mode==0?2u:8u;
        for(unsigned i=0;i<count;++i){
            auto point=Vector{.04*double(i),1.5,0};
            if(mode==2&&i==count-1)point.x=4; // Tracking teleport on release.
            auto frame=f.frame(point,mode==3&&i==count-1?70000000:10000000);
            const auto result=f.update(frame,i==count-1?f.input(SmokeInputKind::right_trigger_release):SmokeInput{});
            if(i==count-1)CHECK(result.event==E::safety_required);
            else CHECK(result.event==E::observed);
        }
        CHECK(f.model.pending_request().action==A::preserve_original_on_interrupt&&f.model.inventory().count==3);
    }
    Fixture broken_inventory;broken_inventory.grab();broken_inventory.prepare();
    auto unexpected=broken_inventory.frame();--unexpected.inventory.count;++unexpected.inventory.revision;
    CHECK(broken_inventory.update(unexpected).event==E::safety_required);
    CHECK(broken_inventory.model.inventory().count==3); // No invented reconciliation.
}
void smoke_classes_and_native_path_receipts(){
    // A current grenade-pocket identity does not authenticate its dynamic
    // smoke weapon/projectile/ammo/emitter classes. Any mismatch fails closed.
    for(unsigned mode=0;mode<4;++mode){
        Fixture f;f.grab();f.prepare();auto frame=f.frame();
        if(mode==0)++frame.weapon_type;
        if(mode==1)++frame.projectile_type;
        if(mode==2)++frame.ammo_type;
        if(mode==3)++frame.emitter_type;
        CHECK(f.update(frame).event==E::safety_required);
        CHECK(f.model.inventory().count==3&&f.model.pending_request().action==A::preserve_original_on_interrupt);
        const auto safety=f.take(A::preserve_original_on_interrupt);
        CHECK(!safety.tracked.predicted_time&&f.model.acknowledge(f.receipt(safety))==E::safety_completed);
    }
    Fixture f;const auto request=f.throw_ready();const auto valid=f.receipt(request);
    CHECK(request.original_ammo_type==f.certificate.original_ammo_type&&
          request.original_emitter_type==f.certificate.original_emitter_type);
    for(unsigned mode=0;mode<12;++mode){
        auto bad=valid;
        switch(mode){
            case 0:bad.original_ammo_consumptions=0;break;
            case 1:bad.original_ammo_consumptions=2;break;
            case 2:bad.original_smoke_emitter_path_preserved=false;break;
            case 3:bad.original_audio_path_preserved=false;break;
            case 4:++bad.original_ammo_type;break;
            case 5:++bad.original_emitter_type;break;
            case 6:++bad.config_serial;break;
            case 7:++bad.original_entry;break;
            case 8:++bad.executed_motion_epoch;break;
            case 9:++bad.executed_reference_generation;break;
            case 10:bad.original_projectiles_spawned=0;break;
            case 11:bad.executed_ns=bad.stamp.monotonic_ns+1;break;
        }
        CHECK(f.model.acknowledge(bad)==E::safety_violation);
        CHECK(f.model.inventory().count==3&&f.model.reserved_count()==1&&!f.model.take_request());
    }
    CHECK(f.model.acknowledge(valid)==E::thrown);
    CHECK(f.model.inventory().count==2&&f.model.available_count()==2&&f.model.reserved_count()==0);
    // A fuse may start on a later original event. Do not invent fuse-on-spawn.
    Fixture later_fuse;const auto later=later_fuse.throw_ready();auto receipt=later_fuse.receipt(later);
    receipt.fuse={};CHECK(later_fuse.model.acknowledge(receipt)==E::thrown);
}
void fresh_release_is_required(){
    for(unsigned mode=0;mode<10;++mode){
        Fixture f;f.grab();f.prepare();
        for(unsigned i=0;i<8;++i)CHECK(f.update(f.frame({.04*double(i),1.5,0})).event==E::observed);
        auto frame=f.frame({.36,1.5,0});
        auto release=f.authenticated_input(frame,SmokeInputKind::right_trigger_release);
        frame.hands.hands[1].trigger=0;
        switch(mode){
            case 0:--release.hand_sequence;break;
            case 1:++release.input_epoch;break;
            case 2:--release.sync_ns;break;
            case 3:release.continuous_input=false;break;
            case 4:release.edge_serial=1;break; // Old pocket edge cannot release.
            case 5:frame.hands.hands[1].trigger_active=0;break;
            case 6:frame.hands.hands[1].trigger=1;break; // No physical release.
            case 7:frame.hands.sync_ns-=61000000;release.sync_ns=frame.hands.sync_ns;break;
            case 8:frame.hands.predicted_time-=10000000;frame.stereo.predicted_time=frame.hands.predicted_time;break;
            case 9:++release.hand_sequence;break;
        }
        CHECK(f.model.update(frame,release).event==E::safety_required);
        CHECK(f.model.pending_request().action==A::preserve_original_on_interrupt&&f.model.inventory().count==3);
        const auto safety=f.take(A::preserve_original_on_interrupt);
        CHECK(!safety.tracked.predicted_time&&f.model.acknowledge(f.receipt(safety))==E::safety_completed);
    }
    Fixture wrong_hand;wrong_hand.grab();wrong_hand.prepare();auto frame=wrong_hand.frame();
    auto left=wrong_hand.authenticated_input(frame,SmokeInputKind::right_trigger_release);left.hand=0;
    CHECK(wrong_hand.model.update(frame,left).event==E::rejected&&!wrong_hand.model.take_request());
    CHECK(wrong_hand.model.stage()==S::prepared&&wrong_hand.model.inventory().count==3);
}
void no_motion_or_loss_automatically_throws(){
    Fixture held;held.grab();held.prepare();
    // Observation duration is synthetic and is not an asserted original fuse.
    for(unsigned i=0;i<180;++i){
        CHECK(held.update(held.frame({.04*double(i),1.5,0})).event==E::observed);
        CHECK(!held.model.take_request()&&held.model.inventory().count==3&&held.model.reserved_count()==1);
    }
    auto released_without_edge=held.frame({7.24,1.5,0});released_without_edge.hands.hands[1].trigger=0;
    CHECK(held.model.update(released_without_edge).event==E::observed&&!held.model.take_request());
    CHECK(held.model.cancel()==E::safety_required);
    const auto safety=held.take(A::preserve_original_on_interrupt);
    CHECK(held.model.acknowledge(held.receipt(safety))==E::safety_completed&&held.model.inventory().count==3);
    for(unsigned mode=0;mode<5;++mode){
        Fixture f;f.grab();f.prepare();auto loss=f.frame();
        if(mode==0)loss.hands.hands[1].grip_flags=0;
        if(mode==1)loss.hands.hands[1].trigger_active=0;
        if(mode==2)loss.hands.hands[1].squeeze_active=0;
        if(mode==3)loss.hands.focused=loss.stereo.focused=0;
        if(mode==4)loss.original_owns_hands=true;
        if(mode==1)loss.hands.hands[1].trigger=0;
        CHECK(f.model.update(loss).event==E::safety_required);
        CHECK(f.model.pending_request().action==A::preserve_original_on_interrupt&&
              f.model.pending_request().tracked.predicted_time==0);
    }
}
void identity_boundaries_and_mapping(){
    std::uint64_t SmokeIdentity::* const identities[]={
        &SmokeIdentity::world,&SmokeIdentity::pawn,&SmokeIdentity::controller,&SmokeIdentity::weapon,
        &SmokeIdentity::world_generation,&SmokeIdentity::xr_session,&SmokeIdentity::space_epoch,
        &SmokeIdentity::input_epoch,&SmokeIdentity::profile_generation};
    for(auto field:identities){
        Fixture f;f.grab();f.prepare();auto frame=f.frame();++(frame.stamp.identity.*field);
        CHECK(f.update(frame).event==E::safety_required);
        CHECK(f.model.inventory().count==3&&f.model.pending_request().action==A::preserve_original_on_interrupt);
    }
    // Translation is independently covered below; rotation still cancels.
    for(unsigned mode=0;mode<3;++mode){
        Fixture f;f.grab();f.prepare();auto frame=f.frame();
        if(mode==0)frame.mapping.game_base.rotation.yaw+=1;
        if(mode==1)frame.mapping.game_base.rotation.pitch+=1;
        if(mode==2)frame.mapping.game_base.rotation.roll+=1;
        CHECK(f.update(frame).event==E::safety_required&&!f.model.pending_request().tracked.predicted_time);
    }
    Fixture stale;stale.grab();stale.prepare();auto old=stale.frame();--old.stamp.binding;
    CHECK(stale.update(old).event==E::rejected&&stale.model.stage()==S::prepared);
    CHECK(!stale.model.take_request()&&stale.model.inventory().count==3);
}
void native_fuse_conservation_and_no_trajectory_on_interrupt(){
    Fixture f(3,SmokeFuseOwner::original_preparation);f.grab();f.prepare();
    const auto fuse=f.model.original_fuse();CHECK(fuse.active);
    CHECK(f.model.cancel()==E::safety_required);const auto safety=f.take(A::preserve_original_on_interrupt);
    CHECK(safety.fuse==fuse&&safety.expected_count==2&&!safety.tracked.predicted_time);
    auto receipt=f.receipt(safety,true,true);auto bad=receipt;bad.native_non_throwing_consequence=false;
    CHECK(f.model.acknowledge(bad)==E::safety_violation&&f.model.inventory().count==3);
    bad=receipt;bad.tracked_initial_pose_and_velocity_applied=true;
    CHECK(f.model.acknowledge(bad)==E::safety_violation);
    CHECK(f.model.acknowledge(receipt)==E::safety_completed&&f.model.inventory().count==2);
    CHECK(f.model.reserved_count()==0&&f.model.available_count()==2);

    Fixture expired;expired.grab();
    CHECK(expired.update(expired.frame(),expired.input(SmokeInputKind::right_trigger_press)).event==E::request_ready);
    const auto pending=expired.take(A::prepare_original);
    auto frame=expired.frame({},110000000);
    CHECK(expired.update(frame).event==E::interrupted&&!expired.model.take_request());
    CHECK(!expired.model.bind(expired.certificate,{5,3,true}));
    CHECK(expired.model.acknowledge(expired.receipt(pending))==E::safety_required);
    CHECK(expired.model.pending_request().action==A::preserve_original_on_interrupt);
}
}
void continuous_grip_owner_flow(){
    auto setup=[](Fixture& f){
        f.certificate.release_input=SmokeReleaseInput::right_grip_release_after_hold;
        CHECK(f.model.bind(f.certificate,{1,3,true}));
    };
    auto held=[](Fixture& f,double z=0.){
        auto frame=f.frame({0,1.5,z});frame.equipment_hold_generation=42;
        auto& h=frame.hands.hands[1];h.squeeze=1.;h.pressed[gears_vr::control_squeeze]=1;
        h.released[gears_vr::control_squeeze]=0;h.trigger_active=0;return frame;
    };
    Fixture f;setup(f);
    const auto first=f.model.adopt_equipment_hold(held(f));
    if(first.event!=E::request_ready)std::fprintf(stderr,"Smoke Grip adoption event=%u stage=%u\n",unsigned(first.event),unsigned(f.model.stage()));
    CHECK(first.event==E::request_ready);
    auto prepare=f.take(A::prepare_original);
    CHECK(f.model.acknowledge(f.receipt(prepare))==E::prepared);
    for(unsigned i=0;i<6;++i)CHECK(f.model.update(held(f,double(i)*.01)).event==E::observed);
    auto release=held(f,.07);release.hands.hands[1].squeeze=0.;
    release.hands.hands[1].released[gears_vr::control_squeeze]=1;
    CHECK(f.model.update(release,f.authenticated_input(release,SmokeInputKind::right_grip_release)).event==E::request_ready);
    auto launch=f.take(A::launch_original);CHECK(f.model.acknowledge(f.receipt(launch))==E::thrown);
    CHECK(f.model.inventory().count==2);
    CHECK(f.model.adopt_equipment_hold(held(f)).event!=E::request_ready);
    auto fresh=held(f);fresh.equipment_hold_generation=43;
    fresh.hands.hands[1].pressed[gears_vr::control_squeeze]=2;
    fresh.hands.hands[1].released[gears_vr::control_squeeze]=1;
    CHECK(f.model.adopt_equipment_hold(fresh).event==E::request_ready);

    Fixture missing_edge;setup(missing_edge);
    CHECK(missing_edge.model.adopt_equipment_hold(held(missing_edge)).event==E::request_ready);
    auto p=missing_edge.take(A::prepare_original);
    CHECK(missing_edge.model.acknowledge(missing_edge.receipt(p))==E::prepared);
    auto lost=held(missing_edge);lost.hands.hands[1].squeeze=0.;
    CHECK(missing_edge.model.update(lost).event==E::safety_required);
    auto safety=missing_edge.take(A::preserve_original_on_interrupt);
    CHECK(safety.expected_count==3&&missing_edge.model.inventory().count==3);
    Fixture empty;setup(empty);CHECK(empty.model.bind(empty.certificate,{1,0,true}));
    const auto cue=empty.model.adopt_equipment_hold(held(empty));CHECK(cue.event==E::no_ammo);
    CHECK(empty.model.adopt_equipment_hold(held(empty)).event!=E::no_ammo);
    auto another=held(empty);another.equipment_hold_generation=43;
    another.hands.hands[1].pressed[gears_vr::control_squeeze]=2;
    another.hands.hands[1].released[gears_vr::control_squeeze]=1;
    const auto next_cue=empty.model.adopt_equipment_hold(another);
    CHECK(next_cue.event==E::no_ammo&&next_cue.audio_token>cue.audio_token);
}
void raw_grip_history_and_source_boundary(){
    for(unsigned scenario=0;scenario<5;++scenario){
        Fixture f;f.certificate.release_input=SmokeReleaseInput::right_grip_release_after_hold;
        if(scenario==1)f.certificate.input_source=gears_vr::StereoSource::MetaOperatorSimulated;
        CHECK(f.model.bind(f.certificate,{1,3,true}));
        SmokeTriggerObserver observer;gears_hand_continuity::MotionHistory history{};
        const auto raw=[&](double x,bool down){
            auto frame=f.frame({x,1.5,0});frame.equipment_hold_generation=42;
            auto& hand=frame.hands.hands[1];hand.trigger_active=0;hand.squeeze=down?1.f:0.f;
            hand.pressed[gears_vr::control_squeeze]=1;hand.released[gears_vr::control_squeeze]=down?0:1;
            if(scenario==1||scenario==2)frame.hands.source=frame.stereo.source=gears_vr::StereoSource::MetaOperatorSimulated;
            if(history.count==gears_hand_continuity::motion_history_capacity){
                for(unsigned i=1;i<history.count;++i)history.frames[i-1]=history.frames[i];
                --history.count;
            }
            auto& row=history.frames[history.count++];row.input=frame.hands;
            row.stream_epoch=11;row.tracking_epoch[0]=21;row.tracking_epoch[1]=22;
            frame.motion_history=history;return frame;
        };
        auto baseline=raw(0,true);SmokeInput input{};
        CHECK(observer.observe(baseline,input,f.certificate.release_input)==SmokeTriggerObservation::baseline);
        if(scenario==2){
            CHECK(f.model.adopt_equipment_hold(baseline).event==E::interrupted);
            CHECK(!f.model.take_request());continue;
        }
        CHECK(f.model.adopt_equipment_hold(baseline).event==E::request_ready);
        auto preparation=f.take(A::prepare_original);
        CHECK(f.model.acknowledge(f.receipt(preparation))==E::prepared);
        SmokeFrame latest{},release{};
        for(unsigned i=0;i<8;++i){latest=raw(.02*double(i),i<6);if(i==6)release=latest;}
        const auto seen=observer.observe(latest,input,f.certificate.release_input);
        CHECK(seen==SmokeTriggerObservation::edge);
        CHECK(input.hand_sequence==release.hands.sequence&&input.sync_ns==release.hands.sync_ns);
        if(scenario==3)latest.motion_history={};
        if(scenario==4)++input.sync_ns;
        const auto decision=f.model.update(latest,input);
        if(scenario>=3){
            CHECK(decision.event==E::safety_required);
            CHECK(f.model.pending_request().action==A::preserve_original_on_interrupt);continue;
        }
        CHECK(decision.event==E::request_ready);const auto launch=f.take(A::launch_original);
        CHECK(near(launch.tracked.velocity_mps.x,2.));
        CHECK(launch.valid_until_ns<=release.hands.sync_ns+f.certificate.request_lease_ns);
        CHECK(f.model.acknowledge(f.receipt(launch))==E::thrown);
        CHECK(f.model.inventory().count==2);
    }
}
void stopped_preparation_after_external_debit(){
    for(unsigned scenario=0;scenario<32;++scenario){
        Fixture f(4);f.certificate.release_input=SmokeReleaseInput::right_grip_release_after_hold;
        CHECK(f.model.bind(f.certificate,{1,4,true}));
        auto held=f.frame();held.equipment_hold_generation=41;
        held.hands.hands[1].squeeze=1;held.hands.hands[1].pressed[gears_vr::control_squeeze]=1;
        CHECK(f.model.adopt_equipment_hold(held).event==E::request_ready);
        const auto prepare=f.take(A::prepare_original);
        CHECK(f.model.acknowledge(f.receipt(prepare))==E::prepared);
        auto changed=f.frame();changed.inventory={2,3,true};
        CHECK(f.model.update(changed).event==E::safety_required);
        const auto stop=f.take(A::preserve_original_on_interrupt);
        auto r=f.receipt(stop);r.inventory={2,3,true};
        CHECK(f.model.acknowledge(r)==E::safety_violation);
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
            case 21:r.executed_ns=r.stamp.monotonic_ns+1;break;
            case 22:++r.config_serial;break;
            case 23:++r.original_entry;break;
            case 24:++r.original_ammo_type;break;
            case 25:++r.original_emitter_type;break;
            case 26:r.original_smoke_emitter_path_preserved=false;break;
            case 27:r.original_audio_path_preserved=false;break;
            case 28:r.original_ammo_consumptions=1;break;
            case 29:r.native_non_throwing_consequence=true;break;
            case 30:r.inventory.count_known=false;break;
            case 31:r.original_preparation_serial=0;break;
        }
        CHECK(f.model.retire_stopped_after_external_debit(r)==(scenario==0));
        if(scenario){CHECK(f.model.inventory().count==4&&f.model.pending_request().token==stop.token);continue;}
        CHECK(f.model.inventory().count==3&&f.model.stage()==S::interrupted);
        CHECK(!f.model.pending_request().token&&!f.model.reserved_count());
        CHECK(!f.model.retire_stopped_after_external_debit(r));
    }
    Fixture launch;const auto request=launch.throw_ready();auto r=launch.receipt(request);
    r.original_projectiles_spawned=0;r.original_projectile=0;r.original_ammo_consumptions=0;
    r.tracked_initial_pose_and_velocity_applied=false;r.fuse={};r.native_safety_complete=true;
    CHECK(!launch.model.retire_stopped_after_external_debit(r));
    CHECK(launch.model.pending_request().token==request.token&&launch.model.inventory().count==3);
}
void bounded_source_gap_rebuilds_only_motion_window(){
    // Synthetic OFFLINE producer rows. A source-continuous 80ms interval is
    // not usable velocity data. Current tracked ownership remains fresh, and
    // a throw must rebuild >=55ms of actual samples with <=60ms intervals.
    for(unsigned scenario=0;scenario<7;++scenario){
        Fixture f;f.certificate.release_input=SmokeReleaseInput::right_grip_release_after_hold;
        CHECK(f.model.bind(f.certificate,{1,3,true}));
        SmokeTriggerObserver observer;gears_hand_continuity::MotionHistory history{};
        std::int64_t xr=9000000000LL;
        const auto raw=[&](double x,bool down,std::int64_t dt=10000000){
            auto frame=f.frame({x,1.5,0},dt);frame.equipment_hold_generation=42;
            auto& hand=frame.hands.hands[1];hand.squeeze=down?1.f:0.f;
            hand.pressed[gears_vr::control_squeeze]=1;hand.released[gears_vr::control_squeeze]=down?0:1;
            if(scenario==6){xr+=10000000;frame.hands.predicted_time=frame.stereo.predicted_time=xr;}
            if(history.count==gears_hand_continuity::motion_history_capacity){
                for(unsigned i=1;i<history.count;++i)history.frames[i-1]=history.frames[i];--history.count;}
            auto& row=history.frames[history.count++];row.input=frame.hands;
            row.stream_epoch=11;row.tracking_epoch[0]=21;row.tracking_epoch[1]=22;
            frame.motion_history=history;return frame;
        };
        auto base=raw(0,true);SmokeInput input{};
        CHECK(observer.observe(base,input,f.certificate.release_input)==SmokeTriggerObservation::baseline);
        CHECK(f.model.adopt_equipment_hold(base).event==E::request_ready);
        auto prep=f.take(A::prepare_original);CHECK(f.model.acknowledge(f.receipt(prep))==E::prepared);
        for(unsigned i=0;i<6;++i){auto row=raw(0,true);observer.observe(row,input,f.certificate.release_input);
            CHECK(f.model.update(row).event==E::observed);}
        auto gap=raw(.8,scenario!=1,scenario==3?120000000:80000000);
        if(scenario==4){++gap.motion_history.frames[gap.motion_history.count-1].input.sequence;++gap.hands.sequence;}
        if(scenario==5)++gap.motion_history.frames[gap.motion_history.count-1].tracking_epoch[1];
        const auto seen=observer.observe(gap,input,f.certificate.release_input);
        const auto decision=f.model.update(gap,seen==SmokeTriggerObservation::edge?input:SmokeInput{});
        if(scenario>=3&&scenario<=5){CHECK(decision.event==E::safety_required);CHECK(f.model.inventory().count==3);continue;}
        if(scenario==1){CHECK(decision.event==E::safety_required);CHECK(f.model.inventory().count==3);continue;}
        CHECK(decision.event==E::observed&&f.model.stage()==S::prepared&&!f.model.pending_request().token);
        const unsigned rows=scenario==2?2:6;
        SmokeFrame release{};
        for(unsigned i=1;i<=rows;++i){auto row=raw(.8+.02*i,i<rows);const auto status=observer.observe(row,input,f.certificate.release_input);
            const auto d=f.model.update(row,status==SmokeTriggerObservation::edge?input:SmokeInput{});
            if(i<rows)CHECK(d.event==E::observed);else{release=row;CHECK(d.event==(scenario==2?E::safety_required:E::request_ready));}}
        if(scenario==2){CHECK(f.model.inventory().count==3);continue;}
        const auto launch=f.take(A::launch_original);
        CHECK(near(launch.tracked.velocity_mps.x,2.));CHECK(near(launch.tracked.release_position.x,.92));
        CHECK(launch.valid_until_ns<=release.hands.sync_ns+f.certificate.request_lease_ns);
        CHECK(f.model.acknowledge(f.receipt(launch))==E::thrown&&f.model.inventory().count==2);
    }
}
void finite_two_grasp_cycles_then_empty(){
    // OFFLINE only: actual state model + original producer-ring observer.
    // Original Engine receipts below are synthetic fixture evidence, never
    // proof of an actual auto-switch, empty-slot sound or particle lifecycle.
    Fixture f(2);f.certificate.release_input=SmokeReleaseInput::right_grip_release_after_hold;
    f.certificate.motion.virtual_mass_kg=2.;CHECK(f.model.bind(f.certificate,{1,2,true}));
    SmokeTriggerObserver observer;gears_hand_continuity::MotionHistory history{};
    unsigned presses{},releases{};bool down{};
    const auto raw=[&](bool squeeze,double x){
        if(squeeze&&!down)++presses;if(!squeeze&&down)++releases;down=squeeze;
        auto frame=f.frame({x,1.5,0});frame.equipment_hold_generation=41+presses;
        auto& hand=frame.hands.hands[1];hand.trigger_active=0;hand.squeeze=squeeze?1.f:0.f;
        hand.pressed[gears_vr::control_squeeze]=presses;hand.released[gears_vr::control_squeeze]=releases;
        if(history.count==gears_hand_continuity::motion_history_capacity){
            for(unsigned i=1;i<history.count;++i)history.frames[i-1]=history.frames[i];--history.count;}
        auto& row=history.frames[history.count++];row.input=frame.hands;
        row.stream_epoch=11;row.tracking_epoch[0]=21;row.tracking_epoch[1]=22;
        frame.motion_history=history;return frame;
    };
    auto baseline=raw(false,0);SmokeInput edge{};
    CHECK(observer.observe(baseline,edge,f.certificate.release_input)==SmokeTriggerObservation::baseline);
    SmokeOriginalReceipt last{};
    for(unsigned cycle=0;cycle<2;++cycle){
        auto held=raw(true,0);CHECK(observer.observe(held,edge,f.certificate.release_input)==SmokeTriggerObservation::baseline);
        CHECK(f.model.adopt_equipment_hold(held).event==E::request_ready);
        const auto prepare=f.take(A::prepare_original);
        CHECK(prepare.before.count==2-cycle&&prepare.expected_count==2-cycle);
        CHECK(f.model.acknowledge(f.receipt(prepare))==E::prepared);
        for(unsigned i=1;i<=8;++i){
            auto frame=raw(i<8,.02*i);const auto observed=observer.observe(frame,edge,f.certificate.release_input);
            const auto decision=f.model.update(frame,observed==SmokeTriggerObservation::edge?edge:SmokeInput{});
            CHECK(decision.event==(i==8?E::request_ready:E::observed));
        }
        const auto launch=f.take(A::launch_original);
        CHECK(launch.before.count==2-cycle&&launch.expected_count==1-cycle);
        CHECK(near(launch.tracked.velocity_mps.x,2.));
        CHECK(launch.original_projectile_type==f.certificate.original_projectile_type&&
              launch.original_emitter_type==f.certificate.original_emitter_type);
        last=f.receipt(launch);CHECK(f.model.acknowledge(last)==E::thrown);
        CHECK(f.model.inventory().count==1-cycle&&f.model.reserved_count()==0);
        CHECK(f.model.acknowledge(last)==E::rejected&&f.model.inventory().count==1-cycle);
        auto still_released=raw(false,.16);observer.observe(still_released,edge,f.certificate.release_input);
        f.model.update(still_released);CHECK(!f.model.take_request()&&f.model.stage()==S::idle);
    }
    auto empty_press=raw(true,0);CHECK(observer.observe(empty_press,edge,f.certificate.release_input)==SmokeTriggerObservation::baseline);
    const auto cue=f.model.adopt_equipment_hold(empty_press);
    CHECK(cue.event==E::no_ammo&&cue.request_original_no_ammo_sound&&cue.audio_for(f.certificate.identity,f.model.binding_token()));
    CHECK(!f.model.take_request()&&!f.model.reserved_count()&&!f.model.inventory().count);
    const auto first_audio=cue.audio_token;
    for(unsigned i=0;i<4;++i){auto held=raw(true,0);observer.observe(held,edge,f.certificate.release_input);
        const auto repeat=f.model.adopt_equipment_hold(held);CHECK(repeat.event!=E::no_ammo&&!repeat.request_original_no_ammo_sound);}
    auto released=raw(false,0);observer.observe(released,edge,f.certificate.release_input);f.model.update(released);
    auto fresh=raw(true,0);CHECK(observer.observe(fresh,edge,f.certificate.release_input)==SmokeTriggerObservation::baseline);
    const auto second=f.model.adopt_equipment_hold(fresh);
    CHECK(second.event==E::no_ammo&&second.audio_token>first_audio);
    CHECK(!f.model.take_request()&&!f.model.inventory().count&&f.model.stage()==S::idle);
}
void normal_carry_rotation_parity(){
    using Observe=SmokeTriggerObservation;
    constexpr auto rule=SmokeReleaseInput::right_grip_release_after_hold;
    enum Case {recorded,left_only,right_only,left_yaw,neutral,expired,roll,cap,rate,identity_change,reference_change,motion_change,units_change,stream_change,tracking_change};
    for(unsigned scenario=0;scenario<15;++scenario){
        Fixture q(3,SmokeFuseOwner::original_projectile);
        q.certificate.release_input=rule;q.certificate.motion.virtual_mass_kg=2.;
        CHECK(q.model.bind(q.certificate,{1,3,true}));SmokeTriggerObserver observer;
        gears_hand_continuity::MotionHistory history{};
        const bool initial_left=scenario!=neutral&&scenario!=right_only;
        const bool initial_right=scenario==recorded||scenario==right_only||scenario>=roll;
        const double old_yaw=.229809513;
        auto raw=[&](std::int64_t delta,bool left,bool right,bool down,double pitch,double yaw){
            auto f=q.frame({0,1.5,0},delta);f.equipment_hold_generation=42;
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
        auto held=raw(1000000,initial_left,initial_right,true,0,old_yaw);SmokeInput edge{};
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
            if(result.event!=E::safety_required)std::fprintf(stderr,"pitch case %u event=%u refusal=%u boundary=%u\n",scenario,unsigned(result.event),unsigned(q.model.last_interrupt_reason()),q.model.last_boundary_mask());
            CHECK(result.event==E::safety_required&&q.model.inventory().count==3);
            if(scenario<=rate)CHECK(q.model.last_interrupt_reason()==SmokeInterruptReason::mapping_boundary&&(q.model.last_boundary_mask()&32));
            CHECK(q.model.pending_request().action!=A::launch_original);
            continue;
        }
        if(result.event!=E::observed)std::fprintf(stderr,"pitch positive %u event=%u refusal=%u boundary=%u\n",scenario,unsigned(result.event),unsigned(q.model.last_interrupt_reason()),q.model.last_boundary_mask());
        CHECK(result.event==E::observed);
        const auto rotation=q.model.last_rotation_observation();
        CHECK(rotation.pitch_changed&&rotation.pitch_continuous);
        CHECK(rotation.consumer_gap_ns==61223400);
        CHECK(rotation.rotation_magnitude<.2&&rotation.rotation_magnitude<=8.*double(rotation.consumer_gap_ns)/1e9);
        if(scenario==left_only)CHECK(!rotation.yaw_changed&&!rotation.episode_after);
        else CHECK(rotation.yaw_changed&&rotation.continuous&&rotation.episode_after);
        auto released=raw(1000000,final_left,final_right,false,pitch,yaw);
        CHECK(observer.observe(released,edge,rule)==Observe::edge&&edge.kind==SmokeInputKind::right_grip_release);
        const auto decision=q.model.update(released,edge);
        if(decision.event!=E::request_ready)std::fprintf(stderr,"pitch release %u event=%u refusal=%u\n",scenario,unsigned(decision.event),unsigned(q.model.last_interrupt_reason()));
        CHECK(decision.event==E::request_ready);
        const auto launch=q.take(A::launch_original);
        CHECK(launch.tracked.predicted_time==released.hands.predicted_time);
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
        Fixture f;f.certificate.release_input=SmokeReleaseInput::right_grip_release_after_hold;
        f.certificate.motion.virtual_mass_kg=2.;
        CHECK(f.model.bind(f.certificate,{1,3,true}));
        SmokeTriggerObserver observer;gears_hand_continuity::MotionHistory history{};
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
        auto held=raw(0,true);SmokeInput edge{};
        CHECK(observer.observe(held,edge,f.certificate.release_input)==SmokeTriggerObservation::baseline);
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
            const auto decision=f.model.update(frame,observed==SmokeTriggerObservation::edge?edge:SmokeInput{});
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
int main(){
    normal_carry_rotation_parity();
    translated_body_origin_is_not_hand_velocity();
    finite_two_grasp_cycles_then_empty();
    bounded_source_gap_rebuilds_only_motion_window();
    stopped_preparation_after_external_debit();
    raw_grip_history_and_source_boundary();
    continuous_grip_owner_flow();
    certificate_gates();grab_and_empty();real_motion_and_one_original_commit();
    receipt_validation_and_old_callbacks();interruption_and_native_fuse();
    ambiguous_and_undispatched_operations();bad_motion_never_throws();
    smoke_classes_and_native_path_receipts();fresh_release_is_required();
    no_motion_or_loss_automatically_throws();identity_boundaries_and_mapping();
    native_fuse_conservation_and_no_trajectory_on_interrupt();
    std::printf("Smoke grab/fuse/original-once hand-trajectory checks passed (%u checks)\n",checks);
}
