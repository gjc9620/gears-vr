#define GEARS_SHAPE_WORLD_TEST_MAIN PriorAssetShapesWorldMain
#include "cover_physx_shape_world_test.cpp"
#undef GEARS_SHAPE_WORLD_TEST_MAIN
#include "cover_physx_asset_shapes.h"
namespace batch=gears_cover_physx_asset_shapes;
constexpr std::uintptr_t batch_root=0x220000,batch_internals=0x230000,batch_publics=0x240000;
static void batch_shape(Memory& m,std::uintptr_t shape,std::uintptr_t internal,owner::Kind kind,
    std::uintptr_t actor,std::uintptr_t body,std::uintptr_t parent){
 m.block(shape,0x40);m.block(internal,0x1c8);
 const unsigned public_index=kind==owner::Kind::box?0:kind==owner::Kind::capsule?1:5;
 const unsigned table=kind==owner::Kind::box?0x226c80:kind==owner::Kind::capsule?0x2272d0:0x2303e8;
 m.put<std::uintptr_t>(shape,base+owner::shape_tables[public_index]);m.put<std::uintptr_t>(shape+0x30,internal);m.put<std::uintptr_t>(shape+0x38,actor);
 m.put<std::uintptr_t>(internal,base+table);m.put<std::uintptr_t>(internal+8,parent);m.put<std::uintptr_t>(internal+0xa0,shape);m.put<std::uintptr_t>(internal+0xd0,body);
 world_put(m,internal+0x78,internal+0x88,{.4,.1,-.2,.9},{2,-3,4});m.put<unsigned>(internal+0x180,0x13579);
 m.put<float>(internal+0x1b0,2.f);m.put<float>(internal+0x1b4,3.f);
 for(unsigned n=0;n<3;++n)m.put<float>(internal+0x1b8+n*4,float(n+2));
}
static Memory batch_fixture(char const* path){auto m=world_fixture(path,owner::Kind::box,true);
 m.put<std::uintptr_t>(0x140088,other_actor_at);m.put<std::uintptr_t>(other_actor_at+0x40,0);
 batch_shape(m,0x290000,0x2a0000,owner::Kind::sphere,other_actor_at,0,0);m.put<std::uintptr_t>(other_actor_at+0x48,0x2a0000);
 m.block(batch_root,0x1f0);m.block(batch_internals,24);m.block(batch_publics,24);
 m.put<std::uintptr_t>(0x160048,batch_root);m.put<std::uintptr_t>(batch_root,base+0x227950);m.put<unsigned>(batch_root+0x168,9);
 for(unsigned n=0;n<3;++n){m.put<std::uintptr_t>(batch_root+0x1b0+n*8,batch_internals+(n?24:0));m.put<std::uintptr_t>(batch_root+0x1d8+n*8,batch_publics+(n?24:0));}
 const std::uintptr_t internals[]={internal_shape_at,0x270000,0x280000},publics[]={shape_at,0x250000,0x260000};
 const owner::Kind kinds[]={owner::Kind::box,owner::Kind::capsule,owner::Kind::sphere};
 for(unsigned n=0;n<3;++n){batch_shape(m,publics[n],internals[n],kinds[n],0x160000,internal_actor_at,batch_root);
  m.put<std::uintptr_t>(batch_internals+n*8,internals[n]);m.put<std::uintptr_t>(batch_publics+n*8,publics[n]);}
 return m;
}
static void batch_capacity_fixture(Memory& m){
 m.block(0x130000,40);m.put(0x12006c,s::pose::ArrayHeader{0x130000,5,5});
 const std::uintptr_t bodies[]={0x140000,0x150000,0x300000,0x310000,0x320000};
 const std::uintptr_t actors[]={other_actor_at,0x160000,0x330000,0x340000,0x350000};
 for(unsigned n=0;n<5;++n){if(n>=2){m.block(bodies[n],0x90);m.block(actors[n],0x50);m.put<std::uintptr_t>(actors[n],base+0x2330a8);m.actor_owner(actors[n],base);
   m.put<std::uintptr_t>(actors[n]+0x38,0x170000);m.put<std::uintptr_t>(actors[n]+0x40,0);}
  m.put<std::uintptr_t>(0x130000+n*8,bodies[n]);m.put<std::uintptr_t>(bodies[n]+0x88,actors[n]);
  const auto root=std::uintptr_t(0x400000)+n*0x20000,in= root+0x1000,pub=root+0x2000;
  m.block(root,0x1f0);m.block(in,512);m.block(pub,512);m.put<std::uintptr_t>(actors[n]+0x48,root);
  m.put<std::uintptr_t>(root,base+0x227950);m.put<unsigned>(root+0x168,9);
  for(unsigned k=0;k<3;++k){m.put<std::uintptr_t>(root+0x1b0+k*8,in+(k?512:0));m.put<std::uintptr_t>(root+0x1d8+k*8,pub+(k?512:0));}
  for(unsigned i=0;i<64;++i){const auto public_shape=root+0x3000+i*0x400,internal=public_shape+0x80;
   batch_shape(m,public_shape,internal,owner::Kind::sphere,actors[n],n==1?internal_actor_at:0,root);
   m.put<std::uintptr_t>(in+i*8,internal);m.put<std::uintptr_t>(pub+i*8,public_shape);}
 }
}
struct BatchGuard {s::Snapshot const& scope;unsigned* calls{};unsigned stop{};
 bool operator()(){return !calls||++*calls!=stop;}
 bool operator()(batch::asset::Snapshot const& value){return operator()()&&s::same(scope,value.query_source);}
};
static int batch_cases(char const* path){try{auto work=std::make_unique<batch::Workspace>();auto out=std::make_unique<batch::Snapshot>();
 auto m=batch_fixture(path);auto read=[&](void* p,void const* ptr,std::size_t n){return m.read(p,ptr,n);};s::Snapshot scope{};
 check(s::capture(base,size,source(),read,[]{return true;},scope)==s::Code::current,"batch current source");const auto initial=m;
 unsigned calls{};check(batch::observe(base,size,scope,read,BatchGuard{scope,&calls},*work,*out)==batch::Code::observed,"whole asset actor/member/world collection");
 const auto guard_count=calls;
 check(out->assets.body_count==2&&out->assets.actor_count==2&&out->count==4&&out->body_actor[0]==0&&out->body_actor[1]==1,"all bodies and first-occurrence actors preserved");
 check(out->actors[0].count==1&&out->actors[1].count==3&&out->actors[1].first==1&&out->rows[0].geometry.parameters.owner.kind==owner::Kind::sphere,"static singleton and dynamic compound all kinds");
 for(unsigned n=0;n<4;++n)check(out->rows[n].actor_index==(n?1u:0u)&&out->rows[n].member_ordinal==(n?n-1:0u)&&out->rows[n].geometry.body_pose_present==(n!=0),"canonical actor/member rows and independent dynamic status");
 check(m.blocks==initial.blocks,"batch never writes original-source bytes");
 auto saved=std::make_unique<batch::Snapshot>(*out);
 // Reading a late actor changes an earlier actor's stored shape pose. Both
 // actor groups are valid in isolation; full-pass equality must still fail.
 bool fired=false;auto changing=[&](void* p,void const* ptr,std::size_t n){if(!fired&&reinterpret_cast<std::uintptr_t>(ptr)==internal_shape_at+0x88){fired=true;m.put<float>(0x2a0088,100.f);}return m.read(p,ptr,n);};
 check(batch::observe(base,size,scope,changing,BatchGuard{scope},*work,*out)==batch::Code::changed&&fired&&batch::same(*out,*saved)&&!work->active,"cross-actor geometry mutation invalidates entire batch");
 m=initial;fired=false;auto member_change=[&](void* p,void const* ptr,std::size_t n){if(!fired&&reinterpret_cast<std::uintptr_t>(ptr)==internal_shape_at+0x88){fired=true;m.put<std::uintptr_t>(batch_publics+16,shape_at);}return m.read(p,ptr,n);};
 check(batch::observe(base,size,scope,member_change,BatchGuard{scope},*work,*out)!=batch::Code::observed&&fired&&batch::same(*out,*saved),"compound member change cannot publish partial geometry");
 m=initial;m.put<std::uintptr_t>(0x140088,0x160000);
 check(batch::observe(base,size,scope,read,BatchGuard{scope},*work,*out)==batch::Code::observed&&out->assets.actor_count==1&&out->count==3&&out->body_actor[0]==0&&out->body_actor[1]==0,"shared BodyData actor sampled once with both body references retained");
 m=initial;m.put<std::uintptr_t>(0x140088,0);
 check(batch::observe(base,size,scope,read,BatchGuard{scope},*work,*out)==batch::Code::observed&&out->assets.actor_count==1&&out->count==3&&out->body_actor[0]==~0u&&out->body_actor[1]==0,"null nonroot BodyData is explicit rather than fabricated");
 m=initial;*out=*saved;m.put<std::uintptr_t>(0x250000,base+owner::shape_tables[2]);
 check(batch::observe(base,size,scope,read,BatchGuard{scope},*work,*out)==batch::Code::world&&batch::same(*out,*saved),"unsupported convex member rejects whole collection");
 m=initial;bool entered=false;batch::Code nested{};
 auto reentrant=[&](void* p,void const* ptr,std::size_t n){if(!entered){entered=true;nested=batch::observe(base,size,scope,read,BatchGuard{scope},*work,*out);}return m.read(p,ptr,n);};
 check(batch::observe(base,size,scope,reentrant,BatchGuard{scope},*work,*out)==batch::Code::observed&&nested==batch::Code::busy,"batch same-work reentry rejected without disturbing outer collection");
 m=initial;*out=*saved;
 check(batch::observe(base,size,scope,read,BatchGuard{scope,&calls,calls+1},*work,*out)==batch::Code::guard&&batch::same(*out,*saved),"initial owner rejection before collecting");
 calls=0;check(batch::observe(base,size,scope,read,BatchGuard{scope,&calls,guard_count},*work,*out)==batch::Code::guard&&batch::same(*out,*saved)&&!work->active,"last original guard boundary protects output");
 unsigned mutation_calls{};
 struct MutatingScopeGuard {s::Snapshot& scope;unsigned& calls;unsigned last{};
  bool operator()(){if(++calls==last)scope.actor+=8;return true;}
  bool operator()(batch::asset::Snapshot const&){return operator()();}
 } mutate_scope{scope,mutation_calls,guard_count};
 const auto saved_scope=scope;
 check(batch::observe(base,size,scope,read,mutate_scope,*work,*out)==batch::Code::changed&&batch::same(*out,*saved)&&!work->active,"final guard cannot change caller query source after closing");scope=saved_scope;
 check(batch::observe(base,size,scope,read,BatchGuard{scope},*work,work->first)==batch::Code::alias,"batch scratch output alias rejected");
 // Empty compound is retained without turning it into a clearance result.
 for(unsigned n=0;n<3;++n){m.put<std::uintptr_t>(batch_root+0x1b0+n*8,0);m.put<std::uintptr_t>(batch_root+0x1d8+n*8,0);}
 check(batch::observe(base,size,scope,read,BatchGuard{scope},*work,*out)==batch::Code::observed&&out->actors[1].count==0&&out->count==1,"empty actor inventory is represented explicitly");
 m.put<std::uintptr_t>(other_actor_at+0x48,0);
 check(batch::observe(base,size,scope,read,BatchGuard{scope},*work,*out)==batch::Code::observed&&out->count==0&&out->assets.actor_count==2,"all empty actors retained as an observation, not an occupancy decision");
 m=initial;batch_capacity_fixture(m);s::Snapshot large_scope{};
 check(s::capture(base,size,source(),read,[]{return true;},large_scope)==s::Code::current,"five-actor capacity fixture scope");
 *out=*saved;
 check(batch::observe(base,size,large_scope,read,BatchGuard{large_scope},*work,*out)==batch::Code::capacity&&work->failed_actor==4&&batch::same(*out,*saved),"320 members exceed global capacity without truncated publish");
 const auto last_root=std::uintptr_t(0x480000);for(unsigned n=0;n<3;++n){m.put<std::uintptr_t>(last_root+0x1b0+n*8,0);m.put<std::uintptr_t>(last_root+0x1d8+n*8,0);}
 check(batch::observe(base,size,large_scope,read,BatchGuard{large_scope},*work,*out)==batch::Code::observed&&out->count==256&&out->assets.actor_count==5&&out->actors[4].count==0,"exact256 shapes and empty fifth actor fully represented");
 *saved=*out;
 for(unsigned n=0;n<3;++n){m.put<std::uintptr_t>(last_root+0x1b0+n*8,last_root+0x1000+(n?8:0));m.put<std::uintptr_t>(last_root+0x1d8+n*8,last_root+0x2000+(n?8:0));}
 check(batch::observe(base,size,large_scope,read,BatchGuard{large_scope},*work,*out)==batch::Code::capacity&&batch::same(*out,*saved),"257 shapes refused without losing the previous complete256");
 check(!GetModuleHandleW(L"PhysXCore.dll"),"batch does not load original DLL");
 }catch(...){check(false,"batch fixture exception");}
 printf("AssetShapes checks=%u failed=%u synthetic object groups; complete supplied asset only, no world/self permit\n",checks,failed);return failed?1:0;
}
int main(int argc,char** argv){if(argc!=2)return 2;if(PriorAssetShapesWorldMain(argc,argv))return 1;return batch_cases(argv[1]);}
