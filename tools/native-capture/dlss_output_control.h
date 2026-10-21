#pragma once
#include "dlss_route_observer.h"
#include "vr_dof_compat.h"
#include "final_transfer_policy.h"
#include "frame_link.h"
#include "ngx_stereo_adapter.h"
namespace gears_dlss_output {
using Key=gears_vr_dof::Key;using Failure=gears_vr_dof::Failure;using Decision=gears_vr_dof::Decision;
using View=gears_dlss_route::View;using Descriptor=gears_dlss_route::Descriptor;
using Args=std::array<float,8>;using Dimensions=std::array<unsigned,4>;
inline uintptr_t bind_caller(bool owned_dof){return owned_dof?0xd7ff84:0xd831c8;}
inline bool setter_caller(bool owned_dof,uintptr_t caller){return owned_dof?(caller==0xd8084e||caller==0xd80925):caller==0xd83b69;}
inline bool quad_caller(bool owned_dof,uintptr_t setter,uintptr_t caller){return owned_dof?
    ((setter==0xd8084e&&caller==0xd808d8)||(setter==0xd80925&&caller==0xd80a7b)):caller==0xd83cbf;}
inline bool noop(Descriptor const& d,unsigned index){return !index&&d==Descriptor{0xffff,0xffff,0};}
inline bool valid_view(View const& v){return v.mask==31&&v.native.read_mask==7&&v.eye_valid&&v.native.eye_valid&&
    v.same_current&&v.eye==v.native.eye&&v.marker==1&&v.eye<2;}
inline bool same(View const& a,View const& b){return valid_view(a)&&valid_view(b)&&a.address==b.address&&a.family==b.family&&
    a.context==b.context&&a.native==b.native&&a.eye==b.eye&&a.marker==b.marker&&
    gears_death_constants::equal(a.source,b.source)&&a.target==b.target&&gears_death_constants::equal(a.sampling,b.sampling);}
// Current constructor regions after a previous SDK input, or for an explicitly
// unpublished current SDK bootstrap/candidate. This is collection-time identity;
// whole-ticket no-SDK classification and candidate discard happen after consume.
struct NativeGap {
    uint64_t world{},ticket{},prior_sdk_ticket{};uintptr_t renderer{};
    bool unpublished_sdk{}; // prior_sdk_ticket equals ticket only for this explicitly unpublished SDK candidate.
    gears_ngx_stereo::Identity epoch{};std::array<uintptr_t,2> views{};
    std::array<std::array<unsigned,4>,2> input{};std::array<unsigned,2> output{},texture{};
};
inline bool set_native_texture(NativeGap& n,gears_ngx_stereo::Resource const& color){
    n.texture={};
    if(!gears_ngx_stereo::valid(n.epoch)||!color.identity||color.device!=n.epoch.device||color.format!=26||color.mip||color.subresource||
       color.samples!=1||n.output!=std::array<unsigned,2>{color.extent.width,color.extent.height})return false;
    n.texture={color.extent.width,color.extent.height};return true;
}
inline bool same(NativeGap const& a,NativeGap const& b){return a.world==b.world&&a.ticket==b.ticket&&a.prior_sdk_ticket==b.prior_sdk_ticket&&a.unpublished_sdk==b.unpublished_sdk&&
    a.renderer==b.renderer&&gears_ngx_stereo::same(a.epoch,b.epoch)&&a.views==b.views&&a.input==b.input&&a.output==b.output&&a.texture==b.texture;}
inline bool native_geometry(View const& v,Dimensions const& d,NativeGap const& n,uint64_t world,uint64_t ticket,uintptr_t renderer,Args& out){
    if(!valid_view(v)||!world||!ticket||world!=n.world||ticket!=n.ticket||!n.prior_sdk_ticket||
       (n.unpublished_sdk?n.prior_sdk_ticket!=ticket:n.prior_sdk_ticket>=ticket)||
       !gears_ngx_stereo::valid(n.epoch)||n.epoch.world!=world||!renderer||n.renderer!=renderer||v.native.renderer!=renderer||
       !n.views[0]||n.views[0]>UINTPTR_MAX-0xd30||n.views[1]!=n.views[0]+0xd30||v.address!=n.views[v.eye]||
       d!=Dimensions{n.output[0],n.output[1],n.texture[0],n.texture[1]}||n.output!=n.texture||!d[0]||d[0]%2||!d[1])return false;
    for(auto size:d)if(size>8192)return false;
    for(unsigned eye=0;eye<2;++eye){auto const& r=n.input[eye];
        if(r[0]!=eye*(d[2]/2)||r[1]||r[2]<64||r[2]>d[2]/2||r[3]<64||r[3]>d[3]||
           r[2]!=n.input[0][2]||r[3]!=n.input[0][3])return false;}
    auto const& r=n.input[v.eye];
    for(unsigned i=0;i<4;++i)if(v.source[i]!=float(r[i])||v.target[i]!=int32_t(r[i]))return false;
    out={float(v.eye*(d[0]/2)),0,float(d[0]/2),float(d[1]),float(r[0]),float(r[1]),float(r[2]),float(r[3])};return true;
}
inline bool geometry(View const& v,Dimensions const& d,Args& forwarded){
    if(!valid_view(v)||!d[0]||d[0]%2||!d[1]||!d[2]||d[2]%2||!d[3])return false;
    for(auto size:d)if(size>8192)return false;
    const unsigned x=v.eye*(d[0]/2),u=v.eye*(d[2]/2);
    if(v.source!=gears_dlss_route::Float4{float(x),0,float(d[0]/2),float(d[1])}||
       v.target!=std::array<int32_t,4>{int32_t(u),0,int32_t(d[2]/2),int32_t(d[3])})return false;
    forwarded={v.source[0],v.source[1],v.source[2],v.source[3],float(v.target[0]),float(v.target[1]),float(v.target[2]),float(v.target[3])};return true;
}
inline bool sdk_geometry(View const& v,Dimensions const& d,gears_ngx_stereo::RecordedOutput const& output,
    uint64_t world,uint64_t ticket,uint64_t bound_color,Args& forwarded){
    using namespace gears_ngx_stereo;
    if(!world||!ticket||!gears_ngx_stereo::valid(output.identity)||!valid_view(v)||output.identity.world!=world||output.ticket!=ticket||output.layout.packed_input||
       d[2]!=output.layout.physical[0].width||d[3]!=output.layout.physical[0].height)return false;
    for(auto value:v.target)if(value<0)return false;
    for(unsigned i=0;i<4;++i)if(v.source[i]!=float(v.target[i]))return false;
    const Rect rendered{uint32_t(v.target[0]),uint32_t(v.target[1]),uint32_t(v.target[2]),uint32_t(v.target[3])};
    Rect source{},destination{};
    if(!output_geometry(output,output.identity,ticket,bound_color,v.eye,rendered,{d[0],d[1]},source,destination))return false;
    forwarded={float(destination.x),float(destination.y),float(destination.width),float(destination.height),
        float(source.x),float(source.y),float(source.width),float(source.height)};return true;
}
struct Eye {
    struct BindingEvidence {
        bool complete{};Descriptor descriptor{};uintptr_t address{},owner{},vtable{},shader_handle{},view{},context{};
        uintptr_t public_global{},public_pointer{},public_wrapper{},public_object{},list{},canonical_list{};
        uint64_t generation{};unsigned public_index{},selector{},slot{},highwater{};gears_dlss_route::InputResource resource{};
    } binding{};
    uintptr_t renderer{},view{},context{},pass{},descriptor{};Args forwarded{};Dimensions dimensions{};
    gears_final_transfer::EyeDrawEvidence draw{};bool output_complete{},tail_transition_validated{},owned_dof{},sdk_output{},unpublished_sdk{};
    gears_ngx_stereo::Identity owned_epoch{};
};
struct Proof {std::array<Eye,2> eyes{};};
using Store=gears_vr_dof::Store<Proof>;
inline bool complete(Store::Entry const& e){
    if(!e.complete||e.failure!=Failure::none||e.setter_mask!=3||e.quad_mask!=3)return false;
    auto const& a=e.payload.eyes[0];auto const& b=e.payload.eyes[1];
    if((a.owned_dof||b.owned_dof)&&(!a.owned_dof||!b.owned_dof||!gears_ngx_stereo::valid(a.owned_epoch)||
       a.owned_epoch.world!=e.key.world||!gears_ngx_stereo::same(a.owned_epoch,b.owned_epoch)))return false;
    if(!a.renderer||a.renderer!=b.renderer||!a.context||a.context!=b.context||!a.view||a.view>UINTPTR_MAX-0xd30||b.view!=a.view+0xd30||
       a.dimensions!=b.dimensions||a.draw.source!=b.draw.source||a.draw.target!=b.draw.target)return false;
    for(unsigned i=0;i<2;++i){auto const& eye=e.payload.eyes[i];auto const& d=eye.draw;
        auto const& bind=eye.binding;
        if(!eye.output_complete||!bind.complete||!d.candidate||d.world!=e.key.world||d.ticket!=e.key.id||d.eye!=i||!d.list||!d.generation||!d.source||!d.target||
           bind.context!=eye.context||bind.view!=eye.view||bind.canonical_list!=d.list||bind.generation!=d.generation||bind.resource.canonical!=d.source||bind.resource.format!=26||
           d.args!=eye.forwarded||d.dimensions!=eye.dimensions)return false;}
    auto const& ba=a.binding;auto const& bb=b.binding;
    if(ba.owner!=bb.owner||ba.vtable!=bb.vtable||ba.shader_handle!=bb.shader_handle||ba.descriptor!=bb.descriptor||ba.address!=bb.address||ba.public_global!=bb.public_global||ba.public_pointer!=bb.public_pointer||
       ba.public_wrapper!=bb.public_wrapper||ba.public_index!=bb.public_index||ba.public_object!=bb.public_object||ba.selector!=bb.selector||ba.slot!=bb.slot||
       ba.resource.object!=bb.resource.object||ba.resource.native!=bb.resource.native||ba.resource.canonical!=bb.resource.canonical)return false;
    return true;
}
}
