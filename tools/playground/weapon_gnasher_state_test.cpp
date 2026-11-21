#include "../vr-common/weapon_gnasher_cycle_commit.h"
#include "../vr-common/weapon_gnasher_cycle_readback.h"
#include "../vr-common/weapon_support_grip.h"
#include "../vr-common/weapon_reload_transaction.h"
#include <cstdio>
#include <cstdlib>
#include <atomic>
#include <cwchar>
#ifdef NDEBUG
#error GnasherStateTest requires active checks
#endif
#define CHECK(v) do{if(!(v)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#v);std::abort();}}while(false)
using uptr=std::uintptr_t;namespace hand=gears_weapon_hand;namespace w=gears_weapon;
std::uint64_t current_hold=1;
hand::ObjectIdentity object(uptr p){return {p,0x8000,0x9000,unsigned(p/8),7,0,1};}
bool source_current=true,inventory_pending=false,owned_readable=true,fail_after_recovery=false,registration_alive=true;std::int64_t observed_now=1000000001;
namespace weaponhand {unsigned current_gt(void*){return 1;}std::int64_t clock_ns(void*){return observed_now;}
struct Client {bool registration_alive(std::uint64_t,std::uint64_t){return ::registration_alive;}}client;}
namespace nativeequipment {bool current(void*,hand::Frame const&){return source_current;}}
namespace lancerreload_runtime {std::atomic<bool> stop_requested{};}
namespace playground {bool instance(uptr p,wchar_t const*){return p==0x5000;}}
namespace lancerreload_inventory {
struct Identity {hand::ObjectIdentity world{},pawn{},weapon{};std::uint64_t generation{},hold_generation{};
 bool valid()const{return hand::identity_valid(world)&&hand::identity_valid(pawn)&&hand::identity_valid(weapon)&&generation&&hold_generation;}
 bool operator==(Identity const& b)const{return hand::same(world,b.world)&&hand::same(pawn,b.pawn)&&hand::same(weapon,b.weapon)&&generation==b.generation&&hold_generation==b.hold_generation;}};
Identity identity(hand::Frame const& f){return {f.world,f.pawn,f.weapon,f.key.world_generation,current_hold};}
bool same_instance_across_hold(Identity a,Identity b){if(b.hold_generation<a.hold_generation)return false;b.hold_generation=a.hold_generation;return a==b;}
using Inventory=w::ReloadTxnInventory<Identity>;using Request=w::ReloadTxnRequest<Identity>;
Inventory actual{};
bool counts_equal(Inventory const& a,Inventory const& b){return a.identity==b.identity&&a.capacity==b.capacity&&a.loaded==b.loaded&&a.reserve==b.reserve&&a.infinite_reserve==b.infinite_reserve;}
struct ObservedInventory {bool sample(hand::Frame const& f,Inventory& out){out=actual;return out.identity==identity(f);}bool sample_owned(Identity const& id,Inventory& out){out=actual;return owned_readable&&id==out.identity;}
 bool rebind_hold(hand::Frame const& f,Identity const& previous,Inventory& out){if(!source_current||!same_instance_across_hold(previous,identity(f)))return false;out=actual;out.identity=identity(f);actual.identity=out.identity;return true;}};
}
namespace gnasher_inventory {bool cancel_known{},cancel_calls{};
bool blocks_fire(hand::ObjectIdentity const&){return inventory_pending;}void recover_round(hand::Frame const&){}
void reject_undispatched_round(hand::Frame const&);}
namespace gnasher_round {bool release_ok=true;unsigned releases{};
bool release_for_stow(lancerreload_inventory::Identity const&,std::uint64_t item){CHECK(item);++releases;return release_ok;}}
namespace gnasher_animation {bool active{},release_ok=true,service_ok=true;unsigned services{};
bool owns_weapon(hand::ObjectIdentity const&){return active;}bool pending_work(){return active;}
bool owns_controller(uptr pc){return active&&pc==0x6100;}
lancerreload_inventory::Identity owned_identity();
bool service_owned(){if(!active)return true;++services;return service_ok&&registration_alive&&owned_readable;}
bool release_owned_after_exit(lancerreload_inventory::Identity const&){if(!active)return true;if(!release_ok||!service_ok||!registration_alive||!owned_readable)return false;active=false;return true;}}
namespace gnasher_fire {
struct Entry {lancerreload_inventory::Identity identity{};lancerreload_inventory::ObservedInventory observer;w::GnasherFireCycle cycle;w::GnasherMechanicalInteraction mechanical;bool prepared{},known=true;};std::array<Entry,1> entries{};Entry& saved=entries[0];
Entry* entry(hand::Frame const& f,bool){return hand::same(f.weapon,saved.identity.weapon)?&saved:nullptr;}
w::GnasherIdentity mechanical_identity(hand::Frame const& f){return {f.world.address,f.pawn.address,f.weapon.address,f.key.world_generation,f.stereo.session,f.stereo.space_epoch,f.controllers.input_epoch,f.controllers.profile_generation};}
w::GnasherInventory counts(hand::Frame const& f,lancerreload_inventory::Inventory const& i){return {mechanical_identity(f),i.revision,i.loaded,i.reserve};}
}
namespace gnasher_animation {lancerreload_inventory::Identity owned_identity(){return gnasher_fire::saved.identity;}}
namespace gnasher_inventory {
void reject_undispatched_round(hand::Frame const&){
    cancel_calls=true;if(!cancel_known)return;
    auto& e=gnasher_fire::saved;const auto r=e.mechanical.inventory_request();CHECK(r.token);
    CHECK(w::gnasher_reject_inserted_round(e.mechanical,e.cycle,
        {r.token,false,{r.identity,r.before_revision,r.before_loaded,r.before_reserve}}));
}}
namespace lancerreload_state {
namespace inv=lancerreload_inventory;
enum class BeginResult {rejected,acquired,recovery_required};
struct RewardReceipt {bool dispatched{},applied{};};
struct Lease {inv::Identity identity{};std::uint64_t serial{};float original_start{};bool acquired{};float reward_start{};}lease;
unsigned starts{},exits{},recoveries{},ui_calls{},reward_calls{};int start_mode{};bool exit_ok=true,reward_ok=true;
float original_start=1.f;
bool time_field(uptr,wchar_t const*,float& v){v=1;return true;}
bool capture_reward_eligibility(inv::Inventory const&,RewardReceipt&){return true;}
BeginResult begin_for_part(hand::Frame const& f,std::uint64_t& serial,hand::LeaseKind part,Lease const* suspended,bool* dispatched){
 serial=0;CHECK(part==hand::LeaseKind::ammo);if(start_mode==1){*dispatched=false;return BeginResult::rejected;}
 ++starts;*dispatched=true;if(start_mode==2)return BeginResult::rejected;
 serial=9;lease={inv::identity(f),serial,original_start,true,suspended?suspended->reward_start:original_start};return start_mode==3?BeginResult::recovery_required:BeginResult::acquired;
}
bool owns(hand::Frame const& f,std::uint64_t serial){return lease.acquired&&lease.serial==serial&&lease.identity==inv::identity(f);}
bool bool_call(uptr,wchar_t const* name,bool& value){value=std::wcscmp(name,L"IsReloading")==0&&lease.acquired;return true;}
bool exit_without_fill(hand::Frame const& f,std::uint64_t serial){CHECK(owns(f,serial));++exits;if(exit_ok)lease={};return exit_ok;}
bool recover_serial(std::uint64_t serial){CHECK(serial==9);++recoveries;if(exit_ok)lease={};if(fail_after_recovery)owned_readable=false;return exit_ok;}
bool set_ui_timing(hand::Frame const&,std::uint64_t serial,std::int64_t total,std::int64_t perfect,bool replace){CHECK(serial==9&&total==4000&&perfect==4000&&replace);++ui_calls;return true;}
bool tick_ui_timing(){return true;}
bool perfect_reward(inv::Request const& request,inv::Inventory const& opening,inv::ObservedInventory&,RewardReceipt& reward){
 CHECK(request.expected.loaded==opening.loaded+3);CHECK(!lease.acquired);if(!reward.dispatched){++reward_calls;reward.dispatched=true;}reward.applied=reward_ok;return reward_ok;
}
}
#include "weapon_gnasher_state.inl"
struct Fixture {
 hand::Frame f{};hand::Decision d{};w::GnasherMechanicalSample sample{};
 w::GnasherClosedLeverReference reference{22,{{0,.2,-.03},{}}};w::GnasherCycleReadbackGate gate;
 Fixture(){using namespace lancerreload_state;lancerreload_runtime::stop_requested=false;current_hold=1;lease={};starts=exits=recoveries=ui_calls=reward_calls=0;start_mode=0;original_start=1.f;exit_ok=reward_ok=source_current=owned_readable=registration_alive=true;inventory_pending=fail_after_recovery=false;gnasher_state::records={};gnasher_fire::saved={};gnasher_animation::active=false;gnasher_animation::release_ok=gnasher_animation::service_ok=true;gnasher_animation::services=0;
  f.key={11,22,33,1,1,1,44,55};f.world=object(0x6000);f.pawn=object(0x6020);f.controller=object(0x6100);f.weapon=object(0x5000);
  gnasher_round::release_ok=true;gnasher_round::releases=0;
  f.stereo.session=4;f.stereo.space_epoch=5;f.controllers.input_epoch=6;f.controllers.profile_generation=7;f.reference_generation=8;
  f.press_token=20;f.left_tracked=f.left_grip_known=f.left_grip_down=true;
  d.key=f.key;d.press_token=f.press_token;d.lease={hand::LeaseKind::ammo,0,f.key.lifecycle,f.press_token};d.new_grant=1;d.release_required=0;d.status=hand::DecisionStatus::granted;
  auto& e=gnasher_fire::saved;e.identity=lancerreload_inventory::identity(f);lancerreload_inventory::actual={e.identity,1,8,2,10,false};
  w::GnasherMechanicalConfig config{};config.calibration_id=1;config.lever_calibrated=config.feed_calibrated=true;config.capacity=8;config.game_ticks_per_second=1000000;
  config.lever.left_front_grip_in_main.position={0,0,.08};config.lever.right_handle_home_in_main={0,-.2,0};config.lever.hinge_axis_in_main={1,0,0};
  const auto id=gnasher_fire::mechanical_identity(f);CHECK(e.mechanical.bind(id,config,{id,1,2,10}));CHECK(e.cycle.bind(e.mechanical.inventory()));
  sample.identity=id;sample.monotonic_ns=980000000;sample.game_tick=999990;sample.reference_generation=8;
  sample.left_tracked=sample.right_tracked=sample.right_handle_held=sample.original_ready=true;step(0,.08);
 }
 w::GnasherMechanicalDecision step(double angle,double depth=.08){
  sample.right_grip={{0,-.2*std::cos(angle),-.2*std::sin(angle)},{std::sin(angle*.5),0,0,std::cos(angle*.5)}};
  sample.left_grip.position={0,0,depth};++sample.sequence;sample.monotonic_ns+=20000000;sample.game_tick+=10;
  f.controllers.sequence=sample.sequence;f.controllers.sync_ns=sample.monotonic_ns;observed_now=sample.monotonic_ns+1;
  return gnasher_fire::saved.mechanical.observe(sample);
 }
 bool begin(){const bool result=gnasher_state::begin(f,d);if(result)sample.game_tick=std::int64_t(double(lancerreload_state::original_start)*1000000.);return result;}
 void insert_rounds(unsigned count){auto& e=gnasher_fire::saved;
  for(unsigned round=1;round<=count;++round){CHECK(e.mechanical.grab_round(sample.identity,round,sample.monotonic_ns).event==w::GnasherMechanicalEvent::round_grabbed);sample.left_round_serial=round;
   for(double depth:{.07,.05,.025,.002})step(0,depth);
   auto request=e.mechanical.inventory_request();CHECK(request.token);auto& actual=lancerreload_inventory::actual;
   ++actual.loaded;--actual.reserve;++actual.revision;w::GnasherInventoryReceipt receipt{request.token,true,gnasher_fire::counts(f,actual)};
   CHECK(w::gnasher_commit_inserted_round(e.mechanical,e.cycle,receipt));sample.left_round_serial=0;
  }
 }
 void rounds(unsigned count=3,std::int64_t closed_tick=0){auto& e=gnasher_fire::saved;insert_rounds(count);
  if(count<3)CHECK(e.mechanical.request_close(sample.identity,sample.monotonic_ns).reload_event==w::ReloadEvent::mechanism_needed);
  sample.left_front_held=true;sample.support_grasp_token=21;
  const double path[]={0.,.1,.2,.34,.48,.34,.2,.1,0.};
  for(unsigned i=0;i<9;++i){if(i==8&&closed_tick)sample.game_tick=closed_tick-10;step(path[i]);}
  CHECK(e.mechanical.cycle_request().token);CHECK(gate.begin(e.mechanical.cycle_request(),reference,observed_now));
 }
 w::GnasherCycleReadback readback(bool ready=false,std::uint64_t sequence=1){auto const request=gnasher_fire::saved.mechanical.cycle_request();return {sample.identity,request.token,22,sample.sequence,sequence,sample.monotonic_ns,reference.in_main,true,ready};}
 bool prepare(){return gnasher_state::prepare_cycle(f,gnasher_fire::saved.mechanical.cycle_request(),gate,readback());}
 void accept(){auto& e=gnasher_fire::saved;w::GnasherCycleReceipt receipt{};CHECK(gate.observe(readback(true,2),observed_now,receipt));
  CHECK(w::gnasher_commit_closed_cycle(e.mechanical,e.cycle,receipt,gnasher_fire::counts(f,lancerreload_inventory::actual)));
  gnasher_state::cycle_accepted(f,receipt);
 }
};
int main(){namespace state=gnasher_state;using S=state::Stage;
 {Fixture x;CHECK(x.begin());CHECK(gnasher_fire::saved.mechanical.request_close(x.sample.identity,x.sample.monotonic_ns).event==w::GnasherMechanicalEvent::reload_cancelled);
  ++x.f.press_token;x.d.press_token=x.d.lease.id=x.f.press_token;x.d.lease.kind=hand::LeaseKind::support;
  CHECK(state::close_empty(x.f,x.d));CHECK(state::record(x.f)->stage==S::empty_settlement&&!lancerreload_state::lease.acquired);
  state::service(x.f);CHECK(state::pending_work());++x.f.key.frame;state::service(x.f);
  CHECK(!state::pending_work()&&!lancerreload_state::reward_calls&&lancerreload_state::starts==1&&lancerreload_state::exits==1);
  CHECK(gnasher_fire::saved.mechanical.inventory().loaded==2&&gnasher_fire::saved.mechanical.inventory().reserve==10);}
 {Fixture x;CHECK(x.begin());CHECK(gnasher_fire::saved.mechanical.request_close(x.sample.identity,x.sample.monotonic_ns).event==w::GnasherMechanicalEvent::reload_cancelled);
  ++x.f.press_token;x.d.press_token=x.d.lease.id=x.f.press_token;x.d.lease.kind=hand::LeaseKind::support;lancerreload_state::exit_ok=false;
  CHECK(!state::close_empty(x.f,x.d)&&state::record(x.f)->stage==S::empty_exiting);state::service(x.f);CHECK(state::pending_work());
  lancerreload_state::exit_ok=true;state::service(x.f);CHECK(state::record(x.f)->stage==S::empty_settlement);
  ++x.f.key.frame;++lancerreload_inventory::actual.loaded;state::service(x.f);CHECK(state::pending_work()&&!lancerreload_state::reward_calls);
  --lancerreload_inventory::actual.loaded;state::service(x.f);CHECK(!state::pending_work()&&lancerreload_state::starts==1);}
 for(unsigned bad=0;bad<8;++bad){Fixture x;if(bad==0)x.d.new_grant=0;if(bad==1)x.d.lease.kind=hand::LeaseKind::support;if(bad==2)++x.d.press_token;
  if(bad==3)source_current=false;if(bad==4)inventory_pending=true;if(bad==5)observed_now+=100000001;if(bad==6)++x.f.controllers.sequence;if(bad==7)++lancerreload_inventory::actual.loaded;
  CHECK(!x.begin()&&!lancerreload_state::starts&&!state::pending_work());}
 for(int mode:{1,2,3}){Fixture x;lancerreload_state::start_mode=mode;CHECK(!x.begin());CHECK(!x.begin());
  CHECK(lancerreload_state::starts==(mode==1?0u:1u));CHECK(state::pending_work()==(mode!=1));CHECK(state::blocks_fire(x.f.weapon)==(mode!=1));}
 {Fixture x;lancerreload_state::start_mode=3;CHECK(!x.begin());state::service(x.f);
  CHECK(!state::pending_work()&&lancerreload_state::starts==1&&lancerreload_state::recoveries==1&&!lancerreload_state::reward_calls);
  CHECK(!x.begin()&&lancerreload_state::starts==1);}
 {Fixture x;lancerreload_state::start_mode=2;CHECK(!x.begin());state::service(x.f);
  CHECK(state::pending_work()&&lancerreload_state::starts==1&&!lancerreload_state::recoveries);}
 {Fixture x;lancerreload_state::start_mode=3;CHECK(!x.begin());fail_after_recovery=true;state::service(x.f);
  CHECK(state::record(x.f)->stage==S::start_settlement&&lancerreload_state::recoveries==1);
  owned_readable=true;state::service(x.f);CHECK(!state::pending_work()&&lancerreload_state::recoveries==1);}
 {Fixture x;CHECK(x.begin());CHECK(!x.begin());CHECK(state::pending_work());state::service(x.f);CHECK(lancerreload_state::ui_calls==1);
  x.rounds();auto bad=x.readback();bad.lever_in_main.position.y+=.02;
  CHECK(!state::prepare_cycle(x.f,gnasher_fire::saved.mechanical.cycle_request(),x.gate,bad));CHECK(!lancerreload_state::exits);
  CHECK(!x.prepare());CHECK(lancerreload_state::exits==1&&!lancerreload_state::reward_calls);
  CHECK(!x.prepare());CHECK(lancerreload_state::exits==1);++x.f.key.frame;CHECK(x.prepare());
  state::service(x.f);CHECK(!lancerreload_state::reward_calls);x.accept();CHECK(state::record(x.f)->stage==S::finishing);
  lancerreload_state::reward_ok=false;state::service(x.f);CHECK(state::pending_work()&&lancerreload_state::reward_calls==1);
  lancerreload_state::reward_ok=true;state::service(x.f);CHECK(!state::pending_work()&&lancerreload_state::reward_calls==1);
  CHECK(!x.begin()&&lancerreload_state::starts==1);}
 {Fixture x;CHECK(x.begin());x.rounds(2);CHECK(!x.prepare());++x.f.key.frame;CHECK(x.prepare());x.accept();state::service(x.f);
  CHECK(!state::pending_work()&&!lancerreload_state::reward_calls);}
 {Fixture x;CHECK(x.begin());x.rounds(2);auto forged=gnasher_fire::saved.mechanical.cycle_request();forged.completion=w::ReloadEvent::perfect;
  CHECK(!state::prepare_cycle(x.f,forged,x.gate,x.readback())&&!lancerreload_state::exits);}
 {Fixture x;lancerreload_state::original_start=1.25f;CHECK(x.begin());x.rounds(3,5250000);
  CHECK(gnasher_fire::saved.mechanical.cycle_request().completion==w::ReloadEvent::perfect);
  CHECK(!x.prepare());++x.f.key.frame;CHECK(x.prepare());x.accept();
  --lancerreload_inventory::actual.reserve;state::service(x.f);CHECK(state::pending_work()&&!lancerreload_state::reward_calls);
  ++lancerreload_inventory::actual.reserve;source_current=false;state::service(x.f);CHECK(!lancerreload_state::reward_calls);
  source_current=true;state::service(x.f);CHECK(!state::pending_work()&&lancerreload_state::reward_calls==1);}
 {Fixture x;CHECK(x.begin());x.rounds();lancerreload_state::exit_ok=false;CHECK(!x.prepare());CHECK(state::record(x.f)->stage==S::exiting);
  state::service(x.f);CHECK(!lancerreload_state::reward_calls);lancerreload_state::exit_ok=true;++x.f.key.frame;state::service(x.f);
  CHECK(state::record(x.f)->stage==S::awaiting_cycle&&!x.prepare());++x.f.key.frame;CHECK(x.prepare());x.accept();state::service(x.f);CHECK(!state::pending_work());}
 {Fixture x;CHECK(x.begin());x.insert_rounds(2);const auto started=gnasher_fire::saved.mechanical.reload_started_game_tick();
  gnasher_animation::active=true;bool changed{};CHECK(state::prepare_stow(x.f,changed)&&changed&&state::stow_ready(x.f));
  CHECK(state::record(x.f)->stage==S::parked&&gnasher_fire::saved.mechanical.hold_suspended());
  CHECK(!gnasher_animation::active&&!lancerreload_state::lease.acquired&&!lancerreload_state::reward_calls);
  state::service(x.f);CHECK(state::record(x.f)->stage==S::parked);++current_hold;++x.f.key.frame;
  x.d.key=x.f.key;CHECK(!state::resume(x.f,&x.d)); // The former ammo press cannot re-enter.
  ++x.f.press_token;x.d.press_token=x.d.lease.id=x.f.press_token;
  lancerreload_state::original_start=10.f;CHECK(state::resume(x.f,&x.d));
  CHECK(lancerreload_state::starts==2&&lancerreload_state::lease.original_start==10.f&&lancerreload_state::lease.reward_start==1.f);
  CHECK(gnasher_fire::saved.mechanical.reload_started_game_tick()==started&&gnasher_fire::saved.mechanical.target()==3&&gnasher_fire::saved.mechanical.inserted()==2);
  CHECK(!gnasher_fire::saved.mechanical.hold_suspended()&&gnasher_fire::saved.mechanical.inventory().loaded==4);
  CHECK(!gnasher_fire::saved.mechanical.can_request_fire(observed_now));}
 {Fixture x;bool changed{};CHECK(state::prepare_stow(x.f,changed)&&!changed);
  state::service(x.f);CHECK(state::record(x.f)->stage==S::parked);
  ++current_hold;++x.f.key.frame;state::service(x.f);
  CHECK(state::record(x.f)->stage==S::idle&&!lancerreload_state::starts);
  CHECK(!gnasher_fire::saved.mechanical.can_request_fire(observed_now));x.step(0,.08);
  CHECK(gnasher_fire::saved.mechanical.can_request_fire(observed_now));}
 {Fixture x;bool changed{};CHECK(state::prepare_stow(x.f,changed));++x.f.key.frame;
  state::service(x.f,true,true);CHECK(state::record(x.f)->stage==S::parked);
  state::service(x.f,false,false);CHECK(state::record(x.f)->stage==S::parked);
  state::service(x.f,false,true);CHECK(state::record(x.f)->stage==S::idle&&!lancerreload_state::starts);}
 for(unsigned bad=0;bad<4;++bad){Fixture x;CHECK(x.begin());x.insert_rounds(2);bool changed{};CHECK(state::prepare_stow(x.f,changed));
  ++current_hold;++x.f.key.frame;++x.f.press_token;x.d.key=x.f.key;x.d.press_token=x.d.lease.id=x.f.press_token;
  if(bad==0)x.d.new_grant=0;if(bad==1)x.d.lease.kind=hand::LeaseKind::support;if(bad==2)++lancerreload_inventory::actual.loaded;
  if(bad==3)++x.f.key.world_generation;
  CHECK(!state::resume(x.f,&x.d)&&lancerreload_state::starts==1&&gnasher_fire::saved.mechanical.hold_suspended());}
 {Fixture x;CHECK(x.begin());x.insert_rounds(2);bool changed{};CHECK(state::prepare_stow(x.f,changed));
  ++current_hold;++x.f.key.frame;++x.f.press_token;x.d.key=x.f.key;x.d.press_token=x.d.lease.id=x.f.press_token;lancerreload_state::start_mode=3;
  CHECK(!state::resume(x.f,&x.d)&&state::record(x.f)->stage==S::recovery_required);
  state::service(x.f);CHECK(state::record(x.f)->stage==S::parked&&gnasher_fire::saved.mechanical.hold_suspended());
  CHECK(lancerreload_state::starts==2&&!lancerreload_state::reward_calls&&!lancerreload_state::lease.acquired);}
 {Fixture x;CHECK(x.begin());x.insert_rounds(2);source_current=false;
  state::service_unselected(x.f.controller.address,0x7000,false);
  CHECK(state::record(x.f)->stage==S::parked&&!lancerreload_state::lease.acquired);
  CHECK(gnasher_fire::saved.mechanical.hold_suspended()&&gnasher_fire::saved.mechanical.inserted()==2);
  CHECK(state::pending_work()&&!lancerreload_state::reward_calls);
  state::service_unselected(x.f.controller.address,0x7000,true);CHECK(!state::pending_work());
  CHECK(gnasher_fire::saved.mechanical.reload_active()&&gnasher_fire::saved.cycle.reload_cycle_pending());}
 {Fixture x;CHECK(x.begin());x.insert_rounds(2);source_current=false;
  state::service_unselected(x.f.controller.address+8,0x7000,false);CHECK(lancerreload_state::lease.acquired);
  registration_alive=false;state::service_unselected(x.f.controller.address,0x7000,false);CHECK(lancerreload_state::lease.acquired);
  registration_alive=true;++lancerreload_state::lease.serial;state::service_unselected(x.f.controller.address,0x7000,false);
  CHECK(lancerreload_state::lease.acquired&&!lancerreload_state::recoveries&&state::record(x.f)->stage==S::feeding);}
 {Fixture x;CHECK(x.begin());x.rounds();CHECK(!x.prepare());++x.f.key.frame;CHECK(x.prepare());x.accept();source_current=false;
  state::service_unselected(x.f.controller.address,0x7000,false);CHECK(!state::pending_work()&&lancerreload_state::reward_calls==1);
  state::service_unselected(x.f.controller.address,0x7000,false);CHECK(lancerreload_state::reward_calls==1);}
 {Fixture x;CHECK(x.begin());auto& mechanical=gnasher_fire::saved.mechanical;
  CHECK(mechanical.grab_round(x.sample.identity,20,x.sample.monotonic_ns).event==w::GnasherMechanicalEvent::round_grabbed);
  gnasher_round::release_ok=false;bool changed{};CHECK(!state::prepare_stow(x.f,changed)&&changed);
  CHECK(mechanical.held_round_serial()==20&&lancerreload_state::lease.acquired&&state::record(x.f)->stage==S::feeding);
  gnasher_round::release_ok=true;CHECK(state::prepare_stow(x.f,changed)&&changed);
  CHECK(!mechanical.held_round_serial()&&mechanical.hold_suspended()&&mechanical.inventory().loaded==2&&mechanical.inventory().reserve==10);}
 {Fixture x;CHECK(x.begin());auto& mechanical=gnasher_fire::saved.mechanical;
  CHECK(mechanical.grab_round(x.sample.identity,20,x.sample.monotonic_ns).event==w::GnasherMechanicalEvent::round_grabbed);x.sample.left_round_serial=20;
  for(double depth:{.07,.05,.025,.002})x.step(0,depth);
  CHECK(mechanical.inventory_request().token);bool changed{};CHECK(!state::prepare_stow(x.f,changed));
  CHECK(!gnasher_round::releases&&mechanical.held_round_serial()==20&&lancerreload_state::lease.acquired);
  state::service_unselected(x.f.controller.address,0x7000,false);CHECK(!gnasher_round::releases);}
 for(bool unselected:{false,true}){Fixture x;CHECK(x.begin());auto& mechanical=gnasher_fire::saved.mechanical;
  CHECK(mechanical.grab_round(x.sample.identity,20,x.sample.monotonic_ns).event==w::GnasherMechanicalEvent::round_grabbed);x.sample.left_round_serial=20;
  for(double depth:{.07,.05,.025,.002})x.step(0,depth);
  CHECK(mechanical.inventory_request().token);gnasher_inventory::cancel_known=true;gnasher_inventory::cancel_calls=false;
  if(unselected){source_current=false;state::service_unselected(x.f.controller.address,0x7000,false);}
  else{bool changed{};CHECK(state::prepare_stow(x.f,changed)&&changed);}
  CHECK(gnasher_inventory::cancel_calls&&!mechanical.inventory_request().token&&!mechanical.held_round_serial());
  CHECK(mechanical.hold_suspended()&&state::record(x.f)->stage==S::parked);
  CHECK(mechanical.inventory().loaded==2&&mechanical.inventory().reserve==10&&!lancerreload_state::reward_calls);
  gnasher_inventory::cancel_known=false;
 }
 {Fixture x;gnasher_animation::active=true;CHECK(state::pending_work()&&state::blocks_fire(x.f.weapon));
  state::service(x.f);CHECK(!gnasher_animation::active&&!state::pending_work()&&!state::blocks_fire(x.f.weapon));
  CHECK(!lancerreload_state::exits&&!lancerreload_state::reward_calls);}
 {Fixture x;gnasher_animation::active=true;source_current=false;gnasher_animation::service_ok=false;
  state::service_unselected(x.f.controller.address,0x7000,true);CHECK(state::pending_work()&&gnasher_animation::services==1);
  gnasher_animation::service_ok=true;state::service_unselected(x.f.controller.address,0x7000,true);
  CHECK(!state::pending_work()&&!lancerreload_state::exits&&!lancerreload_state::reward_calls);}
 {Fixture x;gnasher_animation::active=true;source_current=false;
  state::service_unselected(x.f.controller.address+8,0x7000,true);CHECK(state::pending_work()&&!gnasher_animation::services);
  registration_alive=false;state::service_unselected(x.f.controller.address,0x7000,true);CHECK(state::pending_work());
  registration_alive=true;inventory_pending=true;state::service_unselected(x.f.controller.address,0x7000,true);CHECK(state::pending_work());
  inventory_pending=false;state::service_unselected(x.f.controller.address,0x7000,true);CHECK(!state::pending_work());}
 {Fixture x;gnasher_animation::active=true;
  lancerreload_state::lease.identity=gnasher_fire::saved.identity;lancerreload_state::lease.acquired=true;
  state::service(x.f);CHECK(state::pending_work());
  state::service_unselected(x.f.controller.address,0x7000,true);CHECK(state::pending_work());
  lancerreload_state::lease.acquired=false;state::service_unselected(x.f.controller.address,0x7000,true);
  CHECK(!state::pending_work()&&!lancerreload_state::reward_calls&&!lancerreload_state::exits);}
 std::puts("Production Gnasher original-state adapter: fresh ammo grant, journal-before-dispatch, unknown no replay, original clock, non-filling exit, new-frame paired cycle ACK, ordinary partial closure and once-only reward plus owned-animation stop/unload retention passed (injected engine boundary)");
}
