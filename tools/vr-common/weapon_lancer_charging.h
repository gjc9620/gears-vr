#pragma once
#include "weapon_hammerburst_latch.h"

namespace gears_weapon {
// The original Lancer receiver is a hinged mechanism, not a certified linear
// bolt. All distances below use the existing gun-local metre convention.
// The adapter owns/authenticates the current weapon, original animation and
// inventory. This kernel cannot dispatch, invent tracking or fill ammunition.
class LancerChargingStroke {
public:
    enum class Phase {idle,pulling,waiting_open,returning,complete};
private:
    HammerburstLatchGeometry geometry_{};
    HammerburstLatchStroke hinge_;
    Phase phase_{};
    double wanted_{},observed_{},return_from_{},return_started_{};
    std::uint64_t last_bone_frame_{};
    unsigned closed_frames_{};
    bool actual_full_pull_{},return_can_finish_{};
public:
    void reset() noexcept {*this={};}
    Phase phase() const noexcept {return phase_;}
    double wanted_angle() const noexcept {return wanted_;}
    bool actual_full_pull() const noexcept {return actual_full_pull_;}
    HammerburstLatchRejection last_rejection() const noexcept {return hinge_.last_rejection();}
    bool begin(HammerburstLatchGeometry geometry,HammerburstLatchSample const& hand,
        double actual_angle) noexcept {
        if(phase_!=Phase::idle||!std::isfinite(actual_angle)||std::abs(actual_angle)>.025)return false;
        geometry.observation_interval_ms=250;
        if(!hinge_.begin(geometry,hand,0))return false;
        geometry_=geometry;phase_=Phase::pulling;wanted_=0;observed_=actual_angle;
        actual_full_pull_=return_can_finish_=false;closed_frames_=0;last_bone_frame_=0;return true;
    }
    bool move(HammerburstLatchSample const& hand) noexcept {
        if(phase_!=Phase::pulling)return false;
        if(hinge_.observe(hand)==HammerburstLatchMotion::rejected)return false;
        wanted_=hinge_.angle();return true;
    }
    bool release(double game_time) noexcept {
        if(phase_!=Phase::pulling||!std::isfinite(game_time)||game_time<0)return false;
        // A quick human release may precede the next original-bone update.
        // Keep only the already authenticated full hand travel; time alone
        // cannot supply the missing original mechanism observation.
        if(wanted_>=geometry_.open_angle-.025&&!actual_full_pull_){
            hinge_.reset();phase_=Phase::waiting_open;return true;
        }
        return_from_=observed_;return_started_=game_time;
        return_can_finish_=actual_full_pull_;closed_frames_=0;
        hinge_.reset();phase_=Phase::returning;return true;
    }
    // An invalid/interrupted grip returns the owned mechanism closed, but
    // can never inherit its earlier travel or finish an empty reload.
    bool cancel(double game_time) noexcept {
        if((phase_!=Phase::pulling&&phase_!=Phase::waiting_open)||
           !std::isfinite(game_time)||game_time<0)return false;
        return_from_=observed_;return_started_=game_time;
        return_can_finish_=actual_full_pull_=false;closed_frames_=0;
        hinge_.reset();phase_=Phase::returning;return true;
    }
    bool advance_return(double game_time) noexcept {
        if(phase_!=Phase::returning||!std::isfinite(game_time)||game_time<return_started_)return false;
        const double t=std::clamp((game_time-return_started_)/.15,0.,1.);
        wanted_=return_from_*(1.-t*t*(3.-2.*t));return true;
    }
    bool acknowledge_bone(double actual_angle,std::uint64_t current_frame,double game_time=0) noexcept {
        if(!std::isfinite(actual_angle)||actual_angle<-.03||actual_angle>geometry_.open_angle+.03||!current_frame)return false;
        if(!std::isfinite(game_time)||game_time<0)return false;
        if(current_frame<=last_bone_frame_)return false;
        last_bone_frame_=current_frame;observed_=actual_angle;
        if(phase_==Phase::pulling&&wanted_>=geometry_.open_angle-.025&&
           actual_angle>=geometry_.open_angle-.025)actual_full_pull_=true;
        if(phase_==Phase::waiting_open&&actual_angle>=geometry_.open_angle-.025){
            actual_full_pull_=return_can_finish_=true;return_from_=actual_angle;
            return_started_=game_time;closed_frames_=0;phase_=Phase::returning;
        }
        if(phase_==Phase::returning){
            if(wanted_<=.001&&std::abs(actual_angle)<=.02)++closed_frames_;else closed_frames_=0;
            if(closed_frames_>=2){
                phase_=return_can_finish_?Phase::complete:Phase::idle;
                wanted_=0;actual_full_pull_=false;
            }
        }
        return true;
    }
};
}
