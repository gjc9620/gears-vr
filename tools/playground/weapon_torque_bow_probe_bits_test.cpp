#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "weapon_torque_bow_probe_bits.h"
namespace bits=gears_torque_bow_probe_bits;
struct Reader {
 std::array<unsigned char,1024> memory{};unsigned reads{},change_at{};bool deny{};
 template<class T>bool at(std::uintptr_t p,std::uintptr_t offset,T& value) noexcept {
  ++reads;if(deny||p>memory.size()||offset>memory.size()-p||sizeof(T)>memory.size()-p-offset)return false;
  if(change_at&&reads==change_at)memory[0x250]^=1;
  std::memcpy(&value,memory.data()+p+offset,sizeof(value));return true;
 }
 template<class T>void put(std::uintptr_t p,T value){std::memcpy(memory.data()+p,&value,sizeof(value));}
 void reset(){reads=change_at=0;deny=false;memory={};put(0x124,std::uintptr_t{0x200});put(0x248,std::uintptr_t{0x300});}
};
int main(){
 Reader r;r.reset();bits::Sample s{};
 assert(bits::read(r,0x100,300,s)&&!bits::enabled(s,300));
 for(unsigned name:{300u,331u,363u}){
  r.put(0x250,std::uint64_t{1}<<(name-300));assert(bits::read(r,0x100,name,s)&&bits::enabled(s,name));
  const auto other=name==300?301u:300u;assert(!bits::enabled(s,other));
 }
 assert(!bits::read(r,0x100,299,s)&&!s.frame);assert(!bits::read(r,0x100,364,s));
 assert(!bits::enabled({0x200,0x300,UINT64_MAX},299));assert(!bits::enabled({0x200,0x300,UINT64_MAX},364));
 r.reset();r.put(0x124,std::uintptr_t{});assert(!bits::read(r,0x100,300,s));
 r.reset();r.put(0x248,std::uintptr_t{});assert(!bits::read(r,0x100,300,s));
 r.reset();r.deny=true;assert(!bits::read(r,0x100,300,s));
 r.reset();r.put(0x124,UINTPTR_MAX-8);assert(!bits::read(r,0x100,300,s));
 r.reset();r.change_at=4;assert(!bits::read(r,0x100,300,s));
 r.reset();assert(bits::read(r,0x100,300,s));auto other=s;other.frame++;assert(!bits::same(s,other));other=s;other.state++;assert(!bits::same(s,other));other=s;other.mask++;assert(!bits::same(s,other));
 std::puts("Original probe production reader: names300/331/363 and zero mask; unknown index/null frame/null state/unreadable/overflow/changing mask rejected; distinct frame/state/mask rejected. Owned memory only; no Engine calls/writes.");
}
