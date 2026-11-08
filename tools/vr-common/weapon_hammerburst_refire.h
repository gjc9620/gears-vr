#pragma once
#include "weapon_lancer_refire_loop.h"

// Exact retail HB WeaponFiring.RefireCheckTimer contract. Read-only matching;
// dispatch, input ownership and the single deferred-event owner stay common.
namespace gears_hammerburst_refire {
using Name=gears_lancer_refire_loop::Name;
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
