#include "cover_physx_shape_cache.h"
#include <cstdio>
#include <limits>
namespace c=gears_cover_physx_shape_cache;
static unsigned checks{},failed{};
#define CHECK(x) do{++checks;if(!(x)){++failed;std::printf("FAIL %d %s\n",__LINE__,#x);}}while(false)
constexpr uintptr_t base=0x10000000,cache_pointers_at=base+0x1000,cache_scales_at=base+0x2004;
struct Memory {std::array<unsigned char,0x4000> bytes{};
 template<class T>void put(uintptr_t at,T const& value){std::memcpy(bytes.data()+at-base,&value,sizeof value);}
 bool read(void* dst,void const* at,size_t n){auto p=reinterpret_cast<uintptr_t>(at);if(p<base||p-base>bytes.size()||n>bytes.size()-(p-base))return false;std::memcpy(dst,bytes.data()+p-base,n);return true;}
};
static Memory fixture(unsigned count){Memory m{};m.put(base+0xcc,c::raw::ArrayHeader{count?cache_pointers_at:0,int(count),int(count)});
 m.put(base+0xdc,c::raw::ArrayHeader{count?cache_scales_at:0,int(count),int(count)});
 for(unsigned n=0;n<count;++n){m.put(cache_pointers_at+8*n,uintptr_t(base+0x3000+128*n));m.put(cache_scales_at+12*n,std::array<float,3>{float(n+1),2,3});}return m;}
int main(){c::Source source{base,base+0xcc,base+0xdc};c::Workspace work{};c::Snapshot output{},sentinel{};sentinel.count=123;
 auto guard=[](c::Source const&){return true;};
 for(unsigned count:{0u,1u,3u,64u}){auto m=fixture(count);unsigned reads{};
  auto read=[&](void* p,void const* at,size_t n){++reads;return m.read(p,at,n);};
  CHECK(c::observe(source,read,guard,work,output)==c::Code::observed&&output.count==count);
  CHECK(reads==8+4*count);
  for(unsigned n=0;n<count;++n)CHECK(output.rows[n].descriptor==base+0x3000+128*n&&output.rows[n].scale[0]==float(n+1));
 }
 auto m=fixture(3);auto read=[&](void* p,void const* at,size_t n){return m.read(p,at,n);};
 // Every read failure must reject atomically, including closing headers.
 for(unsigned fail=1;fail<=20;++fail){unsigned calls{};output=sentinel;
  auto fault=[&](void* p,void const* at,size_t n){return ++calls!=fail&&m.read(p,at,n);};
  CHECK(c::observe(source,fault,guard,work,output)==c::Code::read&&output.count==123);
 }
 for(unsigned mutation=0;mutation<4;++mutation){m=fixture(3);output=sentinel;unsigned reads{};
  auto changing=[&](void* p,void const* at,size_t n){++reads;if(reads==11){
   if(mutation==0)m.put(cache_pointers_at,uintptr_t(base+0x3800));
   if(mutation==1)m.put(cache_scales_at,std::array<float,3>{9,2,3});
   if(mutation==2)m.put(base+0xcc,c::raw::ArrayHeader{cache_pointers_at,2,3});
   if(mutation==3)m.put(base+0xdc,c::raw::ArrayHeader{cache_scales_at,3,4});}
   return m.read(p,at,n);};
  const auto code=c::observe(source,changing,guard,work,output);CHECK(code!=c::Code::observed&&output.count==123);
 }
 m=fixture(3);output=sentinel;m.put(base+0xdc,c::raw::ArrayHeader{cache_scales_at,2,3});CHECK(c::observe(source,read,guard,work,output)==c::Code::capacity&&output.count==123);
 m=fixture(3);m.put(base+0xcc,c::raw::ArrayHeader{cache_pointers_at,65,65});CHECK(c::observe(source,read,guard,work,output)==c::Code::capacity);
 for(float bad:{0.f,-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::denorm_min()}){
  m=fixture(3);m.put(cache_scales_at,std::array<float,3>{bad,2,3});CHECK(c::observe(source,read,guard,work,output)==c::Code::value&&output.count==123);}
 m=fixture(3);m.put(cache_pointers_at,uintptr_t{});CHECK(c::observe(source,read,guard,work,output)==c::Code::value);
 m=fixture(3);for(unsigned fail=1;fail<=3;++fail){unsigned calls{};auto reject=[&](c::Source const&){return ++calls!=fail;};
  CHECK(c::observe(source,read,reject,work,output)==c::Code::guard&&output.count==123);}
 auto changed=source;changed.pointers_header+=4;CHECK(c::observe(changed,read,guard,work,output)==c::Code::request);
 work.active=true;CHECK(c::observe(source,read,guard,work,output)==c::Code::busy);work.active=false;
 CHECK(c::observe(source,read,guard,work,work.first)==c::Code::alias);
 // Preserve duplicate descriptor cache_pointers_at and equal scale keys in original
 // order; this reader must not invent uniqueness or choose a cache winner.
 m=fixture(3);m.put(cache_pointers_at+8,uintptr_t(base+0x3000));m.put(cache_scales_at+12,std::array<float,3>{1,2,3});
 CHECK(c::observe(source,read,guard,work,output)==c::Code::observed&&output.rows[0].descriptor==output.rows[1].descriptor&&output.rows[0].scale==output.rows[1].scale);
 c::Selection selected{},old_selection{};old_selection.index=123;
 CHECK(c::select(output,{1,2,3},selected)==c::Lookup::matched&&selected.index==1);
 selected=old_selection;CHECK(c::select(output,{7,2,3},selected)==c::Lookup::miss&&selected.index==123);
 CHECK(c::select(output,{0,2,3},selected)==c::Lookup::request&&selected.index==123);
 for(unsigned axis=0;axis<3;++axis){const float epsilon=1e-4f;const std::array<float,3> requested{epsilon,epsilon,epsilon};
  auto one=output;one.count=1;one.pointers.count=one.scales.count=1;one.rows[0].scale=requested;
  one.rows[0].scale[axis]=2*epsilon;CHECK(c::select(one,requested,selected)==c::Lookup::miss&&selected.index==123);
  one.rows[0].scale[axis]=std::nextafter(2*epsilon,0.f);CHECK(c::select(one,requested,selected)==c::Lookup::matched&&selected.index==0);
  selected=old_selection;one.rows[0].scale[axis]=std::nextafter(2*epsilon,std::numeric_limits<float>::infinity());
  CHECK(c::select(one,requested,selected)==c::Lookup::miss&&selected.index==123);
 }
 auto invalid=output;invalid.count=65;CHECK(c::select(invalid,{1,2,3},selected)==c::Lookup::request&&selected.index==123);
 CHECK(c::select(output,{1,2,3},*reinterpret_cast<c::Selection*>(&output))==c::Lookup::alias);
 std::printf("ShapeCache checks=%u failures=%u opaque cache pairs only; no descriptor or active shape authority\n",checks,failed);return failed?1:0;
}
