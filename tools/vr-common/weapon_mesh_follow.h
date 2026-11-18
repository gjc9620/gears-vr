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
    double mesh_scale_{1};
    bool bound_{};
    bool left_was_down_{},support_latched_{};
    static bool plausible(gears_vr::Viewpoint const& pose) noexcept {
        return gears_vr::finite(pose.position)&&
            std::abs(pose.position.x)<1e7&&std::abs(pose.position.y)<1e7&&
            std::abs(pose.position.z)<1e7&&
            std::isfinite(pose.rotation.pitch)&&std::isfinite(pose.rotation.yaw)&&
            std::isfinite(pose.rotation.roll);
    }
    static bool rigid_basis(gears_vr::Matrix const& matrix) noexcept {
        for(int row=0;row<3;++row)for(int column=0;column<3;++column)
            if(!std::isfinite(matrix[row][column]))return false;
        for(int i=0;i<3;++i)for(int j=i;j<3;++j){
            double product=0;
            for(int row=0;row<3;++row)
                product+=matrix[row][i]*matrix[row][j];
            if(std::abs(product-(i==j?1.:0.))>1e-3)return false;
        }
        const auto cross=gears_vr::Vector{
            matrix[1][0]*matrix[2][1]-matrix[2][0]*matrix[1][1],
            matrix[2][0]*matrix[0][1]-matrix[0][0]*matrix[2][1],
            matrix[0][0]*matrix[1][1]-matrix[1][0]*matrix[0][1]};
        const double determinant=cross.x*matrix[0][2]+cross.y*matrix[1][2]+
            cross.z*matrix[2][2];
        return determinant>.999&&determinant<1.001;
    }
public:
    void reset() noexcept {bound_=left_was_down_=support_latched_=false;mesh_scale_=1;}
    bool bound() const noexcept {return bound_;}
    bool support_latched() const noexcept {return support_latched_;}
    // Same model transform used by the visual follower, including the bound
    // uniform asset scale. Contact/insertion must not use an old animated pose.
    bool model_matrix(gears_vr::Viewpoint const& right,std::array<float,16>& out) const noexcept {
        out={};gears_vr::Viewpoint solved{};
        if(!solve(right,solved))return false;
        const auto basis=gears_vr::ue_matrix(solved.rotation);
        for(int axis=0;axis<3;++axis)for(int world=0;world<3;++world)
            out[axis*4+world]=float(basis[world][axis]*mesh_scale_);
        out[12]=float(solved.position.x);out[13]=float(solved.position.y);out[14]=float(solved.position.z);out[15]=1;
        return true;
    }
    bool bind(gears_vr::Viewpoint const& right_hand,
              std::array<float,16> const& mesh_local_to_world,
              bool left_down_at_bind=false) noexcept {
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
        mesh_scale_=(scale[0]+scale[1]+scale[2])/3;
        left_was_down_=left_down_at_bind;
        bound_=true;return true;
    }
    // Keep the observed mesh-local contact at the controller, but replace
    // pickup-animation orientation with the asset's authored held relation.
    // This is contact preservation, not physical grip calibration.
    bool bind_oriented(gears_vr::Viewpoint const& right_hand,
                       std::array<float,16> const& mesh_local_to_world,
                       gears_vr::Matrix const& mesh_in_hand) noexcept {
        if(!rigid_basis(mesh_in_hand)||!bind(right_hand,mesh_local_to_world))return false;
        const auto mesh_local_offset=gears_vr::transform(
            gears_vr::transpose(hand_to_mesh_rotation_),hand_to_mesh_);
        hand_to_mesh_=gears_vr::transform(mesh_in_hand,mesh_local_offset);
        hand_to_mesh_rotation_=mesh_in_hand;
        return true;
    }
    // A measured weapon grip replaces the first-frame trial offset. The
    // original hand basis is measured in the gun mesh; the controller-to-palm
    // basis is a separate headset calibration and must be supplied explicitly.
    // A mesh probe is still required to authenticate this weapon and its scale.
    bool bind_mount(gears_vr::Viewpoint const& right_controller,
                    std::array<float,16> const& mesh_local_to_world,
                    gears_vr::Vector palm_in_mesh_raw,
                    gears_vr::Matrix const& original_hand_in_mesh,
                    gears_vr::Matrix const& controller_to_palm,
                    bool left_down_at_bind=false) noexcept {
        if(!gears_vr::finite(palm_in_mesh_raw)||
           std::hypot(std::hypot(palm_in_mesh_raw.x,palm_in_mesh_raw.y),
                      palm_in_mesh_raw.z)>100||
           !rigid_basis(original_hand_in_mesh)||
           !rigid_basis(controller_to_palm)||
           !bind(right_controller,mesh_local_to_world,left_down_at_bind))return false;
        hand_to_mesh_rotation_=gears_vr::multiply(controller_to_palm,
            gears_vr::transpose(original_hand_in_mesh));
        hand_to_mesh_=gears_vr::transform(hand_to_mesh_rotation_,
            {-palm_in_mesh_raw.x*mesh_scale_,-palm_in_mesh_raw.y*mesh_scale_,
             -palm_in_mesh_raw.z*mesh_scale_});
        if(!gears_vr::finite(hand_to_mesh_)||
           std::hypot(std::hypot(hand_to_mesh_.x,hand_to_mesh_.y),
                      hand_to_mesh_.z)>250){reset();return false;}
        return true;
    }
    // Derive a NEW mount from an unchanged baseline. A contact delta is in
    // original grip axes; mesh, muzzle and support consume the same result.
    bool adjusted_contact(gears_vr::Pose const& delta,double units,WeaponMeshFollow& out) const noexcept {
        const auto q=delta.orientation;const double norm=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
        if(!bound_||!gears_vr::finite(delta.position)||!std::isfinite(norm)||std::abs(norm-1.)>1e-6||
           !std::isfinite(units)||units<10||units>500)return false;
        if(std::hypot(std::hypot(delta.position.x,delta.position.y),delta.position.z)>.05)return false;
        const auto inverse=gears_vr::transpose(gears_vr::rotation_matrix(delta.orientation));
        auto candidate=*this;
        candidate.hand_to_mesh_=gears_vr::transform(inverse,{hand_to_mesh_.x-delta.position.x*units,
            hand_to_mesh_.y-delta.position.y*units,hand_to_mesh_.z-delta.position.z*units});
        candidate.hand_to_mesh_rotation_=gears_vr::multiply(inverse,hand_to_mesh_rotation_);
        if(!rigid_basis(candidate.hand_to_mesh_rotation_)||!gears_vr::finite(candidate.hand_to_mesh_))return false;
        out=candidate;return true;
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
    // Private trial: the contact is the original character's Lancer left-palm
    // center measured in this exact mesh frame. A fresh LGrip press must occur
    // near it; holding LGrip elsewhere cannot seize the gun. The controller's
    // own grip-to-palm offset still needs headset calibration.
    bool solve_with_support(gears_vr::Viewpoint const& right_hand,
                            gears_vr::Viewpoint const& left_hand,bool left_tracked,
                            bool left_down,gears_vr::Vector contact_in_mesh,
                            gears_vr::Viewpoint& out,bool& support_used) noexcept {
        support_used=false;
        gears_vr::Viewpoint base{};
        if(!solve(right_hand,base)){
            support_latched_=false;left_was_down_=left_down;return false;
        }
        out=base;
        if(!left_tracked||!left_down||!plausible(left_hand)||
           !gears_vr::finite(contact_in_mesh)){
            support_latched_=false;left_was_down_=left_down;return true;
        }
        const auto basis=gears_vr::ue_matrix(base.rotation);
        const auto contact_shift=gears_vr::transform(basis,
            {contact_in_mesh.x*mesh_scale_,contact_in_mesh.y*mesh_scale_,
             contact_in_mesh.z*mesh_scale_});
        const gears_vr::Vector expected{
            base.position.x+contact_shift.x-right_hand.position.x,
            base.position.y+contact_shift.y-right_hand.position.y,
            base.position.z+contact_shift.z-right_hand.position.z};
        const gears_vr::Vector actual{
            left_hand.position.x-right_hand.position.x,
            left_hand.position.y-right_hand.position.y,
            left_hand.position.z-right_hand.position.z};
        const gears_vr::Vector error{actual.x-expected.x,actual.y-expected.y,
                                     actual.z-expected.z};
        const auto distance=std::hypot(std::hypot(error.x,error.y),error.z);
        const bool rising=!left_was_down_;
        left_was_down_=true;
        if(!std::isfinite(distance)){support_latched_=false;return true;}
        if(!support_latched_){
            if(!rising||distance>10.0)return true;
            support_latched_=true;
        }else if(distance>18.0){support_latched_=false;return true;}
        const auto from_length=std::hypot(std::hypot(expected.x,expected.y),expected.z);
        const auto to_length=std::hypot(std::hypot(actual.x,actual.y),actual.z);
        if(from_length<15||to_length<15){support_latched_=false;return true;}
        const gears_vr::Vector from{expected.x/from_length,expected.y/from_length,
                                    expected.z/from_length};
        const gears_vr::Vector to{actual.x/to_length,actual.y/to_length,
                                  actual.z/to_length};
        const auto cosine=(std::clamp)(from.x*to.x+from.y*to.y+from.z*to.z,-1.,1.);
        if(cosine<std::cos(.8)){support_latched_=false;return true;}
        const gears_vr::Quaternion correction_q{
            from.y*to.z-from.z*to.y,from.z*to.x-from.x*to.z,
            from.x*to.y-from.y*to.x,1+cosine};
        const auto correction=gears_vr::rotation_matrix(correction_q);
        const gears_vr::Vector offset{base.position.x-right_hand.position.x,
                                      base.position.y-right_hand.position.y,
                                      base.position.z-right_hand.position.z};
        const auto shifted=gears_vr::transform(correction,offset);
        gears_vr::Viewpoint candidate{
            {right_hand.position.x+shifted.x,right_hand.position.y+shifted.y,
             right_hand.position.z+shifted.z},
            gears_vr::ue_angles(gears_vr::multiply(correction,basis))};
        if(!plausible(candidate)){support_latched_=false;return true;}
        out=candidate;support_used=true;return true;
    }
};

} // namespace gears_weapon
