#define main original_boomshot_regressions
#include "weapon_boomshot_interaction_test.cpp"
#undef main
#include "weapon_boomshot_owner_events.h"
#include <cassert>
#include <cstdio>
namespace {
using Events=BoomshotOwnerEvents;using Result=Events::Result;using Input=Events::InputEvent;
constexpr std::int64_t origin_ns=1000000000;
BoomshotInstanceConfig config(){auto c=boom_config();c.maximum_sample_gap_ns=100000000;return c;}
Input input(Events& events,unsigned raw,std::int64_t tick,std::int64_t sync=0){
 if(!sync)sync=origin_ns+20000000LL*raw;Input out{};
 assert(events.input({raw,1,9,1,sync,tick},sync+1000000,out)==Result::accepted);return out;
}
BoomshotFrame frame(BoomshotInteraction const& core,Input const& in){return {in.stamp(),core.inventory(),true,true,true,true,true,false};}
BoomshotPlan inspection_plan(){auto p=boom_plan();p.plan_id=16;p.condition=BoomshotCondition::inspection;p.count=2;p.steps[1]=p.steps[2];return p;}
BoomshotMotionReceipt motion(BoomshotInteraction const& core,Input const& began,std::uint64_t serial,bool inspect=false){
 const auto plan=inspect?inspection_plan():boom_plan();auto const& step=plan.steps[core.step_index()];
 return {core.binding_token(),serial,plan.plan_id,step.phase_id,step.geometry_id,began.stamp().motion_epoch,
  began.stamp().sequence,began.stamp().monotonic_ns,true,true,true,true,true};
}
BoomshotOriginalReceipt receipt(BoomshotInteraction const& core,Events::OriginalEvent const& event,
 std::uint64_t token,BoomshotInventory inventory,ReloadTxnResult result=ReloadTxnResult::none){
 (void)core;return {event.stamp(),token,event.ordinal(),inventory,true,true,result,true};
}
}
int main(){assert(original_boomshot_regressions()==0);unsigned count{};
 {Events events;Input out{};Events::OriginalEvent ack{};
  assert(events.input({1,1,9,1,origin_ns,0},origin_ns,out)==Result::unbound&&out.stamp().sequence==0);
  assert(events.original(origin_ns,0,ack)==Result::unbound);++count;}
 {Events events;assert(events.bind(boom_id,1));auto first=input(events,100,0);
  assert(first.raw_sequence()==100&&first.stamp().sequence==1&&first.stamp().monotonic_ns==origin_ns+20000000LL*100);
  Input out{};assert(events.input({100,1,9,1,first.stamp().monotonic_ns,0},first.stamp().monotonic_ns,out)==Result::replayed);
  assert(!out.stamp().sequence);++count;}
 for(unsigned reason=0;reason<10;++reason){Events events;assert(events.bind(boom_id,1));auto good=input(events,1,10);(void)good;
  Events::Input source{2,1,9,1,origin_ns+40000000,20};auto now=source.sync_ns;
  switch(reason){case 0:source.sequence=0;break;case 1:source.input_epoch=0;break;case 2:source.motion_epoch=0;break;
   case 3:source.profile_generation=0;break;case 4:source.sync_ns=0;break;case 5:source.game_tick=9;break;
   case 6:now=source.sync_ns-1;break;case 7:now=source.sync_ns+100000001;break;
   case 8:source.sequence=1;break;case 9:source.sync_ns=origin_ns;break;}
  Input out{};assert(events.input(source,now,out)!=Result::accepted&&!out.stamp().sequence);++count;
 }
 {Events events;assert(events.bind(boom_id,1));auto a=input(events,1,0);Input b{},c{},d{};
  auto ns=a.stamp().monotonic_ns+20000000;
  assert(events.input({1,2,9,1,ns,0},ns,b)==Result::accepted&&b.stamp().motion_epoch>a.stamp().motion_epoch);
  ns+=20000000;assert(events.input({1,2,9,2,ns,0},ns,c)==Result::accepted&&c.stamp().motion_epoch>b.stamp().motion_epoch);
  ns+=20000000;assert(events.input({2,2,10,2,ns,0},ns,d)==Result::accepted&&d.stamp().motion_epoch>c.stamp().motion_epoch);++count;}
 {BoomshotInteraction core;Events events;assert(core.bind_inspection(config().inspection(),{boom_id,1,1,2}));
  assert(events.bind(boom_id,core.binding_token()));auto a=input(events,1,10);assert(core.observe(frame(core,a))==BoomshotEvent::motion_reset);
  auto b=input(events,2,20);BoomshotInspectionStart start{core.binding_token(),101,inspection_plan(),true,true};
  assert(core.begin_inspection(frame(core,b),start,1)==BoomshotEvent::started);
  auto c=input(events,3,30);assert(core.complete_mechanism(frame(core,c),motion(core,b,101,true))==BoomshotEvent::step_completed);
  auto d=input(events,4,40);assert(core.complete_mechanism(frame(core,d),motion(core,c,101,true))==BoomshotEvent::request_completion);
  BoomshotInspectionExit exit{};assert(core.take_inspection_exit(exit));Events::OriginalEvent done{};
  const auto observed=d.stamp().monotonic_ns+5000000;assert(events.original(observed,40,done)==Result::accepted);
  assert(core.acknowledge_inspection(receipt(core,done,exit.token,core.inventory()))==BoomshotEvent::inspection_completed);
  Input late{};const auto old_input=d.stamp().monotonic_ns+1000000;
  assert(events.input({5,1,9,1,old_input,40},observed,late)==Result::awaiting_new_input&&!late.stamp().sequence);
  auto fresh=input(events,6,40,d.stamp().monotonic_ns+20000000);
  assert(fresh.stamp().monotonic_ns==d.stamp().monotonic_ns+20000000&&fresh.raw_sequence()==6&&fresh.stamp().sequence==done.stamp().sequence+1);
  assert(core.observe(frame(core,fresh))==BoomshotEvent::motion_reset&&core.inventory().loaded==1&&core.inventory().reserve==2);++count;
 }
 {BoomshotInteraction core;Events events;assert(core.bind(config(),{boom_id,1,0,11})&&events.bind(boom_id,core.binding_token()));
  auto a=input(events,1,0);core.observe(frame(core,a));auto b=input(events,2,10);auto start=boom_start(core.binding_token());
  assert(core.begin_reload(frame(core,b),start,1,ReloadIntentSource::mechanism_open)==BoomshotEvent::started);
  auto c=input(events,3,20);assert(core.complete_mechanism(frame(core,c),motion(core,b,1))==BoomshotEvent::step_completed);
  auto d=input(events,4,30);assert(core.grab_left_shoulder(frame(core,d),boom_carrier())==BoomshotEvent::grabbed);
  auto seated=input(events,5,40);assert(core.insert_ammunition(frame(core,seated),{motion(core,d,1),100,true,true,true})==BoomshotEvent::request_inventory);
  BoomshotInteraction::ReloadRequest request{};assert(core.take_reload_request(request));Events::OriginalEvent inserted{};
  assert(events.original(seated.stamp().monotonic_ns+5000000,50,inserted)==Result::accepted);
  auto expected=request.expected;BoomshotInventory inventory{boom_id,request.before.revision+1,expected.loaded,expected.reserve,expected.infinite_reserve};
  assert(core.acknowledge_inventory(receipt(core,inserted,request.token,inventory))==BoomshotEvent::inventory_committed);
  auto close_begin=input(events,6,4000009);core.observe(frame(core,close_begin));auto closed=input(events,7,4000010);
  assert(core.complete_mechanism(frame(core,closed),motion(core,close_begin,1))==BoomshotEvent::request_completion);
  assert(core.take_reload_request(request)&&request.result==ReloadTxnResult::perfect);Events::OriginalEvent finished{};
  assert(events.original(closed.stamp().monotonic_ns+5000000,14000010,finished)==Result::accepted&&finished.ordinal()>inserted.ordinal());
  assert(core.acknowledge_completion(receipt(core,finished,request.token,core.inventory(),request.result))==BoomshotEvent::reload_completed);
  assert(core.inventory().loaded==1&&core.inventory().reserve==10);++count;
 }
 {Events events;assert(events.bind(boom_id,1));auto a=input(events,1,0);Events::OriginalEvent first{},repeat{};
  assert(events.original(a.stamp().monotonic_ns+1,0,first)==Result::accepted);
  assert(events.original(first.stamp().monotonic_ns,0,repeat)==Result::replayed&&!repeat.ordinal());
  assert(!events.bind(boom_id,1)&&events.bind(boom_id,2));auto b=input(events,2,0);
  assert(b.stamp().binding==2&&b.stamp().sequence>first.stamp().sequence&&b.stamp().monotonic_ns>first.stamp().monotonic_ns);++count;
 }
 std::printf("Boomshot owner events: %u cases plus original core regressions; physical and original observation clocks separated, late input waits unchanged, continuity/replay and inclusive four-second physical completion preserved; synthetic only\n",count);
}
