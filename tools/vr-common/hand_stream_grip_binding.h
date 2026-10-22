#pragma once
#include "pose_math.h"
#include <array>
#include <cmath>
#include <limits>

namespace gears_hand_stream {
// Temporary per-hand grip-to-bone alignment, in game world units. The caller
// owns ticket/identity/action boundaries and resets this binding on hand-back.
// This is not a measured controller-to-palm calibration or a write permit.
class GripHandBinding {
    gears_vr::Vector offset_{};
    gears_vr::Matrix rotation_{};
    bool bound_{};
    static double dot(gears_vr::Vector a,gears_vr::Vector b) noexcept {
        return a.x*b.x+a.y*b.y+a.z*b.z;
    }
    static gears_vr::Vector scaled(gears_vr::Vector a,double scale) noexcept {
        return {a.x*scale,a.y*scale,a.z*scale};
    }
    static gears_vr::Vector cross(gears_vr::Vector a,gears_vr::Vector b) noexcept {
        return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
    }
    static double length(gears_vr::Vector v) noexcept {
        return std::hypot(std::hypot(v.x,v.y),v.z);
    }
    static bool finite(gears_vr::Viewpoint const& pose) noexcept {
        return gears_vr::finite(pose.position)&&std::isfinite(pose.rotation.pitch)&&
            std::isfinite(pose.rotation.yaw)&&std::isfinite(pose.rotation.roll);
    }
    static bool finite(gears_vr::Matrix const& matrix) noexcept {
        for(auto const& row:matrix)for(double value:row)if(!std::isfinite(value))return false;
        return true;
    }
    static void invalidate(gears_vr::Viewpoint& out) noexcept {
        const auto invalid=std::numeric_limits<double>::quiet_NaN();
        out={{invalid,invalid,invalid},{invalid,invalid,invalid}};
    }
    static bool bone_basis(std::array<float,16> const& matrix,gears_vr::Matrix& out) noexcept {
        // Permit ordinary float roundoff (~1e-6), not a scaled/sheared mount.
        constexpr double affine_tolerance=1e-5,rigid_tolerance=1e-4;
        for(float value:matrix)if(!std::isfinite(value))return false;
        if(std::abs(matrix[3])>affine_tolerance||std::abs(matrix[7])>affine_tolerance||
           std::abs(matrix[11])>affine_tolerance||std::abs(double(matrix[15])-1)>affine_tolerance)return false;
        std::array<gears_vr::Vector,3> axes{};
        for(unsigned axis=0;axis<3;++axis){
            axes[axis]={matrix[axis*4],matrix[axis*4+1],matrix[axis*4+2]};
            const auto norm=length(axes[axis]);
            if(std::abs(norm-1)>rigid_tolerance)return false;
            axes[axis]=scaled(axes[axis],1/norm);
        }
        for(unsigned a=0;a<3;++a)for(unsigned b=a+1;b<3;++b)
            if(std::abs(dot(axes[a],axes[b]))>rigid_tolerance)return false;
        const auto determinant=dot(axes[0],cross(axes[1],axes[2]));
        if(!(determinant>0)||std::abs(determinant-1)>3*rigid_tolerance)return false;
        // Remove only accepted rounding noise, after rejecting reflections.
        const auto x=axes[0];
        const auto projection=dot(x,axes[1]);
        const gears_vr::Vector perpendicular{axes[1].x-projection*x.x,
            axes[1].y-projection*x.y,axes[1].z-projection*x.z};
        const auto y=scaled(perpendicular,1/length(perpendicular));
        const auto z=cross(x,y);
        // UE FMatrix has row axes; pose_math has column axes.
        out={{{x.x,y.x,z.x},{x.y,y.y,z.y},{x.z,y.z,z.z}}};
        return finite(out);
    }
public:
    void reset() noexcept {offset_={};rotation_={};bound_=false;}
    bool bound() const noexcept {return bound_;}
    bool bind(gears_vr::Viewpoint const& grip_world,
              std::array<float,16> const& hand_bone_world) noexcept {
        reset(); // A rejected rebind must never retain the old mount.
        if(!finite(grip_world))return false;
        gears_vr::Matrix bone{};
        if(!bone_basis(hand_bone_world,bone))return false;
        const auto inverse_grip=gears_vr::transpose(gears_vr::ue_matrix(grip_world.rotation));
        const gears_vr::Vector delta{double(hand_bone_world[12])-grip_world.position.x,
            double(hand_bone_world[13])-grip_world.position.y,
            double(hand_bone_world[14])-grip_world.position.z};
        const auto offset=gears_vr::transform(inverse_grip,delta);
        const auto rotation=gears_vr::multiply(inverse_grip,bone);
        if(!gears_vr::finite(offset)||!finite(rotation))return false;
        offset_=offset;rotation_=rotation;bound_=true;return true;
    }
    bool solve(gears_vr::Viewpoint const& grip_world,gears_vr::Viewpoint& out) const noexcept {
        const auto grip=grip_world; // Preserve input if caller solves in place.
        invalidate(out); // False never leaves a stale, finite target behind.
        if(!bound_||!finite(grip))return false;
        const auto basis=gears_vr::ue_matrix(grip.rotation);
        const auto offset=gears_vr::transform(basis,offset_);
        const auto rotation=gears_vr::multiply(basis,rotation_);
        if(!gears_vr::finite(offset)||!finite(rotation))return false;
        const gears_vr::Viewpoint candidate{{grip.position.x+offset.x,
            grip.position.y+offset.y,grip.position.z+offset.z},gears_vr::ue_angles(rotation)};
        if(!finite(candidate))return false;
        out=candidate;return true;
    }
};
} // namespace gears_hand_stream
