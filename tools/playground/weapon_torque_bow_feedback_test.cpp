#include "../vr-common/weapon_torque_bow_feedback.h"
#include "../vr-common/weapon_torque_bow_fire_driver.h"
#include "../vr-common/body_slots.h"
#include "weapon_torque_bow_arrow_tracking_fixture.h"
#include <functional>
#define CHECK BOW_ARROW_CHECK
namespace h=gears_weapon_hand;namespace vr=gears_vr;namespace w=gears_weapon;
namespace test {
std::int64_t time=bow_arrow_tracking_fixture::now;bool current=true,schema=true,returned=true,geometry=true,at_shoulder=true,bound=true,pending{};
unsigned invokes{},reads{};std::uint64_t hold=1;unsigned loaded{},reserve{};w::TorqueBowStage stage=w::TorqueBowStage::ready;
std::function<void()> on_invoke;bool mismatch{};
}
namespace weaponhand {unsigned current_gt(void*)noexcept{return 31;}}
namespace nativeequipment {
w::ArmspanCalibration calibration=w::default_body_slot_layout();
bool current(void*,h::Frame const&)noexcept{return test::current;}
}
namespace torquebow_empty_sound {
struct Entry {struct Object{std::uint64_t address{};}function{0x1600},cue{0x1700};unsigned hash{1};};
bool read(h::Frame const&,Entry&out)noexcept{out={};++test::reads;if(test::mismatch&&test::reads%2==0)out.hash=2;return test::schema;}
bool same(Entry const&a,Entry const&b)noexcept{return a.hash==b.hash;}
}
struct Request {std::uint64_t object{},function{};unsigned size{};};
bool invoke(Request*call) {
 CHECK(call&&call->object==0x1400&&call->function==0x1600&&!call->size);++test::invokes;
 if(test::on_invoke){auto callback=std::move(test::on_invoke);test::on_invoke={};callback();}return test::returned;
}
namespace torquebow_input {
namespace driver_api=gears_torque_bow_fire_driver;
struct Backend {
 driver_api::Snapshot last{};h::Frame last_source{};bool last_known{};
 std::int64_t now()const noexcept{return test::time;}
 std::uint64_t hold(h::Frame const&)const noexcept{return test::hold;}
 bool geometry_current(h::Frame const&)const noexcept{return test::geometry;}
 bool observe(h::Frame const&f,h::GripGameState const&,driver_api::Snapshot&out)noexcept{
  last_known=false;if(!test::current)return false;out={{true},test::loaded,test::reserve,0,true};last=out;last_source=f;last_known=true;return true;
 }
}backend;
struct Driver {
 bool bound()const noexcept{return test::bound;}
 w::TorqueBowStage stage()const noexcept{return test::stage;}
 w::TorqueBowInventory inventory()const noexcept{return {1,test::loaded,test::reserve};}
 Driver const& interaction()const noexcept{return *this;}std::uint64_t held_arrow_serial()const noexcept{return 0;}
}driver;
bool pending()noexcept{return test::pending;}
bool shoulder(h::Frame const&)noexcept{return test::at_shoulder;}
}
FILE* log_file{};
#include "weapon_torque_bow_feedback.inl"
struct F:bow_arrow_tracking_fixture::Fixture {
 h::GripGameState game{};h::Decision decision{};
 F(){
  test::time=bow_arrow_tracking_fixture::now;test::current=test::schema=test::returned=test::geometry=test::at_shoulder=test::bound=true;
  test::pending=test::mismatch=false;test::loaded=test::reserve=test::invokes=test::reads=0;test::hold=1;test::stage=w::TorqueBowStage::ready;
  torquebow_feedback::lost();hand.controllers.hands[0].squeeze=0;
  CHECK(w::shoulder_ammo_center(nativeequipment::calibration,{hand.stereo.head.position,hand.reference.orientation},hand.controllers.hands[0].grip.position));
  game.known=game.main_hand_held=true;game.kind=h::WeaponKind::firearm;decision.key=hand.key;decision.status=h::DecisionStatus::no_candidate;
 }
 void next(bool down,unsigned press=0,unsigned release=0){
  test::time+=20000000;hand.now_ns=test::time-1000000;++hand.key.frame;++hand.controllers.sequence;hand.stereo.sequence=hand.controllers.sequence;
  hand.controllers.sync_ns=test::time-4000000;hand.controllers.received_ns=test::time-3000000;hand.stereo.received_ns=test::time-2000000;
  hand.left_grip_down=down;auto&left=hand.controllers.hands[0];left.squeeze=down?1.f:0.f;
  left.pressed[vr::control_squeeze]+=press;left.released[vr::control_squeeze]+=release;
  if(press){hand.press_token+=press;left.last_squeeze_press_sequence=hand.controllers.sequence;left.last_squeeze_press_grip=left.grip;left.last_squeeze_press_flags=15;}
  decision.key=hand.key;decision.press_token=hand.press_token;
 }
 void run(bool observed=true){
  torquebow_input::driver_api::Snapshot snapshot{};torquebow_input::backend.observe(hand,game,snapshot);
  torquebow_feedback::sample(hand,game,decision,observed);
 }
 void baseline(){run();next(true,1);}
};
int main(){
 F f;f.baseline();test::on_invoke=[&]{f.run();};f.run();CHECK(test::invokes==1);f.run();CHECK(test::invokes==1);
 f.next(true);f.run();CHECK(test::invokes==1);torquebow_feedback::lost();f.next(true);f.run();CHECK(test::invokes==1);
 f.next(false,0,1);f.run();f.next(true,1);test::returned=false;f.run();CHECK(test::invokes==2);f.run();CHECK(test::invokes==2);
 for(auto flag:{&test::schema,&test::current,&test::geometry,&test::at_shoulder,&test::bound}){
  F blocked;blocked.baseline();*flag=false;blocked.run();CHECK(!test::invokes);*flag=true;blocked.run();CHECK(!test::invokes);
 }
 F stock;stock.baseline();test::reserve=1;stock.run();CHECK(!test::invokes);test::reserve=0;stock.run();CHECK(!test::invokes);
 F occupied;occupied.baseline();occupied.decision.lease={h::LeaseKind::support,0,occupied.hand.key.lifecycle,occupied.hand.press_token};occupied.run();CHECK(!test::invokes);
 F invalid;invalid.baseline();invalid.decision.status=h::DecisionStatus::needs_release;invalid.run();CHECK(!test::invokes);
 F wrong_pose;wrong_pose.baseline();wrong_pose.hand.controllers.hands[0].last_squeeze_press_grip.position.x+=2;wrong_pose.run();CHECK(!test::invokes);
 F changing;changing.baseline();test::mismatch=true;changing.run();CHECK(!test::invokes);test::mismatch=false;changing.run();CHECK(!test::invokes);
 F busy;busy.baseline();busy.game.busy=true;busy.run();CHECK(!test::invokes);busy.game.busy=false;busy.run();CHECK(!test::invokes);
 F pending;pending.baseline();test::pending=true;pending.run();CHECK(!test::invokes);
 F charging;charging.baseline();test::stage=w::TorqueBowStage::charging;charging.run();CHECK(!test::invokes);
 std::printf("Torque Bow no-ammo feedback caller: %u checks passed; actual intent/caller, injected original sound/Native/inventory, no game/audio certification.\n",bow_arrow_tracking_fixture::checks);
}
