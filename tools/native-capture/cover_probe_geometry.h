#pragma once
#include "cover_physx_query.h"
#include "cover_physx_units.h"
#include <float.h>
// Diagnostic geometry only. Input eye and calibration are in game units;
// every returned shape, delta and path epsilon is in native PhysX units.
// No body/head collider, world fence or movement permission is established.
namespace gears_cover_probe_geometry {
namespace q=gears_cover_physx_query;
enum class Code {prepared,alias,rounding,value};
struct Path {q::Capsule capsule{};q::Vector delta{};double epsilon{};};
inline Code sphere(q::Vector const& game_eye,double game_units_per_metre,q::Sphere& output)noexcept{
    if(q::source::aliases(&game_eye,sizeof game_eye,&output,sizeof output))return Code::alias;
    unsigned control{};if(_controlfp_s(&control,0,0)||(control&_MCW_RC)!=_RC_NEAR)return Code::rounding;
    if(!q::detail::finite(game_eye)||!std::isnormal(game_units_per_metre)||game_units_per_metre<=0)return Code::value;
    const float game_centimetre=float(game_units_per_metre*.01),factor=gears_cover_physx_units::game_to_physics;
    q::Sphere next{{game_eye.x*factor,game_eye.y*factor,game_eye.z*factor},game_centimetre*factor};
    if(!q::detail::scalar(game_centimetre)||game_centimetre<=0||!q::detail::valid(next))return Code::value;
    output=next;return Code::prepared;
}
inline Code path(q::Vector const& game_eye,double game_units_per_metre,Path& output)noexcept{
    if(q::source::aliases(&game_eye,sizeof game_eye,&output,sizeof output))return Code::alias;
    q::Sphere centre{};const auto code=sphere(game_eye,game_units_per_metre,centre);if(code!=Code::prepared)return code;
    const float game_centimetre=float(game_units_per_metre*.01),factor=gears_cover_physx_units::game_to_physics;
    Path next{};next.capsule={{centre.centre.x,centre.centre.y,(game_eye.z-game_centimetre)*factor},
      {centre.centre.x,centre.centre.y,(game_eye.z+game_centimetre)*factor},centre.radius};
    next.delta={centre.radius,0,0};next.epsilon=double(centre.radius)*.001;
    if(!q::detail::valid(next.capsule)||!q::detail::sweep_motion(next.capsule,next.delta)||!std::isnormal(next.epsilon))return Code::value;
    output=next;return Code::prepared;
}
}
