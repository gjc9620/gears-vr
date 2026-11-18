// Per-original-actor mechanical debt, independent of a current hand/clip lease.
// Lookup grants no action or animation rebind. A new hold/input/native context
// still needs its own full current proof and fresh Native part grant.
#include <new>
namespace boomshot_reload_owner {class Owner;}
namespace boomshot_sessions {
namespace hand=gears_weapon_hand;namespace inv=lancerreload_inventory;
struct Slot {
    inv::Identity first{};std::uint64_t generation{};
    bool known{};
private:
    friend class boomshot_reload_owner::Owner;
    gears_weapon::BoomshotInteraction core_{};gears_weapon::BoomshotOwnerEvents events_{};
    bool rebind_idle_source(inv::Identity const& expected,inv::Identity const& next) noexcept {
        gears_weapon::BoomshotInteraction::ReloadRequest request{};
        if(!weaponhand::current_gt(nullptr)||!known||!(first==expected)||!next.valid()||
           core_.held_units()||core_.dispatched_reload_request(request)||
           (core_.stage()!=gears_weapon::BoomshotStage::unbound&&core_.stage()!=gears_weapon::BoomshotStage::ready)||
           next.hold_generation<expected.hold_generation||next.lifecycle<expected.lifecycle)return false;
        auto normalized=next;normalized.hold_generation=expected.hold_generation;normalized.lifecycle=expected.lifecycle;
        if(!(normalized==expected))return false;
        first=next;return true; // Owner separately proved current original closed/counts/grant.
    }
    bool rebind_source(inv::Identity const& expected,inv::Identity const& next) noexcept {
        gears_weapon::BoomshotInteraction::ReloadRequest pending_request{};
        if(!weaponhand::current_gt(nullptr)||!known||!(first==expected)||!next.valid()||
           core_.stage()!=gears_weapon::BoomshotStage::interrupted||core_.held_units()||core_.dispatched_reload_request(pending_request)||
           !hand::same(expected.world,next.world)||!hand::same(expected.pawn,next.pawn)||!hand::same(expected.weapon,next.weapon)||
           expected.game_start!=next.game_start||expected.native_start!=next.native_start||expected.registration!=next.registration||
           expected.world_generation!=next.world_generation||expected.session!=next.session||expected.space!=next.space||expected.input!=next.input)return false;
        // Data only. Owner proves forward fresh hold/lifecycle or an exact
        // undispatched rollback. Core generation, clocks and input floors stay.
        first=next;return true;
    }
    // Only the normal owner may retire data, after it has proved registry
    // death and drained that actor's original resources. Generation allocation
    // stays global/monotonic; a new actor cannot inherit this slot's proofs.
    bool retire_source(inv::Identity const& expected) noexcept {
        gears_weapon::BoomshotInteraction::ReloadRequest request{};
        const auto stage=core_.stage();
        if(!weaponhand::current_gt(nullptr)||!known||!(first==expected)||core_.held_units()||
           core_.dispatched_reload_request(request)||
           (stage!=gears_weapon::BoomshotStage::unbound&&stage!=gears_weapon::BoomshotStage::ready&&stage!=gears_weapon::BoomshotStage::interrupted))return false;
        this->~Slot();::new(static_cast<void*>(this)) Slot{};return true;
    }
public:
    gears_weapon::BoomshotInteraction* core(inv::Identity const& current)noexcept {
        return weaponhand::current_gt(nullptr)&&known&&first==current?&core_:nullptr;
    }
    gears_weapon::BoomshotOwnerEvents* events(inv::Identity const& current)noexcept {
        return weaponhand::current_gt(nullptr)&&known&&first==current?&events_:nullptr;
    }
    bool blocks_fire()const noexcept {
        const auto stage=core_.stage();
        return known&&stage!=gears_weapon::BoomshotStage::unbound&&stage!=gears_weapon::BoomshotStage::ready;
    }
    gears_weapon::BoomshotIdentity identity()const noexcept {
        return {first.world.address,first.pawn.address,first.weapon.address,generation};
    }
};
std::array<Slot,16> slots{};std::uint64_t next_generation{1};
bool same_actor(inv::Identity const& a,inv::Identity const& b) noexcept {
    return a.game_start&&a.game_start==b.game_start&&hand::same(a.world,b.world)&&
        hand::same(a.pawn,b.pawn)&&hand::same(a.weapon,b.weapon);
}
Slot* find(inv::Identity const& id) noexcept {
    for(auto& slot:slots)if(slot.known&&same_actor(slot.first,id))return &slot;return nullptr;
}
Slot* prepare(inv::Identity const& id) noexcept {
    if(!weaponhand::current_gt(nullptr)||!id.valid())return nullptr;
    if(auto* existing=find(id))return existing;
    // Capacity exhaustion cannot erase an old interrupted/open operation.
    if(next_generation==UINT64_MAX)return nullptr;
    for(auto& slot:slots)if(!slot.known){slot.first=id;slot.generation=next_generation++;slot.known=true;return &slot;}
    return nullptr;
}
bool blocks_fire(hand::Frame const& source) noexcept {
    if(!weaponhand::current_gt(nullptr))return true;
    if(auto* slot=find(inv::identity(source)))return slot->blocks_fire();return false;
}
}
