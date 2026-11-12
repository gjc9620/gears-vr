#include "cover_physx_query.h"
#include "cover_physx_asset_bounds.h"
#define GEARS_ASSET_SHAPES_TEST_MAIN PriorAssetEngineMain
#include "cover_physx_asset_shapes_test.cpp"
#undef GEARS_ASSET_SHAPES_TEST_MAIN
namespace query=gears_cover_physx_query;
static unsigned image_primes{},image_closes{},original_calls{};
struct HostImage {
 bool primed{};bool prime(){++image_primes;primed=true;return true;}
 bool close(){++image_closes;primed=false;return true;}
 uintptr_t base()const{return primed?::base:0;}uint32_t size()const{return ::size;}
 void retain_for_process(){}
};
struct NoCalls {};
struct EngineGuard {
 s::Snapshot const& scope;
 bool operator()(){return true;}
 bool operator()(s::Snapshot const& value){return s::same(scope,value);}
 bool operator()(batch::asset::Snapshot const& value){return s::same(scope,value.query_source);}
};
__declspec(noinline)static int engine_cases(char const* path){
 auto m=batch_fixture(path);auto read=[&](void* out,void const* at,size_t n){return m.read(out,at,n);};s::Snapshot scope{};
 check(s::capture(base,size,source(),read,[]{return true;},scope)==s::Code::current,"engine source fixture");
 auto engine=std::make_unique<query::QueryEngine<HostImage,NoCalls>>();auto work=std::make_unique<query::AssetShapeWorkspace>();auto output=std::make_unique<batch::Snapshot>();
 output->count=123;auto key=source();
 check(engine->observe_asset_shapes(key,read,EngineGuard{scope},*work,*output)==query::Code::unavailable&&output->count==123,"unprimed read does not publish");
 check(engine->prime(),"owner primes image without loading original DLL");
 check(engine->observe_asset_shapes(key,read,EngineGuard{scope},*work,*output)==query::Code::queried&&output->count==4&&work->detail==batch::Code::observed,"actual pinned-image transaction reads complete asset");
 gears_cover_physx_asset_bounds::Snapshot ranges{};
 check(gears_cover_physx_asset_bounds::convert(*output,ranges)==gears_cover_physx_asset_bounds::Code::bounded&&ranges.count==4&&ranges.has_shapes,
       "complete observed Engine packet produces all mathematical bounds after close");
 check(image_primes==1&&!engine->pending()&&!engine->uncertain()&&!original_calls,"read releases owner and cannot dispatch original calls");
 const auto saved=std::make_unique<batch::Snapshot>(*output);bool fired=false;
 auto changing=[&](void* dst,void const* at,size_t n){if(!fired&&reinterpret_cast<uintptr_t>(at)==internal_shape_at+0x88){fired=true;m.put<float>(0x2a0088,100);}return m.read(dst,at,n);};
 check(engine->observe_asset_shapes(key,changing,EngineGuard{scope},*work,*output)==query::Code::owner_read&&fired&&batch::same(*output,*saved)&&!engine->pending(),"cross-actor mutation rejects without publishing or retaining ownership");
 m=batch_fixture(path);bool nested=false;query::Code nested_code{};
 auto recursive=[&](void* dst,void const* at,size_t n){if(!nested){nested=true;nested_code=engine->observe_asset_shapes(key,read,EngineGuard{scope},*work,*output);}return m.read(dst,at,n);};
 check(engine->observe_asset_shapes(key,recursive,EngineGuard{scope},*work,*output)==query::Code::queried&&nested_code==query::Code::reentrant,"same engine refuses recursive reader");
 check(engine->observe_asset_shapes(key,read,EngineGuard{scope},*work,work->candidate)==query::Code::request,"scratch alias rejected before capture");
 check(engine->close()&&image_closes==1&&!GetModuleHandleW(L"PhysXCore.dll"),"image closes normally; no original DLL loaded");
 printf("AssetShapesEngine checks=%u failed=%u actual QueryEngine/HostImage only; no original dispatch\n",checks,failed);return failed?1:0;
}
int main(int argc,char** argv){if(argc!=2)return 2;if(PriorAssetEngineMain(argc,argv))return 1;return engine_cases(argv[1]);}
