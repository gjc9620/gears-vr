#pragma once
#include "anchor_observer.h"
#include <cerrno>
#include <cstdlib>
#include <type_traits>

namespace gears_anchor {
// read(key, buffer, capacity) has GetPrivateProfileStringW semantics: NUL
// terminated text and a returned length excluding NUL. No process reads here.
// Failure never modifies out. Live startup admission remains the caller's job.
template<class Read> bool parse_config(Read read,std::uintptr_t module_base,Config& out){
    auto number=[&](const wchar_t* key,unsigned long long& value){wchar_t text[64]{};
        const auto length=read(key,text,64);wchar_t* end{};
        if(!length||length>=63||!*text||*text==L'-'||*text==L'+')return false;
        errno=0;value=std::wcstoull(text,&end,0);return errno!=ERANGE&&end&&end!=text&&!*end;};
    auto load=[&](const wchar_t* key,auto& field){unsigned long long value{};
        if(!number(key,value)||value>static_cast<unsigned long long>(std::numeric_limits<std::remove_reference_t<decltype(field)>>::max()))return false;
        field=static_cast<std::remove_reference_t<decltype(field)>>(value);return true;};
    // Mounted wire4 requires reflected Actor.Owner. Reject legacy wire3 rather
    // than treating its nine-property layout as an ownership certificate.
    unsigned schema{};if(!load(L"schema",schema)||(schema!=1&&schema!=2&&schema!=4))return false;
    Config c{};
#define ANCHOR_CONFIG_WIDEN_(text) L##text
#define ANCHOR_CONFIG_WIDEN(text) ANCHOR_CONFIG_WIDEN_(text)
#define ANCHOR_CONFIG_LOAD(field) if(!load(ANCHOR_CONFIG_WIDEN(#field),c.field))return false
    ANCHOR_CONFIG_LOAD(objects_header);ANCHOR_CONFIG_LOAD(names_header);ANCHOR_CONFIG_LOAD(identity_count);
    ANCHOR_CONFIG_LOAD(object_internal_index);ANCHOR_CONFIG_LOAD(object_name);ANCHOR_CONFIG_LOAD(object_class);ANCHOR_CONFIG_LOAD(object_outer);
    ANCHOR_CONFIG_LOAD(field_super);ANCHOR_CONFIG_LOAD(name_entry_index);ANCHOR_CONFIG_LOAD(name_entry_text);
    ANCHOR_CONFIG_LOAD(role_controller);ANCHOR_CONFIG_LOAD(role_local);ANCHOR_CONFIG_LOAD(role_camera);ANCHOR_CONFIG_LOAD(role_pawn);ANCHOR_CONFIG_LOAD(role_level);
    ANCHOR_CONFIG_LOAD(controller_pawn);ANCHOR_CONFIG_LOAD(pawn_controller);ANCHOR_CONFIG_LOAD(pawn_location);ANCHOR_CONFIG_LOAD(pawn_rotation);ANCHOR_CONFIG_LOAD(controller_rotation);
    ANCHOR_CONFIG_LOAD(eye_height);ANCHOR_CONFIG_LOAD(base_eye_height);ANCHOR_CONFIG_LOAD(player_camera);ANCHOR_CONFIG_LOAD(pc_owner);ANCHOR_CONFIG_LOAD(player);
    ANCHOR_CONFIG_LOAD(camera_view_target);ANCHOR_CONFIG_LOAD(camera_timestamp);ANCHOR_CONFIG_LOAD(camera_pov);
    if(schema==4){unsigned enabled{};if(!load(L"mounted_enabled",enabled)||enabled!=1)return false;
        c.mounted_enabled=true;
        ANCHOR_CONFIG_LOAD(mounted_role_driver);ANCHOR_CONFIG_LOAD(mounted_role_inventory);ANCHOR_CONFIG_LOAD(mounted_role_world_info);
        ANCHOR_CONFIG_LOAD(mounted_property_offset_internal);
        for(unsigned i=0;i<c.mounted_properties.size();++i){wchar_t key[64]{};
            std::swprintf(key,64,L"mounted_property%u_role",i);if(!load(key,c.mounted_properties[i].role))return false;
            std::swprintf(key,64,L"mounted_property%u_offset",i);if(!load(key,c.mounted_properties[i].offset))return false;}
    }
    unsigned cinematic{};if(schema==4&&(!load(L"cinematic_enabled",cinematic)||cinematic>1))return false;
    if(schema==2||cinematic){unsigned enabled{};if(!load(L"cinematic_enabled",enabled)||enabled!=1)return false;
        ANCHOR_CONFIG_LOAD(module_base);ANCHOR_CONFIG_LOAD(role_cinematic_property);ANCHOR_CONFIG_LOAD(role_camera_actor_class);
        ANCHOR_CONFIG_LOAD(cinematic_offset);ANCHOR_CONFIG_LOAD(cinematic_mask);
        ANCHOR_CONFIG_LOAD(role_world_info_property);ANCHOR_CONFIG_LOAD(role_world_info);ANCHOR_CONFIG_LOAD(world_info_offset);
        if(c.module_base!=module_base||c.cinematic_offset>0xfffc)return false;
        c.cinematic_enabled=true;}
#undef ANCHOR_CONFIG_LOAD
#undef ANCHOR_CONFIG_WIDEN
#undef ANCHOR_CONFIG_WIDEN_
    if(!c.identity_count||c.identity_count>128||c.object_internal_index>92||c.object_name>88||c.object_class>88||c.object_outer>88)return false;
    const unsigned offsets[]={c.field_super,c.name_entry_index,c.name_entry_text,c.controller_pawn,c.pawn_controller,c.pawn_location,
        c.pawn_rotation,c.controller_rotation,c.eye_height,c.base_eye_height,c.player_camera,c.pc_owner,c.player,c.camera_view_target,c.camera_timestamp,c.camera_pov};
    for(auto offset:offsets)if(offset>=0x10000)return false;
    for(unsigned i=0;i<c.identity_count;++i){auto& id=c.identities[i];wchar_t key[64]{};
        auto identity_field=[&](const wchar_t* label,auto& field){std::swprintf(key,64,L"id%u_%ls",i,label);return load(key,field);};
        if(!identity_field(L"address",id.address)||!identity_field(L"index",id.index)||!identity_field(L"name_index",id.name_index)||
            !identity_field(L"number",id.number)||!identity_field(L"cls",id.cls)||!identity_field(L"outer",id.outer)||
            !identity_field(L"check_super",id.check_super)||!identity_field(L"super",id.super)||id.check_super>1)return false;
        std::swprintf(key,64,L"id%u_name",i);wchar_t name[65]{};const auto length=read(key,name,65);
        if(!length||length>=64||!id.address||!id.cls)return false;std::copy(name,name+length+1,id.name.begin());
        for(unsigned j=0;j<i;++j)if(c.identities[j].address==id.address)return false;
    }
    if(c.mounted_enabled&&!mounted_levels_config(c))return false;
    out=c;return true;
}
}
