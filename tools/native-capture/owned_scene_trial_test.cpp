#include "owned_scene_trial.h"
#include <cstdio>
#include <cstdlib>
unsigned checks{};
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
using gears_owned_scene::Trial;
using gears_frame_link::ListRef;
Trial selected(){Trial t;CHECK(t.ready(100,200,7));CHECK(t.select(300,8,200));
 CHECK(t.construction(300,8,true,200));CHECK(t.begin(300,8));return t;}
int main(){
 {Trial t;CHECK(!t.candidate());CHECK(!t.ready(0,200,7));CHECK(!t.ready(100,0,7));CHECK(!t.ready(100,200,0));
  CHECK(t.ready(100,200,7));CHECK(t.candidate());CHECK(!t.ready(100,200,8));CHECK(!t.select(300,7,200));
  CHECK(!t.select(0,8,200));CHECK(!t.select(300,8,201));CHECK(t.select(300,8,200));CHECK(!t.candidate());
  CHECK(!t.select(301,9,200));CHECK(!t.construction(300,8,true,201));CHECK(t.error==1);}
 {auto t=selected();CHECK(t.begin(301,8));CHECK(t.begin(300,9));CHECK(!t.begin(300,8));CHECK(t.error==2);}
 {auto t=selected();ListRef members[]={{10,1},{11,2}};
  t.draw(999,{10,1},8,0,true);CHECK(!t.draws&&!t.error);
  t.draw(100,{10,1},8,0,true);t.draw(100,{11,2},8,1,true);CHECK(t.draw_mask==3&&!t.submitted_mask);
  auto r=t.prepare(400,true,members,2);CHECK(r.count==2&&!t.submitted_mask);
  CHECK(t.commit(r,[](uintptr_t id){return ListRef{id,id==10?1u:2u};}));
  CHECK(t.submitted_mask==3&&t.queue==400&&t.submissions==1);t.retire(300);t.end(300,8);
  CHECK(t.destroyed&&t.returned&&!t.error);}
 for(unsigned fault=0;fault<5;++fault){auto t=selected();t.draw(100,{10,fault==0?0u:1u},fault==1?9:8,fault==2?2:0,fault!=3);
  if(fault==4){auto another=selected();another.entered=false;another.draw(100,{10,1},8,0,true);CHECK(another.unknown_draws==1&&!another.error);}
  else if(fault==0)CHECK(t.error==3);else CHECK(t.unknown_draws==1&&!t.error);}
 {auto t=selected();ListRef m{10,1};t.draw(100,m,8,0,true);CHECK(!t.prepare(400,false,&m,1).count&&t.error==5);}
 {auto t=selected();ListRef m[]={{10,1},{10,1}};t.draw(100,m[0],8,0,true);CHECK(!t.prepare(400,true,m,2).count&&t.error==6);}
 {auto t=selected();ListRef m{10,1};t.draw(100,m,8,0,true);auto r=t.prepare(400,true,&m,1);
  CHECK(!t.commit(r,[](uintptr_t id){return ListRef{id,2};}));CHECK(!t.submitted_mask&&t.error==7);}
 {auto t=selected();ListRef m{10,1};t.draw(100,m,8,0,true);auto r=t.prepare(400,true,&m,1);t.draw(100,m,8,1,true);
  CHECK(!t.commit(r,[](uintptr_t id){return ListRef{id,1};}));CHECK(!t.submitted_mask&&t.error==7);}
 {auto t=selected();ListRef m{10,2};t.draw(100,{10,1},8,0,true);CHECK(!t.prepare(400,true,&m,1).count);CHECK(!t.submitted_mask);}
 {auto t=selected();ListRef m{10,1};t.draw(100,m,8,0,true);auto outer=t.prepare(400,true,&m,1);
  auto inner=t.prepare(401,true,&m,1);auto current=[](uintptr_t id){return ListRef{id,1};};
  CHECK(t.commit(inner,current));CHECK(!t.commit(outer,current));CHECK(t.queue==401&&t.error==5&&t.submissions==1);}
 {auto t=selected();for(uintptr_t i=1;i<=9;++i)t.draw(100,{i,1},8,0,true);CHECK(t.error==4);}
 for(unsigned missing=0;missing<8;++missing){auto t=selected();ListRef m{10,1};
  t.draw(100,m,8,0,true);t.draw(100,m,8,1,true);auto receipt=t.prepare(400,true,&m,1);
  CHECK(t.commit(receipt,[](uintptr_t id){return ListRef{id,1};}));t.end(300,8);t.retire(300);CHECK(t.complete());
  if(missing==0)t.returned=false;if(missing==1)t.destroyed=false;if(missing==2)t.unknown_draws=1;
  if(missing==3)t.submitted_mask=1;if(missing==4)t.draws=3;if(missing==5)t.error=1;
  CHECK(!t.advance(missing==6?9:8,missing!=7));CHECK(t.ticket==8&&!t.completed);
 }
 {auto t=selected();ListRef m{10,1};t.draw(100,m,8,0,true);t.draw(100,m,8,1,true);
  auto old=t.prepare(400,true,&m,1);CHECK(t.commit(old,[](uintptr_t id){return ListRef{id,1};}));
  t.end(300,8);t.retire(300);CHECK(t.advance(8,true));
  CHECK(t.candidate()&&t.completed==1&&t.last_ticket==8&&t.born==7&&t.resource==100&&t.adapter==200);
  CHECK(!t.select(300,8,200));CHECK(t.select(300,9,200)); // Same address, a new ticket.
  CHECK(t.construction(300,9,true,200));CHECK(t.begin(300,9));
  t.draw(100,m,9,0,true);t.draw(100,m,9,1,true);
  CHECK(!t.commit(old,[](uintptr_t id){return ListRef{id,1};}));CHECK(t.error==11&&!t.submitted_mask);
 }
 std::printf("Owned scene trial: %u checks passed; no GPU completion assertion\n",checks);
}
