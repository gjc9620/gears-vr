#include <Windows.h>
#include "native_scene_boundary.h"
#include <atomic>
#include <cstdio>
#include <thread>

namespace ctx=gears_native_context;
static unsigned checks{},failures{};
#define C(value) do{++checks;if(!(value)){++failures;std::printf("FAIL line %u\n",unsigned(__LINE__));}}while(false)
static unsigned calls{},mode{};
static ctx::SceneArguments expected{};
static void* result=reinterpret_cast<void*>(static_cast<std::uintptr_t>(0xfedcba9876543210ull));
static void* body(ctx::SceneArguments const& args){
    C(args.local==expected.local&&args.family==expected.family&&args.position==expected.position&&
      args.rotation==expected.rotation&&args.viewport==expected.viewport&&args.drawer==expected.drawer);
    C(ctx::thread_ledger.depth[unsigned(ctx::Kind::scene)]==calls+1);
    C(!ctx::engine_context_clear(ctx::thread_ledger,true));++calls;
    if(mode==1)RaiseException(0xe042c060,0,0,nullptr);
    if(mode==2&&calls==1){const auto saved=calls;C(ctx::invoke_scene(body,args)==result);C(calls==saved+1);
        C(ctx::thread_ledger.depth[unsigned(ctx::Kind::scene)]==1);}
    return result;
}
static unsigned caught_scene_exception(){
    __try {(void)ctx::invoke_scene(body,expected);return 0;}
    __except(EXCEPTION_EXECUTE_HANDLER){return GetExceptionCode();}
}
static void ledger_checks(){
    for(unsigned k=0;k<unsigned(ctx::Kind::count);++k){
        ctx::Ledger ledger{};ctx::Ticket outer{},inner{};
        C(ctx::engine_context_clear(ledger,true));C(!ctx::engine_context_clear(ledger,false));
        C(ctx::enter(ledger,outer,ctx::Kind(k)));C(!ctx::engine_context_clear(ledger,true));
        C(ctx::enter(ledger,inner,ctx::Kind(k)));C(ledger.depth[k]==2);
        C(ctx::leave(ledger,inner));C(ledger.depth[k]==1);C(ctx::leave(ledger,outer));
        C(ctx::engine_context_clear(ledger,true));
        C(!ctx::enter(ledger,outer,ctx::Kind(k)));C(ledger.poisoned);
    }
    ctx::Ledger ledger{},foreign{};ctx::Ticket a{},b{};
    C(ctx::enter(ledger,a,ctx::Kind::scene));C(ctx::enter(ledger,b,ctx::Kind::callback));
    C(!ctx::leave(ledger,a)&&ledger.poisoned&&a.active&&b.active);
    auto copy=b;C(!ctx::leave(ledger,copy)&&ledger.top==&b);
    C(!ctx::leave(foreign,b)&&foreign.poisoned&&b.active);
    C(ctx::leave(ledger,b));C(ctx::leave(ledger,a));C(!ledger.top);
    C(!ctx::engine_context_clear(ledger,true));
    ctx::Ledger overflow{};overflow.next=(std::numeric_limits<std::uint64_t>::max)();ctx::Ticket c{};
    C(!ctx::enter(overflow,c,ctx::Kind::scene)&&overflow.poisoned&&!c.active&&!overflow.top);
    ctx::Ledger invalid{};C(!ctx::enter(invalid,c,ctx::Kind::count)&&invalid.poisoned&&!c.active);
    ctx::Ledger saturated{};saturated.depth[0]=(std::numeric_limits<unsigned>::max)();
    C(!ctx::enter(saturated,c,ctx::Kind::scene)&&saturated.poisoned&&!c.active);
}
static void mutex_checks(){
    ctx::thread_ledger={};ctx::StereoMutex mutex;std::atomic<unsigned> phase{};std::atomic<bool> okay{true};
    mutex.lock();C(ctx::thread_ledger.depth[unsigned(ctx::Kind::stereo)]==1);
    C(!ctx::engine_context_clear(ctx::thread_ledger,true));
    std::thread other([&]{
        if(!ctx::engine_context_clear(ctx::thread_ledger,true)||mutex.try_lock()||ctx::thread_ledger.top)okay=false;
        phase.store(1);while(phase.load()!=2)std::this_thread::yield();
        {std::unique_lock<ctx::StereoMutex> lock(mutex,std::try_to_lock);
            if(!lock.owns_lock()||ctx::engine_context_clear(ctx::thread_ledger,true)||
               ctx::thread_ledger.depth[unsigned(ctx::Kind::stereo)]!=1)okay=false;}
        if(!ctx::engine_context_clear(ctx::thread_ledger,true))okay=false;
    });
    while(phase.load()!=1)std::this_thread::yield();mutex.unlock();phase.store(2);other.join();
    C(okay.load());C(ctx::engine_context_clear(ctx::thread_ledger,true));
    {std::lock_guard<ctx::StereoMutex> lock(mutex);C(!ctx::engine_context_clear(ctx::thread_ledger,true));}
    C(ctx::engine_context_clear(ctx::thread_ledger,true));
}
int main(){
    ledger_checks();mutex_checks();
    expected={reinterpret_cast<void*>(1),reinterpret_cast<void*>(2),reinterpret_cast<void*>(3),
              reinterpret_cast<void*>(4),reinterpret_cast<void*>(5),reinterpret_cast<void*>(6)};
    for(mode=0;mode<3;++mode){
        ctx::thread_ledger={};calls=0;
        if(mode==1)C(caught_scene_exception()==0xe042c060);else C(ctx::invoke_scene(body,expected)==result);
        C(calls==(mode==2?2u:1u));C(ctx::engine_context_clear(ctx::thread_ledger,true));
    }
    std::printf("native_action_context: %u checks, %u failures; real mutex ownership, nested scene and SEH cleanup\n",checks,failures);
    return failures?1:0;
}
