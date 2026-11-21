#include "weapon_hand_offline_fixture.h"
namespace {
namespace unified_weapon_fire {
namespace metadata=gripdamage_metadata;using U32=std::uint32_t;
#include "weapon_muzzle_getter_call.inl"
bool owner_in_receiver(metadata::Reader& r,uptr receiver,uptr owner) noexcept {
 U32 object{};if(!r.path(receiver,object))return false;auto cls=r.result.identities[object].klass;
 std::array<uptr,metadata::max_parents> seen{};unsigned count{};
 while(cls){if(count==seen.size())return false;for(unsigned i=0;i<count;++i)if(seen[i]==cls)return false;
  seen[count++]=cls;metadata::Structure structure{};if(!r.structure(cls,structure))return false;if(cls==owner)return true;cls=structure.super;}return false;
}
}
namespace boomshot_carrier_original {namespace meta=gripdamage_metadata;
#include "weapon_boomshot_carrier_calls.inl"
}
void fake_carrier_endpoint(){}
struct CarrierCallFixture:Memory {
 unsigned shape{};uptr owner{},function{},component{},bone{},returned{},receiver{};
 struct Call {bool valid=true;struct Request{uptr function{};unsigned size{};unsigned char params[64]{};}r;}call;
 explicit CarrierCallFixture(unsigned selected):shape(selected){
  const auto function_type=node(L"Function",class_type,core),name_type=node(L"NameProperty",class_type,core);
  owner=shape==0?cls(L"Weap_Boomer_base",warfare,weapon_class):shape==1?weapon_class:mesh_class;
  if(shape==0)put(weapon,0x50,owner);receiver=shape==2?mesh:weapon;
  function=node(shape==0?L"Notify_AmmoGrab":shape==1?L"SetPawnAmmoAttachment":L"IsComponentAttached",function_type,owner);
  const unsigned size=shape==0?0u:shape==1?4u:20u;call.r.function=function;call.r.size=size;
  put(function,0x88,size);put(function,0x106,std::uint16_t(size));put(function,0x108,std::uint16_t(shape==2?16:0xffff));
  put(function,0x114,reinterpret_cast<uptr>(&fake_carrier_endpoint));if(shape==2)put(function,0xf4,0x400u);
  if(shape==1){component=property(function,L"bAttach",bool_type,0,4);put(component,0x78,std::uint64_t(0x80));put(component,0xd0,1u);}
  if(shape==2){component=property(function,L"Component",component_type,0,8);put(component,0x78,std::uint64_t(0x80));put(component,0xd0,mesh_class);
   bone=property(function,L"BoneName",name_type,8,8);put(bone,0x78,std::uint64_t(0x180));
   returned=property(function,L"ReturnValue",bool_type,16,4);put(returned,0x78,std::uint64_t(0x480));put(returned,0xd0,1u);}
  tables();
 }
 bool inspect(boomshot_carrier_original::Layout& out){meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader r{snapshot,meta::Stage::final_check};
  if(!meta::tables(base,snapshot.objects,snapshot.names))return false;
  if(shape==0)return boomshot_carrier_original::signature(call,receiver,L"Notify_AmmoGrab",L"Weap_Boomer_base",false,0,r,out);
  if(shape==1)return boomshot_carrier_original::signature(call,receiver,L"SetPawnAmmoAttachment",L"WarWeapon",false,1,r,out);
  return boomshot_carrier_original::signature(call,receiver,L"IsComponentAttached",L"SkeletalMeshComponent",true,2,r,out);
 }
};
}
int main(){unsigned count{};using Layout=boomshot_carrier_original::Layout;
 for(unsigned kind=0;kind<3;++kind){auto f=std::make_unique<CarrierCallFixture>(kind);Layout out{};assert(f->inspect(out));++count;}
 for(unsigned bad=0;bad<14;++bad){auto f=std::make_unique<CarrierCallFixture>(2);
  switch(bad){case 0:Memory::put(f->function,0x40,f->weapon_class);break;case 1:Memory::put(f->function,0xf4,0u);break;
   case 2:Memory::put(f->function,0x108,std::uint16_t(12));break;case 3:Memory::put(f->returned,0xd0,0u);break;
   case 4:Memory::put(f->returned,0xd0,3u);break;case 5:Memory::put(f->bone,0x94,0u);break;
   case 6:Memory::put(f->component,0x74,4u);break;case 7:Memory::put(f->component,0x78,std::uint64_t(0x480));break;
   case 8:Memory::put(f->returned,0x78,std::uint64_t(0x80));break;case 9:Memory::put(f->component,0xd0,f->weapon_class);break;
   case 10:Memory::put(f->bone,0x48,f->text(L"Extra"));break;case 11:Memory::put(f->function,0x114,f->weapon);break;
   case 12:Memory::put(f->returned,0x68,f->component);break;case 13:Memory::put(f->function,0x106,std::uint16_t(8));break;}
  Layout out{};assert(!f->inspect(out));++count;
 }
 {auto f=std::make_unique<CarrierCallFixture>(0);Memory::put(f->function,0x108,std::uint16_t(0));Layout out{};assert(!f->inspect(out));++count;}
 {auto f=std::make_unique<CarrierCallFixture>(1);Memory::put(f->component,0xd0,3u);Layout out{};assert(!f->inspect(out));++count;}
 {auto f=std::make_unique<CarrierCallFixture>(0);Memory::put(f->function,0x48,f->text(L"Notify_AmmoReload"));f->tables();
  meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader r{snapshot,meta::Stage::final_check};Layout out{};
  assert(meta::tables(base,snapshot.objects,snapshot.names)&&boomshot_carrier_original::signature(f->call,f->receiver,L"Notify_AmmoReload",L"Weap_Boomer_base",false,0,r,out));++count;}
 std::printf("Boomshot ORIGINAL carrier call schemas: %u owned-memory cases passed; exact Boomer notify, WarWeapon detach and native component/bone/result query; no engine execution\n",count);return 0;
}
