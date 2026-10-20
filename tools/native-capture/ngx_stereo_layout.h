#pragma once
#include <array>
#include <cstdint>

// CPU policy only. All resource, producer and GPU facts require live attestation.
namespace gears_ngx_stereo {
struct Extent { uint32_t width{},height{}; };
struct Point { uint32_t x{},y{}; };
struct Rect { uint32_t x{},y{},width{},height{}; };
inline bool same(Extent a,Extent b){return a.width==b.width&&a.height==b.height;}
inline bool same(Point a,Point b){return a.x==b.x&&a.y==b.y;}
inline bool bounded(Extent e){return e.width>=64&&e.height>=64&&e.width<=8192&&e.height<=8192;}
inline bool contains(Extent e,Rect r){return r.width&&r.height&&r.x<=e.width&&r.y<=e.height&&r.width<=e.width-r.x&&r.height<=e.height-r.y;}
inline bool overlap(Rect a,Rect b){return uint64_t(a.x)<uint64_t(b.x)+b.width&&uint64_t(b.x)<uint64_t(a.x)+a.width&&uint64_t(a.y)<uint64_t(b.y)+b.height&&uint64_t(b.y)<uint64_t(a.y)+a.height;}
struct Identity {
    uint64_t epoch{},world{},device{},parameters{},layout{};
};
inline bool same(Identity a,Identity b){return a.epoch==b.epoch&&a.world==b.world&&a.device==b.device&&a.parameters==b.parameters&&a.layout==b.layout;}
inline bool valid(Identity a){return a.epoch&&a.world&&a.device&&a.parameters&&a.layout;}
struct Layout {
    // Physical Color, Depth, MV, Output allocations, independently supplied.
    std::array<Extent,4> physical{};
    Extent full_output{},input{},eye_output{};
    std::array<Point,2> input_base{};
    uint32_t stride{};
};
// Native Create has dimensions/quality/flags, but no resource pointers. Its
// logical geometry is valid independently of later typed physical bindings.
inline bool valid_create_geometry(Layout const& l){
    if(!bounded(l.full_output)||(l.full_output.width%2||l.full_output.height%2)||!bounded(l.input)||!bounded(l.eye_output))return false;
    if(l.stride!=l.full_output.width/2||!same(l.eye_output,{l.stride,l.full_output.height}))return false;
    if(l.input.width>l.stride||l.input.height>l.eye_output.height)return false;
    if(!same(l.input_base[0],{0,0})||!same(l.input_base[1],{l.stride,0}))return false;
    return true;
}
inline bool valid(Layout const& l){
    if(!valid_create_geometry(l))return false;
    for(auto e:l.physical)if(!bounded(e))return false;
    if(!contains(l.physical[3],{0,0,l.eye_output.width,l.eye_output.height}))return false;
    for(unsigned eye=0;eye<2;++eye){
        auto p=l.input_base[eye];
        for(unsigned i=0;i<3;++i)if(!contains(l.physical[i],{p.x,p.y,l.input.width,l.input.height}))return false;
        if(!contains(l.physical[0],{eye*l.stride,0,l.eye_output.width,l.eye_output.height}))return false;
    }
    return true;
}
inline bool valid_create_layout(Layout const& l){
    if(!valid_create_geometry(l))return false;
    bool empty=true;for(auto e:l.physical)empty=empty&&!e.width&&!e.height;
    return empty||valid(l); // Partial/guessed allocation metadata is rejected.
}
inline bool same(Layout const& a,Layout const& b){
    if(!same(a.full_output,b.full_output)||!same(a.input,b.input)||!same(a.eye_output,b.eye_output)||a.stride!=b.stride)return false;
    for(unsigned i=0;i<4;++i)if(!same(a.physical[i],b.physical[i]))return false;
    return same(a.input_base[0],b.input_base[0])&&same(a.input_base[1],b.input_base[1]);
}
struct Resource {
    uint64_t identity{},device{};Extent extent{};
    uint32_t format{},mip{},samples{},subresource{};
};
struct CopyFacts {
    Identity identity{};uint64_t ticket{},caller{},list{},generation{},list_device{};
    // State legality may be inherited from a byte-attested original Copy route
    // with unchanged resources/subresources/list/order; not GPU completion.
    bool direct{},copy_source{},copy_dest{};
    std::array<Resource,4> resources{};
    std::array<Point,2> input_base{};
    Extent feature_output{};Point output_base{};
    unsigned evaluated_ordinal{};
};
struct CopyPlan {
    std::array<uint32_t,6> source_box{};
    std::array<uint32_t,3> destination{};
    uint64_t source{},target{},list{},generation{},ticket{};unsigned ordinal{};
};
inline bool plan_copy(Layout const& l,Identity id,CopyFacts const& f,unsigned eye,CopyPlan& result){
    if(!valid(l)||!valid(id)||!same(id,f.identity)||eye>1||f.evaluated_ordinal!=eye||!f.ticket||f.caller!=0x1c6093||!f.list||!f.generation||f.list_device!=id.device||!f.direct||!f.copy_source||!f.copy_dest)return false;
    if(!same(f.feature_output,l.eye_output)||!same(f.output_base,{0,0}))return false;
    for(unsigned e=0;e<2;++e)if(!same(f.input_base[e],l.input_base[e]))return false;
    for(unsigned i=0;i<4;++i){auto const& r=f.resources[i];
        if(!r.identity||r.device!=id.device||!same(r.extent,l.physical[i])||((i==0||i==3)&&r.format!=26)||r.mip||r.samples!=1||r.subresource)return false;
        for(unsigned j=0;j<i;++j)if(r.identity==f.resources[j].identity)return false;
    }
    Rect dst{eye*l.stride,0,l.eye_output.width,l.eye_output.height};
    if(!contains(f.resources[3].extent,{0,0,l.eye_output.width,l.eye_output.height})||!contains(f.resources[0].extent,dst))return false;
    for(unsigned later=eye+1;later<2;++later){auto p=f.input_base[later];if(overlap(dst,{p.x,p.y,l.input.width,l.input.height}))return false;}
    result={{0,0,0,l.eye_output.width,l.eye_output.height,1},{dst.x,0,0},f.resources[3].identity,f.resources[0].identity,f.list,f.generation,f.ticket,eye};return true;
}
// complete() attests admitted CPU plans only, not executed copies or GPU work.
// CPU plans are admitted in ordinal order on one ticket; lists may differ.
// Resources must match. GPU execution ordering needs external queue receipts.
class CopySequence {
    Layout layout_{};Identity identity_{};CopyFacts first_{};unsigned next_{};bool failed_{};
public:
    CopySequence(Layout l,Identity id):layout_(l),identity_(id){}
    bool next(CopyFacts const& f,unsigned eye,CopyPlan& out){
        if(failed_||eye!=next_||next_>=2){failed_=true;return false;}
        if(next_){
            if(f.ticket!=first_.ticket){failed_=true;return false;}
            for(unsigned i=0;i<4;++i)if(f.resources[i].identity!=first_.resources[i].identity){failed_=true;return false;}
        }
        if(!plan_copy(layout_,identity_,f,eye,out)){failed_=true;return false;}
        if(!next_)first_=f;++next_;return true;
    }
    bool complete()const{return !failed_&&next_==2;}
    bool failed()const{return failed_;}
};
}
