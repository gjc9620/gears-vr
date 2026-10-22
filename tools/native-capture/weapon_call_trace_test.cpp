#include "weapon_call_trace.h"
#include <cassert>
#include <cstring>
#include <map>
#include <cstdio>

int main(){
    using namespace gears_weapon_call_trace;
    constexpr std::uintptr_t tick=0x1000,wrapper=0x2000,fire=0x3000,child=0x4000;
    constexpr std::uintptr_t tick_fn=0x9000,fire_fn=0xa000,weapon=0xb000,controller=0xc000;
    std::map<std::uintptr_t,unsigned char> bytes;
    auto write=[&](std::uintptr_t at,std::uintptr_t value){
        for(unsigned i=0;i<sizeof(value);++i)bytes[at+i]=static_cast<unsigned char>(value>>(i*8));
    };
    auto set=[&](std::uintptr_t at,std::uintptr_t node,std::uintptr_t object,
                 std::uintptr_t code,std::uintptr_t previous){
        write(at+0x18,node);write(at+0x20,object);write(at+0x28,code);write(at+0x38,previous);
    };
    auto read=[&](void* out,void const* at,std::size_t size){
        auto address=reinterpret_cast<std::uintptr_t>(at);
        for(unsigned i=0;i<size;++i){auto found=bytes.find(address+i);
            if(found==bytes.end())return false;
            static_cast<unsigned char*>(out)[i]=found->second;
        }return true;
    };
    set(tick,tick_fn,controller,0x9100,0);
    set(wrapper,0xd000,weapon,0xd100,tick);
    set(fire,fire_fn,weapon,0xa100,wrapper);
    set(child,0xe000,weapon,0xe100,fire);
    auto accepted=[&]{return fire_body_ancestry(read,child,wrapper,fire_fn,weapon,
        0xa000,0x200,tick,tick_fn,controller,0x9000,0x200);};
    assert(tick_ancestry(read,wrapper,tick,tick_fn,controller,0x9000,0x200));
    assert(accepted());
    assert(direct_caller(read,fire,fire_fn,weapon,wrapper,0xa000,0x200));
    assert(!direct_caller(read,fire,fire_fn,weapon,tick,0xa000,0x200));
    assert(!direct_caller(read,fire,fire_fn,controller,wrapper,0xa000,0x200));
    assert(!direct_caller(read,child,fire_fn,weapon,fire,0xa000,0x200));
    assert(!fire_body_ancestry(read,wrapper,wrapper,fire_fn,weapon,
        0xa000,0x200,tick,tick_fn,controller,0x9000,0x200));
    set(fire,fire_fn,weapon,0xa100,tick);assert(!accepted());
    set(fire,fire_fn,weapon,0xa100,wrapper);
    set(fire,fire_fn,controller,0xa100,wrapper);assert(!accepted());
    set(fire,fire_fn,weapon,0xa300,wrapper);assert(!accepted());
    set(fire,fire_fn,weapon,0xa100,wrapper);
    set(child,0xe000,weapon,0xe100,child);assert(!accepted());
    set(child,0xe000,weapon,0xe100,fire);
    set(wrapper,0xd000,weapon,0xd100,0xdead);assert(!accepted());
    std::puts("weapon call fire-body ancestry gates passed");
}
