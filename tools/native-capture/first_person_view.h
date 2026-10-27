#pragma once
#include "../vr-common/view_origin.h"
#include <algorithm>
#include <cstdint>

// A render-only preview. Never changes a Pawn, firing ray, animation controller
// or the raw HMD pose. The game still owns movement, cover and weapon recoil.
namespace gears_first_person {
class Toggle {
    bool requested_{},armed_{},held_{};
    std::uint64_t since_{};
public:
    bool requested() const {return requested_;}
    void initialize(bool requested){reset();requested_=requested;}
    void reset(){*this=Toggle{};}
    // Both devices share one edge. Focus/context loss requires a neutral sample
    // before accepting a chord, preventing held keys from toggling on return.
    bool update(bool eligible,bool chord,bool neutral,std::uint64_t milliseconds,bool external=false){
        if(!eligible){armed_=held_=false;since_=0;return false;}
        if(external){requested_=!requested_;armed_=false;held_=false;since_=0;return true;}
        if(neutral){armed_=true;held_=false;since_=0;return false;}
        if(!armed_||!chord){held_=false;since_=0;return false;}
        if(!held_){held_=true;since_=milliseconds;return false;}
        if(milliseconds<since_||milliseconds-since_<80)return false;
        requested_=!requested_;armed_=false;held_=false;since_=0;return true;
    }
};
struct Result {gears_vr::StereoMatrix view{};gears_vr::Vector origin{};};
enum class CameraMode {unchanged,preview,native_scope};
inline CameraMode camera_mode(bool requested,bool ordinary,bool weapon_known,bool sniper,bool zoomed){
    if(!requested||!ordinary||!weapon_known)return CameraMode::unchanged;
    return sniper&&zoomed?CameraMode::native_scope:CameraMode::preview;
}
inline bool prepare(gears_vr::StereoMatrix const& original,gears_vr::Vector pawn_eye,
                    double units_per_metre,Result& out,bool body_basis=false){
    using namespace gears_vr;
    if(!stereo_rigid(original)||!finite(pawn_eye)||!std::isfinite(units_per_metre)||
       units_per_metre<10||units_per_metre>500)return false;
    Vector forward{original[2],original[6],original[10]};
    const double horizontal=std::hypot(forward.x,forward.y);
    if(!std::isfinite(horizontal)||horizontal<.05)return false;
    Vector right{-forward.y/horizontal,forward.x/horizontal,0};
    Vector up{forward.y*right.z-forward.z*right.y,
              forward.z*right.x-forward.x*right.z,
              forward.x*right.y-forward.y*right.x};
    // Keep native view forward (including gameplay aim/recoil), remove camera
    // roll and positional camera animation. Pitch never raises this anchor.
    StereoMatrix view{float(right.x),float(up.x),float(forward.x),0,
                      float(right.y),float(up.y),float(forward.y),0,
                      float(right.z),float(up.z),float(forward.z),0,0,0,0,1};
    const double forward_offset=body_basis?0.:.35,up_offset=body_basis?0.:.10;
    Vector eye{pawn_eye.x+forward.x/horizontal*forward_offset*units_per_metre,
               pawn_eye.y+forward.y/horizontal*forward_offset*units_per_metre,
               pawn_eye.z+up_offset*units_per_metre};
    if(!view_with_origin(view,eye,view))return false;
    out={view,eye};return true;
}
// Clip close geometry without translating the eye or hiding the full body.
// The original far plane and depth direction are retained. Applied only to
// the manual body profile's first-person projection; ordinary views retain
// their exact source matrix. This clips all geometry inside the near plane.
inline bool near_clip(gears_vr::StereoMatrix& p,double units,double metres){
    if(!gears_vr::stereo_projection_supported(p)||!std::isfinite(units)||units<10||units>500||
       !std::isfinite(metres)||metres<.01||metres>.2)return false;
    const double a=p[10],b=p[14],n=units*metres;
    const bool reverse=b>0;
    const double far_den=reverse?a:a-1.;
    double next_a{},next_b{};
    // UE's ordinary infinite-far projection keeps a precision margin:
    // A=1-epsilon, B=-near*A. Its depth=1 root is behind the eye, not
    // a physical far plane. Keep A (and that margin) when moving near.
    if(!reverse&&a>0&&a<1){next_a=a;next_b=-n*a;}
    else if(std::abs(far_den)<1e-8){next_a=reverse?0.:1.;next_b=reverse?n:-n;}
    else{
        const double far_distance=-b/far_den;
        if(!std::isfinite(far_distance)||far_distance<=n)return false;
        next_a=reverse?n/(n-far_distance):far_distance/(far_distance-n);
        next_b=reverse?n*far_distance/(far_distance-n):-n*far_distance/(far_distance-n);
    }
    if(!std::isfinite(next_a)||!std::isfinite(next_b))return false;
    p[10]=float(next_a);p[14]=float(next_b);return true;
}
}
