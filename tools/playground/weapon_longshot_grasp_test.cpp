#include "../vr-common/hand_part_grasp.h"
#include "../vr-common/weapon_support_grip.h"
#include "../vr-common/hand_authored_mount_profile.h"
#include "../vr-common/weapon_longshot_grasp_pose.h"
#include "../vr-common/rig_tracking_frame.h"
#include "../vr-common/hand_authored_grasp.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace hand=gears_weapon_hand;namespace api=gears_hand_part_grasp;
using P=gears_vr::Pose;using V=gears_vr::Vector;
bool fresh=true,selected=true,publish_ok=true,api_present=true,round_current=true;
bool v4_present=false;
unsigned authored_publications{};gears_hand_authored_grasp::Request authored_published{};
bool publish_authored(gears_hand_authored_grasp::Request const* q)noexcept {++authored_publications;authored_published=*q;return publish_ok;}
unsigned publications{};api::Request published{};
bool publish(api::Request const* q)noexcept {++publications;published=*q;return publish_ok;}
namespace nativeequipment {bool current(void*,hand::Frame const&){return fresh;}}
namespace lancerreload_inventory {
struct Identity {hand::ObjectIdentity weapon{};bool operator==(Identity const& b)const{return hand::same(weapon,b.weapon);}};
Identity identity(hand::Frame const& f){return {f.weapon};}
}
namespace nativelancerfollow {
struct Key {std::uint64_t hold_generation{1};bool operator==(Key const& b)const{return hold_generation==b.hold_generation;}}bound_key;
struct {bool valid{};hand::Frame source{};Key key{};gears_rig_tracking::Frame prepared{};}plan;
}
namespace longshot_runtime {
enum class Stage {idle,acquiring,opening,waiting_round,holding_round,waiting_close,closing,completing,recovering};Stage stage{};
bool selected(hand::Frame const&){return ::selected;}
bool proposal_known=true;lancerreload_inventory::Identity proposal_identity{};
std::uint64_t proposal_reference{},contact_frame{};std::int64_t contact_sync_ns{};
P root_in_right{};double root_scale=1;V knob_local{.1,.05,.1},bolt_pivot_local{.1,0,.1};
}
namespace longshot_bolt {
V point(P const& p,V v,double scale=1){auto d=gears_vr::transform(gears_vr::rotation_matrix(p.orientation),{v.x*scale,v.y*scale,v.z*scale});return {p.position.x+d.x,p.position.y+d.y,p.position.z+d.z};}
bool granted(hand::Frame const& f,hand::Decision const& d){return !d.release_required&&
 (d.status==hand::DecisionStatus::granted||d.status==hand::DecisionStatus::retained)&&
 d.lease.kind==hand::LeaseKind::magazine&&d.lease.id==f.press_token&&f.left_grip_down;}
}
namespace longshot_round {
std::uint64_t item_serial{};hand::Lease held_lease{};
bool has_presentation{},authored_pinch{};P presented_grip{};
bool presentation_grip(hand::Frame const&,hand::Decision const&,P& out){out=presented_grip;return has_presentation;}
bool current(hand::Frame const&){return round_current;}
bool granted(hand::Frame const& f,hand::Decision const& d){return !d.release_required&&
 d.lease.kind==hand::LeaseKind::ammo&&d.lease.id==f.press_token&&f.left_grip_down;}
}
namespace lancersaw_visual {template<class T>T function(char const* name){
 if constexpr(std::is_same_v<T,gears_hand_authored_grasp::Publish>){assert(!std::strcmp(name,gears_hand_authored_grasp::publish_export));return v4_present?&publish_authored:nullptr;}
 else{assert(!std::strcmp(name,api::publish_export));return api_present?&publish:nullptr;}
}}
#include "weapon_longshot_grasp.inl"
struct Fixture {
 hand::Frame f{};hand::GripGameState game{};hand::Decision d{};
 Fixture(){
  fresh=selected=publish_ok=api_present=round_current=v4_present=true;publications=authored_publications=0;published={};authored_published={};longshot_round::authored_pinch=false;longshot_round::has_presentation=false;
  f.key={1,2,3,4,5,6,7,8};f.weapon.address=64;f.left_tracked=f.left_grip_known=1;
  f.reference_generation=1;f.controllers.sync_ns=1000000000;f.controllers.sequence=1;
  f.controllers.hands[0].squeeze_active=f.controllers.hands[0].trigger_active=1;
  game.known=game.main_hand_held=1;d.key=f.key;
  nativelancerfollow::bound_key={1};nativelancerfollow::plan={};auto& p=nativelancerfollow::plan;
  p.valid=true;p.source=f;p.key=nativelancerfollow::bound_key;p.prepared.units_per_metre=100;
  p.prepared.world_pose_mask=gears_rig_tracking::left_grip_pose|gears_rig_tracking::right_grip_pose;
  p.prepared.world_grip[0].position={10,5,10};
  longshot_runtime::stage=longshot_runtime::Stage::idle;longshot_runtime::proposal_known=true;
  longshot_runtime::proposal_identity=lancerreload_inventory::identity(f);longshot_runtime::proposal_reference=1;
  longshot_runtime::contact_frame=f.key.frame;longshot_runtime::contact_sync_ns=f.controllers.sync_ns;
  longshot_runtime::root_in_right={};longshot_runtime::root_scale=1;
  longshot_grasp::nearby=false;longshot_round::item_serial=0;longshot_round::held_lease={};
 }
 bool tick(){nativelancerfollow::plan.source=f;return longshot_grasp::tick(f,game,d);}
 void active(hand::LeaseKind kind){f.left_grip_down=1;f.press_token=9;d.press_token=9;
  d.status=hand::DecisionStatus::granted;d.lease={kind,0,f.key.lifecycle,9};d.new_grant=1;d.release_required=0;}
};
int main(){
#ifdef _WIN32
 _set_error_mode(_OUT_TO_STDERR);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
 {Fixture v;v.active(hand::LeaseKind::ammo);longshot_runtime::stage=longshot_runtime::Stage::holding_round;
  longshot_round::item_serial=9;longshot_round::held_lease=v.d.lease;
  longshot_round::authored_pinch=v4_present=longshot_round::has_presentation=true;
  longshot_round::presented_grip={{31,32,33},{}};const auto raw=nativelancerfollow::plan.prepared.world_grip[0];
  assert(v.tick()&&authored_publications==1&&publications==0);
  assert(authored_published.profile_id==gears_weapon_longshot_round_grasp::profile_id&&
      authored_published.kind==gears_hand_object_grasp::Kind::magazine&&authored_published.grip.position.x==31);
  assert(authored_published.source.press_token==v.f.press_token&&
      authored_published.source.controllers.sync_ns==v.f.controllers.sync_ns&&
      !authored_published.proximity&&!authored_published.has_trigger_endpoint);
  for(unsigned i=0;i<15;++i)assert(std::abs(authored_published.closed[i].w-gears_weapon_longshot_round_grasp::closed[i].w)<1e-12);
  assert(nativelancerfollow::plan.prepared.world_grip[0].position.x==raw.position.x);
  v4_present=false;assert(!v.tick()&&publications==0&&authored_publications==1); // No legacy skin/mount mismatch.
  v4_present=true;publish_ok=false;assert(!v.tick()&&publications==0);
  publish_ok=true;longshot_round::held_lease.id=8;assert(!v.tick());
 }
 {Fixture v;const auto raw=v.f.controllers;assert(v.tick()&&authored_published.proximity==1&&publications==0);
  assert(authored_published.profile_id==gears_weapon_longshot_bolt_grasp::profile_id);
  for(unsigned i=0;i<15;++i)assert(std::abs(authored_published.closed[i].w-gears_weapon_longshot_bolt_grasp::closed[i].w)<1e-12);
  assert(!std::memcmp(&raw,&v.f.controllers,sizeof raw));
  nativelancerfollow::plan.prepared.world_grip[0].position.x+=6;assert(v.tick()); // Keep7cm.
  nativelancerfollow::plan.prepared.world_grip[0].position.x+=2;assert(!v.tick());
  nativelancerfollow::plan.prepared.world_grip[0].position.x-=2;assert(!v.tick()); // Reentry still needs5cm.
 }
 {Fixture v;v.active(hand::LeaseKind::magazine);const bool result=v.tick();
  assert(result&&authored_published.proximity==0&&authored_publications==1&&publications==0);
  v.f.left_grip_down=0;v.d.lease={};v.d.release_required=1;
  nativelancerfollow::plan.prepared.world_grip[0].position.x+=6;assert(v.tick()&&authored_published.proximity==1);
  v.f.left_grip_down=1;v.f.controllers.hands[0].squeeze=1;assert(!v.tick()); // Held return cannot acquire.
 }
 {Fixture v;v.active(hand::LeaseKind::ammo);longshot_runtime::stage=longshot_runtime::Stage::holding_round;
  longshot_round::item_serial=9;longshot_round::held_lease=v.d.lease;
  assert(v.tick()&&published.preset==api::Preset::precision&&!published.pose.proximity);
  assert(published.pose.grip.position.x==nativelancerfollow::plan.prepared.world_grip[0].position.x);
  longshot_round::has_presentation=true;
  longshot_round::presented_grip=nativelancerfollow::plan.prepared.world_grip[0];
  longshot_round::presented_grip.position.x+=7;
  assert(v.tick()&&published.pose.grip.position.x==longshot_round::presented_grip.position.x);
  assert(nativelancerfollow::plan.prepared.world_grip[0].position.x==longshot_round::presented_grip.position.x-7);
  longshot_round::held_lease.id=8;assert(!v.tick());}
 for(unsigned reason=0;reason<13;++reason){Fixture v;
  switch(reason){case 0:fresh=false;break;case 1:selected=false;break;case 2:nativelancerfollow::plan.valid=false;break;
   case 3:nativelancerfollow::plan.key.hold_generation=2;break;case 4:++longshot_runtime::contact_frame;break;
   case 5:++longshot_runtime::contact_sync_ns;break;case 6:longshot_runtime::proposal_known=false;break;
   case 7:v.game.busy=true;break;case 8:v.f.controllers.hands[0].trigger=1;break;
   case 9:v.d.lease={hand::LeaseKind::support,0,v.f.key.lifecycle,99};break;
   case 10:nativelancerfollow::plan.prepared.world_pose_mask=0;break;case 11:v4_present=false;break;
   case 12:longshot_runtime::stage=longshot_runtime::Stage::waiting_round;break;}
  assert(!v.tick());}
 {Fixture v;assert(v.tick());++v.f.reference_generation;longshot_runtime::proposal_reference=v.f.reference_generation;
  nativelancerfollow::plan.prepared.world_grip[0].position.x+=6;assert(!v.tick());}
 for(bool backwards:{false,true}){Fixture v;assert(v.tick());
  v.f.controllers.sync_ns+=backwards?-1:100000001;longshot_runtime::contact_sync_ns=v.f.controllers.sync_ns;
  nativelancerfollow::plan.prepared.world_grip[0].position.x+=6;
  assert(!v.tick()); // Fresh current source cannot renew an expired/unknown old Snap.
 }
 {Fixture v;v.active(hand::LeaseKind::magazine);v4_present=false;
  assert(!v.tick()&&publications==0&&authored_publications==0); // No mismatched legacy fist.
 }
 std::puts("Longshot part publication: exact source/contact, passive5cm/keep7cm, real lease, release Snap, held-return rejection, precision round and raw-input preservation passed; no game fitting acceptance");
}
