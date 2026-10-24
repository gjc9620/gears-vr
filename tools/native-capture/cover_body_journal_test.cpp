// Repeatable synthetic cleanup regression tests. No game process, snapshots,
// files, hook installation or engine calls are used. All object/registry names
// and addresses below belong to the private VirtualAlloc fixture.
//
// Compile the tested module even when the product CMake option is OFF. World
// admission, entry attestation and selected reflection proofs are fixture inputs;
// these tests verify journal/guard behavior, not retail metadata discovery.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#ifdef GEARS_COVER_BODY_PROTOTYPE
#undef GEARS_COVER_BODY_PROTOTYPE
#endif
#define GEARS_COVER_BODY_PROTOTYPE 1
#include <windows.h>
#include <array>
#include <atomic>
#include <mutex>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <cmath>
#include "anchor_observer.h"
#include "MinHook.h"
uintptr_t base=0x140000000;
std::atomic<uint64_t> committed_world_generation{1};
std::atomic<bool> recording{true};
bool continuous_session=true,anchor_observer_enabled=true;
gears_anchor::Config anchor_config;
struct Scope {bool world()const{return true;}};
uintptr_t denied{},changing_text{};unsigned text_reads{};
bool safe_copy(void* out,void const* from,size_t n){
 auto a=reinterpret_cast<uintptr_t>(from);if(a==denied)return false;
 if(a==changing_text&&++text_reads==2)*reinterpret_cast<wchar_t*>(changing_text)=L'X';
 __try {memcpy(out,from,n);return true;} __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
long long now(){return static_cast<long long>(GetTickCount64());}
// Stub only: the test never reserves or installs a hook.
extern "C" MH_STATUS WINAPI MH_CreateHook(void*,void*,void**){return MH_ERROR_UNSUPPORTED_FUNCTION;}
#include "cover_body_live.inl"
namespace cover_body_live {bool own_entries(){return true;}}
extern "C" void GearsCoverBlendTickHook(){}
extern "C" void GearsCoverChildPropagateHook(){}
namespace b=cover_body_live;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
uintptr_t arena{},node{},pawn{},mesh{},fixture_world{},records{},node_text{};
template<class T>void put(uintptr_t a,T const& v){memcpy(reinterpret_cast<void*>(a),&v,sizeof v);}
void setup(){
 memset(reinterpret_cast<void*>(arena),0,1024*1024);b::metadata={};auto& c=b::metadata;
 c.objects_header=arena+0x100;c.names_header=arena+0x200;c.identity_count=5;c.object_internal_index=8;c.object_name=72;c.object_class=80;c.object_outer=64;c.name_entry_index=0;c.name_entry_text=20;c.field_super=96;
 c.role_controller=0;c.role_local=1;c.role_camera=2;c.role_pawn=3;c.role_level=4;c.controller_pawn=0x100;c.pawn_controller=0x100;c.player_camera=0x108;c.pc_owner=0x108;c.player=0x110;c.camera_view_target=0x100;
 gears_anchor::Registry objects{arena+0x1000,2001,2100},names{arena+0x6000,2001,2100};put(c.objects_header,objects);put(c.names_header,names);
 for(unsigned i=0;i<9;++i){gears_anchor::Identity id{};id.address=arena+0x30000+uintptr_t(i)*0x400;id.index=id.name_index=i;id.cls=arena+0x48000+uintptr_t(i)*0x100;id.outer=i==3?arena+0x31000:0;
  swprintf_s(id.name.data(),id.name.size(),L"Synthetic%u",i);put(id.address+8,id.index);put(id.address+72,id.name_index);put(id.address+76,id.number);put(id.address+80,id.cls);put(id.address+64,id.outer);
  put(objects.data+uintptr_t(i)*8,id.address);auto e=arena+0x20000+uintptr_t(i)*0x200;put(names.data+uintptr_t(i)*8,e);put(e,id.name_index);put(e+20,id.name);
  if(i<5)c.identities[i]=id;if(i==5){node=id.address;node_text=e+20;}if(i==6)mesh=id.address;if(i==7)fixture_world=id.address;
 }
 auto pc=c.identities[0].address,local=c.identities[1].address,camera=c.identities[2].address; pawn=c.identities[3].address;
 put(pc+c.controller_pawn,pawn);put(pc+c.player_camera,camera);put(pc+c.player,local);put(pawn+c.pawn_controller,pc);put(camera+c.camera_view_target,pawn);put(camera+c.pc_owner,pc);
 b::layout={};b::layout.children=0xe4;b::layout.anim=8;b::layout.weight=0x10;b::layout.total=0x14;b::layout.skel=0x60;b::layout.owner=0x128;b::layout.mesh=0x108;b::layout.world=0x110;b::layout.cover=0x118;
 put(node,base+0x13d0080);put(node+0x60,mesh);put(node+0x128,pawn);put(pawn+0x108,mesh);put(pawn+0x110,fixture_world);put(pawn+0x118,static_cast<unsigned char>(2));
 records=arena+0x50000;b::Children children{records,5,5};put(node+0xe4,children);
 b::journals={};auto& j=b::journals[0];j.used=true;C(b::identity(node,j.id,true));j.pawn=pawn;j.mesh=mesh;j.world=fixture_world;j.generation=1;j.children=children;j.installed_step=2;
 for(unsigned i=0;i<5;++i){float native=i==0?1.f:0.f,installed=i==0||i==4?.5f:0.f;put(records+uintptr_t(i)*0x44+0x10,installed);put(records+uintptr_t(i)*0x44+0x14,123.25f);memcpy(&j.native[i],&native,4);memcpy(&j.installed[i],&installed,4);j.owned[i]=i==0||i==4;}
 b::stopping=false;b::failed=false;b::external_change=false;b::drain_tick=0;b::writes=2;b::restores=0;b::retired=0;b::external=0;b::row_count=0;b::step_rows={};b::recount();denied=changing_text=0;text_reads=0;committed_world_generation=1;
}
float weight(unsigned i){float v{};memcpy(&v,reinterpret_cast<void*>(records+uintptr_t(i)*0x44+0x10),4);return v;}
int main(){
 arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,1024*1024,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
 // Owned fields restore; native TotalWeight stays untouched.
 setup();C(b::pending_count()==2);b::restore_all();C(!b::pending_count()&&b::restores==2);C(weight(0)==1&&weight(4)==0);float total{};memcpy(&total,reinterpret_cast<void*>(records+0x14),4);C(total==123.25f);
 // An external Weight change releases that field without overwriting it.
 setup();put(records+0x10,.33f);b::restore_all();C(!b::pending_count()&&b::restores==1&&b::external==1&&b::external_change.load());C(weight(0)==.33f&&weight(4)==0);
 // Positive identity change may retire; an unknown read must retain pending.
 setup();put(node+76,1u);b::restore_all();C(!b::pending_count()&&b::restores==0&&b::retired==2);C(weight(0)==.5f&&weight(4)==.5f);
 setup();denied=node+8;b::restore_all();C(b::pending_count()==2&&b::restores==0&&b::stopping.load());denied=0;b::restore_all();C(!b::pending_count()&&b::restores==2);
 // Still-live old objects keep their obligation when context/storage changes.
 setup();put(node+0x60,mesh+0x400);b::restore_all();C(b::pending_count()==2&&b::retired==0&&b::restores==0&&b::stopping.load());C(weight(0)==.5f&&weight(4)==.5f);put(node+0x60,mesh);b::restore_all();C(!b::pending_count()&&b::restores==2);
 setup();b::Children replacement{records+0x1000,5,5};put(node+0xe4,replacement);b::restore_all();C(b::pending_count()==2&&b::retired==0&&b::restores==0&&b::stopping.load());C(weight(0)==.5f&&weight(4)==.5f);put(node+0xe4,b::journals[0].children);b::restore_all();C(!b::pending_count()&&b::restores==2);
 // Even after a world change, only later proven slot loss permits retirement.
 setup();committed_world_generation=2;b::restore_all();C(b::pending_count()==2&&b::retired==0&&b::restores==0&&b::stopping.load());C(weight(0)==.5f&&weight(4)==.5f);
 b::enabled=true;b::request_drain();b::drain_tick=GetTickCount64()-5001;C(b::drain_failed()&&b::pending_count()==2);
 gears_anchor::Registry registry{};memcpy(&registry,reinterpret_cast<void*>(b::metadata.objects_header),sizeof registry);put(registry.data+uintptr_t(b::journals[0].id.index)*8,uintptr_t(0));b::restore_all();C(!b::pending_count()&&b::retired==2&&b::restores==0);
 setup();put(b::metadata.identities[b::metadata.role_controller].address+b::metadata.controller_pawn,arena+0x32000);b::restore_all();C(b::pending_count()==2&&b::retired==0&&b::restores==0&&b::stopping.load());C(weight(0)==.5f&&weight(4)==.5f);
 gears_anchor::Identity live_old{};C(b::identity(node,live_old)&&b::same(live_old,b::journals[0].id));put(b::metadata.identities[b::metadata.role_controller].address+b::metadata.controller_pawn,pawn);b::restore_all();C(!b::pending_count()&&b::restores==2);
 setup();put(node+0x128,pawn+0x400);b::restore_all();C(b::pending_count()==2&&b::retired==0&&b::restores==0&&b::stopping.load());C(weight(0)==.5f&&weight(4)==.5f);
 setup();put(pawn+0x110,fixture_world+0x400);b::restore_all();C(b::pending_count()==2&&b::retired==0&&b::restores==0&&b::stopping.load());C(weight(0)==.5f&&weight(4)==.5f);
 setup();put(node,base+0x13d0088);b::restore_all();C(b::pending_count()==2&&b::retired==0&&b::restores==0&&b::stopping.load());C(weight(0)==.5f&&weight(4)==.5f);
 // No ownership grants no restore authority; CAS preserves mismatching values.
 setup();auto& j=b::journals[0];j.owned.fill(false);b::recount();b::restore_all();C(!b::pending_count()&&b::restores==0&&weight(0)==.5f);
 setup();uint32_t expected{},observed{};float half=.5f,quarter=.25f;memcpy(&expected,&half,4);uint32_t desired{};memcpy(&desired,&quarter,4);C(b::exchange(records+0x10,expected,desired,observed)&&observed==expected&&weight(0)==quarter);C(b::exchange(records+0x10,expected,0,observed)&&observed==desired&&weight(0)==quarter);C(!b::exchange(records+0x11,expected,0,observed));
 // FName padding is irrelevant; changed text and overlong selected names reject.
 setup();put(node_text+sizeof(L"Synthetic5"),uint64_t(0xffffffffffffffff));gears_anchor::Identity id{};C(b::identity(node,id,true));changing_text=node_text;text_reads=0;C(!b::identity(node,id,true));changing_text=0;
 setup();for(unsigned i=0;i<64;++i)put(node_text+uintptr_t(i)*2,wchar_t(L'A'));put(node_text+128,wchar_t(0));C(!b::identity(node,id));C(b::identity(node,id,false,true));
 // Drain is bounded and never clears pending by timing out.
 setup();b::enabled=true;b::request_drain();C(b::stopping.load()&&b::draining());b::drain_tick=GetTickCount64()-5001;C(b::drain_failed()&&b::pending_count()==2);b::restore_all();C(!b::drain_failed()&&!b::pending_count());

 // Exercise real child_before guard/write logic with synthetic selected layout.
 setup();b::restore_all();b::enabled=true;b::game_thread=GetCurrentThreadId();b::layout.special=0x119;put(pawn+0x119,static_cast<unsigned char>(0));
 b::tick={node,pawn,mesh,fixture_world,b::journals[0].id,b::journals[0].children,1,false};b::injecting=true;b::last_advance=GetTickCount64();b::step=2;
 put(records+8,arena+0x3c000);put(records+uintptr_t(4)*0x44+8,arena+0x3c400);b::writes=0;
 b::child_before(node,0,0.f);C(!b::pending_count()&&weight(0)==1&&weight(4)==0&&b::writes==0);
 put(pawn+0x119,static_cast<unsigned char>(12));b::child_before(node,0,1.f);C(!b::pending_count()&&b::writes==0);put(pawn+0x119,static_cast<unsigned char>(0));
 b::layout.sources[0].address=arena+0x80000;b::layout.sources[0].size=4;uint32_t source_value=100;put(arena+0x80000,source_value);memcpy(b::layout.sources[0].bytes.data(),&source_value,4);b::layout.source_count=1;put(arena+0x80000,uint32_t(101));
 b::child_before(node,0,1.f);C(!b::pending_count()&&b::writes==0);b::layout.source_count=0;
 b::child_before(node,0,1.f);C(b::pending_count()==2&&weight(0)==.5f&&weight(4)==.5f&&b::writes==2);
 put(records+0x14,.5f);b::child_before(node,4,1.f);put(records+uintptr_t(4)*0x44+0x14,.5f);b::blend_before(node);C(!b::pending_count()&&weight(0)==1&&weight(4)==0);
 setup();b::restore_all();b::enabled=true;b::game_thread=GetCurrentThreadId();b::layout.special=0x119;put(pawn+0x119,static_cast<unsigned char>(0));
 // Make only the second changed field unwritable: the first CAS must roll back.
 auto split_records=arena+0x51fe8;memcpy(reinterpret_cast<void*>(split_records),reinterpret_cast<void*>(records),5*0x44);records=split_records;b::Children split_header{records,5,5};put(node+0xe4,split_header);b::journals[0].children=split_header;
 put(records+8,arena+0x3c000);put(records+uintptr_t(4)*0x44+8,arena+0x3c400);b::tick={node,pawn,mesh,fixture_world,b::journals[0].id,split_header,1,false};b::injecting=true;b::last_advance=GetTickCount64();b::step=2;b::writes=0;b::restores=0;
 DWORD old_protection{};C(VirtualProtect(reinterpret_cast<void*>(arena+0x52000),4096,PAGE_READONLY,&old_protection)!=0);
 b::child_before(node,0,1.f);C(b::writes==1&&b::restores==1&&!b::pending_count()&&b::failed.load()&&weight(0)==1&&weight(4)==0);
 DWORD ignored_protection{};C(VirtualProtect(reinterpret_cast<void*>(arena+0x52000),4096,old_protection,&ignored_protection)!=0);
 C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);printf("cover_body_live_journal: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
