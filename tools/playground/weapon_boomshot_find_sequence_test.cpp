#include "weapon_hand_offline_fixture.h"
namespace {
namespace unified_weapon_fire {
namespace metadata=gripdamage_metadata;using U32=std::uint32_t;
bool owner_in_receiver(metadata::Reader& r,uptr receiver,uptr owner) noexcept {
    U32 object{};if(!r.path(receiver,object))return false;
    auto cls=r.result.identities[object].klass;std::array<uptr,metadata::max_parents> seen{};unsigned count{};
    while(cls){if(count==seen.size())return false;
        for(unsigned i=0;i<count;++i)if(seen[i]==cls)return false;
        seen[count++]=cls;metadata::Structure structure{};if(!r.structure(cls,structure))return false;
        if(cls==owner)return true;cls=structure.super;}
    return false;
}
}
namespace boomshot_inspection_source {
namespace meta=gripdamage_metadata;
#include "weapon_boomshot_find_sequence.inl"
}
void fake_find_endpoint() {}
struct FindFixture:Memory {
    uptr function{},input{},result{},sequence_class{};
    struct Call {bool valid=true;struct Request {uptr function{};unsigned size=16;unsigned char params[64]{};}r;}call;
    FindFixture(){
        const auto function_type=node(L"Function",class_type,core),name_type=node(L"NameProperty",class_type,core);
        sequence_class=cls(L"AnimSequence",engine);function=node(L"FindAnimSequence",function_type,mesh_class);
        put(function,0xf4,0x401u);put(function,0xf8,std::uint16_t(0));put(function,0x106,std::uint16_t(16));
        put(function,0x108,std::uint16_t(8));put(function,0x114,reinterpret_cast<uptr>(&fake_find_endpoint));
        input=property(function,L"AnimSeqName",name_type,0,8);result=property(function,L"ReturnValue",object_type,8,8);
        put(input,0x78,std::uint64_t(0x80));put(result,0x78,std::uint64_t(0x480));put(result,0xd0,sequence_class);
        call.r.function=function;tables();
    }
    bool inspect(boomshot_inspection_source::FindLayout& out){
        meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader reader{snapshot,meta::Stage::final_check};
        return meta::tables(base,snapshot.objects,snapshot.names)&&boomshot_inspection_source::find_schema(call,mesh,reader,out);
    }
};
}
int main(){unsigned cases_count{};using Layout=boomshot_inspection_source::FindLayout;
 {auto m=std::make_unique<FindFixture>();Layout value{};assert(m->inspect(value)&&value.input==0&&value.result==8);++cases_count;}
 {auto m=std::make_unique<FindFixture>();Memory::put(m->input,0x94,24u);Memory::put(m->result,0x94,0u);
  Memory::put(m->function,0x106,std::uint16_t(32));Memory::put(m->function,0x108,std::uint16_t(0));m->call.r.size=32;
  Layout value{};assert(m->inspect(value)&&value.input==24&&value.result==0);++cases_count;}
 for(unsigned reason=0;reason<16;++reason){auto m=std::make_unique<FindFixture>();
  switch(reason){
   case 0:Memory::put(m->function,0x40,m->actor_class);break;
   case 1:Memory::put(m->input,0x48,m->text(L"AnimName"));break;
   case 2:Memory::put(m->input,0x78,std::uint64_t(0x180));break;
   case 3:Memory::put(m->result,0x78,std::uint64_t(0x80));break;
   case 4:Memory::put(m->result,0x94,4u);Memory::put(m->function,0x108,std::uint16_t(4));break;
   case 5:Memory::put(m->function,0x108,std::uint16_t(0));break;
   case 6:Memory::put(m->result,0xd0,m->pawn_class);break;
   case 7:Memory::put(m->sequence_class,0x40,m->warfare);break;
   case 8:Memory::put(m->input,0x70,2u);break;
   case 9:Memory::put(m->result,0x74,4u);break;
   case 10:Memory::put(m->function,0xf4,1u);break;
   case 11:Memory::put(m->function,0x114,m->pawn);break;
   case 12:m->call.r.size=15;break;
   case 13:Memory::put(m->result,0x68,m->input);break;
   case 14:Memory::put(m->result,0x40,m->actor_class);break;
   case 15:fail_read=m->result+0xd0;break;
  }
  Layout value{};value.function=1;assert(!m->inspect(value)&&!value.function);++cases_count;
 }
 std::printf("Boomshot original FindAnimSequence schema: %u cases passed; reflected name/object offsets, exact native owner/return class and strict rejection; no engine call\n",cases_count);
}
