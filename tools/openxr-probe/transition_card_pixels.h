#pragma once
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

// Pure CPU RGBA source. No game pixels, OS fonts, graphics API or image library.
namespace transition_card_pixels {
inline constexpr unsigned width=768,height=256;
inline constexpr char title[]="WAITING FOR GAME IMAGE";
inline constexpr char subtitle[]="Updating the VR view";
enum class Variant {returning_to_game,starting_vr,tracking_lost};
inline constexpr char startup_title[]="STARTING VR";
inline constexpr char startup_subtitle[]="Waiting for the game image";
inline constexpr char tracking_title[]="WAITING FOR TRACKING";
inline constexpr char tracking_subtitle[]="Resuming the VR view";
using Glyph=std::array<std::uint8_t,7>;
inline Glyph glyph(char c){
    switch(c){
    case 'A':return {14,17,17,31,17,17,17};case 'B':return {30,17,17,30,17,17,30};
    case 'C':return {14,17,16,16,16,17,14};case 'D':return {30,17,17,17,17,17,30};
    case 'E':return {31,16,16,30,16,16,31};case 'F':return {31,16,16,30,16,16,16};
    case 'G':return {14,17,16,23,17,17,15};case 'H':return {17,17,17,31,17,17,17};
    case 'I':return {14,4,4,4,4,4,14};case 'J':return {7,2,2,2,18,18,12};
    case 'K':return {17,18,20,24,20,18,17};case 'L':return {16,16,16,16,16,16,31};
    case 'M':return {17,27,21,21,17,17,17};case 'N':return {17,25,21,19,17,17,17};
    case 'O':return {14,17,17,17,17,17,14};case 'P':return {30,17,17,30,16,16,16};
    case 'Q':return {14,17,17,17,21,18,13};case 'R':return {30,17,17,30,20,18,17};
    case 'S':return {15,16,16,14,1,1,30};case 'T':return {31,4,4,4,4,4,4};
    case 'U':return {17,17,17,17,17,17,14};case 'V':return {17,17,17,17,17,10,4};
    case 'W':return {17,17,17,21,21,21,10};case 'X':return {17,17,10,4,10,17,17};
    case 'Y':return {17,17,10,4,4,4,4};case 'Z':return {31,1,2,4,8,16,31};
    case 'a':return {0,0,14,1,15,17,15};case 'e':return {0,0,14,17,31,16,14};
    case 'd':return {1,1,15,17,17,17,15};case 'p':return {0,0,30,17,30,16,16};
    case 'u':return {0,0,17,17,17,19,13};case 'v':return {0,0,17,17,17,10,4};
    case 'w':return {0,0,17,17,21,21,10};
    case 'f':return {6,8,8,28,8,8,8};case 'g':return {0,0,15,17,15,1,14};
    case 'h':return {16,16,30,17,17,17,17};case 'i':return {4,0,12,4,4,4,14};
    case 'm':return {0,0,26,21,21,21,21};case 'n':return {0,0,30,17,17,17,17};
    case 'o':return {0,0,14,17,17,17,14};case 'r':return {0,0,22,25,16,16,16};
    case 's':return {0,0,15,16,14,1,30};case 't':return {8,8,28,8,8,9,6};
    case 'x':return {0,0,17,10,4,10,17};
    default:return {};
    }
}
inline unsigned text_width(char const* text,unsigned scale){
    unsigned count=0;for(;text[count];++count){}
    return count?(count*6-1)*scale:0;
}
inline std::vector<std::uint8_t> rgba(Variant variant=Variant::returning_to_game){
    std::vector<std::uint8_t> pixels(std::size_t(width)*height*4);
    const auto pixel=[&](unsigned x,unsigned y,std::array<std::uint8_t,3> color){
        if(x>=width||y>=height)return;
        const auto offset=(std::size_t(y)*width+x)*4;
        pixels[offset]=color[0];pixels[offset+1]=color[1];pixels[offset+2]=color[2];pixels[offset+3]=255;
    };
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x)pixel(x,y,{29,32,38});
    for(unsigned x=20;x<width-20;++x){pixel(x,20,{62,68,78});pixel(x,height-21,{62,68,78});}
    for(unsigned y=20;y<height-20;++y){pixel(20,y,{62,68,78});pixel(width-21,y,{62,68,78});}
    const auto text=[&](char const* value,unsigned y,unsigned scale,std::array<std::uint8_t,3> color){
        unsigned x=(width-text_width(value,scale))/2;
        for(unsigned i=0;value[i];++i){const auto bits=glyph(value[i]);
            for(unsigned row=0;row<7;++row)for(unsigned col=0;col<5;++col)if(bits[row]&(1u<<(4-col)))
                for(unsigned dy=0;dy<scale;++dy)for(unsigned dx=0;dx<scale;++dx)
                    pixel(x+col*scale+dx,y+row*scale+dy,color);
            x+=6*scale;
        }
    };
    const bool starting=variant==Variant::starting_vr;
    const bool tracking=variant==Variant::tracking_lost;
    text(tracking?tracking_title:starting?startup_title:title,73,5,{238,242,247});
    text(tracking?tracking_subtitle:starting?startup_subtitle:subtitle,139,3,{178,188,200});
    return pixels;
}
inline bool save_ppm(std::string const& path,std::vector<std::uint8_t> const& pixels){
    if(pixels.size()!=std::size_t(width)*height*4)return false;
    FILE* file{};
#if defined(_MSC_VER)
    if(fopen_s(&file,path.c_str(),"wb")!=0)return false;
#else
    file=std::fopen(path.c_str(),"wb");
#endif
    if(!file)return false;
    bool ok=std::fprintf(file,"P6\n%u %u\n255\n",width,height)>0;
    for(std::size_t i=0;i<pixels.size()&&ok;i+=4)ok=std::fwrite(pixels.data()+i,1,3,file)==3;
    return std::fclose(file)==0&&ok;
}
}
