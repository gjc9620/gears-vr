#include "weapon_gnasher_hold_geometry.h"
#include "weapon_mesh_follow.h"
#include "weapon_gnasher_live_basis.h"
#include "weapon_gnasher_calibration.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
namespace g=gears_weapon_gnasher_hold;namespace w=gears_weapon;namespace v=gears_vr;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"line %u: %s\n",unsigned(__LINE__),#x);std::exit(1);}}while(false)
bool near(v::Vector a,v::Vector b){return w::length(w::difference(a,b))<1e-9;}
int main(){
 for(double units:{20.,100.,500.}){
  g::Authored a{};CHECK(g::authored(units/100,units,8,a));const auto& c=a.mechanical;
  CHECK(c.valid()&&a.support.valid()&&a.box.valid()&&!c.feed_calibrated&&c.capacity==8);
  // Route socket and physical recognizer must use the SAME authored raw grip
  // and actual skin contact offset, even though their vector bases differ.
  const auto relative=w::sum(c.right_contact_in_grip,w::rotate(v::inverse(c.right_grip_home_in_main),
      w::difference(c.lever.left_front_grip_in_main.position,c.lever.right_handle_home_in_main)));
  CHECK(near(a.support.support_grip_in_weapon,{relative.y,relative.z,-relative.x}));
  const auto palm=w::sum(a.right_in_component_metres.position,w::rotate(a.right_in_component_metres.orientation,c.right_contact_in_grip));
  CHECK(near(palm,w::scaled(g::right_surface_mesh_raw,.01)));
  // Actual production mesh-follow implementation must produce the inverse
  // of this same raw grip when the controller is at the world origin.
  w::WeaponMeshFollow follow;const float scale=float(units/100);
  const std::array<float,16> original_mesh{scale,0,0,0,0,scale,0,0,0,0,scale,0,10,20,30,1};
  CHECK(follow.bind_mount({{40,50,60},{.1,.2,.3}},original_mesh,g::right_grip_mesh_raw.position,
      v::rotation_matrix(g::right_grip_mesh_raw.orientation),v::Matrix{{{1,0,0},{0,1,0},{0,0,1}}}));
  std::array<float,16> actual_mount{};v::Pose expected_mount{};
  CHECK(follow.model_matrix({{},{}},actual_mount));
  CHECK(w::gnasher_live_basis::component_for_main({},a.right_in_component_metres,expected_mount));
  for(unsigned i=0;i<3;++i){
   const double expected_position=i==0?expected_mount.position.x:(i==1?expected_mount.position.y:expected_mount.position.z);
   CHECK(std::abs(actual_mount[12+i]/units-expected_position)<1e-7);
   for(unsigned j=0;j<3;++j)CHECK(std::abs(actual_mount[i*4+j]-v::rotation_matrix(expected_mount.orientation)[j][i]*scale)<1e-6);
  }
  const w::GnasherIdentity id{1,2,3,4,5,6,7,8};w::GnasherMechanicalInteraction mechanical;
  CHECK(mechanical.bind(id,c,{id,1,8,10}));w::GnasherMechanicalSample s{};
  s.identity=id;s.sequence=1;s.monotonic_ns=1000000000;s.game_tick=3500000;
  s.left_tracked=s.right_tracked=s.left_front_held=s.right_handle_held=s.original_ready=true;s.support_grasp_token=1;
  s.left_grip=c.lever.left_front_grip_in_main;
  const auto pose=[&](double angle){
   const v::Quaternion hinge{-std::sin(angle*.5),0,0,std::cos(angle*.5)};
   s.right_grip.orientation=v::normalized(v::multiply(hinge,c.right_grip_home_in_main));
   const auto contact=w::sum(c.lever.pivot_in_main,w::rotate(hinge,w::difference(c.lever.right_handle_home_in_main,c.lever.pivot_in_main)));
   s.right_grip.position=w::difference(contact,w::rotate(s.right_grip.orientation,c.right_contact_in_grip));
  };
  pose(0);CHECK(mechanical.observe(s).event==w::GnasherMechanicalEvent::none);
  CHECK(mechanical.can_request_fire(s.monotonic_ns));
  CHECK(mechanical.after_original_shot({id,11,s.monotonic_ns,s.game_tick,{id,2,7,10}}).event==w::GnasherMechanicalEvent::shot_recorded);
  const auto step=[&](double angle){pose(angle);++s.sequence;s.monotonic_ns+=20000000;s.game_tick+=20000;return mechanical.observe(s);};
  CHECK(step(0).event==w::GnasherMechanicalEvent::restored);
  w::GnasherMechanicalDecision d{};
  for(double angle:{0.,.1,.2,.34,.48,.565,.4,.2,.1,0.}){
   d=step(angle);CHECK(d.drive_lever);CHECK(std::abs(d.lever_angle_radians-angle)<1e-8);
   CHECK(near(d.main_pose.position,{}));CHECK(!mechanical.can_request_fire(s.monotonic_ns));
  }
  CHECK(d.event==w::GnasherMechanicalEvent::request_original_cycle&&mechanical.cycle_request().token);
  CHECK(mechanical.inventory().loaded==7&&mechanical.inventory().reserve==10);
 }
 for(unsigned i=0;i<8;++i){double scale=1,units=100;unsigned capacity=8;g::Authored a{};
  if(i==0)scale=0;if(i==1)scale=10;if(i==2)units=9;if(i==3)units=501;
  if(i==4)capacity=0;if(i==5)capacity=10001;if(i==6)scale=std::numeric_limits<double>::quiet_NaN();
  if(i==7)units=std::numeric_limits<double>::infinity();CHECK(!g::authored(scale,units,capacity,a));
 }
 g::Authored huge{};CHECK(!g::authored(1,10,8,huge)); // No widened ranges to accept a physically oversized model.
 CHECK(gears_weapon_gnasher_calibration::current.valid()); // Independent original-game50672 evidence; contact authoring does not generate it.
 CHECK(gears_weapon_gnasher_calibration::current.evidence==3503621743347927725ull);
 CHECK(gears_weapon_gnasher_calibration::current.law==gears_weapon::gnasher_tracks::Timing::keys_minus_one);
 std::puts("Gnasher original-triangle authored contacts: same raw sockets/skin offset, scale/units, actual-profile physical down-return and strict invalid geometry checks passed; synthetic movement, no game/finger/headset acceptance");
}
