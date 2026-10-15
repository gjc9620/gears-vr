#pragma once
// Diagnostic association only. Native arguments, targets and publication remain
// unchanged. A ledger receipt is not a GPU/UI correctness certificate.
#if defined(GEARS_OWNED_UI_ROUTE_PROBE) && GEARS_OWNED_UI_ROUTE_PROBE
#include "owned_ui_ledger.h"
namespace owned_ui_live {
gears_owned_ui::Ledger ledger;
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
 if(ui_observer_enabled&&recording.load()&&family.canvas)
  ledger.bind(ticket,family.canvas,family.target,family.thread,now());
}
struct FlushRow {uint64_t ticket{};uintptr_t canvas{},target{},caller{};unsigned total{},batch{},material{},other{},read_ok{};};
std::array<FlushRow,32> flush_rows{};unsigned flush_rows_count{};std::mutex flush_rows_mutex;
void sample_flush(FlushContext const& context,uintptr_t caller){
 std::lock_guard<std::mutex> lock(flush_rows_mutex);if(flush_rows_count==flush_rows.size())return;
 auto& row=flush_rows[flush_rows_count++];row.ticket=context.ticket;row.canvas=context.canvas;row.target=context.target;row.caller=caller;
 gears_ui_batch::detail::Reader<decltype(&safe_copy)> read{safe_copy};gears_ui_batch::detail::Array elements{};
 if(!read.field(elements,context.canvas,0x48)||!gears_ui_batch::detail::valid(elements,4096,20)||elements.count>128)return;
 for(int i=0;i<elements.count;++i){gears_ui_batch::detail::Array items{};
  if(!read.field(items,elements.data,size_t(i)*20+4)||!gears_ui_batch::detail::valid(items,65536,8)||items.count>512)return;
  for(int j=0;j<items.count;++j){uintptr_t item{},table{};
   if(++row.total>4096||!read.field(item,items.data,size_t(j)*8)||!read.field(table,item,0))return;
   if(table==base+0x133cc98)++row.batch;else if(table==base+0x136c5f0)++row.material;else ++row.other;
  }
 }
 row.read_ok=1;
}
uintptr_t __fastcall hook_flush(void* canvas){
 Scope active_scope;const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress())-base;
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
   ledger.close(scope.ticket,now());
  else ++read_rejected;
 }
 flushing=saved;return result;
}
uint32_t __fastcall hook_produce(void* item,void* canvas){
 Scope active_scope;uint64_t id{};uintptr_t payload{},table{},target{},secondary{};
 if(ui_observer_enabled&&recording.load()){
  ++producer_calls;
  if(flushing.ticket&&flushing.canvas==reinterpret_cast<uintptr_t>(canvas)&&flushing.thread==GetCurrentThreadId()&&
     target_of(canvas,target,secondary)&&target==flushing.target&&!secondary&&item&&
     safe_copy(&table,item,8)&&table==base+0x133cc98&&safe_copy(&payload,static_cast<unsigned char*>(item)+8,8)&&payload)
   id=ledger.produce(flushing.ticket,payload,now());
  else ++producer_unassociated;
 }
 const auto result=original_produce(item,canvas);
 if(id){uintptr_t after{};const bool cleared=safe_copy(&after,static_cast<unsigned char*>(item)+8,8)&&!after;
  ledger.transferred(id,cleared);}
 return result;
}
uint64_t execute_begin(void* command){
 if(!ui_observer_enabled||!recording.load())return 0;
 std::array<uintptr_t,6> header{};
 if(!command||!safe_copy(header.data(),command,sizeof(header))||header[0]!=base+0x13394f0||!header[1]||header[5]){
  ++read_rejected;return 0;}
 auto id=ledger.execute(header[1],header[4],now());if(!id)++execute_unassociated;return id;
}
void execute_end(uint64_t id,uint32_t result){if(id)ledger.complete(id,result==48,now());}
bool reserve(){
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
 const auto s=ledger.snapshot();const auto& c=s.counts;
 fprintf(file,"owned_ui_route observed_only=1 bound=%llu flushes=%llu produced=%llu transferred=%llu executed=%llu completed=%llu failed=%llu closed=%llu consumed=%llu retired=%llu active=%u retained=%u rejected=%llu binding_rejected=%llu payload_rejected=%llu capacity_rejected=%llu stale_rejected=%llu target_rejected=%llu transfer_rejected=%llu read_rejected=%llu producer_unassociated=%llu execute_unassociated=%llu all_flushes=%llu all_producers=%llu\n",
  c.bound,c.flushes,c.produced,c.transferred,c.executed,c.completed,c.failed,c.closed,c.published,c.retired,s.active_count,s.retained_count,
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
 for(unsigned i=0;i<flush_rows_count;++i){const auto& r=flush_rows[i];
  fprintf(file,"owned_ui_flush ticket=%llu canvas=%llx target=%llx caller=%llx total=%u batch=%u material=%u other=%u read_ok=%u\n",
   r.ticket,r.canvas,r.target,r.caller,r.total,r.batch,r.material,r.other,r.read_ok);
 }
}
}
void owned_ui_published(uint64_t ticket){if(ui_observer_enabled)owned_ui_live::ledger.published(ticket,now());}
#else
namespace owned_ui_live {
struct FamilyScope {FamilyScope(void*,void*,bool){}};
void selected(uint64_t){}
uint64_t execute_begin(void*){return 0;}
void execute_end(uint64_t,uint32_t){}
}
void owned_ui_published(uint64_t){}
#endif
