#define GEARS_SHAPE_WORLD_TEST_MAIN PriorPoseSpanFixtureMain
#include "cover_physx_shape_world_test.cpp"
#undef GEARS_SHAPE_WORLD_TEST_MAIN
#include "cover_physx_shape_geometry.h"
int main(int argc,char** argv){if(argc!=2)return 2;auto m=world_fixture(argv[1],owner::Kind::capsule,true);
 const std::array<float,4> bq{.401522607f,.304909706f,-.0472320467f,.862281024f},sq{-.517145097f,-.482245654f,-.482245654f,.517145157f};
 const std::array<float,3> bp{-78.4930725f,-3.67751884f,12.5791245f},sp{.230697528f,-3.45637852e-9f,.0249756519f};
 for(unsigned n=0;n<4;++n){m.put(internal_actor_at+0x1fc+n*4,bq[n]);m.put(internal_shape_at+0x78+n*4,sq[n]);}
 for(unsigned n=0;n<3;++n){m.put(internal_actor_at+0x20c+n*4,bp[n]);m.put(internal_shape_at+0x88+n*4,sp[n]);}
 auto read=[&](void* p,void const* a,std::size_t n){return m.read(p,a,n);};owner::Observation attribution{};
 check(owner::detail::sample(base,size,shape_at,0x170000,read,attribution)==owner::Code::observed,"recorded actor11 ownership");auto work=std::make_unique<world::Workspace>();world::Snapshot out{};
 const auto code=world::observe(base,size,attribution,read,[](owner::Observation const&){return true;},*work,out);check(code==world::Code::observed,"recorded pose retained independently of strict quaternion norm cutoff");
 if(code==world::Code::observed){check(out.body_quaternion==bq&&out.parameters.stored_quaternion==sq&&out.shape_gram_error>0,"raw pose and explicit matrix error preserved");gears_cover_physx_shape_geometry::Geometry g{};check(gears_cover_physx_shape_geometry::convert(out,g)==gears_cover_physx_shape_geometry::Code::pose,"strict geometry remains closed to raw non-strict pose");}
 check(!GetModuleHandleW(L"PhysXCore.dll"),"original not loaded");printf("PoseReadContract checks=%u failed=%u saved actor11 values; no query\n",checks,failed);return failed?1:0;
}
