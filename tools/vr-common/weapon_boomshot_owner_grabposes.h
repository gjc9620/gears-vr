#pragma once
#include "hand_authored_mount_profile.h"
#include "hand_wrist_calibration.h"
#include "pose_math.h"

// Boomshot source-space authoring. The ammo wrist comes from the selected
// Boomshot.glb ammo/left archive and is expressed in Locust_Boomshot_Magazine
// source coordinates after undoing the workbench .75 / centroid / feed chain.
// The main wrist is the Boomshot project's authored fallback; this readback has
// no adopted Boomshot right-hand capture. Static data only: no input/contact/
// writer grant.
namespace gears_boomshot_owner_grabposes {
namespace vr=gears_vr;
inline constexpr char main_provenance[]="Boomshot.glb.project.json authored_default";
inline constexpr char ammo_capture_id[]="1794499171798-4";
inline constexpr char ammo_model[]="Boomshot.glb";
inline constexpr char ammo_component[]="Locust_Boomshot.Mesh.Locust_Boomshot_Magazine";
inline constexpr char ammo_component_source_sha256[]="3f0f49cba8a4522df276112b85842cc039055a31ae88a4ad32602fcb44170480";
inline constexpr double ammo_capture_time_ns=13928689036178.;

// Boomshot authored main wrist, transformed through the same workbench A/S
// hand basis used for ammo. Source units are raw centimetres; the game model
// scale is applied by the caller once.
inline constexpr vr::Pose main_wrist_in_mesh{{.45673370361328131,-5.4255599975585938,.2020130157470699},
    {-1.5700924586837749e-16,0.,.70710678118654746,.70710678118654757}};
inline constexpr vr::Vector main_contact_in_mesh{0.,0.,0.};
inline constexpr double main_contact_radius_m=.13;

// Source-space wrist in the separate original carrier mesh. The source model
// already carries the player .75 scale in the caller's component transform;
// do not apply .75 again in this relation.
inline constexpr vr::Pose ammo_wrist_in_carrier_source{
    {.12446883396756643,-.10665401193901014,-.10103154195092073},
    {.52237848255229413,.7675935670375067,-.10130811351585792,-.35729190159428131}};
inline constexpr vr::Vector ammo_contact_in_carrier_source{0.,-.00391493,.08760626};
inline constexpr double ammo_contact_radius_m=.10;
inline constexpr double snap_distance_m=.10;
inline constexpr double release_distance_m=.18;
inline constexpr double snap_angle_radians=1.48;

inline bool pose_valid(vr::Pose const& p) noexcept {
    const auto q=p.orientation;const double n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    return vr::finite(p.position)&&std::isfinite(n)&&std::abs(n-1.)<.01;
}

inline bool mesh_wrist(vr::Pose const& root,double scale,vr::Pose const& local,vr::Pose& out) noexcept {
    const auto q=root.orientation;const double n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    if(!vr::finite(root.position)||!std::isfinite(scale)||scale<=0||
       !std::isfinite(n)||std::abs(n-1.)>1e-6||!pose_valid(local))return false;
    const auto d=vr::transform(vr::rotation_matrix(q),
        {local.position.x*scale,local.position.y*scale,local.position.z*scale});
    out={{root.position.x+d.x,root.position.y+d.y,root.position.z+d.z},
         vr::normalized(vr::multiply(q,local.orientation))};
    return pose_valid(out);
}

inline bool main_calibration(double model_scale,double units,gears_hand_wrist::Calibration& out) noexcept {
    gears_hand_authored_mount::Mount free{};
    if(!gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
        gears_hand_authored_mount::Side::right,model_scale,units,free))return false;
    constexpr vr::Quaternion mesh_in_aim{0,0,-.7071067811865475244,.7071067811865475244};
    const auto marker=vr::transform(vr::transpose(vr::rotation_matrix(free.calibration.grip_to_wrist_rotation)),
        free.calibration.grip_to_wrist_metres);
    const auto wrist=vr::normalized(vr::multiply(mesh_in_aim,main_wrist_in_mesh.orientation));
    out={true,vr::transform(vr::rotation_matrix(wrist),marker),wrist};
    return true;
}

inline bool main_grip(vr::Pose const& gun,double scale,double units,
                      double model_scale,vr::Pose& out) noexcept {
    vr::Pose wrist{};
    if(!mesh_wrist(gun,scale,main_wrist_in_mesh,wrist))return false;
    gears_hand_authored_mount::Mount hand{};
    if(!gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
        gears_hand_authored_mount::Side::right,model_scale,units,hand))return false;
    gears_hand_wrist::Calibration calibration{};if(!main_calibration(model_scale,units,calibration))return false;
    out.orientation=vr::normalized(vr::multiply(wrist.orientation,
        vr::inverse(calibration.grip_to_wrist_rotation)));
    const auto d=vr::transform(vr::rotation_matrix(out.orientation),calibration.grip_to_wrist_metres);
    out.position={wrist.position.x-d.x*units,wrist.position.y-d.y*units,wrist.position.z-d.z*units};
    return pose_valid(out);
}

// Inverse of main_grip(): the current physical Grip is converted to a wrist
// with the existing Marcus calibration, then the captured wrist-in-mesh pose
// places the complete gun. Positions are game units; the capture position is
// original-mesh raw units, so only the component uniform scale applies here.
inline bool main_gun_from_grip(vr::Pose const& grip,double scale,double units,
                               double model_scale,vr::Pose& gun) noexcept {
    if(!pose_valid(grip)||!std::isfinite(scale)||scale<=0||
       !std::isfinite(units)||units<10||units>500)return false;
    gears_hand_authored_mount::Mount hand{};
    if(!gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
        gears_hand_authored_mount::Side::right,model_scale,units,hand))return false;
    vr::Pose wrist{};
    gears_hand_wrist::Calibration calibration{};if(!main_calibration(model_scale,units,calibration))return false;
    if(gears_hand_wrist::native_wrist_target(grip,calibration,units,wrist)!=
       gears_hand_wrist::Status::okay)return false;
    const auto inverse_local=vr::inverse(main_wrist_in_mesh.orientation);
    const auto rotated=vr::multiply(wrist.orientation,inverse_local);
    const auto local=vr::Vector{main_wrist_in_mesh.position.x*scale,
        main_wrist_in_mesh.position.y*scale,main_wrist_in_mesh.position.z*scale};
    const auto offset=vr::transform(vr::rotation_matrix(rotated),local);
    gun={{wrist.position.x-offset.x,wrist.position.y-offset.y,wrist.position.z-offset.z},
         vr::normalized(rotated)};
    return pose_valid(gun);
}

struct AmmoCarrierRelation {
    vr::Pose wrist_in_carrier{}; // normalized source metres
};

// Verify the original M0 root pose and expose the captured source-local wrist.
// The selected archive's wrist is already carrier-local; it is not a gun-root
// wrist and must never be transformed by inverse(M0) as though it were one.
// m0_root_scale is retained for call compatibility. The wrist is already
// asset-local, so no root/M0 ratio is applied here.
inline bool ammo_wrist_relation(vr::Pose const& seated_in_root_m,
    double m0_root_scale,double units,AmmoCarrierRelation& out) noexcept {
    if(!pose_valid(seated_in_root_m)||!std::isfinite(m0_root_scale)||m0_root_scale<=0||
       !std::isfinite(units)||units<10||units>500)return false;
    out.wrist_in_carrier={ammo_wrist_in_carrier_source.position,
        ammo_wrist_in_carrier_source.orientation};
    return pose_valid(out.wrist_in_carrier);
}

// Raw LEFT Grip -> physical wrist via the existing Marcus calibration, then
// inverse captured wrist-in-carrier -> complete M0 world pose. Grip remains
// the physical input; the resulting M0 pose is authored output, never input.
inline bool ammo_carrier_from_grip(vr::Pose const& grip,AmmoCarrierRelation const& relation,
    double m0_scale,double units,double hand_model_scale,vr::Pose& carrier) noexcept {
    if(!pose_valid(grip)||!pose_valid(relation.wrist_in_carrier)||!std::isfinite(m0_scale)||m0_scale<=0||
       !std::isfinite(units)||units<10||units>500)return false;
    gears_hand_authored_mount::Mount hand{};
    if(!gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
        gears_hand_authored_mount::Side::left,hand_model_scale,units,hand))return false;
    vr::Pose wrist{};
    if(gears_hand_wrist::native_wrist_target(grip,hand.calibration,units,wrist)!=
       gears_hand_wrist::Status::okay)return false;
    const auto carrier_q=vr::normalized(vr::multiply(wrist.orientation,
        vr::inverse(relation.wrist_in_carrier.orientation)));
    const auto scaled=vr::Vector{relation.wrist_in_carrier.position.x*m0_scale*units,
        relation.wrist_in_carrier.position.y*m0_scale*units,
        relation.wrist_in_carrier.position.z*m0_scale*units};
    const auto offset=vr::transform(vr::rotation_matrix(carrier_q),scaled);
    carrier={{wrist.position.x-offset.x,wrist.position.y-offset.y,wrist.position.z-offset.z},carrier_q};
    return pose_valid(carrier);
}

}
