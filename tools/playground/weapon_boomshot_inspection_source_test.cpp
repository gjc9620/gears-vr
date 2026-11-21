#define main existing_inspection_metadata_cases
#include "weapon_boomshot_inspection_metadata_test.cpp"
#undef main
namespace {
using Name=meta::RawName;
bool source_current=true,native_reload=false,asset_valid=true;unsigned getter_calls{},asset_reads{};
uptr getter_function{},getter_result{};std::function<void()> getter_mutation;
namespace nativeequipment {bool current(void*,hand::Frame const&){return source_current;}}
namespace lancerreload_inventory {
struct Counts {int capacity=1,used{},reserve=2;bool infinite{};
 bool operator==(Counts const& b)const{return capacity==b.capacity&&used==b.used&&reserve==b.reserve&&infinite==b.infinite;}}counts;
bool read(hand::Frame const&,Counts& out){out=counts;return source_current;}
}
namespace lancerreload_state {
struct Lease {bool acquired{};}lease;
bool bool_call(uptr,wchar_t const* label,bool& out){assert(!std::wcscmp(label,L"IsReloading"));out=native_reload;return true;}
}
namespace hammerburst_animation {struct Ownership {bool active{};}ownership;}
namespace lancermain_asset {
enum class Status {ready,invalid};enum class AssetScope {boomshot_projectile};
struct Evidence {hand::ObjectIdentity component{},asset{};};hand::ObjectIdentity source_asset{};
Status read(hand::Frame const& f,Evidence& out,AssetScope){++asset_reads;out={f.mesh,source_asset};return asset_valid?Status::ready:Status::invalid;}
bool identify(meta::Reader& r,uptr address,hand::ObjectIdentity& out){unsigned i{};
 if(!r.path(address,i))return false;auto const& a=r.result.identities[i];out={a.address,a.outer,a.klass,a.index,a.name.index,a.name.number,1};return true;}
}
namespace unified_weapon_fire {
bool owner_in_receiver(meta::Reader& r,uptr receiver,uptr owner) noexcept {
    unsigned object{};if(!r.path(receiver,object))return false;
    auto cls=r.result.identities[object].klass;std::array<uptr,meta::max_parents> seen{};unsigned count{};
    while(cls){if(count==seen.size())return false;
        for(unsigned i=0;i<count;++i)if(seen[i]==cls)return false;
        seen[count++]=cls;meta::Structure structure{};if(!r.structure(cls,structure))return false;
        if(cls==owner)return true;cls=structure.super;}
    return false;
}
}
namespace playground {
struct Args {
    struct Request {uptr function{};unsigned size=16;unsigned char params[64]{};}r;bool valid=true;
    Args(uptr,wchar_t const* label){assert(!std::wcscmp(label,L"FindAnimSequence"));r.function=getter_function;}
    bool run(){++getter_calls;std::memcpy(r.params+8,&getter_result,8);if(getter_mutation)getter_mutation();return true;}
};
}
#include "weapon_boomshot_inspection_source.inl"
struct SourceFixture:InspectionFixture {
    uptr function{};
    SourceFixture(){
        source_current=asset_valid=true;native_reload=false;getter_calls=asset_reads=0;getter_mutation={};
        lancerreload_inventory::counts={};lancerreload_state::lease={};hammerburst_animation::ownership={};
        const auto function_type=node(L"Function",class_type,core),name_type=node(L"NameProperty",class_type,core);
        function=node(L"FindAnimSequence",function_type,mesh_class);put(function,0xf4,0x401u);
        put(function,0x106,std::uint16_t(16));put(function,0x108,std::uint16_t(8));
        put(function,0x114,reinterpret_cast<uptr>(&SourceFixture::endpoint));
        const auto in=property(function,L"AnimSeqName",name_type,0,8),out=property(function,L"ReturnValue",object_type,8,8);
        put(in,0x78,std::uint64_t(0x80));put(out,0x78,std::uint64_t(0x480));put(out,0xd0,sequence_class);
        property(weapon_class,L"WeaponReloadAnim",name_type,0x140,8);put(weapon,0x140,text(L"AR_Reload"));
        lancermain_asset::source_asset=identity(node(L"OriginalMesh",class_type,engine));
        getter_function=function;getter_result=sequence;tables();
    }
    ~SourceFixture(){getter_mutation={};}
    static void endpoint(){}
    bool read(boomshot_inspection_source::Source& out){return boomshot_inspection_source::read(frame,out);}
};
}
int main(){unsigned cases_count{};
 for(unsigned mode=0;mode<3;++mode){auto m=std::make_unique<SourceFixture>();
  if(mode==1)lancerreload_inventory::counts={1,1,0,false};if(mode==2)lancerreload_inventory::counts.infinite=true;
  boomshot_inspection_source::Source out{};assert(m->read(out)&&getter_calls==2&&out.sequence.address==m->sequence);
  assert(out.counts==lancerreload_inventory::counts);++cases_count;
 }
 for(unsigned reason=0;reason<16;++reason){auto m=std::make_unique<SourceFixture>();
  switch(reason){
   case 0:source_current=false;break;
   case 1:hammerburst_animation::ownership.active=true;break;
   case 2:lancerreload_state::lease.acquired=true;break;
   case 3:lancerreload_inventory::counts={1,1,1,false};break;
   case 4:lancerreload_inventory::counts={1,1,0,true};break;
   case 5:lancerreload_inventory::counts.capacity=2;break;
   case 6:native_reload=true;break;
   case 7:Memory::put(m->weapon,0x140,m->text(L"WrongReload"));break;
   case 8:getter_result=m->pawn;break;
   case 9:Memory::put(m->sequence,0x110,meta::Table{0,1,1});break;
   case 10:getter_mutation=[&]{if(getter_calls==1)getter_result=m->pawn;};break;
   case 11:getter_mutation=[&]{lancerreload_inventory::counts.reserve=1;};break;
   case 12:getter_mutation=[&]{source_current=false;};break;
   case 13:getter_mutation=[&]{native_reload=true;};break;
   case 14:getter_mutation=[&]{asset_valid=false;};break;
   case 15:Memory::put(m->function,0x114,m->pawn);break;
  }
  boomshot_inspection_source::Source out{};out.sequence.address=0x1234;
  assert(!m->read(out)&&out.sequence.address==0x1234);++cases_count;
 }
 std::printf("Boomshot inspection source: %u cases passed; zero-target only, two original-getter stubs, unchanged counts/native state/assets and no Play/Stop/Seek/ForceReload\n",cases_count);
}
