#pragma once
#include <array>
#include <cmath>
#include <cstdint>

// Cosmetic placement for the original DrawActionInfo only. These are the
// original HUD's centre-X / bottom inset, not input, subtitle or action data.
namespace gears_hud_action_layout {
inline bool plan(float clip_x,float clip_y,std::array<float,2> const& before_layout,
    std::array<float,2>& out) noexcept {
    if(!std::isfinite(clip_x)||!std::isfinite(clip_y)||clip_x<128||clip_x>16384||clip_y<128||clip_y>16384||
       !std::isfinite(before_layout[0])||before_layout[0]<0||before_layout[0]>clip_x||
       !std::isfinite(before_layout[1])||before_layout[1]<0||before_layout[1]>=clip_y)return false;
    // DrawActionInfo computes Y=ClipY-SafeZoneBottom-subtitle/text/icon sizes.
    // Keep the stock prompt readable above/left of normal held-weapon space.
    out={{clip_x*.24f,clip_y*.72f}};return true;
}
struct Scope {
    std::array<std::uintptr_t,2> addresses{};
    std::array<float,2> before{},wanted{};
    unsigned owned{};
    template<class Read,class Write>bool restore(Read read,Write write) noexcept {
        bool okay=true;
        for(unsigned i=0;i<2;++i)if(owned&(1u<<i)){
            float current{},after{};
            if(!read(addresses[i],current)){okay=false;continue;}
            if(current==before[i]){owned&=~(1u<<i);continue;}
            // A different writer's value is never overwritten as "cleanup".
            if(current!=wanted[i]||!write(addresses[i],before[i])||
               !read(addresses[i],after)||after!=before[i]){okay=false;continue;}
            owned&=~(1u<<i);
        }
        return okay&&!owned;
    }
    template<class Read,class Write>bool apply(Read read,Write write) noexcept {
        if(owned||!addresses[0]||!addresses[1]||addresses[0]%4||addresses[1]%4||
           addresses[0]==addresses[1])return false;
        for(unsigned i=0;i<2;++i){float current{};
            if(!std::isfinite(before[i])||!std::isfinite(wanted[i])||
               !read(addresses[i],current)||current!=before[i])return false;
        }
        for(unsigned i=0;i<2;++i){
            // Journal the attempted store BEFORE it can fault or partially
            // succeed. The existing call-owner unwind can undo the same scope.
            owned|=1u<<i;float after{};
            if(!write(addresses[i],wanted[i])||!read(addresses[i],after)||after!=wanted[i]){
                restore(read,write);return false;
            }
        }
        return true;
    }
};
}
