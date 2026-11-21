#include "../vr-common/weapon_boomshot_mechanics.h"
#include "../vr-common/weapon_boomshot_owner_events.h"
#include "../vr-common/weapon_boomshot_contact_route.h"
#include "../vr-common/weapon_boomshot_barrel_path.h"
#include <cassert>
#pragma push_macro("main")
#undef main
#define main original_rig_cases
#include "weapon_rig_binding_test.cpp"
#undef main
#pragma pop_macro("main")
namespace {
namespace h=gears_weapon_hand;namespace c=gears_weapon;namespace m=c::boomshot_barrel;
std::int64_t clock_value=now;bool is_gt=true;double hand_q{},bone_q{};bool wrong_begin{},wrong_end{},drop_receipt{};
namespace weaponhand {unsigned current_gt(void*)noexcept{return is_gt?1u:0u;}std::int64_t clock_ns(void*)noexcept{return clock_value;}}
// Engine/Rig boundary fixture only. The production physical kernel, grant,
// owner event stream and mechanical core below are real, unmodified types.
namespace boomshot_barrel {
m::Stroke stroke;unsigned begins{},observations{},releases{};double measured_q{};bool measured_q_known{};
m::Sample sample(h::Frame const& f,h::Decision const& d,std::uint64_t operation){
 m::Sample s{};s.stamp={{f.world.address,f.pawn.address,f.weapon.address,f.key.lifecycle},99,
  f.controllers.sequence,181,std::int64_t(f.key.frame),f.controllers.sync_ns};
 s.frame=f.key.frame;s.lease=d.lease.id;s.press=f.press_token;s.operation=operation;
 s.hand_m={0,-hand_q*m::travel_raw/100.,0};s.actual_progress=bone_q;measured_q=bone_q;measured_q_known=true;
 s.grip_held=s.grant_proven=s.bone_measured=s.left_tracked=s.right_tracked=s.lease_current=true;return s;
}
bool begin(h::Frame const& f,h::Decision const& d,std::uint64_t operation,m::Direction direction,c::BoomshotMechanismGrant& grant){
 const auto purpose=direction==m::Direction::opening?c::BoomshotContact::opening:c::BoomshotContact::closing;
 if(!grant.begin_allowed(f,d,purpose,clock_value))return false;auto s=sample(f,d,operation);if(wrong_begin)--s.stamp.sequence;
 if(!stroke.begin(s,direction,100,clock_value))return false;if(!grant.started(f,d,purpose,clock_value)){stroke.release();return false;}++begins;return true;
}
m::Result observe(h::Frame const& f,h::Decision const& d,std::uint64_t operation){++observations;auto s=sample(f,d,operation);if(wrong_end)++s.stamp.sequence;return stroke.observe(s,clock_value);}
bool take_completion(h::Frame const&,h::Decision const&,std::uint64_t,m::Completion& out){if(drop_receipt){out={};return false;}return stroke.take_completion(out);}
void release(){++releases;stroke.release();}
}
#include "weapon_boomshot_core_motion.inl"
struct Owned {
 h::Frame f{};h::Decision d{};c::BoomshotContactRoute route;c::BoomshotInteraction owner;c::BoomshotOwnerEvents events;
 boomshot_core_motion::Bridge bridge;c::BoomshotOwnerEvents::InputEvent input{};
 c::BoomshotFrame frame()const{return {input.stamp(),owner.inventory(),true,true,true,true,true,false};}
 void next(bool mint=true){clock_value+=40000000;++f.key.frame;++f.controllers.sequence;++f.stereo.sequence;
  f.now_ns=clock_value-1000000;f.stereo.received_ns=clock_value-2000000;
  f.controllers.received_ns=clock_value-3000000;f.controllers.sync_ns=clock_value-4000000;
  if(mint)assert(events.input({f.controllers.sequence,f.controllers.input_epoch,181,f.controllers.profile_generation,
   f.controllers.sync_ns,std::int64_t(f.key.frame)},clock_value,input)==c::BoomshotOwnerEvents::Result::accepted);
  d.key=f.key;d.now_ns=clock_value;d.new_grant=0;d.status=h::DecisionStatus::retained;
 }
 void grant(c::BoomshotContact purpose){d={};d.key=f.key;d.proposal_nonce=17;d.press_token=f.press_token;d.now_ns=clock_value;
  d.new_grant=1;d.release_required=0;d.status=h::DecisionStatus::granted;d.lease={h::LeaseKind::magazine,0,f.key.lifecycle,f.press_token};
  const auto proposed=route.propose(f,purpose,clock_value);const bool remembered=route.remember(f,d,purpose,clock_value);
  if(proposed!=purpose||!remembered)std::fprintf(stderr,"grant failure purpose=%u proposed=%u remembered=%u active=%u frame=%llu token=%llu clock=%lld now=%lld sync=%lld\n",unsigned(purpose),unsigned(proposed),unsigned(remembered),unsigned(h::frame_active(f,clock_value)),f.key.frame,f.press_token,clock_value,f.now_ns,f.controllers.sync_ns);
  assert(proposed==purpose&&remembered);}
 Owned(){
  clock_value=now;is_gt=true;hand_q=bone_q=0;wrong_begin=wrong_end=drop_receipt=false;boomshot_barrel::stroke={};boomshot_barrel::measured_q_known=false;
  Fixture rig;f=rig.hand;f.press_token=99;f.left_grip_down=1;auto& left=f.controllers.hands[0];
  left.squeeze_active=left.trigger_active=1;left.squeeze=left.trigger=1;left.pressed[h::left_control(f)]=4;left.released[h::left_control(f)]=3;
  const c::BoomshotIdentity id{f.world.address,f.pawn.address,f.weapon.address,77};
  assert(owner.bind_inspection({id,5,1,100000000,true,true},{id,1,1,2})&&events.bind(id,owner.binding_token()));
  next();assert(owner.observe(frame())==c::BoomshotEvent::motion_reset);next();
  c::BoomshotPlan plan{};plan.identity=id;plan.config_id=5;plan.plan_id=16;plan.condition=c::BoomshotCondition::inspection;plan.count=2;
  plan.current_instance_verified=plan.original_order_verified=plan.quantity_verified=true;
  plan.steps[0]={c::BoomshotStepKind::move_mechanism,c::BoomshotPart::barrel,11,101,0,10,false};
  plan.steps[1]={c::BoomshotStepKind::move_mechanism,c::BoomshotPart::barrel,12,102,0,10,true};
  assert(owner.begin_inspection(frame(),{owner.binding_token(),501,plan,true,true},1)==c::BoomshotEvent::started);
  grant(c::BoomshotContact::opening);
 }
 bool begin(m::Direction direction=m::Direction::opening){return bridge.begin(f,d,route.grant(),owner,input,frame(),direction);}
 auto observe(){return bridge.observe(f,d,route.grant(),owner,input,frame());}
 void halfway(){next();hand_q=bone_q=.5;auto result=observe();if(result.physical!=m::Result::moved||result.event!=c::BoomshotEvent::none)std::fprintf(stderr,"halfway physical=%u event=%u wrong_begin=%u active=%u\n",unsigned(result.physical),unsigned(result.event),unsigned(wrong_begin),unsigned(bridge.active()));assert(result.physical==m::Result::moved&&result.event==c::BoomshotEvent::none);}
 auto endpoint(double target){next();hand_q=bone_q=target;auto result=observe();assert(result.physical==m::Result::moved);next();return observe();}
};
}
int main(){unsigned cases{};
 {Owned x;assert(x.begin());x.halfway();auto repeated=x.observe();assert(repeated.physical==m::Result::waiting&&x.bridge.active());
  auto result=x.endpoint(1);assert(result.physical==m::Result::complete&&result.event==c::BoomshotEvent::step_completed&&x.owner.step_index()==1&&!x.bridge.active());
  // A new real press/purpose is needed for closing; the old opening grant cannot start it.
  assert(!x.begin(m::Direction::closing));x.next();++x.f.press_token;auto& left=x.f.controllers.hands[0];
  ++left.pressed[h::left_control(x.f)];++left.released[h::left_control(x.f)];x.grant(c::BoomshotContact::closing);
  assert(x.owner.observe(x.frame())==c::BoomshotEvent::none&&x.begin(m::Direction::closing));x.halfway();result=x.endpoint(0);
  assert(result.physical==m::Result::complete&&result.event==c::BoomshotEvent::request_completion&&x.owner.stage()==c::BoomshotStage::completion_commit);
  c::BoomshotInspectionExit exit{};assert(x.owner.take_inspection_exit(exit));c::BoomshotOwnerEvents::OriginalEvent finished{};
  assert(x.events.original(clock_value+1000000,std::int64_t(x.f.key.frame),finished)==c::BoomshotOwnerEvents::Result::accepted);
  assert(x.owner.acknowledge_inspection({finished.stamp(),exit.token,finished.ordinal(),x.owner.inventory(),true,true,c::ReloadTxnResult::none,true})==c::BoomshotEvent::inspection_completed);
  assert(x.owner.inventory().loaded==1&&x.owner.inventory().reserve==2);++cases;
 }
 for(unsigned bad=0;bad<10;++bad){Owned x;assert(x.begin());x.next();hand_q=bone_q=.5;auto frame=x.frame();
  switch(bad){case 0:++x.f.controllers.sequence;break;case 1:++frame.stamp.game_tick;break;
   case 2:frame.left_tracked=false;break;case 3:frame.right_hand_equipped=false;break;
   case 4:++frame.observed.revision;break;case 5:frame.original_owns_hands=true;break;
   case 6:++x.d.lease.id;break;case 7:++x.f.controllers.profile_generation;break;
   case 8:clock_value+=100000001;break;case 9:x.owner.interrupt();break;}
  const auto before=boomshot_barrel::observations;auto result=x.bridge.observe(x.f,x.d,x.route.grant(),x.owner,x.input,frame);
  assert(result.physical==m::Result::rejected&&!x.bridge.active()&&x.owner.step_index()==0&&boomshot_barrel::observations==before);++cases;
 }
 {Owned x;assert(!x.begin(m::Direction::closing)&&!x.bridge.active());++cases;}
 {Owned x;is_gt=false;assert(!x.begin()&&!x.bridge.active());is_gt=true;++cases;}
 {Owned x;wrong_begin=true;assert(x.begin());x.halfway();auto result=x.endpoint(1);
  assert(result.physical==m::Result::interrupted&&x.owner.step_index()==0&&!x.bridge.active());++cases;}
 {Owned x;assert(x.begin());x.halfway();x.next();hand_q=bone_q=1;assert(x.observe().physical==m::Result::moved);
  x.next();wrong_end=true;auto result=x.observe();assert(result.physical==m::Result::interrupted&&x.owner.step_index()==0);++cases;}
 {Owned x;assert(x.begin());x.halfway();x.next();hand_q=bone_q=1;assert(x.observe().physical==m::Result::moved);
  x.next();drop_receipt=true;auto result=x.observe();assert(result.physical==m::Result::interrupted&&x.owner.step_index()==0);++cases;}
 {Owned x;assert(x.begin());x.halfway();x.bridge.release(x.owner);assert(!x.bridge.active()&&x.owner.step_index()==0&&x.owner.stage()==c::BoomshotStage::mechanism);++cases;}
 {Owned x;assert(x.begin());x.halfway();x.bridge.release(x.owner);x.next();++x.f.press_token;
  auto& left=x.f.controllers.hands[0];++left.pressed[h::left_control(x.f)];++left.released[h::left_control(x.f)];x.grant(c::BoomshotContact::opening);
  assert(!x.begin());assert(x.owner.observe(x.frame())==c::BoomshotEvent::motion_reset&&x.begin());
  const auto result=x.endpoint(1);assert(result.event==c::BoomshotEvent::step_completed&&x.owner.step_index()==1);++cases;}
 {Owned x;assert(x.begin());x.next();hand_q=bone_q=.5;x.owner.tracking_lost();
  const auto calls=boomshot_barrel::observations;const auto result=x.observe();
  assert(result.physical==m::Result::interrupted&&boomshot_barrel::observations==calls&&x.owner.step_index()==0);++cases;}
 std::printf("Boomshot GT motion bridge: %u cases passed; real grant/kernel/core/event composition, once-only endpoint event, no raw/core sequence substitution, original handback; engine/Rig boundaries synthetic only\n",cases);return 0;
}
