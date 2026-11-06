#pragma once
#include "cover_pose_phase.h"
#include <cstring>

// Data cohort for the actual F7940 full-weight, non-mirrored F8AF4 child.
// The caller must attest both implementations, E08A0 + decoder and F7858,
// retain the real GT/world scope, and observe a normal original return.
// F8AF4's noncached branch calls E08A0: each desired index obtains either
// the full reference atom, or the certified decoder's full atom. Subsequent
// root processing does not cancel that write. This is overwrite coverage,
// not evidence of a clean source pose or permission for a new pose write.
namespace gears_sequence_pose_overwrite {
namespace pose=gears_cover_pose_phase;
struct Source {
    std::uintptr_t node{},mesh{},table{},method{},sequence{};
    std::int32_t node70{},link_index{};std::uint32_t node_epoch{},mesh_epoch{},time_bits{};
    pose::ArrayHeader local{},desired{},cache{};
};
inline bool array(pose::ArrayHeader const& h,unsigned stride)noexcept {
    return h.count>=0&&h.capacity>=h.count&&h.capacity<=512&&
        (!h.capacity||pose::span(h.data,std::size_t(h.capacity)*stride));
}
inline bool aliases(std::uintptr_t a,std::size_t an,std::uintptr_t b,std::size_t bn)noexcept {
    return an&&bn&&pose::span(a,an)&&pose::span(b,bn)&&a<=b+bn-1&&b<=a+an-1;
}
template<class Read>bool snapshot(Read&& read,std::uintptr_t base,std::uintptr_t node,std::uintptr_t mesh,
    Source& s,bool before=true){
    s={};if(!base||!pose::span(node,0x110)||!pose::span(mesh,0x33c))return false;
    auto get=[&](auto& value,std::uintptr_t at){return pose::span(at,sizeof value)&&read(&value,reinterpret_cast<void const*>(at),sizeof value);};
    s.node=node;s.mesh=mesh;std::uintptr_t bound{};
    if(!get(s.table,node)||!pose::span(s.table,0x278)||!get(s.method,s.table+0x270)||s.method!=base+0xf8af4||
       !get(bound,node+0x60)||bound!=mesh||!get(s.node70,node+0x70)||s.node70<0||s.node70>128||
       !get(s.node_epoch,node+0xbc)||!get(s.mesh_epoch,mesh+0x338)||!s.mesh_epoch||s.mesh_epoch==UINT32_MAX||
       !get(s.local,mesh+0x27c)||!get(s.desired,mesh+0x28c)||!get(s.cache,node+0x80)||
       s.local.count!=115||s.desired.count!=115||!array(s.local,32)||!array(s.desired,1)||!array(s.cache,32)||
       !get(s.sequence,node+0xfc)||!get(s.link_index,node+0x104)||s.link_index< -1||
       !get(s.time_bits,node+0xf4))return false;
    if(before&&s.node_epoch>=s.mesh_epoch)return false; // Only the actual noncached call.
    if(aliases(s.local.data,115*32,node,0x110)||aliases(s.local.data,115*32,s.desired.data,115)||
       aliases(s.local.data,115*32,s.cache.data,std::size_t(s.cache.capacity)*32))return false;
    return true;
}
inline bool same(Source const& a,Source const& b,bool returned=false)noexcept {
    return a.node==b.node&&a.mesh==b.mesh&&a.table==b.table&&a.method==b.method&&a.sequence==b.sequence&&
        a.node70==b.node70&&a.link_index==b.link_index&&a.time_bits==b.time_bits&&a.mesh_epoch==b.mesh_epoch&&
        pose::header_equal(a.local,b.local)&&pose::header_equal(a.desired,b.desired)&&
        (returned||(a.node_epoch==b.node_epoch&&pose::header_equal(a.cache,b.cache)));
}
inline bool returned(Source const& before,Source const& after)noexcept {
    return before.node_epoch<before.mesh_epoch&&same(before,after,true)&&after.node_epoch==before.mesh_epoch&&after.cache.count==(before.node70>=2?115:0);
}
}
