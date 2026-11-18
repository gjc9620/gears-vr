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
    // 48572 still shows the old prompt across the inspected rifle. Use the
    // lower-right HUD area, separate from the upper-right ammo panel, while
    // preserving the original text/icon sizing and action semantics.
    out={{clip_x*.84f,clip_y*.08f}};return true;
}
// Original panel X=ClipX-SafeZoneRight-frameUL*ResScaleSmall. Borrow only
// those two SafeZone values during UpdateWeaponIndicator; keep scaling intact.
inline bool weapon_panel_plan(float clip_x,float clip_y,float frame_ul,float resolution,
    std::array<float,2> const& before,std::array<float,2>& out) noexcept {
    for(float v:{clip_x,clip_y,frame_ul,resolution,before[0],before[1]})if(!std::isfinite(v))return false;
    if(clip_x<128||clip_x>16384||clip_y<128||clip_y>16384||frame_ul<1||frame_ul>4096||
       resolution<=0||resolution>16||before[0]<0||before[0]>=clip_x||before[1]<0||before[1]>=clip_y)return false;
    if(frame_ul*resolution>clip_x*.60f)return false;
    // 31424 confirms the central placement is applied but still crosses the
    // raised weapon. Keep the complete panel inside the existing HUD plane's
    // upper-right margin; this does not move the separately placed reload bar.
    out={{clip_x*.04f,clip_y*.02f}};return true;
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
