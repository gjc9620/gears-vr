#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>

// PID51076 reflection and full-function comparison, 2026-10-15. The actual
// widget is WarUIListButton -> UIOptionList, not the separate UIOptionButton.
// The native call retains ownership of index changes and subscriber notification.
namespace gears_graphics_menu {
inline constexpr std::uintptr_t set_index_rva=0xc7f63c,widget_vtable_rva=0x13cdc78;
inline constexpr std::size_t set_index_size=413;
inline constexpr char set_index_sha256[]="c70c9e913d969f02c181911daa2abee690ca09dd1caf02780b1158661511662c";
inline constexpr std::array<unsigned char,32> entry_bytes{
 0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57,
 0x48,0x83,0xec,0x30,0x8b,0xda,0x48,0x8b,0xf9,0x3b,0x91,0xd8,0x05,0x00,0x00,0x0f};
struct Registry {std::uintptr_t data{};std::int32_t count{},capacity{};};
struct Object {std::uintptr_t outer{},cls{};std::uint32_t index{},name{},number{};};
template<class Read,class T> bool get(Read& read,std::uintptr_t address,T& value){
 return address&&address<=std::numeric_limits<std::uintptr_t>::max()-sizeof(T)&&read(address,&value,sizeof(T));
}
inline bool valid(Registry const& r){return r.data&&r.count>1000&&r.count<=r.capacity&&r.capacity<4000000&&r.data<=UINTPTR_MAX-8*std::uintptr_t(r.capacity);}
template<class Read> bool object(Read& read,Registry const& objects,std::uintptr_t address,Object& out){
 std::array<unsigned char,0x60> raw{};std::uintptr_t slot{};
 if(!address||address%8||!get(read,address,raw))return false;
 std::memcpy(&out.index,raw.data()+8,4);std::memcpy(&out.outer,raw.data()+0x40,8);
 std::memcpy(&out.name,raw.data()+0x48,4);std::memcpy(&out.number,raw.data()+0x4c,4);
 std::memcpy(&out.cls,raw.data()+0x50,8);
 return out.index<unsigned(objects.count)&&get(read,objects.data+8*std::uintptr_t(out.index),slot)&&slot==address;
}
template<class Read> bool named(Read& read,Registry const& names,Object const& object,const char16_t* expected){
 std::uintptr_t entry{};std::uint32_t index{};std::array<char16_t,64> text{};
 if(object.name>=unsigned(names.count)||!get(read,names.data+8*std::uintptr_t(object.name),entry)||
    !get(read,entry,index)||index!=object.name||entry>UINTPTR_MAX-20||!get(read,entry+20,text))return false;
 for(unsigned i=0;i<text.size();++i){if(text[i]!=expected[i])return false;if(!text[i])return i!=0;}
 return false;
}
struct Selection {bool matched{};std::int32_t before{},forward{};};
template<class Read> Selection select(Read& read,std::uintptr_t base,std::uintptr_t address,std::int32_t requested){
 Selection result{};result.forward=requested;
 // No additional reads or behavior changes for supported modes or other lists.
 if(requested!=3||!base||base>UINTPTR_MAX-0x1c34810||!address||address>UINTPTR_MAX-0x600)return result;
 std::uintptr_t vtable{};
 if(!get(read,address,vtable)||vtable!=base+widget_vtable_rva)return result;
 Registry objects{},names{};Object option{},cls{};
 if(!get(read,base+0x1c24730,objects)||!valid(objects)||!get(read,base+0x1c34800,names)||!valid(names)||
    !object(read,objects,address,option)||!named(read,names,option,u"optAntialiasingOption")||option.number||
    !object(read,objects,option.cls,cls)||!named(read,names,cls,u"WarUIListButton"))return result;
 std::uintptr_t parent=option.outer;bool scene=false;
 for(unsigned depth=0;parent&&depth<16;++depth){Object outer{};
  if(!object(read,objects,parent,outer))return result;
  if(named(read,names,outer,u"PenderGraphicsSettings")){scene=true;break;}parent=outer.outer;}
 if(!scene)return result;
 std::array<unsigned char,0x40> binding{};
 if(!get(read,address+0x578,binding))return result;
 std::uintptr_t subscriber{},interface_pointer{},text{},store{};std::int32_t length{},capacity{},binding_index{};
 std::memcpy(&subscriber,binding.data(),8);std::memcpy(&interface_pointer,binding.data()+8,8);
 std::memcpy(&text,binding.data()+0x14,8);std::memcpy(&length,binding.data()+0x1c,4);
 std::memcpy(&capacity,binding.data()+0x20,4);std::memcpy(&binding_index,binding.data()+0x24,4);
 std::memcpy(&store,binding.data()+0x38,8);
 constexpr char16_t markup[]=u"<WarfareGameResources:Antialiasing>";
 std::array<char16_t,sizeof(markup)/sizeof(char16_t)> actual{};Object datastore{};
 if(subscriber!=address||interface_pointer!=address+0x4dc||binding_index||!store||
    length!=actual.size()||capacity<length||capacity>1024||!get(read,text,actual)||
    std::memcmp(actual.data(),markup,sizeof(markup))||!object(read,objects,store,datastore)||
    !named(read,names,datastore,u"UIDataStore_WarfareGameResource"))return result;
 if(!get(read,address+0x5d8,result.before)||result.before<0||result.before>3)return result;
 // Preserve the existing supported selection. Repair a pre-existing forbidden
 // index through the original native transaction rather than writing UI memory.
 result.forward=result.before==3?0:result.before;result.matched=true;return result;
}
} // namespace gears_graphics_menu
