#include "weapon_lancer_chainsaw.h"
#include <cassert>
#include <cstdio>

namespace {
gears_weapon::LancerSawSample sample() {
    gears_weapon::LancerSawSample s{};
    s.world=1;s.pawn=2;s.weapon=3;s.session=4;s.space_epoch=5;
    s.input_epoch=6;s.world_generation=7;s.sequence=1;s.created_ms=1000;
    s.lancer_active=s.right_owns_weapon=s.right_tracked=s.left_tracked=true;
    s.game_can_melee=true;
    return s;
}
void advance(gears_weapon::LancerSawSample& s,std::uint64_t ms=80) {
    ++s.sequence;s.created_ms+=ms;
}
void pull(gears_weapon::LancerChainsawInteraction& interaction,
          gears_weapon::LancerSawSample& s) {
    using namespace gears_weapon;
    assert(interaction.observe(s).event==LancerSawEvent::none);
    advance(s);s.left_grip_down=true;
    assert(interaction.observe(s).event==LancerSawEvent::pull_began);
    for(double x:{.05,.10,.15,.19}){
        advance(s);s.left_palm_m.x=x;
        const auto decision=interaction.observe(s);
        if(x<.19)assert(decision.event==LancerSawEvent::none);
        else {
            assert(decision.event==LancerSawEvent::request_original_hold);
            assert(decision.press_original_melee);
            assert(interaction.suppress_original_auto_target());
        }
    }
}
}
int main() {
    using namespace gears_weapon;
    LancerSawContact contact{};
    contact.victim=44;
    contact.original={true,true,true,true,true,true,true,true,true,true,true};
    std::array<float,16> gun_matrix{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    assert(!lancer_chain_outline_from_mesh(gun_matrix,false,contact.blade));
    assert(!lancer_saw_blade_contact(contact)); // no authenticated source mesh
    assert(lancer_chain_outline_from_mesh(gun_matrix,true,contact.blade));
    assert(contact.blade.world_raw[0].y==28);
    contact.target_a_raw={0,60,-20};contact.target_b_raw={0,60,20};
    contact.target_radius_raw=2;contact.blade_radius_raw=1.5;
    assert(lancer_saw_blade_contact(contact)); // chain edge reaches target
    contact.target_a_raw={0,50,1};contact.target_b_raw={0,50,3};
    assert(!lancer_saw_blade_contact(contact)); // housing centre is not the edge
    contact.target_a_raw={0,60,-20};contact.target_b_raw={0,60,20};
    contact.target_a_raw.x=30;contact.target_b_raw.x=30;
    assert(!lancer_saw_blade_contact(contact)); // view-facing is not contact
    contact.target_a_raw.x=0;contact.target_b_raw.x=0;
    contact.original.blade_path_unobstructed=false;
    assert(!lancer_saw_blade_contact(contact)); // wall blocks blade
    contact.original.blade_path_unobstructed=true;
    contact.original.weapon_target_valid=false;
    assert(!lancer_saw_blade_contact(contact)); // original rules still apply
    contact.original.weapon_target_valid=true;
    contact.original.target_controller_valid=false;
    assert(!lancer_saw_blade_contact(contact)); // controllerless dummy is ineligible
    contact.original.target_controller_valid=true;
    auto invalid=gun_matrix;invalid[0]=-1;
    assert(!lancer_chain_outline_from_mesh(invalid,true,contact.blade));
    invalid=gun_matrix;invalid[4]=.5f;
    assert(!lancer_chain_outline_from_mesh(invalid,true,contact.blade));
    invalid=gun_matrix;invalid[12]=100;
    LancerBladeOutline moved{};
    assert(lancer_chain_outline_from_mesh(invalid,true,moved));
    assert(moved.world_raw[0].x==100&&moved.world_raw[0].y==28);
    std::array<float,16> turned{0,1,0,0, -1,0,0,0, 0,0,1,0,
                                100,200,300,1};
    assert(lancer_chain_outline_from_mesh(turned,true,moved));
    assert(moved.world_raw[0].x==72&&moved.world_raw[0].y==200&&
           moved.world_raw[0].z==305);

    LancerChainsawInteraction interaction;
    auto s=sample();
    pull(interaction,s);
    advance(s);s.original_hold=true;s.original_button_held=true;
    assert(interaction.observe(s).event==LancerSawEvent::hold_started);
    assert(interaction.phase()==LancerSawPhase::running);
    s.game_can_melee=false; // original entry gate can close after B is held
    advance(s);s.contact=contact;
    s.contact.target_a_raw.x=30;s.contact.target_b_raw.x=30;
    assert(interaction.observe(s).event==LancerSawEvent::none);
    advance(s);s.contact.target_a_raw.x=0;s.contact.target_b_raw.x=0;
    auto d=interaction.observe(s);
    assert(d.event==LancerSawEvent::request_original_acquire&&d.victim==44);
    assert(interaction.suppress_original_auto_target());
    assert(interaction.observe(s).event==LancerSawEvent::none);
    advance(s);s.original_hold=false;
    assert(interaction.observe(s).event==LancerSawEvent::none);
    advance(s);s.original_execution=true;s.original_victim=44;
    d=interaction.observe(s);
    assert(d.event==LancerSawEvent::animation_handoff);
    assert(!d.release_original_melee&&d.animation_owns_hands);
    assert(!d.unexpected_original_target);
    assert(!interaction.suppress_original_auto_target());
    advance(s);++s.space_epoch;s.right_tracked=false;
    d=interaction.observe(s);
    assert(d.animation_owns_hands&&!d.restore_6dof);
    advance(s);s.original_execution=false;s.original_hold=false;
    d=interaction.observe(s);
    assert(d.animation_owns_hands&&!d.restore_6dof);
    advance(s);s.original_ready=true;
    d=interaction.observe(s);
    assert(d.event==LancerSawEvent::request_original_release);
    assert(d.release_original_melee&&d.animation_owns_hands&&!d.restore_6dof);
    advance(s);
    d=interaction.observe(s);
    assert(d.event==LancerSawEvent::none&&!d.release_original_melee&&
           d.animation_owns_hands&&!d.restore_6dof);
    advance(s);s.original_button_held=false;
    d=interaction.observe(s);
    assert(d.animation_owns_hands&&!d.restore_6dof);
    advance(s);s.right_tracked=true;
    d=interaction.observe(s);
    assert(d.event==LancerSawEvent::restored&&d.restore_6dof);
    assert(interaction.phase()==LancerSawPhase::idle);

    // If the original attack has ended after the gun was stowed, clear the
    // animation lock without restoring a gun that the player no longer owns.
    interaction.reset();s=sample();pull(interaction,s);
    advance(s);s.original_button_held=true;s.original_execution=true;
    d=interaction.observe(s);
    assert(d.event==LancerSawEvent::animation_handoff);
    advance(s);s.original_execution=false;s.original_ready=true;
    d=interaction.observe(s);
    assert(d.event==LancerSawEvent::request_original_release);
    advance(s);s.original_button_held=false;s.right_owns_weapon=false;
    d=interaction.observe(s);
    assert(d.event==LancerSawEvent::interrupted&&!d.restore_6dof&&
           !d.animation_owns_hands&&interaction.phase()==LancerSawPhase::idle);

    // A grip already held on the first frame is a baseline, not a fresh tug.
    s=sample();s.left_grip_down=true;
    assert(interaction.observe(s).event==LancerSawEvent::none);
    advance(s);
    assert(interaction.observe(s).event==LancerSawEvent::none);
    advance(s);s.left_grip_down=false;interaction.observe(s);
    advance(s);s.left_grip_down=true;
    assert(interaction.observe(s).event==LancerSawEvent::pull_began);
    advance(s);s.left_palm_m.x=.2;s.starter_anchor_m.x=-.11;
    assert(interaction.observe(s).event==LancerSawEvent::interrupted);

    interaction.reset();s=sample();interaction.observe(s);
    advance(s);s.left_grip_down=true;
    assert(interaction.observe(s).event==LancerSawEvent::pull_began);
    advance(s);s.left_palm_m.x=.19;
    assert(interaction.observe(s).event==LancerSawEvent::interrupted);

    // Moving only the gun towards a stationary hand cannot pull its cord.
    interaction.reset();s=sample();interaction.observe(s);
    advance(s);s.left_grip_down=true;
    assert(interaction.observe(s).event==LancerSawEvent::pull_began);
    advance(s);s.starter_anchor_m.x=-.08;
    assert(interaction.observe(s).event==LancerSawEvent::none);
    advance(s);s.left_palm_m.y=.10;
    assert(interaction.observe(s).event==LancerSawEvent::interrupted);

    // A recenter or tracking gap drops ownership and releases the original B.
    interaction.reset();s=sample();pull(interaction,s);
    advance(s);++s.space_epoch;
    d=interaction.observe(s);
    assert(d.event==LancerSawEvent::interrupted&&d.release_original_melee);
    assert(!interaction.suppress_original_auto_target());

    interaction.reset();s=sample();pull(interaction,s);
    advance(s,120);
    d=interaction.observe(s);
    assert(d.event==LancerSawEvent::interrupted&&d.release_original_melee);

    // If an original attack somehow starts before the verified contact, hand
    // ownership still yields to its animation and records the unsafe path.
    interaction.reset();s=sample();pull(interaction,s);
    advance(s);s.original_hold=true;interaction.observe(s);
    advance(s);s.original_execution=true;s.original_victim=99;
    d=interaction.observe(s);
    assert(d.animation_owns_hands&&d.unexpected_original_target);
    assert(!d.release_original_melee);

    // Attack entry and a lost XR frame in the same tick still belongs to the
    // original paired animation; releasing B here would cut it short.
    interaction.reset();s=sample();pull(interaction,s);
    advance(s,120);s.original_execution=true;s.original_victim=99;
    ++s.space_epoch;s.right_tracked=false;
    d=interaction.observe(s);
    assert(d.event==LancerSawEvent::animation_handoff&&
           d.animation_owns_hands&&!d.release_original_melee);
    std::puts("Lancer chainsaw VR gesture, contact and handoff passed");
}
