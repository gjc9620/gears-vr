// Included in gears_anchor after Config/Identity. Every mounted role is immutable
// for this certificate. A dismount, Driver change or new Turret requires rebind.
inline bool mounted_levels_config(Config const& c){
    if(!c.identity_count||c.identity_count>c.identities.size()||c.role_level>=c.identity_count)return false;
    auto known=[&](std::uintptr_t address)->Identity const*{for(unsigned i=0;i<c.identity_count;++i)if(c.identities[i].address==address)return &c.identities[i];return nullptr;};
    auto named=[](Identity const* id,wchar_t const* name){return id&&!id->name.back()&&!id->number&&!std::wcscmp(id->name.data(),name);};
    auto const& level=c.identities[c.role_level];
    for(auto role:{c.role_pawn,c.mounted_role_driver,c.role_controller,c.role_camera,c.mounted_role_world_info}){
        if(role>=c.identity_count)return false;auto owner=known(c.identities[role].outer);
        if(!owner||!owner->name[0]||owner->name.back()||!std::wcsncmp(owner->name.data(),L"Default__",9)||owner->cls!=level.cls)return false;
        bool is_level=false;auto cls=owner->cls;
        for(unsigned n=0;n<64&&cls;++n){auto id=known(cls);
            if(!id||!id->check_super||!named(known(id->cls),L"Class"))return false;
            if(named(id,L"Level")){is_level=true;break;}cls=id->super;}
        if(!is_level)return false;
    }
    return c.mounted_role_inventory<c.identity_count &&
        c.identities[c.mounted_role_inventory].outer==c.identities[c.mounted_role_driver].outer &&
        c.identities[c.role_pawn].outer==level.address;
}
template<class Read> bool mounted_links(Config const& c,Read read){
    if(!c.mounted_enabled||!mounted_levels_config(c)||c.mounted_property_offset_internal>0xfffc)return false;
    const unsigned roles[]={c.role_controller,c.role_local,c.role_camera,c.role_pawn,c.role_level,
        c.mounted_role_driver,c.mounted_role_inventory,c.mounted_role_world_info};
    for(unsigned i=0;i<std::size(roles);++i){if(roles[i]>=c.identity_count)return false;
        for(unsigned j=0;j<i;++j)if(roles[j]==roles[i])return false;}
    auto const& controlled=c.identities[c.role_pawn];auto const& driver=c.identities[c.mounted_role_driver];
    auto const& inventory=c.identities[c.mounted_role_inventory];auto const& world=c.identities[c.mounted_role_world_info];
    const auto pc=c.identities[c.role_controller].address,camera=c.identities[c.role_camera].address,
        local=c.identities[c.role_local].address,level=c.identities[c.role_level].address;
    auto known=[&](std::uintptr_t address)->Identity const*{for(unsigned i=0;i<c.identity_count;++i)if(c.identities[i].address==address)return &c.identities[i];return nullptr;};
    auto named=[](Identity const* id,wchar_t const* name){return id&&!id->name.back()&&!id->number&&!std::wcscmp(id->name.data(),name);};
    auto inherits=[&](std::uintptr_t cls,wchar_t const* name){for(unsigned n=0;n<64&&cls;++n){auto id=known(cls);
        if(!id||!id->check_super||!named(known(id->cls),L"Class"))return false;if(named(id,name))return true;cls=id->super;}return false;};
    auto owns=[&](std::uintptr_t cls,std::uintptr_t owner){for(unsigned n=0;n<64&&cls;++n){auto id=known(cls);
        if(!id||!id->check_super)return false;if(cls==owner)return true;cls=id->super;}return false;};
    for(auto role:roles){auto const& id=c.identities[role];if(!id.address||!id.name[0]||!std::wcsncmp(id.name.data(),L"Default__",9))return false;}
    if(!inherits(controlled.cls,L"Turret")||!inherits(controlled.cls,L"Vehicle")||!inherits(driver.cls,L"WarPawn")||
       !inherits(inventory.cls,L"WarInventoryManager")||!inherits(world.cls,L"WorldInfo")||
       controlled.outer!=level||inventory.outer!=driver.outer)return false;
    // Each streamed actor's immediate Level is part of the immutable identity
    // table and rechecked by the enclosing guard's registry/name brackets.
    auto field=[&](auto& v,std::uintptr_t address,unsigned offset){return address&&address<=(std::numeric_limits<std::uintptr_t>::max)()-offset-sizeof(v)&&
        read(&v,reinterpret_cast<const void*>(address+offset),sizeof(v));};
    const wchar_t* names[]={L"Pawn",L"Controller",L"Driver",L"DrivenVehicle",L"PlayerCamera",L"PCOwner",L"Player",L"WorldInfo",L"InvManager",L"Owner"};
    const wchar_t* owners[]={L"Controller",L"Pawn",L"Vehicle",L"Pawn",L"PlayerController",L"Camera",L"PlayerController",L"Actor",L"Pawn",L"Actor"};
    const unsigned sources[]={c.role_controller,c.role_pawn,c.role_pawn,c.mounted_role_driver,c.role_controller,c.role_camera,c.role_controller,c.role_controller,c.mounted_role_driver,c.mounted_role_inventory};
    for(unsigned i=0;i<c.mounted_properties.size();++i){auto const& prop=c.mounted_properties[i];
        if(prop.role>=c.identity_count||prop.offset>0xfff8)return false;auto const& id=c.identities[prop.role];
        auto kind=known(id.cls),owner=known(id.outer);unsigned dim{},size{},offset{};
        if(!named(&id,names[i])||!named(kind,L"ObjectProperty")||!named(known(kind->cls),L"Class")||!named(owner,owners[i])||
           !named(known(owner->cls),L"Class")||!owns(c.identities[sources[i]].cls,id.outer)||
           !field(dim,id.address,0x70)||dim!=1||!field(size,id.address,0x74)||size!=8||
           !field(offset,id.address,c.mounted_property_offset_internal)||offset!=prop.offset)return false;
    }
    auto offset=[&](unsigned i){return c.mounted_properties[i].offset;};
    if(offset(0)!=c.controller_pawn||offset(1)!=c.pawn_controller||offset(4)!=c.player_camera||offset(5)!=c.pc_owner||offset(6)!=c.player)return false;
    // Controller is reflected once on Pawn and applies to both roles; Actor's
    // WorldInfo applies to controlled, human Driver, camera and controller.
    if(!owns(driver.cls,c.identities[c.mounted_properties[1].role].outer)||
       !owns(controlled.cls,c.identities[c.mounted_properties[7].role].outer)||
       !owns(driver.cls,c.identities[c.mounted_properties[7].role].outer)||
       !owns(c.identities[c.role_camera].cls,c.identities[c.mounted_properties[7].role].outer))return false;
    auto link=[&](std::uintptr_t source,unsigned off,std::uintptr_t expected){std::uintptr_t actual{};return field(actual,source,off)&&actual==expected;};
    return link(pc,offset(0),controlled.address)&&link(controlled.address,offset(1),pc)&&link(controlled.address,offset(2),driver.address)&&
        link(driver.address,offset(3),controlled.address)&&link(driver.address,offset(1),0)&&link(driver.address,offset(8),inventory.address)&&link(inventory.address,offset(9),driver.address)&&
        link(pc,offset(4),camera)&&link(camera,offset(5),pc)&&link(pc,offset(6),local)&&link(camera,c.camera_view_target,controlled.address)&&
        link(pc,offset(7),world.address)&&link(controlled.address,offset(7),world.address)&&link(driver.address,offset(7),world.address)&&link(camera,offset(7),world.address);
}
