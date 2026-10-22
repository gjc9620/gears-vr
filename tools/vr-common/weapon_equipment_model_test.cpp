#include "weapon_equipment_model.h"
#include "weapon_input_router.h"
#include <cassert>
#include <cstdio>

using namespace gears_weapon;

int main(){
    EquipmentSnapshot game{11,22,0,{101,0,202,303},true,false};
    WeaponEquipmentModel hands;
    WeaponEquipmentModel first_press;
    assert(first_press.tap(BodySlot::weapon_2,game).event==
           EquipmentEvent::request_original_switch);
    assert(equipment_body_slot(WeaponIntent::grab_ammo)==BodySlot::none);
    assert(equipment_body_slot(WeaponIntent::weapon_1)==BodySlot::weapon_1);
    assert(hands.observe(game).event==EquipmentEvent::ignored);
    assert(hands.tap(BodySlot::shoulder_ammo,game).event==EquipmentEvent::ignored);
    auto empty=hands.tap(BodySlot::grenade,game);
    assert(empty.event==EquipmentEvent::empty_slot&&empty.inventory_slot==1&&!empty.weapon);
    game.weapon_in_slot[1]=404;
    assert(hands.tap(BodySlot::grenade,game).event==EquipmentEvent::empty_slot);
    game.throwable_count_known=true;
    game.throwable_count=0;
    assert(hands.tap(BodySlot::grenade,game).event==EquipmentEvent::empty_slot);
    game.throwable_count=2;
    assert(hands.tap(BodySlot::grenade,game).event==EquipmentEvent::request_original_switch);
    assert(hands.switch_rejected(game).event==EquipmentEvent::interrupted);
    game.throwable_count=0;
    auto switch_to_left=hands.tap(BodySlot::weapon_2,game);
    assert(switch_to_left.event==EquipmentEvent::request_original_switch&&
           switch_to_left.inventory_slot==2&&switch_to_left.weapon==202);
    assert(!hands.held()&&hands.switch_pending()&&!hands.animation_owns_hands());
    assert(hands.tap(BodySlot::weapon_2,game).event==EquipmentEvent::busy);
    game.animation_busy=true;
    assert(hands.observe(game).event==EquipmentEvent::ignored);
    game.current_weapon=202;
    assert(hands.observe(game).event==EquipmentEvent::waiting_for_animation);
    game.animation_busy=false;
    assert(hands.observe(game).event==EquipmentEvent::equipped);
    assert(hands.held()&&hands.held_weapon()==202&&hands.held_slot()==2);
    // Releasing right grip has no model action: the gun remains latched.
    assert(hands.observe(game).event==EquipmentEvent::ignored&&hands.held());
    assert(hands.tap(BodySlot::grenade,game).event==EquipmentEvent::empty_slot);
    assert(hands.held_weapon()==202);
    game.ammo_known[2]=true;game.ammo_available[2]=false;
    auto stow=hands.tap(BodySlot::weapon_2,game);
    assert(stow.event==EquipmentEvent::request_stow&&stow.weapon==202&&!hands.held());
    game.ammo_available[2]=true;
    auto take_again=hands.tap(BodySlot::weapon_2,game);
    assert(take_again.event==EquipmentEvent::request_unholster&&hands.held_weapon()==202);

    auto switch_to_right=hands.tap(BodySlot::weapon_1,game);
    assert(switch_to_right.event==EquipmentEvent::request_original_switch&&
           switch_to_right.inventory_slot==3&&switch_to_right.weapon==303);
    game.current_weapon=0;game.animation_busy=true;
    assert(hands.observe(game).event==EquipmentEvent::ignored);
    game.current_weapon=303;game.animation_busy=false;
    assert(hands.observe(game).event==EquipmentEvent::equipped&&hands.held_weapon()==303);
    game.weapon_in_slot[3]=0;
    assert(hands.observe(game).event==EquipmentEvent::interrupted&&!hands.held());

    game.current_weapon=101;
    assert(hands.tap(BodySlot::weapon_2,game).event==EquipmentEvent::request_original_switch);
    assert(hands.switch_rejected(game).event==EquipmentEvent::interrupted);
    assert(!hands.switch_pending()&&!hands.animation_owns_hands()&&!hands.held());

    game.ammo_known[2]=true;game.ammo_available[2]=false;
    assert(hands.tap(BodySlot::weapon_2,game).event==EquipmentEvent::empty_slot);
    assert(!hands.switch_pending());
    game.ammo_available[2]=true;

    assert(hands.begin_pickup(game).event==EquipmentEvent::pickup_animation);
    assert(hands.animation_owns_hands());
    game.animation_busy=true;
    assert(hands.finish_pickup(game,0).event==EquipmentEvent::ignored);
    game.animation_busy=false;
    assert(hands.finish_pickup(game,0).event==EquipmentEvent::equipped);
    assert(hands.held_weapon()==101&&hands.held_slot()==0);
    game.world=12;
    assert(hands.observe(game).event==EquipmentEvent::interrupted&&!hands.held());
    assert(hands.tap(BodySlot::pistol,game).event==EquipmentEvent::request_unholster);
    game.current_weapon=202;
    assert(hands.tap(BodySlot::pistol,game).event==EquipmentEvent::interrupted);
    assert(!hands.held());
    std::puts("weapon equipment latch, empty-slot, animation and identity checks passed");
}
