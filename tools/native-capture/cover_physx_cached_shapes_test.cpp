#include "cover_physx_cached_shapes.h"
#include <cstdio>
#include <memory>
#include <vector>
#include <limits>
namespace d=gears_cover_physx_cached_shapes;
static unsigned checks{},failures{};
#define CHECK(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %d %s\n",__LINE__,#x);}}while(false)
constexpr uintptr_t arena_base=0x10000000,root_desc=arena_base+0x3000,list_at=arena_base+0x5000;
struct Memory {std::vector<unsigned char> bytes=std::vector<unsigned char>(0x20000);
 template<class T>void put(uintptr_t at,T const& value){std::memcpy(bytes.data()+at-arena_base,&value,sizeof value);}
 bool read(void* dst,void const* at,size_t n){auto p=reinterpret_cast<uintptr_t>(at);if(p<arena_base||p-arena_base>bytes.size()||n>bytes.size()-(p-arena_base))return false;std::memcpy(dst,bytes.data()+p-arena_base,n);return true;}
};
static Memory fixture(unsigned count){Memory m{};m.put(arena_base+0xcc,d::raw::ArrayHeader{arena_base+0x1000,1,1});
 m.put(arena_base+0xdc,d::raw::ArrayHeader{arena_base+0x2004,1,1});m.put(arena_base+0x1000,root_desc);m.put(arena_base+0x2004,std::array<float,3>{1,2,3});
 m.put(root_desc+104,count?d::Vector{list_at,list_at+8*count,list_at+8*count}:d::Vector{});
 for(unsigned n=0;n<count;++n){auto address=arena_base+0x6000+n*0x100;const unsigned tag=n%3+1;
  m.put(list_at+8*n,address);m.put(address,uintptr_t(0x140000000+(tag==1?0x1347dc0:tag==2?0x13486f0:0x1347dd8)));m.put(address+8,tag);
  d::math::Matrix local{};local.values[0]=local.values[4]=local.values[8]=1;local.values[9]=float(n);m.put(address+12,local);m.put(address+60,8u);
  m.put(address+128,2.f);m.put(address+132,3.f);m.put(address+136,4.f);
 }return m;}
int main(){d::Source source{0x140000000,0x2000000};d::cache::Source cache_source{arena_base,arena_base+0xcc,arena_base+0xdc};
 auto work=std::make_unique<d::Workspace>();auto cache_work=std::make_unique<d::cache::Workspace>();auto output=std::make_unique<d::Snapshot>();
 d::cache::Snapshot inventory{};const std::array<float,3> requested{1,2,3};auto guard=[](d::Source const&,d::cache::Snapshot const&){return true;};
 Memory m{};auto read=[&](void* dst,void const* at,size_t n){return m.read(dst,at,n);};
 auto prepare=[&](unsigned count){m=fixture(count);CHECK(d::cache::observe(cache_source,read,[](d::cache::Source const&){return true;},*cache_work,inventory)==d::cache::Code::observed);};
 for(unsigned count:{0u,1u,3u,256u}){prepare(count);CHECK(d::observe(source,inventory,requested,read,guard,*work,*output)==d::Code::observed&&output->count==count);
  CHECK(d::same(output->source,source)&&d::cache::same(output->cache_source,cache_source)&&output->requested_scale==requested);
  for(unsigned n=0;n<count;++n){const auto& row=output->rows[n];CHECK(row.tag==n%3+1&&row.flags==8&&row.local.values[9]==float(n));
   CHECK(row.tag==2?(row.half[0]==2&&row.half[1]==3&&row.half[2]==4):row.radius==2);
   if(row.tag==3)CHECK(row.full_segment_length==3);}
 }
 prepare(3);unsigned reads{},guards{};auto counting=[&](void* dst,void const* at,size_t n){++reads;return m.read(dst,at,n);};
 auto count_guard=[&](d::Source const&,d::cache::Snapshot const&){++guards;return true;};
 CHECK(d::observe(source,inventory,requested,counting,count_guard,*work,*output)==d::Code::observed);const auto read_count=reads,guard_count=guards;
 output->count=123;
 for(unsigned fail=1;fail<=read_count;++fail){unsigned calls{};auto fault=[&](void* dst,void const* at,size_t n){return ++calls!=fail&&m.read(dst,at,n);};
  CHECK(d::observe(source,inventory,requested,fault,guard,*work,*output)!=d::Code::observed&&output->count==123);}
 for(unsigned fail=1;fail<=guard_count;++fail){unsigned calls{};auto fault=[&](d::Source const&,d::cache::Snapshot const&){return ++calls!=fail;};
  CHECK(d::observe(source,inventory,requested,read,fault,*work,*output)!=d::Code::observed&&output->count==123);}
 for(unsigned mutation=0;mutation<5;++mutation){prepare(3);output->count=123;unsigned shape_visits{};
  auto changing=[&](void* dst,void const* at,size_t n){if(reinterpret_cast<uintptr_t>(at)==arena_base+0x6000&&++shape_visits==3){
    if(mutation==0)m.put(arena_base+0x6000+128,9.f);
    if(mutation==1)m.put(list_at,arena_base+0x6100);
    if(mutation==2)m.put(root_desc+104,d::Vector{list_at,list_at+16,list_at+24});
    if(mutation==3)m.put(arena_base+0x2004,std::array<float,3>{2,2,3});
    if(mutation==4)m.put(arena_base+0xcc,d::raw::ArrayHeader{arena_base+0x1000,0,1});}
   return m.read(dst,at,n);};
  CHECK(d::observe(source,inventory,requested,changing,guard,*work,*output)!=d::Code::observed&&output->count==123);
 }
 for(unsigned bad=0;bad<7;++bad){prepare(3);output->count=123;
  if(bad==0)m.put(root_desc+104,d::Vector{list_at,list_at+8*257,list_at+8*257});
  if(bad==1)m.put(arena_base+0x6000+8,99u);if(bad==2)m.put(arena_base+0x6000,uintptr_t(0x140000000));
  if(bad==3)m.put(arena_base+0x6000+128,std::numeric_limits<float>::quiet_NaN());
  if(bad==4)m.put(arena_base+0x6000+12,2.f);if(bad==5)m.put(root_desc+104,d::Vector{list_at+4,list_at+12,list_at+20});
  if(bad==6)m.put(list_at,uintptr_t{});
  CHECK(d::observe(source,inventory,requested,read,guard,*work,*output)!=d::Code::observed&&output->count==123);
 }
 prepare(3);bool reentered=false;auto recursive=[&](d::Source const&,d::cache::Snapshot const&){if(!reentered){reentered=true;CHECK(d::observe(source,inventory,requested,read,guard,*work,*output)==d::Code::busy);}return true;};
 CHECK(d::observe(source,inventory,requested,read,recursive,*work,*output)==d::Code::observed&&reentered);
 CHECK(d::observe(source,inventory,requested,read,guard,*work,work->first)==d::Code::alias);
 std::printf("CachedShapes checks=%u failures=%u reads=%u guards=%u original descriptor fields only; no active identity\n",checks,failures,read_count,guard_count);return failures?1:0;
}
