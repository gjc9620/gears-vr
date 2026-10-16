#pragma once
#include "../vr-common/stereo_frame.h"
#include <algorithm>

// Experimental render-camera adjustment. The game's camera collision and shot
// queries remain authoritative. Never derives its direction from the HMD.
namespace gears_camera_closer {
inline constexpr double maximum_metres=.5, clearance_metres=.4;
inline bool valid(double metres) {return std::isfinite(metres)&&metres>=0&&metres<=maximum_metres;}
struct Result {gears_vr::StereoMatrix view{};double applied_units{};};
inline bool prepare(gears_vr::StereoMatrix const& source,double metres,double units_per_metre,
    bool gameplay,gears_vr::Vector pawn_eye,Result& out) {
    if(!valid(metres)||!std::isfinite(units_per_metre)||units_per_metre<10||units_per_metre>500)return false;
    Result candidate{source,0};
    if(metres==0||!gameplay){out=candidate;return true;}
    if(!gears_vr::stereo_rigid(source)||!gears_vr::finite(pawn_eye))return false;
    // Native row-vector view uses +Z forward. Moving its origin forward changes
    // only camera-space Z translation; this precedes the frozen per-eye poses.
    const double depth=pawn_eye.x*source[2]+pawn_eye.y*source[6]+pawn_eye.z*source[10]+source[14];
    if(!std::isfinite(depth))return false;
    const double amount=(std::min)(metres*units_per_metre,(std::max)(0.,depth-clearance_metres*units_per_metre));
    const double translated=double(source[14])-amount;
    candidate.view[14]=static_cast<float>(translated);
    if(!std::isfinite(candidate.view[14])||std::abs(double(candidate.view[14])-translated)>.02)return false;
    candidate.applied_units=double(source[14])-candidate.view[14];
    if(candidate.applied_units<0||candidate.applied_units>amount+.02)return false;
    out=candidate;return true;
}
}
