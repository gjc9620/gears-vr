#define GEARS_QUERY_SESSION_TEST_MAIN PriorAssetReadSessionMain
#include "cover_physics_query_live_test.cpp"
#undef GEARS_QUERY_SESSION_TEST_MAIN
static unsigned typed_mutation{};
static void mutate_typed_read(){
 switch(typed_mutation){
 case 0:put(body_instances[0]+0x60,7);break;
 case 1:put(body_instances[0]+0x68,uintptr_t{});break;
 case 2:put(body_instances[0]+0x90,arena+0x80000);break;
 case 3:put(props[7]+0x94,0x98u);break;
 case 4:put(props[7]+0x74,4u);break;
 case 5:put(props[6]+0xd0,fixture_pawn_class);break;
 }
}
static unsigned asset_read_calls{},extra_shape_checks{};static bool expired_read{},wrong_read_asset{};static void expire_read(){expired_read=true;}
struct ShapeHost:HostEngine {
 template<class Read,class Guard>q::Code observe_asset_shapes(q::source::Source const& source,Read read,Guard current,
    q::AssetShapeWorkspace&,gears_cover_physx_asset_shapes::Snapshot& output){
  ++asset_read_calls;active=true;struct Finish{bool& active;~Finish(){active=false;}}finish{active};
  if(!h::mutex.try_lock())return q::Code::source;h::mutex.unlock();
  q::source::Snapshot value{};if(q::source::capture(0x180000000,0x300000,source,read,current,value)!=q::source::Code::current)return q::Code::source;
  if(during_query)during_query();if(!current(value))return q::Code::source;
  for(unsigned n=0;n<extra_shape_checks;++n)if(!current(value))return q::Code::source;
  gears_cover_physx_asset_actors::Workspace actors_work;gears_cover_physx_asset_actors::Snapshot actors;
  if(gears_cover_physx_asset_actors::observe(0x180000000,0x300000,value,read,current,actors_work,actors)!=gears_cover_physx_asset_actors::Code::observed)return q::Code::source;
  if(wrong_read_asset)actors.bodies[0].actor+=8;
  if(!current(actors))return q::Code::source;
  // Real actor observer against this supplied typed memory, no shape/world
  // or native module dispatch. Separate Engine fixture covers shape rows.
  output={};output.assets=actors;return q::Code::queried;
 }
};
using ShapeSession=service::Session<ShapeHost>;
static ShapeSession* read_session{};static bool drain_pending{},drain_polled{};
static void drain_read(){read_session->request_drain();drain_pending=read_session->pending_count()!=0;drain_polled=read_session->poll_stop();}
__declspec(noinline)static int read_session_cases(){
 arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
 auto scratch=std::make_unique<q::AssetShapeWorkspace>();auto output=std::make_unique<gears_cover_physx_asset_shapes::Snapshot>();
 reset();{auto owner=std::make_unique<ShapeSession>();output->count=123;
  C(owner->observe_asset_shapes(actor,physics_expected,source_read,guard,*scratch,*output)==q::Code::unavailable&&output->count==123);
  C(owner->start(true,true));C(owner->observe_asset_shapes(actor,physics_expected,source_read,guard,*scratch,*output)==q::Code::queried&&output->count==0);
  C(output->assets.query_source.source.mesh==fixture_mesh&&asset_read_calls==1&&query_calls==0&&native_calls==0);
  C(output->assets.body_count==2&&output->assets.actor_count==2);
  C(owner->observation().stage==service::Stage::complete);
  C(owner->observation().physics_rechecks==1&&owner->observation().asset_read_guards>0);
  output->count=123;during_query=replace_root;
  C(owner->observe_asset_shapes(actor,physics_expected,source_read,guard,*scratch,*output)!=q::Code::queried&&output->count==123&&query_calls==0);
  during_query=nullptr;owner->request_drain();C(owner->poll_stop()&&!owner->pending_count());}
 reset();{auto owner=std::make_unique<ShapeSession>();C(owner->start(true,true));read_session=owner.get();during_query=drain_read;output->count=123;
  C(owner->observe_asset_shapes(actor,physics_expected,source_read,guard,*scratch,*output)!=q::Code::queried&&output->count==123);
  C(drain_pending&&!drain_polled&&owner->poll_stop()&&!owner->pending_count()&&query_calls==0);during_query=nullptr;}
 // Actual Session with repeated typed SourceGuard calls, same empty Host.
 reset();extra_shape_checks=64;expired_read=false;unsigned full_calls{};
 {auto owner=std::make_unique<ShapeSession>();C(owner->start(true,true));auto full=[&](){++full_calls;return !expired_read;};
  C(owner->observe_asset_shapes(actor,physics_expected,source_read,full,*scratch,*output)==q::Code::queried);owner->request_drain();C(owner->poll_stop());}
 printf("AssetReadFullOwner calls=%u repeated_typed_checks=%u\n",full_calls,extra_shape_checks);
 for(unsigned fail=1;fail<=full_calls;++fail){reset();unsigned calls{};auto owner=std::make_unique<ShapeSession>();C(owner->start(true,true));output->count=123;
  C(owner->observe_asset_shapes(actor,physics_expected,source_read,[&](){return ++calls!=fail;},*scratch,*output)!=q::Code::queried&&output->count==123);owner->request_drain();C(owner->poll_stop());}
 reset();expired_read=false;during_query=expire_read;
 {auto owner=std::make_unique<ShapeSession>();C(owner->start(true,true));output->count=123;
  C(owner->observe_asset_shapes(actor,physics_expected,source_read,[]{return !expired_read;},*scratch,*output)!=q::Code::queried&&output->count==123);owner->request_drain();C(owner->poll_stop());}
 for(typed_mutation=0;typed_mutation<6;++typed_mutation){reset();during_query=mutate_typed_read;auto owner=std::make_unique<ShapeSession>();C(owner->start(true,true));output->count=123;
  C(owner->observe_asset_shapes(actor,physics_expected,source_read,guard,*scratch,*output)!=q::Code::queried&&output->count==123);owner->request_drain();C(owner->poll_stop());}
 reset();during_query=nullptr;wrong_read_asset=true;
 {auto owner=std::make_unique<ShapeSession>();C(owner->start(true,true));output->count=123;
  C(owner->observe_asset_shapes(actor,physics_expected,source_read,guard,*scratch,*output)!=q::Code::queried&&output->count==123);owner->request_drain();C(owner->poll_stop());}
 wrong_read_asset=false;
 during_query=nullptr;extra_shape_checks=0;
 C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
 printf("AssetShapeSession checks=%u failures=%u actual Session with supplied actor packet; no shape/world/original calls\n",checks,failures);return failures?1:0;
}
int main(){if(PriorAssetReadSessionMain())return 1;return read_session_cases();}
