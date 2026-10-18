// Actual CloseScene wrapper + exact restore prelude against synthetic memory.
// No native DLL, game process, or input.
#define GEARS_PATH_CHOICE_CLOSE_RESTORE 1
#define GEARS_PATH_CHOICE_CLOSE_RESTORE_TEST 1
#include "path_choice_close_observer_test.cpp"
using Completion=position::CloseRestoreCompletion;
using Status=position::CloseRestoreStatus;
struct RestoreFixture:CloseFixture {
    RestoreFixture(){position::restore_retries=0;position::restored=0;position::setter_failed=0;
        position::identity_lost=0;position::unresolved=false;position::terminal=false;
        close_obs::restore_slots=0;close_obs::restore_attempted=0;close_obs::restore_completed=0;close_obs::restore_failed=0;
        for(unsigned i=0;i<2;++i){close_obs::restore_ready[i]=false;close_obs::restore_rows[i]={};}}
    template<class Original,class Read,class Set,class Certify>
    uintptr_t run(Original native,Read read,Set setter,Certify certify){
        return close_obs::invoke_with(reinterpret_cast<void*>(f.c.client),reinterpret_cast<void*>(f.c.scene),0x9988776655443322ull,
            native,read,[]{return true;},[&](trial::Config const& c){return position::restore_before_close(c,read,setter,certify);});}
    template<class Original,class Set> uintptr_t run(Original native,Set setter){return run(native,safe_copy,setter,[]{return true;});}
    template<class Original> uintptr_t run(Original native){return run(native,f.setter());}
    std::array<std::uint32_t,2> positions(){std::array<std::uint32_t,2> p{};assert(trial::positions(f.c,safe_copy,p));return p;}
    void original_bits(){assert((positions()==std::array<std::uint32_t,2>{f.c.top_bits,f.c.bottom_bits}));}
    void select_position(){auto req=std::make_shared<position::Request>();req->config=f.c;req->expires=GetTickCount64()+8000;
        std::atomic_store(&position::selected,std::shared_ptr<position::Request const>(req));}
};
static void idle_and_same_nonce(){
    RestoreFixture f;f.select_position();unsigned calls{};
    auto native=[&](void* client,void* scene){++calls;assert(client==reinterpret_cast<void*>(f.f.c.client)&&scene==reinterpret_cast<void*>(f.f.c.scene));
        f.original_bits();assert(close_obs::restore_ready[0].load(std::memory_order_acquire)&&close_obs::restore_rows[0].completion.status==Status::restored);
        assert(!position::inside&&position::owner_activity==0&&position::in_flight==0);return sentinel;};
    assert(f.run(native)==sentinel&&calls==1&&position::restored==1&&!position::pending_restore&&!position::state.lease.active);
    auto const& receipt=close_obs::restore_rows[0];assert(close_obs::restore_ready[0]&&receipt.completion.status==Status::restored);
    assert(receipt.enter<=receipt.completed&&receipt.completed<=receipt.original_enter&&receipt.completion.setter_calls==2);
    assert(receipt.completion.read_mask==3&&receipt.completion.observed==f.positions()&&!receipt.completion.borrowed);
    assert(position::state.consumed_nonce==f.f.c.nonce&&position::restore_retries==1);
    struct Candidate{uintptr_t client,scene;}candidate{f.f.c.client,f.f.c.scene};unsigned setters{};
    auto set=[&](auto,float,unsigned,unsigned,int){++setters;return true;};
    assert(position::invoke_with(candidate,[]{return sentinel;},safe_copy,set,[]{return true;})==sentinel);
    assert(setters==0&&position::applied==1&&!position::state.lease.active); // Same object + selected nonce cannot reapply.
}
static void borrowed_and_drain(){
    RestoreFixture f;f.select_position();struct Candidate{uintptr_t client,scene;}candidate{f.f.c.client,f.f.c.scene};
    unsigned calls{};auto native=[&](void*,void*){++calls;f.original_bits();assert(position::inside&&position::owner_activity==1&&position::in_flight==1);return sentinel;};
    auto outer=[&]{assert(f.run(native)==sentinel);assert(position::inside&&position::owner_activity==1&&position::in_flight==1);return sentinel;};
    assert(position::invoke_with(candidate,outer,safe_copy,f.f.setter(),[]{return true;})==sentinel);
    assert(calls==1&&close_obs::restore_rows[0].completion.borrowed&&!position::inside&&position::in_flight==0&&position::owner_activity==0);
    {RestoreFixture g;position::request_drain();position::drain_started=GetTickCount64()-2001;unsigned writes{};
        auto setter=[&](auto panel,float v,unsigned face,unsigned input,int origin){++writes;assert(position::draining()&&!position::terminal&&position::in_flight==1);
            return g.f.setter()(panel,v,face,input,origin);};
        assert(g.run([&](void*,void*){g.original_bits();return sentinel;},setter)==sentinel&&writes==2&&!position::pending_restore&&!position::terminal);}
    {RestoreFixture g;position::owner_activity=2;unsigned writes{};auto setter=[&](auto,float,unsigned,unsigned,int){++writes;return true;};
        assert(g.run([](void*,void*){return sentinel;},setter)==sentinel&&writes==0&&position::pending_restore&&position::owner_activity==2);}
}
static void diagnostic_quota_and_nested(){
    RestoreFixture f;close_obs::slots=2;close_obs::restore_slots=2;unsigned native_calls{},writes{};
    auto native=[&](void*,void*){++native_calls;f.original_bits();return sentinel;};
    for(unsigned i=0;i<6;++i){
        if(i){++f.f.c.nonce;f.request.nonce=f.f.c.nonce;position::restore_retries=0;f.f.apply();
            auto req=std::make_shared<close_obs::Active>();req->request=f.request;req->expires=GetTickCount64()+8000;
            std::atomic_store(&close_obs::selected,std::shared_ptr<close_obs::Active const>(req));}
        auto setter=[&](auto panel,float v,unsigned face,unsigned input,int origin){++writes;assert(position::restore_retries==1&&position::inside);
            return f.f.setter()(panel,v,face,input,origin);};
        assert(f.run(native,setter)==sentinel);
    }
    assert(native_calls==6&&writes==12&&position::restored==6&&close_obs::restore_completed==6&&close_obs::restore_slots==2&&close_obs::slots==2);
    {RestoreFixture g;unsigned nested_calls{},sets{};bool nested_entered=false;
        auto original=[&](void*,void*){++nested_calls;return sentinel;};
        auto setter=[&](auto panel,float v,unsigned face,unsigned input,int origin){++sets;
            if(!nested_entered){nested_entered=true;assert(g.run(original)==sentinel);}
            return g.f.setter()(panel,v,face,input,origin);};
        assert(g.run(original,setter)==sentinel&&nested_calls==2&&sets==2&&position::restored==1&&close_bypass(close_obs::Bypass::nested)==1);}
}
static void failures_original_and_foreign(){
    {RestoreFixture f;auto before=read_calls.load();unsigned calls{},sets{};
        std::thread foreign([&]{auto setter=[&](auto,float,unsigned,unsigned,int){++sets;return true;};
            assert(f.run([&](void*,void*){++calls;return sentinel;},setter)==sentinel);});foreign.join();
        assert(calls==1&&sets==0&&read_calls==before&&close_obs::restore_slots==0);}
    {RestoreFixture f;unsigned calls{};assert(f.run([&](void*,void*){++calls;f.original_bits();return uintptr_t(0);})==0);
        assert(calls==1&&position::restored==1);}
    {RestoreFixture f;unsigned calls{};try{f.run([&](void*,void*)->uintptr_t{++calls;f.original_bits();throw 712;});assert(false);}catch(int e){assert(e==712);}
        assert(calls==1&&position::restored==1&&!close_obs::rows[0].normal&&close_obs::depth==0&&!position::inside&&position::in_flight==0);}
    {RestoreFixture f;unsigned calls{},sets{};auto setter=[&](auto panel,float v,unsigned face,unsigned input,int origin){++sets;
            if(face==3)return false;return f.f.setter()(panel,v,face,input,origin);};
        auto entry=f.positions();assert(f.run([&](void*,void*){++calls;return sentinel;},setter)==sentinel);
        assert(calls==1&&sets==2&&position::setter_failed==1&&position::pending_restore&&!position::terminal);
        assert(f.positions()[0]==f.f.c.top_bits&&f.positions()[1]==entry[1]&&position::state.lease.touched[1]&&!position::state.lease.touched[0]);}
    {RestoreFixture f;unsigned calls{},sets{};auto setter=[&](auto,float,unsigned,unsigned,int){++sets;return true;};
        assert(f.run([&](void*,void*){++calls;return sentinel;},safe_copy,setter,[]()->bool{throw 901;})==sentinel);
        assert(calls==1&&sets==0&&close_obs::restore_rows[0].completion.status==Status::exception&&!position::terminal);}
    {RestoreFixture f;unsigned calls{},sets{};bool wrote=false;
        auto read=[&](void* to,void const* from,std::size_t n){if(wrote)throw 333;return safe_copy(to,from,n);};
        auto setter=[&](auto panel,float v,unsigned face,unsigned input,int origin){++sets;auto ok=f.f.setter()(panel,v,face,input,origin);wrote=true;return ok;};
        assert(f.run([&](void*,void*){++calls;return sentinel;},read,setter,[]{return true;})==sentinel);
        assert(calls==1&&sets==1&&position::terminal&&position::unresolved&&position::pending_restore&&position::state.lease.active);
        assert(close_obs::restore_rows[0].completion.status==Status::exception&&close_obs::rows[0].after.read_exception);}
    {RestoreFixture f;unsigned calls{};
        auto native=[&](void*,void*){++calls;return sentinel;};
        assert(close_obs::invoke_with(reinterpret_cast<void*>(f.f.c.client),reinterpret_cast<void*>(f.f.c.scene),1,native,safe_copy,[]{return true;},
            [](trial::Config const&)->Completion{throw 999;})==sentinel);
        assert(calls==1&&close_obs::restore_rows[0].completion.status==Status::exception);}
}
static void control_admission_denied(){
    for(unsigned variant=0;variant<6;++variant){RestoreFixture f;unsigned sets{},calls{};
        if(variant==0)++position::successful_nonce;
        if(variant==1)put(f.f.c.panel+0x64,position::state.lease.owned[0]+1);
        if(variant==2)put(f.f.c.panel+0x70,std::uint32_t(0));
        if(variant==3)put(f.request.function+0x114,base+close_core::exec_rva+1);
        if(variant==4)position::terminal=true;
        auto setter=[&](auto,float,unsigned,unsigned,int){++sets;return true;};
        auto native=[&](void*,void*){++calls;return sentinel;};
        assert(f.run(native,safe_copy,setter,[&]{return variant!=5;})==sentinel);
        assert(calls==1&&sets==0&&position::pending_restore);
        if(variant<5)assert(close_obs::restore_slots==0);
        else assert(close_obs::restore_rows[0].completion.status==Status::rejected);
    }
}
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE_TEST)
#define CLOSE_RESTORE_TEST_ENTRY close_restore_fixture_tests
#else
#define CLOSE_RESTORE_TEST_ENTRY main
#endif
int CLOSE_RESTORE_TEST_ENTRY(){idle_and_same_nonce();borrowed_and_drain();diagnostic_quota_and_nested();failures_original_and_foreign();control_admission_denied();
    std::puts("PathChoiceCloseRestoreTest passed");return 0;}
