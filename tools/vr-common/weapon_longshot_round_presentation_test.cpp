#include "weapon_longshot_round_presentation.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
namespace p=gears_weapon_longshot_round_presentation;namespace g=p::geometry;
using P=p::P;using V=p::V;
P cartridge(double depth,double lateral){const double a=-3.14159265358979323846/2;
 const g::Q q{0,0,std::sin(a/2),std::cos(a/2)};
 return {g::sub({-depth*100,lateral*100,0},gears_vr::transform(gears_vr::rotation_matrix(q),{0,15,0})),q};}
V tip(P const& item){return g::add(item.position,gears_vr::transform(gears_vr::rotation_matrix(item.orientation),{0,15,0}));}
double distance(V a,V b){const auto d=g::sub(a,b);return std::hypot(std::hypot(d.x,d.y),d.z);}
struct Fixture {
 p::State state; p::hand::Frame f{};p::hand::Lease lease{p::hand::LeaseKind::ammo,0,1,3};
 P mount{{3,4,5},{0,0,0,1}};P raw=cartridge(.03,.04);
 Fixture(){f.key.frame=2;f.key.lifecycle=1;f.press_token=3;f.controllers.sequence=4;f.controllers.sync_ns=1000000000;
  f.left_tracked=f.left_grip_known=f.left_grip_down=1;}
 bool prepare(std::int64_t gap=0){if(gap){++f.key.frame;++f.controllers.sequence;f.controllers.sync_ns+=gap;}
  return state.prepare(f,lease,lease.id,1,7,raw,mount,{}, {},1,100);}
 void settle(){assert(prepare());assert(prepare(60000000));assert(prepare(60000000));assert(state.sample().weight==1);}
};
int main(){
#ifdef _WIN32
 _set_error_mode(_OUT_TO_STDERR);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
 unsigned checks{};
 {Fixture v;assert(v.prepare());assert(v.state.sample().inside&&v.state.sample().weight==0);
  assert(v.prepare()&&v.state.sample().weight==0);++checks;
  assert(v.prepare(60000000));auto const& mid=v.state.sample();
  assert(std::abs(mid.weight-.5)<1e-12&&std::abs(tip(mid.display).y-2)<1e-10);
  assert(std::abs(mid.measurement.raw_depth_m-.03)<1e-12&&std::abs(mid.measurement.raw_lateral_m-.04)<1e-12);++checks;
  assert(v.prepare(60000000));auto const& full=v.state.sample();assert(full.weight==1&&std::abs(tip(full.display).y)<1e-10);
  const auto mapped=g::add(full.grip.position,gears_vr::transform(gears_vr::rotation_matrix(full.grip.orientation),v.mount.position));
  assert(distance(mapped,full.display.position)<1e-10&&v.state.matches(v.f,v.lease,3,1,7));++checks;}
 {Fixture v;v.settle();v.raw=cartridge(.04,.07);assert(v.prepare(20000000)&&v.state.sample().inside);++checks;
  v.raw=cartridge(.04,.09);assert(v.prepare(20000000)&&!v.state.sample().inside&&v.state.sample().weight==1);++checks;
  assert(v.prepare(60000000)&&std::abs(v.state.sample().weight-.5)<1e-12);
  assert(std::abs(v.state.sample().measurement.raw_depth_m-.04)<1e-12);++checks;
  assert(v.prepare(60000000)&&v.state.sample().weight==0&&distance(v.state.sample().raw.position,v.state.sample().display.position)<1e-10);++checks;}
 {Fixture v;v.raw=cartridge(.04,.07);assert(v.prepare()&&!v.state.sample().inside&&v.state.sample().weight==0);++checks;}
 {Fixture v;v.settle();v.raw=cartridge(.04,.07);assert(v.prepare(100000001)&&!v.state.sample().inside&&v.state.sample().weight==0);++checks;}
 {Fixture v;v.settle();v.raw=cartridge(.04,.04);assert(v.prepare(100000001)&&v.state.sample().inside&&v.state.sample().weight==0);++checks;}
 for(unsigned changed=0;changed<8;++changed){Fixture v;v.settle();
  switch(changed){case 0:++v.f.reference_generation;break;case 1:++v.f.stereo.session;break;case 2:++v.f.stereo.space_epoch;break;
  case 3:++v.f.controllers.input_epoch;break;case 4:++v.f.controllers.profile_generation;break;case 5:++v.f.key.registration_cookie;break;
  case 6:++v.f.key.world_generation;break;case 7:++v.lease.id;v.f.press_token=v.lease.id;break;}
  assert(v.prepare(20000000)&&v.state.sample().weight==0);++checks;}
 for(unsigned fault=0;fault<10;++fault){Fixture v;assert(v.prepare());
  switch(fault){case 0:v.f.left_grip_down=0;break;case 1:v.f.left_tracked=0;break;case 2:v.f.left_grip_known=0;break;
  case 3:v.lease.kind=p::hand::LeaseKind::support;break;case 4:++v.lease.lifecycle;break;case 5:++v.f.press_token;break;
  case 6:--v.f.controllers.sequence;--v.f.controllers.sync_ns;break;case 7:--v.f.key.frame;break;
  case 8:v.raw.position.x+=1;break;case 9:v.mount.position.x+=1;break;}
  assert(!v.prepare()&&!v.state.sample().valid);++checks;}
 std::printf("LongshotRoundPresentationTest: %u checks; one raw/display/grip sample, hysteresis,120ms fade, context/replay/source loss; no inventory permission\n",checks);
}
