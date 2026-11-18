// Original input cleanup is deliberately separate from a pose/action lease.
// A soft tracking/reference loss revokes ExistingActionLifetime, while our
// already-owned original Charge still needs non-firing cleanup on normal GT.
// Its Native query observes incarnation only; no Frame is renewed or R2 admitted.
namespace torquebow_cleanup {
namespace hand=gears_weapon_hand;namespace meta=gripdamage_metadata;
using Owns=bool(*)(hand::Frame const&,std::uint64_t) noexcept;
class Anchor {
    hand::EquipmentIdentity parent_{};std::uint64_t serial_{};
    bool links(hand::Frame const& f) const noexcept {
        meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
        meta::PointerField pc{},pc_world{},controller{},world{},pawn{},instigator{};
        r.reuse_initial_identities=true; // Discovery within this links query only.
        const bool discovered=meta::tables(base,schema.objects,schema.names)&&weaponhand::object(r,f.world)&&
            weaponhand::object(r,f.controller)&&weaponhand::object(r,f.pawn)&&weaponhand::object(r,f.weapon)&&
            weaponhand::derives(r,f.weapon.address,L"Weap_Bow_base",L"WarfareGame")&&
            weaponhand::pointer(r,f.controller.address,L"Pawn",f.pawn.address,pc)&&
            weaponhand::pointer(r,f.controller.address,L"WorldInfo",f.world.address,pc_world)&&
            weaponhand::pointer(r,f.pawn.address,L"Controller",f.controller.address,controller)&&
            weaponhand::pointer(r,f.pawn.address,L"WorldInfo",f.world.address,world)&&
            weaponhand::pointer(r,f.pawn.address,L"Weapon",f.weapon.address,pawn)&&
            weaponhand::pointer(r,f.weapon.address,L"Instigator",f.pawn.address,instigator);
        r.reuse_initial_identities=false;
        if(!discovered)return false;
        for(auto const* field:{&pc,&pc_world,&controller,&world,&pawn,&instigator}){
            if(schema.field_count==schema.fields.size())return false;
            schema.fields[schema.field_count++]=field->property;
        }
        if(!unified_weapon_fire::retained_metadata(r))return false;
        uptr current_pc{},current_pc_world{},current_controller{},current_world{},current_weapon{},current_instigator{};
        return r.at(f.controller.address,pc.property.offset,current_pc)&&current_pc==f.pawn.address&&
            r.at(f.controller.address,pc_world.property.offset,current_pc_world)&&current_pc_world==f.world.address&&
            r.at(f.pawn.address,controller.property.offset,current_controller)&&current_controller==f.controller.address&&
            r.at(f.pawn.address,world.property.offset,current_world)&&current_world==f.world.address&&
            r.at(f.pawn.address,pawn.property.offset,current_weapon)&&current_weapon==f.weapon.address&&
            r.at(f.weapon.address,instigator.property.offset,current_instigator)&&current_instigator==f.pawn.address;
    }
public:
    bool capture_before_press(hand::Frame const& f,std::uint64_t serial,Owns owns) noexcept {
        if(serial_||!serial||!owns||!owns(f,serial)||!weaponhand::current_gt(nullptr)||
           !nativeequipment::current(nullptr,f)||!links(f))return false;
        hand::ExistingActionLifetime fresh{};
        if(!weaponhand::client.capture_existing_action_lifetime_on_gt(f.key,fresh)||
           !hand::equipment_matches(f,fresh.parent)||!torquebow_cleanup_context::current(fresh.parent)||
           !owns(f,serial)||!nativeequipment::current(nullptr,f))return false;
        // Store ancestry/serial only. The old pose epoch is not a cleanup
        // permission, and no future source can replace these identities.
        parent_=fresh.parent;serial_=serial;return true;
    }
    bool current(hand::Frame const& owned_source,std::uint64_t serial,Owns owns) const noexcept {
        return serial_&&serial==serial_&&owns&&owns(owned_source,serial)&&
            hand::equipment_matches(owned_source,parent_)&&weaponhand::current_gt(nullptr)&&
            owned_source.key.game_pid==GetCurrentProcessId()&&
            weaponhand::client.registration_alive(parent_.key.registration_cookie,parent_.key.native_started_qpc)&&
            torquebow_cleanup_context::current(parent_)&&
            nativeequipment::restoration_quiet()&&links(owned_source)&&owns(owned_source,serial)&&links(owned_source)&&
            weaponhand::client.registration_alive(parent_.key.registration_cookie,parent_.key.native_started_qpc)&&
            torquebow_cleanup_context::current(parent_);
    }
    std::uint64_t serial() const noexcept {return serial_;}
    // Only the journal owner may discard an UNDISPATCHED reservation or a
    // fully observed cleanup. A rejected/unknown call never invokes reset.
    bool retire(std::uint64_t serial,bool no_original_debt) noexcept {
        if(!serial_||serial!=serial_||!no_original_debt)return false;*this=Anchor{};return true;
    }
};
}
