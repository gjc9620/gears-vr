#pragma once
#include "weapon_troika_grip_solver.h"
#include "weapon_troika_handle_geometry.h"
#include <array>
#include <cstdint>

namespace gears_weapon::troika_mechanics {
using Point=TroikaSolverPoint;using Matrix=std::array<float,16>;using Vector=gears_vr::Vector;
enum class Space:unsigned {unknown,world,actor,component,parent_bone,bone,other_bone};
struct Input {
    std::uint64_t binding{},calibration{};double units_per_metre{},maximum_error_metres{};
    std::array<Matrix,13> bone_world{};Matrix component_world{};
    std::array<std::int32_t,3> actor_rotation{};
    std::array<unsigned,2> control_bone{},rotation_mask{},other_bone{13,13};
    std::array<Space,2> space{};std::array<Vector,2> anchors{};
    bool original_control_binding_and_reference_closed{};
};
struct Geometry {
    TroikaSerialGeometry serial{};Vector world_origin{};double units_per_metre{};
    unsigned parent_control{},child_control{};
};
inline Point point(Vector p){return {p.x,p.y,p.z};}
inline Vector vector(Point p){return {p.x,p.y,p.z};}
inline bool rigid(Matrix const& m) noexcept {
    Vector local{};double scale{};
    return troika_handle_geometry::local_point(m,{m[12],m[13],m[14]},local,scale);
}
inline bool axis(Matrix const& frame,unsigned mask,Point& out) noexcept {
    if(!mask||(mask&~7u)||(mask&(mask-1))||!rigid(frame))return false;
    const unsigned index=mask==1?0:mask==2?1:2;
    const Point raw{frame[index*4],frame[index*4+1],frame[index*4+2]};
    const auto length=std::sqrt(troika_solver_detail::norm(raw));
    out={raw.x/length,raw.y/length,raw.z/length};return true;
}
inline Matrix identity() noexcept {return {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};}
inline Matrix actor_basis(std::array<std::int32_t,3> const& raw) noexcept {
    constexpr double radians=6.28318530717958647692/65536;
    const auto basis=gears_vr::ue_matrix({double(raw[0]%65536)*radians,double(raw[1]%65536)*radians,double(raw[2]%65536)*radians});
    auto result=identity();
    // pose_math returns columns; native row-vector matrices store local axes
    // as rows. Keep that conversion explicit, including nonzero actor roll.
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)result[i*4+j]=static_cast<float>(basis[j][i]);
    return result;
}
inline bool rotation_axis(Input const& in,unsigned role,Point& out,bool& child_follows) noexcept {
    if(role>1)return false;const auto bone=in.control_bone[role];if(bone!=1&&bone!=2)return false;
    Matrix frame{};child_follows=false;
    switch(in.space[role]){
    case Space::world:frame=identity();break;
    case Space::actor:frame=actor_basis(in.actor_rotation);break;
    case Space::component:frame=in.component_world;break;
    case Space::parent_bone:frame=in.bone_world[bone==1?0:1];child_follows=bone==2;break;
    case Space::bone:frame=in.bone_world[bone];child_follows=bone==2;break;
    case Space::other_bone:{
        const auto other=in.other_bone[role];
        // Other frames below the second hinge can also carry spinner/recoil
        // animation. Their dependence is not this two-joint model. An original
        // parent control expressed in its child is likewise not independent.
        if(other>2||(bone==1&&other==2))return false;
        frame=in.bone_world[other];child_follows=bone==2&&other!=0;break;
    }
    default:return false;
    }
    return axis(frame,in.rotation_mask[role],out);
}
inline bool geometry(Input const& in,Geometry& output) noexcept {
    using namespace troika_solver_detail;
    if(!in.original_control_binding_and_reference_closed||!in.binding||!in.calibration||
       !std::isfinite(in.units_per_metre)||in.units_per_metre<10||in.units_per_metre>500||
       !std::isfinite(in.maximum_error_metres)||in.maximum_error_metres<=0||in.maximum_error_metres>.25||
       in.control_bone[0]==in.control_bone[1]||!rigid(in.bone_world[0])||!rigid(in.bone_world[1])||!rigid(in.bone_world[2]))return false;
    unsigned parent=2,child=2;
    for(unsigned role=0;role<2;++role){if(in.control_bone[role]==1)parent=role;else if(in.control_bone[role]==2)child=role;else return false;}
    if(parent==2||child==2)return false;
    Geometry next{};next.world_origin={in.bone_world[1][12],in.bone_world[1][13],in.bone_world[1][14]};
    next.units_per_metre=in.units_per_metre;next.parent_control=parent;next.child_control=child;
    const auto local=[&](Vector p){return Point{(p.x-next.world_origin.x)/in.units_per_metre,
        (p.y-next.world_origin.y)/in.units_per_metre,(p.z-next.world_origin.z)/in.units_per_metre};};
    auto& serial=next.serial;serial.binding=in.binding;serial.calibration=in.calibration;
    serial.parent.centre={};serial.child.centre=local({in.bone_world[2][12],in.bone_world[2][13],in.bone_world[2][14]});
    bool ignored{};
    if(!rotation_axis(in,parent,serial.parent.axis,ignored)||!rotation_axis(in,child,serial.child.axis,serial.child_axis_follows_parent)||
       !gears_vr::finite(in.anchors[0])||!gears_vr::finite(in.anchors[1]))return false;
    serial.left=local(in.anchors[0]);serial.right=local(in.anchors[1]);serial.max_hand_error_m=in.maximum_error_metres;
    // Search intervals only. ORIGINAL limits must be evaluated by the original
    // ProcessViewRotation/CanLookAtPoint consumer before an aim write is legal.
    serial.parent.minimum=serial.child.minimum=-pi;serial.parent.maximum=serial.child.maximum=pi;
    serial.original_hierarchy_axes_verified=true;
    const TroikaSerialCalibration check{serial.binding,serial.calibration,serial.parent,serial.child,serial.left,serial.right,
        serial.max_hand_error_m,false,serial.child_axis_follows_parent};
    if(!troika_serial_detail::geometry_valid(check))return false;
    output=next;return true;
}
inline bool propose(Geometry const& g,std::uint64_t ticket,std::array<Vector,2> const& targets,
    bool both_tracked,bool both_owned,TroikaSerialCandidate& output) noexcept {
    if(!g.serial.original_hierarchy_axes_verified||!std::isfinite(g.units_per_metre)||g.units_per_metre<10||g.units_per_metre>500)return false;
    const auto local=[&](Vector p){return Point{(p.x-g.world_origin.x)/g.units_per_metre,
        (p.y-g.world_origin.y)/g.units_per_metre,(p.z-g.world_origin.z)/g.units_per_metre};};
    TroikaSolverSample sample{g.serial.binding,g.serial.calibration,ticket,local(targets[0]),local(targets[1]),
        both_tracked,both_tracked,both_owned,both_owned,true};
    return propose_troika_serial_grips(g.serial,sample,output);
}
inline bool direction(Geometry const& g,TroikaSerialCandidate const& candidate,Vector original,Vector& output) noexcept {
    namespace d=troika_serial_detail;using namespace troika_solver_detail;
    if(candidate.binding!=g.serial.binding||candidate.calibration!=g.serial.calibration||
       !gears_vr::finite(original)||!candidate.fit.frame_ticket||!finite(candidate.fit.parent_angle)||!finite(candidate.fit.child_angle)||
       std::abs(candidate.fit.parent_angle)>pi||std::abs(candidate.fit.child_angle)>pi||
       !d::axis(g.serial.parent.axis)||!d::axis(g.serial.child.axis))return false;
    Point value=point(original);if(norm(value)<1e-12)return false;
    if(g.serial.child_axis_follows_parent)value=d::rotate_axis(d::rotate_axis(value,g.serial.child.axis,candidate.fit.child_angle),g.serial.parent.axis,candidate.fit.parent_angle);
    else value=d::rotate_axis(d::rotate_axis(value,g.serial.parent.axis,candidate.fit.parent_angle),g.serial.child.axis,candidate.fit.child_angle);
    const auto length=std::sqrt(norm(value));if(!finite(length)||length<1e-8)return false;
    output={value.x/length,value.y/length,value.z/length};return true;
}
} // namespace gears_weapon::troika_mechanics
