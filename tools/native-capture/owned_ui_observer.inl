#pragma once
#include "owned_ui_world_retirement.h"
// CPU association is independent of GPU submission proof. Target substitution
// is enabled only by the separate, experimental GEARS_OWNED_UI_BATCH build.
#if defined(GEARS_OWNED_UI_ROUTE_PROBE) && GEARS_OWNED_UI_ROUTE_PROBE
#include "owned_ui_ledger.h"
namespace owned_ui_live {
gears_owned_ui::Ledger ledger;
#if defined(GEARS_OWNED_UI_BATCH) && GEARS_OWNED_UI_BATCH
constexpr bool routing=true;
#else
constexpr bool routing=false;
#endif
#if defined(GEARS_OWNED_UI_MATERIAL) && GEARS_OWNED_UI_MATERIAL
constexpr bool material_routing=true;
#else
constexpr bool material_routing=false;
#endif
std::atomic<uint64_t> route_commands{},route_draws{},route_faults{},route_plane_draws{},route_reticle_draws{};
std::atomic<unsigned> first_route_fault{};
struct DrawRejection {uint64_t ticket{},pair_ticket{};uintptr_t caller{},actual{},expected{};unsigned flags{},width{},height{};
 gears_viewport::DrawState state{};};
DrawRejection first_draw_rejection{};std::mutex draw_rejection_mutex;
void reject_draw(DrawRejection const& row){std::lock_guard<std::mutex> lock(draw_rejection_mutex);if(!first_draw_rejection.ticket)first_draw_rejection=row;}
void fault(uint64_t ticket,unsigned reason){
 if(!routing)return;
 ledger.invalidate(ticket);++route_faults;unsigned empty{};first_route_fault.compare_exchange_strong(empty,reason);
 request_session_stop(SessionStop::proof_failure);
}
using Flush=uintptr_t(__fastcall*)(void*);
using Produce=uint32_t(__fastcall*)(void*,void*);
Flush original_flush{};Produce original_produce{};
void* flush_target{};void* produce_target{};
std::atomic<uint64_t> read_rejected{},producer_unassociated{},execute_unassociated{},flush_calls{},producer_calls{};
struct Family {uintptr_t canvas{},target{};unsigned thread{};};
thread_local Family family;
struct FlushContext {uint64_t ticket{};uintptr_t canvas{},target{};unsigned thread{};};
thread_local FlushContext flushing;
bool target_of(void* canvas,uintptr_t& target,uintptr_t& secondary){
 std::array<unsigned char,20> data{};
 if(!canvas||!safe_copy(data.data(),canvas,data.size()))return false;
 memcpy(&target,data.data()+4,8);memcpy(&secondary,data.data()+12,8);return target!=0;
}
struct FamilyScope {
 Family saved=family;
 FamilyScope(void* canvas,void* native_family,bool certified){
  family={};uintptr_t target{},secondary{},expected{};
  if(!ui_observer_enabled||!recording.load()||!certified)return;
  if(!target_of(canvas,target,secondary)||secondary||!native_family||
     !safe_copy(&expected,static_cast<unsigned char*>(native_family)+0x10,8)||target!=expected){++read_rejected;return;}
  family={reinterpret_cast<uintptr_t>(canvas),target,GetCurrentThreadId()};
 }
 ~FamilyScope(){family=saved;}
};
void selected(uint64_t ticket){
 if(!ui_observer_enabled||!recording.load()||!family.canvas||
    !ledger.bind(ticket,family.canvas,family.target,family.thread,now()))fault(ticket,1);
}
struct FlushRow {uint64_t ticket{};uintptr_t canvas{},target{},caller{},first_other_table{};unsigned total{},batch{},material{},other{},read_ok{};};
std::array<FlushRow,32> flush_rows{};unsigned flush_rows_count{};std::mutex flush_rows_mutex;
FlushRow rejected_flush{};unsigned rejected_flush_reason{};
FlushRow inventory(FlushContext const& context,uintptr_t caller){
 FlushRow row{};row.ticket=context.ticket;row.canvas=context.canvas;row.target=context.target;row.caller=caller;
 gears_ui_batch::detail::Reader<decltype(&safe_copy)> read{safe_copy};gears_ui_batch::detail::Array elements{};
 if(!read.field(elements,context.canvas,0x48)||!gears_ui_batch::detail::valid(elements,4096,20)||elements.count>128)return row;
 for(int i=0;i<elements.count;++i){gears_ui_batch::detail::Array items{};
  if(!read.field(items,elements.data,size_t(i)*20+4)||!gears_ui_batch::detail::valid(items,65536,8)||items.count>512)return row;
  for(int j=0;j<items.count;++j){uintptr_t item{},table{};
   if(++row.total>4096||!read.field(item,items.data,size_t(j)*8)||!read.field(table,item,0))return row;
   if(table==base+0x133cc98)++row.batch;else if(table==base+0x136c5f0)++row.material;
   else {++row.other;if(!row.first_other_table)row.first_other_table=table;}
  }
 }
 row.read_ok=1;return row;
}
void sample_flush(FlushContext const& context,uintptr_t caller){
 std::lock_guard<std::mutex> lock(flush_rows_mutex);
 if(!routing&&flush_rows_count==flush_rows.size())return;
 const auto row=inventory(context,caller);
 if(flush_rows_count<flush_rows.size())flush_rows[flush_rows_count++]=row;
 if(routing){
  const unsigned reason=!row.read_ok?1:row.material&&!material_routing?2:row.other?3:
   !ledger.expect(context.ticket,row.batch)?4:
   material_routing&&!ledger.expect(context.ticket,row.material,gears_owned_ui::Kind::Material)?4:0;
  if(reason){if(!rejected_flush_reason){rejected_flush=row;rejected_flush_reason=reason;}fault(context.ticket,2);}
 }
}
uintptr_t __fastcall hook_flush(void* canvas){
 Scope active_scope;if(!active_scope.world())return original_flush(canvas);
 const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress())-base;
 const auto saved=flushing;flushing={};uintptr_t target{},secondary{};
 if(ui_observer_enabled&&recording.load()){
  ++flush_calls;
  if(target_of(canvas,target,secondary)&&!secondary){
   const auto thread=GetCurrentThreadId();auto ticket=ledger.begin_flush(reinterpret_cast<uintptr_t>(canvas),target,thread,caller,now());
   if(ticket){flushing={ticket,reinterpret_cast<uintptr_t>(canvas),target,thread};sample_flush(flushing,caller);}
  }else ++read_rejected;
 }
 const auto scope=flushing;
 const auto result=original_flush(canvas); // Preserve incidental RAX as well.
 if(scope.ticket&&caller==0x2a6b9f){
  uintptr_t after{},optional{};
  if(target_of(canvas,after,optional)&&after==scope.target&&!optional&&GetCurrentThreadId()==scope.thread)
   {if(!ledger.close(scope.ticket,now()))fault(scope.ticket,3);}
  else {++read_rejected;fault(scope.ticket,3);}
 }
 flushing=saved;return result;
}
uint32_t __fastcall hook_produce(void* item,void* canvas){
 Scope active_scope;if(!active_scope.world())return original_produce(item,canvas);
 uint64_t id{};uintptr_t payload{},table{},target{},secondary{};
 if(ui_observer_enabled&&recording.load()){
  ++producer_calls;
  if(flushing.ticket&&flushing.canvas==reinterpret_cast<uintptr_t>(canvas)&&flushing.thread==GetCurrentThreadId()&&
     target_of(canvas,target,secondary)&&target==flushing.target&&!secondary&&item&&
     safe_copy(&table,item,8)&&table==base+0x133cc98&&safe_copy(&payload,static_cast<unsigned char*>(item)+8,8)&&payload)
   id=ledger.produce(flushing.ticket,payload,now());
  else ++producer_unassociated;
  if(flushing.ticket&&!id)fault(flushing.ticket,4);
 }
 const auto result=original_produce(item,canvas);
 if(id){uintptr_t after{};const bool cleared=safe_copy(&after,static_cast<unsigned char*>(item)+8,8)&&!after;
  if(!ledger.transferred(id,cleared))fault(flushing.ticket,5);}
 return result;
}
uint64_t execute_begin(void* command){
 if(!ui_observer_enabled||!recording.load())return 0;
 std::array<uintptr_t,6> header{};
 if(!command||!safe_copy(header.data(),command,sizeof(header))||header[0]!=base+0x13394f0||!header[1]||header[5]){
  ++read_rejected;return 0;}
 auto id=ledger.execute(header[1],header[4],now());if(!id)++execute_unassociated;return id;
}
void execute_end(uint64_t id,uint32_t result,bool sequence_complete){
 if(!id)return;gears_owned_ui::Execution e{};ledger.execution(id,e);
 const bool recorded=ledger.complete(id,result==48&&sequence_complete,now());
 if(result!=48||!sequence_complete||!recorded)fault(e.ticket,6);
}
#if defined(GEARS_OWNED_UI_BATCH) && GEARS_OWNED_UI_BATCH
struct Route {uint64_t ticket{},receipt{};uintptr_t resource{};unsigned source_width{},source_height{};};
thread_local Route route;
struct CommandScope {
 Route saved=route;
 alignas(16) std::array<uintptr_t,6> header{};
 void* argument{};
 CommandScope(uint64_t receipt,void* original,uintptr_t caller):argument(original){
  route={};if(!receipt)return;
  gears_owned_ui::Execution e{};std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);
  const auto& trial=gears_owned_scene_live::trial;
  unsigned dimensions[2]{};
  if(!ledger.execution(receipt,e)||caller!=base+0x19f2f3||!safe_copy(header.data(),original,sizeof(header))||
     header[0]!=base+0x13394f0||header[1]!=e.payload||header[4]!=e.target||header[5]||
     !gears_owned_scene_live::healthy()||trial.ticket!=e.ticket||!trial.adapter||!trial.resource||!owned_scene_gpu::watching()){
   fault(e.ticket,7);return;
  }
  memcpy(dimensions,header.data()+2,sizeof(dimensions));
  if(!gears_native_render_size::width()||!gears_native_render_size::height()||
     !dimensions[0]||!dimensions[1]||dimensions[0]>8192||dimensions[1]>8192){fault(e.ticket,8);return;}
  // The checked Execute body uses only this 48-byte header; it destroys the
  // original payload exactly once. The native queued header stays untouched.
  header[4]=trial.adapter;argument=header.data();
  route={e.ticket,receipt,trial.resource,dimensions[0],dimensions[1]};++route_commands;
 }
 ~CommandScope(){route=saved;}
};
bool verify_execute(){
 BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};
 bool ok=BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
 if(ok)ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
 struct Chunk{uintptr_t rva;ULONG size;};
 const Chunk chunks[]={{0x19e64c,0x145},{0x8bee1e,0xa2}};
 std::array<unsigned char,0x145> bytes{};unsigned char digest[32]{};char hex[65]{};
 for(auto c:chunks)if(ok)ok=safe_copy(bytes.data(),reinterpret_cast<void*>(base+c.rva),c.size)&&BCryptHashData(hash,bytes.data(),c.size,0)>=0;
 if(ok)ok=BCryptFinishHash(hash,digest,32,0)>=0;
 for(unsigned i=0;i<32;++i)sprintf_s(hex+2*i,3,"%02x",digest[i]);
 if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
 return ok&&!strcmp(hex,"1b804155ae4801e5c554227420cf988fb994138d305d6d11c349295410f8be25");
}
#endif
bool reserve(){
#if defined(GEARS_OWNED_UI_BATCH) && GEARS_OWNED_UI_BATCH
 if(!verify_execute())return false;
#endif
 flush_target=reinterpret_cast<void*>(base+0x1172c4);produce_target=reinterpret_cast<void*>(base+0x117430);
 constexpr unsigned char flush_bytes[]={0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x6c,0x24,0x18,0x48,0x89,0x74,0x24,0x20,0x57};
 constexpr unsigned char produce_bytes[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x10,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56};
 constexpr unsigned char destructor_call[]={0xe8,0x25,0x07,0xe7,0xff};
 std::array<unsigned char,16> bytes{},checked{};uintptr_t entry{};
 if(!safe_copy(bytes.data(),flush_target,16)||memcmp(bytes.data(),flush_bytes,16)||
    !safe_copy(bytes.data(),produce_target,16)||memcmp(bytes.data(),produce_bytes,16)||
    !safe_copy(bytes.data(),reinterpret_cast<void*>(base+0x2a6b9a),5)||memcmp(bytes.data(),destructor_call,5)||
    !safe_copy(&entry,reinterpret_cast<void*>(base+0x133cc98),8)||entry!=base+0x117430)return false;
 if(MH_CreateHook(flush_target,reinterpret_cast<void*>(hook_flush),reinterpret_cast<void**>(&original_flush))!=MH_OK||
    MH_CreateHook(produce_target,reinterpret_cast<void*>(hook_produce),reinterpret_cast<void**>(&original_produce))!=MH_OK)return false;
 return safe_copy(bytes.data(),flush_target,16)&&!memcmp(bytes.data(),flush_bytes,16)&&
  safe_copy(checked.data(),produce_target,16)&&!memcmp(checked.data(),produce_bytes,16);
}
void print(FILE* file){
 {std::lock_guard<std::mutex> lock(draw_rejection_mutex);const auto& r=first_draw_rejection;if(r.ticket){const auto& s=r.state;
  fprintf(file,"owned_ui_draw_rejected ticket=%llu pair=%llu caller=%llx actual=%llx expected=%llx width=%u height=%u flags=%u tracked=%u vp_count=%u sc_count=%u depth_known=%u depth=%.9g,%.9g viewport=%.9g,%.9g,%.9g,%.9g scissor=%.9g,%.9g,%.9g,%.9g\n",
   r.ticket,r.pair_ticket,r.caller,r.actual,r.expected,r.width,r.height,r.flags,s.tracked,s.viewport_count,s.scissor_count,s.depth_known,s.min_depth,s.max_depth,
   s.viewport.x,s.viewport.y,s.viewport.width,s.viewport.height,s.scissor.x,s.scissor.y,s.scissor.width,s.scissor.height);}}
 fprintf(file,"owned_ui_batch enabled=%u commands=%llu draws=%llu plane=%llu reticle=%llu faults=%llu first_fault=%u\n",
  routing,route_commands.load(),route_draws.load(),route_plane_draws.load(),route_reticle_draws.load(),route_faults.load(),first_route_fault.load());
 const auto s=ledger.snapshot();const auto& c=s.counts;
 fprintf(file,"owned_ui_route observed_only=%u bound=%llu flushes=%llu produced=%llu transferred=%llu executed=%llu completed=%llu failed=%llu closed=%llu consumed=%llu retired=%llu active=%u retained=%u rejected=%llu binding_rejected=%llu payload_rejected=%llu capacity_rejected=%llu stale_rejected=%llu target_rejected=%llu transfer_rejected=%llu read_rejected=%llu producer_unassociated=%llu execute_unassociated=%llu all_flushes=%llu all_producers=%llu\n",
  routing?0:1,c.bound,c.flushes,c.produced,c.transferred,c.executed,c.completed,c.failed,c.closed,c.published,c.retired,s.active_count,s.retained_count,
  c.rejected,c.binding_rejected,c.payload_rejected,c.capacity_rejected,c.stale_rejected,c.target_rejected,c.transfer_rejected,
  read_rejected.load(),producer_unassociated.load(),execute_unassociated.load(),flush_calls.load(),producer_calls.load());
 fprintf(file,"owned_ui_retire_order empty=%llu with_payload=%llu consumed_before_completion=%llu consumed_before_close=%llu retained_nonempty=%u\n",
  c.retired_empty,c.retired_with_payload,c.consumed_before_completion,c.consumed_before_closure,s.retained_nonempty_count);
 auto cycle=[&](gears_owned_ui::Cycle const& r,unsigned sample_kind){
  fprintf(file,"owned_ui_cycle ticket=%llu retained=%u sample_kind=%u canvas=%llx target=%llx thread=%llu caller=%llx bound_qpc=%lld first_produce_qpc=%lld last_complete_qpc=%lld close_qpc=%lld consume_qpc=%lld flushes=%llu produced=%llu transferred=%llu completed=%llu failed=%llu closed=%u consumed=%u consumed_before_completion=%u consumed_before_close=%u ambiguous_payload=%u\n",
   r.ticket,unsigned(sample_kind!=0),sample_kind,r.canvas,r.target,uint64_t(r.thread),r.caller,r.bind_qpc,r.first_production_qpc,r.last_completion_qpc,r.closure_qpc,r.publication_qpc,
   r.flushes,r.produced,r.transferred,r.completed,r.failed,r.closed,r.published,r.published_before_completion,r.published_before_closure,r.ambiguous_payload);
 };
 for(unsigned i=0;i<s.retained_count;++i)cycle(s.retained[i],1);
 for(unsigned i=0;i<s.active_count;++i)cycle(s.active[i],0);
 for(unsigned i=0;i<s.retained_nonempty_count;++i)cycle(s.retained_nonempty[i],2);
 std::lock_guard<std::mutex> lock(flush_rows_mutex);
 if(rejected_flush_reason){const auto& r=rejected_flush;
  fprintf(file,"owned_ui_flush_rejected reason=%u ticket=%llu canvas=%llx target=%llx caller=%llx total=%u batch=%u material=%u other=%u read_ok=%u first_other_table=%llx module_base=%llx\n",
   rejected_flush_reason,r.ticket,r.canvas,r.target,r.caller,r.total,r.batch,r.material,r.other,r.read_ok,r.first_other_table,base);
 }
 for(unsigned i=0;i<flush_rows_count;++i){const auto& r=flush_rows[i];
  fprintf(file,"owned_ui_flush ticket=%llu canvas=%llx target=%llx caller=%llx total=%u batch=%u material=%u other=%u read_ok=%u\n",
   r.ticket,r.canvas,r.target,r.caller,r.total,r.batch,r.material,r.other,r.read_ok);
 }
}
}
void owned_ui_published(uint64_t ticket){if(ui_observer_enabled)owned_ui_live::ledger.published(ticket,now());}
void owned_ui_discard_completed(uint64_t ticket){
 if(ui_observer_enabled&&!owned_ui_live::ledger.discard_completed(ticket,now()))owned_ui_live::fault(ticket,44);
}
bool owned_ui_ready(uint64_t ticket){return !owned_ui_live::routing||owned_ui_live::ledger.ready(ticket);}
bool owned_ui_world_cpu_ready(gears_owned_ui_world::Guards guards){
 guards.route_fault=guards.route_fault||owned_ui_live::route_faults.load()!=0;
 return owned_ui_live::routing&&gears_owned_ui_world::cpu_ready(owned_ui_live::ledger,guards,now(),owned_ui_live::material_routing?3u:1u);
}
bool owned_ui_prepare_world_retirement(gears_owned_ui_world::Guards guards,gears_owned_ui_world::Receipt& out){
 guards.route_fault=guards.route_fault||owned_ui_live::route_faults.load()!=0;
 return owned_ui_live::routing&&gears_owned_ui_world::prepare_cpu(owned_ui_live::ledger,guards,now(),owned_ui_live::material_routing?3u:1u,out);
}
bool owned_ui_world_retirement_current(gears_owned_ui_world::Guards guards,gears_owned_ui_world::Receipt const& receipt){
 guards.route_fault=guards.route_fault||owned_ui_live::route_faults.load()!=0;
 return owned_ui_live::routing&&gears_owned_ui_world::current(owned_ui_live::ledger,guards,receipt);
}
#else
namespace owned_ui_live {
struct FamilyScope {FamilyScope(void*,void*,bool){}};
void selected(uint64_t){}
uint64_t execute_begin(void*){return 0;}
void execute_end(uint64_t,uint32_t,bool){}
}
void owned_ui_published(uint64_t){}
void owned_ui_discard_completed(uint64_t){}
bool owned_ui_ready(uint64_t){return true;}
bool owned_ui_world_cpu_ready(gears_owned_ui_world::Guards){return false;}
bool owned_ui_prepare_world_retirement(gears_owned_ui_world::Guards,gears_owned_ui_world::Receipt&){return false;}
bool owned_ui_world_retirement_current(gears_owned_ui_world::Guards,gears_owned_ui_world::Receipt const&){return false;}
#endif
