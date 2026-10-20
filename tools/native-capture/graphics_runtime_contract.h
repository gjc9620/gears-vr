#pragma once
#include "graphics_lock_routes.h"
namespace gears_graphics_runtime {
enum class Failure:unsigned {none,dispatcher_bytes,queueflag,selector_thread,initial_state,
 admission,normal_region,command_region,command_extent,route,return_stride,readback};
struct Window {std::uintptr_t rva;const char* hex;std::size_t size;};
// Same runtime snapshot SHA as graphics_lock.h; queued caller live matched ±32
// in PID46520. Slot8 lookup and complete stride tail need fresh admission too.
inline constexpr Window windows[]={
 {0x19f255,"488b03488b5008493bd50f8589000000",16},
 {0x19f2d3,"49608b83b8000000894168488b4b10e8096df4ffbfc0000000eb07488bcbffd2",32},
 {0x19f2f3,"8bf8488b0b4c8b014c3bc675054c8923eb0833d2488bcb41ffd08b0de595a901",32},
 {0x19f30d,"8b0de595a9014c8b05d695a9018d51fff7d903d78bc14823d04903d0488915c095a9018b05ea6ec0",40}
};
template<class Read> bool bytes(std::uintptr_t base,Read& read,std::uintptr_t& failed){
 for(auto const& w:windows){unsigned char expected[40]{},actual[40]{};std::uintptr_t address{};
  if(!gears_graphics_lock::add(base,w.rva,address)){failed=w.rva;return false;}
  for(std::size_t i=0;i<w.size;++i){const auto digit=[](char c){return c<='9'?unsigned(c-'0'):unsigned(c-'a'+10);};
   expected[i]=static_cast<unsigned char>((digit(w.hex[2*i])<<4)|digit(w.hex[2*i+1]));}
  if(!read(address,actual,w.size)||std::memcmp(actual,expected,w.size)){failed=w.rva;return false;}}
 failed=0;return true;
}
template<class Read> Failure queued(std::uintptr_t base,gears_graphics_lock::routes::Call call,Read& read,std::uintptr_t& failed){
 if(call.return_rva!=0x19f2f3)return Failure::route;
 if(!bytes(base,read,failed))return Failure::dispatcher_bytes;
 std::uint32_t flag{},thread{};
 if(!gears_graphics_lock::read_value(read,base+0x1c204b0,flag)||flag!=1)return Failure::queueflag;
 if(!gears_graphics_lock::read_value(read,base+0x1c204dc,thread)||!thread||thread!=call.thread)return Failure::selector_thread;
 return Failure::none;
}
} // namespace gears_graphics_runtime
