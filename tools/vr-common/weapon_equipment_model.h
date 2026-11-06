#pragma once
#include "body_slots.h"
#include <array>
#include <atomic>
#include <cstdint>

namespace gears_weapon {

// Identities and slot contents must come from the authenticated game-thread
// inventory. A hand gesture alone must never create or remove a weapon.
struct EquipmentSnapshot {
    std::uint64_t world{},pawn{},current_weapon{};
    std::array<std::uint64_t,4> weapon_in_slot{};
    bool can_switch{},animation_busy{};
    // The live adapter reads each occupied weapon's original HasAnyAmmo on
    // the same game thread. Unknown is allowed for isolated model fixtures;
    // live snapshot acquisition fails rather than asserting availability.
    std::array<bool,4> ammo_available{},ammo_known{};
    // SetWeaponFromSlot also accepts a dry weapon when its original
    // bCanSelectWithoutAmmo flag is set. The live snapshot must read this
    // alongside HasAnyAmmo; an unknown flag fails closed as false.
    std::array<bool,4> can_select_without_ammo{};
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

namespace equipment_detail {
// Copies of a proposed model may consume an ID without committing it. IDs are
// never reused by reset, stow, pickup or a different model instance.
inline std::atomic<std::uint64_t> next_hold{1};
inline std::uint64_t new_hold() noexcept {
    auto value=next_hold.load(std::memory_order_relaxed);
    while(value&&value!=~std::uint64_t(0)){
        if(next_hold.compare_exchange_weak(value,value+1,std::memory_order_relaxed))return value;
    }
    return 0;
}
}

// Right-grip presses are momentary; this model latches the held weapon until
// the player places it at its original slot. The live adapter must execute
// original inventory switching, weapon display, firing gate and sound only
// after checking the same game-thread identities again.
class WeaponEquipmentModel {
    enum class Phase : std::uint8_t { neutral,held,switching,pickup };
    Phase phase_{Phase::neutral};
    std::uint64_t world_{},pawn_{},weapon_{},previous_weapon_{};
    std::uint64_t hold_generation_{};
    std::uint8_t slot_{4};

    void clear_hold() noexcept {phase_=Phase::neutral;weapon_=previous_weapon_=hold_generation_=0;slot_=4;}
    bool become_held() noexcept {
        const auto generation=equipment_detail::new_hold();
        if(!generation){clear_hold();return false;}
        hold_generation_=generation;phase_=Phase::held;return true;
    }
    bool same_player(EquipmentSnapshot const& game) const noexcept {
        return game.valid()&&game.world==world_&&game.pawn==pawn_;
    }
public:
    void reset() noexcept {*this=WeaponEquipmentModel{};}
    bool held() const noexcept {return phase_==Phase::held;}
    bool animation_owns_hands() const noexcept {return phase_==Phase::pickup;}
    bool switch_pending() const noexcept {return phase_==Phase::switching;}
    // Visual handoff only. This does not expose a hold generation or authorize
    // fire/inventory. The consumer must also observe the old weapon still
    // current, visible and attached to this player's actual right-hand bone.
    bool departing_for(std::uint64_t world,std::uint64_t pawn,std::uint64_t weapon) const noexcept {
        return phase_==Phase::switching&&previous_weapon_&&world_==world&&pawn_==pawn&&previous_weapon_==weapon;
    }
    std::uint64_t held_weapon() const noexcept {return held()?weapon_:0;}
    std::uint64_t hold_generation() const noexcept {return held()?hold_generation_:0;}
    bool held_for(std::uint64_t world,std::uint64_t pawn,std::uint64_t weapon) const noexcept {
        return held()&&world_==world&&pawn_==pawn&&weapon_==weapon;
    }
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
                if(!become_held())return {EquipmentEvent::interrupted};
                previous_weapon_=0;
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
        // Returning a held weapon to its original slot must still work after
        // the player has spent its last round.
        if(item&&phase_==Phase::held&&slot_==*slot&&weapon_==item&&
           game.current_weapon==item){
            clear_hold();return {EquipmentEvent::request_stow,*slot,item};
        }
        // A grenade actor may outlive its last charge. If the authoritative
        // count cannot be read, fail closed instead of creating a throwable.
        if(!item||(game.ammo_known[*slot]&&!game.ammo_available[*slot]&&
                   !game.can_select_without_ammo[*slot])||
           (*slot==1&&(!game.throwable_count_known||!game.throwable_count)))
            return {EquipmentEvent::empty_slot,*slot};
        if(game.current_weapon==item){
            if(!become_held())return {EquipmentEvent::interrupted};
            slot_=*slot;weapon_=item;previous_weapon_=0;
            return {EquipmentEvent::request_unholster,*slot,item};
        }
        phase_=Phase::switching;hold_generation_=0;slot_=*slot;weapon_=item;
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
        if(!become_held())return {EquipmentEvent::interrupted};
        slot_=inventory_slot;weapon_=game.current_weapon;
        return {EquipmentEvent::equipped,slot_,weapon_};
    }
};
}
