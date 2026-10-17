#define NOMINMAX
#define GEARS_PATH_CHOICE_POSITION_TRIAL 1
#include <windows.h>
#include <atomic>
#include <memory>
#include <cstdio>
#include <cassert>
#include <cstdint>
#include <thread>
#include <future>
#include <set>
static uintptr_t base=0x140000000;
static long long session_started_qpc=3;
static std::atomic<uint64_t> committed_world_generation{4};
static std::atomic<unsigned> reads{},stops{};
static bool safe_copy(void*,void const*,std::size_t){++reads;return false;}
namespace gears_native_metadata {inline bool matches(unsigned char const*,unsigned,char const*){return false;}}
enum class SessionStop {proof_failure};
static void request_session_stop(SessionStop){++stops;}
#include "path_choice_position_trial_live.inl"
using namespace gears_path_choice_position;
using namespace path_choice_position_live;
struct Candidate {uintptr_t client=0x1000,scene=0x2000;};
struct Engine {
    Config c;Context ctx;std::map<std::uintptr_t,unsigned char> memory;unsigned calls{};int fail_call{};bool mutate_on_failure{};
    Engine(){c.nonce=1;c.pid=1;c.creation_time=2;c.started_qpc=3;c.world_generation=4;c.base=0x140000000;c.client=0x1000;
        c.scene=0x2000;c.panel=0x3000;c.panel_vtable=0x4000;c.duration_ms=3000;c.top_bits=bits(.0374f);c.bottom_bits=bits(.9626f);
        ctx={1,2,3,4,c.base,c.client,c.scene};put(c.panel,c.panel_vtable);put(c.panel_vtable+0x3d8,c.base+0x2988d8);
        put(c.panel_vtable+0x3b8,c.base+0x297338);put(c.panel+0x70,std::uint32_t(0x04040404));
        put(c.panel+0x64,c.top_bits);put(c.panel+0x6c,c.bottom_bits);
        c.checks.push_back({0x5000,{0xaa}});memory[0x5000]=0xaa;}
    template<class T> void put(std::uintptr_t address,T v){auto p=reinterpret_cast<unsigned char const*>(&v);for(std::size_t i=0;i<sizeof(v);++i)memory[address+i]=p[i];}
    bool read(void* out,void const* from,std::size_t size){auto p=reinterpret_cast<std::uintptr_t>(from);auto q=static_cast<unsigned char*>(out);
        for(std::size_t i=0;i<size;++i){auto it=memory.find(p+i);if(it==memory.end())return false;q[i]=it->second;}return true;}
    auto reader(){return [this](auto out,auto from,auto size){return read(out,from,size);};}
    auto setter(){return [this](auto panel,float v,unsigned face,unsigned input,int origin){
        assert(panel==c.panel&&(face==1||face==3)&&input==0&&origin==0);++calls;
        bool fail=int(calls)==fail_call;if(!fail||mutate_on_failure)put(panel+0x60+face*4,bits(v));return !fail;};}
    std::array<std::uint32_t,2> pos(){std::array<std::uint32_t,2> p{};assert(positions(c,reader(),p));return p;}
};

static void reset(){disable();state={};owner_thread=0;in_flight=0;owner_activity=0;inside=false;restore_retries=0;drain_started=0;
    pending_restore=false;ever_modified=false;drain_requested=false;unresolved=false;terminal=false;
    loaded_nonce=0;applied=0;restored=0;rejected=0;identity_lost=0;ownership_lost=0;setter_failed=0;reads=0;stops=0;}
static std::shared_ptr<Request> select(Engine& e){auto ctx=context(e.c.client,e.c.scene);
    e.c.pid=ctx.pid;e.c.creation_time=ctx.creation_time;e.c.started_qpc=ctx.started_qpc;e.c.world_generation=ctx.world_generation;
    e.ctx=ctx;auto req=std::make_shared<Request>();req->config=e.c;req->expires=GetTickCount64()+8000;
    std::atomic_store(&selected,std::shared_ptr<Request const>(req));return req;}
static void gates(){reset();Candidate candidate;unsigned originals{};auto original=[&](){++originals;return uintptr_t(0xfedcba9876543210ull);};
    assert(invoke(candidate,original)==0xfedcba9876543210ull&&originals==1&&reads==0);
    auto req=std::make_shared<Request>();req->expires=GetTickCount64()+1000;req->config.nonce=1;
    req->config.client=0x9000;req->config.scene=candidate.scene;std::atomic_store(&selected,std::shared_ptr<Request const>(req));
    invoke(candidate,original);assert(originals==2&&reads==0);
    req->config.client=candidate.client;req->expires=0;invoke(candidate,original);assert(originals==3&&reads==0);
    req->expires=GetTickCount64()+1000;invoke(candidate,original);assert(originals==4&&reads==1&&rejected==1&&!std::atomic_load(&selected));
    state.lease.active=true;state.lease.config.world_generation=5;state.lease.touched[0]=true;
    invoke(candidate,original);assert(originals==5&&identity_lost==1&&stops==1&&reads==1&&unresolved&&terminal);
}
static void expiry_and_retry(){reset();Engine e;auto req=select(e);Candidate c;unsigned originals{};auto original=[&](){++originals;return uintptr_t(77);};
    auto cert=[](){return true;};auto before=e.pos();
    assert(invoke_with(c,original,e.reader(),e.setter(),cert)==77&&applied==1&&e.calls==2&&pending_restore);
    auto owned=e.pos();for(unsigned i=0;i<20;++i)invoke_with(c,original,e.reader(),e.setter(),cert);
    assert(e.pos()==owned&&e.calls==2&&originals==21);
    // First later-frame restore succeeds on Top but fails on Bottom. Keep the
    // known remaining ownership and leave hooks alive for a later retry.
    req->expires=0;e.fail_call=4;invoke_with(c,original,e.reader(),e.setter(),cert);
    assert(setter_failed==1&&pending_restore&&restore_retries==1&&stops==0&&!terminal&&e.calls==4);
    assert(e.pos()[0]==before[0]&&e.pos()[1]==owned[1]);
    e.fail_call=0;invoke_with(c,original,e.reader(),e.setter(),cert);
    assert(restored==1&&!pending_restore&&!unresolved&&e.pos()==before&&e.calls==5&&originals==23&&stops==0);
}
static void failed_apply_rollback(){reset();Engine e;select(e);Candidate c;unsigned originals{};auto before=e.pos();
    // Fail Bottom apply (2), then fail Top rollback (3); retain Top for retry.
    auto setter=[&](auto panel,float v,unsigned face,unsigned input,int origin){
        unsigned next=e.calls+1;e.fail_call=next==2||next==3?int(next):0;return e.setter()(panel,v,face,input,origin);};
    auto original=[&](){++originals;return uintptr_t(1);};auto cert=[](){return true;};
    invoke_with(c,original,e.reader(),setter,cert);
    assert(setter_failed==1&&pending_restore&&stops==0&&e.calls==3&&!terminal&&restore_retries==0);
    invoke_with(c,original,e.reader(),setter,cert);
    assert(restored==1&&!pending_restore&&stops==0&&e.calls==4&&e.pos()==before&&originals==2);
}
static void threaded_drain(){reset();Engine e;select(e);Candidate c;auto before=e.pos();
    std::promise<void> entered,release,applied_done,restore_now;auto release_f=release.get_future().share();auto restore_f=restore_now.get_future().share();
    std::atomic<unsigned> originals{},read_calls{},set_calls{};
    auto original=[&](){++originals;return uintptr_t(99);};
    auto reader=[&](auto out,auto from,auto size){++read_calls;return e.read(out,from,size);};
    auto setter=[&](auto panel,float v,unsigned face,unsigned input,int origin){++set_calls;
        if(set_calls==1){entered.set_value();release_f.wait();}return e.setter()(panel,v,face,input,origin);};
    auto cert=[](){return true;};
    std::thread owner([&]{assert(invoke_with(c,original,reader,setter,cert)==99);applied_done.set_value();restore_f.wait();
        assert(invoke_with(c,original,reader,setter,cert)==99);});
    entered.get_future().wait();auto stable_reads=read_calls.load();
    assert(invoke_with(c,original,reader,setter,cert)==99&&read_calls==stable_reads&&set_calls==1);
    request_drain();assert(draining()&&in_flight==1);
    drain_started=GetTickCount64()-2001;assert(draining()&&!terminal);drain_started=GetTickCount64();release.set_value();applied_done.get_future().wait();
    assert(pending_restore&&set_calls==2&&draining());stable_reads=read_calls.load();
    // Foreign thread cannot restore the owner lease, even during drain.
    assert(invoke_with(c,original,reader,setter,cert)==99&&read_calls==stable_reads&&set_calls==2&&pending_restore);
    restore_now.set_value();owner.join();
    assert(originals==4&&set_calls==4&&restored==1&&!draining()&&!pending_restore&&e.pos()==before&&!unresolved);
    // Permanent drain prevents another nonce from reopening mutation.
    ++e.c.nonce;select(e);invoke_with(c,original,reader,setter,cert);assert(set_calls==4);
}
static void bounded_drain(){reset();Engine e;select(e);Candidate c;auto original=[](){return uintptr_t(1);};auto cert=[](){return true;};
    invoke_with(c,original,e.reader(),e.setter(),cert);request_drain();assert(draining());
    drain_started=GetTickCount64()-2001;in_flight=1;assert(draining()&&!terminal);in_flight=0;
    assert(!draining()&&terminal&&unresolved&&pending_restore);auto calls=e.calls;
    invoke_with(c,original,e.reader(),e.setter(),cert);assert(e.calls==calls&&pending_restore);
    reset();Engine e2;select(e2);invoke_with(c,original,e2.reader(),e2.setter(),cert);disable();
    auto fail=[](auto,float,unsigned,unsigned,int){return false;};
    for(unsigned i=0;i<retry_limit;++i){invoke_with(c,original,e2.reader(),fail,cert);if(i+1<retry_limit)assert(stops==0&&!terminal&&pending_restore);}
    assert(stops==1&&terminal&&unresolved&&pending_restore&&!draining());
}
int main(){gates();expiry_and_retry();failed_apply_rollback();threaded_drain();bounded_drain();std::puts("PathChoicePositionTrialLiveTest passed");}
