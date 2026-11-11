#define GEARS_BODY_SETUP_TEST_MAIN PriorAssetBodyMain
#include "cover_body_setup_live_test.cpp"
#undef GEARS_BODY_SETUP_TEST_MAIN
#include "cover_asset_setup_live.inl"
static uintptr_t setup_cls{},setup_name_kind{},setup_bone_field{},setup_array_field{},setup_inner{},setup_list{};
static std::array<uintptr_t,64> setup_members{};
static void asset_fixture(unsigned kind,unsigned count=2){array_setup(kind);
 setup_cls=arena+0xe1000;setup_name_kind=arena+0xe1400;setup_list=arena+0xe1800;setup_inner=arena+0xe1c00;
 klass(setup_cls,schema_index++,L"RB_BodySetup",schema_owner);klass(setup_name_kind,schema_index++,L"NameProperty",0);
 const auto aggregate=make_field(schema_owner,L"AggGeom",structure_kind,72,0x60,array_aggregate);put(schema_owner+0x80,aggregate);
 setup_bone_field=make_field(setup_cls,L"BoneName",setup_name_kind,8,0xb4);put(setup_cls+0x80,setup_bone_field);
 setup_array_field=make_field(asset_class,L"BodySetup",array_kind,16,0x68);put(asset_class+0x80,setup_array_field);
 typed_property(setup_inner,schema_index++,setup_array_field,L"Inner",fixture_object_property,8,0,setup_cls);put(setup_array_field+0xd0,setup_inner);
 put(physics_asset+0x68,roles::pose::ArrayHeader{setup_list,int(count),int(count)});
 for(unsigned n=0;n<count;++n){setup_members[n]=arena+0xf0000+n*0x200;
  // Keep synthetic name entries outside the fixture's actor-memory region.
  object(setup_members[n],1200+n,setup_cls,physics_asset,L"CurrentSetup",n);put(setup_list+n*8,setup_members[n]);
  put(setup_members[n]+0xb4,gears_cover_authored_array::g::names::Name{int(10+n),n});}
}
int main(){setvbuf(stdout,nullptr,_IONBF,0);if(PriorAssetBodyMain())return 1;
 arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
 namespace asset=cover_asset_setup_live;namespace a=gears_cover_authored_array;
 auto member_proof=std::make_unique<h::Proof>();auto shape_proof=std::make_unique<h::Proof>();
 auto scratch=std::make_unique<a::Workspace>();auto output=std::make_unique<a::Snapshot>();
 for(unsigned kind=0;kind<3;++kind){printf("asset kind=%u begin\n",kind);asset_fixture(kind);roles::Snapshot typed{};C(observe(typed));
  asset::Snapshot inventory{};*member_proof={};C(asset::prepare(*member_proof,typed,inventory)&&inventory.count==2);
  cover_body_setup_live::Layout layout{};*shape_proof={};
  C(cover_body_setup_live::prepare(*shape_proof,inventory.rows[1].setup,inventory.rows[1].bone,kind,layout));
  const auto payload=arena+0xe3000;put(layout.aggregate_base+layout.array.field.offset,a::s::pose::ArrayHeader{payload,1,1});
  std::array<unsigned char,80> raw{};float tm[16]{};tm[0]=tm[5]=tm[10]=tm[15]=1;memcpy(raw.data(),tm,64);float dims[]{2,3,4};
  memcpy(raw.data()+64,dims,(kind==0?1:kind==1?3:2)*4);memcpy(reinterpret_cast<void*>(payload),raw.data(),80);
  a::Source source{};source.header=layout.aggregate_base+layout.array.field.offset;source.body_index=1;source.bone=inventory.rows[1].bone;
  source.kind=static_cast<a::g::metrics::Kind>(kind);source.reflected_extent=layout.array.element.extent;
  source.collision_mask=layout.array.element.collision_mask;source.receiver_scale={1,1,1};auto caller=[](a::Source const&){return true;};
  C(asset::capture(inventory,*member_proof,layout,*shape_proof,source,safe_copy,caller,*scratch,*output)==a::Code::captured);
  auto saved=std::make_unique<a::Snapshot>(*output);
  for(unsigned mutation=0;mutation<4;++mutation){bool fired=false;
   auto read=[&](void* dst,void const* at,size_t size){if(!fired&&reinterpret_cast<uintptr_t>(at)==payload){fired=true;
    if(mutation==0)put(setup_list,setup_members[1]);if(mutation==1)put(physics_asset+0x70,1);
    if(mutation==2)put(fixture_mesh+0x248,physics_instance);if(mutation==3)put(typed.array.data,typed.bodies[1].identity.address);}
    return safe_copy(dst,at,size);};
   C(asset::capture(inventory,*member_proof,layout,*shape_proof,source,read,caller,*scratch,*output)==a::Code::guard&&fired&&!memcmp(saved.get(),output.get(),sizeof *output));
   put(setup_list,setup_members[0]);put(physics_asset+0x70,2);put(fixture_mesh+0x248,physics_asset);put(typed.array.data,typed.bodies[0].identity.address);
  }
  source.body_index=0;C(asset::capture(inventory,*member_proof,layout,*shape_proof,source,safe_copy,caller,*scratch,*output)==a::Code::request);source.body_index=1;
  for(unsigned bad=0;bad<12;++bad){*member_proof={};inventory.count=123;
   if(bad==0)put(setup_list,setup_members[1]);if(bad==1)put(physics_asset+0x70,1);if(bad==2)put(setup_inner+0x74,4u);
   if(bad==3)put(setup_inner+0xd0,body_class);if(bad==4)put(setup_inner+0x40,physics_asset);if(bad==5)put(setup_inner+0x94,4u);
   if(bad==6)put(setup_bone_field+0x94,0xb8u);if(bad==7)put(fixture_mesh+0x248,physics_instance);
   if(bad==8)put(setup_array_field+0x94,0x78u);if(bad==9)put(setup_members[0]+0xb4,a::g::names::Name{});
   if(bad==10)put(setup_members[0]+0xb4,a::g::names::Name{-1,0});
   if(bad==11){const auto default_id=object(setup_members[0],1200,setup_cls,physics_asset,L"Default__Setup");
    *shape_proof={};cover_body_setup_live::Layout rejected{};rejected.aggregate_base=123;
    C(!cover_body_setup_live::prepare(*shape_proof,default_id,{10,0},kind,rejected)&&rejected.aggregate_base==123);}
   C(!asset::prepare(*member_proof,typed,inventory)&&inventory.count==123);
   put(setup_list,setup_members[0]);put(physics_asset+0x70,2);put(setup_inner+0x74,8u);put(setup_inner+0xd0,setup_cls);put(setup_inner+0x40,setup_array_field);
   put(setup_inner+0x94,0u);put(setup_bone_field+0x94,0xb4u);put(fixture_mesh+0x248,physics_asset);put(setup_array_field+0x94,0x68u);put(setup_members[0]+0xb4,a::g::names::Name{10,0});
   object(setup_members[0],1200,setup_cls,physics_asset,L"CurrentSetup");
  }
 }
 // Full64 authored members: no fixed Proof overflow or truncation on a common
 // class. This typed body list is constructed, not a live roles observation.
 asset_fixture(0,64);roles::Snapshot typed{};if(!observe(typed))return 98;typed.array.count=typed.array.capacity=64;typed.root=1;
 put(physics_instance+0x6c,typed.array);
 for(unsigned n=0;n<64;++n){typed.bodies[n].identity.address=arena+0x80000+n*0x200;typed.bodies[n].index=int(n);typed.bodies[n].owner=fixture_mesh;
  put(typed.array.data+n*8,typed.bodies[n].identity.address);}
 asset::Snapshot inventory{};*member_proof={};C(asset::prepare(*member_proof,typed,inventory)&&inventory.count==64);
 C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);printf("AssetSetup checks=%u failures=%u authored membership only; synthetic instances\n",checks,failures);return failures?1:0;
}
