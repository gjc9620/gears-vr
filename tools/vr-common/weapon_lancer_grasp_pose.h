#pragma once
#include "hand_object_grasp.h"
#include "hand_authored_mount_profile.h"

// Cosmetic authoring for original Lancer parts. No controller/contact change.
namespace gears_weapon_lancer_grasp {
namespace vr=gears_vr;namespace grasp=gears_hand_object_grasp;
// Original handle barrel PCA is within1.5deg of mesh+Y. Finger flexion is
// about wrist Z, so put Z along that barrel, X into its side, and Y upward.
// The former Y180 pose put finger curl along the barrel instead of around it.
inline const vr::Quaternion wrist_in_part{0,std::sqrt(.5),std::sqrt(.5),0};
// Centre the original barrel inside this C-specific finger loop. This is a
// cosmetic wrist mount only; the real grasp point and physical rail stay put.
inline constexpr vr::Vector starter_contact_in_wrist{10,0,0};
inline constexpr vr::Vector loading_contact_in_wrist{9.75,-4.6,-2.1};
// A cylinder constrains the finger-row direction, but does not prescribe a
// palm roll from the wrist-to-centre POSITION vector. Project the actual
// wrist orientation onto that axis with the shortest rotation instead.
inline bool cylinder_orientation(vr::Quaternion physical,vr::Vector barrel,double direction,vr::Quaternion& out) noexcept {
    const double norm=barrel.x*barrel.x+barrel.y*barrel.y+barrel.z*barrel.z;
    if(!vr::stereo_pose_valid({{},physical})||!vr::finite(barrel)||!std::isfinite(norm)||std::abs(norm-1.)>1e-6||
       (direction!=1.&&direction!=-1.))return false;
    const auto from=vr::transform(vr::rotation_matrix(physical),{0,0,1});
    const vr::Vector to{barrel.x*direction,barrel.y*direction,barrel.z*direction};
    const double dot=std::clamp(from.x*to.x+from.y*to.y+from.z*to.z,-1.,1.);
    vr::Quaternion arc{};
    if(dot<-.999999){
        const auto x=vr::transform(vr::rotation_matrix(physical),{1,0,0});arc={x.x,x.y,x.z,0};
    }else arc=vr::normalized({from.y*to.z-from.z*to.y,from.z*to.x-from.x*to.z,from.x*to.y-from.y*to.x,1.+dot});
    out=vr::normalized(vr::multiply(arc,physical));return true;
}
inline bool part_grip(grasp::Kind kind,vr::Quaternion gun,vr::Quaternion motion,
    vr::Vector contact,vr::Pose const& physical_wrist,double model_scale,double units,vr::Pose& out,double starter_direction=0) noexcept {
    if((kind!=grasp::Kind::loading&&kind!=grasp::Kind::starter)||
       !vr::stereo_pose_valid({contact,gun})||!vr::stereo_pose_valid({{},motion})||
       !vr::stereo_pose_valid(physical_wrist))return false;
    gears_hand_authored_mount::Mount mount{};
    if(!gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
       gears_hand_authored_mount::Side::left,model_scale,units,mount))return false;
    const auto part=vr::normalized(vr::multiply(gun,motion));
    const auto axes=vr::rotation_matrix(part);
    const auto barrel=vr::transform(axes,{0,1,0});
    const auto side=vr::transform(axes,{-1,0,0});
    const vr::Vector approach{contact.x-physical_wrist.position.x,
        contact.y-physical_wrist.position.y,contact.z-physical_wrist.position.z};
    const auto dot=[](vr::Vector a,vr::Vector b){return a.x*b.x+a.y*b.y+a.z*b.z;};
    const double along=dot(approach,barrel);
    const vr::Vector radial{approach.x-barrel.x*along,approach.y-barrel.y*along,approach.z-barrel.z*along};
    const double length=std::hypot(std::hypot(radial.x,radial.y),radial.z);
    // A cylindrical handle admits an approach around its circumference. The
    // former single side-facing basis folded the wrist even after a valid
    // contact from above/below. Keep the finger-row axis on the actual part,
    // but face the palm into the part from THIS unsnapped wrist. This rotates
    // only cosmetic skin; contact/travel still use the original input.
    // An axial/inside-axis approach has no stable radial direction. Let the
    // existing blend release instead of inventing a 180-degree wrist turn.
    if(!std::isfinite(length)||(kind==grasp::Kind::loading&&length<.25*model_scale))return false;
    const vr::Vector cross{side.y*radial.z-side.z*radial.y,
        side.z*radial.x-side.x*radial.z,side.x*radial.y-side.y*radial.x};
    const double angle=std::atan2(dot(barrel,cross),dot(side,radial));
    const double s=std::sin(angle*.5);
    const vr::Quaternion around{barrel.x*s,barrel.y*s,barrel.z*s,std::cos(angle*.5)};
    auto wrist=vr::normalized(vr::multiply(around,vr::multiply(part,wrist_in_part)));
    if(kind==grasp::Kind::starter){
        const auto row=vr::transform(vr::rotation_matrix(physical_wrist.orientation),{0,0,1});
        const double direction=starter_direction?starter_direction:(dot(row,barrel)<0?-1.:1.);
        if(!cylinder_orientation(physical_wrist.orientation,barrel,direction,wrist))return false;
    }
    if(kind==grasp::Kind::loading){
        // E is a small index/thumb pinch on a tab, not C's cylindrical Grip.
        // The old barrel-aligned palm could turn a naturally approaching
        // wrist almost backwards and push the sleeve across the eye. Fit
        // toward that part with at most30deg of initial cosmetic rotation;
        // preserve the actual controller pose and the distinct pinch fingers.
        // PartFit then retains this mount through the real linear stroke.
        constexpr double maximum=0.52359877559829887308;
        auto delta=vr::normalized(vr::multiply(wrist,vr::inverse(physical_wrist.orientation)));
        if(delta.w<0)delta={-delta.x,-delta.y,-delta.z,-delta.w};
        const double sine=std::hypot(std::hypot(delta.x,delta.y),delta.z);
        if(2*std::atan2(sine,delta.w)>maximum&&sine>1e-12){
            const double factor=std::sin(maximum*.5)/sine;
            delta={delta.x*factor,delta.y*factor,delta.z*factor,std::cos(maximum*.5)};
            wrist=vr::normalized(vr::multiply(delta,physical_wrist.orientation));
        }
    }
    // Original Marcus115 LOD0 surface: C sits in the curled palm; E sits
    // between the opposed distal index/thumb pads, not at C's palm anchor.
    // Preserve the existing correction budget and final Native admission.
    const vr::Vector palm=kind==grasp::Kind::starter?starter_contact_in_wrist:loading_contact_in_wrist;
    const auto d=vr::transform(vr::rotation_matrix(wrist),{palm.x*model_scale,palm.y*model_scale,palm.z*model_scale});
    const auto grip=vr::normalized(vr::multiply(wrist,vr::inverse(mount.calibration.grip_to_wrist_rotation)));
    const auto offset=vr::transform(vr::rotation_matrix(grip),mount.calibration.grip_to_wrist_metres);
    const vr::Pose result{{contact.x-d.x-offset.x*units,contact.y-d.y-offset.y*units,
        contact.z-d.z-offset.z*units},grip};
    if(!vr::stereo_pose_valid(result))return false;out=result;return true;
}
// E retains its acquired local mount. C retains its cylinder-direction branch
// but follows current physical roll, not a wrist-to-centre radial heuristic.
// Caller owns the exact contact/weapon/tracking key. This is skin only.
struct PartFit {
    bool known{};grasp::Kind kind{};double model_scale{},units{};
    vr::Quaternion rotation{};vr::Vector offset{};double starter_direction{};
    void reset() noexcept {*this={};}
    bool solve(bool new_grasp,grasp::Kind requested,vr::Quaternion gun,vr::Quaternion motion,
        vr::Vector contact,vr::Pose const& physical,double scale,double unit,vr::Pose& out) noexcept {
        if(new_grasp)reset();
        if((requested!=grasp::Kind::loading&&requested!=grasp::Kind::starter)||
           !vr::stereo_pose_valid({contact,gun})||!vr::stereo_pose_valid({{},motion})||
           !vr::stereo_pose_valid(physical)||!std::isfinite(scale)||scale<=0||
           !std::isfinite(unit)||unit<10||unit>500)return false;
        const auto part=vr::normalized(vr::multiply(gun,motion));
        const bool capture=!known||kind!=requested||model_scale!=scale||units!=unit;
        if(capture){
            const auto row=vr::transform(vr::rotation_matrix(physical.orientation),{0,0,1});
            const auto barrel=vr::transform(vr::rotation_matrix(part),{0,1,0});
            starter_direction=row.x*barrel.x+row.y*barrel.y+row.z*barrel.z<0?-1.:1.;
            vr::Pose initial{};
            if(!part_grip(requested,gun,motion,contact,physical,scale,unit,initial,starter_direction))return false;
            rotation=vr::multiply(vr::inverse(part),initial.orientation);
            offset=vr::transform(vr::transpose(vr::rotation_matrix(part)),
                {initial.position.x-contact.x,initial.position.y-contact.y,initial.position.z-contact.z});
            kind=requested;model_scale=scale;units=unit;known=true;
        }
        const auto d=vr::transform(vr::rotation_matrix(part),offset);
        vr::Pose result{{contact.x+d.x,contact.y+d.y,contact.z+d.z},
            vr::normalized(vr::multiply(part,rotation))};
        if(requested==grasp::Kind::starter){
            // Retain the same grasp and cylinder-direction branch, while
            // allowing natural roll around the cylinder from this fresh
            // physical wrist. Cosmetic unfolding no longer rigidly carries
            // a position-derived wrist roll through a forced quarter turn.
            if(!part_grip(requested,gun,motion,contact,physical,scale,unit,result,starter_direction))return false;
            gears_hand_authored_mount::Mount mount{};
            if(!gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
               gears_hand_authored_mount::Side::left,scale,unit,mount))return false;
            const auto raw_rotation=vr::multiply(physical.orientation,
                vr::inverse(mount.calibration.grip_to_wrist_rotation));
            const auto wrist_offset=vr::transform(vr::rotation_matrix(raw_rotation),mount.calibration.grip_to_wrist_metres);
            const vr::Vector raw_grip{physical.position.x-wrist_offset.x*unit,
                physical.position.y-wrist_offset.y*unit,physical.position.z-wrist_offset.z*unit};
            const auto axis=vr::transform(vr::rotation_matrix(part),{0,1,0});
            // A cylindrical grasp need not stay at its exact centre. The
            // original685-vertex C group spans Y20.065..33.489; the centre
            // Y25.016 +/-3raw units stays on that handle. Choose the nearest
            // bounded axial skin fit while it unfolds. The common large-C
            // skin envelope also retains ordinary mechanical overtravel.
            // No radial contact or physical input change is manufactured.
            const double along=(raw_grip.x-result.position.x)*axis.x+
                (raw_grip.y-result.position.y)*axis.y+(raw_grip.z-result.position.z)*axis.z;
            const double slide=std::clamp(along,-3.*scale,3.*scale);
            result.position={result.position.x+axis.x*slide,result.position.y+axis.y*slide,result.position.z+axis.z*slide};
        }
        if(!vr::stereo_pose_valid(result))return false;out=result;return true;
    }
};
}
