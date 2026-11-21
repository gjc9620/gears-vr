#include "../vr-common/weapon_gnasher_cycle_commit.h"
#include "../vr-common/weapon_support_grip.h"
#include "../vr-common/weapon_reload_transaction.h"
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <functional>
#ifdef NDEBUG
#error GnasherInventoryTest requires active checks
#endif
#define CHECK(v) do{if(!(v)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#v);std::abort();}}while(false)
using uptr=std::uintptr_t;namespace hand=gears_weapon_hand;namespace weapon=gears_weapon;
bool source_current=true,registered=true,owns_lease=true,readable=true;unsigned setter_dispatches{};int writer_mode{};
std::int64_t observed_now{};std::function<void()> reenter;
unsigned compensation_calls{};bool compensation_applies=true;std::function<void()> compensation_reenter;
hand::ObjectIdentity object(uptr p){return {p,0x8000,0x9000,unsigned(p/8),7,0,1};}
namespace weaponhand {unsigned current_gt(void*){return GetCurrentThreadId();}std::int64_t clock_ns(void*){return observed_now;}
struct Client {bool registration_alive(std::uint64_t c,std::uint64_t n){return registered&&c==33&&n==22;}}client;}
namespace nativeequipment {bool current(void*,hand::Frame const&){return source_current;}}
namespace playground {bool instance(uptr p,wchar_t const*){return p==0x5000;}}
namespace lancerreload_inventory {
struct Identity {hand::ObjectIdentity world{},pawn{},weapon{};std::uint64_t game_start{},native_start{},registration{},world_generation{},hold_generation{};
 bool valid()const{return hand::identity_valid(world)&&hand::identity_valid(pawn)&&hand::identity_valid(weapon)&&game_start&&native_start&&registration&&world_generation&&hold_generation;}
 bool operator==(Identity const& b)const{return hand::same(world,b.world)&&hand::same(pawn,b.pawn)&&hand::same(weapon,b.weapon)&&game_start==b.game_start&&native_start==b.native_start&&registration==b.registration&&world_generation==b.world_generation&&hold_generation==b.hold_generation;}};
Identity identity(hand::Frame const& f){return {f.world,f.pawn,f.weapon,f.key.game_start,f.key.native_started_qpc,f.key.registration_cookie,f.key.world_generation,1};}
bool same_instance_across_hold(Identity a,Identity b){b.hold_generation=a.hold_generation;return a==b;}
using Inventory=weapon::ReloadTxnInventory<Identity>;using Request=weapon::ReloadTxnRequest<Identity>;using Receipt=weapon::ReloadTxnReceipt<Identity>;
Inventory actual{};
struct ObservedInventory {bool sample(hand::Frame const&,Inventory& out){out=actual;return readable;}bool sample_owned(Identity const& id,Inventory& out){out=actual;return readable&&out.identity==id;}};
using OwnsReload=bool(*)(hand::Frame const&,std::uint64_t);enum class CommitResult {rejected,committed,reconciliation_required};
bool counts_equal(Inventory const& a,Inventory const& b){return a.identity==b.identity&&a.capacity==b.capacity&&a.loaded==b.loaded&&a.reserve==b.reserve&&a.infinite_reserve==b.infinite_reserve;}
// Boundary injection only. The production adapter below remains unchanged;
// this test never invokes real game setters or creates an original state lease.
struct InsertionWriter {CommitResult commit(hand::Frame const& f,Request const& r,ObservedInventory&,OwnsReload owns,Receipt& out){
 CHECK(owns(f,r.reload_serial));CHECK(r.call==weapon::ReloadTxnCall::insert);CHECK(r.expected.loaded==r.before.loaded+1&&r.expected.reserve+1==r.before.reserve);
 ++setter_dispatches;if(reenter)reenter();out={};
 if(writer_mode==2)return CommitResult::rejected;
 if(writer_mode==1){actual.reserve=r.expected.reserve;++actual.revision;return CommitResult::reconciliation_required;}
 if(writer_mode==4)return CommitResult::reconciliation_required;
 actual.loaded=r.expected.loaded;actual.reserve=r.expected.reserve;actual.revision=r.before.revision+2;
 if(writer_mode==3)return CommitResult::rejected; // Foreign/mismatched counts cannot establish an owned write.
 out.token=r.token;out.accepted=true;out.observed=actual;return CommitResult::committed;}};
}
namespace lancerreload_state {
struct Lease {lancerreload_inventory::Identity identity{};std::uint64_t serial{};bool acquired{};}lease;
bool owned_state(){return owns_lease;}
bool restore_partial_insert(lancerreload_inventory::Request const& r,lancerreload_inventory::ObservedInventory&){
 ++compensation_calls;if(compensation_reenter)compensation_reenter();
 if(compensation_applies){lancerreload_inventory::actual.reserve=r.before.reserve;++lancerreload_inventory::actual.revision;}
 return compensation_applies;
}
}
namespace gnasher_fire {
struct Entry {lancerreload_inventory::Identity identity{};lancerreload_inventory::ObservedInventory observer;weapon::GnasherFireCycle cycle;weapon::GnasherMechanicalInteraction mechanical{};bool known=true;};
std::array<Entry,16> entries{};
Entry* entry(hand::Frame const& f,bool){for(auto& e:entries)if(e.known&&hand::same(e.identity.weapon,f.weapon))return &e;return nullptr;}
weapon::GnasherIdentity mechanical_identity(hand::Frame const& f){return {f.world.address,f.pawn.address,f.weapon.address,f.key.world_generation,f.stereo.session,f.stereo.space_epoch,f.controllers.input_epoch,f.controllers.profile_generation};}
}
namespace gnasher_round {
bool captured=true,withdrawal_succeeds=true;unsigned withdrawals{};
std::function<void()> withdrawal_reenter{};
bool captured_for(hand::Frame const&,std::uint64_t item){return captured&&item==1;}
bool release_for_stow(lancerreload_inventory::Identity const& owner,std::uint64_t item){
 CHECK(owner==lancerreload_state::lease.identity&&item==1);++withdrawals;
 if(withdrawal_reenter)withdrawal_reenter();return withdrawal_succeeds;
}
}
#include "weapon_gnasher_inventory.inl"
bool owns(hand::Frame const&,std::uint64_t serial){return owns_lease&&serial==9;}
struct Fixture {
 hand::Frame f{};weapon::GnasherMechanicalInteraction mechanical;gnasher_fire::Entry entry;gnasher_inventory::Insertion insertion;
 weapon::GnasherMechanicalSample sample{};
 Fixture(){setter_dispatches=compensation_calls=0;writer_mode=0;source_current=registered=owns_lease=readable=compensation_applies=true;reenter={};compensation_reenter={};
  gnasher_round::captured=gnasher_round::withdrawal_succeeds=true;gnasher_round::withdrawals=0;gnasher_round::withdrawal_reenter={};
  f.key={11,22,33,1,1,1,44,55};f.world=object(0x6000);f.pawn=object(0x6020);f.weapon=object(0x5000);
  f.stereo.session=4;f.stereo.space_epoch=5;f.controllers.input_epoch=6;f.controllers.profile_generation=7;f.reference_generation=8;
  entry.identity=lancerreload_inventory::identity(f);lancerreload_inventory::actual={entry.identity,1,8,2,10,false};
  lancerreload_state::lease={entry.identity,9,true};
  weapon::GnasherMechanicalConfig config{};config.calibration_id=1;config.lever_calibrated=config.feed_calibrated=true;config.capacity=8;config.game_ticks_per_second=1000000;
  config.lever.left_front_grip_in_main.position={0,0,.08};config.lever.right_handle_home_in_main={0,-.2,0};config.lever.hinge_axis_in_main={1,0,0};
  const auto id=gnasher_fire::mechanical_identity(f);CHECK(mechanical.bind(id,config,{id,1,2,10}));CHECK(entry.cycle.bind(mechanical.inventory()));
  sample.identity=id;sample.monotonic_ns=1000000000;sample.game_tick=1;sample.left_tracked=sample.right_tracked=sample.right_handle_held=sample.original_ready=true;
  sample.reference_generation=f.reference_generation;sample.right_grip.position={0,-.2,0};
  step(.07);CHECK(mechanical.begin_reload(id,sample.monotonic_ns,sample.game_tick).event==weapon::GnasherMechanicalEvent::reload_started);
  CHECK(mechanical.grab_round(id,1,sample.monotonic_ns).event==weapon::GnasherMechanicalEvent::round_grabbed);sample.left_round_serial=1;
  for(double d:{.07,.05,.025})CHECK(step(d).event==weapon::GnasherMechanicalEvent::none);
  CHECK(step(.002).event==weapon::GnasherMechanicalEvent::request_inventory_commit);
 }
 weapon::GnasherMechanicalDecision step(double depth){sample.left_grip.position={0,0,depth};++sample.sequence;sample.monotonic_ns+=20000000;++sample.game_tick;
  f.controllers.sequence=sample.sequence;f.controllers.sync_ns=sample.monotonic_ns;observed_now=sample.monotonic_ns+1;return mechanical.observe(sample);}
 auto dispatch(){return insertion.dispatch(f,9,mechanical,entry,owns);}
};
int main(){using R=gnasher_inventory::Result;
 {Fixture x;gnasher_round::captured=false;CHECK(x.dispatch()==R::rejected);
  CHECK(!setter_dispatches&&!gnasher_round::withdrawals&&x.mechanical.held_round_serial()==1);}
 {Fixture x;gnasher_round::withdrawal_succeeds=false;
  gnasher_round::withdrawal_reenter=[&]{const auto n=gnasher_round::withdrawals;
   CHECK(x.insertion.reconcile(x.f,x.mechanical,x.entry)==R::consumer_pending);
   CHECK(x.insertion.recover(x.f,x.mechanical,x.entry)==R::consumer_pending);
   CHECK(x.dispatch()==R::reconciliation_required&&!x.insertion.retire());CHECK(gnasher_round::withdrawals==n);};
  CHECK(x.dispatch()==R::consumer_pending);
  CHECK(setter_dispatches==1&&gnasher_round::withdrawals==1&&x.insertion.pending()&&!x.insertion.retire());
  CHECK(x.mechanical.inventory().loaded==3&&x.mechanical.inventory().reserve==9&&x.mechanical.held_round_serial()==0);
  CHECK(x.entry.cycle.reload_cycle_pending()&&x.insertion.blocks_fire(x.f.weapon));
  CHECK(x.dispatch()==R::reconciliation_required&&setter_dispatches==1);
  const auto before=x.mechanical.inventory();
  CHECK(x.insertion.reconcile(x.f,x.mechanical,x.entry)==R::consumer_pending&&setter_dispatches==1);
  CHECK(x.mechanical.inventory().revision==before.revision&&!x.mechanical.inventory_request().token);
  registered=false;gnasher_round::withdrawal_succeeds=true;const auto attempts=gnasher_round::withdrawals;
  CHECK(x.insertion.reconcile(x.f,x.mechanical,x.entry)==R::consumer_pending&&gnasher_round::withdrawals==attempts);
  registered=true;observed_now+=5000000000;source_current=false;x.f.weapon=object(0x5100);
  ++x.f.key.world_generation;x.f.world=object(0x7000);x.f.pawn=object(0x7020);
  CHECK(x.insertion.reconcile(x.f,x.mechanical,x.entry)==R::committed);
  CHECK(setter_dispatches==1&&!x.insertion.pending()&&x.mechanical.inventory().loaded==3&&x.mechanical.inventory().reserve==9);
  const auto hidden=gnasher_round::withdrawals;CHECK(x.insertion.reconcile(x.f,x.mechanical,x.entry)==R::committed);
  CHECK(gnasher_round::withdrawals==hidden&&x.insertion.retire());}
 {Fixture x;observed_now+=5000000000;source_current=false;
  CHECK(x.insertion.reject_undispatched(x.f,x.mechanical,x.entry)==R::rejected);
  CHECK(!setter_dispatches&&!x.mechanical.inventory_request().token&&x.mechanical.held_round_serial()==1);
  CHECK(x.mechanical.inventory().loaded==2&&x.entry.cycle.inventory().loaded==2);
  CHECK(x.dispatch()==R::rejected&&!setter_dispatches);}
 {Fixture x;writer_mode=1;CHECK(x.dispatch()==R::reconciliation_required);
  CHECK(x.insertion.reject_undispatched(x.f,x.mechanical,x.entry)==R::reconciliation_required);
  CHECK(x.mechanical.inventory_request().token&&setter_dispatches==1);}
 for(unsigned bad=0;bad<5;++bad){Fixture x;
  if(bad==0)++x.f.key.world_generation;if(bad==1)registered=false;if(bad==2)++lancerreload_inventory::actual.loaded;
  if(bad==3)--lancerreload_inventory::actual.reserve;if(bad==4)++x.entry.identity.weapon.registry_index;
  CHECK(x.insertion.reject_undispatched(x.f,x.mechanical,x.entry)==R::reconciliation_required);
  CHECK(!setter_dispatches&&x.mechanical.inventory_request().token&&x.mechanical.held_round_serial()==1);}
 {Fixture x;reenter=[&]{CHECK(x.insertion.pending());CHECK(x.insertion.blocks_fire(x.f.weapon));CHECK(x.insertion.reconcile(x.f,x.mechanical,x.entry)==R::reconciliation_required);CHECK(x.dispatch()==R::reconciliation_required);};
  CHECK(x.dispatch()==R::committed);CHECK(setter_dispatches==1&&!x.insertion.pending());
  CHECK(x.mechanical.inventory().loaded==3&&x.entry.cycle.inventory().loaded==3&&x.entry.cycle.reload_cycle_pending());
  CHECK(x.dispatch()==R::rejected&&setter_dispatches==1);CHECK(x.insertion.retire());CHECK(x.dispatch()==R::rejected&&setter_dispatches==1);}
 for(unsigned field=0;field<8;++field){Fixture x;
  if(field==0)observed_now+=100000001;if(field==1)++x.f.controllers.sequence;if(field==2)++x.f.reference_generation;
  if(field==3)source_current=false;if(field==4)owns_lease=false;if(field==5)lancerreload_inventory::actual.capacity=7;
  if(field==6)lancerreload_inventory::actual.infinite_reserve=true;if(field==7)++x.f.weapon.registry_index;
  CHECK(x.dispatch()==R::rejected);CHECK(!setter_dispatches&&!x.insertion.pending()&&x.mechanical.inventory().loaded==2);}
 {Fixture x;writer_mode=1;CHECK(x.dispatch()==R::reconciliation_required);CHECK(setter_dispatches==1&&x.insertion.pending());
  CHECK(x.mechanical.inventory().loaded==2&&x.entry.cycle.inventory().reserve==10);CHECK(!x.insertion.retire());
  CHECK(x.dispatch()==R::reconciliation_required&&setter_dispatches==1);
  // The original owned counts later settle. Receipt-only reconciliation uses
  // no old input and can finish while the owner has selected another weapon.
  observed_now+=5000000000;x.f.weapon=object(0x5100);lancerreload_inventory::actual.loaded=3;++lancerreload_inventory::actual.revision;
  CHECK(x.insertion.reconcile(x.f,x.mechanical,x.entry)==R::committed);CHECK(setter_dispatches==1&&x.mechanical.inventory().loaded==3);}
 for(int outcome:{2,4}){Fixture x;writer_mode=outcome;CHECK(x.dispatch()==R::rejected);CHECK(setter_dispatches==1&&!x.insertion.pending());
  CHECK(x.mechanical.inventory().loaded==2&&x.mechanical.held_round_serial()==1&&!x.mechanical.inventory_request().token);
  CHECK(!x.entry.cycle.blocked());CHECK(x.insertion.retire());}
 {Fixture x;writer_mode=3;CHECK(x.dispatch()==R::reconciliation_required);CHECK(x.insertion.pending()&&x.mechanical.inventory().loaded==2);}
 {Fixture x;writer_mode=1;CHECK(x.dispatch()==R::reconciliation_required);
  compensation_reenter=[&]{CHECK(x.insertion.recover(x.f,x.mechanical,x.entry)==R::reconciliation_required);CHECK(x.dispatch()==R::reconciliation_required);};
  observed_now+=5000000000;source_current=false;x.f.weapon=object(0x5100);
  CHECK(x.insertion.recover(x.f,x.mechanical,x.entry)==R::rejected);
  CHECK(compensation_calls==1&&setter_dispatches==1&&!x.insertion.pending());
  CHECK(x.mechanical.inventory().loaded==2&&x.mechanical.inventory().reserve==10&&x.mechanical.held_round_serial()==1);
  CHECK(x.insertion.recover(x.f,x.mechanical,x.entry)==R::rejected&&compensation_calls==1);}
 {Fixture x;writer_mode=1;CHECK(x.dispatch()==R::reconciliation_required);compensation_applies=false;
  CHECK(x.insertion.recover(x.f,x.mechanical,x.entry)==R::reconciliation_required);compensation_applies=true;
  CHECK(x.insertion.recover(x.f,x.mechanical,x.entry)==R::reconciliation_required&&compensation_calls==1&&setter_dispatches==1);
  lancerreload_inventory::actual.reserve=10;++lancerreload_inventory::actual.revision;
  CHECK(x.insertion.reconcile(x.f,x.mechanical,x.entry)==R::rejected);}
 for(unsigned bad=0;bad<5;++bad){Fixture x;writer_mode=1;CHECK(x.dispatch()==R::reconciliation_required);
  if(bad==0)++lancerreload_state::lease.serial;if(bad==1)owns_lease=false;if(bad==2)lancerreload_state::lease.acquired=false;
  if(bad==3)++lancerreload_state::lease.identity.world_generation;if(bad==4)--lancerreload_inventory::actual.reserve;
  CHECK(x.insertion.recover(x.f,x.mechanical,x.entry)==R::reconciliation_required&&!compensation_calls&&setter_dispatches==1);}
 {Fixture x;writer_mode=1;CHECK(x.dispatch()==R::reconciliation_required);++x.f.key.world_generation;
  lancerreload_inventory::actual.loaded=3;++lancerreload_inventory::actual.revision;
  CHECK(x.insertion.reconcile(x.f,x.mechanical,x.entry)==R::reconciliation_required);CHECK(setter_dispatches==1&&x.mechanical.inventory().loaded==2);}
 {Fixture x;writer_mode=1;CHECK(x.dispatch()==R::reconciliation_required);
  gnasher_inventory::records={};gnasher_fire::entries={};
  gnasher_inventory::records[0]={x.entry.identity,x.insertion,true};
  gnasher_fire::entries[0]=x.entry;gnasher_fire::entries[0].mechanical=x.mechanical;
  CHECK(gnasher_inventory::pending_work()&&gnasher_inventory::blocks_fire(x.f.weapon));
  CHECK(!gnasher_inventory::blocks_fire(object(0x5100)));
  x.f.weapon=object(0x5100);lancerreload_inventory::actual.loaded=3;++lancerreload_inventory::actual.revision;
  gnasher_inventory::service(x.f);CHECK(!gnasher_inventory::pending_work());
  CHECK(gnasher_fire::entries[0].mechanical.inventory().loaded==3&&gnasher_fire::entries[0].cycle.reload_cycle_pending());}
 std::puts("Production Gnasher inventory adapter: one original writer dispatch, source/reference freshness, reentry, paired count ACK, partial retention, late receipt and no setter replay passed (injected original-writer boundary)");
}
