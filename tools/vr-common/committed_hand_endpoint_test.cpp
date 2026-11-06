#include "committed_hand_interaction.h"
#include "weapon_retained_item_motion.h"
#include "weapon_equipment_model.h"
#include "left_hand_interaction_owner.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef NDEBUG
#error Endpoint checks must remain enabled
#endif
namespace e=gears_committed_hand;
namespace h=gears_weapon_hand;
unsigned checks{};
#define C(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::exit(1);}}while(false)
constexpr std::int64_t now=2000000000;
h::ObjectIdentity object(std::uint64_t p){return {p,0x8000,0x9000,unsigned(p/8),7,0,1};}
e::Frame fixture(){
    e::Frame f{};f.parent={1,2,3,4,5,6,7,8};
    f.world=object(0x1000);f.controller=object(0x2000);f.pawn=object(0x3000);
    f.mesh=object(0x4000);f.weapon=object(0x5000);
    f.serial=f.tick=f.compose=f.hold_generation=f.binding_generation=f.reference_generation=1;
    f.native_reference={1,e::rig::reference_stable,0};f.producer={1,{1,1}};
    f.acquisition_begin_ns=now-3000000;f.acquisition_end_ns=now-2000000;f.committed_ns=now-1000000;f.returned_ns=now;
    auto& s=f.stereo;s.session=s.sequence=s.space_epoch=1;s.predicted_time=123;
    s.received_ns=now-4000000;s.head_flags=s.view_flags=15;s.focused=s.visible=s.should_render=1;
    s.head={{0,1.6,0},{0,0,0,1}};
    for(unsigned i=0;i<2;++i){s.eyes[i].pose.position={i?.032:-.032,1.6,0};s.eyes[i].fov={-.8,.8,.8,-.8};}
    auto& c=f.controllers;c.source=s.source;c.session=s.session;c.sequence=s.sequence;c.space_epoch=s.space_epoch;
    c.input_epoch=c.profile_generation=1;c.predicted_time=s.predicted_time;c.focused=1;
    c.received_ns=now-4000000;c.sync_ns=now-5000000;
    for(auto& hand:c.hands){hand.grip_active=1;hand.grip_flags=15;}
    f.geometry=gears_hand_authored_mount::marcus115_signature;f.units_per_metre=100;return f;
}
void acquisition_cohort_cases(){
    namespace pair=gears_committed_interaction;
    namespace vr=gears_vr;
    for(unsigned selected=0;selected<2;++selected){
        pair::Frame actual{};actual.right=fixture();
        h::Frame expected{};expected.controllers=actual.right.controllers;
        expected.interaction_control=selected;
        const auto control=h::left_control(expected);
        auto& old=actual.right.controllers.hands[0];auto& current=expected.controllers.hands[0];
        old.squeeze_active=old.trigger_active=current.squeeze_active=current.trigger_active=1;
        ++current.pressed[control];
        if(control==vr::control_trigger)current.trigger=1;else current.squeeze=1;
        C(!pair::same_left_control_cohort(actual,expected));
        // Reproduce2572's available old pair on the new press, followed by
        // current actual magazine contact on the very next owner frame.
        vr::LeftHandInteractionOwner owner;vr::LeftHandInteractionFrame f{};
        f.lifecycle=4;f.frame=1806;f.now_ns=now;f.context_valid=f.tracked=true;
        C(owner.update(f).status==vr::LeftHandInteractionStatus::released);
        ++f.frame;f.press_token=4;f.grip_down=true;
        f.requests.magazine_observation_pending=!pair::same_left_control_cohort(actual,expected);
        auto d=owner.update(f);C(d.status==vr::LeftHandInteractionStatus::no_candidate&&!d.new_grant&&!d.release_required);
        ++f.frame;f.now_ns+=80000000;actual.right.controllers=expected.controllers;
        C(pair::same_left_control_cohort(actual,expected));
        f.requests.magazine_observation_pending=false;f.requests.magazine=true;
        d=owner.update(f);C(d.status==vr::LeftHandInteractionStatus::granted&&d.new_grant&&d.press_token==4);
        // An unrelated right-hand shot must not stall this left-hand grasp.
        ++expected.controllers.hands[1].pressed[vr::control_trigger];
        C(pair::same_left_control_cohort(actual,expected));
        ++expected.controllers.hands[0].released[control];
        C(!pair::same_left_control_cohort(actual,expected));
        actual.right.controllers=expected.controllers;++actual.right.controllers.input_epoch;
        C(!pair::same_left_control_cohort(actual,expected));
    }
}
int main(){
    acquisition_cohort_cases();
    auto held=fixture();C(e::fresh(held,now,false));
    h::Frame action{};action.key=held.parent;action.world=held.world;action.controller=held.controller;
    action.pawn=held.pawn;action.mesh=held.mesh;action.weapon=held.weapon;
    e::rig::Frame tracking{};tracking.binding_generation=held.binding_generation;tracking.reference_generation=held.reference_generation;
    tracking.native_reference=held.native_reference;tracking.units_per_metre=held.units_per_metre;
    tracking.producer=held.producer;tracking.stereo=held.stereo;tracking.controllers=held.controllers;
    C(e::matches(held,action,tracking,1,now,false));
    auto observed=held;observed.purpose=e::Purpose::observed_held_visual;
    C(e::fresh(observed,now,false));C(e::current_result(observed,now+20000000,false));
    C(!e::matches(observed,action,tracking,1,now,false));
    auto old_abi=observed;old_abi.abi_version=2;C(!e::fresh(old_abi,now,false));
    auto old_request=e::Request{};old_request.cookie=3;old_request.native_started=2;
    old_request.abi_version=2;C(!e::request_valid(old_request));
    C(!e::current_result(observed,now+100000001,false));
    // Late rendering can observe actual new geometry without renewing the old
    // acquisition/commit/return clock or recovering any action permission.
    e::Frame late{};auto palm=held.right_grip;palm.position.x=12;
    C(e::observe_visual(held,palm,2,now+10000000,false,late));
    auto expected=held;expected.purpose=e::Purpose::observed_held_visual;
    expected.serial=2;expected.right_grip=palm;
    C(!std::memcmp(&late,&expected,sizeof late));
    C(!e::matches(late,action,tracking,1,now+10000000,false));
    auto untouched=late;
    C(!e::observe_visual(held,palm,1,now,false,late));
    C(!std::memcmp(&late,&untouched,sizeof late));
    C(!e::observe_visual(held,palm,2,now+40000001,false,late));
    auto bad_palm=palm;bad_palm.orientation={0,0,0,0};
    C(!e::observe_visual(held,bad_palm,2,now,false,late));
    auto free_observation=held;free_observation.purpose=e::Purpose::free_hand_visual;
    free_observation.hold_generation=free_observation.parent.lifecycle=0;
    C(!e::observe_visual(free_observation,palm,2,now,false,late));
    C(e::observe_visual(observed,palm,2,now+10000000,false,late));
    C(late.returned_ns==observed.returned_ns&&late.controllers.sync_ns==observed.controllers.sync_ns&&
      late.acquisition_begin_ns==observed.acquisition_begin_ns&&late.committed_ns==observed.committed_ns);
    auto visual=held;visual.purpose=e::Purpose::free_hand_visual;visual.hold_generation=visual.parent.lifecycle=0;
    C(e::fresh(visual,now,false));C(e::current_result(visual,now+20000000,false));
    C(!e::matches(visual,action,tracking,1,now,false));
    action.key.lifecycle=0;C(!e::matches(visual,action,tracking,0,now,false));
    for(unsigned i=0;i<6;++i){auto bad=visual;
        switch(i){case 0:bad.hold_generation=1;break;case 1:bad.parent.lifecycle=1;break;
        case 2:bad.reserved=1;break;case 3:bad.abi_version=1;break;
        case 4:bad.purpose=static_cast<e::Purpose>(9);break;case 5:bad.controllers.sync_ns=now-40000001;break;}
        C(!e::fresh(bad,now,false));}
    C(!e::current_result(visual,now+100000001,false));
    auto incomplete=held;incomplete.hold_generation=0;C(!e::fresh(incomplete,now,false));
    gears_committed_interaction::Frame pair{};pair.right=visual;pair.model_scale=1;
    C(!gears_committed_interaction::matches(pair,action,0,now,false));
    pair.right=held;pair.right.controllers.hands[0].squeeze_active=1;pair.right.controllers.hands[0].squeeze=.8f;
    h::RetainedItemInput input{};input.parent=gears_committed_interaction::equipment_identity(pair);
    input.lease={h::LeaseKind::ammo,0,held.parent.lifecycle,9};input.press_token=9;
    input.controller_sequence=held.controllers.sequence;input.source=held.controllers.source;
    input.session=held.controllers.session;input.space=held.controllers.space_epoch;
    input.input=held.controllers.input_epoch;input.profile=held.controllers.profile_generation;
    input.original_input_ns=now-500000000;input.decision_ns=now-400000000;
    C(gears_committed_interaction::matches_retained_item(pair,input,1,now,false));
    {
        auto pinched=pair;auto pinch=input;pinch.interaction_control=1;
        auto& left=pinched.right.controllers.hands[0];
        left.squeeze=0;left.trigger_active=1;left.trigger=.8f;
        C(gears_committed_interaction::matches_retained_item(pinched,pinch,1,now,false));
        ++left.released[gears_vr::control_trigger];
        C(!gears_committed_interaction::matches_retained_item(pinched,pinch,1,now,false));
        --left.released[gears_vr::control_trigger];left.trigger=0;left.squeeze=.8f;
        C(!gears_committed_interaction::matches_retained_item(pinched,pinch,1,now,false));
        left.trigger=.8f;pinch.interaction_control=2;
        C(!gears_committed_interaction::matches_retained_item(pinched,pinch,1,now,false));
    }
    auto observed_pair=pair;observed_pair.right.purpose=e::Purpose::observed_held_visual;
    C(!gears_committed_interaction::matches_retained_item(observed_pair,input,1,now,false));
    // Original decision may be historical; actual geometry must still be new,
    // and a missed release/repress, tracking loss or identity change rejects it.
    for(unsigned i=0;i<11;++i){auto bad=pair;auto association=input;
        switch(i){
        case 0:++bad.right.controllers.hands[0].pressed[gears_vr::control_squeeze];break;
        case 1:++bad.right.controllers.hands[0].released[gears_vr::control_squeeze];break;
        case 2:bad.right.controllers.hands[0].squeeze=.35f;break;
        case 3:bad.right.controllers.hands[0].grip_active=0;break;
        case 4:++bad.right.parent.frame;break;
        case 5:++bad.right.weapon.name_number;break;
        case 6:++bad.right.controllers.input_epoch;break;
        case 7:association.lease.kind=h::LeaseKind::support;break;
        case 8:++association.press_token;break;
        case 9:association.original_input_ns=now+1;break;
        case 10:bad.right.returned_ns=now-100000001;break;
        }
        C(!gears_committed_interaction::matches_retained_item(bad,association,1,now,false));
    }
    // Original switch may temporarily make the new weapon current while the
    // old mesh is still visible. Visual identity is separate from a held grant.
    gears_weapon::EquipmentSnapshot game{11,22,101,{101,0,202,303},true,false};
    gears_weapon::WeaponEquipmentModel model;
    C(model.tap(gears_weapon::BodySlot::pistol,game).event==gears_weapon::EquipmentEvent::request_unholster);
    C(model.held());C(!model.departing_for(11,22,101));
    C(model.tap(gears_weapon::BodySlot::weapon_2,game).event==gears_weapon::EquipmentEvent::request_original_switch);
    C(model.departing_for(11,22,101));C(!model.held()&&!model.hold_generation());
    C(!model.departing_for(12,22,101));C(!model.departing_for(11,23,101));C(!model.departing_for(11,22,202));
    game.current_weapon=202;game.animation_busy=true;model.observe(game);C(model.departing_for(11,22,101));
    game.animation_busy=false;model.observe(game);C(!model.departing_for(11,22,101)&&model.held());
    game.current_weapon=202;C(model.tap(gears_weapon::BodySlot::pistol,game).event==gears_weapon::EquipmentEvent::request_original_switch);
    C(model.departing_for(11,22,202));model.switch_rejected(game);C(!model.departing_for(11,22,202));
    std::printf("Committed hand endpoint: %u checks passed; visual data never grants hold/fire\n",checks);
}
