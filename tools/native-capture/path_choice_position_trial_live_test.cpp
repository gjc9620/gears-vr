#define NOMINMAX
#define GEARS_PATH_CHOICE_POSITION_TRIAL 1
#include "path_choice_position_trial.h"
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
    first_loss={};loss_claimed=false;loss_ready=false;
    successful_nonce=0;first_auth_failure={};auth_failure_claimed=false;auth_failure_ready=false;
    registry_accepted_reads=0;for(unsigned i=0;i<2;++i){registry_accepted_claimed[i]=false;registry_accepted_ready[i]=false;registry_accepted_by_header[i]=0;registry_accepted_receipts[i]={};}
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
static void ownership_receipts(){
    for(unsigned failure=0;failure<5;++failure){
        reset();Engine e;select(e);Candidate c;unsigned originals{};
        auto original=[&](){++originals;return uintptr_t(123);};auto certify=[](){return true;};
        invoke_with(c,original,e.reader(),e.setter(),certify);auto owned=e.pos();
        if(failure==1||failure==4){e.put(e.c.panel+0x64,e.c.top_bits);e.put(e.c.panel+0x6c,e.c.bottom_bits);}
        if(failure==2)e.put(e.c.panel+0x64,bits(.2f));
        unsigned units_reads{};
        auto reader=[&](void* out,void const* from,std::size_t size){auto at=reinterpret_cast<uintptr_t>(from);
            if(size==4&&((failure==0&&(at==e.c.panel+0x64||at==e.c.panel+0x6c))||
                (failure==3&&at==e.c.panel+0x6c)))return false;
            if(at==e.c.panel+0x70&&size==4&&failure==4&&++units_reads==2)return false;
            return e.read(out,from,size);
        };
        std::thread worker([&]{while(!loss_ready.load(std::memory_order_acquire))std::this_thread::yield();
            assert(first_loss.original[0]==e.c.top_bits&&first_loss.original[1]==e.c.bottom_bits);
            assert(first_loss.owned==owned&&first_loss.panel==e.c.panel&&first_loss.scene==e.c.scene);
            assert(first_loss.world==e.c.world_generation&&first_loss.current_world==e.c.world_generation);
        });
        assert(invoke_with(c,original,reader,e.setter(),certify)==123);worker.join();
        assert(ownership_lost==1&&stops==1&&originals==2&&e.calls==2&&loss_ready.load(std::memory_order_acquire));
        auto const receipt=first_loss;
        const unsigned mask=failure==0?0u:failure==3?1u:3u;
        assert(receipt.read_mask==mask&&receipt.positions_complete==(mask==3));
        assert(receipt.units_readable==(failure!=4)&&receipt.units==(failure==4?0u:0x04040404u));
        if(failure==0)assert(receipt.observed[0]==0&&receipt.observed[1]==0);
        if(failure==1||failure==4)assert(receipt.observed[0]==e.c.top_bits&&receipt.observed[1]==e.c.bottom_bits);
        if(failure==2)assert(receipt.observed[0]==bits(.2f)&&receipt.observed[1]==owned[1]);
        if(failure==3)assert(receipt.observed[0]==owned[0]&&receipt.observed[1]==0);
        // The terminal hook and an attempted second publication cannot replace
        // the first receipt, including its failure mask and original tick.
        invoke_with(c,original,e.reader(),e.setter(),certify);
        publish_loss(e.c,99,{1,2},3,true,e.reader());
        assert(first_loss.observed==receipt.observed&&first_loss.read_mask==receipt.read_mask&&
            first_loss.tick==receipt.tick&&first_loss.current_world==receipt.current_world&&e.calls==2);
    }
}
static void auth_failure_checks(){
    for(bool readable:{true,false}){reset();Engine e;
        Check check;check.address=0x6000;for(unsigned i=0;i<40;++i){check.bytes.push_back(static_cast<unsigned char>(i+10));e.memory[check.address+i]=check.bytes.back();}
        e.c.checks.push_back(check);select(e);Candidate candidate;unsigned originals{};
        auto original=[&]{++originals;return uintptr_t(0xfedcba9876543210ull);};auto cert=[] {return true;};
        invoke_with(candidate,original,e.reader(),e.setter(),cert);assert(applied==1&&!auth_failure_ready);
        if(readable)e.memory[check.address+25]^=1;
        unsigned target_reads{};
        auto reader=[&](void* out,void const* from,std::size_t size){
            if(reinterpret_cast<uintptr_t>(from)==check.address){++target_reads;if(!readable){static_cast<unsigned char*>(out)[0]=0xff;return false;}}
            return e.read(out,from,size);};
        std::thread worker([&]{while(!auth_failure_ready.load(std::memory_order_acquire))std::this_thread::yield();
            auto const& r=first_auth_failure;assert(r.nonce==e.c.nonce&&r.panel==e.c.panel&&r.scene==e.c.scene&&r.world==e.c.world_generation);
            assert(r.kind==AuthKind::check&&r.stage==AuthStage::hold&&r.check_index==1&&r.length==40&&r.address==check.address);
            assert(r.readable==readable&&r.first_mismatch==(readable?25u:UINT_MAX)&&r.window_offset==(readable?17u:0u)&&r.window_size==16);
            for(unsigned i=0;i<16;++i){assert(r.expected[i]==check.bytes[r.window_offset+i]);
                assert(r.observed[i]==(readable?static_cast<unsigned char>(check.bytes[r.window_offset+i]^(r.window_offset+i==25?1:0)):0));}
        });
        assert(invoke_with(candidate,original,reader,e.setter(),cert)==0xfedcba9876543210ull);worker.join();
        assert(target_reads==1&&identity_lost==1&&ownership_lost==0&&e.calls==2&&originals==2&&terminal&&stops==1);
        auto const saved=first_auth_failure;unsigned char expected=1,actual=2;
        publish_auth_failure(e.c,e.ctx,AuthStage::restore,AuthKind::units,7,0,0,0x8000,1,&expected,&actual,true);
        invoke_with(candidate,original,e.reader(),e.setter(),cert);
        assert(first_auth_failure.qpc==saved.qpc&&first_auth_failure.expected==saved.expected&&first_auth_failure.observed==saved.observed&&
            first_auth_failure.address==saved.address&&first_auth_failure.caller_hint==saved.caller_hint&&saved.qpc>0&&saved.thread==GetCurrentThreadId());
    }
}
static void auth_failure_fixed_fields_and_context(){
    for(unsigned failure=0;failure<5;++failure){reset();Engine e;select(e);Candidate c;auto original=[] {return uintptr_t(1);};auto cert=[] {return true;};
        invoke_with(c,original,e.reader(),e.setter(),cert);unsigned expected_field=UINT_MAX;
        if(failure==0)e.put(e.c.panel,uintptr_t(0x4008));
        if(failure==1)e.put(e.c.panel_vtable+0x3d8,e.c.base+0x2988d9);
        if(failure==2)e.put(e.c.panel_vtable+0x3b8,e.c.base+0x297339);
        if(failure==3)e.put(e.c.panel+0x70,std::uint32_t(0x04050404));
        if(failure==4){++committed_world_generation;expected_field=3;}
        unsigned read_calls{};auto reader=[&](void* out,void const* from,std::size_t size){++read_calls;return e.read(out,from,size);};
        invoke_with(c,original,reader,e.setter(),cert);if(failure==4)--committed_world_generation;
        assert(auth_failure_ready&&identity_lost==1&&e.calls==2);
        auto const& r=first_auth_failure;
        assert(r.stage==AuthStage::hold&&r.kind==(failure==4?AuthKind::context:static_cast<AuthKind>(unsigned(AuthKind::vtable)+failure)));
        assert(r.check_index==UINT_MAX&&r.context_field==expected_field&&r.readable&&r.first_mismatch!=UINT_MAX);
        if(failure==4)assert(read_calls==0&&r.current_world==r.world+1&&r.address==0);
    }
}
static void auth_reader_forwarding_and_foreign(){
    reset();Engine e;select(e);Candidate c;auto original=[] {return uintptr_t(1);};auto cert=[] {return true;};
    invoke_with(c,original,e.reader(),e.setter(),cert);
    std::vector<std::pair<uintptr_t,std::size_t>> plain,observed;
    auto record=[&](auto& calls){return [&](void* out,void const* from,std::size_t size){calls.emplace_back(reinterpret_cast<uintptr_t>(from),size);return e.read(out,from,size);};};
    assert(authenticate(e.c,e.ctx,record(plain)));
    assert(authenticate(e.c,e.ctx,auth_reader(e.c,e.ctx,AuthStage::hold,123,record(observed))));
    assert(plain==observed&&!auth_failure_ready);
    e.memory[0x5000]^=1;plain.clear();observed.clear();
    std::thread foreign([&]{auto wrong=e.ctx;++wrong.world_generation;
        assert(!authenticate(e.c,e.ctx,auth_reader(e.c,wrong,AuthStage::hold,123,record(observed))));
        assert(!auth_failure_ready);});foreign.join();
    assert(!authenticate(e.c,e.ctx,record(plain)));assert(plain==observed&&!auth_failure_ready);
    unsigned callbacks{},reads_before=reads.load();std::thread foreign_wrapper([&]{
        invoke_with(c,[&]{++callbacks;return uintptr_t(1);},e.reader(),e.setter(),cert);});foreign_wrapper.join();
    assert(callbacks==1&&reads==reads_before&&!auth_failure_ready&&e.calls==2);
    ++successful_nonce;assert(!authenticate(e.c,e.ctx,auth_reader(e.c,e.ctx,AuthStage::hold,123,e.reader())));
    assert(!auth_failure_ready);--successful_nonce;
}
static void auth_failure_during_restore(){
    reset();Engine e;auto req=select(e);Candidate c;unsigned originals{};auto original=[&]{++originals;return uintptr_t(1);};auto cert=[] {return true;};
    invoke_with(c,original,e.reader(),e.setter(),cert);req->expires=0;unsigned check_reads{};
    // Holding authenticate succeeds; the original restore authenticate's next
    // exact check read changes. There is no extra diagnostic game read.
    auto reader=[&](void* out,void const* from,std::size_t size){auto at=reinterpret_cast<uintptr_t>(from);
        if(at==0x5000&&++check_reads==2)e.memory[at]^=1;return e.read(out,from,size);};
    invoke_with(c,original,reader,e.setter(),cert);
    assert(auth_failure_ready&&first_auth_failure.stage==AuthStage::restore&&first_auth_failure.kind==AuthKind::check&&
        first_auth_failure.check_index==0&&check_reads==2&&e.calls==2&&identity_lost==1&&originals==2);
}
static void auth_duplicate_check_index(){
    reset();Engine e;e.c.checks.push_back(e.c.checks[0]);select(e);Candidate c;auto original=[] {return uintptr_t(1);};auto cert=[] {return true;};
    invoke_with(c,original,e.reader(),e.setter(),cert);unsigned calls{};
    auto reader=[&](void* out,void const* from,std::size_t size){bool ok=e.read(out,from,size);
        if(reinterpret_cast<uintptr_t>(from)==0x5000&&++calls==2)static_cast<unsigned char*>(out)[0]^=1;return ok;};
    invoke_with(c,original,reader,e.setter(),cert);
    assert(auth_failure_ready&&first_auth_failure.kind==AuthKind::check&&first_auth_failure.check_index==1&&calls==2&&e.calls==2);
}
static void configure_prefix(Engine& e,bool schema2=true){
    if(schema2){e.c.schema=2;e.c.registry_header_policy=RegistryHeaderPolicy::fixed_prefix_v1;}
    for(unsigned i=0;i<2;++i){RegistryHeader h{0x700000+i*0x100000,100,200};Check check;check.address=e.c.base+registry_header_rvas[i];
        check.bytes.resize(16);std::memcpy(check.bytes.data(),&h,16);e.c.checks.push_back(check);e.put(check.address,h);}
}
static void prefix_live_diagnostics_and_restore(){
    reset();Engine e;configure_prefix(e);auto request=select(e);Candidate c;auto original=[] {return uintptr_t(1);};auto cert=[] {return true;};
    invoke_with(c,original,e.reader(),e.setter(),cert);assert(applied==1&&registry_accepted_reads==0);
    std::thread worker([&]{for(unsigned i=0;i<2;++i){while(!registry_accepted_ready[i].load(std::memory_order_acquire))std::this_thread::yield();
        auto const& r=registry_accepted_receipts[i];assert(r.header_index==i&&r.check_index==i+1&&r.initial.num==100&&r.observed.num==180&&
            r.initial.data==r.observed.data&&r.initial.max==200&&r.observed.max==200&&r.stage==AuthStage::hold&&r.nonce==e.c.nonce&&r.qpc>0);}});
    for(int num:{180,120,100,150}){for(unsigned i=0;i<2;++i)e.put(e.c.base+registry_header_rvas[i],RegistryHeader{0x700000+i*0x100000,num,200});
        invoke_with(c,original,e.reader(),e.setter(),cert);assert(!auth_failure_ready&&identity_lost==0&&state.lease.active);}
    worker.join();assert(registry_accepted_reads==6&&registry_accepted_by_header[0]==3&&registry_accepted_by_header[1]==3);
    assert(registry_accepted_receipts[0].observed.num==180); // Immutable first accepted read; no high-water gate.
    std::vector<std::pair<uintptr_t,std::size_t>> plain,observed;
    auto record=[&](auto& calls){return [&](void* out,void const* from,std::size_t size){calls.emplace_back(reinterpret_cast<uintptr_t>(from),size);return e.read(out,from,size);};};
    assert(authenticate(e.c,e.ctx,record(plain))&&authenticate(e.c,e.ctx,auth_reader(e.c,e.ctx,AuthStage::hold,1,record(observed)))&&plain==observed);
    request->expires=0;invoke_with(c,original,e.reader(),e.setter(),cert);
    assert(restored==1&&e.calls==4&&!pending_restore&&!auth_failure_ready&&!unresolved);
}
static void prefix_apply_and_schema1_failure(){
    for(bool schema2:{true,false}){reset();Engine e;configure_prefix(e,schema2);select(e);Candidate c;auto original=[] {return uintptr_t(1);};auto cert=[] {return true;};
        if(schema2)for(unsigned i=0;i<2;++i)e.put(e.c.base+registry_header_rvas[i],RegistryHeader{0x700000+i*0x100000,101,200});
        invoke_with(c,original,e.reader(),e.setter(),cert);assert(applied==1);
        if(schema2){assert(registry_accepted_reads>0&&registry_accepted_receipts[0].stage==AuthStage::apply&&!auth_failure_ready);}
        else{e.put(e.c.base+registry_header_rvas[0],RegistryHeader{0x700000,101,200});invoke_with(c,original,e.reader(),e.setter(),cert);
            assert(identity_lost==1&&registry_accepted_reads==0&&auth_failure_ready&&first_auth_failure.first_mismatch==8&&first_auth_failure.check_index==1);}
    }
}
int main(){ownership_receipts();gates();expiry_and_retry();failed_apply_rollback();threaded_drain();bounded_drain();auth_failure_checks();auth_failure_fixed_fields_and_context();auth_reader_forwarding_and_foreign();auth_failure_during_restore();auth_duplicate_check_index();prefix_live_diagnostics_and_restore();prefix_apply_and_schema1_failure();std::puts("PathChoicePositionTrialLiveTest passed");}
