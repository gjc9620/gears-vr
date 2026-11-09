// Normal-GT cancellation of a previously owned ORIGINAL HB burst. No pose,
// button, ammunition, BurstCount, VM-frame or timer memory is written here.
// This adapter is deliberately not wired into the event owner until the HB
// finite-completion and first-event paths can be enabled together.
namespace hammerburst_burst_cancel {
namespace meta=gripdamage_metadata;
namespace hand=gears_weapon_hand;
template<std::size_t N,std::size_t P>
bool owner(meta::Reader& r,unsigned slot,wchar_t const(&label)[N],wchar_t const(&package_name)[P]) noexcept {
    unsigned kind{},package{};
    return r.named(slot,label)&&r.path(r.result.identities[slot].klass,kind)&&
        weaponhand::core_type(r,kind,L"Class")&&r.path(r.result.identities[slot].outer,package)&&
        r.named(package,package_name)&&!r.result.identities[package].outer;
}
// Check ALL actual PendingFire entries, rather than equating the controller's
// visible R2 flag with an empty original weapon-input queue. Closing reads use
// the same pointer, property/inner layouts, allocation and bytes.
bool pending_up(meta::Reader& r,uptr weapon) noexcept {
    meta::PointerField manager{},manager_after{};weaponhand::BoolField field{},field_after{};
    if(!weaponhand::read_pointer(r,weapon,L"InvManager",manager)||!manager.value||
       !owner(r,manager.owner,L"Inventory",L"Engine")||
       !weaponhand::derives(r,manager.value,L"InventoryManager",L"Engine")||
       !weaponhand::field(r,manager.value,L"PendingFire",L"ArrayProperty",16,field)||
       !owner(r,field.owner,L"InventoryManager",L"Engine"))return false;
    const auto property=r.result.identities[field.property.identity].address;
    uptr inner_address{},inner_after{};meta::Property inner{},inner_again{};
    if(!r.at(property,0xd0,inner_address)||!inner_address||!r.property(inner_address,inner)||
       r.result.identities[inner.identity].outer!=property||!weaponhand::core_type(r,inner.type,L"IntProperty")||
       inner.dimension!=1||inner.element_bytes!=4||inner.offset)return false;
    Array array{},again{};std::array<std::int32_t,64> bytes{},closing{};
    if(!r.at(manager.value,field.property.offset,array)||array.count<0||array.capacity<array.count||
       array.capacity>int(bytes.size())||(!array.data&&(array.count||array.capacity))||
       (array.count&&!copy(bytes.data(),array.data,std::size_t(array.count)*sizeof(bytes[0]))))return false;
    for(int i=0;i<array.count;++i)if(bytes[std::size_t(i)])return false;
    if(!weaponhand::read_pointer(r,weapon,L"InvManager",manager_after)||manager_after.value!=manager.value||
       manager_after.owner!=manager.owner||!meta::same_property(manager.property,manager_after.property)||
       !weaponhand::field(r,manager.value,L"PendingFire",L"ArrayProperty",16,field_after)||
       field.owner!=field_after.owner||!meta::same_property(field.property,field_after.property)||
       !r.at(property,0xd0,inner_after)||inner_after!=inner_address||!r.property(inner_address,inner_again)||
       !meta::same_property(inner,inner_again)||!r.at(manager.value,field.property.offset,again)||
       std::memcmp(&array,&again,sizeof array)||
       (array.count&&(!copy(closing.data(),array.data,std::size_t(array.count)*sizeof(bytes[0]))||bytes!=closing)))return false;
    return unified_weapon_fire::retained_metadata(r);
}
bool goto_layout(meta::Reader& r,playground::Args& call) noexcept {
    unsigned fn{},kind{},owned{};meta::Structure structure{};
    std::uint16_t native{},result{};
    if(!weapon_part_assets::native_function(call,L"GotoState",L"Object",L"Core")||
       !r.path(call.r.function,fn)||!r.path(r.result.identities[fn].klass,kind)||
       !weaponhand::core_type(r,kind,L"Function")||!r.path(r.result.identities[fn].outer,owned)||
       !owner(r,owned,L"Object",L"Core")||!r.structure(call.r.function,structure)||
       !r.at(call.r.function,0xf8,native)||native!=113||call.r.size!=24||
       !r.at(call.r.function,0x108,result)||result!=0xffff||structure.extent!=24)return false;
    const auto parameter=[&](auto const& label,auto const& type,unsigned bytes,unsigned offset) noexcept {
        meta::Property p{};const auto address=call.prop(label);
        if(!(address&&r.property(address,p)&&r.result.identities[p.identity].outer==call.r.function&&
            r.named(p.identity,label)&&weaponhand::core_type(r,p.type,type)&&p.dimension==1&&
            p.element_bytes==bytes&&p.offset==offset&&(p.flags&0x590)==0x90&&
            meta::property_fits(p,call.r.size)&&meta::property_fits(p,structure.extent)))return false;
        if(r.result.field_count==meta::max_fields)return false;
        r.result.fields[r.result.field_count++]=p;return true;
    };
    return parameter(L"NewState",L"NameProperty",8,0)&&parameter(L"Label",L"NameProperty",8,8)&&
        parameter(L"bForceEvents",L"BoolProperty",4,16)&&parameter(L"bKeepStack",L"BoolProperty",4,20)&&
        unified_weapon_fire::retained_metadata(r);
}
// Caller owns the completion debt and must retain it when this returns false.
// An already ended original Firing state is accepted only with PendingFire UP.
bool complete(weaponhand::HoldBinding const& held) noexcept {
    if(!weaponhand::current_gt(nullptr)||!held.known||
       !lancerrefire::same_hold(held,weaponhand::held_binding)||
       !weaponhand::client.registration_alive(held.native_cookie,held.native_started))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,held.world)||
       !weaponhand::object(r,held.controller)||!weaponhand::object(r,held.pawn)||!weaponhand::object(r,held.weapon)||
       !lancerrefire::hammerburst_refire::receiver(r,held.weapon.address))return false;
    vrfire::lr::Game game{};
    if(!vrfire::read_game(vrfire::input.actors(),nullptr,game)||!game.known||!game.r2_known||game.r2_held||
       !hand::same(game.actors.world,held.world)||!hand::same(game.actors.controller,held.controller)||
       !hand::same(game.actors.pawn,held.pawn)||!hand::same(game.actors.weapon,held.weapon)||
       !pending_up(r,held.weapon.address))return false;
    Name current{};meta::NameText text{};
    if(!lancerrefire::state_name(r,held.weapon.address,current)||
       !meta::fresh_name(schema.names,current.index,text))return false;
    if(!meta::exact_text(text,L"WeaponFiring"))return true;
    const auto state=playground::field(held.weapon.klass,L"BurstDelaying",L"State");
    unsigned state_slot{},state_type{},state_owner{};
    if(!state||!r.path(state,state_slot)||!r.named(state_slot,L"BurstDelaying")||
       !r.path(schema.identities[state_slot].klass,state_type)||!weaponhand::core_type(r,state_type,L"State")||
       !r.path(schema.identities[state_slot].outer,state_owner)||
       !owner(r,state_owner,L"Weap_LocustAssaultRifle",L"WarfareGame"))return false;
    playground::Args go(held.weapon.address,L"GotoState");
    if(!goto_layout(r,go)||!pending_up(r,held.weapon.address)||
       !lancerrefire::same_hold(held,weaponhand::held_binding)||!weaponhand::object(r,held.weapon))return false;
    const auto destination=schema.identities[state_slot].name;
    Name name{destination.index,destination.number};
    if(!go.set(L"NewState",name).valid||!go.run()||
       !lancerrefire::state_name(r,held.weapon.address,current)||current.index!=name.index||current.number!=name.number||
       !pending_up(r,held.weapon.address))return false;
    return true; // Normal Firing.EndState cleared the original refire timer.
}
}
