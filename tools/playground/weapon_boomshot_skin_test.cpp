#include "../vr-common/weapon_hand_client.h"
#include "../vr-common/weapon_boomshot_grasp_pose.h"
#include "../vr-common/weapon_boomshot_hand_mount.h"
#include "../vr-common/weapon_two_hand_hold.h"
#pragma push_macro("main")
#undef main
#define main previous_skin_advance_cases
#include "weapon_boomshot_advance_test.cpp"
#undef main
#pragma pop_macro("main")
namespace {
namespace authored=gears_boomshot_grasp_pose;namespace handmount=gears_boomshot_hand_mount;
Normal* skin_fixture{};bool skin_source_known=true;unsigned skin_publications{},skin_source_reads{};
authored::Part captured_part{};gears_vr::Pose captured_grip{};bool captured_proximity{};
namespace nativelancerfollow {struct Key {std::uint64_t hold_generation{10};}bound_key;}
namespace boomshot_skin_source {
struct Source {struct {h::ObjectIdentity asset{};}pose;gears_rig_tracking::Frame rig{};
 gears_vr::Pose root{},barrel{};double root_scale{1},barrel_scale{1},hand_scale{1},units{100};};
Source supplied{};
bool read(h::Frame const&,Source& out)noexcept{++skin_source_reads;out=supplied;return skin_source_known;}
bool support_world(Source const& s,gears_vr::Pose& out)noexcept{return handmount::support(s.root,s.root_scale,s.hand_scale,s.units,out);}
bool mechanism_world(Source const& s,bool closing,gears_vr::Pose& out)noexcept{return handmount::mechanism(s.barrel,s.barrel_scale,s.hand_scale,s.units,closing,out);}
bool relative_support(Source const& s,gears_vr::Pose& out)noexcept{if(!support_world(s,out))return false;out.position=handmount::mul(out.position,1/s.units);return gears_weapon_two_hand::geometry(out);}
bool support_relative(h::Frame const& f,gears_vr::Pose& out)noexcept{return !h::index_pinch(f)&&!boomshot_reload_owner::owner.blocks_fire(f)&&skin_source_known&&relative_support(supplied,out);}
}
namespace boomshot_grasp {
bool publish_marcus115(h::Frame const&,authored::Part part,gears_vr::Pose const& grip,bool proximity){
 ++skin_publications;captured_part=part;captured_grip=grip;captured_proximity=proximity;return true;
}
}
#define BOOMSHOT_SKIN_SOURCE_BOUNDARY_FIXTURE 1
#include "weapon_boomshot_skin.inl"
#undef BOOMSHOT_SKIN_SOURCE_BOUNDARY_FIXTURE
namespace skin=boomshot_skin;
void refresh_skin(Normal& x){auto& s=boomshot_skin_source::supplied;s={};s.pose.asset=object(0x6100,21);s.rig=x.rig.rig;
 s.barrel.position=m::closed_raw;s.barrel.position.y-=m::travel_raw*bone_q;}
void reset_skin(Normal& x){auto& owner=boomshot_reload_owner::owner;owner.~Owner();::new(static_cast<void*>(&owner)) boomshot_reload_owner::Owner{};
 skin::passive_selection.lost();skin_fixture=&x;skin_source_known=true;skin_publications=0;refresh_skin(x);}
auto skin_advance(Normal& x){const auto result=boomshot_reload_owner::owner.advance(x.input.f,x.input.d,x.input.route.grant(),insertion_fixture());refresh_skin(x);return result;}
bool skin_publish(Normal& x){refresh_skin(x);return skin::publish_left(x.input.f,x.input.d,x.input.route.grant());}
void skin_open(Normal& x){assert(skin_advance(x)==c::BoomshotEvent::none);x.next();assert(skin_advance(x)==c::BoomshotEvent::started);
 x.next();skin_advance(x);x.next();hand_q=bone_q=.5;skin_advance(x);x.next();hand_q=bone_q=1;skin_advance(x);x.next();
 assert(skin_advance(x)==c::BoomshotEvent::step_completed);}
void skin_take(Normal& x){skin_open(x);x.shoulder_grant();assert(skin_advance(x)==c::BoomshotEvent::grabbed);}
void neutral(Normal& x){x.input.f.left_grip_down=0;x.input.f.controllers.hands[0].squeeze=x.input.f.controllers.hands[0].trigger=0;
 x.input.d.lease={};x.input.d.new_grant=0;x.input.d.status=h::DecisionStatus::no_candidate;}
}
int main(){unsigned count{};
 {Normal x(1);reset_skin(x);assert(skin_advance(x)==c::BoomshotEvent::none);const auto before=x.input.f;
  assert(skin_publish(x)&&captured_part==authored::Part::mechanism_open&&!captured_proximity&&!native_calls&&!inventory_writer_calls);
  assert(h::same_frame(before,x.input.f));++count;}
 {Normal x(2);reset_skin(x);skin_open(x);assert(!skin_publish(x)&&!skin_publications);++count;}
 {Normal x(3);reset_skin(x);skin_take(x);const auto before=x.input.f;assert(skin_publish(x)&&captured_part==authored::Part::carrier&&!captured_proximity);
  assert(gears_vr::stereo_same_pose(captured_grip,boomshot_skin_source::supplied.rig.world_grip[0])&&h::same_frame(before,x.input.f)&&!inventory_writer_calls);++count;}
 {Normal x(4);reset_skin(x);skin_take(x);x.next();boomshot_carrier_original::raw_depth=.13;skin_advance(x);assert(skin_publish(x)&&captured_part==authored::Part::carrier);
  x.next();boomshot_carrier_original::raw_depth=.1;skin_advance(x);assert(skin_publish(x)&&captured_part==authored::Part::carrier_seating&&!inventory_writer_calls);
  assert(gears_vr::stereo_same_pose(captured_grip,boomshot_skin_source::supplied.rig.world_grip[0]));++count;}
 {Normal x(5);reset_skin(x);skin_take(x);for(double depth:{.13,.1,.05,0.,0.}){x.next();boomshot_carrier_original::raw_depth=depth;skin_advance(x);}
  assert(!skin_publish(x)&&inventory_writer_calls==1);closing_grant(x);assert(skin_publish(x)&&captured_part==authored::Part::mechanism_close&&!captured_proximity);++count;}
 {Normal x(6);reset_skin(x);x.input.d.lease.kind=h::LeaseKind::support;assert(skin_publish(x)&&captured_part==authored::Part::support&&!captured_proximity);
  assert(gears_weapon_two_hand::geometry(captured_grip));x.input.d.lease.id++;assert(!skin_publish(x));++count;}
 {Normal x(7);reset_skin(x);neutral(x);gears_vr::Pose target{};assert(boomshot_skin_source::support_world(boomshot_skin_source::supplied,target));
  boomshot_skin_source::supplied.rig.world_grip[0]=target;
  assert(skin::publish_left(x.input.f,x.input.d,x.input.route.grant())&&captured_part==authored::Part::support&&captured_proximity&&!native_calls);++count;}
 {Normal x(8);reset_skin(x);neutral(x);gears_vr::Pose target{};assert(boomshot_skin_source::mechanism_world(boomshot_skin_source::supplied,false,target));
  boomshot_skin_source::supplied.rig.world_grip[0]=target;
  assert(skin::publish_left(x.input.f,x.input.d,x.input.route.grant())&&captured_part==authored::Part::mechanism_open&&captured_proximity);++count;}
 {Normal x(9);reset_skin(x);neutral(x);boomshot_skin_source::supplied.rig.world_grip[0].position={1000,1000,1000};
  assert(!skin::publish_left(x.input.f,x.input.d,x.input.route.grant())&&!skin_publications);++count;}
 {Normal x(10);reset_skin(x);skin_source_known=false;assert(!skin_publish(x)&&!skin_publications);++count;}
 {Normal x(11);reset_skin(x);skin_take(x);normal_fault=70;assert(!skin_publish(x)&&!skin_publications&&!inventory_writer_calls);++count;}
 {Normal x(12);reset_skin(x);skin_open(x);assert(boomshot_reload_owner::owner.drain());assert(!skin_publish(x)&&!skin_publications);++count;}
 {Normal x(13);reset_skin(x);h::GripGameState game{};game.known=game.main_hand_held=true;game.kind=h::WeaponKind::firearm;
  assert(skin::fill_support_mount(x.input.f,game)&&game.mount_known&&h::same(game.mount_asset,object(0x6100,21))&&game.mount_revision==10);
  const auto before=x.input.f;x.input.f.interaction_control=unsigned(gears_vr::LeftInteractionControl::pinch);
  assert(!skin::fill_support_mount(x.input.f,game));x.input.f=before;game.busy=true;assert(!skin::fill_support_mount(x.input.f,game));++count;}
 {Normal x(14);reset_skin(x);assert(skin_advance(x)==c::BoomshotEvent::none);h::GripGameState game{};game.known=game.main_hand_held=true;game.kind=h::WeaponKind::firearm;
  assert(!skin::fill_support_mount(x.input.f,game));++count;}
 {Normal x(15);reset_skin(x);x.input.d.reserved=1;assert(!skin_publish(x)&&!skin_publications);++count;}
 {Normal x(16);reset_skin(x);neutral(x);gears_vr::Pose target{};
  assert(boomshot_skin_source::support_world(boomshot_skin_source::supplied,target));
  const auto publish_at=[&](double metres){auto& s=boomshot_skin_source::supplied;s.barrel.position={10000,10000,10000};
   s.rig.world_grip[0]=target;s.rig.world_grip[0].position.x+=metres*s.units;
   return skin::publish_left(x.input.f,x.input.d,x.input.route.grant());};
  assert(publish_at(.10)&&captured_part==authored::Part::support&&captured_proximity);
  x.next();neutral(x);assert(publish_at(.19)&&captured_part==authored::Part::support);
  x.next();neutral(x);assert(!publish_at(.201));x.next();neutral(x);assert(!publish_at(.19));
  x.next();neutral(x);assert(publish_at(.10)&&!native_calls&&!inventory_writer_calls);++count;}
 {Normal x(17);reset_skin(x);neutral(x);gears_vr::Pose target{};
  auto& s=boomshot_skin_source::supplied;assert(boomshot_skin_source::support_world(s,target));
  s.barrel.position={10000,10000,10000};s.rig.world_grip[0]=target;
  assert(skin::publish_left(x.input.f,x.input.d,x.input.route.grant()));
  x.next();neutral(x);skin_source_known=false;assert(!skin::publish_left(x.input.f,x.input.d,x.input.route.grant()));
  skin_source_known=true;x.next();neutral(x);s.rig.world_grip[0].position.x+=.19*s.units;
  assert(!skin::publish_left(x.input.f,x.input.d,x.input.route.grant())&&!native_calls&&!inventory_writer_calls);++count;}
 for(unsigned boundary=0;boundary<3;++boundary){Normal x(20+boundary);reset_skin(x);
  if(boundary==0)x.input.f.left_grip_down=0;
  if(boundary==1)x.input.f.left_grip_known=0;
  if(boundary==2)x.input.f.left_tracked=0;
  h::GripGameState before{};before.known=before.main_hand_held=true;before.kind=h::WeaponKind::firearm;
  auto after=before;skin_source_reads=0;skin_source_known=false;
  assert(!skin::fill_support_mount(x.input.f,before));skin_source_known=true;
  assert(!skin::fill_support_mount(x.input.f,after));
  assert(h::same_game(before,after)&&!after.mount_known&&!skin_source_reads);++count;
 }
 {Normal x(24);reset_skin(x);h::GripGameState game{};game.known=game.main_hand_held=true;game.kind=h::WeaponKind::firearm;
  skin_source_reads=0;skin_source_known=false;assert(!skin::fill_support_mount(x.input.f,game)&&!game.mount_known&&skin_source_reads==1);
  skin_source_known=true;assert(skin::fill_support_mount(x.input.f,game)&&game.mount_known&&skin_source_reads==2);++count;}
 std::printf("Boomshot actual skin selection/publication: %u cases passed; real reload owner/core/grants/RAW insertion, separate open/close/carrier/entry/support/proximity profiles, released/unknown/untracked left cannot introduce support metadata, held support retains original live source requirement; Source/engine/Native V4 export boundaries stubbed, no pixel fit proof\n",count);return 0;
}
