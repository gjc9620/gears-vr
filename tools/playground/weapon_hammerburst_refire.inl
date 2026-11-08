// HB's original firing state has its own RefireCheckTimer. Only its exact
// read-only contract is adapted here; the existing event/input owner dispatches.
namespace hammerburst_refire {
bool receiver(meta::Reader& r,uptr weapon) noexcept {
    return lancermain_asset::held_asset_name(r,weapon,lancermain_asset::AssetScope::supported_instant_fire)==
        lancermain_asset::HeldAsset::hammerburst;
}
bool state_owner(meta::Reader& r,unsigned state,unsigned& owner) noexcept {
    unsigned type{},owner_type{},package{};
    return r.named(state,L"WeaponFiring")&&r.path(r.result.identities[state].klass,type)&&
        weaponhand::core_type(r,type,L"State")&&r.path(r.result.identities[state].outer,owner)&&
        r.named(owner,L"Weap_LocustAssaultRifle")&&r.path(r.result.identities[owner].klass,owner_type)&&
        weaponhand::core_type(r,owner_type,L"Class")&&r.path(r.result.identities[owner].outer,package)&&
        r.named(package,L"WarfareGame")&&!r.result.identities[package].outer;
}
bool candidate(meta::Reader& r,uptr weapon,uptr function) noexcept {
    unsigned fn{},type{},state{},owner{};
    return r.path(function,fn)&&r.named(fn,L"RefireCheckTimer")&&
        r.path(r.result.identities[fn].klass,type)&&weaponhand::core_type(r,type,L"Function")&&
        r.path(r.result.identities[fn].outer,state)&&state_owner(r,state,owner)&&receiver(r,weapon);
}
bool vm_shape(meta::Reader& r,uptr function,unsigned bytes,unsigned output) noexcept {
    std::uint16_t parameters{},result{},native{};unsigned flags{};uptr exec{};
    return r.at(function,0x106,parameters)&&parameters==bytes&&r.at(function,0x108,result)&&result==output&&
        r.at(function,0xf8,native)&&!native&&r.at(function,0xf4,flags)&&!(flags&0x400)&&
        r.at(function,0x114,exec)&&exec==base+0x1d6ca0;
}
bool original_loop(meta::Reader& r,uptr weapon,uptr function,unsigned& reason) noexcept {
    reason=1;unsigned fn{},state{},owner{};meta::Structure structure{};
    if(!candidate(r,weapon,function)||!r.path(function,fn)||
       !r.path(r.result.identities[fn].outer,state)||!state_owner(r,state,owner)||
       !r.structure(function,structure))return false;
    reason=2;std::uint16_t parameters{},result{},native{};unsigned flags{};uptr exec{};
    if(!r.at(function,0x106,parameters)||parameters||!r.at(function,0x108,result)||result!=0xffff||
       !r.at(function,0xf8,native)||native||!r.at(function,0xf4,flags)||(flags&0x400)||
       !r.at(function,0x114,exec)||exec!=base+0x1d6ca0||structure.children||structure.extent)return false;
    // Reasons3+ have proved this exact zero-parameter original VM entry.
    reason=31;weaponhand::BoolField down{},targeting{};
    if(!weaponhand::boolean(r,weapon,L"bWeaponPutDown",down)||
       !weaponhand::boolean(r,weapon,L"bWasTargeting",targeting)||
       r.result.identities[targeting.property.identity].outer!=r.result.identities[owner].address)return false;
    playground::Args put(weapon,L"PutDownWeapon"),should(weapon,L"ShouldRefire"),ammo(weapon,L"FireAmmunition"),
        timing(weapon,L"TimeWeaponFiring");
    unsigned put_slot{},should_slot{},ammo_slot{},timing_slot{},should_owner{},ammo_owner{},should_type{},ammo_type{};
    meta::Structure put_structure{},should_structure{},ammo_structure{},timing_structure{};
    reason=32;
    if(!put.valid||!should.valid||!ammo.valid||!timing.valid||
       !weapon_function(r,put.r.function,L"PutDownWeapon",put_slot,put_structure)||
       !r.path(should.r.function,should_slot)||!r.named(should_slot,L"ShouldRefire")||
       !r.path(r.result.identities[should_slot].klass,should_type)||!weaponhand::core_type(r,should_type,L"Function")||
       !r.path(r.result.identities[should_slot].outer,should_owner)||should_owner!=owner||
       !r.structure(should.r.function,should_structure)||
       !r.path(ammo.r.function,ammo_slot)||!r.named(ammo_slot,L"FireAmmunition")||
       !r.path(r.result.identities[ammo_slot].klass,ammo_type)||!weaponhand::core_type(r,ammo_type,L"Function")||
       !r.path(r.result.identities[ammo_slot].outer,ammo_owner)||ammo_owner!=owner||
       !r.structure(ammo.r.function,ammo_structure)||
       !weapon_function(r,timing.r.function,L"TimeWeaponFiring",timing_slot,timing_structure))return false;
    const auto return_field=should.prop(L"ReturnValue");meta::Property returned{};
    if(should.r.size!=4||ammo.r.size||ammo_structure.children||ammo_structure.extent||
       !vm_shape(r,should.r.function,4,0)||!vm_shape(r,ammo.r.function,0,0xffff)||
       !return_field||!r.property(return_field,returned)||
       r.result.identities[returned.identity].outer!=should.r.function||
       !weaponhand::core_type(r,returned.type,L"BoolProperty")||returned.dimension!=1||
       returned.element_bytes!=4||returned.offset||(returned.flags&0x580)!=0x580||
       !meta::property_fits(returned,should.r.size)||!meta::property_fits(returned,should_structure.extent))return false;
    const auto burst_address=playground::field(r.result.identities[owner].address,L"BurstDelaying",L"State");
    unsigned burst{},burst_type{};
    reason=33;
    if(!burst_address||!r.path(burst_address,burst)||!r.named(burst,L"BurstDelaying")||
       r.result.identities[burst].outer!=r.result.identities[owner].address||
       !r.path(r.result.identities[burst].klass,burst_type)||!weaponhand::core_type(r,burst_type,L"State"))return false;
    Script body{};reason=34;
    if(!script(r,function,body))return false;
    const auto name=[&](unsigned slot){const auto n=r.result.identities[slot].name;
        return gears_hammerburst_refire::Name{n.index,n.number};};
    if(!gears_hammerburst_refire::timer(body.bytes.data(),std::size_t(body.array.count),
       r.result.identities[down.property.identity].address,r.result.identities[targeting.property.identity].address,
       name(put_slot),name(should_slot),name(ammo_slot),name(burst)))return false;
    // Reuse the unchanged Engine timer-producer certificate with this actual
    // receiver. Its virtual RefireCheckTimer name dispatches the HB override;
    // neither the original recurring timer nor its interval is rewritten.
    unsigned engine_weapon{},engine_state{};
    reason=35;
    if(!r.path(r.result.identities[timing_slot].outer,engine_weapon))return false;
    const auto engine_state_address=playground::field(r.result.identities[engine_weapon].address,L"WeaponFiring",L"State");
    if(!engine_state_address||!r.path(engine_state_address,engine_state))return false;
    const auto engine_timer=playground::field(engine_state_address,L"RefireCheckTimer",L"Function");
    unsigned engine_reason{};
    if(!engine_timer||engine_timer==function||!lancerrefire::original_loop(r,weapon,engine_timer,engine_reason))return false;
    reason=36;
    if(!unified_weapon_fire::retained_metadata(r)||!unchanged(r,function,body))return false;
    reason=0;return true;
}
// Preserve the old Lancer-only selection exactly; only HB adds a new route.
bool supported_receiver(meta::Reader& r,uptr weapon) noexcept {
    return lancermain_asset::held_asset_name(r,weapon,lancermain_asset::AssetScope::lancer_only)==
        lancermain_asset::HeldAsset::lancer||receiver(r,weapon);
}
}
