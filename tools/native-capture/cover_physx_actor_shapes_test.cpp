#define GEARS_SHAPE_OWNER_TEST_MAIN PriorActorShapesOwnerMain
#include "cover_physx_shape_owner_test.cpp"
#undef GEARS_SHAPE_OWNER_TEST_MAIN
#include "cover_physx_actor_shapes.h"
#include <memory>
namespace inventory=gears_cover_physx_actor_shapes;
int main(int argc,char** argv){
 if(argc!=2)return 2;auto memory=ownership_fixture(argv[1]);memory.block(0x160000,0x50);memory.block(internal_shape_at,0x170);
 memory.put<std::uintptr_t>(0x160048,internal_shape_at);memory.put<unsigned>(internal_shape_at+0x168,0);
 auto read=[&](void* d,void const* a,std::size_t n){return memory.read(d,a,n);};auto work=std::make_unique<inventory::Workspace>();inventory::Snapshot out{},sentinel{};sentinel.actor=0xdead;
 // The real DLL stays data; the owner/code admission is an explicit seam.
 check(inventory::observe(base,size,0x160000,0x170000,read,[]{return true;},*work,out)==inventory::Code::observed&&out.count==1&&out.body==internal_actor_at,"dynamic single member with reverse body preserved");
 memory.put<std::uintptr_t>(0x160040,0);memory.put<std::uintptr_t>(internal_shape_at+0xd0,0x160000);memory.put<std::uintptr_t>(internal_shape_at+0x148,0);
 check(inventory::observe(base,size,0x160000,0x170000,read,[]{return true;},*work,out)==inventory::Code::observed&&!out.body&&out.rows[0].public_shape==shape_at,"static single shape membership without fabricated dynamic body");
 memory.block(0x200000,0x1f0);memory.block(0x210000,16);memory.block(0x220000,16);
 memory.put<std::uintptr_t>(0x160048,0x200000);memory.put<std::uintptr_t>(0x200000,base+0x227950);memory.put<unsigned>(0x200168,9);
 for(unsigned n=0;n<3;++n){memory.put<std::uintptr_t>(0x2001b0+n*8,0x210000+(n?8:0));memory.put<std::uintptr_t>(0x2001d8+n*8,0x220000+(n?8:0));}
 memory.put<std::uintptr_t>(0x210000,internal_shape_at);memory.put<std::uintptr_t>(0x220000,shape_at);memory.put<std::uintptr_t>(internal_shape_at+8,0x200000);
 check(inventory::observe(base,size,0x160000,0x170000,read,[]{return true;},*work,out)==inventory::Code::observed&&out.type==9&&out.count==1&&!out.body,"static compound parallel member arrays");
 s::Snapshot actor_scope{};check(s::capture(base,size,source(),read,[]{return true;},actor_scope)==s::Code::current,"query source for static compound ownership");
 q::Hit static_hit{};memcpy(static_hit.bytes.data()+8,&shape_at,8);owner::Observation attributed{};
 auto admitted=[&](s::Snapshot const& value){return s::same(value,actor_scope);};
 check(owner::observe(base,size,actor_scope,static_hit,read,admitted,attributed)==owner::Code::observed&&!attributed.internal_actor,"production ownership bracket accepts certified static compound member");
 memory.put<unsigned char>(base+0x28a60,0x90);owner::Observation unchanged{};unchanged.actor=0xdead;attributed=unchanged;
 check(owner::observe(base,size,actor_scope,static_hit,read,admitted,attributed)==owner::Code::guard&&owner::same(attributed,unchanged),"new original count code certificate rejects mutation before publishing");
 memory.put<unsigned char>(base+0x28a60,0x48);
 const auto initial=memory;memory.calls=0;check(inventory::observe(base,size,0x160000,0x170000,read,[]{return true;},*work,out)==inventory::Code::observed,"count read budget");const auto reads=memory.calls;
 for(unsigned n=1;n<=reads;++n){memory=initial;memory.calls=0;memory.fail_at=n;out=sentinel;check(inventory::observe(base,size,0x160000,0x170000,read,[]{return true;},*work,out)!=inventory::Code::observed&&inventory::same(out,sentinel),"every inventory read failure preserves full output");}
 for(unsigned bad=0;bad<8;++bad){memory=initial;out=sentinel;
  if(bad==0)memory.put<std::uintptr_t>(0x2001e0,0x220010);
  if(bad==1)memory.put<std::uintptr_t>(0x220000,shape_at+8);
  if(bad==2)memory.put<std::uintptr_t>(internal_shape_at+8,0);
  if(bad==3)memory.put<std::uintptr_t>(internal_shape_at+0xd0,internal_actor_at);
  if(bad==4)memory.put<std::uintptr_t>(shape_at+0x38,other_actor_at);
  if(bad==5)memory.put<std::uintptr_t>(0x160038,0x180000);
  if(bad==6)memory.put<std::uintptr_t>(0x2001b8,0x210000+65*8);
  if(bad==7)memory.put<std::uintptr_t>(0x200000,base+0x226c80);
  check(inventory::observe(base,size,0x160000,0x170000,read,[]{return true;},*work,out)!=inventory::Code::observed&&inventory::same(out,sentinel),"mismatched arrays parent body actor Scene capacity or table reject");}
 memory=initial;out=sentinel;unsigned calls{};auto change=[&](){if(++calls==2)memory.put<std::uintptr_t>(0x160048,0);return true;};
 check(inventory::observe(base,size,0x160000,0x170000,read,change,*work,out)==inventory::Code::changed&&inventory::same(out,sentinel),"whole actor inventory closes over both reads");
 memory=initial;memory.put<std::uintptr_t>(0x160048,0);check(inventory::observe(base,size,0x160000,0x170000,read,[]{return true;},*work,out)==inventory::Code::observed&&!out.count&&!out.rows[0].public_shape,"empty representation clears old members");
 printf("CoverActorShapes checks=%u failed=%u original PE as data; code/owner guard is a seam, no query integration\n",checks,failed);return failed?1:0;
}
