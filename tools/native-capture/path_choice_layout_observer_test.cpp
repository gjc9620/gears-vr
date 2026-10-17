// Offline execution of the actual wrapper against synthetic memory. No game/DLL.
#define NOMINMAX
#define GEARS_PATH_CHOICE_POSITION_TRIAL 1
#define GEARS_UI_SCENE_ROOT_OBSERVER 1
#define GEARS_PATH_CHOICE_LAYOUT_OBSERVER 1
#include <windows.h>
#include <intrin.h>
#include <atomic>
#include <memory>
#include <cstdio>
#include <cassert>
#include <cstdint>
#include <thread>
#include <future>
#include <map>
#include <string>
#include "native_metadata_contract.h"
#pragma comment(lib,"bcrypt.lib")
static uintptr_t base=0x140000000;
static long long session_started_qpc=3;
static std::atomic<uint64_t> committed_world_generation{4};
static std::atomic<bool> recording{true};
static bool ui_observer_enabled=true,admitted=true;
static thread_local unsigned stereo_publish_bypass{};
static std::atomic<unsigned> active{},read_calls{};
struct Scope {Scope(){++active;}~Scope(){--active;}bool world()const{return admitted;}};
static std::map<uintptr_t,unsigned char> memory;
static bool safe_copy(void* out,void const* from,std::size_t size){
    ++read_calls;auto address=reinterpret_cast<uintptr_t>(from);auto target=static_cast<unsigned char*>(out);
    for(std::size_t i=0;i<size;++i){auto it=memory.find(address+i);if(it==memory.end())return false;target[i]=it->second;}return true;
}
template<class T> static void put(uintptr_t address,T const& value){auto p=reinterpret_cast<unsigned char const*>(&value);
    for(std::size_t i=0;i<sizeof(value);++i)memory[address+i]=p[i];}
enum class SessionStop {proof_failure};
static void request_session_stop(SessionStop){}
#include "path_choice_position_trial_live.inl"
#include "path_choice_layout_observer.inl"
namespace observer=path_choice_layout_observer;
namespace position=path_choice_position_live;
namespace trial=gears_path_choice_position;
static std::atomic<unsigned> original_calls{};
static void* expected_object{};static void const* expected_input{};
static unsigned mode{},nested{};
static std::promise<void>* entered{},*release_original{};
static void __fastcall original(void* object,void const* input){
    ++original_calls;assert(object==expected_object&&input==expected_input);
    if(mode==1){auto const& c=position::state.lease.config;put(c.panel+0x64,c.top_bits);put(c.panel+0x6c,c.bottom_bits);}
    if(mode==2&&!nested){++nested;observer::hook(object,input);--nested;}
    if(mode==3)throw 71;
    if(mode==4){entered->set_value();release_original->get_future().wait();}
    if(mode==5){auto const& c=position::state.lease.config;put(c.panel+0x64,c.top_bits);put(c.panel+0x6c,c.bottom_bits);
        position::publish_loss(c,c.world_generation,{c.top_bits,c.bottom_bits},3,true,safe_copy);++position::ownership_lost;}
}
struct Fixture {
    trial::Config c;std::array<std::uint32_t,3> input{0x12345678,0xabcdef01,0x87654321};
    Fixture(){
        memory.clear();original_calls=0;read_calls=0;mode=0;nested=0;observer::depth=0;observer::slots=0;
        for(unsigned i=0;i<observer::row_limit;++i){observer::rows[i]={};observer::ready[i]=false;}
        observer::original=original;recording=true;ui_observer_enabled=true;admitted=true;stereo_publish_bypass=0;
        position::disable();position::state={};position::owner_thread=GetCurrentThreadId();position::owner_activity=0;position::inside=false;
        position::terminal=false;position::drain_requested=false;position::pending_restore=false;position::in_flight=0;
        position::applied=0;position::successful_nonce=0;position::ownership_lost=0;position::loss_ready=false;position::loss_claimed=false;position::first_loss={};
        committed_world_generation=4;
        c.nonce=1;c.panel=0x3000;c.client=0x1000;c.scene=0x2000;c.panel_vtable=base+observer::vtable_rva;
        auto ctx=position::context(c.client,c.scene);c.base=ctx.base;c.pid=ctx.pid;c.creation_time=ctx.creation_time;
        c.started_qpc=ctx.started_qpc;c.world_generation=ctx.world_generation;c.duration_ms=3000;
        c.top_bits=trial::bits(.0374f);c.bottom_bits=trial::bits(.9626f);
        c.checks.push_back({0x5000,{0xab}});memory[0x5000]=0xab;
        put(c.panel,c.panel_vtable);put(c.panel_vtable+0x3d8,base+0x2988d8);put(c.panel_vtable+0x3b8,base+0x297338);
        put(c.panel_vtable+observer::slot_offset,base+observer::code_rva);put(c.panel+0x70,std::uint32_t(0x04040404));
        put(c.panel+0x64,c.top_bits);put(c.panel+0x6c,c.bottom_bits);put(c.panel+0x3e8,std::array<std::uint32_t,3>{7,8,9});
        expected_object=reinterpret_cast<void*>(c.panel);expected_input=input.data();put(reinterpret_cast<uintptr_t>(input.data()),input);
    }
    auto setter(){return [&](auto panel,float value,unsigned face,unsigned,unsigned){put(panel+0x60+face*4,trial::bits(value));return true;};}
    void apply(){auto ctx=position::context(c.client,c.scene);
        assert(trial::apply(position::state,c,ctx,safe_copy,setter())==trial::Result::applied);position::count(trial::Result::applied);}
    void call(){observer::hook(expected_object,expected_input);}
};
static void call_subtree(){
    Fixture f;auto req=std::make_shared<position::Request>();req->config=f.c;req->expires=GetTickCount64()+7000;
    std::atomic_store(&position::selected,std::shared_ptr<position::Request const>(req));
    struct Candidate{uintptr_t client=0x1000,scene=0x2000;} candidate;
    mode=1;const auto input=f.input;
    assert(position::invoke_with(candidate,[&]{f.call();return uintptr_t(123);},safe_copy,f.setter(),[]{return true;})==123);
    assert(original_calls==1&&observer::ready[0].load(std::memory_order_acquire));
    auto const& r=observer::rows[0];assert(r.normal&&r.depth==1&&r.caller&&r.thread==GetCurrentThreadId());
    assert(r.before.identity&&r.after.identity&&r.before.mask==15&&r.after.mask==15&&r.input_readable&&r.input==input&&f.input==input);
    assert(r.before.position==position::state.lease.owned&&r.after.position[0]==f.c.top_bits&&r.after.position[1]==f.c.bottom_bits);
    assert((r.before.stored==std::array<std::uint32_t,3>{7,8,9})&&r.before.stored==r.after.stored&&r.before.applied==1);
    assert(r.before.units==0x04040404&&r.before.world==4&&r.exit>=r.enter&&active==0&&observer::depth==0);
}
static void gates(){
    Fixture f;f.call();assert(original_calls==1&&observer::slots==0);f.apply();
    auto bypass=[&]{auto reads=read_calls.load();f.call();assert(observer::slots==0&&read_calls==reads);};
    ++position::state.lease.config.nonce;bypass();--position::state.lease.config.nonce;
    std::thread foreign([&]{bypass();});foreign.join();
    expected_object=reinterpret_cast<void*>(f.c.panel+8);bypass();expected_object=reinterpret_cast<void*>(f.c.panel);
    recording=false;bypass();recording=true;ui_observer_enabled=false;bypass();ui_observer_enabled=true;
    admitted=false;bypass();admitted=true;stereo_publish_bypass=1;bypass();stereo_publish_bypass=0;
    auto denied=[&]{f.call();assert(observer::slots==0);};
    ++committed_world_generation;denied();--committed_world_generation;
    ++session_started_qpc;denied();--session_started_qpc;
    memory[0x5000]^=1;denied();memory[0x5000]^=1;
    put(f.c.panel,f.c.panel_vtable+8);denied();put(f.c.panel,f.c.panel_vtable);
    put(f.c.panel_vtable+observer::slot_offset,base+observer::code_rva+1);denied();put(f.c.panel_vtable+observer::slot_offset,base+observer::code_rva);
    f.call();assert(observer::slots==1&&observer::ready[0]);
}
static void bounds_nesting_exceptions(){
    Fixture f;f.apply();mode=2;f.call();assert(original_calls==2&&observer::slots==2&&observer::depth==0);
    assert(observer::rows[0].depth==1&&observer::rows[1].depth==2&&observer::ready[0]&&observer::ready[1]);
    mode=3;try{f.call();assert(false);}catch(int n){assert(n==71);}
    assert(original_calls==3&&observer::depth==0&&observer::ready[2]&&!observer::rows[2].normal&&active==0);
    mode=0;memory.erase(f.c.panel+0x6c);f.call();assert(observer::ready[3]&&observer::rows[3].before.mask==11);
    auto reads=read_calls.load();for(unsigned i=0;i<10;++i)f.call();
    assert(original_calls==14&&observer::slots==4&&read_calls==reads&&observer::depth==0);
}
static std::string progress(){FILE* file{};assert(tmpfile_s(&file)==0&&file);observer::progress(file);rewind(file);
    std::string text;char buffer[512];while(auto n=fread(buffer,1,sizeof(buffer),file))text.append(buffer,n);fclose(file);return text;}
static void publication(){
    Fixture f;f.apply();mode=4;std::promise<void> start,release;entered=&start;release_original=&release;
    std::thread owner([&]{position::owner_thread=GetCurrentThreadId();f.call();});
    start.get_future().wait();auto text=progress();assert(text.find("path_layout_claimed=1")!=std::string::npos&&text.find("path_layout_0=")==std::string::npos);
    release.set_value();owner.join();text=progress();assert(text.find("path_layout_0=")!=std::string::npos&&observer::ready[0]);
}
static void loss_timeline(){
    Fixture f;f.apply();mode=5;f.call();auto const& r=observer::rows[0];
    assert(!r.before.loss_ready&&r.before.ownership_lost==0&&r.after.loss_ready&&r.after.ownership_lost==1);
    assert(r.after.loss_tick==position::first_loss.tick&&r.after.position[0]==f.c.top_bits&&r.after.position[1]==f.c.bottom_bits);
    mode=0;f.call();assert(observer::rows[1].before.loss_ready&&observer::rows[1].before.loss_tick==r.after.loss_tick);
}
static void complete_certification(){
    Fixture f;std::array<unsigned char,161> body{};for(unsigned i=0;i<body.size();++i)body[i]=static_cast<unsigned char>(i);
    put(base+observer::code_rva,body);unsigned hash_calls{};
    auto hash=[&](unsigned char const* data,unsigned n,char const* sha){++hash_calls;
        assert(n==161&&std::string(sha)==observer::code_sha256);return std::equal(body.begin(),body.end(),data);};
    assert(observer::certify_with(safe_copy,hash)&&hash_calls==1);
    memory[base+observer::code_rva+160]^=1;assert(!observer::certify_with(safe_copy,hash));memory[base+observer::code_rva+160]^=1;
    memory.erase(base+observer::code_rva+160);assert(!observer::certify_with(safe_copy,hash));put(base+observer::code_rva,body);
    put(f.c.panel_vtable+observer::slot_offset,base+observer::code_rva+1);assert(!observer::certify_with(safe_copy,hash));
    put(f.c.panel_vtable+observer::slot_offset,base+observer::code_rva);
    unsigned code_reads{};auto changing=[&](void* out,void const* from,std::size_t n){bool ok=safe_copy(out,from,n);
        if(ok&&reinterpret_cast<uintptr_t>(from)==base+observer::code_rva&&++code_reads==2)static_cast<unsigned char*>(out)[160]^=1;return ok;};
    assert(!observer::certify_with(changing,hash));
    unsigned slot_reads{};auto slot_changing=[&](void* out,void const* from,std::size_t n){bool ok=safe_copy(out,from,n);
        if(ok&&reinterpret_cast<uintptr_t>(from)==base+observer::vtable_rva+observer::slot_offset&&++slot_reads==2)static_cast<unsigned char*>(out)[0]^=1;return ok;};
    assert(!observer::certify_with(slot_changing,hash));
}
int main(){call_subtree();gates();bounds_nesting_exceptions();publication();loss_timeline();complete_certification();std::puts("PathChoiceLayoutObserverTest passed");}
