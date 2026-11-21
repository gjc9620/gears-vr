#include "../vr-common/weapon_gnasher_cycle_commit.h"
#include "../vr-common/weapon_gnasher_round_pose.h"
#include "../vr-common/weapon_hand_participant.h"
#include "../vr-common/body_slots.h"
#include <cstdio>
#include <cstdlib>
#ifdef NDEBUG
#error GnasherReloadDispatchTest requires active checks
#endif
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"line %u: %s\n",unsigned(__LINE__),#x);std::abort();}}while(false)
namespace hand=gears_weapon_hand;namespace weapon=gears_weapon;namespace vr=gears_vr;
namespace inv {
struct Identity {hand::ObjectIdentity world{},pawn{},weapon{};std::uint64_t generation{};
 bool operator==(Identity const& b)const{return hand::same(world,b.world)&&hand::same(pawn,b.pawn)&&hand::same(weapon,b.weapon)&&generation==b.generation;}};
Identity identity(hand::Frame const& f){return {f.world,f.pawn,f.weapon,f.key.world_generation};}
}
std::int64_t now=1000000001,game_now=1000000;unsigned begins{},captures{},shows{},withdrawals{},writes{},rejects{},empty_exits{},clip_releases{};
bool source_ok=true,capture_ok=true,show_ok=true,hide_ok=true,lease_ok=true,clip_ok=true,inventory_pending=false;bool close_admitted=true,close_exit_ok=true;
namespace nativeequipment {weapon::ArmspanCalibration calibration=weapon::default_body_slot_layout();bool current(void*,hand::Frame const&){return source_ok;}}
namespace weaponhand {std::int64_t clock_ns(void*){return now;}}
namespace gnasher_fire {
struct Entry {inv::Identity identity{};weapon::GnasherMechanicalInteraction mechanical{};weapon::GnasherFireCycle cycle{};}entry_value;
Entry* entry(hand::Frame const&,bool){return &entry_value;}
}
namespace lancerreload_state {bool owns(hand::Frame const&,std::uint64_t s){return lease_ok&&s==9;}}
namespace gnasher_state {
enum class Stage {idle,feeding,recovery_required,empty_exiting,empty_settlement};struct Record {inv::Identity identity{};std::uint64_t serial{};Stage stage{};}value;
Record* record(hand::Frame const&){return &value;}
bool fresh_part_grant(hand::Frame const& f,hand::Decision const& d,hand::LeaseKind kind){
 return d.new_grant&&d.status==hand::DecisionStatus::granted&&!d.release_required&&hand::same(d.key,f.key)&&d.press_token==f.press_token&&
  d.lease.kind==kind&&d.lease.id==f.press_token&&hand::lease_valid(d.lease,f.key.lifecycle)&&f.left_tracked&&f.left_grip_known&&f.left_grip_down;
}
bool fresh_ammo_grant(hand::Frame const& f,hand::Decision const& d){return fresh_part_grant(f,d,hand::LeaseKind::ammo);}
bool begin(hand::Frame const& f,hand::Decision const&){
 ++begins;auto& m=gnasher_fire::entry_value.mechanical;
 const weapon::GnasherReloadAcceptance acceptance{m.inventory().identity,f.controllers.sequence,f.controllers.sync_ns,now,game_now,m.inventory(),true};
 if(m.accept_original_reload(acceptance).event!=weapon::GnasherMechanicalEvent::reload_started)return false;
 value={inv::identity(f),9,Stage::feeding};return true;
}
bool close_empty(hand::Frame const&,hand::Decision const&){++empty_exits;if(!close_admitted)return false;value.stage=close_exit_ok?Stage::empty_settlement:Stage::empty_exiting;return close_exit_ok;}
}
namespace gnasher_animation {struct Owner{bool active{};}owner;
bool release_after_exit(hand::Frame const&,std::uint64_t){++clip_releases;if(!clip_ok)return false;owner.active=false;return true;}}
namespace gnasher_round {
std::uint64_t item{};bool captured_for(hand::Frame const& f,std::uint64_t i){return capture_ok&&source_ok&&item==i&&f.press_token==i&&f.left_tracked&&f.left_grip_down;}
bool retains_ammo_route(hand::Frame const& f){return item&&item==f.press_token&&f.left_grip_down;}
bool release_for_stow(inv::Identity const& id,std::uint64_t i){++withdrawals;CHECK(id==gnasher_fire::entry_value.identity&&i==item);if(!hide_ok)return false;item=0;return true;}
bool capture(hand::Frame const&,hand::Decision const&,std::uint64_t i,gears_gnasher_round_pose::Mount const&,weapon::GnasherMechanicalConfig const&){
 ++captures;if(!capture_ok)return false;CHECK(!item);item=i;return true;}
bool show(hand::Frame const&,hand::Decision const&){++shows;return show_ok;}
}
namespace gnasher_inventory {
enum class Result {rejected,committed,reconciliation_required,consumer_pending};
bool blocks_fire(hand::ObjectIdentity const&){return inventory_pending;}
bool retire_settled(hand::Frame const&){return false;}
Result reject_undispatched_round(hand::Frame const&){++rejects;auto& e=gnasher_fire::entry_value;
 const auto q=e.mechanical.inventory_request();const weapon::GnasherInventoryReceipt receipt{q.token,false,e.mechanical.inventory()};
 CHECK(weapon::gnasher_reject_inserted_round(e.mechanical,e.cycle,receipt));return Result::rejected;}
Result commit_round(hand::Frame const&,std::uint64_t serial,bool(*)(hand::Frame const&,std::uint64_t)){
 CHECK(serial==9);++writes;auto& e=gnasher_fire::entry_value;const auto q=e.mechanical.inventory_request();
 auto after=e.mechanical.inventory();++after.revision;++after.loaded;--after.reserve;
 CHECK(weapon::gnasher_commit_inserted_round(e.mechanical,e.cycle,{q.token,true,after}));
 CHECK(gnasher_round::release_for_stow(e.identity,q.round_serial));return Result::committed;}
}
namespace gnasher_runtime {
struct Geometry {weapon::GnasherMechanicalConfig mechanical{};gears_gnasher_round_pose::Mount round{};std::uint64_t feed_evidence{};};
struct Profile {Geometry geometry{};bool known{};}profile;
std::uint64_t animation_serial{};bool curve_known{},down_seen{};
#include "weapon_gnasher_reload_dispatch.inl"
}
hand::ObjectIdentity object(std::uint64_t a){return {a,0x8000,0x9000,unsigned(a/8),7,0,1};}
struct Fixture {
 hand::Frame f{};hand::Decision d{};weapon::GnasherMechanicalSample s{};
 Fixture(unsigned loaded=2,unsigned reserve=10){
  begins=captures=shows=withdrawals=writes=rejects=empty_exits=clip_releases=0;
  source_ok=capture_ok=show_ok=hide_ok=lease_ok=clip_ok=close_admitted=close_exit_ok=true;inventory_pending=false;gnasher_round::item=0;
  gnasher_animation::owner={};gnasher_state::value={};gnasher_fire::entry_value={};gnasher_runtime::profile={};
  f.key={11,22,33,1,10,1,44,55};f.world=object(0x6000);f.pawn=object(0x6020);f.weapon=object(0x5000);
  f.stereo.head.position={0,1.7,0};f.controllers.hands[0].grip.position={-.2295,1.585,.05};
  f.press_token=1;f.left_tracked=f.left_grip_known=f.left_grip_down=true;f.reference_generation=8;
  auto& p=gnasher_runtime::profile;p.known=true;p.geometry.feed_evidence=77;
  auto& c=p.geometry.mechanical;c.calibration_id=9;c.lever_calibrated=c.feed_calibrated=true;c.capacity=8;c.game_ticks_per_second=1000000;
  c.lever.left_front_grip_in_main.position={0,0,.08};c.lever.right_handle_home_in_main={0,-.2,0};c.lever.hinge_axis_in_main={1,0,0};
  p.geometry.round={9,8,{{0,-gears_gnasher_round_pose::authored_length_metres,0},{}}};c.feed.item_in_left_grip=p.geometry.round.tip_in_grip_metres();
  const weapon::GnasherIdentity id{f.world.address,f.pawn.address,f.weapon.address,1,4,5,6,7};
  auto& e=gnasher_fire::entry_value;e.identity=inv::identity(f);CHECK(e.mechanical.bind(id,c,{id,1,loaded,reserve}));CHECK(e.cycle.bind(e.mechanical.inventory()));
  s.identity=id;s.monotonic_ns=1000000000;s.game_tick=1000000;s.left_tracked=s.right_tracked=s.right_handle_held=s.original_ready=true;
  s.right_grip.position={0,-.2,0};s.reference_generation=8;step(.07);grant(hand::LeaseKind::ammo);
  CHECK(c.valid()&&p.geometry.round.matches_feed(c));CHECK(gnasher_state::fresh_ammo_grant(f,d));
 }
 void grant(hand::LeaseKind kind,bool fresh=true){d={};d.key=f.key;d.press_token=f.press_token;d.lease={kind,0,f.key.lifecycle,f.press_token};d.status=fresh?hand::DecisionStatus::granted:hand::DecisionStatus::retained;d.new_grant=fresh;d.release_required=0;}
 weapon::GnasherMechanicalDecision step(double depth){s.left_grip.position={0,0,depth};++s.sequence;s.monotonic_ns+=20000000;s.game_tick+=20000;
  f.controllers.sequence=s.sequence;f.controllers.sync_ns=s.monotonic_ns;now=s.monotonic_ns+1;game_now=s.game_tick;++f.key.frame;
  return gnasher_fire::entry_value.mechanical.observe(s);}
 bool dispatch(){return gnasher_runtime::reload::dispatch(f,d,gnasher_fire::entry_value);}
 void begin(){CHECK(dispatch());CHECK(begins==1&&captures==1&&gnasher_round::item==1);s.left_round_serial=1;}
 void insert(){for(double depth:{.07,.05,.025,.002}){step(depth);grant(hand::LeaseKind::ammo,false);dispatch();}}
};
int main(){
 {Fixture x;gnasher_runtime::profile.geometry.feed_evidence=0;CHECK(!x.dispatch()&&!begins&&!captures);CHECK(!gnasher_runtime::reload::requests_ammo(x.f));}
 {Fixture x;CHECK(gnasher_runtime::reload::requests_ammo(x.f));x.f.controllers.hands[0].grip.position={};CHECK(x.dispatch()&&!begins&&!captures);}
 {Fixture x;x.begin();auto& m=gnasher_fire::entry_value.mechanical;const auto started=m.reload_started_game_tick();CHECK(started==x.s.game_tick&&m.target()==3);
  x.insert();CHECK(writes==1&&m.inventory().loaded==3&&m.inventory().reserve==9&&!m.held_round_serial());
  for(unsigned i=0;i<3;++i){x.step(.002);x.grant(hand::LeaseKind::ammo,false);x.dispatch();}CHECK(writes==1&&captures==1&&begins==1);
  for(unsigned round=2;round<=3;++round){x.f.press_token=round;x.s.left_round_serial=0;x.step(.07);x.grant(hand::LeaseKind::ammo);CHECK(x.dispatch());
   x.s.left_round_serial=round;x.insert();CHECK(m.reload_started_game_tick()==started&&m.target()==3);}
  CHECK(writes==3&&begins==1&&captures==3&&m.inventory().loaded==5&&m.inventory().reserve==7&&m.inserted()==3);
  CHECK(!gnasher_runtime::reload::requests_ammo(x.f));}
 {Fixture x;x.begin();x.insert();auto& m=gnasher_fire::entry_value.mechanical;
  x.s.left_round_serial=0;++x.f.press_token;x.step(.07);x.grant(hand::LeaseKind::support);CHECK(x.dispatch());
  CHECK(m.stage()==weapon::GnasherMechanicalStage::lever&&m.target()==3&&m.inserted()==1&&m.inventory().loaded==3&&begins==1&&!empty_exits);}
 {Fixture x;x.begin();auto& m=gnasher_fire::entry_value.mechanical;x.f.left_grip_down=false;
  hide_ok=false;CHECK(gnasher_runtime::reload::withdraw_unused(x.f,gnasher_fire::entry_value)&&m.held_round_serial()==1);
  hide_ok=true;CHECK(gnasher_runtime::reload::withdraw_unused(x.f,gnasher_fire::entry_value)&&!m.held_round_serial());
  CHECK(m.inventory().loaded==2&&m.inventory().reserve==10&&!writes);
  x.f.left_grip_down=true;x.s.left_round_serial=0;++x.f.press_token;x.step(.07);x.grant(hand::LeaseKind::support);CHECK(x.dispatch());CHECK(empty_exits==1&&!m.reload_active());}
 {Fixture x;x.begin();auto& m=gnasher_fire::entry_value.mechanical;
  x.f.left_grip_down=false;CHECK(gnasher_runtime::reload::withdraw_unused(x.f,gnasher_fire::entry_value));
  x.f.left_grip_down=true;x.s.left_round_serial=0;++x.f.press_token;x.step(.07);x.grant(hand::LeaseKind::support);
  const auto started=m.reload_started_game_tick();close_admitted=false;CHECK(x.dispatch());
  CHECK(m.reload_active()&&m.reload_started_game_tick()==started&&gnasher_state::value.stage==gnasher_state::Stage::feeding&&!writes);
  close_admitted=true;close_exit_ok=false;++x.f.press_token;x.step(.07);x.grant(hand::LeaseKind::support);CHECK(x.dispatch());
  CHECK(!m.reload_active()&&gnasher_state::value.stage==gnasher_state::Stage::empty_exiting&&empty_exits==2&&!writes);
 }
 {Fixture x;gnasher_animation::owner.active=true;clip_ok=false;CHECK(x.dispatch()&&!begins&&!captures&&clip_releases==1);}
 {Fixture x;capture_ok=false;CHECK(x.dispatch()&&begins==1&&!gnasher_fire::entry_value.mechanical.held_round_serial()&&!writes);}
 {Fixture x;x.begin();show_ok=false;for(double depth:{.07,.05,.025,.002}){x.step(depth);x.grant(hand::LeaseKind::ammo,false);x.dispatch();}
  CHECK(!writes&&gnasher_fire::entry_value.mechanical.inventory_request().token);
  x.step(.002);x.grant(hand::LeaseKind::ammo,false);x.dispatch();CHECK(rejects==1&&!writes&&gnasher_fire::entry_value.mechanical.held_round_serial()==1);}
 for(unsigned mode=0;mode<2;++mode){Fixture x(mode?8:2,mode?10:0);CHECK(x.dispatch()&&!captures&&!writes);CHECK(!gnasher_fire::entry_value.mechanical.reload_active());}
 std::puts("Gnasher production reload dispatch: real mechanics/fire classes, one original start across three fresh rounds, retained Grip no respawn, L2/S10->L5/S7, partial close, zero exit, actual display withdrawal, expired unpublished proposal and strict missing evidence/resource checks passed; original state/engine boundaries injected.");
}
