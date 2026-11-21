#include "weapon_hand_offline_fixture.h"
namespace {
namespace unified_weapon_fire {
namespace metadata=gripdamage_metadata;using U32=std::uint32_t;
#include "weapon_muzzle_getter_call.inl"
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
namespace boomshot_inspection_clip {
namespace meta=gripdamage_metadata;
#include "weapon_boomshot_node_getter_schema.inl"
}
void fake_getter_endpoint(){}
struct GetterFixture:Memory {
    uptr owner{},function{},result{},skel{},anim{},node_class{};
    struct Call {bool valid=true;struct Request {uptr function{};unsigned size=8;unsigned char params[64]{};}r;}call;
    GetterFixture(){
        owner=cls(L"Weap_Boomer_base",warfare,weapon_class);const auto content=node(L"warfaregamecontentweapons",package_type);
        put(weapon,0x50,cls(L"Weap_Boomer",content,owner));node_class=cls(L"AnimNodeSequence",engine);
        const auto function_type=node(L"Function",class_type,core);function=node(L"GetWeaponAnimNodeSeq",function_type,owner);
        put(function,0x88,24u);put(function,0x106,std::uint16_t(8));put(function,0x108,std::uint16_t(0));
        put(function,0x114,reinterpret_cast<uptr>(&fake_getter_endpoint));
        result=property(function,L"ReturnValue",object_type,0,8);put(result,0x78,std::uint64_t(0x480));put(result,0xd0,node_class);
        skel=property(function,L"SkelMesh",component_type,8,8);put(skel,0xd0,mesh_class);
        anim=property(function,L"AnimSeq",object_type,16,8);put(anim,0xd0,node_class);
        call.r.function=function;tables();
    }
    bool inspect(boomshot_inspection_clip::NodeGetterLayout& out){
        meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader reader{snapshot,meta::Stage::final_check};
        return meta::tables(base,snapshot.objects,snapshot.names)&&boomshot_inspection_clip::node_getter_schema(call,weapon,reader,out);
    }
};
}
int main(){unsigned cases_count{};using Layout=boomshot_inspection_clip::NodeGetterLayout;
 {auto m=std::make_unique<GetterFixture>();Layout value{};assert(m->inspect(value)&&value.result==0&&value.local_offsets[0]==8&&value.local_offsets[1]==16);++cases_count;}
 for(unsigned reason=0;reason<20;++reason){auto m=std::make_unique<GetterFixture>();
  switch(reason){
   case 0:Memory::put(m->function,0x40,m->weapon_class);break; // WarWeapon fallback is not this original override.
   case 1:Memory::put(m->owner,0x48,m->text(L"Weap_AssaultRifle"));break;
   case 2:Memory::put(m->owner,0x40,m->engine);break;
   case 3:Memory::put(m->function,0xf4,0x400u);break;
   case 4:Memory::put(m->function,0xf8,std::uint16_t(1));break;
   case 5:Memory::put(m->function,0x114,m->weapon);break;
   case 6:Memory::put(m->function,0x108,std::uint16_t(8));break;
   case 7:Memory::put(m->result,0xd0,m->actor_class);break;
   case 8:Memory::put(m->result,0x78,std::uint64_t(0x80));break;
   case 9:Memory::put(m->skel,0x50,m->object_type);break;
   case 10:Memory::put(m->skel,0xd0,m->actor_class);break;
   case 11:Memory::put(m->anim,0x94,8u);break;
   case 12:Memory::put(m->anim,0x78,std::uint64_t(0x80));break;
   case 13:Memory::put(m->skel,0x48,m->text(L"OtherLocal"));break;
   case 14:Memory::put(m->anim,0x68,m->result);break;
   case 15:Memory::put(m->result,0x70,2u);break;
   case 16:m->call.r.size=16;break;
   case 17:Memory::put(m->weapon,0x50,m->concrete);break;
   case 18:fail_read=m->anim+0xd0;break;
   case 19:mutate_read=m->anim+0xd0;mutation=[&]{Memory::put(m->owner,0x48,m->text(L"OtherOwner"));};break;
  }
  Layout value{};value.bytes=1;assert(!m->inspect(value)&&!value.bytes);++cases_count;
 }
 std::printf("Boomshot overridden node getter: %u cases passed; exact original owner/receiver, return and ComponentProperty locals, changed/recycled/foreign rejection; owned memory only\n",cases_count);
}
