#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string>
#include <map>
#include <vector>
namespace {
using uptr=std::uintptr_t;
std::map<uptr,std::wstring> labels;
std::vector<std::array<unsigned char,0x140>> objects;
template<class T>T get(uptr address) noexcept {T result{};std::memcpy(&result,reinterpret_cast<void const*>(address),sizeof result);return result;}
template<class T>void put(uptr address,T value) noexcept {std::memcpy(reinterpret_cast<void*>(address),&value,sizeof value);}
bool registered(uptr address) noexcept {return labels.find(address)!=labels.end();}
uptr node(wchar_t const* name){objects.emplace_back();const auto p=reinterpret_cast<uptr>(objects.back().data());labels[p]=name;return p;}
namespace playground {
bool named(uptr address,wchar_t const* label) noexcept {const auto it=labels.find(address);return it!=labels.end()&&it->second==label;}
struct Args {
 struct Request {uptr function{};unsigned size{16};} r;
 uptr index{},material{};
 uptr prop(wchar_t const* label) noexcept {return !std::wcscmp(label,L"ElementIndex")?index:material;}
};
}
bool native_function(playground::Args const& call,wchar_t const* label,wchar_t const*) noexcept {
 return registered(call.r.function)&&playground::named(call.r.function,label);
}
namespace hammerburst_animation {
bool input_field(playground::Args const& call,uptr field,wchar_t const* kind,unsigned bytes) noexcept {
 if(!registered(field)||get<uptr>(field+0x40)!=call.r.function||
    !playground::named(get<uptr>(field+0x50),kind)||get<unsigned>(field+0x70)!=1||
    get<unsigned>(field+0x74)!=bytes||(get<unsigned long long>(field+0x78)&0x580)!=0x80)return false;
 const auto offset=get<unsigned>(field+0x94);return offset<=call.r.size&&bytes<=call.r.size-offset;
}
}
#include "weapon_part_material_schema.inl"
}
int main(){
 objects.reserve(16);
 const auto function=node(L"GetMaterial"),index=node(L"ElementIndex"),material=node(L"ReturnValue"),
     integer=node(L"IntProperty"),object_property=node(L"ObjectProperty"),klass=node(L"MaterialInterface"),
     class_type=node(L"Class"),package=node(L"Engine"),package_type=node(L"Package"),extra=node(L"Unexpected");
 put(function+0x88,12u);put(function+0x108,static_cast<unsigned short>(4));put(function+0x80,index);
 put(index+0x40,function);put(index+0x50,integer);put(index+0x70,1u);put(index+0x74,4u);put(index+0x78,0x80ull);put(index+0x68,material);
 put(material+0x40,function);put(material+0x50,object_property);put(material+0x70,1u);put(material+0x74,8u);
 put(material+0x78,0x580ull);put(material+0x94,4u);put(material+0xd0,klass);
 put(klass+0x40,package);put(klass+0x50,class_type);put(package+0x50,package_type);
 playground::Args call{{function,12},index,material};
 assert(material_schema(call,false)&&!material_schema(call,true));
 put(material+0x94,0u);assert(!material_schema(call,false));put(material+0x94,4u);
 put(function+0x108,static_cast<unsigned short>(0));assert(!material_schema(call,false));put(function+0x108,static_cast<unsigned short>(4));
 put(material+0x78,0x480ull);assert(!material_schema(call,false));put(material+0x78,0x580ull);
 call.r.size=16;assert(!material_schema(call,false));call.r.size=12;
 put(index+0x94,4u);assert(!material_schema(call,false));put(index+0x94,0u);
 put(material+0x94,8u);assert(!material_schema(call,false));put(material+0x94,4u);
 put(material+0x68,index);assert(!material_schema(call,false));put(material+0x68,uptr{});
 put(material+0x68,extra);put(extra+0x40,function);put(extra+0x78,0x80ull);
 assert(!material_schema(call,false));put(material+0x68,uptr{});
 put(material+0xd0,object_property);assert(!material_schema(call,false));put(material+0xd0,klass);
 put(package+0x40,function);assert(!material_schema(call,false));put(package+0x40,uptr{});
 put(function+0x88,11u);assert(!material_schema(call,false));put(function+0x88,12u);
 assert(material_schema(call,false));
 labels[function]=L"SetMaterial";labels[material]=L"Material";put(material+0x78,0x80ull);
 put(function+0x108,static_cast<unsigned short>(0xffff));
 assert(material_schema(call,true)&&!material_schema(call,false));
 call.r.size=16;assert(!material_schema(call,true));call.r.size=12;
 put(material+0x94,8u);assert(!material_schema(call,true));put(material+0x94,4u);
 put(index+0x78,0x480ull);assert(!material_schema(call,true));put(index+0x78,0x80ull);
 put(material+0x40,package);assert(!material_schema(call,true));put(material+0x40,function);
 assert(material_schema(call,true));
 std::puts("Original material ABI: packed retail Get/SetMaterial12/index0/object4 with exact return/input flags and distinct return offsets; old16-byte setters, wrong size/offsets/flags/overlap/return offset/cycle/extra parameter/class/package/extent/owner rejected. Owned-memory fixture, no game certification.");
}
