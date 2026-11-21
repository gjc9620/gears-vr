#pragma once
#include "weapon_hammerburst_latch.h"

namespace gears_weapon {
// Gun-local metres. This kernel owns no engine calls or ammunition. The
// adapter supplies the current original slide axis/travel and actual bones.
struct SnubSlideGeometry {
    gears_vr::Vector closed_contact{},axis{};
    double travel{},contact_radius{.045},lateral_tolerance{.03},maximum_speed{2.5};
    bool current_instance_verified{};
    double overtravel{.025}; // Hand travel beyond the mechanical stop; part remains clamped.
    double reverse_tolerance{.012};
    bool retain_grasp_outside_bounds{};
    double endpoint_assistance{}; // Optional bounded end alignment; defaults preserve other weapons.
    bool sliding_grip_socket{}; // Retain on projected rail; axial overtravel is not release.
};
class SnubSlideStroke {
public:
    enum class Phase {idle,pulling,waiting_open,returning,complete};
    enum class Rejection {none,sample,token,order,gap,lateral,speed,range};
private:
    using V=gears_vr::Vector;
    SnubSlideGeometry geometry_{};HammerburstLatchSample last_{};
    Phase phase_{};Rejection rejection_{};
    V start_{};double wanted_{},observed_{},return_from_{},return_started_{};
    std::uint64_t bone_frame_{};unsigned closed_frames_{};
    bool full_{},return_can_finish_{};
    static V sub(V a,V b) noexcept {return {a.x-b.x,a.y-b.y,a.z-b.z};}
    static double dot(V a,V b) noexcept {return a.x*b.x+a.y*b.y+a.z*b.z;}
    static double length(V a) noexcept {return std::sqrt(dot(a,a));}
    static bool valid(HammerburstLatchSample const& s) noexcept {
        return s.sequence&&s.milliseconds&&s.grip_token&&s.tracked&&s.gripped&&gears_vr::finite(s.hand);
    }
public:
    void reset() noexcept {*this={};}
    Phase phase() const noexcept {return phase_;}
    Rejection last_rejection() const noexcept {return rejection_;}
    double wanted_distance() const noexcept {return wanted_;}
    bool actual_full_pull() const noexcept {return full_;}
    bool begin(SnubSlideGeometry const& g,HammerburstLatchSample const& s,double actual) noexcept {
        if(phase_!=Phase::idle||!g.current_instance_verified||!valid(s)||
           !gears_vr::finite(g.closed_contact)||!gears_vr::finite(g.axis)||std::abs(length(g.axis)-1)>1e-5||
           !std::isfinite(g.travel)||g.travel<.01||g.travel>.40||
           !std::isfinite(g.contact_radius)||g.contact_radius<=0||g.contact_radius>.20||
           !std::isfinite(g.lateral_tolerance)||g.lateral_tolerance<=0||g.lateral_tolerance>.20||
           !std::isfinite(g.maximum_speed)||g.maximum_speed<=0||g.maximum_speed>5||
           !std::isfinite(g.overtravel)||g.overtravel<0||g.overtravel>.35||
           !std::isfinite(g.reverse_tolerance)||g.reverse_tolerance<0||g.reverse_tolerance>.15||
           !std::isfinite(g.endpoint_assistance)||g.endpoint_assistance<0||g.endpoint_assistance>g.travel*.5||
           !std::isfinite(actual)||std::abs(actual)>.002||length(sub(s.hand,g.closed_contact))>g.contact_radius)return false;
        geometry_=g;last_=s;start_=s.hand;wanted_=0;observed_=actual;bone_frame_=0;
        closed_frames_=0;full_=return_can_finish_=false;rejection_=Rejection::none;phase_=Phase::pulling;return true;
    }
    bool move(HammerburstLatchSample const& s) noexcept {
        if(phase_!=Phase::pulling||rejection_!=Rejection::none)return false;
        const auto reject=[&](Rejection why){rejection_=why;full_=false;return false;};
        if(!valid(s))return reject(Rejection::sample);
        if(s.grip_token!=last_.grip_token)return reject(Rejection::token);
        if(s.sequence<=last_.sequence||s.milliseconds<=last_.milliseconds)return reject(Rejection::order);
        const auto elapsed=s.milliseconds-last_.milliseconds;
        // Observation interval only. The live source retains its own100ms
        // tracking/lease bound; this cannot renew an old native permission.
        if(elapsed>250)return reject(Rejection::gap);
        const auto delta=sub(s.hand,start_);const double axial=dot(delta,geometry_.axis);
        const V lateral{delta.x-axial*geometry_.axis.x,delta.y-axial*geometry_.axis.y,delta.z-axial*geometry_.axis.z};
        if(length(sub(s.hand,last_.hand))>geometry_.maximum_speed*double(elapsed)/1000.)return reject(Rejection::speed);
        const bool lateral_out=length(lateral)>geometry_.lateral_tolerance;
        const bool axial_out=axial<-geometry_.reverse_tolerance||axial>geometry_.travel+geometry_.overtravel;
        // Project onto the line through the acquired hand pose. The socket
        // slides along that line; only radial displacement can leave it.
        // Actual mechanism travel remains clamped below. No extra travel,
        // tracking, token or mechanical acknowledgement is manufactured.
        if(geometry_.sliding_grip_socket&&lateral_out)return reject(Rejection::lateral);
        if(!geometry_.sliding_grip_socket&&(lateral_out||axial_out)){
            if(!geometry_.retain_grasp_outside_bounds)return reject(lateral_out?Rejection::lateral:Rejection::range);
            // Keep an already held constrained part, without creating any
            // stroke progress outside its generous mechanical corridor.
            // The next actual current in-range hand can continue this same
            // grasp; release, identity, sequence, tracking and speed still gate it.
            last_=s;return true;
        }
        last_=s;
        double assist{};
        if(geometry_.endpoint_assistance>0){
            const double gate=geometry_.travel-geometry_.endpoint_assistance;
            const double start=std::max(0.,gate-geometry_.endpoint_assistance);
            const double t=std::clamp((axial-start)/(gate-start),0.,1.);
            assist=geometry_.endpoint_assistance*t*t*(3.-2.*t);
        }
        wanted_=std::clamp(axial+assist,0.,geometry_.travel);return true;
    }
    bool release(double game_time) noexcept {
        if(phase_!=Phase::pulling||!std::isfinite(game_time)||game_time<0)return false;
        if(rejection_!=Rejection::none)return cancel(game_time);
        if(wanted_>=geometry_.travel-.0015&&!full_){phase_=Phase::waiting_open;return true;}
        return_from_=observed_;return_started_=game_time;return_can_finish_=full_;
        closed_frames_=0;phase_=Phase::returning;return true;
    }
    bool cancel(double game_time) noexcept {
        if((phase_!=Phase::pulling&&phase_!=Phase::waiting_open)||!std::isfinite(game_time)||game_time<0)return false;
        return_from_=observed_;return_started_=game_time;return_can_finish_=full_=false;
        closed_frames_=0;phase_=Phase::returning;return true;
    }
    bool advance_return(double game_time) noexcept {
        if(phase_!=Phase::returning||!std::isfinite(game_time)||game_time<return_started_)return false;
        const double t=std::clamp((game_time-return_started_)/.15,0.,1.);
        wanted_=return_from_*(1-t*t*(3-2*t));return true;
    }
    bool acknowledge_bone(double actual,std::uint64_t frame,double game_time) noexcept {
        if(phase_==Phase::idle||phase_==Phase::complete||!std::isfinite(actual)||actual<-.002||
           actual>geometry_.travel+.002||!frame||frame<=bone_frame_||!std::isfinite(game_time)||game_time<0)return false;
        bone_frame_=frame;observed_=std::clamp(actual,0.,geometry_.travel);
        if(phase_==Phase::pulling&&rejection_==Rejection::none&&wanted_>=geometry_.travel-.0015&&
           observed_>=geometry_.travel-.0015)full_=true;
        if(phase_==Phase::waiting_open&&observed_>=geometry_.travel-.0015){
            full_=return_can_finish_=true;return_from_=observed_;return_started_=game_time;closed_frames_=0;phase_=Phase::returning;
        }
        if(phase_==Phase::returning){
            if(wanted_<=.0002&&observed_<=.0015)++closed_frames_;else closed_frames_=0;
            if(closed_frames_>=2){phase_=return_can_finish_?Phase::complete:Phase::idle;wanted_=0;full_=false;}
        }
        return true;
    }
};
} // namespace gears_weapon
