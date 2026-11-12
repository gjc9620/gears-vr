#include "cover_proof_pass.h"
#include <cstdio>
#include <memory>
namespace p=gears_cover_proof_pass;
struct Identity {std::uintptr_t address{};unsigned value{};};
static unsigned checks{},failures{},id_reads{},field_reads{};
#define C(x) do{++checks;if(!(x)){++failures;printf("FAIL %d %s\n",__LINE__,#x);}}while(false)
int main(){
    auto cache=std::make_unique<p::Cache<Identity>>();std::array<unsigned,3> values{11,22,33};
    auto same=[](Identity const& a,Identity const& b){return a.address==b.address&&a.value==b.value;};
    auto capture=[&](std::uintptr_t address,Identity& out){++id_reads;if(!address||address>values.size())return false;out={address,values[address-1]};return true;};
    auto read=[&](void* out,std::uintptr_t address,unsigned size){++field_reads;if(!address||address>values.size()||size>4)return false;std::memcpy(out,&values[address-1],size);return true;};
    auto field=[&](auto& c,std::uintptr_t address,unsigned expected){return c.field(address,4,reinterpret_cast<unsigned char const*>(&expected),read);};
    C(!cache->identity({1,11},capture,same));C(cache->begin());C(!cache->begin());
    for(unsigned i=0;i<200;++i){C(cache->identity({1,11},capture,same));C(field(*cache,1,11));}
    C(id_reads==1&&field_reads==1&&cache->last.identity_visits==200&&cache->last.field_visits==200);
    C(!cache->identity({1,12},capture,same));C(!field(*cache,1,12));cache->end();
    const auto total=cache->total;cache->end();C(cache->total.passes==total.passes);
    C(!field(*cache,1,11));values[0]=99;C(cache->begin());
    C(!cache->identity({1,11},capture,same));C(!field(*cache,1,11));C(cache->identity({1,99},capture,same));C(field(*cache,1,99));cache->end();
    // Every next barrier is cold, including after a previously successful one.
    for(unsigned pass=0;pass<3;++pass){const auto ir=id_reads,fr=field_reads;C(cache->begin());
        C(cache->identity({1,99},capture,same)&&field(*cache,1,99));cache->end();C(id_reads==ir+1&&field_reads==fr+1);}
    p::Cache<Identity,1,1> small;C(small.begin());
    for(unsigned pass=0;pass<4;++pass)for(unsigned i=0;i<3;++i){C(small.identity({i+1,values[i]},capture,same));C(field(small,i+1,values[i]));}
    C(small.last.fallbacks>0);values[2]=77;C(!small.identity({3,33},capture,same));C(!field(small,3,33));small.end();
    C(cache->begin());unsigned expected=99;
    for(unsigned size:{0u,9u})C(!cache->field(1,size,reinterpret_cast<unsigned char const*>(&expected),read));
    C(!cache->field(1,4,nullptr,read));C(!cache->field(UINTPTR_MAX,4,reinterpret_cast<unsigned char const*>(&expected),read));
    C(!cache->identity({4,0},capture,same));C(!field(*cache,4,0));cache->end();
    p::Cache<Identity,256,256> collisions;unsigned bias{};
    auto colliding_capture=[&](std::uintptr_t address,Identity& actual){actual={address,unsigned(address/2048)+bias};return true;};
    auto colliding_read=[&](void* out,std::uintptr_t address,unsigned size){if(size!=4)return false;const auto value=unsigned(address/2048)+bias;std::memcpy(out,&value,4);return true;};
    C(collisions.begin());
    for(unsigned i=1;i<=65;++i){const auto address=std::uintptr_t(i)*2048;
        C(collisions.identity({address,i},colliding_capture,same));
        C(collisions.field(address,4,reinterpret_cast<unsigned char const*>(&i),colliding_read));
    }
    C(collisions.last.fallbacks==2);bias=1;unsigned expected_collision=65;
    C(!collisions.identity({65*2048,65},colliding_capture,same));
    C(!collisions.field(65*2048,4,reinterpret_cast<unsigned char const*>(&expected_collision),colliding_read));collisions.end();
    C(collisions.begin());C(!collisions.identity({2048,1},colliding_capture,same));collisions.end();
    C(cache->begin());const auto prior_fields=field_reads;unsigned exact=99;
    C(cache->field(1,1,reinterpret_cast<unsigned char const*>(&exact),read));C(field(*cache,1,99));
    C(field_reads==prior_fields+2);cache->end();
    cache->epoch=(std::numeric_limits<std::uint64_t>::max)();values[0]=55;
    C(cache->begin()&&cache->epoch==1);C(!cache->identity({1,99},capture,same));C(cache->identity({1,55},capture,same));C(field(*cache,1,55));cache->end();
    printf("ProofPass checks=%u failures=%u fresh distinct observations per pass; duplicates compared to actual, saturation rereads\n",checks,failures);return failures?1:0;
}
