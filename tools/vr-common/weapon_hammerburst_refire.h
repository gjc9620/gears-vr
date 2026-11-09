#pragma once
#include "weapon_lancer_refire_loop.h"
#include <initializer_list>

// Exact retail HB WeaponFiring.RefireCheckTimer contract. Read-only matching;
// dispatch, input ownership and the single deferred-event owner stay common.
namespace gears_hammerburst_refire {
using Name=gears_lancer_refire_loop::Name;
struct ShouldBindings {
    std::uint64_t instigator{},human_function{},super_should{},mode{},count{},limit{},returned{};
    Name has_ammo{};
};
// Full original local-human finite-burst decision, export37982/package91c0f56e.
// Seven serialized object references relocate32->64; the name is resolved
// independently by the runtime adapter. This is not a general VM evaluator.
inline bool should_refire(unsigned char const* code,std::size_t bytes,ShouldBindings const& b) noexcept {
    if(!code||!b.instigator||!b.human_function||!b.super_should||!b.mode||!b.count||
       !b.limit||b.count==b.limit||!b.returned||!b.has_ammo.index||b.has_ammo.number)return false;
    std::array<unsigned char,111> expected{};std::size_t n{};
    const auto raw=[&](std::initializer_list<unsigned char> values){for(auto x:values)expected[n++]=x;};
    const auto pointer=[&](std::uint64_t p){std::memcpy(expected.data()+n,&p,8);n+=8;};
    raw({0x07,0x2a,0,0x81,0x19,0x01});pointer(b.instigator);
    raw({0x0a,0,0x04,0x1c});pointer(b.human_function);
    raw({0x16,0x16,0x04,0x1c});pointer(b.super_should);
    raw({0x16,0x06,0x64,0,0x07,0x45,0,0x81,0x1b});
    std::memcpy(expected.data()+n,&b.has_ammo,sizeof b.has_ammo);n+=sizeof b.has_ammo;
    raw({0x01});pointer(b.mode);
    raw({0x50,0x16,0x16,0x04,0x28,0x07,0x62,0,0x96,0x38,0x3a,0x01});pointer(b.count);
    raw({0x38,0x3a,0x01});pointer(b.limit);
    raw({0x16,0x04,0x27,0x04,0x28,0x04,0x3a});pointer(b.returned);raw({0x53});
    return n==expected.size()&&bytes==n&&!std::memcmp(code,expected.data(),n);
}
// Original byte BurstCount increment followed by the real WarWeapon super
// call (export36176). Replaying or writing a counter is never an alternative.
inline bool ammunition(unsigned char const* code,std::size_t bytes,
                       std::uint64_t count,std::uint64_t super_ammunition) noexcept {
    if(!code||!count||!super_ammunition||count==super_ammunition)return false;
    std::array<unsigned char,24> expected{{0x8b,0x01}};
    std::memcpy(expected.data()+2,&count,8);expected[10]=0x16;expected[11]=0x1c;
    std::memcpy(expected.data()+12,&super_ammunition,8);
    expected[20]=0x16;expected[21]=0x04;expected[22]=0x0b;expected[23]=0x53;
    return bytes==expected.size()&&!std::memcmp(code,expected.data(),bytes);
}
inline bool timer(unsigned char const* code,std::size_t bytes,
    std::uint64_t putting_down,std::uint64_t was_targeting,
    Name put_down,Name should_refire,Name ammunition,Name burst_delaying) noexcept {
    if(!code||!putting_down||!was_targeting||putting_down==was_targeting||
       !put_down.index||put_down.number||!should_refire.index||should_refire.number||
       !ammunition.index||ammunition.number||!burst_delaying.index||burst_delaying.number||
       put_down.index==should_refire.index||put_down.index==ammunition.index||
       should_refire.index==ammunition.index)return false;
    // Package91c0f56e, export37979: serialized71 bytes, runtime79 after
    // relocating the two reflected property operands from32 to64 bits.
    // Both branch targets are runtime offsets (0x19 and0x32).
    std::array<unsigned char,79> expected{};std::size_t n{};
    const auto byte=[&](unsigned char v){expected[n++]=v;};
    const auto pointer=[&](std::uint64_t v){std::memcpy(expected.data()+n,&v,8);n+=8;};
    const auto name=[&](Name v){std::memcpy(expected.data()+n,&v,sizeof v);n+=sizeof v;};
    byte(0x07);byte(0x19);byte(0);byte(0x2d);byte(0x01);pointer(putting_down);
    byte(0x1b);name(put_down);byte(0x16);byte(0x04);byte(0x0b);
    byte(0x07);byte(0x32);byte(0);byte(0x1b);name(should_refire);byte(0x16);
    byte(0x1b);name(ammunition);byte(0x16);byte(0x04);byte(0x0b);
    byte(0x14);byte(0x2d);byte(0x01);pointer(was_targeting);byte(0x28);
    byte(0x71);byte(0x21);name(burst_delaying);byte(0x50);byte(0x50);byte(0x50);
    byte(0x16);byte(0x04);byte(0x0b);byte(0x53);
    return n==expected.size()&&bytes==n&&!std::memcmp(code,expected.data(),n);
}
}
