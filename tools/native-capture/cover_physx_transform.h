#pragma once
#include "full_body_c0_planner.h"
#include "cover_physx_matrix.h"
#include "cover_physx_units.h"
#include <cfloat>
#include <cstddef>
#include <cstdint>
#include <limits>

// Query geometry uses the original UE -> PhysX transform convention, not the
// user's OpenXR metres/game-unit calibration. Retained original F2610 stores
// the transposed3x3 and multiplies translation by float32 0x3ca3d70a loaded
// at actual1417070c4. Its caller F26D4 first normalizes the source axes.
// This adapter accepts only an already-rigid world matrix. It does not read
// game memory, call PhysX, identify a scene or certify collision/occupancy.
namespace gears_cover_physx_transform {
namespace body=gears_full_body_c0;
enum class Code {converted,alias,nonfinite,affine,nonrigid,numeric};
inline constexpr float game_to_physics=gears_cover_physx_units::game_to_physics;
namespace detail {
inline bool disjoint(void const* a,std::size_t an,void const* b,std::size_t bn) noexcept {
    const auto x=reinterpret_cast<std::uintptr_t>(a),y=reinterpret_cast<std::uintptr_t>(b);
    const auto max=(std::numeric_limits<std::uintptr_t>::max)();
    return an&&bn&&an-1<=max-x&&bn-1<=max-y&&(x+an-1<y||y+bn-1<x);
}
}
inline Code convert(body::Matrix64 const& in,Matrix34& output) noexcept {
    if(!detail::disjoint(&in,sizeof in,&output,sizeof output))return Code::alias;
    for(float value:in.values)if(!std::isfinite(value))return Code::nonfinite;
    const auto& m=in.values;
    if(m[3]!=0||m[7]!=0||m[11]!=0||m[15]!=1)return Code::affine;
    constexpr double tolerance=3e-5;
    for(unsigned r=0;r<3;++r)for(unsigned c=r;c<3;++c){
        double product=0;for(unsigned k=0;k<3;++k)product+=double(m[4*r+k])*m[4*c+k];
        if(std::abs(product-(r==c?1.:0.))>tolerance)return Code::nonrigid;
    }
    const double determinant=double(m[0])*(double(m[5])*m[10]-double(m[6])*m[9])-
        double(m[1])*(double(m[4])*m[10]-double(m[6])*m[8])+
        double(m[2])*(double(m[4])*m[9]-double(m[5])*m[8]);
    if(std::abs(determinant-1.)>tolerance)return Code::nonrigid;
    Matrix34 next{};
    for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)
        next.values[3*row+col]=m[4*col+row];
    for(unsigned axis=0;axis<3;++axis){
        const float value=m[12+axis]*game_to_physics;
        // Avoid depending on the caller's FTZ/DAZ state for subnormal inputs
        // or outputs. The actual original SSE rounding order stays one mulss.
        if(!std::isfinite(value)||(m[12+axis]!=0&&std::abs(m[12+axis])<FLT_MIN)||
           (value==0&&m[12+axis]!=0)||(value!=0&&std::abs(value)<FLT_MIN))return Code::numeric;
        next.values[9+axis]=value;
    }
    output=next;return Code::converted;
}
} // namespace gears_cover_physx_transform
