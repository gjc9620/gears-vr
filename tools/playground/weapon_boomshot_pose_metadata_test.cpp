#pragma push_macro("main")
#undef main
#define main existing_asset_regressions
#include "weapon_lancer_asset_read_test.cpp"
#undef main
#pragma pop_macro("main")
#include <cmath>
#include <limits>
namespace {
using Name=gripdamage_metadata::RawName;
namespace playground {struct Vector{float x{},y{},z{};};struct Rotation{std::int32_t pitch{},yaw{},roll{};};}
namespace unified_weapon_fire {
namespace metadata=gripdamage_metadata;using U32=std::uint32_t;
#include "weapon_muzzle_getter_call.inl"
}
#include "weapon_boomshot_socket_metadata.inl"
#include "weapon_boomshot_pose_metadata.inl"
namespace pose=boomshot_pose;
struct PoseFixture:AssetMemory {
 uptr array_type{},struct_type{},integer_type{},script_type{},primitive{},matrix{},space{},refs{},parent{},world_field{},space_inner{},ref_inner{};
 pose::References records{};std::array<pose::Matrix,3> bones{};pose::Matrix component_matrix{};la::Evidence evidence{};
 PoseFixture(){
  array_type=node(L"ArrayProperty",class_type,core);struct_type=node(L"StructProperty",class_type,core);
  integer_type=node(L"IntProperty",class_type,core);script_type=node(L"ScriptStruct",class_type,core);
  const auto original=cls(L"Weap_Boomer_base",warfare,weapon_class),content=node(L"warfaregamecontentweapons",package_type);
  const auto boomer=cls(L"Weap_Boomer",content,original),package=node(L"Locust_Boomshot",package_type),group=node(L"Mesh",package_type,package);
  put(weapon,0x50,boomer);put(asset,0x48,text(L"Locust_Boomshot"));put(asset,0x40,group);
  primitive=cls(L"PrimitiveComponent",engine,actor_class);put(mesh_class,0x60,primitive);
  const auto object=cls(L"Object",core);matrix=node(L"Matrix",script_type,object);put(matrix,0x88,64u);
  refs=property(asset_class,L"RefSkeleton",array_type,0x160,16);ref_inner=node(L"RefSkeleton",integer_type,refs);
  put(ref_inner,0x70,1u);put(ref_inner,0x74,4u);put(refs,0xd0,ref_inner);
  space=property(mesh_class,L"SpaceBases",array_type,0x150,16);space_inner=node(L"SpaceBases",struct_type,space);
  put(space_inner,0x70,1u);put(space_inner,0x74,64u);put(space_inner,0xd0,matrix);put(space,0xd0,space_inner);
  parent=property(mesh_class,L"ParentAnimComponent",component_type,0x168,8);put(parent,0xd0,mesh_class);
  world_field=property(primitive,L"LocalToWorld",struct_type,0x180,64);put(world_field,0xd0,matrix);
  const wchar_t* bone_labels[]={L"Boomshot_Root",L"Boomshot_Magazine",L"Boomshot_Barrel"};
  for(unsigned i=0;i<3;++i){const auto name=text(bone_labels[i]);std::memcpy(records[i].data(),&name,sizeof name);
   bones[i]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};}
  bones[1][13]=30;bones[1][14]=2;bones[2][13]=56.06922149658203f;bones[2][14]=9.812527656555176f;
  component_matrix={0,.5,0,0,-.5,0,0,0,0,0,.5,0,100,200,300,1};
  put(asset,0x160,meta::Table{reinterpret_cast<uptr>(records.data()),3,3});
  put(component,0x150,meta::Table{reinterpret_cast<uptr>(bones.data()),3,3});put(component,0x180,component_matrix);
  frame.weapon=identity(weapon);tables();assert(la::read(frame,evidence,la::AssetScope::boomshot_projectile)==la::Status::ready);
 }
 bool inspect(pose::Source& out){meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader reader{snapshot,meta::Stage::final_check};pose::Reads reads{};
  pose::Source before{},after{};
  if(!meta::tables(base,snapshot.objects,snapshot.names)||!pose::metadata(frame,evidence,reader,reads,before))return false;
  reads.repeat=true;
  if(!pose::metadata(frame,evidence,reader,reads,after)||reads.cursor!=reads.count||reads.field_cursor!=reads.field_count||
   reads.name_cursor!=reads.name_count||!pose::same_reference(before,after)||before.world!=after.world)return false;
  out=before;return true;
 }
};
}
#ifndef BOOMSHOT_POSE_FIXTURE_ONLY
int main(){unsigned count{};
 for(unsigned bone=0;bone<3;++bone){auto m=std::make_unique<PoseFixture>();for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)m->bones[bone][row*4+col]=0;
  pose::Source out{};if(bone==1){assert(m->inspect(out)&&out.magazine_collapsed&&pose::collapsed_magazine(out.world[1])&&pose::affine(out.world[0])&&pose::affine(out.world[2]));}
  else assert(!m->inspect(out));++count;}
 for(unsigned bad=0;bad<3;++bad){auto m=std::make_unique<PoseFixture>();for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)m->bones[1][row*4+col]=0;
  if(bad==0)m->bones[1][0]=1e-8f;if(bad==1)m->bones[1][14]=std::numeric_limits<float>::quiet_NaN();if(bad==2)m->bones[1][15]=0;
  pose::Source out{};assert(!m->inspect(out));++count;}
 {auto m=std::make_unique<PoseFixture>();pose::Source out{};assert(m->inspect(out));
  assert(out.world[0][12]==100&&out.world[2][12]==float(100-.5*56.06922149658203));
  assert(out.world[2][13]==200&&out.world[2][14]==float(300+.5*9.812527656555176));
  assert(out.reference_bytes==m->records&&out.inner[0].element_bytes==4);++count;}
 {auto m=std::make_unique<PoseFixture>();pose::Source a{};assert(m->inspect(a));auto b=a;
  ++b.inner[0].identity;++b.inner[0].type;assert(pose::same_reference(a,b));++count;
  b=a;++b.inner_addresses[0];assert(!pose::same_reference(a,b));++count;
  b=a;++b.fields[2];assert(!pose::same_reference(a,b));++count;
  b=a;b.reference_bytes[1][60]=1;assert(!pose::same_reference(a,b));++count;
 }
 for(unsigned reason=0;reason<32;++reason){auto m=std::make_unique<PoseFixture>();
  switch(reason){
   case 0:Memory::put(m->component,0x168,m->mesh);break;
   case 1:Memory::put(m->parent,0x50,m->object_type);break;
   case 2:Memory::put(m->parent,0xd0,m->actor_class);break;
   case 3:Memory::put(m->primitive,0x40,m->warfare);break;
   case 4:Memory::put(m->component,0x150,meta::Table{reinterpret_cast<uptr>(m->bones.data()),2,3});break;
   case 5:Memory::put(m->component,0x150,meta::Table{reinterpret_cast<uptr>(m->bones.data()),3,2});break;
   case 6:Memory::put(m->component,0x150,meta::Table{reinterpret_cast<uptr>(m->bones.data()),3,1025});break;
   case 7:Memory::put(m->component,0x150,meta::Table{UINTPTR_MAX-10,3,3});break;
   case 8:Memory::put(m->asset,0x160,meta::Table{reinterpret_cast<uptr>(m->records.data()),4,4});break;
   case 9:Memory::put(m->ref_inner,0x74,68u);break; // Preserve original dummy declaration.
   case 10:Memory::put(m->space_inner,0x74,32u);break;
   case 11:Memory::put(m->space_inner,0x40,m->refs);break;
   case 12:Memory::put(m->matrix,0x88,12u);break;
   case 13:Memory::put(m->matrix,0x48,m->text(L"Vector"));break;
   case 14:Memory::put(m->matrix,0x40,m->engine);break;
   case 15:Memory::put(m->matrix,0x50,m->class_type);break;
   case 16:Memory::put(m->space_inner,0x94,8u);break;
   case 17:Memory::put(m->space_inner,0x70,2u);break;
   case 18:Memory::put(m->world_field,0x74,48u);break;
   case 19:Memory::put(reinterpret_cast<uptr>(m->records[1].data()),0,m->text(L"b_AR_Magazine"));break;
   case 20:{auto name=m->text(L"Boomshot_Root");name.number=1;Memory::put(reinterpret_cast<uptr>(m->records[0].data()),0,name);break;}
   case 21:Memory::put(m->component,0x150,meta::Table{reinterpret_cast<uptr>(m->records.data()),3,3});break;
   case 22:m->bones[2][13]=std::numeric_limits<float>::quiet_NaN();break;
   case 23:m->bones[2][0]=-1;break;
   case 24:m->bones[2][1]=.1f;break;
   case 25:m->bones[2][0]=2;break;
   case 26:m->bones[2][15]=0;break;
   case 27:fail_read=reinterpret_cast<uptr>(m->bones[2].data())+32;break;
   case 28:mutate_read=reinterpret_cast<uptr>(m->bones[2].data())+32;mutation=[&]{m->bones[0][12]=1;};break;
   case 29:mutate_read=reinterpret_cast<uptr>(m->bones[2].data())+32;mutation=[&]{m->records[1][60]=1;};break;
   case 30:mutate_read=reinterpret_cast<uptr>(m->bones[2].data())+32;mutation=[&]{Memory::put(m->parent,0x50,m->object_type);};break;
   case 31:mutate_read=reinterpret_cast<uptr>(m->bones[2].data())+32;mutation=[&]{Memory::put(m->world_field,0x48,m->text(L"OtherMatrix"));};break;
  }
  pose::Source out{};out.component.address=1234;assert(!m->inspect(out)&&out.component.address==1234);++count;
 }
 std::printf("Boomshot current pose metadata: %u cases passed; original opaque68 refs, typed Matrix64 SpaceBases, actual component product, null parent, closing repeats; owned memory only\n",count);
}
#endif
