#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <windows.h>

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
 DWORD thread{};uint32_t width{},height{},owned{},nested{};
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
inline void created(unsigned api,HRESULT result){
 if(current&&api<current->calls.size()){++current->calls[api];if(FAILED(result))++current->failed[api];}
}
#else
inline void created(unsigned,HRESULT){}
#endif
}
