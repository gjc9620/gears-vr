#pragma once
#include "weapon_ammo_card_plan.h"
#include <array>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace weapon_ammo_card::pixels {
inline constexpr unsigned width=768,height=256;
using Glyph=std::array<std::uint8_t,7>;
inline Glyph glyph(char c) noexcept {
    switch(c){
    case '0':return {14,17,19,21,25,17,14};case '1':return {4,12,4,4,4,4,14};
    case '2':return {14,17,1,2,4,8,31};case '3':return {30,1,1,14,1,1,30};
    case '4':return {2,6,10,18,31,2,2};case '5':return {31,16,16,30,1,1,30};
    case '6':return {14,16,16,30,17,17,14};case '7':return {31,1,2,4,8,8,8};
    case '8':return {14,17,17,14,17,17,14};case '9':return {14,17,17,15,1,1,14};
    case 'A':return {14,17,17,31,17,17,17};case 'D':return {30,17,17,17,17,17,30};
    case 'E':return {31,16,16,30,16,16,31};case 'F':return {31,16,16,30,16,16,16};
    case 'G':return {14,17,16,23,17,17,15};case 'H':return {17,17,17,31,17,17,17};
    case 'I':return {14,4,4,4,4,4,14};case 'L':return {16,16,16,16,16,16,31};
    case 'N':return {17,25,21,19,17,17,17};case 'O':return {14,17,17,17,17,17,14};
    case 'R':return {30,17,17,30,20,18,17};case 'S':return {15,16,16,14,1,1,30};
    case 'U':return {17,17,17,17,17,17,14};case 'V':return {17,17,17,17,17,10,4};
    default:return {};
    }
}
struct Text {std::string current,reserve;bool throwable{};};
inline bool labels(Counts const& counts,Text& out){
    if(!valid(counts))return false;
    Text candidate{};candidate.throwable=counts.kind==AmmoKind::throwable;
    candidate.current=candidate.throwable&&counts.infinite_spare?"INF":std::to_string(counts.in_gun);
    if(!candidate.throwable)candidate.reserve=counts.infinite_spare?"INF":std::to_string(counts.spare);
    out=std::move(candidate);return true;
}
// Fixed opaque high-contrast compositor source; no game image, progress bar,
// temporal AA or scene exposure is sampled. Actual headset clarity is pending.
inline bool rgba(Counts const& counts,std::vector<std::uint8_t>& out){
    Text value{};if(!labels(counts,value))return false;
    std::vector<std::uint8_t> result(std::size_t(width)*height*4);
    auto pixel=[&](unsigned x,unsigned y,std::array<std::uint8_t,3> color){
        if(x>=width||y>=height)return;
        const auto i=(std::size_t(y)*width+x)*4;
        result[i]=color[0];result[i+1]=color[1];result[i+2]=color[2];result[i+3]=255;
    };
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x)pixel(x,y,{12,17,22});
    auto text=[&](std::string const& s,unsigned center,unsigned y,unsigned scale,std::array<std::uint8_t,3> color){
        const unsigned length=unsigned(s.size()),span=(length*6-1)*scale;
        unsigned x=center-span/2;
        for(char c:s){const auto g=glyph(c);
            for(unsigned row=0;row<7;++row)for(unsigned col=0;col<5;++col)if(g[row]&(1u<<(4-col)))
                for(unsigned dy=0;dy<scale;++dy)for(unsigned dx=0;dx<scale;++dx)
                    pixel(x+col*scale+dx,y+row*scale+dy,color);
            x+=6*scale;
        }
    };
    if(value.throwable){
        text("HELD",width/2,24,4,{167,190,211});
        text(value.current,width/2,90,18,{247,250,253});
    }else{
        text("IN GUN",width/4,24,4,{167,190,211});
        text("RESERVE",width*3/4,24,4,{167,190,211});
        // Maximum validated digit counts fit each half without clipping.
        text(value.current,width/4,90,10,{247,250,253});
        text(value.reserve,width*3/4,90,8,{247,250,253});
    }
    out=std::move(result);return true;
}
inline bool save_ppm(std::string const& path,Counts const& counts){
    std::vector<std::uint8_t> value;if(!rgba(counts,value))return false;
    FILE* file{};
#if defined(_MSC_VER)
    if(fopen_s(&file,path.c_str(),"wb")!=0)return false;
#else
    file=std::fopen(path.c_str(),"wb");
#endif
    if(!file)return false;
    bool ok=std::fprintf(file,"P6\n%u %u\n255\n",width,height)>0;
    for(std::size_t i=0;i<value.size()&&ok;i+=4)ok=std::fwrite(value.data()+i,1,3,file)==3;
    return std::fclose(file)==0&&ok;
}
}
