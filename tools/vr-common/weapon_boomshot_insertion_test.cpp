#include "weapon_boomshot_insertion.h"
#include <cassert>
#include <cstdio>
namespace insert=gears_weapon::boomshot_insertion;
insert::Geometry geometry(){insert::Geometry g{};g.id=9;g.outward={0,0,-1};g.travel_m=.1;g.approach_margin_m=.015;
 g.capture_radius_m=.01;g.retain_radius_m=.015;g.capture_angle_degrees=12;g.retain_angle_degrees=18;g.seat_tolerance_m=.002;g.verified=true;return g;}
insert::Sample sample(unsigned n,double depth){insert::Sample s{};s.stamp={{1,2,3,4},5,n,6,100+n,1000000000+40000000LL*n};
 s.raw_carrier_in_root_m.position={0,0,-depth};s.frame=n;s.lease=7;s.press=8;s.item=9;s.operation=10;s.geometry_id=9;
 s.grip_held=s.new_grant=s.left_tracked=s.right_tracked=s.lease_current=true;return s;}
int main(){auto g=geometry();
 {insert::Stroke stroke;auto s=sample(1,.13);assert(stroke.begin(g,s,s.stamp.monotonic_ns));
  s=sample(2,.1);assert(stroke.observe(s,s.stamp.monotonic_ns)==insert::Result::moved);
  s=sample(3,.05);assert(stroke.observe(s,s.stamp.monotonic_ns)==insert::Result::moved);
  s=sample(4,0);assert(stroke.observe(s,s.stamp.monotonic_ns)==insert::Result::moved);
  assert(stroke.observe(s,s.stamp.monotonic_ns)==insert::Result::waiting);
  s=sample(5,0);assert(stroke.observe(s,s.stamp.monotonic_ns)==insert::Result::complete&&!stroke.active());
  assert(stroke.began().frame==2);insert::Completion proof{},again{};
  assert(stroke.take_completion(proof)&&proof.valid()&&proof.began().frame==2&&proof.ended().frame==5&&
   proof.began().stamp.sequence==2&&proof.ended().stamp.monotonic_ns==s.stamp.monotonic_ns);
  assert(!stroke.take_completion(again)&&!again.valid());}
 {insert::Stroke stroke;auto s=sample(1,.01);assert(stroke.begin(g,s,s.stamp.monotonic_ns));
  for(unsigned i=2;i<6;++i){s=sample(i,0);assert(stroke.observe(s,s.stamp.monotonic_ns)==insert::Result::approaching);}}
 {insert::Stroke stroke;auto s=sample(1,.13);assert(stroke.begin(g,s,s.stamp.monotonic_ns));
  s=sample(2,0);assert(stroke.observe(s,s.stamp.monotonic_ns)==insert::Result::interrupted); // impossible jump
 }
 {insert::Stroke stroke;auto s=sample(1,.13);assert(stroke.begin(g,s,s.stamp.monotonic_ns));
  s=sample(2,.1);assert(stroke.observe(s,s.stamp.monotonic_ns)==insert::Result::moved);
  s=sample(3,.05);assert(stroke.observe(s,s.stamp.monotonic_ns)==insert::Result::moved);
  s=sample(4,.08);assert(stroke.observe(s,s.stamp.monotonic_ns)==insert::Result::moved); // reversible physical motion
  s=sample(5,.02);assert(stroke.observe(s,s.stamp.monotonic_ns)==insert::Result::moved);
  s=sample(6,0);assert(stroke.observe(s,s.stamp.monotonic_ns)==insert::Result::moved);
  s=sample(7,0);assert(stroke.observe(s,s.stamp.monotonic_ns)==insert::Result::complete);}
 for(unsigned reason=0;reason<14;++reason){insert::Stroke stroke;auto s=sample(1,.13);assert(stroke.begin(g,s,s.stamp.monotonic_ns));
  s=sample(2,.1);assert(stroke.observe(s,s.stamp.monotonic_ns)==insert::Result::moved);s=sample(3,.05);
  switch(reason){case 0:s.stamp.identity.weapon++;break;case 1:s.stamp.binding++;break;case 2:s.stamp.motion_epoch++;break;
    case 3:s.lease++;break;case 4:s.press++;break;case 5:s.item++;break;case 6:s.operation++;break;
    case 7:s.grip_held=false;break;case 8:s.left_tracked=false;break;case 9:s.right_tracked=false;break;
    case 10:s.lease_current=false;break;case 11:s.raw_carrier_in_root_m.position.x=.016;break;
    case 12:s.stamp.monotonic_ns+=100000001;break;}
  if(reason==13)++s.geometry_id;
  assert(stroke.observe(s,s.stamp.monotonic_ns)==insert::Result::interrupted&&!stroke.active());}
 {auto bad=g;bad.verified=false;insert::Stroke stroke;auto s=sample(1,.13);assert(!stroke.begin(bad,s,s.stamp.monotonic_ns));
  s.new_grant=false;assert(!stroke.begin(g,s,s.stamp.monotonic_ns));}
 {insert::Stroke stroke;auto s=sample(1,.13);assert(stroke.begin(g,s,s.stamp.monotonic_ns));
  s=sample(2,.1);assert(stroke.observe(s,s.stamp.monotonic_ns)==insert::Result::moved);
  s=sample(3,.05);s.raw_carrier_in_root_m.orientation={0,0,std::sin(.2),std::cos(.2)};
  assert(stroke.observe(s,s.stamp.monotonic_ns)==insert::Result::interrupted);}
 {insert::Stroke stroke;auto s=sample(1,.13);assert(stroke.begin(g,s,s.stamp.monotonic_ns));stroke.release();
  assert(!stroke.begin(g,s,s.stamp.monotonic_ns));++s.press;++s.lease;assert(stroke.begin(g,s,s.stamp.monotonic_ns));}
 {insert::Stroke stroke;auto s=sample(1,.13);assert(stroke.begin(g,s,s.stamp.monotonic_ns));
  s=sample(2,.1);assert(stroke.observe(s,s.stamp.monotonic_ns)==insert::Result::moved);
  s=sample(3,.05);assert(stroke.observe(s,s.stamp.monotonic_ns)==insert::Result::moved);
  s=sample(4,0);s.raw_carrier_in_root_m.position.x=.012;assert(stroke.observe(s,s.stamp.monotonic_ns)==insert::Result::moved);
  s=sample(5,0);s.raw_carrier_in_root_m.position.x=.012;assert(stroke.observe(s,s.stamp.monotonic_ns)==insert::Result::waiting);
  s=sample(6,0);assert(stroke.observe(s,s.stamp.monotonic_ns)==insert::Result::waiting);
  s=sample(7,0);assert(stroke.observe(s,s.stamp.monotonic_ns)==insert::Result::complete);}
 std::puts("Boomshot insertion: mandatory certified geometry, fresh non-replayed grant, outside/entry/raw travel, strict later-frame seat, reversal and14 interrupted sources passed; synthetic geometry only");
}
