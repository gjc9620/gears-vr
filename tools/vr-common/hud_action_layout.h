#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

// Cosmetic placement for the original DrawActionInfo and small, explicitly
// owned original-HUD float journals. These never authorize input or actions.
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
template<std::size_t N>struct FloatScope {
    static_assert(N>0&&N<=2);
    std::array<std::uintptr_t,N> addresses{};
    std::array<float,N> before{},wanted{};
    unsigned owned{};
    template<class Read,class Write>bool restore(Read read,Write write) noexcept {
        bool okay=true;
        for(unsigned i=0;i<N;++i)if(owned&(1u<<i)){
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
        if(owned)return false;
        for(unsigned i=0;i<N;++i){
            if(!addresses[i]||addresses[i]%4)return false;
            for(unsigned j=0;j<i;++j)if(addresses[j]==addresses[i])return false;
        }
        for(unsigned i=0;i<N;++i){float current{};
            if(!std::isfinite(before[i])||!std::isfinite(wanted[i])||
               !read(addresses[i],current)||current!=before[i])return false;
        }
        for(unsigned i=0;i<N;++i){
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
using Scope=FloatScope<2>;
}
