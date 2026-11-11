#define GEARS_POINT_SCALE_TEST_MAIN PriorCreationPointMain
#include "cover_point_scale_live_test.cpp"
#undef GEARS_POINT_SCALE_TEST_MAIN
#include "cover_creation_scale_live.inl"
__declspec(noinline) static int creation_scale_cases(){
 arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
 namespace creation=cover_creation_scale_live;namespace a=gears_cover_authored_array;using Triple=std::array<float,3>;
 auto proof=std::make_unique<h::Proof>();auto members=std::make_unique<h::Proof>();auto shape=std::make_unique<h::Proof>();
 auto scratch=std::make_unique<a::Workspace>();auto output=std::make_unique<a::Snapshot>();
 for(unsigned kind=0;kind<3;++kind){scale_fixture(kind);roles::Snapshot typed{};C(observe(typed));
  creation::Snapshot value{},sentinel{};sentinel.scale.value={123,123,123};*proof={};
  C(creation::prepare(*proof,typed,value)&&value.scale.value==Triple({6,6,6})&&h::same(value.instance,typed.instance));
  cover_asset_setup_live::Snapshot inventory{};*members={};C(cover_asset_setup_live::prepare(*members,typed,inventory));
  cover_body_setup_live::Layout layout{};*shape={};C(cover_body_setup_live::prepare(*shape,inventory.rows[1].setup,inventory.rows[1].bone,kind,layout));
  const auto payload=arena+0xe3000;put(layout.aggregate_base+layout.array.field.offset,a::s::pose::ArrayHeader{payload,1,1});
  std::array<unsigned char,80> raw{};float tm[16]{};tm[0]=tm[5]=tm[10]=tm[15]=1;memcpy(raw.data(),tm,64);float dims[]{2,3,4};
  memcpy(raw.data()+64,dims,(kind==0?1:kind==1?3:2)*4);memcpy(reinterpret_cast<void*>(payload),raw.data(),80);
  a::Source source{};source.header=layout.aggregate_base+layout.array.field.offset;source.body_index=1;source.bone=inventory.rows[1].bone;
  source.kind=static_cast<a::g::metrics::Kind>(kind);source.reflected_extent=layout.array.element.extent;source.collision_mask=layout.array.element.collision_mask;
  source.receiver_scale=value.scale.value;auto caller=[](a::Source const&){return true;};
  C(creation::capture(value,*proof,inventory,*members,layout,*shape,source,safe_copy,caller,*scratch,*output)==a::Code::captured);
  auto saved=std::make_unique<a::Snapshot>(*output);bool fired=false;
  auto read=[&](void* dst,void const* at,size_t size){if(!fired&&reinterpret_cast<uintptr_t>(at)==payload){fired=true;put(typed.instance.address+0x60,uintptr_t{});}return safe_copy(dst,at,size);};
  C(creation::capture(value,*proof,inventory,*members,layout,*shape,source,read,caller,*scratch,*output)==a::Code::guard&&fired&&!memcmp(saved.get(),output.get(),sizeof *output));
  put(typed.instance.address+0x60,typed.owner.address);
  auto mismatched=inventory;mismatched.instance=inventory.asset;
  C(creation::capture(value,*proof,mismatched,*members,layout,*shape,source,safe_copy,caller,*scratch,*output)==a::Code::request&&!memcmp(saved.get(),output.get(),sizeof *output));
  C(h::current(*proof));const auto owner_address=typed.instance.address+0x60;
  put(owner_address,uintptr_t{});C(!h::current(*proof));*proof={};value=sentinel;
  C(!creation::prepare(*proof,typed,value)&&value.scale.value==sentinel.scale.value);put(owner_address,typed.owner.address);
  put(typed.mesh.address+0x250,uintptr_t{});*proof={};C(!creation::prepare(*proof,typed,value)&&value.scale.value==sentinel.scale.value);
  put(typed.mesh.address+0x250,typed.instance.address);
  auto wrong=typed;wrong.owner={};*proof={};C(!creation::prepare(*proof,wrong,value)&&value.scale.value==sentinel.scale.value);
  wrong=typed;wrong.instance=typed.asset;*proof={};C(!creation::prepare(*proof,wrong,value));
  // Nonuniform creation input is observed, not silently changed to uniform.
  put(typed.mesh.address+0x1ec,1.f);*proof={};C(creation::prepare(*proof,typed,value)&&value.scale.value==Triple({12,6,6}));
  put(typed.owner.address+0x198,4.f);C(!h::current(*proof));*proof={};C(creation::prepare(*proof,typed,value)&&value.scale.value==Triple({16,8,8}));
 }
 C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);printf("CreationScale checks=%u failures=%u current reflected inputs only; historical cache correspondence unproven\n",checks,failures);return failures?1:0;
}
int main(){if(PriorCreationPointMain())return 1;return creation_scale_cases();}
