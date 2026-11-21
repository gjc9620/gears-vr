#include "../vr-common/weapon_hand_client.h"
#define main previous_gt_advance_cases
#include "weapon_boomshot_advance_test.cpp"
#undef main
#include <atomic>
namespace gears_boomshot_grasp_pose {enum class Part {main_grip};}
namespace {
using uptr=std::uint64_t;FILE* log_file{};
Normal* runtime_fixture{};bool geometry_known=true,contact_known=true,near_contact=true,copy_allowed=true,manual=true;
unsigned geometry_getters{},pose_getters{},main_skin_calls{},contact_reads{};
namespace vrequip {struct Model {std::uint64_t hold_generation()const{return 10;}}model;}
namespace nativelancerfollow {
bool bound_key_known=true;struct Bound {h::ObjectIdentity weapon{};std::uint64_t hold_generation{10};}bound_key;
struct Mount {bool bound()const{return true;}}mount;
}
namespace unified_weapon_fire {bool manual_profile(){return manual;}}
namespace lancerreload_runtime {std::atomic<bool> stop_requested{};}
namespace lancerreload_inventory {
bool integer(uptr,wchar_t const* label,int& out){assert(!std::wcscmp(label,L"AmmoUsedCount"));out=owned_counts.used;return normal_fault!=60;}
}
namespace gears_boomshot_contact_fixture {struct Geometry {double progress{},distance_m{};};}
}
namespace gears_boomshot_contact {using Geometry=gears_boomshot_contact_fixture::Geometry;}
namespace {
namespace boomshot_contact {
void clear(){}
bool observe(h::Frame const&,gears_boomshot_contact::Geometry& out){++contact_reads;out.progress=bone_q;out.distance_m=near_contact?.02:.2;return contact_known;}
}
namespace boomshot_seating {
bool read(h::Frame const&,c::boomshot_insertion::Geometry& out){out=geometry_known?insertion_fixture():c::boomshot_insertion::Geometry{};return geometry_known;}
bool certify(h::Frame const&){++geometry_getters;return geometry_known;}
}
namespace boomshot_pose {bool certify(h::Frame const&){++pose_getters;return true;}}
namespace boomshot_grasp {bool publish_marcus115(h::Frame const&,gears_boomshot_grasp_pose::Part,gears_vr::Pose const&,bool){++main_skin_calls;return true;}}
namespace boomshot_skin {
bool support_geometry(h::Frame const&,gears_vr::Pose&)noexcept{return false;}
bool fill_support_mount(h::Frame const&,h::GripGameState&)noexcept{return false;}
bool publish_left(h::Frame const&,h::Decision const&,c::BoomshotMechanismGrant&)noexcept{return false;}
}
namespace boomshot_runtime {void propose(h::Frame const&,h::GripGameState&)noexcept;}
h::GripGameState base_game(){h::GripGameState g{};g.known=g.main_hand_held=true;g.kind=h::WeaponKind::firearm;return g;}
namespace weaponhand {
struct Client {
 bool armed()const{return copy_allowed;}
 std::uint32_t last_disarm_reason()const{return 0;}
 h::FrameReadObservation withdrawal_on_gt()const{return {};}
 bool copy_frame_on_gt(h::Frame& f,h::GripGameState& g,h::Decision& d,h::FrameReadObservation* observation=nullptr){
  if(observation){*observation={};if(!copy_allowed)observation->failure=h::FrameReadFailure::publication;}
  if(!copy_allowed)return false;assert(runtime_fixture);f=runtime_fixture->input.f;g=base_game();d=runtime_fixture->input.d;boomshot_runtime::propose(f,g);return true;
 }
}client;
}
#define BOOMSHOT_RUNTIME_SEATING_BOUNDARY_FIXTURE 1
#define BOOMSHOT_RUNTIME_SKIN_BOUNDARY_FIXTURE 1
namespace boomshot_guide {void tick(h::Frame const&,h::GripGameState const&,h::Decision const&,c::BoomshotMechanismGrant&)noexcept{}}
#define BOOMSHOT_RUNTIME_GUIDE_BOUNDARY_FIXTURE 1
namespace boomshot_feedback {void lost()noexcept{}void sample(h::Frame const&,h::GripGameState const&,h::Decision const&)noexcept{}}
#define BOOMSHOT_RUNTIME_FEEDBACK_BOUNDARY_FIXTURE 1
namespace boomshot_endpoint {void lost()noexcept{}void sample(h::Frame const&)noexcept{}}
#define BOOMSHOT_RUNTIME_ENDPOINT_BOUNDARY_FIXTURE 1
#include "weapon_boomshot_runtime.inl"
#undef BOOMSHOT_RUNTIME_ENDPOINT_BOUNDARY_FIXTURE
#undef BOOMSHOT_RUNTIME_FEEDBACK_BOUNDARY_FIXTURE
#undef BOOMSHOT_RUNTIME_GUIDE_BOUNDARY_FIXTURE
#undef BOOMSHOT_RUNTIME_SEATING_BOUNDARY_FIXTURE
#undef BOOMSHOT_RUNTIME_SKIN_BOUNDARY_FIXTURE
namespace runtime=boomshot_runtime;
void runtime_reset(Normal& x){
 boomshot_reload_owner::owner.~Owner();::new(static_cast<void*>(&boomshot_reload_owner::owner)) boomshot_reload_owner::Owner{};
 runtime::route.~BoomshotContactRoute();::new(static_cast<void*>(&runtime::route)) c::BoomshotContactRoute{};
 runtime::candidate={};runtime::controller_owner=0;runtime_fixture=&x;
 geometry_known=contact_known=near_contact=copy_allowed=manual=true;geometry_getters=pose_getters=main_skin_calls=contact_reads=0;
 nativelancerfollow::bound_key_known=true;nativelancerfollow::bound_key={x.input.f.weapon,10};lancerreload_runtime::stop_requested=false;
}
h::GripGameState native_observe(Normal& x){auto game=base_game();runtime::propose(x.input.f,game);return game;}
void native_commit(Normal& x){runtime::committed(x.input.f,x.input.d);}
void runtime_step(Normal& x){native_observe(x);native_commit(x);runtime::tick(x.input.f.controller.address);x.refresh();}
void runtime_open(Normal& x){runtime_step(x);assert(boomshot_reload_owner::owner.stage()==OwnerStage::prepared);x.next();runtime_step(x);
 assert(native_calls==1);x.next();runtime_step(x);x.next();hand_q=bone_q=.5;runtime_step(x);x.next();hand_q=bone_q=1;runtime_step(x);x.next();runtime_step(x);
 assert(x.core()->stage()==c::BoomshotStage::take_ammunition);}
void runtime_take(Normal& x){x.shoulder_grant();const auto game=native_observe(x);assert(game.request_ammo&&!game.request_magazine);
 native_commit(x);runtime::tick(x.input.f.controller.address);x.refresh();assert(x.core()->held_units()==1&&boomshot_carrier_original::owner.active());}
void runtime_insert(Normal& x){for(double depth:{.13,.1,.05,0.,0.}){x.next();boomshot_carrier_original::raw_depth=depth;runtime_step(x);}
 assert(x.core()->inventory().loaded==1&&inventory_writer_calls==1&&visual_seat_calls==1);}
void runtime_close(Normal& x){closing_grant(x);hand_q=bone_q=1;runtime_step(x);assert(boomshot_barrel::stroke.active());
 x.next();hand_q=bone_q=.5;runtime_step(x);x.next();hand_q=bone_q=0;runtime_step(x);x.next();runtime_step(x);}
}
int main(){unsigned count{};
 {Normal x(1);runtime_reset(x);const auto game=native_observe(x);assert(game.request_magazine&&!native_calls&&!pose_getters&&!geometry_getters);
  native_commit(x);assert(runtime::candidate.frozen&&runtime::route.grant().active()&&!native_calls&&!pose_getters&&!geometry_getters);
  runtime::tick(x.input.f.controller.address);assert(boomshot_reload_owner::owner.stage()==OwnerStage::prepared&&native_calls==0);
  x.next();runtime_step(x);x.next();runtime_step(x);x.next();hand_q=bone_q=.5;runtime_step(x);x.next();hand_q=bone_q=1;runtime_step(x);x.next();runtime_step(x);
  assert(x.core()->stage()==c::BoomshotStage::take_ammunition);runtime_take(x);runtime_insert(x);
  const auto frozen=native_observe(x);assert(frozen.object_incumbents[0].kind==h::LeaseKind::ammo&&!frozen.request_magazine&&!frozen.request_ammo);
  runtime_close(x);assert(!runtime::pending_work()&&!boomshot_reload_owner::owner.pending()&&native_calls==1&&inventory_writer_calls==1&&original_exit_calls==1&&original_reward_calls==1);
  const auto after=native_observe(x);assert(after.object_incumbents[0].kind==h::LeaseKind::magazine);++count;}
 {Normal x(2);runtime_reset(x);contact_known=false;auto game=native_observe(x);assert(game.magazine_observation_pending&&!game.request_magazine);
  x.input.d.new_grant=0;x.input.d.lease={};native_commit(x);contact_known=true;
  game=native_observe(x);assert(game.magazine_observation_pending&&!game.request_magazine&&runtime::candidate.frozen);runtime::tick(x.input.f.controller.address);
  assert(!boomshot_reload_owner::owner.pending()&&!native_calls);++count;}
 {Normal x(3);runtime_reset(x);geometry_known=false;lancerreload_inventory::owned_counts.used=0;x.refresh();auto game=native_observe(x);
  assert(game.request_magazine&&!game.magazine_observation_pending&&!native_calls);native_commit(x);runtime::tick(x.input.f.controller.address);
  assert(boomshot_reload_owner::owner.stage()==OwnerStage::prepared&&x.core()->inspection_only()&&!native_calls);
  x.next();runtime_step(x);x.next();runtime_step(x);x.next();hand_q=bone_q=.5;runtime_step(x);x.next();hand_q=bone_q=1;runtime_step(x);x.next();runtime_step(x);
  assert(x.core()->stage()==c::BoomshotStage::mechanism&&x.core()->step_index()==1);runtime_close(x);
  assert(!runtime::pending_work()&&!boomshot_reload_owner::owner.pending()&&!native_calls&&!inventory_writer_calls&&!original_exit_calls&&!original_reward_calls&&boomshot_inspection_clip::backend.finishes==1);++count;}
 {Normal x(4);runtime_reset(x);native_observe(x);++x.input.d.lease.id;native_commit(x);runtime::tick(x.input.f.controller.address);
  assert(!runtime::route.grant().active()&&!boomshot_reload_owner::owner.pending()&&!native_calls);++count;}
 {Normal x(5);runtime_reset(x);native_observe(x);native_commit(x);--x.input.f.controllers.sync_ns;native_observe(x);
  assert(runtime::candidate.poisoned&&!runtime::route.grant().active()&&!native_calls);runtime::tick(x.input.f.controller.address);assert(!native_calls);++count;}
 {Normal x(6);runtime_reset(x);native_observe(x);native_commit(x);auto game=base_game();game.object_incumbents[0]={h::LeaseKind::other,0,x.input.f.key.lifecycle,999};
  runtime::propose(x.input.f,game);assert(game.object_incumbents[0].id==999&&runtime::candidate.poisoned&&!runtime::route.grant().active());++count;}
 {Normal x(7);runtime_reset(x);runtime_open(x);runtime_take(x);normal_fault=35;runtime_insert(x);assert(boomshot_reload_owner::owner.stage()==OwnerStage::seating);
  x.next();const auto before=native_observe(x);assert(!before.request_magazine&&runtime::empty_slots(before.object_incumbents));native_commit(x);normal_fault=0;
  runtime::tick(x.input.f.controller.address);x.refresh();const auto after=native_observe(x);
  assert(runtime::candidate.frozen&&!after.request_magazine&&runtime::empty_slots(after.object_incumbents)&&visual_seat_calls==1&&native_calls==1);++count;}
 {Normal x(8);runtime_reset(x);runtime_open(x);const auto pc=x.input.f.controller.address;lancerreload_runtime::stop_requested=true;
  runtime::service_unselected(pc+1,x.input.f.weapon.address);assert(original_exit_calls==0&&runtime::pending_work());
  runtime::service_unselected(pc,x.input.f.weapon.address);assert(original_exit_calls==1&&!runtime::pending_work()&&boomshot_reload_owner::owner.blocks_fire(x.input.f));++count;}
 {Normal x(9);runtime_reset(x);runtime_open(x);runtime_take(x);runtime::service_unselected(x.input.f.controller.address,0x8888);
  assert(!runtime::pending_work()&&original_exit_calls==1&&!boomshot_carrier_original::owner.active()&&inventory_writer_calls==0&&x.core()->inventory().reserve==2);++count;}
 {Normal x(10);runtime_reset(x);contact_known=false;auto game=native_observe(x);assert(game.magazine_observation_pending&&!game.request_magazine);
  contact_known=true;near_contact=false;x.next();game=native_observe(x);assert(!game.magazine_observation_pending&&!game.request_magazine);++count;}
 {Normal x(11);runtime_reset(x);runtime_open(x);copy_allowed=false;const auto calls=native_calls;runtime::tick(x.input.f.controller.address);
  assert(runtime::pending_work()&&native_calls==calls&&original_exit_calls==0&&inventory_writer_calls==0);++count;}
 {Normal x(12);runtime_reset(x);runtime_open(x);auto foreign=lancerreload_inventory::identity(x.input.f);foreign.weapon=object(0x7000,77);lancerreload_state::lease={foreign,888,true};
  runtime::service_unselected(x.input.f.controller.address,foreign.weapon.address);
  assert(runtime::pending_work()&&lancerreload_state::lease.serial==888&&original_exit_calls==0);++count;}
 {Normal x(13);runtime_reset(x);geometry_known=false;runtime_step(x);
  assert(runtime::route.grant().active()&&!boomshot_reload_owner::owner.pending()&&!native_calls&&!boomshot_inspection_clip::backend.plays);++count;}
 {Normal x(14);runtime_reset(x);lancerreload_inventory::owned_counts={1,1,0,false};x.refresh();runtime_step(x);x.next();runtime_step(x);
  assert(boomshot_reload_owner::owner.pending()&&x.core()->inspection_only()&&boomshot_inspection_clip::owner.active());
  lancerreload_runtime::stop_requested=true;runtime::service_unselected(x.input.f.controller.address,x.input.f.weapon.address);
  assert(!runtime::pending_work()&&boomshot_reload_owner::owner.blocks_fire(x.input.f)&&!native_calls&&!original_exit_calls&&boomshot_inspection_clip::backend.finishes==1);++count;}
 std::printf("Boomshot Native-to-GT runtime: %u cases passed; production propose/commit/tick + actual owner/core/events/raw Stroke, callbacks data-only, exact committed contribution incl no-part frozen, no source/foreign-holder masking, once-only full fixture reload and cleanup; geometry/engine/Client-copy boundaries stubbed, actual Native arbitration/source copy and game not proved\n",count);return 0;
}
