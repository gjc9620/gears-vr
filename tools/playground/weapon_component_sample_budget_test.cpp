#include "weapon_component_sample_budget.h"
#include <cassert>
#include <cstdio>
#include <limits>
#ifdef NDEBUG
#error Active checks required
#endif
int main(){
    namespace s=gears_weapon_component_samples;using gears_vr::Pose;
    unsigned checks{},attempt{};
#define CHECK(x) do{++checks;assert(x);}while(false)
    const auto identity=[](std::uint64_t generation){s::Identity i{};i.key={1,2,3,4,5,6,7,8};i.hold_generation=generation;return i;};
    auto id=identity(1);Pose grip{};s::Budget budget;
    for(unsigned i=1;i<=3;++i){CHECK(budget.claim(id,grip,attempt)&&attempt==i);++id.key.frame;}
    CHECK(!budget.claim(id,grip,attempt)&&attempt==0); // Frame advances do not reset a still input's budget.
    auto q=grip;q.orientation.w=-1;CHECK(!budget.claim(id,q,attempt));
    q.position.x=.004;CHECK(!budget.claim(id,q,attempt));q.position.x=.006;CHECK(budget.claim(id,q,attempt)&&attempt==4);
    for(unsigned i=5;i<=6;++i)CHECK(budget.claim(id,q,attempt)&&attempt==i);
    CHECK(!budget.claim(id,q,attempt));const auto h=std::sqrt(.5);q.orientation={0,0,h,h};
    CHECK(budget.claim(id,q,attempt)&&attempt==7); // Rotation alone re-arms sampling.
    for(unsigned i=8;i<=24;++i){q.position.x+=.01;CHECK(budget.claim(id,q,attempt)&&attempt==i);}
    q.position.x+=1;CHECK(!budget.claim(id,q,attempt)&&attempt==0);
    for(unsigned i=2;i<=8;++i)CHECK(budget.claim(identity(i),grip,attempt)&&attempt==1);
    CHECK(!budget.claim(identity(9),grip,attempt)); // Eight identities are a hard cap, no eviction/retry loop.
    s::Budget fresh;q=grip;q.position.x=std::numeric_limits<double>::quiet_NaN();CHECK(!fresh.claim(id,q,attempt));
    q=grip;q.orientation={0,0,0,0};CHECK(!fresh.claim(id,q,attempt));CHECK(!fresh.claim(identity(0),grip,attempt));
    CHECK(fresh.claim(id,grip,attempt)&&attempt==1);
    {s::Budget epochs;auto a=identity(1);for(unsigned i=0;i<3;++i)CHECK(epochs.claim(a,grip,attempt));
        CHECK(!epochs.claim(a,grip,attempt));++a.reference_generation;CHECK(epochs.claim(a,grip,attempt)&&attempt==1);
        ++a.input_epoch;CHECK(epochs.claim(a,grip,attempt)&&attempt==1);}
    std::printf("WeaponComponentSampleBudgetTest: %u checks; bounded observation policy, no tracking/equipment permit\n",checks);
}
