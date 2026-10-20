#include "death_dof_control.h"
#include <cassert>
#include <cstdio>
#include <limits>
int main(){using namespace gears_death_dof;
    Gate g{true,true,true,true,true,true,7,7,3,3,0xd803bf,0x1000,0,1};Descriptor d{0,192,16};Float4 value{1,-0.0f,2,3};
    assert(gate(g,d,value)==accepted);auto out=forwarded(value,true);assert(out[0]==0&&value[0]==1);
    assert(std::memcmp(out.data()+1,value.data()+1,12)==0&&equal(forwarded(value,false),value));
    g.enabled=false;assert(gate(g,d,value)==configuration);g.enabled=true;
    g.frozen_world=0;assert(gate(g,d,value)==world);g.frozen_world=4;assert(gate(g,d,value)==world);g.frozen_world=3;
    g.frozen_ticket=8;assert(gate(g,d,value)==ticket);g.frozen_ticket=7;
    g.caller=0xd803c0;assert(gate(g,d,value)==caller);g.caller=0xd803bf;
    g.index=1;assert(gate(g,d,value)==index);g.index=0;g.native_valid=false;assert(gate(g,d,value)==view);g.native_valid=true;
    d[1]=191;assert(gate(g,d,value)==descriptor);d[1]=192;value[0]=0;assert(gate(g,d,value)==source);value[0]=1;
    value[3]=std::numeric_limits<float>::infinity();assert(gate(g,d,value)==source);value[3]=3;
    assert(descriptor_owner(0x10fe)==0x1000&&!descriptor_owner(0xfd)&&!descriptor_owner(0x10ff));
    Recorder recorder;assert(recorder.bind(3,0x1000)&&!recorder.bind(4,0x1000)&&!recorder.bind(3,0x2000));unsigned slot{};
    for(unsigned i=0;i<limit;++i)assert(recorder.admit(3,i+1,slot)==accepted&&slot==i);
    assert(recorder.admit(3,1,slot)==duplicate&&recorder.find(3,1)->duplicates==1);
    assert(recorder.admit(3,limit+1,slot)==capacity&&recorder.overflow==1&&recorder.count==limit);
    std::puts("DeathDofControlTest passed");
}
