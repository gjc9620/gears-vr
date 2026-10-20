#pragma once
#include "owned_scene_trial.h"

namespace gears_owned_scene {
// CPU bookkeeping only. The integration serializes calls and certifies world
// identity and the ordered output boundary; this manager makes no GPU claim.
enum class Promotion { not_pending, promoted, not_ready, identity_mismatch };
struct Pipeline {
 Trial active{},pending{};
 bool output_observed{};
 // The integration has already certified retirement of the previous resource.
 // Keep its completed count, but never inherit its permission to overlap.
 bool reset_resource(){
  if(pending.ticket||pending.resource)return false;
  const auto completed=active.completed;
  active={};active.completed=completed;output_observed=false;return true;
 }
 // A recreated resource starts serially. Do not queue a second renderer until
 // one complete output of THIS pipeline has crossed the certified boundary.
 bool output_boundary(uint64_t ticket){
  if(!ticket||active.ticket!=ticket||!active.complete())return false;
  output_observed=true;return true;
 }
 bool select(uintptr_t renderer,uint64_t ticket,uintptr_t adapter,bool allow_pending){
  if(!active.ticket)return active.select(renderer,ticket,adapter);
  if(!allow_pending||!output_observed||pending.resource||pending.ticket)return false;
  Trial next{};
  if(!next.ready(active.resource,active.adapter,active.born))return false;
  next.last_ticket=active.ticket>active.last_ticket?active.ticket:active.last_ticket;
  if(!next.select(renderer,ticket,adapter))return false;
  pending=next;return true;
 }
 bool pending_match(uintptr_t renderer,uint64_t ticket)const{
  return ticket&&pending.ticket==ticket&&pending.renderer==renderer;
 }
 Trial* find(uintptr_t renderer,uint64_t ticket){
  if(!ticket)return nullptr;
  const bool a=active.ticket==ticket&&active.renderer==renderer;
  const bool p=pending_match(renderer,ticket);
  return a==p?nullptr:(a?&active:&pending);
 }
 const Trial* find(uintptr_t renderer,uint64_t ticket)const{
  if(!ticket)return nullptr;
  const bool a=active.ticket==ticket&&active.renderer==renderer;
  const bool p=pending_match(renderer,ticket);
  return a==p?nullptr:(a?&active:&pending);
 }
 // A renderer-only retirement event is ambiguous when both slots match.
 Trial* find_renderer(uintptr_t renderer){
  const bool a=active.ticket&&active.renderer==renderer;
  const bool p=pending.ticket&&pending.renderer==renderer;
  return a==p?nullptr:(a?&active:&pending);
 }
 bool construction(uintptr_t renderer,uint64_t ticket,bool verified,uintptr_t stored){
  auto* trial=find(renderer,ticket);
  return trial&&trial->construction(renderer,ticket,verified,stored);
 }
 Promotion promote(uintptr_t renderer,uint64_t ticket,bool ordered_old_output){
  if(!pending.ticket)return Promotion::not_pending;
  if(!pending_match(renderer,ticket)||pending.resource!=active.resource||
     pending.adapter!=active.adapter||pending.born!=active.born)
   return Promotion::identity_mismatch;
  if(!pending.built||pending.entered||pending.returned||pending.destroyed||pending.error||
     !ordered_old_output||!active.complete()||pending.ticket<=active.ticket||
     pending.ticket<=active.last_ticket)return Promotion::not_ready;
  Trial old=active;
  if(!old.advance(old.ticket,true))return Promotion::not_ready;
  Trial next=pending;
  next.completed=old.completed;next.last_ticket=old.last_ticket;
  active=next;pending={};return Promotion::promoted;
 }
};
}
