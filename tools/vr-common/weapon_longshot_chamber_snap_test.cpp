#include "weapon_longshot_chamber_snap.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <limits>
namespace s=gears_weapon_longshot_chamber_snap;using V=s::V;using P=s::P;
static constexpr double pi=3.14159265358979323846;
P cartridge(double depth,double lateral,double angle=0,double units=100){
 const auto a=-pi/2+angle;const gears_vr::Quaternion q{0,0,std::sin(a/2),std::cos(a/2)};
 const auto axis=gears_vr::transform(gears_vr::rotation_matrix(q),{0,.15*units,0});
 return {{-depth*units-axis.x,lateral*units-axis.y,-axis.z},q};
}
double distance(V a,V b){const auto d=s::sub(a,b);return std::hypot(std::hypot(d.x,d.y),d.z);}
P transform(P root,P p){return {s::add(root.position,gears_vr::transform(gears_vr::rotation_matrix(root.orientation),p.position)),gears_vr::normalized(gears_vr::multiply(root.orientation,p.orientation))};}
int main(){
#ifdef _WIN32
 _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
 unsigned cases{};
 for(double depth:{.08,.03,.005,-.006}){
  s::Alignment a{};assert(s::align(cartridge(depth,.04,.2),{}, {},1,100,false,a));
  const auto axis=gears_vr::transform(gears_vr::rotation_matrix(a.item.orientation),{0,1,0});
  const auto tip=s::add(a.item.position,s::mul(axis,15));
  assert(distance(axis,{1,0,0})<1e-12&&distance(tip,{-depth*100,0,0})<1e-12);
  assert(std::abs(a.raw_depth_m-depth)<1e-12&&std::abs(a.raw_lateral_m-.04)<1e-12);++cases;
 }
 for(unsigned fault=0;fault<9;++fault){
  auto raw=cartridge(.03,.04);P root{};V port{};double scale=1,units=100;
  switch(fault){case 0:raw=cartridge(.080001,0);break;case 1:raw=cartridge(-.015001,0);break;
   case 2:raw=cartridge(.03,.060001);break;case 3:raw=cartridge(.03,0,pi/6+.000001);break;
   case 4:raw.orientation={};raw.orientation.w=0;break;case 5:scale=0;break;
   case 6:units=0;break;case 7:port.x=NAN;break;case 8:raw=cartridge(.03,0,pi);break;}
  s::Alignment a{};a.raw_depth_m=99;assert(!s::align(raw,root,port,scale,units,false,a)&&a.raw_depth_m==99);++cases;
 }
 {s::Alignment a{};assert(!s::align(cartridge(.09,.07),{}, {},1,100,false,a));
  assert(s::align(cartridge(.09,.07),{}, {},1,100,true,a));
  assert(!s::align(cartridge(.100001,.07),{}, {},1,100,true,a));
  assert(!s::align(cartridge(.09,.080001),{}, {},1,100,true,a));++cases;}
 {const P root{{98,-42,170},{0,0,std::sin(.35),std::cos(.35)}};
  s::Alignment a{},b{};const auto raw=cartridge(.03,.04,.2);
  assert(s::align(raw,{}, {},1,100,false,a));assert(s::align(transform(root,raw),root,{},1,100,false,b));
  const auto expected=transform(root,a.item);assert(distance(expected.position,b.item.position)<1e-10);
  assert(std::abs(a.raw_depth_m-b.raw_depth_m)<1e-12);++cases;}
 {s::Alignment a{};assert(s::align(cartridge(.03,.04,0,200),{}, {},1,200,false,a));
  assert(std::abs(a.raw_depth_m-.03)<1e-12&&std::abs(a.raw_lateral_m-.04)<1e-12);++cases;}
 {s::Alignment a{};assert(!s::align(cartridge(.06,.08),{}, {},2,100,false,a));
  assert(s::align(cartridge(.06,.04),{}, {},2,100,false,a));
  assert(std::abs(a.raw_depth_m-.06)<1e-12&&std::abs(a.raw_lateral_m-.04)<1e-12);++cases;}
 {s::Alignment a{};const P mount{{3,4,5},{0,0,std::sin(.2),std::cos(.2)}};P grip{};
  assert(s::align(cartridge(.03,.04,.2),{}, {},1,100,false,a)&&s::grip_for_item(a.item,mount,grip));
  const auto restored=transform(grip,mount);assert(distance(restored.position,a.item.position)<1e-10);++cases;}
 {auto raw=cartridge(.03,.04);P root{};raw.position.x=std::numeric_limits<double>::max();root.position.x=-raw.position.x;
  s::Alignment a{};a.raw_depth_m=99;assert(!s::align(raw,root,{},1,100,false,a)&&a.raw_depth_m==99);++cases;}
 {P item{},mount{},out{};item.position.x=std::numeric_limits<double>::max();mount.position.x=-item.position.x;
  out.position.y=99;assert(!s::grip_for_item(item,mount,out)&&out.position.y==99);++cases;}
 std::printf("LongshotChamberSnap geometry:%u cases; raw axial depth preserved, entry/keep sizes, opposite/malformed rejection, transform/scale and exact item-grip contact; not composed/runtime insertion\n",cases);
}
