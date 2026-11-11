#define GEARS_SHAPE_OWNER_TEST_MAIN PriorOwnerFieldsMain
#include "cover_physx_shape_owner_test.cpp"
#undef GEARS_SHAPE_OWNER_TEST_MAIN
int main(int argc,char** argv){if(argc!=2)return 2;
 auto m=ownership_fixture(argv[1]);auto read=[&](void* p,void const* a,std::size_t n){return m.read(p,a,n);};
 namespace inv=gears_cover_physx_actor_shapes;auto work=std::make_unique<inv::Workspace>();inv::Snapshot group{};owner::Observation out{};
 for(unsigned mode=0;mode<2;++mode){
  // Independent original constructor layout: Shape+d0=public Actor,
  // Shape+148=optional Body. Neither value is inferred from this reader.
  const std::uintptr_t body=mode?0:internal_actor_at;
  m.put<std::uintptr_t>(0x160040,body);m.put<std::uintptr_t>(internal_shape_at+0xd0,0x160000);m.put<std::uintptr_t>(internal_shape_at+0x148,body);
  check(owner::detail::sample(base,size,shape_at,0x170000,read,out)==owner::Code::observed&&out.actor==0x160000&&out.internal_actor==body,"constructor owner fields qualify dynamic/static shape");
  check(inv::observe(base,size,0x160000,0x170000,read,[]{return true;},*work,group)==inv::Code::observed&&group.count==1&&group.body==body,"constructor owner fields qualify full actor inventory");
 }
 m.put<std::uintptr_t>(0x160040,internal_actor_at);m.put<std::uintptr_t>(internal_shape_at+0x148,internal_actor_at);m.put<std::uintptr_t>(internal_shape_at+0xd0,internal_actor_at);
 check(owner::detail::sample(base,size,shape_at,0x170000,read,out)==owner::Code::links,"Body cannot replace public Actor owner");
 check(inv::observe(base,size,0x160000,0x170000,read,[]{return true;},*work,group)==inv::Code::links,"inventory rejects wrong public owner");
 m.put<std::uintptr_t>(internal_shape_at+0xd0,0x160000);m.put<std::uintptr_t>(internal_shape_at+0x148,0);
 check(owner::detail::sample(base,size,shape_at,0x170000,read,out)==owner::Code::links,"missing dynamic Body rejected");
 check(inv::observe(base,size,0x160000,0x170000,read,[]{return true;},*work,group)==inv::Code::links,"inventory rejects missing dynamic Body");
 check(!GetModuleHandleW(L"PhysXCore.dll"),"original module not loaded");
 printf("OwnerFields checks=%u failed=%u original constructor fields only; no game/query\n",checks,failed);return failed?1:0;
}
