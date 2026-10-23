#include "independent_view_history.h"
#include <cassert>
#include <cstring>
#include <thread>

namespace {
std::uintptr_t object=0x1111;
void* result=&object;
unsigned factories{},releases{};
bool read_ok=true;
void* factory(){++factories;return result;}
bool read(void* out,const void* value,std::size_t size){
    if(!read_ok)return false;
    if(value!=&object||size!=sizeof(object))return false;
    std::memcpy(out,value,size);return true;
}
void release(void* value){assert(value==&object);++releases;}
}
int main(){
    using gears_view_history::State;
    State live;
    assert(live.retired()&&live.reset());
    assert(live.ensure(factory,read,0x1111,0x2000,0x3000));
    assert(live.ready()&&!live.retired()&&!live.reset());
    assert(live.ensure(factory,read,0x1111,0x2000,0x3000)&&factories==1);
    assert(!live.ensure(factory,read,0x1111,reinterpret_cast<std::uintptr_t>(&object),0x3000));
    assert(!live.begin_delete(reinterpret_cast<void*>(0x2000),1));
    live.release(release);live.release(release);
    assert(releases==1&&!live.ready()&&!live.retired()&&!live.reset());
    std::thread deletion([&]{assert(live.begin_delete(&object,1));live.complete_delete();});
    deletion.join();
    assert(live.retired()&&!live.begin_delete(&object,1)&&live.reset());
    assert(live.ensure(factory,read,0x1111,0x2000,0x3000)&&factories==2);
    live.release(release);assert(live.begin_delete(&object,1));live.complete_delete();assert(live.retired());

    // Unsafe factory returns must never be released as an owned state.
    for(unsigned fault=0;fault<4;++fault){
        State rejected;auto before_release=releases;auto before_factory=factories;
        result=fault==0?nullptr:fault==1?reinterpret_cast<void*>(0x2000):&object;
        read_ok=fault!=2;object=fault==3?0x2222:0x1111;
        assert(!rejected.ensure(factory,read,0x1111,0x2000,0x3000));
        assert(!rejected.ensure(factory,read,0x1111,0x2000,0x3000)&&factories==before_factory+1);
        rejected.release(release);
        assert(releases==before_release&&!rejected.ready()&&!rejected.retired()&&!rejected.reset());
    }
    // A destructor call without native delete completion/flag is not retirement.
    State no_free;result=&object;object=0x1111;read_ok=true;
    assert(no_free.ensure(factory,read,0x1111,0x2000,0x3000));no_free.release(release);
    assert(no_free.begin_delete(&object,0));no_free.complete_delete();assert(!no_free.retired()&&!no_free.reset());
    std::puts("Independent view-history ownership/deferred deletion/world reset checks passed");
}
