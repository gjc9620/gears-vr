#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
// Private Native88 contract. These checks do not establish clipping/input/GPU acceptance.
namespace gears_path_reflow_control {
inline std::uint32_t bits(float x){std::uint32_t b;std::memcpy(&b,&x,4);return b;}
inline float value(std::uint32_t b){float x;std::memcpy(&x,&b,4);return x;}
inline bool within_ulps(std::uint32_t a,std::uint32_t b,unsigned limit=2){
    if(!std::isfinite(value(a))||!std::isfinite(value(b))||((a|b)&0x80000000u))return false;
    return (a>b?a-b:b-a)<=limit;
}
struct Input {std::array<std::uint32_t,4> faces{};unsigned char type{};int origin{},sort{};};
struct Plan {Input original{},adjusted{};float height{},offset{};std::array<std::uint32_t,2> expected{};};
template<class Config,class Snapshot> bool metadata(Config const& c,Snapshot const& s){
    return s.identity&&s.mask==2047&&s.units==0x04040404&&s.raw_1dc==c.scene&&s.raw_4e4==0&&
        s.raw_4e8&&s.num==2&&s.max==2&&s.raw_4f8==1&&s.selected_bits==1064098280&&s.zero_bits==1064098280;
}
template<class Config,class Snapshot> bool prepare(Config const& c,Snapshot const& before,Input const& input,float delta,Plan& out){
    if(!metadata(c,before)||input.type!=1||input.origin||input.sort)return false;
    std::array<float,4> v{};for(unsigned i=0;i<4;++i){v[i]=value(input.faces[i]);if(!std::isfinite(v[i])||v[i]<=0)return false;}
    const float width=v[0]+v[2],height=v[1]+v[3];
    if(!std::isfinite(width)||!std::isfinite(height)||width<=0||height<=0||v[0]>=v[2]||v[1]>=v[3]||
       !within_ulps(bits(v[1]/height),c.top_bits)||!within_ulps(bits(v[3]/height),c.bottom_bits)||
       !within_ulps(bits(v[0]/width),before.position[0])||!within_ulps(bits(v[2]/width),before.position[2]))return false;
    Plan p{};p.original=input;p.adjusted=input;p.height=height;p.offset=height*delta;
    if(!std::isfinite(p.offset)||p.offset<=0)return false;
    p.adjusted.faces[1]=bits(v[1]+p.offset);p.adjusted.faces[3]=bits(v[3]+p.offset);
    p.expected={bits(value(c.top_bits)+delta),bits(value(c.bottom_bits)+delta)};
    if(!std::isfinite(value(p.adjusted.faces[1]))||!std::isfinite(value(p.adjusted.faces[3])))return false;
    out=p;return true;
}
template<class Config,class Snapshot> bool stable(Config const& c,Snapshot const& before,Snapshot const& after){
    return metadata(c,before)&&metadata(c,after)&&before.world==after.world&&
        before.position[0]==after.position[0]&&before.position[2]==after.position[2]&&
        before.raw_4e8==after.raw_4e8;
}
template<class Config,class Snapshot> bool controlled_exit(Config const& c,Snapshot const& before,Snapshot const& after,Plan const& p){
    return stable(c,before,after)&&within_ulps(after.position[1],p.expected[0])&&within_ulps(after.position[3],p.expected[1]);
}
template<class Config,class Snapshot> bool original_exit(Config const& c,Snapshot const& before,Snapshot const& after){
    return stable(c,before,after)&&after.position[1]==c.top_bits&&after.position[3]==c.bottom_bits;
}
}
