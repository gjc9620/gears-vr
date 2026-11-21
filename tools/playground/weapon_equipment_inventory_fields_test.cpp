#include "../vr-common/weapon_hand_participant.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <string>
#include "weapon_metadata_parse.h"

// Exercise the ACTUAL shared selector and actual new typed pair with owned
// descriptors. No engine/registry/current Frame/FPS or gameplay claim.
#define GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL 1
namespace {
using uptr=std::uintptr_t;namespace abi=gears_weapon_hand;
struct Node {std::array<unsigned char,256> bytes{};std::wstring label;bool live{};};
std::array<Node,160> nodes;unsigned used{},checks{},name_reads{};uptr fail_get{};
void require(bool value,const char* why){++checks;if(!value){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
uptr make(wchar_t const* label){if(used>=nodes.size())std::abort();auto& n=nodes[used++];n={};n.label=label;n.live=true;return reinterpret_cast<uptr>(n.bytes.data());}
bool registered(uptr address){for(unsigned i=0;i<used;++i)if(address==reinterpret_cast<uptr>(nodes[i].bytes.data()))return nodes[i].live;return false;}
template<class T>void put(uptr address,T const& value){std::memcpy(reinterpret_cast<void*>(address),&value,sizeof value);}
template<class T>T get(uptr address){T out{};if(address==fail_get)return out;
    for(unsigned i=0;i<used;++i){const auto p=reinterpret_cast<uptr>(nodes[i].bytes.data());
        if(nodes[i].live&&address>=p&&address-p<=256-sizeof(T)){std::memcpy(&out,reinterpret_cast<void const*>(address),sizeof out);return out;}}
    return out;
}
namespace component_names {bool enabled=false;std::size_t query_limit(wchar_t const* label){return std::wcslen(label)+1;}}
namespace playground {
bool name(uptr object,wchar_t* out,std::size_t limit){++name_reads;
    for(unsigned i=0;i<used;++i)if(object==reinterpret_cast<uptr>(nodes[i].bytes.data())&&nodes[i].live){
        if(nodes[i].label.size()>=limit)return false;std::wmemcpy(out,nodes[i].label.c_str(),nodes[i].label.size()+1);return true;}
    return false;
}
bool named(uptr object,wchar_t const* label){wchar_t text[128]{};return name(object,text,128)&&!_wcsicmp(text,label);}
#include "query_local_fields.inl"
uptr property(uptr owner,wchar_t const* label){if(!registered(owner))return 0;
    const wchar_t* labels[]={label};std::array<uptr,1> out{};return fields(get<uptr>(owner+0x50),labels,out)?out[0]:0;}
}
bool identify(void*,std::uint64_t address,abi::ObjectIdentity& out){
    if(!registered(address))return false;out={};out.address=address;out.klass=get<uptr>(address+0x50);out.known=1;return true;
}
namespace idle_inventory {
#include "weapon_equipment_inventory_fields.inl"
}
using Field=idle_inventory::Field;
uptr owner{},derived{},parent{},inventory{},slot{},object_type{},byte_type{},bad_type{},prefix{};
uptr prepend(uptr cls,wchar_t const* label,uptr kind,unsigned offset,unsigned width){
    const auto p=make(label);put(p+0x50,kind);put(p+0x70,1u);put(p+0x74,width);put(p+0x94,offset);
    put(p+0x68,get<uptr>(cls+0x80));put(cls+0x80,p);return p;
}
void reset(){used=0;name_reads=0;fail_get=0;component_names::enabled=false;
    object_type=make(L"ObjectProperty");byte_type=make(L"ByteProperty");bad_type=make(L"BoolProperty");
    parent=make(L"InventoryClass");derived=make(L"RifleClass");owner=make(L"ActualRifle");
    put(owner+0x50,derived);put(derived+0x60,parent);put(derived+0x88,256u);put(parent+0x88,256u);
    inventory=prepend(parent,L"Inventory",object_type,0xb0,8);slot=prepend(derived,L"CharacterSlot",byte_type,0xb8,1);
    for(unsigned i=0;i<40;++i)prefix=prepend(derived,L"Unrelated",object_type,0xc0,8);
}
bool pair(Field& a,Field& b){return idle_inventory::weapon_fields(owner,a,b);}
void rejects(){Field a{},b{};a.offset=171;b.offset=172;const auto before_a=a,before_b=b;
    require(!pair(a,b),"invalid pair rejected");require(!std::memcmp(&a,&before_a,sizeof a)&&!std::memcmp(&b,&before_b,sizeof b),"pair rejection preserves both outputs");}
void retire(uptr address){for(unsigned i=0;i<used;++i)if(address==reinterpret_cast<uptr>(nodes[i].bytes.data()))nodes[i].live=false;}
}
int main(){
    reset();Field old_a{},old_b{},a{},b{};
    require(idle_inventory::typed(owner,L"Inventory",L"ObjectProperty",8,old_a)&&idle_inventory::typed(owner,L"CharacterSlot",L"ByteProperty",1,old_b),"separate original selectors");
    const auto before=name_reads;name_reads=0;require(pair(a,b),"batched selectors");const auto after=name_reads;
    require(a.identity.address==old_a.identity.address&&a.offset==old_a.offset&&a.width==old_a.width&&b.identity.address==old_b.identity.address&&b.offset==old_b.offset&&b.width==old_b.width,"same actual selected descriptors and layouts");
    require(after<before,"shared prefix visited once");std::printf("Inventory pair actual selector name reads: %u -> %u; no live timing claim\n",before,after);
    for(bool accelerated:{false,true}){reset();component_names::enabled=accelerated;require(pair(a,b),"both existing name modes");}
    for(unsigned which=0;which<2;++which)for(unsigned mutation=0;mutation<7;++mutation){reset();const auto p=which?slot:inventory;
        if(mutation==0)put(p+0x50,bad_type);
        if(mutation==1)put(p+0x70,2u);
        if(mutation==2)put(p+0x74,which?8u:1u);
        if(mutation==3)put(p+0x94,256u);
        if(mutation==4)retire(p);
        if(mutation==5)fail_get=p+0x74;
        if(mutation==6)retire(get<uptr>(p+0x50));
        rejects();}
    reset();retire(owner);rejects();reset();retire(derived);rejects();reset();put(derived+0x88,0u);rejects();
    for(unsigned which=0;which<2;++which){reset();prepend(derived,which?L"CharacterSlot":L"Inventory",bad_type,0xb0,which?1u:8u);rejects();}
    reset();require(pair(a,b),"initial binding");const auto old_inventory=a.identity.address;
    const auto replacement=make(L"Inventory");put(replacement+0x50,object_type);put(replacement+0x70,1u);put(replacement+0x74,8u);put(replacement+0x94,0xb0u);
    // Only an INTERIOR next link changes; old descriptor and pointer remain.
    put(replacement+0x68,get<uptr>(prefix+0x68));put(prefix+0x68,replacement);
    require(pair(a,b)&&a.identity.address==replacement&&a.identity.address!=old_inventory,"fresh lookup detects earlier interior selector");
    reset();a.offset=99;require(!idle_inventory::weapon_fields(owner,a,a)&&a.offset==99,"aliased outputs rejected unchanged");
    std::printf("WeaponEquipmentInventoryFieldsTest: %u checks passed; owned descriptors only\n",checks);return 0;
}
