#define GEARS_SHAPE_PARAMETERS_TEST_MAIN PriorShapeWorldParametersMain
#include "cover_physx_shape_parameters_test.cpp"
#undef GEARS_SHAPE_PARAMETERS_TEST_MAIN
#include "cover_physx_shape_world.h"
namespace world=gears_cover_physx_shape_world;
struct WorldVec {double x{},y{},z{};};struct WorldQuat {double x{},y{},z{},w{1};};
static WorldQuat world_unit(WorldQuat v){const auto n=std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z+v.w*v.w);return {v.x/n,v.y/n,v.z/n,v.w/n};}
static WorldVec world_rotate(WorldQuat q,WorldVec v){q=world_unit(q);const double dot=q.x*v.x+q.y*v.y+q.z*v.z,uu=q.x*q.x+q.y*q.y+q.z*q.z;
 return {2*dot*q.x+(q.w*q.w-uu)*v.x+2*q.w*(q.y*v.z-q.z*v.y),
  2*dot*q.y+(q.w*q.w-uu)*v.y+2*q.w*(q.z*v.x-q.x*v.z),
  2*dot*q.z+(q.w*q.w-uu)*v.z+2*q.w*(q.x*v.y-q.y*v.x)};}
static WorldVec world_add(WorldVec a,WorldVec b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
static void world_put(Memory& m,std::uintptr_t q_at,std::uintptr_t p_at,WorldQuat q,WorldVec p){q=world_unit(q);
 const float values[]={float(q.x),float(q.y),float(q.z),float(q.w),float(p.x),float(p.y),float(p.z)};
 for(unsigned n=0;n<4;++n)m.put<float>(q_at+n*4,values[n]);for(unsigned n=0;n<3;++n)m.put<float>(p_at+n*4,values[4+n]);}
static Memory world_fixture(char const* path,owner::Kind kind,bool dynamic){auto m=parameter_fixture(path,kind);m.block(internal_actor_at,0x218);
 m.put<std::uintptr_t>(internal_actor_at,base+0x226868);world_put(m,internal_actor_at+0x1fc,internal_actor_at+0x20c,{.2,-.3,.4,.8},{10,-20,30});
 world_put(m,internal_shape_at+0x78,internal_shape_at+0x88,{.4,.1,-.2,.9},{2,-3,4});
 if(!dynamic){links(m,other_actor_at,0);m.put<std::uintptr_t>(other_actor_at+0x40,0);}return m;
}
static bool world_close(WorldVec a,WorldVec b){return std::hypot(a.x-b.x,a.y-b.y,a.z-b.z)<3e-5;}
static int world_cases(char const* path){try{auto work=std::make_unique<world::Workspace>();world::Snapshot out{},sentinel{};sentinel.body_table=0xdeadbeef;
 for(bool dynamic:{false,true})for(const auto kind:{owner::Kind::box,owner::Kind::capsule,owner::Kind::sphere}){
  auto m=world_fixture(path,kind,dynamic);auto read=[&](void* p,void const* at,std::size_t n){return m.read(p,at,n);};
  const auto source_blocks=m.blocks;
  s::Snapshot scope{};check(s::capture(base,size,source(),read,[]{return true;},scope)==s::Code::current,"world fixture scope");
  q::Hit hit{};memcpy(hit.bytes.data()+8,&shape_at,8);owner::Observation attribution{};
  auto source_guard=[&](s::Snapshot const& value){return s::same(scope,value);};
  check(owner::observe(base,size,scope,hit,read,source_guard,attribution)==owner::Code::observed,"world typed shape ownership");
  auto guard=[&](owner::Observation const& key){owner::Observation current{};return owner::observe(base,size,scope,hit,read,source_guard,current)==owner::Code::observed&&owner::same(current,key);};
  check(world::observe(base,size,attribution,read,guard,*work,out)==world::Code::observed&&out.body_pose_present==dynamic,"original static/dynamic route observed");
  check(m.blocks==source_blocks,"world reader and ownership guards never change source bytes");
  const WorldQuat aq=world_unit({.2,-.3,.4,.8}),sq=world_unit({.4,.1,-.2,.9});
  const WorldVec probes[]={{0,0,0},{1,0,0},{0,1,0},{0,0,1},{2.25,-3.5,4.75}};
  for(auto v:probes){auto expected=world_add(world_rotate(sq,v),{2,-3,4});if(dynamic)expected=world_add(world_rotate(aq,expected),{10,-20,30});
   const auto* a=out.shape_world.values;const WorldVec actual{a[0]*v.x+a[1]*v.y+a[2]*v.z+a[9],a[3]*v.x+a[4]*v.y+a[5]*v.z+a[10],a[6]*v.x+a[7]*v.y+a[8]*v.z+a[11]};
   check(world_close(actual,expected),"independent asymmetric quaternion point oracle");}
  const auto initial=m;
  if(dynamic){for(unsigned bad=0;bad<8;++bad){m=initial;out=sentinel;
    if(bad==0)m.put<std::uintptr_t>(internal_actor_at,base+0x226a00);if(bad==1)m.put<std::uintptr_t>(base+0x226870,base+0x4e70);
    if(bad==2)m.put<float>(internal_actor_at+0x1fc,2.f);if(bad==3)m.put<float>(internal_actor_at+0x210,std::numeric_limits<float>::quiet_NaN());
    if(bad==4)m.put<unsigned char>(base+0x4e50,0x90);if(bad==5)m.put<unsigned char>(base+0x178ff0,0x90);
    if(bad==6)m.put<float>(base+0x2266c0,2.f);if(bad==7)m.put<std::uintptr_t>(base+0x2330a8+0x48,base+0x133ff0);
    check(world::observe(base,size,attribution,read,guard,*work,out)!=world::Code::observed&&world::same(out,sentinel)&&!work->active,"body type/code/slot/pose/constant fault preserves result");}
   m=initial;unsigned calls{};out=sentinel;auto changing=[&](owner::Observation const& key){if(++calls==2)m.put<float>(internal_actor_at+0x20c,100.f);return guard(key);};
   check(world::observe(base,size,attribution,read,changing,*work,out)==world::Code::changed&&world::same(out,sentinel),"body world pose change between complete passes rejected");
  }else{m=initial;bool unused_read=false;
   auto unused=[&](void* p,void const* ptr,std::size_t n){const auto at=reinterpret_cast<std::uintptr_t>(ptr);
    if((at>=other_actor_at+0x70&&at<other_actor_at+0xa0)||(at>=internal_actor_at&&at<internal_actor_at+0x218)){unused_read=true;return false;}return m.read(p,ptr,n);};
   check(world::observe(base,size,attribution,unused,guard,*work,out)==world::Code::observed&&!unused_read,"static shape uses stored global pose without consuming unused actor cache or dynamic Body");}
 }
 auto m=world_fixture(path,owner::Kind::capsule,true);auto read=[&](void* p,void const* at,std::size_t n){return m.read(p,at,n);};
 owner::Observation attribution{};check(owner::detail::sample(base,size,shape_at,0x170000,read,attribution)==owner::Code::observed,"world fault injection attribution");
 auto admitted=[](owner::Observation const&){return true;};m.calls=0;
 check(world::observe(base,size,attribution,read,admitted,*work,out)==world::Code::observed,"world read budget");const auto reads=m.calls;const auto initial=m;
 for(unsigned n=1;n<=reads;++n){m=initial;m.calls=0;m.fail_at=n;out=sentinel;
  check(world::observe(base,size,attribution,read,admitted,*work,out)!=world::Code::observed&&world::same(out,sentinel)&&!work->active,"every world read failure preserves output and releases work");}
 for(unsigned stop=1;stop<=3;++stop){m=initial;m.fail_at=0;unsigned calls{};out=sentinel;auto guard=[&](owner::Observation const&){return ++calls!=stop;};
  check(world::observe(base,size,attribution,read,guard,*work,out)==world::Code::guard&&world::same(out,sentinel),"every world guard boundary rejected atomically");}
 m=initial;m.fail_at=0;bool entered=false;world::Code nested{};
 auto reentrant=[&](owner::Observation const& key){if(!entered){entered=true;nested=world::observe(base,size,key,read,admitted,*work,out);}return true;};
 check(world::observe(base,size,attribution,read,reentrant,*work,out)==world::Code::observed&&nested==world::Code::busy,"world reader rejects same-work reentry");
 check(world::observe(base,size,attribution,read,admitted,*work,work->first)==world::Code::alias,"world scratch alias rejected");
 check(!GetModuleHandleW(L"PhysXCore.dll"),"world observation never loads original DLL");printf("world_reads=%u\n",reads);
 // Independent point oracle over varied normalized asymmetric quaternions.
 namespace math=gears_cover_physx_pose_math;
 for(unsigned i=0;i<128;++i){const double t=double(i+1)*.017;
  const auto aq=world_unit({std::sin(t),std::cos(t*.7),std::sin(t*.3),.8}),sq=world_unit({std::cos(t*.4),std::sin(t*.9),-.2,.7});
  const std::array<float,4> aqs{float(aq.x),float(aq.y),float(aq.z),float(aq.w)},sqs{float(sq.x),float(sq.y),float(sq.z),float(sq.w)};
  const std::array<float,3> ap{float(10*t),float(-5*t),float(3*t)},sp{2,-3,4};math::Matrix a{},shape{},joined{};
  check(math::quaternion(aqs,ap,a)&&math::quaternion(sqs,sp,shape)&&math::compose(a,shape,joined),"varied original-form matrix composition");
  const WorldVec probes[]={{0,0,0},{1,0,0},{0,1,0},{0,0,1},{2.25,-3.5,4.75}};
  for(auto v:probes){const auto expected=world_add(world_rotate(aq,world_add(world_rotate(sq,v),{2,-3,4})),{ap[0],ap[1],ap[2]});
   const auto* x=joined.values;const WorldVec actual{x[0]*v.x+x[1]*v.y+x[2]*v.z+x[9],x[3]*v.x+x[4]*v.y+x[5]*v.z+x[10],x[6]*v.x+x[7]*v.y+x[8]*v.z+x[11]};
   check(world_close(actual,expected),"varied world composition agrees with independent quaternion rotation");}
 }
 math::Matrix a{},shape{},rejected{};check(math::quaternion({0,0,0,1},{0,0,0},a)&&math::quaternion({0,0,0,1},{2,3,4},shape),"identity rejection baseline");
 memset(&rejected,0x5a,sizeof rejected);const auto before=rejected;
 check(!math::quaternion({0,0,0,2},{0,0,0},rejected)&&!memcmp(&before,&rejected,sizeof before),"nonunit quaternion rejected without normalization or output change");
 check(!math::quaternion({0,0,0,1},{std::numeric_limits<float>::denorm_min(),0,0},rejected)&&!memcmp(&before,&rejected,sizeof before),"subnormal source rejected without depending on FTZ");
 a.values[0]=-1;check(!math::compose(a,shape,rejected)&&!memcmp(&before,&rejected,sizeof before),"reflected matrix rejected without output change");
 }catch(...){check(false,"world fixture exception");}
 printf("ShapeWorld checks=%u failed=%u original PE as data; original-unit CPU pose only, no query fence\n",checks,failed);return failed?1:0;
}
#ifndef GEARS_SHAPE_WORLD_TEST_MAIN
#define GEARS_SHAPE_WORLD_TEST_MAIN main
#endif
int GEARS_SHAPE_WORLD_TEST_MAIN(int argc,char** argv){if(argc!=2)return 2;if(PriorShapeWorldParametersMain(argc,argv))return 1;return world_cases(argv[1]);}
