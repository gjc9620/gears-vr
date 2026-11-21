#include "../vr-common/weapon_torque_bow_grasp_pose.h"
#include "../vr-common/weapon_hand_client.h"
#include "weapon_torque_bow_arrow_tracking_fixture.h"
#include <cstring>
#define CHECK BOW_ARROW_CHECK
namespace h=gears_weapon_hand;namespace vr=gears_vr;namespace api=gears_hand_authored_grasp;
namespace test {
bool current=true,available=true,executable=true,selected=true,accept=true;unsigned calls{};api::Request last{};
bool publish(api::Request const* q) noexcept {CHECK(q&&api::structural(*q));++calls;last=*q;return accept;}
}
namespace weaponhand {
void* native_module=reinterpret_cast<void*>(1);
unsigned current_gt(void*) noexcept {return 31;}
bool executable(api::Publish,void*) noexcept {return test::executable;}
}
api::Publish bow_get_proc(void*,char const* name) noexcept {
 CHECK(std::strcmp(name,api::publish_export)==0);return test::available?test::publish:nullptr;
}
namespace nativeequipment {bool current(void*,h::Frame const&) noexcept{return test::current;}}
namespace torquebow_input {
struct Backend {
 vr::Pose support_grip{{.35,0,0},{0,0,0,1}};bool support_known=true;
 bool geometry_current(h::Frame const&)const noexcept{return test::selected;}
}backend;
}
FILE* log_file{};
#define GetProcAddress bow_get_proc
#include "weapon_torque_bow_grasp.inl"
#undef GetProcAddress
int main(){
 namespace grasp=torquebow_grasp;namespace part=gears_torque_bow_grasp_pose;
 bow_arrow_tracking_fixture::Fixture fixture;auto&f=fixture.hand;
 grasp::main_hand(f);CHECK(test::calls==1&&test::last.mode==api::Mode::right_main_fingers&&test::last.has_trigger_endpoint);
 CHECK(h::same_frame(test::last.source,f));
 CHECK(grasp::left_part(f,part::Part::arrow_shaft,fixture.rig.world_grip[0],false)&&test::calls==2&&test::last.kind==api::object::Kind::loading);
 const auto shaft=test::last.profile_id;CHECK(grasp::left_part(f,part::Part::arrow_feed,fixture.rig.world_grip[0],false));
 CHECK(test::last.profile_id!=shaft&&h::same_frame(test::last.source,f));
 CHECK(!grasp::left_part(f,part::Part::support,{},false));
 h::GripGameState game{};game.known=game.main_hand_held=game.mount_known=true;game.kind=h::WeaponKind::firearm;
 game.mount_asset=bow_arrow_tracking_fixture::object(0x1600,6);game.mount_revision=1;
 const auto delta=part::am::sub(f.controllers.hands[0].grip.position,f.controllers.hands[1].grip.position);
 game.mount.support_grip_in_weapon=part::am::rotate(vr::inverse(f.controllers.hands[1].grip.orientation),delta);
 game.mount.support_length_tolerance_metres=.08;game.mount.maximum_support_correction_radians=.6;
 game.acquire_metres=.055;game.release_metres=.08;
 h::Decision decision{};decision.key=f.key;decision.release_required=0;
 auto before=test::calls;grasp::support_hand(f,game,decision);CHECK(test::calls==before+1&&test::last.proximity&&test::last.kind==api::object::Kind::support);
 f.left_grip_down=1;f.press_token=9;decision.press_token=9;decision.lease={h::LeaseKind::support,0,f.key.lifecycle,9};decision.status=h::DecisionStatus::granted;
 before=test::calls;grasp::support_hand(f,game,decision);CHECK(test::calls==before+1&&!test::last.proximity);
 before=test::calls;decision.release_required=1;grasp::support_hand(f,game,decision);CHECK(test::calls==before);decision.release_required=0;
 decision.press_token=8;grasp::support_hand(f,game,decision);CHECK(test::calls==before);decision.press_token=9;
 game.object_incumbents[0]={h::LeaseKind::ammo,0,f.key.lifecycle,9};grasp::support_hand(f,game,decision);CHECK(test::calls==before);game.object_incumbents={};
 f.interaction_control=unsigned(vr::LeftInteractionControl::pinch);grasp::support_hand(f,game,decision);CHECK(test::calls==before);f.interaction_control=0;
 game.busy=true;grasp::support_hand(f,game,decision);CHECK(test::calls==before);game.busy=false;
 f.left_tracked=0;grasp::support_hand(f,game,decision);CHECK(test::calls==before);f.left_tracked=1;
 for(auto p:{&test::current,&test::available,&test::executable,&test::selected}){*p=false;grasp::main_hand(f);CHECK(test::calls==before);*p=true;}
 std::printf("Torque Bow V4 publisher: %u checks passed; actual caller/profile/current-lease gates, Native/Engine injected; no surface or runtime certification.\n",bow_arrow_tracking_fixture::checks);
}
