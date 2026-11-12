#pragma once
#include "cover_view.h"

namespace gears_vr {
// Extract only world heading from an authenticated pre-HMD Scene base. This
// never retains its eye, head pose, frame, timestamp or write receipt.
inline bool cover_level_heading(StereoMatrix const& m,Vector eye,CoverViewAnchor& out){
    for(auto value:m)if(!std::isfinite(value))return false;
    if(std::abs(m[3])>1e-5||std::abs(m[7])>1e-5||std::abs(m[11])>1e-5||std::abs(m[15]-1)>1e-5)return false;
    const Vector r{m[0],m[4],m[8]},u{m[1],m[5],m[9]},f{m[2],m[6],m[10]};
    if(std::abs(cover_dot(r,r)-1)>1e-5||std::abs(cover_dot(u,u)-1)>1e-5||std::abs(cover_dot(f,f)-1)>1e-5||
       std::abs(cover_dot(r,u))>1e-5||std::abs(cover_dot(r,f))>1e-5||std::abs(cover_dot(u,f))>1e-5)return false;
    const Vector cross{r.y*u.z-r.z*u.y,r.z*u.x-r.x*u.z,r.x*u.y-r.y*u.x};
    if(cover_dot(cross,f)<1-1e-5)return false;
    const double horizontal=std::hypot(f.x,f.y);if(horizontal<1e-4)return false;
    CoverViewAnchor candidate{eye,{f.x/horizontal,f.y/horizontal,0},{-f.y/horizontal,f.x/horizontal,0}};
    if(!cover_view_basis(candidate)||cover_dot(candidate.right_world,r)<1e-4)return false;
    out=candidate;return true;
}
}
