#pragma once
#include "weapon_call_boundary.h"
#include "weapon_support_grip.h"
#include <cmath>
#include <limits>

// Optional typed evidence for a held-weapon HUD query. This is NOT the opaque
// V1 CallFunction State and does not change that ABI. A receiver must obtain
// expected_call/current_source from its own admitted transaction/publication.
namespace gears_weapon_hud_receipt {
struct Receipt {
    std::uint32_t size{sizeof(Receipt)},version{1};
    gears_weapon_call::Invocation call{};
    gears_weapon_hand::Frame source{};
    std::uint64_t root_frame{},hold_generation{};
    std::int64_t applied_ns{};
    std::array<float,3> start{},end{},direction{};
};
enum class Reason {accepted,shape,invocation,source,clock,geometry};
inline Reason validate(Receipt const& value,
    gears_weapon_call::Invocation const& expected_call,
    gears_weapon_hand::Frame const& current_source,std::int64_t now_ns) noexcept {
    namespace hand=gears_weapon_hand;
    if(value.size!=sizeof(Receipt)||value.version!=1||!value.root_frame||
       (value.root_frame&7)||!value.hold_generation)return Reason::shape;
    if(!gears_weapon_call::invocation_valid(expected_call)||
       !gears_weapon_call::same(value.call,expected_call)||
       value.root_frame!=expected_call.caller_frame||
       expected_call.self!=current_source.weapon.address)return Reason::invocation;
    if(!hand::frame_active(current_source,now_ns)||!hand::identity_valid(current_source.weapon)||
       !hand::same_frame(value.source,current_source)||
       expected_call.hand_cookie!=current_source.key.registration_cookie||
       expected_call.native_started_qpc!=current_source.key.native_started_qpc||
       expected_call.thread_id!=current_source.key.thread_id)return Reason::source;
    // No timestamp renewal: the publisher's original tracking stamps remain
    // inside source and frame_active checks their independent lifetimes.
    if(value.applied_ns<=0||value.applied_ns<value.source.now_ns||
       now_ns<value.applied_ns||now_ns-value.applied_ns>100000000)return Reason::clock;
    double norm{},length{};
    std::array<double,3> ray{};
    for(unsigned i=0;i<3;++i){
        if(!std::isfinite(value.start[i])||!std::isfinite(value.end[i])||
           !std::isfinite(value.direction[i])||std::abs(value.start[i])>1e8f||
           std::abs(value.end[i])>1e8f)return Reason::geometry;
        ray[i]=double(value.end[i])-value.start[i];
        length+=ray[i]*ray[i];norm+=double(value.direction[i])*value.direction[i];
    }
    length=std::sqrt(length);
    if(!std::isfinite(length)||length<1||length>1e6||std::abs(norm-1)>.001)
        return Reason::geometry;
    for(unsigned i=0;i<3;++i){
        // Quantization of large world coordinates affects the endpoint.
        const double tolerance=.001*length+8*std::numeric_limits<float>::epsilon()*
            (std::abs(double(value.start[i]))+std::abs(double(value.end[i])));
        if(std::abs(ray[i]-length*value.direction[i])>tolerance)return Reason::geometry;
    }
    return Reason::accepted;
}
static_assert(std::is_standard_layout_v<Receipt>&&std::is_trivially_copyable_v<Receipt>);
#if defined(_WIN32)
using Submit=bool(__cdecl*)(Receipt const*) noexcept;
#else
using Submit=bool(*)(Receipt const*) noexcept;
#endif
inline constexpr char submit_export[]="GearsVrSubmitWeaponHudQueryV1";
}
