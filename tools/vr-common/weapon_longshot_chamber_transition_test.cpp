#include "weapon_longshot_chamber_snap.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <limits>
namespace s=gears_weapon_longshot_chamber_snap;using V=s::V;using P=s::P;
constexpr double pi=3.14159265358979323846;
P cartridge(double depth,double lateral,double angle,double units){
 const auto a=-pi/2+angle;const s::Q q{0,0,std::sin(a/2),std::cos(a/2)};
 const auto axis=s::vr::transform(s::vr::rotation_matrix(q),{0,1,0});
 return {s::sub({-depth*units,lateral*units,0},s::mul(axis,.15*units)),q};
}
P transform(P root,P p){return {s::add(root.position,s::vr::transform(s::vr::rotation_matrix(root.orientation),p.position)),
 s::vr::normalized(s::vr::multiply(root.orientation,p.orientation))};}
double distance(V a,V b){const auto d=s::sub(a,b);return std::hypot(std::hypot(d.x,d.y),d.z);}
int main(){
#ifdef _WIN32
 _set_error_mode(_OUT_TO_STDERR);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
 unsigned checks{};
 for(double units:{50.,100.,200.})for(double scale:{.5,1.,2.}){
  const P root{{98,-42,170},{0,0,std::sin(.35),std::cos(.35)}};
  const auto raw=transform(root,cartridge(.03,.04,.2,units));s::Correction c{};
  assert(s::make_correction(raw,root,{},scale,units,false,c));
  for(double t:{0.,.25,.5,.75,1.}){s::Alignment result{};
   assert(s::apply_correction(raw,root,{},scale,units,c,t,result));
   const auto tip=s::add(result.item.position,s::vr::transform(s::vr::rotation_matrix(result.item.orientation),{0,.15*units,0}));
   const auto local=s::mul(s::vr::transform(s::vr::transpose(s::vr::rotation_matrix(root.orientation)),s::sub(tip,root.position)),1/units);
   const double smooth=t*t*(3-2*t);
   assert(std::abs(local.x+.03)<1e-12&&std::abs(local.y-.04*(1-smooth))<1e-12);
   assert(std::abs(result.raw_depth_m-.03)<1e-12&&std::abs(result.raw_lateral_m-.04)<1e-12);
   if(t==0)assert(distance(raw.position,result.item.position)<1e-10);
   if(t==1){s::Alignment full{};assert(s::align(raw,root,{},scale,units,false,full));assert(distance(full.item.position,result.item.position)<1e-10);}
   P grip{};const P mount{{3,4,5},{0,0,0,1}};assert(s::grip_for_item(result.item,mount,grip));
   assert(distance(transform(grip,mount).position,result.item.position)<1e-10);++checks;
  }
 }
 const auto raw=cartridge(.03,.04,.2,100);s::Correction c{};assert(s::make_correction(raw,{}, {},1,100,false,c));
 {const auto outside=cartridge(.04,.09,0,100);s::Alignment result{};
  assert(!s::align(outside,{}, {},1,100,true,result));
  // A fading cosmetic correction may remain bounded outside contact; its
  // success is deliberately NOT a chamber-contact or insertion verdict.
  assert(s::apply_correction(outside,{}, {},1,100,c,.5,result));
  assert(std::abs(result.raw_depth_m-.04)<1e-12&&std::abs(result.raw_lateral_m-.09)<1e-12);++checks;}
 for(unsigned fault=0;fault<8;++fault){auto bad=c;double t=.5,units=100;P source=raw;
  switch(fault){case 0:t=-.1;break;case 1:t=1.1;break;case 2:t=std::numeric_limits<double>::quiet_NaN();break;
  case 3:bad.tip_offset_m.x=.000001;break;case 4:bad.tip_offset_m.y=.081;break;
  case 5:bad.rotation_in_root={0,0,0,0};break;case 6:units=0;break;case 7:source.position.x=NAN;break;}
  s::Alignment result{};result.raw_depth_m=99;
  assert(!s::apply_correction(source,{}, {},1,units,bad,t,result)&&result.raw_depth_m==99);++checks;
 }
 std::printf("LongshotChamberTransitionTest: %u checks; smooth correction preserves current physical axial depth and exact item/grip mount; no gameplay permission\n",checks);
}
