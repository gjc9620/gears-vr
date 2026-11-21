#include "../vr-common/weapon_torque_bow_interaction.h"
#include "../vr-common/weapon_reload_transaction.h"
#include "../vr-common/weapon_equipment_observation.h"
#include "../vr-common/weapon_torque_bow_fire_driver.h"
#include "../vr-common/weapon_torque_bow_feed.h"
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <climits>
using uptr=std::uint64_t;
namespace h=gears_weapon_hand;namespace w=gears_weapon;
namespace fixture {
bool valid=true,world=true,held=true,reloading{},timer{},start_dispatch=true,start_ok=true,start_serial=true,abort_unknown{},reward_unknown{};
unsigned begins{},writes{},aborts{},rewards{},restores{};int reserve=2,loaded{};int writer_mode{};
std::uint64_t shared_serial{};std::int64_t now=100000000;
std::uint64_t next_original_serial{7};float start_seconds{1};
}
namespace weaponhand {unsigned current_gt(void*)noexcept{return 1;}std::int64_t clock_ns(void*)noexcept{return fixture::now;}}
namespace playground {bool instance(uptr,wchar_t const*)noexcept{return true;}}
namespace nativeequipment {bool current(void*,h::Frame const&)noexcept{return fixture::valid;}}
namespace vrequip {struct Model {bool held_for(uptr,uptr,uptr)const noexcept{return fixture::held;}}model;}
namespace lancerreload_inventory {
// Engine/Native/inventory backend is injected. The bridge and its one model
// are production code; this mock identity is not an incarnation certificate.
struct Identity {std::uint64_t weapon{},world{};bool valid()const noexcept{return weapon&&world;}
 bool operator==(Identity const& b)const noexcept{return weapon==b.weapon&&world==b.world;}};
Identity identity(h::Frame const& f)noexcept{return {f.weapon.address,f.world.address};}
using Inventory=w::ReloadTxnInventory<Identity>;using Request=w::ReloadTxnRequest<Identity>;using Receipt=w::ReloadTxnReceipt<Identity>;
using OwnsReload=bool(*)(h::Frame const&,std::uint64_t);
bool counts_equal(Inventory const&a,Inventory const&b)noexcept{return a.identity==b.identity&&a.capacity==b.capacity&&a.loaded==b.loaded&&a.reserve==b.reserve&&a.infinite_reserve==b.infinite_reserve;}
struct Counts {int capacity=1,used{},reserve{};bool infinite{};};
struct ObservedInventory {
 Identity bound{};std::uint64_t revision{};int last_loaded{},last_reserve{};
 bool sample_owned(Identity const&id,Inventory&out)noexcept{
  if(!fixture::valid||!id.valid()||(revision&&!(bound==id)))return false;
  if(!revision||last_loaded!=fixture::loaded||last_reserve!=fixture::reserve)++revision;
  bound=id;last_loaded=fixture::loaded;last_reserve=fixture::reserve;
  out={bound,revision,1,unsigned(fixture::loaded),unsigned(fixture::reserve),false};return true;
 }
 bool sample(h::Frame const&f,Inventory&out)noexcept{return sample_owned(identity(f),out);}
};
enum class CommitResult {rejected,committed,reconciliation_required};
struct InsertionWriter {
 CommitResult commit(h::Frame const&f,Request const&r,ObservedInventory&,OwnsReload owns,Receipt&)noexcept{
  assert(owns(f,r.reload_serial));++fixture::writes;
  if(fixture::writer_mode==1)return CommitResult::rejected;
  fixture::reserve=int(r.expected.reserve);
  if(fixture::writer_mode==2)return CommitResult::reconciliation_required;
  fixture::loaded=int(r.expected.loaded);
  if(fixture::writer_mode==4){fixture::loaded=0;return CommitResult::committed;} // later count change after confirmed full insert
  return fixture::writer_mode==3?CommitResult::reconciliation_required:CommitResult::committed;
 }
};
}
namespace torquebow_cleanup {
struct Anchor {
 std::uint64_t value{};using Owns=bool(*)(h::Frame const&,std::uint64_t)noexcept;
 bool capture_before_press(h::Frame const&f,std::uint64_t n,Owns owns)noexcept{if(!fixture::world||!owns(f,n))return false;value=n;return true;}
 bool current(h::Frame const&f,std::uint64_t n,Owns owns)const noexcept{return value&&value==n&&fixture::world&&owns(f,n);}
 std::uint64_t serial()const noexcept{return value;}
 bool retire(std::uint64_t n,bool clear)noexcept{if(n!=value||!clear)return false;value=0;return true;}
};
}
namespace lancerreload_state {
namespace inv=lancerreload_inventory;
struct Lease{};
struct RewardReceipt {bool eligibility_known{},eligible{},dispatched{},call_confirmed{},applied{};};
enum class BeginResult {rejected,acquired,recovery_required};
bool capture_reward_eligibility(inv::Inventory const&,RewardReceipt&r)noexcept{r.eligibility_known=r.eligible=true;return true;}
bool owns(h::Frame const&,std::uint64_t serial)noexcept{return fixture::reloading&&serial&&serial==fixture::shared_serial;}
bool retains_serial(std::uint64_t serial)noexcept{return serial&&serial==fixture::shared_serial;}
BeginResult begin_for_part(h::Frame const&,std::uint64_t&serial,h::LeaseKind kind,Lease const*,bool*dispatched)noexcept{
 assert(kind==h::LeaseKind::ammo);++fixture::begins;*dispatched=fixture::start_dispatch;
 if(!*dispatched)return BeginResult::rejected;
 fixture::reloading=true;serial=fixture::start_serial?fixture::next_original_serial++:0;fixture::shared_serial=serial;
 return fixture::start_ok?BeginResult::acquired:BeginResult::rejected;
}
bool accepted_game_start(std::uint64_t,float&seconds)noexcept{seconds=fixture::start_seconds;return true;}
bool bool_call(uptr,wchar_t const*,bool&v)noexcept{v=fixture::reloading;return true;}
bool recover_serial(std::uint64_t serial)noexcept{
 if(!retains_serial(serial))return true;
 if(fixture::reloading){++fixture::aborts;if(fixture::abort_unknown)return false;fixture::reloading=false;}
 if(fixture::timer)return false;fixture::shared_serial=0;return true;
}
bool restore_partial_insert(inv::Request const&r,inv::ObservedInventory&)noexcept{
 ++fixture::restores;
 if(fixture::loaded!=int(r.before.loaded)||fixture::reserve!=int(r.expected.reserve))return false;
 fixture::reserve=int(r.before.reserve);return true;
}
bool perfect_reward(inv::Request const&,inv::Inventory const&,inv::ObservedInventory&,RewardReceipt&r)noexcept{
 if(!r.dispatched){r.dispatched=true;++fixture::rewards;r.call_confirmed=!fixture::reward_unknown;}
 if(!r.call_confirmed)return false;r.applied=true;return true;
}
}
namespace torquebow_state {
struct Observed {bool active=true;lancerreload_inventory::Counts inventory{};
 bool stopped()const noexcept{return !fixture::reloading;}};
bool read(h::Frame const&,Observed&out)noexcept{out={};out.inventory.used=1-fixture::loaded;out.inventory.reserve=fixture::reserve;return fixture::valid;}
bool read_owned(h::Frame const&f,Observed&out,torquebow_cleanup::Anchor const&a,std::uint64_t serial,torquebow_cleanup::Anchor::Owns owns)noexcept{
 return a.current(f,serial,owns)&&read(f,out);
}
}
#include "weapon_torque_bow_reload_live.inl"
namespace {
namespace live=torquebow_reload_live;
unsigned checks{};
#define CHECK(c) do{++checks;assert(c);}while(0)
struct F {
 h::Frame frame{};h::Decision decision{};w::TorqueBowInteraction model;
 std::uint64_t sequence{},events{};std::int64_t tick=1000000;
 F(){
  fixture::valid=fixture::world=fixture::held=true;fixture::reloading=fixture::timer=false;
  fixture::start_dispatch=fixture::start_ok=fixture::start_serial=true;
  fixture::abort_unknown=fixture::reward_unknown=false;fixture::begins=fixture::writes=fixture::aborts=fixture::rewards=fixture::restores=0;
  fixture::reserve=2;fixture::loaded=fixture::writer_mode=0;fixture::shared_serial=0;fixture::now+=1000000000;
  fixture::next_original_serial=7;fixture::start_seconds=1;
  live::owner={};live::outstanding=false;live::last_intent_owner={};live::last_intent=0;
  frame.world.address=8;frame.pawn.address=16;frame.controller.address=24;frame.weapon.address=32;
  frame.key={1,2,3,4,5,6,7,8};frame.controllers.session=7;frame.controllers.space_epoch=8;frame.controllers.input_epoch=9;
  frame.press_token=10;frame.left_tracked=frame.left_grip_known=frame.left_grip_down=1;
  decision.key=frame.key;decision.press_token=10;decision.lease={h::LeaseKind::ammo,0,frame.key.lifecycle,10};
  decision.status=h::DecisionStatus::granted;decision.new_grant=1;decision.release_required=0;
  w::TorqueBowConfig config{};config.identity={8,16,24,32,6,7,8,9};config.instance_config_serial=22;config.capacity=1;
  config.game_ticks_per_second=1000000;config.original_class_verified=config.original_charge_entry_verified=config.original_end_fire_verified=config.non_firing_abort_verified=true;
  config.manual_reload_phase_verified=config.reload_plan_verified=true;config.reload_step_count=1;config.reload_steps[0]={w::TorqueBowReloadAction::insert_arrow,123};
  CHECK(model.bind(config,{1,0,2}));const auto s=stamp();CHECK(model.observe({s,model.inventory(),{true},true,true,true,false,s.monotonic_ns})==w::TorqueBowEvent::observed);
 }
 w::TorqueBowStamp stamp(){fixture::now+=1000000;return {model.identity(),++sequence,fixture::now,tick};}
 void begin(){
  CHECK(live::reserve_before_begin(frame,decision,model.inventory()));CHECK(live::begin_original(frame,decision)==live::Begin::accepted);
  std::uint64_t serial{};std::int64_t start{};CHECK(live::accepted(serial,start)&&serial==7&&start==1000000);
  tick=1500000;CHECK(model.begin_left_shoulder({stamp(),10,serial,true,true,true,true,start},
   {11,w::TorqueBowAmmoSource::left_shoulder,true,true,10,true,true})==w::TorqueBowEvent::reload_started);
 }
 w::TorqueBowRequest close(){
  tick=5000000;const auto floor=sequence;CHECK(model.complete_step(stamp(),{22,7,123,floor,11,true,true,true,true,true})==w::TorqueBowEvent::request_ready);
  w::TorqueBowRequest request{};CHECK(model.take_request(request,fixture::now));CHECK(live::begin_completion(frame,decision,request));return request;
 }
 void acknowledge(w::TorqueBowRequest const&r,bool success){
  w::TorqueBowActionReceipt receipt{};CHECK(live::receipt(frame,receipt));CHECK(receipt.accepted==success);
  CHECK(!live::retire_acknowledged(r.token,model));receipt.stamp=stamp();receipt.original_event_serial=++events;
  CHECK(model.acknowledge(receipt)==(success?w::TorqueBowEvent::perfect:w::TorqueBowEvent::interrupted));
  CHECK(live::retire_acknowledged(r.token,model)&&!live::pending());
 }
};
void completed(){
 for(int mode:{0,3}){F f;f.begin();const auto r=f.close();fixture::writer_mode=mode;
  CHECK(live::service(&f.frame));CHECK(fixture::writes==1&&fixture::aborts==1&&fixture::rewards==1);
  CHECK(live::service(&f.frame)&&fixture::writes==1&&fixture::rewards==1);
  CHECK(!live::begin_completion(f.frame,f.decision,r));f.acknowledge(r,true);
  CHECK(fixture::loaded==1&&fixture::reserve==1);
 }
}
void refused_and_partial(){
 for(int mode:{1,2}){F f;f.begin();const auto r=f.close();fixture::writer_mode=mode;
  CHECK(live::service(&f.frame));CHECK(fixture::loaded==0&&fixture::reserve==2&&fixture::writes==1&&!fixture::rewards);
  CHECK(fixture::restores==(mode==2?1u:0u));f.acknowledge(r,false);
 }
 F stopped;stopped.begin();const auto request=stopped.close();CHECK(live::stop());CHECK(!fixture::writes);stopped.acknowledge(request,false);
}
void unknowns(){
 F begin;CHECK(live::reserve_before_begin(begin.frame,begin.decision,begin.model.inventory()));fixture::start_ok=fixture::start_serial=false;
 CHECK(live::begin_original(begin.frame,begin.decision)==live::Begin::unresolved&&live::pending());
 CHECK(live::begin_original(begin.frame,begin.decision)==live::Begin::rejected);CHECK(!live::stop()&&fixture::begins==1&&!fixture::aborts);
 F exit;exit.begin();const auto request=exit.close();fixture::abort_unknown=true;
 CHECK(!live::service(&exit.frame)&&fixture::aborts==1);CHECK(!live::service(&exit.frame)&&fixture::aborts==1);
 fixture::reloading=false;fixture::timer=true;CHECK(!live::service(&exit.frame)&&fixture::aborts==1);
 fixture::timer=false;CHECK(live::service(&exit.frame)&&fixture::aborts==1);exit.acknowledge(request,true);
 F reward;reward.begin();const auto rewarded=reward.close();fixture::reward_unknown=true;
 CHECK(!live::service(&reward.frame)&&fixture::rewards==1);CHECK(!live::service(&reward.frame)&&fixture::writes==1&&fixture::rewards==1);
 live::owner.reward.call_confirmed=true;CHECK(live::service(&reward.frame)&&fixture::rewards==1);reward.acknowledge(rewarded,true);
}
void boundaries(){
 F pinch;pinch.frame.interaction_control=unsigned(gears_vr::LeftInteractionControl::pinch);
 CHECK(!live::ammo_grant(pinch.frame,pinch.decision));
 CHECK(!live::reserve_before_begin(pinch.frame,pinch.decision,pinch.model.inventory())&&!fixture::begins);
 F f;auto wrong=f.decision;wrong.lease.kind=h::LeaseKind::support;CHECK(!live::reserve_before_begin(f.frame,wrong,f.model.inventory()));
 wrong=f.decision;wrong.new_grant=0;CHECK(!live::reserve_before_begin(f.frame,wrong,f.model.inventory()));
 fixture::world=false;CHECK(!live::reserve_before_begin(f.frame,f.decision,f.model.inventory())&&!live::pending());fixture::world=true;
 CHECK(live::reserve_before_begin(f.frame,f.decision,f.model.inventory()));fixture::start_dispatch=false;
 CHECK(live::begin_original(f.frame,f.decision)==live::Begin::rejected&&!live::pending());
 CHECK(!live::reserve_before_begin(f.frame,f.decision,f.model.inventory())); // same real press cannot restart
 F changed;changed.begin();const auto request=changed.close();fixture::world=false;CHECK(!live::service(&changed.frame)&&!fixture::writes);
 fixture::world=true;fixture::valid=false;CHECK(!live::service(&changed.frame)&&!fixture::writes);fixture::valid=true;
 CHECK(live::stop()&&!fixture::writes);changed.acknowledge(request,false);
 F spent;spent.begin();spent.close();fixture::writer_mode=4;
 CHECK(!live::service(&spent.frame)&&live::owner.phase==live::Phase::conflict);
 CHECK(!fixture::restores&&fixture::reserve==1&&fixture::loaded==0&&live::pending()); // never refund a proved full insert
}
namespace driver_api=gears_torque_bow_fire_driver;namespace vr=gears_vr;
// Actual Driver + actual model/Decoder + actual production reload journal.
// The old original engine and inventory backends above remain injected.
struct CombinedBackend {
 std::int64_t tick{1000000};std::uint64_t hold_generation{1},arrow{10},grip{10};
 bool fire_owner{},down{},charge{},spawn{},decline_completion{},delay_accept{};
 unsigned presses{},releases{},shots{},cancels{};
 std::int64_t now()noexcept{return fixture::now+=1000;}
 std::uint64_t hold(h::Frame const&)noexcept{return hold_generation;}
 bool owned()const noexcept{return fire_owner;}
 bool has_spawn()const noexcept{return spawn;}
 bool reload_owned()const noexcept{return live::pending();}
 driver_api::Snapshot current()const noexcept{
  return {{!fixture::reloading&&!charge,charge,false,down,false,false,false},unsigned(fixture::loaded),unsigned(fixture::reserve),tick,!fixture::reloading&&!charge&&!down};
 }
 bool observe(h::Frame const&f,h::GripGameState const&,driver_api::Snapshot&out)noexcept{
  if(!fixture::valid||!vr::controller_frame_matches(f.controllers,f.stereo,now())||fixture::now-f.controllers.sync_ns>100000000)return false;
  out=current();return true;
 }
 bool configure(h::Frame const&f,h::GripGameState const&,driver_api::Snapshot const&,w::TorqueBowConfig&config)noexcept{
  config={};config.identity={f.world.address,f.pawn.address,f.controller.address,f.weapon.address,f.key.world_generation,
   f.controllers.session,f.controllers.space_epoch,f.controllers.input_epoch};config.instance_config_serial=22;config.capacity=1;config.game_ticks_per_second=1000000;
  config.original_class_verified=config.original_charge_entry_verified=config.original_end_fire_verified=config.non_firing_abort_verified=true;
  config.manual_reload_phase_verified=config.reload_plan_verified=true;config.reload_step_count=1;config.reload_steps[0]={w::TorqueBowReloadAction::insert_arrow,123};return true;
 }
 bool guard(h::Frame const&)noexcept{return fixture::world;}
 bool prepare(h::Frame const&,h::GripGameState const&)noexcept{return fixture::valid;}
 w::TorqueBowCallResult press(h::Frame const&)noexcept{
  CHECK(!live::pending()&&!fire_owner&&fixture::loaded==1);++presses;fire_owner=down=charge=true;return w::TorqueBowCallResult::accepted;
 }
 w::TorqueBowCallResult release(h::Frame const&)noexcept{
  CHECK(fire_owner&&charge&&fixture::loaded==1);++releases;down=charge=false;spawn=true;fixture::loaded=0;return w::TorqueBowCallResult::accepted;
 }
 bool read_owned(driver_api::Snapshot&out)noexcept{if(!fire_owner||!fixture::world)return false;out=current();return true;}
 void fence()noexcept{}
 bool cancel(w::TorqueBowCancelRoute,driver_api::Snapshot&out)noexcept{
  if(!fixture::world||spawn)return false;if(fire_owner){++cancels;fire_owner=down=charge=false;}out=current();return true;
 }
 bool settle(driver_api::Snapshot&out)noexcept{if(!fire_owner||charge||down||spawn)return false;fire_owner=false;out=current();return true;}
 driver_api::Shot account(w::TorqueBowInteraction&model,w::TorqueBowStamp const&stamp,std::uint64_t event,w::TorqueBowEvent&received)noexcept{
  if(!spawn)return driver_api::Shot::none;auto inventory=model.inventory();++inventory.revision;--inventory.loaded;
  received=model.record_original_shot({stamp,event,++shots,inventory,true});
  if(received!=w::TorqueBowEvent::shot_recorded&&received!=w::TorqueBowEvent::safety_violation)return driver_api::Shot::conflict;
  spawn=false;return driver_api::Shot::recorded;
 }
 bool reload_begin(h::Frame const&f,h::GripGameState const&,h::Decision const&d,w::TorqueBowInventory const&inventory)noexcept{
  if(fire_owner||spawn||!live::reserve_before_begin(f,d,inventory))return false;
  grip=f.press_token;++arrow;fixture::start_seconds=float(double(tick)/1000000.);live::begin_original(f,d);return live::pending();
 }
 w::TorqueBowCallResult reload_accept(h::Frame const&f,h::GripGameState const&,driver_api::ReloadAcceptance&out)noexcept{
  out={};if(delay_accept)return w::TorqueBowCallResult::unknown;
  if(live::owner.phase==live::Phase::recovering)return w::TorqueBowCallResult::rejected;
  if(!live::accepted(out.original_serial,out.original_start_tick))return w::TorqueBowCallResult::unknown;
  out.actual=current();out.arrow_serial=arrow;out.left_grip_edge=f.press_token;
  out.fresh_left_grip=out.left_shoulder_contact=out.automatic_fill_suppressed=true;return w::TorqueBowCallResult::accepted;
 }
 bool reload_sample(h::Frame const&f,driver_api::Snapshot&out)noexcept{if(!live::matches(f))return false;out=current();return true;}
 bool reload_regrab(h::Frame const&f,h::Decision const&d,w::TorqueBowArrowReceipt&out)noexcept{
  if(!d.new_grant||!live::ammo_grant(f,d)||f.press_token<=grip)return false;
  grip=f.press_token;out={++arrow,w::TorqueBowAmmoSource::left_shoulder,true,true,grip,true,true};return true;
 }
 bool reload_complete(h::Frame const&f,h::GripGameState const&,w::TorqueBowRequest const&r)noexcept{
  if(decline_completion)return false;h::Decision d{};d.key=f.key;d.press_token=f.press_token;d.lease={h::LeaseKind::ammo,0,f.key.lifecycle,f.press_token};
  d.status=h::DecisionStatus::retained;d.release_required=0;return live::begin_completion(f,d,r);
 }
 bool reload_reject(w::TorqueBowRequest const&r)noexcept{return live::reject_completion(r);}
 void reload_stop()noexcept{live::stop();}
 void reload_service(h::Frame const*f)noexcept{if(live::owner.phase!=live::Phase::holding)live::service(f);}
 bool reload_receipt(h::Frame const*,w::TorqueBowActionReceipt&r,driver_api::Snapshot&out)noexcept{
  if(!live::receipt_owned(r))return false;out=current();return true;
 }
 bool reload_retire(std::uint64_t token,w::TorqueBowInteraction const&model)noexcept{return live::retire_acknowledged(token,model);}
};
struct DriverF {
 F baseline;driver_api::Driver driver;CombinedBackend backend;h::GripGameState game{};
 h::Frame&frame=baseline.frame;h::Decision&decision=baseline.decision;
 DriverF(){
  game.known=game.main_hand_held=true;
  auto&input=frame.controllers;auto&stereo=frame.stereo;
  input.session=stereo.session=7;input.space_epoch=stereo.space_epoch=8;input.sequence=stereo.sequence=20;
  input.profile_generation=1;input.input_epoch=9;input.focused=stereo.focused=1;input.predicted_time=stereo.predicted_time=100;
  for(auto&hand:input.hands){hand.grip_active=1;hand.grip_flags=15;}
  input.hands[1].trigger_active=1;input.hands[1].buttons_active=vr::controller_stick_click;
  stamp();CHECK(run()==driver_api::Status::baseline);
 }
 void stamp(){frame.controllers.sync_ns=frame.controllers.received_ns=frame.stereo.received_ns=fixture::now;frame.now_ns=fixture::now;decision.key=frame.key;}
 void next(std::int64_t tick){fixture::now+=10000000;backend.tick=tick;++frame.key.frame;++frame.controllers.sequence;frame.stereo.sequence=frame.controllers.sequence;stamp();}
 driver_api::Status run(){return driver.tick(backend,frame,game);}
 void begin(){CHECK(driver.begin_reload(backend,frame,game,decision)==driver_api::Status::owned);CHECK(driver.stage()==w::TorqueBowStage::reload_steps&&driver.pending()&&fixture::begins==1);}
 void close(std::int64_t tick=5000000){
  next(tick);run();driver_api::ReloadMotion motion{22,live::owner.original_serial,123,driver.interaction().held_arrow_serial(),20,true,true,true,true,true};
  CHECK(driver.complete_reload_motion(backend,frame,game,motion));
 }
 void trigger(bool down){auto&hand=frame.controllers.hands[1];++(down?hand.pressed[vr::control_trigger]:hand.released[vr::control_trigger]);hand.trigger=down?1.f:0.f;}
 void close_raw(){
  namespace a=gears_torque_bow_arrow_mount;a::Mount arrow_mount{};w::TorqueBowFeedCalibration plan{};
  CHECK(a::author_original({true,{.01,.02,.03},{0,0,0,1}},.3,.3,100,arrow_mount));
  CHECK(w::author_torque_bow_feed(22,.3,arrow_mount,plan));w::TorqueBowArrowFeed feed;
  const auto arrow=driver.interaction().held_arrow_serial();
  CHECK(feed.arm(frame.weapon.address,arrow,frame.world.address,frame.controllers.space_epoch,plan));
  std::uint64_t beginning{};
  for(double depth:{0.,plan.mount.approach_depth_metres+.001,plan.mount.approach_depth_metres*.5,.001}){
   next(backend.tick+30000);fixture::now+=20000000;stamp();run();
   const auto q=vr::normalized(vr::multiply(plan.mount.port_in_weapon.orientation,vr::inverse(plan.mount.item_in_left_grip.orientation)));
   const auto p=w::difference(w::sum(plan.mount.port_in_weapon.position,{depth,0,0}),w::rotate(q,plan.mount.item_in_left_grip.position));
   const w::FeedSample sample{frame.weapon.address,arrow,frame.world.address,frame.controllers.space_epoch,
    frame.controllers.sequence,frame.controllers.sync_ns,{}, {p,q},true,true};
   const auto result=feed.observe_raw(sample,plan);
   if(result==w::FeedResult::approaching&&!beginning)beginning=sample.sequence;
   if(result!=w::FeedResult::inserted){CHECK(!fixture::writes&&driver.inventory().loaded==0);continue;}
   driver_api::ReloadMotion motion{22,live::owner.original_serial,123,arrow,beginning,true,true,true,true,true};
   CHECK(driver.complete_reload_motion(backend,frame,game,motion));
   CHECK(!driver.complete_reload_motion(backend,frame,game,motion)); // no repeated inventory setter
  }
  CHECK(fixture::writes==1&&driver.inventory().loaded==1);
 }
};
void one_driver_cycle(){
 DriverF f;f.begin();f.close_raw();CHECK(!f.driver.pending()&&!live::pending()&&f.driver.inventory().loaded==1&&fixture::reserve==1);
 CHECK(fixture::writes==1&&fixture::rewards==1&&fixture::aborts==1);
 f.next(5010000);f.run();f.next(5020000);f.trigger(true);f.run();CHECK(f.backend.presses==1&&f.driver.stage()==w::TorqueBowStage::charging);
 f.next(5030000);f.trigger(false);f.run();CHECK(f.backend.releases==1&&f.backend.shots==1&&f.driver.inventory().loaded==0&&f.driver.inventory().reserve==1&&!f.driver.pending());
 f.next(6000000);f.frame.press_token=f.decision.press_token=11;f.decision.lease.id=11;f.decision.new_grant=1;f.run();
 CHECK(f.driver.begin_reload(f.backend,f.frame,f.game,f.decision)==driver_api::Status::owned&&fixture::begins==2);
 f.next(7000000);f.run();driver_api::ReloadMotion motion{22,live::owner.original_serial,123,f.driver.interaction().held_arrow_serial(),f.frame.controllers.sequence-1,true,true,true,true,true};
 CHECK(f.driver.complete_reload_motion(f.backend,f.frame,f.game,motion));CHECK(f.driver.inventory().loaded==1&&f.driver.inventory().reserve==0&&fixture::writes==2&&!f.driver.pending());
}
void one_driver_interruptions(){
 DriverF refused;refused.begin();refused.backend.decline_completion=true;refused.close();
 CHECK(!fixture::writes&&!refused.driver.pending()&&refused.driver.stage()==w::TorqueBowStage::interrupted&&refused.driver.inventory().loaded==0);
 DriverF unknown;unknown.begin();fixture::abort_unknown=true;unknown.close();CHECK(unknown.driver.pending()&&fixture::writes==1&&fixture::aborts==1);
 unknown.next(5100000);unknown.run();CHECK(unknown.driver.pending()&&fixture::writes==1&&fixture::aborts==1);
 fixture::reloading=false;unknown.next(5200000);unknown.run();CHECK(!unknown.driver.pending()&&unknown.driver.inventory().loaded==1&&fixture::writes==1&&fixture::aborts==1);
 DriverF stopping;stopping.begin();CHECK(stopping.driver.stop(stopping.backend,w::TorqueBowCancelRoute::put_down)==driver_api::Status::idle);
 CHECK(!stopping.driver.pending()&&!live::pending()&&!fixture::writes&&stopping.driver.stage()==w::TorqueBowStage::interrupted);
}
void one_driver_regrip_and_cancel(){
 for(auto finish:{5000000LL,5000001LL}){
  DriverF f;f.begin();f.next(2500000);f.frame.left_grip_down=0;f.run();CHECK(f.driver.release_reload_arrow(f.backend,f.frame));
  CHECK(!f.driver.interaction().held_arrow_serial()&&live::pending()&&fixture::reserve==2);
  f.next(4500000);f.frame.left_grip_down=1;f.frame.press_token=f.decision.press_token=11;f.decision.lease.id=11;f.run();
  CHECK(f.driver.regrab_reload_arrow(f.backend,f.frame,f.decision));CHECK(fixture::begins==1);
  const auto begin=f.frame.controllers.sequence;f.next(finish);f.run();
  driver_api::ReloadMotion motion{22,live::owner.original_serial,123,f.driver.interaction().held_arrow_serial(),20,true,true,true,true,true};
  CHECK(!f.driver.complete_reload_motion(f.backend,f.frame,f.game,motion)&&!fixture::writes); // old-arrow motion floor
  motion.begin_input_sequence=begin;CHECK(f.driver.complete_reload_motion(f.backend,f.frame,f.game,motion));
  CHECK(fixture::writes==1&&fixture::rewards==(finish==5000000?1u:0u)&&f.driver.inventory().loaded==1&&!f.driver.pending());
 }
 DriverF cancel;cancel.begin();cancel.close();cancel.next(5010000);cancel.run();cancel.next(5020000);cancel.trigger(true);cancel.run();
 CHECK(cancel.driver.stage()==w::TorqueBowStage::charging);
 cancel.next(5030000);++cancel.frame.controllers.hands[1].pressed[vr::control_stick];++cancel.frame.controllers.hands[1].released[vr::control_stick];cancel.run();
 CHECK(cancel.backend.cancels==1&&!cancel.backend.shots&&!cancel.backend.releases&&cancel.driver.inventory().loaded==1&&!cancel.driver.pending());
 cancel.next(5040000);cancel.trigger(false);cancel.run();CHECK(!cancel.backend.releases&&!cancel.backend.shots);
}
void one_driver_late_accept_and_context(){
 DriverF delayed;delayed.backend.delay_accept=true;
 CHECK(delayed.driver.begin_reload(delayed.backend,delayed.frame,delayed.game,delayed.decision)==driver_api::Status::owned);
 CHECK(delayed.driver.stage()==w::TorqueBowStage::ready&&fixture::begins==1&&delayed.driver.pending());
 delayed.next(1700000);delayed.backend.delay_accept=false;delayed.run();CHECK(delayed.driver.stage()==w::TorqueBowStage::reload_steps&&fixture::begins==1);
 delayed.next(5000000);delayed.run();driver_api::ReloadMotion motion{22,7,123,delayed.driver.interaction().held_arrow_serial(),delayed.frame.controllers.sequence-1,true,true,true,true,true};
 CHECK(delayed.driver.complete_reload_motion(delayed.backend,delayed.frame,delayed.game,motion));CHECK(fixture::rewards==1&&!delayed.driver.pending());
 DriverF blocked;blocked.begin();fixture::world=false;
 CHECK(blocked.driver.stop(blocked.backend,w::TorqueBowCancelRoute::put_down)==driver_api::Status::cleaning);
 CHECK(blocked.driver.pending()&&!blocked.driver.reset_drained(blocked.backend)&&!fixture::writes&&!fixture::aborts);
 fixture::world=true;CHECK(blocked.driver.stop(blocked.backend,w::TorqueBowCancelRoute::put_down)==driver_api::Status::idle);
 CHECK(blocked.driver.reset_drained(blocked.backend)&&!live::pending()&&!fixture::writes);
 DriverF expired;expired.begin();fixture::now+=100000001;
 CHECK(expired.driver.poll(expired.backend)==driver_api::Status::idle&&!expired.driver.pending()&&!fixture::writes&&fixture::aborts==1);
}
}
int main(){completed();refused_and_partial();unknowns();boundaries();one_driver_cycle();one_driver_interruptions();one_driver_regrip_and_cancel();one_driver_late_accept_and_context();
 std::printf("Torque Bow original reload bridge: %u checks passed; actual Driver/model/Decoder/bridge, injected Native/Engine/shared-state/inventory backend; no runtime certification.\n",checks);}
