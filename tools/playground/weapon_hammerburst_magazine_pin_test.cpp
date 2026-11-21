#include "weapon_hammerburst_magazine_pin.inl"
#include <cassert>
#include <cstdio>
using namespace gears_weapon_hand;namespace pin=hammerburst_magazine_pin;
static ObjectIdentity id(std::uint64_t a){return {a,0x8000,0x9000,unsigned(a/8),7,0,1};}
struct Host {Frame f{};Decision d{};pin::Observation o{};ExistingActionLifetime life{};
 gears_vr::StereoSample stereo{};gears_hand_continuity::TrackedInput raw{};
 gears_hand_continuity::Publisher publisher{44,55};bool life_ok{true};};
static bool lifetime(void* p,ExistingActionLifetime const& v) noexcept {return static_cast<Host*>(p)->life_ok&&v.epoch;}
static void ready(Host& h){
 h.f.key={11,22,33,1,5,1,44,55};h.f.flags=scene_owned|input_matched|gameplay_interactable;
 h.f.world=id(0x1000);h.f.controller=id(0x2000);h.f.pawn=id(0x3000);h.f.mesh=id(0x4000);h.f.weapon=id(0x5000);
 h.f.press_token=9;h.f.left_tracked=h.f.left_grip_known=h.f.left_grip_down=1;
 h.d.key=h.f.key;h.d.press_token=9;h.d.status=DecisionStatus::granted;h.d.release_required=0;
 h.f.reference_generation=1;h.f.reference.orientation.w=1;
 h.d.lease={LeaseKind::magazine,0,1,9};
 h.life={{h.f.key,h.f.world,h.f.controller,h.f.pawn,h.f.mesh,h.f.weapon,h.f.reference,h.f.reference_generation},1};
 h.o.weapon=h.f.weapon;h.o.component=id(0x6000);h.o.world=h.f.world;h.o.pawn=h.f.pawn;
 h.o.mesh=h.f.mesh;h.o.controller_identity=h.f.controller;h.o.lease_kind=LeaseKind::magazine;
 h.o.item=17;h.o.lease_id=9;h.o.lease_lifecycle=1;h.o.hold_generation=7;h.o.stage=4;h.o.controller=h.f.controller.address;
 h.stereo.session=1;h.stereo.sequence=5;h.stereo.predicted_time=123;h.stereo.received_ns=1000000000;
 h.stereo.focused=h.stereo.visible=h.stereo.should_render=1;h.stereo.head_flags=h.stereo.view_flags=15;
 h.stereo.head.orientation.w=1;h.stereo.eyes[0].pose.position.x=-.032;h.stereo.eyes[1].pose.position.x=.032;
 for(auto& e:h.stereo.eyes){e.pose.orientation.w=1;e.fov={-.8,.8,.8,-.8};}
 auto& c=h.raw.input;c.session=1;c.sequence=5;c.space_epoch=h.stereo.space_epoch;c.input_epoch=c.profile_generation=1;
 c.predicted_time=123;c.received_ns=c.sync_ns=1000000000;c.focused=1;
 for(auto& x:c.hands){x.grip_active=x.aim_active=x.squeeze_active=1;x.grip_flags=x.aim_flags=15;}
 c.hands[0].squeeze=1;c.hands[0].pressed[gears_vr::control_squeeze]=1;h.raw.stream_epoch=h.raw.tracking_epoch[0]=h.raw.tracking_epoch[1]=1;
 h.f.controllers=c;h.f.stereo=h.stereo;}
int main(){Host h;ready(h);pin::Seam seam{lifetime};pin::Pin p;
 assert(pin::bind(p,h.f,h.d,17,h.o.component,4,h.o,h.stereo,h.raw,h.publisher,1001000000,false,h.life));
 FrameReadObservation fail{};fail.failure=FrameReadFailure::publication;
 assert(pin::retain(p,fail,seam,&h,h.stereo,h.raw,h.publisher,1001000000,false,h.o));
 auto bad=h.o;bad.component=id(0x7000);assert(!pin::retain(p,fail,seam,&h,h.stereo,h.raw,h.publisher,1001000000,false,bad));
 h.life_ok=false;assert(!pin::retain(p,fail,seam,&h,h.stereo,h.raw,h.publisher,1001000000,false,h.o));
 h.life_ok=true;
 // Old Frame and its sample are now 104ms old; only independently current
 // raw input can preserve this logical incumbent, without advancing a Frame.
 h.stereo.sequence=h.raw.input.sequence=6;
 h.stereo.received_ns=h.raw.input.received_ns=h.raw.input.sync_ns=1100000000;
 assert(pin::retain(p,fail,seam,&h,h.stereo,h.raw,h.publisher,1104000000,false,h.o));
 assert(p.parent.key.frame==5&&p.parent.controllers.sequence==5);
 unsigned cases=5;
 for(unsigned change=0;change<16;++change){auto o=h.o;auto copy=p;auto raw=h.raw;auto eye=h.stereo;auto owner=h.publisher;auto why=fail;
  switch(change){
   case 0:++o.world.registry_index;break;case 1:++o.pawn.registry_index;break;
   case 2:++o.mesh.registry_index;break;case 3:++o.controller_identity.registry_index;break;
   case 4:++o.weapon.registry_index;break;case 5:++o.item;break;case 6:++o.lease_id;break;
   case 7:++o.lease_lifecycle;break;case 8:++o.hold_generation;break;case 9:++o.stage;break;
   case 10:o.lease_kind=LeaseKind::ammo;break;case 11:++owner.start;break;
   case 12:++raw.tracking_epoch[0];break;case 13:raw.input.hands[0].squeeze=0;break;
   case 14:++raw.input.hands[0].released[gears_vr::control_squeeze];break;
   case 15:why.failure=FrameReadFailure::game_changed;break;}
  assert(!pin::retain(copy,why,seam,&h,eye,raw,owner,1104000000,false,o));++cases;}
 assert(!pin::retain(p,fail,seam,&h,h.stereo,h.raw,h.publisher,1200000001,false,h.o));++cases;
 Host original;ready(original);auto life=original.life;++life.parent.pawn.registry_index;
 assert(!pin::bind(p,original.f,original.d,17,original.o.component,4,original.o,original.stereo,original.raw,original.publisher,1001000000,false,life));++cases;
 for(unsigned change=0;change<6;++change){auto candidate=original;auto result=p;
  switch(change){case 0:candidate.d.release_required=1;break;case 1:++candidate.d.key.lifecycle;break;
   case 2:candidate.f.left_grip_down=0;break;case 3:++candidate.raw.input.sequence;break;
   case 4:candidate.o.component.known=0;break;case 5:candidate.life.parent.reference_generation=0;break;}
  assert(!pin::bind(result,candidate.f,candidate.d,17,candidate.o.component,4,candidate.o,candidate.stereo,candidate.raw,candidate.publisher,1001000000,false,candidate.life));++cases;}
 original.d.lease.kind=original.o.lease_kind=LeaseKind::ammo;original.o.stage=6;
 assert(pin::bind(p,original.f,original.d,17,original.o.component,6,original.o,original.stereo,original.raw,original.publisher,1001000000,false,original.life));++cases;
 fail.failure=FrameReadFailure::thread_or_unarmed;
 assert(pin::retain(p,fail,seam,&h,original.stereo,original.raw,original.publisher,1001000000,false,original.o));++cases;
 h.life_ok=false;
 assert(!pin::retain(p,fail,seam,&h,original.stereo,original.raw,original.publisher,1001000000,false,original.o));++cases;
 std::printf("HB magazine pin test passed: %u cases (CPU only)\n",cases);}
