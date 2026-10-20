#include "owned_scene_pipeline.h"
#include <cassert>
#include <iostream>

using namespace gears_owned_scene;
namespace {
Pipeline prepared(){
 Pipeline p{};assert(p.active.ready(10,20,100));
 // Warm-pipeline fixture: an earlier output in this resource generation
 // already completed. Cold-generation ordering is exercised below.
 p.output_observed=true;
 assert(p.select(30,101,20,false));
 assert(p.construction(30,101,true,20));
 assert(p.select(40,102,20,true));
 assert(p.construction(40,102,true,20));return p;
}
void finish(Trial& t){
 assert(t.begin(t.renderer,t.ticket));
 const gears_frame_link::ListRef list{50,t.ticket};
 t.draw(t.resource,list,t.ticket,0,true);t.draw(t.resource,list,t.ticket,1,true);
 const auto receipt=t.prepare(60,true,&list,1);
 assert(t.commit(receipt,[&](uintptr_t){return list;}));
 t.end(t.renderer,t.ticket);t.retire(t.renderer);assert(t.complete());
}
// Compare every field, avoiding padding-dependent memcmp.
bool same(Trial const& a,Trial const& b){
 if(a.resource!=b.resource||a.adapter!=b.adapter||a.renderer!=b.renderer||a.queue!=b.queue||
    a.born!=b.born||a.ticket!=b.ticket||a.last_ticket!=b.last_ticket||a.completed!=b.completed||
    a.built!=b.built||a.entered!=b.entered||a.returned!=b.returned||a.destroyed!=b.destroyed||
    a.error!=b.error||a.draw_mask!=b.draw_mask||a.submitted_mask!=b.submitted_mask||
    a.draws!=b.draws||a.unknown_draws!=b.unknown_draws||a.submissions!=b.submissions)return false;
 for(unsigned i=0;i<a.rows.size();++i)
  if(a.rows[i].list.identity!=b.rows[i].list.identity||a.rows[i].list.generation!=b.rows[i].list.generation||
     a.rows[i].mask!=b.rows[i].mask||a.rows[i].draws!=b.rows[i].draws)return false;
 return true;
}
void unchanged(Pipeline& p,Promotion expected,uintptr_t renderer=40,uint64_t ticket=102,bool guard=true){
 const auto before=p;assert(p.promote(renderer,ticket,guard)==expected);
 assert(same(p.active,before.active)&&same(p.pending,before.pending));
}
}
int main(){
 {Pipeline p{};unchanged(p,Promotion::not_pending);}
 {
  Pipeline p{};assert(p.active.ready(10,20,100));
  assert(p.select(30,101,20,true));assert(p.construction(30,101,true,20));
  assert(!p.output_boundary(101));assert(!p.output_boundary(102));
  assert(!p.select(40,102,20,true));assert(!p.pending.ticket);
  finish(p.active);assert(!p.output_boundary(102));assert(!p.output_observed);
  assert(p.output_boundary(101)&&p.output_observed);
  assert(p.select(40,102,20,true));assert(p.construction(40,102,true,20));
  assert(p.promote(40,102,true)==Promotion::promoted);
  assert(p.active.ticket==102&&p.active.completed==1);
  // Use the same resource reset as native handoff; old warm-up cannot carry
  // across, while the cumulative completed count remains available.
  assert(p.reset_resource());
  assert(!p.output_observed&&!p.pending.ticket&&!p.active.resource&&p.active.completed==1);
  assert(p.active.ready(11,21,200));assert(p.select(50,201,21,true));
  assert(!p.select(60,202,21,true));
 }
 {
  auto p=prepared();unchanged(p,Promotion::not_ready);finish(p.active);
  unchanged(p,Promotion::not_ready,40,102,false);
  unchanged(p,Promotion::identity_mismatch,41);unchanged(p,Promotion::identity_mismatch,40,103);
  assert(p.promote(40,102,true)==Promotion::promoted);
  assert(p.active.ticket==102&&p.active.last_ticket==101&&p.active.completed==1);
  assert(p.active.built&&!p.active.entered&&!p.active.returned&&!p.active.destroyed);
  assert(!p.pending.resource&&!p.pending.ticket);
  assert(!p.select(50,102,20,true));assert(!p.pending.resource);
  assert(p.select(50,103,20,true));assert(p.construction(50,103,true,20));finish(p.active);
  assert(p.promote(50,103,true)==Promotion::promoted&&p.active.completed==2);
 }
 {
  // Once overlap is disabled after a consumer-thread move, the promoted frame
  // must retire before the next ordinary renderer can be selected.
  auto p=prepared();finish(p.active);
  assert(p.promote(40,102,true)==Promotion::promoted);
  finish(p.active);
  assert(p.active.advance(102,true));
  assert(!p.active.ticket&&!p.pending.ticket&&p.active.last_ticket==102);
  assert(p.select(50,103,20,false));
  assert(p.construction(50,103,true,20));
  finish(p.active);
  assert(p.active.advance(103,true)&&p.active.completed==3);
 }
 {
  auto p=prepared();finish(p.active);const auto before=p;
  assert(!p.reset_resource());assert(p.output_observed);
  assert(same(p.active,before.active)&&same(p.pending,before.pending));
  assert(!p.select(50,103,20,true));assert(same(p.active,before.active)&&same(p.pending,before.pending));
  assert(!p.construction(41,102,true,20));assert(same(p.pending,before.pending));
  assert(p.find(30,101)==&p.active&&p.find(40,102)==&p.pending&&!p.find(30,102));
  assert(p.find_renderer(40)==&p.pending&&!p.find_renderer(99));
  p.pending.renderer=30;assert(!p.find_renderer(30));
 }
 {
  auto p=prepared();finish(p.active);assert(!p.construction(40,102,false,20));
  unchanged(p,Promotion::not_ready);
  p=prepared();finish(p.active);assert(!p.construction(40,102,true,21));
  unchanged(p,Promotion::not_ready);
 }
 for(unsigned field=0;field<3;++field){
  auto p=prepared();finish(p.active);
  if(field==0)++p.pending.resource;if(field==1)++p.pending.adapter;if(field==2)++p.pending.born;
  unchanged(p,Promotion::identity_mismatch);
 }
 for(unsigned field=0;field<5;++field){
  auto p=prepared();finish(p.active);
  if(field==0)p.pending.built=false;if(field==1)p.pending.entered=true;
  if(field==2)p.pending.returned=true;if(field==3)p.pending.destroyed=true;
  if(field==4)p.active.completed=UINT64_MAX;
  unchanged(p,Promotion::not_ready);
 }
 {
  Pipeline p{};assert(p.active.ready(10,20,100));
  assert(!p.select(30,100,20,true));assert(!p.active.ticket);
  assert(p.select(30,101,20,false));
  assert(p.construction(30,101,true,20));finish(p.active);assert(p.output_boundary(101));
  const auto blank=p.pending;
  assert(!p.select(40,102,20,false));assert(!p.select(40,102,21,true));
  assert(!p.select(0,102,20,true));assert(!p.select(40,101,20,true));
  assert(same(p.pending,blank));
  assert(p.select(40,102,20,true));
 }
 std::cout<<"owned_scene_pipeline_test CPU_only passed\n";
}
