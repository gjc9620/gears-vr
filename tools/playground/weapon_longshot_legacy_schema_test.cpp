#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>
using uptr=std::uintptr_t;
namespace hand {struct ObjectIdentity {uptr address{};};}
namespace meta {
using U32=std::uint32_t;constexpr unsigned max_fields=64;
struct Identity {uptr klass{},outer{};};
struct Structure {U32 identity{};uptr super{},children{};U32 extent{};};
struct Reader {
 struct {std::array<Identity,128> identities{};} result;
 std::array<std::wstring,128> names{};std::array<uptr,128> next{};
 std::array<bool,128> present{},field{};
 bool object_known=true,core_class=true,change_link{},change_structure{},change_name{},lose_field_on_close{};
 unsigned reads{},structures{},children{14};uptr head=10;
 Reader(){
  present[1]=present[2]=present[3]=present[4]=true;
  result.identities[1].klass=2;result.identities[2]={4,3};
  names[2]=L"AnimSequence";names[3]=L"Engine";
  const wchar_t* labels[]={L"Notifies",L"RawAnimData",L"TranslationData",L"RotationData",L"CompressedByteStream",
   L"CompressedTrackOffsets",L"RotationCompressionFormat",L"TranslationCompressionFormat",L"CompressionScheme",
   L"bNoLoopingInterpolation",L"RateScale",L"NumFrames",L"SequenceLength",L"SequenceName"};
  for(unsigned i=0;i<children;++i){unsigned id=10+i;present[id]=field[id]=true;names[id]=labels[i];result.identities[id].outer=2;next[id]=i+1<children?id+1:0;}
 }
 bool path(uptr address,U32& slot){if(address>=128||!present[address])return false;slot=U32(address);if(change_name&&reads>=children&&address==10)names[10]=L"KeyEncodingFormat";return true;}
 template<std::size_t N>bool named(U32 slot,wchar_t const(&name)[N],bool=true){return names[slot]==name;}
 bool structure(uptr address,Structure& out){if(address!=2)return false;++structures;out={2,5,head,500};if(change_structure&&structures>1)out.extent=600;return true;}
 bool at(uptr address,uptr offset,uptr& value){assert(offset==0x68);++reads;if(!present[address])return false;value=next[address];if(change_link&&reads>children)value=0;return true;}
};
}
namespace weaponhand {
bool object(meta::Reader& r,hand::ObjectIdentity const& object){return r.object_known&&object.address==1;}
template<std::size_t N>bool core_type(meta::Reader& r,unsigned slot,wchar_t const(&name)[N]){return r.core_class&&slot==4&&std::wstring(name)==L"Class";}
template<std::size_t N>bool derives(meta::Reader& r,uptr address,wchar_t const(&name)[N],wchar_t const* package){return r.field[address]&&std::wstring(name)==L"Field"&&std::wstring(package)==L"Core"&&!(r.lose_field_on_close&&r.reads>=r.children);}
}
namespace longshot_animation {
#include "weapon_longshot_legacy_schema.inl"
}
int main(){
 const hand::ObjectIdentity sequence{1};unsigned rejected{};
 {meta::Reader r;assert(longshot_animation::legacy_uniform_schema(r,sequence));assert(r.reads==2*r.children&&r.structures==2);}
 for(unsigned reason=0;reason<12;++reason){meta::Reader r;
  switch(reason){
   case 0:r.names[2]=L"DerivedAnimSequence";break;
   case 1:r.names[3]=L"OtherPackage";break;
   case 2:r.result.identities[3].outer=99;break;
   case 3:r.names[13]=L"KeyEncodingFormat";break;
   case 4:r.result.identities[13].outer=99;break;
   case 5:r.next[13]=10;break;
   case 6:r.present[13]=false;break;
   case 7:r.change_link=true;break;
   case 8:r.change_structure=true;break;
   case 9:r.change_name=true;break;
   case 10:r.lose_field_on_close=true;break;
   case 11:r.head=0;break;
  }
  assert(!longshot_animation::legacy_uniform_schema(r,sequence));assert(r.reads<=2*meta::max_fields);++rejected;
 }
 {meta::Reader r;r.core_class=false;assert(!longshot_animation::legacy_uniform_schema(r,sequence));++rejected;}
 {meta::Reader r;r.object_known=false;assert(!longshot_animation::legacy_uniform_schema(r,sequence));++rejected;}
 {meta::Reader r;r.children=65;for(unsigned i=10;i<75;++i){r.present[i]=r.field[i]=true;r.result.identities[i].outer=2;r.next[i]=i==74?0:i+1;}assert(!longshot_animation::legacy_uniform_schema(r,sequence));assert(r.reads==64);++rejected;}
 std::printf("Longshot legacy schema: complete stable exact-class absence proof accepted; %u unknown/newer/cyclic/changed/oversized schemas rejected; no engine calls or writes\n",rejected);
}
