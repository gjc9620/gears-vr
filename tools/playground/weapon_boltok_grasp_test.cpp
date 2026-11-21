#include "../vr-common/weapon_boltok_grasp_pose.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
using uptr=std::uintptr_t;namespace hand=gears_weapon_hand;namespace a=gears_hand_authored_grasp;
namespace gears_weapon_hand {struct GripGameState{};}
FILE* log_file{};unsigned native_calls{},copies{},checks{};bool available=true,mock_acceptance=true,copy_ok=true;
hand::Frame physical{};a::Request last_request{};
namespace nativelancerfollow {bool bound_key_known=true,bound_is_boltok=true;struct {hand::ObjectIdentity weapon{};std::uint64_t hold_generation=1;}bound_key;}
namespace vrequip {struct Model {bool held=true;std::uint64_t generation=1;std::uint64_t hold_generation(){return generation;}bool held_for(uptr,uptr,uptr){return held;}}model;}
namespace weaponhand {struct Client {bool copy_frame_on_gt(hand::Frame& f,hand::GripGameState&,hand::Decision&){++copies;f=physical;return copy_ok;}}client;}
bool __cdecl native_publish(a::Request const* q) noexcept {++native_calls;last_request=*q;return mock_acceptance;}
template<class Fn>Fn function(char const*) noexcept {return available?static_cast<Fn>(&native_publish):nullptr;}
#include "weapon_boltok_grasp.inl"
#define C(x) do{++checks;if(!(x)){std::printf("FAIL line%u %s\n",__LINE__,#x);std::exit(1);}}while(0)
int main(){
 physical.weapon.address=64;physical.controller.address=32;physical.controllers.sync_ns=1000000000;physical.controllers.sequence=9;nativelancerfollow::bound_key.weapon=physical.weapon;
 const auto original=physical;main_skin::publish(physical);C(native_calls==1);C(last_request.mode==a::Mode::right_main_fingers&&last_request.kind==a::object::Kind::none&&a::identity(last_request.grip));C(std::memcmp(&last_request.source,&original,sizeof original)==0);
 main_skin::publish_without_mechanism(32);C(copies==1&&native_calls==2);C(std::memcmp(&last_request.source,&original,sizeof original)==0);
 main_skin::publish_without_mechanism(99);C(copies==2&&native_calls==2);
 copy_ok=false;main_skin::publish_without_mechanism(32);C(copies==3&&native_calls==2);copy_ok=true;
 nativelancerfollow::bound_key_known=false;main_skin::publish_without_mechanism(32);C(copies==3&&native_calls==2);nativelancerfollow::bound_key_known=true;
 nativelancerfollow::bound_is_boltok=false;main_skin::publish(physical);main_skin::publish_without_mechanism(32);C(copies==3&&native_calls==2);nativelancerfollow::bound_is_boltok=true;
 ++nativelancerfollow::bound_key.weapon.address;main_skin::publish(physical);C(native_calls==2);nativelancerfollow::bound_key.weapon=physical.weapon;
 ++vrequip::model.generation;main_skin::publish(physical);C(native_calls==2);--vrequip::model.generation;
 vrequip::model.held=false;main_skin::publish(physical);C(native_calls==2);vrequip::model.held=true;
 available=false;main_skin::publish(physical);C(native_calls==2);available=true;
 mock_acceptance=false;main_skin::publish(physical);C(native_calls==3&&!main_skin::last_accepted);C(std::memcmp(&physical,&original,sizeof original)==0);
 std::printf("Boltok actual cosmetic publisher: %u checks; guarded owner/frame/Native API boundaries injected. No mechanical or C0/pixel proof\n",checks);
}
