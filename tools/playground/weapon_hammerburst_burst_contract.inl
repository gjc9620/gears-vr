// Additional original HB finite-burst certificate. Included after the common
// original-event owner; this has no hook, callback, timer or dispatch owner.
namespace hammerburst_burst_contract {
namespace meta=gripdamage_metadata;
namespace hand=gears_weapon_hand;
struct Certificate {
    weaponhand::HoldBinding held{};
    meta::Property count{},limit{};
    uptr count_property{},limit_property{};
    hand::ObjectIdentity timer{},should{},ammunition{},human{};
    bool known{};
};
template<std::size_t N,std::size_t P>
bool original_owner(meta::Reader& r,unsigned slot,wchar_t const(&label)[N],wchar_t const(&package)[P]) noexcept {
    return hammerburst_burst_cancel::owner(r,slot,label,package);
}
bool retain(meta::Reader& r,meta::Property const& p) noexcept {
    if(r.result.field_count==meta::max_fields)return false;
    r.result.fields[r.result.field_count++]=p;return true;
}
// Native getter invocation is confined to normal GT preflight, never a fire
// observer. It distinguishes the original local-human finite branch from AI.
bool human(meta::Reader& r,uptr pawn,playground::Args& query,unsigned& fn) noexcept {
    unsigned type{},owner{};meta::Property result{};meta::Structure structure{};
    const auto address=query.prop(L"ReturnValue");std::uint16_t output{};
    unsigned mask{},flags{};
    if(!query.valid||query.r.object!=pawn||query.r.size!=4||!address||
       !r.path(query.r.function,fn)||!r.named(fn,L"IsHumanControlled")||
       !r.path(r.result.identities[fn].klass,type)||!weaponhand::core_type(r,type,L"Function")||
       !r.path(r.result.identities[fn].outer,owner)||!original_owner(r,owner,L"Pawn",L"Engine")||
       !r.at(query.r.function,0xf4,flags)||!(flags&0x400)||
       !weapon_part_assets::native_function(query,L"IsHumanControlled",L"Pawn",L"Engine")||
       !r.at(query.r.function,0x108,output)||output||!r.structure(query.r.function,structure)||
       !r.property(address,result)||r.result.identities[result.identity].outer!=query.r.function||
       !r.named(result.identity,L"ReturnValue")||!weaponhand::core_type(r,result.type,L"BoolProperty")||
       result.dimension!=1||result.element_bytes!=4||result.offset||(result.flags&0x580)!=0x580||
       !meta::property_fits(result,4)||!meta::property_fits(result,structure.extent)||
       !r.at(address,0xd0,mask)||!mask||(mask&(mask-1))||!retain(r,result)||
       !unified_weapon_fire::retained_metadata(r)||!query.run())return false;
    unsigned mask_after{};
    return (query.output<unsigned>()&mask)&&r.at(address,0xd0,mask_after)&&mask==mask_after&&
        unified_weapon_fire::retained_metadata(r);
}
bool prepare(weaponhand::HoldBinding const& held,uptr timer,Certificate& out) noexcept {
    out={};
    if(!held.known||!weaponhand::current_gt(nullptr)||
       !lancerrefire::same_hold(held,weaponhand::held_binding))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    unsigned reason{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,held.world)||
       !weaponhand::object(r,held.controller)||!weaponhand::object(r,held.pawn)||!weaponhand::object(r,held.weapon)||
       !lancerrefire::hammerburst_refire::original_loop(r,held.weapon.address,timer,reason))return false;
    weaponhand::BoolField count{},limit{},mode{};meta::PointerField instigator{},controller{};
    if(!weaponhand::field(r,held.weapon.address,L"BurstCount",L"ByteProperty",1,count)||
       !weaponhand::field(r,held.weapon.address,L"NumBurstShots",L"ByteProperty",1,limit)||
       count.owner!=limit.owner||!original_owner(r,count.owner,L"Weap_LocustAssaultRifle",L"WarfareGame")||
       !weaponhand::field(r,held.weapon.address,L"CurrentFireMode",L"ByteProperty",1,mode)||
       !original_owner(r,mode.owner,L"Weapon",L"Engine")||
       !weaponhand::pointer(r,held.weapon.address,L"Instigator",held.pawn.address,instigator)||
       !original_owner(r,instigator.owner,L"Actor",L"Engine")||
       !weaponhand::pointer(r,held.pawn.address,L"Controller",held.controller.address,controller)||
       !original_owner(r,controller.owner,L"Pawn",L"Engine"))return false;
    playground::Args should(held.weapon.address,L"ShouldRefire"),ammo(held.weapon.address,L"FireAmmunition"),
        has_ammo(held.weapon.address,L"HasAmmo"),is_human(held.pawn.address,L"IsHumanControlled");
    unsigned should_slot{},ammo_slot{},has_slot{},human_slot{},super_should_slot{},super_ammo_slot{},timer_slot{};
    meta::Structure hb{},super{},super_should_structure{},super_ammo_structure{},has_structure{};
    if(!should.valid||!ammo.valid||!has_ammo.valid||!r.path(should.r.function,should_slot)||
       !r.path(ammo.r.function,ammo_slot)||!r.path(timer,timer_slot)||
       r.result.identities[should_slot].outer!=r.result.identities[count.owner].address||
       r.result.identities[ammo_slot].outer!=r.result.identities[count.owner].address||
       !r.structure(r.result.identities[count.owner].address,hb)||!r.structure(hb.super,super)||
       !original_owner(r,super.identity,L"WarWeapon",L"WarfareGame")||
       !lancerrefire::weapon_function(r,has_ammo.r.function,L"HasAmmo",has_slot,has_structure,true))return false;
    const auto super_should=playground::field(hb.super,L"ShouldRefire",L"Function");
    const auto super_ammo=playground::field(hb.super,L"FireAmmunition",L"Function");
    if(!super_should||!super_ammo||
       !lancerrefire::weapon_function(r,super_should,L"ShouldRefire",super_should_slot,super_should_structure,true)||
       !lancerrefire::weapon_function(r,super_ammo,L"FireAmmunition",super_ammo_slot,super_ammo_structure,true)||
       r.result.identities[super_should_slot].outer!=hb.super||r.result.identities[super_ammo_slot].outer!=hb.super)return false;
    meta::Property returned{};const auto return_address=should.prop(L"ReturnValue");
    if(!return_address||!r.property(return_address,returned))return false; // Full shape checked by original_loop above.
    lancerrefire::Script should_body{},ammo_body{};
    const auto ammo_name=r.result.identities[has_slot].name;
    const gears_hammerburst_refire::ShouldBindings bindings{
        r.result.identities[instigator.property.identity].address,is_human.r.function,super_should,
        r.result.identities[mode.property.identity].address,r.result.identities[count.property.identity].address,
        r.result.identities[limit.property.identity].address,return_address,{ammo_name.index,ammo_name.number}};
    if(!human(r,held.pawn.address,is_human,human_slot)||
       !lancerrefire::script(r,should.r.function,should_body)||!lancerrefire::script(r,ammo.r.function,ammo_body)||
       !gears_hammerburst_refire::should_refire(should_body.bytes.data(),std::size_t(should_body.array.count),bindings)||
       !gears_hammerburst_refire::ammunition(ammo_body.bytes.data(),std::size_t(ammo_body.array.count),bindings.count,super_ammo)||
       !retain(r,count.property)||!retain(r,limit.property)||!retain(r,mode.property)||!retain(r,instigator.property)||
       !retain(r,controller.property)||!retain(r,returned))return false;
    unsigned char before_count{},before_limit{},before_mode{},after_count{},after_limit{},after_mode{};
    if(!r.at(held.weapon.address,count.property.offset,before_count)||before_count>6||
       !r.at(held.weapon.address,limit.property.offset,before_limit)||before_limit!=6||
       !r.at(held.weapon.address,mode.property.offset,before_mode)||before_mode||
       !lancerrefire::unchanged(r,should.r.function,should_body)||!lancerrefire::unchanged(r,ammo.r.function,ammo_body)||
       !unified_weapon_fire::retained_metadata(r)||
       !r.at(held.weapon.address,count.property.offset,after_count)||before_count!=after_count||
       !r.at(held.weapon.address,limit.property.offset,after_limit)||before_limit!=after_limit||
       !r.at(held.weapon.address,mode.property.offset,after_mode)||before_mode!=after_mode||
       !weaponhand::pointer(r,held.weapon.address,L"Instigator",held.pawn.address,instigator)||
       !weaponhand::pointer(r,held.pawn.address,L"Controller",held.controller.address,controller)||
       !weaponhand::object(r,held.weapon)||!lancerrefire::same_hold(held,weaponhand::held_binding))return false;
    out.held=held;out.count=count.property;out.limit=limit.property;
    out.count_property=bindings.count;out.limit_property=bindings.limit;
    out.timer=lancerrefire::identity(r,timer_slot);out.should=lancerrefire::identity(r,should_slot);
    out.ammunition=lancerrefire::identity(r,ammo_slot);out.human=lancerrefire::identity(r,human_slot);
    out.known=true;return true;
}
}
