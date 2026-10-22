#include "weapon_bolo_grenade_interaction.h"
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
    std::uint64_t epoch{1},reference{1};
    std::int64_t ns{2000000000},game_tick{100};
    bool trigger_down{};
    explicit Fixture(std::uint32_t count=3,BoloFuseOwner fuse=BoloFuseOwner::original_projectile,
                     Vector offset={}){
        certificate=config(fuse);certificate.centre_in_right_grip_metres=offset;
        CHECK(model.bind(certificate,{1,count,true}));
    }
    BoloStamp stamp(std::int64_t delta=1000000){
        ns+=delta;++game_tick;return {certificate.identity,model.binding_token(),++sequence,epoch,ns,game_tick};
    }
    BoloFrame frame(Vector position={0,1.5,0},std::int64_t delta=10000000){
        BoloFrame f{};f.stamp=stamp(delta);f.inventory=model.inventory();
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
        CHECK(f.model.pending_request().action==A::preserve_original_on_interrupt&&f.model.inventory().count==3);
        const auto request=f.take(A::preserve_original_on_interrupt);
        CHECK(f.model.acknowledge(f.receipt(request))==E::safety_completed);
        CHECK(f.step(BoloInputKind::right_trigger_release).event==E::rejected);
        CHECK(!f.model.take_request()&&f.model.inventory().count==3);
    }
    for(unsigned mode=0;mode<4;++mode){
        Fixture f;f.grab();f.prepare();
        const unsigned count=mode==0?2u:8u;
        for(unsigned i=0;i<count;++i){
            auto point=Vector{mode==1?0:.04*double(i),1.5,0};
            if(mode==2&&i==count-1)point.x=4; // Tracking teleport on release.
            auto frame=f.frame(point,mode==3&&i==count-1?70000000:10000000);
            const auto result=f.model.update(frame,i==count-1?f.input(frame,BoloInputKind::right_trigger_release):BoloInput{});
            if(i==count-1)CHECK(result.event==E::safety_required);
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
}
int main(){
    certificate_gates();grab_and_empty();real_motion_and_one_original_commit();
    receipt_validation_and_old_callbacks();interruption_and_native_fuse();
    ambiguous_and_undispatched_operations();bad_motion_never_throws();
    input_provenance_and_verified_press();
    std::printf("Bolo grab/fuse/original-once hand-trajectory checks passed (%u checks)\n",checks);
}
