#pragma once
#include "anchor_observer.h"

namespace gears_first_person {
// The render thread must not use the game-thread's TLS weapon certificate.
// Resolve the current weapon's registered class chain using the existing
// reflected UObject/registry layout; recheck every observed class before use.
template<class Read> bool weapon_type(gears_anchor::Config const& c,Read read,std::uintptr_t pawn,
                                      unsigned weapon_offset,bool& is_sniper){
    using namespace gears_anchor;
    if(!pawn||weapon_offset>0xfff8||pawn>(std::numeric_limits<std::uintptr_t>::max)()-weapon_offset||
       c.object_internal_index>92||c.object_name>88||c.object_class>88||c.field_super>0xfff8)return false;
    auto bytes=[&](void* dest,std::uintptr_t address,std::size_t size){return address&&size&&
        address<=(std::numeric_limits<std::uintptr_t>::max)()-size&&read(dest,reinterpret_cast<void const*>(address),size);};
    Registry objects{},names{};
    if(!bytes(&objects,c.objects_header,16)||!bytes(&names,c.names_header,16)||!objects.data||!names.data||
       objects.count<=0||objects.count>objects.capacity||objects.capacity>4000000||
       names.count<=0||names.count>names.capacity||names.capacity>4000000)return false;
    struct Observed {std::uintptr_t address{},super{};std::array<unsigned char,0x60> header{};bool parent{};};
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
    }
    is_sniper=sniper;return true;
}
}
