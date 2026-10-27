#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

// Existing retail Ref115/Space/Local layout, observation only. The caller
// authenticates live mesh/asset identities and typed component metadata.
// No cached admission, engine call, writes, world-ray or render-phase claim.
namespace gears_weapon_component_pose {
using Address=std::uint64_t;
using Matrix=std::array<float,16>;
struct Array {Address data{};std::int32_t count{},capacity{};};
static_assert(sizeof(Array)==16);
struct Snapshot {
    Address mesh{},asset{},master{};
    std::uint32_t epoch{};
    Array space{},local{},desired{},reference{};
    Matrix component{};
    std::array<Matrix,115> space_values{};
    std::array<unsigned char,115*32> local_values{};
    std::array<unsigned char,115*68> reference_values{};
};
inline bool valid(Array const& a,unsigned stride,bool required=true) noexcept {
    return a.count>=0&&a.capacity>=a.count&&a.capacity<=512&&
        (!required||(a.count==115&&a.data))&&(!a.count||a.data)&&
        a.data<=std::numeric_limits<Address>::max()-std::uint64_t(a.count)*stride;
}
template<class Read> bool capture(Read&& read,Address mesh,Address asset,Matrix const& component,Snapshot& out) noexcept {
    if(!mesh||!asset||mesh>std::numeric_limits<Address>::max()-0x33c||
       asset>std::numeric_limits<Address>::max()-0xb4)return false;
    Snapshot a{};a.mesh=mesh;a.asset=asset;a.component=component;
    Address selected_asset{};std::array<unsigned char,56> header{},again{};
    if(!read(mesh+0x338,&a.epoch,4)||!read(mesh+0x26c,header.data(),header.size())||
       !read(mesh+0x218,&selected_asset,8)||selected_asset!=asset||
       !read(asset+0xa4,&a.reference,sizeof a.reference))return false;
    std::memcpy(&a.space,header.data(),16);std::memcpy(&a.local,header.data()+16,16);
    std::memcpy(&a.desired,header.data()+32,16);std::memcpy(&a.master,header.data()+48,8);
    if(a.master||!valid(a.space,64)||!valid(a.local,32)||!valid(a.desired,1,false)||!valid(a.reference,68))return false;
    if(!read(a.space.data,a.space_values.data(),sizeof a.space_values)||
       !read(a.local.data,a.local_values.data(),a.local_values.size())||
       !read(a.reference.data,a.reference_values.data(),a.reference_values.size()))return false;
    for(auto const& m:a.space_values)for(float v:m)if(!std::isfinite(v))return false;
    for(float v:component)if(!std::isfinite(v))return false;
    // Close only headers here. The caller repeats the complete snapshot after
    // gun reads and compares it, so arrays bracket the gun observation too.
    std::uint32_t epoch{};Array reference{};Matrix ctw{};
    if(!read(mesh+0x26c,again.data(),again.size())||again!=header||
       !read(mesh+0x338,&epoch,4)||epoch!=a.epoch||
       !read(mesh+0x218,&selected_asset,8)||selected_asset!=asset||
       !read(asset+0xa4,&reference,sizeof reference)||std::memcmp(&reference,&a.reference,sizeof reference)||
       !read(mesh+0xa0,ctw.data(),sizeof ctw)||ctw!=component)return false;
    out=a;return true;
}
inline bool same(Snapshot const& a,Snapshot const& b) noexcept {
    const auto equal=[](Array const& x,Array const& y){return x.data==y.data&&x.count==y.count&&x.capacity==y.capacity;};
    return a.mesh==b.mesh&&a.asset==b.asset&&a.master==b.master&&a.epoch==b.epoch&&
        equal(a.space,b.space)&&equal(a.local,b.local)&&equal(a.desired,b.desired)&&equal(a.reference,b.reference)&&
        a.component==b.component&&a.space_values==b.space_values&&a.local_values==b.local_values&&a.reference_values==b.reference_values;
}
inline Matrix world(Snapshot const& pose,unsigned bone) noexcept {
    Matrix result{};if(bone>=pose.space_values.size())return result;
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)
        for(unsigned k=0;k<4;++k)result[i*4+j]+=pose.space_values[bone][i*4+k]*pose.component[k*4+j];
    return result;
}
} // namespace gears_weapon_component_pose
