#pragma once
#include <cmath>
#include <cstdint>

namespace gears_weapon {
// The live adapter supplies the lever's angle around a pivot verified against
// the weapon animation. Controller/world poses alone are not a lever angle.
struct LeverTravel {
    double home_max_radians{},down_min_radians{},maximum_speed_radians_per_second{};
    std::int64_t minimum_cycle_ns{},maximum_cycle_ns{},maximum_sample_gap_ns{};
    bool valid() const noexcept {
        return std::isfinite(home_max_radians)&&std::isfinite(down_min_radians)&&
            std::isfinite(maximum_speed_radians_per_second)&&home_max_radians>=0&&
            down_min_radians>home_max_radians&&down_min_radians<=3.14&&
            maximum_speed_radians_per_second>0&&minimum_cycle_ns>0&&
            maximum_cycle_ns>minimum_cycle_ns&&maximum_sample_gap_ns>0&&
            maximum_sample_gap_ns<=maximum_cycle_ns;
    }
};
struct LeverPoseSample {
    std::uint64_t weapon{},world{},space_epoch{},sequence{};
    std::int64_t monotonic_ns{};
    double lever_angle_radians{};
    bool left_front_held{},right_handle_held{},tracked{};
};
enum class LeverPhase : std::uint8_t { unarmed, waiting_home, push_down, return_home };

class GnasherLever {
    LeverPhase phase_{LeverPhase::unarmed};
    std::uint64_t weapon_{},world_{},space_epoch_{},last_sequence_{};
    std::int64_t started_ns_{},last_ns_{};
    double last_angle_{};
    void restart(LeverPoseSample const& sample,LeverTravel const& travel) noexcept {
        phase_=sample.lever_angle_radians<=travel.home_max_radians?
            LeverPhase::push_down:LeverPhase::waiting_home;
        started_ns_=last_ns_=sample.monotonic_ns;last_sequence_=sample.sequence;
        last_angle_=sample.lever_angle_radians;
    }
public:
    bool arm(std::uint64_t weapon,std::uint64_t world,std::uint64_t space_epoch) noexcept {
        reset();if(!weapon||!world||!space_epoch)return false;
        weapon_=weapon;world_=world;space_epoch_=space_epoch;
        phase_=LeverPhase::waiting_home;return true;
    }
    void reset() noexcept {*this=GnasherLever{};}
    LeverPhase phase() const noexcept {return phase_;}
    bool observe(LeverPoseSample const& sample,LeverTravel const& travel) noexcept {
        if(phase_==LeverPhase::unarmed||!travel.valid())return false;
        if(sample.weapon!=weapon_||sample.world!=world_||sample.space_epoch!=space_epoch_||
           !sample.tracked||!sample.left_front_held||!sample.right_handle_held||
           !std::isfinite(sample.lever_angle_radians)||sample.lever_angle_radians<0||
           sample.lever_angle_radians>3.14||sample.monotonic_ns<=0){reset();return false;}
        if(!last_sequence_){restart(sample,travel);return false;}
        if(sample.sequence<=last_sequence_||sample.monotonic_ns<=last_ns_){reset();return false;}
        const auto gap=sample.monotonic_ns-last_ns_;
        if(gap>travel.maximum_sample_gap_ns||
           std::abs(sample.lever_angle_radians-last_angle_)>
               travel.maximum_speed_radians_per_second*(double(gap)/1e9)){
            restart(sample,travel);return false;
        }
        last_sequence_=sample.sequence;last_ns_=sample.monotonic_ns;
        last_angle_=sample.lever_angle_radians;
        if(sample.monotonic_ns-started_ns_>travel.maximum_cycle_ns){restart(sample,travel);return false;}
        if(phase_==LeverPhase::waiting_home){
            if(sample.lever_angle_radians<=travel.home_max_radians)restart(sample,travel);
        }else if(phase_==LeverPhase::push_down){
            if(sample.lever_angle_radians>=travel.down_min_radians)phase_=LeverPhase::return_home;
        }else if(phase_==LeverPhase::return_home&&
                 sample.lever_angle_radians<=travel.home_max_radians){
            if(sample.monotonic_ns-started_ns_>=travel.minimum_cycle_ns){reset();return true;}
            restart(sample,travel);
        }
        return false;
    }
};
}
