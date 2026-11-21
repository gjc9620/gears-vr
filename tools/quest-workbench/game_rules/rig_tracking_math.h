#pragma once
#include "view_origin.h"

namespace gears_vr::rig_tracking {
// base_view is the final rendered BASE before HMD/eye composition. Its origin
// already includes committed virtual movement. Never add the head again here.
// These are coordinate transforms only: no grip-to-hand-bone calibration,
// anatomical alignment, authoritative aim line or tracking-ticket validation.
inline Vector map_vector(StereoMatrix const& view,Vector v) {
    return {double(view[0])*v.x+double(view[1])*v.y-double(view[2])*v.z,
            double(view[4])*v.x+double(view[5])*v.y-double(view[6])*v.z,
            double(view[8])*v.x+double(view[9])*v.y-double(view[10])*v.z};
}

inline bool point(StereoMatrix const& base_view,Pose const& reference,Vector raw,
                  double units_per_metre,Vector& out_world) {
    // Retain the renderer/cover helper's admitted scale and rigid-pose policy.
    if(!stereo_rigid(base_view)||!stereo_pose_valid(reference)||!finite(raw)||
       !std::isfinite(units_per_metre)||units_per_metre<10||units_per_metre>500)return false;
    const auto local=transform(transpose(rotation_matrix(reference.orientation)),
        {raw.x-reference.position.x,raw.y-reference.position.y,raw.z-reference.position.z});
    const auto origin=stereo_camera_position(base_view);
    const auto delta=map_vector(base_view,local);
    const Vector candidate{origin.x+units_per_metre*delta.x,origin.y+units_per_metre*delta.y,
                           origin.z+units_per_metre*delta.z};
    if(!finite(candidate))return false;
    out_world=candidate;return true;
}

namespace detail {
// Matrix columns are proper UE world forward/right/up, not XR right/up/back.
inline bool quaternion(Matrix const& m,Quaternion& out) {
    Quaternion q{};double s{};const double trace=m[0][0]+m[1][1]+m[2][2];
    if(trace>0){
        s=2*std::sqrt(trace+1);if(!std::isfinite(s)||s<1e-8)return false;
        q={(m[2][1]-m[1][2])/s,(m[0][2]-m[2][0])/s,(m[1][0]-m[0][1])/s,s/4};
    }else if(m[0][0]>=m[1][1]&&m[0][0]>=m[2][2]){
        s=2*std::sqrt(1+m[0][0]-m[1][1]-m[2][2]);if(!std::isfinite(s)||s<1e-8)return false;
        q={s/4,(m[0][1]+m[1][0])/s,(m[0][2]+m[2][0])/s,(m[2][1]-m[1][2])/s};
    }else if(m[1][1]>=m[2][2]){
        s=2*std::sqrt(1+m[1][1]-m[0][0]-m[2][2]);if(!std::isfinite(s)||s<1e-8)return false;
        q={(m[0][1]+m[1][0])/s,s/4,(m[1][2]+m[2][1])/s,(m[0][2]-m[2][0])/s};
    }else{
        s=2*std::sqrt(1+m[2][2]-m[0][0]-m[1][1]);if(!std::isfinite(s)||s<1e-8)return false;
        q={(m[0][2]+m[2][0])/s,(m[1][2]+m[2][1])/s,s/4,(m[1][0]-m[0][1])/s};
    }
    const double norm=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);
    if(!std::isfinite(norm)||norm<1e-8)return false;
    out={q.x/norm,q.y/norm,q.z/norm,q.w/norm};return true;
}
} // namespace detail

inline bool pose(StereoMatrix const& base_view,Pose const& reference,Pose const& raw,
                 double units_per_metre,Pose& out_world) {
    if(!stereo_pose_valid(raw))return false;
    Pose candidate{};
    if(!point(base_view,reference,raw.position,units_per_metre,candidate.position))return false;
    const auto relative=multiply(transpose(rotation_matrix(reference.orientation)),rotation_matrix(raw.orientation));
    const auto forward=map_vector(base_view,{-relative[0][2],-relative[1][2],-relative[2][2]});
    const auto right=map_vector(base_view,{relative[0][0],relative[1][0],relative[2][0]});
    const auto up=map_vector(base_view,{relative[0][1],relative[1][1],relative[2][1]});
    const Matrix world{{{forward.x,right.x,up.x},{forward.y,right.y,up.y},{forward.z,right.z,up.z}}};
    if(!detail::quaternion(world,candidate.orientation)||!stereo_pose_valid(candidate))return false;
    out_world=candidate;return true;
}
} // namespace gears_vr::rig_tracking
