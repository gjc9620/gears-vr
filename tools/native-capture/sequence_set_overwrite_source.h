#pragma once
#include "cover_pose_phase.h"
#include <cmath>
#include <cstring>
// Walking node2FDE20's noncached first-full-weight branch evaluates E08A0
// into the canonical output, then returns WITHOUT advancing its own epoch.
// The partial branch initializes every desired32-byte atom from its first
// positive scratch pose, then blends and calls F6D8C. Cached copies are excluded.
namespace gears_sequence_set_overwrite {
namespace pose=gears_cover_pose_phase;
struct Entry {std::array<unsigned char,32> raw{};};
struct Source {
    std::uintptr_t node{},mesh{},table{},method{};
    uint32_t epoch{},mesh_epoch{},time_bits{},epsilon{},full{};int32_t node70{};bool blended{};
    pose::ArrayHeader local{},desired{},cache{},sequences{};
    std::array<unsigned char,115> indices{};
    std::array<Entry,16> entries{};int selected{-1};
};
inline float value(uint32_t bits) noexcept {float f{};std::memcpy(&f,&bits,4);return f;}
inline std::uintptr_t sequence(Entry const& e) noexcept {std::uintptr_t p{};std::memcpy(&p,e.raw.data()+16,8);return p;}
inline int32_t link(Entry const& e) noexcept {int32_t p{};std::memcpy(&p,e.raw.data()+24,4);return p;}
inline uint32_t weight(Entry const& e) noexcept {uint32_t p{};std::memcpy(&p,e.raw.data()+28,4);return p;}
inline bool eligible(Source const& s) noexcept {
    if(!s.mesh_epoch||s.mesh_epoch==UINT32_MAX||s.epoch>=s.mesh_epoch||
       s.epsilon!=0x3727c5ac||s.full!=0x3f7fff58||!std::isfinite(value(s.time_bits))||
       s.node70<0||s.node70>128||s.sequences.count<1||s.sequences.count>16||s.selected<0||s.selected>=s.sequences.count)return false;
    int first=-1;
    for(int i=0;i<s.sequences.count;++i){const auto w=value(weight(s.entries[unsigned(i)]));
        if(!std::isfinite(w)||w<0||w>1)return false;
        if(w>value(s.epsilon)){if(link(s.entries[unsigned(i)])< -1)return false;if(first<0)first=i;}
        if(w>=value(s.full))return !s.blended&&i==s.selected;
    }return s.blended&&first>=0&&first==s.selected;
}
template<class Read>bool snapshot(Read&& read,std::uintptr_t base,std::uintptr_t node,std::uintptr_t mesh,Source& s){
    s={};s.selected=-1;
    if(!base||!pose::span(node,0x140)||!pose::span(mesh,0x33c))return false;
    auto get=[&](auto& v,std::uintptr_t at){return pose::span(at,sizeof v)&&read(&v,reinterpret_cast<void const*>(at),sizeof v);};
    s.node=node;s.mesh=mesh;std::uintptr_t bound{};
    if(!get(s.table,node)||!pose::span(s.table,0x278)||!get(s.method,s.table+0x270)||s.method!=base+0x2fde20||
       !get(bound,node+0x60)||bound!=mesh||!get(s.node70,node+0x70)||!get(s.epoch,node+0xbc)||!get(s.mesh_epoch,mesh+0x338)||
       !get(s.local,mesh+0x27c)||!get(s.desired,mesh+0x28c)||!get(s.cache,node+0x80)||
       !get(s.time_bits,node+0xf4)||!get(s.sequences,node+0x130)||
       !get(s.epsilon,base+0x17070d8)||!get(s.full,base+0x17070d4)||
       s.local.count!=115||s.desired.count!=115||!pose::header_valid(s.local,32)||!pose::header_valid(s.desired,1)||
       s.sequences.count<1||s.sequences.count>16||s.sequences.capacity<s.sequences.count||s.sequences.capacity>128||
       !pose::span(s.sequences.data,std::size_t(s.sequences.capacity)*32)||!get(s.indices,s.desired.data))return false;
    auto overlaps=[](std::uintptr_t a,std::size_t an,std::uintptr_t b,std::size_t bn){
        return an&&bn&&pose::span(a,an)&&pose::span(b,bn)&&a<=b+bn-1&&b<=a+an-1;};
    if(overlaps(s.local.data,115*32,node,0x140)||overlaps(s.local.data,115*32,s.desired.data,115)||
       overlaps(s.local.data,115*32,s.sequences.data,std::size_t(s.sequences.capacity)*32))return false;
    std::array<bool,115> seen{};for(auto i:s.indices){if(i>=115||seen[i])return false;seen[i]=true;}
    int first=-1;
    for(int i=0;i<s.sequences.count;++i){auto& e=s.entries[unsigned(i)];
        if(!get(e,s.sequences.data+unsigned(i)*32))return false;
        if(first<0&&value(weight(e))>value(s.epsilon))first=i;
        if(s.selected<0&&value(weight(e))>=value(s.full))s.selected=i;
    }if(s.selected<0&&first>=0){s.selected=first;s.blended=true;}return true;
}
inline bool same(Source const& a,Source const& b,bool returned=false) noexcept {
    if(a.node!=b.node||a.mesh!=b.mesh||a.table!=b.table||a.method!=b.method||(!returned&&a.epoch!=b.epoch)||
       a.mesh_epoch!=b.mesh_epoch||a.time_bits!=b.time_bits||a.epsilon!=b.epsilon||a.full!=b.full||
       a.selected!=b.selected||a.blended!=b.blended||a.node70!=b.node70||a.indices!=b.indices||!pose::header_equal(a.local,b.local)||
       !pose::header_equal(a.desired,b.desired)||(!returned&&!pose::header_equal(a.cache,b.cache))||
       !pose::header_equal(a.sequences,b.sequences)||a.sequences.count<1||a.sequences.count>16)return false;
    for(int i=0;i<a.sequences.count;++i)if(a.entries[unsigned(i)].raw!=b.entries[unsigned(i)].raw)return false;
    return true;
}
inline bool returned(Source const& before,Source const& after) noexcept {
    return eligible(before)&&(before.blended?
        same(before,after,true)&&after.epoch==before.mesh_epoch&&after.cache.count==(before.node70>=2?115:0):same(before,after));
}
}
