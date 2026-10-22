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
    contact.victim=44;contact.original_target_valid=contact.unobstructed=true;
    contact.blade_base_raw={0,0,0};contact.blade_tip_raw={0,40,0};
    contact.target_a_raw={10,20,-20};contact.target_b_raw={10,20,20};
    contact.target_radius_raw=9;contact.blade_radius_raw=2;
    assert(lancer_saw_blade_contact(contact)); // blade side reaches target
    contact.target_a_raw.x=30;contact.target_b_raw.x=30;
    assert(!lancer_saw_blade_contact(contact)); // view-facing is not contact
    contact.target_a_raw.x=10;contact.target_b_raw.x=10;
    contact.unobstructed=false;
    assert(!lancer_saw_blade_contact(contact)); // wall blocks blade
    contact.unobstructed=true;contact.original_target_valid=false;
    assert(!lancer_saw_blade_contact(contact)); // original rules still apply

    LancerChainsawInteraction interaction;
    auto s=sample();
    pull(interaction,s);
    advance(s);s.original_hold=true;
    assert(interaction.observe(s).event==LancerSawEvent::hold_started);
    assert(interaction.phase()==LancerSawPhase::running);
    s.game_can_melee=false; // original entry gate can close after B is held
    advance(s);contact.original_target_valid=true;s.contact=contact;
    s.contact.target_a_raw.x=30;s.contact.target_b_raw.x=30;
    assert(interaction.observe(s).event==LancerSawEvent::none);
    advance(s);s.contact.target_a_raw.x=10;s.contact.target_b_raw.x=10;
    auto d=interaction.observe(s);
    assert(d.event==LancerSawEvent::request_original_acquire&&d.victim==44);
    assert(interaction.suppress_original_auto_target());
    assert(interaction.observe(s).event==LancerSawEvent::none);
    advance(s);s.original_hold=false;
    assert(interaction.observe(s).event==LancerSawEvent::none);
    advance(s);s.original_execution=true;s.original_victim=44;
    d=interaction.observe(s);
    assert(d.event==LancerSawEvent::animation_handoff);
    assert(d.release_original_melee&&d.animation_owns_hands);
    assert(!d.unexpected_original_target);
    assert(!interaction.suppress_original_auto_target());
    advance(s);++s.space_epoch;s.right_tracked=false;
    d=interaction.observe(s);
    assert(d.animation_owns_hands&&!d.restore_6dof);
    advance(s);s.original_execution=false;s.original_hold=false;
    d=interaction.observe(s);
    assert(d.animation_owns_hands&&!d.restore_6dof);
    advance(s);s.original_ready=true;s.right_tracked=true;
    d=interaction.observe(s);
    assert(d.event==LancerSawEvent::restored&&d.restore_6dof);
    assert(interaction.phase()==LancerSawPhase::idle);

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
    assert(d.release_original_melee);
    std::puts("Lancer chainsaw VR gesture, contact and handoff passed");
}
