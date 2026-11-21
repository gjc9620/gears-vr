#include <cassert>
#include "../vr-common/weapon_boomshot_contact_route.h"
#include "../vr-common/weapon_boomshot_barrel_path.h"
#define main original_rig_cases
#include "weapon_rig_binding_test.cpp"
#undef main
namespace {
namespace h=gears_weapon_hand;namespace route=gears_weapon;
using Contact=route::BoomshotContact;using Route=route::BoomshotContactRoute;
using Motion=route::boomshot_barrel::Stroke;
h::Frame contact_frame(unsigned control=0){Fixture fixture;auto f=fixture.hand;
 f.press_token=99;f.left_grip_down=1;f.interaction_control=control;
 auto& left=f.controllers.hands[0];left.squeeze_active=left.trigger_active=1;left.squeeze=left.trigger=1;
 left.pressed[h::left_control(f)]=4;left.released[h::left_control(f)]=3;assert(h::frame_active(f,now));return f;}
h::Decision decision(h::Frame const& f,std::int64_t stamp,bool fresh=true){h::Decision d{};d.key=f.key;d.proposal_nonce=17;
 d.press_token=f.press_token;d.now_ns=stamp;d.lease={h::LeaseKind::magazine,0,f.key.lifecycle,f.press_token};
 d.status=fresh?h::DecisionStatus::granted:h::DecisionStatus::retained;d.new_grant=unsigned(fresh);d.release_required=0;return d;}
void advance(h::Frame& f,std::int64_t stamp){++f.key.frame;f.now_ns=stamp-1000000;
 ++f.controllers.sequence;++f.stereo.sequence;f.stereo.received_ns=stamp-2000000;
 f.controllers.received_ns=stamp-3000000;f.controllers.sync_ns=stamp-4000000;}
void remember(Route& r,h::Frame const& f,h::Decision const& d){assert(r.propose(f,Contact::opening,now)==Contact::opening);assert(r.remember(f,d,Contact::opening,now));}
}
int main(){unsigned count{};
 for(unsigned control=0;control<2;++control){auto f=contact_frame(control);auto d=decision(f,now);Route r;remember(r,f,d);
  const auto origin=f;const auto origin_decision=d;const auto stamp=now+80000000;advance(f,stamp);d=decision(f,stamp,false);
  assert(r.grant().begin_allowed(f,d,Contact::opening,stamp)&&!r.grant().begin_allowed(f,d,Contact::closing,stamp));
  std::array<h::Lease,2> incumbents{};assert(r.grant().append_incumbent(f,Contact::opening,stamp,incumbents)&&h::same(incumbents[0],d.lease));
  route::boomshot_barrel::Sample sample{};sample.stamp={{1,2,3,4},5,f.controllers.sequence,6,123,f.controllers.sync_ns};
  sample.frame=f.key.frame;sample.lease=d.lease.id;sample.press=f.press_token;sample.operation=7;
  sample.grip_held=sample.left_tracked=sample.right_tracked=sample.lease_current=sample.bone_measured=true;
  sample.grant_proven=r.grant().begin_allowed(f,d,Contact::opening,stamp);Motion stroke;
  assert(stroke.begin(sample,route::boomshot_barrel::Direction::opening,100,stamp));
  assert(r.grant().started(f,d,Contact::opening,stamp)&&!r.grant().begin_allowed(f,d,Contact::opening,stamp));
  assert(d.new_grant==0&&h::same_frame(origin,contact_frame(control))); // No forged Native new-grant bit or source renewal.
  assert(r.remember(origin,origin_decision,Contact::opening,now)); // Duplicate commit keeps started bit.
  assert(!r.grant().begin_allowed(f,d,Contact::opening,stamp));
  const auto later=now+1000000000;advance(f,later);d=decision(f,later,false);
  assert(r.grant().retained(f,d,Contact::opening,later)&&!r.grant().retained(f,d,Contact::closing,later));
  incumbents={};assert(r.grant().append_incumbent(f,Contact::opening,later,incumbents));++count;
 }
 for(unsigned reason=0;reason<32;++reason){auto origin=contact_frame();auto old=decision(origin,now);Route r;remember(r,origin,old);
  auto f=origin;auto stamp=now+80000000;advance(f,stamp);auto d=decision(f,stamp,false);
  switch(reason){case 0:++f.key.registration_cookie;break;case 1:++f.key.native_started_qpc;break;
   case 2:++f.key.game_start;break;case 3:++f.key.world_generation;break;case 4:++f.key.lifecycle;break;
   case 5:++f.world.registry_index;break;case 6:++f.pawn.registry_index;break;case 7:++f.weapon.registry_index;break;
   case 8:++f.mesh.registry_index;break;case 9:++f.controller.registry_index;break;case 10:++f.reference_generation;break;
   case 11:f.reference.position.x+=.01;break;case 12:++f.stereo.session;f.controllers.session=f.stereo.session;break;
   case 13:++f.stereo.space_epoch;f.controllers.space_epoch=f.stereo.space_epoch;break;
   case 14:++f.controllers.input_epoch;break;case 15:++f.controllers.profile_generation;break;
   case 16:f.interaction_control=1;break;case 17:++f.controllers.hands[0].released[h::left_control(f)];break;
   case 18:++f.controllers.hands[0].pressed[h::left_control(f)];break;case 19:f.left_grip_down=0;break;
   case 20:f.left_tracked=0;break;case 21:f.controllers.hands[0].squeeze_active=0;break;
   case 22:++d.lease.id;break;case 23:d.status=h::DecisionStatus::granted;d.new_grant=1;break;
   case 24:d.reserved=1;break;case 25:d.release_required=1;break;case 26:d.proposal_nonce=0;break;
   case 27:d.now_ns=stamp+1;break;case 28:d.now_ns=f.now_ns-1;break;
   case 29:f.controllers.sequence=origin.controllers.sequence-1;break;case 30:f.controllers.sync_ns=origin.controllers.sync_ns-1;break;
   case 31:stamp=now+247000000;advance(f,stamp);d=decision(f,stamp,false);break;}
  assert(!r.grant().begin_allowed(f,d,Contact::opening,stamp));++count;
 }
 {auto f=contact_frame();auto d=decision(f,now);Route r;remember(r,f,d);const auto stamp=now+246000000;advance(f,stamp);d=decision(f,stamp,false);
  assert(r.grant().begin_allowed(f,d,Contact::opening,stamp));++count;}
 {auto f=contact_frame();auto d=decision(f,now);Route r;remember(r,f,d);r.reset();
  assert(r.propose(f,Contact::opening,now)==Contact::opening&&!r.remember(f,d,Contact::opening,now));++count;}
 {auto f=contact_frame();auto d=decision(f,now);Route r;remember(r,f,d);
  assert(r.propose(f,Contact::closing,now)==Contact::opening);r.grant().clear();
  assert(r.propose(f,Contact::none,now)==Contact::opening&&!r.remember(f,d,Contact::opening,now));++count;}
 {auto f=contact_frame();auto d=decision(f,now);Route r;remember(r,f,d);auto changed=f;++changed.controllers.sequence;
  assert(r.propose(changed,Contact::opening,now)==Contact::none&&r.propose(f,Contact::opening,now)==Contact::none);++count;}
 {auto f=contact_frame();auto d=decision(f,now);Route r;assert(r.propose(f,Contact::opening,now)==Contact::opening);
  assert(r.propose(f,Contact::closing,now)==Contact::none&&!r.remember(f,d,Contact::opening,now));++count;}
 {auto f=contact_frame();auto d=decision(f,now);Route r;assert(r.propose(f,Contact::opening,now)==Contact::opening);
  ++f.controllers.sequence;assert(r.propose(f,Contact::opening,now)==Contact::none&&!r.remember(f,d,Contact::opening,now));++count;}
 {auto f=contact_frame();auto d=decision(f,now);Route r;remember(r,f,d);advance(f,now+80000000);
  std::array<h::Lease,2> slots{{{h::LeaseKind::ammo,0,f.key.lifecycle,1},{h::LeaseKind::other,0,f.key.lifecycle,2}}};const auto saved=slots;
  assert(!r.grant().append_incumbent(f,Contact::opening,now+80000000,slots)&&h::same(slots[0],saved[0])&&h::same(slots[1],saved[1]));++count;}
 {auto f=contact_frame();auto d=decision(f,now);Route r;remember(r,f,d);advance(f,now+80000000);
  std::array<h::Lease,2> slots{};slots[0].reserved=1;const auto invalid=slots;
  assert(!r.grant().append_incumbent(f,Contact::opening,now+80000000,slots)&&h::same(slots[0],invalid[0]));++count;
  slots={d.lease,d.lease};assert(!r.grant().append_incumbent(f,Contact::opening,now+80000000,slots));++count;}
 for(unsigned control=0;control<2;++control){auto f=contact_frame(control);auto d=decision(f,now);d.lease.kind=h::LeaseKind::ammo;Route r;
  assert(r.propose(f,Contact::take_carrier,now)==Contact::take_carrier&&r.remember(f,d,Contact::take_carrier,now));
  assert(r.grant().begin_allowed(f,d,Contact::take_carrier,now)&&!r.grant().begin_allowed(f,d,Contact::opening,now)&&!r.grant().begin_allowed(f,d,Contact::closing,now));
  assert(r.grant().started(f,d,Contact::take_carrier,now)&&!r.grant().begin_allowed(f,d,Contact::take_carrier,now));
  advance(f,now+80000000);d=decision(f,now+80000000,false);d.lease.kind=h::LeaseKind::ammo;
  std::array<h::Lease,2> slots{};assert(r.grant().retained(f,d,Contact::take_carrier,now+80000000)&&r.grant().append_incumbent(f,Contact::take_carrier,now+80000000,slots)&&h::same(slots[0],d.lease));++count;
 }
 {auto f=contact_frame();auto d=decision(f,now);Route r;r.propose(f,Contact::take_carrier,now);
  assert(!r.remember(f,d,Contact::take_carrier,now));++count;}
 {auto f=contact_frame();auto d=decision(f,now);d.lease.kind=h::LeaseKind::ammo;Route r;r.propose(f,Contact::opening,now);
  assert(!r.remember(f,d,Contact::opening,now));++count;}
 std::printf("Boomshot exact-purpose Native grant: %u cases passed; retained first preparation, 250ms original window, no new input, same-control/lease/identity, started/replay/conflict and incumbent checks; synthetic only\n",count);
}
