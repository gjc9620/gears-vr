#pragma once
#include "rig_tracking_math.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace gears_weapon {
// All coordinates are gun-local METRES. The live adapter must compose the
// actual parent bones and authenticate its original Hammerburst before use.
struct HammerburstLatchGeometry {
    gears_vr::Vector pivot{},axis{},closed_contact{};
    double open_angle{},contact_radius{.035},radial_tolerance{.035},axial_tolerance{.03};
    bool current_instance_verified{};
    // Observation spacing only, never live input/lease freshness. Lancer's
    // committed-pose consumer may miss one13Hz publication during ownership
    // changes; default Hammerburst behavior retains its100ms bound.
    std::uint64_t observation_interval_ms{100};
};
struct HammerburstLatchSample {
    gears_vr::Vector hand{};
    std::uint64_t sequence{},milliseconds{},grip_token{};
    bool tracked{},gripped{};
};
enum class HammerburstLatchMotion {rejected,moving,closed,open,held_stationary};
enum class HammerburstLatchRejection {none,inactive,sample,token,order,gap,radius,axis,speed,range};
class HammerburstLatchStroke {
    using V=gears_vr::Vector;
    HammerburstLatchGeometry geometry_{};
    HammerburstLatchSample last_{};
    V radial_{};double radius_{},height_{},angle_{};bool active_{};
    HammerburstLatchRejection rejection_{};
    static V sub(V a,V b) noexcept {return {a.x-b.x,a.y-b.y,a.z-b.z};}
    static double dot(V a,V b) noexcept {return a.x*b.x+a.y*b.y+a.z*b.z;}
    static V cross(V a,V b) noexcept {return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
    static double length(V a) noexcept {return std::sqrt(dot(a,a));}
    static V scale(V a,double s) noexcept {return {a.x*s,a.y*s,a.z*s};}
    static V add(V a,V b) noexcept {return {a.x+b.x,a.y+b.y,a.z+b.z};}
    bool valid(HammerburstLatchSample const& s) const noexcept {
        return s.tracked&&s.gripped&&s.sequence&&s.milliseconds&&s.grip_token&&gears_vr::finite(s.hand);
    }
public:
    static constexpr double stationary_position_epsilon_metres=1e-5;
    void reset() noexcept {active_=false;}
    bool active() const noexcept {return active_;}
    double angle() const noexcept {return angle_;}
    HammerburstLatchRejection last_rejection() const noexcept {return rejection_;}
    // Diagnostic value only; it grants no continuity or permission.
    std::uint64_t last_observation_ms() const noexcept {return last_.milliseconds;}
    gears_vr::Vector last_hand_in_weapon() const noexcept {return last_.hand;}
    bool begin(HammerburstLatchGeometry const& g,HammerburstLatchSample const& s,double actual_angle) noexcept {
        reset();
        if(!g.current_instance_verified||!valid(s)||!gears_vr::finite(g.pivot)||
           !gears_vr::finite(g.axis)||!gears_vr::finite(g.closed_contact)||
           std::abs(length(g.axis)-1)>1e-4||!std::isfinite(g.open_angle)||g.open_angle<.05||g.open_angle>3.14||
           !std::isfinite(actual_angle)||actual_angle<0||actual_angle>g.open_angle||
           !std::isfinite(g.contact_radius)||g.contact_radius<=0||g.contact_radius>.1||
           !std::isfinite(g.radial_tolerance)||g.radial_tolerance<=0||g.radial_tolerance>.1||
           !std::isfinite(g.axial_tolerance)||g.axial_tolerance<=0||g.axial_tolerance>.1||
           !g.observation_interval_ms||g.observation_interval_ms>250)return false;
        const auto contact=sub(g.closed_contact,g.pivot);
        const auto axial=scale(g.axis,dot(contact,g.axis)),radial=sub(contact,axial);
        if(length(radial)<.01)return false;
        const auto rotated=add(axial,add(scale(radial,std::cos(actual_angle)),scale(cross(g.axis,radial),std::sin(actual_angle))));
        if(length(sub(sub(s.hand,g.pivot),rotated))>g.contact_radius)return false;
        const auto hand=sub(s.hand,g.pivot);height_=dot(hand,g.axis);
        radial_=sub(hand,scale(g.axis,height_));radius_=length(radial_);
        if(radius_<.01)return false;
        geometry_=g;last_=s;angle_=actual_angle;active_=true;return true;
    }
    HammerburstLatchMotion observe(HammerburstLatchSample const& s,bool retain_stationary=false) noexcept {
        const auto reject=[&](HammerburstLatchRejection why){rejection_=why;reset();return HammerburstLatchMotion::rejected;};
        rejection_=HammerburstLatchRejection::none;
        if(!active_)return reject(HammerburstLatchRejection::inactive);
        if(!valid(s))return reject(HammerburstLatchRejection::sample);
        if(s.grip_token!=last_.grip_token)return reject(HammerburstLatchRejection::token);
        if(s.sequence<=last_.sequence||s.milliseconds<=last_.milliseconds)return reject(HammerburstLatchRejection::order);
        if(s.milliseconds-last_.milliseconds>geometry_.observation_interval_ms){
            // Opt-in callers must still own the SAME current live grab.
            // Numerical contact noise may renew a stationary baseline, but
            // no movement, angle or endpoint is accepted across the gap.
            if(retain_stationary&&length(sub(s.hand,last_.hand))<=stationary_position_epsilon_metres){
                const auto geometry=geometry_;const auto retained_angle=angle_;
                if(begin(geometry,s,retained_angle))return HammerburstLatchMotion::held_stationary;
            }
            return reject(HammerburstLatchRejection::gap);
        }
        const auto hand=sub(s.hand,geometry_.pivot);const double height=dot(hand,geometry_.axis);
        const auto radial=sub(hand,scale(geometry_.axis,height));const double radius=length(radial);
        if(radius<.01||std::abs(radius-radius_)>geometry_.radial_tolerance)return reject(HammerburstLatchRejection::radius);
        if(std::abs(height-height_)>geometry_.axial_tolerance)return reject(HammerburstLatchRejection::axis);
        const double delta=std::atan2(dot(geometry_.axis,cross(radial_,radial)),dot(radial_,radial));
        if(std::abs(delta)>12.*double(s.milliseconds-last_.milliseconds)/1000.)return reject(HammerburstLatchRejection::speed);
        const double next=angle_+delta;
        if(!std::isfinite(next)||next<-.05||next>geometry_.open_angle+.05)return reject(HammerburstLatchRejection::range);
        last_=s;radial_=radial;angle_=std::clamp(next,0.,geometry_.open_angle);
        if(angle_<=.02)return HammerburstLatchMotion::closed;
        if(angle_>=geometry_.open_angle-.02)return HammerburstLatchMotion::open;
        return HammerburstLatchMotion::moving;
    }
};
} // namespace gears_weapon
