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
std::uint64_t pose_cursor=UINT64_MAX;
ULONGLONG next_intent_open{};
bool intent_open{};
std::atomic<unsigned> last_ammo_status{};
std::atomic<std::uint64_t> pose_accepted{},pose_rejected{};
gears_weapon::HandWorldFrame last_intent_pose{};
bool last_intent_pose_known{};
ULONGLONG next_pose_log{};
ULONGLONG next_reload_native_probe{};
uptr last_reload_native_weapon{};
bool test_environment(){
    wchar_t behind[4]{},muted[4]{};
    return GetEnvironmentVariableW(L"GEARSVR_TEST_WINDOWS_BEHIND",behind,4)==1&&behind[0]==L'1'&&
        GetEnvironmentVariableW(L"GEARSVR_TEST_MUTE_GAME",muted,4)==1&&muted[0]==L'1';
}
bool read_bool(uptr object,const wchar_t* field,bool& value);
// Read only: identify which original component is being rendered before any
// attempt to move the weapon. A named WeaponMesh default subobject need not be
// the current actor's Mesh property in every equip state.
struct MeshPoseProbe {
    uptr mesh{},weapon_mesh{},asset{};
    wchar_t component_name[96]{},asset_name[96]{};
    playground::Vector translation{};
    playground::Rotation rotation{};
    float local_to_world[16]{};
    bool transform_known{},absolute_translation{},absolute_rotation{},absolute_scale{},hidden{};
};
bool mesh_pose_probe(uptr weapon,MeshPoseProbe& out){
    out={};
    if(!playground::instance(weapon,L"WarWeapon")||playground::deleted(weapon))return false;
    out.mesh=playground::pointer(weapon,L"Mesh");
    out.weapon_mesh=playground::pointer(weapon,L"WeaponMesh");
    if(!playground::instance(out.mesh,L"SkeletalMeshComponent")||
       playground::deleted(out.mesh))return false;
    name(out.mesh,out.component_name,96);
    out.asset=playground::pointer(out.mesh,L"SkeletalMesh");
    if(out.asset)name(out.asset,out.asset_name,96);
    const auto read_struct=[&](const wchar_t* field,void* destination,unsigned bytes){
        const auto property=playground::property(out.mesh,field);
        if(!property)return false;
        const auto cls=get<uptr>(out.mesh+0x50);
        const auto offset=get<unsigned>(property+0x94);
        const auto size=get<unsigned>(cls+0x88);
        return get<unsigned>(property+0x74)==bytes&&
            playground::named(get<uptr>(property+0x50),L"StructProperty")&&
            offset<=size&&bytes<=size-offset&&copy(destination,out.mesh+offset,bytes);
    };
    out.transform_known=read_struct(L"Translation",&out.translation,sizeof(out.translation))&&
        read_struct(L"Rotation",&out.rotation,sizeof(out.rotation))&&
        read_struct(L"LocalToWorld",out.local_to_world,sizeof(out.local_to_world));
    return out.transform_known&&read_bool(out.mesh,L"AbsoluteTranslation",out.absolute_translation)&&
        read_bool(out.mesh,L"AbsoluteRotation",out.absolute_rotation)&&
        read_bool(out.mesh,L"AbsoluteScale",out.absolute_scale)&&
        read_bool(out.mesh,L"HiddenGame",out.hidden);
}
// The shipped weapon can own both a skinned magazine on its gun mesh and a
// separate StaticMeshComponent for the left hand. Observe the latter before
// deciding whether a VR reload needs any derived mesh at all. This does not
// attach, hide, duplicate, or otherwise change either original component.
struct MagazineVisualProbe {
    uptr component{},asset{};
    wchar_t component_name[96]{},asset_name[96]{};
    bool present{},attached_to_pawn{},hidden{};
};
bool magazine_visual_probe(uptr weapon,uptr pawn,MagazineVisualProbe& out){
    out={};
    if(!playground::instance(weapon,L"WarWeapon")||playground::deleted(weapon)||
       !playground::instance(pawn,L"WarPawn")||playground::deleted(pawn)||
       playground::pointer(weapon,L"Instigator")!=pawn)return false;
    const auto field=playground::property(weapon,L"MagazineMesh");
    if(!field||get<unsigned>(field+0x74)!=sizeof(uptr)||
       !playground::named(get<uptr>(field+0x50),L"ComponentProperty"))return false;
    out.component=playground::pointer(weapon,L"MagazineMesh");
    if(!out.component)return true;
    if(!playground::instance(out.component,L"StaticMeshComponent")||
       playground::deleted(out.component))return false;
    out.present=true;
    name(out.component,out.component_name,96);
    out.asset=playground::pointer(out.component,L"StaticMesh");
    if(out.asset){
        if(!playground::instance(out.asset,L"StaticMesh")||
           playground::deleted(out.asset))return false;
        name(out.asset,out.asset_name,96);
    }
    if(!read_bool(out.component,L"HiddenGame",out.hidden))return false;
    const auto pawn_mesh=playground::pointer(pawn,L"Mesh");
    if(!playground::instance(pawn_mesh,L"SkeletalMeshComponent")||
       playground::deleted(pawn_mesh))return false;
    playground::Args attached(pawn_mesh,L"IsComponentAttached");
    const auto input=attached.prop(L"Component"),result=attached.prop(L"ReturnValue");
    if(!attached.valid||!input||!result||
       get<unsigned>(input+0x74)!=sizeof(uptr)||
       !playground::named(get<uptr>(input+0x50),L"ComponentProperty")||
       get<unsigned>(result+0x74)!=4||
       !playground::named(get<uptr>(result+0x50),L"BoolProperty")||
       !attached.set(L"Component",out.component).run())return false;
    const auto mask=get<unsigned>(result+0xd0);
    if(!mask)return false;
    out.attached_to_pawn=(attached.output<unsigned>()&mask)!=0;
    return true;
}
// The original SetWeaponAmmoBoneDisplay resolves AmmoControl on the weapon's
// current skinned mesh, then changes its active strength. Mirror only the
// lookup and read the current/target strengths; never call the setter here.
struct AmmoBoneVisualProbe {
    uptr mesh{},control{};
    wchar_t control_name[96]{};
    bool present{};
    float strength{},target{},bone_scale{};
};
bool ammo_control_name(Name& out){
    static Name cached{};
    static bool found{};
    if(found){out=cached;return true;}
    const auto names=get<Array>(base+0x1c34800);
    if(!names.data||names.count<1000||names.count>names.capacity||names.capacity>4000000)
        return false;
    constexpr wchar_t wanted[]=L"AmmoControl";
    for(int index=0;index<names.count;++index){
        const auto entry=get<uptr>(names.data+size_t(index)*8);
        if(!entry||get<unsigned>(entry)!=unsigned(index))continue;
        wchar_t value[sizeof(wanted)/sizeof(wchar_t)]{};
        if(!copy(value,entry+20,sizeof(value))||
           std::memcmp(value,wanted,sizeof(value)))continue;
        cached={unsigned(index),0};found=true;out=cached;return true;
    }
    return false;
}
bool read_control_float(uptr control,const wchar_t* field,float& out){
    const auto property=playground::property(control,field);
    const auto cls=get<uptr>(control+0x50);
    if(!property||!cls||get<unsigned>(property+0x74)!=sizeof(float)||
       !playground::named(get<uptr>(property+0x50),L"FloatProperty"))return false;
    const auto offset=get<unsigned>(property+0x94),size=get<unsigned>(cls+0x88);
    return offset<=size&&sizeof(float)<=size-offset&&
        copy(&out,control+offset,sizeof(out))&&std::isfinite(out);
}
bool ammo_bone_visual_probe(uptr weapon,AmmoBoneVisualProbe& out){
    out={};
    if(!playground::instance(weapon,L"WarWeapon")||playground::deleted(weapon))return false;
    out.mesh=playground::pointer(weapon,L"Mesh");
    if(!playground::instance(out.mesh,L"SkeletalMeshComponent")||
       playground::deleted(out.mesh))return false;
    Name control_name{};
    if(!ammo_control_name(control_name))return false;
    playground::Args find(out.mesh,L"FindSkelControl");
    const auto input=find.prop(L"InControlName"),result=find.prop(L"ReturnValue");
    if(!find.valid||!input||!result||
       get<unsigned>(input+0x74)!=sizeof(Name)||
       !playground::named(get<uptr>(input+0x50),L"NameProperty")||
       get<unsigned>(result+0x74)!=sizeof(uptr)||
       !playground::named(get<uptr>(result+0x50),L"ObjectProperty")||
       get<unsigned>(result+0x94)>find.r.size||
       sizeof(uptr)>find.r.size-get<unsigned>(result+0x94)||
       !find.set(L"InControlName",control_name).run())return false;
    out.control=find.output<uptr>();
    if(!out.control)return true;
    if(!playground::instance(out.control,L"SkelControlBase")||
       playground::deleted(out.control))return false;
    out.present=true;
    name(out.control,out.control_name,96);
    return read_control_float(out.control,L"ControlStrength",out.strength)&&
        read_control_float(out.control,L"StrengthTarget",out.target)&&
        read_control_float(out.control,L"BoneScale",out.bone_scale);
}
// Locate the original UFunction wrappers without calling either function.
// The historical disk EXE is protected, so a live, validated module-relative
// address is needed before bounded native analysis can proceed.
struct ReloadNativeProbe {
    uptr function{},entry{};
    unsigned native_index{},parameter_bytes{};
    bool reflected{},executable_in_game{};
};
bool game_image_readable(uptr address){
    if(!address)return false;
    MEMORY_BASIC_INFORMATION page{};
    return VirtualQuery(reinterpret_cast<void*>(address),&page,sizeof(page))&&
        page.State==MEM_COMMIT&&page.Type==MEM_IMAGE&&
        page.AllocationBase==reinterpret_cast<void*>(base)&&
        !(page.Protect&(PAGE_GUARD|PAGE_NOACCESS));
}
bool game_executable(uptr address){
    if(!game_image_readable(address))return false;
    MEMORY_BASIC_INFORMATION page{};
    if(!VirtualQuery(reinterpret_cast<void*>(address),&page,sizeof(page)))return false;
    switch(page.Protect&0xff){
        case PAGE_EXECUTE:case PAGE_EXECUTE_READ:
        case PAGE_EXECUTE_READWRITE:case PAGE_EXECUTE_WRITECOPY:return true;
        default:return false;
    }
}
ReloadNativeProbe reload_native_probe(uptr war_weapon,const wchar_t* function_name){
    ReloadNativeProbe result{};
    if(!registered(war_weapon)||
       !playground::named(war_weapon,L"WarWeapon")||
       !playground::named(get<uptr>(war_weapon+0x50),L"Class"))return result;
    result.function=playground::field(war_weapon,function_name,L"Function");
    if(!registered(result.function)||
       get<uptr>(result.function+0x40)!=war_weapon)return result;
    result.reflected=true;
    result.native_index=get<unsigned short>(result.function+0xf8);
    result.parameter_bytes=get<unsigned short>(result.function+0x106);
    const auto first=get<uptr>(result.function+0x114);
    const auto second=get<uptr>(result.function+0x114);
    if(first&&first==second&&game_executable(first)){
        result.entry=first;result.executable_in_game=true;
    }
    return result;
}
// These are the two virtual call offsets in the independently verified
// retail wrappers, not the UFunction entry points themselves. Read the
// authenticated current weapon's own vtable to resolve any class override.
uptr reload_virtual_target(uptr weapon,unsigned slot){
    if(!playground::instance(weapon,L"WarWeapon")||playground::deleted(weapon))return 0;
    const auto table=get<uptr>(weapon);
    if(!game_image_readable(table)||!game_image_readable(table+slot)||
       table!=get<uptr>(weapon))return 0;
    const auto first=get<uptr>(table+slot),second=get<uptr>(table+slot);
    return first&&first==second&&game_executable(first)?first:0;
}
// Read the fields selected by the byte-matched native getter, without
// invoking it. The final value remains a candidate until its separate tail
// block and a real getter output have been checked in the current process.
struct ReloadWindowProbe {
    bool valid{};
    float reload_duration{},start_field{},pre_field{},super_field{},sweet_field{};
    unsigned owner_index{},duration_count{};
    float durations[8]{};
    float derived_start{},derived_super{},derived_end{};
};
bool reload_float_field(uptr weapon,const wchar_t* field,unsigned expected,float& out){
    const auto property=playground::property(weapon,field);
    const auto cls=get<uptr>(weapon+0x50);
    if(!property||!cls||!playground::named(get<uptr>(property+0x50),L"FloatProperty")||
       get<unsigned>(property+0x74)!=sizeof(float)||
       get<unsigned>(property+0x94)!=expected||
       expected>get<unsigned>(cls+0x88)||
       sizeof(float)>get<unsigned>(cls+0x88)-expected)return false;
    return copy(&out,weapon+expected,sizeof(out))&&std::isfinite(out);
}
ReloadWindowProbe reload_window_probe(uptr weapon){
    ReloadWindowProbe out{};
    if(!playground::instance(weapon,L"WarWeapon")||playground::deleted(weapon))return out;
    const auto cls=get<uptr>(weapon+0x50);
    if(!reload_float_field(weapon,L"ReloadDuration",0x614,out.reload_duration)||
       !reload_float_field(weapon,L"AR_PossibleSuccessStartPoint",0x780,out.start_field)||
       !reload_float_field(weapon,L"AR_PreReactionWindowDuration",0x77c,out.pre_field)||
       !reload_float_field(weapon,L"AR_SuperSweetSpotDuration",0x784,out.super_field)||
       !reload_float_field(weapon,L"AR_SweetSpotDuration",0x788,out.sweet_field))return out;
    const auto property=playground::property(weapon,L"ActiveReload_SuperSweetSpotDurations");
    if(!property||!playground::named(get<uptr>(property+0x50),L"StructProperty")||
       get<unsigned>(property+0x94)!=0x7a4||
       get<unsigned>(property+0x74)<sizeof(Array)||
       0x7a4>get<unsigned>(cls+0x88)||
       sizeof(Array)>get<unsigned>(cls+0x88)-0x7a4)return out;
    Array durations{};
    if(!copy(&durations,weapon+0x7a4,sizeof(durations))||!durations.data||
       durations.count<1||durations.count>8||
       durations.capacity<durations.count||durations.capacity>8||
       !copy(out.durations,durations.data,size_t(durations.count)*sizeof(float)))return out;
    out.duration_count=unsigned(durations.count);
    for(unsigned i=0;i<out.duration_count;++i)
        if(!std::isfinite(out.durations[i])||out.durations[i]<=0)return out;
    const auto owner=playground::pointer(weapon,L"Instigator");
    if(!playground::instance(owner,L"WarPawn")||playground::deleted(owner)||
       owner!=get<uptr>(weapon+0x8c))return out;
    const auto owner_cls=get<uptr>(owner+0x50);
    unsigned char index{};
    if(get<unsigned>(owner_cls+0x88)<=0x47d||
       !copy(&index,owner+0x47d,sizeof(index))||index>=out.duration_count)return out;
    out.owner_index=index;
    out.derived_start=out.start_field==-1.0f?out.reload_duration*0.5f:out.start_field;
    out.derived_super=out.super_field==-1.0f?out.durations[index]:out.super_field;
    out.derived_end=out.derived_start+out.derived_super;
    out.valid=std::isfinite(out.derived_end)&&out.derived_end>0;
    return out;
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
    intent_bridge.close();intent_open=false;intent_cursor=pose_cursor=UINT64_MAX;
    last_intent_pose={};last_intent_pose_known=false;
    next_intent_open=next_pose_log=0;
    next_reload_native_probe=0;last_reload_native_weapon=0;
    last_ammo_status.store(0,std::memory_order_release);
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
#if defined(GEARS_PLAYGROUND_WEAPON_MESH_FOLLOW_TEST)
#include "vr_weapon_mesh_follow.inl"
#include "vr_lancer_reload_visual_probe.inl"
#include "vr_lancer_socket_probe.inl"
#endif
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
#if defined(GEARS_PLAYGROUND_EQUIPMENT_INTENT_TEST)
bool read_ammo_supply(uptr weapon,bool& available){
    if(!playground::instance(weapon,L"WarWeapon")||playground::deleted(weapon))return false;
    const auto spare=playground::property(weapon,L"SpareAmmoCount");
    if(!spare||get<unsigned>(spare+0x74)!=4||
       !playground::named(get<uptr>(spare+0x50),L"IntProperty"))return false;
    const int count=get<int>(weapon+get<unsigned>(spare+0x94));
    if(count<0||count>1000000)return false;
    playground::Args infinite(weapon,L"HasInfiniteSpareAmmo");
    if(!result_property(infinite,L"BoolProperty",4)||!infinite.run())return false;
    const auto mask=get<unsigned>(infinite.prop(L"ReturnValue")+0xd0);
    available=count>0||(infinite.output<unsigned>()&mask)!=0;
    return true;
}
#endif
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
            error=L"slot_dry_selection_query";
            if(!read_bool(item,L"bCanSelectWithoutAmmo",
                          game.can_select_without_ammo[slot]))return false;
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
        a.can_select_without_ammo!=b.can_select_without_ammo||
        a.animation_busy!=b.animation_busy||a.throwable_count!=b.throwable_count||
        a.throwable_count_known!=b.throwable_count_known;
}
#if defined(GEARS_PLAYGROUND_EQUIPMENT_SWITCH_TEST)
bool play_empty_slot_sound(gears_weapon::EquipmentSnapshot const& game,unsigned slot){
    // The original slot function skips feedback when the slot has no actor.
    // Reuse a real equipped/slot weapon's own empty-clip cue, locally only.
    const uptr candidates[]={slot<4?static_cast<uptr>(game.weapon_in_slot[slot]):0,
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
#if defined(GEARS_PLAYGROUND_LANCER_RELOAD_HOLD_TEST)
#include "vr_lancer_reload_hold.inl"
#endif
#if defined(GEARS_PLAYGROUND_LANCER_CHAINSAW_HOLD_TEST)
#include "vr_lancer_chainsaw_hold.inl"
#endif
#if defined(GEARS_PLAYGROUND_LANCER_MAGAZINE_FOLLOW_TEST)
#include "vr_lancer_magazine_follow.inl"
#endif
void tick(uptr pc){
    const auto now=GetTickCount64();
#if defined(GEARS_PLAYGROUND_LANCER_CHAINSAW_HOLD_TEST)
    lancer_saw_hold_tick(pc);
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_SWITCH_TEST)
    unsigned requested_slot=queued_slot.exchange(4,std::memory_order_acq_rel);
#if defined(GEARS_PLAYGROUND_EQUIPMENT_INTENT_TEST)
    gears_weapon::EquipmentIntentEvent grip_event{};
    bool from_grip=false,ammo_request=false,reload_request=false;
#if defined(GEARS_PLAYGROUND_LANCER_RELOAD_HOLD_TEST)
    const bool test_reload_start=lancer_reload_test_start.exchange(false,std::memory_order_acq_rel);
    reload_request=test_reload_start;
    const bool reload_complete_request=lancer_reload_test_complete.exchange(false,std::memory_order_acq_rel);
#endif
    if(playground::ready&&test_environment()){
        if(!intent_open&&now>=next_intent_open){
            intent_open=intent_bridge.open(false);next_intent_open=now+500;
            intent_cursor=pose_cursor=UINT64_MAX;
        }
        gears_weapon::HandWorldFrame world_hands{};
        if(intent_open&&intent_bridge.read_pose(pose_cursor,world_hands)){
            const auto pawn=playground::pointer(pc,L"Pawn");
            const bool same=pc==playground::arena_pc&&pawn==playground::arena_pawn&&
                playground::pointer(pc,L"WorldInfo")==playground::arena_world&&
                playground::pointer(pawn,L"Weapon")==world_hands.weapon&&
                world_hands.controller==pc&&world_hands.pawn==pawn;
            if(same){
                ++pose_accepted;
                last_intent_pose=world_hands;last_intent_pose_known=true;
#if defined(GEARS_PLAYGROUND_WEAPON_MESH_FOLLOW_TEST)
                mesh_follow_hands=world_hands;mesh_follow_hands_known=true;
#endif
                if(log_file&&now>=next_pose_log){
                    next_pose_log=now+500;
                    fwprintf(log_file,L"hand_world_pose sequence=%llu input_epoch=%llu "
                        L"world_generation=%llu mask=%u grip=%u "
                        L"left=%.3f,%.3f,%.3f right=%.3f,%.3f,%.3f "
                        L"right_rot=%.4f,%.4f,%.4f\n",
                        world_hands.sequence,world_hands.input_epoch,
                        world_hands.world_generation,unsigned(world_hands.valid_mask),
                        unsigned(world_hands.grip_down_mask),
                        world_hands.left.position.x,world_hands.left.position.y,world_hands.left.position.z,
                        world_hands.right.position.x,world_hands.right.position.y,world_hands.right.position.z,
                        world_hands.right.rotation.pitch,world_hands.right.rotation.yaw,world_hands.right.rotation.roll);
                    MeshPoseProbe mesh{};
                    const bool known=mesh_pose_probe(world_hands.weapon,mesh);
                    fwprintf(log_file,L"weapon_mesh_probe weapon=%p mesh=%p weapon_mesh=%p "
                        L"component=%ls asset=%ls known=%u "
                        L"translation=%.3f,%.3f,%.3f rotation=%d,%d,%d "
                        L"world_origin=%.3f,%.3f,%.3f absolute=%u,%u,%u hidden=%u\n",
                        reinterpret_cast<void*>(world_hands.weapon),reinterpret_cast<void*>(mesh.mesh),
                        reinterpret_cast<void*>(mesh.weapon_mesh),mesh.component_name,mesh.asset_name,
                        unsigned(known),
                        mesh.translation.x,mesh.translation.y,mesh.translation.z,
                        mesh.rotation.pitch,mesh.rotation.yaw,mesh.rotation.roll,
                        mesh.local_to_world[12],mesh.local_to_world[13],mesh.local_to_world[14],
                        unsigned(mesh.absolute_translation),unsigned(mesh.absolute_rotation),
                        unsigned(mesh.absolute_scale),unsigned(mesh.hidden));
                    MagazineVisualProbe magazine{};
                    const bool magazine_known=magazine_visual_probe(
                        world_hands.weapon,pawn,magazine);
                    fwprintf(log_file,L"weapon_magazine_probe weapon=%p known=%u present=%u "
                        L"component=%p name=%ls asset=%p asset_name=%ls "
                        L"attached_to_pawn=%u hidden=%u\n",
                        reinterpret_cast<void*>(world_hands.weapon),unsigned(magazine_known),
                        unsigned(magazine.present),reinterpret_cast<void*>(magazine.component),
                        magazine.component_name,reinterpret_cast<void*>(magazine.asset),
                        magazine.asset_name,unsigned(magazine.attached_to_pawn),
                        unsigned(magazine.hidden));
                    AmmoBoneVisualProbe ammo_bone{};
                    const bool ammo_bone_known=ammo_bone_visual_probe(
                        world_hands.weapon,ammo_bone);
                    fwprintf(log_file,L"weapon_ammo_bone_probe weapon=%p known=%u present=%u "
                        L"mesh=%p control=%p name=%ls strength=%.3f target=%.3f "
                        L"bone_scale=%.3f\n",
                        reinterpret_cast<void*>(world_hands.weapon),unsigned(ammo_bone_known),
                        unsigned(ammo_bone.present),reinterpret_cast<void*>(ammo_bone.mesh),
                        reinterpret_cast<void*>(ammo_bone.control),ammo_bone.control_name,
                        ammo_bone.strength,ammo_bone.target,ammo_bone.bone_scale);
                    fflush(log_file);
                }
            }else ++pose_rejected;
        }
        if(intent_open&&requested_slot>=4&&intent_bridge.read_next(intent_cursor,grip_event)){
            from_grip=true;
            if(grip_event.kind==gears_weapon::EquipmentIntentKind::slot)
                requested_slot=grip_event.inventory_slot;
            else if(grip_event.kind==gears_weapon::EquipmentIntentKind::grab_ammo)
                ammo_request=true;
            else if(grip_event.kind==gears_weapon::EquipmentIntentKind::reload)
                reload_request=true;
        }
    }
#endif
    if(now<next_probe&&requested_slot>=4
#if defined(GEARS_PLAYGROUND_EQUIPMENT_INTENT_TEST)
       &&!ammo_request&&!reload_request
#if defined(GEARS_PLAYGROUND_LANCER_RELOAD_HOLD_TEST)
       &&!reload_complete_request&&!lancer_reload_hold.active
#endif
#endif
       )return;
#else
    if(now<next_probe)return;
#endif
    next_probe=now+500;
    gears_weapon::EquipmentSnapshot game{};
    const wchar_t* error{};
    const bool ok=snapshot(pc,game,error);
#if defined(GEARS_PLAYGROUND_LANCER_RELOAD_HOLD_TEST)
    if(ok)lancer_refresh(pc,game);
    else if(lancer_reload_hold.active){
        lancer_reload_hold={};lancer_reload_status.store(8,std::memory_order_release);
    }
    if(reload_request){
        const auto& pose=last_intent_pose;
        const bool certified=test_reload_start||
            (from_grip&&last_intent_pose_known&&
             gears_weapon::equipment_intent_matches_pose(grip_event,pose,now,
                 pc,game.pawn,game.current_weapon));
        const auto status=ok&&certified?lancer_begin(pc,game):2u;
        lancer_reload_status.store(status,std::memory_order_release);
        if(log_file){fwprintf(log_file,L"lancer_reload_hold_begin status=%u weapon=%p source=%ls\n",
            status,reinterpret_cast<void*>(static_cast<uptr>(game.current_weapon)),
            test_reload_start?L"test":L"r3");fflush(log_file);}
    }
    if(reload_complete_request){
        unsigned status=2;
        if(ok){
#if defined(GEARS_PLAYGROUND_LANCER_MAGAZINE_FOLLOW_TEST)
            status=lancer_magazine_restore()?lancer_complete(pc,game):7u;
#else
            status=lancer_complete(pc,game);
#endif
        }
        lancer_reload_status.store(status,std::memory_order_release);
        if(log_file){fwprintf(log_file,L"lancer_reload_hold_complete status=%u weapon=%p\n",
            status,reinterpret_cast<void*>(static_cast<uptr>(game.current_weapon)));fflush(log_file);}
    }
#endif
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
#if defined(GEARS_PLAYGROUND_EQUIPMENT_INTENT_TEST)
    // This identity-bound lookup needs a live equipped weapon, but no hand
    // tracking or XR input. It can therefore run in an owned simulator arena.
    if(ok&&log_file&&playground::ready&&test_environment()&&
       pc==playground::arena_pc&&game.world==playground::arena_world&&
       game.pawn==playground::arena_pawn&&game.current_weapon&&
       (now>=next_reload_native_probe||
        static_cast<uptr>(game.current_weapon)!=last_reload_native_weapon)){
        next_reload_native_probe=now+5000;
        last_reload_native_weapon=static_cast<uptr>(game.current_weapon);
        const auto war_weapon=playground::find_class(L"WarWeapon");
        const auto values=reload_native_probe(war_weapon,L"GetActiveReloadValues");
        const auto result=reload_native_probe(
            war_weapon,L"DetermineActiveReloadSuccessOrFailure");
        const auto weapon=static_cast<uptr>(game.current_weapon);
        const auto getter_target=reload_virtual_target(weapon,0x638);
        const auto judge_target=reload_virtual_target(weapon,0x630);
        const auto print_entry=[&](const wchar_t* label,ReloadNativeProbe const& entry){
            fwprintf(log_file,L"weapon_reload_native_probe weapon=%p function=%ls "
                L"reflected=%u ufunction=%p native_index=%u "
                L"parameter_bytes=%u executable_in_game=%u entry_rva=%llx\n",
                reinterpret_cast<void*>(static_cast<uptr>(game.current_weapon)),label,
                unsigned(entry.reflected),reinterpret_cast<void*>(entry.function),
                entry.native_index,entry.parameter_bytes,
                unsigned(entry.executable_in_game),
                entry.executable_in_game?entry.entry-base:0);
        };
        print_entry(L"GetActiveReloadValues",values);
        print_entry(L"DetermineActiveReloadSuccessOrFailure",result);
        fwprintf(log_file,L"weapon_reload_virtual_probe weapon=%p "
            L"getter_slot=638 getter_rva=%llx judge_slot=630 judge_rva=%llx\n",
            reinterpret_cast<void*>(weapon),getter_target?getter_target-base:0,
            judge_target?judge_target-base:0);
        const auto window=reload_window_probe(weapon);
        fwprintf(log_file,L"weapon_reload_window_probe weapon=%p valid=%u "
            L"reload_duration=%.9g start_field=%.9g pre_field=%.9g "
            L"super_field=%.9g sweet_field=%.9g owner_index=%u "
            L"duration_count=%u duration_0=%.9g duration_1=%.9g duration_2=%.9g "
            L"derived_start=%.9g derived_super=%.9g derived_end=%.9g\n",
            reinterpret_cast<void*>(weapon),unsigned(window.valid),
            window.reload_duration,window.start_field,window.pre_field,
            window.super_field,window.sweet_field,window.owner_index,
            window.duration_count,window.durations[0],window.durations[1],
            window.durations[2],window.derived_start,window.derived_super,
            window.derived_end);
        fflush(log_file);
    }
#endif
#if defined(GEARS_PLAYGROUND_LANCER_MAGAZINE_FOLLOW_TEST)
    if(ok)lancer_magazine_test_tick(pc,game);
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_INTENT_TEST)
    if(ammo_request){
        unsigned status=4;
        if(ok&&playground::ready&&pc==playground::arena_pc&&
           game.world==playground::arena_world&&game.pawn==playground::arena_pawn&&
           from_grip&&last_intent_pose_known&&
           gears_weapon::equipment_intent_matches_pose(grip_event,last_intent_pose,
               now,pc,game.pawn,game.current_weapon)){
            bool available{};
            if(read_ammo_supply(static_cast<uptr>(game.current_weapon),available)){
                // A positive check only proves that a physical round/magazine
                // may be offered later; it does not consume or create one.
                status=available?1:(play_empty_slot_sound(game,4)?2:3);
#if defined(GEARS_PLAYGROUND_LANCER_MAGAZINE_FOLLOW_TEST)
                if(available&&lancer_reload_hold.active){
                    status=lancer_magazine_grab(pc,game,last_intent_pose)?5:4;
                }
#endif
            }
        }
        last_ammo_status.store(status,std::memory_order_release);
        if(log_file){fwprintf(log_file,L"ammo_grab_test status=%u weapon=%p\n",status,
            reinterpret_cast<void*>(static_cast<uptr>(game.current_weapon)));fflush(log_file);}
    }
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_SWITCH_TEST)
    if(requested_slot<4){
        // Only an owned private arena may exercise the original state path.
        // Recheck all identities on the game thread; the external request
        // carries a slot number, never a weapon pointer.
        if(!ok||!playground::ready||pc!=playground::arena_pc||
           game.world!=playground::arena_world||game.pawn!=playground::arena_pawn
#if defined(GEARS_PLAYGROUND_EQUIPMENT_INTENT_TEST)
           ||(from_grip&&(!last_intent_pose_known||
               !gears_weapon::equipment_intent_matches_pose(grip_event,last_intent_pose,
                   now,pc,game.pawn,game.current_weapon)))
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
        L"slots=%p,%p,%p,%p grenades=%u known=%u ammo=%u%u%u%u dry_select=%u%u%u%u switch=%u anim=%u\n",
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
        unsigned(game.can_select_without_ammo[0]),unsigned(game.can_select_without_ammo[1]),
        unsigned(game.can_select_without_ammo[2]),unsigned(game.can_select_without_ammo[3]),
        unsigned(game.can_switch),unsigned(game.animation_busy));
    fflush(log_file);
}
}
