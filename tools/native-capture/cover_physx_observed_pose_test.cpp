#define GEARS_SHAPE_WORLD_TEST_MAIN PriorObservedPoseFixtureMain
#include "cover_physx_shape_world_test.cpp"
#undef GEARS_SHAPE_WORLD_TEST_MAIN
int main(int argc,char** argv){if(argc!=2)return 2;auto m=world_fixture(argv[1],owner::Kind::capsule,true);
 const std::array<float,4> body_q{.559741497f,-.394303977f,.607427657f,.402782083f},stored_q{0,.707106769f,.707106769f,0};
 const std::array<float,3> body_p{-78.8432846f,-3.34397388f,11.5933132f},stored_p{.0599999949f,-6.82121011e-15f,-2.13162816e-16f};
 for(unsigned n=0;n<4;++n){m.put(internal_actor_at+0x1fc+n*4,body_q[n]);m.put(internal_shape_at+0x78+n*4,stored_q[n]);}
 for(unsigned n=0;n<3;++n){m.put(internal_actor_at+0x20c+n*4,body_p[n]);m.put(internal_shape_at+0x88+n*4,stored_p[n]);}
 auto read=[&](void* p,void const* a,std::size_t n){return m.read(p,a,n);};owner::Observation attribution{};
 check(owner::detail::sample(base,size,shape_at,0x170000,read,attribution)==owner::Code::observed,"actual constructor layout ownership");
 auto work=std::make_unique<world::Workspace>();world::Snapshot out{};
 const auto code=world::observe(base,size,attribution,read,[](owner::Observation const&){return true;},*work,out);
 check(code==world::Code::observed,"recorded near-unit quaternion retained with original matrix without normalization");
 if(code==world::Code::observed){check(out.body_quaternion==body_q&&out.body_position==body_p&&out.parameters.stored_quaternion==stored_q,"recorded raw input unchanged");check(!world::math::rigid(out.actor_world),"raw original matrix not falsely labelled strict rigid");}
 check(!GetModuleHandleW(L"PhysXCore.dll"),"original not loaded");printf("ObservedPoseReplay checks=%u failed=%u saved game values; no original dispatch\n",checks,failed);return failed?1:0;
}
