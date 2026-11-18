#pragma once
#include "stereo_frame.h"
#include "weapon_longshot_chamber_snap.h"
// Cosmetic geometry only. These lines cannot supply contact, insertion,
// progress, timing or a hand/action lease.
namespace gears_weapon_longshot_guide {
using V=gears_vr::Vector;using P=gears_vr::Pose;
struct Line {V start{},end{};};
struct Shape {std::array<Line,32> lines{};unsigned count{};};
inline V add(V a,V b)noexcept{return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline V sub(V a,V b)noexcept{return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline V scale(V a,double n)noexcept{return {a.x*n,a.y*n,a.z*n};}
inline double dot(V a,V b)noexcept{return a.x*b.x+a.y*b.y+a.z*b.z;}
inline bool normalize(V& p)noexcept{const double n=std::hypot(std::hypot(p.x,p.y),p.z);
 if(!gears_vr::finite(p)||!std::isfinite(n)||n<1e-8)return false;p=scale(p,1/n);return true;}
inline bool arrow(V from,V to,V side,double head,Shape& out)noexcept {
 V forward=sub(to,from);if(!gears_vr::finite(from)||!gears_vr::finite(to)||!normalize(forward)||
    !std::isfinite(head)||head<=0||!normalize(side)||std::abs(dot(side,forward))>.001||out.count>29)return false;
 const auto base=add(to,scale(forward,-head));
 out.lines[out.count++]={from,to};out.lines[out.count++]={to,add(base,scale(side,head*.55))};
 out.lines[out.count++]={to,add(base,scale(side,-head*.55))};return true;
}
inline bool chamber(P const& root,V port_metres,double model_scale,double units,Shape& out)noexcept {
 if(!gears_vr::stereo_pose_valid(root)||!gears_vr::finite(port_metres)||!std::isfinite(model_scale)||
    model_scale<=.1||model_scale>=10||!std::isfinite(units)||units<10||units>500)return false;
 Shape result{};const double u=model_scale*units;const auto matrix=gears_vr::rotation_matrix(root.orientation);
 // The port follows the model; acquisition/seat dimensions are physical cm.
 const auto world=[&](V p){return add(root.position,gears_vr::transform(matrix,add(scale(port_metres,u),scale(p,units))));};
 // The ordinary renderer has a bounded per-frame cue budget. Put direction
 // and seated alignment first so a partial batch still explains the action.
 constexpr double depth=gears_weapon_longshot_chamber_snap::enter_depth_m;
 constexpr double radius=gears_weapon_longshot_chamber_snap::enter_radius_m;
 if(!arrow(world({-depth,0,0}),world({0,0,0}),gears_vr::transform(matrix,{0,0,1}),.015*units,result))return false;
 // Approach depth8cm/radius6cm and the actual seated lateral1.5cm. No
 // display Snap alters the separately measured physical cartridge tip.
 for(unsigned ring=0;ring<3;++ring){const double ring_radius=ring==0?.015:radius,ring_depth=ring==1?-depth:0;
  for(unsigned i=0;i<8;++i){const double a=i*3.14159265358979323846/4,b=(i+1)*3.14159265358979323846/4;
   result.lines[result.count++]={world({ring_depth,ring_radius*std::cos(a),ring_radius*std::sin(a)}),world({ring_depth,ring_radius*std::cos(b),ring_radius*std::sin(b)})};
  }
 }
 for(unsigned i=0;i<4;++i){const double a=i*3.14159265358979323846/2;
  result.lines[result.count++]={world({-depth,radius*std::cos(a),radius*std::sin(a)}),world({0,radius*std::cos(a),radius*std::sin(a)})};}
 out=result;return true;
}
// One cartridge at the existing shoulder target, plus an extraction arrow.
// The glyph is not a spawned item and cannot satisfy shoulder capture.
inline bool single_round(P const& shoulder,double units,Shape& out)noexcept {
 if(!gears_vr::stereo_pose_valid(shoulder)||!std::isfinite(units)||units<10||units>500)return false;
 Shape result{};const auto matrix=gears_vr::rotation_matrix(shoulder.orientation);
 const auto world=[&](V p){return add(shoulder.position,gears_vr::transform(matrix,scale(p,units)));};
 const double half=gears_weapon_longshot_chamber_snap::cartridge_length_m/2;
 const std::array<std::array<double,2>,7> profile{{{-half,-.007},{half-.04,-.007},{half-.02,-.004},
     {half,0},{half-.02,.004},{half-.04,.007},{-half,.007}}};
 // Direction first: a time-bounded partial batch still points out of the pouch.
 if(!arrow(world({-.04,0,-.035}),world({.04,0,-.035}),gears_vr::transform(matrix,{0,0,1}),.015*units,result))return false;
 for(unsigned plane=0;plane<2;++plane)for(unsigned i=0;i<profile.size();++i){
  const auto a=profile[i],b=profile[(i+1)%profile.size()];
  result.lines[result.count++]={world({a[0],plane?0:a[1],plane?a[1]:0}),world({b[0],plane?0:b[1],plane?b[1]:0})};
 }
 out=result;return true;
}
inline bool no_round(P const& shoulder,double units,Shape& out)noexcept {
 Shape cartridge{};if(!single_round(shoulder,units,cartridge))return false;
 Shape result{};
 // Strike first; a partial drawing must not look like a normal pickup cue.
 const auto matrix=gears_vr::rotation_matrix(shoulder.orientation);
 const auto world=[&](V p){return add(shoulder.position,gears_vr::transform(matrix,scale(p,units)));};
 result.lines[result.count++]={world({-.09,0,-.025}),world({.09,0,.025})};
 result.lines[result.count++]={world({-.09,0,.025}),world({.09,0,-.025})};
 // Copy only the cartridge outline, omitting its extraction arrow.
 for(unsigned i=3;i<cartridge.count;++i)result.lines[result.count++]=cartridge.lines[i];
 out=result;return true;
}
}
