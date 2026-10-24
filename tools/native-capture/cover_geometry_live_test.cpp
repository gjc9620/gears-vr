// Standalone synthetic registry/reflection and changing-read tests. No process
// opening, game function, hook installation, camera, XR or game-memory operation.
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
#include <limits>
#include "anchor_observer.h"
#include "cover_surface_getters.h"
#include "../vr-common/pose_math.h"
#include "MinHook.h"
uintptr_t base=0x140000000;
std::atomic<uint64_t> committed_world_generation{1};std::atomic<unsigned> active{};
std::atomic<bool> recording{true};bool continuous_session=true,anchor_observer_enabled=true;
double stereo_units_per_metre=100;gears_anchor::Config anchor_config;
struct Scope {bool world()const{return true;}};
uintptr_t arena{},denied{};unsigned reads{};
void (*read_hook)(uintptr_t,size_t){};
bool safe_copy(void* out,void const* from,size_t size){
 const auto address=reinterpret_cast<uintptr_t>(from);++reads;if(read_hook)read_hook(address,size);
 if(!address||address==denied||address<arena||size>1024*1024||address>arena+1024*1024-size)return false;
 memcpy(out,from,size);return true;
}
long long now(){return static_cast<long long>(GetTickCount64());}
extern "C" MH_STATUS WINAPI MH_CreateHook(void*,void*,void**){return MH_ERROR_UNSUPPORTED_FUNCTION;}
#include "cover_body_live.inl"
#if GEARS_COVER_BODY_PROTOTYPE
bool entries_valid=true;
namespace cover_body_live {bool own_entries(){return entries_valid;}}
extern "C" void GearsCoverBlendTickHook(){}
extern "C" void GearsCoverChildPropagateHook(){}
extern "C" void GearsCoverRawInvoke(cover_body_live::CallPacket*,void*) noexcept(false){}
#endif
#include "cover_neutral_eye_live.inl"
#include "cover_geometry_live.inl"
namespace g=cover_geometry_live;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;printf("FAIL %u: %s [reason=%s stage=%s field=%ls]\n",__LINE__,#x,g::reason_name(g::last_reason),g::stage,g::rejected_field);}}while(0)
#if GEARS_COVER_BODY_PROTOTYPE
namespace b=cover_body_live;namespace n=cover_neutral_eye_live;
unsigned queries{},property_count{};uint64_t clock_ms{};
uintptr_t fixture_pawn{},fixture_mesh{},fixture_link{},fixture_world{},fixture_pc{},pawn_class{},link_class{},generic_class{},fixture_actor_class{};
uintptr_t class_class{},object_property{},int_property{},byte_property{},bool_property{},struct_property{},array_property{},float_property{},script_class{};
uintptr_t vector_type{},rotation_type{},slot_type{},fixture_world_class{},slots_property{},inner_property{},link_properties[5]{},actor_properties[3]{},slot_properties[7]{},vector_properties[3]{},rotation_properties[3]{},neutral_properties[5]{};
gears_anchor::Receipt scene;
template<class T>void put(uintptr_t address,T const& value){memcpy(reinterpret_cast<void*>(address),&value,sizeof value);}
template<class T>T get(uintptr_t address){T value{};memcpy(&value,reinterpret_cast<void const*>(address),sizeof value);return value;}
gears_anchor::Identity object(uintptr_t address,unsigned index,uintptr_t cls,uintptr_t outer,const wchar_t* name,unsigned number=0){
 gears_anchor::Identity id{};id.address=address;id.index=id.name_index=index;id.cls=cls;id.outer=outer;id.number=number;wcscpy_s(id.name.data(),id.name.size(),name);
 put(address+8,id.index);put(address+72,id.name_index);put(address+76,id.number);put(address+80,id.cls);put(address+64,id.outer);
 auto objects=get<gears_anchor::Registry>(b::metadata.objects_header),names=get<gears_anchor::Registry>(b::metadata.names_header);
 put(objects.data+uintptr_t(index)*8,address);const auto entry=arena+0x20000+uintptr_t(index)*0x100;put(names.data+uintptr_t(index)*8,entry);put(entry,index);put(entry+20,id.name);return id;
}
uintptr_t property(uintptr_t owner,const wchar_t* name,uintptr_t kind,unsigned width,unsigned offset,uintptr_t d0=0){
 const auto address=arena+0x88000+uintptr_t(property_count)*0x200;object(address,100+property_count++,kind,owner,name);
 put(address+0x70,1u);put(address+0x74,width);put(address+0x94,offset);put(address+0xd0,d0);return address;
}
template<size_t N>void chain(uintptr_t owner,uintptr_t const(&properties)[N]){
 put(owner+0x80,properties[0]);for(size_t i=0;i<N;++i)put(properties[i]+0x68,i+1<N?properties[i+1]:uintptr_t{});
}
void refresh_scene(){
 gears_anchor::PawnBinding binding{};C(gears_anchor::capture_pawn_binding(b::metadata,safe_copy,binding));scene={};scene.invocation=7;scene.thread=GetCurrentThreadId();scene.scene_qpc=1;scene.family_qpc=2;scene.renderer_qpc=3;
 scene.anchor.pawn=binding.pawn.address;scene.anchor.pawn_identity=binding.pawn;scene.anchor.controller=binding.controller;scene.anchor.camera=binding.camera;scene.anchor.level=binding.level;
 scene.anchor.location=get<g::Vector>(fixture_pawn+b::metadata.pawn_location);
}
void setup(){
 read_hook=nullptr;denied=0;memset(reinterpret_cast<void*>(arena),0,1024*1024);auto& c=b::metadata;c={};c.objects_header=arena+0x100;c.names_header=arena+0x200;
 c.object_internal_index=8;c.object_name=72;c.object_class=80;c.object_outer=64;c.field_super=96;c.name_entry_index=0;c.name_entry_text=20;c.identity_count=5;
 c.role_controller=0;c.role_local=1;c.role_camera=2;c.role_pawn=3;c.role_level=4;c.controller_pawn=c.pawn_controller=0x100;c.player_camera=c.pc_owner=0x108;c.player=0x110;c.camera_view_target=0x100;c.pawn_location=0x1a0;c.pawn_rotation=0x1c0;
 put(c.objects_header,gears_anchor::Registry{arena+0x1000,2001,2100});put(c.names_header,gears_anchor::Registry{arena+0x6000,2001,2100});
 class_class=arena+0x60000;generic_class=arena+0x60400;object(class_class,10,class_class,0,L"Class");object(generic_class,11,class_class,0,L"Object");
 pawn_class=arena+0x60800;object(pawn_class,12,class_class,0,L"WarPawn");const auto mesh_class=arena+0x60c00;object(mesh_class,13,class_class,0,L"SkeletalMeshComponent");link_class=arena+0x61000;object(link_class,14,class_class,0,L"CoverLink");
 const wchar_t* types[]={L"ObjectProperty",L"IntProperty",L"ByteProperty",L"BoolProperty",L"StructProperty",L"ArrayProperty",L"FloatProperty",L"ScriptStruct"};uintptr_t types_at[8]{};
 for(unsigned i=0;i<8;++i){types_at[i]=arena+0x61400+uintptr_t(i)*0x400;object(types_at[i],15+i,class_class,0,types[i]);}
 object_property=types_at[0];int_property=types_at[1];byte_property=types_at[2];bool_property=types_at[3];struct_property=types_at[4];array_property=types_at[5];float_property=types_at[6];script_class=types_at[7];
 fixture_actor_class=arena+0x63800;object(fixture_actor_class,24,class_class,0,L"Actor");for(auto at:{fixture_actor_class,pawn_class,link_class,generic_class})put(at+0x88,0x1000u);
 fixture_world_class=arena+0x63c00;object(fixture_world_class,25,class_class,0,L"WorldInfo");put(fixture_world_class+0x88,0x1000u);
 for(auto at:{pawn_class,link_class,generic_class})put(at+0x60,fixture_actor_class);
 const auto level=arena+0x38000;const uintptr_t addresses[]={arena+0x32000,arena+0x34000,arena+0x36000,arena+0x30000,level};
 const wchar_t* labels[]={L"SyntheticPC",L"SyntheticLocal",L"SyntheticCamera",L"SyntheticPawn",L"SyntheticLevel"};
 for(unsigned i=0;i<5;++i)c.identities[i]=object(addresses[i],i,i==3?pawn_class:generic_class,i==3?level:0,labels[i]);
 fixture_pc=addresses[0];fixture_pawn=addresses[3];fixture_world=arena+0x3a000;fixture_mesh=arena+0x3c000;fixture_link=arena+0x3e000;
 object(fixture_world,5,fixture_world_class,level,L"SyntheticWorld");object(fixture_mesh,6,mesh_class,fixture_pawn,L"SyntheticMesh");object(fixture_link,7,link_class,level,L"SyntheticLink");
 put(fixture_pc+c.controller_pawn,fixture_pawn);put(fixture_pc+c.player_camera,addresses[2]);put(fixture_pc+c.player,addresses[1]);put(fixture_pawn+c.pawn_controller,fixture_pc);put(addresses[2]+c.camera_view_target,fixture_pawn);put(addresses[2]+c.pc_owner,fixture_pc);
 b::layout={};b::layout.mesh=0x180;b::layout.world=0x188;b::layout.cover=0x190;b::layout.special=0x191;
 put(fixture_pawn+0x180,fixture_mesh);for(auto at:{fixture_pc,fixture_pawn,fixture_link})put(at+0x188,fixture_world);put(fixture_pawn+0x190,static_cast<unsigned char>(2));put(fixture_pawn+0x191,static_cast<unsigned char>(0));put(fixture_pawn+0x1a0,g::Vector{100,200,300});put(fixture_pawn+0x1c0,g::Rotator{100,200,300});
 const unsigned offsets[]={0x200,0x208,0x20c,0x20d,0x210};for(unsigned i=0;i<5;++i){neutral_properties[i]=arena+0x65000+uintptr_t(i)*0x400;const auto type=i==0?object_property:(i==1?int_property:(i==4?bool_property:byte_property));
  object(neutral_properties[i],30+i,type,pawn_class,n::specs[i].name);put(neutral_properties[i]+0x70,1u);put(neutral_properties[i]+0x74,n::specs[i].width);put(neutral_properties[i]+0x94,offsets[i]);}
 chain(pawn_class,neutral_properties);put(neutral_properties[0]+0xd0,link_class);put(neutral_properties[4]+0xd0,0x20u);put(fixture_pawn+0x200,fixture_link);put(fixture_pawn+0x208,1);put(fixture_pawn+0x210,0x801u);
 vector_type=arena+0x68000;rotation_type=arena+0x68400;slot_type=arena+0x68800;
 object(vector_type,40,script_class,0,L"Vector");object(rotation_type,41,script_class,0,L"Rotator");object(slot_type,42,script_class,0,L"CoverSlot");put(vector_type+0x88,12u);put(rotation_type+0x88,12u);put(slot_type+0x88,212u);
 property_count=0;
 for(unsigned i=0;i<3;++i){vector_properties[i]=property(vector_type,g::vector_specs[i].name,float_property,4,i*4);rotation_properties[i]=property(rotation_type,g::rotation_specs[i].name,int_property,4,i*4);}chain(vector_type,vector_properties);chain(rotation_type,rotation_properties);
 actor_properties[0]=property(fixture_actor_class,L"Location",struct_property,12,0x1a0,vector_type);actor_properties[1]=property(fixture_actor_class,L"Rotation",struct_property,12,0x1c0,rotation_type);actor_properties[2]=property(fixture_actor_class,L"WorldInfo",object_property,8,0x188,fixture_world_class);chain(fixture_actor_class,actor_properties);
 link_properties[0]=property(link_class,L"MidHeight",float_property,4,0x220);link_properties[1]=property(link_class,L"StandHeight",float_property,4,0x224);
 link_properties[2]=property(link_class,L"StandingLeanOffset",struct_property,12,0x228,vector_type);link_properties[3]=property(link_class,L"CrouchLeanOffset",struct_property,12,0x234,vector_type);link_properties[4]=slots_property=property(link_class,L"Slots",array_property,16,0x240);chain(link_class,link_properties);
 inner_property=property(slots_property,L"Slots",struct_property,212,0,slot_type);put(slots_property+0xd0,inner_property);
 for(unsigned i=0;i<4;++i)slot_properties[i]=property(slot_type,g::slot_specs[i].name,bool_property,4,196,uintptr_t(1)<<i);
 slot_properties[4]=property(slot_type,L"CoverType",byte_property,1,9);slot_properties[5]=property(slot_type,L"LocationOffset",struct_property,12,12,vector_type);slot_properties[6]=property(slot_type,L"RotationOffset",struct_property,12,24,rotation_type);chain(slot_type,slot_properties);
 put(fixture_link+0x1a0,g::Vector{10,20,30});put(fixture_link+0x1c0,g::Rotator{0,16384,0});put(fixture_link+0x220,83.25f);put(fixture_link+0x224,155.5f);put(fixture_link+0x228,g::Vector{1,2,3});put(fixture_link+0x234,g::Vector{4,5,6});
 const auto rows=arena+0xa0000;put(fixture_link+0x240,g::Array{rows,3,3});for(unsigned i=0;i<3;++i){const auto row=rows+uintptr_t(i)*212;put(row+196,0xfffffff5u);put(row+9,static_cast<unsigned char>(2));put(row+12,g::Vector{2,3,4});put(row+24,g::Rotator{0,-16384,0});}
 const auto names=get<gears_anchor::Registry>(c.names_header);const auto head_entry=arena+0x20000+199*0x100;put(names.data+199*8,head_entry);put(head_entry,199u);std::array<wchar_t,10> head{L'b',L'_',L'M',L'F',L'_',L'H',L'e',L'a',L'd',0};put(head_entry+20,head);
 b::layout_ready=true;b::game_thread=GetCurrentThreadId();b::initial_world=1;committed_world_generation=1;b::failed=false;b::external_change=false;b::stopping=false;b::transactions=0;b::injecting=false;b::step=0;b::journals={};entries_valid=true;recording=true;
 n::proof={};n::calibration={};g::proof={};clock_ms=queries=reads=0;anchor_config=c;refresh_scene();
}
struct Query {
 template<class Read,class Guard>bool operator()(uintptr_t module,uintptr_t component,uint64_t name,unsigned thread,Read,Guard guard,gears_cover_bones::Result& out)const{
  ++queries;C(module==base&&component==fixture_mesh&&name==199&&thread==GetCurrentThreadId());if(!guard())return false;
  const auto location=get<g::Vector>(fixture_pawn+0x1a0);out={};out.index=27;out.world.values[0]=out.world.values[5]=out.world.values[10]=out.world.values[15]=1;
  out.world.values[12]=location[0]+2;out.world.values[13]=location[1]+3;out.world.values[14]=location[2]+35;return guard();
 }
};
n::EyeSample eye(){n::EyeSample out{};C(!n::sample_with(&scene,out,[]{return clock_ms;},Query{}));clock_ms=100;C(n::sample_with(&scene,out,[]{return clock_ms;},Query{}));return out;}
std::array<unsigned char,1024*1024> original_arena{};
bool take(n::EyeSample const& e,g::Geometry& out,bool arena_unchanged=true){
 std::array<unsigned char,sizeof(b::Layout)> saved{};memcpy(saved.data(),&b::layout,sizeof b::layout);if(arena_unchanged)memcpy(original_arena.data(),reinterpret_cast<void*>(arena),original_arena.size());
 const auto count=queries;const bool result=g::sample_with(&scene,e,out);C(!memcmp(saved.data(),&b::layout,sizeof b::layout));C(queries==count);
 if(arena_unchanged)C(!memcmp(original_arena.data(),reinterpret_cast<void*>(arena),original_arena.size()));return result;
}
uintptr_t trigger{},change_address{};unsigned trigger_count{},trigger_nth{};uint64_t change_value{};size_t change_size{};bool throw_read{},change_generation{};n::EyeSample* change_eye{};
void changing_read(uintptr_t address,size_t){
 if(address!=trigger||++trigger_count!=trigger_nth)return;
 if(throw_read)throw 7;
 if(change_generation)committed_world_generation=2;
 else if(change_eye)change_eye->scene_invocation++;
 else memcpy(reinterpret_cast<void*>(change_address),&change_value,change_size);
}
void plan(uintptr_t watched,unsigned nth,uintptr_t target,uint64_t value,size_t size){
 trigger=watched;trigger_count=0;trigger_nth=nth;change_address=target;change_value=value;change_size=size;throw_read=change_generation=false;change_eye=nullptr;read_hook=changing_read;
}
bool close(double a,double b){return std::abs(a-b)<1e-8;}
unsigned native_queries{},native_guards{},native_mode{};n::EyeSample* native_eye{};
void native_setup(){
 setup();const auto pawn_location=get<g::Vector>(fixture_pawn+b::metadata.pawn_location),link_location=get<g::Vector>(fixture_link+0x1a0);
 const auto pawn_rotation=get<g::Rotator>(fixture_pawn+b::metadata.pawn_rotation),link_rotation=get<g::Rotator>(fixture_link+0x1c0);
 b::metadata.pawn_location=0x130;b::metadata.pawn_rotation=0x13c;put(actor_properties[0]+0x94,0x130u);put(actor_properties[1]+0x94,0x13cu);
 put(fixture_pawn+0x130,pawn_location);put(fixture_pawn+0x13c,pawn_rotation);put(fixture_link+0x130,link_location);put(fixture_link+0x13c,link_rotation);
 put(slots_property+0x94,0x340u);put(fixture_link+0x340,get<g::Array>(fixture_link+0x240));anchor_config=b::metadata;refresh_scene();
 native_queries=native_guards=native_mode=0;native_eye=nullptr;
}
struct NativeQuery {
 template<class Read,class Guard>
 bool operator()(uintptr_t module,uintptr_t link,int slot,unsigned thread,Read,Guard guard,gears_cover_surface::Result& result)const{
  ++native_queries;C(module==base&&link==fixture_link&&slot==1&&thread==GetCurrentThreadId());
  ++native_guards;if(!guard())return false;
  result={};result.location={700,800,900};result.rotation={{-123,456,789}};result.slot=slot;
  const auto array=get<g::Array>(fixture_link+0x340);result.slots={array.data,array.count,array.capacity};
  if(native_mode==1)return false;
  if(native_mode==2)result.location.x=std::numeric_limits<float>::quiet_NaN();
  if(native_mode==3)result.slot++;
  if(native_mode==4)result.slots.capacity++;
  if(native_mode==5)throw 7;
  if(native_mode==6)put(fixture_link+0x220,84.25f);
  if(native_mode==7)scene.invocation++;
  if(native_mode==8)committed_world_generation=2;
  if(native_mode==9)put(fixture_link+0x188,fixture_world+8);
  if(native_mode==10&&native_eye)native_eye->neutral_offset.z++;
  if(native_mode==11)put(slot_properties[5]+0x94,16u);
  if(native_mode==12)put(fixture_pawn+0x208,2);
  if(native_mode==13)put(array.data+212+12,g::Vector{3,3,4});
  if(native_mode==14)entries_valid=false;
  if(native_mode==15)scene.key.stamp++;
  if(native_mode==16){const auto previous=get<float>(fixture_link+0x220);put(fixture_link+0x220,previous+1);
   ++native_guards;C(!guard());put(fixture_link+0x220,previous);return true;}
  // Even a faulty injected Query that omits its own closing Guard cannot
  // bypass the production sample's independent final bracket.
  if(native_mode>=6)return true;
  ++native_guards;return guard();
 }
};
bool take_native(n::EyeSample const& e,g::Geometry& out,bool arena_unchanged=true){
 std::array<unsigned char,sizeof(b::Layout)> saved{};memcpy(saved.data(),&b::layout,sizeof b::layout);
 std::array<unsigned char,sizeof(g::Geometry)> output_before{};memcpy(output_before.data(),&out,sizeof out);
 if(arena_unchanged)memcpy(original_arena.data(),reinterpret_cast<void*>(arena),original_arena.size());const auto bone_queries=queries;
 const bool accepted=g::sample_native_with(&scene,e,out,NativeQuery{});
 if(!accepted)C(!memcmp(output_before.data(),&out,sizeof out));
 C(!memcmp(saved.data(),&b::layout,sizeof b::layout)&&queries==bone_queries);
 if(arena_unchanged)C(!memcmp(original_arena.data(),reinterpret_cast<void*>(arena),original_arena.size()));return accepted;
}
void native_tests(){
 native_setup();auto e=eye();g::Geometry out{};C(take_native(e,out));
 C(out.native_getter_verified&&out.native_slot_world.x==700&&out.native_slot_world.y==800&&out.native_slot_world.z==900&&out.native_slot_rotation==g::Rotator({-123,456,789}));
 C(out.candidate_only&&!out.collision_verified&&!out.atomic_snapshot_verified&&out.candidates.candidate_only&&!out.candidates.native_getter_verified&&!out.candidates.collision_verified);
 C(close(out.candidates.slot_world.x,7)&&close(out.candidates.slot_world.y,22)&&close(out.candidates.slot_world.z,34)&&close(out.candidates.mid_top_z,117.25));
 C(native_queries==1&&native_guards==2&&out.link_location==g::Vector({10,20,30})&&out.selected.index==1);
 C(take(e,out)&&!out.native_getter_verified&&out.native_slot_world.x==0&&out.native_slot_rotation==g::Rotator({0,0,0}));
 // The legacy synthetic offsets are a valid candidate, never a native ABI.
 setup();e=eye();C(take(e,out));out.selected.index=999;native_queries=0;C(!take_native(e,out)&&out.selected.index==999&&native_queries==0&&g::last_reason==g::Reason::native_layout);
 for(unsigned which=0;which<6;++which){
  native_setup();
  if(which==0){const auto value=get<g::Vector>(fixture_pawn+0x130);b::metadata.pawn_location=0x134;put(actor_properties[0]+0x94,0x134u);put(fixture_pawn+0x134,value);}
  if(which==1){const auto value=get<g::Rotator>(fixture_pawn+0x13c);b::metadata.pawn_rotation=0x140;put(actor_properties[1]+0x94,0x140u);put(fixture_pawn+0x140,value);}
  if(which==2){const auto value=get<g::Array>(fixture_link+0x340);put(slots_property+0x94,0x344u);put(fixture_link+0x344,value);}
  if(which==3)put(slot_properties[5]+0x94,16u);
  if(which==4)put(slot_properties[6]+0x94,28u);
  if(which==5)put(inner_property+0x74,208u);
  anchor_config=b::metadata;refresh_scene();e=eye();out.selected.index=999;C(!take_native(e,out)&&out.selected.index==999&&native_queries==0);
 }
 for(unsigned mode=1;mode<=16;++mode){
  native_setup();e=eye();native_mode=mode;native_eye=&e;out.selected.index=999;out.native_slot_world={111,222,333};out.native_getter_verified=false;
  C(!take_native(e,out,mode<6||mode==7||mode==8||mode==10||mode==14||mode==15||mode==16)&&out.selected.index==999&&out.native_slot_world.x==111&&!out.native_getter_verified);
  C(native_queries==1);if(mode==5){C(g::last_reason==g::Reason::exception);C(b::mutex.try_lock());b::mutex.unlock();}
  else C(g::last_reason==(mode>=6?g::Reason::changed:g::Reason::native_getter));
 }
 // Changes during the shared prefix are still rejected against the original
 // receipt, before the injected getter can claim any native values.
 native_setup();e=eye();C(take(e,out));plan(fixture_link+0x220,1,reinterpret_cast<uintptr_t>(&scene.key.stamp),scene.key.stamp+1,sizeof(scene.key.stamp));
 out.selected.index=999;C(!take_native(e,out)&&native_queries==0&&out.selected.index==999&&g::last_reason==g::Reason::changed);read_hook=nullptr;
 // Default adapter is wired but synthetic code memory has no current hashes;
 // it rejects before invoking any engine function.
 native_setup();e=eye();out.selected.index=999;C(!g::sample_native_with(&scene,e,out)&&out.selected.index==999&&g::last_reason==g::Reason::native_getter);
 C(b::mutex.try_lock());b::mutex.unlock();
}
int main(){
 arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,original_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
 setup();auto e=eye();g::Geometry out{};const auto first_reads=reads;C(take(e,out));const auto cold_reads=reads-first_reads;C(out.link_location==g::Vector({10,20,30})&&out.link_rotation==g::Rotator({0,16384,0}));
 C(out.pawn_location==g::Vector({100,200,300})&&out.pawn_rotation==g::Rotator({100,200,300}));C(out.standing_lean_offset==g::Vector({1,2,3})&&out.crouch_lean_offset==g::Vector({4,5,6}));
 C(out.mid_height==83.25f&&out.stand_height==155.5f&&out.slot_stride==212&&out.selected.index==1&&out.selected.cover_type==2);
 C(out.selected.permissions[0].value&&!out.selected.permissions[1].value&&out.selected.permissions[2].value&&!out.selected.permissions[3].value&&out.selected.permissions[3].mask==8);
 C(close(out.candidates.slot_world.x,7)&&close(out.candidates.slot_world.y,22)&&close(out.candidates.slot_world.z,34)&&close(out.candidates.forward.x,1)&&close(out.candidates.forward.y,0));
 C(close(out.candidates.mid_top_z,117.25)&&close(out.candidates.stand_top_z,189.5));C(out.candidate_only&&out.candidates.candidate_only&&!out.collision_verified&&!out.native_getter_verified&&!out.atomic_snapshot_verified&&!out.candidates.collision_verified&&!out.candidates.native_getter_verified);
 C(out.metadata_sources>50&&out.metadata_identities>20&&queries==1);const auto second_reads=reads;C(take(e,out));const auto warm_reads=reads-second_reads;
 printf("geometry proof: %u sources, %u identities; cold=%u/warm=%u synthetic reads\n",out.metadata_sources,out.metadata_identities,cold_reads,warm_reads);
 // A streaming Level may own the Link while the Pawn is in another Level.
 // Current typed WorldInfo, rather than equal Outer, binds both actors.
 setup();const auto streamed=arena+0x3f000;object(streamed,45,generic_class,0,L"SyntheticStreamingLevel");put(fixture_link+64,streamed);e=eye();C(e.link.outer!=e.pawn.outer&&take(e,out));
 // TLS only carries the current caller's receipt; nested scopes poison both.
 C(!g::sample(e,out)&&g::last_reason==g::Reason::no_scene);{g::SceneScope scope(&scene);C(g::sample(e,out));{g::SceneScope nested(&scene);C(!g::sample(e,out)&&g::last_reason==g::Reason::nested_scene);}C(!g::sample(e,out)&&g::last_reason==g::Reason::nested_scene);}C(!g::sample(e,out));
 for(unsigned which=0;which<5;++which){setup();e=eye();if(which==0)scene.invocation++;if(which==1)scene.thread++;if(which==2)scene.family_qpc=0;if(which==3)scene.renderer_qpc=1;if(which==4)b::game_thread=scene.thread+1;out.selected.index=999;C(!take(e,out)&&out.selected.index==999);}
 // Every copied Eye binding is tied to current neutral context/calibration.
 for(unsigned which=0;which<13;++which){setup();e=eye();if(which==0)e.link.address+=8;if(which==1)e.mesh.address+=8;if(which==2)e.pawn.number++;if(which==3)e.current_world+=8;if(which==4)e.world_generation++;if(which==5)e.slot++;if(which==6)e.native_ca++;if(which==7)e.transition=true;if(which==8)e.neutral_offset.z++;if(which==9)e.head_name++;if(which==10)e.calibrated_ms++;if(which==11)e.mutation_authorized=true;if(which==12)e.view_activation_authorized=true;
  out.selected.index=999;C(!take(e,out)&&out.selected.index==999&&g::last_reason==g::Reason::eye_binding);}
 // A native flag word's neighbors are raw data, not permission masks.
 setup();e=eye();put(slot_properties[3]+0x98,0xffffffffu);C(take(e,out)&&out.selected.permissions[3].mask==8&&!out.selected.permissions[3].value);
 // Struct/array/component type, dimensions, owner bounds and actual D0 mask.
 for(unsigned which=0;which<19;++which){setup();e=eye();
  if(which==0)put(actor_properties[0]+0x70,2u);if(which==1)put(actor_properties[0]+0x74,16u);if(which==2)put(actor_properties[0]+0x94,0xfffeu);
  if(which==3)put(actor_properties[0]+0xd0,rotation_type);if(which==4)put(vector_properties[1]+0x94,8u);if(which==5)put(vector_properties[0]+80,int_property);
  if(which==6)put(inner_property+0x74,208u);if(which==7)put(inner_property+0x94,4u);if(which==8)put(slot_type+0x88,208u);if(which==9)put(inner_property+64,link_class);
  if(which==10)put(slot_properties[0]+0xd0,0u);if(which==11)put(slot_properties[0]+0xd0,3u);if(which==12)put(slot_properties[5]+0xd0,rotation_type);
  if(which==13)put(slot_properties[6]+0x94,208u);if(which==14)put(link_properties[0]+80,int_property);if(which==15)put(slot_properties[4]+0x70,2u);
  if(which==16)put(link_properties[4]+0x68,link_properties[4]);if(which==17)put(actor_properties[0]+76,1u);if(which==18)put(link_class+0x88,0u);
  out.selected.index=999;C(!take(e,out)&&out.selected.index==999&&(g::last_reason==g::Reason::layout_rejected||g::last_reason==g::Reason::neutral_proof));}
 setup();e=eye();C(take(e,out));put(vector_properties[2]+0x94,4u);C(!take(e,out)&&g::last_reason==g::Reason::layout_changed);
 // An unknown extra field is harmless; a duplicate selected field is rejected.
 setup();const auto duplicate=property(link_class,L"MidHeight",float_property,4,0x250);put(link_properties[4]+0x68,duplicate);e=eye();C(!take(e,out)&&g::last_reason==g::Reason::layout_rejected);
 setup();const auto unrelated=property(link_class,L"Synthetic_Unrelated",float_property,4,0x250);put(link_properties[4]+0x68,unrelated);e=eye();C(take(e,out));
 for(unsigned which=0;which<10;++which){setup();e=eye();auto a=get<g::Array>(fixture_link+0x240);
  if(which==0)a.count=0;if(which==1)a.count=-1;if(which==2)a.count=257;if(which==3)a.capacity=2;if(which==4)a.capacity=257;if(which==5)a.data=0;if(which==6)a.data++;if(which==7)a.data=UINTPTR_MAX-15;if(which==8)a.count=1;if(which==9)a.data=arena+1024*1024-4;
  put(fixture_link+0x240,a);out.selected.index=999;C(!take(e,out)&&out.selected.index==999);}
 for(unsigned which=0;which<8;++which){setup();e=eye();const uintptr_t targets[]={fixture_link+0x1a0,fixture_link+0x220,fixture_link+0x224,fixture_link+0x228,fixture_link+0x234,get<g::Array>(fixture_link+0x240).data+212+12,fixture_pawn+0x1a0,fixture_link+0x1a0+8};
  put(targets[which],which&1?std::numeric_limits<float>::infinity():std::numeric_limits<float>::quiet_NaN());C(!take(e,out));}
 for(auto target:{fixture_pc,fixture_link}){setup();e=eye();put(target+0x188,fixture_world+8);C(!take(e,out)&&(g::last_reason==g::Reason::world||g::last_reason==g::Reason::eye_binding));}
 // Brackets reject selected values, fixture_world, metadata and Eye changes mid-read.
 for(unsigned which=0;which<7;++which){setup();e=eye();C(take(e,out));const auto selected=get<g::Array>(fixture_link+0x240).data+212;
  if(which==0)plan(fixture_link+0x220,2,fixture_link+0x220,0x40000000u,4);
  if(which==1)plan(fixture_link+0x220,2,selected+196,0x8u,4);
  if(which==2)plan(fixture_link+0x220,1,actor_properties[0]+0x94,0x1a4u,4);
  if(which==3){plan(fixture_link+0x220,1,0,0,0);change_generation=true;}
  if(which==4){plan(fixture_link+0x220,1,0,0,0);change_eye=&e;}
  if(which==5)plan(fixture_link+0x220,1,fixture_pawn+0x208,2u,4);
  if(which==6)plan(fixture_link+0x220,1,fixture_link+0x188,fixture_world+8,8);
  out.selected.index=999;C(!take(e,out,false)&&out.selected.index==999);read_hook=nullptr;}
 setup();e=eye();C(take(e,out));plan(fixture_link+0x220,1,0,0,0);throw_read=true;out.selected.index=999;C(!take(e,out)&&out.selected.index==999&&g::last_reason==g::Reason::exception);read_hook=nullptr;C(take(e,out));
 // An exception during metadata preparation also leaves all body bytes and
 // fixture memory unchanged. The caller-owned output is committed only once.
 setup();e=eye();plan(vector_properties[0]+0x70,1,0,0,0);throw_read=true;out.selected.index=999;C(!take(e,out)&&out.selected.index==999&&g::last_reason==g::Reason::exception);read_hook=nullptr;
 C(b::mutex.try_lock());b::mutex.unlock();C(!g::proof.ready);g::proof.last_attempt=0;g::proof.attempted=false;C(take(e,out));
 setup();e=eye();denied=fixture_link+0x224;C(!take(e,out)&&g::last_reason==g::Reason::read);denied=0;C(take(e,out));
 setup();e=eye();entries_valid=false;C(!take(e,out)&&g::last_reason==g::Reason::body_guard);
 unsigned overflow{};C(!g::source_field(overflow,UINTPTR_MAX-2,4));
 // The original final VirtualFree check remains below the added native suite.
 printf("geometry legacy candidate checks including final teardown: %u\n",checks+1);native_tests();
 C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);printf("cover_geometry_live: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
#else
int main(){g::Geometry out{};out.selected.index=999;g::EyeSample e{};C(!g::sample_with(nullptr,e,out)&&!g::sample_native_with(nullptr,e,out)&&g::last_reason==g::Reason::body_unavailable&&out.selected.index==999);C(!g::sample(e,out));printf("cover_geometry_live disabled: %u checks, %u failures\n",checks,failures);return failures?1:0;}
#endif
