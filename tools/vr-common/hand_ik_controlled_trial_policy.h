#pragma once
#include "hand_ik_trial_policy.h"

namespace gears_hand_ik_controlled {
using gears_hand_ik::Triple;
constexpr unsigned window_count=6,maximum_samples=4096;
constexpr std::uint64_t wait_limit_ms=5000,total_limit_ms=15000;
constexpr std::array<int,window_count> x_offsets{{0,1,-1,-1,1,0}};
inline int offset(unsigned window) noexcept {
    return window<window_count?x_offsets[window]:0;
}
inline bool target(unsigned window,Triple const& hand,Triple& out) noexcept {
    if(window>=window_count||!gears_hand_ik::finite(hand))return false;
    out=hand;out[0]+=float(offset(window))*gears_hand_ik::trial_offset_units;
    return gears_hand_ik::finite(out);
}
// This is a recording protocol. Neither an individual displacement nor six
// apparently directional displacements can authorize a visible-hand feature.
inline bool effect_proved() noexcept {return false;}
struct TrendSummary {
    unsigned samples{};
    std::uint64_t began{},ended{};
    bool valid{},mesh_tick_known{},mesh_tick_progress{},render_progress{};
    unsigned distinct_mesh_ticks{};
    bool complete() const noexcept {
        return valid&&samples>=8&&distinct_mesh_ticks>=8&&ended>=began&&
            ended-began>=gears_hand_ik::recovery_ms&&mesh_tick_known&&
            mesh_tick_progress&&render_progress;
    }
};
// Only caller-certified, read-only baseline samples enter this accumulator.
// Duplicate millisecond stamps do not pad the observation duration or count.
// Qualification also needs eight distinct known mesh Tick values: callbacks
// from the same Tick or alternating between two Tick tags cannot pad that count.
// The BoolProperty ControlTickTag is deliberately absent from qualification.
class ReadOnlyTrend {
    TrendSummary summary_{};
    float first_render_{},last_world_{};
    std::array<int,8> mesh_ticks_{};
    void observe_mesh_tick(bool known,int mesh_tick) noexcept {
        if(!known||summary_.distinct_mesh_ticks==mesh_ticks_.size())return;
        for(unsigned i=0;i<summary_.distinct_mesh_ticks;++i)if(mesh_ticks_[i]==mesh_tick)return;
        mesh_ticks_[summary_.distinct_mesh_ticks++]=mesh_tick;
        summary_.mesh_tick_progress=summary_.distinct_mesh_ticks>1;
    }
public:
    void reset() noexcept {*this={};}
    TrendSummary const& summary() const noexcept {return summary_;}
    bool observe(std::uint64_t now,float world,float render,bool tick_known,
                 int mesh_tick,bool qualified) noexcept {
        const bool usable=qualified&&std::isfinite(world)&&std::isfinite(render)&&
            render>0&&world>=render&&world-render<=.1f;
        if(!summary_.samples){
            summary_={1,now,now,usable,tick_known,false,false};
            first_render_=render;last_world_=world;observe_mesh_tick(tick_known,mesh_tick);
            return false;
        }
        if(!usable||now<summary_.ended||world<last_world_)summary_.valid=false;
        summary_.mesh_tick_known=summary_.mesh_tick_known&&tick_known;
        observe_mesh_tick(tick_known,mesh_tick);
        if(render>first_render_)summary_.render_progress=true;
        if(now>summary_.ended)++summary_.samples;
        summary_.ended=now;last_world_=world;
        return summary_.complete();
    }
};
struct WindowEvidence {
    bool strength_verified{},restore_verified{},pulse_in_time{};
    bool mesh_tick_progress{},render_progress{},same_mesh_world{};
    TrendSummary before{},after{};
    bool complete() const noexcept {
        return strength_verified&&restore_verified&&pulse_in_time&&
            mesh_tick_progress&&render_progress&&same_mesh_world&&
            before.complete()&&after.complete();
    }
};
class SequenceEvidence {
    std::array<WindowEvidence,window_count> windows_{};
    unsigned completed_{};
public:
    bool append(unsigned window,WindowEvidence const& evidence) noexcept {
        if(window!=completed_||window>=window_count)return false;
        windows_[completed_++]=evidence;return true;
    }
    unsigned completed() const noexcept {return completed_;}
    bool complete() const noexcept {
        if(completed_!=window_count)return false;
        for(auto const& window:windows_)if(!window.complete())return false;
        return true;
    }
};
} // namespace gears_hand_ik_controlled
