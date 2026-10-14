#define GEARS_NATIVE_RESIZE_PROBE 1
#include "resource_resize_probe.h"
#include <cassert>
#include <cstdio>
#include <thread>

int main(){
 using namespace gears_resize_probe;
 Buffer<2> buffer;uint64_t first{},second{},overflow{};
 auto a=buffer.reserve(first);assert(a&&first==0&&!buffer.ready[0]);
 {
  Window parent(a);created(5,S_OK);
  auto b=buffer.reserve(second);assert(b&&second==1);
  {Window child(b);assert(b->nested==1);created(2,E_FAIL);created(99,S_OK);}
  assert(current==a&&a->calls[5]==1&&a->calls[2]==0&&b->calls[2]==1&&b->failed[2]==1);
  // Other threads must not contribute to this call's counts.
  std::thread other([]{created(5,S_OK);});other.join();assert(a->calls[5]==1);
  assert(!buffer.reserve(overflow)&&overflow==2);
  {Window full(nullptr);created(5,S_OK);}assert(current==a&&a->calls[5]==1);
  buffer.finish(second);assert(buffer.ready[1].load());
 }
 assert(!current);buffer.finish(first);assert(buffer.ready[0]&&buffer.attempts==3);
 puts("resize probe bounded records, thread isolation, nesting and overflow PASS");return 0;
}
