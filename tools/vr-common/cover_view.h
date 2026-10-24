#pragma once
#include "cover_interaction.h"
#include "view_origin.h"

namespace gears_vr {

// An authenticated, level cover basis. Forward points FROM the player INTO the
// cover. The owner supplies a neutral eye calibrated without virtual dragging
// or tracked-head displacement. A current animated head position is unsuitable:
// it already includes the pose response which this camera is about to produce.
struct CoverViewAnchor {
    Vector neutral_eye_world;
    Vector forward_world;
    Vector right_world;
};
struct CoverViewResult {
    StereoMatrix base_view{};
    Vector base_eye_world;
};
inline double cover_dot(Vector a,Vector b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline bool cover_view_basis(CoverViewAnchor const& a){
    constexpr double epsilon=1e-5;
    return finite(a.neutral_eye_world)&&finite(a.forward_world)&&finite(a.right_world)&&
        std::abs(a.forward_world.z)<epsilon&&std::abs(a.right_world.z)<epsilon&&
        std::abs(cover_dot(a.forward_world,a.forward_world)-1)<epsilon&&
        std::abs(cover_dot(a.right_world,a.right_world)-1)<epsilon&&
        std::abs(cover_dot(a.forward_world,a.right_world))<epsilon&&
        std::abs(a.right_world.x+a.forward_world.y)<epsilon&&
        std::abs(a.right_world.y-a.forward_world.x)<epsilon;
}

// Build the BASE camera only. stereo_matrices subsequently applies the frozen
// raw HMD and both eye poses exactly once. No native cover-camera displacement,
// Pawn.EyeHeight, preview forward/up lift, or current animated head is added.
// Failure leaves out unchanged. This does not certify the cover surface/reach.
inline bool prepare_cover_view(CoverViewAnchor const& anchor,CoverOffset virtual_offset,
                               double units_per_metre,CoverViewResult& out){
    if(!cover_view_basis(anchor)||!std::isfinite(virtual_offset.right)||!std::isfinite(virtual_offset.up)||
       !std::isfinite(units_per_metre)||units_per_metre<10||units_per_metre>500)return false;
    const auto& f=anchor.forward_world;const auto& r=anchor.right_world;
    StereoMatrix view{float(r.x),0,float(f.x),0,
                      float(r.y),0,float(f.y),0,
                      0,1,0,0,0,0,0,1};
    Vector eye{anchor.neutral_eye_world.x+r.x*virtual_offset.right*units_per_metre,
               anchor.neutral_eye_world.y+r.y*virtual_offset.right*units_per_metre,
               anchor.neutral_eye_world.z+virtual_offset.up*units_per_metre};
    if(!view_with_origin(view,eye,view))return false;
    out={view,eye};return true;
}

struct CoverPhysicalDisplacement {double right=0,up=0,forward=0;};
// Body/exposure follows the SAME head-position delta that stereo_matrices uses.
// The result is in metres in the fixed reference camera's right/up/forward
// basis. Rotation is deliberately absent from this calculation; natural head
// motion accompanying a turn is still retained in the measured positions.
// Do not add this displacement to prepare_cover_view's virtual_offset.
inline bool cover_physical_displacement(Pose const& reference,Pose const& head,
                                       CoverPhysicalDisplacement& out){
    if(!stereo_pose_valid(reference)||!stereo_pose_valid(head))return false;
    const auto delta=transform(transpose(rotation_matrix(reference.orientation)),
        {head.position.x-reference.position.x,head.position.y-reference.position.y,head.position.z-reference.position.z});
    if(!finite(delta))return false;
    out={delta.x,delta.y,-delta.z};return true;
}

// A measured monotonic neutral-to-top curve, in metres ABOVE its neutral head.
// These are caller-owned measurements, not assumed native game constants.
// The bounded driver can saturate its animation while real HMD motion remains
// unrestricted in the separate camera path.
struct CoverHeightKey {double u=0,height_metres=0;};
inline bool cover_up_weight(CoverHeightKey const* keys,unsigned count,double desired_height_metres,double& out){
    if(!keys||count<2||count>32||!std::isfinite(desired_height_metres)||
       keys[0].u!=0||keys[0].height_metres!=0||keys[count-1].u!=1)return false;
    for(unsigned i=0;i<count;++i){
        if(!std::isfinite(keys[i].u)||!std::isfinite(keys[i].height_metres))return false;
        if(i&&(keys[i].u<=keys[i-1].u||keys[i].height_metres<=keys[i-1].height_metres))return false;
    }
    if(desired_height_metres<=0){out=0;return true;}
    if(desired_height_metres>=keys[count-1].height_metres){out=1;return true;}
    for(unsigned i=1;i<count;++i)if(desired_height_metres<=keys[i].height_metres){
        auto const& lo=keys[i-1];auto const& hi=keys[i];
        out=lo.u+(hi.u-lo.u)*(desired_height_metres-lo.height_metres)/(hi.height_metres-lo.height_metres);
        return true;
    }
    return false;
}
} // namespace gears_vr
