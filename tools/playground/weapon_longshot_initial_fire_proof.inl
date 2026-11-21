// Included inside lancerrefire, after its unchanged original initial_proof.
// Reuses that owner's event/cohort/deadline/claim and normal-GT dispatcher.
// No Longshot queue, timer, input edge or second firing owner is introduced.
bool longshot_initial_proof(meta::Reader& r,uptr weapon,uptr function,
    Script& body,unsigned& bytes,uptr& refire,unsigned& reason) noexcept {
    unsigned fn{},kind{},state{},state_kind{},owner{},owner_kind{},package{};
    reason=11;
    if(!r.path(function,fn)||!r.named(fn,L"BeginState")||
       !r.path(r.result.identities[fn].klass,kind)||!weaponhand::core_type(r,kind,L"Function")||
       !r.path(r.result.identities[fn].outer,state)||!r.named(state,L"WeaponFiring")||
       !r.path(r.result.identities[state].klass,state_kind)||!weaponhand::core_type(r,state_kind,L"State")||
       !r.path(r.result.identities[state].outer,owner)||!r.named(owner,L"Weap_SniperRifle")||
       !r.path(r.result.identities[owner].klass,owner_kind)||!weaponhand::core_type(r,owner_kind,L"Class")||
       !r.path(r.result.identities[owner].outer,package)||!r.named(package,L"WarfareGame")||
       r.result.identities[package].outer)return false;
    reason=12;meta::Structure structure{};std::uint16_t count{},result{},native{};
    unsigned flags{};uptr exec{};
    if(!r.structure(function,structure)||!r.at(function,0x106,count)||(count!=0&&count!=8)||
       !r.at(function,0x108,result)||result!=0xffff||!r.at(function,0xf8,native)||native||
       !r.at(function,0xf4,flags)||(flags&0x400)||!r.at(function,0x114,exec)||exec!=base+0x1d6ca0)return false;
    reason=13;meta::Property previous{};
    const auto field=playground::field(function,L"PreviousStateName");
    if(!field||!r.property(field,previous)||!r.named(previous.identity,L"PreviousStateName")||
       r.result.identities[previous.identity].outer!=function||!weaponhand::core_type(r,previous.type,L"NameProperty")||
       previous.dimension!=1||previous.element_bytes!=8||previous.offset!=0||
       !meta::property_fits(previous,structure.extent)||(count==8&&(previous.flags&0x580)!=0x80)||
       (count==0&&(previous.flags&0x580))||structure.children!=field||get<uptr>(field+0x68))return false;
    reason=14;unsigned feedback{},feedback_kind{};
    const auto feedback_function=playground::field(r.result.identities[owner].address,
        L"DelayedWeaponStopPS5TriggerFX",L"Function");
    if(!feedback_function||!r.path(feedback_function,feedback)||
       !r.named(feedback,L"DelayedWeaponStopPS5TriggerFX")||r.result.identities[feedback].outer!=r.result.identities[owner].address||
       !r.path(r.result.identities[feedback].klass,feedback_kind)||!weaponhand::core_type(r,feedback_kind,L"Function")||
       !script(r,function,body)||body.array.count!=41)return false;
    uptr parent{};std::memcpy(&parent,body.bytes.data()+1,sizeof parent);
    const auto feedback_name=r.result.identities[feedback].name;
    reason=15;
    if(!gears_longshot_initial_fire::stock_wrapper(body.bytes.data(),std::size_t(body.array.count),
       parent,field,{feedback_name.index,feedback_name.number}))return false;
    reason=16;Script parent_body{};unsigned parent_bytes{},parent_reason{};
    // Parent proof still authenticates Engine.Weapon.WeaponFiring, its exact
    // FireAmmunition/TimeWeaponFiring body and original looping RefireCheckTimer.
    if(!initial_proof(r,weapon,parent,parent_body,parent_bytes,refire,parent_reason))return false;
    reason=17;
    if(!unified_weapon_fire::retained_metadata(r)||!unchanged(r,function,body))return false;
    bytes=count;reason=0;return true;
}
