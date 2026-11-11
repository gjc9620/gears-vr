#define GEARS_ASSET_SETUP_TEST_MAIN PriorPointAssetMain
#include "cover_asset_setup_live_test.cpp"
#undef GEARS_ASSET_SETUP_TEST_MAIN
#include "cover_point_scale_live.inl"
static uintptr_t scale_vector{},scale_owner_property{},scale_component_property{},scale_actor_property{},scale_vector_tail{};
static void scale_fixture(unsigned kind){asset_fixture(kind);scale_vector=arena+0xe4000;
 object(scale_vector,schema_index++,script_kind,0,L"Vector");put(scale_vector+0x88,12u);
 const wchar_t* axes[]={L"X",L"Y",L"Z"};uintptr_t previous=0;
 for(unsigned n=0;n<3;++n){const auto field=make_field(scale_vector,axes[n],schema_float,4,n*4);
  if(previous)put(previous+0x68,field);else put(scale_vector+0x80,field);previous=field;}scale_vector_tail=previous;
 scale_owner_property=make_field(fixture_mesh_class,L"Owner",fixture_object_property,8,0x78,fixture_actor);put(props[1]+0x68,scale_owner_property);
 scale_component_property=make_field(fixture_mesh_class,L"Scale",schema_float,4,0x1e8);put(scale_owner_property+0x68,scale_component_property);
 const auto component_xyz=make_field(fixture_mesh_class,L"Scale3D",structure_kind,12,0x1ec,scale_vector);put(scale_component_property+0x68,component_xyz);
 scale_actor_property=make_field(fixture_actor,L"DrawScale",schema_float,4,0x198);put(instigator_property+0x68,scale_actor_property);
 const auto owner_xyz=make_field(fixture_actor,L"DrawScale3D",structure_kind,12,0x19c,scale_vector);put(scale_actor_property+0x68,owner_xyz);
 put(fixture_mesh+0x78,fixture_pawn);put(fixture_mesh+0x1e8,2.f);put(fixture_pawn+0x198,3.f);
 for(unsigned n=0;n<3;++n){put(fixture_mesh+0x1ec+n*4,.5f);put(fixture_pawn+0x19c+n*4,2.f);}
 // Complete the synthetic schema before taking a fresh binding. Adding Actor
 // members correctly invalidates the earlier physics_setup construction proof.
 h::base_proof={};h::instigator_proof={};C(h::sample(&scene,actor));
}
__declspec(noinline) static int point_scale_cases(){
 arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
 namespace scale=cover_point_scale_live;namespace a=gears_cover_authored_array;using Triple=std::array<float,3>;
 auto producer=std::make_unique<h::Proof>();auto members=std::make_unique<h::Proof>();auto shape=std::make_unique<h::Proof>();
 auto scratch=std::make_unique<a::Workspace>();auto output=std::make_unique<a::Snapshot>();
 for(unsigned kind=0;kind<3;++kind){scale_fixture(kind);roles::Snapshot typed{};if(!observe(typed)){
  printf("scale fixture roles code=%u step=%u pass=%u reason=%s stage=%s field=%ls\n",unsigned(role_verdict),unsigned(workspace.witness.step),workspace.witness.pass,h::reason_name(h::last_reason),h::stage,h::rejected_field);return 98;}
  cover_asset_setup_live::Snapshot inventory{};*members={};C(cover_asset_setup_live::prepare(*members,typed,inventory));
  cover_body_setup_live::Layout layout{};*shape={};C(cover_body_setup_live::prepare(*shape,inventory.rows[1].setup,inventory.rows[1].bone,kind,layout));
  scale::Snapshot measured{};*producer={};C(scale::prepare(*producer,typed.mesh,measured)&&measured.value==Triple({6,6,6}));
  const auto payload=arena+0xe3000;put(layout.aggregate_base+layout.array.field.offset,a::s::pose::ArrayHeader{payload,1,1});
  std::array<unsigned char,80> raw{};float tm[16]{};tm[0]=tm[5]=tm[10]=tm[15]=1;memcpy(raw.data(),tm,64);float dims[]{2,3,4};
  memcpy(raw.data()+64,dims,(kind==0?1:kind==1?3:2)*4);memcpy(reinterpret_cast<void*>(payload),raw.data(),80);
  a::Source source{};source.header=layout.aggregate_base+layout.array.field.offset;source.body_index=1;source.bone=inventory.rows[1].bone;
  source.kind=static_cast<a::g::metrics::Kind>(kind);source.reflected_extent=layout.array.element.extent;source.collision_mask=layout.array.element.collision_mask;
  source.receiver_scale=measured.value;auto caller=[](a::Source const&){return true;};
  C(scale::capture(measured,*producer,inventory,*members,layout,*shape,source,safe_copy,caller,*scratch,*output)==a::Code::captured&&output->rows[0].shape.authored.receiver_scale==measured.value);
  auto saved=std::make_unique<a::Snapshot>(*output);
  for(unsigned mutation=0;mutation<4;++mutation){bool fired=false;
   auto read=[&](void* dst,void const* at,size_t size){if(!fired&&reinterpret_cast<uintptr_t>(at)==payload){fired=true;
    if(mutation==0)put(fixture_mesh+0x1e8,4.f);if(mutation==1)put(fixture_pawn+0x19c,4.f);
    if(mutation==2)put(fixture_mesh+0x78,uintptr_t{});if(mutation==3)put(scale_vector_tail+0x94,4u);}return safe_copy(dst,at,size);};
   C(scale::capture(measured,*producer,inventory,*members,layout,*shape,source,read,caller,*scratch,*output)==a::Code::guard&&fired&&!memcmp(saved.get(),output.get(),sizeof *output));
   put(fixture_mesh+0x1e8,2.f);put(fixture_pawn+0x19c,2.f);put(fixture_mesh+0x78,fixture_pawn);put(scale_vector_tail+0x94,8u);
  }
  source.receiver_scale={1,1,1};C(scale::capture(measured,*producer,inventory,*members,layout,*shape,source,safe_copy,caller,*scratch,*output)==a::Code::request&&!memcmp(saved.get(),output.get(),sizeof *output));
  const auto component_name=get<unsigned>(scale_component_property+72);
  for(unsigned bad=0;bad<7;++bad){*producer={};measured.value={123,123,123};
   if(bad==0)put(scale_component_property+0x94,0x1e4u);if(bad==1)put(scale_owner_property+0x94,0x80u);
   if(bad==2)put(fixture_pawn+0x198,-1.f);if(bad==3)put(fixture_mesh+0x1e8,std::numeric_limits<float>::infinity());
   if(bad==4)put(scale_vector+0x88,16u);if(bad==5)put(fixture_mesh+0x1e8,0.f);
   if(bad==6)put(scale_component_property+72,get<unsigned>(scale_actor_property+72));
   C(!scale::prepare(*producer,typed.mesh,measured)&&measured.value==Triple({123,123,123}));
   put(scale_component_property+0x94,0x1e8u);put(scale_owner_property+0x94,0x78u);put(fixture_pawn+0x198,3.f);put(fixture_mesh+0x1e8,2.f);put(scale_vector+0x88,12u);
   put(scale_component_property+72,component_name);
  }
  // No Owner: neither its scalar nor vector is consumed.
  put(fixture_mesh+0x78,uintptr_t{});put(fixture_pawn+0x198,std::numeric_limits<float>::quiet_NaN());denied=fixture_pawn+0x198;
  *producer={};C(scale::prepare(*producer,typed.mesh,measured)&&!measured.input.owner_present&&measured.value==Triple({1,1,1}));denied=0;
  put(fixture_mesh+0x1ec,2.f);*producer={};C(scale::prepare(*producer,typed.mesh,measured)&&measured.value==Triple({4,1,1}));
  source.receiver_scale=measured.value;C(scale::capture(measured,*producer,inventory,*members,layout,*shape,source,safe_copy,caller,*scratch,*output)==a::Code::request&&!memcmp(saved.get(),output.get(),sizeof *output));
 }
 C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);printf("PointScale checks=%u failures=%u synthetic source; original route attestation remains external\n",checks,failures);return failures?1:0;
}
#ifndef GEARS_POINT_SCALE_TEST_MAIN
#define GEARS_POINT_SCALE_TEST_MAIN main
#endif
int GEARS_POINT_SCALE_TEST_MAIN(){if(PriorPointAssetMain())return 1;return point_scale_cases();}
