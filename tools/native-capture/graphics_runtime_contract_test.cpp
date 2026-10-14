#include "graphics_runtime_contract.h"
#include <cstdio>
#include <cstdlib>
using namespace gears_graphics_runtime;
struct Read {uintptr_t base=0x140000000,bad{};uint32_t flag=1,thread=52244;
 bool operator()(uintptr_t address,void* out,size_t size){
  for(auto const& w:windows)if(address==base+w.rva&&size==w.size){unsigned char value[40]{};
   for(size_t i=0;i<size;++i){auto digit=[](char c){return c<='9'?unsigned(c-'0'):unsigned(c-'a'+10);};
    value[i]=static_cast<unsigned char>((digit(w.hex[i*2])<<4)|digit(w.hex[i*2+1]));}
   if(w.rva==bad)value[size-1]^=1;memcpy(out,value,size);return true;}
  if(size==4&&address==base+0x1c204b0){memcpy(out,&flag,4);return true;}
  if(size==4&&address==base+0x1c204dc){memcpy(out,&thread,4);return true;}return false;}
};
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %u: %s\n",__LINE__,#x);std::exit(1);}}while(0)
int main(){Read read;uintptr_t failed{};
 CHECK(bytes(read.base,read,failed));CHECK(failed==0);
 CHECK(queued(read.base,{52244,0x19f2f3},read,failed)==Failure::none);
 // A new process/current RHI role admits its actual ID, never the prior PID ID.
 read.thread=1234;CHECK(queued(read.base,{1234,0x19f2f3},read,failed)==Failure::none);
 CHECK(queued(read.base,{52244,0x19f2f3},read,failed)==Failure::selector_thread);
 read.thread=0;CHECK(queued(read.base,{0,0x19f2f3},read,failed)==Failure::selector_thread);
 read.thread=1234;read.flag=0;CHECK(queued(read.base,{1234,0x19f2f3},read,failed)==Failure::queueflag);
 read.flag=2;CHECK(queued(read.base,{1234,0x19f2f3},read,failed)==Failure::queueflag);
 read.flag=1;CHECK(queued(read.base,{1234,0x123456},read,failed)==Failure::route);
 for(auto const& w:windows){read.bad=w.rva;CHECK(queued(read.base,{1234,0x19f2f3},read,failed)==Failure::dispatcher_bytes);CHECK(failed==w.rva);}
 read.bad=0;CHECK(queued(read.base,{1234,0x19f2f3},read,failed)==Failure::none);
 std::printf("graphics runtime contract: %u CPU checks passed\n",checks);
}
