#pragma once
#include "unpossessed_quad_observer.h"
#include <cmath>
namespace gears_death_constants {
using Float4=std::array<float,4>;
using Descriptor=std::array<uint16_t,3>;
inline bool finite(Float4 const& v){for(auto f:v)if(!std::isfinite(f))return false;return true;}
inline bool equal(Float4 const& a,Float4 const& b){return std::memcmp(a.data(),b.data(),16)==0;}
inline unsigned role(uintptr_t caller){return caller==0xd80739?0u:caller==0xd80925?1u:2u;}
inline uintptr_t owner(uintptr_t descriptor,unsigned kind){const uintptr_t offset=kind==0?0xb2:kind==1?0xaa:0;
    return offset&&descriptor>=offset&&descriptor%2==0?descriptor-offset:0;}
inline bool eligible(bool enabled,bool recording,bool diagnostics,bool frozen,bool unpossessed,uint64_t ticket,uint64_t frozen_ticket,
                     unsigned ordinal,uintptr_t caller,unsigned index){
    return enabled&&recording&&diagnostics&&frozen&&unpossessed&&ticket&&ticket==frozen_ticket&&ordinal==49&&role(caller)<2&&index==0;
}
inline bool destination(Descriptor const& d,unsigned index,uintptr_t data,uintptr_t& address){
    address=0;if(index||d[0]>=16||d[2]<16||!data||data>UINTPTR_MAX-d[1]||data+d[1]>UINTPTR_MAX-15)return false;
    address=data+d[1];return true; // Exact uint16 start offset for R9=0; 16 bytes.
}
struct View {
    gears_unpossessed_quad::NativeView native{};uintptr_t context{};uint32_t thread{};
    Float4 source_rect{};std::array<int32_t,4> target_rect{};bool rectangles{};
    bool operator==(View const& b)const{return native==b.native&&context==b.context&&thread==b.thread&&rectangles==b.rectangles&&
        equal(source_rect,b.source_rect)&&target_rect==b.target_rect;}
};
struct Destination {
    uintptr_t root{},slot{},context{},table{},bank{},data{},address{};uint32_t highwater{};unsigned char active{};
    unsigned read_mask{};Float4 value{};bool readable{},finite_value{};
    bool same_path(Destination const& b)const{return root==b.root&&slot==b.slot&&context==b.context&&table==b.table&&bank==b.bank&&
        data==b.data&&address==b.address&&active==b.active&&read_mask==b.read_mask;}
};
struct Setter {
    bool present{};unsigned kind{},eye{},index{};uint64_t ticket{},world{},result{};uintptr_t caller{},rcx{},descriptor_address{},source_address{},owner{};
    Descriptor before_descriptor{},after_descriptor{};bool descriptor_before{},descriptor_after{};
    Float4 before_source{},after_source{};bool source_before{},source_after{};
    View before_view{},after_view{};Destination before_destination{},after_destination{};
    long long before_qpc{},after_qpc{};
};
struct Final {
    bool present{};unsigned eye{};uint64_t ticket{},world{};long long qpc{};View view{};
    std::array<float,8> args{};std::array<unsigned,4> dimensions{};
    bool owner_pair{},view_pair{};std::array<Float4,2> source{};std::array<bool,2> source_valid{};
    std::array<Descriptor,2> descriptors{};std::array<bool,2> descriptor_current{},descriptor_matches{};
    std::array<Destination,2> destination{};
};
struct Recorder {
    std::array<Setter,4> setters{};std::array<Final,2> finals{};uint64_t duplicate_setters{},duplicate_finals{},rejected{};
    Setter* reserve(unsigned eye,unsigned kind){if(eye>1||kind>1){++rejected;return nullptr;}
        auto& row=setters[eye*2+kind];if(row.present){++duplicate_setters;return nullptr;}
        row.present=true;row.eye=eye;row.kind=kind;return &row;}
};
}
