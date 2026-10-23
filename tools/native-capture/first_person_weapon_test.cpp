#include "first_person_weapon.h"
#include <map>
#include <vector>
#include <cassert>
#include <cstdio>
struct Memory {
    std::map<std::uintptr_t,std::vector<unsigned char>> blocks;
    void reserve(std::uintptr_t a,std::size_t n){blocks[a].resize(n);}
    template<class T>void put(std::uintptr_t a,T const& value){
        auto i=blocks.upper_bound(a);assert(i!=blocks.begin());--i;
        assert(a-i->first+sizeof(value)<=i->second.size());std::memcpy(i->second.data()+a-i->first,&value,sizeof(value));}
    bool read(void* out,void const* source,std::size_t n){
        auto a=reinterpret_cast<std::uintptr_t>(source);auto i=blocks.upper_bound(a);
        if(i==blocks.begin())return false;--i;
        if(a-i->first>i->second.size()||n>i->second.size()-(a-i->first))return false;
        std::memcpy(out,i->second.data()+a-i->first,n);return true;}
};
int main(){
    Memory m;gears_anchor::Config c{};
    c.objects_header=0x1000;c.names_header=0x1100;c.object_internal_index=8;c.object_name=72;c.object_class=80;c.field_super=96;c.name_entry_text=20;
    m.reserve(0x1000,0x200);m.reserve(0x2000,0x100);m.reserve(0x3000,0x100);
    m.put(0x1000,gears_anchor::Registry{0x2000,8,16});m.put(0x1100,gears_anchor::Registry{0x3000,8,16});
    const wchar_t* names[]={L"Class",L"WarWeapon",L"Weap_SniperRifle",L"SniperVariant",L"AssaultRifle",L"PawnClass",L"Pawn",L"Weapon"};
    for(unsigned i=0;i<8;++i){
        const std::uintptr_t a=0x10000+i*0x400,n=0x20000+i*0x200;
        m.reserve(a,0x300);m.reserve(n,0x200);m.put(0x2000+i*8,a);m.put(0x3000+i*8,n);
        m.put(a+8,i);m.put(a+72,i);m.put(a+80,std::uintptr_t(0x10000));m.put(n,i);
        for(unsigned j=0;names[i][j];++j)m.put(n+20+j*sizeof(wchar_t),names[i][j]);
    }
    constexpr std::uintptr_t base=0x10400,sniper=0x10800,variant=0x10c00,rifle=0x11000,pawn=0x11800,weapon=0x11c00;
    m.put(sniper+96,base);m.put(variant+96,sniper);m.put(rifle+96,base);
    m.put(pawn+0x200,weapon);m.put(weapon+80,rifle);m.put(weapon+76,unsigned(4)); // Instance number must be accepted.
    bool result=true;auto read=[&](void* o,void const* a,std::size_t n){return m.read(o,a,n);};
    assert(gears_first_person::weapon_type(c,read,pawn,0x200,result)&&!result);
    m.put(weapon+80,variant);assert(gears_first_person::weapon_type(c,read,pawn,0x200,result)&&result);
    m.put(weapon+80,rifle);m.put(rifle+96,rifle);assert(!gears_first_person::weapon_type(c,read,pawn,0x200,result));
    m.put(rifle+96,base);m.put(0x2000+4*8,std::uintptr_t(0));assert(!gears_first_person::weapon_type(c,read,pawn,0x200,result));
    m.put(0x2000+4*8,rifle);
    unsigned pawn_reads{};auto race=[&](void* o,void const* a,std::size_t n){
        if(reinterpret_cast<std::uintptr_t>(a)==pawn+0x200&&++pawn_reads==2){m.put(pawn+0x200,std::uintptr_t(0));}
        return m.read(o,a,n);};
    assert(!gears_first_person::weapon_type(c,race,pawn,0x200,result));
    // A sniper class alone must not disable the preview. Resolve its live
    // reflected zoom bit, including inherited sniper variants and packed flags.
    c.object_outer=64;c.module_base=0x10000000;c.cinematic_enabled=true;
    m.put(pawn+0x200,weapon);m.put(weapon+80,variant);
    m.put(0x1000,gears_anchor::Registry{0x2000,11,16});m.put(0x1100,gears_anchor::Registry{0x3000,11,16});
    const wchar_t* extra[]{L"BoolProperty",L"bZoomed",L"UnusedField"};
    for(unsigned j=0;j<3;++j){const unsigned i=j+8;const std::uintptr_t a=0x10000+i*0x400,n=0x20000+i*0x200;
        m.reserve(a,0x300);m.reserve(n,0x200);m.put(0x2000+i*8,a);m.put(0x3000+i*8,n);
        m.put(a+8,i);m.put(a+72,i);m.put(a+80,std::uintptr_t(0x10000));m.put(n,i);
        for(unsigned k=0;extra[j][k];++k)m.put(n+20+k*sizeof(wchar_t),extra[j][k]);
    }
    constexpr std::uintptr_t bool_class=0x12000,zoom=0x12400,other=0x12800,table=0x40000;
    m.put(sniper+0x80,other);m.put(other+0x68,zoom);
    m.put(zoom+80,bool_class);m.put(zoom+64,sniper);m.put(zoom,table);
    m.put(zoom+0x70,unsigned(1));m.put(zoom+0x74,unsigned(4));m.put(zoom+0x94,unsigned(0x240));m.put(zoom+0xd0,unsigned(4));
    m.reserve(table,0x300);m.put(table+0x2a0,c.module_base+gears_anchor::cinematic_bool_accessor_rva);
    const auto dispatch=c.module_base+gears_anchor::cinematic_gnatives_rva+0x2d*8;
    m.reserve(dispatch,8);m.put(dispatch,c.module_base+gears_anchor::cinematic_bool_native_rva);
    gears_first_person::WeaponState state{};gears_first_person::ZoomHint hint{};
    auto inspect=[&](){return gears_first_person::weapon_state(c,read,pawn,0x200,state,hint);};
    m.put(weapon+0x240,unsigned(0x80));assert(inspect()&&state.sniper&&!state.zoomed&&hint.owner==sniper&&hint.property==zoom);
    unsigned traversals=0;auto cached=[&](void* o,void const* a,std::size_t n){
        if(reinterpret_cast<std::uintptr_t>(a)==sniper+0x80)++traversals;
        return m.read(o,a,n);};
    m.put(weapon+0x240,unsigned(0x84));
    assert(gears_first_person::weapon_state(c,cached,pawn,0x200,state,hint)&&state.sniper&&state.zoomed&&traversals==0);
    m.put(weapon+0x240,unsigned(0x80));assert(inspect()&&!state.zoomed);
    auto rejected=[&](){const auto old=state;assert(!inspect());assert(state.sniper==old.sniper&&state.zoomed==old.zoomed&&!hint.property);};
    m.put(zoom+64,rifle);rejected();m.put(zoom+64,sniper);assert(inspect());
    m.put(zoom+0xd0,unsigned(3));rejected();m.put(zoom+0xd0,unsigned(4));assert(inspect());
    m.put(zoom+0x94,unsigned(65533));rejected();m.put(zoom+0x94,unsigned(0x240));assert(inspect());
    m.put(zoom+0x74,unsigned(1));rejected();m.put(zoom+0x74,unsigned(4));assert(inspect());
    m.put(bool_class+76,unsigned(1));rejected();m.put(bool_class+76,unsigned(0));assert(inspect());
    m.put(table+0x2a0,c.module_base+1);rejected();m.put(table+0x2a0,c.module_base+gears_anchor::cinematic_bool_accessor_rva);assert(inspect());
    m.put(dispatch,c.module_base+1);rejected();m.put(dispatch,c.module_base+gears_anchor::cinematic_bool_native_rva);assert(inspect());
    m.put(0x2000+9*8,std::uintptr_t(0));rejected();m.put(0x2000+9*8,zoom);assert(inspect());
    hint={};m.put(other+0x68,other);rejected();m.put(other+0x68,zoom);assert(inspect());
    unsigned flag_reads{};auto changing=[&](void* o,void const* a,std::size_t n){
        if(reinterpret_cast<std::uintptr_t>(a)==weapon+0x240&&++flag_reads==2)m.put(weapon+0x240,unsigned(4));
        return m.read(o,a,n);};
    const auto old=state;assert(!gears_first_person::weapon_state(c,changing,pawn,0x200,state,hint));
    assert(state.sniper==old.sniper&&state.zoomed==old.zoomed&&!hint.property);
    assert(inspect()&&state.zoomed);
    // Changes to unrelated packed Bool bits do not invalidate the zoom sample.
    flag_reads=0;auto unrelated=[&](void* o,void const* a,std::size_t n){
        if(reinterpret_cast<std::uintptr_t>(a)==weapon+0x240&&++flag_reads==2)m.put(weapon+0x240,unsigned(0x84));
        return m.read(o,a,n);};
    assert(gears_first_person::weapon_state(c,unrelated,pawn,0x200,state,hint)&&state.zoomed);
    unsigned property_reads=0;auto metadata_race=[&](void* o,void const* a,std::size_t n){
        if(reinterpret_cast<std::uintptr_t>(a)==zoom+0xd0&&++property_reads==3)m.put(zoom+0xd0,unsigned(8));
        return m.read(o,a,n);};
    assert(!gears_first_person::weapon_state(c,metadata_race,pawn,0x200,state,hint));m.put(zoom+0xd0,unsigned(4));
    pawn_reads=0;auto late_switch=[&](void* o,void const* a,std::size_t n){
        if(reinterpret_cast<std::uintptr_t>(a)==pawn+0x200&&++pawn_reads==3)m.put(pawn+0x200,std::uintptr_t(0));
        return m.read(o,a,n);};
    assert(!gears_first_person::weapon_state(c,late_switch,pawn,0x200,state,hint));m.put(pawn+0x200,weapon);
    m.put(weapon+80,rifle);assert(inspect()&&!state.sniper&&!state.zoomed&&!hint.property);
    std::puts("first-person weapon: class/instance guards, live sniper zoom, checked hint, property/dispatch/slot/flag races and weapon switching passed");
}
