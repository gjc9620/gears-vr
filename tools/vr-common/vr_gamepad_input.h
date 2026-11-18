#pragma once
#include <atomic>
#include "controller_continuity_frame.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>

// VR face/menu buttons enter the ORIGINAL gamepad bindings. VR weapon
// triggers/grips retain their existing single owners. A known menu uses the
// original D-pad navigation for the left stick; gameplay keeps its VR owner.
namespace gears_vr_gamepad {
namespace vr=gears_vr;namespace hc=gears_hand_continuity;
constexpr std::uint16_t start=0x10,left_thumb=0x40,right_thumb=0x80,left_shoulder=0x100,right_shoulder=0x200,a=0x1000,b=0x2000,x=0x4000,y=0x8000;
constexpr std::uint16_t up=1,down=2,left=4,right=8;
// Restore an owned equipment journal before original input can change its
// state OR pause the PlayerTick that performs that restoration. The same
// mask is used to notice a pending edge and to defer its actual delivery.
constexpr std::uint16_t equipment_restore_buttons=a|b|x|y|start;
struct State {
    std::uint16_t buttons{};std::int16_t lx{},ly{},rx{},ry{};std::uint8_t lt{},rt{};
    bool operator==(State const& p)const noexcept{return buttons==p.buttons&&lx==p.lx&&ly==p.ly&&rx==p.rx&&ry==p.ry&&lt==p.lt&&rt==p.rt;}
};
class Route {
    struct Button {unsigned hand,control;std::uint32_t raw;std::uint16_t pad;};
    static constexpr std::array<Button,7> buttons_{{
        {1,vr::control_a,vr::controller_a,a},{1,vr::control_b,vr::controller_b,b},
        {0,vr::control_x,vr::controller_x,x},{0,vr::control_y,vr::controller_y,y},
        {0,vr::control_stick,vr::controller_stick_click,left_thumb},
        {1,vr::control_stick,vr::controller_stick_click,right_thumb},
        {0,vr::control_menu,vr::controller_menu,start}}};
    hc::TrackedInput previous_{};hc::Publisher publisher_{};
    State base_{},returned_{};std::uint32_t packet_{};
    std::array<std::int64_t,7> pulse_{};
    std::uint16_t blocked_{},menu_direction_{},physical_buttons_{},consumed_{};bool axes_blocked_{},known_{},axes_owned_{},menu_navigation_{};
    std::int64_t stamp_{};bool look_suppressed_{},look_needs_neutral_{};
    static std::int16_t axis(float value) noexcept {
        return static_cast<std::int16_t>(std::lround(std::clamp(double(value),-1.,1.)*(value<0?32768.:32767.)));
    }
    static std::uint16_t direction(std::int16_t lx,std::int16_t ly,std::uint16_t previous) noexcept {
        constexpr int press=18022,release=11468; //55% engage,35% release.
        if((previous==up&&ly>=release)||(previous==down&&ly<=-release)||
           (previous==right&&lx>=release)||(previous==left&&lx<=-release))return previous;
        if(std::abs(int(ly))>=std::abs(int(lx)))return ly>=press?up:ly<=-press?down:std::uint16_t{};
        return lx>=press?right:lx<=-press?left:std::uint16_t{};
    }
    bool key(hc::TrackedInput const& t,hc::Publisher p)const noexcept {
        return publisher_.pid==p.pid&&publisher_.start==p.start&&previous_.stream_epoch==t.stream_epoch&&
            previous_.tracking_epoch[0]==t.tracking_epoch[0]&&previous_.tracking_epoch[1]==t.tracking_epoch[1]&&
            hc::detail::same_key(previous_.input,t.input);
    }
public:
    void invalidate() noexcept {base_={};pulse_={};blocked_=menu_direction_=physical_buttons_=consumed_=0;axes_blocked_=known_=axes_owned_=menu_navigation_=false;stamp_=0;look_suppressed_=look_needs_neutral_=false;}
    bool observe(vr::ControllerInputFrame const& f,hc::TrackedInput const& t,hc::Publisher publisher,
                 std::int64_t now,bool physical_input_allowed,bool allow_sim=false) noexcept {
        const bool simulated=f.source==vr::StereoSource::MetaOperatorSimulated;
        if(!publisher.pid||!publisher.start||!hc::tracked_input_shape_valid(t)||!hc::same_input(f,t.input)||
           !vr::stereo_source_allowed(f.source,allow_sim)||!f.focused||(!simulated&&!physical_input_allowed)||
           now<f.sync_ns||now<f.received_ns||now-f.sync_ns>100000000||now-f.received_ns>100000000){invalidate();return false;}
        const bool display_changed=known_&&f.display_mode!=previous_.input.display_mode;
        const bool boundary=!known_||!key(t,publisher)||f.sequence<previous_.input.sequence||
            f.sync_ns<previous_.input.sync_ns||f.received_ns<previous_.input.received_ns;
        const bool menu_hint=f.display_mode&&now>=f.display_observed_ns&&now-f.display_observed_ns<=1500000000;
        const bool navigation=menu_hint&&f.display_observed_ns>0&&f.display_mode>=2&&f.display_mode<=4;
        const bool navigation_changed=known_&&navigation!=menu_navigation_;
        axes_owned_=!menu_hint||f.display_mode!=1;
        std::uint16_t raw{};
        for(auto const& button:buttons_)if(f.hands[button.hand].buttons&f.hands[button.hand].buttons_active&button.raw)raw|=button.pad;
        physical_buttons_=raw;consumed_&=physical_buttons_;
        // Some runtimes reserve Menu for their system. Both thumb clicks are
        // an in-VR fallback; suppress their separate gameplay clicks in it.
        const bool chord=(raw&(left_thumb|right_thumb))==(left_thumb|right_thumb);
        if(chord)raw=std::uint16_t((raw&~(left_thumb|right_thumb))|start);
        State next{};next.buttons=raw;
        if(axes_owned_){
            // A known menu needs button/axis state, not a spatial hand pose.
            const bool menu=menu_hint&&f.display_mode!=1;
            for(unsigned i=0;i<2;++i)if(f.hands[i].stick_active&&(menu||
                vr::controller_pose_usable(f.hands[i].grip,f.hands[i].grip_flags,f.hands[i].grip_active))){
                if(i==0){next.lx=axis(f.hands[i].stick_x);next.ly=axis(f.hands[i].stick_y);}
                else{next.rx=axis(f.hands[i].stick_x);next.ry=axis(f.hands[i].stick_y);}
            }
        }
        const bool neutral_axes=std::abs(int(next.lx))<4916&&std::abs(int(next.ly))<4916&&
            std::abs(int(next.rx))<4916&&std::abs(int(next.ry))<4916;
        // A menu/theater hint does not replace a controller or tracking epoch.
        // Preserve digital levels/edges across it; otherwise a B press begun
        // during the asynchronous pause display handoff is swallowed. Axes
        // still rearm separately before entering a different routing context.
        if(boundary){pulse_={};blocked_=raw;}
        else if(f.sequence>previous_.input.sequence){
            const auto old_sync=previous_.input.sync_ns;
            if(now>=old_sync&&now-old_sync<=100000000&&f.sync_ns>old_sync&&f.sync_ns-old_sync<=100000000){
                for(unsigned i=0;i<buttons_.size();++i){auto const& button=buttons_[i];
                    if(f.hands[button.hand].pressed[button.control]>previous_.input.hands[button.hand].pressed[button.control]&&
                       !((blocked_|consumed_)&button.pad))pulse_[i]=old_sync; // Conservative ORIGINAL edge bound.
                }
            }
        }
        if(boundary||display_changed||navigation_changed)axes_blocked_=!neutral_axes;
        blocked_&=raw;next.buttons&=std::uint16_t(~(blocked_|consumed_));
        if(chord){
            const bool previous_chord=!boundary&&
                (previous_.input.hands[0].buttons&previous_.input.hands[0].buttons_active&vr::controller_stick_click)&&
                (previous_.input.hands[1].buttons&previous_.input.hands[1].buttons_active&vr::controller_stick_click);
            // Keep the real second-thumb edge as the original pause pulse.
            // Otherwise a short fallback chord released while equipment is
            // restoring disappears, even though a physical Menu tap survives.
            // Reuse its original timestamp; no repeated held-chord edges.
            if(!previous_chord)pulse_[6]=std::max(pulse_[6],std::max(pulse_[4],pulse_[5]));
            pulse_[4]=pulse_[5]=0;
        }
        if(neutral_axes)axes_blocked_=false;
        if(axes_blocked_)next.lx=next.ly=next.rx=next.ry=0;
        menu_direction_=navigation&&!axes_blocked_?direction(next.lx,next.ly,menu_direction_):std::uint16_t{};
        if(navigation){
            // One original navigation input, never analog+digital duplication.
            // The original game owns repeat timing and widget selection.
            next.buttons|=menu_direction_;next.lx=next.ly=0;
        }
        menu_navigation_=navigation;
        base_=next;previous_=t;publisher_=publisher;stamp_=f.sync_ns;known_=true;return true;
    }
    // Used by an independently authenticated gameplay owner AFTER it accepts
    // an input. Unlike a temporary fence, consumption must never replay that
    // pulse on handoff, and a held button stays suppressed until physical
    // release. Record physical levels BEFORE thumb-chord translation so a
    // pause chord cannot accidentally rearm a still-held R3 on chord exit.
    void consume_buttons(std::uint16_t mask) noexcept {
        if(!known_)return;
        for(unsigned i=0;i<buttons_.size();++i)if(mask&buttons_[i].pad)pulse_[i]=0;
        consumed_|=std::uint16_t(mask&physical_buttons_);
        base_.buttons&=std::uint16_t(~mask);
    }
    std::uint16_t pending_buttons(std::int64_t now)const noexcept {
        if(!known_||now<stamp_||now-stamp_>100000000)return 0;
        auto bits=base_.buttons;
        for(unsigned i=0;i<pulse_.size();++i)
            if(pulse_[i]&&now>=pulse_[i]&&now-pulse_[i]<=100000000)bits|=buttons_[i].pad;
        return bits;
    }
    State sample(std::int64_t now,bool game_consumer,std::uint32_t& packet,std::uint16_t fenced=0,bool suppress_look=false) noexcept {
        State value{};
        if(known_&&now>=stamp_&&now-stamp_<=100000000){
            value=base_;
            for(unsigned i=0;i<pulse_.size();++i){
                if(pulse_[i]&&(now<pulse_[i]||now-pulse_[i]>100000000))pulse_[i]=0;
                if(pulse_[i]){value.buttons|=buttons_[i].pad;if(game_consumer&&!(fenced&buttons_[i].pad))pulse_[i]=0;}
            }
        }else invalidate();
        const bool look_neutral=std::abs(int(value.rx))<4916&&std::abs(int(value.ry))<4916;
        if(look_suppressed_&&!suppress_look&&!look_neutral)look_needs_neutral_=true;
        if(look_neutral)look_needs_neutral_=false;
        look_suppressed_=suppress_look;
        if(suppress_look||look_needs_neutral_)value.rx=value.ry=0;
        value.buttons&=std::uint16_t(~(fenced|consumed_));
        if(!(value==returned_)){returned_=value;++packet_;}
        packet=packet_;return value;
    }
    bool axes_owned()const noexcept{return known_&&axes_owned_;}
};
// Native mounted Scene arms a restriction; only an actual neutral right-axis
// game poll acknowledges it. Ordinary authenticated handoff/stop clears it.
class MountedLookVeto {
    std::atomic<unsigned> state_{}; //0 ordinary,1 armed,2 zero game poll observed
public:
    bool arm_ready() noexcept {unsigned expected=0;state_.compare_exchange_strong(expected,1,std::memory_order_acq_rel);return ready();}
    bool suppressed() const noexcept {return state_.load(std::memory_order_acquire)!=0;}
    bool ready() const noexcept {return state_.load(std::memory_order_acquire)==2;}
    void game_poll_zero() noexcept {unsigned expected=1;state_.compare_exchange_strong(expected,2,std::memory_order_acq_rel);}
    void clear() noexcept {state_.store(0,std::memory_order_release);}
};
}
