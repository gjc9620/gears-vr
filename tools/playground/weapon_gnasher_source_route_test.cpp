#include "weapon_lancer_r2.h"
#include <cstdio>
#include <cstdlib>
namespace hand=gears_weapon_hand;
namespace gnasher_runtime {
namespace hand=gears_weapon_hand;
#include "weapon_gnasher_source_route.inl"
}
unsigned checks{};
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %u: %s\n",unsigned(__LINE__),#x);std::exit(1);}}while(false)
hand::ObjectIdentity object(std::uint64_t n){return {n,0x1000,0x2000,unsigned(n/8),7,0,1};}
int main(){
 hand::Frame f{};f.key={11,22,33,1,10,1,44,55};f.world=object(0x6000);f.pawn=object(0x6020);
 f.controller=object(0x6040);f.mesh=object(0x6060);f.weapon=object(0x6080);
 f.controllers.sequence=17;f.controllers.sync_ns=100;f.now_ns=150;f.reference_generation=3;
 hand::Decision decision{};decision.key=f.key;
 hand::GripGameState base{};base.known=base.main_hand_held=base.mount_known=true;
 base.kind=hand::WeaponKind::firearm;base.mount_asset=object(0x7000);base.mount_revision=1;
 base.mount.support_grip_in_weapon={0,0,-.3};base.acquire_metres=.08;base.release_metres=.2;
 auto route=base;route.mount.support_grip_in_weapon={.02,0,-.4};route.mount_revision=9901;
 route.request_ammo=true; // Source observation must never restore candidates.
 CHECK(base.mount.valid()&&route.mount.valid());
 // Reproduce the old production comparison using an actual generic/base read.
 CHECK(!gears_lancer_r2::same_fire_observation(route,base));CHECK(hand::game_difference(route,base)&8u);
 auto observed=base;
 auto project=[&](hand::Frame const& sample,hand::Frame const& publication,hand::Frame const& decision_sample,
                  hand::Decision const& d,bool known,bool current,hand::GripGameState& game){
   return gnasher_runtime::project_source_route(sample,publication,base,route,decision_sample,d,known,current,game,nullptr,true);
 };
 CHECK(project(f,f,f,decision,true,true,observed));
 CHECK(gears_lancer_r2::same_fire_observation(route,observed));
 CHECK(!observed.request_ammo&&!observed.request_magazine&&!observed.magazine_observation_pending);
 CHECK(hand::same(observed.object_incumbents[0],base.object_incumbents[0]));
 CHECK(f.controllers.sync_ns==100&&f.now_ns==150);
 const auto rejected=[&](hand::GripGameState changed){const auto original=changed;
   CHECK(!project(f,f,f,decision,true,true,changed));
   CHECK(hand::same_game(changed,original)&&changed.support_box==original.support_box);
 };
 using Change=void(*)(hand::GripGameState&);
 const Change changes[]={
  [](auto& g){g.known=false;},[](auto& g){g.main_hand_held=false;},[](auto& g){g.busy=true;},
  [](auto& g){g.animation_owns_hands=true;},[](auto& g){g.kind=hand::WeaponKind::hand_thrown;},
  [](auto& g){g.mount_known=false;},[](auto& g){++g.mount_asset.name_number;},[](auto& g){++g.mount_revision;},
  [](auto& g){g.mount.right_grip_in_weapon.position.x+=.01;},[](auto& g){g.mount.support_grip_in_weapon.x+=.01;},
  [](auto& g){g.mount.muzzle_in_weapon.x+=.01;},[](auto& g){g.mount.bore_forward_in_weapon.x+=.01;},
  [](auto& g){g.mount.support_length_tolerance_metres+=.01;},[](auto& g){g.mount.maximum_support_correction_radians+=.01;},
  [](auto& g){g.acquire_metres+=.01;},[](auto& g){g.release_metres+=.01;},
  [](auto& g){g.support_box.acquire_half.x+=.01;},[](auto& g){g.request_ammo=true;},
  [](auto& g){g.request_magazine=true;},[](auto& g){g.magazine_observation_pending=true;},
  [](auto& g){g.object_incumbents[0].id=42;},[](auto& g){g.object_incumbents[1].id=42;}
 };
 for(auto change:changes){auto changed=base;change(changed);rejected(changed);}
 const auto reject_frame=[&](hand::Frame changed){auto value=base;
   CHECK(!project(changed,f,f,decision,true,true,value));CHECK(hand::same_game(value,base));
   CHECK(!project(f,changed,f,decision,true,true,value));
   CHECK(!project(f,f,changed,decision,true,true,value));
 };
 auto changed=f;++changed.key.frame;reject_frame(changed);
 changed=f;++changed.controllers.sequence;reject_frame(changed);
 changed=f;++changed.controllers.sync_ns;reject_frame(changed);
 changed=f;++changed.now_ns;reject_frame(changed);
 changed=f;++changed.weapon.name_number;reject_frame(changed);
 changed=f;++changed.key.registration_cookie;reject_frame(changed);
 changed=f;++changed.reference_generation;reject_frame(changed);
 changed=f;++changed.controllers.input_epoch;reject_frame(changed);
 observed=base;CHECK(!project(f,f,f,decision,false,true,observed));
 CHECK(!project(f,f,f,decision,true,false,observed));
 auto other_decision=decision;++other_decision.key.frame;CHECK(!project(f,f,f,other_decision,true,true,observed));
 auto broken=route;broken.mount_revision=0;
 CHECK(!gnasher_runtime::project_source_route(f,f,base,broken,f,decision,true,true,observed,nullptr,true));
 broken=route;++broken.mount_asset.name_number;
 CHECK(!gnasher_runtime::project_source_route(f,f,base,broken,f,decision,true,true,observed,nullptr,true));
 broken=route;broken.busy=true;
 CHECK(!gnasher_runtime::project_source_route(f,f,base,broken,f,decision,true,true,observed,nullptr,true));
 // Actual generic reader excludes GN and therefore has no generic mount.
 // This must work only with a NEW successful actual GN asset/binding reread.
 auto no_mount=base;no_mount.mount_known=false;no_mount.mount={};no_mount.mount_asset={};no_mount.mount_revision=0;
 observed=no_mount;
 CHECK(!gnasher_runtime::project_source_route(f,f,no_mount,route,f,decision,true,true,observed));
 CHECK(hand::same_game(observed,no_mount));
 CHECK(gnasher_runtime::project_source_route(f,f,no_mount,route,f,decision,true,true,observed,nullptr,true));
 CHECK(gears_lancer_r2::same_fire_observation(observed,route));
 observed=base;
 CHECK(hand::same_game(observed,base));
 unsigned failure{};auto mismatch=base;mismatch.busy=true;
 CHECK(!gnasher_runtime::project_source_route(f,f,base,route,f,decision,true,true,mismatch,&failure,true)&&failure==9);
 CHECK(!gnasher_runtime::project_source_route(changed,f,base,route,f,decision,true,true,observed,&failure,true)&&failure==2);
 CHECK(!gnasher_runtime::project_source_route(f,f,base,route,f,decision,true,false,observed,&failure,true)&&failure==1);
 std::printf("Gnasher committed source-route projection: %u checks passed; real comparison mismatch reproduced, fresh base/core/mount/candidate/incumbent and exact-frame rejection preserved; no engine or headset evidence\n",checks);
}
