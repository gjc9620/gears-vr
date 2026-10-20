#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <windows.h>
#include "native_render_size.h"

// Optional, bounded observation. No locks, allocation or logging on the game
// thread. API counts belong to the innermost observed call on this thread;
// they count returned API calls, not live allocations or completed GPU work.
namespace gears_resize_probe {
#if defined(GEARS_NATIVE_RESIZE_PROBE) && GEARS_NATIVE_RESIZE_PROBE
struct Snapshot {
 uint32_t width{},height{},valid{};
 std::array<uint32_t,7> config{};
};
struct Row {
 uintptr_t object{},caller{},renderer{},result{};
 uint64_t ticket{},begin{},end{};
 DWORD thread{};uint32_t width{},height{},owned{},nested{},forward_width{},forward_height{},policy{};
 Snapshot before{},after{};
 std::array<uint32_t,8> calls{},failed{};
};
template<size_t N> struct Buffer {
 std::array<Row,N> rows{};std::array<std::atomic<bool>,N> ready{};
 std::atomic<uint64_t> attempts{};
 Row* reserve(uint64_t& index){index=attempts.fetch_add(1,std::memory_order_relaxed);return index<N?&rows[size_t(index)]:nullptr;}
 void finish(uint64_t index){if(index<N)ready[size_t(index)].store(true,std::memory_order_release);}
};
inline thread_local Row* current{};
struct Window {
 Row* previous{};
 explicit Window(Row* row):previous(current){current=row;if(row)row->nested=previous?1:0;}
 ~Window(){current=previous;}
};
// Retain capacity for the object/thread first encountered in a certified owned
// consume. The session geometry is immutable. Never write the engine mode flag.
// This changes allocation requests only; view rectangles/projections stay native.
struct Capacity {
 uintptr_t object{};DWORD thread{};uint32_t legacy_width{},legacy_height{};
 unsigned choose(uintptr_t target,DWORD tid,bool owned,const Snapshot& before,uint32_t& width,uint32_t& height){
  if((before.valid&3)!=3||before.config[0]||!target||!tid||!width||!height||width>8192||height>8192||
     !before.width||!before.height||before.width>8192||before.height>8192)return 3;
  const auto owned_width=gears_native_render_size::width(),owned_height=gears_native_render_size::height();
  if(!object){if(!owned||width!=owned_width||height!=owned_height)return 0;object=target;thread=tid;legacy_width=before.width;legacy_height=before.height;}
  if(object!=target||thread!=tid)return 4;
  if(owned&&(width!=owned_width||height!=owned_height))return 5;
  if(!owned&&(width!=legacy_width||height!=legacy_height))return 5;
  const auto old_width=width,old_height=height;
  if(width<before.width)width=before.width;if(height<before.height)height=before.height;
  return width!=old_width||height!=old_height?2:1;
 }
};
struct PinnedCapacity {
 std::atomic<DWORD> owner{};Capacity state;
 unsigned choose(uintptr_t target,DWORD tid,bool owned,const Snapshot& before,uint32_t& width,uint32_t& height){
  auto selected=owner.load(std::memory_order_acquire);
  if(!selected){
   // Validate without touching shared non-atomic state. Only the winning
   // thread may subsequently read or update state, including rejected calls.
   Capacity candidate;auto w=width,h=height;
   const auto result=candidate.choose(target,tid,owned,before,w,h);
   if(!candidate.object)return result;
   DWORD absent{};
   if(!owner.compare_exchange_strong(absent,tid,std::memory_order_acq_rel)&&absent!=tid)return 4;
  }else if(selected!=tid)return 4;
  return state.choose(target,tid,owned,before,width,height);
 }
};
inline void created(unsigned api,HRESULT result){
 if(current&&api<current->calls.size()){++current->calls[api];if(FAILED(result))++current->failed[api];}
}
#else
inline void created(unsigned,HRESULT){}
#endif
}
