#pragma once
#include "weapon_lancer_muzzle_observation.h"
#include <cmath>
#include <cstring>
#include <limits>

// Actual retail nine-bone gun arrays, observation only. The caller authenticates
// current component/asset identities and typed matrices around both captures.
// This reads no getters and certifies no bore/aim or final rendering stage.
namespace gears_weapon_lancer_muzzle {
using Matrix=std::array<float,16>;
struct Array {Address data{};std::int32_t count{},capacity{};};
static_assert(sizeof(Array)==16);
struct Pose {
    Address mesh{},asset{},master{};
    std::uint32_t epoch{};
    Array space{},local{},desired{},reference{};
    Matrix component{};
    std::array<Matrix,9> space_values{};
    std::array<unsigned char,9*32> local_values{};
    std::array<unsigned char,9*68> reference_values{};
};
inline bool valid(Array const& a,unsigned stride,bool required=true) noexcept {
    return a.count>=0&&a.capacity>=a.count&&a.capacity<=512&&
        (!required||(a.count==9&&a.data))&&(!a.count||a.data)&&
        a.data<=std::numeric_limits<Address>::max()-Address(a.count)*stride;
}
inline bool affine(Matrix const& m) noexcept {
    for(auto x:m)if(!std::isfinite(x))return false;
    if(std::abs(m[3])>1e-5f||std::abs(m[7])>1e-5f||std::abs(m[11])>1e-5f||std::abs(m[15]-1)>1e-5f)return false;
    const double d=double(m[0])*(double(m[5])*m[10]-double(m[6])*m[9])-
        double(m[1])*(double(m[4])*m[10]-double(m[6])*m[8])+double(m[2])*(double(m[4])*m[9]-double(m[5])*m[8]);
    return std::isfinite(d)&&std::abs(d)>1e-10;
}
template<class Read>bool capture(Read&& read,Address mesh,Address asset,Matrix const& component,Pose& out) noexcept {
    if(!mesh||!asset||mesh>std::numeric_limits<Address>::max()-0x33c||
       asset>std::numeric_limits<Address>::max()-0xb4||!affine(component))return false;
    Pose a{};a.mesh=mesh;a.asset=asset;a.component=component;
    std::array<unsigned char,56> header{},again{};Address selected{};
    if(!read(mesh+0x338,&a.epoch,4)||!read(mesh+0x26c,header.data(),header.size())||
       !read(mesh+0x218,&selected,8)||selected!=asset||!read(asset+0xa4,&a.reference,sizeof a.reference))return false;
    std::memcpy(&a.space,header.data(),16);std::memcpy(&a.local,header.data()+16,16);
    std::memcpy(&a.desired,header.data()+32,16);std::memcpy(&a.master,header.data()+48,8);
    if(a.master||!valid(a.space,64)||!valid(a.local,32)||!valid(a.desired,1,false)||!valid(a.reference,68))return false;
    if(!read(a.space.data,a.space_values.data(),sizeof a.space_values)||
       !read(a.local.data,a.local_values.data(),a.local_values.size())||
       !read(a.reference.data,a.reference_values.data(),a.reference_values.size()))return false;
    for(auto const& m:a.space_values)if(!affine(m))return false;
    std::uint32_t epoch{};Array reference{};Matrix ctw{};
    if(!read(mesh+0x26c,again.data(),again.size())||again!=header||!read(mesh+0x338,&epoch,4)||epoch!=a.epoch||
       !read(mesh+0x218,&selected,8)||selected!=asset||!read(asset+0xa4,&reference,sizeof reference)||
       std::memcmp(&reference,&a.reference,sizeof reference)||!read(mesh+0xa0,ctw.data(),sizeof ctw)||ctw!=component)return false;
    out=a;return true;
}
inline bool same(Pose const& a,Pose const& b) noexcept {
    const auto array=[](Array const& x,Array const& y){return x.data==y.data&&x.count==y.count&&x.capacity==y.capacity;};
    return a.mesh==b.mesh&&a.asset==b.asset&&a.master==b.master&&a.epoch==b.epoch&&
        array(a.space,b.space)&&array(a.local,b.local)&&array(a.desired,b.desired)&&array(a.reference,b.reference)&&
        a.component==b.component&&a.space_values==b.space_values&&a.local_values==b.local_values&&a.reference_values==b.reference_values;
}
inline bool location(Pose const& pose,Socket const& socket,unsigned& bone,std::array<double,3>& out) noexcept {
    if(!socket.object||socket.configured_name.index!=socket.socket_name.index||
       socket.configured_name.number!=socket.socket_name.number||!affine(pose.component))return false;
    for(auto x:socket.location)if(!std::isfinite(x))return false;
    unsigned found=9;
    for(unsigned i=0;i<9;++i){Name name{};std::memcpy(&name,pose.reference_values.data()+i*68,8);
        if(name.index==socket.bone_name.index&&name.number==socket.bone_name.number){if(found!=9)return false;found=i;}}
    if(found==9||!affine(pose.space_values[found]))return false;
    // UE3 row-vector convention: socket relative point -> bone Space -> gun CTW.
    std::array<double,4> local{socket.location[0],socket.location[1],socket.location[2],1},model{},world{};
    for(unsigned j=0;j<4;++j)for(unsigned k=0;k<4;++k)model[j]+=local[k]*pose.space_values[found][k*4+j];
    for(unsigned j=0;j<4;++j)for(unsigned k=0;k<4;++k)world[j]+=model[k]*pose.component[k*4+j];
    for(auto x:world)if(!std::isfinite(x))return false;
    if(std::abs(world[3]-1)>1e-4)return false;
    bone=found;out={world[0],world[1],world[2]};return true;
}
} // namespace gears_weapon_lancer_muzzle
