#include "../vr-common/weapon_gnasher_cycle_commit.h"
#include "../vr-common/weapon_shot_scope.h"
#include "../vr-common/weapon_call_boundary.h"
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <limits>
#define CHECK(v) do{if(!(v)){std::fprintf(stderr,"line %u: %s\n",unsigned(__LINE__),#v);std::exit(1);}}while(false)
using uptr=std::uintptr_t;
namespace h=gears_weapon_hand;namespace s=gears_weapon_shot;
FILE* log_file{};bool fresh=true,sample_ok=true,clock_ok=true,manual=true;std::int64_t observed_now=1000000100;
float game_seconds=3.5f;
unsigned name_reads{},guard_checks{};
unsigned engine_reads{};unsigned loaded=8,reserve=10;std::uint64_t revision=1,hold=1;
bool model_held=true;
constexpr uptr custom=0x1010,pellets=0x1020,gun=0x5000;
h::ObjectIdentity object(uptr p,uptr outer=0x8000,uptr cls=0x9000){return {p,outer,cls,unsigned(p/8),7,0,1};}
namespace weaponhand {unsigned current_gt(void*) noexcept{return GetCurrentThreadId();} std::int64_t clock_ns(void*) noexcept{return observed_now;}}
namespace nativeequipment {bool current(void*,h::Frame const&) noexcept{return fresh;}}
namespace vrequip {struct Model {bool held_for(uptr,uptr,uptr)const noexcept{return model_held;} std::uint64_t hold_generation()const noexcept{return hold;}} model;}
namespace unified_weapon_fire {bool manual_profile() noexcept{return manual;}}
namespace playground {
bool instance(uptr p,wchar_t const* text) noexcept{return p==gun&&!std::wcscmp(text,L"Weap_Shotgun");}
bool named(uptr p,wchar_t const* text) noexcept{++name_reads;return p==custom&&!std::wcscmp(text,L"CustomFire");}
}
namespace lancerreload_state {bool time_field(uptr,wchar_t const*,float& out) noexcept{++engine_reads;out=game_seconds;return clock_ok;}}
namespace lancerreload_inventory {
#include "weapon_gnasher_inventory_identity_test.inl"
struct Inventory {std::uint64_t revision{};unsigned loaded{},reserve{};};
struct ObservedInventory {
 bool sample(h::Frame const&,Inventory& out) noexcept{++engine_reads;out={revision,loaded,reserve};return sample_ok&&fresh;}
 bool sample_owned(Identity const&,Inventory& out) noexcept{++engine_reads;out={revision,loaded,reserve};return sample_ok;}
 bool rebind_hold(h::Frame const& f,Identity const&,Inventory& out) noexcept{return sample(f,out);}
};
}
namespace weaponshot {s::Stack stack;}
namespace gnasher_inventory {bool blocks_fire(h::ObjectIdentity const&) noexcept{return false;}}
namespace gnasher_state {bool blocks_fire(h::ObjectIdentity const&) noexcept{return false;}}
#if defined(GEARS_TEST_ORIGINAL)
#include "J:/codexWorktree/gearsVr/analysis/lancer/build/gnasher-empty-token-components/weapon_gnasher_fire_receipt.original.inl"
#else
#include "weapon_gnasher_fire_receipt.inl"
#endif
s::Binding binding(s::Role role,uptr fn){
 s::Binding b{};b.role=role;b.script=0x10000;b.bytes=b.capacity=128;b.script_hash=1;b.exec=0x20000;
 b.identities={object(fn,0x2000,0x9000),object(0x2000,0x3000,0xa000),object(0x3000,0,0xb000),object(0x9000,0xc000,0xa000),object(0xa000,0xc000,0xa000)};
 CHECK(b.valid());return b;
}
h::Frame frame(){
 h::Frame f{};f.key={11,22,33,1,1,1,44,55};f.world=object(0x6000);f.pawn=object(0x6020);f.controller=object(0x6010);f.mesh=object(0x6030);f.weapon=object(gun);
 f.stereo.session=4;f.stereo.space_epoch=5;f.controllers.input_epoch=6;f.controllers.profile_generation=7;f.reference_generation=8;
 f.controllers.sequence=1;f.controllers.sync_ns=1000000000;return f;
}
h::GripSnapshot grip(h::Frame const& f){
 h::GripSnapshot g{};g.grip=h::Grip::single;g.key=f.key;g.world=f.world;g.pawn=f.pawn;g.controller=f.controller;g.mesh=f.mesh;g.weapon=f.weapon;
 g.input_session=f.stereo.session;g.space_epoch=f.stereo.space_epoch;g.input_epoch=f.controllers.input_epoch;g.profile_generation=f.controllers.profile_generation;g.reference_generation=f.reference_generation;
 g.input_sequence=f.controllers.sequence;g.press_token=f.press_token;
 g.committed_ns=1000000000;g.input_received_ns=g.stereo_received_ns=990000000;return g;
}
void check_empty_token(gears_weapon_call::Invocation const& call){
 const auto before=gnasher_fire::entries;const auto names=name_reads,reads=engine_reads;
 const s::Token empty{};
 gnasher_fire::begin(call,empty);gnasher_fire::returned(call,empty);
 CHECK(std::memcmp(before.data(),gnasher_fire::entries.data(),sizeof(before))==0);
 CHECK(engine_reads==reads);
#if defined(GEARS_TEST_ORIGINAL)
 CHECK(name_reads==names+2);
#else
 CHECK(name_reads==names);
#endif
 ++guard_checks;
}
void protect_original_responsibilities(){
 const auto reset_cold=[](){gnasher_fire::entries={};hold=0;model_held=false;auto value=frame();gnasher_fire::service(value);return value;};
 {
  auto value=reset_cold();auto* old=gnasher_fire::entry(value,false);CHECK(old);old->accepted_call=77;
  hold=1;model_held=true;auto* next=gnasher_fire::entry(value,true);
  CHECK(next!=old&&old->accepted_call==77&&old->identity.hold_generation==0&&gnasher_fire::blocks_fire(value.weapon));
 }
 {
  auto value=reset_cold();auto* old=gnasher_fire::entry(value,false);CHECK(old);
  CHECK(old->cycle.bind(gnasher_fire::counts(value,{1,8,10})));
  hold=1;model_held=true;auto* next=gnasher_fire::entry(value,true);
  CHECK(next!=old&&old->cycle.inventory().revision==1&&gnasher_fire::blocks_fire(value.weapon));
 }
 {
  auto value=reset_cold();auto* old=gnasher_fire::entry(value,false);CHECK(old);
  const auto cookie=old->identity.registration;++value.key.registration_cookie;hold=1;model_held=true;
  CHECK(gnasher_fire::entry(value,true)!=old&&old->identity.registration==cookie&&old->identity.hold_generation==0);
 }
 {
  gnasher_fire::entries={};hold=2;model_held=false;auto value=frame();gnasher_fire::service(value);
  auto* old=gnasher_fire::entry(value,false);CHECK(old);hold=1;model_held=true;
  CHECK(gnasher_fire::entry(value,true)!=old&&old->identity.hold_generation==2);
 }
 gnasher_fire::entries={};hold=1;model_held=true;
}
int main(){
 protect_original_responsibilities();
 {gears_weapon_call::Invocation empty_call{};empty_call.self=gun;empty_call.callee=custom;
  empty_call.thread_id=GetCurrentThreadId();empty_call.hand_cookie=11;empty_call.native_started_qpc=22;empty_call.call_id=77;
  check_empty_token(empty_call);
  const auto unknown=weaponshot::stack.mask();CHECK(unknown.entered&&!unknown.previous_depth);
  const auto before=gnasher_fire::entries;const auto names=name_reads;
  gnasher_fire::begin(empty_call,unknown);gnasher_fire::returned(empty_call,unknown);
  CHECK(name_reads==names+2&&std::memcmp(before.data(),gnasher_fire::entries.data(),sizeof(before))==0);
  weaponshot::stack.leave(unknown);++guard_checks;
 }
 // Real inventory Identity rejects hold0; the former weakened stub hid an
 // extra cold entry that vetoed this actor after a real hold had calibrated.
 auto f=frame();hold=0;model_held=false;const auto initial_reads=engine_reads;
 gnasher_fire::service(f);auto* e=gnasher_fire::entry(f,false);
 CHECK(e&&!e->prepared&&e->preparation_stage==15&&engine_reads==initial_reads);
 CHECK(!e->identity.valid());auto* cold=e;
 CHECK(gnasher_fire::blocks_fire(f.weapon)); // Delayed observation must not remove the unbound-gun veto.
 hold=1;gnasher_fire::service(f);unsigned known{};
 for(auto const& slot:gnasher_fire::entries)known+=unsigned(slot.known);
 CHECK(known==1&&cold->identity.hold_generation==0&&engine_reads==initial_reads);
 hold=2;model_held=true;++f.key.lifecycle;gnasher_fire::service(f);e=gnasher_fire::entry(f,false);
 CHECK(e==cold&&e->identity.valid()&&e->identity.hold_generation==hold&&e->identity.lifecycle==f.key.lifecycle);
 known=0;for(auto const& slot:gnasher_fire::entries)known+=unsigned(slot.known);CHECK(known==1);
 CHECK(engine_reads>initial_reads);
 CHECK(e&&!e->prepared&&!e->cycle.blocked()&&gnasher_fire::blocks_fire(f.weapon));
 // Initial loaded pickup needs calibration, never a fake empty cycle. Bind
 // a synthetic physical controller before exercising the actual receipt inl.
 gears_weapon::GnasherMechanicalConfig initial_config{};initial_config.calibration_id=1;initial_config.lever_calibrated=true;
 initial_config.capacity=8;initial_config.game_ticks_per_second=1000000;
 initial_config.lever.left_front_grip_in_main.position={0,0,.08};initial_config.lever.right_handle_home_in_main={0,-.2,0};initial_config.lever.hinge_axis_in_main={1,0,0};
 const auto initial_id=gnasher_fire::mechanical_identity(f);CHECK(e->mechanical.bind(initial_id,initial_config,{initial_id,1,8,10}));
 gears_weapon::GnasherMechanicalSample initial{};initial.identity=initial_id;initial.sequence=f.controllers.sequence;
 initial.monotonic_ns=f.controllers.sync_ns;initial.game_tick=3500000;initial.left_tracked=initial.right_tracked=initial.right_handle_held=initial.original_ready=true;
 initial.left_grip.position={0,0,.08};initial.right_grip.position={0,-.2,0};
 CHECK(e->mechanical.observe(initial).event==gears_weapon::GnasherMechanicalEvent::none);
 gnasher_fire::service(f);CHECK(e->prepared&&!gnasher_fire::blocks_fire(f.weapon));
 {
  // A new GT source arrives before raw mechanics. No first-shot admission
  // can use the previous raw sample, so this call must not read inventory.
  const auto saved=*e;const auto saved_now=observed_now;
  auto next=f;next.left_grip_known=1;next.left_grip_down=0;
  ++next.controllers.sequence;next.controllers.sync_ns+=20000000;
  observed_now=next.controllers.sync_ns+100;
  const auto before=engine_reads;gnasher_fire::service(next);
  CHECK(!e->prepared&&e->preparation_stage==9&&engine_reads==before);
  CHECK(!e->cycle.blocked()&&!e->mechanical.mechanism_pending());
  auto current=initial;current.sequence=next.controllers.sequence;current.monotonic_ns=next.controllers.sync_ns;
  CHECK(e->mechanical.observe(current).event==gears_weapon::GnasherMechanicalEvent::none);
  // Actual counts can change during that interval: the later valid-source
  // call must perform its fresh read and reject the changed loaded count.
  --loaded;++revision;gnasher_fire::service(next);
  CHECK(engine_reads>before&&!e->prepared&&e->preparation_stage==10);
  CHECK(e->mechanical.inventory().loaded==8&&!e->cycle.blocked());
  ++loaded;--revision;*e=saved;observed_now=saved_now;
 }
 for(float bad:{-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){
  game_seconds=bad;gnasher_fire::service(f);CHECK(!e->prepared&&!e->cycle.blocked());
  game_seconds=3.5f;gnasher_fire::service(f);CHECK(e->prepared);
 }
 sample_ok=false;gnasher_fire::service(f);CHECK(!e->prepared);sample_ok=true;gnasher_fire::service(f);CHECK(e->prepared);
 const auto b=binding(s::Role::shotgun_custom,custom);
 const auto token=weaponshot::stack.enter(b,f.weapon,0x1234,grip(f),1000000001,[](void*,void const*,std::size_t){return false;});
 gears_weapon_call::Invocation call{};call.call_id=1;call.self=gun;call.callee=custom;call.hand_cookie=f.key.registration_cookie;call.native_started_qpc=f.key.native_started_qpc;call.thread_id=GetCurrentThreadId();
 check_empty_token(call); // Even while a genuine root is active, an empty token cannot borrow it.
 {
  const auto before=gnasher_fire::entries;const auto names=name_reads;
  auto foreign=call;foreign.self=0xdead;
  gnasher_fire::begin(foreign,token);gnasher_fire::returned(foreign,token);
  CHECK(name_reads==names+2&&std::memcmp(before.data(),gnasher_fire::entries.data(),sizeof(before))==0);
  const auto nested=weaponshot::stack.mask();CHECK(nested.entered&&nested.previous_depth==1);
  const auto nested_names=name_reads;
  gnasher_fire::begin(call,nested);gnasher_fire::returned(call,nested);
  CHECK(name_reads==nested_names&&std::memcmp(before.data(),gnasher_fire::entries.data(),sizeof(before))==0);
  weaponshot::stack.leave(nested);++guard_checks;
 }
 auto bad=call;++bad.hand_cookie;const auto reads=engine_reads;
 gnasher_fire::returned(bad,token);CHECK(!e->cycle.blocked());
 gnasher_fire::begin(call,token);CHECK(e->admitted_shot.accepted&&engine_reads==reads);
 // Each bad-source fixture must deny automatic refire, but cannot invent a
 // confirmed shot. Restore the owned synthetic fixture to test the valid path.
 const auto healthy=*e;auto changed=f;++changed.controllers.profile_generation;e->source=changed;
 gnasher_fire::returned(call,token);CHECK(e->cycle.blocked()&&e->cycle.faulted()&&!e->cycle.confirmed());
 *e=healthy;
 gnasher_fire::abandoned(call,token);CHECK(e->cycle.blocked()&&e->cycle.faulted()&&!e->cycle.confirmed());
 *e=healthy;
 auto wrong_token=token;++wrong_token.call;gnasher_fire::returned(call,wrong_token);CHECK(!e->cycle.blocked());
 gnasher_fire::returned(call,token);CHECK(e->cycle.blocked()&&e->cycle.pending());CHECK(engine_reads==reads); // Subscriber performs no engine reads/calls.
 gnasher_fire::returned(call,token);CHECK(!e->cycle.faulted());weaponshot::stack.leave(token);
 model_held=false;const auto pending_reads=engine_reads;
 gnasher_fire::service(f);CHECK(e->cycle.pending()&&!e->cycle.confirmed());CHECK(engine_reads>pending_reads);
 --loaded;++revision;gnasher_fire::service(f);CHECK(e->cycle.confirmed()&&!e->cycle.pending());CHECK(e->cycle.receipt().observed.loaded==7);
 model_held=true;
 CHECK(gnasher_fire::blocks_fire(f.weapon));CHECK(!gnasher_fire::blocks_fire(object(0x5100)));
 const auto receipt=e->cycle.receipt();CHECK(receipt.game_tick==3500000&&receipt.original_event_sequence>0);
 ++hold;gnasher_fire::service(f);CHECK(!e->prepared&&e->cycle.blocked());CHECK(gnasher_fire::entry(f,false)==e);CHECK(e->identity.hold_generation==hold);
 // Arbitrarily late normal ticks and a changed held weapon never clear the debt.
 for(unsigned i=0;i<8;++i)gnasher_fire::service(f);CHECK(e->cycle.blocked());
 auto other=f;other.weapon=object(0x5100);gnasher_fire::service(other);CHECK(gnasher_fire::blocks_fire(f.weapon)&&!gnasher_fire::blocks_fire(other.weapon));
 {
  // A genuine normal-return receipt may arrive well after the raw input's
  // action lifetime. Preserve its independently witnessed original admission;
  // do not renew that raw sample or lose the real -1 count/debt.
  gnasher_fire::entries={};hold=1;loaded=8;reserve=10;revision=1;observed_now=1000000100;
  auto live=frame();live.controllers.sequence=1;live.controllers.sync_ns=1000000000;
  gnasher_fire::service(live);auto* active=gnasher_fire::entry(live,false);CHECK(active);
  gears_weapon::GnasherMechanicalConfig config{};config.calibration_id=1;config.lever_calibrated=true;
  config.capacity=8;config.game_ticks_per_second=1000000;config.lever.left_front_grip_in_main.position={0,0,.08};
  config.lever.right_handle_home_in_main={0,-.2,0};config.lever.hinge_axis_in_main={1,0,0};
  const auto id=gnasher_fire::mechanical_identity(live);CHECK(active->mechanical.bind(id,config,{id,1,8,10}));
  gears_weapon::GnasherMechanicalSample sample{};sample.identity=id;sample.sequence=1;sample.monotonic_ns=live.controllers.sync_ns;
  sample.game_tick=3500000;sample.left_tracked=sample.right_tracked=sample.right_handle_held=sample.original_ready=true;
  sample.left_grip.position={0,0,.08};sample.right_grip.position={0,-.2,0};
  CHECK(active->mechanical.observe(sample).event==gears_weapon::GnasherMechanicalEvent::none);
  gnasher_fire::service(live);CHECK(active->prepared);
  const auto root=weaponshot::stack.enter(b,live.weapon,0x1234,grip(live),1000000001,[](void*,void const*,std::size_t){return false;});
  auto invocation=call;invocation.call_id=99;const auto before_reads=engine_reads;
  gnasher_fire::begin(invocation,root);CHECK(active->admitted_shot.accepted&&engine_reads==before_reads);
  observed_now+=2000000000;gnasher_fire::returned(invocation,root);CHECK(engine_reads==before_reads);
  weaponshot::stack.leave(root);--loaded;++revision;gnasher_fire::service(live);
  CHECK(active->cycle.confirmed()&&active->mechanical_shot==active->cycle.receipt().original_event_sequence);
  CHECK(active->mechanical.inventory().loaded==7&&active->mechanical.mechanism_pending());
  CHECK(active->cycle.receipt().monotonic_ns==observed_now&&!active->mechanical.can_request_fire(observed_now));
  gnasher_fire::service(live);CHECK(active->mechanical.inventory().loaded==7&&active->cycle.blocked());
  // Continue the SAME actual adapter entry through a synthetic physical
  // stroke and independently supplied original bone/readiness observations.
  // The fixture is not Gnasher geometry or a runtime/controller acceptance.
  const auto move=[&](double angle){
   observed_now+=20000000;++sample.sequence;sample.monotonic_ns=observed_now;
   game_seconds+=.02f;sample.game_tick=static_cast<std::int64_t>(double(game_seconds)*1000000.);
   sample.left_front_held=true;sample.support_grasp_token=1;
   sample.right_grip.position={0,-.2*std::cos(angle),-.2*std::sin(angle)};
   sample.right_grip.orientation={std::sin(angle*.5),0,0,std::cos(angle*.5)};
   live.controllers.sequence=sample.sequence;live.controllers.sync_ns=sample.monotonic_ns;
   return active->mechanical.observe(sample);
  };
  CHECK(move(0).event==gears_weapon::GnasherMechanicalEvent::rejected); // Two-second receipt gap does not count as continuous movement.
  CHECK(move(0).event==gears_weapon::GnasherMechanicalEvent::restored);
  gears_weapon::GnasherMechanicalDecision physical{};
  for(double angle:{0.,.1,.2,.34,.48,.565,.4,.2,.1,0.}){
   physical=move(angle);CHECK(physical.drive_lever);
   CHECK(gnasher_fire::blocks_fire(live.weapon));gnasher_fire::service(live);CHECK(!active->prepared);
  }
  CHECK(physical.event==gears_weapon::GnasherMechanicalEvent::request_original_cycle);
  const auto physical_request=active->mechanical.cycle_request();
  gears_weapon::GnasherClosedLeverReference closed{101,{{0,0,0},{}}};
  gears_weapon::GnasherCycleReadbackGate actual_bones;
  CHECK(actual_bones.begin(physical_request,closed,observed_now));
  gears_weapon::GnasherCycleReadback readback{id,physical_request.token,closed.calibration_id,
   sample.sequence,1,observed_now,{{0,0,0},{std::sin(.03),0,0,std::cos(.03)}},true,true};
  gears_weapon::GnasherCycleReceipt closure{};
  CHECK(!actual_bones.observe(readback,observed_now,closure)); // Native ready alone cannot close a visibly open lever.
  readback.lever_in_main=closed.in_main;readback.original_ready=false;++readback.original_observation_sequence;
  CHECK(!actual_bones.observe(readback,observed_now,closure));
  CHECK(active->mechanical.mechanism_pending()&&active->cycle.blocked());
  // The later original readiness receipt authenticates its CURRENT source;
  // it never changes the physical completion timestamp or replays that move.
  observed_now+=250000000;readback.observed_ns=observed_now;++readback.input_sequence;
  readback.original_ready=true;++readback.original_observation_sequence;
  CHECK(actual_bones.observe(readback,observed_now,closure));
  CHECK(gears_weapon::gnasher_commit_closed_cycle(active->mechanical,active->cycle,closure,
      gnasher_fire::counts(live,{revision,loaded,reserve})));
  CHECK(!active->cycle.blocked()&&!active->mechanical.mechanism_pending());
  gnasher_fire::service(live);CHECK(!active->prepared); // Closed receipt cannot renew a raw source.
  CHECK(move(0).event==gears_weapon::GnasherMechanicalEvent::rejected); // Independent readiness delay also breaks tracking continuity.
  CHECK(move(0).event==gears_weapon::GnasherMechanicalEvent::restored);
  gnasher_fire::service(live);CHECK(active->prepared&&!gnasher_fire::blocks_fire(live.weapon));
  auto second_grip=grip(live);second_grip.committed_ns=observed_now;
  second_grip.stereo_received_ns=second_grip.input_received_ns=sample.monotonic_ns;
  const auto second=weaponshot::stack.enter(b,live.weapon,0x1234,second_grip,observed_now,[](void*,void const*,std::size_t){return false;});
  invocation.call_id=100;const auto second_reads=engine_reads;const auto first_shot=active->mechanical_shot;
  gnasher_fire::begin(invocation,second);gnasher_fire::returned(invocation,second);
  CHECK(engine_reads==second_reads);weaponshot::stack.leave(second);
  --loaded;++revision;gnasher_fire::service(live);
  CHECK(active->cycle.confirmed()&&active->mechanical_shot>first_shot);
  CHECK(active->mechanical.inventory().loaded==6&&active->mechanical.inventory().reserve==10);
  CHECK(active->mechanical.mechanism_pending()&&gnasher_fire::blocks_fire(live.weapon));
 }
 std::printf("Token guard directed groups=%u name_reads=%u engine_reads=%u original=%u\n",guard_checks,name_reads,engine_reads,
#if defined(GEARS_TEST_ORIGINAL)
1u
#else
0u
#endif
);
 std::puts("Production Gnasher CustomFire receipt: witnessed admission, delayed return -1, blocked refire, physical down/return plus independent actual endpoint/readiness, fresh-source recovery and second original shot passed; synthetic engine/geometry boundaries only");
}
