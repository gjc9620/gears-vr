#pragma once
#include <cstdint>
namespace gears_vr {
enum class LeftInteractionControl : std::uint32_t {grip,pinch};
// Select one logical action from ORIGINAL independent squeeze/trigger edges.
// No physical packet is rewritten and a held control cannot become another.
struct LeftControlSample {bool known{},down{};};
struct LeftControlSelection {LeftInteractionControl control{LeftInteractionControl::grip};bool known{},down{};};
class LeftControlSelector {
    LeftInteractionControl selected_{LeftInteractionControl::grip};
    bool armed_{},latched_{},pinch_enabled_{};
public:
    LeftControlSelection update(LeftControlSample grip,LeftControlSample pinch,bool allow_pinch) noexcept {
        if(!allow_pinch){
            const bool changed=pinch_enabled_&&latched_&&selected_==LeftInteractionControl::pinch;
            pinch_enabled_=false;latched_=armed_=false;selected_=LeftInteractionControl::grip;
            return {selected_,grip.known&&!changed,grip.down&&!changed};
        }
        pinch_enabled_=true;
        if(!grip.known||!pinch.known){armed_=latched_=false;return {selected_,false,false};}
        if(!grip.down&&!pinch.down){armed_=true;latched_=false;return {selected_,true,false};}
        if(!latched_){
            if(!armed_||grip.down==pinch.down)return {selected_,false,false};
            selected_=pinch.down?LeftInteractionControl::pinch:LeftInteractionControl::grip;
            latched_=true;armed_=false;
        }
        return {selected_,true,selected_==LeftInteractionControl::pinch?pinch.down:grip.down};
    }
};
}
