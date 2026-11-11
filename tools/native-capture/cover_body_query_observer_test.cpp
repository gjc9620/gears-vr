#include "cover_body_query_observer.h"
#define GEARS_QUERY_SESSION_TEST_MAIN PriorBatchMain
#include "cover_physics_query_live_test.cpp"
#undef GEARS_QUERY_SESSION_TEST_MAIN
int main(){if(PriorBatchMain())return 1;
 arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
 reset();auto session=std::make_unique<HostSession>();C(session->start(true,true));
 namespace batch=gears_cover_body_query_observer;namespace cat=batch::c;
 auto catalog=std::make_unique<cat::Catalog>();catalog->units=cat::p::Units::physx;catalog->body_count=1;catalog->count=3;
 cat::g::names::Mapping map{};map.count=1;map.root_bone_index=2;map.geometry=cat::g::body::rig::marcus115_signature;
 map.rows[0].bone_index=2;map.rows[0].name={10,0};map.reference_names[2]=map.rows[0].name;
 cat::g::Inventory inventory{};inventory.count={1,1,1,0};
 for(unsigned i=0;i<3;++i){auto& s=catalog->rows[i].shape;s.units=cat::p::Units::physx;s.bone_index=2;s.bone={10,0};s.kind=static_cast<cat::g::metrics::Kind>(i);}
 catalog->rows[0].shape.sphere={{1,2,3},1};catalog->rows[1].shape.box=test_box;catalog->rows[2].shape.capsule={{1,2,3},{1,2,5},1};
 batch::Workspace work{};batch::Result out{};
 C(batch::observe(*session,actor,physics_expected,*catalog,map,&inventory,source_read,guard,work,out)==batch::Code::observed&&out.count==3&&out.raw_hit[0]&&out.raw_hit[1]&&out.raw_hit[2]);
 const auto saved=out;const auto calls_before=query_calls;catalog->rows[2].shape.capsule.radius=-1;
 C(batch::observe(*session,actor,physics_expected,*catalog,map,&inventory,source_read,guard,work,out)==batch::Code::catalog&&query_calls==calls_before&&!memcmp(&out,&saved,sizeof out));
 catalog->rows[2].shape.capsule.radius=1;
 catalog->units=cat::p::Units::unspecified;
 C(batch::observe(*session,actor,physics_expected,*catalog,map,&inventory,source_read,guard,work,out)==batch::Code::catalog&&query_calls==calls_before&&!memcmp(&out,&saved,sizeof out));
 catalog->units=cat::p::Units::physx;catalog->rows[2].shape.units=cat::p::Units::unspecified;
 C(batch::observe(*session,actor,physics_expected,*catalog,map,&inventory,source_read,guard,work,out)==batch::Code::catalog&&query_calls==calls_before&&!memcmp(&out,&saved,sizeof out));
 catalog->rows[2].shape.units=cat::p::Units::physx;
 for(unsigned fail=0;fail<3;++fail){const auto first=query_calls;auto closing=[&](){return guard()&&query_calls<first+fail+1;};
 C(batch::observe(*session,actor,physics_expected,*catalog,map,&inventory,source_read,closing,work,out)!=batch::Code::observed&&!memcmp(&out,&saved,sizeof out));}
 const auto first=query_calls;auto mutate=[&](){if(query_calls>first)catalog->rows[2].array_index=7;return guard();};
 C(batch::observe(*session,actor,physics_expected,*catalog,map,&inventory,source_read,mutate,work,out)!=batch::Code::observed&&!memcmp(&out,&saved,sizeof out));
 session->request_drain();C(session->poll_stop());C(native_calls==0);C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
 printf("BodyQueryObserver checks=%u failures=%u actual Session; Host original dispatch; raw only\n",checks,failures);return failures?1:0;
}
