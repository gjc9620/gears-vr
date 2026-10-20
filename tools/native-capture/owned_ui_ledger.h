#pragma once
#include <array>
#include <cstdint>
#include <mutex>

namespace gears_owned_ui {
// CPU lifecycle/order gate only. GPU submission and resource identity must be
// certified independently before publication; readiness alone is insufficient.
enum class Kind : unsigned { Batch,Material };
// MaterialDispatched certifies the observed producer dispatch/return only;
// unlike BatchCleared it does not assert item+8 was cleared or GPU work ran.
enum class TransferProof : unsigned { BatchCleared,MaterialDispatched,Rejected };
struct KindCounts {uint64_t expected{},produced{},transferred{},executed{},completed{},failed{};};
struct Cycle {
 uint64_t ticket{};uintptr_t canvas{},target{},thread{},caller{};
 int64_t bind_qpc{},first_production_qpc{},last_completion_qpc{},closure_qpc{},publication_qpc{},discard_qpc{};
 uint64_t flushes{},expected_batches{},produced{},transferred{},completed{},failed{}; // Aggregates across both kinds.
 bool closed{},published{},published_before_completion{},published_before_closure{},ambiguous_payload{},invalid{},discarded{};
 std::array<KindCounts,2> by_kind{};
 std::array<uint64_t,2> inventory_calls{};
};
struct Execution {uint64_t id{},ticket{};uintptr_t canvas{},target{},payload{};Kind kind{Kind::Batch};};
struct Counters {
 uint64_t bound{},flushes{},produced{},transferred{},executed{},completed{},failed{},closed{},published{},retired{},discarded{};
 uint64_t consumed_before_completion{},consumed_before_closure{},retired_with_payload{},retired_empty{};
 uint64_t rejected{},binding_rejected{},payload_rejected{},capacity_rejected{},stale_rejected{},target_rejected{},transfer_rejected{};
 uint64_t kind_rejected{};std::array<KindCounts,2> by_kind{};
 std::array<uint64_t,2> inventory_calls{};
};
struct CpuRetirement {
 bool valid{};unsigned kind_mask{};uint64_t ticket_cutoff{},maximum_ticket{},next_receipt_id{},bound{},produced{},retired{},discarded{},serial{};
 int64_t qpc{};
};
struct Snapshot {
 Counters counts{};unsigned active_count{},retained_count{},retained_nonempty_count{};
 std::array<Cycle,256> active{};std::array<Cycle,32> retained{},retained_nonempty{};
};
class Ledger {
 struct Receipt {uint64_t id{},ticket{};uintptr_t payload{};bool transferred{},executing{},completed{},ambiguous{};
  Kind kind{Kind::Batch};uintptr_t expected_target{};};
 mutable std::mutex mutex_;
 std::array<Cycle,256> cycles_{};std::array<Cycle,32> retained_{},retained_nonempty_{};
 std::array<Receipt,1024> receipts_{};
 Counters counts_{};unsigned retained_count_{},retained_nonempty_count_{};uint64_t maximum_ticket_{},next_id_{};
 bool retirement_fault_{};
 std::array<bool,2> inventory_fault_{};
 uint64_t retirement_serial_{};
 uint64_t retirement_cutoff_{};unsigned retirement_kind_mask_{};int64_t retirement_qpc_{};
 bool reject(uint64_t& reason){if(&reason==&counts_.capacity_rejected)retirement_fault_=true;++counts_.rejected;++reason;return false;}
 void invalid(Cycle* c){if(c){c->invalid=true;retirement_fault_=true;}}
 Cycle* cycle(uint64_t ticket){for(auto& c:cycles_)if(c.ticket==ticket&&ticket)return &c;return nullptr;}
 Receipt* receipt(uint64_t id){for(auto& r:receipts_)if(r.id==id&&id)return &r;return nullptr;}
 static bool valid_kind(Kind kind){return unsigned(kind)<2;}
 static bool kinds_ready(Cycle const& c){
  for(auto const& k:c.by_kind)if(k.produced!=k.expected||k.transferred!=k.produced||k.completed!=k.produced||k.failed)return false;
  return true;
 }
 static void order_flags(Cycle& c){
  c.published_before_completion=c.published&&c.completed&&c.publication_qpc<c.last_completion_qpc;
  c.published_before_closure=c.published&&c.closed&&c.publication_qpc<c.closure_qpc;
 }
 bool cpu_cycle_drained(Cycle const& c)const{
  if(!c.closed||c.invalid||c.ambiguous_payload||c.failed||c.produced!=c.expected_batches||
     c.transferred!=c.produced||c.completed!=c.produced||!kinds_ready(c))return false;
  std::array<uint64_t,2> counted{};
  for(auto const& r:receipts_)if(r.id&&r.ticket==c.ticket){
   if(!valid_kind(r.kind)||!r.payload||!r.expected_target||!r.transferred||!r.executing||!r.completed||r.ambiguous)return false;
   ++counted[unsigned(r.kind)];
  }
  for(unsigned i=0;i<counted.size();++i)if(counted[i]!=c.by_kind[i].produced||c.by_kind[i].executed!=c.by_kind[i].produced)return false;
  return true;
 }
 void retire(Cycle& c){
  if(!c.closed||(!c.published&&!c.discarded)||c.transferred!=c.produced||c.completed!=c.produced)return;
  if(!c.flushes)retirement_fault_=true;
  for(unsigned i=0;i<2;++i)if(c.inventory_calls[i]!=c.flushes)inventory_fault_[i]=true;
  order_flags(c);if(retained_count_<retained_.size())retained_[retained_count_++]=c;
  if(c.published_before_completion)++counts_.consumed_before_completion;
  if(c.published_before_closure)++counts_.consumed_before_closure;
  if(c.produced){
   ++counts_.retired_with_payload;
   if(retained_nonempty_count_<retained_nonempty_.size())retained_nonempty_[retained_nonempty_count_++]=c;
  }else ++counts_.retired_empty;
  for(auto& r:receipts_)if(r.ticket==c.ticket)r={};
  c={};++counts_.retired;
 }
 bool retirement_ready_locked(uint64_t cutoff,int64_t qpc,unsigned kind_mask)const{
  if(cutoff<maximum_ticket_||qpc<=0||(kind_mask!=1&&kind_mask!=3)||retirement_fault_||counts_.failed||retirement_serial_==UINT64_MAX)return false;
  for(unsigned i=0;i<2;++i){
   if(kind_mask&(1u<<i)){if(inventory_fault_[i]||counts_.inventory_calls[i]!=counts_.flushes)return false;}
   else if(counts_.by_kind[i].produced||counts_.by_kind[i].expected)return false;
  }
  for(auto const& c:cycles_)if(c.ticket){
   if(c.ticket>cutoff||!c.flushes||c.published||c.discarded||!cpu_cycle_drained(c)||qpc<c.closure_qpc||qpc<c.last_completion_qpc)return false;
   for(unsigned i=0;i<2;++i)if((kind_mask&(1u<<i))&&c.inventory_calls[i]!=c.flushes)return false;
  }
  for(auto const& r:receipts_)if(r.id){bool owner=false;for(auto const& c:cycles_)if(c.ticket==r.ticket){owner=true;break;}
   if(!owner)return false;}
  return true;
 }
public:
 bool can_bind(uintptr_t canvas,uintptr_t target,uintptr_t thread)const{
  std::lock_guard<std::mutex> lock(mutex_);bool space=false;
  if(!canvas||!target||!thread||retirement_fault_)return false;
  for(auto const& c:cycles_){
   if(!c.ticket)space=true;
   if(c.ticket&&c.canvas==canvas&&c.thread==thread&&!c.closed)return false;
  }
  return space;
 }
 bool retirement_ready(uint64_t cutoff,int64_t qpc,unsigned kind_mask)const{
  std::lock_guard<std::mutex> lock(mutex_);return retirement_ready_locked(cutoff,qpc,kind_mask);
 }
 // Memory-only precommit stage. The caller first closes new production while
 // keeping queued consumer hooks admitted, and owns the world/ticket cutoff.
 // Preflight ALL cycles before discarding ANY; no partial prefix retirement.
 // This is strictly CPU evidence, never GPU/resource or UObject liveness proof.
 bool discard_completed_through(uint64_t cutoff,int64_t qpc,unsigned kind_mask,CpuRetirement& out){
  std::lock_guard<std::mutex> lock(mutex_);
  if(!retirement_ready_locked(cutoff,qpc,kind_mask))return false;
  for(auto& c:cycles_)if(c.ticket){c.discarded=true;c.discard_qpc=qpc;++counts_.discarded;retire(c);}
  CpuRetirement r{};r.valid=true;r.kind_mask=kind_mask;r.ticket_cutoff=cutoff;r.maximum_ticket=maximum_ticket_;
  r.next_receipt_id=next_id_;r.bound=counts_.bound;r.produced=counts_.produced;r.retired=counts_.retired;
  r.discarded=counts_.discarded;r.serial=++retirement_serial_;r.qpc=qpc;
  retirement_cutoff_=cutoff;retirement_kind_mask_=kind_mask;retirement_qpc_=qpc;out=r;return true;
 }
 bool retirement_current(CpuRetirement const& r)const{
  std::lock_guard<std::mutex> lock(mutex_);
  if(!r.valid||!r.serial||r.serial!=retirement_serial_||r.qpc<=0||(r.kind_mask!=1&&r.kind_mask!=3)||
     r.ticket_cutoff!=retirement_cutoff_||r.kind_mask!=retirement_kind_mask_||r.qpc!=retirement_qpc_||
     retirement_fault_||counts_.failed||maximum_ticket_!=r.maximum_ticket||maximum_ticket_>r.ticket_cutoff||next_id_!=r.next_receipt_id||
     counts_.bound!=r.bound||counts_.produced!=r.produced||counts_.retired!=r.retired||counts_.discarded!=r.discarded)return false;
  for(auto const& c:cycles_)if(c.ticket)return false;
  for(auto const& p:receipts_)if(p.id)return false;return true;
 }
 // Read-only CPU payload drain proof. Caller first closes new production and
 // serializes producer/consumer admission. No GPU completion or world token.
 // Cover ALL active tickets; the last ticket alone can hide an older payload.
 bool cpu_drained()const{
  std::lock_guard<std::mutex> lock(mutex_);
  for(auto const& c:cycles_)if(c.ticket&&!cpu_cycle_drained(c))return false;
  for(auto const& r:receipts_)if(r.id){bool owner=false;
   for(auto const& c:cycles_)if(c.ticket==r.ticket&&c.ticket){owner=true;break;}
   if(!owner)return false;
  }
  return true;
 }
 // Explicitly abandon one completed CPU cycle without claiming publication.
 // Caller owns the ticket/world cutoff and separately proves GPU retirement.
 // Unknown, pending, invalid or previously retired cycles never get erased.
 bool discard_completed(uint64_t ticket,int64_t qpc){
  std::lock_guard<std::mutex> lock(mutex_);auto c=cycle(ticket);
  if(!c||c->published||c->discarded||qpc<=0||qpc<c->closure_qpc||qpc<c->last_completion_qpc||
     !cpu_cycle_drained(*c))return reject(counts_.stale_rejected);
  c->discarded=true;c->discard_qpc=qpc;++counts_.discarded;retire(*c);return true;
 }
 bool bind(uint64_t ticket,uintptr_t canvas,uintptr_t target,uintptr_t thread,int64_t qpc){
  std::lock_guard<std::mutex> lock(mutex_);
  if(!ticket||!canvas||!target||!thread||ticket<=maximum_ticket_)return reject(counts_.binding_rejected);
  Cycle* empty{};for(auto& c:cycles_){
   if(c.ticket&&c.canvas==canvas&&c.thread==thread&&!c.closed)return reject(counts_.binding_rejected);
   if(!c.ticket&&!empty)empty=&c;
  }
  if(!empty)return reject(counts_.capacity_rejected);
  empty->ticket=ticket;empty->canvas=canvas;empty->target=target;empty->thread=thread;empty->bind_qpc=qpc;
  maximum_ticket_=ticket;++counts_.bound;return true;
 }
 uint64_t begin_flush(uintptr_t canvas,uintptr_t target,uintptr_t thread,uintptr_t caller,int64_t){
  std::lock_guard<std::mutex> lock(mutex_);Cycle* match{};
  for(auto& c:cycles_)if(c.ticket&&!c.closed&&c.canvas==canvas&&c.thread==thread){
   if(match){reject(counts_.binding_rejected);return 0;}match=&c;
  }
  if(!match){reject(counts_.binding_rejected);return 0;}
  if(!target||match->target!=target){reject(counts_.target_rejected);return 0;}
  match->caller=caller;++match->flushes;++counts_.flushes;return match->ticket;
 }
 bool close(uint64_t ticket,int64_t qpc){
  std::lock_guard<std::mutex> lock(mutex_);auto c=cycle(ticket);
  if(!c||c->closed)return reject(counts_.stale_rejected);
  c->closed=true;c->closure_qpc=qpc;++counts_.closed;order_flags(*c);retire(*c);return true;
 }
 // CPU completion gate only. Inventory includes every native item dispatch;
 // unsupported or missing receipts therefore cannot make this gate complete.
 bool expect(uint64_t ticket,uint64_t batch_count,Kind kind=Kind::Batch){
  std::lock_guard<std::mutex> lock(mutex_);auto c=cycle(ticket);
  if(!c)return reject(counts_.stale_rejected);
  if(!valid_kind(kind)){invalid(c);return reject(counts_.kind_rejected);}
  if(c->closed||c->published||!c->flushes||c->invalid){invalid(c);return reject(counts_.stale_rejected);}
  if(batch_count>UINT64_MAX-c->expected_batches||batch_count>UINT64_MAX-counts_.by_kind[unsigned(kind)].expected){invalid(c);return reject(counts_.capacity_rejected);}
  c->expected_batches+=batch_count;c->by_kind[unsigned(kind)].expected+=batch_count;
  counts_.by_kind[unsigned(kind)].expected+=batch_count;++counts_.inventory_calls[unsigned(kind)];++c->inventory_calls[unsigned(kind)];return true;
 }
 bool invalidate(uint64_t ticket){
  std::lock_guard<std::mutex> lock(mutex_);auto c=cycle(ticket);
  if(!c)return reject(counts_.stale_rejected);invalid(c);return true;
 }
 bool ready(uint64_t ticket)const{
  std::lock_guard<std::mutex> lock(mutex_);
  for(auto const& c:cycles_)if(ticket&&c.ticket==ticket)return !c.published&&c.closed&&!c.invalid&&!c.ambiguous_payload&&!c.failed&&
   c.produced==c.expected_batches&&c.transferred==c.produced&&c.completed==c.produced&&kinds_ready(c);
  return false;
 }
 bool execution(uint64_t id,Execution& out)const{
  std::lock_guard<std::mutex> lock(mutex_);out={};
  for(auto const& r:receipts_)if(id&&r.id==id&&r.executing&&!r.completed&&!r.ambiguous)
   for(auto const& c:cycles_)if(c.ticket==r.ticket&&!c.invalid&&!c.ambiguous_payload){
    out={r.id,r.ticket,c.canvas,r.expected_target,r.payload,r.kind};return true;
   }
  return false;
 }
 // expected_target is certified by the producer integration, never selected by
 // the consumer. Only legacy Batch may default to the bound Canvas target.
 uint64_t produce(uint64_t ticket,uintptr_t payload,int64_t qpc,Kind kind=Kind::Batch,uintptr_t expected_target=0){
  std::lock_guard<std::mutex> lock(mutex_);auto c=cycle(ticket);
  if(!c||c->closed||!c->flushes){reject(counts_.stale_rejected);return 0;}
  if(!valid_kind(kind)){invalid(c);reject(counts_.kind_rejected);return 0;}
  if(!expected_target){if(kind==Kind::Batch)expected_target=c->target;
   else {invalid(c);reject(counts_.target_rejected);return 0;}}
  if(!payload){reject(counts_.payload_rejected);return 0;}
  Receipt* empty{};for(auto& r:receipts_){
   if(r.id&&r.payload==payload){
    r.ambiguous=true;retirement_fault_=true;c->ambiguous_payload=true;if(auto previous=cycle(r.ticket))previous->ambiguous_payload=true;
    reject(counts_.payload_rejected);return 0;
   }
   if(!r.id&&!empty)empty=&r;
  }
  if(!empty||next_id_==UINT64_MAX){reject(counts_.capacity_rejected);return 0;}
  *empty={++next_id_,ticket,payload,false,false,false,false,kind,expected_target};if(!c->produced)c->first_production_qpc=qpc;
  ++c->produced;++counts_.produced;++c->by_kind[unsigned(kind)].produced;++counts_.by_kind[unsigned(kind)].produced;return empty->id;
 }
 bool transferred(uint64_t id,bool cleared){return transferred(id,cleared?TransferProof::BatchCleared:TransferProof::Rejected);}
 bool transferred(uint64_t id,TransferProof proof){
  std::lock_guard<std::mutex> lock(mutex_);auto r=receipt(id);
  if(!r)return reject(counts_.stale_rejected);
  if(r->transferred){invalid(cycle(r->ticket));return reject(counts_.stale_rejected);}
  if(!((r->kind==Kind::Batch&&proof==TransferProof::BatchCleared)||
       (r->kind==Kind::Material&&proof==TransferProof::MaterialDispatched))){
   invalid(cycle(r->ticket));return reject(counts_.transfer_rejected);}
  auto c=cycle(r->ticket);if(!c)return reject(counts_.stale_rejected);
  r->transferred=true;++c->transferred;++counts_.transferred;
  ++c->by_kind[unsigned(r->kind)].transferred;++counts_.by_kind[unsigned(r->kind)].transferred;retire(*c);return true;
 }
 uint64_t execute(uintptr_t payload,uintptr_t target,int64_t,Kind kind=Kind::Batch){
  std::lock_guard<std::mutex> lock(mutex_);Receipt* match{};
  for(auto& r:receipts_)if(r.id&&r.payload==payload){
   if(match){reject(counts_.payload_rejected);return 0;}match=&r;
  }
  if(match&&match->ambiguous){reject(counts_.payload_rejected);return 0;}
  if(!match||match->executing||match->completed){
   if(match)invalid(cycle(match->ticket));
   reject(counts_.stale_rejected);return 0;
  }
  auto c=cycle(match->ticket);
  if(!valid_kind(kind)||match->kind!=kind){invalid(c);reject(counts_.kind_rejected);return 0;}
  if(!c||!target||match->expected_target!=target){invalid(c);reject(counts_.target_rejected);return 0;}
  match->executing=true;++counts_.executed;++c->by_kind[unsigned(kind)].executed;++counts_.by_kind[unsigned(kind)].executed;return match->id;
 }
 bool complete(uint64_t id,bool ok,int64_t qpc,Kind kind=Kind::Batch){
  std::lock_guard<std::mutex> lock(mutex_);auto r=receipt(id);
  if(!r)return reject(counts_.stale_rejected);
  if(!valid_kind(kind)||r->kind!=kind){invalid(cycle(r->ticket));return reject(counts_.kind_rejected);}
  if(!r->executing||r->completed){invalid(cycle(r->ticket));return reject(counts_.stale_rejected);}
  auto c=cycle(r->ticket);if(!c)return reject(counts_.stale_rejected);
  r->completed=true;++c->completed;++counts_.completed;++c->by_kind[unsigned(kind)].completed;++counts_.by_kind[unsigned(kind)].completed;
  if(c->completed==1||qpc>c->last_completion_qpc)c->last_completion_qpc=qpc;
  if(!ok){++c->failed;++counts_.failed;++c->by_kind[unsigned(kind)].failed;++counts_.by_kind[unsigned(kind)].failed;}
  order_flags(*c);retire(*c);return true;
 }
 bool published(uint64_t ticket,int64_t qpc){
  std::lock_guard<std::mutex> lock(mutex_);auto c=cycle(ticket);
  if(!c)for(unsigned i=0;i<retained_count_;++i)if(retained_[i].ticket==ticket)c=&retained_[i];
  if(!c||c->published||c->discarded)return reject(counts_.stale_rejected);
  c->published=true;c->publication_qpc=qpc;++counts_.published;order_flags(*c);
  // Active cycles remain available for publication even beyond retained history.
  // A retained cycle is already published, so only active cycles reach here.
  retire(*c);return true;
 }
 Snapshot snapshot()const{
  std::lock_guard<std::mutex> lock(mutex_);Snapshot s;s.counts=counts_;s.retained_count=retained_count_;s.retained=retained_;
  s.retained_nonempty_count=retained_nonempty_count_;s.retained_nonempty=retained_nonempty_;
  for(auto const& c:cycles_)if(c.ticket)s.active[s.active_count++]=c;return s;
 }
};
}
