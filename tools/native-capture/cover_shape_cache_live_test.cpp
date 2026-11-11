#define GEARS_POINT_SCALE_TEST_MAIN PriorCachePointMain
#include "cover_point_scale_live_test.cpp"
#undef GEARS_POINT_SCALE_TEST_MAIN
#include "cover_shape_cache_live.inl"
__declspec(noinline) static int cache_live_cases(){
 arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
 namespace live=cover_shape_cache_live;namespace cache=gears_cover_physx_shape_cache;
 auto proof=std::make_unique<h::Proof>();auto work=std::make_unique<cache::Workspace>();cache::Snapshot output{};
 for(unsigned kind=0;kind<3;++kind){scale_fixture(kind);const auto setup=setup_members[1];gears_anchor::Identity id{};C(h::identity(setup,id));
  const wchar_t* names[]={L"CollisionGeom",L"CollisionGeomScale3D"};uintptr_t arrays[2]{},inners[2]{};
  for(unsigned n=0;n<2;++n){arrays[n]=make_field(setup_cls,names[n],array_kind,16,0xcc+16*n);
   inners[n]=make_field(arrays[n],names[n],structure_kind,n?12:8,0,n?scale_vector:pointer_type);put(arrays[n]+0xd0,inners[n]);}
  put(setup_bone_field+0x68,arrays[0]);put(arrays[0]+0x68,arrays[1]);
  const auto pointer_data=arena+0xe8000,scale_data=arena+0xe9004;
  put(setup+0xcc,cache::raw::ArrayHeader{pointer_data,2,2});put(setup+0xdc,cache::raw::ArrayHeader{scale_data,2,2});
  put(pointer_data,arena+0xeb000);put(pointer_data+8,arena+0xec000);put(scale_data,std::array<float,3>{1,2,3});put(scale_data+12,std::array<float,3>{2,3,4});
  const auto bone=get<live::names::Name>(setup+0xb4);live::Layout layout{};*proof={};
  C(live::prepare(*proof,id,bone,layout)&&layout.source.setup==setup);
  auto guard=[](cache::Source const&){return true;};C(live::observe(layout,*proof,safe_copy,guard,*work,output)==cache::Code::observed&&output.count==2);
  const auto saved=output;bool changed=false;
  auto read=[&](void* dst,void const* at,size_t size){if(!changed&&reinterpret_cast<uintptr_t>(at)==pointer_data){changed=true;put(inners[1]+0x74,16u);}return safe_copy(dst,at,size);};
  C(live::observe(layout,*proof,read,guard,*work,output)==cache::Code::guard&&changed&&cache::same(saved,output));put(inners[1]+0x74,12u);
  for(unsigned bad=0;bad<6;++bad){*proof={};layout.source.setup=123;
   if(bad==0)put(arrays[0]+0x94,0xc8u);if(bad==1)put(inners[0]+0x74,4u);if(bad==2)put(inners[1]+0x94,4u);
   if(bad==3)put(inners[1]+0xd0,pointer_type);if(bad==4)put(setup+0xb4,live::names::Name{999,0});if(bad==5)put(scale_vector+0x88,16u);
   C(!live::prepare(*proof,id,bone,layout)&&layout.source.setup==123);
   put(arrays[0]+0x94,0xccu);put(inners[0]+0x74,8u);put(inners[1]+0x94,0u);put(inners[1]+0xd0,scale_vector);put(setup+0xb4,bone);put(scale_vector+0x88,12u);
  }
 }
 C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);printf("ShapeCacheLive checks=%u failures=%u typed opaque cache only; no active shape join\n",checks,failures);return failures?1:0;
}
int main(){if(PriorCachePointMain())return 1;return cache_live_cases();}
