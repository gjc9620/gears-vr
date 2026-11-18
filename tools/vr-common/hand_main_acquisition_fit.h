#pragma once
#include "hand_wrist_calibration.h"
#include <cstdint>

// Cosmetic mount transition only. The Native C0 caller must supply its current
// physical Grip, authenticated free/held mounts and original admitted clock.
// Reset on a new real hold/context. No mesh/world pose, tracking or authority
// is retained here; both wrist and gun consume the same final C0 endpoint.
namespace gears_hand_main_acquisition {
namespace vr=gears_vr;namespace wrist=gears_hand_wrist;
struct Fit {
    std::int64_t started_ns{},last_ns{};double seconds{};
    void reset() noexcept {*this={};}
    bool arm(std::int64_t now,double duration) noexcept {
        if(now<=0||started_ns||last_ns||!std::isfinite(duration)||duration<0||duration>.5)return false;
        started_ns=last_ns=now;seconds=duration;return true;
    }
    bool sample(std::int64_t now,double duration,wrist::Calibration const& free,
        wrist::Calibration const& held,wrist::Calibration& out,double& weight) noexcept {
        if(now<=0||(bool(started_ns)!=bool(last_ns))||started_ns>last_ns||(last_ns&&now<last_ns)||!std::isfinite(duration)||duration<0||duration>.5||
           (started_ns&&duration!=seconds)||!free.known||!held.known||
           !vr::finite(free.grip_to_wrist_metres)||!vr::finite(held.grip_to_wrist_metres))return false;
        vr::Quaternion a{},b{};
        if(!wrist::detail::unit(free.grip_to_wrist_rotation,a)||
           !wrist::detail::unit(held.grip_to_wrist_rotation,b))return false;
        // These are the SAME authored palm marker under two wrist bases.
        // Refuse to invent a positional transition between unrelated mounts.
        const auto marker=vr::transform(vr::transpose(vr::rotation_matrix(a)),free.grip_to_wrist_metres);
        const auto other=vr::transform(vr::transpose(vr::rotation_matrix(b)),held.grip_to_wrist_metres);
        if(std::hypot(std::hypot(marker.x-other.x,marker.y-other.y),marker.z-other.z)>1e-7)return false;
        const auto begin=started_ns?started_ns:now;
        const double t=duration>0?std::clamp(double(now-begin)/(duration*1e9),0.,1.):1.;
        const double alpha=t*t*(3.-2.*t);
        wrist::Calibration candidate{};
        if(alpha>=1.)candidate=held;
        else if(alpha<=0.)candidate=free;
        else{
            if(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w<0)b={-b.x,-b.y,-b.z,-b.w};
            vr::Quaternion q{};
            if(!wrist::detail::unit({a.x+(b.x-a.x)*alpha,a.y+(b.y-a.y)*alpha,
                a.z+(b.z-a.z)*alpha,a.w+(b.w-a.w)*alpha},q))return false;
            // Rotate one marker with the blended basis. Linear interpolation
            // of two world-space wrist origins would detach the tracked palm.
            candidate={true,vr::transform(vr::rotation_matrix(q),marker),q};
        }
        started_ns=begin;last_ns=now;seconds=duration;out=candidate;weight=alpha;return true;
    }
};
}
