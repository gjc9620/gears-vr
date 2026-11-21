#include "weapon_boomshot_contact_geometry.h"
#define now fixture_now
#define main existing_rig_binding_regressions
#include "weapon_rig_binding_test.cpp"
#undef main
#undef now
namespace bc=gears_boomshot_contact;
namespace ep=gears_committed_hand;
namespace {
struct ContactFixture {
 Fixture tracking{};ep::Frame actual{};bc::Measurement measured{};
 ContactFixture(){auto const& f=tracking.hand;auto const& r=tracking.rig;
  actual.parent=f.key;actual.world=f.world;actual.controller=f.controller;actual.pawn=f.pawn;actual.mesh=f.mesh;actual.weapon=f.weapon;
  actual.serial=17;actual.tick=18;actual.compose=19;actual.hold_generation=1;
  actual.binding_generation=r.binding_generation;actual.reference_generation=r.reference_generation;
  actual.native_reference=r.native_reference;actual.producer=r.producer;
  actual.acquisition_begin_ns=fixture_now-4000000;actual.acquisition_end_ns=fixture_now-3000000;
  actual.committed_ns=fixture_now-2000000;actual.returned_ns=fixture_now-1000000;
  actual.stereo=f.stereo;actual.controllers=f.controllers;actual.geometry=gears_hand_authored_mount::marcus115_signature;
  actual.units_per_metre=r.units_per_metre;actual.right_grip=r.world_grip[1];
  measured.source=f;measured.committed=actual;measured.component=object(0x1500,6);measured.asset=object(0x1600,7);
  measured.root={{50,60,70},{0,0,.6,.8}};measured.barrel_relative.position=gears_weapon::boomshot_barrel::closed_raw;
  measured.scale=1;measured.measured_ns=fixture_now-500000;measured.known=true;
 }
 bool run(bc::Geometry& out,std::int64_t stamp=fixture_now) const {
  return bc::evaluate(measured,tracking.hand,tracking.rig,actual,1,stamp,out);
 }
};
}
using uptr=std::uintptr_t;
namespace contact_boundary {
ContactFixture* active{};unsigned failure{},part_reads{},endpoint_reads{};
void reset(ContactFixture& f,unsigned reason=0){active=&f;failure=reason;part_reads=endpoint_reads=0;}
}
namespace weaponhand {
unsigned current_gt(void*) noexcept{return contact_boundary::failure==1?0u:1u;}
std::int64_t clock_ns(void*) noexcept{return fixture_now;}
}
namespace vrequip {struct ContactModel {std::uint64_t hold_generation()const noexcept{return 1;}} model;}
namespace nativeequipment {bool current(void*,wh::Frame const&) noexcept{return contact_boundary::failure!=14;}}
namespace boomshot_carrier {bool selected(wh::Frame const&) noexcept{return contact_boundary::failure!=2;}}
namespace hammerburst_animation {struct Parts {wh::ObjectIdentity component{},asset{};std::array<std::array<float,16>,3> world{};};}
namespace handrig {
bool read_endpoint(ep::Frame& out) noexcept {
 ++contact_boundary::endpoint_reads;
 if(contact_boundary::failure==3||(contact_boundary::failure==12&&contact_boundary::endpoint_reads==2))return false;
 out=contact_boundary::active->actual;
 if(contact_boundary::failure==13&&contact_boundary::endpoint_reads==2)++out.serial;
 return true;
}
struct ReplyResult {rt::Reply reply{};};
ReplyResult read(rt::Frame& out,std::uint64_t,unsigned) noexcept {
 out=contact_boundary::active->tracking.rig;ReplyResult result{};
 result.reply=contact_boundary::failure==4?rt::Reply::unavailable:rt::Reply::accepted;return result;
}
}
namespace boomshot_parts {
bool read(wh::Frame const&,hammerburst_animation::Parts& out) noexcept {
 const auto call=++contact_boundary::part_reads,reason=contact_boundary::failure;
 if((reason==5&&call==1)||(reason==6&&call==2))return false;
 out.component=contact_boundary::active->measured.component;out.asset=contact_boundary::active->measured.asset;
 out.world[0][0]=1;out.world[2][0]=2;
 if(reason==9&&call==2)out.world[0][1]=1;
 if(reason==10&&call==2)++out.component.registry_index;
 if(reason==11&&call==2)++out.asset.registry_index;
 return true;
}
}
namespace hammerburst_runtime {
bool matrix(std::array<float,16> const& value,bc::P& out,double& scale) noexcept {
 if(contact_boundary::failure==7)return false;
 auto const& m=contact_boundary::active->measured;scale=m.scale;out=m.root;
 if(value[0]==2){out.position=bc::point(m.root,m.barrel_relative.position,scale);
  out.orientation=gears_vr::multiply(m.root.orientation,m.barrel_relative.orientation);
  if(contact_boundary::failure==8)scale+=.01;
 }
 return true;
}
}
#include "weapon_boomshot_contact.inl"
namespace boomshot_pose {
struct Source {std::array<std::array<float,16>,3> world{};std::int64_t observed_begin_ns{};};
bool ready(wh::Frame const&,Source& out) noexcept {
 out={};if(contact_boundary::failure==15)return false;
 out.world[0][0]=1;out.world[2][0]=2;out.observed_begin_ns=fixture_now-500000;
 return true;
}
}
#include "weapon_boomshot_contact_current.inl"
void current_observation_cases(){
 {ContactFixture f;contact_boundary::reset(f);bc::Geometry g{};
  CHECK(boomshot_contact::observe(f.tracking.hand,g));
  CHECK(!contact_boundary::part_reads&&!contact_boundary::endpoint_reads&&!g.endpoint_serial);
  CHECK(g.geometry_acquired_ns==fixture_now-500000&&g.input_sync_ns==f.tracking.hand.controllers.sync_ns);
  const auto old=g.surface_world;f.measured.root.position.x+=7;
  ++f.tracking.hand.key.frame;++f.tracking.rig.scene_invocation;
  CHECK(boomshot_contact::observe(f.tracking.hand,g)&&std::abs(g.surface_world.x-old.x-7)<1e-9);
  CHECK(!contact_boundary::part_reads&&!contact_boundary::endpoint_reads);}
 for(unsigned reason:{1u,4u,7u,8u,15u}){ContactFixture f;contact_boundary::reset(f,reason);bc::Geometry g{};g.endpoint_serial=99;
  CHECK(!boomshot_contact::observe(f.tracking.hand,g)&&!g.endpoint_serial);
  CHECK(!contact_boundary::part_reads&&!contact_boundary::endpoint_reads);}
 {ContactFixture f;contact_boundary::reset(f);++f.tracking.hand.key.frame;bc::Geometry g{};
  CHECK(!boomshot_contact::observe(f.tracking.hand,g));}
}
void original_boundary_cases(){
 {ContactFixture f;contact_boundary::reset(f);CHECK(boomshot_contact::measure(f.tracking.hand));bc::Geometry g{};
  CHECK(contact_boundary::part_reads==2&&contact_boundary::endpoint_reads==2&&boomshot_contact::sample(f.tracking.hand,g));
  CHECK(g.endpoint_serial==17&&g.input_sync_ns==f.tracking.hand.controllers.sync_ns);}
 for(unsigned reason=1;reason<=14;++reason){ContactFixture f;contact_boundary::reset(f);
  CHECK(boomshot_contact::measure(f.tracking.hand)); // Seed an actual prior measurement.
  contact_boundary::reset(f,reason);CHECK(!boomshot_contact::measure(f.tracking.hand));
  if(reason==1){ // Off-thread rejection may not mutate the GT-owned cache.
   CHECK(boomshot_contact::measurement.known);bc::Geometry g{};CHECK(!boomshot_contact::sample(f.tracking.hand,g));
  }else CHECK(!boomshot_contact::measurement.known); // GT failure never advertises old geometry.
 }
 boomshot_contact::clear();CHECK(!boomshot_contact::measurement.known);
}
int main(){CHECK(existing_rig_binding_regressions()==0);original_boundary_cases();current_observation_cases();unsigned count{};
 {ContactFixture f;bc::Geometry g{};CHECK(f.run(g));
  const auto expected=bc::point(f.measured.root,bc::closed_surface_raw,1);
  CHECK(gears_vr::finite(g.surface_world)&&std::abs(g.surface_world.x-expected.x)<1e-9);
  CHECK(g.input_sync_ns==f.tracking.hand.controllers.sync_ns&&g.geometry_acquired_ns==f.actual.acquisition_begin_ns);
  CHECK(g.endpoint_serial==17&&g.progress==0);++count;}
 for(unsigned reason=0;reason<24;++reason){ContactFixture f;bc::Geometry g{};g.endpoint_serial=99;
  switch(reason){case 0:f.measured.known=false;break;case 1:f.measured.component={};break;
   case 2:f.measured.asset={};break;case 3:f.measured.scale=0;break;case 4:f.measured.measured_ns=fixture_now+1;break;
   case 5:f.measured.measured_ns=fixture_now-100000001;break;case 6:++f.measured.source.key.frame;break;
   case 7:++f.actual.serial;break;case 8:f.actual.right_grip.position.x+=1;break;case 9:++f.actual.returned_ns;break;
   case 10:f.actual.purpose=ep::Purpose::observed_held_visual;break;case 11:f.actual.purpose=ep::Purpose::free_hand_visual;break;
   case 12:++f.actual.hold_generation;break;case 13:++f.actual.native_reference.epoch;break;
   case 14:++f.actual.producer.tracking[0];break;case 15:f.tracking.rig.world_pose_mask&=~rt::left_grip_pose;break;
   case 16:f.measured.barrel_relative.position.x=.01;break;case 17:f.measured.barrel_relative.orientation={.1,0,0,.99498743710662};break;
   case 18:++f.measured.source.controllers.hands[0].pressed[0];break;
   case 19:++f.actual.controllers.sequence;break;case 20:++f.actual.controllers.sync_ns;break;
   case 21:++f.tracking.rig.reference_generation;break;case 22:++f.tracking.rig.controllers.sequence;break;
   case 23:f.measured.root.position.x=std::numeric_limits<double>::quiet_NaN();break;}
  CHECK(!f.run(g)&&!g.endpoint_serial);++count;
 }
 for(double scale:{.3,1.,2.})for(double progress:{0.,.5,1.}){ContactFixture f;f.measured.scale=scale;
  f.measured.barrel_relative.position.y-=progress*gears_weapon::boomshot_barrel::travel_raw;
  bc::Geometry g{};CHECK(f.run(g)&&std::abs(g.progress-progress)<1e-9);
  auto raw=bc::closed_surface_raw;raw.y-=progress*gears_weapon::boomshot_barrel::travel_raw;
  const auto surface=bc::point(f.measured.root,raw,scale);const auto d=bc::minus(f.tracking.rig.world_grip[0].position,surface);
  CHECK(std::abs(g.distance_m-std::hypot(d.x,d.y,d.z)/100)<1e-9);++count;
 }
 // Actual committed palm can differ from raw right tracking under fixed-arm
 // IK. Measured root remains independent of that raw right controller.
 {ContactFixture f;f.actual.right_grip.position.x+=100;f.measured.committed=f.actual;bc::Geometry g{};
  CHECK(f.run(g));const auto surface=bc::point(f.measured.root,bc::closed_surface_raw,1);
  CHECK(std::abs(g.surface_world.x-surface.x)<1e-9);++count;}
 // No cache reuse on a later owning frame/C0. It needs a new bone read, even
 // when original input is still fresh; source timestamps are never renewed.
 {ContactFixture f;bc::Geometry g{};++f.tracking.hand.key.frame;++f.tracking.rig.scene_invocation;
  CHECK(!f.run(g));++count;}
 std::printf("Boomshot measured contact geometry: %u cases plus original rig regressions; 15 GT and7 current-observation boundary cases; exact Frame/C0, double original bones, raw left, failure clears cache; no action grant\n",count);
}
