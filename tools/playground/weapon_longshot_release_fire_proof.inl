// Longshot's original ShouldRefire override is admitted ONLY for the common
// owner's existing no-shot state handback, after its independently read R2-up.
bool longshot_should_proof(meta::Reader& r,uptr weapon,uptr pawn,uptr controller,
    playground::Args& call,unsigned& fn,meta::Structure& structure) noexcept {
    unsigned kind{},state{},state_kind{},owner{},owner_kind{},package{};
    if(!call.valid||!r.path(call.r.function,fn)||!r.named(fn,L"ShouldRefire")||
       !r.path(r.result.identities[fn].klass,kind)||!weaponhand::core_type(r,kind,L"Function")||
       !r.path(r.result.identities[fn].outer,state)||!r.named(state,L"WeaponFiring")||
       !r.path(r.result.identities[state].klass,state_kind)||!weaponhand::core_type(r,state_kind,L"State")||
       !r.path(r.result.identities[state].outer,owner)||!r.named(owner,L"Weap_SniperRifle")||
       !r.path(r.result.identities[owner].klass,owner_kind)||!weaponhand::core_type(r,owner_kind,L"Class")||
       !r.path(r.result.identities[owner].outer,package)||!r.named(package,L"WarfareGame")||
       r.result.identities[package].outer||!r.structure(call.r.function,structure))return false;
    std::uint16_t count{},native{};unsigned flags{};uptr exec{};
    if(!r.at(call.r.function,0x106,count)||count!=4||!r.at(call.r.function,0xf8,native)||native||
       !r.at(call.r.function,0xf4,flags)||(flags&0x400)||!r.at(call.r.function,0x114,exec)||exec!=base+0x1d6ca0)return false;
    meta::PointerField instigator{},pawn_controller{};weaponhand::BoolField mode{},fire{};
    if(!weaponhand::pointer(r,weapon,L"Instigator",pawn,instigator)||
       !weaponhand::pointer(r,pawn,L"Controller",controller,pawn_controller)||
       !weaponhand::field(r,weapon,L"CurrentFireMode",L"ByteProperty",1,mode)||
       !weaponhand::field(r,controller,L"bFire",L"ByteProperty",1,fire))return false;
    playground::Args human(pawn,L"IsHumanControlled"),end(weapon,L"EndFire");
    unsigned human_fn{},human_kind{},human_owner{},human_class{},human_package{},end_fn{};
    meta::Structure end_structure{};
    if(!human.valid||!r.path(human.r.function,human_fn)||!r.named(human_fn,L"IsHumanControlled")||
       !r.path(r.result.identities[human_fn].klass,human_kind)||!weaponhand::core_type(r,human_kind,L"Function")||
       !r.path(r.result.identities[human_fn].outer,human_owner)||!r.named(human_owner,L"Pawn")||
       !r.path(r.result.identities[human_owner].klass,human_class)||!weaponhand::core_type(r,human_class,L"Class")||
       !r.path(r.result.identities[human_owner].outer,human_package)||!r.named(human_package,L"Engine")||
       r.result.identities[human_package].outer||!end.valid||
       !weapon_function(r,end.r.function,L"EndFire",end_fn,end_structure,true))return false;
    Script body{};const auto name=r.result.identities[end_fn].name;
    const gears_longshot_initial_fire::ReleaseBindings bindings{
        r.result.identities[instigator.property.identity].address,human.r.function,
        r.result.identities[mode.property.identity].address,r.result.identities[pawn_controller.property.identity].address,
        r.result.identities[fire.property.identity].address,call.prop(L"ReturnValue"),{name.index,name.number}};
    if(!script(r,call.r.function,body)||!gears_longshot_initial_fire::stock_release(body.bytes.data(),
       std::size_t(body.array.count),bindings)||!unified_weapon_fire::retained_metadata(r)||
       !unchanged(r,call.r.function,body))return false;
    return true;
}
