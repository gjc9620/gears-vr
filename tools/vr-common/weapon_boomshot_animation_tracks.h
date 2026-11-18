#pragma once
#include "weapon_boomshot_barrel_path.h"
#include <algorithm>
#include <array>
#include <cstring>

namespace gears_weapon::boomshot_tracks {
// Original three-track, uniform Float96 translation stream. All rotations
// are constant. This decoder never grants an animation/hand/ammo operation.
struct Samples {
    std::array<std::array<gears_vr::Vector,256>,3> positions{};
    std::array<std::array<float,3>,3> rotations{};
    unsigned count{};float length{};
};
inline bool decode(std::array<std::int32_t,12> const& offsets,
    unsigned char const* bytes,std::size_t size,unsigned frames,float length,Samples& out) noexcept {
    if(!bytes||size<72||size>65536||frames<7||frames>256||
       !std::isfinite(length)||length<=0||length>10)return false;
    struct Span {std::size_t begin{},end{};};std::array<Span,6> spans{};Samples value{};
    value.count=frames;value.length=length;
    for(unsigned bone=0;bone<3;++bone){
        const auto t=offsets[bone*4],n=offsets[bone*4+1],q=offsets[bone*4+2];
        if(t<0||q<0||n!=(bone==2?int(frames):1)||offsets[bone*4+3]!=1)return false;
        const auto tb=std::size_t(n)*12;
        if(std::size_t(t)>size||tb>size-std::size_t(t)||std::size_t(q)>size||12>size-std::size_t(q))return false;
        spans[bone*2]={std::size_t(t),std::size_t(t)+tb};spans[bone*2+1]={std::size_t(q),std::size_t(q)+12};
        auto& rotation=value.rotations[bone];std::memcpy(rotation.data(),bytes+q,12);double squared{};
        for(auto axis:rotation){if(!std::isfinite(axis))return false;squared+=double(axis)*axis;}
        if(squared>1.00001)return false;
        for(unsigned i=0;i<frames;++i){std::array<float,3> p{};std::memcpy(p.data(),bytes+t+(n==1?0:12*i),12);
            for(auto axis:p)if(!std::isfinite(axis)||std::abs(axis)>100000)return false;
            value.positions[bone][i]={p[0],p[1],p[2]};}
    }
    std::sort(spans.begin(),spans.end(),[](Span a,Span b){return a.begin<b.begin;});
    if(spans.front().begin||spans.back().end!=size)return false;
    for(unsigned i=1;i<spans.size();++i)if(spans[i].begin!=spans[i-1].end)return false;
    out=value;return true;
}
class Path {
    std::array<double,256> q_{};unsigned count_{},open_begin_{},open_end_{},close_begin_{},close_end_{};
    float length_{};
public:
    bool bind(Samples const& values) noexcept {
        if(values.count<7||values.count>256||!std::isfinite(values.length)||values.length<=0||values.length>10)return false;
        Path next{};next.count_=values.count;next.length_=values.length;
        for(unsigned i=0;i<values.count;++i)if(!boomshot_barrel::progress(values.positions[2][i],next.q_[i]))return false;
        constexpr double epsilon=1e-5;
        if(next.q_[0]>epsilon||next.q_[values.count-1]>epsilon)return false;
        unsigned i=1;
        while(i<values.count&&next.q_[i]<=epsilon)++i;
        if(i==values.count)return false;next.open_begin_=i-1;
        for(;i<values.count&&next.q_[i]<1-epsilon;++i)
            if(next.q_[i]<next.q_[i-1]-epsilon)return false;
        if(i==values.count)return false;next.open_end_=i;
        while(i+1<values.count&&next.q_[i+1]>=1-epsilon)++i;
        next.close_begin_=i++;
        if(next.close_begin_<=next.open_end_||i==values.count)return false;
        for(;i<values.count&&next.q_[i]>epsilon;++i)
            if(next.q_[i]>next.q_[i-1]+epsilon)return false;
        if(i==values.count)return false;next.close_end_=i;
        for(;i<values.count;++i)if(next.q_[i]>epsilon)return false;
        *this=next;return true;
    }
    // Reference uniform-key timing only. A later actual bone ACK is mandatory;
    // this mapping cannot certify the engine's time/interpolation convention.
    bool time(double q,boomshot_barrel::Direction direction,float& out) const noexcept {
        if(!count_||!std::isfinite(q)||q<0||q>1)return false;
        const bool opening=direction==boomshot_barrel::Direction::opening;
        const auto begin=opening?open_begin_:close_begin_,end=opening?open_end_:close_end_;
        double key=double(end);
        if((opening&&q<=q_[begin])||(!opening&&q>=q_[begin]))key=double(begin);
        else for(unsigned i=begin;i<end;++i){const double a=q_[i],b=q_[i+1];
            if((opening&&q<=b)||(!opening&&q>=b)){
                if(std::abs(b-a)<=1e-10)return false;key=i+(q-a)/(b-a);break;}}
        out=float(key*length_/count_);return std::isfinite(out)&&out>=0&&out<=length_;
    }
    unsigned opening_begin() const noexcept{return open_begin_;}
    unsigned opening_end() const noexcept{return open_end_;}
    unsigned closing_begin() const noexcept{return close_begin_;}
    unsigned closing_end() const noexcept{return close_end_;}
};
}
