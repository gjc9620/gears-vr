// Identity-only metadata carrier; original V1 logic and entry remain unchanged.
namespace weaponhand {
bool observe_equipment(abi::EquipmentIdentity const& f,abi::GripGameState& out) noexcept {
#if defined(GEARS_PLAYGROUND_RESTORE_STAGE_CORE)
    if(!weaponrestore::healthy())return false;
#endif
    out={};if(!enabled.load(std::memory_order_acquire)||!current_gt(nullptr))return false;
#if defined(GEARS_PLAYGROUND_WEAPON_MESH_FOLLOW_TEST)
    // The old trial has its own support latch. Never report an empty hand
    // while that independent driver is active; migrate it before coexistence.
    if(vrequip::mesh_follow_active_visible.load(std::memory_order_acquire))return false;
#endif
#if defined(GEARS_PLAYGROUND_LANCER_MAGAZINE_FOLLOW_TEST)
    if(vrequip::lancer_magazine_active_visible.load(std::memory_order_acquire))return false;
#endif
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names)||!object(r,f.world)||!object(r,f.controller)||
       !object(r,f.pawn)||!object(r,f.mesh)||!object(r,f.weapon)||
       !derives(r,f.controller.address,L"WarPC",L"WarfareGame")||
       !derives(r,f.pawn.address,L"WarPawn",L"WarfareGame")||
       !derives(r,f.world.address,L"WorldInfo",L"Engine")||
       !derives(r,f.mesh.address,L"SkeletalMeshComponent",L"Engine"))return false;
    // Same query-local grouped field walks as the action-Frame reader. This
    // metadata-only callback can run repeatedly during one body composition;
    // do not separately rescan the controller/weapon hierarchy for each link
    // and delete flag. Every value and the original closing rereads below
    // remain current; no identity, pose, permission or field is cached.
    std::array<meta::PointerField,6> links{};PawnObserveFields pawn_fields;
    ControllerObserveFields controller_fields;WeaponObserveFields weapon_fields;
    if(!controller_fields.begin(r,f.controller.address)||
       !controller_fields.pointer(r,ControllerObserveFields::pawn_link,f.pawn.address,links[0])||
       !controller_fields.pointer(r,ControllerObserveFields::world_link,f.world.address,links[1])||
       !pawn_fields.begin(r,f.pawn.address)||
       !pawn_fields.pointer(r,PawnObserveFields::controller_link,f.controller.address,links[2])||
       !pawn_fields.pointer(r,PawnObserveFields::mesh_link,f.mesh.address,links[3])||
       !pawn_fields.pointer(r,PawnObserveFields::weapon_link,f.weapon.address,links[4]))return false;
    if(f.weapon.address&&(!derives(r,f.weapon.address,L"WarWeapon",L"WarfareGame")||
       !weapon_fields.begin(r,f.weapon.address)||
       !weapon_fields.pointer(r,WeaponObserveFields::instigator_link,f.pawn.address,links[5])))return false;
    std::array<BoolField,6> flags{};
    const std::array<uptr,4> actors{f.world.address,f.controller.address,f.pawn.address,f.weapon.address};
    for(std::size_t i=0;i<actors.size();++i)if(actors[i]&&
       (!(i==2?pawn_fields.boolean(r,PawnObserveFields::deleted_flag,flags[i]):
          i==1?controller_fields.boolean(r,ControllerObserveFields::deleted_flag,flags[i]):
          i==3?weapon_fields.boolean(r,WeaponObserveFields::deleted_flag,flags[i]):boolean(r,actors[i],L"bDeleteMe",flags[i]))||
        (flags[i].bits&flags[i].mask)))return false;
    if(!pawn_fields.boolean(r,PawnObserveFields::switching_flag,flags[4])||
       !pawn_fields.boolean(r,PawnObserveFields::no_firing_flag,flags[5]))return false;
    abi::GripGameState game{};game.known=true;
    game.main_hand_held=f.weapon.address&&vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)&&
        held_binding.known&&held_binding.generation&&held_binding.generation==vrequip::model.hold_generation()&&
        abi::same(held_binding.world,f.world)&&abi::same(held_binding.controller,f.controller)&&
        abi::same(held_binding.pawn,f.pawn)&&abi::same(held_binding.weapon,f.weapon);
#if defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_TRIAL)
    game.main_hand_held=game.main_hand_held&&held_binding.native_cookie==f.key.registration_cookie&&
        held_binding.native_started==f.key.native_started_qpc&&held_binding.world_generation==f.key.world_generation;
#endif
    game.animation_owns_hands=vrequip::model.animation_owns_hands();
    game.busy=vrequip::model.switch_pending()||(flags[4].bits&flags[4].mask)||(flags[5].bits&flags[5].mask);
    if(f.weapon.address){bool known{};
        const auto grenade=derives(r,f.weapon.address,L"Weap_Grenade",L"WarfareGame",&known);
        if(!known)return false;
        game.kind=grenade?abi::WeaponKind::hand_thrown:abi::WeaponKind::firearm;
    }
    // Until an actual per-asset controller/palm mount is certified, no support
    // candidate is invented from temporary mesh-follow calibration.
    game.mount_known=false;
    // Close the local search binding before all original final rereads.
    if(!pawn_fields.finish(r))return false;
    const auto identities=schema.identity_count,structures=schema.structure_count;
    for(U32 i=0;i<identities;++i){U32 slot{};if(!r.identity(schema.identities[i].address,slot)||slot!=i)return false;}
    for(U32 i=0;i<structures;++i){meta::Structure fresh{};
        if(!r.structure(schema.identities[schema.structures[i].identity].address,fresh)||
           !meta::same_structure(schema.structures[i],fresh))return false;
    }
    for(auto const& link:links)if(link.property.element_bytes){
        meta::Property fresh{};
        if(!r.property(schema.identities[link.property.identity].address,fresh)||!meta::same_property(fresh,link.property))return false;
    }
    const std::array<uptr,6> link_objects{f.controller.address,f.controller.address,f.pawn.address,
        f.pawn.address,f.pawn.address,f.weapon.address};
    for(std::size_t i=0;i<links.size();++i)if(links[i].property.element_bytes){uptr value{};
        if(!r.at(link_objects[i],links[i].property.offset,value)||value!=links[i].value)return false;
    }
    for(auto const& flag:flags)if(flag.mask){meta::Property fresh{};U32 mask{},bits{};
        const auto property=schema.identities[flag.property.identity].address;
        if(!r.property(property,fresh)||!meta::same_property(fresh,flag.property)||!r.at(property,0xd0,mask)||mask!=flag.mask||
           !r.at(flag.object,flag.property.offset,bits)||(bits&mask)!=(flag.bits&mask))return false;
    }
    meta::Table objects{},names{};
    if(!meta::tables(base,objects,names)||!meta::same_table(objects,schema.objects)||!meta::same_table(names,schema.names))return false;
    out=game;return true;
}
}
