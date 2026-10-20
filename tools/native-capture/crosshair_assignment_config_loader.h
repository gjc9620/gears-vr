#pragma once
#include "crosshair_assignment.h"
#include <cerrno>
#include <cstdlib>
#include <type_traits>

namespace gears_crosshair_assignment {
struct TickRoot {unsigned owner{},function{};std::uintptr_t script{};unsigned size{};};
// Stable owned storage. Copy/move always rebind the internal pointer.
struct OwnedConfig {
    gears_ui::Config identities{}; Config assignment{};
    bool mounted_suspended{};std::uintptr_t suspended_controller{},suspended_controlled{},suspended_driver{},suspended_camera{};
    unsigned pid{},tick_root_count{};std::array<TickRoot,2> tick_roots{};
    OwnedConfig(){assignment.identities=&identities;}
    OwnedConfig(OwnedConfig const& x){*this=x;}
    OwnedConfig& operator=(OwnedConfig const& x){
        mounted_suspended=x.mounted_suspended;suspended_controller=x.suspended_controller;suspended_controlled=x.suspended_controlled;
        suspended_driver=x.suspended_driver;suspended_camera=x.suspended_camera;
        identities=x.identities;assignment=x.assignment;pid=x.pid;
        tick_root_count=x.tick_root_count;tick_roots=x.tick_roots;
        assignment.identities=&identities;return *this;
    }
    OwnedConfig(OwnedConfig&& x) noexcept {*this=x;}
    OwnedConfig& operator=(OwnedConfig&& x) noexcept {return *this=x;}
};

// read(key,buffer,capacity): GetPrivateProfileStringW semantics. Section is
// selected by caller. Pure parsing, NOT live admission; validate_bindings and
// current script/root identities must still be checked immediately at startup.
template<class Read> bool parse_config(Read read,unsigned expected_pid,std::uintptr_t module_base,OwnedConfig& out){
    auto text=[&](const wchar_t* key,wchar_t* buffer,unsigned capacity){
        const auto length=read(key,buffer,capacity);
        if(!length||length>=capacity-1||buffer[length]!=0||std::wcslen(buffer)!=length)return false;
        for(unsigned i=0;i<length;++i)if(buffer[i]==L'\r'||buffer[i]==L'\n'||buffer[i]==L'=')return false;
        return true;
    };
    auto load=[&](const wchar_t* key,auto& field){wchar_t buffer[64]{};
        if(!text(key,buffer,64)||buffer[0]<L'0'||buffer[0]>L'9')return false;
        for(auto* p=buffer;*p;++p)if(*p==L' '||*p==L'\t'||*p==L'+'||*p==L'-')return false;
        errno=0;wchar_t* end{};auto value=std::wcstoull(buffer,&end,0);
        using T=std::remove_reference_t<decltype(field)>;
        if(errno==ERANGE||!end||*end||value>static_cast<unsigned long long>((std::numeric_limits<T>::max)()))return false;
        field=static_cast<T>(value);return true;
    };
    OwnedConfig parsed;auto& c=parsed.assignment;auto& ids=parsed.identities;
    unsigned schema{},contract{};wchar_t sha[66]{};
    if(!load(L"schema",schema)||(schema!=1&&schema!=2)||!load(L"native_contract",contract)||contract!=schema||
       !text(L"sha256",sha,66)||std::wcscmp(sha,L"bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4")||
       !load(L"pid",parsed.pid)||!expected_pid||parsed.pid!=expected_pid||
       !load(L"base",ids.base)||!module_base||ids.base!=module_base||
       module_base>(std::numeric_limits<std::uintptr_t>::max)()-0x1b9cc30)return false;
    if(schema==2){
        if(!load(L"controller",parsed.suspended_controller)||!load(L"controlled",parsed.suspended_controlled)||
           !load(L"driver",parsed.suspended_driver)||!load(L"camera",parsed.suspended_camera)||
           !parsed.suspended_controller||!parsed.suspended_controlled||!parsed.suspended_driver||!parsed.suspended_camera)return false;
        parsed.mounted_suspended=true;out=parsed;return true;
    }
#define CH_W_(x) L##x
#define CH_W(x) CH_W_(x)
#define CH_ID(x) if(!load(CH_W(#x),ids.x))return false
    CH_ID(objects_header);CH_ID(names_header);CH_ID(identity_count);CH_ID(super_count);
    CH_ID(object_internal_index);CH_ID(object_name);CH_ID(object_class);CH_ID(object_outer);
    CH_ID(field_super);CH_ID(name_entry_index);CH_ID(name_entry_text);CH_ID(property_offset_internal);
#undef CH_ID
    if(!ids.objects_header||!ids.names_header||!ids.identity_count||ids.identity_count>96||ids.super_count>96||
       ids.object_internal_index>92||ids.object_name>88||ids.object_class>88||ids.object_outer>88||
       ids.field_super>=65536||ids.name_entry_index>=65536||ids.name_entry_text>=65536||ids.property_offset_internal!=0x94)return false;
    for(unsigned i=0;i<ids.identity_count;++i){auto& id=ids.identities[i];wchar_t key[64]{};
        auto field=[&](const wchar_t* name,auto& value){std::swprintf(key,64,L"id%u_%ls",i,name);return load(key,value);};
        if(!field(L"address",id.address)||!field(L"index",id.index)||!field(L"name_index",id.name_index)||
           !field(L"number",id.number)||!field(L"cls",id.cls)||!field(L"outer",id.outer)||
           !id.address||!id.cls||id.index>=4000000||id.name_index>=4000000||id.number>=4000000)return false;
        std::swprintf(key,64,L"id%u_name",i);if(!text(key,id.name.data(),unsigned(id.name.size())))return false;
        for(unsigned j=0;j<i;++j)if(ids.identities[j].address==id.address||ids.identities[j].index==id.index)return false;
    }
    for(unsigned i=0;i<ids.identity_count;++i)if(!gears_ui::known(ids,ids.identities[i].cls))return false;
    for(unsigned i=0;i<ids.super_count;++i){auto& s=ids.supers[i];wchar_t key[64]{};
        std::swprintf(key,64,L"super%u_identity",i);if(!load(key,s.identity)||s.identity>=ids.identity_count)return false;
        std::swprintf(key,64,L"super%u_parent",i);if(!load(key,s.parent)||(s.parent&&!gears_ui::known(ids,s.parent)))return false;
        for(unsigned j=0;j<i;++j)if(ids.supers[j].identity==s.identity)return false;
        auto cls=gears_ui::known(ids,ids.identities[s.identity].cls);if(!cls||std::wcscmp(cls->name.data(),L"Class"))return false;
    }
    for(unsigned i=0;i<ids.super_count;++i){std::array<std::uintptr_t,40> seen{};
        auto current=ids.identities[ids.supers[i].identity].address;unsigned count{};
        while(current){if(count==seen.size())return false;
            for(unsigned j=0;j<count;++j)if(seen[j]==current)return false;
            seen[count++]=current;bool found=false;
            for(unsigned j=0;j<ids.super_count;++j)if(ids.identities[ids.supers[j].identity].address==current){
                current=ids.supers[j].parent;found=true;break;}
            if(!found)return false;
        }
    }
#define CH_ROLE(x) if(!load(CH_W(#x),c.x)||c.x>=ids.identity_count)return false
    CH_ROLE(writer);CH_ROLE(war_pc);CH_ROLE(controller);CH_ROLE(pawn);CH_ROLE(weapon);CH_ROLE(war_pawn);CH_ROLE(war_weapon);
    CH_ROLE(lhs);CH_ROLE(member);CH_ROLE(impact);CH_ROLE(impact_struct);CH_ROLE(vector_struct);CH_ROLE(ww);
#undef CH_ROLE
    const unsigned roles[]{c.writer,c.war_pc,c.controller,c.pawn,c.weapon,c.war_pawn,c.war_weapon,c.lhs,c.member,c.impact,c.impact_struct,c.vector_struct,c.ww};
    for(unsigned i=0;i<std::size(roles);++i)for(unsigned j=0;j<i;++j)if(roles[i]==roles[j])return false;
    auto addr=[&](unsigned role){return ids.identities[role].address;};
    if(!detail::named(c,c.war_pc,L"WarPC",L"Class")||!detail::named(c,c.war_pawn,L"WarPawn",L"Class")||
       !detail::named(c,c.war_weapon,L"WarWeapon",L"Class")||
       !detail::named(c,c.writer,L"UpdateHUDActions",L"Function",addr(c.war_pc),true)||
       !detail::named(c,c.lhs,L"LastCrosshairHitLocation",L"StructProperty",addr(c.war_pc),true)||
       !detail::named(c,c.member,L"HitLocation",L"StructProperty",addr(c.impact_struct),true)||
       !detail::named(c,c.impact,L"Impact",L"StructProperty",addr(c.writer),true)||
       !detail::named(c,c.ww,L"WW",L"ObjectProperty",addr(c.writer),true)||
       !detail::named(c,c.impact_struct,L"ImpactInfo",L"ScriptStruct")||!detail::named(c,c.vector_struct,L"Vector",L"ScriptStruct"))return false;
    for(auto pair:{std::pair<unsigned,unsigned>{c.controller,c.war_pc},{c.pawn,c.war_pawn},{c.weapon,c.war_weapon}})
        if(!gears_ui::derives(ids,ids.identities[pair.first].cls,addr(pair.second))||
           !std::wcsncmp(ids.identities[pair.first].name.data(),L"Default__",9))return false;
#define CH_VALUE(x) if(!load(CH_W(#x),c.x))return false
    CH_VALUE(script);CH_VALUE(script_size);CH_VALUE(impact_offset);CH_VALUE(member_offset);CH_VALUE(ww_offset);CH_VALUE(impact_size);CH_VALUE(property_structure);
#undef CH_VALUE
    if(!c.script||c.script_size<0x380||c.script_size>131072||c.script>(std::numeric_limits<std::uintptr_t>::max)()-c.script_size||
       c.impact_offset>=65536||c.ww_offset>=65536||c.impact_size<12||c.impact_size>65536||
       c.member_offset>c.impact_size-12||c.property_structure<0x60||c.property_structure>=65536)return false;
    // Optional legacy transport. Empty and absent are indistinguishable with
    // the caller's empty-default GetPrivateProfileStringW adapter.
    wchar_t view_text[64]{};const auto view_length=read(L"view_enabled",view_text,64);
    if(view_length){unsigned enabled{};
        if(!load(L"view_enabled",enabled)||enabled>1)return false;
        c.view.enabled=enabled!=0;
    }
    if(c.view.enabled){
#define CH_VIEW_ROLE(x) if(!load(CH_W("view_" #x),c.view.x)||c.view.x>=ids.identity_count)return false
        CH_VIEW_ROLE(cam_loc);CH_VIEW_ROLE(cam_rot);CH_VIEW_ROLE(start_trace);CH_VIEW_ROLE(trace_dir);CH_VIEW_ROLE(rotator_struct);
#undef CH_VIEW_ROLE
#define CH_VIEW_OFFSET(x) if(!load(CH_W("view_" #x "_offset"),c.view.x##_offset)||c.view.x##_offset>=65524)return false
        CH_VIEW_OFFSET(cam_loc);CH_VIEW_OFFSET(cam_rot);CH_VIEW_OFFSET(start_trace);CH_VIEW_OFFSET(trace_dir);
#undef CH_VIEW_OFFSET
        if(!detail::named(c,c.view.cam_loc,L"CamLoc",L"StructProperty",addr(c.writer),true)||
           !detail::named(c,c.view.cam_rot,L"CamRot",L"StructProperty",addr(c.writer),true)||
           !detail::named(c,c.view.start_trace,L"StartTrace",L"StructProperty",addr(c.writer),true)||
           !detail::named(c,c.view.trace_dir,L"TraceDir",L"StructProperty",addr(c.writer),true)||
           !detail::named(c,c.view.rotator_struct,L"Rotator",L"ScriptStruct"))return false;
        const unsigned offsets[]{c.view.cam_loc_offset,c.view.cam_rot_offset,c.view.start_trace_offset,c.view.trace_dir_offset};
        for(unsigned i=0;i<std::size(offsets);++i){
            const auto overlap=[&](unsigned start,unsigned size){return
                std::uint64_t(offsets[i])<std::uint64_t(start)+size&&std::uint64_t(start)<std::uint64_t(offsets[i])+12;};
            if(overlap(c.ww_offset,8)||overlap(c.impact_offset,c.impact_size))return false;
            for(unsigned j=0;j<i;++j)if(overlap(offsets[j],12))return false;
        }
    }
    wchar_t aim_text[64]{};
    if(read(L"aim_enabled",aim_text,64)){unsigned enabled{};
        if(!load(L"aim_enabled",enabled)||enabled>1)return false;
        c.aim.enabled=enabled!=0;
    }
    if(c.aim.enabled){
        if(!c.view.enabled||!load(L"aim_property",c.aim.property)||c.aim.property>=ids.identity_count||
           !load(L"aim_offset",c.aim.offset)||c.aim.offset>65532||!load(L"aim_mask",c.aim.mask)||
           !c.aim.mask||(c.aim.mask&(c.aim.mask-1))||
           !detail::named(c,c.aim.property,L"bIsTargeting",L"BoolProperty")||
           !gears_ui::derives(ids,ids.identities[c.controller].cls,ids.identities[c.aim.property].outer))return false;
    }
    auto link=[&](const wchar_t* key,const wchar_t* name,unsigned owner,Field& f){wchar_t k[64]{};
        std::swprintf(k,64,L"%ls_property",key);if(!load(k,f.property)||f.property>=ids.identity_count)return false;
        std::swprintf(k,64,L"%ls_offset",key);if(!load(k,f.offset)||f.offset>=65536)return false;
        auto const& prop=ids.identities[f.property];
        return detail::named(c,f.property,name,L"ObjectProperty")&&gears_ui::derives(ids,ids.identities[owner].cls,prop.outer);
    };
    if(!link(L"controller_pawn",L"Pawn",c.controller,c.controller_pawn)||!link(L"pawn_controller",L"Controller",c.pawn,c.pawn_controller)||
       !link(L"pawn_weapon",L"Weapon",c.pawn,c.pawn_weapon)||!link(L"weapon_instigator",L"Instigator",c.weapon,c.weapon_instigator))return false;
    if(!load(L"tick_root_count",parsed.tick_root_count)||(parsed.tick_root_count!=0&&parsed.tick_root_count!=2))return false;
    for(unsigned i=0;i<parsed.tick_root_count;++i){auto& root=parsed.tick_roots[i];wchar_t k[64]{};
        auto field=[&](const wchar_t* name,auto& value){std::swprintf(k,64,L"root%u_%ls",i,name);return load(k,value);};
        if(!field(L"owner",root.owner)||!field(L"function",root.function)||!field(L"script",root.script)||!field(L"size",root.size)||
           root.owner>=ids.identity_count||root.function>=ids.identity_count||!root.script||!root.size||root.size>131072||
           root.script>(std::numeric_limits<std::uintptr_t>::max)()-root.size||
           !detail::named(c,root.function,L"PlayerTick",L"Function",addr(root.owner),true))return false;
        if(i==0?root.owner!=c.war_pc:!detail::named(c,root.owner,L"PlayerDriving",L"State",addr(c.war_pc),true))return false;
        if(i&&root.function==parsed.tick_roots[0].function)return false;
    }
#undef CH_W
#undef CH_W_
    out=parsed;return true;
}
} // namespace gears_crosshair_assignment
