#pragma once
#include "crosshair_assignment.h"

namespace gears_hand_pose {
// Read-only pose context, called only after the current anchor receipt has
// passed submission_matches. Animation/camera ownership is deliberately not
// an input. This is not an equipment edge or permission to write a skeleton.
// The immutable field/player certificate is useful even if its old weapon
// instance has been destroyed, or the certified Weapon field is currently null.
template<class Read> bool current_weapon(gears_crosshair_assignment::Config const& c,
        gears_anchor::Anchor const& anchor,std::uint64_t world,std::uint64_t current_world,
        Read read,std::uintptr_t& out) {
    if(!world||world!=current_world||anchor.unpossessed||!anchor.controller||!anchor.pawn||
       !anchor.level||!c.identities)return false;
    auto const& ids=*c.identities;
    if(!ids.identity_count||ids.identity_count>ids.identities.size()||
       ids.super_count>ids.supers.size()||ids.object_internal_index>92||ids.object_name>88||
       ids.object_class>88||ids.object_outer>88||ids.field_super>=65536||
       ids.name_entry_index>=65536||ids.name_entry_text>=65536||
       ids.property_offset_internal!=0x94)return false;
    const unsigned roles[]{c.controller,c.pawn,c.war_pc,c.war_pawn,c.war_weapon,
        c.controller_pawn.property,c.pawn_controller.property,c.pawn_weapon.property,c.weapon_instigator.property};
    for(auto role:roles)if(role>=ids.identity_count)return false;
    for(unsigned i=0;i<ids.super_count;++i)if(ids.supers[i].identity>=ids.identity_count)return false;
    auto id=[&](unsigned i)->gears_ui::Identity const& {return ids.identities[i];};
    if(id(c.controller).address!=anchor.controller||id(c.pawn).address!=anchor.pawn||
       !gears_anchor::equal_identity(id(c.pawn),anchor.pawn_identity)||
       id(c.pawn).outer!=anchor.level)return false;
    gears_ui::Reader<Read> r{read};gears_ui::Registry objects{},names{};
    if(!gears_ui::registry(r,ids.objects_header,objects)||!gears_ui::registry(r,ids.names_header,names))return false;
    auto fresh=[&](gears_ui::Identity const& value){return value.name[0]&&!value.name.back()&&
        gears_ui::fresh_identity(ids,r,objects,names,value);};
    for(auto role:roles)if(!fresh(id(role)))return false;
    if(!gears_crosshair_assignment::detail::named(c,c.war_pc,L"WarPC",L"Class")||
       !gears_crosshair_assignment::detail::named(c,c.war_pawn,L"WarPawn",L"Class")||
       !gears_crosshair_assignment::detail::named(c,c.war_weapon,L"WarWeapon",L"Class")||
       !gears_ui::derives(ids,id(c.controller).cls,id(c.war_pc).address)||
       !gears_ui::derives(ids,id(c.pawn).cls,id(c.war_pawn).address))return false;
    auto property=[&](gears_crosshair_assignment::Field field,wchar_t const* name,std::uintptr_t owner){
        auto const& p=id(field.property);auto cls=gears_ui::known(ids,p.cls);
        unsigned dimension{},size{},offset{};
        return cls&&fresh(*cls)&&!p.number&&!std::wcscmp(p.name.data(),name)&&
            !std::wcscmp(cls->name.data(),L"ObjectProperty")&&gears_ui::derives(ids,owner,p.outer)&&
            field.offset<=0xfff8&&r.field(dimension,p.address,0x70)&&dimension==1&&
            r.field(size,p.address,0x74)&&size==8&&r.field(offset,p.address,0x94)&&offset==field.offset;
    };
    auto layout=[&]{return property(c.controller_pawn,L"Pawn",id(c.controller).cls)&&
        property(c.pawn_controller,L"Controller",id(c.pawn).cls)&&
        property(c.pawn_weapon,L"Weapon",id(c.pawn).cls)&&
        property(c.weapon_instigator,L"Instigator",id(c.war_weapon).address);};
    auto links=[&](std::uintptr_t weapon){std::uintptr_t value{};
        return r.field(value,anchor.controller,c.controller_pawn.offset)&&value==anchor.pawn&&
            r.field(value,anchor.pawn,c.pawn_controller.offset)&&value==anchor.controller&&
            r.field(value,anchor.pawn,c.pawn_weapon.offset)&&value==weapon&&
            (!weapon||(r.field(value,weapon,c.weapon_instigator.offset)&&value==anchor.pawn));};
    std::uintptr_t weapon{};
    if(!layout()||!r.field(weapon,anchor.pawn,c.pawn_weapon.offset)||!links(weapon))return false;

    // A current non-null weapon may differ from the startup weapon. Capture
    // its object/class headers without touching the obsolete instance, and
    // retain a bounded, reread class chain down to the certified WarWeapon.
    auto capture=[&](std::uintptr_t address,gears_ui::Identity& value){
        gears_ui::Identity candidate{};candidate.address=address;std::array<unsigned char,96> raw{};
        if(!r.bytes(raw.data(),address,raw.size()))return false;
        auto field=[&](auto& v,unsigned offset){std::memcpy(&v,raw.data()+offset,sizeof(v));};
        field(candidate.index,ids.object_internal_index);field(candidate.name_index,ids.object_name);
        field(candidate.number,ids.object_name+4);field(candidate.cls,ids.object_class);field(candidate.outer,ids.object_outer);
        std::uintptr_t entry{};
        if(candidate.name_index>=unsigned(names.count)||!r.field(entry,names.data,candidate.name_index*8)||
           !r.field(candidate.name,entry,ids.name_entry_text)||!fresh(candidate)||
           !std::wcsncmp(candidate.name.data(),L"Default__",9))return false;
        value=candidate;return true;
    };
    gears_ui::Identity weapon_identity{};
    std::array<gears_ui::Identity,40> classes{};std::array<std::uintptr_t,40> parents{};unsigned depth{};
    if(weapon){
        if(!capture(weapon,weapon_identity))return false;
        auto cls=weapon_identity.cls;bool derived=false;
        while(cls&&depth<classes.size()){
            for(unsigned i=0;i<depth;++i)if(classes[i].address==cls)return false;
            if(!capture(cls,classes[depth])||classes[depth].number||classes[depth].cls!=id(c.war_weapon).cls||
               !r.field(parents[depth],cls,ids.field_super))return false;
            ++depth;
            if(cls==id(c.war_weapon).address){derived=true;break;}
            cls=parents[depth-1];
        }
        if(!derived)return false;
    }
    if(!layout()||!links(weapon))return false;
    for(auto role:roles)if(!fresh(id(role)))return false;
    if(weapon&&!fresh(weapon_identity))return false;
    for(unsigned i=0;i<depth;++i){std::uintptr_t parent{};
        if(!fresh(classes[i])||!r.field(parent,classes[i].address,ids.field_super)||parent!=parents[i])return false;}
    gears_ui::Registry after_objects{},after_names{};
    if(!gears_ui::registry(r,ids.objects_header,after_objects)||!gears_ui::registry(r,ids.names_header,after_names)||
       objects.data!=after_objects.data||objects.count!=after_objects.count||objects.capacity!=after_objects.capacity||
       names.data!=after_names.data||names.count!=after_names.count||names.capacity!=after_names.capacity||!links(weapon))return false;
    out=weapon;return true;
}
} // namespace gears_hand_pose
