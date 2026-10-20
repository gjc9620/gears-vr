#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>

namespace gears_hook_entry {
struct Proof {
    std::uintptr_t target{},detour{},relay{};
    std::array<unsigned char,32> original32{},patched32{};
    std::array<unsigned char,14> relay14{};
};
inline bool range(std::uintptr_t address,std::size_t size){
    return address&&size&&size-1<=std::numeric_limits<std::uintptr_t>::max()-address;
}
inline bool relay_target(std::uintptr_t target,std::array<unsigned char,32> const& patch,std::uintptr_t& relay){
    if(!range(target,32)||patch[0]!=0xe9||target>UINTPTR_MAX-5)return false;
    std::int32_t displacement{};std::memcpy(&displacement,patch.data()+1,4);
    const auto after=target+5;
    if(displacement<0){const auto magnitude=std::uint64_t(-std::int64_t(displacement));
        if(magnitude>=after)return false;relay=after-std::uintptr_t(magnitude);
    }else{if(std::uint64_t(displacement)>UINTPTR_MAX-after)return false;relay=after+std::uintptr_t(displacement);}
    return range(relay,14);
}
inline bool structure(Proof const& p){
    static_assert(sizeof(std::uintptr_t)==8,"x64 hook entry proofs only");
    std::uintptr_t relay{};
    if(!p.detour||!relay_target(p.target,p.patched32,relay)||relay!=p.relay||
       p.relay14[0]!=0xff||p.relay14[1]!=0x25)return false;
    for(unsigned i=2;i<6;++i)if(p.relay14[i])return false;
    std::uint64_t destination{};std::memcpy(&destination,p.relay14.data()+6,8);
    return destination==p.detour&&std::memcmp(p.original32.data()+5,p.patched32.data()+5,27)==0;
}
// Read has bool(void* destination, const void* source, size_t bytes) semantics.
// Repeated reads bracket both regions; this detects changes, not malicious ABA.
template<class Read> bool verify(Proof const& p,Read read){
    if(!structure(p))return false;
    std::array<unsigned char,32> first{},last{};
    std::array<unsigned char,14> relay_first{},relay_last{};
    return read(first.data(),reinterpret_cast<void const*>(p.target),first.size())&&first==p.patched32&&
        read(relay_first.data(),reinterpret_cast<void const*>(p.relay),relay_first.size())&&relay_first==p.relay14&&
        read(relay_last.data(),reinterpret_cast<void const*>(p.relay),relay_last.size())&&relay_last==p.relay14&&
        read(last.data(),reinterpret_cast<void const*>(p.target),last.size())&&last==p.patched32;
}
// Caller authenticates original bytes and target/detour identities beforehand.
// Output is single-assignment: neither failure nor reseal changes a sealed proof.
template<class Read> bool seal(std::uintptr_t target,std::uintptr_t detour,
    std::array<unsigned char,32> const& original,Read read,Proof& output){
    if(output.target||output.detour||output.relay||output.original32!=std::array<unsigned char,32>{}||
       output.patched32!=std::array<unsigned char,32>{}||output.relay14!=std::array<unsigned char,14>{})return false;
    Proof candidate{};candidate.target=target;candidate.detour=detour;candidate.original32=original;
    if(!range(target,32)||!detour||!read(candidate.patched32.data(),reinterpret_cast<void const*>(target),32)||
       !relay_target(target,candidate.patched32,candidate.relay)||
       !read(candidate.relay14.data(),reinterpret_cast<void const*>(candidate.relay),14)||!verify(candidate,read))return false;
    output=candidate;return true;
}
// Never overlays arbitrary process reads. Each request must lie wholly within
// this exact 32-byte entry, and current patch evidence is reverified every time.
template<class Read> bool read_original(Proof const& p,std::uintptr_t address,void* output,std::size_t size,Read read){
    if(!output||!range(address,size)||address<p.target||address-p.target>=32||size>32-(address-p.target)||!verify(p,read))return false;
    std::memcpy(output,p.original32.data()+std::size_t(address-p.target),size);return true;
}
}
