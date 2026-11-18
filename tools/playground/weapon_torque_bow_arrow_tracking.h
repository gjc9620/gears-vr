#pragma once
#include "weapon_rig_binding.h"
#include "../vr-common/weapon_torque_bow_arrow_mount.h"
#include "../vr-common/weapon_mesh_follow.h"
// Data only. The GT caller separately authenticates the actual Native reader,
// live source, arrow reservation and hand lease. No snapped/C0 palm is input.
namespace gears_torque_bow_arrow_tracking {
namespace hand=gears_weapon_hand;namespace rig=gears_rig_tracking;
namespace mount=gears_torque_bow_arrow_mount;namespace vr=gears_vr;
struct Sample {
    vr::Pose source_world{},tip_world{};
    std::uint64_t scene{},input_sequence{};
    std::int64_t input_sync_ns{};
    double units_per_metre{};
};
struct RootSample {vr::Pose world{};double scale{};std::uint64_t scene{},input_sequence{};std::int64_t input_sync_ns{};};
// The SAME raw right controller, with the existing immutable grip-to-mesh
// mount. Never a rendered/C0/Snap weapon transform. Original b_TB_Root has
// zero translation and reference yaw+16384 in that component's mesh space.
inline bool root_sample(hand::Frame const& f,rig::Frame const& raw,gears_weapon::WeaponMeshFollow const& fixed,
    std::int64_t now,RootSample& out) noexcept {
    out={};
    if(!fixed.bound()||gears_equipment_rig::match(f,raw,now,unsigned(f.stereo.source))!=gears_equipment_rig::Status::matched||
       !(raw.world_pose_mask&rig::right_grip_pose))return false;
    const auto& grip=raw.world_grip[1];std::array<float,16> matrix{};
    if(!fixed.model_matrix({grip.position,vr::ue_angles(vr::rotation_matrix(grip.orientation))},matrix))return false;
    vr::Matrix basis{};double scales[3]{};
    for(unsigned axis=0;axis<3;++axis){
        scales[axis]=std::hypot(std::hypot(double(matrix[axis*4]),double(matrix[axis*4+1])),double(matrix[axis*4+2]));
        if(!std::isfinite(scales[axis])||scales[axis]<=.1||scales[axis]>=10)return false;
        for(unsigned row=0;row<3;++row)basis[row][axis]=matrix[axis*4+row]/scales[axis];
    }
    for(unsigned axis=1;axis<3;++axis)if(std::abs(scales[axis]-scales[0])>1e-5*scales[0])return false;
    vr::Quaternion mesh{};if(!vr::rig_tracking::detail::quaternion(basis,mesh))return false;
    RootSample value{};value.world={{matrix[12],matrix[13],matrix[14]},
        vr::normalized(vr::multiply(mesh,vr::Quaternion{0,0,.7071067811865475244,.7071067811865475244}))};
    if(!mount::pose_valid(value.world))return false;
    value.scale=scales[0];value.scene=f.key.frame;value.input_sequence=f.controllers.sequence;value.input_sync_ns=f.controllers.sync_ns;
    out=value;return true;
}
inline bool close_vector(vr::Vector a,vr::Vector b,double epsilon) noexcept {
    return vr::finite(a)&&vr::finite(b)&&std::abs(a.x-b.x)<=epsilon&&
        std::abs(a.y-b.y)<=epsilon&&std::abs(a.z-b.z)<=epsilon;
}
inline bool coherent(mount::Mount const& m) noexcept {
    if(!mount::pose_valid(m.source_in_grip_units)||!mount::pose_valid(m.tip_in_grip_units)||
       !std::isfinite(m.component_scale)||m.component_scale<=.1||m.component_scale>=10||
       !std::isfinite(m.units_per_metre)||m.units_per_metre<10||m.units_per_metre>500)return false;
    const auto expected=mount::add(m.source_in_grip_units.position,
        mount::rotate(m.source_in_grip_units.orientation,mount::mul(mount::tip_raw,m.component_scale)));
    const auto& a=m.source_in_grip_units.orientation;const auto& b=m.tip_in_grip_units.orientation;
    const double dot=a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w;
    return std::abs(std::abs(dot)-1)<=1e-10&&close_vector(expected,m.tip_in_grip_units.position,1e-8)&&
        close_vector(mount::mul(m.tip_in_grip_units.position,1/m.units_per_metre),m.raw_tip_in_grip_m,1e-10);
}
inline bool sample(hand::Frame const& f,rig::Frame const& raw,mount::Mount const& m,
    std::int64_t now,Sample& out) noexcept {
    out={};
    if(!coherent(m)||gears_equipment_rig::match(f,raw,now,unsigned(f.stereo.source))!=gears_equipment_rig::Status::matched||
       !(raw.world_pose_mask&rig::left_grip_pose)||!f.left_tracked||!f.left_grip_known||
       m.units_per_metre!=raw.units_per_metre)return false;
    const auto& grip=raw.world_grip[0];Sample value{};
    value.source_world={mount::add(grip.position,mount::rotate(grip.orientation,m.source_in_grip_units.position)),
        vr::normalized(vr::multiply(grip.orientation,m.source_in_grip_units.orientation))};
    // Both display and mechanics refer to THIS same immutable original tip.
    value.tip_world={mount::add(value.source_world.position,
        mount::rotate(value.source_world.orientation,mount::mul(mount::tip_raw,m.component_scale))),value.source_world.orientation};
    if(!mount::pose_valid(value.source_world)||!mount::pose_valid(value.tip_world))return false;
    value.scene=f.key.frame;value.input_sequence=f.controllers.sequence;
    value.input_sync_ns=f.controllers.sync_ns;value.units_per_metre=raw.units_per_metre;out=value;return true;
}
}
