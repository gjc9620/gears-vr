// Synthetic current-registry / typed-reflection tests. The native query seam
// returns private matrices; production getter byte/ABI checks have their own
// test. No process opening, hook installation, camera or game operation.
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
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include "anchor_observer.h"
#include "../vr-common/pose_math.h"
#include "MinHook.h"
uintptr_t base=0x140000000;
std::atomic<uint64_t> committed_world_generation{1};std::atomic<unsigned> active{};
std::atomic<bool> recording{true};bool continuous_session=true,anchor_observer_enabled=true;
double stereo_units_per_metre=100;gears_anchor::Config anchor_config;
struct Scope {bool world()const{return true;}};
uintptr_t arena{},denied{};unsigned reads{};
bool safe_copy(void* out,void const* from,size_t size){
 const auto address=reinterpret_cast<uintptr_t>(from);++reads;
 if(!address||address==denied||address<arena||size>1024*1024||address>arena+1024*1024-size)return false;
 memcpy(out,from,size);return true;
}
long long now(){return static_cast<long long>(GetTickCount64());}
extern "C" MH_STATUS WINAPI MH_CreateHook(void*,void*,void**){return MH_ERROR_UNSUPPORTED_FUNCTION;}
#include "cover_body_live.inl"
bool entries_valid=true;
namespace cover_body_live {bool own_entries(){return entries_valid;}}
extern "C" void GearsCoverBlendTickHook(){}
extern "C" void GearsCoverChildPropagateHook(){}
extern "C" void GearsCoverRawInvoke(cover_body_live::CallPacket*,void*) noexcept(false){}
#include "cover_neutral_eye_live.inl"
namespace b=cover_body_live;namespace n=cover_neutral_eye_live;
unsigned checks{},failures{},queries{};uint64_t clock_ms{},seen_name{};uintptr_t seen_mesh{};
#define C(x) do{++checks;if(!(x)){++failures;printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
uintptr_t fixture_pawn{},fixture_mesh{},fixture_link{},fixture_world{},pawn_class{},link_class{},fixture_actor_class{},fixture_world_class{},fixture_pc_class{},fixture_world_prop{},props[6]{},head_entry{};
gears_anchor::Receipt fixture_scene;bool query_ok=true,invalid_matrix{};int dynamic_index=27;
gears_vr::Vector head_offset{2,3,35};
enum class Change {none,ca,slot,location,world_generation,header_capacity,header_growth,name,bool_neighbor,link_world,controller_world,pawn_world,all_worlds,world_type,world_property,mask};Change change{};
template<class T>void put(uintptr_t address,T const& value){memcpy(reinterpret_cast<void*>(address),&value,sizeof value);}
template<class T>T get(uintptr_t address){T value{};memcpy(&value,reinterpret_cast<void const*>(address),sizeof value);return value;}
gears_anchor::Identity object(uintptr_t address,unsigned index,uintptr_t cls,uintptr_t outer,const wchar_t* name,unsigned number=0){
 gears_anchor::Identity id{};id.address=address;id.index=id.name_index=index;id.cls=cls;id.outer=outer;id.number=number;wcscpy_s(id.name.data(),id.name.size(),name);
 put(address+8,id.index);put(address+72,id.name_index);put(address+76,id.number);put(address+80,id.cls);put(address+64,id.outer);
 auto objects=get<gears_anchor::Registry>(b::metadata.objects_header),names=get<gears_anchor::Registry>(b::metadata.names_header);
 put(objects.data+uintptr_t(index)*8,address);auto entry=arena+0x20000+uintptr_t(index)*0x100;put(names.data+uintptr_t(index)*8,entry);put(entry,index);put(entry+20,id.name);return id;
}
void refresh_scene(){
 gears_anchor::PawnBinding binding{};C(gears_anchor::capture_pawn_binding(b::metadata,safe_copy,binding));fixture_scene={};fixture_scene.invocation=7;fixture_scene.thread=GetCurrentThreadId();fixture_scene.scene_qpc=1;fixture_scene.family_qpc=2;fixture_scene.renderer_qpc=3;
 fixture_scene.anchor.pawn=binding.pawn.address;fixture_scene.anchor.pawn_identity=binding.pawn;fixture_scene.anchor.controller=binding.controller;fixture_scene.anchor.camera=binding.camera;fixture_scene.anchor.level=binding.level;
 fixture_scene.anchor.location=get<std::array<float,3>>(fixture_pawn+b::metadata.pawn_location);
}
void setup(){
 memset(reinterpret_cast<void*>(arena),0,1024*1024);auto& c=b::metadata;c={};c.objects_header=arena+0x100;c.names_header=arena+0x200;
 c.object_internal_index=8;c.object_name=72;c.object_class=80;c.object_outer=64;c.field_super=96;c.name_entry_index=0;c.name_entry_text=20;c.identity_count=5;
 c.role_controller=0;c.role_local=1;c.role_camera=2;c.role_pawn=3;c.role_level=4;c.controller_pawn=c.pawn_controller=0x100;c.player_camera=c.pc_owner=0x108;c.player=0x110;c.camera_view_target=0x100;c.pawn_location=0x1a0;
 put(c.objects_header,gears_anchor::Registry{arena+0x1000,2001,2100});put(c.names_header,gears_anchor::Registry{arena+0x6000,2001,2100});
 auto meta=arena+0x60000,generic=arena+0x60400;object(meta,10,meta,0,L"Class");object(generic,11,meta,0,L"Object");
 pawn_class=arena+0x60800;object(pawn_class,12,meta,0,L"WarPawn");put(pawn_class+0x88,0x1000u);
 auto mesh_class=arena+0x60c00;object(mesh_class,13,meta,0,L"SkeletalMeshComponent");link_class=arena+0x61000;object(link_class,14,meta,0,L"CoverLink");
 const wchar_t* types[]={L"ObjectProperty",L"IntProperty",L"ByteProperty",L"BoolProperty"};uintptr_t type_classes[4]{};
 for(unsigned i=0;i<4;++i){type_classes[i]=arena+0x61400+uintptr_t(i)*0x400;object(type_classes[i],15+i,meta,0,types[i]);}
 fixture_actor_class=arena+0x62400;fixture_world_class=arena+0x62800;fixture_pc_class=arena+0x62c00;fixture_world_prop=arena+0x63000;
 object(fixture_actor_class,19,meta,0,L"Actor");object(fixture_world_class,20,meta,0,L"WorldInfo");object(fixture_pc_class,21,meta,0,L"WarPC");
 for(auto cls:{pawn_class,link_class,fixture_pc_class,fixture_world_class}){put(cls+0x60,fixture_actor_class);put(cls+0x88,0x1000u);}put(fixture_actor_class+0x60,generic);put(fixture_actor_class+0x88,0x1000u);
 object(fixture_world_prop,22,type_classes[0],fixture_actor_class,L"WorldInfo");put(fixture_actor_class+0x80,fixture_world_prop);put(fixture_world_prop+0x70,1u);put(fixture_world_prop+0x74,8u);put(fixture_world_prop+0x94,0x188u);put(fixture_world_prop+0xd0,fixture_world_class);
 auto level=arena+0x38000;const uintptr_t addresses[]={arena+0x32000,arena+0x34000,arena+0x36000,arena+0x30000,level};
 const wchar_t* labels[]={L"SyntheticPC",L"SyntheticLocal",L"SyntheticCamera",L"SyntheticPawn",L"SyntheticLevel"};
 for(unsigned i=0;i<5;++i)c.identities[i]=object(addresses[i],i,i==3?pawn_class:(i==0?fixture_pc_class:generic),i==3?level:0,labels[i]);
 fixture_pawn=addresses[3];fixture_world=arena+0x3a000;fixture_mesh=arena+0x3c000;fixture_link=arena+0x3e000;
 object(fixture_world,5,fixture_world_class,level,L"SyntheticWorld");object(fixture_mesh,6,mesh_class,fixture_pawn,L"SyntheticMesh");object(fixture_link,7,link_class,level,L"SyntheticLink");
 put(addresses[0]+c.controller_pawn,fixture_pawn);put(addresses[0]+c.player_camera,addresses[2]);put(addresses[0]+c.player,addresses[1]);
 put(fixture_pawn+c.pawn_controller,addresses[0]);put(addresses[2]+c.camera_view_target,fixture_pawn);put(addresses[2]+c.pc_owner,addresses[0]);
 b::layout={};b::layout.mesh=0x180;b::layout.world=0x188;b::layout.cover=0x190;b::layout.special=0x191;
 put(fixture_link+0x188,fixture_world);put(addresses[0]+0x188,fixture_world);
 put(fixture_pawn+0x180,fixture_mesh);put(fixture_pawn+0x188,fixture_world);put(fixture_pawn+0x190,static_cast<unsigned char>(2));put(fixture_pawn+0x191,static_cast<unsigned char>(0));put(fixture_pawn+c.pawn_location,std::array<float,3>{100,200,300});
 for(unsigned i=0;i<n::field_count;++i){props[i]=arena+0x65000+uintptr_t(i)*0x400;auto type=type_classes[i==0?0:(i==1?1:(i==4?3:2))];object(props[i],30+i,type,pawn_class,n::specs[i].name);
  put(props[i]+0x70,1u);put(props[i]+0x74,n::specs[i].width);put(props[i]+0x94,i==0?0x200u:(i==1?0x208u:(i==2?0x20cu:(i==3?0x20du:0x210u))));put(props[i]+0x68,i+1<n::field_count?props[i]+0x400:uintptr_t(0));}
 put(pawn_class+0x80,props[0]);put(props[0]+0xd0,link_class);put(props[4]+0xd0,0x20u);put(fixture_pawn+0x200,fixture_link);put(fixture_pawn+0x208,0);put(fixture_pawn+0x210,0x801u);
 auto names=get<gears_anchor::Registry>(c.names_header);head_entry=arena+0x20000+199*0x100;put(names.data+199*8,head_entry);put(head_entry,199u);std::array<wchar_t,10> head{L'b',L'_',L'M',L'F',L'_',L'H',L'e',L'a',L'd',0};put(head_entry+20,head);
 b::layout_ready=true;b::game_thread=GetCurrentThreadId();b::initial_world=1;committed_world_generation=1;b::failed=false;b::external_change=false;b::stopping=false;b::transactions=0;b::injecting=false;b::step=0;b::journals={};entries_valid=true;
 n::proof={};n::calibration={};n::last_reason=n::Reason::body_unavailable;
 clock_ms=queries=reads=0;denied=0;query_ok=true;invalid_matrix=false;head_offset={2,3,35};change=Change::none;dynamic_index=27;stereo_units_per_metre=100;refresh_scene();
}
struct Query {
 template<class Read,class Guard>bool operator()(uintptr_t module,uintptr_t component,uint64_t name,unsigned thread,Read,Guard guard,gears_cover_bones::Result& out)const{
  ++queries;seen_mesh=component;seen_name=name;C(module==base&&thread==GetCurrentThreadId());if(!guard()||!query_ok)return false;
  if(change==Change::ca)put(fixture_pawn+0x20c,static_cast<unsigned char>(7));
  if(change==Change::slot)put(fixture_pawn+0x208,1);
  if(change==Change::location){auto v=get<std::array<float,3>>(fixture_pawn+0x1a0);v[0]+=1;put(fixture_pawn+0x1a0,v);}
  if(change==Change::world_generation)committed_world_generation=2;
  if(change==Change::header_capacity){auto h=get<gears_anchor::Registry>(b::metadata.names_header);++h.capacity;put(b::metadata.names_header,h);}
  if(change==Change::header_growth){for(auto address:{b::metadata.names_header,b::metadata.objects_header}){auto h=get<gears_anchor::Registry>(address);++h.count;put(address,h);}}
  if(change==Change::name)put(head_entry+20,wchar_t(L'X'));
  if(change==Change::bool_neighbor)put(fixture_pawn+0x210,0xc01u);
  if(change==Change::link_world)put(fixture_link+0x188,uintptr_t(0));
  if(change==Change::controller_world)put(b::metadata.identities[0].address+0x188,uintptr_t(0));
  if(change==Change::pawn_world)put(fixture_pawn+0x188,uintptr_t(0));
  if(change==Change::world_type)put(fixture_world+80,pawn_class);
  if(change==Change::world_property)put(fixture_world_prop+0x94,0x198u);
  if(change==Change::mask)put(props[4]+0xd0,0x40u);
  if(change==Change::all_worlds){const auto alternate=arena+0x3f000;object(alternate,8,fixture_world_class,fixture_scene.anchor.level,L"SyntheticWorld2");for(auto actor:{fixture_pawn,fixture_link,b::metadata.identities[0].address})put(actor+0x188,alternate);}
  auto location=get<std::array<float,3>>(fixture_pawn+0x1a0);out={};out.index=dynamic_index;out.world.values[0]=out.world.values[5]=out.world.values[10]=out.world.values[15]=1;
  out.world.values[12]=float(location[0]+head_offset.x);out.world.values[13]=float(location[1]+head_offset.y);out.world.values[14]=float(location[2]+head_offset.z);if(invalid_matrix)out.world.values[3]=1;
  return guard();
 }
};
bool sample(n::EyeSample& out){return n::sample_with(&fixture_scene,out,[]{return clock_ms;},Query{});}
bool calibrate(n::EyeSample& out){C(!sample(out));clock_ms=100;return sample(out);}
std::array<unsigned char,1024*1024> untouched{};
std::array<unsigned char,sizeof(b::Layout)> original_layout{};
int main(){
 arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,untouched.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;n::EyeSample out{};out.slot=999;
 setup();memcpy(untouched.data(),reinterpret_cast<void*>(arena),untouched.size());memcpy(original_layout.data(),&b::layout,sizeof b::layout);
 C(calibrate(out));C(queries==1&&seen_name==199&&out.head_index==27&&seen_mesh==fixture_mesh&&out.neutral_eye_world.z==335&&out.neutral_eye_world.x==102);
 C(!out.mutation_authorized&&!out.view_activation_authorized&&out.transition_mask==0x20&&!out.transition);
 C(!memcmp(untouched.data(),reinterpret_cast<void*>(arena),untouched.size()));C(!memcmp(original_layout.data(),&b::layout,sizeof b::layout));
 // Streaming Levels may differ; each selected WorldInfo relation and its
 // class/metadata remain independently certified in the neutral Proof.
 setup();const auto streamed_level=arena+0x39000;object(streamed_level,23,get<uintptr_t>(fixture_scene.anchor.level+80),0,L"StreamedLevel");put(fixture_link+64,streamed_level);put(fixture_world+64,streamed_level);
 memcpy(untouched.data(),reinterpret_cast<void*>(arena),untouched.size());memcpy(original_layout.data(),&b::layout,sizeof b::layout);
 C(calibrate(out)&&out.link.outer!=fixture_scene.anchor.level&&out.current_world==fixture_world&&n::proof.shared_actor_world);
 C(n::proof.world_fields[n::pawn_world].id.address==fixture_world_prop&&n::proof.world_fields[n::link_world].id.address==fixture_world_prop&&n::proof.world_fields[n::controller_world].id.address==fixture_world_prop);
 C(n::proof.world_class.address==fixture_world_class&&!memcmp(untouched.data(),reinterpret_cast<void*>(arena),untouched.size())&&!memcmp(original_layout.data(),&b::layout,sizeof b::layout));
 // Independently declared role fields have their own descriptor and offset;
 // a mutable body offset is never used as the Link/PC cache.
 setup();for(unsigned role=0;role<2;++role){const auto owner=role?fixture_pc_class:link_class,property=arena+0x63400+uintptr_t(role)*0x400;object(property,24+role,get<uintptr_t>(fixture_world_prop+80),owner,L"WorldInfo");
  put(owner+0x80,property);put(property+0x70,1u);put(property+0x74,8u);put(property+0x94,role?0x1b8u:0x198u);put(property+0xd0,fixture_world_class);put((role?b::metadata.identities[0].address:fixture_link)+(role?0x1b8:0x198),fixture_world);}
 memcpy(original_layout.data(),&b::layout,sizeof b::layout);C(calibrate(out)&&!n::proof.shared_actor_world&&n::proof.world_fields[n::link_world].offset==0x198&&n::proof.world_fields[n::controller_world].offset==0x1b8);
 C(!memcmp(original_layout.data(),&b::layout,sizeof b::layout));
 setup();const auto derived_link=arena+0x63c00;object(derived_link,26,get<uintptr_t>(link_class+80),0,L"DerivedCoverLink");put(derived_link+0x60,link_class);put(derived_link+0x88,0x1000u);put(fixture_link+80,derived_link);C(calibrate(out)&&n::proof.shared_actor_world);
 // A different or invalid world, and a non-WorldInfo actual class, reject
 // before native getters. WorldInfo's Outer is deliberately not a Level key.
 for(unsigned role=0;role<3;++role){setup();const auto alien=arena+0x3f000;object(alien,8,fixture_world_class,fixture_scene.anchor.level,L"OtherWorld");const auto actor=role==0?fixture_pawn:(role==1?fixture_link:b::metadata.identities[0].address);put(actor+0x188,alien);C(!sample(out)&&!queries&&n::last_reason==n::Reason::link);}
 setup();put(fixture_world+8,1999u);C(!sample(out)&&!queries&&n::last_reason==n::Reason::link);
 setup();put(fixture_world+80,pawn_class);C(!sample(out)&&!queries&&n::last_reason==n::Reason::link);
 for(unsigned which=0;which<11;++which){setup();
  if(which==0)put(fixture_world_prop+0x70,2u);if(which==1)put(fixture_world_prop+0x74,4u);if(which==2)put(fixture_world_prop+0x94,0xfffcu);if(which==3)put(fixture_world_prop+64,pawn_class);
  if(which==4)put(fixture_world_prop+0xd0,fixture_pc_class);if(which==5)put(fixture_world_class+76,1u);if(which==6)put(fixture_world_prop+80,get<uintptr_t>(props[4]+80));if(which==7)put(fixture_actor_class+0x88,0x18cu);
  if(which==8)put(link_class+0x60,link_class);if(which==9)put(fixture_pc_class+0x60,uintptr_t(0));if(which==10)b::layout.world=0x198;
  C(!sample(out)&&!queries&&n::last_reason==n::Reason::layout_rejected);}
 // Every selected property and class-path source remains fresh after cache.
 for(unsigned which=0;which<7;++which){setup();C(calibrate(out));
  if(which==0)put(fixture_world_prop+0x94,0x198u);if(which==1)put(fixture_world_prop+0xd0,fixture_pc_class);if(which==2)put(fixture_world_prop+80,get<uintptr_t>(props[4]+80));if(which==3)put(fixture_world_class+76,1u);
  if(which==4)put(fixture_world_class+0x60,uintptr_t(0));if(which==5)put(link_class+0x60,uintptr_t(0));if(which==6)put(props[4]+0xd0,0x40u);
  clock_ms=200;C(!sample(out)&&n::last_reason==n::Reason::layout_changed&&!n::calibration.ready);}
 for(auto value:{Change::link_world,Change::controller_world,Change::pawn_world,Change::all_worlds,Change::world_type,Change::world_property,Change::mask}){setup();change=value;C(!calibrate(out)&&queries==1&&!n::calibration.ready);}
 setup();C(calibrate(out));const auto frozen_world_offset=n::proof.world_fields[n::pawn_world].offset;b::layout.world=0x198;clock_ms=200;C(!sample(out)&&!n::calibration.ready&&n::proof.world_fields[n::pawn_world].offset==frozen_world_offset);
 setup();C(calibrate(out));
 // Freeze the neutral offset; only current root translation is composed after
 // native/virtual pose changes. No fresh head query and no HMD access exist.
 put(fixture_pawn+0x1a0,std::array<float,3>{110,220,330});put(fixture_pawn+0x20c,static_cast<unsigned char>(7));put(fixture_pawn+0x20d,static_cast<unsigned char>(7));put(fixture_pawn+0x191,static_cast<unsigned char>(12));put(fixture_pawn+0x210,0x821u);b::injecting=true;b::step=3;head_offset.z=80;clock_ms=200;
 C(sample(out)&&queries==1&&out.neutral_eye_world.x==112&&out.neutral_eye_world.y==223&&out.neutral_eye_world.z==365&&out.native_ca==7&&out.special_move==12&&out.transition);
 // Typed neutral fields and the actual masked bit all gate first calibration.
 for(unsigned which=0;which<6;++which){setup();if(which==0)put(fixture_pawn+0x20c,static_cast<unsigned char>(7));if(which==1)put(fixture_pawn+0x20d,static_cast<unsigned char>(7));if(which==2)put(fixture_pawn+0x191,static_cast<unsigned char>(12));if(which==3)put(fixture_pawn+0x210,0x821u);if(which==4){b::injecting=true;b::step=2;}if(which==5)b::journals[0].round_open=true;
  C(!sample(out));clock_ms=200;C(!sample(out)&&!queries&&n::last_reason==n::Reason::neutral_wait);}
 setup();C(!sample(out));clock_ms=99;C(!sample(out)&&!queries);clock_ms=100;C(sample(out)&&queries==1);
 // The standalone bool reader uses D0, ignores unrelated word bits, and does
 // not interpret flags at other offsets as a mask.
 setup();put(props[4]+0x98,0xffffffffu);change=Change::bool_neighbor;C(calibrate(out)&&queries==1&&!out.transition);
 for(unsigned which=0;which<6;++which){setup();if(which==0)put(props[4]+0xd0,3u);if(which==1)put(props[4]+0x74,1u);if(which==2)put(props[1]+0x70,2u);if(which==3)put(props[1]+0x94,0xfffeu);if(which==4)put(props[0]+64,link_class);if(which==5)put(props[2]+76,1u);
  C(!sample(out)&&!queries&&n::last_reason==n::Reason::layout_rejected);}
 // Duplicate selected fields reject; unrelated numbered / long names pass.
 setup();props[5]=arena+0x68000;object(props[5],35,get<uintptr_t>(props[0]+80),pawn_class,L"CurrentLink");put(props[4]+0x68,props[5]);C(!sample(out)&&n::last_reason==n::Reason::layout_rejected);
 setup();props[5]=arena+0x68000;object(props[5],35,get<uintptr_t>(props[0]+80),pawn_class,L"CONTEXT_UNRELATED",481);put(props[4]+0x68,props[5]);C(calibrate(out));
 setup();put(props[2]+0x68,props[2]);C(!sample(out)&&n::last_reason==n::Reason::layout_rejected);
 // Field/source/cohort/identity changes during a getter cannot commit a cache.
 for(auto value:{Change::ca,Change::slot,Change::location,Change::world_generation,Change::header_capacity,Change::name}){setup();change=value;C(!calibrate(out)&&queries==1&&!n::calibration.ready);}
 setup();change=Change::header_growth;C(calibrate(out)&&queries==1);
 setup();C(calibrate(out));put(props[1]+0x94,0x209u);clock_ms=200;C(!sample(out)&&n::last_reason==n::Reason::layout_changed&&!n::calibration.ready);
 // Confirmed binding changes require a fresh 100ms neutral window and query.
 setup();C(calibrate(out));put(fixture_pawn+0x208,1);clock_ms=200;C(!sample(out));clock_ms=300;C(sample(out)&&queries==2&&out.slot==1);
 setup();C(calibrate(out));const auto mesh2=arena+0x3f000;object(mesh2,8,get<uintptr_t>(fixture_mesh+80),fixture_pawn,L"SyntheticMesh2");put(fixture_pawn+0x180,mesh2);clock_ms=200;C(!sample(out));clock_ms=300;C(sample(out)&&queries==2&&out.mesh.address==mesh2);
 setup();C(calibrate(out));const auto pawn2=arena+0x31000,new_mesh=arena+0x3f000;memcpy(reinterpret_cast<void*>(pawn2),reinterpret_cast<void*>(fixture_pawn),0x400);object(pawn2,8,pawn_class,fixture_scene.anchor.level,L"SyntheticPawn2");object(new_mesh,9,get<uintptr_t>(fixture_mesh+80),pawn2,L"SyntheticMesh2");put(pawn2+0x180,new_mesh);
 put(b::metadata.identities[b::metadata.role_controller].address+b::metadata.controller_pawn,pawn2);put(b::metadata.identities[b::metadata.role_camera].address+b::metadata.camera_view_target,pawn2);fixture_pawn=pawn2;fixture_mesh=new_mesh;refresh_scene();clock_ms=200;C(!sample(out));clock_ms=300;C(sample(out)&&queries==2&&out.pawn.address==pawn2);
 setup();C(calibrate(out));put(fixture_mesh+76,1u);clock_ms=200;C(!sample(out));clock_ms=300;C(sample(out)&&queries==2&&out.mesh.number==1);
 setup();C(calibrate(out));committed_world_generation=2;b::initial_world=2;clock_ms=200;C(!sample(out));clock_ms=300;C(sample(out)&&queries==2&&out.world_generation==2);
 setup();C(calibrate(out));put(fixture_pawn+0x200,uintptr_t(0));clock_ms=200;C(!sample(out)&&!n::calibration.ready);put(fixture_pawn+0x200,fixture_link);clock_ms=300;C(!sample(out));clock_ms=400;C(!sample(out)&&queries==1&&n::last_reason==n::Reason::query_wait);clock_ms=1100;C(sample(out)&&queries==2);
 setup();C(calibrate(out));const auto link2=arena+0x3f000;object(link2,8,link_class,fixture_scene.anchor.level,L"SyntheticLink2");put(link2+0x188,fixture_world);put(fixture_pawn+0x200,link2);clock_ms=200;C(!sample(out));clock_ms=300;C(sample(out)&&queries==2&&out.link.address==link2);
 setup();auto names=get<gears_anchor::Registry>(b::metadata.names_header);put(names.data+199*8,uintptr_t(0));head_entry=arena+0x20000+217*0x100;put(names.data+217*8,head_entry);put(head_entry,217u);std::array<wchar_t,10> head{L'b',L'_',L'M',L'F',L'_',L'H',L'e',L'a',L'd',0};put(head_entry+20,head);dynamic_index=9;C(calibrate(out)&&seen_name==217&&out.head_index==9);
 // Query retry budget survives failures; only a new binding receives eight
 // new attempts. Retries are at least 1s apart and never happen each frame.
 setup();query_ok=false;C(!sample(out));for(unsigned i=0;i<8;++i){clock_ms=100+uint64_t(i)*1000;if(i){clock_ms-=100;C(!sample(out));clock_ms+=100;}C(!sample(out)&&queries==i+1);}
 clock_ms=8100;C(!sample(out));clock_ms=8200;C(!sample(out)&&queries==8&&n::last_reason==n::Reason::attempt_limit);put(fixture_pawn+0x208,1);clock_ms=8300;C(!sample(out));clock_ms=8400;C(!sample(out)&&queries==9);
 setup();query_ok=false;C(!calibrate(out));clock_ms=101;C(!sample(out));clock_ms=201;C(!sample(out)&&queries==1&&n::last_reason==n::Reason::query_wait);
 setup();invalid_matrix=true;C(!calibrate(out)&&n::last_reason==n::Reason::invalid_matrix);setup();head_offset.z=201;C(!calibrate(out)&&n::last_reason==n::Reason::range);
 // Scene/thread/body failure never mutates caller output or calls native.
 setup();out.slot=999;fixture_scene.thread=GetCurrentThreadId()+1;C(!sample(out)&&out.slot==999&&!queries);setup();fixture_scene.anchor.pawn+=8;C(!sample(out)&&!queries);
 setup();entries_valid=false;C(!sample(out)&&!queries);setup();denied=fixture_mesh+8;C(!sample(out)&&!queries);
 setup();recording=false;C(!sample(out)&&!queries);recording=true;
 // Continuous u is not its diagnostic step bucket. A small real lean cannot
 // be captured as the next cover's neutral eye merely because bucket == 0.
 setup();b::enabled=true;b::target_source=b::TargetSource::live;b::live_state={};
 b::live_state.valid=true;b::live_state.target.pawn.address=fixture_pawn;b::live_state.target.mesh.address=fixture_mesh;b::live_state.target.u=.05f;
 C(!sample(out)&&n::last_reason==n::Reason::neutral_wait&&!queries);clock_ms=200;
 C(!sample(out)&&!queries);b::live_state.target.u=0;C(!sample(out));clock_ms=300;C(sample(out)&&queries==1);
 setup();b::enabled=true;b::target_source=b::TargetSource::live;b::live_state={};
 auto& owned=b::journals[0];owned.used=true;owned.pawn=fixture_pawn;owned.mesh=fixture_mesh;owned.owned[0]=true;owned.native_visible[0]=true;owned.target_u=.05f;
 C(!sample(out)&&n::last_reason==n::Reason::neutral_wait&&!queries);owned.owned[0]=false;C(!sample(out));clock_ms=100;C(sample(out)&&queries==1);
 b::target_source=b::TargetSource::advance;b::live_state={};b::enabled=false;
 unsigned overflow_value{};C(!n::source_field(overflow_value,UINTPTR_MAX-2,4));
 C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);printf("cover_neutral_eye_live: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
