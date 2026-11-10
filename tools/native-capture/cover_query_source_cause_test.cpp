#include "cover_query_source_cause.h"
#include <cstdio>
#include <thread>
namespace c=gears_cover_query_source_cause;
unsigned checks{},failures{};
#define C(v) do{++checks;if(!(v)){++failures;std::printf("FAIL %d %s\n",__LINE__,#v);}}while(false)
int main(){
    const c::Key key{0x1000,42,7,0};c::Witness first{};
    C(!c::armed());c::note(key,3,1);C(!first.observed);
    {c::Scope off(false,key,3,first);C(!c::armed());c::note(key,3,1);C(!first.observed);}
    for(unsigned field=0;field<3;++field){auto bad=key;
        if(field==0)bad.entry=0;if(field==1)bad.compose=0;if(field==2)bad.thread=0;
        c::Scope rejected(true,bad,3,first);C(!c::armed());}
    {c::Scope outer(true,key,3,first);C(c::armed());
        std::thread other_thread([&](){C(!c::armed());c::note(key,3,2);});other_thread.join();C(!first.observed);
        for(unsigned field=0;field<4;++field){auto other=key;
            if(field==0)other.entry++;if(field==1)other.compose++;if(field==2)other.thread++;if(field==3)other.stage++;
            c::note(other,3,1);C(!first.observed);}
        c::note(key,2,1);C(!first.observed);
        c::Witness nested{};auto different=key;different.compose++;
        {c::Scope inner(true,different,3,nested);c::note(key,3,1);C(!first.observed&&!nested.observed);
            c::note(different,3,2);C(nested.observed&&nested.causes==2);}
        c::note(key,3,1);C(first.observed&&first.causes==1&&c::same(first.key,key));
        c::note(key,3,2);C(first.causes==1);
        try{c::Witness unwound{};c::Scope inner(true,different,3,unwound);throw 1;}catch(int){}
        C(c::armed());c::note(key,3,3);C(first.causes==1);
    }
    C(!c::armed());
    // Entry address reuse in a later scope does not inherit an old witness.
    c::Witness later{};auto key2=key;key2.compose++;
    {c::Scope next(true,key2,3,later);c::note(key,3,2);C(!later.observed);c::note(key2,3,3);C(later.causes==3);}
    C(!c::armed());
    std::printf("query source cause: %u checks, %u failures; data-only observation\n",checks,failures);
    return failures?1:0;
}
