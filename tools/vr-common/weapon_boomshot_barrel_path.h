#pragma once
#include "weapon_boomshot_types.h"
#include "pose_math.h"

namespace gears_weapon::boomshot_barrel {
// MP_WarMachine export23ffe2a1..., reference mesh a3b70678.... Stored local
// translation only. One rotation key, so this is not a hinge. Engine animation
// time/convention and actual contact still require current-instance evidence.
inline constexpr double travel_raw=8.970882415771484;
inline constexpr gears_vr::Vector closed_raw{0,56.06922149658203,9.812527656555176};
enum class Direction {opening,closing};
enum class Result {rejected,waiting,moved,complete,interrupted};
struct Sample {
    BoomshotStamp stamp{};
    // Both in the original, unsteered gun root, divided by component scale.
    // hand is RAW controller input, never a fitted C0/skin hand endpoint.
    gears_vr::Vector hand_m{};
    std::uint64_t frame{},lease{},press{},operation{};
    double actual_progress{},position_error_m{},rotation_error_degrees{};
    // The live adapter authenticates a current grant or its exact original
    // Native receipt after bounded preparation; it never changes Decision.
    bool grip_held{},grant_proven{},bone_measured{},left_tracked{},right_tracked{},lease_current{};
};
inline bool progress(gears_vr::Vector barrel_raw,double& out) noexcept {
    if(!gears_vr::finite(barrel_raw)||std::abs(barrel_raw.x-closed_raw.x)>.002||
       std::abs(barrel_raw.z-closed_raw.z)>.002)return false;
    const double q=(closed_raw.y-barrel_raw.y)/travel_raw;
    if(q<-.002||q>1.002)return false;
    out=(std::max)(0.,(std::min)(1.,q));return true;
}
// Original sample-index mapping. This is NOT permission to infer completion
// from animation time: completion below requires later measured bone receipt.
inline bool original_key(double q,Direction direction,double& out) noexcept {
    if(!std::isfinite(q)||q<0||q>1)return false;
    constexpr double open_mid=.3192464226379375;
    if(direction==Direction::opening){out=q<=open_mid?66+q/open_mid:67+(q-open_mid)/(1-open_mid);return true;}
    constexpr double a=.8931853756256742,b=.5641480093023533;
    out=q>=a?72+(1-q)/(1-a):q>=b?73+(a-q)/(a-b):74+(b-q)/b;return true;
}
// Minted only after raw physical travel AND a later independent bone ACK.
// These are original input samples, not core owner-event sequence numbers.
class Completion {
    friend class Stroke;
    Sample begin_{},end_{};Direction direction_{};bool known_{};
public:
    bool valid()const noexcept{return known_;}
    Sample const& began()const noexcept{return begin_;}
    Sample const& ended()const noexcept{return end_;}
    Direction direction()const noexcept{return direction_;}
};
class Stroke {
    Sample began_{};Completion completion_{};
    BoomshotStamp source_{};
    gears_vr::Vector begin_hand_{};
    std::uint64_t frame_{},issued_frame_{},lease_{},press_{},operation_{};
    double travel_{},start_{},wanted_{};
    Direction direction_{};
    bool active_{};
    static bool bone(Sample const& s) noexcept {
        return s.bone_measured&&std::isfinite(s.actual_progress)&&s.actual_progress>=0&&s.actual_progress<=1&&
            std::isfinite(s.position_error_m)&&s.position_error_m>=0&&s.position_error_m<=.002&&
            std::isfinite(s.rotation_error_degrees)&&s.rotation_error_degrees>=0&&s.rotation_error_degrees<=2;
    }
    static bool usable(Sample const& s,std::int64_t now) noexcept {
        return s.stamp.valid()&&now>=s.stamp.monotonic_ns&&now-s.stamp.monotonic_ns<=100000000&&
            s.frame&&s.lease&&s.press&&s.operation&&s.grip_held&&s.left_tracked&&s.right_tracked&&
            s.lease_current&&gears_vr::finite(s.hand_m);
    }
public:
    bool begin(Sample const& s,Direction direction,double units_per_metre,std::int64_t now) noexcept {
        if(active_||(direction!=Direction::opening&&direction!=Direction::closing)||
           !usable(s,now)||!s.grant_proven||!bone(s)||!std::isfinite(units_per_metre)||
           units_per_metre<10||units_per_metre>500)return false;
        began_=s;completion_={};source_=s.stamp;begin_hand_=s.hand_m;frame_=issued_frame_=s.frame;
        lease_=s.lease;press_=s.press;operation_=s.operation;
        travel_=travel_raw/units_per_metre;start_=wanted_=s.actual_progress;
        direction_=direction;active_=true;return true;
    }
    Result observe(Sample const& s,std::int64_t now) noexcept {
        if(!active_)return Result::rejected;
        if(!usable(s,now)||!(s.stamp.identity==source_.identity)||s.stamp.binding!=source_.binding||
           s.stamp.motion_epoch!=source_.motion_epoch||s.lease!=lease_||s.press!=press_||s.operation!=operation_||
           s.stamp.monotonic_ns<source_.monotonic_ns||s.stamp.monotonic_ns-source_.monotonic_ns>100000000||
           s.stamp.game_tick<source_.game_tick){active_=false;return Result::interrupted;}
        if(s.stamp.sequence<=source_.sequence||s.frame<=frame_)return Result::waiting;
        const double dt=double(s.stamp.monotonic_ns-source_.monotonic_ns)/1e9;
        const auto delta=gears_vr::Vector{s.hand_m.x-begin_hand_.x,s.hand_m.y-begin_hand_.y,s.hand_m.z-begin_hand_.z};
        const double q=start_-delta.y/travel_;
        if(std::hypot(delta.x,delta.z)>.018||q<-.02||q>1.02||
           std::abs(q-wanted_)*travel_>1.5*dt+.002){active_=false;return Result::interrupted;}
        source_=s.stamp;frame_=s.frame;
        const double next=(std::max)(0.,(std::min)(1.,q));
        if(std::abs(next-wanted_)>.0001){wanted_=next;issued_frame_=s.frame;return Result::moved;}
        const double endpoint=direction_==Direction::opening?1.:0.;
        if(issued_frame_>began_.frame&&std::abs(wanted_-endpoint)<=.015&&s.frame>issued_frame_&&bone(s)&&
           std::abs(s.actual_progress-endpoint)<=.015){
            completion_.begin_=began_;completion_.end_=s;completion_.direction_=direction_;completion_.known_=true;
            active_=false;return Result::complete;
        }
        return Result::waiting;
    }
    bool take_completion(Completion& out) noexcept {
        out={};if(!completion_.known_)return false;out=completion_;completion_={};return true;
    }
    void release() noexcept {active_=false;completion_={};}
    bool active() const noexcept {return active_;}
    double wanted() const noexcept {return wanted_;}
};
}
