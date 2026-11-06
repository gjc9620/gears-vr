#pragma once
#include "cover_pose_phase.h"
#include <cmath>
#include <cstring>
// FACB0 noncached per-bone blend only. With a positive base
// child, bounded nonnegative masks and a supported mode, every desired atom obtains
// an initial complete 32-byte store (FB1F7 or FB7ED). Later blends may update
// it again. Mode one converts the scratch quaternion at FB57D, then rejoins
// the same complete first store at FB1D2; it does not skip that store.
// Neither changed values nor a newer epoch alone are a witness.
namespace gears_per_bone_overwrite {
namespace pose=gears_cover_pose_phase;
struct Child {std::uintptr_t node{};std::uint32_t weight{};};
struct Layer {pose::ArrayHeader weights{};std::array<std::uint32_t,115> bits{};};
struct Source {
    std::uintptr_t node{},mesh{},table{},method{},children{};
    std::int32_t count{};std::uint32_t epoch{},mesh_epoch{},epsilon{};unsigned char mode{};
    pose::ArrayHeader descriptors{},local{},desired{};
    std::array<unsigned char,115> indices{};
    std::array<Child,16> child{};std::array<Layer,15> layer{};
};
inline float value(std::uint32_t bits)noexcept {float v{};std::memcpy(&v,&bits,4);return v;}
inline bool eligible(Source const& s)noexcept {
    if(!s.mesh_epoch||s.mesh_epoch==UINT32_MAX||s.epoch==s.mesh_epoch||s.mode>1||s.count<2||s.count>16||s.epsilon!=0x3727c5ac)return false;
    const auto epsilon=value(s.epsilon);bool upper{};
    for(unsigned i=0;i<unsigned(s.count);++i){const auto weight=value(s.child[i].weight);
        if(!std::isfinite(weight)||weight<0||weight>1)return false;
        if(!i&&weight<=epsilon)return false;if(i&&weight>epsilon)upper=true;
    }
    if(!upper)return false; // FACB0 delegates directly when only base is active.
    for(unsigned i=0;i<unsigned(s.count-1);++i)for(auto bits:s.layer[i].bits){const auto weight=value(bits);
        if(!std::isfinite(weight)||weight<0||weight>1)return false;}
    return true;
}
// FACB0 FAD98 chooses the direct child-zero path only when no upper child
// exceeds epsilon. This is a forwarding classification, never store coverage.
inline bool base_only(Source const& s)noexcept {
    if(!s.mesh_epoch||s.epoch==s.mesh_epoch||s.count<2||s.count>16||s.epsilon!=0x3727c5ac)return false;
    const auto epsilon=value(s.epsilon);
    for(unsigned i=0;i<unsigned(s.count);++i){const auto weight=value(s.child[i].weight);
        if(!std::isfinite(weight)||weight<0||weight>1||(!i&&weight<=epsilon)||(i&&weight>epsilon))return false;}
    return s.child[0].node!=0;
}
template<class Read>bool snapshot(Read&& read,std::uintptr_t base,std::uintptr_t node,std::uintptr_t mesh,Source& s){
    s={};if(!base||base>UINTPTR_MAX-0x17070dc||!pose::span(node,0x124)||!pose::span(mesh,0x33c))return false;
    auto get=[&](auto& v,std::uintptr_t at){return pose::span(at,sizeof v)&&read(&v,reinterpret_cast<void const*>(at),sizeof v);};
    s.node=node;s.mesh=mesh;std::uintptr_t bound{};
    if(!get(s.table,node)||!pose::span(s.table,0x278)||!get(s.method,s.table+0x270)||s.method!=base+0xfacb0||
       !get(bound,node+0x60)||bound!=mesh||!get(s.epoch,node+0xbc)||!get(s.mesh_epoch,mesh+0x338)||
       !get(s.mode,node+0x110)||!get(s.children,node+0xe4)||!get(s.count,node+0xec)||s.count<2||s.count>16||
       !pose::span(s.children,std::size_t(s.count)*68)||!get(s.descriptors,node+0x100)||
       s.descriptors.count!=s.count-1||s.descriptors.capacity<s.descriptors.count||s.descriptors.capacity>128||
       !pose::span(s.descriptors.data,std::size_t(s.descriptors.capacity)*112)||
       !get(s.local,mesh+0x27c)||!get(s.desired,mesh+0x28c)||s.local.count!=115||s.desired.count!=115||
       !pose::header_valid(s.local,32)||!pose::header_valid(s.desired,1)||!get(s.indices,s.desired.data)||
       !get(s.epsilon,base+0x17070d8))return false;
    auto overlap=[](std::uintptr_t a,std::size_t an,std::uintptr_t b,std::size_t bn){return a<=b+bn-1&&b<=a+an-1;};
    auto aliases=[&](std::uintptr_t at,std::size_t n){return overlap(s.local.data,115*32,at,n);};
    if(aliases(node,0x124)||aliases(s.children,std::size_t(s.count)*68)||aliases(s.descriptors.data,std::size_t(s.descriptors.capacity)*112)||aliases(s.desired.data,115))return false;
    std::array<bool,115> seen{};for(auto index:s.indices){if(index>=115||seen[index])return false;seen[index]=true;}
    for(unsigned i=0;i<unsigned(s.count);++i){auto& c=s.child[i];const auto at=s.children+i*68;
        if(!get(c.node,at+8)||!get(c.weight,at+16))return false;}
    for(unsigned i=0;i<unsigned(s.count-1);++i){auto& l=s.layer[i];
        if(!get(l.weights,s.descriptors.data+i*112+0x2c)||l.weights.count!=115||!pose::header_valid(l.weights,4)||
           aliases(l.weights.data,std::size_t(l.weights.capacity)*4)||!get(l.bits,l.weights.data))return false;}
    return true;
}
inline bool same(Source const& a,Source const& b,bool epoch=true)noexcept {
    if(a.count<2||a.count>16||a.node!=b.node||a.mesh!=b.mesh||a.table!=b.table||a.method!=b.method||a.children!=b.children||
       a.count!=b.count||(epoch&&a.epoch!=b.epoch)||a.mesh_epoch!=b.mesh_epoch||a.epsilon!=b.epsilon||a.mode!=b.mode||
       !pose::header_equal(a.descriptors,b.descriptors)||!pose::header_equal(a.local,b.local)||!pose::header_equal(a.desired,b.desired)||a.indices!=b.indices)return false;
    for(unsigned i=0;i<unsigned(a.count);++i)if(a.child[i].node!=b.child[i].node||a.child[i].weight!=b.child[i].weight)return false;
    for(unsigned i=0;i<unsigned(a.count-1);++i)if(!pose::header_equal(a.layer[i].weights,b.layer[i].weights)||a.layer[i].bits!=b.layer[i].bits)return false;
    return true;
}
inline bool returned(Source const& before,Source const& after)noexcept {
    return eligible(before)&&same(before,after,false)&&after.epoch==before.mesh_epoch;
}
}
