#pragma once
#include <algorithm>
#include <cmath>

// Raw left-stick axes, in the original PlayerInput movement basis. No HMD
// rotation, actor position, movement state, key event or elapsed time is made up.
namespace gears_controller_locomotion {
struct Axes {float right{},forward{};bool active{};};
// Yaw only. Vertical look remains physical HMD tracking, not a tilted camera.
// Return a normalized stick demand. Do not put it directly into aTurn: retail
// PlayerInput applies legacy axis scaling and a persistent turn-deceleration
// history there. The caller converts demand to a per-tick yaw delta instead.
inline float turn(float x,bool active,float deadzone) noexcept {
    if(!active||!std::isfinite(x)||!std::isfinite(deadzone)||deadzone<0||deadzone>.5f||
       std::abs(x)>1||std::abs(x)<=deadzone)return 0;
    return std::copysign((std::abs(x)-deadzone)/(1-deadzone),x);
}
inline float yaw_delta(float demand,float seconds) noexcept {
    if(!std::isfinite(demand)||std::abs(demand)>1||!std::isfinite(seconds)||seconds<=0||seconds>.25f)return 0;
    // Smooth turn: full deflection is 90 degrees/second, in UE rotator units.
    return demand*seconds*(65536.f/4.f);
}
inline Axes axes(float x,float y,bool active) noexcept {
    if(!active||!std::isfinite(x)||!std::isfinite(y)||std::abs(x)>1||std::abs(y)>1)return {};
    constexpr double deadzone=.18;
    const double length=std::hypot(double(x),double(y));
    if(length<=deadzone)return {};
    const double amount=((std::min)(length,1.)-deadzone)/(1.-deadzone);
    return {float(x/length*amount),float(y/length*amount),true};
}
}
