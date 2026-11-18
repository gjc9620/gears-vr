#pragma once
#include "root_blend_overwrite.h"
// F7940 full-weight mirrored child -> F4FEC. With a complete mirror table,
// F4FEC's final loop performs a complete 32-byte store at F5646 for each
// desired atom. The table-missing forwarding branch is explicitly excluded.
// Only observes the original call; never changes a mirror flag or bone.
namespace gears_mirrored_pose_overwrite {
namespace root=gears_root_blend_overwrite;
namespace pose=gears_cover_pose_phase;
struct Source {
    std::uintptr_t asset{};pose::ArrayHeader reference{},mirror{};
    std::array<unsigned char,115*68> reference_bytes{};
    std::array<unsigned char,115*8> mirror_bytes{};
    std::array<unsigned char,2> axes{};
};
struct Observation {unsigned stage{},bad_index{115};std::uintptr_t asset{};pose::ArrayHeader reference{},mirror{};unsigned axes{};};
inline unsigned child_index(root::Source const& s,bool after_return=false) noexcept {
    if(!s.mesh_epoch||s.mesh_epoch==UINT32_MAX||(after_return?s.root_epoch!=s.mesh_epoch:s.root_epoch>=s.mesh_epoch)||
       s.child_count<1||s.child_count>16||s.full!=0x3f7fff58||s.epsilon!=0x3727c5ac)return 16;
    float full{};std::memcpy(&full,&s.full,4);
    for(unsigned i=0;i<unsigned(s.child_count);++i){float w{};std::memcpy(&w,&s.child[i].weight,4);
        if(!std::isfinite(w))return 16;
        if(w>=full)return s.child[i].node&&(s.child[i].mirror&1)?i:16;
    }return 16;
}
template<class Read>bool snapshot(Read&& read,root::Source const& r,Source& s,bool after_return=false,Observation* observation=nullptr) {
    Observation detail{};auto note=[&](unsigned stage){detail.stage=stage;detail.asset=s.asset;detail.reference=s.reference;detail.mirror=s.mirror;detail.axes=unsigned(s.axes[0])|(unsigned(s.axes[1])<<8);if(observation)*observation=detail;};
    note(1);
    s={};if(child_index(r,after_return)==16||r.local.count!=115||r.desired.count!=115)return false;
    auto get=[&](auto& v,std::uintptr_t p){return pose::span(p,sizeof v)&&read(&v,reinterpret_cast<void const*>(p),sizeof v);};
    note(2);if(!get(s.asset,r.mesh+0x218)||!pose::span(s.asset,0x106))return false;
    note(3);if(!get(s.reference,s.asset+0xa4)||!get(s.mirror,s.asset+0xf4)||!get(s.axes,s.asset+0x104))return false;
    note(4);if(
       s.reference.count!=115||s.mirror.count!=115||!pose::header_valid(s.reference,68)||!pose::header_valid(s.mirror,8)||
       !get(s.reference_bytes,s.reference.data)||!get(s.mirror_bytes,s.mirror.data))return false;
    auto overlaps=[](std::uintptr_t a,std::size_t n,std::uintptr_t b,std::size_t m){return a<=b+m-1&&b<=a+n-1;};
    note(5);if(overlaps(r.local.data,115*32,s.asset,0x106)||overlaps(r.local.data,115*32,s.reference.data,115*68)||
       overlaps(r.local.data,115*32,s.mirror.data,115*8))return false;
    // Every intermediate reference and swapped mirror index stays in the
    // authenticated complete cohort. F5AB8 compares axis bytes with 1/2/4;
    // all other byte values use its explicit no-op default and still reach
    // F4FEC's final store loop. Axis bytes are preserved and compared, not
    // array indices or an authority condition.
    for(unsigned i=0;i<115;++i){std::int32_t parent{},other{};
        std::memcpy(&parent,s.reference_bytes.data()+i*68+56,4);
        std::memcpy(&other,s.mirror_bytes.data()+i*8,4);
        detail.bad_index=i;note(7);if((i&& (parent<0||parent>=int(i)))||(!i&&parent!=-1&&parent!=0)||
           other<0||other>=115)return false;
    }
    note(0);return true;
}
inline bool same(Source const& a,Source const& b) noexcept {
    return a.asset==b.asset&&pose::header_equal(a.reference,b.reference)&&pose::header_equal(a.mirror,b.mirror)&&
        a.axes==b.axes&&a.reference_bytes==b.reference_bytes&&a.mirror_bytes==b.mirror_bytes;
}
inline bool returned(root::Source const& before,root::Source const& after,Source const& a,Source const& b) noexcept {
    return child_index(before)<16&&root::same(before,after,false)&&after.root_epoch==before.mesh_epoch&&same(a,b);
}
}
