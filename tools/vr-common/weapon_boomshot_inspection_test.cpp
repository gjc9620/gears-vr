#define main existing_boomshot_regressions
#include "weapon_boomshot_interaction_test.cpp"
#undef main
#include <cassert>
#include <cstdio>
namespace {
constexpr BoomshotPlan inspection_plan(){auto p=boom_plan();p.plan_id=16;p.condition=BoomshotCondition::inspection;p.count=2;p.steps[1]=p.steps[2];return p;}
constexpr BoomshotInspectionStart inspection_start(BoomshotInteraction const& w){return {w.binding_token(),101,inspection_plan(),true,true};}
constexpr BoomshotMotionReceipt inspection_motion(BoomshotInteraction const& w,std::uint64_t began,unsigned epoch=1){
 auto p=inspection_plan();const auto step=p.steps[w.step_index()];
 return {w.binding_token(),101,p.plan_id,step.phase_id,step.geometry_id,epoch,began,static_cast<std::int64_t>(began*10),true,true,true,true,true};}
BoomshotOriginalReceipt exit_ack(BoomshotInteraction const& w,BoomshotInspectionExit const& request,unsigned sequence){
 return {live_frame(w,sequence).stamp,request.token,sequence,w.inventory(),true,true,ReloadTxnResult::none,true};}
bool setup(BoomshotInteraction& w,unsigned loaded=0,unsigned reserve=0,bool infinite=false){
 if(!w.bind(boom_config(),{boom_id,1,loaded,reserve,infinite}))return false;
 w.observe(live_frame(w,1));return w.begin_inspection(live_frame(w,2),inspection_start(w),1)==BoomshotEvent::started;}
bool close(BoomshotInteraction& w){return w.complete_mechanism(live_frame(w,3),inspection_motion(w,2))==BoomshotEvent::step_completed&&
 w.stage()==BoomshotStage::mechanism&&w.step_index()==1&&
 w.complete_mechanism(live_frame(w,4),inspection_motion(w,3))==BoomshotEvent::request_completion;}
}
int main(){assert(existing_boomshot_regressions()==0);unsigned cases_count{};
 for(unsigned variant=0;variant<3;++variant){BoomshotInteraction w;
  const unsigned loaded=variant?1:0,reserve=variant==1?11:0;assert(setup(w,loaded,reserve,variant==2));
  assert(!w.can_request_fire()&&!w.held_units());assert(w.grab_left_shoulder(live_frame(w,3),boom_carrier())==BoomshotEvent::rejected);
  assert(close(w));BoomshotInteraction::ReloadRequest inventory_request{};assert(!w.take_reload_request(inventory_request));
  BoomshotInspectionExit q{},again{};assert(w.take_inspection_exit(q)&&q.binding==w.binding_token()&&q.animation_serial==101);
  assert(!w.take_inspection_exit(again)&&!w.can_request_fire());
  assert(w.acknowledge_inspection(exit_ack(w,q,5))==BoomshotEvent::inspection_completed);
  assert(w.inventory().loaded==loaded&&w.inventory().reserve==reserve&&w.inventory().revision==1&&w.inventory().infinite_reserve==(variant==2));
  assert(w.acknowledge_inspection(exit_ack(w,q,6))==BoomshotEvent::rejected&&w.stage()==BoomshotStage::ready);++cases_count;}
 for(unsigned reason=0;reason<12;++reason){BoomshotInteraction w;assert(w.bind(boom_config(),{boom_id,1,0,0}));w.observe(live_frame(w,1));
  auto s=inspection_start(w);auto f=live_frame(w,2);std::uint64_t intent=1;
  switch(reason){case 0:s.original_gun_animation_owned=false;break;case 1:s.native_reload_absent=false;break;case 2:s.animation_serial=0;break;
   case 3:++s.binding;break;case 4:s.plan=boom_plan();break;case 5:f.observed.reserve=1;break;case 6:f.left_tracked=false;break;
   case 7:f.original_owns_hands=true;break;case 8:++f.stamp.identity.weapon;break;case 9:intent=0;break;
   case 10:s.plan.steps[1].units=1;break;case 11:s.plan.steps[1].phase_id=s.plan.steps[0].phase_id;break;}
  assert(w.begin_inspection(f,s,intent)!=BoomshotEvent::started);++cases_count;}
 {BoomshotInteraction w;assert(!setup(w,0,1));++cases_count;}
 {BoomshotInteraction w;assert(!setup(w,0,0,true));++cases_count;}
 {BoomshotInteraction w;assert(setup(w)&&close(w));BoomshotInspectionExit q{};assert(w.take_inspection_exit(q));
  auto bad=exit_ack(w,q,5);bad.result_applied=ReloadTxnResult::perfect;
  assert(w.acknowledge_inspection(bad)==BoomshotEvent::interrupted&&w.inventory().loaded==0);
  assert(!w.take_inspection_exit(q));assert(w.acknowledge_inspection(exit_ack(w,q,6))==BoomshotEvent::inspection_completed);
  assert(w.stage()==BoomshotStage::interrupted&&w.resume_ready(live_frame(w,7),true,true));++cases_count;}
 {BoomshotInteraction w;assert(setup(w));assert(w.complete_mechanism(live_frame(w,3),inspection_motion(w,2))==BoomshotEvent::step_completed);
  assert(w.interrupt()==BoomshotEvent::interrupted);auto f=live_frame(w,4);f.stamp.motion_epoch=2;
  assert(w.resume(f,2,0,1,false));f=live_frame(w,5);f.stamp.motion_epoch=2;assert(w.observe(f)==BoomshotEvent::motion_reset);
  f=live_frame(w,6);f.stamp.motion_epoch=2;assert(w.complete_mechanism(f,inspection_motion(w,5,2))==BoomshotEvent::request_completion);++cases_count;}
 {BoomshotInteraction w;assert(setup(w)&&close(w));assert(w.interrupt()==BoomshotEvent::interrupted);BoomshotInspectionExit q{};
  assert(w.take_inspection_exit(q)&&!w.resume(live_frame(w,5),2,0,2,true));
  assert(w.acknowledge_inspection(exit_ack(w,q,5))==BoomshotEvent::inspection_completed);
  assert(w.resume_ready(live_frame(w,6),true,true));++cases_count;}
 {BoomshotInteraction w;assert(setup(w)&&close(w));BoomshotInspectionExit q{};assert(w.take_inspection_exit(q));
  auto bad=exit_ack(w,q,5);++bad.observed.loaded;assert(w.acknowledge_inspection(bad)==BoomshotEvent::interrupted);++cases_count;}
 {BoomshotInteraction w;assert(setup(w)&&close(w));BoomshotInspectionExit q{};assert(w.take_inspection_exit(q));
  assert(w.acknowledge_inspection(exit_ack(w,q,5))==BoomshotEvent::inspection_completed);assert(w.reset()&&w.bind(boom_config(),{boom_id,1,0,0}));
  w.observe(live_frame(w,1));auto s=inspection_start(w);
  assert(w.begin_inspection(live_frame(w,2),s,2)==BoomshotEvent::rejected);++s.animation_serial;
  assert(w.begin_inspection(live_frame(w,3),s,2)==BoomshotEvent::started);++cases_count;}
 // A mechanism-only certificate cannot silently acquire the full fire/reload
 // certificate. Exercise the actual entry points before, during and after use.
 for(unsigned variant=0;variant<3;++variant){BoomshotInteraction w;
  const unsigned loaded=variant?1:0,reserve=variant==1?11:0;const bool infinite=variant==2;
  auto full=boom_config();full.original_fire_semantics_verified=false;
  full.ammunition_type=0;full.original_projectile_count=0;full.after_shot={};
  assert(!full.valid()&&full.inspection().valid());
  assert(!w.bind(full,{boom_id,1,loaded,reserve,infinite}));
  assert(w.bind_inspection(full.inspection(),{boom_id,1,loaded,reserve,infinite})&&w.inspection_only());
  const auto deny_authority=[&](unsigned sequence){auto f=live_frame(w,sequence);f.original_can_fire=true;
   BoomshotFireRequest shot{};BoomshotInteraction::ReloadRequest reload{};
   assert(w.request_original_fire(f,sequence)==BoomshotEvent::rejected&&!w.can_request_fire()&&!w.take_fire_request(shot));
   assert(w.begin_reload(f,boom_start(w.binding_token()),sequence,ReloadIntentSource::mechanism_open)==BoomshotEvent::rejected);
   assert(w.begin_r3(f,boom_start(w.binding_token()))==BoomshotEvent::rejected);
   assert(w.grab_left_shoulder(f,boom_carrier())==BoomshotEvent::rejected&&!w.held_units());
   assert(!w.take_reload_request(reload)&&!w.dispatched_reload_request(reload));
   BoomshotOriginalReceipt forged{f.stamp,99,sequence,{boom_id,2,1,0},true,true,ReloadTxnResult::perfect,true};
   assert(w.after_original_shot({forged,1,true})==BoomshotEvent::rejected);
   assert(w.acknowledge_inventory(forged)==BoomshotEvent::rejected&&w.acknowledge_completion(forged)==BoomshotEvent::rejected);
   assert(w.inventory().loaded==loaded&&w.inventory().reserve==reserve&&w.inventory().revision==1&&w.inventory().infinite_reserve==infinite);
  };
  deny_authority(1);w.observe(live_frame(w,1));
  assert(w.begin_inspection(live_frame(w,2),inspection_start(w),1)==BoomshotEvent::started);
  deny_authority(3);assert(close(w));deny_authority(5);
  BoomshotInspectionExit q{};assert(w.take_inspection_exit(q));
  assert(w.acknowledge_inspection(exit_ack(w,q,5))==BoomshotEvent::inspection_completed);
  w.observe(live_frame(w,6));deny_authority(7);
  const auto old_binding=w.binding_token();assert(w.reset()&&!w.inspection_only());
  assert(w.bind(boom_config(),{boom_id,1,1,11})&&!w.inspection_only()&&w.binding_token()>old_binding);
  w.observe(live_frame(w,1));auto f=live_frame(w,2);f.original_can_fire=true;
  assert(w.request_original_fire(f,1)==BoomshotEvent::request_fire);++cases_count;
 }
 for(unsigned reason=0;reason<12;++reason){BoomshotInteraction w;auto c=boom_config().inspection();BoomshotInventory inventory{boom_id,1,0,0};
  switch(reason){case 0:c.identity.world=0;break;case 1:c.identity.pawn=0;break;case 2:c.identity.weapon=0;break;
   case 3:c.identity.generation=0;break;case 4:c.config_id=0;break;case 5:c.capacity=2;break;
   case 6:c.maximum_sample_gap_ns=0;break;case 7:c.maximum_sample_gap_ns=100000001;break;
   case 8:c.current_instance_verified=false;break;case 9:c.original_class_verified=false;break;
   case 10:inventory.revision=0;break;case 11:inventory.loaded=2;break;}
  assert(!w.bind_inspection(c,inventory)&&w.stage()==BoomshotStage::unbound);++cases_count;
 }
 {BoomshotInteraction w;assert(w.bind_inspection(boom_config().inspection(),{boom_id,1,0,0}));
  w.observe(live_frame(w,1));assert(w.begin_inspection(live_frame(w,2),inspection_start(w),1)==BoomshotEvent::started);
  assert(close(w));BoomshotInspectionExit q{};assert(w.take_inspection_exit(q));
  assert(w.acknowledge_inspection(exit_ack(w,q,5))==BoomshotEvent::inspection_completed);
  const auto old_binding=w.binding_token();assert(w.reset()&&w.bind_inspection(boom_config().inspection(),{boom_id,1,0,0}));
  assert(w.binding_token()>old_binding);w.observe(live_frame(w,1));auto start=inspection_start(w);
  assert(w.begin_inspection(live_frame(w,2),start,2)==BoomshotEvent::rejected);++start.animation_serial;
  assert(w.begin_inspection(live_frame(w,3),start,1)==BoomshotEvent::rejected);
  assert(w.begin_inspection(live_frame(w,4),start,2)==BoomshotEvent::started);
  assert(w.acknowledge_inspection(exit_ack(w,q,5))==BoomshotEvent::rejected);++cases_count;
 }
 std::printf("Boomshot zero-target inspection: %u cases plus existing core regressions; restricted binding rejects fire/reload/inventory/reward, preserves nonces and explicitly returns animation ownership\n",cases_count);
}
