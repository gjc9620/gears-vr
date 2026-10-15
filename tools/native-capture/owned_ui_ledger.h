#pragma once
#include <array>
#include <cstdint>
#include <mutex>

namespace gears_owned_ui {
// CPU lifecycle/order gate only. GPU submission and resource identity must be
// certified independently before publication; readiness alone is insufficient.
struct Cycle {
 uint64_t ticket{};uintptr_t canvas{},target{},thread{},caller{};
 int64_t bind_qpc{},first_production_qpc{},last_completion_qpc{},closure_qpc{},publication_qpc{};
 uint64_t flushes{},expected_batches{},produced{},transferred{},completed{},failed{};
 bool closed{},published{},published_before_completion{},published_before_closure{},ambiguous_payload{},invalid{};
};
struct Execution {uint64_t id{},ticket{};uintptr_t canvas{},target{},payload{};};
struct Counters {
 uint64_t bound{},flushes{},produced{},transferred{},executed{},completed{},failed{},closed{},published{},retired{};
 uint64_t consumed_before_completion{},consumed_before_closure{},retired_with_payload{},retired_empty{};
 uint64_t rejected{},binding_rejected{},payload_rejected{},capacity_rejected{},stale_rejected{},target_rejected{},transfer_rejected{};
};
struct Snapshot {
 Counters counts{};unsigned active_count{},retained_count{},retained_nonempty_count{};
 std::array<Cycle,256> active{};std::array<Cycle,32> retained{},retained_nonempty{};
};
class Ledger {
 struct Receipt {uint64_t id{},ticket{};uintptr_t payload{};bool transferred{},executing{},completed{},ambiguous{};};
 mutable std::mutex mutex_;
 std::array<Cycle,256> cycles_{};std::array<Cycle,32> retained_{},retained_nonempty_{};
 std::array<Receipt,1024> receipts_{};
 Counters counts_{};unsigned retained_count_{},retained_nonempty_count_{};uint64_t maximum_ticket_{},next_id_{};
 bool reject(uint64_t& reason){++counts_.rejected;++reason;return false;}
 Cycle* cycle(uint64_t ticket){for(auto& c:cycles_)if(c.ticket==ticket&&ticket)return &c;return nullptr;}
 Receipt* receipt(uint64_t id){for(auto& r:receipts_)if(r.id==id&&id)return &r;return nullptr;}
 static void order_flags(Cycle& c){
  c.published_before_completion=c.published&&c.completed&&c.publication_qpc<c.last_completion_qpc;
  c.published_before_closure=c.published&&c.closed&&c.publication_qpc<c.closure_qpc;
 }
 void retire(Cycle& c){
  if(!c.closed||!c.published||c.transferred!=c.produced||c.completed!=c.produced)return;
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
public:
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
 bool expect(uint64_t ticket,uint64_t batch_count){
  std::lock_guard<std::mutex> lock(mutex_);auto c=cycle(ticket);
  if(!c)return reject(counts_.stale_rejected);
  if(c->closed||c->published||!c->flushes||c->invalid){c->invalid=true;return reject(counts_.stale_rejected);}
  if(batch_count>UINT64_MAX-c->expected_batches){c->invalid=true;return reject(counts_.capacity_rejected);}
  c->expected_batches+=batch_count;return true;
 }
 bool invalidate(uint64_t ticket){
  std::lock_guard<std::mutex> lock(mutex_);auto c=cycle(ticket);
  if(!c)return reject(counts_.stale_rejected);c->invalid=true;return true;
 }
 bool ready(uint64_t ticket)const{
  std::lock_guard<std::mutex> lock(mutex_);
  for(auto const& c:cycles_)if(ticket&&c.ticket==ticket)return !c.published&&c.closed&&!c.invalid&&!c.ambiguous_payload&&!c.failed&&
   c.produced==c.expected_batches&&c.transferred==c.produced&&c.completed==c.produced;
  return false;
 }
 bool execution(uint64_t id,Execution& out)const{
  std::lock_guard<std::mutex> lock(mutex_);out={};
  for(auto const& r:receipts_)if(id&&r.id==id&&r.executing&&!r.completed&&!r.ambiguous)
   for(auto const& c:cycles_)if(c.ticket==r.ticket&&!c.invalid&&!c.ambiguous_payload){
    out={r.id,r.ticket,c.canvas,c.target,r.payload};return true;
   }
  return false;
 }
 uint64_t produce(uint64_t ticket,uintptr_t payload,int64_t qpc){
  std::lock_guard<std::mutex> lock(mutex_);auto c=cycle(ticket);
  if(!c||c->closed||!c->flushes){reject(counts_.stale_rejected);return 0;}
  if(!payload){reject(counts_.payload_rejected);return 0;}
  Receipt* empty{};for(auto& r:receipts_){
   if(r.id&&r.payload==payload){
    r.ambiguous=true;c->ambiguous_payload=true;if(auto previous=cycle(r.ticket))previous->ambiguous_payload=true;
    reject(counts_.payload_rejected);return 0;
   }
   if(!r.id&&!empty)empty=&r;
  }
  if(!empty||next_id_==UINT64_MAX){reject(counts_.capacity_rejected);return 0;}
  *empty={++next_id_,ticket,payload};if(!c->produced)c->first_production_qpc=qpc;
  ++c->produced;++counts_.produced;return empty->id;
 }
 bool transferred(uint64_t id,bool cleared){
  std::lock_guard<std::mutex> lock(mutex_);auto r=receipt(id);
  if(!r)return reject(counts_.stale_rejected);
  if(r->transferred){if(auto c=cycle(r->ticket))c->invalid=true;return reject(counts_.stale_rejected);}
  if(!cleared){if(auto c=cycle(r->ticket))c->invalid=true;return reject(counts_.transfer_rejected);}
  auto c=cycle(r->ticket);if(!c)return reject(counts_.stale_rejected);
  r->transferred=true;++c->transferred;++counts_.transferred;retire(*c);return true;
 }
 uint64_t execute(uintptr_t payload,uintptr_t target,int64_t){
  std::lock_guard<std::mutex> lock(mutex_);Receipt* match{};
  for(auto& r:receipts_)if(r.id&&r.payload==payload){
   if(match){reject(counts_.payload_rejected);return 0;}match=&r;
  }
  if(match&&match->ambiguous){reject(counts_.payload_rejected);return 0;}
  if(!match||match->executing||match->completed){
   if(match)if(auto c=cycle(match->ticket))c->invalid=true;
   reject(counts_.stale_rejected);return 0;
  }
  auto c=cycle(match->ticket);if(!c||!target||c->target!=target){reject(counts_.target_rejected);return 0;}
  match->executing=true;++counts_.executed;return match->id;
 }
 bool complete(uint64_t id,bool ok,int64_t qpc){
  std::lock_guard<std::mutex> lock(mutex_);auto r=receipt(id);
  if(!r)return reject(counts_.stale_rejected);
  if(!r->executing||r->completed){if(auto c=cycle(r->ticket))c->invalid=true;return reject(counts_.stale_rejected);}
  auto c=cycle(r->ticket);if(!c)return reject(counts_.stale_rejected);
  r->completed=true;++c->completed;++counts_.completed;
  if(c->completed==1||qpc>c->last_completion_qpc)c->last_completion_qpc=qpc;
  if(!ok){++c->failed;++counts_.failed;}order_flags(*c);retire(*c);return true;
 }
 bool published(uint64_t ticket,int64_t qpc){
  std::lock_guard<std::mutex> lock(mutex_);auto c=cycle(ticket);
  if(!c)for(unsigned i=0;i<retained_count_;++i)if(retained_[i].ticket==ticket)c=&retained_[i];
  if(!c||c->published)return reject(counts_.stale_rejected);
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
