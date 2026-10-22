#pragma once
#include "pose_math.h"
#include <array>
#include <cmath>

namespace gears_weapon {

// Temporary private test calibration. It preserves the original weapon mesh's
// starting pose and makes subsequent right-hand motion move that same mesh.
// Final gameplay needs a measured grip mount per weapon instead of this latch.
class WeaponMeshFollow {
    gears_vr::Vector hand_to_mesh_{};
    gears_vr::Matrix hand_to_mesh_rotation_{};
    bool bound_{};
    static bool plausible(gears_vr::Viewpoint const& pose) noexcept {
        return gears_vr::finite(pose.position)&&
            std::abs(pose.position.x)<1e7&&std::abs(pose.position.y)<1e7&&
            std::abs(pose.position.z)<1e7&&
            std::isfinite(pose.rotation.pitch)&&std::isfinite(pose.rotation.yaw)&&
            std::isfinite(pose.rotation.roll);
    }
public:
    void reset() noexcept {bound_=false;}
    bool bound() const noexcept {return bound_;}
    bool bind(gears_vr::Viewpoint const& right_hand,
              std::array<float,16> const& mesh_local_to_world) noexcept {
        if(bound_||!plausible(right_hand))return false;
        for(float x:mesh_local_to_world)if(!std::isfinite(x))return false;
        const auto& m=mesh_local_to_world;
        if(std::abs(m[3])>1e-4||std::abs(m[7])>1e-4||
           std::abs(m[11])>1e-4||std::abs(m[15]-1)>1e-4)return false;
        gears_vr::Matrix mesh_basis{};
        double scale[3]{};
        for(int axis=0;axis<3;++axis){
            scale[axis]=std::hypot(std::hypot(double(m[axis*4]),double(m[axis*4+1])),
                                   double(m[axis*4+2]));
            if(scale[axis]<.1||scale[axis]>10)return false;
            for(int world=0;world<3;++world)
                mesh_basis[world][axis]=m[axis*4+world]/scale[axis];
        }
        for(int i=0;i<3;++i)for(int j=i+1;j<3;++j){
            if(std::abs(scale[i]-scale[j])>(std::max)(scale[i],scale[j])*.02)return false;
            double dot=0;for(int world=0;world<3;++world)
                dot+=mesh_basis[world][i]*mesh_basis[world][j];
            if(std::abs(dot)>1e-3)return false;
        }
        const auto cross=gears_vr::Vector{
            mesh_basis[1][0]*mesh_basis[2][1]-mesh_basis[2][0]*mesh_basis[1][1],
            mesh_basis[2][0]*mesh_basis[0][1]-mesh_basis[0][0]*mesh_basis[2][1],
            mesh_basis[0][0]*mesh_basis[1][1]-mesh_basis[1][0]*mesh_basis[0][1]};
        const double determinant=cross.x*mesh_basis[0][2]+cross.y*mesh_basis[1][2]+
                                 cross.z*mesh_basis[2][2];
        if(determinant<.999||determinant>1.001)return false;
        const gears_vr::Vector origin{m[12],m[13],m[14]};
        if(!gears_vr::finite(origin))return false;
        const auto hand_basis=gears_vr::ue_matrix(right_hand.rotation);
        const gears_vr::Vector displacement{origin.x-right_hand.position.x,
            origin.y-right_hand.position.y,origin.z-right_hand.position.z};
        const auto offset=gears_vr::transform(gears_vr::transpose(hand_basis),displacement);
        if(!gears_vr::finite(offset)||std::hypot(std::hypot(offset.x,offset.y),offset.z)>250)return false;
        hand_to_mesh_=offset;
        hand_to_mesh_rotation_=gears_vr::multiply(gears_vr::transpose(hand_basis),mesh_basis);
        bound_=true;return true;
    }
    bool solve(gears_vr::Viewpoint const& right_hand,gears_vr::Viewpoint& out) const noexcept {
        if(!bound_||!plausible(right_hand))return false;
        const auto hand_basis=gears_vr::ue_matrix(right_hand.rotation);
        const auto shift=gears_vr::transform(hand_basis,hand_to_mesh_);
        const auto mesh_basis=gears_vr::multiply(hand_basis,hand_to_mesh_rotation_);
        gears_vr::Viewpoint candidate{{right_hand.position.x+shift.x,
            right_hand.position.y+shift.y,right_hand.position.z+shift.z},
            gears_vr::ue_angles(mesh_basis)};
        if(!plausible(candidate))return false;
        out=candidate;return true;
    }
};

} // namespace gears_weapon
