#pragma once
#include "weapon_boomshot_hand_mount.h"
#include "weapon_boomshot_insertion.h"
// World-space cosmetic geometry only. All positions come from the current
// authenticated gun/RAW Rig. Shapes neither enlarge contact nor drive parts.
namespace gears_boomshot_guide {
namespace vr=gears_vr;namespace m=gears_boomshot_hand_mount;
enum class Step:unsigned {none,opening,shoulder,entry,closing};
enum class Result:unsigned {none,opened,seated,closed};
struct View {Step step{};bool entered{};};
struct Line {vr::Vector start{},end{};unsigned char red{255},green{220},blue{};};
struct Shape {std::array<Line,40> lines{};unsigned count{};};
inline bool line(Shape& shape,vr::Vector a,vr::Vector b) noexcept {
    if(shape.count==shape.lines.size()||!vr::finite(a)||!vr::finite(b))return false;
    shape.lines[shape.count++]={a,b};return true;
}
inline vr::Vector add(vr::Vector a,vr::Vector b) noexcept{return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline bool ring(Shape& shape,vr::Vector center,vr::Vector x,vr::Vector y,double radius) noexcept {
    if(!std::isfinite(radius)||radius<=0)return false;
    const auto point=[&](double angle){return add(center,add(m::mul(x,radius*std::cos(angle)),m::mul(y,radius*std::sin(angle))));};
    for(unsigned i=0;i<12;++i)if(!line(shape,point(i*6.2831853071795864769/12),point((i+1)*6.2831853071795864769/12)))return false;
    return true;
}
inline bool arrow(Shape& shape,vr::Vector start,vr::Vector end,vr::Vector side,double head) noexcept {
    vr::Vector forward{},second{};if(!m::unit(m::sub(end,start),forward)||!m::unit(m::cross(forward,side),second))return false;
    const auto neck=add(end,m::mul(forward,-head));
    return line(shape,start,end)&&line(shape,end,add(neck,m::mul(side,head*.6)))&&line(shape,end,add(neck,m::mul(side,-head*.6)))&&
        line(shape,end,add(neck,m::mul(second,head*.6)))&&line(shape,end,add(neck,m::mul(second,-head*.6)));
}
inline bool make(View view,vr::Pose const& root,vr::Pose const& barrel,double scale,double units,
    vr::Pose const& shoulder,gears_weapon::boomshot_insertion::Geometry const& geometry,Shape& out) noexcept {
    out={};if(view.step==Step::none||!vr::stereo_pose_valid(root)||!vr::stereo_pose_valid(barrel)||
        !std::isfinite(scale)||scale<.01||scale>10||!std::isfinite(units)||units<10||units>500)return false;
    Shape value{};
    if(view.step==Step::shoulder){
        if(!vr::stereo_pose_valid(shoulder))return false;
        const auto y=m::rotate(shoulder.orientation,{0,1,0}),z=m::rotate(shoulder.orientation,{0,0,1});
        if(!ring(value,shoulder.position,y,z,.055*units)||
           !line(value,add(shoulder.position,m::mul(y,-.025*units)),add(shoulder.position,m::mul(y,.025*units)))||
           !line(value,add(shoulder.position,m::mul(z,-.025*units)),add(shoulder.position,m::mul(z,.025*units))))return false;
    }else if(view.step==Step::opening||view.step==Step::closing){
        const auto local=m::sub(m::mechanism_surface_closed_raw,gears_weapon::boomshot_barrel::closed_raw);
        const auto center=m::point(barrel,local,scale),x=m::rotate(barrel.orientation,{1,0,0}),z=m::rotate(barrel.orientation,{0,0,1});
        const auto direction=m::rotate(root.orientation,{0,view.step==Step::opening?-1.:1.,0});
        if(!ring(value,center,x,z,.025*units)||
           !arrow(value,center,add(center,m::mul(direction,.075*units)),x,.02*units))return false;
    }else if(view.step==Step::entry){
        if(!gears_weapon::boomshot_insertion::valid(geometry))return false;
        // The production RAW kernel divides by BOTH root scale and units.
        // Reverse that exact normalization for these world-space marks.
        const double root_units=scale*units;
        const auto seated=m::point(root,geometry.seated_in_root_m.position,root_units);
        const auto outward=m::rotate(root.orientation,geometry.outward);
        vr::Vector tangent{},bitangent{};
        const vr::Vector seed=std::abs(outward.z)<.9?vr::Vector{0,0,1}:vr::Vector{1,0,0};
        if(!m::unit(m::cross(outward,seed),tangent)||!m::unit(m::cross(outward,tangent),bitangent))return false;
        const auto mouth=add(seated,m::mul(outward,geometry.travel_m*root_units));
        const double radius=geometry.capture_radius_m*root_units;
        const auto seated_rotation=vr::normalized(vr::multiply(root.orientation,geometry.seated_in_root_m.orientation));
        const auto axis=m::rotate(seated_rotation,{1,0,0});vr::Vector key{};
        if(!m::unit(m::sub(axis,m::mul(outward,m::dot(axis,outward))),key))return false;
        if(!ring(value,mouth,tangent,bitangent,radius)||
           !line(value,add(mouth,m::mul(key,radius)),add(mouth,m::mul(key,1.5*radius)))||
           !arrow(value,add(mouth,m::mul(outward,.06*units)),mouth,tangent,.015*units))return false;
        if(view.entered){
            // The owner has accepted entry, not q1 or a seated result. A second
            // smaller ring distinguishes that state from the approach arrow.
            if(!ring(value,mouth,tangent,bitangent,.55*radius))return false;
            // Readable from an aimed/front view even when the XY mouth rings
            // collapse to a line. This check means ENTRY only, never seated/q1.
            const auto status=add(mouth,m::mul(outward,-.035*units));
            const auto a=add(status,m::mul(key,-.022*units));
            const auto b=add(status,add(m::mul(key,-.005*units),m::mul(outward,.012*units)));
            const auto c=add(status,add(m::mul(key,.022*units),m::mul(outward,-.02*units)));
            if(!line(value,a,b)||!line(value,b,c))return false;
        }
    }else return false;
    out=value;return value.count!=0;
}
inline bool result(Result kind,vr::Pose const& root,vr::Pose const& barrel,double scale,double units,
    gears_weapon::boomshot_insertion::Geometry const& geometry,Shape& out,vr::Quaternion const& observer) noexcept {
    if(kind==Result::none||out.count>out.lines.size()||!vr::stereo_pose_valid(root)||!vr::stereo_pose_valid(barrel)||
       !vr::stereo_pose_valid({{},observer})||!std::isfinite(scale)||scale<.01||scale>10||!std::isfinite(units)||units<10||units>500)return false;
    vr::Vector center{};
    if(kind==Result::seated){
        if(!gears_weapon::boomshot_insertion::valid(geometry))return false;
        center=m::point(root,geometry.seated_in_root_m.position,scale*units);
    }else if(kind==Result::opened||kind==Result::closed){
        center=m::point(barrel,m::sub(m::mechanism_surface_closed_raw,gears_weapon::boomshot_barrel::closed_raw),scale);
    }else return false;
    center=add(center,m::rotate(root.orientation,{0,0,.055*units}));
    // Only the cosmetic glyph faces the observer; its anchor remains the
    // CURRENT actual part. This orientation never reaches aim, skin or motion.
    const auto at=[&](double x,double z){return add(center,m::rotate(observer,{0,x*units,z*units}));};
    Shape mark{};
    if(kind==Result::seated){
        if(!line(mark,at(0,.025),at(.025,0))||!line(mark,at(.025,0),at(0,-.025))||
           !line(mark,at(0,-.025),at(-.025,0))||!line(mark,at(-.025,0),at(0,.025)))return false;
    }else if(kind==Result::opened){
        if(!line(mark,at(-.025,.015),at(0,-.015))||!line(mark,at(0,-.015),at(.025,.015)))return false;
    }else if(!line(mark,at(-.025,0),at(-.005,-.015))||!line(mark,at(-.005,-.015),at(.025,.025)))return false;
    if(mark.count>out.lines.size()-out.count)return false;
    for(unsigned i=0;i<mark.count;++i){auto edge=mark.lines[i];edge.red=40;edge.green=235;edge.blue=120;out.lines[out.count++]=edge;}
    return true;
}
}
