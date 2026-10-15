#include "owned_ui_ledger.h"
#include <cstdio>
#include <cstdlib>
#include <memory>
unsigned checks{};
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
using gears_owned_ui::Ledger;
void bind(Ledger& l,uint64_t ticket=1){CHECK(l.bind(ticket,10,20,30,1));CHECK(l.begin_flush(10,20,30,40,2)==ticket);}
int main(){
 {auto storage=std::make_unique<Ledger>();auto& l=*storage;bind(l);auto id=l.produce(1,100,3);CHECK(id);CHECK(l.transferred(id,true));CHECK(l.published(1,4));
  CHECK(l.close(1,5));CHECK(l.snapshot().active_count==1);CHECK(l.execute(100,20,6)==id);CHECK(l.complete(id,true,7));
  auto s=l.snapshot();CHECK(!s.active_count&&s.retained_count==1&&s.counts.retired==1);
  auto c=s.retained[0];CHECK(c.first_production_qpc==3&&c.last_completion_qpc==7&&c.closure_qpc==5);
  CHECK(c.published_before_completion&&c.published_before_closure);CHECK(!l.complete(id,true,8));CHECK(!l.transferred(id,true));}
 {auto storage=std::make_unique<Ledger>();auto& l=*storage;bind(l);auto id=l.produce(1,100,3);CHECK(l.execute(100,20,4)==id);CHECK(l.complete(id,false,5));
  CHECK(l.close(1,6));CHECK(l.snapshot().active_count==1);CHECK(!l.transferred(id,false));CHECK(l.transferred(id,true));
  CHECK(l.snapshot().active_count==1);CHECK(l.published(1,7));auto s=l.snapshot();CHECK(!s.active_count&&s.retained[0].failed==1);
  CHECK(!s.retained[0].published_before_completion);}
 {auto storage=std::make_unique<Ledger>();auto& l=*storage;bind(l);auto old=l.produce(1,100,3);CHECK(!l.bind(2,10,20,30,4));CHECK(l.close(1,5));bind(l,2);
  CHECK(l.snapshot().active_count==2);auto next=l.produce(2,101,6);CHECK(next>old);
  CHECK(l.transferred(old,true));CHECK(l.execute(100,20,7)==old);CHECK(l.complete(old,true,8));CHECK(l.published(1,8));CHECK(l.snapshot().active_count==1);
  CHECK(!l.execute(101,21,9));CHECK(l.execute(101,20,9)==next);CHECK(!l.execute(101,20,10));
  CHECK(l.complete(next,true,10));CHECK(l.transferred(next,true));CHECK(l.close(2,11));CHECK(l.published(2,11));CHECK(!l.execute(100,20,12));
  bind(l,3);auto fresh=l.produce(3,100,13);CHECK(fresh>next);CHECK(!l.complete(old,true,14));CHECK(!l.transferred(old,true));}
 {auto storage=std::make_unique<Ledger>();auto& l=*storage;CHECK(!l.bind(0,10,20,30,1));CHECK(!l.bind(1,10,20,0,1));bind(l);
  CHECK(!l.begin_flush(10,20,31,40,2));CHECK(!l.begin_flush(10,21,30,40,2));
  auto id=l.produce(1,100,3);CHECK(!l.produce(1,100,4));CHECK(!l.execute(100,20,4));
  CHECK(l.snapshot().active[0].ambiguous_payload);CHECK(!l.complete(id,true,5));CHECK(!l.produce(99,101,6));}
 {auto storage=std::make_unique<Ledger>();auto& l=*storage;for(uint64_t i=1;i<=256;++i)CHECK(l.bind(i,uintptr_t(i),20,30,1));CHECK(!l.bind(257,999,20,30,1));
  CHECK(l.snapshot().counts.capacity_rejected==1);}
 {auto storage=std::make_unique<Ledger>();auto& l=*storage;bind(l);for(uintptr_t i=1;i<=1024;++i)CHECK(l.produce(1,i,3));CHECK(!l.produce(1,1025,4));
  CHECK(l.snapshot().counts.capacity_rejected==1);CHECK(l.close(1,5));CHECK(l.snapshot().active_count==1);}
 {auto storage=std::make_unique<Ledger>();auto& l=*storage;for(uint64_t i=1;i<=40;++i){bind(l,i);CHECK(l.close(i,3));CHECK(l.snapshot().active_count==1);CHECK(l.published(i,4));}
  auto s=l.snapshot();CHECK(!s.active_count&&s.retained_count==32&&s.counts.retired==40);CHECK(s.retained[31].ticket==32);
  CHECK(s.counts.retired_empty==40&&!s.counts.retired_with_payload&&!s.retained_nonempty_count);
  bind(l,41);auto id=l.produce(41,100,5);CHECK(id);CHECK(l.transferred(id,true));CHECK(l.published(41,6));
  CHECK(l.execute(100,20,7)==id);CHECK(l.complete(id,true,8));CHECK(l.close(41,9));s=l.snapshot();
  CHECK(!s.active_count&&s.retained_count==32&&s.retained_nonempty_count==1);
  CHECK(s.retained_nonempty[0].ticket==41&&s.retained_nonempty[0].last_completion_qpc==8);
  CHECK(s.retained_nonempty[0].published_before_completion&&s.retained_nonempty[0].published_before_closure);
  CHECK(s.counts.retired==41&&s.counts.retired_empty==40&&s.counts.retired_with_payload==1);
  CHECK(s.counts.consumed_before_completion==1&&s.counts.consumed_before_closure==1);
  CHECK(!l.close(41,10));CHECK(l.snapshot().counts.consumed_before_completion==1);}
 std::printf("owned UI ledger: %u checks passed\n",checks);
}
