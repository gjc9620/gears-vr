#define GEARS_SHAPE_OWNER_TEST_MAIN PriorShapeParametersOwnerMain
#include "cover_physx_shape_owner_test.cpp"
#undef GEARS_SHAPE_OWNER_TEST_MAIN
#include "cover_physx_shape_parameters.h"
namespace parameters=gears_cover_physx_shape_parameters;
static Memory parameter_fixture(char const* path,owner::Kind kind){auto m=ownership_fixture(path);m.block(internal_shape_at,0x1c8);
 const unsigned index=kind==owner::Kind::box?0:kind==owner::Kind::capsule?1:5;
 const unsigned table=kind==owner::Kind::box?0x226c80:kind==owner::Kind::capsule?0x2272d0:0x2303e8;
 m.put<std::uintptr_t>(shape_at,base+owner::shape_tables[index]);m.put<std::uintptr_t>(internal_shape_at,base+table);
 for(unsigned n=0;n<4;++n)m.put<float>(internal_shape_at+0x78+n*4,n==3?1.f:0.f);
 for(unsigned n=0;n<3;++n){m.put<float>(internal_shape_at+0x88+n*4,float(n+2));m.put<float>(internal_shape_at+0x1b8+n*4,float(n+2));}
 m.put<unsigned>(internal_shape_at+0x180,0x13579u);m.put<float>(internal_shape_at+0x1b0,2.f);m.put<float>(internal_shape_at+0x1b4,3.f);return m;
}
static int parameter_cases(char const* path){
 try{auto work=std::make_unique<parameters::Workspace>();
 for(const auto kind:{owner::Kind::box,owner::Kind::capsule,owner::Kind::sphere}){auto m=parameter_fixture(path,kind);
  auto read=[&](void* p,void const* at,std::size_t n){return m.read(p,at,n);};
  s::Snapshot scope{};check(s::capture(base,size,source(),read,[]{return true;},scope)==s::Code::current,"current PE-backed scope");
  q::Hit hit{};memcpy(hit.bytes.data()+8,&shape_at,8);owner::Observation attribution{};
  auto source_guard=[&](s::Snapshot const& value){return s::same(scope,value);};
  check(owner::observe(base,size,scope,hit,read,source_guard,attribution)==owner::Code::observed,"original ownership reader for parameter kind");
  auto guard=[&](owner::Observation const& key){owner::Observation current{};
   return owner::observe(base,size,scope,hit,read,source_guard,current)==owner::Code::observed&&owner::same(current,key);};
  parameters::Parameters out{},sentinel{};sentinel.flags=0xdeadbeef;
  check(parameters::observe(base,size,attribution,read,guard,*work,out)==parameters::Code::observed,"typed current parameter capture");
  check(owner::same(out.owner,attribution)&&out.flags==0x13579u&&out.stored_position[0]==2&&out.stored_position[2]==4&&out.stored_quaternion[3]==1,"stored fields and ownership preserved");
  if(kind==owner::Kind::box)check((out.box_dimensions==std::array<float,3>{2,3,4})&&!out.radius&&!out.half_segment,"box internal original vector getter fields");
  else check(out.radius==2&&out.half_segment==(kind==owner::Kind::capsule?3:0),"capsule half segment versus sphere radius only");
  const auto initial=m;
  for(unsigned bad=0;bad<9;++bad){m=initial;out=sentinel;
   if(bad==0)m.put<unsigned char>(base+0xb1350,0x90);if(bad==1)m.put<std::uintptr_t>(base+(kind==owner::Kind::box?0x226c80:kind==owner::Kind::capsule?0x2272d0:0x2303e8)+0x70,base+0xb1350);
   if(bad==2)m.put<float>(internal_shape_at+(kind==owner::Kind::box?0x1b8:0x1b0),-1.f);
   if(bad==3)m.put<float>(internal_shape_at+0x88,std::numeric_limits<float>::quiet_NaN());
   if(bad==4)m.put<float>(internal_shape_at+0x84,std::numeric_limits<float>::infinity());
   if(bad==5)m.put<std::uintptr_t>(shape_at+0x38,other_actor_at);
   if(bad==6)m.put<unsigned char>(base+0x12770,0x90);
   if(bad==7)m.put<std::uintptr_t>(internal_shape_at,base+0x227950);
   if(bad==8)m.put<std::uintptr_t>(shape_at+0x30,internal_shape_at+8);
   check(parameters::observe(base,size,attribution,read,guard,*work,out)!=parameters::Code::observed&&parameters::same(out,sentinel)&&!work->active,"parameter/code/type/link failure retains old result");
  }
  m=initial;unsigned calls{};out=sentinel;
  auto changing=[&](owner::Observation const& key){if(++calls==2)m.put<float>(internal_shape_at+0x88,10.f);return guard(key);};
  check(parameters::observe(base,size,attribution,read,changing,*work,out)==parameters::Code::changed&&parameters::same(out,sentinel),"stored pose mutation between passes rejected");
  for(unsigned stop=1;stop<=3;++stop){m=initial;calls=0;out=sentinel;
   auto stopping=[&](owner::Observation const& key){return ++calls!=stop&&guard(key);};
   check(parameters::observe(base,size,attribution,read,stopping,*work,out)==parameters::Code::guard&&parameters::same(out,sentinel)&&!work->active,"every outer guard rejection keeps output");}
  m=initial;out=sentinel;bool entered=false;parameters::Code inner{};
  auto reentrant=[&](owner::Observation const& key){if(!entered){entered=true;inner=parameters::observe(base,size,key,read,guard,*work,out);}return guard(key);};
  check(parameters::observe(base,size,attribution,read,reentrant,*work,out)==parameters::Code::observed&&inner==parameters::Code::busy,"same workspace reentry rejected without disrupting outer read");
  m=initial;out=sentinel;auto original_owner=attribution;calls=0;
  auto changed_input=[&](owner::Observation const& key){if(++calls==3)attribution.actor+=8;return guard(key);};
  check(parameters::observe(base,size,attribution,read,changed_input,*work,out)==parameters::Code::guard&&parameters::same(out,sentinel),"final guard cannot change caller attribution");attribution=original_owner;
 }
 auto m=parameter_fixture(path,owner::Kind::capsule);auto read=[&](void* p,void const* at,std::size_t n){return m.read(p,at,n);};
 owner::Observation attribution{};check(owner::detail::sample(base,size,shape_at,0x170000,read,attribution)==owner::Code::observed,"fault-injection attribution");
 parameters::Parameters out{},sentinel{};sentinel.flags=0x12345678;
 auto admitted=[](owner::Observation const&){return true;};m.calls=0;
 check(parameters::observe(base,size,attribution,read,admitted,*work,out)==parameters::Code::observed,"parameter-only read budget");const auto reads=m.calls;
 const auto initial=m;
 for(unsigned n=1;n<=reads;++n){m=initial;m.calls=0;m.fail_at=n;out=sentinel;
  check(parameters::observe(base,size,attribution,read,admitted,*work,out)!=parameters::Code::observed&&parameters::same(out,sentinel)&&!work->active,"every failed parameter read leaves output and releases workspace");}
 m=initial;m.fail_at=0;links(m,other_actor_at,0);m.put<std::uintptr_t>(other_actor_at+0x40,0);
 check(owner::detail::sample(base,size,shape_at,0x170000,read,attribution)==owner::Code::observed&&!attribution.internal_actor,"static attribution without dynamic body");
 check(parameters::observe(base,size,attribution,read,admitted,*work,out)==parameters::Code::observed&&!out.owner.internal_actor&&out.stored_position[0]==2,"static stored pose is observed without guessing dynamic actor transform");
 check(parameters::observe(base,size,attribution,read,admitted,*work,work->first)==parameters::Code::alias,"scratch/output overlap rejected");
 out=sentinel;volatile bool inject_exception=true;
 auto throws=[&](owner::Observation const&)->bool{if(inject_exception)throw 1;return true;};
 check(parameters::observe(base,size,attribution,read,throws,*work,out)==parameters::Code::read&&parameters::same(out,sentinel)&&!work->active,"guard exception keeps output and releases workspace");
 m.put<std::uintptr_t>(shape_at,base+owner::shape_tables[2]);
 check(owner::detail::sample(base,size,shape_at,0x170000,read,attribution)==owner::Code::observed&&attribution.kind==owner::Kind::convex,"known unsupported kind attribution");
 out=sentinel;check(parameters::observe(base,size,attribution,read,admitted,*work,out)==parameters::Code::unsupported&&parameters::same(out,sentinel),"convex is not reinterpreted as a supported primitive");
 check(!GetModuleHandleW(L"PhysXCore.dll"),"original DLL never loaded");
 printf("parameter_reads=%u\n",reads);
 }catch(...){check(false,"parameter fixture exception");}
 printf("ShapeParameters checks=%u failed=%u original PE as data; stored fields only, no world/eligibility permit\n",checks,failed);return failed?1:0;
}
#ifndef GEARS_SHAPE_PARAMETERS_TEST_MAIN
#define GEARS_SHAPE_PARAMETERS_TEST_MAIN main
#endif
int GEARS_SHAPE_PARAMETERS_TEST_MAIN(int argc,char** argv){if(argc!=2)return 2;if(PriorShapeParametersOwnerMain(argc,argv))return 1;return parameter_cases(argv[1]);}
