#include "weapon_ammo_card_pixels.h"
#include <cassert>
#include <cstdio>

using namespace weapon_ammo_card;
int main(int argc,char** argv){
    pixels::Text text{};Counts counts{};
    assert(original_firearm_counts(60,13,240,false,counts));
    assert(pixels::labels(counts,text)&&text.current=="47"&&text.reserve=="240"&&!text.throwable);
    counts.infinite_spare=true;assert(pixels::labels(counts,text)&&text.current=="47"&&text.reserve=="INF");
    assert(original_throwable_counts(3,false,counts));
    assert(pixels::labels(counts,text)&&text.current=="3"&&text.reserve.empty()&&text.throwable);
    counts.infinite_spare=true;assert(pixels::labels(counts,text)&&text.current=="INF"&&text.reserve.empty());
    for(char c='0';c<='9';++c){unsigned lit=0;for(auto row:pixels::glyph(c))lit+=row;
        assert(lit);for(char other='0';other<c;++other)assert(pixels::glyph(c)!=pixels::glyph(other));}
    for(char c:std::string("IN GUN RESERVE HELD INF"))if(c!=' '){unsigned lit=0;
        for(auto row:pixels::glyph(c))lit+=row;assert(lit);}
    std::vector<std::uint8_t> image;
    for(Counts c:std::array<Counts,5>{{{AmmoKind::firearm,0,0,false},{AmmoKind::firearm,47,240,false},
        {AmmoKind::firearm,10000,1000000,true},{AmmoKind::throwable,1000000,0,false},{AmmoKind::throwable,0,0,true}}}){
        assert(pixels::rgba(c,image)&&image.size()==std::size_t(pixels::width)*pixels::height*4);
        unsigned lit=0;
        for(std::size_t i=0;i<image.size();i+=4){assert(image[i+3]==255);if(image[i]>200)++lit;}
        assert(lit>1000);
        // Background around the numbers remains opaque and free of progress bars.
        for(unsigned x=0;x<pixels::width;++x){auto i=(std::size_t(pixels::height-1)*pixels::width+x)*4;
            assert(image[i]==12&&image[i+1]==17&&image[i+2]==22);}
    }
    const auto old=image;assert(!pixels::rgba({},image)&&image==old);
    if(argc>1){assert(original_firearm_counts(60,13,240,false,counts));assert(pixels::save_ppm(argv[1],counts));}
    std::puts("WeaponAmmoCardPixelsTest passed");
}
