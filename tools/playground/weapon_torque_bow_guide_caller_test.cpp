#include "../vr-common/weapon_torque_bow_guide.h"
#include "../vr-common/body_slots.h"
#include "weapon_torque_bow_arrow_tracking.h"
#include "weapon_torque_bow_arrow_tracking_fixture.h"
#include <cstring>
#include <cwchar>
#include <functional>
#include <map>
#define CHECK BOW_ARROW_CHECK
using uptr=std::uintptr_t;using FARPROC=void*;
void* GetModuleHandleW(wchar_t const*){return nullptr;}
namespace h=gears_weapon_hand;namespace vr=gears_vr;namespace w=gears_weapon;
namespace test {
std::int64_t time=bow_arrow_tracking_fixture::now,call_cost{};bool valid=true,alive=true,held=true,geometry=true,bound=true,run_ok=true;
unsigned calls{},fn_generation{1};std::function<void()> reentry;gears_rig_tracking::Frame rig{};
alignas(16) std::array<unsigned char,512> function{},vector_type{};
alignas(16) std::array<std::array<unsigned char,512>,7> properties{};
std::map<std::wstring,uptr> labels;std::map<uptr,std::wstring> names;
uptr fn(){return reinterpret_cast<uptr>(function.data());}
template<class T>void put(uptr p,unsigned n,T const& v){std::memcpy(reinterpret_cast<void*>(p+n),&v,sizeof(v));}
void setup(){
 function={};properties={};vector_type={};labels.clear();names.clear();
 const wchar_t* text[]={L"LineStart",L"LineEnd",L"R",L"G",L"B",L"bPersistentLines",L"Unexpected"};
 const unsigned offset[]={0,12,24,25,26,28,32};const unsigned width[]={12,12,1,1,1,4,4};
 for(unsigned i=0;i<7;++i){const auto p=reinterpret_cast<uptr>(properties[i].data());labels[text[i]]=p;names[p]=text[i];
  put(p,0x40,fn());put(p,0x68,i<5?reinterpret_cast<uptr>(properties[i+1].data()):uptr{});
  put(p,0x70,1u);put(p,0x74,width[i]);put(p,0x78,0x80ull);put(p,0x94,offset[i]);
  if(i<2)put(p,0xd0,reinterpret_cast<uptr>(vector_type.data()));
 }
 put(fn(),0x80,reinterpret_cast<uptr>(properties[0].data()));put(fn(),0xf4,0x2400u);put(fn(),0x108,std::uint16_t(0xffff));
 put(fn(),0x114,uptr{0x10000});put(reinterpret_cast<uptr>(vector_type.data()),0x88,12u);
}
}
template<class T>T get(uptr p){T v{};if(p)std::memcpy(&v,reinterpret_cast<void const*>(p),sizeof(v));return v;}
bool registered(uptr p){return p!=0;}
namespace playground {
struct Vector {float x{},y{},z{};};
struct Args {
 struct Record {uptr function{};unsigned size{32};std::array<unsigned char,64> params{};} r;
 bool valid{true};Args(uptr,wchar_t const*){r.function=test::fn();}
 uptr prop(wchar_t const* name){auto it=test::labels.find(name);return it==test::labels.end()?0:it->second;}
 template<class T>Args& set(wchar_t const* name,T v){auto p=prop(name);if(!p){valid=false;return *this;}
  const auto off=get<unsigned>(p+0x94);if(off+sizeof(v)>r.params.size()){valid=false;return *this;}
  std::memcpy(r.params.data()+off,&v,sizeof(v));return *this;}
 bool run(){
  CHECK(get<unsigned char>(reinterpret_cast<uptr>(r.params.data())+24)==255);
  CHECK(get<unsigned char>(reinterpret_cast<uptr>(r.params.data())+25)==220);
  CHECK(get<unsigned char>(reinterpret_cast<uptr>(r.params.data())+26)==0);
  CHECK(get<unsigned>(reinterpret_cast<uptr>(r.params.data())+28)==0);
  ++test::calls;test::time+=test::call_cost;
  if(test::reentry){auto work=std::move(test::reentry);test::reentry={};work();}return test::run_ok;
 }
};
bool named(uptr p,wchar_t const* label){return p==reinterpret_cast<uptr>(test::vector_type.data())&&std::wcscmp(label,L"Vector")==0;}
}
namespace weaponhand {
std::int64_t clock_ns(void*)noexcept{return test::time;}bool current_gt(void*)noexcept{return test::valid;}
bool executable(FARPROC,void*)noexcept{return true;}
struct Client {bool registration_alive(std::uint64_t,std::uint64_t)noexcept{return test::alive;}}client;
}
namespace nativeequipment {
w::ArmspanCalibration calibration=w::default_body_slot_layout();
bool domain_ready()noexcept{return test::valid;}bool current(void*,h::Frame const&)noexcept{return test::valid;}
bool identify(void*,uptr p,h::ObjectIdentity& out)noexcept{out=bow_arrow_tracking_fixture::object(p,test::fn_generation);return test::valid;}
}
namespace vrequip {struct Model {bool held_for(std::uint64_t,std::uint64_t,std::uint64_t)const noexcept{return test::held;}}model;}
namespace nativelancerfollow {
w::WeaponMeshFollow mount;
bool bool_arg(playground::Args& call,wchar_t const* name,bool value){return call.set(name,unsigned(value)).valid;}
}
namespace nativeequipment_rig {
enum class Status {unavailable,ready};struct Result {Status status;};
Result acquire(h::Frame const&,gears_rig_tracking::Frame& out){out=test::rig;return {Status::ready};}
}
namespace torquebow_original {
// Existing reflection/Native/Engine helpers are injected. New production
// guide caller, parameter enumeration, RawRig mapping and selector are real.
bool original_function(playground::Args const& call,wchar_t const*,wchar_t const*){return call.valid;}
bool input_property(playground::Args& call,wchar_t const* name,wchar_t const*,unsigned bytes,unsigned offset){
 auto p=call.prop(name);return p&&get<uptr>(p+0x40)==call.r.function&&get<unsigned>(p+0x70)==1&&
  get<unsigned>(p+0x74)==bytes&&get<unsigned>(p+0x94)==offset&&(get<unsigned long long>(p+0x78)&0x580)==0x80;
}
}
namespace torquebow_guide_source {struct Progress {double count{.5},maximum{1};};bool progress(h::Frame const&,Progress& out){out={};return true;}}
namespace torquebow_input {
struct Snapshot {struct Original {bool charging{};}original;unsigned loaded{},reserve{1};};
struct Backend {
 Snapshot last{};h::Frame last_source{};bool last_known{true};double geometry_bow_scale{.3};
 std::int64_t now()noexcept{return test::time;}std::uint64_t hold(h::Frame const&)noexcept{return 1;}
 bool geometry_current(h::Frame const&)noexcept{return test::geometry;}
}backend;
struct Driver {
 bool bound()const noexcept{return test::bound;}bool matches_source(h::Frame const&,std::uint64_t)const noexcept{return true;}
 w::TorqueBowInventory inventory()const noexcept{return {1,backend.last.loaded,backend.last.reserve};}
 Driver const& interaction()const noexcept{return *this;}w::TorqueBowStage stage()const noexcept{return w::TorqueBowStage::ready;}
 std::uint64_t instance_config_serial()const noexcept{return 1;}std::uint64_t held_arrow_serial()const noexcept{return 0;}
 std::uint64_t last_loaded_token()const noexcept{return 0;}std::uint64_t last_confirmed_cancel_edge()const noexcept{return 0;}
 std::uint64_t last_original_projectile_serial()const noexcept{return 0;}
}driver;
}
FILE* log_file{};
#include "weapon_torque_bow_guide.inl"
struct F:bow_arrow_tracking_fixture::Fixture {
 h::GripGameState game{};
 F(){test::setup();test::valid=test::alive=test::held=test::geometry=test::bound=test::run_ok=true;test::calls=0;test::call_cost=0;
  test::time=bow_arrow_tracking_fixture::now;test::fn_generation=1;test::rig=rig;game.known=game.main_hand_held=true;
  torquebow_guide::selector={};torquebow_guide::drawn_known=false;torquebow_input::backend.last={};torquebow_input::backend.last_known=true;
  torquebow_input::backend.last_source=hand;
  const auto& right=rig.world_grip[1];const std::array<float,16> matrix{.3f,0,0,0,0,.3f,0,0,0,0,.3f,0,12,13,14,1};
  nativelancerfollow::mount={};CHECK(nativelancerfollow::mount.bind({right.position,vr::ue_angles(vr::rotation_matrix(right.orientation))},matrix));
 }
 void run(bool observed=true){torquebow_guide::tick(hand,game,observed);}
};
int main(){
 F valid;test::reentry=[&]{valid.run();};valid.run();CHECK(test::calls==14);valid.run();CHECK(test::calls==14);
 F budget;test::call_cost=4000000;budget.run();CHECK(test::calls==2);budget.run();CHECK(test::calls==2);
 F unknown;test::run_ok=false;unknown.run();CHECK(test::calls==1);test::run_ok=true;unknown.run();CHECK(test::calls==1);
 F changed;test::reentry=[](){++test::fn_generation;};changed.run();CHECK(test::calls==1);
 for(auto flag:{&test::valid,&test::alive,&test::held,&test::geometry,&test::bound}){F blocked;*flag=false;blocked.run();CHECK(test::calls==0);}
 F busy;busy.game.busy=true;busy.run();CHECK(!test::calls);
 F old;old.run(false);CHECK(!test::calls);
 F wrong;torquebow_input::backend.last_source.key.frame++;wrong.run();CHECK(!test::calls);
 F extra;const auto p=reinterpret_cast<uptr>(test::properties[5].data());test::put(p,0x68,reinterpret_cast<uptr>(test::properties[6].data()));extra.run();CHECK(!test::calls);
 F overlap;test::put(test::labels[L"R"],0x94,12u);overlap.run();CHECK(!test::calls);
 F cycle;test::put(test::labels[L"bPersistentLines"],0x68,test::labels[L"LineStart"]);cycle.run();CHECK(!test::calls);
 F result;test::put(test::fn(),0x108,std::uint16_t(0));result.run();CHECK(!test::calls);
 F stale;test::time+=100000001;stale.run();CHECK(!test::calls);
 F raw;test::rig.scene_invocation++;raw.run();CHECK(!test::calls);
 std::printf("Torque Bow guide caller: %u checks passed; real caller/selector/RawRig, injected reflection/Native/original lines, no pixels certified.\n",bow_arrow_tracking_fixture::checks);
}
