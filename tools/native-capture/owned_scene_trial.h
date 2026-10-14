#pragma once
#include <array>
#include <cstdint>
#include "frame_link.h"

namespace gears_owned_scene {
// One resource, one admitted renderer at a time, no GPU completion or retirement claims.
// Every method is serialized by capture's API-order lock. Native target getters
// may run concurrently: the retained Owner is never mutated after publication.
struct Row {gears_frame_link::ListRef list{};unsigned mask{};uint64_t draws{};};
struct Receipt {uintptr_t queue{};uint64_t generation{},ticket{};std::array<Row,8> rows{};unsigned count{};};
struct Trial {
 uintptr_t resource{},adapter{},renderer{},queue{};uint64_t born{},ticket{},last_ticket{},completed{};
 bool built{},entered{},returned{},destroyed{};unsigned error{},draw_mask{},submitted_mask{};
 uint64_t draws{},unknown_draws{},submissions{};std::array<Row,8> rows{};
 bool ready(uintptr_t r,uintptr_t a,uint64_t generation){
  if(resource||!r||!a||!generation)return false;
  resource=r;adapter=a;born=generation;return true;
 }
 bool candidate()const{return resource&&adapter&&!ticket&&!error;}
 bool select(uintptr_t target,uint64_t frame,uintptr_t selected){
  if(!candidate()||!target||frame<=born||frame<=last_ticket||selected!=adapter)return false;
  renderer=target;ticket=frame;return true;
 }
 bool construction(uintptr_t target,uint64_t frame,bool verified,uintptr_t stored){
  if(target!=renderer||frame!=ticket||!ticket)return true;
  built=verified&&stored==adapter;if(!built)error=1;return built;
 }
 bool begin(uintptr_t target,uint64_t frame){
  if(target!=renderer||frame!=ticket||!ticket)return true;
  if(!built||entered){error=2;return false;}entered=true;return true;
 }
 void end(uintptr_t target,uint64_t frame){if(target==renderer&&frame==ticket&&ticket)returned=true;}
 void retire(uintptr_t target){if(ticket&&target==renderer)destroyed=true;}
 void draw(uintptr_t r,gears_frame_link::ListRef list,uint64_t frame,unsigned eye,bool exact){
  if(r!=resource||!resource)return;
  ++draws;
  if(!entered||frame!=ticket||eye>1||!exact){++unknown_draws;return;}
  if(error||!list.identity||!list.generation){error=3;return;}
  Row* empty{};Row* found{};
  for(auto& row:rows){if(row.list.identity==list.identity&&row.list.generation==list.generation)found=&row;
   if(!row.list.identity&&!empty)empty=&row;}
  if(!found)found=empty;if(!found){error=4;return;}
  found->list=list;found->mask|=1u<<eye;++found->draws;draw_mask|=1u<<eye;
 }
 Receipt prepare(uintptr_t q,bool direct,const gears_frame_link::ListRef* members,unsigned count){
  Receipt result{};if(!ticket||error||!members)return result;
  for(auto const& row:rows)if(row.list.identity)for(unsigned i=0;i<count;++i)
   if(row.list.identity==members[i].identity&&row.list.generation==members[i].generation){
    if(!q||!direct||(queue&&queue!=q)||result.count==result.rows.size()){error=5;return {};}
    for(unsigned j=0;j<result.count;++j)if(result.rows[j].list.identity==row.list.identity){error=6;return {};}
    result.rows[result.count++]=row;
   }
  if(result.count){result.queue=q;result.generation=born;result.ticket=ticket;}return result;
 }
 // Called only AFTER the original Execute returned, under the same ordering
 // lock. Matching generation/content is checked again for reentrant API calls.
 template<class Current> bool commit(Receipt const& receipt,Current current){
  if(!receipt.count)return true;if(error)return false;
  if(receipt.generation!=born||receipt.ticket!=ticket||!ticket){error=11;return false;}
  if(!receipt.queue||(queue&&queue!=receipt.queue)){error=5;return false;}
  for(unsigned i=0;i<receipt.count;++i){auto const& before=receipt.rows[i];const auto now=current(before.list.identity);
   bool match=false;for(auto const& row:rows)if(row.list.identity==before.list.identity&&
    row.list.generation==before.list.generation&&row.mask==before.mask&&row.draws==before.draws)match=true;
   if(!match||now.identity!=before.list.identity||now.generation!=before.list.generation){error=7;return false;}}
  queue=receipt.queue;for(unsigned i=0;i<receipt.count;++i)submitted_mask|=receipt.rows[i].mask;
  ++submissions;return true;
 }
 bool complete()const{return ticket&&built&&entered&&returned&&destroyed&&draw_mask==3&&submitted_mask==3&&
  !error&&!unknown_draws&&draws==2&&submissions&&queue;}
 // Integration certifies that the same ticket's owned-output boundary has
 // consumed all relevant submissions and queued any sampling/publication copy.
 // Owner allocation/adapter lifetime is unchanged; queued Canvas is not covered.
 bool advance(uint64_t consumed_ticket,bool ordered_boundary){
  if(!ordered_boundary||consumed_ticket!=ticket||!complete()||completed==UINT64_MAX)return false;
  const auto next_count=completed+1;const auto previous=ticket;
  const auto r=resource,a=adapter;const auto generation=born;
  *this={};resource=r;adapter=a;born=generation;last_ticket=previous;completed=next_count;
  return true;
 }
};
}
