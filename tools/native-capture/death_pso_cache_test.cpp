#include "death_pso_cache.h"
#include <cassert>
#include <cstdio>
int main(){
    using namespace gears_death_pso;
    assert(selected(true,true,true,true,true,7,7,49));
    assert(!selected(false,true,true,true,true,7,7,49));
    assert(!selected(true,true,true,true,true,7,8,49));
    assert(!selected(true,true,true,true,true,7,7,3));
    Bank bank;auto index=bank.slot(100);assert(!bank.current(100,1));
    bank.witness(index,1,200,10);assert(bank.current(100,1)&&bank.current(100,1)->witness_qpc==10);
    assert(!bank.current(100,0)&&!bank.current(100,2));
    for(unsigned reason=0;reason<3;++reason){ // Successful Reset, ClearState, ExecuteBundle all invalidate.
        bank.invalidate(100);assert(!bank.current(100,1));bank.witness(index,1,200,11);
    }
    bank.witness(index,2,201,12);assert(!bank.current(100,1)&&bank.current(100,2));
    bank.invalidate(0);assert(!bank.current(100,2));
    for(uintptr_t i=1;i<list_limit;++i)assert(bank.slot(100+i)<list_limit);
    assert(bank.slot(999)==list_limit&&bank.overflow==1&&bank.capacity_lost);
    bank.witness(index,2,201,13);assert(!bank.current(100,2));
    Captures captures;assert(captures.admit(200)==0&&captures.admit(200)==0&&captures.count==1);
    assert(captures.admit(201)==1&&captures.admit(202)==pso_limit&&captures.overflow==1);
    assert(!save_size(0)&&save_size(blob_limit)&&!save_size(blob_limit+1));
    std::puts("DeathPsoCacheTest passed");
}
