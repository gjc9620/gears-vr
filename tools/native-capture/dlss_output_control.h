#pragma once
#include "dlss_route_observer.h"
#include "vr_dof_compat.h"
#include "final_transfer_policy.h"
#include "frame_link.h"
namespace gears_dlss_output {
using Key=gears_vr_dof::Key;using Failure=gears_vr_dof::Failure;using Decision=gears_vr_dof::Decision;
using View=gears_dlss_route::View;using Descriptor=gears_dlss_route::Descriptor;
using Args=std::array<float,8>;using Dimensions=std::array<unsigned,4>;
inline bool noop(Descriptor const& d,unsigned index){return !index&&d==Descriptor{0xffff,0xffff,0};}
inline bool valid_view(View const& v){return v.mask==31&&v.native.read_mask==7&&v.eye_valid&&v.native.eye_valid&&
    v.same_current&&v.eye==v.native.eye&&v.marker==1&&v.eye<2;}
inline bool same(View const& a,View const& b){return valid_view(a)&&valid_view(b)&&a.address==b.address&&a.family==b.family&&
    a.context==b.context&&a.native==b.native&&a.eye==b.eye&&a.marker==b.marker&&
    gears_death_constants::equal(a.source,b.source)&&a.target==b.target&&gears_death_constants::equal(a.sampling,b.sampling);}
inline bool geometry(View const& v,Dimensions const& d,Args& forwarded){
    if(!valid_view(v)||!d[0]||d[0]%2||!d[1]||!d[2]||d[2]%2||!d[3])return false;
    for(auto size:d)if(size>8192)return false;
    const unsigned x=v.eye*(d[0]/2),u=v.eye*(d[2]/2);
    if(v.source!=gears_dlss_route::Float4{float(x),0,float(d[0]/2),float(d[1])}||
       v.target!=std::array<int32_t,4>{int32_t(u),0,int32_t(d[2]/2),int32_t(d[3])})return false;
    forwarded={v.source[0],v.source[1],v.source[2],v.source[3],float(v.target[0]),float(v.target[1]),float(v.target[2]),float(v.target[3])};return true;
}
struct Eye {
    uintptr_t renderer{},view{},context{},pass{},descriptor{};Args forwarded{};Dimensions dimensions{};
    gears_final_transfer::EyeDrawEvidence draw{};bool output_complete{};
};
struct Proof {std::array<Eye,2> eyes{};};
using Store=gears_vr_dof::Store<Proof>;
inline bool complete(Store::Entry const& e){
    if(!e.complete||e.failure!=Failure::none||e.setter_mask!=3||e.quad_mask!=3)return false;
    auto const& a=e.payload.eyes[0];auto const& b=e.payload.eyes[1];
    if(!a.renderer||a.renderer!=b.renderer||!a.context||a.context!=b.context||!a.view||a.view>UINTPTR_MAX-0xd30||b.view!=a.view+0xd30||
       a.dimensions!=b.dimensions||a.draw.source!=b.draw.source||a.draw.target!=b.draw.target)return false;
    for(unsigned i=0;i<2;++i){auto const& eye=e.payload.eyes[i];auto const& d=eye.draw;
        if(!eye.output_complete||!d.candidate||d.world!=e.key.world||d.ticket!=e.key.id||d.eye!=i||!d.list||!d.generation||!d.source||!d.target||
           d.args!=eye.forwarded||d.dimensions!=eye.dimensions)return false;}
    return true;
}
}
