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
    std::puts("first-person weapon: numbered instances, current rifle/sniper hierarchy, cycles, stale slots and weapon-change race passed");
}
