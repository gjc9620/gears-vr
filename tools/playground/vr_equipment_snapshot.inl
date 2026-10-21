// Private, opt-in game-thread inventory observer. It performs no equipment
// action. All function calls below are original read-only UnrealScript queries.
namespace vrequip {
gears_weapon::WeaponEquipmentModel model;
gears_weapon::EquipmentSnapshot previous{};
const wchar_t* previous_error{};
bool have_previous{};
ULONGLONG next_probe{};

bool result_property(playground::Args& call,const wchar_t* kind,unsigned size){
    const auto p=call.prop(L"ReturnValue");
    return call.valid&&p&&get<unsigned>(p+0x74)==size&&
        playground::named(get<uptr>(p+0x50),kind)&&
        get<unsigned>(p+0x94)<=call.r.size&&
        size<=call.r.size-get<unsigned>(p+0x94);
}
bool read_bool(uptr object,const wchar_t* field,bool& value){
    const auto p=playground::property(object,field);
    if(!p||get<unsigned>(p+0x74)!=4||
       !playground::named(get<uptr>(p+0x50),L"BoolProperty"))return false;
    value=(get<unsigned>(object+get<unsigned>(p+0x94))&get<unsigned>(p+0xd0))!=0;
    return true;
}
bool read_slot(uptr inv,std::uint8_t slot,uptr& weapon){
    playground::Args call(inv,L"GetWeaponInSlot");
    const auto p=call.prop(L"Slot");
    if(!p||get<unsigned>(p+0x74)!=1||
       !playground::named(get<uptr>(p+0x50),L"ByteProperty")||
       !result_property(call,L"ObjectProperty",sizeof(uptr)))return false;
    if(!call.set(L"Slot",slot).run())return false;
    weapon=call.output<uptr>();
    if(!weapon)return true;
    if(!playground::instance(weapon,L"WarWeapon")||playground::deleted(weapon))return false;
    const auto character_slot=playground::property(weapon,L"CharacterSlot");
    return character_slot&&get<unsigned>(character_slot+0x74)==1&&
        playground::named(get<uptr>(character_slot+0x50),L"ByteProperty")&&
        get<std::uint8_t>(weapon+get<unsigned>(character_slot+0x94))==slot;
}
bool read_can_switch(uptr pc,bool& value){
    playground::Args call(pc,L"CanSwitchWeapons");
    if(!result_property(call,L"BoolProperty",4)||!call.run())return false;
    const auto mask=get<unsigned>(call.prop(L"ReturnValue")+0xd0);
    value=(call.output<unsigned>()&mask)!=0;
    return true;
}
bool read_throwable_count(uptr weapon,uptr pawn,std::uint32_t& count){
    // For human-controlled grenade weapons HasAnyAmmo checks spare charges,
    // not GetTotalAmmoInGun's spare + current magazine expression.
    if(playground::pointer(weapon,L"Instigator")!=pawn)return false;
    const auto spare=playground::property(weapon,L"SpareAmmoCount");
    if(!spare||get<unsigned>(spare+0x74)!=4||
       !playground::named(get<uptr>(spare+0x50),L"IntProperty"))return false;
    const auto charges=get<int>(weapon+get<unsigned>(spare+0x94));
    if(charges<0||charges>10000)return false;
    playground::Args call(weapon,L"HasAnyAmmo");
    if(!result_property(call,L"BoolProperty",4)||!call.run())return false;
    const auto mask=get<unsigned>(call.prop(L"ReturnValue")+0xd0);
    const bool usable=(call.output<unsigned>()&mask)!=0;
    // Infinite spare ammo (used by the private Playground) is selectable even
    // if the stored finite spare count is zero.
    count=usable?(charges?std::uint32_t(charges):~std::uint32_t{0}):0;
    return true;
}
bool snapshot(uptr pc,gears_weapon::EquipmentSnapshot& game,const wchar_t*& error){
    game={};error=L"identity";
    if(!playground::instance(pc,L"WarPC")||playground::deleted(pc))return false;
    const auto pawn=playground::pointer(pc,L"Pawn");
    const auto world=playground::pointer(pc,L"WorldInfo");
    if(!playground::instance(pawn,L"WarPawn")||playground::deleted(pawn)||
       playground::pointer(pawn,L"Controller")!=pc||
       !playground::instance(world,L"WorldInfo"))return false;
    const auto inv=playground::pointer(pawn,L"InvManager");
    error=L"inventory_manager";
    if(!playground::instance(inv,L"WarInventoryManager")||playground::deleted(inv))return false;
    game.world=world;game.pawn=pawn;
    const auto current=playground::pointer(pawn,L"Weapon");
    if(current&&playground::instance(current,L"WarWeapon")&&
       !playground::deleted(current))game.current_weapon=current;
    error=L"inventory_slot_query";
    for(std::uint8_t slot=0;slot<4;++slot){
        uptr item{};
        if(!read_slot(inv,slot,item))return false;
        for(std::uint8_t earlier=0;earlier<slot;++earlier)
            if(item&&game.weapon_in_slot[earlier]==item)return false;
        game.weapon_in_slot[slot]=item;
    }
    error=L"switch_query";
    if(!read_can_switch(pc,game.can_switch))return false;
    error=L"animation_state";
    if(!read_bool(pawn,L"bSwitchingWeapons",game.animation_busy))return false;
    // GetWeaponInSlot can still return a grenade actor with zero charges.
    const auto grenade=static_cast<uptr>(game.weapon_in_slot[1]);
    if(grenade){
        error=L"grenade_type_or_count";
        if(!playground::instance(grenade,L"Weap_Grenade")||
           !read_throwable_count(grenade,pawn,game.throwable_count))return false;
        game.throwable_count_known=true;
    }else game.throwable_count_known=true;
    error=L"ok";
    return true;
}
bool changed(gears_weapon::EquipmentSnapshot const& a,
             gears_weapon::EquipmentSnapshot const& b){
    return a.world!=b.world||a.pawn!=b.pawn||a.current_weapon!=b.current_weapon||
        a.weapon_in_slot!=b.weapon_in_slot||a.can_switch!=b.can_switch||
        a.animation_busy!=b.animation_busy||a.throwable_count!=b.throwable_count||
        a.throwable_count_known!=b.throwable_count_known;
}
void tick(uptr pc){
    const auto now=GetTickCount64();
    if(now<next_probe)return;
    next_probe=now+500;
    gears_weapon::EquipmentSnapshot game{};
    const wchar_t* error{};
    const bool ok=snapshot(pc,game,error);
    if(!ok){model.reset();game={};}
    else model.observe(game);
    if(have_previous&&previous_error==error&&!changed(previous,game))return;
    have_previous=true;previous_error=error;previous=game;
    if(!log_file)return;
    fwprintf(log_file,L"equipment_snapshot status=%ls world=%p pawn=%p current=%p "
        L"slots=%p,%p,%p,%p grenades=%u known=%u switch=%u anim=%u\n",
        error,reinterpret_cast<void*>(static_cast<uptr>(game.world)),
        reinterpret_cast<void*>(static_cast<uptr>(game.pawn)),
        reinterpret_cast<void*>(static_cast<uptr>(game.current_weapon)),
        reinterpret_cast<void*>(static_cast<uptr>(game.weapon_in_slot[0])),
        reinterpret_cast<void*>(static_cast<uptr>(game.weapon_in_slot[1])),
        reinterpret_cast<void*>(static_cast<uptr>(game.weapon_in_slot[2])),
        reinterpret_cast<void*>(static_cast<uptr>(game.weapon_in_slot[3])),
        game.throwable_count,unsigned(game.throwable_count_known),
        unsigned(game.can_switch),unsigned(game.animation_busy));
    fflush(log_file);
}
}
