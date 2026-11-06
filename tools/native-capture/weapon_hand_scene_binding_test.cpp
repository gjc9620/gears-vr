// Synthetic current-registry Scene tests. No process, game, hook or XR access.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#ifndef GEARS_COVER_BODY_PROTOTYPE
#define GEARS_COVER_BODY_PROTOTYPE 1
#endif
#include <windows.h>
#include <array>
#include <atomic>
#include <mutex>
#include <cstdio>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include "anchor_observer.h"
#include "MinHook.h"
uintptr_t base=0x140000000;
std::atomic<uint64_t> committed_world_generation{1};std::atomic<unsigned> active{};
std::atomic<bool> recording{true};bool continuous_session=true,anchor_observer_enabled=true;
gears_anchor::Config anchor_config;
struct Scope {bool world()const{return true;}};
uintptr_t arena{},denied{};unsigned reads{},native_calls{};size_t denied_read_size{};
void (*read_hook)(uintptr_t,size_t){};
bool safe_copy(void* out,void const* from,size_t size){
 const auto address=reinterpret_cast<uintptr_t>(from);++reads;if(read_hook)read_hook(address,size);
 if(!address||address==denied||(denied_read_size&&size==denied_read_size)||address<arena||size>1024*1024||address>arena+1024*1024-size)return false;
 memcpy(out,from,size);return true;
}
long long now(){return static_cast<long long>(GetTickCount64());}
extern "C" MH_STATUS WINAPI MH_CreateHook(void*,void*,void**){++native_calls;return MH_ERROR_UNSUPPORTED_FUNCTION;}
#if !defined(GEARS_SCENE_BINDING_NO_BODY)
#include "cover_body_live.inl"
#endif
#if GEARS_COVER_BODY_PROTOTYPE && !defined(GEARS_SCENE_BINDING_NO_BODY)
bool entries_valid=true;
namespace cover_body_live {bool own_entries(){return entries_valid;}}
extern "C" void GearsCoverBlendTickHook(){++native_calls;}
extern "C" void GearsCoverChildPropagateHook(){++native_calls;}
extern "C" void GearsCoverRawInvoke(cover_body_live::CallPacket*,void*) noexcept(false){++native_calls;}
#endif
#include "weapon_hand_scene_binding.inl"
namespace h=weapon_hand_scene_binding;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;printf("FAIL %u: %s reason=%s stage=%s field=%ls\n",__LINE__,#x,h::reason_name(h::last_reason),h::stage,h::rejected_field);}}while(0)
#if GEARS_COVER_BODY_PROTOTYPE && !defined(GEARS_SCENE_BINDING_NO_BODY)
namespace b=cover_body_live;
uint64_t fake_ms{};uint64_t test_ms(){return fake_ms;}
uintptr_t fixture_pc{},fixture_pawn{},fixture_mesh{},fixture_world{},fixture_weapon{},fixture_weapon2{};
uintptr_t fixture_meta{},fixture_pc_class{},fixture_pawn_class{},fixture_pawn_base{},fixture_actor{},fixture_mesh_class{},fixture_world_class{},fixture_world_child{},fixture_weapon_base{},fixture_warweapon{},fixture_gun_a{},fixture_gun_b{},fixture_object_property{};
uintptr_t fixture_world_property{},weapon_property{},instigator_property{},mesh_property{},special_property{},fixture_component_property{},fixture_byte_property{};
gears_anchor::Receipt scene;
std::array<unsigned char,1024*1024> saved_arena{};
template<class T>void put(uintptr_t address,T const& value){memcpy(reinterpret_cast<void*>(address),&value,sizeof value);}
template<class T>T get(uintptr_t address){T value{};memcpy(&value,reinterpret_cast<void const*>(address),sizeof value);return value;}
gears_anchor::Identity object(uintptr_t address,unsigned index,uintptr_t cls,uintptr_t outer,const wchar_t* name,unsigned number=0){
 gears_anchor::Identity id{};id.address=address;id.index=id.name_index=index;id.cls=cls;id.outer=outer;id.number=number;wcscpy_s(id.name.data(),id.name.size(),name);
 put(address+8,id.index);put(address+72,id.name_index);put(address+76,id.number);put(address+80,id.cls);put(address+64,id.outer);
 const auto objects=get<gears_anchor::Registry>(b::metadata.objects_header),names=get<gears_anchor::Registry>(b::metadata.names_header);
 put(objects.data+uintptr_t(index)*8,address);const auto entry=arena+0x20000+uintptr_t(index)*0x100;put(names.data+uintptr_t(index)*8,entry);put(entry,index);put(entry+20,id.name);return id;
}
void klass(uintptr_t address,unsigned index,const wchar_t* name,uintptr_t parent){object(address,index,fixture_meta,0,name);put(address+0x60,parent);put(address+0x88,0x1000u);}
void property(uintptr_t address,unsigned index,uintptr_t owner,const wchar_t* name,unsigned offset,uintptr_t reference){
 object(address,index,fixture_object_property,owner,name);put(address+0x70,1u);put(address+0x74,8u);put(address+0x94,offset);put(address+0xd0,reference);
}
void refresh_scene(){
 gears_anchor::PawnBinding p{};C(gears_anchor::capture_pawn_binding(b::metadata,safe_copy,p));scene={};scene.invocation=17;scene.thread=GetCurrentThreadId();scene.scene_qpc=1;scene.family_qpc=2;scene.renderer_qpc=3;
 scene.anchor.pawn=p.pawn.address;scene.anchor.pawn_identity=p.pawn;scene.anchor.controller=p.controller;scene.anchor.camera=p.camera;scene.anchor.level=p.level;
}
void setup(){
 read_hook=nullptr;denied=denied_read_size=0;memset(reinterpret_cast<void*>(arena),0,1024*1024);auto& c=b::metadata;c={};c.objects_header=arena+0x100;c.names_header=arena+0x200;
 c.object_internal_index=8;c.object_name=72;c.object_class=80;c.object_outer=64;c.field_super=96;c.name_entry_index=0;c.name_entry_text=20;c.identity_count=5;
 c.role_controller=0;c.role_local=1;c.role_camera=2;c.role_pawn=3;c.role_level=4;c.controller_pawn=c.pawn_controller=0x100;c.player_camera=c.pc_owner=0x108;c.player=0x110;c.camera_view_target=0x100;
 put(c.objects_header,gears_anchor::Registry{arena+0x1000,2001,2100});put(c.names_header,gears_anchor::Registry{arena+0x6000,2001,2100});
 fixture_meta=arena+0x60000;object(fixture_meta,10,fixture_meta,0,L"Class");
 fixture_pc_class=arena+0x60400;fixture_pawn_class=arena+0x60800;fixture_mesh_class=arena+0x60c00;fixture_actor=arena+0x61000;fixture_pawn_base=arena+0x61400;fixture_object_property=arena+0x61800;
 fixture_world_class=arena+0x61c00;fixture_world_child=arena+0x62000;fixture_weapon_base=arena+0x62800;fixture_warweapon=arena+0x62c00;fixture_gun_a=arena+0x63000;fixture_gun_b=arena+0x63400;
 fixture_component_property=arena+0x63800;fixture_byte_property=arena+0x63c00;
 klass(fixture_actor,14,L"Actor",0);klass(fixture_pc_class,11,L"WarPC",fixture_actor);klass(fixture_pawn_base,15,L"Pawn",fixture_actor);klass(fixture_pawn_class,12,L"WarPawn",fixture_pawn_base);klass(fixture_mesh_class,13,L"SkeletalMeshComponent",0);
 klass(fixture_object_property,16,L"ObjectProperty",0);klass(fixture_world_class,17,L"WorldInfo",fixture_actor);klass(fixture_world_child,18,L"WarWorldInfo",fixture_world_class);
 klass(fixture_component_property,24,L"ComponentProperty",0);klass(fixture_byte_property,25,L"ByteProperty",0);
 klass(fixture_weapon_base,20,L"Weapon",fixture_actor);klass(fixture_warweapon,21,L"WarWeapon",fixture_weapon_base);klass(fixture_gun_a,22,L"GunA",fixture_warweapon);klass(fixture_gun_b,23,L"GunB",fixture_warweapon);
 const auto level=arena+0x38000;const uintptr_t addresses[]={arena+0x32000,arena+0x34000,arena+0x36000,arena+0x30000,level};
 const wchar_t* labels[]={L"SyntheticPC",L"SyntheticLocal",L"SyntheticCamera",L"SyntheticPawn",L"SyntheticLevel"};
 for(unsigned i=0;i<5;++i)c.identities[i]=object(addresses[i],i,i==3?fixture_pawn_class:fixture_pc_class,i==3?level:0,labels[i]);
 fixture_pc=addresses[0];fixture_pawn=addresses[3];fixture_world=arena+0x3a000;fixture_mesh=arena+0x3c000;fixture_weapon=arena+0x3e000;fixture_weapon2=arena+0x40000;
 object(fixture_world,5,fixture_world_child,arena+0x42000,L"SyntheticWorld");object(fixture_mesh,6,fixture_mesh_class,fixture_pawn,L"SyntheticMesh");
 object(fixture_weapon,7,fixture_gun_a,arena+0x44000,L"CurrentWeapon");object(fixture_weapon2,8,fixture_gun_b,arena+0x46000,L"NextWeapon");
 put(fixture_pc+c.controller_pawn,fixture_pawn);put(fixture_pc+c.player_camera,addresses[2]);put(fixture_pc+c.player,addresses[1]);put(fixture_pawn+c.pawn_controller,fixture_pc);put(addresses[2]+c.camera_view_target,fixture_pawn);put(addresses[2]+c.pc_owner,fixture_pc);
 b::layout={};b::layout.mesh=0x180;b::layout.world=0x188;b::layout.cover=0x190;b::layout.special=0x191;
 // Nonzero unused bytes reveal accidental blanket reset/restoration shortcuts.
 b::layout.sources.back().address=0x13579;b::layout.ids.back().name[3]=L'X';
 b::layout.sources[0].address=arena+0x900;b::layout.sources[0].size=4;b::layout.source_count=1;put(arena+0x900,0x1234u);memcpy(b::layout.sources[0].bytes.data(),reinterpret_cast<void*>(arena+0x900),4);
 put(fixture_pawn+0x180,fixture_mesh);put(fixture_pawn+0x188,fixture_world);put(fixture_pc+0x188,fixture_world);
 put(fixture_pawn+0x190,static_cast<unsigned char>(0));put(fixture_pawn+0x191,static_cast<unsigned char>(0));put(fixture_pawn+0x1d0,fixture_weapon);
 put(fixture_weapon+0x218,fixture_pawn);put(fixture_weapon2+0x218,fixture_pawn);
 fixture_world_property=arena+0x68000;weapon_property=arena+0x68400;instigator_property=arena+0x68800;mesh_property=arena+0x68c00;special_property=arena+0x69000;
 property(fixture_world_property,30,fixture_actor,L"WorldInfo",0x188,fixture_world_class);property(weapon_property,31,fixture_pawn_base,L"Weapon",0x1d0,fixture_weapon_base);property(instigator_property,32,fixture_actor,L"Instigator",0x218,fixture_pawn_base);
 property(mesh_property,33,fixture_pawn_base,L"Mesh",0x180,fixture_mesh_class);put(mesh_property+80,fixture_component_property);
 property(special_property,34,fixture_pawn_class,L"SpecialMove",0x191,0);put(special_property+80,fixture_byte_property);put(special_property+0x74,1u);
 put(fixture_actor+0x80,fixture_world_property);put(fixture_world_property+0x68,instigator_property);put(fixture_pawn_base+0x80,weapon_property);put(weapon_property+0x68,mesh_property);put(fixture_pawn_class+0x80,special_property);
 // Ordinary cold-start must succeed with NO cover hook/layout/GT state.
 b::enabled=false;b::layout_ready=false;b::game_thread=0;b::initial_world=0;committed_world_generation=1;b::failed=true;b::external_change=true;b::stopping=true;entries_valid=false;recording=true;
 h::base_proof={};h::instigator_proof={};h::metadata={};h::metadata_bytes={};h::metadata_ready=false;h::metadata_generation=0;h::game_thread=0;h::clock_ms=test_ms;fake_ms=reads=native_calls=0;anchor_config=c;refresh_scene();
}
bool take(h::Binding& out,bool unchanged=true,gears_anchor::Receipt const* receipt=&scene,bool visual=false){
 std::array<unsigned char,sizeof(b::Layout)> layout{};memcpy(layout.data(),&b::layout,sizeof b::layout);
 if(unchanged)memcpy(saved_arena.data(),reinterpret_cast<void*>(arena),saved_arena.size());
 const bool result=visual?h::sample_visual(receipt,out):h::sample(receipt,out);C(!memcmp(layout.data(),&b::layout,sizeof b::layout));C(native_calls==0);
 if(unchanged)C(!memcmp(saved_arena.data(),reinterpret_cast<void*>(arena),saved_arena.size()));return result;
}
uintptr_t trigger{},change_address{};unsigned hit{},nth{};uint64_t changed_value{};size_t changed_size{},trigger_size{};bool throw_read{},change_generation{},change_scene{},change_thread{};
void changing_read(uintptr_t address,size_t size){if(address!=trigger||(trigger_size&&size!=trigger_size)||++hit!=nth)return;
 if(throw_read)throw 9;
 if(change_generation)committed_world_generation=2;
 else if(change_scene)scene.invocation++;
 else if(change_thread)h::game_thread=GetCurrentThreadId()+1;
 else memcpy(reinterpret_cast<void*>(change_address),&changed_value,changed_size);
}
void plan(uintptr_t watched,unsigned number,uintptr_t target,uint64_t value,size_t size){
 trigger=watched;trigger_size=0;nth=number;hit=0;change_address=target;changed_value=value;changed_size=size;throw_read=change_generation=change_scene=change_thread=false;read_hook=changing_read;
}
int main(){
 arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
 // A transient weapon handoff must not freeze a read-only eye observation,
 // and that observation must never acquire equipment or pose permission.
 setup();h::Binding visual{};put(fixture_weapon+0x218,fixture_pc);
 C(!take(visual));C(take(visual,true,&scene,true));
 C(visual.context_valid&&!visual.weapon_known&&!visual.weapon.address);
 C(h::same_visual_key(visual,visual)&&!h::same_key(visual,visual)&&!h::current_actor(visual));
 {auto foreign=visual;foreign.mesh=visual.pawn;C(!h::same_visual_key(visual,foreign));}
 setup();put(fixture_pawn+0x100,fixture_weapon);C(!take(visual,true,&scene,true));
 setup();h::Binding out{};denied=fixture_pawn+0x200;const auto start=reads;C(take(out));const auto cold_reads=reads-start;
 C(out.context_valid&&out.weapon_known&&out.weapon.address==fixture_weapon&&out.controller.address==fixture_pc&&out.pawn.address==fixture_pawn&&out.mesh.address==fixture_mesh&&out.world.address==fixture_world);
 C(out.world.outer!=scene.anchor.level&&out.weapon.outer!=out.pawn.outer&&out.shared_actor_world&&out.special_move==0&&out.scene_invocation==17&&out.world_generation==1);
 C(!out.actor_or_mesh_fields_written&&!out.atomic_snapshot_verified);const auto warmed=reads;C(take(out));printf("binding synthetic reads cold=%u warm=%u; base sources=%u/ids=%u instigator sources=%u/ids=%u\n",cold_reads,reads-warmed,h::base_proof.source_count,h::base_proof.identity_count,h::instigator_proof.source_count,h::instigator_proof.identity_count);
 // Head visibility uses a fresh real local-model read, without requiring the
 // old gun to remain selected. It never accepts another owner or mesh.
 C(h::current_local_visual_model(out));
 put(fixture_pawn+0x1d0,fixture_weapon2);C(h::current_local_visual_model(out));C(!h::current_actor(out));
 put(fixture_pawn+0x1d0,fixture_weapon);
 {auto bad=out;bad.mesh=out.weapon;C(!h::current_local_visual_model(bad));bad=out;bad.controller=out.pawn;C(!h::current_local_visual_model(bad));}
 recording=false;C(!h::current_local_visual_model(out));recording=true;
 committed_world_generation=2;C(!h::current_local_visual_model(out));committed_world_generation=1;
 h::game_thread=GetCurrentThreadId()+1;C(!h::current_local_visual_model(out));h::game_thread=GetCurrentThreadId();
 put(fixture_pc+0x188,uintptr_t(0));C(!h::current_local_visual_model(out));put(fixture_pc+0x188,fixture_pawn);
 C(!b::enabled&&!b::layout_ready&&b::game_thread==0&&b::initial_world==0&&h::game_thread==GetCurrentThreadId());
 // Cached render Scene identity remains immutable while an actual Lancer
 // ready/execution policy change is freshly observed under the next Tick.
 setup();klass(fixture_gun_a,22,L"Weap_AssaultRifle",fixture_warweapon);C(take(out));
 {const auto original=out;h::Binding tick{};
  for(unsigned move:{33u,34u,0u}){const auto previous=out.special_move;put(fixture_pawn+0x191,static_cast<unsigned char>(move));
   C(h::current_tick_actor(out,tick)&&tick.special_move==move&&h::same_key(out,tick)&&h::current_actor(tick));
   C(out.special_move==previous&&out.scene_invocation==original.scene_invocation);out=tick;}
  const auto expect=out;tick.special_move=255;
  put(fixture_pawn+0x191,static_cast<unsigned char>(12));C(!h::current_tick_actor(expect,tick)&&tick.special_move==255);
  put(fixture_pawn+0x191,static_cast<unsigned char>(33));put(fixture_pawn+0x1d0,fixture_weapon2);
  C(!h::current_tick_actor(expect,tick)&&tick.special_move==255);put(fixture_pawn+0x1d0,fixture_weapon);
  committed_world_generation=2;C(!h::current_tick_actor(expect,tick)&&tick.special_move==255);committed_world_generation=1;
  plan(fixture_pawn+0x191,2,fixture_pawn+0x191,0,1);C(!h::current_tick_actor(expect,tick)&&tick.special_move==255);read_hook=nullptr;}
 setup();C(take(out));{h::Binding tick{};C(h::current_tick_actor(out,tick));
  put(fixture_pawn+0x191,static_cast<unsigned char>(33));tick.special_move=255;
  C(!h::current_tick_actor(out,tick)&&tick.special_move==255);} // GunA is not Lancer.
 setup();b::metadata={};b::layout={};denied=arena+0x900;C(take(out)); // No body metadata/entry/source dependency.
 setup();scene.thread++;C(!take(out)&&h::game_thread==0);refresh_scene();C(take(out)&&h::game_thread==GetCurrentThreadId());
 {auto key=out;C(h::same_key(out,key));++key.scene_invocation;++key.special_move;C(h::same_key(out,key));++key.weapon.number;C(!h::same_key(out,key));key=out;++key.controller.number;C(!h::same_key(out,key));key=out;++key.world_generation;C(!h::same_key(out,key));key=out;key.weapon_known=false;C(!h::same_key(out,key));key=out;key.context_valid=false;C(!h::same_key(out,key));}
 // Typed null means known empty, even if a previous gun's metadata disappears.
 put(fixture_pawn+0x1d0,uintptr_t(0));put(fixture_gun_a+76,3u);C(take(out)&&out.weapon_known&&out.context_valid&&!out.weapon.address&&!out.weapon.cls);
 {auto key=out;C(h::same_key(out,key));key.weapon.address=fixture_weapon;C(!h::same_key(out,key));}
 setup();C(take(out));put(fixture_pawn+0x1d0,fixture_weapon2);C(take(out)&&out.weapon.address==fixture_weapon2&&out.weapon.cls==fixture_gun_b);
 put(fixture_weapon2+76,7u);C(take(out)&&out.weapon.number==7);put(fixture_pawn+0x191,static_cast<unsigned char>(12));C(take(out)&&out.special_move==12);
 setup();put(fixture_pawn+0x1d0,uintptr_t(0));C(take(out)&&out.weapon_known&&!out.weapon.address&&h::instigator_proof.attempts==0);
 setup();C(take(out));const auto prior_generation=out;committed_world_generation=2;C(take(out)&&out.world_generation==2&&!h::same_key(prior_generation,out)&&b::initial_world==0);
 setup();put(mesh_property+80,fixture_object_property);C(take(out)); // Both reflected pointer metaclasses are certified.
 for(unsigned which=0;which<6;++which){setup();if(which==0)put(mesh_property+0xd0,fixture_weapon_base);if(which==1)put(mesh_property+0x74,4u);if(which==2)put(mesh_property+0x70,2u);if(which==3)put(special_property+0x74,4u);if(which==4)put(special_property+80,fixture_object_property);if(which==5)anchor_config.name_entry_text=0x1001;C(!take(out));}
 // Separately certified PC declaration is never assumed to share Actor offset.
 setup();const auto separate=arena+0x69c00;property(separate,37,fixture_pc_class,L"WorldInfo",0x198,fixture_world_class);put(fixture_pc_class+0x80,separate);put(fixture_pc+0x198,fixture_world);C(take(out)&&!out.shared_actor_world);
 for(unsigned which=0;which<11;++which){setup();if(which==0)recording=false;if(which==1)anchor_config.identity_count=0;if(which==2)anchor_config.objects_header=0;if(which==3)h::game_thread=GetCurrentThreadId()+1;if(which==4)committed_world_generation=0;if(which==5)scene.renderer_qpc=0;if(which==6)anchor_config.role_pawn=127;if(which==7)scene.thread++;if(which==8)scene.family_qpc=0;if(which==9)scene.invocation=0;if(which==10)anchor_config.identities[3].cls=fixture_actor;
  out.scene_invocation=999;C(!take(out)&&out.scene_invocation==999);}
 setup();C(!take(out,true,nullptr)&&h::last_reason==h::Reason::no_scene);
 for(unsigned which=0;which<12;++which){setup();if(which==0)put(fixture_pc+0x188,uintptr_t(0));if(which==1)put(fixture_pawn+0x188,fixture_weapon);if(which==2)put(fixture_pc+0x100,uintptr_t(0));if(which==3)put(fixture_pawn+0x100,fixture_weapon);if(which==4)put(fixture_weapon+0x218,fixture_pc);if(which==5)put(fixture_weapon+80,fixture_weapon_base);if(which==6)put(fixture_pawn+80,fixture_actor);if(which==7)put(fixture_mesh+80,fixture_actor);if(which==8)put(fixture_mesh+76,0u),put(fixture_pawn+0x180,uintptr_t(0));if(which==9)scene.anchor.cinematic=true;if(which==10)scene.anchor.unpossessed=true;if(which==11)put(fixture_world+80,fixture_actor);
  out.scene_invocation=999;C(!take(out)&&out.scene_invocation==999);}
 // Exact property metaclass, owner extent, dimension, width and D0 references.
 for(unsigned which=0;which<14;++which){setup();if(which==0)put(fixture_world_property+0x70,2u);if(which==1)put(fixture_world_property+0x74,4u);if(which==2)put(fixture_world_property+0x94,0xfffcu);if(which==3)put(fixture_world_property+80,fixture_actor);if(which==4)put(fixture_world_property+0xd0,fixture_actor);if(which==5)put(weapon_property+0xd0,fixture_warweapon);if(which==6)put(instigator_property+0xd0,fixture_pawn_class);if(which==7)put(fixture_actor+0x88,0x100u);if(which==8)put(weapon_property+64,fixture_actor);if(which==9)put(fixture_world_property+76,1u);if(which==10)put(weapon_property+0x68,weapon_property);if(which==11)put(instigator_property+0x74,16u);if(which==12)put(instigator_property+0x94,0xfffcu);if(which==13)put(fixture_pawn_base+0x80,uintptr_t(0));
  out.scene_invocation=999;C(!take(out)&&out.scene_invocation==999);}
 setup();C(take(out));put(fixture_world_property+0x94,0x198u);C(!take(out)&&h::last_reason==h::Reason::layout_changed);
 setup();put(weapon_property+0x74,4u);for(unsigned i=0;i<8;++i){fake_ms=uint64_t(i)*1000;C(!take(out)&&h::base_proof.attempts==i+1);C(!take(out)&&h::base_proof.attempts==i+1);}fake_ms=9000;C(!take(out)&&h::base_proof.attempts==8);put(weapon_property+0x74,8u);C(!take(out)&&h::base_proof.attempts==8);
 setup();put(instigator_property+0x74,4u);for(unsigned i=0;i<8;++i){fake_ms=uint64_t(i)*1000;C(!take(out)&&h::instigator_proof.attempts==i+1);C(!take(out)&&h::instigator_proof.attempts==i+1);}fake_ms=9000;C(!take(out)&&h::instigator_proof.attempts==8);
 for(unsigned which=0;which<9;++which){setup();C(take(out));if(which==0)plan(fixture_pawn+0x1d0,2,fixture_pawn+0x1d0,fixture_weapon2,8);if(which==1)plan(fixture_pawn+0x1d0,2,fixture_weapon+76,1u,4);if(which==2)plan(fixture_pawn+0x188,2,fixture_pc+0x188,0,8);if(which==3)plan(fixture_pawn+0x1d0,2,fixture_weapon+0x218,fixture_pc,8);if(which==4)plan(fixture_pawn+0x1d0,2,fixture_world_property+0x94,0x198u,4);if(which==5){plan(fixture_pawn+0x1d0,2,0,0,0);change_generation=true;}if(which==6){plan(fixture_pawn+0x1d0,2,0,0,0);change_scene=true;}if(which==7){plan(fixture_pawn+0x1d0,2,0,0,0);change_thread=true;}if(which==8)plan(fixture_pawn+0x1d0,2,b::metadata.names_header+12,2101u,4);
  out.scene_invocation=999;C(!take(out,false)&&out.scene_invocation==999);read_hook=nullptr;}
 setup();C(take(out));plan(fixture_pawn+0x1d0,2,reinterpret_cast<uintptr_t>(&anchor_config.controller_pawn),0x118u,4);out.scene_invocation=999;C(!take(out)&&out.scene_invocation==999);read_hook=nullptr;
 // Fail/exception during either metadata proof and during warm dynamic reads.
 for(unsigned which=0;which<3;++which){setup();if(which==2)C(take(out));plan(which==0?fixture_world_property+0x70:(which==1?instigator_property+0x70:fixture_pawn+0x1d0),1,0,0,0);throw_read=true;
  out.scene_invocation=999;C(!take(out)&&out.scene_invocation==999&&h::last_reason==h::Reason::exception);read_hook=nullptr;const bool unlocked=h::mutex.try_lock();C(unlocked);if(unlocked)h::mutex.unlock();fake_ms=1000;C(take(out));}
 setup();denied=fixture_weapon+0x218;C(!take(out));denied=0;fake_ms=1000;C(take(out));unsigned overflow{};C(!h::source_field(h::base_proof,overflow,UINTPTR_MAX-2,4));
 // The layout-specific read batching must preserve both identity observations,
 // the complete semantic name, and no-partial-output failure behavior.
 setup();C(take(out));gears_anchor::Identity id{};auto begin_reads=reads;C(h::identity(fixture_weapon,id));
 C(reads-begin_reads==14&&id.address==fixture_weapon&&!wcscmp(id.name.data(),L"CurrentWeapon"));
 for(unsigned field=0;field<7;++field){setup();C(take(out));id={};id.address=999;
  const auto names=get<gears_anchor::Registry>(h::metadata.names_header);
  const auto entry=get<uintptr_t>(names.data+uintptr_t(get<unsigned>(fixture_weapon+72))*8);
  if(field<5){const unsigned offsets[]={8,64,72,76,80};const auto offset=offsets[field];
   const auto width=field==1||field==4?8u:4u;const auto value=width==8?get<uintptr_t>(fixture_weapon+offset):uint64_t(get<unsigned>(fixture_weapon+offset));
   plan(fixture_weapon+8,2,fixture_weapon+offset,value+1,width);trigger_size=80;
  }else{plan(entry,2,entry+(field==5?20:0),field==5?uint64_t(L'X'):uint64_t(get<unsigned>(entry)+1),field==5?2:4);trigger_size=146;}
  C(!h::identity(fixture_weapon,id)&&id.address==999);C(hit==2);read_hook=nullptr;
 }
 // An oversized read may be unavailable even though all original scalar
 // fields / the terminated short name are readable. Fall back, never use
 // partial block bytes or silently discard a required scalar read.
 for(size_t blocked:{size_t(80),size_t(146)}){setup();C(take(out));denied_read_size=blocked;id={};C(h::identity(fixture_weapon,id)&&id.address==fixture_weapon);denied_read_size=0;}
 setup();C(take(out));id={};id.address=999;const auto name_table=get<gears_anchor::Registry>(h::metadata.names_header);
 const auto name_entry=get<uintptr_t>(name_table.data+uintptr_t(get<unsigned>(fixture_weapon+72))*8);
 std::array<wchar_t,64> expected_name{};wcscpy_s(expected_name.data(),expected_name.size(),L"CurrentWeapon");
 // Real name entries have unrelated nonzero bytes after the first terminator.
 // They must remain zero in the semantic Identity, and may change between
 // observations without changing the object's actual name.
 memset(reinterpret_cast<void*>(name_entry+20+28),0xa5,98);C(h::identity(fixture_weapon,id)&&id.name==expected_name);
 plan(name_entry,2,name_entry+20+28,0x5a5au,2);trigger_size=146;
 C(h::identity(fixture_weapon,id)&&id.name==expected_name&&hit==2);read_hook=nullptr;
 std::array<wchar_t,64> long_name{};long_name.fill(L'Q');put(name_entry+20,long_name);id={};id.address=999;
 C(!h::identity(fixture_weapon,id)&&id.address==999);C(h::identity(fixture_weapon,id,false,true)&&id.name[62]==L'Q'&&id.name[63]==0);
 put(name_entry+20,wchar_t(0));id.address=999;C(!h::identity(fixture_weapon,id)&&id.address==999);
 C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);printf("weapon_hand_scene_binding: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
#elif !GEARS_COVER_BODY_PROTOTYPE
int main(){h::Binding out{};out.scene_invocation=999;C(!h::sample(nullptr,out)&&out.scene_invocation==999&&h::last_reason==h::Reason::body_unavailable);printf("weapon_hand_scene_binding disabled: %u checks, %u failures\n",checks,failures);return failures?1:0;}
#else
int main(){h::Binding out{};out.scene_invocation=999;C(!h::sample(nullptr,out)&&out.scene_invocation==999&&h::last_reason==h::Reason::no_scene);printf("weapon_hand_scene_binding compiled without ANY body declaration: %u checks, %u failures\n",checks,failures);return failures?1:0;}
#endif
