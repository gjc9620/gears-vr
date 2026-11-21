#include "headset_fps_state.h"
#include "xr_delivery_counter.h"
#include <cassert>
#include <cstdio>
#include <string>
int main(){
    using gears_fps_display::Rate;
    Rate r;r.render_text(1,0);assert(!r.available&&std::string(r.text)=="--");
    std::uint64_t n=0;
    for(unsigned tick=0;tick<=480;++tick){
        const auto t=1000000000ULL+std::uint64_t(tick)*1000000000ULL/120;
        if(tick%4==0){++n;r.observe(t,1,1,n);}
        r.render_text(t,n);
        if(tick<120)assert(!r.available);
        else assert(r.available&&r.fps==30&&std::string(r.text)=="GAME 30.0 FPS");
    }
    const auto saved=r.fps;
    gears_xr_delivery::Counter accepted;accepted.observe(true,{1,1,1,1,1});
    accepted.observe(false,{1,1,1,2,2});accepted.observe(true,{1,1,1,1,1});
    accepted.observe(true,{1,1,1,1,999});assert(accepted.distinct==1);
    r.observe(5000000000ULL,1,1,n);r.render_text(5000000000ULL,n);assert(r.fps==saved);
    r.render_text(6000000000ULL,n);assert(r.available&&r.fps==0&&std::string(r.text)=="GAME 0.0 FPS");
    r.render_text(5999999999ULL,n);assert(!r.available);
    r.render_text(6000000000ULL,n);assert(r.available&&r.fps==0);
    r.observe(6000000001ULL,1,2,n+1);assert(!r.available&&std::string(r.text)=="--");
    r.observe(6000000002ULL,2,3,n+2);assert(!r.available&&r.session==2);
    Rate fast;for(unsigned i=0;i<=360;++i){const auto t=1000000000ULL+std::uint64_t(i)*1000000000ULL/120;
        fast.observe(t,3,4,i+1);fast.render_text(t,i+1);if(i>=120)assert(fast.available&&fast.fps==120);}
    Rate slow;slow.observe(1000000000ULL,1,1,1);slow.observe(2000000000ULL,1,1,2);
    assert(slow.available&&slow.fps==1);slow.render_text(3000000000ULL,2);assert(slow.fps==0);
    Rate jitter;for(unsigned i=0;i<=90;++i){const auto t=1000000000ULL+std::uint64_t(i)*1000000000ULL/30+(i%2)*1000000ULL;
        jitter.observe(t,1,1,i+1);jitter.render_text(t,i+1);if(i>30)assert(jitter.available&&jitter.fps>=29&&jitter.fps<=31);}
    Rate jump;jump.observe(1,1,1,1);jump.observe(2000000000ULL,1,1,5);assert(!jump.available);
    Rate full;for(unsigned i=1;i<=1025;++i)full.observe(i,1,1,i);assert(full.overflow&&!full.available);
    std::printf("{\"passed\":true,\"actual_30\":%.1f,\"actual_120\":%.1f,\"repeated_frames_add_zero\":true,\"stall\":0,\"low_fps\":1}\n",saved,fast.fps);
}
