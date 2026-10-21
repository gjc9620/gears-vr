#pragma once
#include "crosshair_assignment_config_loader.h"

namespace gears_crosshair_assignment {
// Same-Pawn adaptation only. Each attempt starts from the immutable startup
// certificate, never a previous dynamic/TLS candidate. No old actor read is
// required once its single role slot is replaced. Publishing is all-or-nothing.
template<class Read> bool rebind_weapon(OwnedConfig const& startup,Read read,OwnedConfig& out){
    auto const& original=startup.identities;auto const& initial=startup.assignment;
    if(initial.identities!=&original||!original.identity_count||original.identity_count>original.identities.size()||
       original.super_count>original.supers.size()||original.object_internal_index>92||original.object_name>88||
       original.object_class>88||original.object_outer>88||original.field_super>=65536||
       original.name_entry_index>=65536||original.name_entry_text>=65536||original.property_offset_internal!=0x94||
       initial.property_structure<0x60||initial.property_structure>=65536)return false;
    const unsigned roles[]{initial.writer,initial.war_pc,initial.controller,initial.pawn,initial.weapon,
        initial.war_pawn,initial.war_weapon,initial.lhs,initial.member,initial.impact,initial.impact_struct,
        initial.vector_struct,initial.ww,initial.controller_pawn.property,initial.pawn_controller.property,
        initial.pawn_weapon.property,initial.weapon_instigator.property};
    for(auto role:roles)if(role>=original.identity_count)return false;
    if(initial.aim.enabled&&initial.aim.property>=original.identity_count)return false;
    for(unsigned i=0;i<original.super_count;++i)if(original.supers[i].identity>=original.identity_count)return false;
    for(unsigned i=0;i<original.super_count;++i){
        for(unsigned j=0;j<i;++j)if(original.supers[i].identity==original.supers[j].identity)return false;
        std::array<std::uintptr_t,96> seen{};unsigned count{};
        auto current=original.identities[original.supers[i].identity].address;
        while(current){
            if(count==seen.size())return false;
            for(unsigned j=0;j<count;++j)if(seen[j]==current)return false;
            seen[count++]=current;bool found=false;
            for(unsigned j=0;j<original.super_count;++j)
                if(original.identities[original.supers[j].identity].address==current){
                    current=original.supers[j].parent;found=true;break;}
            if(!found)break; // A superclass without a startup edge is read afresh below when needed.
        }
    }
    for(auto field:{initial.controller_pawn,initial.pawn_controller,initial.pawn_weapon,initial.weapon_instigator})
        if(field.offset>=65536)return false;
    if(initial.view.enabled)for(auto role:{initial.view.cam_loc,initial.view.cam_rot,initial.view.start_trace,
        initial.view.trace_dir,initial.view.rotator_struct})if(role>=original.identity_count)return false;
    for(unsigned i=0;i<original.identity_count;++i){auto const& id=original.identities[i];
        if(!id.address||!id.cls||!id.name[0]||id.name.back())return false;
        for(unsigned j=0;j<i;++j)if(id.address==original.identities[j].address||id.index==original.identities[j].index)return false;
    }
    OwnedConfig candidate=startup;auto& ids=candidate.identities;auto& c=candidate.assignment;
    gears_ui::Reader<Read> r{read};gears_ui::Registry objects{},names{};
    if(!gears_ui::registry(r,ids.objects_header,objects)||!gears_ui::registry(r,ids.names_header,names))return false;
    // The Class metaclass must already be an unambiguous startup identity.
    const gears_ui::Identity* metaclass{};
    for(unsigned i=0;i<original.identity_count;++i){auto const& id=original.identities[i];
        if(!id.number&&!std::wcscmp(id.name.data(),L"Class")){if(metaclass)return false;metaclass=&id;}}
    if(!metaclass||metaclass->cls!=metaclass->address||
       !gears_ui::fresh_identity(ids,r,objects,names,*metaclass))return false;
    auto capture=[&](std::uintptr_t address,gears_ui::Identity& result){
        gears_ui::Identity id{};id.address=address;std::array<unsigned char,96> raw{};
        if(!r.bytes(raw.data(),address,raw.size()))return false;
        auto field=[&](auto& value,unsigned offset){std::memcpy(&value,raw.data()+offset,sizeof(value));};
        field(id.index,ids.object_internal_index);field(id.name_index,ids.object_name);
        field(id.number,ids.object_name+4);field(id.cls,ids.object_class);field(id.outer,ids.object_outer);
        std::uintptr_t entry{};unsigned entry_index{};std::array<wchar_t,64> text{};
        if(!id.cls||id.index>=unsigned(objects.count)||id.name_index>=unsigned(names.count)||
           !r.field(entry,names.data,id.name_index*8)||!entry||!r.field(entry_index,entry,ids.name_entry_index)||
           entry_index!=id.name_index||!r.field(text,entry,ids.name_entry_text))return false;
        unsigned length=0;for(;length<text.size()&&text[length];++length){}
        if(!length||length==text.size())return false;
        std::copy_n(text.begin(),length,id.name.begin());
        if(!gears_ui::fresh_identity(ids,r,objects,names,id))return false;
        result=id;return true;
    };
    const auto pawn=ids.identities[c.pawn].address,war_weapon=ids.identities[c.war_weapon].address;
    std::uintptr_t current{};gears_ui::Identity weapon{};
    if(!r.field(current,pawn,c.pawn_weapon.offset)||!current||!capture(current,weapon)||
       !std::wcsncmp(weapon.name.data(),L"Default__",9))return false;
    // Reject collisions with every immutable role; only the old weapon slot
    // may be replaced, including a valid same-slot UObject generation change.
    for(unsigned i=0;i<ids.identity_count;++i)if(i!=c.weapon&&
        (ids.identities[i].address==weapon.address||ids.identities[i].index==weapon.index))return false;
    ids.identities[c.weapon]=weapon;
    const auto original_count=ids.identity_count;
    std::array<std::uintptr_t,40> chain{},parents{};unsigned depth{};auto cls=weapon.cls;
    while(cls){
        if(depth==chain.size())return false;
        for(unsigned i=0;i<depth;++i)if(chain[i]==cls)return false;
        gears_ui::Identity fresh{};std::uintptr_t parent{};
        if(!capture(cls,fresh)||fresh.cls!=metaclass->address||fresh.number||
           !std::wcsncmp(fresh.name.data(),L"Default__",9)||!r.field(parent,cls,ids.field_super))return false;
        if(parent==cls)return false;
        for(unsigned i=0;i<depth;++i)if(parent==chain[i])return false;
        unsigned index=ids.identity_count;
        for(unsigned i=0;i<ids.identity_count;++i){auto const& known=ids.identities[i];
            if(known.address==cls){
                fresh.super=known.super;fresh.check_super=known.check_super;
                if(!gears_anchor::equal_identity(fresh,known))return false;index=i;
            }else if(known.index==fresh.index)return false;
        }
        if(index==ids.identity_count){
            if(ids.identity_count==ids.identities.size())return false;
            fresh.super=parent;fresh.check_super=1;ids.identities[ids.identity_count++]=fresh;
        }
        bool edge=false;
        for(unsigned i=0;i<ids.super_count;++i)if(ids.supers[i].identity==index){
            if(edge||ids.supers[i].parent!=parent)return false;edge=true;}
        if(!edge){if(ids.super_count==ids.supers.size())return false;ids.supers[ids.super_count++]={index,parent};}
        chain[depth]=cls;parents[depth]=parent;++depth;
        if(cls==war_weapon)break;
        cls=parent;
    }
    if(!depth||chain[depth-1]!=war_weapon||!validate_bindings(c,read,false))return false;
    // validate_bindings checks the script header/dispatch; snapshot() normally
    // checks these exact assignment tokens. Rebinding has no FFrame, so retain
    // that bounded writer contract explicitly rather than assuming header
    // identity authenticates script bytes.
    auto assignment_tokens_match=[&](){
        std::array<unsigned char,38> expected{},actual{};expected[0]=0x0f;expected[1]=1;expected[10]=0x35;
        const std::pair<unsigned,unsigned> pointers[]{{2,c.lhs},{11,c.member},{19,c.impact_struct},{30,c.impact}};
        for(auto const& pointer:pointers){const auto address=ids.identities[pointer.second].address;
            std::memcpy(expected.data()+pointer.first,&address,sizeof(address));}
        return r.field(actual,c.script,0x359)&&actual==expected;
    };
    if(!assignment_tokens_match())return false;
    gears_ui::Registry final_objects{},final_names{};std::uintptr_t final_weapon{};
    if(!gears_ui::registry(r,ids.objects_header,final_objects)||!gears_ui::registry(r,ids.names_header,final_names)||
       objects.data!=final_objects.data||objects.count!=final_objects.count||objects.capacity!=final_objects.capacity||
       names.data!=final_names.data||names.count!=final_names.count||names.capacity!=final_names.capacity||
       !r.field(final_weapon,pawn,c.pawn_weapon.offset)||final_weapon!=current||
       !gears_ui::fresh_identity(ids,r,objects,names,ids.identities[c.weapon]))return false;
    for(unsigned i=original_count;i<ids.identity_count;++i)
        if(!gears_ui::fresh_identity(ids,r,objects,names,ids.identities[i]))return false;
    for(unsigned i=0;i<depth;++i){std::uintptr_t final_parent{};
        if(!r.field(final_parent,chain[i],ids.field_super)||final_parent!=parents[i])return false;}
    if(!gears_ui::registry(r,ids.objects_header,final_objects)||!gears_ui::registry(r,ids.names_header,final_names)||
       objects.data!=final_objects.data||objects.count!=final_objects.count||objects.capacity!=final_objects.capacity||
       names.data!=final_names.data||names.count!=final_names.count||names.capacity!=final_names.capacity||
       !r.field(final_weapon,pawn,c.pawn_weapon.offset)||final_weapon!=current)return false;
    if(!assignment_tokens_match())return false;
    out=candidate;return true;
}

// Activation happens after the external certificate was collected. Equipment
// may have changed in that interval, before tick-time rebinding is available.
// Reuse the same-Pawn proof, then require the original (not detoured) entry.
// Never publish a partial candidate or relax an immutable startup check.
template<class Read> bool prepare_activation(OwnedConfig const& startup,Read read,OwnedConfig& out,
    ValidationFailure* failure=nullptr){
    ValidationFailure initial{};
    if(validate_bindings(startup.assignment,read,true,&initial)){
        out=startup;if(failure)*failure=ValidationFailure::none;return true;
    }
    OwnedConfig candidate;
    if(rebind_weapon(startup,read,candidate)&&validate_bindings(candidate.assignment,read,true)){
        out=candidate;if(failure)*failure=ValidationFailure::none;return true;
    }
    if(failure)*failure=initial;return false;
}
}
