#include <optional>
unsigned ui_steps{},bone_receipt_captures{},borrowed_bone_ticks{};
#include "../vr-common/weapon_hand_participant.h"
#include "../vr-common/weapon_hand_client.h"
#include "../vr-common/weapon_support_grip.h"
#include "../vr-common/rig_tracking_frame.h"
#include "../vr-common/body_slots.h"
#include "../vr-common/weapon_reload_transaction.h"
#include "../vr-common/weapon_longshot_reload.h"
#include "../vr-common/weapon_longshot_bolt_stroke.h"
#include "../vr-common/weapon_longshot_bolt_contact.h"
#include "../vr-common/weapon_longshot_round_presentation.h"
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <crtdbg.h>
using uptr=std::uintptr_t;namespace hand=gears_weapon_hand;
using P=gears_vr::Pose;using V=gears_vr::Vector;
FILE* log_file{};
hand::Frame actual_frame{};hand::GripGameState actual_game{};hand::Decision actual_decision{};
float game_seconds=1.5f;bool fresh=true,rig_matches=true,resource_ok=true,native_exit_ok=true,write_rejected=false,clip_restore_ok=true;
unsigned inventory_calls{},rewards{},native_exits{},guide_calls{},ui_timing_calls{};
bool prepare_expires_source{};
std::uint64_t mock_hold_generation=1;
P actual_item{};
bool strict_begin_observation{},guide_expires_source{};unsigned begin_observation_difference{};
bool begin_observation_current(hand::Frame const&);
namespace weaponhand {
unsigned current_gt(void*){return 1;}
std::int64_t clock_ns(void*){return actual_frame.controllers.sync_ns;}
struct Client {bool copy_frame_on_gt(hand::Frame& f,hand::GripGameState& g,hand::Decision& d){f=actual_frame;g=actual_game;d=actual_decision;return fresh;}}client;
}
namespace lancerreload_inventory {
struct Identity {hand::ObjectIdentity world{},pawn{},weapon{};std::uint64_t game_start{},native_start{},lifecycle{},input{},hold_generation{},session{},space{};
 bool valid()const{return weapon.address&&game_start&&native_start&&lifecycle;}
 bool operator==(Identity const& b)const{return hand::same(world,b.world)&&hand::same(pawn,b.pawn)&&hand::same(weapon,b.weapon)&&game_start==b.game_start&&native_start==b.native_start&&lifecycle==b.lifecycle&&input==b.input&&hold_generation==b.hold_generation;}};
using Inventory=gears_weapon::ReloadTxnInventory<Identity>;using Request=gears_weapon::ReloadTxnRequest<Identity>;
using Receipt=gears_weapon::ReloadTxnReceipt<Identity>;using Transaction=gears_weapon::ReloadTransaction<Identity>;
Identity identity(hand::Frame const& f){return {f.world,f.pawn,f.weapon,f.key.game_start,f.key.native_started_qpc,f.key.lifecycle,f.controllers.input_epoch,mock_hold_generation};}
bool same_instance_across_hold(Identity a,Identity b){if(b.lifecycle<a.lifecycle||b.hold_generation<a.hold_generation)return false;b.lifecycle=a.lifecycle;b.hold_generation=a.hold_generation;return a==b;}
Inventory actual{};
bool counts_equal(Inventory const& a,Inventory const& b){return a.identity==b.identity&&a.loaded==b.loaded&&a.reserve==b.reserve&&a.capacity==b.capacity&&a.infinite_reserve==b.infinite_reserve;}
struct ObservedInventory {
 Identity bound{};
 bool sample(hand::Frame const& f,Inventory& out){if(!fresh||!(actual.identity==identity(f))||(bound.valid()&&!(bound==identity(f))))return false;bound=identity(f);out=actual;return true;}
 bool sample_owned(Identity const& id,Inventory& out){auto actual_id=actual.identity;actual_id.lifecycle=id.lifecycle;actual_id.hold_generation=id.hold_generation;if(!(bound==id)||!(actual_id==id))return false;out=actual;out.identity=bound;return true;}
 bool rebind_hold(hand::Frame const& f,Identity const& expected,Inventory& out){
  if(!(bound==expected)||!same_instance_across_hold(expected,identity(f))||!fresh||!sample_owned(expected,out))return false;
  bound=identity(f);out.identity=bound;return true;
 }
};
enum class CommitResult {committed,rejected,reconciliation_required};
struct InsertionWriter {template<class Owns>CommitResult commit(hand::Frame const& f,Request const& request,ObservedInventory&,Owns owns,Receipt& out){
 if(write_rejected){out={request.token,false,actual,gears_weapon::ReloadTxnResult::none};return CommitResult::rejected;}
 if(!owns(f,request.reload_serial)||!counts_equal(actual,request.before))return CommitResult::rejected;
 ++inventory_calls;actual=request.expected;actual.revision=request.before.revision+1;out={request.token,true,actual,gears_weapon::ReloadTxnResult::none};return CommitResult::committed;}};
}
namespace vrequip {
struct Model {std::uint64_t hold_generation(){return mock_hold_generation;}}model;
struct MeshPoseProbe {uptr mesh{},asset{};float local_to_world[16]{};};
bool mesh_pose_probe(uptr,MeshPoseProbe& out){out.mesh=4;out.asset=5;return fresh;}
}
namespace nativelancerfollow {
bool bound_key_known=true,bound_is_longshot=true;double bound_units=100;
struct Key {hand::ObjectIdentity weapon{},component{},asset{};std::uint64_t hold_generation=1;}bound_key;
}
namespace nativeequipment {
gears_weapon::ArmspanCalibration calibration=gears_weapon::default_body_slot_layout();
bool current(void*,hand::Frame const&){return fresh;}
}
namespace gears_equipment_rig {enum class Status {matched,rejected};Status match(hand::Frame const&,gears_rig_tracking::Frame const&,std::int64_t,unsigned){return rig_matches?Status::matched:Status::rejected;}}
namespace handrig {struct Result {gears_rig_tracking::Reply reply;};Result read(gears_rig_tracking::Frame& rig,unsigned,unsigned){rig={};rig.world_pose_mask=gears_rig_tracking::right_grip_pose;return {gears_rig_tracking::Reply::accepted};}}
namespace hammerburst_runtime {bool matrix(std::array<float,16> const&,P& out,double& size){out={};size=1;return true;}}
namespace hammerburst_animation {struct {bool active{},stopped{};}ownership;}
namespace lancerreload_visual {bool active{};}
namespace weapon_part_inputs {std::atomic<bool> original_source_enabled{true};}
namespace lancerreload_runtime {std::atomic<bool> stop_requested{},outstanding{};bool weapon_retired(hand::ObjectIdentity const&){return false;}}
namespace lancerreload_state {
namespace inv=lancerreload_inventory;
struct {bool acquired{};float reward_start{};std::uint64_t serial{};}lease;
struct RewardReceipt {bool applied{},eligibility_known{};inv::Identity eligibility_owner{};};
bool capture_reward_eligibility(inv::Inventory const& opening,RewardReceipt& receipt){
 if(!inv::counts_equal(opening,inv::actual))return false;
 receipt.eligibility_known=true;receipt.eligibility_owner=opening.identity;return true;
}
enum class BeginResult {rejected,acquired,recovery_required};
bool time_field(uptr,const wchar_t*,float& out){out=game_seconds;return fresh;}
BeginResult begin(hand::Frame const& f,std::uint64_t& serial){
 if(!fresh){serial=0;return BeginResult::rejected;}
 if(strict_begin_observation&&!begin_observation_current(f)){serial=0;return BeginResult::rejected;}
 serial=100;lease={true,game_seconds,serial};return BeginResult::acquired;}
bool owns(hand::Frame const&,std::uint64_t serial){return fresh&&lease.acquired&&lease.serial==serial;}
bool set_ui_timing(hand::Frame const&,std::uint64_t,std::int64_t,std::int64_t,bool){++ui_timing_calls;return fresh;}
bool tick_ui_timing(){++ui_steps;return true;}
bool recover_owned(){if(!native_exit_ok)return false;if(lease.acquired)++native_exits;lease={};return true;}
bool recover_serial(std::uint64_t serial){return !serial||!lease.acquired||lease.serial!=serial||recover_owned();}
bool restore_partial_insert(inv::Request const&,inv::ObservedInventory&){return true;}
bool perfect_reward(inv::Request const& request,inv::Inventory const& opening,inv::ObservedInventory&,RewardReceipt& receipt){
 assert(request.result==gears_weapon::ReloadTxnResult::perfect);
 if(!receipt.eligibility_known||!inv::same_instance_across_hold(receipt.eligibility_owner,opening.identity)||!(opening.identity==request.identity))return false;
 if(!receipt.applied){++rewards;receipt.applied=true;}return true;
}
}
namespace longshot_clip {
bool owned(){return hammerburst_animation::ownership.active;}
enum class Result {rejected,pending,ready,recovery_required};
Result next_service=Result::ready;bool last_original_mode{},ui_ack_ok=true;unsigned ui_ack_calls{};
Result start(hand::Frame const&,std::uint64_t,bool original=false){last_original_mode=original;hammerburst_animation::ownership.active=true;return Result::ready;}
Result service(hand::Frame const&){const auto result=next_service;next_service=Result::ready;return result;}
bool restore(){if(!clip_restore_ok)return false;hammerburst_animation::ownership.active=false;return true;}
bool acknowledge_ui_start(std::uint64_t){++ui_ack_calls;return ui_ack_ok&&fresh&&hammerburst_animation::ownership.active;}
}
namespace longshot_fire {
struct {lancerreload_inventory::Identity identity{};gears_weapon::LongshotShot shot{};bool active{};}confirmed;
}
namespace longshot_bolt {
struct Validation {explicit Validation(hand::Frame const&) {}};
struct BonesReceipt {unsigned ui_sequence{};};
BonesReceipt capture_bones(hand::Frame const&,P,P,double){++bone_receipt_captures;return {ui_steps};}
using Result=gears_weapon::longshot_tracks::BoltResult;
Result next_result=Result::waiting;bool prepared{};V handle_in_bone{};
struct Stroke {bool running{};void release(){running=false;}bool active()const{return running;}}stroke;
V sub(V a,V b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
V mul(V a,double n){return {a.x*n,a.y*n,a.z*n};}
V point(P const& p,V v,double scale=1){auto d=gears_weapon::rotate(p.orientation,mul(v,scale));return {p.position.x+d.x,p.position.y+d.y,p.position.z+d.z};}
V local(P const& p,V v,double scale=1){return mul(gears_weapon::rotate(gears_vr::inverse(p.orientation),sub(v,p.position)),1/scale);}
bool bones(hand::Frame const&,P& root,P& part,double& size){root=part={};size=1;return fresh;}
bool granted(hand::Frame const& f,hand::Decision const& d){return hand::same(f.key,d.key)&&!d.release_required&&d.lease.kind==hand::LeaseKind::magazine&&d.lease.id==f.press_token&&f.left_tracked&&f.left_grip_known&&f.left_grip_down;}
bool prepare(hand::Frame const&){prepared=true;if(prepare_expires_source)fresh=false;return true;}
bool prepare_serviced(hand::Frame const& f){return prepare(f);}
bool begin(hand::Frame const& f,hand::Decision const& d,gears_weapon::longshot_tracks::BoltDirection,hand::Lease const& accepted){if(!granted(f,d)||(!d.new_grant&&!hand::same(accepted,d.lease)))return false;stroke.running=true;return true;}
Result tick(hand::Frame const&,hand::Decision const&,BonesReceipt* receipt=nullptr){if(receipt){assert(receipt->ui_sequence==ui_steps);++borrowed_bone_ticks;}const auto r=next_result;next_result=Result::waiting;if(r==Result::complete||r==Result::interrupted)stroke.running=false;return r;}
void reset(){prepared=false;stroke.running=false;next_result=Result::waiting;}
}
namespace snub_visual {double length(V v){return std::hypot(std::hypot(v.x,v.y),v.z);}}
namespace longshot_round {
namespace presentation=gears_weapon_longshot_round_presentation;
presentation::State displayed;
std::uint64_t presentation_cycle{};
bool ready=true;std::uint64_t item_serial{};
bool current(hand::Frame const&){return fresh&&resource_ok;}
bool granted(hand::Frame const& f,hand::Decision const& d){return hand::same(f.key,d.key)&&!d.release_required&&d.lease.kind==hand::LeaseKind::ammo&&d.lease.id==f.press_token&&f.left_grip_down&&f.left_grip_known&&f.left_tracked;}
bool capture_shoulder(hand::Frame const& f,hand::Decision const& d,std::uint64_t item){if(!granted(f,d)||!d.new_grant)return false;item_serial=item;return true;}
bool physical_pose(hand::Frame const&,hand::Decision const&,P& out){out=actual_item;return fresh&&item_serial;}
bool prepare_presentation(hand::Frame const& f,hand::Decision const& d,P root,V port,double scale,std::uint64_t cycle){
 presentation_cycle=cycle;return current(f)&&granted(f,d)&&displayed.prepare(f,d.lease,item_serial,1,cycle,actual_item,{},root,port,scale,100);}
bool presentation_sample(hand::Frame const& f,hand::Decision const& d,presentation::Sample& out){
 if(!current(f)||!granted(f,d)||!displayed.matches(f,d.lease,item_serial,1,presentation_cycle))return false;out=displayed.sample();return true;}
bool show(hand::Frame const& f,hand::Decision const& d){return current(f)&&granted(f,d)&&item_serial;}
bool hide(){item_serial=0;displayed.reset();return true;}
bool restore(){item_serial=0;displayed.reset();return true;}
}
namespace longshot_scope_owner {bool yield_ok=true;bool yield(hand::Frame const&) noexcept {return yield_ok;}}
#include "weapon_longshot_runtime.inl"
namespace longshot_runtime {void guide(hand::Frame const&)noexcept {++guide_calls;if(guide_expires_source)fresh=false;}}
using Stage=longshot_runtime::Stage;
bool begin_observation_current(hand::Frame const& f){
 auto current=actual_game;current.request_ammo=current.request_magazine=false;current.object_incumbents={};
 longshot_runtime::propose(f,current);
 begin_observation_difference=hand::game_difference(current,actual_game);
 return hand::same_pose_consumer_observation(f,actual_decision,current,actual_game);
}
struct Fixture {
 Fixture(bool shot=true){
  ui_timing_calls=0;prepare_expires_source=false;
  longshot_runtime::reset();longshot_runtime::debts={};longshot_runtime::last_intent=0;longshot_runtime::last_intent_identity={};
  longshot_runtime::completed_guide_shot={};
  longshot_runtime::no_ammo_cue={};
  inventory_calls=rewards=native_exits=guide_calls=0;fresh=rig_matches=resource_ok=native_exit_ok=clip_restore_ok=true;
  write_rejected=false;
  strict_begin_observation=guide_expires_source=false;begin_observation_difference=0;
  longshot_scope_owner::yield_ok=true;
  longshot_clip::next_service=longshot_clip::Result::ready;longshot_clip::last_original_mode=false;
  longshot_clip::ui_ack_ok=true;longshot_clip::ui_ack_calls=0;
  mock_hold_generation=1;
  game_seconds=1.5f;actual_frame={};actual_decision={};actual_game={};actual_frame.controller.address=55;
  actual_frame.world.address=1;actual_frame.pawn.address=2;actual_frame.weapon.address=3;actual_frame.key.game_start=1;actual_frame.key.native_started_qpc=2;
  actual_frame.key.lifecycle=1;actual_frame.key.frame=1;actual_frame.reference_generation=1;actual_frame.stereo.session=1;
  actual_frame.controllers.sequence=1;actual_frame.controllers.sync_ns=1000000000;
  actual_frame.controllers.input_epoch=1;
  for(auto& h:actual_frame.controllers.hands){h.grip_active=1;h.grip_flags=15;}
  actual_frame.left_tracked=actual_frame.left_grip_known=1;
  actual_game.known=actual_game.main_hand_held=1;actual_game.kind=hand::WeaponKind::firearm;
  nativelancerfollow::bound_key={actual_frame.weapon,{4},{5},1};
  lancerreload_inventory::actual={lancerreload_inventory::identity(actual_frame),1,1,0,5,false};
  lancerreload_state::lease={};hammerburst_animation::ownership.active=false;lancerreload_runtime::stop_requested=false;
  longshot_fire::confirmed={lancerreload_inventory::identity(actual_frame),{},shot};longshot_fire::confirmed.shot.confirmed=shot;longshot_fire::confirmed.shot.game_tick=1000000;
  longshot_fire::confirmed.shot.original_event_sequence=shot?1:0;
 }
 void step(){++actual_frame.key.frame;++actual_frame.controllers.sequence;actual_frame.controllers.sync_ns+=20000000;actual_decision.key=actual_frame.key;assert(longshot_runtime::tick(55));actual_decision.new_grant=0;}
 void press(hand::LeaseKind kind,std::uint64_t token){actual_frame.left_grip_down=1;actual_frame.press_token=token;actual_decision.key=actual_frame.key;actual_decision.press_token=token;
  actual_decision.lease={kind,0,actual_frame.key.lifecycle,token};actual_decision.status=hand::DecisionStatus::granted;actual_decision.new_grant=1;actual_decision.release_required=0;
  if(strict_begin_observation){
   ++actual_frame.key.frame;++actual_frame.controllers.sequence;actual_frame.controllers.sync_ns+=20000000;actual_decision.key=actual_frame.key;
   actual_frame.controllers.hands[0].grip.position={.1412027,.09274045,.04996176};
   assert(longshot_runtime::bind_contact(actual_frame));
   actual_game.request_ammo=actual_game.request_magazine=false;actual_game.object_incumbents={};
   longshot_runtime::propose(actual_frame,actual_game);assert(actual_game.request_magazine);
   longshot_runtime::committed(actual_frame,actual_decision);assert(longshot_runtime::tick(55));actual_decision.new_grant=0;return;
  }
  longshot_runtime::committed(actual_frame,actual_decision);step();}
 void release(){actual_frame.left_grip_down=0;actual_decision.release_required=1;actual_decision.lease={};step();}
 void open(std::uint64_t token=1){press(hand::LeaseKind::magazine,token);assert(longshot_runtime::stage==Stage::acquiring);
  step();if(!longshot_runtime::standalone){step();step();}assert(longshot_runtime::stage==Stage::opening);
  longshot_bolt::next_result=longshot_bolt::Result::complete;step();release();}
 void tip(double depth){auto port=longshot_runtime::port_local;actual_item.orientation={0,0,-.7071067811865475,.7071067811865475};actual_item.position={(port.x-depth)*100-15,port.y*100,port.z*100};step();}
 void load(std::uint64_t token=2){assert(longshot_runtime::stage==Stage::waiting_round);press(hand::LeaseKind::ammo,token);assert(longshot_runtime::stage==Stage::holding_round);
  tip(.08);tip(.06);tip(.03);tip(.005);assert(longshot_runtime::stage==Stage::waiting_close);assert(inventory_calls==1);release();}
 gears_weapon::ReloadTxnResult close(float seconds,std::uint64_t token=3){press(hand::LeaseKind::magazine,token);assert(longshot_runtime::stage==Stage::closing);
  game_seconds=seconds;longshot_bolt::next_result=longshot_bolt::Result::complete;step();assert(longshot_runtime::stage==Stage::completing);
  auto result=longshot_runtime::completion.result;step();step();assert(longshot_runtime::stage==Stage::idle);return result;}
};
int main(){
 _set_error_mode(_OUT_TO_STDERR);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
 _CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
 {Fixture f;assert(tmpfile_s(&log_file)==0&&log_file);f.open();f.load();
  assert(f.close(5.f)==gears_weapon::ReloadTxnResult::ordinary);
  rewind(log_file);wchar_t line[4096]{};bool saw_closed{};
  while(fgetws(line,4096,log_file))if(std::wcsstr(line,L"longshot_reload_closed ")){
   assert(std::wcsstr(line,L"shot_tick_us=1000000 closed_tick_us=5000000 elapsed_us=4000000 request_result=2 shot_event=1"));saw_closed=true;}
  assert(saw_closed);fclose(log_file);log_file=nullptr;}
 {Fixture f(false);lancerreload_inventory::actual.loaded=1;
  assert(!longshot_runtime::guide_needed(actual_frame));}
 {Fixture f;assert(longshot_runtime::guide_needed(actual_frame));f.open();f.load();
  assert(longshot_runtime::guide_needed(actual_frame));f.close(5.f);
  assert(!longshot_runtime::guide_needed(actual_frame));
  assert(longshot_fire::confirmed.active&&longshot_fire::confirmed.shot.game_tick==1000000);
  ++actual_frame.key.lifecycle;++mock_hold_generation;
  assert(!longshot_runtime::guide_needed(actual_frame));
  longshot_fire::confirmed.identity=lancerreload_inventory::identity(actual_frame);
  ++longshot_fire::confirmed.shot.original_event_sequence;
  assert(longshot_runtime::guide_needed(actual_frame));}
 {Fixture f;f.open();lancerreload_runtime::stop_requested=true;f.step();
  assert(longshot_runtime::stage==Stage::idle&&longshot_runtime::guide_needed(actual_frame));}
 {Fixture f;f.open();f.load();lancerreload_runtime::stop_requested=true;f.step();
  assert(longshot_runtime::stage==Stage::idle&&longshot_runtime::guide_needed(actual_frame));
  lancerreload_runtime::stop_requested=false;f.open(4);f.close(5.f,5);
  assert(!longshot_runtime::guide_needed(actual_frame));}
 {Fixture f;f.open();f.load();f.press(hand::LeaseKind::magazine,3);
  longshot_bolt::next_result=longshot_bolt::Result::complete;f.step();
  assert(longshot_runtime::stage==Stage::completing&&!longshot_runtime::guide_needed(actual_frame));
  native_exit_ok=false;f.step();assert(!longshot_runtime::guide_needed(actual_frame));
  native_exit_ok=true;f.step();f.step();assert(!longshot_runtime::guide_needed(actual_frame));}
 {Fixture f;auto wrong=actual_frame;wrong.weapon.address=999;
  assert(!longshot_runtime::guide_needed(wrong));longshot_fire::confirmed.shot.confirmed=false;
  assert(!longshot_runtime::guide_needed(actual_frame));}
 std::puts("Longshot guide lifecycle: loaded/complete hidden, confirmed-shot/retry/debt cues, completion cleanup and new-shot rearm passed; no image claim");
 {Fixture f(false);lancerreload_inventory::actual.reserve=0;f.press(hand::LeaseKind::magazine,1);
  assert(longshot_runtime::stage==Stage::idle&&longshot_runtime::no_ammo_hint(actual_frame));
  assert(longshot_runtime::guide_needed(actual_frame)&&inventory_calls==0&&!lancerreload_state::lease.acquired);
  const auto began=longshot_runtime::no_ammo_cue.source.controllers.sync_ns;
  f.step();longshot_runtime::note_no_ammo(actual_frame);
  assert(longshot_runtime::no_ammo_cue.source.controllers.sync_ns==began);
  auto later=actual_frame;later.controllers.sync_ns=began+749999999;assert(longshot_runtime::no_ammo_hint(later));
  later.controllers.sync_ns=began+750000000;assert(!longshot_runtime::no_ammo_hint(later));
  f.press(hand::LeaseKind::magazine,2);assert(longshot_runtime::no_ammo_hint(actual_frame));
  assert(longshot_runtime::no_ammo_cue.token==2&&longshot_runtime::no_ammo_cue.source.controllers.sync_ns>began);
  assert(inventory_calls==0&&rewards==0&&longshot_fire::confirmed.shot.game_tick==1000000);}
 {Fixture f;lancerreload_inventory::actual.reserve=0;f.press(hand::LeaseKind::magazine,1);
  for(unsigned change=0;change<7;++change){auto wrong=actual_frame;
   switch(change){case 0:++wrong.weapon.address;break;case 1:++wrong.reference_generation;break;
    case 2:++wrong.controllers.input_epoch;break;case 3:++wrong.stereo.space_epoch;break;
    case 4:++wrong.key.game_start;break;case 5:wrong.controllers.source=gears_vr::StereoSource::MetaOperatorSimulated;break;
    case 6:--wrong.key.frame;break;}
   assert(!longshot_runtime::no_ammo_hint(wrong));}
  ++nativelancerfollow::bound_key.hold_generation;assert(!longshot_runtime::no_ammo_hint(actual_frame));}
 {Fixture f;lancerreload_inventory::actual.reserve=0;lancerreload_inventory::actual.infinite_reserve=true;
  f.press(hand::LeaseKind::magazine,1);assert(longshot_runtime::stage==Stage::acquiring&&!longshot_runtime::no_ammo_hint(actual_frame));}
 {Fixture f;lancerreload_inventory::actual.reserve=0;lancerreload_inventory::actual.loaded=1;
  f.press(hand::LeaseKind::magazine,1);assert(longshot_runtime::stage==Stage::idle&&!longshot_runtime::no_ammo_hint(actual_frame));}
 {Fixture f;lancerreload_inventory::actual.reserve=0;fresh=false;actual_frame.left_grip_down=1;actual_frame.press_token=1;
  assert(!longshot_runtime::tick(55)&&!longshot_runtime::no_ammo_cue.active&&inventory_calls==0);}
 {Fixture f;lancerreload_inventory::actual.reserve=0;f.press(hand::LeaseKind::magazine,1);
  assert(longshot_runtime::no_ammo_hint(actual_frame));lancerreload_inventory::actual.reserve=5;
  f.press(hand::LeaseKind::magazine,2);assert(longshot_runtime::stage==Stage::acquiring&&!longshot_runtime::no_ammo_hint(actual_frame));
  assert(inventory_calls==0&&longshot_runtime::started_us==1000000);}
 std::puts("Longshot no-ammo feedback: actual finite-empty refusal only, one cue per intent,750ms cosmetic expiry, context/hold rejection, infinite/loaded/read-failure suppression and original shot clock preserved");
 {Fixture f;ui_timing_calls=0;prepare_expires_source=true;f.press(hand::LeaseKind::magazine,1);f.step();
  assert(longshot_runtime::stage==Stage::acquiring&&ui_timing_calls==0&&!fresh);
  prepare_expires_source=false;fresh=true;f.step();assert(ui_timing_calls==1&&longshot_runtime::stage==Stage::acquiring);
  f.step();assert(longshot_runtime::stage==Stage::opening&&inventory_calls==0);}
 {Fixture f;f.press(hand::LeaseKind::magazine,1);longshot_clip::ui_ack_ok=false;f.step();f.step();
  assert(longshot_clip::ui_ack_calls==1&&longshot_runtime::stage==Stage::idle&&native_exits==1&&inventory_calls==0);}
 {Fixture f;f.press(hand::LeaseKind::magazine,1);f.step();const auto frame=actual_frame.key.frame;
  for(unsigned i=0;i<3;++i)assert(longshot_runtime::tick(55)&&actual_frame.key.frame==frame&&ui_timing_calls==0);
  f.step();assert(ui_timing_calls==1&&longshot_runtime::stage==Stage::acquiring&&inventory_calls==0);}
 {Fixture f;f.press(hand::LeaseKind::magazine,1);f.step();fresh=false;f.step();
  assert(ui_timing_calls==0&&inventory_calls==0);fresh=true;actual_frame.left_grip_down=0;f.step();
  assert(longshot_runtime::stage==Stage::idle&&ui_timing_calls==0&&longshot_runtime::ui_prepare_frame==0);}
 {Fixture f;f.press(hand::LeaseKind::magazine,1);f.step();actual_decision.lease={};f.step();
  assert(longshot_runtime::stage==Stage::idle&&ui_timing_calls==0&&inventory_calls==0);}
 {Fixture f;guide_expires_source=true;f.press(hand::LeaseKind::magazine,1);
  assert(longshot_runtime::stage==Stage::acquiring&&lancerreload_state::lease.acquired&&fresh&&guide_calls==0);}
 {Fixture f;strict_begin_observation=true;actual_game.request_magazine=true;
  f.press(hand::LeaseKind::magazine,1);
  std::fprintf(stderr,"same-frame original-begin observed game_difference=%u\n",begin_observation_difference);
  assert(longshot_runtime::stage==Stage::acquiring&&lancerreload_state::lease.acquired&&begin_observation_difference==0);
  for(unsigned reason=0;reason<6;++reason){
   auto source=actual_frame;auto current=actual_game;
   current.request_ammo=current.request_magazine=false;current.object_incumbents={};
   switch(reason){case 0:current.busy=true;break;case 1:current.animation_owns_hands=true;break;
    case 2:current.object_incumbents[0]={hand::LeaseKind::ammo,0,source.key.lifecycle,99};break;
    case 3:++source.reference_generation;break;case 4:source.controllers.hands[0].grip.position.x+=.01;break;
    case 5:current.main_hand_held=false;break;}
   longshot_runtime::propose(source,current);
   assert(!hand::same_pose_consumer_observation(source,actual_decision,current,actual_game));
   if(reason==2)assert(current.object_incumbents[0].id==99);
  }
  auto later=actual_frame;++later.key.frame;++later.controllers.sequence;later.controllers.sync_ns+=20000000;
  auto current=actual_game;current.request_ammo=current.request_magazine=false;current.object_incumbents={};
  longshot_runtime::propose(later,current);
  assert(!current.request_magazine&&hand::same(current.object_incumbents[0],actual_decision.lease));}
#if !defined(GEARS_LONGSHOT_TEST_PRE_SCOPE)
 {Fixture f;longshot_runtime::proposal_known=true;longshot_runtime::proposal_identity=lancerreload_inventory::identity(actual_frame);
  longshot_scope_owner::yield_ok=false;f.press(hand::LeaseKind::magazine,1);
  assert(longshot_runtime::stage==Stage::idle&&!lancerreload_state::lease.acquired&&inventory_calls==0);
  longshot_scope_owner::yield_ok=true;f.step();assert(longshot_runtime::stage==Stage::acquiring);}
#endif
 for(auto result:{longshot_clip::Result::rejected,longshot_clip::Result::recovery_required}){
  Fixture f;f.press(hand::LeaseKind::magazine,1);assert(longshot_clip::last_original_mode);
  longshot_clip::next_service=result;f.step();
  assert(longshot_runtime::stage==Stage::idle&&!lancerreload_state::lease.acquired&&native_exits==1&&inventory_calls==0);
 }
 {Fixture f;f.press(hand::LeaseKind::magazine,1);longshot_clip::next_service=longshot_clip::Result::pending;f.step();
  assert(longshot_runtime::stage==Stage::acquiring&&native_exits==0&&inventory_calls==0);}
 for(float seconds:{4.5f,5.f,5.5f}){Fixture f;f.open();f.load();assert(longshot_runtime::blocks_fire(actual_frame.weapon));
  const auto expected=seconds<5?gears_weapon::ReloadTxnResult::perfect:gears_weapon::ReloadTxnResult::ordinary;
  assert(f.close(seconds)==expected);assert(lancerreload_inventory::actual.loaded==1&&lancerreload_inventory::actual.reserve==4);
  assert(inventory_calls==1&&rewards==unsigned(expected==gears_weapon::ReloadTxnResult::perfect)&&native_exits==1&&!longshot_runtime::blocks_fire(actual_frame.weapon));}
 {Fixture f(false);f.open();f.load();assert(f.close(2.f)==gears_weapon::ReloadTxnResult::ordinary&&rewards==0);}
 {Fixture f;f.open();f.press(hand::LeaseKind::ammo,2);assert(longshot_runtime::stage==Stage::holding_round);f.release();
  assert(longshot_runtime::stage==Stage::waiting_round&&inventory_calls==0&&lancerreload_inventory::actual.reserve==5);f.load(3);f.close(4.5f,4);}
 {Fixture f;f.open();f.load();lancerreload_runtime::stop_requested=true;f.step();assert(longshot_runtime::stage==Stage::idle&&longshot_runtime::blocks_fire(actual_frame.weapon));
  lancerreload_runtime::stop_requested=false;f.open(4);assert(longshot_runtime::stage==Stage::waiting_close);f.close(6.f,5);
  assert(inventory_calls==1&&lancerreload_inventory::actual.reserve==4&&!longshot_runtime::blocks_fire(actual_frame.weapon));}
 {Fixture f;f.open();actual_game.busy=1;f.step();assert(longshot_runtime::stage==Stage::idle&&inventory_calls==0&&native_exits==1);}
 {Fixture f;f.open();f.load();f.press(hand::LeaseKind::magazine,3);game_seconds=4.5f;
  longshot_bolt::next_result=longshot_bolt::Result::complete;f.step();native_exit_ok=false;f.step();
  assert(longshot_runtime::stage==Stage::completing&&rewards==0&&inventory_calls==1);
  native_exit_ok=true;f.step();f.step();assert(longshot_runtime::stage==Stage::idle&&rewards==1&&inventory_calls==1);}
 {Fixture f;f.open();f.load();f.press(hand::LeaseKind::magazine,3);game_seconds=4.5f;
  longshot_bolt::next_result=longshot_bolt::Result::complete;f.step();lancerreload_runtime::stop_requested=true;f.step();
  assert(longshot_runtime::stage==Stage::completing&&inventory_calls==1);
  f.step();assert(longshot_runtime::stage==Stage::idle&&rewards==1&&!lancerreload_runtime::outstanding);}
 {Fixture f;f.open();f.press(hand::LeaseKind::ammo,2);write_rejected=true;f.tip(.08);f.tip(.06);f.tip(.03);f.tip(.005);
  assert(longshot_runtime::stage==Stage::idle&&inventory_calls==0&&lancerreload_inventory::actual.loaded==0&&lancerreload_inventory::actual.reserve==5);
  assert(longshot_runtime::debt_for(lancerreload_inventory::actual.identity)<0);}
 {Fixture f;lancerreload_inventory::actual.infinite_reserve=true;lancerreload_inventory::actual.reserve=0;
  f.open();f.load();f.close(5.f);assert(lancerreload_inventory::actual.loaded==1&&lancerreload_inventory::actual.reserve==0);}
 {Fixture f;lancerreload_inventory::actual.reserve=0;f.press(hand::LeaseKind::magazine,1);
  assert(longshot_runtime::stage==Stage::idle&&inventory_calls==0&&!lancerreload_state::lease.acquired);}
 {Fixture f;f.open();f.load();lancerreload_runtime::stop_requested=true;f.step();lancerreload_runtime::stop_requested=false;
  f.open(4);assert(f.close(4.5f,5)==gears_weapon::ReloadTxnResult::perfect&&rewards==1&&inventory_calls==1);}
 {Fixture f;f.open();f.load();lancerreload_runtime::stop_requested=true;f.step();lancerreload_runtime::stop_requested=false;
  ++actual_frame.key.lifecycle;++mock_hold_generation;nativelancerfollow::bound_key.hold_generation=mock_hold_generation;
  lancerreload_inventory::actual.identity=lancerreload_inventory::identity(actual_frame);
  f.open(4);assert(f.close(5.f,5)==gears_weapon::ReloadTxnResult::ordinary&&rewards==0&&inventory_calls==1);}
 {Fixture f;f.open();f.load();lancerreload_runtime::stop_requested=true;f.step();lancerreload_runtime::stop_requested=false;
  f.open(4);lancerreload_runtime::stop_requested=true;f.step();lancerreload_runtime::stop_requested=false;
  f.open(6);assert(f.close(4.5f,7)==gears_weapon::ReloadTxnResult::perfect&&rewards==1&&inventory_calls==1);}
 {Fixture f;f.open();f.load();lancerreload_runtime::stop_requested=true;f.step();lancerreload_runtime::stop_requested=false;
  ++actual_frame.controllers.input_epoch;lancerreload_inventory::actual.identity=lancerreload_inventory::identity(actual_frame);
  f.press(hand::LeaseKind::magazine,4);assert(longshot_runtime::stage==Stage::idle&&inventory_calls==1&&rewards==0&&longshot_runtime::blocks_fire(actual_frame.weapon));}
 {Fixture f;f.open();f.load();lancerreload_runtime::stop_requested=true;f.step();lancerreload_runtime::stop_requested=false;
  --lancerreload_inventory::actual.reserve;++lancerreload_inventory::actual.revision;
  f.press(hand::LeaseKind::magazine,4);assert(longshot_runtime::stage==Stage::idle&&inventory_calls==1&&longshot_runtime::blocks_fire(actual_frame.weapon));}
 {Fixture f;f.open();lancerreload_runtime::stop_requested=true;f.step();lancerreload_runtime::stop_requested=false;
  ++actual_frame.key.lifecycle;++mock_hold_generation;nativelancerfollow::bound_key.hold_generation=mock_hold_generation;
  lancerreload_inventory::actual.identity=lancerreload_inventory::identity(actual_frame);
  f.open(4);f.load(5);assert(f.close(4.5f,6)==gears_weapon::ReloadTxnResult::perfect&&inventory_calls==1&&rewards==1);}
 std::puts("Longshot production runtime: full cycles, eligibility, exact4s, interrupted clock/reward, hold rebind, finite/infinite counts, rejection and shutdown passed");
 {Fixture f;f.open();clip_restore_ok=false;lancerreload_runtime::stop_requested=true;f.step();
  assert(longshot_runtime::stage==Stage::recovering&&longshot_runtime::pending_work()&&native_exits==1&&!lancerreload_state::lease.acquired);
  lancerreload_state::lease={true,game_seconds,777};clip_restore_ok=true;f.step();
  assert(longshot_runtime::stage==Stage::idle&&!longshot_runtime::pending_work()&&lancerreload_state::lease.acquired&&lancerreload_state::lease.serial==777&&native_exits==1);}
 {Fixture f;f.open();f.load();f.press(hand::LeaseKind::magazine,3);
  longshot_bolt::next_result=longshot_bolt::Result::complete;f.step();clip_restore_ok=false;f.step();
  assert(longshot_runtime::stage==Stage::completing&&longshot_runtime::pending_work()&&native_exits==1&&!lancerreload_state::lease.acquired);
  lancerreload_state::lease={true,game_seconds,777};clip_restore_ok=true;f.step();f.step();
  assert(longshot_runtime::stage==Stage::idle&&!longshot_runtime::pending_work()&&lancerreload_state::lease.acquired&&lancerreload_state::lease.serial==777&&native_exits==1);}
 std::puts("Longshot scoped cleanup: interrupted and physically completed old cycles preserve a newer original reload lease while old clip restoration is pending");
}
