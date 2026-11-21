#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
namespace {
using uptr=std::uintptr_t;
std::array<unsigned char,0x140> function{};
bool registered(uptr p) noexcept {return p==reinterpret_cast<uptr>(function.data());}
template<class T>T get(uptr p) noexcept {T v{};std::memcpy(&v,reinterpret_cast<void const*>(p),sizeof(v));return v;}
template<class T>void put(uptr p,T v) noexcept {std::memcpy(reinterpret_cast<void*>(p),&v,sizeof(v));}
namespace playground {struct Args {struct Request {uptr function{};unsigned size{};} r;bool valid{};};}
#include "weapon_torque_bow_putdown_schema.inl"
}
int main(){
 const auto fn=reinterpret_cast<uptr>(function.data());
 put(fn+0x108,static_cast<unsigned short>(0xffff));playground::Args call{{fn,0},true};
 assert(put_down_schema(call));
 call.r.size=4;assert(!put_down_schema(call));call.r.size=0;
 put(fn+0x88,4u);assert(!put_down_schema(call));put(fn+0x88,0u);
 put(fn+0x108,static_cast<unsigned short>(0));assert(!put_down_schema(call));put(fn+0x108,static_cast<unsigned short>(0xffff));
 call.valid=false;assert(!put_down_schema(call));call.valid=true;
 call.r.function=0;assert(!put_down_schema(call));call.r.function=fn+1;assert(!put_down_schema(call));call.r.function=fn;
 assert(put_down_schema(call));
 std::puts("Retail void PutDownWeapon production ABI gate: exact0/extent0/returnFFFF accepted; old bool-size4, nonzero extent, return offset, invalid call and unregistered function rejected. Owned memory only.");
}
