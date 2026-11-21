#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "scoped_read_validation.h"
#include <vector>
#include <random>
#include <chrono>
#include <cstdio>
using namespace gears_read_validation;
using Plain=BatchedProof<32768,1048576,65536,1048576,64>;
static Plain original;static SourceProof cached;
struct Input {uintptr_t address;unsigned bytes;};static std::vector<Input> inputs;static unsigned char* arena;static unsigned checks;
#define C(x) do{++checks;if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
template<class P>static void record(P& p){p.begin();for(auto in:inputs)p.record(in.address,reinterpret_cast<void const*>(in.address),in.bytes);}
__declspec(noinline) static bool copy(void* to,uintptr_t from,unsigned size)noexcept {__try {memcpy(to,reinterpret_cast<void const*>(from),size);return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
template<class A,class B>static bool parity(A const& a,B const& b){
 if(a.valid!=b.valid||a.failure!=b.failure||a.failure_address!=b.failure_address||a.failure_bytes!=b.failure_bytes||a.count!=b.count||a.used!=b.used||a.merged!=b.merged||a.batched!=b.batched||a.batch_count!=b.batch_count||a.planned_bytes!=b.planned_bytes||a.padding_bytes!=b.padding_bytes)return false;
 if(memcmp(a.ranges.data(),b.ranges.data(),a.merged*sizeof(a.ranges[0]))||memcmp(a.compact.data(),b.compact.data(),a.used))return false;
 if(a.batched&&(memcmp(a.batches.data(),b.batches.data(),a.batch_count*sizeof(a.batches[0]))||memcmp(a.expected.data(),b.expected.data(),a.planned_bytes)||memcmp(a.mask.data(),b.mask.data(),a.planned_bytes)))return false;return true;}
static int targeted(){
 // Each descriptor change must sort, then execute the exact original compaction/planner.
 for(unsigned which=0;which<5;++which){record(original);record(cached);C(original.seal()&&cached.seal());record(original);record(cached);auto hits=cached.order_cache.hits,misses=cached.order_cache.misses;
 if(which==0){++original.ranges[0].address;++cached.ranges[0].address;}
 if(which==1){++original.ranges[0].bytes;++cached.ranges[0].bytes;}
 if(which==2){std::swap(original.ranges[0],original.ranges[1]);std::swap(cached.ranges[0],cached.ranges[1]);}
 if(which==3){++original.ranges[0].offset;++cached.ranges[0].offset;}
 if(which==4){--original.count;--cached.count;}
 const auto a=original.seal(),b=cached.seal();C(a==b&&parity(original,cached));C(cached.order_cache.hits==hits&&cached.order_cache.misses==misses+1);}
 // All witness bytes are recaptured and compacted; unchanged order never retains old bytes.
 auto addr=reinterpret_cast<uintptr_t>(arena)+8;arena[8]^=1;record(original);record(cached);C(original.seal()&&cached.seal()&&parity(original,cached));C(original.validate(copy)==Result::same&&cached.validate(copy)==Result::same);arena[8]^=1;C(original.validate(copy)==Result::changed&&cached.validate(copy)==Result::changed);
 using Small=BatchedProof<16,1024,32,128,64>;using Fast=BatchedProof<16,1024,32,128,64,true>;Small a;Fast b;
 auto capture=[&](){a.begin();b.begin();for(unsigned size:{4u,8u}){a.record(addr,reinterpret_cast<void const*>(addr),size);b.record(addr,reinterpret_cast<void const*>(addr),size);}};
 capture();C(a.seal()&&b.seal());capture();a.captured[4]^=1;b.captured[4]^=1;C(!a.seal()&&!b.seal()&&a.failure==Failure::overlap_changed&&a.failure==b.failure&&!b.order_cache.valid);
 capture();a.record(0,arena,4);b.record(0,arena,4);C(!a.seal()&&!b.seal()&&a.failure==b.failure&&!b.order_cache.valid);
 capture();unsigned altered=0;a.record(addr,&altered,4);b.record(addr,&altered,4);C(!a.seal()&&!b.seal()&&a.failure==Failure::duplicate_changed&&a.failure==b.failure);
 a.begin();b.begin();C(!a.seal()&&!b.seal()&&a.failure==Failure::empty&&a.failure==b.failure);
 using Exhaust=BatchedProof<2,4096,4,0,64>;using ExhaustFast=BatchedProof<2,4096,4,0,64,true>;Exhaust x;ExhaustFast y;
 for(unsigned repeat=0;repeat<2;++repeat){x.begin();y.begin();x.record(reinterpret_cast<uintptr_t>(arena),arena,4096);y.record(reinterpret_cast<uintptr_t>(arena),arena,4096);C(x.seal()&&y.seal()&&!x.batched&&!y.batched&&parity(x,y));C(x.validate(copy)==Result::same&&y.validate(copy)==Result::same);}C(y.order_cache.hits==1);
 printf("targeted checks=%u descriptor_fallbacks=5 fresh_byte_overlap_record_failure_plan_exhaustion=passed\n",checks);return 0;}
int main(){arena=static_cast<unsigned char*>(VirtualAlloc(nullptr,8*1024*1024,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));C(arena);memset(arena,0x42,8*1024*1024);
 for(unsigned i=0;i<30052;++i){const unsigned offsets[]={8,8,64,128},sizes[]={4,8,12,8};inputs.push_back({reinterpret_cast<uintptr_t>(arena)+(i/4)*512+offsets[i%4],sizes[i%4]});}
 std::mt19937 random(1234);std::shuffle(inputs.begin(),inputs.end(),random);record(original);record(cached);C(original.seal()&&cached.seal());C(original.count==30052&&cached.count==30052);C(targeted()==0);C(VirtualFree(arena,0,MEM_RELEASE));printf("checks=%u failures=0\n",checks);return 0;}
