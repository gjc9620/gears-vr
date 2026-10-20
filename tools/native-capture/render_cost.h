#pragma once
#include <windows.h>
#include <array>
#include <atomic>
#include <cstdint>

// Bounded, optional CPU profiling. Nested categories overlap; never sum them
// to claim total renderer time. No allocation, locks, I/O or GPU timestamp claim.
namespace gears_render_cost {
// Published once for a bounded external sampler. Zero when profiling is off.
inline std::atomic<DWORD> first_thread{};
enum Kind : unsigned {safe_read,engine_view,output_stack,viewport_draw,frame_draw,reset_tracking,barrier_observation,frame_lock,draw_forward,count};
struct Counter {uint64_t calls{},ticks{};};
struct Record {std::array<Counter,count> items{};uint64_t total_ticks{};
 uint64_t kernel_100ns{},user_100ns{},cpu_samples{},cpu_failures{};
 DWORD thread{};unsigned thread_changes{};};
#if defined(GEARS_NATIVE_RENDER_COST_PROBE) && GEARS_NATIVE_RENDER_COST_PROBE
inline thread_local Record* current{};
inline uint64_t stamp(){LARGE_INTEGER value;QueryPerformanceCounter(&value);return value.QuadPart;}
struct Cpu {uint64_t kernel{},user{};};
inline bool cpu(Cpu& out){
 FILETIME creation{},exit{},kernel{},user{};
 if(!GetThreadTimes(GetCurrentThread(),&creation,&exit,&kernel,&user))return false;
 out={(uint64_t(kernel.dwHighDateTime)<<32)|kernel.dwLowDateTime,
      (uint64_t(user.dwHighDateTime)<<32)|user.dwLowDateTime};return true;
}
struct Window {
 Record* previous{};Record* record{};uint64_t start{};
 Cpu before{};bool cpu_valid{};
 explicit Window(Record* r):previous(current),record(r){current=r;if(r){
   const auto thread=GetCurrentThreadId();if(!r->thread)r->thread=thread;else if(r->thread!=thread)++r->thread_changes;
   DWORD absent{};first_thread.compare_exchange_strong(absent,thread,std::memory_order_relaxed);
   cpu_valid=cpu(before);start=stamp();}}
 ~Window(){if(record){record->total_ticks+=stamp()-start;Cpu after{};
   if(cpu_valid&&cpu(after)&&after.kernel>=before.kernel&&after.user>=before.user){
    record->kernel_100ns+=after.kernel-before.kernel;record->user_100ns+=after.user-before.user;++record->cpu_samples;
   }else ++record->cpu_failures;
  }current=previous;}
};
struct Scope {
 Counter* counter{};uint64_t start{};
 explicit Scope(Kind kind){if(current){counter=&current->items[kind];start=stamp();}}
 ~Scope(){if(counter){counter->ticks+=stamp()-start;++counter->calls;}}
};
#else
struct Window {explicit Window(Record*){}};
struct Scope {explicit Scope(Kind){}};
#endif
}
