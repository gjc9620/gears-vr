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
 Capacity capacity;Snapshot old{};old.valid=255;old.width=3840;old.height=2160;
 uint32_t w=3840,h=2160;
 assert(capacity.choose(1,2,false,old,w,h)==0&&!capacity.object);
 w=4096;h=2048;assert(capacity.choose(1,2,true,old,w,h)==2&&w==4096&&h==2160);
 old.width=w;old.height=h;w=3840;h=2160;
 assert(capacity.choose(1,2,false,old,w,h)==2&&w==4096&&h==2160);
 w=4096;h=2048;assert(capacity.choose(1,2,true,old,w,h)==2&&h==2160);
 w=3840;h=2160;assert(capacity.choose(7,2,false,old,w,h)==4&&w==3840);
 assert(capacity.choose(1,7,false,old,w,h)==4&&w==3840);
 assert(capacity.choose(1,2,true,old,w,h)==5&&w==3840);
 old.config[0]=1;assert(capacity.choose(1,2,false,old,w,h)==3&&w==3840);
 old.config[0]=0;old.valid=0;assert(capacity.choose(1,2,false,old,w,h)==3&&w==3840);
 old.valid=255;w=0;assert(capacity.choose(1,2,false,old,w,h)==3&&w==0);
 w=9000;assert(capacity.choose(1,2,false,old,w,h)==3&&w==9000);
 w=1920;h=1080;assert(capacity.choose(1,2,false,old,w,h)==5&&w==1920&&h==1080);
 PinnedCapacity pin;old.width=3840;old.height=2160;w=4096;h=2048;
 assert(pin.choose(1,2,true,old,w,h)==2&&pin.owner==2);
 std::thread different([&]{uint32_t x=4096,y=2048;assert(pin.choose(9,3,true,old,x,y)==4&&x==4096&&y==2048);});different.join();
 puts("resize probe bounded records, thread isolation, nesting and overflow PASS");return 0;
}
