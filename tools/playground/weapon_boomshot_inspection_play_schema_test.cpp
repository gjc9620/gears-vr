#include "weapon_hand_offline_fixture.h"
namespace {
namespace hand=gears_weapon_hand;
template<class T>T get(uptr address){T value{};copy(&value,address,sizeof value);return value;}
bool registered(uptr address){meta::Table objects{},names{};unsigned index{};
 return meta::tables(base,objects,names)&&meta::registered_known(objects,address,index);}
namespace playground {
bool named(uptr address,wchar_t const* label){
 meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader reader{snapshot,meta::Stage::final_check};unsigned index{};
 if(!meta::tables(base,snapshot.objects,snapshot.names)||!reader.path(address,index))return false;
 const auto& text=snapshot.name_text[snapshot.identities[index].name_slot];
 return text.length==std::wcslen(label)&&!std::wmemcmp(text.units.data(),label,text.length);
}
struct Args {
 struct Request {uptr function{};unsigned size{};unsigned char params[4096]{};}r;bool valid=true;
 uptr prop(wchar_t const* label){for(auto p=get<uptr>(r.function+0x80);p;p=get<uptr>(p+0x68))if(named(p,label))return p;return 0;}
};
}
namespace nativeequipment {
bool identify(void*,uptr address,hand::ObjectIdentity& out){
 meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader reader{snapshot,meta::Stage::final_check};unsigned i{};
 if(!meta::tables(base,snapshot.objects,snapshot.names)||!reader.path(address,i))return false;
 auto const& a=snapshot.identities[i];out={a.address,a.outer,a.klass,a.index,a.name.index,a.name.number,1};return true;
}
}
namespace lancer_charging {
bool script_call(playground::Args const& call,wchar_t const* label){const auto owner=get<uptr>(call.r.function+0x40);
 const auto package=get<uptr>(owner+0x40);return call.valid&&registered(call.r.function)&&playground::named(call.r.function,label)&&
 registered(owner)&&registered(package)&&playground::named(owner,L"WarWeapon")&&playground::named(package,L"WarfareGame")&&!get<uptr>(package+0x40);}
}
namespace vrequip {
bool result_property(playground::Args& call,wchar_t const* kind,unsigned width){const auto p=call.prop(L"ReturnValue");
 return p&&playground::named(get<uptr>(p+0x50),kind)&&get<unsigned>(p+0x74)==width&&
 (get<std::uint64_t>(p+0x78)&0x480)==0x480;}
}
namespace boomshot_inspection_clip {
#include "weapon_boomshot_inspection_play_schema.inl"
}
struct PlayFixture:Memory {
    uptr function{};std::array<uptr,7> parameters{};playground::Args call;
    PlayFixture(){
        const auto function_type=node(L"Function",class_type,core),name_type=node(L"NameProperty",class_type,core),float_type=node(L"FloatProperty",class_type,core);
        function=node(L"PlayWeaponAnim",function_type,weapon_class);put(function,0x106,std::uint16_t(32));put(function,0x108,std::uint16_t(28));
        const wchar_t* parameter_names[]={L"AnimName",L"Rate",L"BlendInTime",L"BlendOutTime",L"bLooping",L"bOverride",L"ReturnValue"};
        const unsigned offsets[]={0,8,12,16,20,24,28};
        for(unsigned i=0;i<7;++i){parameters[i]=property(function,parameter_names[i],i==0?name_type:(i==4||i==5)?bool_type:float_type,offsets[i],i?4:8,i==4||i==5?1:0);
            put(parameters[i],0x78,std::uint64_t(i==6?0x480:0x80));}
        call.r.function=function;call.r.size=32;tables();
    }
    bool inspect(boomshot_inspection_clip::PlayLayout& out){return boomshot_inspection_clip::play_layout(call,out);}
};
}
int main(){unsigned cases_count{};using Layout=boomshot_inspection_clip::PlayLayout;
 {auto m=std::make_unique<PlayFixture>();Layout out{};assert(m->inspect(out)&&out.offsets[0]==0&&out.offsets[6]==28);++cases_count;}
 {auto m=std::make_unique<PlayFixture>();Memory::put(m->parameters[5],0x94,20u);Memory::put(m->parameters[5],0xd0,2u);
  Memory::put(m->parameters[6],0x94,24u);Memory::put(m->function,0x108,std::uint16_t(24));m->call.r.size=28;
  Layout out{};assert(m->inspect(out)&&out.offsets[4]==out.offsets[5]&&out.masks[4]==1&&out.masks[5]==2);++cases_count;}
 for(unsigned reason=0;reason<16;++reason){auto m=std::make_unique<PlayFixture>();
  switch(reason){
   case 0:Memory::put(m->function,0x40,m->actor_class);break;
   case 1:Memory::put(m->parameters[0],0x74,4u);break;
   case 2:Memory::put(m->parameters[2],0x94,8u);break;
   case 3:Memory::put(m->parameters[5],0x94,20u);break;
   case 4:Memory::put(m->parameters[4],0xd0,0u);break;
   case 5:Memory::put(m->parameters[4],0xd0,3u);break;
   case 6:Memory::put(m->parameters[6],0x78,std::uint64_t(0x80));break;
   case 7:Memory::put(m->parameters[2],0x78,std::uint64_t(0x180));break;
   case 8:Memory::put(m->parameters[3],0x70,2u);break;
   case 9:Memory::put(m->function,0x108,std::uint16_t(0));break;
   case 10:Memory::put(m->parameters[4],0x94,18u);break;
   case 11:Memory::put(m->parameters[1],0x40,m->weapon_class);break;
   case 12:m->call.r.size=27;break;
   case 13:Memory::put(m->parameters[5],0x50,m->object_type);break;
   case 14:m->call.valid=false;break;
   case 15:Memory::put(m->function,0x48,m->text(L"ForceReload"));break;
  }
  Layout out{};out.bytes=1;assert(!m->inspect(out)&&!out.bytes);++cases_count;
 }
 std::printf("Boomshot original PlayWeaponAnim layout: %u cases passed; typed named fields, reflected offsets/return, distinct or proven disjoint Bool masks; owned memory only\n",cases_count);
}
