#include "headset_fps_pixels.h"
#include <cassert>
#include <cstdio>
int main(int argc,char**argv){
    const auto a=headset_fps_pixels::rgba("GAME 30.0 FPS");
    const auto b=headset_fps_pixels::rgba("GAME 30.0 FPS");
    assert(a==b&&a.size()==std::size_t(headset_fps_pixels::width)*headset_fps_pixels::height*4);
    unsigned bright{};for(std::size_t i=0;i<a.size();i+=4){assert(a[i+3]==255);bright+=a[i]>200;}
    assert(bright>100);assert(headset_fps_pixels::width==768&&headset_fps_pixels::height==256);
    if(argc==3)assert(headset_fps_pixels::save_ppm(argv[2],a));
    std::printf("{\"headset_fps_pixels_checks\":true,\"bright\":%u}\n",bright);
}
