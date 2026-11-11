#include "cover_authored_array.h"
#define GEARS_PHYSICS_ROLES_TEST_MAIN PriorPrimitiveSchemaMain
#include "cover_physics_roles_live_test.cpp"
#undef GEARS_PHYSICS_ROLES_TEST_MAIN
#include "cover_primitive_layout_live.inl"
static uintptr_t schema_owner{},schema_element{},schema_matrix{},schema_plane{},schema_float{},schema_bool{},schema_flag{},schema_last{};
static unsigned schema_index=180;
static uintptr_t schema_next{};
static uintptr_t make_field(uintptr_t owner,wchar_t const* name,uintptr_t kind,unsigned size,unsigned offset,uintptr_t target=0){
 const auto at=schema_next;schema_next+=0x200;typed_property(at,schema_index++,owner,name,kind,size,offset,target);return at;}
static void schema_setup(unsigned kind){physics_setup();schema_index=180;schema_next=arena+0xb0000;
 schema_owner=arena+0xa0000;schema_element=arena+0xa0400;schema_matrix=arena+0xa0800;schema_plane=arena+0xa0c00;schema_float=arena+0xa1000;schema_bool=arena+0xa1400;
 klass(schema_owner,schema_index++,L"KMeshProps",0);klass(schema_float,schema_index++,L"FloatProperty",0);klass(schema_bool,schema_index++,L"BoolProperty",0);
 const wchar_t* types[]={L"KSphereElem",L"KBoxElem",L"KSphylElem"};const unsigned sizes[]={72,80,76};
 object(schema_element,schema_index++,script_kind,schema_owner,types[kind]);put(schema_element+0x88,sizes[kind]);
 object(schema_matrix,schema_index++,script_kind,0,L"Matrix");put(schema_matrix+0x88,64u);object(schema_plane,schema_index++,script_kind,0,L"Plane");put(schema_plane+0x88,16u);
 const wchar_t* xyzw[]={L"X",L"Y",L"Z",L"W"};const wchar_t* planes[]={L"XPlane",L"YPlane",L"ZPlane",L"WPlane"};
 for(unsigned group=0;group<2;++group){uintptr_t previous=0;for(unsigned i=0;i<4;++i){const auto at=make_field(group?schema_matrix:schema_plane,group?planes[i]:xyzw[i],group?structure_kind:schema_float,group?16:4,i*(group?16:4),group?schema_plane:0);
  if(previous)put(previous+0x68,at);else put((group?schema_matrix:schema_plane)+0x80,at);previous=at;if(!group&&i==3)schema_last=at;}}
 const auto tm=make_field(schema_element,L"TM",structure_kind,64,0,schema_matrix);put(schema_element+0x80,tm);uintptr_t previous=tm;
 const wchar_t* dim[3][3]={{L"Radius",nullptr,nullptr},{L"X",L"Y",L"Z"},{L"Radius",L"Length",nullptr}};const unsigned counts[]={1,3,2};
 for(unsigned i=0;i<counts[kind];++i){const auto at=make_field(schema_element,dim[kind][i],schema_float,4,64+i*4);put(previous+0x68,at);previous=at;}
 schema_flag=make_field(schema_element,L"bNoRBCollision",schema_bool,4,64+counts[kind]*4);put(previous+0x68,schema_flag);put(schema_flag+0xd0,1u);
}
#ifndef GEARS_PRIMITIVE_LAYOUT_TEST_MAIN
#define GEARS_PRIMITIVE_LAYOUT_TEST_MAIN main
#endif
int GEARS_PRIMITIVE_LAYOUT_TEST_MAIN(){arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
 namespace layout=cover_primitive_layout_live;auto proof=std::make_unique<h::Proof>();layout::Layout out{};
 for(unsigned kind=0;kind<3;++kind){schema_setup(kind);gears_anchor::Identity element{},owner{};C(h::identity(schema_element,element)&&h::identity(schema_owner,owner));*proof={};
  C(layout::prepare(*proof,element,owner,kind,out)&&out.collision_mask==1&&out.plane.address==schema_plane);
  put(schema_last+0x94,8u);C(!h::current(*proof));put(schema_last+0x94,12u);
  namespace array=gears_cover_authored_array;
  const auto header_at=arena+0xc0000,payload=arena+0xc0100;
  put(header_at,array::s::pose::ArrayHeader{payload,1,1});
  std::array<unsigned char,80> bytes{};float matrix_values[16]{};matrix_values[0]=matrix_values[5]=matrix_values[10]=matrix_values[15]=1;
  memcpy(bytes.data(),matrix_values,64);const float dimensions[]{2,3,4};const unsigned count=kind==0?1:kind==1?3:2;
  memcpy(bytes.data()+64,dimensions,count*4);const unsigned disabled=1;memcpy(bytes.data()+64+count*4,&disabled,4);
  memcpy(reinterpret_cast<void*>(payload),bytes.data(),80);
  array::Source source{};source.header=header_at;source.bone={10,0};source.kind=static_cast<array::g::metrics::Kind>(kind);
  source.reflected_extent=out.extent;source.collision_mask=out.collision_mask;source.receiver_scale={1,1,1};const auto approved=source;
  auto storage=std::make_unique<array::Workspace>();auto captured=std::make_unique<array::Snapshot>();
  auto certified=[&](array::Source const& candidate){return array::same(candidate,approved)&&h::current(*proof);};
  C(array::capture(source,safe_copy,certified,*storage,*captured)==array::Code::captured&&captured->count==1&&captured->rows[0].collision_disabled);
  const auto preserved=std::make_unique<array::Snapshot>(*captured);bool fired=false;
  auto changing=[&](void* dst,void const* address,size_t size){if(!fired&&reinterpret_cast<uintptr_t>(address)==payload){fired=true;put(schema_last+0x94,8u);}return safe_copy(dst,address,size);};
  C(array::capture(source,changing,certified,*storage,*captured)==array::Code::guard&&fired&&!memcmp(preserved.get(),captured.get(),sizeof *captured));
  put(schema_last+0x94,12u);

  for(unsigned bad=0;bad<5;++bad){*proof={};out.collision_mask=123;
   if(bad==0)put(schema_matrix+0x88,80u);if(bad==1)put(schema_last+0x94,8u);if(bad==2)put(schema_flag+0xd0,3u);
   if(bad==3)put(schema_element+0x40,fixture_pawn);if(bad==4)put(schema_flag+0x68,schema_flag);
   C(!layout::prepare(*proof,element,owner,kind,out)&&out.collision_mask==123);
   put(schema_matrix+0x88,64u);put(schema_last+0x94,12u);put(schema_flag+0xd0,1u);put(schema_element+0x40,schema_owner);put(schema_flag+0x68,uintptr_t{});
  }
 }
 C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);printf("PrimitiveLayout checks=%u failures=%u actual reflection Proof; synthetic registry, no runtime authority\n",checks,failures);return failures?1:0;
}
