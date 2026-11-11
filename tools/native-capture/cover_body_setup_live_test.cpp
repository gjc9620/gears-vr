#define GEARS_ARRAY_LAYOUT_TEST_MAIN PriorBodyArrayMain
#include "cover_array_layout_live_test.cpp"
#undef GEARS_ARRAY_LAYOUT_TEST_MAIN
#include "cover_body_setup_live.inl"
#ifndef GEARS_BODY_SETUP_TEST_MAIN
#define GEARS_BODY_SETUP_TEST_MAIN main
#endif
int GEARS_BODY_SETUP_TEST_MAIN(){if(PriorBodyArrayMain())return 1;
 arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
 namespace setup=cover_body_setup_live;namespace a=gears_cover_authored_array;
 auto proof=std::make_unique<h::Proof>();auto work=std::make_unique<a::Workspace>();auto output=std::make_unique<a::Snapshot>();
 for(unsigned kind=0;kind<3;++kind){array_setup(kind);
  const auto cls=arena+0xe1000,instance=arena+0xe1400,name_type=arena+0xe1800;
  klass(cls,schema_index++,L"RB_BodySetup",schema_owner);klass(name_type,schema_index++,L"NameProperty",0);
  object(instance,schema_index++,cls,0,L"CurrentSetup");
  const auto agg=make_field(schema_owner,L"AggGeom",structure_kind,72,0x60,array_aggregate);put(schema_owner+0x80,agg);
  const auto bone=make_field(cls,L"BoneName",name_type,8,0xb4);put(cls+0x80,bone);
  a::g::names::Name expected{10,2};put(instance+0xb4,expected);
  gears_anchor::Identity id{};C(h::identity(instance,id));*proof={};setup::Layout layout{};
  C(setup::prepare(*proof,id,expected,kind,layout)&&layout.aggregate_base==instance+0x60);
  const auto payload=arena+0xe2000;put(layout.aggregate_base+layout.array.field.offset,a::s::pose::ArrayHeader{payload,1,1});
  std::array<unsigned char,80> raw{};float tm[16]{};tm[0]=tm[5]=tm[10]=tm[15]=1;memcpy(raw.data(),tm,64);
  float dims[]{2,3,4};memcpy(raw.data()+64,dims,(kind==0?1:kind==1?3:2)*4);memcpy(reinterpret_cast<void*>(payload),raw.data(),80);
  a::Source source{};source.header=layout.aggregate_base+layout.array.field.offset;source.bone=expected;source.kind=static_cast<a::g::metrics::Kind>(kind);
  source.reflected_extent=layout.array.element.extent;source.collision_mask=layout.array.element.collision_mask;source.receiver_scale={1,1,1};
  auto guard=[](a::Source const&){return true;};
  C(setup::capture(layout,*proof,source,safe_copy,guard,*work,*output)==a::Code::captured&&output->count==1);
  const auto saved=std::make_unique<a::Snapshot>(*output);bool changed=false;
  auto read=[&](void* dst,void const* at,size_t size){if(!changed&&reinterpret_cast<uintptr_t>(at)==payload){changed=true;put(instance+0xb4,a::g::names::Name{11,2});}return safe_copy(dst,at,size);};
  C(setup::capture(layout,*proof,source,read,guard,*work,*output)==a::Code::guard&&changed&&!memcmp(saved.get(),output.get(),sizeof *output));put(instance+0xb4,expected);
  ++source.bone.number;C(setup::capture(layout,*proof,source,safe_copy,guard,*work,*output)==a::Code::request&&!memcmp(saved.get(),output.get(),sizeof *output));--source.bone.number;
  for(unsigned bad=0;bad<6;++bad){*proof={};layout.aggregate_base=123;
   if(bad==0)put(bone+0x94,0xb8u);if(bad==1)put(agg+0xd0,schema_matrix);if(bad==2)put(instance+0xb4,a::g::names::Name{11,2});
   if(bad==3)put(cls+0x60,uintptr_t{});if(bad==4)put(agg+0x94,0x80u);if(bad==5)put(bone+0x68,bone);
   C(!setup::prepare(*proof,id,expected,kind,layout)&&layout.aggregate_base==123);
   put(bone+0x94,0xb4u);put(agg+0xd0,array_aggregate);put(instance+0xb4,expected);put(cls+0x60,schema_owner);put(agg+0x94,0x60u);put(bone+0x68,uintptr_t{});
  }
  // The aggregate position is reflected, not the fixture's previous 0x60.
  put(agg+0x94,0xc0u);*proof={};
  C(setup::prepare(*proof,id,expected,kind,layout)&&layout.aggregate_base==instance+0xc0);
  source.header=layout.aggregate_base+layout.array.field.offset;put(source.header,a::s::pose::ArrayHeader{payload,1,1});
  C(setup::capture(layout,*proof,source,safe_copy,guard,*work,*output)==a::Code::captured);
  for(unsigned mutation=0;mutation<2;++mutation){changed=false;
   auto mutate=[&](void* dst,void const* at,size_t size){if(!changed&&reinterpret_cast<uintptr_t>(at)==payload){changed=true;
     if(mutation)put(instance+76,1u);else put(agg+0xd0,schema_matrix);}return safe_copy(dst,at,size);};
   C(setup::capture(layout,*proof,source,mutate,guard,*work,*output)==a::Code::guard&&changed&&!memcmp(saved.get(),output.get(),sizeof *output));
   put(instance+76,0u);put(agg+0xd0,array_aggregate);
  }
  auto reject_owner=[](a::Source const&){return false;};
  C(setup::capture(layout,*proof,source,safe_copy,reject_owner,*work,*output)==a::Code::guard&&!memcmp(saved.get(),output.get(),sizeof *output));
 }
 C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);printf("BodySetup checks=%u failures=%u synthetic instances; no C0 permission\n",checks,failures);return failures?1:0;
}
