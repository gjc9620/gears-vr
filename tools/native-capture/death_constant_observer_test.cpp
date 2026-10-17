#include "death_constant_observer.h"
#include <cassert>
#include <cstdio>
#include <limits>
int main(){
    using namespace gears_death_constants;
    assert(role(0xd80739)==0&&role(0xd80925)==1&&role(0xd8084e)==2);
    assert(owner(0x100b2,0)==0x10000&&owner(0x100aa,1)==0x10000);
    assert(!owner(0xb0,0)&&!owner(0x100b3,0)&&!owner(0x100b2,2));
    assert(eligible(true,true,true,true,true,7,7,49,0xd80739,0));
    assert(!eligible(false,true,true,true,true,7,7,49,0xd80739,0));
    assert(!eligible(true,true,true,true,true,7,8,49,0xd80739,0));
    assert(!eligible(true,true,true,true,true,7,7,3,0xd80739,0));
    assert(!eligible(true,true,true,true,true,7,7,49,0xd80739,1));
    uintptr_t address{};Descriptor descriptor{15,32,16};assert(destination(descriptor,0,0x10000,address)&&address==0x10020);
    assert(!destination(descriptor,1,0x10000,address));descriptor[0]=16;assert(!destination(descriptor,0,0x10000,address));
    descriptor={0,32,15};assert(!destination(descriptor,0,0x10000,address));descriptor[2]=16;
    assert(!destination(descriptor,0,UINTPTR_MAX-20,address));
    Float4 values{1,2,3,4};assert(finite(values)&&equal(values,values));values[0]=std::numeric_limits<float>::infinity();assert(!finite(values));
    Recorder rows;for(unsigned eye=0;eye<2;++eye)for(unsigned kind=0;kind<2;++kind)assert(rows.reserve(eye,kind));
    assert(!rows.reserve(0,0)&&rows.duplicate_setters==1&&!rows.reserve(2,0)&&rows.rejected==1);
    Destination before{},after{};before.address=after.address=0x1000;assert(before.same_path(after));after.data=1;assert(!before.same_path(after));
    std::puts("DeathConstantObserverTest passed");
}
