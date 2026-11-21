#include "../vr-common/weapon_gnasher_fire.h"
#include "../vr-common/weapon_call_boundary.h"
#include <windows.h>
#include <atomic>
#include <array>
#include <cstring>
#include <cwchar>
#include <cstdio>
#include <cstdlib>
#ifdef NDEBUG
#error GnasherRefireTest requires active checks
#endif
#define CHECK(v) do{if(!(v)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#v);std::abort();}}while(false)
using uptr=std::uintptr_t;namespace hand=gears_weapon_hand;
struct Array {uptr data{};int count{},capacity{};};
uptr base=0x10000000,fixture_function{},prop{};bool current=true,registered=true,manual=true,objects=true,layout_owner=true,cas_ok=true;
unsigned descriptor_reads{},writes{};std::atomic<bool> enabled{true};
hand::Frame source{};
alignas(8) std::array<unsigned char,512> function_memory{},property_memory{};
std::array<unsigned char,8> script_bytes{1,2,3,4,5,6,7,8};
template<class T>T get(uptr p){T out{};std::memcpy(&out,reinterpret_cast<void const*>(p),sizeof out);return out;}
template<class T>void put(uptr p,T v){std::memcpy(reinterpret_cast<void*>(p),&v,sizeof v);}
bool copy(void* out,uptr p,std::size_t n){std::memcpy(out,reinterpret_cast<void const*>(p),n);return true;}
hand::ObjectIdentity object(uptr p){return {p,0x8000,0x9000,unsigned(p/8),7,0,1};}
namespace weaponhand {
unsigned current_gt(void*){return GetCurrentThreadId();}
struct Client {bool registration_alive(std::uint64_t c,std::uint64_t n){return registered&&c==source.key.registration_cookie&&n==source.key.native_started_qpc;}} client;
}
namespace nativeequipment {
bool current(void*,hand::Frame const&){return ::current;}
bool identify(void*,uptr p,hand::ObjectIdentity& out){out=object(p);return true;}
}
namespace unified_weapon_fire {bool manual_profile(){return manual;}}
namespace playground {
bool instance(uptr p,wchar_t const* label){return p==source.weapon.address&&!std::wcscmp(label,L"Weap_Shotgun");}
bool named(uptr p,wchar_t const* label){return (p==fixture_function&&!std::wcscmp(label,L"ShouldRefire"))||
    (p==prop&&!std::wcscmp(label,L"ReturnValue"))||(p==0x7000&&!std::wcscmp(label,L"BoolProperty"));}
uptr pointer(uptr p,wchar_t const* label){
    if(!std::wcscmp(label,L"WorldInfo")&&(p==source.controller.address||p==source.pawn.address))return source.world.address;
    if(p==source.controller.address&&!std::wcscmp(label,L"Pawn"))return source.pawn.address;
    if(p==source.pawn.address&&!std::wcscmp(label,L"Controller"))return source.controller.address;
    if(p==source.weapon.address&&!std::wcscmp(label,L"Instigator"))return source.pawn.address;
    return 0;
}
struct Args {struct Record {unsigned size=4;uptr function=fixture_function;} r;
    Args(uptr,wchar_t const*){++descriptor_reads;}
    uptr prop(wchar_t const*){return ::prop;}
    bool run()=delete; // No engine execution path exists in this fixture.
};
}
namespace vrequip {bool result_property(playground::Args const&,wchar_t const*,unsigned){return true;}}
namespace lancerreload_ui {
bool object(hand::ObjectIdentity const& id){return objects&&id.known==1&&id.name_index==7;}
bool original_function(uptr p,wchar_t const* label,wchar_t const* owner,bool){return layout_owner&&p==fixture_function&&!std::wcscmp(label,L"ShouldRefire")&&!std::wcscmp(owner,L"WarWeapon");}
}
namespace weaponshot {
bool hash_script(uptr p,unsigned n,std::uint64_t& out){out=14695981039346656037ull;for(unsigned i=0;i<n;++i){out^=get<unsigned char>(p+i);out*=1099511628211ull;}return true;}
}
namespace lancerreload_manual {
struct Binding {hand::ObjectIdentity world{},controller{},pawn{},weapon{},function{},property{};
    std::uint64_t cookie{},native{},hold{},script_hash{},exec{};Array script{};unsigned mask{},offset{},extent{};bool valid{},starter{};};
// The real shared namespace also owns these overloads. Keep them present
// so the production Binding alias cannot accidentally use unqualified ADL.
bool layout(Binding const&){return false;}
bool current(gears_weapon_call::Invocation const&,Binding const&){return false;}
bool compare_result(uptr p,unsigned before,unsigned after,bool& exchanged){exchanged=false;if(!cas_ok)return false;
    if(get<unsigned>(p)==before){put(p,after);exchanged=true;++writes;}return true;}
}
namespace lancerreload_inventory {
struct Identity {hand::ObjectIdentity weapon{};bool operator==(Identity const& b)const{return hand::same(weapon,b.weapon);}};
Identity identity(hand::Frame const& f){return {f.weapon};}
}
namespace gnasher_fire {
struct Entry {bool prepared=true;gears_weapon::GnasherFireCycle cycle;};Entry state;
Entry* entry(hand::Frame const& f,bool){return hand::same(f.weapon,source.weapon)?&state:nullptr;}
bool blocks_fire(hand::ObjectIdentity const& w){return hand::same(w,source.weapon)&&state.cycle.blocked();}
}
#include "weapon_gnasher_refire.inl"
int main(){
 source.key={11,22,33,1,1,1,44,55};source.world=object(0x6000);source.controller=object(0x6010);
 source.pawn=object(0x6020);source.weapon=object(0x5000);
 fixture_function=reinterpret_cast<uptr>(function_memory.data());prop=reinterpret_cast<uptr>(property_memory.data());
 put<unsigned>(fixture_function+0xf4,0);put<std::uint16_t>(fixture_function+0xf8,0);put<uptr>(fixture_function+0x114,base+0x1d6ca0);
 put<std::uint16_t>(fixture_function+0x106,4);put<std::uint16_t>(fixture_function+0x108,0);put<unsigned>(fixture_function+0x88,4);
 put<Array>(fixture_function+0x8c,{reinterpret_cast<uptr>(script_bytes.data()),8,8});
 put<uptr>(prop+0x40,fixture_function);put<uptr>(prop+0x50,0x7000);put<unsigned>(prop+0x70,1);put<unsigned>(prop+0x74,4);
 put<unsigned long long>(prop+0x78,0x580);put<unsigned>(prop+0x94,0);put<unsigned>(prop+0xd0,4);
 gears_weapon::GnasherIdentity id{1,2,3,4,5,6,7,8};CHECK(gnasher_fire::state.cycle.bind({id,1,8,10}));
 CHECK(gnasher_refire::prepare(source));CHECK(gnasher_refire::bound_for(source));
 alignas(4) unsigned result=0xa5;gears_weapon_call::Invocation call{};call.hand_cookie=33;call.native_started_qpc=22;
 call.call_id=1;call.thread_id=GetCurrentThreadId();call.self=source.weapon.address;call.callee=fixture_function;call.result=reinterpret_cast<uptr>(&result);
 CHECK(gnasher_refire::returned(call));CHECK(result==0xa5&&writes==0); // Initial loaded gun is unaffected.
 CHECK(gnasher_fire::state.cycle.returned(1,1000,100));const auto reads=descriptor_reads;
 for(unsigned field=0;field<8;++field){auto wrong=call;
  if(field==0)++wrong.self;if(field==1)++wrong.hand_cookie;if(field==2)++wrong.native_started_qpc;
  if(field==3)++wrong.thread_id;if(field==4)wrong.reserved=1;if(field==5)++wrong.callee;
  if(field==6)wrong.result=0;if(field==7)wrong.abi_version++;
  CHECK(gnasher_refire::returned(wrong));CHECK(result==0xa5&&writes==0);}
 for(unsigned field=0;field<6;++field){
  if(field==0)registered=false;if(field==1)objects=false;if(field==2)layout_owner=false;
  if(field==3)manual=false;if(field==4)enabled=false;if(field==5)script_bytes[0]^=1;
  CHECK(gnasher_refire::returned(call));CHECK(result==0xa5&&writes==0);
  registered=objects=layout_owner=manual=true;enabled=true;if(field==5)script_bytes[0]^=1;}
 CHECK(gnasher_refire::returned(call));CHECK(result==0xa1&&writes==1&&gnasher_refire::blocked==1);
 CHECK(descriptor_reads==reads);CHECK(gnasher_fire::state.cycle.pending());
 CHECK(gnasher_refire::returned(call));CHECK(result==0xa1&&writes==1); // False remains false, other bits preserved.
 result=0xa5;cas_ok=false;CHECK(!gnasher_refire::returned(call));CHECK(result==0xa5);cas_ok=true;
 CHECK(gnasher_fire::state.cycle.confirm({id,2,7,10}));CHECK(gnasher_refire::returned(call));CHECK(result==0xa1);
 CHECK(gnasher_fire::state.cycle.request_cycle(1,1));CHECK(gnasher_fire::state.cycle.acknowledge_cycle(1,{id,2,7,10}));
 result=0xa5;CHECK(gnasher_refire::returned(call));CHECK(result==0xa5); // Only actual cycle ACK permits original result again.
 std::puts("Production Gnasher ShouldRefire veto: actual cycle debt, exact call/receiver/schema/hash, no callback engine execution, false-only bit and ACK boundaries passed");
}
