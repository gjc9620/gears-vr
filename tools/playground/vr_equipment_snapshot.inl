// Private, opt-in game-thread inventory observer. The separately gated switch
// test may invoke the game's original slot function in an owned test arena.
namespace vrequip {
gears_weapon::WeaponEquipmentModel model;
gears_weapon::EquipmentSnapshot previous{};
const wchar_t* previous_error{};
bool have_previous{};
ULONGLONG next_probe{};
#if defined(GEARS_PLAYGROUND_EQUIPMENT_SWITCH_TEST)
std::atomic<unsigned> queued_slot{4};
std::atomic<unsigned> last_switch_status{};
ULONGLONG switch_deadline{};
#if defined(GEARS_PLAYGROUND_EQUIPMENT_INTENT_TEST)
gears_weapon::EquipmentIntentBridge intent_bridge;
std::uint64_t intent_cursor=UINT64_MAX;
ULONGLONG next_intent_open{};
bool intent_open{};
bool test_environment(){
    wchar_t behind[4]{},muted[4]{};
    return GetEnvironmentVariableW(L"GEARSVR_TEST_WINDOWS_BEHIND",behind,4)==1&&behind[0]==L'1'&&
        GetEnvironmentVariableW(L"GEARSVR_TEST_MUTE_GAME",muted,4)==1&&muted[0]==L'1';
}
#endif

gears_weapon::BodySlot body_for_inventory_slot(unsigned slot){
    switch(slot){
        case 0:return gears_weapon::BodySlot::pistol;
        case 1:return gears_weapon::BodySlot::grenade;
        case 2:return gears_weapon::BodySlot::weapon_2;
        case 3:return gears_weapon::BodySlot::weapon_1;
        default:return gears_weapon::BodySlot::none;
    }
}
void reset_switch_test(){
    queued_slot.store(4,std::memory_order_release);
    last_switch_status.store(0,std::memory_order_release);
    switch_deadline=0;
    model.reset();
#if defined(GEARS_PLAYGROUND_EQUIPMENT_INTENT_TEST)
    intent_bridge.close();intent_open=false;intent_cursor=UINT64_MAX;next_intent_open=0;
#endif
}
#endif

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
bool read_has_any_ammo(uptr weapon,bool& value){
    playground::Args call(weapon,L"HasAnyAmmo");
    if(!result_property(call,L"BoolProperty",4)||!call.run())return false;
    const auto mask=get<unsigned>(call.prop(L"ReturnValue")+0xd0);
    value=(call.output<unsigned>()&mask)!=0;
    return true;
}
bool read_throwable_count(uptr weapon,uptr pawn,bool usable,std::uint32_t& count){
    // For human-controlled grenade weapons HasAnyAmmo checks spare charges,
    // not GetTotalAmmoInGun's spare + current magazine expression.
    if(playground::pointer(weapon,L"Instigator")!=pawn)return false;
    const auto spare=playground::property(weapon,L"SpareAmmoCount");
    if(!spare||get<unsigned>(spare+0x74)!=4||
       !playground::named(get<uptr>(spare+0x50),L"IntProperty"))return false;
    const auto charges=get<int>(weapon+get<unsigned>(spare+0x94));
    if(charges<0||charges>10000)return false;
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
        if(item){
            error=L"slot_ammo_query";
            if(!read_has_any_ammo(item,game.ammo_available[slot]))return false;
            game.ammo_known[slot]=true;
            error=L"inventory_slot_query";
        }
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
           !read_throwable_count(grenade,pawn,game.ammo_available[1],game.throwable_count))return false;
        game.throwable_count_known=true;
    }else game.throwable_count_known=true;
    error=L"ok";
    return true;
}
bool changed(gears_weapon::EquipmentSnapshot const& a,
             gears_weapon::EquipmentSnapshot const& b){
    return a.world!=b.world||a.pawn!=b.pawn||a.current_weapon!=b.current_weapon||
        a.weapon_in_slot!=b.weapon_in_slot||a.can_switch!=b.can_switch||
        a.ammo_available!=b.ammo_available||a.ammo_known!=b.ammo_known||
        a.animation_busy!=b.animation_busy||a.throwable_count!=b.throwable_count||
        a.throwable_count_known!=b.throwable_count_known;
}
#if defined(GEARS_PLAYGROUND_EQUIPMENT_SWITCH_TEST)
bool play_empty_slot_sound(gears_weapon::EquipmentSnapshot const& game,unsigned slot){
    // The original slot function skips feedback when the slot has no actor.
    // Reuse a real equipped/slot weapon's own empty-clip cue, locally only.
    const uptr candidates[]={static_cast<uptr>(game.weapon_in_slot[slot]),
                             static_cast<uptr>(game.current_weapon)};
    for(const auto weapon:candidates){
        if(!playground::instance(weapon,L"WarWeapon")||playground::deleted(weapon))continue;
        const auto sound_property=playground::property(weapon,L"FireNoAmmoSound");
        if(!sound_property||get<unsigned>(sound_property+0x74)!=sizeof(uptr)||
           !playground::named(get<uptr>(sound_property+0x50),L"ObjectProperty"))continue;
        const auto sound=get<uptr>(weapon+get<unsigned>(sound_property+0x94));
        if(!playground::instance(sound,L"SoundCue")||playground::deleted(sound))continue;
        playground::Args play(weapon,L"WeaponPlaySoundLocal");
        const auto parameter=play.prop(L"Sound");
        if(!parameter||get<unsigned>(parameter+0x74)!=sizeof(uptr)||
           !playground::named(get<uptr>(parameter+0x50),L"ObjectProperty"))continue;
        if(play.set(L"Sound",sound).run())return true;
    }
    return false;
}
#endif
void tick(uptr pc){
    const auto now=GetTickCount64();
#if defined(GEARS_PLAYGROUND_EQUIPMENT_SWITCH_TEST)
    unsigned requested_slot=queued_slot.exchange(4,std::memory_order_acq_rel);
#if defined(GEARS_PLAYGROUND_EQUIPMENT_INTENT_TEST)
    gears_weapon::EquipmentIntentEvent grip_event{};
    bool from_grip=false;
    if(playground::ready&&test_environment()){
        if(!intent_open&&now>=next_intent_open){
            intent_open=intent_bridge.open(false);next_intent_open=now+500;
            intent_cursor=UINT64_MAX;
        }
        if(intent_open&&requested_slot>=4&&intent_bridge.read_next(intent_cursor,grip_event)){
            requested_slot=grip_event.inventory_slot;from_grip=true;
        }
    }
#endif
    if(now<next_probe&&requested_slot>=4)return;
#else
    if(now<next_probe)return;
#endif
    next_probe=now+500;
    gears_weapon::EquipmentSnapshot game{};
    const wchar_t* error{};
    const bool ok=snapshot(pc,game,error);
    if(!ok){model.reset();game={};
#if defined(GEARS_PLAYGROUND_EQUIPMENT_SWITCH_TEST)
        if(switch_deadline)
            last_switch_status.store(2,std::memory_order_release);
        switch_deadline=0;
#endif
    }
    else{
        const auto observed=model.observe(game);
#if defined(GEARS_PLAYGROUND_EQUIPMENT_SWITCH_TEST)
        if(observed.event==gears_weapon::EquipmentEvent::equipped){
            switch_deadline=0;
            last_switch_status.store(6,std::memory_order_release);
        }else if(observed.event==gears_weapon::EquipmentEvent::interrupted&&switch_deadline){
            switch_deadline=0;
            last_switch_status.store(2,std::memory_order_release);
        }else if(model.switch_pending()&&switch_deadline&&now>=switch_deadline){
            model.switch_rejected(game);switch_deadline=0;
            last_switch_status.store(7,std::memory_order_release);
        }
#endif
    }
#if defined(GEARS_PLAYGROUND_EQUIPMENT_SWITCH_TEST)
    if(requested_slot<4){
        // Only an owned private arena may exercise the original state path.
        // Recheck all identities on the game thread; the external request
        // carries a slot number, never a weapon pointer.
        if(!ok||!playground::ready||pc!=playground::arena_pc||
           game.world!=playground::arena_world||game.pawn!=playground::arena_pawn
#if defined(GEARS_PLAYGROUND_EQUIPMENT_INTENT_TEST)
           ||(from_grip&&(grip_event.controller!=pc||grip_event.pawn!=game.pawn||
                           grip_event.weapon!=game.current_weapon))
#endif
           ){
            last_switch_status.store(2,std::memory_order_release);
        }else{
            const auto decision=model.tap(body_for_inventory_slot(requested_slot),game);
            unsigned status=3;
            if(decision.event==gears_weapon::EquipmentEvent::empty_slot){
                status=play_empty_slot_sound(game,requested_slot)?4:9;
            }
            else if(decision.event==gears_weapon::EquipmentEvent::request_unholster||
                    decision.event==gears_weapon::EquipmentEvent::request_stow)status=5;
            else if(decision.event==gears_weapon::EquipmentEvent::request_original_switch){
                const auto inv=playground::pointer(static_cast<uptr>(game.pawn),L"InvManager");
                playground::Args switch_slot(inv,L"SetWeaponFromSlot");
                const auto parameter=switch_slot.prop(L"Slot");
                const bool valid=playground::instance(inv,L"WarInventoryManager")&&
                    parameter&&get<unsigned>(parameter+0x74)==1&&
                    playground::named(get<uptr>(parameter+0x50),L"ByteProperty")&&
                    (get<unsigned long long>(parameter+0x78)&0x80)!=0;
                if(valid&&switch_slot.set(L"Slot",static_cast<std::uint8_t>(requested_slot)).run()){
                    switch_deadline=now+5000;status=1;
                }else{
                    model.switch_rejected(game);switch_deadline=0;status=8;
                }
            }
            last_switch_status.store(status,std::memory_order_release);
            if(log_file){fwprintf(log_file,L"equipment_switch_test source=%ls slot=%u status=%u weapon=%p\n",
#if defined(GEARS_PLAYGROUND_EQUIPMENT_INTENT_TEST)
                from_grip?L"xr_grip":L"external",
#else
                L"external",
#endif
                requested_slot,status,reinterpret_cast<void*>(static_cast<uptr>(decision.weapon)));
                fflush(log_file);}
        }
    }
    if(model.switch_pending())next_probe=now+50;
#endif
    if(have_previous&&previous_error==error&&!changed(previous,game))return;
    have_previous=true;previous_error=error;previous=game;
    if(!log_file)return;
    fwprintf(log_file,L"equipment_snapshot status=%ls world=%p pawn=%p current=%p "
        L"slots=%p,%p,%p,%p grenades=%u known=%u ammo=%u%u%u%u switch=%u anim=%u\n",
        error,reinterpret_cast<void*>(static_cast<uptr>(game.world)),
        reinterpret_cast<void*>(static_cast<uptr>(game.pawn)),
        reinterpret_cast<void*>(static_cast<uptr>(game.current_weapon)),
        reinterpret_cast<void*>(static_cast<uptr>(game.weapon_in_slot[0])),
        reinterpret_cast<void*>(static_cast<uptr>(game.weapon_in_slot[1])),
        reinterpret_cast<void*>(static_cast<uptr>(game.weapon_in_slot[2])),
        reinterpret_cast<void*>(static_cast<uptr>(game.weapon_in_slot[3])),
        game.throwable_count,unsigned(game.throwable_count_known),
        unsigned(game.ammo_available[0]),unsigned(game.ammo_available[1]),
        unsigned(game.ammo_available[2]),unsigned(game.ammo_available[3]),
        unsigned(game.can_switch),unsigned(game.animation_busy));
    fflush(log_file);
}
}
