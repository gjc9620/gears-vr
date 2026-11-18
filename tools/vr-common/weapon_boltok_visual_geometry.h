#pragma once
#include "weapon_boltok_mechanics.h"
namespace gears_weapon {
// Bounds and pivot from saved original Object02 audit, source a1f04758... .
// Authoring uses100 raw units/metre, not an assumed game100 units/metre.
// Current component scale and current rig units determine physical size.
inline constexpr Vector boltok_loading_face_in_bone_raw{
    0,-3.8,0}; // Exact workbench whole-cylinder feed = Object02 pivot + XR{0,0,.038}.
inline constexpr double boltok_authoring_raw_per_metre=100.;
struct BoltokVisualGeometry {
    double scale{},units{};std::uint64_t calibration{};
    bool valid() const noexcept {return std::isfinite(scale)&&scale>.0001&&scale<100&&
        std::isfinite(units)&&units>=10&&units<=500&&calibration;}
    double physical_scale() const noexcept {return boltok_authoring_raw_per_metre*scale/units;}
    bool cylinder(Pose const& bone,Pose& out) const noexcept {
        if(!valid()||!gears_vr::stereo_pose_valid(bone))return false;
        out={sum(bone.position,rotate(bone.orientation,scaled(boltok_loading_face_in_bone_raw,scale))),bone.orientation};
        return gears_vr::stereo_pose_valid(out);
    }
    bool round(Pose const& face,unsigned slot,Pose& out) const noexcept {
        if(!valid()||slot>=6||!gears_vr::stereo_pose_valid(face))return false;
        const double a=slot*6.283185307179586/6,raw=boltok_authoring_raw_per_metre*scale;
        const Vector local{.0192*std::cos(a)*raw,-.003*raw,-.0192*std::sin(a)*raw};
        out={sum(face.position,rotate(face.orientation,local)),face.orientation};return gears_vr::stereo_pose_valid(out);
    }
    bool ports(BoltokChamberGeometry& out) const noexcept {
        out={};if(!valid())return false;const auto k=physical_scale();
        out.calibration=calibration;out.current_instance_verified=true;
        for(unsigned i=0;i<6;++i){const double a=i*6.283185307179586/6;auto& p=out.ports[i];
            p.port_in_weapon.position={.0192*std::cos(a)*k,0,-.0192*std::sin(a)*k};
            p.item_in_left_grip.position={0,.003*k,0}; // Round base sits behind face.
            p.outward_axis_in_weapon={0,-1,0};p.lateral_tolerance_metres=.006*k;
            p.seated_depth_metres=(std::min)(.006,.006*k);
            p.maximum_sample_gap_ns=100000000;
        }
        // Port tolerances scale DOWN with actual model size; they must never
        // inherit a magazine-sized capture volume for adjacent small holes.
        return out.valid();
    }
};
}
