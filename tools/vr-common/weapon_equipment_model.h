#pragma once
#include "body_slots.h"
#include <array>
#include <cstdint>

namespace gears_weapon {

// Identities and slot contents must come from the authenticated game-thread
// inventory. A hand gesture alone must never create or remove a weapon.
struct EquipmentSnapshot {
    std::uint64_t world{},pawn{},current_weapon{};
    std::array<std::uint64_t,4> weapon_in_slot{};
    bool can_switch{},animation_busy{};
    // The grenade slot can retain a weapon actor after its throwable count is
    // zero. Only a game-thread inventory read may assert count_known.
    std::uint32_t throwable_count{};
    bool throwable_count_known{};
    bool valid() const noexcept {return world&&pawn;}
};

enum class EquipmentEvent : std::uint8_t {
    ignored,empty_slot,busy,request_original_switch,request_unholster,
    request_stow,waiting_for_animation,equipped,pickup_animation,interrupted
};
struct EquipmentDecision {
    EquipmentEvent event{EquipmentEvent::ignored};
    std::uint8_t inventory_slot{4};
    std::uint64_t weapon{};
};

// Right-grip presses are momentary; this model latches the held weapon until
// the player places it at its original slot. The live adapter must execute
// original inventory switching, weapon display, firing gate and sound only
// after checking the same game-thread identities again.
class WeaponEquipmentModel {
    enum class Phase : std::uint8_t { neutral,held,switching,pickup };
    Phase phase_{Phase::neutral};
    std::uint64_t world_{},pawn_{},weapon_{},previous_weapon_{};
    std::uint8_t slot_{4};

    void clear_hold() noexcept {phase_=Phase::neutral;weapon_=previous_weapon_=0;slot_=4;}
    bool same_player(EquipmentSnapshot const& game) const noexcept {
        return game.valid()&&game.world==world_&&game.pawn==pawn_;
    }
public:
    void reset() noexcept {*this=WeaponEquipmentModel{};}
    bool held() const noexcept {return phase_==Phase::held;}
    bool animation_owns_hands() const noexcept {return phase_==Phase::pickup;}
    bool switch_pending() const noexcept {return phase_==Phase::switching;}
    std::uint64_t held_weapon() const noexcept {return held()?weapon_:0;}
    std::uint8_t held_slot() const noexcept {return held()?slot_:4;}

    EquipmentDecision observe(EquipmentSnapshot const& game) noexcept {
        if(!game.valid()){reset();return {EquipmentEvent::interrupted};}
        if(!world_){world_=game.world;pawn_=game.pawn;return {};}
        if(!same_player(game)){reset();world_=game.world;pawn_=game.pawn;
            return {EquipmentEvent::interrupted};}
        if(phase_==Phase::held){
            if(slot_>=4||game.weapon_in_slot[slot_]!=weapon_||
               game.current_weapon!=weapon_){
                clear_hold();return {EquipmentEvent::interrupted};
            }
        }else if(phase_==Phase::switching){
            if(slot_>=4||game.weapon_in_slot[slot_]!=weapon_){
                clear_hold();return {EquipmentEvent::interrupted};
            }
            if(game.current_weapon==weapon_){
                if(game.animation_busy)
                    return {EquipmentEvent::waiting_for_animation,slot_,weapon_};
                phase_=Phase::held;previous_weapon_=0;
                return {EquipmentEvent::equipped,slot_,weapon_};
            }
            if(game.current_weapon&&game.current_weapon!=previous_weapon_&&
               !game.animation_busy){
                clear_hold();return {EquipmentEvent::interrupted};
            }
        }
        return {};
    }

    EquipmentDecision tap(BodySlot body_slot,EquipmentSnapshot const& game) noexcept {
        if(!game.valid())return {};
        if(!world_){world_=game.world;pawn_=game.pawn;}
        if(!same_player(game)){
            reset();world_=game.world;pawn_=game.pawn;
            return {EquipmentEvent::interrupted};
        }
        auto slot=original_inventory_slot(body_slot);
        if(!slot||*slot>=4)return {};
        if(phase_==Phase::held&&
           (slot_>=4||game.weapon_in_slot[slot_]!=weapon_||
            game.current_weapon!=weapon_)){
            clear_hold();return {EquipmentEvent::interrupted};
        }
        if(phase_==Phase::pickup||phase_==Phase::switching||
           game.animation_busy||!game.can_switch)return {EquipmentEvent::busy};
        const auto item=game.weapon_in_slot[*slot];
        if(!item||(*slot==1&&game.throwable_count_known&&!game.throwable_count))
            return {EquipmentEvent::empty_slot,*slot};
        if(phase_==Phase::held&&slot_==*slot&&weapon_==item&&
           game.current_weapon==item){
            clear_hold();return {EquipmentEvent::request_stow,*slot,item};
        }
        if(game.current_weapon==item){
            phase_=Phase::held;slot_=*slot;weapon_=item;previous_weapon_=0;
            return {EquipmentEvent::request_unholster,*slot,item};
        }
        phase_=Phase::switching;slot_=*slot;weapon_=item;
        previous_weapon_=game.current_weapon;
        return {EquipmentEvent::request_original_switch,*slot,item};
    }

    // Called if the original game rejects the requested switch or never starts
    // it. A synthetic hand request must not stay pending forever.
    EquipmentDecision switch_rejected(EquipmentSnapshot const& game) noexcept {
        if(!same_player(game)||phase_!=Phase::switching)return {};
        clear_hold();return {EquipmentEvent::interrupted};
    }

    EquipmentDecision begin_pickup(EquipmentSnapshot const& game) noexcept {
        if(!same_player(game)||phase_==Phase::pickup||phase_==Phase::switching)
            return {EquipmentEvent::busy};
        clear_hold();phase_=Phase::pickup;
        return {EquipmentEvent::pickup_animation};
    }
    EquipmentDecision finish_pickup(EquipmentSnapshot const& game,
                                    std::uint8_t inventory_slot) noexcept {
        if(!same_player(game)||phase_!=Phase::pickup||inventory_slot>=4||
           game.animation_busy||!game.current_weapon||
           game.weapon_in_slot[inventory_slot]!=game.current_weapon)
            return {};
        phase_=Phase::held;slot_=inventory_slot;weapon_=game.current_weapon;
        return {EquipmentEvent::equipped,slot_,weapon_};
    }
};
}
