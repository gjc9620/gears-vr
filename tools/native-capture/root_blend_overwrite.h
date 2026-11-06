#pragma once
#include "cover_pose_phase.h"
#include <cmath>
#include <cstring>
// Actual attested F7940 implementation entry, including derived callers;
// the vtable method may be a derived wrapper. Exact F7940 blend branch only. Cached copies, single full-weight children,
// partial desired sets and unknown layouts are not overwrite witnesses.
namespace gears_root_blend_overwrite {
namespace pose=gears_cover_pose_phase;
struct Child {std::uintptr_t node{};std::uint32_t weight{};unsigned char mirror{};};
struct Source {
    std::uintptr_t root{},mesh{},table{},method{},children{};
    std::int32_t child_count{};std::uint32_t root_epoch{},mesh_epoch{},epsilon{},full{};
    pose::ArrayHeader local{},desired{};
    std::array<unsigned char,115> indices{};
    std::array<Child,16> child{};
};
template<class Read>bool snapshot(Read&& read,std::uintptr_t base,std::uintptr_t root,std::uintptr_t mesh,
    std::uintptr_t out,std::uintptr_t desired,Source& s,bool attested_implementation_entry=false) {
    s={};if(!base||base>UINTPTR_MAX-0x17070dc||!pose::span(root,0xf0)||!pose::span(mesh,0x33c)||out!=mesh+0x27c||desired!=mesh+0x28c)return false;
    auto get=[&](auto& value,std::uintptr_t at){return pose::span(at,sizeof value)&&read(&value,reinterpret_cast<void const*>(at),sizeof value);};
    s.root=root;s.mesh=mesh;std::uintptr_t bound{};
    if(!get(s.table,root)||!pose::span(s.table,0x278)||!get(s.method,s.table+0x270)||(!attested_implementation_entry&&s.method!=base+0xf7940)||!pose::span(s.method,32)||
       !get(bound,root+0x60)||bound!=mesh||!get(s.root_epoch,root+0xbc)||!get(s.mesh_epoch,mesh+0x338)||
       !get(s.children,root+0xe4)||!get(s.child_count,root+0xec)||s.child_count<0||(!attested_implementation_entry&&s.child_count==0)||s.child_count>16||
       (s.child_count&&!pose::span(s.children,std::size_t(s.child_count)*68))||!get(s.local,out)||!get(s.desired,desired)||
       s.local.count!=115||s.desired.count!=115||!pose::header_valid(s.local,32)||!pose::header_valid(s.desired,1)||
       !get(s.indices,s.desired.data)||!get(s.epsilon,base+0x17070d8)||!get(s.full,base+0x17070d4)||
       s.epsilon!=0x3727c5ac||s.full!=0x3f7fff58)return false;
    auto overlap=[](std::uintptr_t a,std::size_t an,std::uintptr_t b,std::size_t bn){return a<=b+bn-1&&b<=a+an-1;};
    if(overlap(s.local.data,115*32,s.desired.data,115)||overlap(s.local.data,115*32,root,0xf0)||
       (s.child_count&&overlap(s.local.data,115*32,s.children,std::size_t(s.child_count)*68)))return false;
    std::array<bool,115> seen{};for(auto index:s.indices){if(index>=115||seen[index])return false;seen[index]=true;}
    for(unsigned i=0;i<unsigned(s.child_count);++i){auto& c=s.child[i];const auto at=s.children+i*68;
        if(!get(c.node,at+8)||!get(c.weight,at+16)||!get(c.mirror,at+60))return false;}
    return true;
}
// Certified F7940 zero-child branch calls F7858 for every desired bone.
// The consumer must additionally attest the helper's exact bytes and the
// same immutable reference cohort. This is a store witness, not a pose grant.
inline bool reference_leaf(Source const& s)noexcept {
    return s.mesh_epoch&&s.mesh_epoch!=UINT32_MAX&&s.root_epoch!=s.mesh_epoch&&s.child_count==0;
}
inline bool blend(Source const& s)noexcept {
    if(!s.mesh_epoch||s.mesh_epoch==UINT32_MAX||s.root_epoch==s.mesh_epoch||s.child_count<1||s.child_count>16)return false;
    float epsilon{},full{};std::memcpy(&epsilon,&s.epsilon,4);std::memcpy(&full,&s.full,4);
    if(s.epsilon!=0x3727c5ac||s.full!=0x3f7fff58)return false;
    bool has_weight{};for(unsigned i=0;i<unsigned(s.child_count);++i){float weight{};std::memcpy(&weight,&s.child[i].weight,4);
        if(!std::isfinite(weight)||weight>=full)return false;if(weight>epsilon)has_weight=true;}
    return has_weight;
}
inline bool delegates(Source const& s,std::uintptr_t node_target)noexcept {
    if(!node_target||!s.mesh_epoch||s.root_epoch==s.mesh_epoch||s.child_count<1||s.child_count>16||s.full!=0x3f7fff58||s.epsilon!=0x3727c5ac)return false;
    float full{};std::memcpy(&full,&s.full,4);
    for(unsigned i=0;i<unsigned(s.child_count);++i){float weight{};auto const& c=s.child[i];std::memcpy(&weight,&c.weight,4);
        if(!std::isfinite(weight))return false;if(weight>=full)return c.node==node_target&&!(c.mirror&1);}
    return false;
}
inline std::uintptr_t full_weight_child(Source const& s)noexcept {
    if(!s.mesh_epoch||s.mesh_epoch==UINT32_MAX||s.root_epoch>=s.mesh_epoch||
       s.child_count<1||s.child_count>16||s.full!=0x3f7fff58||s.epsilon!=0x3727c5ac)return 0;
    float full{};std::memcpy(&full,&s.full,4);
    for(unsigned i=0;i<unsigned(s.child_count);++i){float weight{};auto const& c=s.child[i];std::memcpy(&weight,&c.weight,4);
        if(!std::isfinite(weight))return 0;
        if(weight>=full)return (c.mirror&1)?0:c.node;
    }
    return 0;
}
inline bool same(Source const& a,Source const& b,bool epoch=true)noexcept {
    if(a.child_count<0||a.child_count>16||a.root!=b.root||a.mesh!=b.mesh||a.table!=b.table||a.method!=b.method||a.children!=b.children||
       a.child_count!=b.child_count||(epoch&&a.root_epoch!=b.root_epoch)||a.mesh_epoch!=b.mesh_epoch||
       a.epsilon!=b.epsilon||a.full!=b.full||!pose::header_equal(a.local,b.local)||
       !pose::header_equal(a.desired,b.desired)||a.indices!=b.indices)return false;
    for(unsigned i=0;i<unsigned(a.child_count)&&i<a.child.size();++i)
        if(a.child[i].node!=b.child[i].node||a.child[i].weight!=b.child[i].weight||a.child[i].mirror!=b.child[i].mirror)return false;
    return true;
}
inline bool returned(Source const& before,Source const& after)noexcept {
    return blend(before)&&same(before,after,false)&&after.root_epoch==before.mesh_epoch;
}
}
