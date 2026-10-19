#include "owned_ui_ledger.h"
#include <cstdio>
#include <cstdlib>
#include <memory>
unsigned checks{};
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
using gears_owned_ui::Ledger;
using gears_owned_ui::Kind;
using gears_owned_ui::TransferProof;
void bind(Ledger& l,uint64_t ticket=1){CHECK(l.bind(ticket,10,20,30,1));CHECK(l.begin_flush(10,20,30,40,2)==ticket);}
void legacy_cases(){
 {auto storage=std::make_unique<Ledger>();auto& l=*storage;
  CHECK(l.can_bind(10,20,30));CHECK(!l.can_bind(0,20,30));bind(l);
  CHECK(!l.can_bind(10,20,30));CHECK(l.can_bind(11,20,30));
  CHECK(l.expect(1,1));CHECK(l.produce(1,100,3));CHECK(l.close(1,4));
  CHECK(!l.ready(1));CHECK(l.can_bind(10,20,30)); // Closed producer; GPU/consumer need not be ready.
  const auto before=l.snapshot().counts;CHECK(l.can_bind(10,20,30));
  const auto after=l.snapshot().counts;CHECK(before.bound==after.bound&&before.rejected==after.rejected);
  CHECK(l.bind(2,10,20,30,5));CHECK(!l.can_bind(10,20,30));}
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
 {auto storage=std::make_unique<Ledger>();auto& l=*storage;bind(l);CHECK(l.expect(1,0));CHECK(!l.ready(1));
  CHECK(l.close(1,3));CHECK(l.ready(1));CHECK(l.published(1,4));CHECK(!l.ready(1));}
 {auto storage=std::make_unique<Ledger>();auto& l=*storage;bind(l);CHECK(l.expect(1,1));auto id=l.produce(1,100,3);
  CHECK(l.transferred(id,true));CHECK(l.close(1,4));CHECK(!l.ready(1));
  gears_owned_ui::Execution e;CHECK(!l.execution(id,e)&&!e.id);CHECK(!l.execution(id+1,e));
  CHECK(l.execute(100,20,5)==id);CHECK(l.execution(id,e));
  CHECK(e.id==id&&e.ticket==1&&e.canvas==10&&e.target==20&&e.payload==100);
  CHECK(!l.ready(1));CHECK(l.complete(id,true,6));CHECK(!l.execution(id,e)&&!e.id);CHECK(l.ready(1));
  CHECK(l.invalidate(1));CHECK(!l.ready(1));CHECK(l.published(1,7));CHECK(!l.execution(id,e));}
 {auto storage=std::make_unique<Ledger>();auto& l=*storage;bind(l);CHECK(l.expect(1,1));auto id=l.produce(1,100,3);
  CHECK(l.execute(100,20,4)==id);CHECK(l.complete(id,true,5));CHECK(l.close(1,6));CHECK(!l.ready(1));
  CHECK(l.transferred(id,true));CHECK(l.ready(1));CHECK(!l.transferred(id,true));CHECK(!l.ready(1));}
 {auto storage=std::make_unique<Ledger>();auto& l=*storage;bind(l);CHECK(l.expect(1,1));CHECK(l.expect(1,1));
  auto id=l.produce(1,100,3);CHECK(l.transferred(id,true));CHECK(l.execute(100,20,4)==id);CHECK(l.complete(id,true,5));
  CHECK(l.close(1,6));CHECK(!l.ready(1));CHECK(l.snapshot().active[0].expected_batches==2);}
 {auto storage=std::make_unique<Ledger>();auto& l=*storage;bind(l);CHECK(l.expect(1,UINT64_MAX));CHECK(!l.expect(1,1));
  CHECK(l.snapshot().active[0].invalid);CHECK(l.close(1,3));CHECK(!l.ready(1));}
 {auto storage=std::make_unique<Ledger>();auto& l=*storage;CHECK(l.bind(1,10,20,30,1));CHECK(!l.expect(1,0));
  CHECK(l.snapshot().active[0].invalid);CHECK(!l.ready(1));CHECK(!l.expect(99,0));CHECK(!l.invalidate(99));}
 {auto storage=std::make_unique<Ledger>();auto& l=*storage;bind(l);CHECK(l.expect(1,0));CHECK(l.close(1,3));CHECK(l.ready(1));
  CHECK(!l.expect(1,0));CHECK(!l.ready(1));}
 {auto storage=std::make_unique<Ledger>();auto& l=*storage;bind(l);CHECK(l.expect(1,1));auto id=l.produce(1,100,3);
  CHECK(!l.transferred(id,false));CHECK(l.transferred(id,true));CHECK(l.execute(100,20,4)==id);
  gears_owned_ui::Execution e;CHECK(!l.execution(id,e));CHECK(l.complete(id,true,5));CHECK(l.close(1,6));CHECK(!l.ready(1));}
 {auto storage=std::make_unique<Ledger>();auto& l=*storage;bind(l);CHECK(l.expect(1,1));auto id=l.produce(1,100,3);
  CHECK(l.transferred(id,true));CHECK(l.execute(100,20,4)==id);CHECK(l.complete(id,false,5));CHECK(l.close(1,6));CHECK(!l.ready(1));}
 {auto storage=std::make_unique<Ledger>();auto& l=*storage;bind(l);CHECK(l.expect(1,1));auto id=l.produce(1,100,3);
  CHECK(l.execute(100,20,4)==id);CHECK(l.invalidate(1));gears_owned_ui::Execution e;CHECK(!l.execution(id,e));
  CHECK(l.complete(id,true,5));CHECK(l.transferred(id,true));CHECK(l.close(1,6));CHECK(!l.ready(1));}
}
void material_cases(){
 // Material may execute and free its payload before producer return. Its
 // dispatch proof deliberately does not require the Batch item-clear contract.
 for(bool early:{false,true}){
  auto storage=std::make_unique<Ledger>();auto& l=*storage;bind(l);
  CHECK(l.expect(1,1));CHECK(l.expect(1,1,Kind::Material));
  auto batch=l.produce(1,100,3);auto material=l.produce(1,101,3,Kind::Material,21);CHECK(batch&&material);
  CHECK(l.transferred(batch,true));CHECK(l.execute(100,20,4)==batch);CHECK(l.complete(batch,true,5));
  if(!early)CHECK(l.transferred(material,TransferProof::MaterialDispatched));
  CHECK(l.execute(101,21,6,Kind::Material)==material);
  gears_owned_ui::Execution e{};CHECK(l.execution(material,e));
  CHECK(e.kind==Kind::Material&&e.target==21&&e.canvas==10&&e.ticket==1&&e.payload==101);
  CHECK(l.complete(material,true,7,Kind::Material));CHECK(l.close(1,8));
  CHECK(l.ready(1)==!early);
  if(early){CHECK(l.transferred(material,TransferProof::MaterialDispatched));CHECK(l.ready(1));}
  auto c=l.snapshot().active[0];CHECK(c.produced==2&&c.transferred==2&&c.completed==2&&c.expected_batches==2);
  for(auto const& k:c.by_kind)CHECK(k.expected==1&&k.produced==1&&k.transferred==1&&k.executed==1&&k.completed==1&&!k.failed);
  CHECK(l.published(1,9));CHECK(l.snapshot().active_count==0);
 }
 // Equal aggregate counts cannot replace a missing receipt of another kind.
 {
  auto storage=std::make_unique<Ledger>();auto& l=*storage;bind(l);CHECK(l.expect(1,1));CHECK(l.expect(1,1,Kind::Material));
  for(uintptr_t payload:{100u,101u}){auto id=l.produce(1,payload,3);CHECK(id);CHECK(l.transferred(id,true));CHECK(l.execute(payload,20,4)==id);CHECK(l.complete(id,true,5));}
  CHECK(l.close(1,6));CHECK(!l.ready(1));
  auto c=l.snapshot().active[0];CHECK(c.produced==c.expected_batches&&c.completed==c.produced);
 }
 // Payload uniqueness spans kinds and completed-but-not-retired receipts.
 for(bool old_material:{false,true})for(bool completed:{false,true}){
  auto storage=std::make_unique<Ledger>();auto& l=*storage;bind(l);
  auto old_kind=old_material?Kind::Material:Kind::Batch;auto new_kind=old_material?Kind::Batch:Kind::Material;
  auto id=l.produce(1,100,3,old_kind,21);CHECK(id);CHECK(l.expect(1,1,old_kind));
  if(completed){CHECK(l.execute(100,21,4,old_kind)==id);CHECK(l.complete(id,true,5,old_kind));}
  CHECK(l.close(1,6));bind(l,2);CHECK(!l.produce(2,100,7,new_kind,22));
  auto s=l.snapshot();CHECK(s.active_count==2&&s.active[0].ambiguous_payload&&s.active[1].ambiguous_payload);
  CHECK(!l.ready(1)&&!l.ready(2));CHECK(!l.execute(100,21,8,old_kind));
 }
 // Wrong consumer kind/target, wrong proof, failure, and duplicate lifecycle
 // events never make the CPU completion gate ready.
 for(unsigned misuse=0;misuse<9;++misuse){
  auto storage=std::make_unique<Ledger>();auto& l=*storage;bind(l);CHECK(l.expect(1,1,Kind::Material));
  auto id=l.produce(1,100,3,Kind::Material,21);CHECK(id);
  if(misuse==0)CHECK(!l.execute(100,21,4));
  if(misuse==1)CHECK(!l.execute(100,20,4,Kind::Material));
  if(misuse==2)CHECK(!l.transferred(id,true)); // Legacy bool is Batch only.
  if(misuse==3)CHECK(!l.transferred(id,TransferProof::Rejected));
  CHECK(l.execute(100,21,5,Kind::Material)==id);
  if(misuse==4)CHECK(!l.complete(id,true,6));
  CHECK(l.complete(id,misuse!=5,7,Kind::Material));
  CHECK(l.transferred(id,TransferProof::MaterialDispatched));
  if(misuse==6)CHECK(!l.transferred(id,TransferProof::MaterialDispatched));
  if(misuse==7)CHECK(!l.complete(id,true,8,Kind::Material));
  if(misuse==8)CHECK(!l.execute(100,21,8,Kind::Material));
  CHECK(l.close(1,9));CHECK(!l.ready(1));
  auto c=l.snapshot().active[0];CHECK(misuse==5?c.failed==1&&c.by_kind[1].failed==1:c.invalid);
 }
 {
  auto storage=std::make_unique<Ledger>();auto& l=*storage;bind(l);CHECK(l.expect(1,1,Kind::Material));
  CHECK(!l.produce(1,100,3,Kind::Material));CHECK(l.snapshot().active[0].invalid); // Explicit producer target mandatory.
 }
 {
  auto storage=std::make_unique<Ledger>();auto& l=*storage;bind(l);auto id=l.produce(1,100,3);CHECK(id);
  CHECK(!l.transferred(id,TransferProof::MaterialDispatched));CHECK(l.snapshot().active[0].invalid);
 }
 std::printf("owned UI ledger: %u checks passed\n",checks);
}
void discard_cases(){
 for(bool material:{false,true}){
  auto storage=std::make_unique<Ledger>();auto& l=*storage;CHECK(l.cpu_drained());bind(l);
  const auto kind=material?Kind::Material:Kind::Batch;
  CHECK(l.expect(1,1,kind));auto id=l.produce(1,100,3,kind,20);CHECK(id);
  CHECK(!l.cpu_drained()&&!l.discard_completed(1,4));
  CHECK(l.execute(100,20,4,kind)==id);CHECK(l.complete(id,true,5,kind));
  CHECK(!l.cpu_drained()&&!l.discard_completed(1,6)); // Missing producer transfer.
  CHECK(l.transferred(id,material?TransferProof::MaterialDispatched:TransferProof::BatchCleared));
  CHECK(!l.cpu_drained()&&!l.discard_completed(1,6)); // Missing Canvas close.
  CHECK(l.close(1,7));CHECK(l.cpu_drained());
  auto before=l.snapshot();CHECK(l.cpu_drained());auto after=l.snapshot();
  CHECK(before.active_count==after.active_count&&before.counts.retired==after.counts.retired&&after.active[0].ticket==1);
  CHECK(!l.discard_completed(2,8)&&!l.discard_completed(0,8)&&!l.discard_completed(1,6));
  CHECK(l.discard_completed(1,8));auto s=l.snapshot();
  CHECK(!s.active_count&&s.retained_count==1&&s.counts.discarded==1&&s.counts.retired==1&&!s.counts.published);
  CHECK(s.retained[0].discarded&&s.retained[0].discard_qpc==8&&!s.retained[0].published&&!s.retained[0].publication_qpc);
  CHECK(!s.counts.consumed_before_completion&&!s.counts.consumed_before_closure&&l.cpu_drained());
  CHECK(!l.published(1,9)&&!l.discard_completed(1,9)&&!l.complete(id,true,9,kind));
  CHECK(!l.bind(1,10,20,30,10));bind(l,2);CHECK(l.expect(2,0));CHECK(l.close(2,11));CHECK(l.discard_completed(2,12));
  CHECK(l.snapshot().counts.discarded==2&&l.cpu_drained());
 }
 // Every older cycle must drain; a newer completed cycle cannot cover it.
 {auto storage=std::make_unique<Ledger>();auto& l=*storage;bind(l);CHECK(l.expect(1,1));auto id=l.produce(1,100,3);CHECK(id);
  CHECK(l.transferred(id,true));CHECK(l.close(1,4));bind(l,2);CHECK(l.expect(2,0));CHECK(l.close(2,5));
  CHECK(!l.cpu_drained());CHECK(l.discard_completed(2,6));CHECK(!l.cpu_drained()&&!l.discard_completed(1,6));
  CHECK(l.execute(100,20,7)==id);CHECK(l.complete(id,true,8));CHECK(l.cpu_drained());CHECK(l.discard_completed(1,9));}
 // Equal aggregate counts do not cover missing Material inventory.
 {auto storage=std::make_unique<Ledger>();auto& l=*storage;bind(l);CHECK(l.expect(1,1));CHECK(l.expect(1,1,Kind::Material));
  for(uintptr_t p:{100u,101u}){auto id=l.produce(1,p,3);CHECK(l.transferred(id,true));CHECK(l.execute(p,20,4)==id);CHECK(l.complete(id,true,5));}
  CHECK(l.close(1,6));CHECK(!l.cpu_drained()&&!l.discard_completed(1,7));CHECK(l.snapshot().active_count==1);}
 for(unsigned fault=0;fault<4;++fault){
  auto storage=std::make_unique<Ledger>();auto& l=*storage;bind(l);CHECK(l.expect(1,1));auto id=l.produce(1,100,3);CHECK(id);
  CHECK(l.transferred(id,true));CHECK(l.execute(100,20,4)==id);CHECK(l.complete(id,fault!=0,5));
  if(fault==1)CHECK(l.invalidate(1));
  if(fault==2)CHECK(!l.produce(1,100,6));
  if(fault==3)CHECK(!l.transferred(id,true));
  CHECK(l.close(1,7));CHECK(!l.cpu_drained()&&!l.discard_completed(1,8));CHECK(l.snapshot().active_count==1);
 }
 // Already published retirement cannot be relabeled as an explicit discard.
 {auto storage=std::make_unique<Ledger>();auto& l=*storage;bind(l);CHECK(l.expect(1,0));CHECK(l.close(1,3));CHECK(l.published(1,4));
  CHECK(!l.discard_completed(1,5)&&!l.snapshot().counts.discarded&&l.cpu_drained());}
 std::printf("owned UI discard/drain: %u total checks passed\n",checks);
}
int main(){legacy_cases();material_cases();discard_cases();}
