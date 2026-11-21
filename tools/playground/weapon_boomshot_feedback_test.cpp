#include "../vr-common/weapon_boomshot_feedback.h"
#define main previous_feedback_skin_cases
#include "weapon_boomshot_skin_test.cpp"
#undef main
namespace {
using uptr=std::uint64_t;FILE* log_file{};
unsigned cue_calls{},cue_reads{},cue_failure{};bool cue_returned=true;Normal* feedback_fixture{};
struct Request {uptr object{},function{};unsigned size{};};
bool invoke(Request* call){assert(feedback_fixture&&call->object==feedback_fixture->input.f.weapon.address&&call->function==0x8800&&!call->size);++cue_calls;return cue_returned;}
namespace vrequip {struct Model {std::uint64_t hold_generation()const{return 10;}}model;}
namespace boomshot_empty_sound {
struct Entry {h::ObjectIdentity function{},cue{};};
bool read(h::Frame const&,Entry& out)noexcept{++cue_reads;out={object(0x8800,31),object(0x8900,32)};if(cue_failure==2&&cue_reads%2==0)++out.cue.name_number;return cue_failure!=1;}
bool same(Entry const& a,Entry const& b)noexcept{return h::same(a.function,b.function)&&h::same(a.cue,b.cue);}
}
namespace boomshot_feedback {bool current(h::Frame const& f)noexcept{return nativeequipment::current(nullptr,f);}bool active(h::Frame const&)noexcept{return cue_failure!=3;}}
#define BOOMSHOT_FEEDBACK_SOUND_BOUNDARY_FIXTURE 1
#define BOOMSHOT_FEEDBACK_CURRENT_BOUNDARY_FIXTURE 1
#define BOOMSHOT_FEEDBACK_STATE_BOUNDARY_FIXTURE 1
#include "weapon_boomshot_feedback.inl"
#undef BOOMSHOT_FEEDBACK_STATE_BOUNDARY_FIXTURE
#undef BOOMSHOT_FEEDBACK_CURRENT_BOUNDARY_FIXTURE
#undef BOOMSHOT_FEEDBACK_SOUND_BOUNDARY_FIXTURE
namespace feedback=boomshot_feedback;
h::GripGameState feedback_game(){h::GripGameState game{};game.known=game.main_hand_held=true;game.kind=h::WeaponKind::firearm;return game;}
void feedback_reset(Normal& x){reset_skin(x);feedback::intent={};feedback_fixture=&x;cue_calls=cue_reads=cue_failure=0;cue_returned=true;
 lancerreload_inventory::owned_counts={1,1,0,false};x.refresh();neutral(x);
 auto& left=x.input.f.controllers.hands[0];assert(c::shoulder_ammo_center(nativeequipment::calibration,{x.input.f.stereo.head.position,x.input.f.reference.orientation},left.grip.position));
 left.released[gears_vr::control_squeeze]=left.pressed[gears_vr::control_squeeze];
 left.last_squeeze_press_grip=left.grip;left.last_squeeze_press_flags=15;assert(h::frame_active(x.input.f,clock_value));
}
void edge(Normal& x,bool down){x.next();auto& f=x.input.f;auto& left=f.controllers.hands[0];
 if(down){++left.pressed[gears_vr::control_squeeze];++f.press_token;left.last_squeeze_press_sequence=f.controllers.sequence;left.last_squeeze_press_grip=left.grip;left.last_squeeze_press_flags=15;}
 else ++left.released[gears_vr::control_squeeze];f.left_grip_down=down;left.squeeze=down?1.f:0.f;left.trigger=0;
 x.input.d.press_token=f.press_token;x.input.d.lease={};x.input.d.new_grant=0;x.input.d.status=h::DecisionStatus::no_candidate;
}
void feedback_sample(Normal& x){const auto before=x.input.f;feedback::sample(x.input.f,feedback_game(),x.input.d);assert(h::same_frame(before,x.input.f));}
}
int main(){unsigned cases{};
 {Normal x(1);feedback_reset(x);feedback_sample(x);edge(x,true);feedback_sample(x);
  if(cue_calls!=1||cue_reads!=2){lancerreload_inventory::Counts counts{};
   std::fprintf(stderr,"feedback fixture calls=%u reads=%u frame=%u decision=%u shoulder=%u quiet=%u pending=%u source_failure=%u down=%u token=%llu\n",cue_calls,cue_reads,
    unsigned(h::frame_active(x.input.f,clock_value)),unsigned(skin::decision(x.input.f,x.input.d)),unsigned(feedback::shoulder(x.input.f)),
    unsigned(feedback::quiet(x.input.f,counts)),unsigned(boomshot_reload_owner::owner.pending()),source_failure,x.input.f.left_grip_down,x.input.f.press_token);}
  assert(cue_calls==1&&cue_reads==2);
  feedback_sample(x);x.next();x.input.d.status=h::DecisionStatus::no_candidate;feedback_sample(x);assert(cue_calls==1);
  edge(x,false);feedback_sample(x);edge(x,true);feedback_sample(x);assert(cue_calls==2&&!native_calls&&!inventory_writer_calls);++cases;}
 {Normal x(2);feedback_reset(x);feedback_sample(x);edge(x,true);cue_returned=false;feedback_sample(x);feedback_sample(x);assert(cue_calls==1);
  x.next();x.input.d.status=h::DecisionStatus::no_candidate;cue_returned=true;feedback_sample(x);assert(cue_calls==1);++cases;}
 for(unsigned fail:{1u,2u}){Normal x(3+fail);feedback_reset(x);feedback_sample(x);edge(x,true);cue_failure=fail;feedback_sample(x);cue_failure=0;
  feedback_sample(x);assert(!cue_calls);edge(x,false);feedback_sample(x);edge(x,true);feedback_sample(x);assert(cue_calls==1);++cases;}
 for(unsigned reason=0;reason<7;++reason){Normal x(6+reason);feedback_reset(x);feedback_sample(x);edge(x,true);
  switch(reason){case 0:lancerreload_inventory::owned_counts.reserve=1;x.refresh();break;case 1:lancerreload_inventory::owned_counts.infinite=true;x.refresh();break;
   case 2:normal_fault=29;break;case 3:fire_neutral=false;break;case 4:native_reload=true;break;
   case 5:x.input.d.lease={h::LeaseKind::cover,0,x.input.f.key.lifecycle,1};break;
   case 6:x.input.f.controllers.hands[0].last_squeeze_press_grip.position={100,100,100};break;}
  feedback_sample(x);assert(!cue_calls&&!native_calls&&!inventory_writer_calls);++cases;}
 {Normal x(13);feedback_reset(x);edge(x,true);feedback_sample(x);assert(!cue_calls);++cases;}
 {Normal x(14);feedback_reset(x);lancerreload_inventory::owned_counts.used=0;x.refresh();feedback_sample(x);edge(x,true);feedback_sample(x);
  assert(cue_calls==1&&lancerreload_inventory::owned_counts.used==0&&!native_calls&&!inventory_writer_calls);++cases;}
 {Normal x(15);feedback_reset(x);feedback_sample(x);edge(x,true);cue_failure=3;feedback_sample(x);assert(!cue_calls&&!cue_reads);++cases;}
 for(unsigned pending_kind=0;pending_kind<4;++pending_kind){Normal x(16+pending_kind);feedback_reset(x);feedback_sample(x);edge(x,true);auto game=feedback_game();
  if(pending_kind==0)game.request_magazine=true;if(pending_kind==1)game.request_ammo=true;if(pending_kind==2)game.magazine_observation_pending=true;
  if(pending_kind==3)x.input.d.new_grant=1;
  feedback::sample(x.input.f,game,x.input.d);assert(!cue_calls&&!cue_reads);feedback_sample(x);assert(!cue_calls);++cases;}
 std::printf("Boomshot GT no-ammo feedback: %u cases passed; real physical intent + owner/debt/count/shoulder filters, once-only unknown return and metadata failure consumption, finite/infinite/full/foreign/historical press; Source/current/Engine sound boundary stubbed, no audible/game proof\n",cases);
}
