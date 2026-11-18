#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
namespace headset_fps_pixels {
inline constexpr unsigned width=768,height=256;
inline constexpr unsigned panel_width=320,panel_height=96,panel_x=224,panel_y=80;
inline std::array<std::uint8_t,7> glyph(char c){
    switch(c){
    case '0':return {14,17,19,21,25,17,14};case '1':return {4,12,4,4,4,4,14};
    case '2':return {14,17,1,2,4,8,31};case '3':return {30,1,1,14,1,1,30};
    case '4':return {2,6,10,18,31,2,2};case '5':return {31,16,16,30,1,1,30};
    case '6':return {14,16,16,30,17,17,14};case '7':return {31,1,2,4,8,8,8};
    case '8':return {14,17,17,14,17,17,14};case '9':return {14,17,17,15,1,1,14};
    case '.':return {0,0,0,0,0,12,12};case '-':return {0,0,0,31,0,0,0};
    case ' ':return {};case 'A':return {14,17,17,31,17,17,17};case 'C':return {14,17,16,16,16,17,14};
    case 'E':return {31,16,16,30,16,16,31};case 'F':return {31,16,16,30,16,16,16};
    case 'G':return {14,17,16,23,17,17,15};case 'M':return {17,27,21,21,17,17,17};
    case 'P':return {30,17,17,30,16,16,16};case 'S':return {15,16,16,14,1,1,30};
    default:return {};
    }
}
inline unsigned text_width(char const* s,unsigned scale){unsigned n=0;while(s[n])++n;return n?(n*6-1)*scale:0;}
inline std::vector<std::uint8_t> rgba(std::string const& value){
    std::vector<std::uint8_t> out(std::size_t(width)*height*4);
    const auto put=[&](unsigned x,unsigned y,std::array<std::uint8_t,3> c){
        if(x>=width||y>=height)return;auto* p=out.data()+(std::size_t(y)*width+x)*4;p[0]=c[0];p[1]=c[1];p[2]=c[2];p[3]=255;};
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x)put(x,y,{20,24,30});
    for(unsigned y=panel_y;y<panel_y+panel_height;++y)for(unsigned x=panel_x;x<panel_x+panel_width;++x)put(x,y,{20,24,30});
    for(unsigned x=panel_x+10;x<panel_x+panel_width-10;++x){put(x,panel_y+8,{70,100,120});put(x,panel_y+panel_height-9,{70,100,120});}
    const unsigned scale=3,x0=panel_x+(panel_width-text_width(value.c_str(),scale))/2;
    unsigned x=x0;for(char c:value){const auto bits=glyph(c);for(unsigned row=0;row<7;++row)for(unsigned col=0;col<5;++col)if(bits[row]&(1u<<(4-col)))for(unsigned dy=0;dy<scale;++dy)for(unsigned dx=0;dx<scale;++dx)put(x+col*scale+dx,panel_y+31+row*scale+dy,{235,242,250});x+=6*scale;}
    return out;
}
inline bool save_ppm(std::string const& path,std::vector<std::uint8_t> const& pixels){
    if(pixels.size()!=std::size_t(width)*height*4)return false;FILE* f{};if(fopen_s(&f,path.c_str(),"wb")!=0)return false;
    bool ok=std::fprintf(f,"P6\n%u %u\n255\n",width,height)>0;for(std::size_t i=0;i<pixels.size()&&ok;i+=4)ok=std::fwrite(pixels.data()+i,1,3,f)==3;return std::fclose(f)==0&&ok;
}
}
