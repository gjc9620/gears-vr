#include "../vr-common/weapon_hand_client.h"
#include "../vr-common/weapon_torque_bow_original.h"
#include "weapon_torque_bow_fire.h"
#include <cassert>
#include <cstdio>
using uptr=std::uintptr_t;
struct Name {unsigned index{},number{};};
namespace h=gears_weapon_hand;using R=gears_weapon::TorqueBowCallResult;
namespace torquebow_input_owner {std::uint64_t projectile_owner(h::Frame const&) noexcept;}
namespace fixture {
bool live=true,registration=true,muzzle=true,bind=true,probe_supported=true;unsigned presses{},releases{},stops{},begins{};
std::uint64_t hold=1;R result=R::accepted;
}
namespace lancerreload_inventory {
struct Counts {int capacity{1},used{},reserve{5};bool infinite{};
 bool operator==(Counts const& b)const{return capacity==b.capacity&&used==b.used&&reserve==b.reserve&&infinite==b.infinite;}};
}
namespace torquebow_state {
struct Observed {lancerreload_inventory::Counts inventory{};bool charge{},input_down{},fire{},active{true};
 bool stopped()const{return !charge&&!input_down&&!fire;}}actual;
bool read(h::Frame const&,Observed& out){out=actual;return true;}
bool tick_enabled(uptr,bool& out){out=false;return fixture::probe_supported;}
}
namespace torquebow_cancel_live {bool active_name(uptr,Name& out){out={1,0};return true;}}
namespace vrequip {struct Model {std::uint64_t hold_generation(){return fixture::hold;}} model;}
namespace nativeequipment {bool current(void*,h::Frame const&){return fixture::live;}}
namespace unified_weapon_fire {
bool same_context(h::FrameKey a,h::FrameKey b){a.frame=b.frame=0;return h::same(a,b);}
}
namespace torquebow_input_fence {bool available(){return true;}}
namespace torquebow_cleanup {
using Owns=bool(*)(h::Frame const&,std::uint64_t) noexcept;
struct Anchor {std::uint64_t serial{};
 bool capture_before_press(h::Frame const& f,std::uint64_t s,Owns owns){if(!fixture::live||!owns(f,s))return false;serial=s;return true;}
 bool current(h::Frame const& f,std::uint64_t s,Owns owns)const{return serial==s&&fixture::registration&&owns(f,s);}
 bool retire(std::uint64_t s,bool clean){if(serial!=s||!clean)return false;serial=0;return true;}};
}
namespace torquebow_state {
bool read_owned(h::Frame const& f,Observed& out,torquebow_cleanup::Anchor const& a,std::uint64_t s,torquebow_cleanup::Owns owns){
 if(!a.current(f,s,owns))return false;out=actual;return true;
}
}
namespace torquebow_buttons {
struct Entry {bool press{};};
bool bind(h::ObjectIdentity const&,bool press,Entry& out){out.press=press;return fixture::bind;}
R dispatch(h::Frame const& f,Entry const& entry,std::uint64_t s,torquebow_cleanup::Owns owns,bool(*ready)(h::Frame const&) noexcept){
 if(!owns(f,s)||!ready(f)||!fixture::live||fixture::result==R::rejected)return R::rejected;
 // Invocation callback sees the journal already in flight; an unknown return
 // may still have applied these original effects and must never be replayed.
 assert(s&&torquebow_input_owner::projectile_owner(f)==s);
 if(entry.press){++fixture::presses;torquebow_state::actual.charge=torquebow_state::actual.input_down=true;}
 else{++fixture::releases;torquebow_state::actual.charge=torquebow_state::actual.input_down=false;}
 return fixture::result;
}
}
namespace torquebow_fire {
bool queued{},consume_ok=true;SpawnReceipt queued_spawn{};
bool peek_spawn(SpawnReceipt& out) noexcept {out=queued_spawn;return queued;}
bool consume_spawn(std::uint64_t call) noexcept {
 if(!queued||call!=queued_spawn.call||!consume_ok)return false;queued=false;return true;
}
bool ready_for(h::Frame const&,std::int64_t) noexcept {return fixture::muzzle;}
void clear_source() noexcept {fixture::muzzle=false;++fixture::stops;}
}
namespace torquebow_charge {
struct CancelOwner {
 bool pending_{},was_charge{};gears_weapon::TorqueBowCancelPhase phase_{};
 bool begin(h::Frame const& f,torquebow_cleanup::Anchor const& a,std::uint64_t s,torquebow_cleanup::Owns owns,
            bool(*stop)(std::uint64_t) noexcept,gears_weapon::TorqueBowCancelRoute,bool projectile_accounted){
  if(!a.current(f,s,owns)||(torquebow_state::actual.fire&&!projectile_accounted))return false;
  assert(!fixture::muzzle);assert(stop(s));++fixture::begins;
  pending_=true;was_charge=torquebow_state::actual.charge;phase_=gears_weapon::TorqueBowCancelPhase::stop_injection;return true;
 }
 bool pending()const{return pending_;}
 gears_weapon::TorqueBowCancelPhase phase()const{return phase_;}
 bool began_in_charge()const{return was_charge;}
 bool drain(){if(!fixture::registration)return false;
  torquebow_state::actual.charge=torquebow_state::actual.input_down=false;
  phase_=gears_weapon::TorqueBowCancelPhase::complete;pending_=false;return true;}
 bool retire_completed(std::uint64_t){if(phase_!=gears_weapon::TorqueBowCancelPhase::complete)return false;*this={};return true;}
}cancel_owner;
}
#include "weapon_torque_bow_input_owner.inl"
namespace o=torquebow_input_owner;
int main(){
 h::Frame f{};f.key={1,2,3,4,5,6,7,8};f.reference_generation=1;
 f.controllers.session=f.controllers.space_epoch=f.controllers.profile_generation=f.controllers.input_epoch=1;
 f.controllers.sequence=100;f.controllers.sync_ns=1000000000;
 o::StopReceipt stopped{};
 assert(!o::projectile_owner(f));
 torquebow_state::actual.active=false;assert(!o::reserve_before_press(f)&&!o::pending());torquebow_state::actual.active=true;
 fixture::probe_supported=false;assert(!o::reserve_before_press(f)&&!o::pending());fixture::probe_supported=true;
 assert(o::reserve_before_press(f));auto first=o::serial();assert(o::pending()&&!o::projectile_owner(f));
 assert(o::cancel(o::Route::active,stopped)&&stopped.complete&&!o::pending());assert(!fixture::presses&&!fixture::begins);
 fixture::muzzle=true;fixture::result=R::rejected;assert(o::reserve_before_press(f));assert(o::serial()>first);
 assert(o::press(f)==R::rejected);assert(o::cancel(o::Route::active,stopped));assert(!fixture::presses&&!fixture::begins);
 fixture::muzzle=true;fixture::result=R::unknown;assert(o::reserve_before_press(f));assert(o::press(f)==R::unknown);
 assert(!o::projectile_owner(f)); // An uncertain original return cannot admit another root.
 assert(o::normal_release(f)==R::rejected&&!fixture::releases);
 fixture::live=false;assert(o::cancel(o::Route::active,stopped));assert(stopped.complete&&stopped.began_in_charge&&stopped.ammo_unchanged_from_press);
 assert(fixture::presses==1&&!fixture::releases&&fixture::begins==1&&!o::pending());
 fixture::live=fixture::muzzle=true;fixture::result=R::accepted;assert(o::reserve_before_press(f));assert(o::press(f)==R::accepted);
 assert(o::projectile_owner(f)==o::serial());
 auto changed=f;++changed.controllers.space_epoch;
 assert(!o::projectile_owner(changed));assert(o::normal_release(changed)==R::rejected&&!fixture::releases);
 ++fixture::hold;assert(!o::projectile_owner(f));--fixture::hold;
 changed=f;changed.controllers.source=changed.stereo.source=gears_vr::StereoSource::MetaOperatorSimulated;
 assert(!o::projectile_owner(changed)&&o::normal_release(changed)==R::rejected);
 assert(o::normal_release(f)==R::accepted&&fixture::releases==1);assert(o::retire_neutral()&&!o::pending());
 fixture::muzzle=true;assert(o::reserve_before_press(f));assert(o::press(f)==R::accepted);
 torquebow_state::actual.inventory.used=1;torquebow_state::actual.charge=false; // Already-real fatigue arrow.
 assert(o::cancel(o::Route::active,stopped));assert(stopped.complete&&!stopped.ammo_unchanged_from_press&&!stopped.began_in_charge);
 assert(torquebow_state::actual.inventory.used==1); // No refund or false no-shot cancellation.
 torquebow_state::actual.inventory.used=0;fixture::muzzle=true;
 assert(o::reserve_before_press(f));assert(o::press(f)==R::accepted);
 torquebow_fire::queued=true;const auto earlier_begins=fixture::begins;
 assert(!o::cancel(o::Route::active,stopped)&&!stopped.complete&&o::pending());
 assert(o::journal.cleanup_complete&&fixture::begins==earlier_begins+1);
 assert(!o::retire_neutral()&&!o::projectile_owner(f));
 assert(!o::cancel(o::Route::active,stopped)&&fixture::begins==earlier_begins+1);
 torquebow_fire::queued=false;
 assert(o::cancel(o::Route::active,stopped)&&stopped.complete&&!o::pending());
 assert(fixture::begins==earlier_begins+1); // Receipt wait never replays completed original cleanup.
 fixture::muzzle=true;
 torquebow_fire::queued=true;assert(!o::reserve_before_press(f));torquebow_fire::queued=false;
 // An actual projectile callback and its observed original ammo delta must
 // both exist before the REAL pure interaction model receives one shot.
 f.world={16,0,256,1,2,0,1};f.pawn={24,0,256,2,3,0,1};
 f.controller={32,0,256,3,4,0,1};f.weapon={40,0,256,4,5,0,1};
 assert(o::reserve_before_press(f));assert(o::press(f)==R::accepted);
 using E=gears_weapon::TorqueBowEvent;using SS=o::ShotStatus;
 gears_weapon::TorqueBowConfig config{};
 config.identity={16,24,32,40,f.key.world_generation,1,1,1};config.instance_config_serial=1;config.capacity=1;
 config.original_class_verified=config.original_charge_entry_verified=config.original_end_fire_verified=config.non_firing_abort_verified=true;
 gears_weapon::TorqueBowInteraction model;assert(model.bind(config,{1,1,5}));
 std::uint64_t sequence{};std::int64_t ns=1000000000;
 auto stamp=[&](){ns+=1000000;return gears_weapon::TorqueBowStamp{config.identity,++sequence,ns,0};};
 const auto observed=stamp();gears_weapon::TorqueBowOriginalState idle{true,false,false,false,false};
 assert(model.observe({observed,model.inventory(),idle,true,true,true,false,observed.monotonic_ns})==E::observed);
 assert(model.trigger(stamp(),{1,true,true,true})==E::request_ready);
 gears_weapon::TorqueBowRequest request{};assert(model.take_request(request,ns));
 const gears_weapon::TorqueBowOriginalState charged{false,true,false,true,false};
 assert(model.acknowledge({stamp(),request.token,1,true,model.inventory(),charged,{}})==E::charge_started);
 auto late_model=model;
 auto& spawn=torquebow_fire::queued_spawn;
 spawn={f,{48,0,80,3,4,0,1},{80,0,256,5,6,0,1},77,88,o::serial(),ns};torquebow_fire::queued=true;
 o::ShotObservation shot{};E event{};
 assert(o::inspect_original_shot(shot)==SS::waiting_for_ammo);
 ++spawn.input_serial;assert(o::inspect_original_shot(shot)==SS::source_mismatch);--spawn.input_serial;
 auto actual_source=spawn.source;++spawn.source.key.world_generation;
 assert(o::inspect_original_shot(shot)==SS::source_mismatch);spawn.source=actual_source;
 torquebow_state::actual.inventory.used=1;--torquebow_state::actual.inventory.reserve;
 assert(o::inspect_original_shot(shot)==SS::inventory_mismatch);++torquebow_state::actual.inventory.reserve;
 assert(o::inspect_original_shot(shot)==SS::ready);
 auto wrong_stamp=stamp();++wrong_stamp.identity.world_generation;
 assert(o::record_original_shot(model,wrong_stamp,2,event)==SS::model_rejected&&model.inventory().loaded==1);
 torquebow_fire::consume_ok=false;
 assert(o::record_original_shot(model,stamp(),2,event)==SS::receipt_retirement&&event==E::shot_recorded);
 assert(model.inventory().loaded==0&&model.inventory().reserve==5&&model.inventory().revision==2);
 assert(o::record_original_shot(model,stamp(),3,event)==SS::receipt_retirement&&event==E::rejected);
 assert(model.inventory().revision==2);torquebow_fire::consume_ok=true;
 assert(o::record_original_shot(model,stamp(),4,event)==SS::recorded&&model.inventory().revision==2);
 assert(!torquebow_fire::queued&&o::inspect_original_shot(shot)==SS::none);
 ++spawn.call;++spawn.shot;torquebow_fire::queued=true;
 assert(o::inspect_original_shot(shot)==SS::source_mismatch&&!o::retire_neutral());
 torquebow_fire::queued=false; // Fixture restores an injected impossible second arrow; production never discards it.
 torquebow_state::actual.charge=torquebow_state::actual.input_down=false;
 assert(o::retire_neutral());torquebow_state::actual.inventory.used=0;fixture::muzzle=true;
 assert(o::reserve_before_press(f));assert(o::press(f)==R::accepted);
 spawn={f,{56,0,80,4,5,0,1},{80,0,256,5,6,0,1},79,90,o::serial(),ns};torquebow_fire::queued=true;
 assert(late_model.tracking_lost()==E::abort_required);fixture::live=false;++fixture::hold;
 torquebow_state::actual.inventory.used=1;torquebow_state::actual.charge=false;
 assert(!o::projectile_owner(f));
 assert(o::record_original_shot(late_model,stamp(),5,event)==SS::recorded&&event==E::safety_violation);
 assert(late_model.inventory().loaded==0&&torquebow_state::actual.inventory.used==1); // Real late shot never refunded.
 assert(o::cancel(o::Route::active,stopped)&&!stopped.ammo_unchanged_from_press);
 --fixture::hold;fixture::live=fixture::muzzle=true;torquebow_state::actual.inventory.used=0;
 assert(o::reserve_before_press(f));assert(o::press(f)==R::accepted);fixture::registration=false;
 assert(!o::cancel(o::Route::active,stopped)&&o::pending()&&o::serial()); // Terminal loss retains responsibility.
 assert(!o::projectile_owner(f)); // The retained cleanup journal is not a projectile permission.
 std::puts("Bow original journal/accounting: owned projectile roots, exact callback+ammo pairing, no premature retirement/repeated cleanup, one model delivery, late real shot safety/no refund and terminal debt passed; injected typed/state/anchor/projectile backend only.");
}
