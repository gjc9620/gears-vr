#pragma once
#include "anchor_observer.h"

namespace gears_first_person {
struct WeaponState {bool sniper{},zoomed{};};
// Address hint only. Every use revalidates the property, owner, Bool layout,
// live registry slots and flag; no cached value authorizes a camera decision.
struct ZoomHint {std::uintptr_t owner{},property{};};
// The render thread must not use the game-thread's TLS weapon certificate.
// Resolve the current weapon's registered class chain using the existing
// reflected UObject/registry layout; recheck every observed class before use.
template<class Read> bool weapon_state(gears_anchor::Config const& c,Read read,std::uintptr_t pawn,
    unsigned weapon_offset,WeaponState& out,ZoomHint& hint,bool inspect_zoom=true){
    using namespace gears_anchor;
    const auto previous_hint=hint;hint={};
    if(!pawn||weapon_offset>0xfff8||pawn>(std::numeric_limits<std::uintptr_t>::max)()-weapon_offset||
       c.object_internal_index>92||c.object_name>88||c.object_class>88||c.object_outer>88||c.field_super>0xfff8)return false;
    auto bytes=[&](void* dest,std::uintptr_t address,std::size_t size){return address&&size&&
        address<=(std::numeric_limits<std::uintptr_t>::max)()-size&&read(dest,reinterpret_cast<void const*>(address),size);};
    auto field=[&](auto& value,std::uintptr_t address,unsigned offset){return
        address<=(std::numeric_limits<std::uintptr_t>::max)()-offset&&bytes(&value,address+offset,sizeof(value));};
    Registry objects{},names{};
    if(!bytes(&objects,c.objects_header,16)||!bytes(&names,c.names_header,16)||!objects.data||!names.data||
       objects.count<=0||objects.count>objects.capacity||objects.capacity>4000000||
       names.count<=0||names.count>names.capacity||names.capacity>4000000)return false;
    struct Observed {std::uintptr_t address{},super{},name_entry{};unsigned name_index{};
        std::array<unsigned char,0x60> header{};std::array<wchar_t,64> name{};bool parent{};};
    std::array<Observed,24> observed{};unsigned count{};
    auto object=[&](std::uintptr_t address,std::uintptr_t& cls,std::array<wchar_t,64>& name,bool parent)->bool{
        if(count>=observed.size())return false;
        Observed value{};value.address=address;value.parent=parent;
        if(!bytes(value.header.data(),address,value.header.size()))return false;
        unsigned index{},name_index{},number{};std::uintptr_t slot{},entry{};
        std::memcpy(&index,value.header.data()+c.object_internal_index,4);
        std::memcpy(&name_index,value.header.data()+c.object_name,4);
        std::memcpy(&number,value.header.data()+c.object_name+4,4);
        std::memcpy(&cls,value.header.data()+c.object_class,8);
        if(index>=unsigned(objects.count)||name_index>=unsigned(names.count)||(parent&&number)||!cls||
           !bytes(&slot,objects.data+std::uintptr_t(index)*8,8)||slot!=address||
           !bytes(&entry,names.data+std::uintptr_t(name_index)*8,8)||!entry)return false;
        unsigned entry_index{};
        if(!bytes(&entry_index,entry+c.name_entry_index,4)||entry_index!=name_index||
           !bytes(name.data(),entry+c.name_entry_text,sizeof(name))||!name[0]||
           std::find(name.begin(),name.end(),wchar_t{})==name.end())return false;
        value.name_entry=entry;value.name_index=name_index;value.name=name;
        if(parent&&!bytes(&value.super,address+c.field_super,8))return false;
        observed[count++]=value;return true;
    };
    std::uintptr_t weapon{},cls{};std::array<wchar_t,64> name{};
    if(!bytes(&weapon,pawn+weapon_offset,8)||!weapon||!object(weapon,cls,name,false))return false;
    bool known=false,sniper=false;
    for(unsigned depth=0;depth<16&&cls;++depth){
        std::uintptr_t meta{};std::array<wchar_t,64> class_name{},meta_name{};
        if(!object(cls,meta,class_name,true))return false;
        const auto parent=observed[count-1].super;std::uintptr_t unused{};
        if(!object(meta,unused,meta_name,false)||std::wcscmp(meta_name.data(),L"Class"))return false;
        if(!std::wcscmp(class_name.data(),L"Weap_SniperRifle")){sniper=true;known=true;break;}
        if(!std::wcscmp(class_name.data(),L"WarWeapon")){known=true;break;}
        cls=parent;
    }
    if(!known)return false;
    std::uintptr_t zoom_property{};unsigned zoom_offset{},zoom_mask{},zoom_word{};
    auto zoom_layout=[&](){
        unsigned dimension{},size{},offset{},mask{};std::uintptr_t vtable{},accessor{},dispatch{};
        return c.cinematic_enabled&&c.module_base&&
            field(dimension,zoom_property,0x70)&&dimension==1&&field(size,zoom_property,0x74)&&size==4&&
            field(offset,zoom_property,0x94)&&offset==zoom_offset&&offset<=65532&&
            field(mask,zoom_property,0xd0)&&mask==zoom_mask&&mask&&!(mask&(mask-1))&&
            field(vtable,zoom_property,0)&&vtable&&field(accessor,vtable,0x2a0)&&accessor==c.module_base+cinematic_bool_accessor_rva&&
            field(dispatch,c.module_base,unsigned(cinematic_gnatives_rva+0x2d*8))&&dispatch==c.module_base+cinematic_bool_native_rva;
    };
    if(sniper&&inspect_zoom){
        if(previous_hint.owner==cls&&previous_hint.property)zoom_property=previous_hint.property;
        else {
            // Supported EXE's reflected UStruct.Children / UField.Next layout
            // (reloaded-profile.json). The traversal only discovers a hint;
            // the selected property is authenticated independently below.
            std::uintptr_t child{};std::array<std::uintptr_t,256> visited{};unsigned children{};
            if(!field(child,cls,0x80))return false;
            while(child){
                if(children==visited.size()||std::find(visited.begin(),visited.begin()+children,child)!=visited.begin()+children)return false;
                visited[children++]=child;
                const auto retained=count;std::uintptr_t type{};std::array<wchar_t,64> child_name{};
                if(!object(child,type,child_name,false))return false;
                count=retained;
                if(!std::wcscmp(child_name.data(),L"bZoomed")){zoom_property=child;break;}
                if(!field(child,child,0x68))return false;
            }
            if(!zoom_property)return false;
        }
        std::uintptr_t type{},meta{},unused{};std::array<wchar_t,64> property_name{},type_name{},meta_name{};
        if(!object(zoom_property,type,property_name,false)||std::wcscmp(property_name.data(),L"bZoomed"))return false;
        const auto& property=observed[count-1];unsigned number{};std::uintptr_t owner{};
        std::memcpy(&number,property.header.data()+c.object_name+4,4);
        std::memcpy(&owner,property.header.data()+c.object_outer,8);
        auto numbered=[&](){unsigned n{};std::memcpy(&n,observed[count-1].header.data()+c.object_name+4,4);return n!=0;};
        if(number||owner!=cls||!object(type,meta,type_name,false)||numbered()||std::wcscmp(type_name.data(),L"BoolProperty")||
           !object(meta,unused,meta_name,false)||numbered()||std::wcscmp(meta_name.data(),L"Class")||
           !field(zoom_offset,zoom_property,0x94)||!field(zoom_mask,zoom_property,0xd0)||!zoom_layout()||
           !field(zoom_word,weapon,zoom_offset))return false;
    }
    std::uintptr_t final_weapon{};Registry final_objects{},final_names{};
    if(!bytes(&final_weapon,pawn+weapon_offset,8)||final_weapon!=weapon||
       !bytes(&final_objects,c.objects_header,16)||!bytes(&final_names,c.names_header,16)||
       std::memcmp(&objects,&final_objects,16)||std::memcmp(&names,&final_names,16))return false;
    for(unsigned i=0;i<count;++i){
        auto const& value=observed[i];std::array<unsigned char,0x60> header{};
        std::uintptr_t super{},slot{};unsigned index{};
        std::memcpy(&index,value.header.data()+c.object_internal_index,4);
        if(!bytes(header.data(),value.address,header.size())||header!=value.header||
           !bytes(&slot,objects.data+std::uintptr_t(index)*8,8)||slot!=value.address)return false;
        if(value.parent&&(!bytes(&super,value.address+c.field_super,8)||super!=value.super))return false;
        std::uintptr_t entry{};unsigned entry_index{};std::array<wchar_t,64> final_name{};
        if(!bytes(&entry,names.data+std::uintptr_t(value.name_index)*8,8)||entry!=value.name_entry||
           !field(entry_index,entry,c.name_entry_index)||entry_index!=value.name_index||
           !bytes(final_name.data(),entry+c.name_entry_text,sizeof(final_name))||final_name!=value.name)return false;
    }
    if(zoom_property){unsigned after{};
        if(!zoom_layout()||!field(after,weapon,zoom_offset)||((after^zoom_word)&zoom_mask))return false;
    }
    if(!field(final_weapon,pawn,weapon_offset)||final_weapon!=weapon||
       !bytes(&final_objects,c.objects_header,16)||!bytes(&final_names,c.names_header,16)||
       std::memcmp(&objects,&final_objects,16)||std::memcmp(&names,&final_names,16))return false;
    if(zoom_property)hint={cls,zoom_property};
    out={sniper,bool(zoom_word&zoom_mask)};return true;
}
template<class Read> bool weapon_type(gears_anchor::Config const& c,Read read,std::uintptr_t pawn,
    unsigned weapon_offset,bool& is_sniper){
    WeaponState state{};ZoomHint hint{};
    if(!weapon_state(c,read,pawn,weapon_offset,state,hint,false))return false;
    is_sniper=state.sniper;return true;
}
}
