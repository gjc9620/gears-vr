#pragma once
#include "cover_pose_phase.h"
#include <cstring>
// Data continuity for the attested F7940/FACB0 cached-array branch. This is
// not an execution receipt. The private live observer must additionally prove
// exact original entry/normal return, code and current memcpy export binding.
namespace gears_cached_pose_overwrite {
namespace pose=gears_cover_pose_phase;
struct Header {
    uintptr_t node{},mesh{},table{},method{};unsigned kind{};
    uint32_t node_epoch{},mesh_epoch{};
    pose::ArrayHeader local{},desired{},cache{};
    std::array<unsigned char,115> indices{};
};
struct Source {Header header{};std::array<pose::Atom,115> cache{},local{};};
inline bool overlaps(uintptr_t a,size_t n,uintptr_t b,size_t m) noexcept {
    return pose::span(a,n)&&pose::span(b,m)&&a<=b+m-1&&b<=a+n-1;
}
template<class Read>bool header(Read&& read,uintptr_t base,uintptr_t node,uintptr_t mesh,
    uintptr_t out,uintptr_t desired,unsigned kind,Header& result){
    Header h{};h.node=node;h.mesh=mesh;h.kind=kind;
    if((kind!=0&&kind!=2)||!base||base>UINTPTR_MAX-0xfacb0||!pose::span(node,0xc0)||!pose::span(mesh,0x33c)||
       out!=mesh+0x27c||desired!=mesh+0x28c)return false;
    auto get=[&](auto& v,uintptr_t at){return pose::span(at,sizeof v)&&read(&v,reinterpret_cast<void const*>(at),sizeof v);};
    uintptr_t bound{};
    if(!get(h.table,node)||!pose::span(h.table,0x278)||!get(h.method,h.table+0x270)||
       h.method!=base+(kind==0?0xf7940:0xfacb0)||!get(bound,node+0x60)||bound!=mesh||
       !get(h.node_epoch,node+0xbc)||!get(h.mesh_epoch,mesh+0x338)||!h.mesh_epoch||h.mesh_epoch==UINT32_MAX||h.node_epoch!=h.mesh_epoch||
       !get(h.local,out)||!get(h.desired,desired)||!get(h.cache,node+0x80)||
       !pose::header_valid(h.local,32)||!pose::header_valid(h.desired,1)||!pose::header_valid(h.cache,32)||
       h.local.count!=115||h.local.capacity!=115||h.desired.count!=115||h.desired.capacity>4096||
       h.cache.count!=115||h.cache.capacity>4096||!get(h.indices,h.desired.data))return false;
    const uintptr_t meta[]={node,mesh,h.table,h.desired.data};const size_t sizes[]={0xc0,0x33c,0x278,size_t(h.desired.capacity)};
    for(unsigned i=0;i<4;++i)if(overlaps(h.local.data,115*32,meta[i],sizes[i])||
        overlaps(h.cache.data,size_t(h.cache.capacity)*32,meta[i],sizes[i]))return false;
    if(overlaps(h.local.data,115*32,h.cache.data,size_t(h.cache.capacity)*32))return false;
    std::array<bool,115> seen{};for(auto index:h.indices){if(index>=115||seen[index])return false;seen[index]=true;}
    result=h;return true;
}
inline bool same(Header const& a,Header const& b) noexcept {
    return a.node==b.node&&a.mesh==b.mesh&&a.table==b.table&&a.method==b.method&&a.kind==b.kind&&
        a.node_epoch==b.node_epoch&&a.mesh_epoch==b.mesh_epoch&&pose::header_equal(a.local,b.local)&&
        pose::header_equal(a.desired,b.desired)&&pose::header_equal(a.cache,b.cache)&&a.indices==b.indices;
}
template<class Read>bool snapshot(Read&& read,uintptr_t base,Header const& selected,Source& result){
    Source s{};
    if(!header(read,base,selected.node,selected.mesh,selected.mesh+0x27c,selected.mesh+0x28c,selected.kind,s.header)||
       !same(selected,s.header)||!read(s.cache.data(),reinterpret_cast<void const*>(s.header.cache.data),sizeof s.cache)||
       !read(s.local.data(),reinterpret_cast<void const*>(s.header.local.data),sizeof s.local))return false;
    Header closing{};if(!header(read,base,selected.node,selected.mesh,selected.mesh+0x27c,selected.mesh+0x28c,selected.kind,closing)||!same(s.header,closing))return false;
    result=s;return true;
}
inline bool same(Source const& a,Source const& b) noexcept {
    return same(a.header,b.header)&&!memcmp(a.cache.data(),b.cache.data(),sizeof a.cache)&&!memcmp(a.local.data(),b.local.data(),sizeof a.local);
}
inline bool returned(Source const& before,Source const& after) noexcept {
    return same(before.header,after.header)&&!memcmp(before.cache.data(),after.cache.data(),sizeof before.cache)&&
        !memcmp(before.cache.data(),after.local.data(),sizeof before.cache);
}
}
