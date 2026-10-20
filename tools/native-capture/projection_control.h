#pragma once
#include "native_source_view_args.h"
#include <cmath>

namespace gears_projection {
enum class Phase : unsigned {a1,b,a2,c,a3};
inline Phase phase(long long timestamp,long long origin,long long frequency) {
    auto elapsed=timestamp-origin;
    if(elapsed<5*frequency)return Phase::a1;
    if(elapsed<10*frequency)return Phase::b;
    if(elapsed<15*frequency)return Phase::a2;
    if(elapsed<20*frequency)return Phase::c;
    return Phase::a3;
}
struct Metadata {bool enabled{};Phase phase{};long long pair_qpc{};
    std::array<std::array<float,16>,2> expected{};};
inline bool matches(const std::array<float,16>& effective,const std::array<float,16>& expected,bool after_prepare) {
    for(unsigned i=0;i<16;++i)if(!std::isfinite(effective[i]) ||
        (!(after_prepare && (i==8 || i==9)) && effective[i]!=expected[i]))return false;
    return true;
}
inline bool supported(const gears_source_args::Snapshot& source) {
    const size_t p=0xb0;
    for(unsigned i=0;i<16;++i)if(!std::isfinite(source.value<float>(p+4*i)))return false;
    for(unsigned i:{1u,2u,3u,4u,6u,7u,12u,13u,15u})if(source.value<float>(p+4*i)!=0.f)return false;
    return source.value<float>(p)>0.f && source.value<float>(p+20)>0.f &&
        source.value<float>(p+44)==1.f && source.value<float>(p+56)!=0.f;
}
inline bool apply(gears_source_args::Snapshot& input,Phase selected) {
    if(!supported(input))return false;
    size_t offset=selected==Phase::b?0xb0:0xd0;
    float original=input.value<float>(offset),value=original;
    if(selected==Phase::b)value*=2.f;
    else if(selected==Phase::c)value+=.1f;
    else return true;
    if(!std::isfinite(value) || value==original)return false;
    memcpy(input.bytes.data()+offset,&value,4);return true;
}
}
