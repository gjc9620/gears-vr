// Offline execution of the actual wrapper against synthetic memory. No game/DLL.
#define NOMINMAX
#define GEARS_PATH_CHOICE_POSITION_TRIAL 1
#define GEARS_UI_SCENE_ROOT_OBSERVER 1
#define GEARS_PATH_CHOICE_LAYOUT_OBSERVER 1
#include "path_choice_position_trial.h"
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
static std::atomic<unsigned> active{},read_calls{},array_reads{},session_stop_calls{};
struct Scope {Scope(){++active;}~Scope(){--active;}bool world()const{return admitted;}};
static std::map<uintptr_t,unsigned char> memory;
static bool safe_copy(void* out,void const* from,std::size_t size){
    ++read_calls;auto address=reinterpret_cast<uintptr_t>(from);auto target=static_cast<unsigned char*>(out);
    if(address>=0x6000&&address<0x6100)++array_reads;
    for(std::size_t i=0;i<size;++i){auto it=memory.find(address+i);if(it==memory.end())return false;target[i]=it->second;}return true;
}
template<class T> static void put(uintptr_t address,T const& value){auto p=reinterpret_cast<unsigned char const*>(&value);
    for(std::size_t i=0;i<sizeof(value);++i)memory[address+i]=p[i];}
enum class SessionStop {proof_failure};
[[maybe_unused]] static void request_session_stop(SessionStop){++session_stop_calls;}
#include "path_choice_position_trial_live.inl"
#include "path_choice_layout_observer.inl"
namespace path_choice_layout_observer {bool install_child_hook(){return true;}}
namespace observer=path_choice_layout_observer;
namespace position=path_choice_position_live;
namespace trial=gears_path_choice_position;
static std::atomic<unsigned> original_calls{};
static void* expected_object{};
static unsigned mode{},nested{};
static constexpr uintptr_t sentinel=0xfedcba9876543210ull;
static constexpr uintptr_t child_sentinel=0x87654321fedcba98ull;
static std::atomic<unsigned> child_original_calls{};
static bool run_child_in_parent{};static unsigned child_mode{},child_repetitions=1,child_nested{};
static std::array<std::uint32_t,4> child_input{0x7fc12345,0x80000000,1,0xc2354321};
static unsigned char child_input_type=0xfe;static int child_origin=-123456789,child_sort=0x76543210;
static uintptr_t call_child();
static std::promise<void>* entered{},*release_original{};
static uintptr_t __fastcall original(void* object){
    ++original_calls;assert(object==expected_object);
    if(mode==1){auto const& c=position::state.lease.config;put(c.panel+0x64,c.top_bits);put(c.panel+0x6c,c.bottom_bits);}
    if(mode==2&&!nested){++nested;assert(observer::hook(object)==sentinel);--nested;}
    if(mode==3)throw 71;
    if(mode==4){entered->set_value();release_original->get_future().wait();}
    if(mode==5){auto const& c=position::state.lease.config;put(c.panel+0x64,c.top_bits);put(c.panel+0x6c,c.bottom_bits);
        position::publish_loss(c,c.world_generation,{c.top_bits,c.bottom_bits},3,true,safe_copy);++position::ownership_lost;}
    if(mode==6)++committed_world_generation;
    if(run_child_in_parent)for(unsigned i=0;i<child_repetitions;++i)assert(call_child()==child_sentinel);
    if(mode==7){entered->set_value();release_original->get_future().wait();}
    return sentinel;
}
static uintptr_t __fastcall child_original(void* object,float left,float top,float right,float bottom,unsigned char input_type,int origin,int sort){
    ++child_original_calls;assert(object==expected_object);
    assert((std::array<std::uint32_t,4>{trial::bits(left),trial::bits(top),trial::bits(right),trial::bits(bottom)}==child_input));
    assert(input_type==child_input_type&&origin==child_origin&&sort==child_sort);
    if(child_mode==1)throw 93;
    if(child_mode==2&&!child_nested){++child_nested;assert(call_child()==child_sentinel);--child_nested;}
    if(child_mode==3){entered->set_value();release_original->get_future().wait();}
    if(child_mode==4){auto const& c=position::state.lease.config;put(c.panel+0x64,c.top_bits);put(c.panel+0x6c,c.bottom_bits);}
    return child_sentinel;
}
static uintptr_t call_child(){return observer::child_hook(expected_object,trial::value(child_input[0]),trial::value(child_input[1]),
    trial::value(child_input[2]),trial::value(child_input[3]),child_input_type,child_origin,child_sort);}
struct Fixture {
    trial::Config c;
    Fixture(){
        memory.clear();original_calls=0;read_calls=0;array_reads=0;mode=0;nested=0;observer::depth=0;observer::slots=0;observer::entries=0;
        observer::parent_scope=nullptr;observer::child_depth=0;observer::child_slots=0;observer::child_entries=0;
        child_original_calls=0;run_child_in_parent=false;child_mode=0;child_repetitions=1;child_nested=0;
        for(auto& count:observer::child_bypass)count=0;
        for(unsigned i=0;i<observer::row_limit;++i){observer::child_rows[i]={};observer::child_ready[i]=false;}
        observer::child_original=child_original;
        for(auto& count:observer::bypass)count=0;
        for(unsigned i=0;i<observer::row_limit;++i){observer::rows[i]={};observer::ready[i]=false;}
        observer::original=original;recording=true;ui_observer_enabled=true;admitted=true;stereo_publish_bypass=0;
        session_stop_calls=0;
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
        position::active_nonce=0;position::consumed_nonce_ack=0;position::last_restored_nonce=0;position::last_restored_tick=0;position::service_renewed=0;
        std::atomic_store(&position::last_request_receipt,std::shared_ptr<position::Request const>{});
#endif
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
        put(c.panel_vtable+observer::slot_offset,base+observer::code_rva);put(c.panel_vtable+observer::child_slot,base+observer::child_rva);
        put(c.panel+0x70,std::uint32_t(0x04040404));put(c.panel+0x60,std::uint32_t(11));put(c.panel+0x68,std::uint32_t(33));
        put(c.panel+0x64,c.top_bits);put(c.panel+0x6c,c.bottom_bits);
        put(c.panel+0x1dc,uintptr_t(0x78787878));put(c.panel+0x4e4,static_cast<unsigned char>(1));
        put(c.panel+0x4e8,uintptr_t(0x6000));put(c.panel+0x4f0,int(2));put(c.panel+0x4f4,int(2));put(c.panel+0x4f8,std::uint32_t(0x12345678));
        put(0x6000,trial::bits(.8f));put(0x6004,trial::bits(.9f));expected_object=reinterpret_cast<void*>(c.panel);
    }
    auto setter(){return [&](auto panel,float value,unsigned face,unsigned,unsigned){put(panel+0x60+face*4,trial::bits(value));return true;};}
    void apply(){auto ctx=position::context(c.client,c.scene);
        assert(trial::apply(position::state,c,ctx,safe_copy,setter())==trial::Result::applied);position::count(trial::Result::applied);}
    void call(){assert(observer::hook(expected_object)==sentinel);}
};
static uint64_t count(observer::Bypass why){return observer::bypass[unsigned(why)].load();}
static void call_subtree(){
    Fixture f;auto req=std::make_shared<position::Request>();req->config=f.c;req->expires=GetTickCount64()+7000;
    std::atomic_store(&position::selected,std::shared_ptr<position::Request const>(req));
    struct Candidate{uintptr_t client=0x1000,scene=0x2000;} candidate;mode=1;
    assert(position::invoke_with(candidate,[&]{f.call();return uintptr_t(123);},safe_copy,f.setter(),[]{return true;})==123);
    assert(original_calls==1&&observer::entries==1&&observer::ready[0].load(std::memory_order_acquire));
    auto const& r=observer::rows[0];assert(r.normal&&r.depth==1&&r.caller&&r.thread==GetCurrentThreadId()&&r.result==sentinel);
    assert(r.before.identity&&r.after.identity&&r.before.mask==2047&&r.after.mask==2047);
    assert(r.before.position[1]==position::state.lease.owned[0]&&r.before.position[3]==position::state.lease.owned[1]);
    assert(r.after.position[1]==f.c.top_bits&&r.after.position[3]==f.c.bottom_bits);
    assert(r.before.position[0]==11&&r.before.position[2]==33&&r.after.position[0]==11&&r.after.position[2]==33);
    assert(r.before.raw_1dc==0x78787878&&r.before.raw_4e4==1&&r.before.raw_4e8==0x6000&&r.before.num==2&&r.before.max==2);
    assert(r.before.raw_4f8==0x12345678&&r.before.selected_bits==trial::bits(.9f)&&r.before.zero_bits==trial::bits(.8f));
    assert(r.before.units==0x04040404&&r.before.world==4&&r.before.applied==1&&r.exit>=r.enter&&active==0&&observer::depth==0);
}
static void gates(){
    Fixture f;f.call();assert(original_calls==1&&observer::slots==0&&count(observer::Bypass::no_successful_nonce)==1);f.apply();
    auto bypass=[&](observer::Bypass why){auto reads=read_calls.load(),calls=original_calls.load();auto before=count(why);
        f.call();assert(observer::slots==0&&read_calls==reads&&original_calls==calls+1&&count(why)==before+1);};
    ++position::state.lease.config.nonce;bypass(observer::Bypass::no_successful_nonce);--position::state.lease.config.nonce;
    std::thread foreign([&]{bypass(observer::Bypass::foreign_thread);});foreign.join();
    expected_object=reinterpret_cast<void*>(f.c.panel+8);bypass(observer::Bypass::wrong_object);expected_object=reinterpret_cast<void*>(f.c.panel);
    recording=false;bypass(observer::Bypass::not_recording);recording=true;
    ui_observer_enabled=false;bypass(observer::Bypass::disabled);ui_observer_enabled=true;
    admitted=false;bypass(observer::Bypass::world);admitted=true;stereo_publish_bypass=1;bypass(observer::Bypass::stereo);stereo_publish_bypass=0;
    ++committed_world_generation;bypass(observer::Bypass::world);--committed_world_generation;
    auto denied=[&]{auto before=count(observer::Bypass::identity);f.call();assert(observer::slots==0&&count(observer::Bypass::identity)==before+1);};
    ++session_started_qpc;denied();--session_started_qpc;
    memory[0x5000]^=1;denied();memory[0x5000]^=1;
    put(f.c.panel,f.c.panel_vtable+8);denied();put(f.c.panel,f.c.panel_vtable);
    put(f.c.panel_vtable+observer::slot_offset,base+observer::code_rva+1);denied();put(f.c.panel_vtable+observer::slot_offset,base+observer::code_rva);
    put(f.c.panel_vtable+observer::child_slot,base+observer::child_rva+1);denied();put(f.c.panel_vtable+observer::child_slot,base+observer::child_rva);
    f.call();assert(observer::slots==1&&observer::ready[0]);
    uint64_t rejected{};for(auto const& counter:observer::bypass)rejected+=counter.load();assert(observer::entries==rejected+1);
}
static void bounds_nesting_exceptions(){
    Fixture f;f.apply();auto unchanged=memory;mode=2;f.call();assert(original_calls==2&&observer::slots==2&&observer::depth==0&&memory==unchanged);
    assert(observer::rows[0].depth==1&&observer::rows[1].depth==2&&observer::ready[0]&&observer::ready[1]);
    mode=3;try{f.call();assert(false);}catch(int n){assert(n==71);}
    assert(original_calls==3&&observer::depth==0&&observer::ready[2]&&!observer::rows[2].normal&&active==0&&memory==unchanged);
    mode=0;memory.erase(f.c.panel+0x6c);f.call();assert(observer::ready[3]&&observer::rows[3].before.mask==2039);
    auto reads=read_calls.load();for(unsigned i=0;i<10;++i)f.call();
    assert(original_calls==14&&observer::entries==14&&observer::slots==4&&read_calls==reads&&observer::depth==0&&count(observer::Bypass::quota)==10);
}
static std::string progress(){FILE* file{};assert(tmpfile_s(&file)==0&&file);observer::progress(file);rewind(file);
    std::string text;char buffer[512];while(auto n=fread(buffer,1,sizeof(buffer),file))text.append(buffer,n);fclose(file);
    auto tuple_count=[&](std::string const& key){auto begin=text.find(key);assert(begin!=std::string::npos);begin+=key.size();auto end=text.find(' ',begin);
        return 1u+unsigned(std::count(text.begin()+begin,text.begin()+end,','));};
    assert(tuple_count("path_layout_bypass_tuple=")==unsigned(observer::Bypass::count));
    assert(tuple_count("path_layout_child_bypass_tuple=")==unsigned(observer::ChildBypass::count));return text;}
static void publication(){
    Fixture f;f.apply();mode=4;std::promise<void> start,release;entered=&start;release_original=&release;
    std::thread owner([&]{position::owner_thread=GetCurrentThreadId();f.call();});
    start.get_future().wait();auto text=progress();assert(text.find("path_layout_claimed=1")!=std::string::npos&&text.find("path_layout_0=")==std::string::npos);
    release.set_value();owner.join();text=progress();assert(text.find("path_layout_0=")!=std::string::npos&&observer::ready[0]);
}
static void loss_timeline(){
    Fixture f;f.apply();mode=5;f.call();auto const& r=observer::rows[0];
    assert(!r.before.loss_ready&&r.before.ownership_lost==0&&r.after.loss_ready&&r.after.ownership_lost==1);
    assert(r.after.loss_tick==position::first_loss.tick&&r.after.position[1]==f.c.top_bits&&r.after.position[3]==f.c.bottom_bits);
    mode=0;f.call();assert(observer::rows[1].before.loss_ready&&observer::rows[1].before.loss_tick==r.after.loss_tick);
    mode=6;f.call();assert(!observer::rows[2].after.identity&&observer::rows[2].after.mask==0&&observer::rows[2].normal);
}
static void bounded_array(){
    for(unsigned bad=0;bad<8;++bad){Fixture f;f.apply();
        if(bad==0)put(f.c.panel+0x4f0,int(-1));
        if(bad==1)put(f.c.panel+0x4f4,int(1));
        if(bad==2)put(f.c.panel+0x4f4,int(65));
        if(bad==3)put(f.c.panel+0x4e8,UINTPTR_MAX-3);
        if(bad==4)put(f.c.panel+0x4e8,uintptr_t(0));
        if(bad==5)put(f.c.panel+0x4e4,static_cast<unsigned char>(2));
        if(bad==6)memory.erase(f.c.panel+0x4e4);
        if(bad==7)memory.erase(0x6004);
        auto before=memory;array_reads=0;f.call();auto const& s=observer::rows[0].before;
        assert(!(s.mask&1024)&&memory==before);
        if(bad<5)assert(!(s.mask&512)&&array_reads==0);else assert((s.mask&512)&&s.zero_bits==trial::bits(.8f));
    }
}
static uint64_t child_count(observer::ChildBypass why){return observer::child_bypass[unsigned(why)].load();}
static void child_arguments_and_effect(){
    Fixture f;f.apply();run_child_in_parent=true;child_mode=4;f.call();
    assert(original_calls==1&&child_original_calls==1&&observer::slots==1&&observer::child_slots==1);
    assert(observer::child_ready[0].load(std::memory_order_acquire));auto const& r=observer::child_rows[0];
    assert(r.normal&&r.result==child_sentinel&&r.input==child_input&&r.input_type==child_input_type&&r.origin==child_origin&&r.sort==child_sort);
    assert(r.caller&&r.parent_row==0&&r.nonce==f.c.nonce&&r.panel==f.c.panel&&r.scene==f.c.scene&&r.client==f.c.client);
    assert(r.world==4&&r.thread==GetCurrentThreadId()&&r.exit>=r.enter&&r.before.mask==2047&&r.after.mask==2047);
    assert(r.before.position[1]==position::state.lease.owned[0]&&r.before.position[3]==position::state.lease.owned[1]);
    assert(r.after.position[1]==f.c.top_bits&&r.after.position[3]==f.c.bottom_bits);
    assert(observer::rows[0].after.position==r.after.position&&observer::child_depth==0&&!observer::parent_scope&&active==0);
}
static void child_gates(){
    Fixture f;f.apply();auto pass=[&](observer::ChildBypass why){auto reads=read_calls.load(),calls=child_original_calls.load();auto before=child_count(why);
        assert(call_child()==child_sentinel);assert(observer::child_slots==0&&read_calls==reads&&child_original_calls==calls+1&&child_count(why)==before+1);};
    pass(observer::ChildBypass::no_scope);
    std::thread foreign([&]{observer::parent_scope=reinterpret_cast<observer::ParentScope const*>(uintptr_t(1));
        pass(observer::ChildBypass::foreign_thread);observer::parent_scope=nullptr;});foreign.join();
    stereo_publish_bypass=1;pass(observer::ChildBypass::stereo);stereo_publish_bypass=0;
    ui_observer_enabled=false;pass(observer::ChildBypass::disabled);ui_observer_enabled=true;
    admitted=false;pass(observer::ChildBypass::world);admitted=true;
    recording=false;pass(observer::ChildBypass::not_recording);recording=true;
    observer::Row row{};row.panel=f.c.panel;row.scene=f.c.scene;row.client=f.c.client;row.world=f.c.world_generation;row.nonce=f.c.nonce;row.depth=1;
    observer::ParentScope scope{&row,3};observer::parent_scope=&scope;
    expected_object=reinterpret_cast<void*>(f.c.panel+8);pass(observer::ChildBypass::wrong_object);expected_object=reinterpret_cast<void*>(f.c.panel);
    ++position::state.lease.config.nonce;pass(observer::ChildBypass::no_successful_nonce);--position::state.lease.config.nonce;
    ++committed_world_generation;pass(observer::ChildBypass::world);--committed_world_generation;
    row.depth=2;pass(observer::ChildBypass::nested);row.depth=1;
    memory[0x5000]^=1;auto before=child_count(observer::ChildBypass::identity);assert(call_child()==child_sentinel);
    assert(observer::child_slots==0&&child_count(observer::ChildBypass::identity)==before+1);memory[0x5000]^=1;
    observer::parent_scope=nullptr;
    uint64_t rejected{};for(auto const& n:observer::child_bypass)rejected+=n.load();assert(rejected==observer::child_entries);
}
static void child_nested_and_exception(){
    {Fixture f;f.apply();child_mode=1;try{call_child();assert(false);}catch(int n){assert(n==93);}
        assert(child_original_calls==1&&observer::child_slots==0&&observer::child_depth==0&&!observer::parent_scope&&active==0);}
    {Fixture f;f.apply();run_child_in_parent=true;mode=2;auto before=memory;f.call();
        assert(original_calls==2&&child_original_calls==2&&observer::child_slots==1&&child_count(observer::ChildBypass::nested)==1);
        assert(observer::child_rows[0].parent_row==0&&memory==before&&!observer::parent_scope&&observer::child_depth==0);}
    {Fixture f;f.apply();run_child_in_parent=true;child_mode=2;auto before=memory;f.call();
        assert(child_original_calls==2&&observer::child_slots==1&&child_count(observer::ChildBypass::nested)==1&&memory==before);}
    {Fixture f;f.apply();run_child_in_parent=true;mode=2;observer::slots=3;f.call();
        assert(observer::slots==4&&child_original_calls==2&&observer::child_slots==1);
        assert(child_count(observer::ChildBypass::no_scope)==1&&observer::child_rows[0].parent_row==3&&!observer::parent_scope);}
    {Fixture f;f.apply();run_child_in_parent=true;child_mode=1;try{f.call();assert(false);}catch(int n){assert(n==93);}
        assert(observer::child_ready[0]&&!observer::child_rows[0].normal&&observer::ready[0]&&!observer::rows[0].normal);
        assert(!observer::parent_scope&&observer::child_depth==0&&observer::depth==0&&active==0&&child_original_calls==1);
        child_mode=0;f.call();assert(observer::child_ready[1]&&observer::child_rows[1].normal&&child_original_calls==2);}
}
static void child_quota(){
    Fixture f;f.apply();run_child_in_parent=true;child_repetitions=6;auto before=memory;f.call();
    assert(original_calls==1&&child_original_calls==6&&observer::child_slots==4&&child_count(observer::ChildBypass::quota)==2&&memory==before);
    for(unsigned i=0;i<4;++i)assert(observer::child_ready[i]&&observer::child_rows[i].parent_row==0&&observer::child_rows[i].input==child_input);
}
static void child_publication(){
    for(bool completed_child:{false,true}){Fixture f;f.apply();run_child_in_parent=true;
        if(completed_child)mode=7;else child_mode=3;
        std::promise<void> start,release;entered=&start;release_original=&release;
        std::thread owner([&]{position::owner_thread=GetCurrentThreadId();f.call();});
        start.get_future().wait();auto text=progress();
        assert(text.find("path_layout_0=")==std::string::npos);
        assert((text.find("path_layout_child_0=")!=std::string::npos)==completed_child);
        release.set_value();owner.join();text=progress();assert(text.find("path_layout_child_0=")!=std::string::npos&&observer::child_ready[0]);
    }
}
static void complete_certification(){
    Fixture f;std::array<unsigned char,observer::code_size> code{};std::array<unsigned char,observer::child_size> child{};
    for(unsigned i=0;i<code.size();++i)code[i]=static_cast<unsigned char>(i%251);
    for(unsigned i=0;i<child.size();++i)child[i]=static_cast<unsigned char>((i+7)%251);
    auto seed=[&]{unsigned offset{};for(auto const& span:observer::code_chunks){for(unsigned i=0;i<span.size;++i)memory[base+span.rva+i]=code[offset+i];offset+=span.size;}
        offset=0;for(auto const& span:observer::child_chunks){for(unsigned i=0;i<span.size;++i)memory[base+span.rva+i]=child[offset+i];offset+=span.size;}};seed();
    unsigned hash_calls{};auto hash=[&](unsigned char const* data,unsigned n,char const* sha){++hash_calls;
        if(n==code.size()){assert(std::string(sha)==observer::code_sha256);return std::equal(code.begin(),code.end(),data);}
        assert(n==child.size()&&std::string(sha)==observer::child_sha256);return std::equal(child.begin(),child.end(),data);};
    assert(observer::certify_with(safe_copy,hash)&&hash_calls==2);
    for(auto span:{observer::code_chunks[0],observer::code_chunks[1],observer::child_chunks[0],observer::child_chunks[1]}){
        auto tail=base+span.rva+span.size-1;memory[tail]^=1;assert(!observer::certify_with(safe_copy,hash));seed();
        memory.erase(tail);assert(!observer::certify_with(safe_copy,hash));seed();
        unsigned chunk_reads{};auto changing=[&](void* out,void const* from,std::size_t n){bool ok=safe_copy(out,from,n);
            if(ok&&reinterpret_cast<uintptr_t>(from)==base+span.rva&&++chunk_reads==2)static_cast<unsigned char*>(out)[n-1]^=1;return ok;};
        assert(!observer::certify_with(changing,hash));
    }
    for(auto slot:{observer::slot_offset,observer::child_slot}){
        auto address=base+observer::vtable_rva+slot;auto expected=slot==observer::slot_offset?base+observer::code_rva:base+observer::child_rva;
        put(address,expected+1);assert(!observer::certify_with(safe_copy,hash));put(address,expected);
        unsigned reads{};auto changing=[&](void* out,void const* from,std::size_t n){bool ok=safe_copy(out,from,n);
            if(ok&&reinterpret_cast<uintptr_t>(from)==address&&++reads==2)static_cast<unsigned char*>(out)[0]^=1;return ok;};
        assert(!observer::certify_with(changing,hash));
    }
}
int main(){call_subtree();gates();bounds_nesting_exceptions();publication();loss_timeline();bounded_array();child_arguments_and_effect();child_gates();child_nested_and_exception();child_quota();child_publication();complete_certification();std::puts("PathChoiceLayoutObserverTest passed");return 0;}
