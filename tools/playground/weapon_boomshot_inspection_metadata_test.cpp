#include "weapon_hand_offline_fixture.h"
namespace {
namespace hand=gears_weapon_hand;
namespace unified_weapon_fire {
namespace metadata=gripdamage_metadata;using U32=std::uint32_t;
#include "weapon_muzzle_getter_call.inl"
}
namespace boomshot_animation {
namespace meta=gripdamage_metadata;namespace hand=gears_weapon_hand;
#include "weapon_boomshot_animation_identity.inl"
}
struct InspectionFixture:Memory {
    uptr sequence{},sequence_class{},set{},folder{},package{},sequence_name{},notifies{},inner{},notify_structure{},struct_type{};
    std::array<unsigned char,64> unused_capacity{};
    InspectionFixture(){
        const auto set_class=cls(L"AnimSet",engine);sequence_class=cls(L"AnimSequence",engine);
        package=node(L"Locust_Boomshot",package_type);folder=node(L"animation",package_type,package);
        set=node(L"Animset_Locust_Boomshot",set_class,folder);sequence=node(L"AnimSequence_2",sequence_class,set);
        const auto name_type=node(L"NameProperty",class_type,core),array_type=node(L"ArrayProperty",class_type,core);
        struct_type=node(L"StructProperty",class_type,core);const auto script_type=node(L"ScriptStruct",class_type,core);
        notify_structure=node(L"AnimNotifyEvent",script_type,sequence_class);put(notify_structure,0x88,20u);
        sequence_name=property(sequence_class,L"SequenceName",name_type,0x100,8);
        notifies=property(sequence_class,L"Notifies",array_type,0x110,16);inner=node(L"Notifies",struct_type,notifies);
        put(inner,0x70,1u);put(inner,0x74,20u);put(inner,0xd0,notify_structure);put(notifies,0xd0,inner);
        put(sequence,0x100,text(L"AR_Reload"));put(sequence,0x110,meta::Table{});tables();
    }
    bool inspect(){meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader reader{snapshot,meta::Stage::final_check};
        return meta::tables(base,snapshot.objects,snapshot.names)&&boomshot_animation::inspection_no_notifies(reader,identity(sequence));}
};
}
int main(){unsigned cases_count{};
 {auto m=std::make_unique<InspectionFixture>();assert(m->inspect());++cases_count;}
 {auto m=std::make_unique<InspectionFixture>();Memory::put(m->sequence,0x110,meta::Table{reinterpret_cast<uptr>(m->unused_capacity.data()),0,2});
  assert(m->inspect());++cases_count;} // Allocated empty storage still has zero events.
 for(unsigned reason=0;reason<18;++reason){auto m=std::make_unique<InspectionFixture>();
  switch(reason){
   case 0:Memory::put(m->sequence,0x100,m->text(L"AR_Fire_Boomshot"));break;
   case 1:Memory::put(m->sequence,0x110,meta::Table{reinterpret_cast<uptr>(m->unused_capacity.data()),1,1});break;
   case 2:Memory::put(m->sequence,0x110,meta::Table{0,0,-1});break;
   case 3:Memory::put(m->sequence,0x110,meta::Table{0,0,1});break;
   case 4:Memory::put(m->sequence,0x110,meta::Table{reinterpret_cast<uptr>(m->unused_capacity.data()),0,1025});break;
   case 5:Memory::put(m->inner,0x40,m->sequence_class);break;
   case 6:Memory::put(m->inner,0x70,2u);break;
   case 7:Memory::put(m->inner,0x74,4u);break;
   case 8:Memory::put(m->notify_structure,0x40,m->pawn_class);break;
   case 9:Memory::put(m->notify_structure,0x88,0u);break;
   case 10:Memory::put(m->set,0x48,m->text(L"Animset_COGAssaultRifle"));break;
   case 11:Memory::put(m->package,0x40,m->engine);break;
   case 12:Memory::put(m->folder,0x48,m->text(L"Mesh"));break;
   case 13:Memory::put(m->inner,0x50,m->object_type);break;
   case 14:fail_read=m->notifies+0xd0;break;
   case 15:mutate_read=m->sequence+0x110;mutation=[&]{Memory::put(m->sequence,0x100,m->text(L"Other"));};break;
   case 16:mutate_read=m->sequence+0x110;mutation=[&]{Memory::put(m->sequence,0x110,meta::Table{0,1,1});};break;
   case 17:mutate_read=m->notify_structure+0x88;mutation=[&]{Memory::put(m->inner,0x74,24u);};break;
  }
  assert(!m->inspect());++cases_count;
 }
 std::printf("Boomshot zero-target clip metadata: %u cases passed; exact original set, AR_Reload, typed empty Notifies and repeated reads; no engine call\n",cases_count);
 return 0;
}
