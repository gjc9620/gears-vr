#define GEARS_PRIMITIVE_LAYOUT_TEST_MAIN PriorPrimitiveLayoutMain
#include "cover_primitive_layout_live_test.cpp"
#undef GEARS_PRIMITIVE_LAYOUT_TEST_MAIN
#include "cover_array_layout_live.inl"
static uintptr_t array_aggregate{},array_field{},array_inner{},array_tail{};
static void array_setup(unsigned kind){schema_setup(kind);array_aggregate=arena+0xd0000;array_inner=arena+0xd0400;
 object(array_aggregate,schema_index++,script_kind,schema_owner,L"KAggregateGeom");put(array_aggregate+0x88,72u);
 const wchar_t* names[]={L"SphereElems",L"BoxElems",L"SphylElems",L"ConvexElems"};uintptr_t previous=0;
 for(unsigned i=0;i<4;++i){const auto at=make_field(array_aggregate,names[i],array_kind,16,16*i);
  if(previous)put(previous+0x68,at);else put(array_aggregate+0x80,at);previous=at;if(i==kind)array_field=at;}
 array_tail=previous;typed_property(array_inner,schema_index++,array_field,L"Inner",structure_kind,80,0,schema_element);put(array_field+0xd0,array_inner);
}
int main(){if(PriorPrimitiveLayoutMain())return 1;
 arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
 namespace layout=cover_array_layout_live;auto proof=std::make_unique<h::Proof>();layout::Layout out{};
 for(unsigned kind=0;kind<3;++kind){array_setup(kind);gears_anchor::Identity aggregate{},owner{};C(h::identity(array_aggregate,aggregate)&&h::identity(schema_owner,owner));*proof={};
  C(layout::prepare(*proof,aggregate,owner,kind,out)&&out.stride==80&&out.field.offset==16*kind&&out.element.kind==kind);
  put(array_inner+0xd0,schema_matrix);C(!h::current(*proof));put(array_inner+0xd0,schema_element);
  namespace array=gears_cover_authored_array;const auto aggregate_base=arena+0xe0000,payload=arena+0xe0100;
  put(aggregate_base+out.field.offset,array::s::pose::ArrayHeader{payload,1,1});
  std::array<unsigned char,80> raw{};float tm[16]{};tm[0]=tm[5]=tm[10]=tm[15]=1;memcpy(raw.data(),tm,64);
  const float dims[]{2,3,4};const unsigned count=kind==0?1:kind==1?3:2;memcpy(raw.data()+64,dims,count*4);memcpy(reinterpret_cast<void*>(payload),raw.data(),80);
  array::Source source{};source.header=aggregate_base+out.field.offset;source.bone={10,0};source.kind=static_cast<array::g::metrics::Kind>(kind);
  source.reflected_extent=out.element.extent;source.collision_mask=out.element.collision_mask;source.receiver_scale={1,1,1};
  auto storage=std::make_unique<array::Workspace>();auto captured=std::make_unique<array::Snapshot>();auto caller=[](array::Source const&){return true;};
  C(layout::capture(out,*proof,aggregate_base,source,safe_copy,caller,*storage,*captured)==array::Code::captured&&captured->count==1);
  const auto saved=std::make_unique<array::Snapshot>(*captured);bool fired=false;
  auto changing=[&](void* dst,void const* address,size_t size){if(!fired&&reinterpret_cast<uintptr_t>(address)==payload){fired=true;put(array_inner+0xd0,schema_matrix);}return safe_copy(dst,address,size);};
  C(layout::capture(out,*proof,aggregate_base,source,changing,caller,*storage,*captured)==array::Code::guard&&fired&&!memcmp(saved.get(),captured.get(),sizeof *captured));
  put(array_inner+0xd0,schema_element);source.header+=16;
  C(layout::capture(out,*proof,aggregate_base,source,safe_copy,caller,*storage,*captured)==array::Code::request&&!memcmp(saved.get(),captured.get(),sizeof *captured));

  for(unsigned bad=0;bad<6;++bad){*proof={};out.stride=123;
   if(bad==0)put(array_inner+0x74,72u);if(bad==1)put(array_inner+0xd0,schema_matrix);if(bad==2)put(array_inner+0x70,2u);
   if(bad==3)put(array_inner+0x40,fixture_pawn);if(bad==4)put(array_inner+0x94,4u);if(bad==5)put(array_tail+0x68,array_tail);
   C(!layout::prepare(*proof,aggregate,owner,kind,out)&&out.stride==123);
   put(array_inner+0x74,80u);put(array_inner+0xd0,schema_element);put(array_inner+0x70,1u);put(array_inner+0x40,array_field);put(array_inner+0x94,0u);put(array_tail+0x68,uintptr_t{});
  }
 }
 C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);printf("ArrayLayout checks=%u failures=%u reflection association only; synthetic registry\n",checks,failures);return failures?1:0;
}
