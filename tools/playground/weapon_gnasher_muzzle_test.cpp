#include "../vr-common/weapon_gnasher_fire.h"
#include "../vr-common/weapon_shot_scope.h"
#include "../vr-common/weapon_call_boundary.h"
#include <windows.h>
#include <array>
#include <atomic>
#include <cstring>
#include <cwchar>
#include <cstdio>
#include <cstdlib>
#ifdef NDEBUG
#error GnasherMuzzleTest requires active checks
#endif
#define CHECK(v) do{if(!(v)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#v);std::abort();}}while(false)
using uptr=std::uintptr_t;namespace hand=gears_weapon_hand;namespace shot=gears_weapon_shot;
constexpr uptr gun=0x5000,custom_fn=0x1010,pellet_fn=0x1020,aim_fn=0x3000,trace_fn=0x3010,muzzle_fn=0x3020;
bool live=true,metadata_ok=true,owned=true,refire_bound=true,write_ok=true;unsigned metadata_reads{},writes{};
FILE* log_file=nullptr;std::atomic<bool> enabled{true};
constexpr std::int64_t fixture_ns=1000000100;
hand::ObjectIdentity object(uptr p,uptr outer=0x8000,uptr cls=0x9000){return {p,outer,cls,unsigned(p/8),7,0,1};}
struct alignas(8) VMFrame {std::array<unsigned char,64> bytes{};};std::array<VMFrame,8> vm{};
uptr address(unsigned i){return reinterpret_cast<uptr>(vm[i].bytes.data());}
void body(unsigned i,shot::Binding const& b,uptr previous){auto p=vm[i].bytes.data();auto fn=b.identities[0].address;auto pc=b.script+16;
 std::memcpy(p+0x18,&fn,8);std::memcpy(p+0x20,&gun,8);std::memcpy(p+0x28,&pc,8);std::memcpy(p+0x38,&previous,8);}
namespace weaponhand {unsigned current_gt(void*){return GetCurrentThreadId();}std::int64_t clock_ns(void*){return fixture_ns;}
struct Client {bool registration_alive(std::uint64_t,std::uint64_t){return live;}} client;}
namespace weaponshot {
shot::Stack stack;
bool read(void* out,void const* p,std::size_t n){auto a=reinterpret_cast<uptr>(p);
 for(auto const& v:vm){auto first=reinterpret_cast<uptr>(v.bytes.data());if(a>=first&&a-first<=64&&n<=64-(a-first)){std::memcpy(out,p,n);return true;}}return false;}
}
namespace playground {
struct Vector {float x{},y{},z{};};struct Rotation {int pitch{},yaw{},roll{};};
bool instance(uptr p,wchar_t const* label){return p==gun&&!std::wcscmp(label,L"Weap_Shotgun");}
bool named(uptr p,wchar_t const* label){return (p==aim_fn&&!std::wcscmp(label,L"GetAdjustedAimFor"))||
 (p==trace_fn&&!std::wcscmp(label,L"GetWeaponStartTraceLocation"))||(p==muzzle_fn&&!std::wcscmp(label,L"GetMuzzleLoc"));}
struct Args {struct Record {uptr function{};} r;bool valid=true;
 Args(uptr,wchar_t const* label){r.function=!std::wcscmp(label,L"GetAdjustedAimFor")?aim_fn:!std::wcscmp(label,L"GetWeaponStartTraceLocation")?trace_fn:muzzle_fn;}
 bool run()=delete;
};
}
namespace lancerreload_inventory {struct Identity {hand::ObjectIdentity weapon{};bool operator==(Identity const& b)const{return hand::same(weapon,b.weapon);}};Identity identity(hand::Frame const& f){return {f.weapon};}}
namespace gnasher_fire {
struct Entry {unsigned preparation_stage=14,preparation_flags{},preparation_rejection{};bool prepared=true;gears_weapon::GnasherFireCycle cycle;lancerreload_inventory::Identity identity{object(gun)};hand::Frame source{};};Entry state;
Entry* entry(hand::Frame const& f,bool){return hand::same(f.weapon,state.identity.weapon)?&state:nullptr;}
unsigned abandoned_count{};
void abandoned(gears_weapon_call::Invocation const&,shot::Token const&){++abandoned_count;state.cycle.unresolved_return(100);}
}
namespace lancerreload_manual {bool bound_for(hand::Frame const&){return owned;}}
namespace gnasher_refire {
unsigned prepare_stage{};
bool prepare(hand::Frame const&){return refire_bound;}bool bound_for(hand::Frame const&){return refire_bound;}
bool returned(gears_weapon_call::Invocation const&){return true;}
}
namespace lancermain_asset {enum class AssetScope {gnasher_custom};}
namespace unified_weapon_fire {
using U32=std::uint32_t;constexpr unsigned scope_capacity=12,pending_capacity=16;
struct MuzzleSample {hand::Frame source{};std::uint64_t epoch=1;std::int64_t sampled_ns=fixture_ns,render_acquisition_ns{};playground::Vector origin{1,2,3};playground::Rotation aim{11,22,33};bool valid{};};
std::atomic<bool> control_running{true},write_faulted{};std::atomic<std::uint64_t> control_epoch{1};
bool held_current(MuzzleSample const& s){return owned&&s.valid;}
struct SourceScope {shot::Token scope{};shot::Binding binding{};gears_weapon_call::Invocation call{};MuzzleSample sample{};bool valid{};};
enum class ResultKind {none,base_aim,trace_origin,pushed_origin,physical_origin};
struct ReturnSignature {ResultKind kind{};unsigned fingerprint{};};
struct Pending {bool live{},uncertain{};unsigned depth{};std::uint64_t scope_call{},epoch{};gears_weapon_call::Invocation call{};shot::Packet packet{};ReturnSignature signature{};std::array<U32,3> desired{};};
bool manual_profile(){return true;}
bool live_sample(MuzzleSample const& s,std::int64_t stamp){return live&&s.valid&&stamp<=fixture_ns+100000000;}
bool typed_result(gears_weapon_call::Invocation const& c,MuzzleSample const&,ReturnSignature& out,bool){++metadata_reads;
 out={c.callee==aim_fn?ResultKind::base_aim:c.callee==trace_fn?ResultKind::trace_origin:c.callee==muzzle_fn?ResultKind::physical_origin:ResultKind::none,1};return metadata_ok&&out.kind!=ResultKind::none;}
bool prepare_source(hand::Frame const& frame,hand::GripGameState const&,lancermain_asset::AssetScope,MuzzleSample& out,bool(*preflight)(MuzzleSample const&)noexcept){MuzzleSample s{};s.source=frame;s.valid=true;if(!preflight(s))return false;out=s;return true;}
bool source_matches_shot(MuzzleSample const& s,shot::Packet const& p){return hand::same(s.source.key,p.grip.key)&&hand::same(s.source.weapon,p.weapon)&&s.source.controllers.profile_generation==p.grip.profile_generation;}
bool packet_for(gears_weapon_call::Invocation const& c,SourceScope const& s,shot::Packet& out){return gears_weapon_call::invocation_valid(c)&&c.hand_cookie==s.sample.source.key.registration_cookie&&c.native_started_qpc==s.sample.source.key.native_started_qpc&&c.thread_id==GetCurrentThreadId()&&weaponshot::stack.packet(s.sample.source.weapon,c.caller_frame,weaponshot::read,out)&&source_matches_shot(s.sample,out);}
bool same_signature(ReturnSignature a,ReturnSignature b){return a.kind==b.kind&&a.fingerprint==b.fingerprint;}
bool write_context(Pending const& p,SourceScope const& s){return p.depth==weaponshot::stack.depth()&&p.scope_call==s.scope.call&&p.epoch==s.sample.epoch&&weaponshot::stack.current(p.packet);}
bool store_result(Pending& p,SourceScope const&){if(!write_ok){p.uncertain=true;return false;}std::memcpy(reinterpret_cast<void*>(p.call.result),p.desired.data(),12);++writes;return true;}
}
#include "weapon_gnasher_muzzle.inl"
shot::Binding binding(shot::Role role,uptr fn){shot::Binding b{};b.role=role;b.script=0x10000;b.bytes=b.capacity=128;b.script_hash=1;b.exec=0x20000;
 b.identities={object(fn,0x2000,0x9000),object(0x2000,0x3000,0xa000),object(0x3000,0,0xb000),object(0x9000,0xc000,0xa000),object(0xa000,0xc000,0xa000)};CHECK(b.valid());return b;}
hand::Frame frame(){hand::Frame f{};f.key={11,22,33,1,1,1,44,55};f.world=object(0x6000);f.controller=object(0x6010);f.pawn=object(0x6020);f.mesh=object(0x6030);f.weapon=object(gun);f.controllers.profile_generation=7;return f;}
hand::GripSnapshot grip(hand::Frame const& f){hand::GripSnapshot g{};g.grip=hand::Grip::single;g.key=f.key;g.world=f.world;g.controller=f.controller;g.pawn=f.pawn;g.mesh=f.mesh;g.weapon=f.weapon;g.profile_generation=7;g.committed_ns=1000000000;g.input_received_ns=g.stereo_received_ns=990000000;return g;}
gears_weapon_call::Invocation invocation(uptr fn,uptr caller,uptr receiver=gun){gears_weapon_call::Invocation c{};c.hand_cookie=33;c.native_started_qpc=22;c.call_id=fn;c.thread_id=GetCurrentThreadId();c.self=receiver;c.callee=fn;c.caller_frame=caller;return c;}
int main(){
 auto f=frame();hand::GripGameState game{};gears_weapon::GnasherIdentity id{1,2,3,4,5,6,7,8};CHECK(gnasher_fire::state.cycle.bind({id,1,8,10}));
 gnasher_fire::state.source=f;CHECK(gnasher_muzzle::defer_new_press(f));
 auto later=f;++later.key.frame;CHECK(!gnasher_muzzle::defer_new_press(later));
 gnasher_fire::state.prepared=false;CHECK(!gnasher_muzzle::defer_new_press(f));gnasher_fire::state.prepared=true;
 gnasher_fire::state.prepared=false;CHECK(!gnasher_muzzle::prepare(f,game));CHECK(gnasher_muzzle::prepare_gate==2);gnasher_fire::state.prepared=true;
 owned=false;CHECK(!gnasher_muzzle::prepare(f,game));CHECK(gnasher_muzzle::prepare_gate==6);owned=true;
 refire_bound=false;CHECK(!gnasher_muzzle::prepare(f,game));CHECK(gnasher_muzzle::prepare_gate==7);refire_bound=true;
 metadata_ok=false;CHECK(!gnasher_muzzle::prepare(f,game));CHECK(gnasher_muzzle::prepare_gate==8);metadata_ok=true;
 CHECK(gnasher_muzzle::prepare(f,game));CHECK(gnasher_muzzle::prepare_gate==0);CHECK(gnasher_muzzle::ready_for(f,20000000));
 const auto custom=binding(shot::Role::shotgun_custom,custom_fn),pellets=binding(shot::Role::shotgun_pellets,pellet_fn);
 auto root=weaponshot::stack.enter(custom,f.weapon,address(0),grip(f),fixture_ns,weaponshot::read);body(1,custom,address(0));
 auto root_call=invocation(custom_fn,address(0));gnasher_muzzle::begin(root_call,root);CHECK(gnasher_muzzle::active());
 alignas(4) std::array<int,3> result{91,92,93};auto aim=invocation(aim_fn,address(1),f.pawn.address);aim.result=reinterpret_cast<uptr>(result.data());
 gnasher_muzzle::begin(aim,{});CHECK(gnasher_muzzle::returned(aim));CHECK((result==std::array<int,3>{11,22,33}));CHECK(writes==1);
 auto alias=weaponshot::stack.enter(pellets,f.weapon,address(1),{},fixture_ns,weaponshot::read);body(2,pellets,address(1));auto pellet_call=invocation(pellet_fn,address(1));
 gnasher_muzzle::sample.origin={9,9,9};gnasher_muzzle::begin(pellet_call,alias);CHECK(gnasher_muzzle::active());
 CHECK(gnasher_muzzle::active()->admission.shot==gnasher_muzzle::scopes[0].admission.shot);
 alignas(4) std::array<float,3> origin{91,92,93};auto muzzle=invocation(muzzle_fn,address(2));muzzle.result=reinterpret_cast<uptr>(origin.data());
 gnasher_muzzle::begin(muzzle,{});CHECK(gnasher_muzzle::returned(muzzle));CHECK((origin==std::array<float,3>{1,2,3}));CHECK(writes==2);
 const auto reads=metadata_reads;
 for(uptr excluded:{uptr(0x3030),uptr(0x3040),uptr(0x3050)}){auto collision=invocation(excluded,address(2));collision.result=reinterpret_cast<uptr>(origin.data());
  origin={91,92,93};gnasher_muzzle::begin(collision,{});CHECK(gnasher_muzzle::returned(collision));CHECK((origin==std::array<float,3>{91,92,93}));}
 CHECK(metadata_reads==reads&&writes==2); // Pushed/pulled/CalcWeaponFire never enter a result writer.
 auto masked=weaponshot::stack.mask();auto nested=invocation(0x4000,address(2));gnasher_muzzle::begin(nested,masked);
 CHECK(!gnasher_muzzle::active());gnasher_muzzle::begin(muzzle,{});CHECK(gnasher_muzzle::returned(muzzle));CHECK(writes==2);
 CHECK(gnasher_muzzle::cleanup(nested,masked));weaponshot::stack.leave(masked);
 origin={91,92,93};gnasher_muzzle::begin(muzzle,{});live=false;CHECK(gnasher_muzzle::returned(muzzle));CHECK(writes==2);live=true;
 CHECK(gnasher_muzzle::cleanup(pellet_call,alias));weaponshot::stack.leave(alias);
 CHECK(gnasher_muzzle::returned(root_call));CHECK(gnasher_muzzle::cleanup(root_call,root));weaponshot::stack.leave(root);CHECK(!gnasher_fire::abandoned_count);CHECK(!gnasher_muzzle::sample.valid);
 CHECK(!gnasher_muzzle::ready_for(f,0));CHECK(weaponshot::stack.drained());
 // Other firing roots and orphan pellet entries cannot borrow this source.
 CHECK(gnasher_muzzle::prepare(f,game));auto war=binding(shot::Role::war_instant,0x1040);
 auto other=weaponshot::stack.enter(war,f.weapon,address(0),grip(f),fixture_ns,weaponshot::read);auto other_call=invocation(0x1040,address(0));gnasher_muzzle::begin(other_call,other);CHECK(!gnasher_muzzle::active());
 CHECK(gnasher_muzzle::cleanup(other_call,other));weaponshot::stack.leave(other);
 auto orphan=weaponshot::stack.enter(pellets,f.weapon,address(0),grip(f),fixture_ns,weaponshot::read);gnasher_muzzle::begin(pellet_call,orphan);CHECK(!gnasher_muzzle::active());
 CHECK(gnasher_muzzle::cleanup(pellet_call,orphan));weaponshot::stack.leave(orphan);
 CHECK(gnasher_muzzle::prepare(f,game));
 auto interrupted=weaponshot::stack.enter(custom,f.weapon,address(0),grip(f),fixture_ns,weaponshot::read);
 gnasher_muzzle::begin(root_call,interrupted);CHECK(gnasher_muzzle::active());
 CHECK(gnasher_muzzle::cleanup(root_call,interrupted));weaponshot::stack.leave(interrupted);
 CHECK(gnasher_fire::abandoned_count==1&&gnasher_fire::state.cycle.faulted()&&gnasher_fire::state.cycle.blocked());
 CHECK(!gnasher_muzzle::defer_new_press(f));
 CHECK(!gnasher_muzzle::prepare(f,game));CHECK(!gnasher_muzzle::ready_for(f,0));
 std::puts("Production Gnasher muzzle: real CustomFire/pellet Stack/Token/ancestry, same-shot inheritance, mask/foreign/orphan rejection, collision-result exclusion and cycle debt gate passed (injected metadata/writer boundaries)");
}
