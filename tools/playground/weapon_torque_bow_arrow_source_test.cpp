#include <cstdio>
FILE* log_file{};
#define GEARS_WEAPON_METADATA_PARSE_TEST_STATS 1
#define GEARS_PLAYGROUND_NATIVE_MAIN_MOUNT_PROVIDER_TRIAL 1
#include "../vr-common/weapon_main_mount_provider.h"
#include "../vr-common/weapon_torque_bow_arrow_scale.h"
#include "weapon_hand_offline_fixture.h"
namespace {
#include "weapon_lancer_asset_read.inl"
#include "weapon_torque_bow_arrow_source.inl"
#include "weapon_torque_bow_arrow_parent.inl"
#include "weapon_torque_bow_arrow_attributes.inl"
struct ArrowMemory:Memory {
 uptr component{},asset{},package{},scale_property{},vector_property{},vector_type{},light{},body_asset{},socket{},socket_scale{},socket_array{};
 std::array<uptr,2> socket_items{};
 ArrowMemory(){
  auto bow=cls(L"Weap_Bow_base",warfare,weapon_class);
  auto static_component=cls(L"StaticMeshComponent",engine),static_asset=cls(L"StaticMesh",engine);
  property(weapon_class,L"MagazineMesh",component_type,0x140,8);
  property(static_component,L"StaticMesh",object_type,0x100,8);
  component=node(L"MagazineMesh0",static_component,weapon);package=node(L"Locust_Torquebow",package_type);
  asset=node(L"Locust_Torquebow_Rocket",static_asset,package);
  const auto float_type=cls(L"FloatProperty",core),struct_type=cls(L"StructProperty",core),
      script_struct=cls(L"ScriptStruct",core),core_object=cls(L"Object",core);
  vector_type=node(L"Vector",script_struct,core_object);put(vector_type,0x88,12u);
  scale_property=property(static_component,L"Scale",float_type,0x108,4);
  vector_property=property(static_component,L"Scale3D",struct_type,0x10c,12);put(vector_property,0xd0,vector_type);
  property(static_component,L"AbsoluteScale",bool_type,0x118,4,1);
  property(mesh_class,L"LightEnvironment",component_type,0x130,8);
  light=node(L"ArrowLight",cls(L"LightEnvironmentComponent",engine),pawn);
  const auto name_type=cls(L"NameProperty",core),array_type=cls(L"ArrayProperty",core),
      body_type=cls(L"SkeletalMesh",engine),socket_type=cls(L"SkeletalMeshSocket",engine);
  property(pawn_class,L"bIsMirrored",bool_type,0x134,4,1);
  property(pawn_class,L"LeftHandSocketName",name_type,0x138,8);
  property(pawn_class,L"RightHandSocketName",name_type,0x140,8);
  property(mesh_class,L"SkeletalMesh",object_type,0x138,8);
  body_asset=node(L"BodyAsset",body_type,engine);socket=node(L"ArrowSocket",socket_type,body_asset);
  const auto socket_name=property(socket_type,L"SocketName",name_type,0x100,8);
  (void)socket_name;socket_scale=property(socket_type,L"RelativeScale",struct_type,0x108,12);put(socket_scale,0xd0,vector_type);
  socket_array=property(body_type,L"Sockets",array_type,0x100,16);
  const auto element=property(socket_array,L"Inner",object_type,0,8);put(socket_array,0xd0,element);
  const meta::RawName left=text(L"LeftHand"),right=text(L"RightHand");
  put(pawn,0x138,left);put(pawn,0x140,right);put(socket,0x100,left);
  socket_items[0]=socket;put(body_asset,0x100,meta::Table{reinterpret_cast<uptr>(socket_items.data()),1,2});
  put(socket,0x108,std::array<float,3>{{1,1,1}});put(mesh,0x138,body_asset);
  put(weapon,0x50,bow);put(weapon,0x140,component);put(component,0x100,asset);
  put(component,0x108,.8f);put(component,0x10c,std::array<float,3>{{1,1,1}});put(mesh,0x130,light);
  frame.weapon=identity(weapon);frame.key.thread_id=GetCurrentThreadId();tables();
 }
};
}

void fresh_closure_cases(){
 for(unsigned k=0;k<6;++k){auto m=std::make_unique<ArrowMemory>();bool changed{};torquebow_arrow_source::Binding b{};
  read_observer=[&](uptr,std::size_t){if(changed||torquebow_arrow_source::asset_gate!=5)return;changed=true;
   switch(k){
    case 0:Memory::put(m->weapon,0x140,uptr{});break;
    case 1:Memory::put(m->component,0x100,uptr{});break;
    case 2:Memory::put(m->asset,0x48,Memory::get<meta::RawName>(m->pawn,0x48));break;
    case 3:Memory::put(m->component,8,999u);break;
    case 4:Memory::put(m->component,0x50,Memory::get<uptr>(m->asset,0x50));break;
    case 5:Memory::put(Memory::get<uptr>(m->weapon_class,0x80),0x94,0x148u);break;
   }
  };
  const bool ok=torquebow_arrow_source::read(m->frame,b);read_observer={};assert(changed&&!ok);
 }
 for(unsigned k=0;k<10;++k){auto m=std::make_unique<ArrowMemory>();torquebow_arrow_source::Binding b{};
  assert(torquebow_arrow_source::read(m->frame,b));bool changed{};torquebow_arrow_source::Attributes a{};
  read_observer=[&](uptr,std::size_t){if(changed||torquebow_arrow_source::attributes_gate!=8)return;changed=true;
   switch(k){
    case 0:Memory::put(m->component,0x108,.9f);break;
    case 1:Memory::put(m->scale_property,0x94,0x120u);break;
    case 2:Memory::put(m->vector_type,0x88,16u);break;
    case 3:Memory::put(m->socket,0x108,std::array<float,3>{{2,2,2}});break;
    case 4:Memory::put(m->mesh,0x130,uptr{});break;
    case 5:Memory::put(m->component,0x118,1u);break;
    case 6:Memory::put(m->component,0x40,m->pawn);break;
    case 7:Memory::put(m->pawn,0x118,uptr{});break;
    case 8:m->socket_items[1]=m->socket;Memory::put(m->body_asset,0x100,meta::Table{reinterpret_cast<uptr>(m->socket_items.data()),2,2});break;
    case 9:Memory::put(m->component,0x50,Memory::get<uptr>(m->asset,0x50));break;
   }
  };
  const bool ok=torquebow_arrow_source::attributes(m->frame,b,a);read_observer={};assert(changed&&!ok);
 }
 {auto m=std::make_unique<ArrowMemory>();torquebow_arrow_source::Binding b{};torquebow_arrow_source::Attributes first{},second{};
  assert(torquebow_arrow_source::read(m->frame,b)&&torquebow_arrow_source::attributes(m->frame,b,first));
  Memory::put(m->component,0x108,.9f);assert(torquebow_arrow_source::attributes(m->frame,b,second));
  assert(first.scale.scalar!=second.scale.scalar&&std::abs(second.scale.scalar-.9)<1e-6);
 }
 std::puts("Six late source changes and ten late attribute/mesh/socket/layout changes refused; positive values remain fresh in a new query.");
}

int main(){
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
 fresh_closure_cases();
 auto memory=std::make_unique<ArrowMemory>();auto& m=*memory;torquebow_arrow_source::Binding source{};
 assert(torquebow_arrow_source::read(m.frame,source));assert(source.component.address==m.component&&source.asset.address==m.asset);
 torquebow_arrow_source::Attributes attributes{};double world_scale{};
 assert(torquebow_arrow_source::attributes(m.frame,source,attributes));
 {std::uint64_t source_reads{},attributes_reads{};
  read_observer=[&](uptr,std::size_t){++source_reads;};assert(torquebow_arrow_source::read(m.frame,source));
  read_observer=[&](uptr,std::size_t){++attributes_reads;};assert(torquebow_arrow_source::attributes(m.frame,source,attributes));read_observer={};
  std::printf("Actual Arrow source query=%llu reads; complete attributes query including source brackets=%llu reads; no game timing claim.\n",source_reads,attributes_reads);
 }

 assert(std::abs(attributes.scale.scalar-.8)<1e-6&&attributes.scale.xyz[0]==1&&!attributes.scale.absolute&&attributes.light.address==m.light);
 assert(attributes.socket.socket.address==m.socket&&attributes.socket.asset.address==m.body_asset);
 assert(gears_torque_bow_arrow_scale::world(attributes.scale,.3,attributes.socket.xyz,world_scale)&&std::abs(world_scale-.24)<1e-6);
 Memory::put(m.socket,0x108,std::array<float,3>{{.5f,.5f,.5f}});
 assert(torquebow_arrow_source::attributes(m.frame,source,attributes));
 assert(gears_torque_bow_arrow_scale::world(attributes.scale,.3,attributes.socket.xyz,world_scale)&&std::abs(world_scale-.12)<1e-6);
 Memory::put(m.component,0x118,1u);assert(torquebow_arrow_source::attributes(m.frame,source,attributes));
 assert(gears_torque_bow_arrow_scale::world(attributes.scale,.3,attributes.socket.xyz,world_scale)&&std::abs(world_scale-.8)<1e-6);
 Memory::put(m.component,0x118,0u);Memory::put(m.socket,0x108,std::array<float,3>{{1,1,1}});
 m.socket_items[1]=m.socket;Memory::put(m.body_asset,0x100,meta::Table{reinterpret_cast<uptr>(m.socket_items.data()),2,2});
 assert(!torquebow_arrow_source::attributes(m.frame,source,attributes));
 Memory::put(m.body_asset,0x100,meta::Table{reinterpret_cast<uptr>(m.socket_items.data()),1,2});
 Memory::put(m.pawn,0x134,1u);assert(!torquebow_arrow_source::attributes(m.frame,source,attributes));
 const auto right_socket=m.node(L"RightSocket",Memory::get<uptr>(m.socket,0x50),m.body_asset);
 Memory::put(right_socket,0x100,Memory::get<meta::RawName>(m.pawn,0x140));Memory::put(right_socket,0x108,std::array<float,3>{{2,2,2}});
 m.socket_items[1]=right_socket;Memory::put(m.body_asset,0x100,meta::Table{reinterpret_cast<uptr>(m.socket_items.data()),2,2});m.tables();
 assert(torquebow_arrow_source::attributes(m.frame,source,attributes)&&attributes.socket.socket.address==right_socket);
 assert(gears_torque_bow_arrow_scale::world(attributes.scale,.3,attributes.socket.xyz,world_scale)&&std::abs(world_scale-.48)<1e-6);
 Memory::put(m.pawn,0x134,0u);
 mutate_read=m.socket+0x108;mutation=[&]{Memory::put(m.socket,0x108,std::array<float,3>{{2,2,2}});};
 assert(!torquebow_arrow_source::attributes(m.frame,source,attributes));Memory::put(m.socket,0x108,std::array<float,3>{{1,1,1}});
 Memory::put(m.component,0x118,1u);assert(torquebow_arrow_source::attributes(m.frame,source,attributes)&&attributes.scale.absolute);
 assert(gears_torque_bow_arrow_scale::world(attributes.scale,.3,world_scale)&&std::abs(world_scale-.8)<1e-6);
 Memory::put(m.component,0x118,0u);Memory::put(m.mesh,0x130,uptr{});
 assert(torquebow_arrow_source::attributes(m.frame,source,attributes)&&!attributes.light.address);
 Memory::put(m.mesh,0x130,m.light);Memory::put(m.component,0x108,0.f);
 assert(!torquebow_arrow_source::attributes(m.frame,source,attributes));Memory::put(m.component,0x108,.8f);
 mutate_read=m.component+0x10c;mutation=[&]{Memory::put(m.vector_type,0x88,16u);};
 assert(!torquebow_arrow_source::attributes(m.frame,source,attributes));Memory::put(m.vector_type,0x88,12u);
 Memory::put(m.vector_property,0x74,16u);assert(!torquebow_arrow_source::attributes(m.frame,source,attributes));Memory::put(m.vector_property,0x74,12u);
 Memory::put(m.vector_type,0x48,m.text(L"Rotator"));m.tables();assert(!torquebow_arrow_source::attributes(m.frame,source,attributes));
 Memory::put(m.vector_type,0x48,m.text(L"Vector"));m.tables();
 Memory::put(m.mesh,0x130,m.asset);assert(!torquebow_arrow_source::attributes(m.frame,source,attributes));Memory::put(m.mesh,0x130,m.light);
 mutate_read=m.component+0x10c;mutation=[&]{Memory::put(m.component,0x108,.9f);};
 assert(!torquebow_arrow_source::attributes(m.frame,source,attributes));Memory::put(m.component,0x108,.8f);
 assert(torquebow_arrow_source::attributes(m.frame,source,attributes));
 attributes.scale.xyz[1]=1.2;assert(!gears_torque_bow_arrow_scale::world(attributes.scale,.3,world_scale)&&!world_scale);
 attributes.scale.xyz[1]=1;assert(!gears_torque_bow_arrow_scale::world(attributes.scale,0,world_scale));
 Memory::put(m.component,0x40,m.pawn);m.tables();assert(!torquebow_arrow_source::read(m.frame,source));
 Memory::put(m.component,0x40,m.weapon);Memory::put(m.package,0x48,m.text(L"OtherPackage"));m.tables();
 assert(!torquebow_arrow_source::read(m.frame,source));
 Memory::put(m.package,0x48,m.text(L"Locust_Torquebow"));Memory::put(m.asset,0x48,m.text(L"TorqueBow"));m.tables();
 assert(!torquebow_arrow_source::read(m.frame,source));
 Memory::put(m.asset,0x48,m.text(L"Locust_Torquebow_Rocket"));m.tables();
 mutate_read=m.component+0x100;mutation=[&]{Memory::put(m.weapon,0x140,uptr{});};
 assert(!torquebow_arrow_source::read(m.frame,source));
 std::puts("Torque Bow original arrow ownership/asset, actual typed local scale/light/socket, mirrored socket, duplicate rejection, absolute/avatar/socket scaling and read-race rejection passed; owned-memory fixture only, no runtime material/geometry/fit certification.");
}

