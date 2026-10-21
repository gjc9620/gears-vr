#pragma once
#include "weapon_pose_solver.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>

namespace gears_weapon {

// Positions are XR LOCAL metres. XrTime is used only for differences between
// frames from the same session/space epoch; it is never compared to QPC time.
struct GrenadeThrowConfig {
    static constexpr double mass_kg=3.0;
    double gravity_mps2{9.81};
    double minimum_speed_mps{.5};
    double maximum_speed_mps{25.0};
    std::int64_t history_ns{140000000};
    std::int64_t minimum_span_ns{55000000};
    std::int64_t maximum_sample_gap_ns{60000000};
    bool valid() const noexcept {
        return std::isfinite(gravity_mps2)&&gravity_mps2>0&&gravity_mps2<40&&
            std::isfinite(minimum_speed_mps)&&minimum_speed_mps>=0&&
            std::isfinite(maximum_speed_mps)&&maximum_speed_mps>minimum_speed_mps&&
            maximum_speed_mps<=40&&history_ns>=minimum_span_ns&&
            history_ns<=300000000&&minimum_span_ns>=20000000&&
            maximum_sample_gap_ns>0&&maximum_sample_gap_ns<=history_ns;
    }
};

struct GrenadeThrowSolution {
    Vector release_position{};
    Vector velocity_mps{};
    Vector impulse_kg_mps{}; // mass * velocity, if the game API needs impulse.
    double mass_kg{GrenadeThrowConfig::mass_kg};
    double gravity_mps2{};
    std::uint64_t weapon{},world{},session{},space_epoch{},reference_generation{};
    std::int64_t predicted_time{};
};

struct GameGrenadeLaunch {
    Vector position{},velocity{},impulse{},gravity{}; // UE world units.
    double mass_kg{};
};

// Convert the same tracked release pose and velocity with one frozen recenter
// reference. HMD orientation at release is deliberately absent. The live game
// adapter still owns projectile spawning, collision and the gravity setting.
inline bool grenade_world_launch(GrenadeThrowSolution const& tracked,
                                  Pose const& reference,
                                  gears_vr::Viewpoint const& game_base,
                                  double units_per_metre,GameGrenadeLaunch& out) noexcept {
    if(!tracked.weapon||!tracked.world||!tracked.session||!tracked.space_epoch||
       !tracked.reference_generation||tracked.predicted_time<=0||
       !gears_vr::stereo_pose_valid(reference)||
       !gears_vr::finite(tracked.release_position)||
       !gears_vr::finite(tracked.velocity_mps)||
       !gears_vr::finite(game_base.position)||
       !std::isfinite(game_base.rotation.pitch)||
       !std::isfinite(game_base.rotation.yaw)||
       !std::isfinite(game_base.rotation.roll)||
       !std::isfinite(units_per_metre)||units_per_metre<10||units_per_metre>500||
       !std::isfinite(tracked.gravity_mps2)||tracked.gravity_mps2<=0||
       tracked.mass_kg!=GrenadeThrowConfig::mass_kg)return false;
    const auto inverse=gears_vr::transpose(gears_vr::rotation_matrix(reference.orientation));
    const auto base=gears_vr::ue_matrix(game_base.rotation);
    const auto to_game=[&](Vector xr){return gears_vr::transform(base,
        gears_vr::xr_to_ue(gears_vr::transform(inverse,xr)));};
    GameGrenadeLaunch result{};
    result.position=sum(game_base.position,scaled(to_game(difference(
        tracked.release_position,reference.position)),units_per_metre));
    result.velocity=scaled(to_game(tracked.velocity_mps),units_per_metre);
    result.impulse=scaled(result.velocity,tracked.mass_kg);
    result.gravity={0,0,-tracked.gravity_mps2*units_per_metre};
    result.mass_kg=tracked.mass_kg;
    if(!gears_vr::finite(result.position)||!gears_vr::finite(result.velocity)||
       !gears_vr::finite(result.impulse)||!gears_vr::finite(result.gravity))return false;
    out=result;return true;
}

inline Vector grenade_ballistic_position(GrenadeThrowSolution const& throw_state,
                                         double elapsed_seconds) noexcept {
    if(!std::isfinite(elapsed_seconds)||elapsed_seconds<0)return throw_state.release_position;
    auto point=sum(throw_state.release_position,
                   scaled(throw_state.velocity_mps,elapsed_seconds));
    point.y-=.5*throw_state.gravity_mps2*elapsed_seconds*elapsed_seconds;
    return point;
}

class GrenadeThrowTracker {
    struct Sample {std::int64_t time{};Vector position{};};
    static constexpr unsigned capacity=24;
    std::array<Sample,capacity> samples_{};
    unsigned count_{};
    std::uint64_t weapon_{},world_{},reference_generation_{};
    std::uint64_t session_{},space_epoch_{},profile_generation_{},input_epoch_{},last_sequence_{};
    unsigned hand_{2};
    bool armed_{};

    void clear_samples() noexcept {count_=0;last_sequence_=0;}
    void append(Sample value,std::int64_t window) noexcept {
        while(count_&&value.time-samples_[0].time>window){
            for(unsigned i=1;i<count_;++i)samples_[i-1]=samples_[i];
            --count_;
        }
        if(count_==capacity){
            for(unsigned i=1;i<count_;++i)samples_[i-1]=samples_[i];
            --count_;
        }
        samples_[count_++]=value;
    }
    std::optional<GrenadeThrowSolution> solve(GrenadeThrowConfig const& config) const noexcept {
        if(count_<4||samples_[count_-1].time-samples_[0].time<config.minimum_span_ns)
            return std::nullopt;
        double sum_t{},sum_tt{};
        Vector sum_p{},sum_tp{};
        for(unsigned i=0;i<count_;++i){
            const double t=double(samples_[i].time-samples_[0].time)/1e9;
            sum_t+=t;sum_tt+=t*t;
            sum_p=sum(sum_p,samples_[i].position);
            sum_tp=sum(sum_tp,scaled(samples_[i].position,t));
        }
        const double n=double(count_),denominator=n*sum_tt-sum_t*sum_t;
        if(!std::isfinite(denominator)||denominator<1e-8)return std::nullopt;
        const auto velocity=scaled(difference(scaled(sum_tp,n),scaled(sum_p,sum_t)),
                                   1/denominator);
        const auto speed=length(velocity);
        if(!gears_vr::finite(velocity)||!std::isfinite(speed)||
           speed<config.minimum_speed_mps||speed>config.maximum_speed_mps||
           length(difference(samples_[count_-1].position,samples_[0].position))<.04)
            return std::nullopt;
        GrenadeThrowSolution result{};
        result.release_position=samples_[count_-1].position;
        result.velocity_mps=velocity;
        result.impulse_kg_mps=scaled(velocity,GrenadeThrowConfig::mass_kg);
        result.gravity_mps2=config.gravity_mps2;
        result.weapon=weapon_;result.world=world_;result.session=session_;
        result.space_epoch=space_epoch_;result.reference_generation=reference_generation_;
        result.predicted_time=samples_[count_-1].time;
        return result;
    }
public:
    void reset() noexcept {*this=GrenadeThrowTracker{};}
    bool begin(std::uint64_t weapon,std::uint64_t world,unsigned hand,
               std::uint64_t reference_generation) noexcept {
        reset();
        if(!weapon||!world||hand>1||!reference_generation)return false;
        weapon_=weapon;world_=world;hand_=hand;
        reference_generation_=reference_generation;armed_=true;return true;
    }
    bool armed() const noexcept {return armed_;}
    // The caller decides what gesture releases the grenade. A weapon-grip
    // toggle is not silently interpreted as a throw. Tracking loss or a
    // reference change cancels the attempt without spawning a projectile.
    std::optional<GrenadeThrowSolution> advance(
        gears_vr::ControllerInputFrame const& hands,gears_vr::StereoSample const& stereo,
        std::uint64_t weapon,std::uint64_t world,std::uint64_t reference_generation,
        std::int64_t now_ns,bool release,GrenadeThrowConfig const& config={},
        bool allow_sim=false) noexcept {
        if(!armed_||!config.valid()||weapon!=weapon_||world!=world_||
           reference_generation!=reference_generation_||
           !gears_vr::controller_frame_matches(hands,stereo,now_ns,allow_sim)||
           !gears_vr::controller_pose_usable(hands.hands[hand_].grip,
               hands.hands[hand_].grip_flags,hands.hands[hand_].grip_active)){
            reset();return std::nullopt;
        }
        if(!session_){
            session_=hands.session;space_epoch_=hands.space_epoch;
            profile_generation_=hands.profile_generation;input_epoch_=hands.input_epoch;
        }else if(hands.session!=session_||hands.space_epoch!=space_epoch_||
                 hands.profile_generation!=profile_generation_||hands.input_epoch!=input_epoch_){
            reset();return std::nullopt;
        }
        if(last_sequence_&&(hands.sequence<=last_sequence_||
           hands.predicted_time<=samples_[count_-1].time)){
            reset();return std::nullopt;
        }
        const Sample latest{hands.predicted_time,hands.hands[hand_].grip.position};
        if(count_){
            const auto gap=latest.time-samples_[count_-1].time;
            if(gap>config.maximum_sample_gap_ns||
               length(difference(latest.position,samples_[count_-1].position))>
                   config.maximum_speed_mps*(double(gap)/1e9))clear_samples();
        }
        append(latest,config.history_ns);last_sequence_=hands.sequence;
        if(!release)return std::nullopt;
        const auto result=solve(config);
        reset();return result;
    }
};
}
