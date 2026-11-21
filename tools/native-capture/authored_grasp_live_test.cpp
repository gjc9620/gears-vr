#include "../vr-common/hand_authored_grasp.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
using DWORD=unsigned long;DWORD test_thread=7;DWORD GetCurrentThreadId(){return test_thread;}
long long test_ns=1000000000;namespace gears_vr {long long monotonic_ns(){return test_ns;}}
namespace api=gears_hand_authored_grasp;namespace hand=gears_weapon_hand;
hand::Frame test_frame{};bool enabled=true,extra_owner=true,boltok_close_owner=false;unsigned admission_count{};
hand::LeaseKind left_owner=hand::LeaseKind::magazine;
bool safe_copy(void* to,void const* from,std::size_t n){if(!from)return false;std::memcpy(to,from,n);return true;}
namespace gears_native_rig_tracking {
struct CoreSnapshot {struct {std::uint64_t game_start{},native_started_qpc{};}native;
 std::uint64_t world_generation{},reference_generation{};hand::ObjectIdentity world{},pawn{},mesh{},weapon{};gears_vr::StereoSample stereo{};};
}
namespace weapon_hand_runtime {
struct Broker {
 bool current(hand::Frame const& f){++admission_count;return enabled&&test_thread==7&&test_ns>=f.controllers.sync_ns&&test_ns-f.controllers.sync_ns<=100000000&&!std::memcmp(&f,&test_frame,sizeof f);}
 bool admit_weapon_render(hand::Frame const& f){return current(f);}
 bool admit_part_visibility(hand::Frame const& f){return current(f)&&f.left_grip_down&&(left_owner==hand::LeaseKind::magazine||left_owner==hand::LeaseKind::ammo);}
 bool copy_part_visibility_frame(hand::Frame& f,unsigned* diagnostic=nullptr){if(diagnostic)*diagnostic=0;if(!admit_part_visibility(test_frame)){if(diagnostic)*diagnostic=8;return false;}f=test_frame;return true;}
 bool admit_support_visibility(hand::Frame const& f){return current(f)&&left_owner==hand::LeaseKind::support;}
 bool admit_socket_visibility(hand::Frame const& f){return current(f)&&left_owner==hand::LeaseKind::none&&!hand::control_held(f.controllers.hands[0],gears_vr::control_squeeze);}
}broker;Broker* current(){return &broker;}
}
namespace authored_grasp_live {bool additional_owner_current(api::Request const&) noexcept{return extra_owner;}bool boltok_close_owner_current(api::Request const&) noexcept{return boltok_close_owner;}}
#include "authored_grasp_live.inl"
unsigned checks{};
#define C(x) do{++checks;if(!(x)){std::printf("FAIL line%u %s\n",__LINE__,#x);std::exit(1);}}while(0)
gears_native_rig_tracking::CoreSnapshot core(){
 gears_native_rig_tracking::CoreSnapshot c{};c.native={test_frame.key.game_start,test_frame.key.native_started_qpc};c.world_generation=test_frame.key.world_generation;c.reference_generation=test_frame.reference_generation;c.world=test_frame.world;c.pawn=test_frame.pawn;c.mesh=test_frame.mesh;c.weapon=test_frame.weapon;c.stereo=test_frame.stereo;return c;
}
int main(){
 test_frame.key={1,2,3,4,5,6,100,7};test_frame.controllers.sync_ns=test_ns;test_frame.controllers.sequence=4;test_frame.controllers.input_epoch=2;test_frame.controllers.profile_generation=3;test_frame.reference_generation=7;test_frame.stereo.session=5;test_frame.stereo.space_epoch=6;
 test_frame.world.address=16;test_frame.pawn.address=32;test_frame.mesh.address=48;test_frame.weapon.address=64;test_frame.left_grip_down=test_frame.left_grip_known=test_frame.left_tracked=1;test_frame.press_token=1;
 for(auto& h:test_frame.controllers.hands){h.grip_active=h.squeeze_active=h.trigger_active=1;h.grip_flags=15;h.squeeze=1;h.pressed[gears_vr::control_squeeze]=1;}
 api::Request q{};q.source=test_frame;q.geometry_signature=gears_hand_authored_mount::marcus115_signature;q.profile_id=19;q.side=api::Side::right;q.mode=api::Mode::right_main_fingers;
 api::Binding bindings[2]{};for(auto& b:bindings){b.geometry_signature=gears_hand_authored_mount::marcus115_signature;b.reference=q.reference;}
 api::Output out[2]{};auto current=test_frame.controllers;
 auto select=[&](){authored_grasp_live::target(core(),current,bindings,out);};
 C(authored_grasp_live::publish(&q));const auto before=test_frame;select();C(out[1].known&&!out[0].known);C(out[1].profile_id==19);C(std::memcmp(&out[1].source,&q.source,sizeof q.source)==0);C(std::memcmp(&before,&test_frame,sizeof test_frame)==0);
 q.has_trigger_endpoint=1;q.trigger_closed[0]={0,0,1,0};C(authored_grasp_live::publish(&q));current.hands[1].trigger=.5;select();C(out[1].known&&std::abs(out[1].local[0].z-std::sqrt(.5))<1e-12);
 auto reset=[&](){test_thread=7;test_ns=1000000000;enabled=extra_owner=true;current=test_frame.controllers;bindings[1].geometry_signature=gears_hand_authored_mount::marcus115_signature;bindings[1].reference=q.reference;C(authored_grasp_live::publish(&q));};
 reset();enabled=false;select();C(!out[1].known);enabled=true;select();C(!out[1].known);
 reset();extra_owner=false;select();C(!out[1].known);extra_owner=true;select();C(!out[1].known);
 reset();test_ns+=100000001;select();C(!out[1].known);
 reset();current.sync_ns=test_ns+1;select();C(!out[1].known);
 reset();current.sequence--;select();C(!out[1].known);
 reset();current.input_epoch++;select();C(!out[1].known);
 reset();current.profile_generation++;select();C(!out[1].known);
 reset();current.hands[1].released[gears_vr::control_trigger]++;select();C(!out[1].known);
 reset();current.hands[1].grip_active=0;select();C(!out[1].known);
 reset();current.hands[1].trigger_active=0;select();C(!out[1].known);
 reset();bindings[1].geometry_signature[31]^=1;select();C(!out[1].known);
 reset();bindings[1].reference[4]={0,0,.01,std::sqrt(.9999)};select();C(!out[1].known);
 reset();auto wrong=core();wrong.weapon.address+=8;authored_grasp_live::target(wrong,current,bindings,out);C(!out[1].known);
 reset();test_thread=8;select();C(!out[1].known);test_thread=7;
 reset();authored_grasp_live::invalidate();select();C(!out[1].known);
 q.has_trigger_endpoint=0;q.trigger_closed={};q.side=api::Side::left;q.mode=api::Mode::left_part;q.kind=api::object::Kind::magazine;current=test_frame.controllers;
 C(authored_grasp_live::publish(&q));select();C(out[0].known&&out[0].left.has_authored_pose&&out[0].left.kind==q.kind&&!out[1].known);C(out[0].left.preset==api::object::Preset::authored);
 left_owner=hand::LeaseKind::none;C(!authored_grasp_live::publish(&q));select();C(!out[0].known);
 test_frame.controllers.hands[0].squeeze=0;test_frame.left_grip_down=0;q.source=test_frame;q.proximity=1;current=test_frame.controllers;C(authored_grasp_live::publish(&q));select();C(out[0].known&&out[0].left.passive_socket);
 current.hands[0].squeeze=1;select();C(!out[0].known);
 C(!authored_grasp_live::publish(nullptr));C(admission_count>0);
 // Separate CURRENT finger refresh cannot replay an expired wrist goal.
 const auto finger_base=test_frame;
 // Actual Native close-support policy is exercised by this fixture harness.
 auto close_case=[&](){
  authored_grasp_live::invalidate();enabled=extra_owner=true;boltok_close_owner=false;left_owner=hand::LeaseKind::support;test_thread=7;test_ns=1000000000;
  test_frame=finger_base;test_frame.controllers.sync_ns=test_ns;test_frame.left_grip_down=1;test_frame.press_token=21;
  test_frame.controllers.hands[0].squeeze=1;test_frame.controllers.hands[0].pressed[gears_vr::control_squeeze]=5;
  test_frame.controllers.hands[0].released[gears_vr::control_squeeze]=4;current=test_frame.controllers;
  api::Request close{};close.source=test_frame;close.geometry_signature=gears_hand_authored_mount::marcus115_signature;
  close.profile_id=0x424f4c544f4b0011ULL;close.side=api::Side::left;close.mode=api::Mode::left_part;close.kind=api::object::Kind::support;
  close.grip.position={0,0,-.05};close.grip.orientation={};close.closed[0]={0,0,std::sin(.2),std::cos(.2)};
  bindings[0].geometry_signature=close.geometry_signature;bindings[0].reference=close.reference;return close;
 };
 auto close=close_case();C(!authored_grasp_live::publish(&close)); // profile alone is not owner admission.
 close=close_case();boltok_close_owner=true;C(authored_grasp_live::publish(&close));select();
 C(out[0].known&&out[0].left.kind==api::object::Kind::support&&out[0].left.grip.position.z==-.05);
 close=close_case();boltok_close_owner=true;close.profile_id++;C(!authored_grasp_live::publish(&close));
 close=close_case();boltok_close_owner=true;close.grip.position={0,0,-.001};C(!authored_grasp_live::publish(&close));
 close=close_case();boltok_close_owner=true;close.proximity=1;C(!authored_grasp_live::publish(&close));
 close=close_case();boltok_close_owner=true;close.side=api::Side::right;C(!authored_grasp_live::publish(&close));
 close=close_case();boltok_close_owner=true;close.kind=api::object::Kind::magazine;C(!authored_grasp_live::publish(&close));
 close=close_case();boltok_close_owner=true;left_owner=hand::LeaseKind::none;C(!authored_grasp_live::publish(&close));
 close=close_case();boltok_close_owner=true;enabled=false;select();C(!out[0].known);
 close=close_case();boltok_close_owner=true;C(authored_grasp_live::publish(&close));select();C(out[0].known);
 auto finger_case=[&](){
  authored_grasp_live::invalidate();enabled=extra_owner=true;left_owner=hand::LeaseKind::magazine;test_thread=7;test_ns=1000000000;
  test_frame=finger_base;test_frame.controllers.sync_ns=test_ns;test_frame.left_grip_down=1;test_frame.press_token=11;
  test_frame.controllers.hands[0].squeeze=1;test_frame.controllers.hands[0].pressed[gears_vr::control_squeeze]=3;
  test_frame.controllers.hands[0].released[gears_vr::control_squeeze]=2;current=test_frame.controllers;
  api::Request part{};part.source=test_frame;part.geometry_signature=gears_hand_authored_mount::marcus115_signature;
  part.profile_id=91;part.side=api::Side::left;part.mode=api::Mode::left_part;part.kind=api::object::Kind::magazine;
  part.closed[0]={0,0,std::sin(.2),std::cos(.2)};bindings[0].geometry_signature=part.geometry_signature;bindings[0].reference=part.reference;
  part.grip.position={1,2,3};C(authored_grasp_live::publish(&part));
  test_ns+=120000000;test_frame.controllers.sync_ns=test_ns;test_frame.controllers.sequence+=9;test_frame.key.frame++;
  current=test_frame.controllers;part.source=test_frame;part.grip={};return part;
 };
 auto finger=finger_case();const auto fresh_source=test_frame;
 C(authored_grasp_live::publish_fingers(&finger));select();
 C(out[0].known&&out[0].finger_override&&out[0].left.kind==api::object::Kind::none);
 C(out[0].finger_kind==api::object::Kind::magazine);
 C(std::memcmp(&out[0].source,&fresh_source,sizeof fresh_source)==0);
 C(std::memcmp(out[0].local.data(),finger.closed.data(),sizeof finger.closed)==0);
 C(std::memcmp(&test_frame,&fresh_source,sizeof fresh_source)==0);
 // Index acquisition uses the SAME current part gate, with no squeeze.
 finger=finger_case();test_frame.interaction_control=unsigned(gears_vr::LeftInteractionControl::pinch);
 test_frame.controllers.hands[0].squeeze=0;test_frame.controllers.hands[0].trigger=1;
 finger.source=test_frame;current=test_frame.controllers;C(authored_grasp_live::publish_fingers(&finger));select();
 C(out[0].known&&out[0].finger_override&&out[0].left.kind==api::object::Kind::none);
 current.hands[0].trigger=0;current.hands[0].released[gears_vr::control_trigger]++;select();C(!out[0].known);
 auto fresh_finger=[&](){auto value=finger_case();C(authored_grasp_live::publish_fingers(&value));return value;};
 finger=fresh_finger();test_ns+=100000001;select();C(!out[0].known);
 // Old fixed profile plus a NEW actually committed Native frame. Old source
 // stays byte-identical; only current Native provenance may reach output.
 finger=fresh_finger();const auto registered=finger.source;test_ns+=120000000;
 test_frame.key.frame++;test_frame.now_ns=test_ns;test_frame.controllers.sync_ns=test_ns;
 test_frame.controllers.sequence++;current=test_frame.controllers;select();
 C(out[0].known&&out[0].finger_override);C(out[0].source.controllers.sync_ns==test_ns);
 C(std::memcmp(&authored_grasp_live::part_fingers.request.source,&registered,sizeof registered)==0);
 // The current C0 input can precede the separately committed part frame.
 // No output for that order, but retain ONLY the fixed local descriptor.
 // A later genuine C0 observation must still join the current grant.
 finger=fresh_finger();const auto registered_order=finger.source;
 test_frame.key.frame++;test_frame.controllers.sequence++;test_frame.controllers.sync_ns=test_ns;
 select();C(!out[0].known);C(authored_grasp_live::part_fingers.armed.load());
 current=test_frame.controllers;select();C(out[0].known&&out[0].finger_override);
 C(out[0].source.controllers.sequence==current.sequence);
 C(std::memcmp(&authored_grasp_live::part_fingers.request.source,&registered_order,sizeof registered_order)==0);
 // The second admission can reject after a successful current-frame copy.
 // No output and no replay of that grant. The fixed descriptor survives and
 // only a later newly validated frame may supply the finger command.
 finger=fresh_finger();const auto registered_second=finger.source;
 extra_owner=false;select();C(!out[0].known);C(authored_grasp_live::part_fingers.armed.load());
 extra_owner=true;test_frame.key.frame++;test_frame.controllers.sequence++;
 current=test_frame.controllers;select();C(out[0].known&&out[0].finger_override);
 C(out[0].source.controllers.sequence==current.sequence);
 C(std::memcmp(&authored_grasp_live::part_fingers.request.source,&registered_second,sizeof registered_second)==0);
 // Neutral C gets the authored15 under the EXISTING socket permission,
 // never a part lease or a V5 active-finger registration.
 auto neutral=finger_case();neutral.kind=api::object::Kind::starter;neutral.proximity=1;
 test_frame.left_grip_down=0;test_frame.controllers.hands[0].squeeze=0;
 neutral.source=test_frame;current=test_frame.controllers;left_owner=hand::LeaseKind::none;
 C(authored_grasp_live::publish(&neutral));select();
 C(out[0].known&&out[0].left.passive_socket&&out[0].left.has_authored_pose);
 C(std::memcmp(out[0].local.data(),neutral.closed.data(),sizeof neutral.closed)==0);
 C(!authored_grasp_live::publish_fingers(&neutral)); // Passive never uses active registration.
 current.hands[0].squeeze=1;current.hands[0].pressed[gears_vr::control_squeeze]++;select();C(!out[0].known);
 // No current Native grant means NO output, not deletion of a fixed
 // local pose descriptor. A later NEW current frame must still be admitted.
 finger=fresh_finger();const auto registered_gap=finger.source;
 enabled=false;select();C(!out[0].known);enabled=true;
 test_ns+=120000000;select();C(!out[0].known); // Still stale, no output.
 test_frame.key.frame++;test_frame.now_ns=test_ns;test_frame.controllers.sync_ns=test_ns;
 test_frame.controllers.sequence++;current=test_frame.controllers;select();
 C(out[0].known&&out[0].finger_override&&out[0].source.controllers.sync_ns==test_ns);
 C(std::memcmp(&authored_grasp_live::part_fingers.request.source,&registered_gap,sizeof registered_gap)==0);
 // A failed REPUBLISH during an otherwise continuous grasp must not erase
 // the fixed descriptor. A later real current frame still supplies authority.
 finger=fresh_finger();const auto before_failed_publish=authored_grasp_live::part_fingers.request;
 enabled=false;C(!authored_grasp_live::publish_fingers(&finger));
 C(authored_grasp_live::part_fingers.armed.load());
 C(std::memcmp(&authored_grasp_live::part_fingers.request,&before_failed_publish,sizeof before_failed_publish)==0);
 select();C(!out[0].known);enabled=true;test_ns+=120000000;
 test_frame.key.frame++;test_frame.controllers.sequence++;test_frame.controllers.sync_ns=test_ns;
 current=test_frame.controllers;select();C(out[0].known&&out[0].finger_override);
 C(out[0].source.controllers.sync_ns==test_ns);
 // Malformed/foreign-thread publications cannot clear another valid fixed
 // descriptor. They also cannot authorize any output after actual release.
 finger=fresh_finger();C(!authored_grasp_live::publish_fingers(nullptr));
 auto malformed=finger;malformed.grip.position.x=1;C(!authored_grasp_live::publish_fingers(&malformed));
 test_thread=8;C(!authored_grasp_live::publish_fingers(&finger));test_thread=7;
 select();C(out[0].known&&out[0].finger_override);
 finger=fresh_finger();enabled=false;C(!authored_grasp_live::publish_fingers(&finger));
 test_frame.controllers.hands[0].released[gears_vr::control_squeeze]++;
 test_frame.controllers.hands[0].pressed[gears_vr::control_squeeze]++;test_frame.press_token++;
 enabled=true;current=test_frame.controllers;select();C(!out[0].known);
 // Explicit lifecycle invalidation remains a real descriptor revocation.
 finger=fresh_finger();enabled=false;C(!authored_grasp_live::publish_fingers(&finger));
 authored_grasp_live::invalidate();enabled=true;select();C(!out[0].known);
 // A release/repress DURING that unavailable gap cannot revive old fitting.
 finger=fresh_finger();enabled=false;select();C(!out[0].known);
 test_frame.controllers.hands[0].released[gears_vr::control_squeeze]++;
 test_frame.controllers.hands[0].pressed[gears_vr::control_squeeze]++;
 test_frame.press_token++;enabled=true;current=test_frame.controllers;select();C(!out[0].known);
 // A held button alone cannot move a preset across a new grasp, control,
 // identity, lifecycle, reference, input epoch or release/repress edge.
 for(unsigned mutation=0;mutation<12;++mutation){
  finger=fresh_finger();
  switch(mutation){
   case 0:test_frame.press_token++;break;case 1:test_frame.interaction_control++;break;
   case 2:test_frame.key.lifecycle++;break;case 3:test_frame.key.registration_cookie++;break;
   case 4:test_frame.weapon.address++;break;case 5:test_frame.mesh.address++;break;
   case 6:test_frame.reference_generation++;break;case 7:test_frame.stereo.space_epoch++;break;
   case 8:test_frame.controllers.input_epoch++;break;case 9:test_frame.controllers.profile_generation++;break;
   case 10:test_frame.controllers.hands[0].pressed[gears_vr::control_squeeze]++;break;
   case 11:test_frame.controllers.hands[0].released[gears_vr::control_trigger]++;break;
  }
  current=test_frame.controllers;select();C(!out[0].known);
 }

 finger=fresh_finger();left_owner=hand::LeaseKind::none;select();C(!out[0].known); // Raw squeeze1 alone is insufficient.
 finger=fresh_finger();extra_owner=false;select();C(!out[0].known);
 finger=fresh_finger();current.hands[0].squeeze=0;current.hands[0].released[gears_vr::control_squeeze]++;select();C(!out[0].known);
 finger=fresh_finger();current.hands[0].trigger_active=0;current.hands[0].released[gears_vr::control_trigger]++;select();C(!out[0].known);
 finger=fresh_finger();current.hands[0].grip_flags=0;select();C(!out[0].known);
 finger=fresh_finger();current.input_epoch++;select();C(!out[0].known);
 finger=fresh_finger();current.profile_generation++;select();C(!out[0].known);
 finger=fresh_finger();bindings[0].geometry_signature[31]^=1;select();C(!out[0].known);
 finger=fresh_finger();bindings[0].reference[4]={0,0,.01,std::sqrt(.9999)};select();C(!out[0].known);bindings[0].reference=finger.reference;
 finger=fresh_finger();auto changed_core=core();changed_core.weapon.address+=8;authored_grasp_live::target(changed_core,current,bindings,out);C(!out[0].known);
 finger=fresh_finger();test_thread=8;select();C(!out[0].known);test_thread=7;
 finger=fresh_finger();authored_grasp_live::invalidate();select();C(!out[0].known);
 finger=finger_case();finger.grip.position.x=.1;C(!authored_grasp_live::publish_fingers(&finger));select();C(!out[0].known);
 finger=finger_case();finger.proximity=1;C(!authored_grasp_live::publish_fingers(&finger));
 finger=finger_case();finger.kind=api::object::Kind::support;C(!authored_grasp_live::publish_fingers(&finger));
 finger=finger_case();finger.side=api::Side::right;C(!authored_grasp_live::publish_fingers(&finger));
 finger=fresh_finger();auto dynamic=finger;dynamic.grip.position={4,5,6};C(authored_grasp_live::publish(&dynamic));select();
 C(out[0].known&&out[0].finger_override&&out[0].left.grip.position.x==4&&out[0].left.grip.position.y==5&&out[0].left.grip.position.z==6);
 C(std::memcmp(&out[0].source,&dynamic.source,sizeof dynamic.source)==0);
 finger=fresh_finger();dynamic=finger;dynamic.profile_id++;dynamic.grip.position={4,5,6};C(authored_grasp_live::publish(&dynamic));select();
 C(out[0].known&&!out[0].finger_override&&out[0].profile_id==dynamic.profile_id&&out[0].left.grip.position.x==4);
 // Diagnostic accounting must not recover an expired goal or turn a
 // cosmetic socket into a held part. Exercise the production adapter.
 dynamic=finger_case();dynamic.kind=api::object::Kind::starter;dynamic.grip.position={1,2,3};
 auto prior=authored_grasp_live::dynamic_observations[6][10].load();
 C(authored_grasp_live::publish(&dynamic));select();C(out[0].known&&out[0].left.kind==api::object::Kind::starter);
 C(authored_grasp_live::dynamic_observations[6][10].load()==prior+1);
 prior=authored_grasp_live::dynamic_observations[6][3].load();test_ns+=100000001;select();
 C(!out[0].known&&authored_grasp_live::dynamic_observations[6][3].load()==prior+1);
 dynamic=finger_case();dynamic.kind=api::object::Kind::starter;dynamic.grip.position={1,2,3};
 C(authored_grasp_live::publish(&dynamic));auto wrong_dynamic_core=core();wrong_dynamic_core.weapon.address+=8;
 prior=authored_grasp_live::dynamic_observations[6][4].load();
 authored_grasp_live::target(wrong_dynamic_core,current,bindings,out);
 C(!out[0].known&&authored_grasp_live::dynamic_observations[6][4].load()==prior+1);
 dynamic=finger_case();dynamic.kind=api::object::Kind::starter;dynamic.proximity=1;dynamic.grip.position={1,2,3};
 test_frame.left_grip_down=0;test_frame.controllers.hands[0].squeeze=0;left_owner=hand::LeaseKind::none;
 current=test_frame.controllers;dynamic.source=test_frame;C(authored_grasp_live::publish(&dynamic));select();
 C(out[0].known&&out[0].left.passive_socket);
 prior=authored_grasp_live::dynamic_observations[7][7].load();current.hands[0].squeeze=1;select();
 C(!out[0].known&&authored_grasp_live::dynamic_observations[7][7].load()==prior+1);
 std::printf("Authored Native actual adapter: %u checks; injected broker/extra-owner and CoreSnapshot. No live C0, gun-specific identity or skin proof\n",checks);
}
