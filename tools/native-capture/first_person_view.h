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
                    double units_per_metre,Result& out){
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
    Vector eye{pawn_eye.x+forward.x/horizontal*.35*units_per_metre,
               pawn_eye.y+forward.y/horizontal*.35*units_per_metre,
               pawn_eye.z+.10*units_per_metre};
    if(!view_with_origin(view,eye,view))return false;
    out={view,eye};return true;
}
}
