#include "weapon_longshot_guide_geometry.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
namespace g=gears_weapon_longshot_guide;namespace v=gears_vr;
int main(){
#ifdef _WIN32
 _set_error_mode(_OUT_TO_STDERR);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
 for(double model_scale:{.5,1.,2.})for(double units:{10.,100.,500.})for(v::Quaternion q:{v::Quaternion{},v::normalized({.2,.3,.1,.8})}){
  const v::Pose root{{31,-8,23},q};const v::Vector port{.26,0,.105};g::Shape shape{};
  assert(g::chamber(root,port,model_scale,units,shape)&&shape.count==31);
  const auto inv=v::rotation_matrix(v::inverse(q));
  const auto local=[&](v::Vector p){return g::scale(v::transform(inv,g::sub(p,root.position)),1/units);};
  for(unsigned i=0;i<24;++i){const auto p=g::sub(local(shape.lines[3+i].start),g::scale(port,model_scale));const unsigned ring=i/8;
   assert(std::abs(p.x-(ring==1?-.08:0))<1e-9);
   assert(std::abs(std::hypot(p.y,p.z)-(ring==0?.015:.06))<1e-9);}
  const auto direction=g::sub(local(shape.lines[0].end),local(shape.lines[0].start));
  assert(direction.x>.079999&&std::abs(direction.y)<1e-9&&std::abs(direction.z)<1e-9);
 }
 // The shoulder cue depicts ONE15cm cartridge and points outwards. Its
 // physical dimensions survive both game-unit and body/world rotations.
 for(double units:{10.,100.,500.})for(v::Quaternion q:{v::Quaternion{},v::normalized({.2,.3,.1,.8})}){
  const v::Pose shoulder{{-15,22,7},q};g::Shape cue{};
  assert(g::single_round(shoulder,units,cue)&&cue.count==17);
  const auto inv=v::rotation_matrix(v::inverse(q));
  const auto local=[&](v::Vector p){return g::scale(v::transform(inv,g::sub(p,shoulder.position)),1/units);};
  auto a=local(cue.lines[0].start),b=local(cue.lines[0].end);
  assert(std::abs(a.x+.04)<1e-9&&std::abs(b.x-.04)<1e-9&&std::abs(a.z+.035)<1e-9&&std::abs(b.z+.035)<1e-9);
  double rear=1,tip=-1,radius{};
  for(unsigned i=3;i<cue.count;++i)for(const auto point:{cue.lines[i].start,cue.lines[i].end}){
   const auto p=local(point);rear=(std::min)(rear,p.x);tip=(std::max)(tip,p.x);radius=(std::max)(radius,std::hypot(p.y,p.z));
  }
  assert(std::abs(tip-rear-gears_weapon_longshot_chamber_snap::cartridge_length_m)<1e-9&&std::abs(radius-.007)<1e-9);
  g::Shape unavailable{};assert(g::no_round(shoulder,units,unavailable)&&unavailable.count==16);
  const auto strike_start=local(unavailable.lines[0].start),strike_end=local(unavailable.lines[0].end);
  assert(std::abs(strike_start.x+.09)<1e-9&&std::abs(strike_start.z+.025)<1e-9&&std::abs(strike_end.x-.09)<1e-9&&std::abs(strike_end.z-.025)<1e-9);
  for(unsigned i=2;i<unavailable.count;++i){const auto actual_line=unavailable.lines[i],expected_line=cue.lines[i+1];
   assert(actual_line.start.x==expected_line.start.x&&actual_line.start.y==expected_line.start.y&&actual_line.start.z==expected_line.start.z&&actual_line.end.x==expected_line.end.x&&actual_line.end.y==expected_line.end.y&&actual_line.end.z==expected_line.end.z);}
 }
 g::Shape sentinel{};sentinel.count=31;assert(!g::single_round({},0,sentinel)&&sentinel.count==31);
 auto invalid=v::Pose{};invalid.position.x=std::numeric_limits<double>::quiet_NaN();
 assert(!g::single_round(invalid,100,sentinel)&&sentinel.count==31);
 assert(!g::no_round(invalid,100,sentinel)&&sentinel.count==31);
 g::Shape out{};assert(!g::arrow({}, {},{0,1,0},.01,out)&&out.count==0);
 assert(!g::arrow({}, {1,0,0},{1,0,0},.01,out)&&out.count==0);
 assert(!g::chamber({}, {},1,0,out)&&out.count==0);
 std::puts("Longshot guide geometry: chamber acquisition constants, real approach/seated dimensions, single15cm shoulder round/extraction arrow and world-basis invariance passed; cosmetic only, no visible-game acceptance");
}
