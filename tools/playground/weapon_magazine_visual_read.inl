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
