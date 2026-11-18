#pragma once
#include "weapon_troika_mechanics.h"
#include "weapon_troika_fire_owner.h"

// Post-write observed gun geometry. This is neither an engine-call receipt nor
// a C0 hand/skin certificate and cannot authorize original firing by itself.
namespace gears_weapon::troika_gun_consumption {
namespace math=troika_mechanics;namespace fire=gears_troika_fire;
namespace d=troika_serial_detail;namespace old=troika_solver_detail;
using Vector=gears_vr::Vector;using Matrix=math::Matrix;
struct Prediction {std::array<Matrix,6> bones{};double units{};std::uint64_t binding{},calibration{},ticket{};};
struct Match {double maximum_position_error_metres{},maximum_rotation_error_radians{};};
inline bool later(fire::Run const& previous,fire::Run const& current,std::int64_t now) noexcept {
    gears_troika_geometry::Request a{},b{};a.source=previous.source;a.query_id=previous.control.query_id;
    b.source=current.source;b.query_id=current.control.query_id;
    auto const& x=previous.source.frame;auto const& y=current.source.frame;
    return gears_troika_control::valid(previous.control,a,now)&&gears_troika_control::valid(current.control,b,now)&&
        previous.control.both_owned&&current.control.both_owned&&fire::same_input_life(previous.source,current.source)&&
        fire::hand::same(previous.control.lease,current.control.lease)&&previous.control.press_token==current.control.press_token&&
        previous.control.right_press_sequence==current.control.right_press_sequence&&
        previous.control.right_window_begin_ns==current.control.right_window_begin_ns&&
        y.publication_sequence>x.publication_sequence&&y.scene_invocation>x.scene_invocation&&y.scene_qpc>x.scene_qpc&&
        y.published_ns>x.published_ns&&y.controllers.sequence>=x.controllers.sequence&&y.controllers.sync_ns>=x.controllers.sync_ns;
}
inline bool predict(math::Geometry const& g,TroikaSerialCandidate const& candidate,
    std::array<Matrix,6> const& before,Prediction& output) noexcept {
    Vector check{};if(!math::direction(g,candidate,{1,0,0},check)||!std::isfinite(g.units_per_metre)||
        g.units_per_metre<10||g.units_per_metre>500||!g.serial.original_hierarchy_axes_verified)return false;
    Prediction next{};next.units=g.units_per_metre;next.binding=candidate.binding;next.calibration=candidate.calibration;next.ticket=candidate.fit.frame_ticket;
    const TroikaSerialCalibration geometry{g.serial.binding,g.serial.calibration,g.serial.parent,g.serial.child,
        g.serial.left,g.serial.right,g.serial.max_hand_error_m,false,g.serial.child_axis_follows_parent};
    if(!d::geometry_valid(geometry))return false;
    for(unsigned bone=0;bone<6;++bone){
        if(!math::rigid(before[bone]))return false;next.bones[bone]=before[bone];if(!bone)continue;
        auto position=math::point({(before[bone][12]-g.world_origin.x)/next.units,
            (before[bone][13]-g.world_origin.y)/next.units,(before[bone][14]-g.world_origin.z)/next.units});
        position=bone==1?d::about(position,geometry.parent,candidate.fit.parent_angle):
            d::forward(geometry,position,candidate.fit.parent_angle,candidate.fit.child_angle);
        for(unsigned n=0;n<3;++n){
            const Vector row{before[bone][n*4],before[bone][n*4+1],before[bone][n*4+2]};Vector moved{};
            if(bone==1)moved=math::vector(d::rotate_axis(math::point(row),geometry.parent.axis,candidate.fit.parent_angle));
            else {if(!math::direction(g,candidate,row,moved))return false;
                const double length=std::sqrt(row.x*row.x+row.y*row.y+row.z*row.z);moved={moved.x*length,moved.y*length,moved.z*length};}
            next.bones[bone][n*4]=float(moved.x);next.bones[bone][n*4+1]=float(moved.y);next.bones[bone][n*4+2]=float(moved.z);
        }
        next.bones[bone][12]=float(g.world_origin.x+position.x*next.units);
        next.bones[bone][13]=float(g.world_origin.y+position.y*next.units);
        next.bones[bone][14]=float(g.world_origin.z+position.z*next.units);
        if(!math::rigid(next.bones[bone]))return false;
    }
    output=next;return true;
}
inline bool matches(Prediction const& expected,std::array<Matrix,6> const& actual,Match& output) noexcept {
    if(!expected.binding||!expected.calibration||!expected.ticket||!std::isfinite(expected.units)||expected.units<10||expected.units>500)return false;
    Match next{};
    for(unsigned bone=0;bone<6;++bone){const auto& a=expected.bones[bone];const auto& b=actual[bone];
        if(!math::rigid(a)||!math::rigid(b))return false;
        const double dx=(double(a[12])-b[12])/expected.units,dy=(double(a[13])-b[13])/expected.units,dz=(double(a[14])-b[14])/expected.units;
        const double position=std::sqrt(dx*dx+dy*dy+dz*dz);next.maximum_position_error_metres=(std::max)(next.maximum_position_error_metres,position);
        double trace{};
        for(unsigned row=0;row<3;++row){double dot{},aa{},bb{};for(unsigned n=0;n<3;++n){dot+=double(a[row*4+n])*b[row*4+n];aa+=double(a[row*4+n])*a[row*4+n];bb+=double(b[row*4+n])*b[row*4+n];}
            if(std::abs(aa-bb)>aa*1e-4)return false;trace+=dot/std::sqrt(aa*bb);}
        const double rotation=std::acos(std::clamp((trace-1)*.5,-1.,1.));
        next.maximum_rotation_error_radians=(std::max)(next.maximum_rotation_error_radians,rotation);
        if(!std::isfinite(position)||!std::isfinite(rotation)||position>.002||rotation>.008726646259971648)return false;
    }
    output=next;return true;
}
// Relative socket translation is transformed by the ACTUAL selected muzzle
// bone; the ORIGINAL world getter and physical-start getter must agree too.
inline bool selected_socket(Matrix const& bone,std::array<float,3> const& relative,Vector getter,Vector physical,double units) noexcept {
    if(!math::rigid(bone)||!gears_vr::finite(getter)||!gears_vr::finite(physical)||!std::isfinite(units)||units<10||units>500)return false;
    for(float value:relative)if(!std::isfinite(value))return false;
    const Vector transformed{bone[12]+relative[0]*bone[0]+relative[1]*bone[4]+relative[2]*bone[8],
        bone[13]+relative[0]*bone[1]+relative[1]*bone[5]+relative[2]*bone[9],
        bone[14]+relative[0]*bone[2]+relative[1]*bone[6]+relative[2]*bone[10]};
    const auto close=[&](Vector a,Vector b){const double dx=(a.x-b.x)/units,dy=(a.y-b.y)/units,dz=(a.z-b.z)/units;
        return dx*dx+dy*dy+dz*dz<=.00000025;}; // 0.5mm; no guessed central muzzle.
    return close(transformed,getter)&&close(getter,physical);
}
inline bool socket_rotation(Matrix const& bone,std::array<std::int32_t,3> const& relative,
    std::array<std::int32_t,3> const& getter) noexcept {
    if(!math::rigid(bone))return false;const auto local=math::actor_basis(relative),world=math::actor_basis(getter);
    double trace{};
    for(unsigned row=0;row<3;++row){Vector expected{};
        expected.x=local[row*4]*bone[0]+local[row*4+1]*bone[4]+local[row*4+2]*bone[8];
        expected.y=local[row*4]*bone[1]+local[row*4+1]*bone[5]+local[row*4+2]*bone[9];
        expected.z=local[row*4]*bone[2]+local[row*4+1]*bone[6]+local[row*4+2]*bone[10];
        const double length=std::sqrt(expected.x*expected.x+expected.y*expected.y+expected.z*expected.z);
        if(!std::isfinite(length)||length<1e-8)return false;
        trace+=(expected.x*world[row*4]+expected.y*world[row*4+1]+expected.z*world[row*4+2])/length;
    }
    const double error=std::acos(std::clamp((trace-1)*.5,-1.,1.));
    return std::isfinite(error)&&error<=.0005; // Rotator quantization and float world matrices.
}
} // namespace gears_weapon::troika_gun_consumption
