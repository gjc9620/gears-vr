#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace gears_longshot_initial_fire {
struct Name {std::uint32_t index{},number{};};
// Exact expanded retail Weap_SniperRifle.WeaponFiring.BeginState body:
// super.BeginState(PreviousStateName); SetTimer(.2,false,PS5Feedback, omitted).
// This recognizes evidence; it never interprets or resumes a VM frame.
inline bool stock_wrapper(unsigned char const* code,std::size_t bytes,
    std::uint64_t parent,std::uint64_t previous,Name feedback) noexcept {
    if(!code||!parent||!previous||!feedback.index||feedback.number)return false;
    std::array<unsigned char,41> expected{};std::size_t n{};
    expected[n++]=0x1c;std::memcpy(expected.data()+n,&parent,8);n+=8;
    expected[n++]=0x00;std::memcpy(expected.data()+n,&previous,8);n+=8;
    expected[n++]=0x16;expected[n++]=0x61;expected[n++]=0x18;
    expected[n++]=0x1e;const float delay=.2f;
    std::memcpy(expected.data()+n,&delay,4);n+=4;
    expected[n++]=0x28;expected[n++]=0x21;
    std::memcpy(expected.data()+n,&feedback,8);n+=8;
    expected[n++]=0x50;expected[n++]=0x16;
    expected[n++]=0x04;expected[n++]=0x0b;expected[n++]=0x53;
    return n==expected.size()&&bytes==n&&!std::memcmp(code,expected.data(),n);
}
struct ReleaseBindings {
    std::uint64_t instigator{},human{},mode{},controller{},fire{},result{};Name end_fire{};
};
// Exact original ShouldRefire: optional original EndFire/bFire=0, always false.
// It is used only for the existing original-state handback after real R2-up.
inline bool stock_release(unsigned char const* code,std::size_t bytes,ReleaseBindings const& b) noexcept {
    if(!code||!b.instigator||!b.human||!b.mode||!b.controller||!b.fire||!b.result||
       !b.end_fire.index||b.end_fire.number)return false;
    std::array<unsigned char,96> expected{};std::size_t n{};
    const auto byte=[&](unsigned char v){expected[n++]=v;};
    const auto pointer=[&](std::uint64_t v){std::memcpy(expected.data()+n,&v,8);n+=8;};
    byte(0x07);byte(0x53);byte(0);byte(0x19);byte(0x01);pointer(b.instigator);
    byte(0x0a);byte(0);byte(0x04);byte(0x1c);pointer(b.human);byte(0x16);byte(0x1b);
    std::memcpy(expected.data()+n,&b.end_fire,8);n+=8;
    byte(0x01);pointer(b.mode);byte(0x16);byte(0x0f);byte(0x19);byte(0x19);byte(0x01);pointer(b.instigator);
    byte(0x09);byte(0);byte(0x08);byte(0x01);pointer(b.controller);
    byte(0x09);byte(0);byte(0x01);byte(0x01);pointer(b.fire);
    byte(0x24);byte(0);byte(0x04);byte(0x28);byte(0x04);byte(0x3a);pointer(b.result);byte(0x53);
    return n==expected.size()&&bytes==n&&!std::memcmp(code,expected.data(),n);
}
}
