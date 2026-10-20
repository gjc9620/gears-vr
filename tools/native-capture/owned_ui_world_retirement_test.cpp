#include "owned_ui_world_retirement.h"
#include "game_ui_material.h"
#include <cstdio>
#include <cstdlib>
#include <memory>
using namespace gears_owned_ui;
namespace world=gears_owned_ui_world;
static unsigned checks{};
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %u: %s\n",unsigned(__LINE__),#x);std::exit(1);}}while(0)
static void bind(Ledger& ledger,uint64_t ticket){CHECK(ledger.bind(ticket,uintptr_t(ticket*0x1000),0x200,3,1));
 CHECK(ledger.begin_flush(uintptr_t(ticket*0x1000),0x200,3,4,2)==ticket);}
static uint64_t item(Ledger& ledger,uint64_t ticket,uintptr_t payload,Kind kind,bool complete=true){
 auto id=ledger.produce(ticket,payload,3,kind,0x200);CHECK(id);
 CHECK(ledger.transferred(id,kind==Kind::Batch?TransferProof::BatchCleared:TransferProof::MaterialDispatched));
 CHECK(ledger.execute(payload,0x200,4,kind)==id);if(complete)CHECK(ledger.complete(id,true,5,kind));return id;}
static world::Guards guards(uint64_t cutoff=20){world::Guards g{};g.old_world=7;g.ticket_cutoff=cutoff;g.production_closed=true;return g;}
static void readiness_and_guards(){
 auto store=std::make_unique<Ledger>();auto& ledger=*store;auto g=guards();world::Receipt receipt{};receipt.old_world=99;
 CHECK(world::cpu_ready(ledger,g,10,3));CHECK(!world::prepare_cpu(ledger,g,10,3,receipt)&&receipt.old_world==99);
 g.resources_retired=true;
 for(unsigned variant=0;variant<6;++variant){auto bad=g;
  if(variant==0)bad.old_world=0;if(variant==1)bad.production_closed=false;if(variant==2)bad.semantic_pending=1;
  if(variant==3)bad.material_pending=true;if(variant==4)bad.material_broken=true;if(variant==5)bad.route_fault=true;
  CHECK(!world::prepare_cpu(ledger,bad,10,3,receipt)&&receipt.old_world==99);}
 CHECK(world::prepare_cpu(ledger,g,10,3,receipt)&&world::current(ledger,g,receipt));
 {auto changed=receipt;changed.cpu.kind_mask=1;CHECK(!world::current(ledger,g,changed));
  changed=receipt;changed.cpu.qpc++;CHECK(!world::current(ledger,g,changed));
  changed=receipt;changed.cpu.ticket_cutoff++;auto moved=g;moved.ticket_cutoff++;CHECK(!world::current(ledger,moved,changed));}
 auto stale=receipt;CHECK(world::prepare_cpu(ledger,g,11,3,receipt));CHECK(!world::current(ledger,g,stale));
 g.old_world++;CHECK(!world::current(ledger,g,receipt));g.old_world--;g.ticket_cutoff++;CHECK(!world::current(ledger,g,receipt));
}
static void all_cycles_and_monotonicity(){
 auto store=std::make_unique<Ledger>();auto& ledger=*store;auto g=guards();g.resources_retired=true;
 bind(ledger,1);CHECK(ledger.expect(1,1));CHECK(ledger.expect(1,0,Kind::Material));auto first=item(ledger,1,0x400,Kind::Batch,false);
 CHECK(ledger.close(1,6));bind(ledger,2);CHECK(ledger.expect(2,0));CHECK(ledger.expect(2,1,Kind::Material));item(ledger,2,0x500,Kind::Material);CHECK(ledger.close(2,6));
 world::Receipt receipt{};receipt.old_world=99;CHECK(!world::cpu_ready(ledger,g,10,3));
 CHECK(!world::prepare_cpu(ledger,g,10,3,receipt)&&receipt.old_world==99);
 auto before=ledger.snapshot();CHECK(before.active_count==2&&!before.counts.discarded);
 CHECK(ledger.complete(first,true,8));CHECK(world::cpu_ready(ledger,g,10,3));
 CHECK(!world::prepare_cpu(ledger,g,7,3,receipt)&&ledger.snapshot().active_count==2);
 CHECK(world::prepare_cpu(ledger,g,10,3,receipt));auto retired=ledger.snapshot();
 CHECK(!retired.active_count&&retired.counts.discarded==2&&!retired.counts.published&&retired.counts.retired==2);
 CHECK(receipt.cpu.ticket_cutoff==20&&receipt.cpu.maximum_ticket==2&&world::current(ledger,g,receipt));
 CHECK(retired.retained[0].discarded&&!retired.retained[0].published&&!retired.retained[0].publication_qpc);
 CHECK(!ledger.bind(1,0x1000,0x200,3,20));bind(ledger,21);CHECK(!world::current(ledger,g,receipt));
 CHECK(ledger.expect(21,1));CHECK(ledger.expect(21,0,Kind::Material));auto fresh=item(ledger,21,0x400,Kind::Batch);CHECK(fresh>first);
 CHECK(ledger.close(21,22));CHECK(!world::prepare_cpu(ledger,g,23,3,receipt));g.old_world=8;g.ticket_cutoff=21;
 CHECK(world::prepare_cpu(ledger,g,23,3,receipt)&&world::current(ledger,g,receipt));
}
static void inventory_and_sticky_faults(){
 for(unsigned variant=0;variant<4;++variant){auto store=std::make_unique<Ledger>();auto& ledger=*store;auto g=guards();g.resources_retired=true;
  bind(ledger,1);CHECK(ledger.expect(1,0));if(variant!=0)CHECK(ledger.expect(1,0,Kind::Material));
  if(variant==1)CHECK(ledger.invalidate(1));CHECK(ledger.close(1,6));
  if(variant==2){CHECK(ledger.expect(1,0)==false);} // Invalid even if later retired by legacy publication.
  if(variant<3)CHECK(ledger.published(1,8));
  if(variant==3){ // Repeated inventory cannot impersonate another flush.
   bind(ledger,2);CHECK(ledger.expect(2,0));CHECK(ledger.expect(2,0));CHECK(ledger.expect(2,0,Kind::Material));CHECK(ledger.close(2,8));}
  world::Receipt output{};output.old_world=99;CHECK(!world::prepare_cpu(ledger,g,10,3,output)&&output.old_world==99);
 }
 // No inventory-free bound cycle is accepted as the allowed empty ledger.
 {auto store=std::make_unique<Ledger>();auto& ledger=*store;CHECK(ledger.bind(1,10,20,30,1));CHECK(ledger.close(1,2));
  CHECK(ledger.cpu_drained());CpuRetirement r{};CHECK(!ledger.discard_completed_through(1,3,3,r));}
 {auto store=std::make_unique<Ledger>();auto& ledger=*store;
  for(uint64_t t=1;t<=256;++t){bind(ledger,t);CHECK(ledger.expect(t,0));CHECK(ledger.expect(t,0,Kind::Material));CHECK(ledger.close(t,6));}
  CHECK(!ledger.bind(257,0x999,0x200,3,7)&&ledger.cpu_drained());CpuRetirement r{};
  CHECK(!ledger.discard_completed_through(256,10,3,r)&&ledger.snapshot().active_count==256);}
}
static void material_and_last_check(){
 auto store=std::make_unique<Ledger>();auto& ledger=*store;auto g=guards();g.resources_retired=true;
 gears_game_material::Completion material;CHECK(material.begin());g.material_pending=material.pending()!=0;g.material_broken=material.blocks_present();
 world::Receipt receipt{};CHECK(!world::prepare_cpu(ledger,g,10,3,receipt));CHECK(!material.end(false,false));
 g.material_pending=false;g.material_broken=material.blocks_present();CHECK(world::prepare_cpu(ledger,g,10,3,receipt));
 unsigned payloads=2,sidecars=3,bindings=4;auto clear=[&]{payloads=0;sidecars=0;bindings=0;};
 g.semantic_pending=1;if(world::current(ledger,g,receipt))clear();CHECK(payloads==2&&sidecars==3&&bindings==4);
 g.semantic_pending=0;CHECK(material.end(false,false));g.material_broken=material.blocks_present();
 CHECK(material.pending()==0&&g.material_broken&&!world::current(ledger,g,receipt));
 g.material_broken=false;if(world::current(ledger,g,receipt))clear();CHECK(!payloads&&!sidecars&&!bindings);
}
int main(){readiness_and_guards();all_cycles_and_monotonicity();inventory_and_sticky_faults();material_and_last_check();
 std::printf("OwnedUiWorldRetirementTest: %u checks passed\n",checks);}
