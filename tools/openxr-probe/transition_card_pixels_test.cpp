#include "transition_card_pixels.h"
#include <cassert>
#include <cstring>
#include <iostream>

int main(int argc,char** argv){
    using namespace transition_card_pixels;
    const auto pixels=rgba();assert(pixels==rgba());
    assert(pixels.size()==std::size_t(width)*height*4);
    assert(pixels[0]==29&&pixels[1]==32&&pixels[2]==38);
    unsigned title_pixels=0,subtitle_pixels=0;
    for(std::size_t i=0;i<pixels.size();i+=4){
        assert(pixels[i+3]==255);
        const auto y=i/4/width;
        if(pixels[i]==238&&pixels[i+1]==242&&pixels[i+2]==247){++title_pixels;assert(y>=73&&y<108);}
        if(pixels[i]==178&&pixels[i+1]==188&&pixels[i+2]==200){++subtitle_pixels;assert(y>=139&&y<160);}
    }
    assert(title_pixels>1000&&subtitle_pixels>1000);
    assert(text_width(title,5)<width-40&&text_width(subtitle,3)<width-40);
    const auto startup=rgba(Variant::starting_vr);
    assert(startup==rgba(Variant::starting_vr)&&startup!=pixels);
    assert(startup.size()==pixels.size()&&startup[0]==29&&startup[1]==32&&startup[2]==38);
    unsigned startup_title_pixels=0,startup_subtitle_pixels=0;
    for(std::size_t i=0;i<startup.size();i+=4){
        assert(startup[i+3]==255);const auto y=i/4/width;
        if(startup[i]==238&&startup[i+1]==242&&startup[i+2]==247){++startup_title_pixels;assert(y>=73&&y<108);}
        if(startup[i]==178&&startup[i+1]==188&&startup[i+2]==200){++startup_subtitle_pixels;assert(y>=139&&y<160);}
    }
    assert(startup_title_pixels>1000&&startup_subtitle_pixels>1000);
    assert(text_width(startup_title,5)<width-40&&text_width(startup_subtitle,3)<width-40);
    assert(!std::strcmp(startup_title,"STARTING VR")&&!std::strcmp(startup_subtitle,"Waiting for the game image"));
    for(auto text:{title,subtitle,startup_title,startup_subtitle})for(unsigned i=0;text[i];++i)if(text[i]!=' '){
        bool drawn=false;for(auto row:glyph(text[i]))drawn=drawn||row!=0;assert(drawn);
    }
    assert(!save_ppm("unused-transition-card-preview.ppm",{})); // Reject before creating a file.
    if(argc==3&&!std::strcmp(argv[1],"--preview"))assert(save_ppm(argv[2],pixels));
    else if(argc!=1)return 2;
    std::cout<<"Transition-card pure RGBA/font/bounds/opacity/determinism tests passed\n";
}
