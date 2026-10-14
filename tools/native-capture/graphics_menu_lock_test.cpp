#include "graphics_menu_lock.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {
struct Memory {
 struct Region {std::uintptr_t address;std::vector<unsigned char> bytes;};
 std::vector<Region> regions;
 unsigned reads{};
 void add(std::uintptr_t address,std::size_t size){regions.push_back({address,std::vector<unsigned char>(size)});}
 bool operator()(std::uintptr_t address,void* out,std::size_t size){
  ++reads;
  for(auto const& r:regions)if(address>=r.address&&address-r.address<=r.bytes.size()&&size<=r.bytes.size()-(address-r.address)){
   std::memcpy(out,r.bytes.data()+(address-r.address),size);return true;
  }
  return false;
 }
 void bytes(std::uintptr_t address,void const* value,std::size_t size){
  for(auto& r:regions)if(address>=r.address&&address-r.address<=r.bytes.size()&&size<=r.bytes.size()-(address-r.address)){
   std::memcpy(r.bytes.data()+(address-r.address),value,size);return;
  }
  std::fprintf(stderr,"Invalid fixture write\n");std::abort();
 }
 template<class T> void put(std::uintptr_t address,T value){bytes(address,&value,sizeof(value));}
};
struct Fixture {
 static constexpr std::uintptr_t base=0x140000000ULL,widget=0x20000000,cls=0x20001000,
  panel=0x20002000,scene=0x20003000,store=0x20004000,objects=0x30000000,names=0x30010000,text=0x40000000;
 Memory memory;
 Fixture(){
  memory.add(base+0x1c24730,sizeof(gears_graphics_menu::Registry));
  memory.add(base+0x1c34800,sizeof(gears_graphics_menu::Registry));
  memory.add(objects,1001*8);memory.add(names,1001*8);
  memory.put(base+0x1c24730,gears_graphics_menu::Registry{objects,1001,1001});
  memory.put(base+0x1c34800,gears_graphics_menu::Registry{names,1001,1001});
  make_object(widget,1,1,panel,cls,u"optAntialiasingOption",0x600);
  make_object(cls,2,2,0,0,u"WarUIListButton");
  make_object(panel,3,3,scene,0,u"Panel");
  make_object(scene,4,4,0,0,u"PenderGraphicsSettings");
  make_object(store,5,5,0,0,u"UIDataStore_WarfareGameResource",0x204);
  memory.put(widget,base+gears_graphics_menu::widget_vtable_rva);
  memory.put(widget+0x578,widget);memory.put(widget+0x580,widget+0x4dc);
  constexpr char16_t markup[]=u"<WarfareGameResources:Antialiasing>";
  memory.add(text,sizeof(markup));memory.bytes(text,markup,sizeof(markup));
  memory.put(widget+0x58c,text);
  memory.put<std::int32_t>(widget+0x594,sizeof(markup)/sizeof(char16_t));
  memory.put<std::int32_t>(widget+0x598,sizeof(markup)/sizeof(char16_t));
  memory.put<std::int32_t>(widget+0x59c,0);
  memory.put(widget+0x5b0,store);
  memory.put<std::int32_t>(widget+0x5d8,2);memory.put<std::int32_t>(store+0x200,3);
 }
 void make_object(std::uintptr_t address,unsigned index,unsigned name,std::uintptr_t outer,std::uintptr_t type,char16_t const* label,std::size_t size=0x60){
  memory.add(address,size);memory.put<std::uint32_t>(address+8,index);
  memory.put(address+0x40,outer);memory.put<std::uint32_t>(address+0x48,name);memory.put(address+0x50,type);
  memory.put(objects+8*index,address);
  auto entry=std::uintptr_t(0x50000000+name*0x100);
  memory.add(entry,20+128);memory.put(names+8*name,entry);memory.put<std::uint32_t>(entry,name);
  std::u16string value(label);memory.bytes(entry+20,value.c_str(),(value.size()+1)*2);
 }
};
int failures{};
void expect(bool condition,char const* label){if(!condition){++failures;std::fprintf(stderr,"FAIL: %s\n",label);}}
template<class Change> void reject(char const* label,Change change){
 Fixture f;change(f.memory);
 auto s=gears_graphics_menu::select(f.memory,Fixture::base,Fixture::widget,3);
 expect(!s.matched&&s.forward==3,label);
}
}
int main(){
 for(int before=0;before<=3;++before){
  Fixture f;f.memory.put<std::int32_t>(Fixture::widget+0x5d8,before);
  auto snapshot=f.memory.regions;
  auto s=gears_graphics_menu::select(f.memory,Fixture::base,Fixture::widget,3);
  expect(s.matched&&s.before==before&&s.forward==(before==3?0:before),"forbidden request preserves supported index or repairs 3");
  bool unchanged=snapshot.size()==f.memory.regions.size();
  for(std::size_t i=0;i<snapshot.size();++i)unchanged=unchanged&&snapshot[i].bytes==f.memory.regions[i].bytes;
  expect(unchanged,"selection never writes object, binding, or datastore");
 }
 for(int request:{-1,0,1,2,4,100}){
  Memory unreadable;auto s=gears_graphics_menu::select(unreadable,Fixture::base,Fixture::widget,request);
  expect(!s.matched&&s.forward==request&&unreadable.reads==0,"other requests forward without reads");
 }
 reject("different widget vtable",[](Memory& m){m.put(Fixture::widget,Fixture::base+1);});
 reject("different control name",[](Memory& m){m.put<std::uint32_t>(Fixture::widget+0x48,3);});
 reject("numbered control",[](Memory& m){m.put<std::uint32_t>(Fixture::widget+0x4c,1);});
 reject("different class",[](Memory& m){m.put(Fixture::widget+0x50,Fixture::panel);});
 reject("different scene",[](Memory& m){m.put<std::uint32_t>(Fixture::scene+0x48,3);});
 reject("detached scene ancestry",[](Memory& m){m.put<std::uintptr_t>(Fixture::panel+0x40,0);});
 reject("cyclic ancestry",[](Memory& m){m.put(Fixture::panel+0x40,Fixture::panel);});
 reject("wrong subscriber",[](Memory& m){m.put(Fixture::widget+0x578,Fixture::panel);});
 reject("wrong subscriber interface",[](Memory& m){m.put(Fixture::widget+0x580,Fixture::widget+0x508);});
 reject("different binding index",[](Memory& m){m.put<std::int32_t>(Fixture::widget+0x59c,1);});
 reject("different markup",[](Memory& m){m.put<char16_t>(Fixture::text,u'!');});
 reject("markup length mismatch",[](Memory& m){m.put<std::int32_t>(Fixture::widget+0x594,1);});
 reject("markup capacity overflow",[](Memory& m){m.put<std::int32_t>(Fixture::widget+0x598,1025);});
 reject("different datastore",[](Memory& m){m.put(Fixture::widget+0x5b0,Fixture::panel);});
 reject("object registry mismatch",[](Memory& m){m.put(Fixture::objects+8,Fixture::panel);});
 reject("object index out of range",[](Memory& m){m.put<std::uint32_t>(Fixture::widget+8,1001);});
 reject("invalid object registry count",[](Memory& m){m.put(Fixture::base+0x1c24730,gears_graphics_menu::Registry{Fixture::objects,1000,1001});});
 reject("invalid name registry capacity",[](Memory& m){m.put(Fixture::base+0x1c34800,gears_graphics_menu::Registry{Fixture::names,1001,1000});});
 reject("name entry index mismatch",[](Memory& m){m.put<std::uint32_t>(0x50000100,2);});
 reject("name index out of range",[](Memory& m){m.put<std::uint32_t>(Fixture::widget+0x48,1001);});
 for(int before:{-1,4,INT32_MAX})reject("invalid current index",[before](Memory& m){m.put<std::int32_t>(Fixture::widget+0x5d8,before);});
 Memory empty;
 auto s=gears_graphics_menu::select(empty,Fixture::base,UINTPTR_MAX-0x100,3);
 expect(!s.matched&&s.forward==3&&empty.reads==0,"widget address overflow rejected before read");
 std::printf("graphics_menu_lock: %s (%d failures)\n",failures?"FAIL":"PASS",failures);
 return failures?1:0;
}
