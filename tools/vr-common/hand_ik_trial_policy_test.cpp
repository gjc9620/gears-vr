#include "hand_ik_trial_policy.h"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace gears_hand_ik;
int main(){
    assert(unload_allowed(true,false,0)&&unload_allowed(true,false,5));
    assert(!unload_allowed(true,true,0)&&!unload_allowed(true,true,7));
    for(const auto active:{1u,2u,3u,8u})assert(!unload_allowed(true,false,active));
    for(unsigned state=0;state<9;++state)assert(!unload_allowed(false,false,state));
    StabilityGate gate;
    for(unsigned i=0;i<7;++i)assert(!gate.observe(1000+i*40,true,true));
    assert(gate.observe(1280,true,true)); // Both 8 ticks and 250 ms required.
    gate.reset();
    for(unsigned i=0;i<8;++i)assert(!gate.observe(1000+i*10,true,true));
    assert(!gate.observe(1300,true,false)&&gate.ticks()==1);
    assert(!gate.observe(1400,false,true)&&gate.ticks()==0);
    for(unsigned i=0;i<20;++i)assert(!gate.observe(1500,true,true)); // Duplicate callbacks cannot pad ticks.
    assert(gate.ticks()==1);
    assert(!gate.observe(1499,true,true)&&gate.ticks()==1);
    OwnedWrite write;write.original_target={{999,888,777}};write.written_target={{1,2,3}};
    assert(write.can_restore({{trial_strength,trial_strength,0}},{{1,2,3}}));
    assert(!write.can_restore({{.100001f,trial_strength,0}},{{1,2,3}}));
    assert(!write.can_restore({{trial_strength,trial_strength,.001f}},{{1,2,3}}));
    assert(!write.can_restore({{trial_strength,trial_strength,0}},{{1,2,3.000001f}}));
    assert(!write.can_restore({{0,0,0}},{{1,2,3}}));
    assert(!write.can_restore({{trial_strength,trial_strength,0}},
        {{std::numeric_limits<float>::quiet_NaN(),2,3}}));
    assert(zero_strength({{0,0,0}})&&!zero_strength({{0,.001f,0}}));
    assert(pulse_in_time(1000,1400)&&!pulse_in_time(1000,1401)&&!pulse_in_time(1000,999));
    assert(directional_effect({{0,0,0}},{{.1f,0,0}},{{0,0,0}},{{0,0,0}},true));
    assert(!directional_effect({{0,0,0}},{{.1f,0,0}},{{0,0,0}},{{0,0,0}},false));
    assert(!directional_effect({{0,0,0}},{{.1f,0,0}},{{0,0,0}},{{.1f,0,0}},true));
    assert(!directional_effect({{0,0,0}},{{-.1f,0,0}},{{0,0,0}},{{0,0,0}},true));
    assert(returned({{1,2,3}},{{1.01f,2,3}})&&!returned({{1,2,3}},{{1.1f,2,3}}));
    std::puts("Hand IK stability, compare-before-restore and inconclusive policy passed");
}
