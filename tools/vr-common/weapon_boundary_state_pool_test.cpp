#include <windows.h>
#include "weapon_boundary_state_pool.h"
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>
#ifdef NDEBUG
#error WeaponBoundaryStatePoolTest requires active assertions
#endif
namespace {
namespace b=gears_weapon_boundary;
using Reply=gears_weapon_hand::Reply;
using Pool=b::StatePool<1>;
static_assert(!std::is_copy_constructible_v<Pool>&&!std::is_move_constructible_v<Pool>);
static_assert(std::is_trivial_v<b::PoolTicket>&&std::is_standard_layout_v<b::PoolTicket>);
template<class T,class=void>struct HasReset:std::false_type {};
template<class T>struct HasReset<T,std::void_t<decltype(std::declval<T&>().reset())>>:std::true_type {};
static_assert(!HasReset<Pool>::value);
b::Request request(){b::Request r{};r.source=b::Source::process_event;r.flags=b::related;
    r.hand_cookie=10;r.native_started_qpc=20;r.context=30;r.object=40;r.function=50;r.thread_id=60;return r;}
b::Finish complete(){b::Finish f{};f.outcome=b::Outcome::returned;f.flags=b::stage_closed|b::handback_complete;return f;}
void accepted(b::PoolOpenCall const& call,std::uint64_t generation=5,std::uint64_t scope=70){
    *call.receipt={};call.receipt->hand_cookie=call.request->hand_cookie;call.receipt->native_started_qpc=call.request->native_started_qpc;
    call.receipt->thread_id=call.request->thread_id;call.receipt->revocation_generation=generation;call.receipt->scope_id=scope;
    call.receipt->flags=b::scope_active|b::engine_calls_allowed;call.state->storage[0]=std::byte{0x5a};
}
template<class P>void active(P& pool,typename P::Ticket& ticket,b::PoolOpenCall& call,std::uint64_t minimum=5,std::uint64_t generation=5){
    assert(pool.claim(ticket,request(),minimum)&&pool.begin_open(ticket,call));accepted(call,generation);
    assert(pool.open_returned(ticket,Reply::accepted)==b::PoolOpen::active);
}
bool state_zero(b::State const& state){for(auto x:state.storage)if(x!=std::byte{})return false;return true;}
unsigned cases{};
void passed(char const* label){++cases;std::printf("%02u %s\n",cases,label);}

// Artificial provider only. Retaining a pointer and raising SEH here tests PG
// storage survival, never the real Native provider's TLS/unlink correctness.
thread_local b::State* fake_tls{};
enum class Fault {open_after_link,close_before_unlink,close_after_unlink};
Fault fault=Fault::open_after_link;
constexpr DWORD test_exception=0xe0421051;
Reply fake_open(b::Request const*,b::State* state,b::Receipt*) noexcept {
    fake_tls=state;state->storage[17]=std::byte{0xa7};RaiseException(test_exception,0,0,nullptr);return Reply::invalid;
}
b::End fake_close(b::State* state,b::Receipt const*,b::Finish const*) noexcept {
    assert(fake_tls==state);
    if(fault==Fault::close_after_unlink){fake_tls=nullptr;*state={};}
    RaiseException(test_exception,0,0,nullptr);return b::End::unresolved;
}
bool invoke_open(b::PoolOpenCall const& call,Reply& reply) noexcept {
    __try{reply=fake_open(call.request,call.state,call.receipt);return true;}
    __except(GetExceptionCode()==test_exception?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return false;}
}
bool invoke_close(b::PoolCloseCall const& call,b::End& reply) noexcept {
    __try{reply=fake_close(call.state,call.receipt,call.finish);return true;}
    __except(GetExceptionCode()==test_exception?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return false;}
}
b::State const* failed_open_returns(){
    static b::StatePool<2> pool;b::PoolTicket ticket{};b::PoolOpenCall call{};
    assert(pool.claim(ticket,request(),5)&&pool.begin_open(ticket,call));Reply reply=Reply::invalid;
    assert(!invoke_open(call,reply)&&pool.isolate(ticket));
    b::PoolDiagnostic d{};assert(pool.describe(ticket,d)&&d.phase==b::PoolPhase::isolated&&d.attempted==1&&!d.receipt_frozen);
    assert(d.state==call.state&&d.state==fake_tls&&d.state->storage[17]==std::byte{0xa7});
    b::PoolTicket other{};assert(pool.claim(other,request(),5));assert(pool.cancel_unopened(other));
    assert(pool.describe_isolated(ticket.index,d)&&d.state==fake_tls&&d.state->storage[17]==std::byte{0xa7});return d.state;
}
__declspec(noinline) void churn_stack(){volatile unsigned char bytes[8192]{};for(unsigned i=0;i<8192;++i)bytes[i]=static_cast<unsigned char>(i);}
void seh_cases(){
    const auto retained=failed_open_returns();churn_stack();assert(fake_tls==retained&&fake_tls->storage[17]==std::byte{0xa7});fake_tls=nullptr;
    passed("Open linked State survives SEH, caller return and stack reuse at original address");
    for(auto selected:{Fault::close_before_unlink,Fault::close_after_unlink}){
        // Separate permanent pools keep every uncertain State alive to exit.
        static b::StatePool<2> pool;b::PoolTicket ticket{};b::PoolOpenCall opened{};active(pool,ticket,opened);
        fake_tls=opened.state;fault=selected;b::PoolCloseCall call{};assert(pool.begin_close(ticket,complete(),call));
        b::End reply=b::End::unresolved;assert(!invoke_close(call,reply)&&pool.isolate(ticket));
        b::PoolDiagnostic d{};assert(pool.describe(ticket,d)&&d.phase==b::PoolPhase::isolated&&d.attempted==3&&d.receipt_frozen);
        assert(d.state==opened.state&&pool.retained()==pool.isolated());
        if(selected==Fault::close_after_unlink)assert(!fake_tls&&state_zero(*d.state));else assert(fake_tls==d.state);
        fake_tls=nullptr;passed(selected==Fault::close_after_unlink?"Close SEH after apparent unlink/zero still retains slot":"Close SEH before unlink retains exact slot and frozen receipt");
    }
}
void receipt_cases(){
    {static Pool pool;b::PoolTicket ticket{};b::PoolOpenCall opened{};auto input=request();assert(pool.claim(ticket,input,5));
        input.hand_cookie=999;assert(pool.begin_open(ticket,opened)&&opened.request->hand_cookie==10);
        accepted(opened,7);assert(pool.open_returned(ticket,Reply::accepted)==b::PoolOpen::active);
        b::PoolDiagnostic d{};assert(pool.describe(ticket,d)&&d.receipt.revocation_generation==7&&d.minimum_revocation_generation==5);
        const auto address=opened.state;opened.receipt->revocation_generation=999;opened.receipt->hand_cookie=999;
        std::uint64_t later_global_generation=900;b::PoolCloseCall close{};assert(pool.begin_close(ticket,complete(),close));
        assert(close.state==address&&close.receipt->revocation_generation==7&&close.receipt->hand_cookie==10&&later_global_generation==900);
        assert(pool.close_returned(ticket,b::End::closed)==b::PoolClose::recycled&&!pool.retained());
        assert(pool.close_returned(ticket,b::End::closed)==b::PoolClose::invalid);
        passed("accepted address and Open receipt freeze survive caller output/global generation changes");}
    {static b::StatePool<12> pool;
        for(unsigned which=0;which<12;++which){b::PoolTicket ticket{};b::PoolOpenCall opened{};
            assert(pool.claim(ticket,request(),5)&&pool.begin_open(ticket,opened));accepted(opened);
            switch(which){
            case 0:opened.receipt->hand_cookie++;break;case 1:opened.receipt->native_started_qpc++;break;
            case 2:opened.receipt->thread_id++;break;case 3:opened.receipt->scope_id=0;break;
            case 4:opened.receipt->revocation_generation=0;break;case 5:opened.receipt->revocation_generation=4;break;
            case 6:opened.receipt->flags=b::scope_active;break;case 7:opened.receipt->flags=b::engine_calls_allowed;break;
            case 8:opened.receipt->flags=7;break;case 9:opened.receipt->size--;break;case 10:opened.receipt->abi_version++;break;
            case 11:const_cast<b::Request*>(opened.request)->hand_cookie++;break;
            }
            const auto output=*opened.receipt;assert(pool.open_returned(ticket,Reply::accepted)==b::PoolOpen::isolated);
            b::PoolDiagnostic d{};assert(pool.describe(ticket,d)&&d.receipt_frozen&&b::pool_same_receipt(d.receipt,output));
            assert(d.state==opened.state&&d.state->storage[0]==std::byte{0x5a});
        }
        b::PoolTicket full{};assert(pool.retained()==12&&pool.isolated()==12&&!pool.claim(full,request(),5));
        passed("twelve malformed accepted receipts/requests quarantine without clearing or reuse");}
    {static Pool pool;b::PoolTicket ticket{};auto input=request();input.flags|=b::unresolved;b::PoolOpenCall opened{};
        assert(pool.claim(ticket,input,5)&&pool.begin_open(ticket,opened));accepted(opened);
        assert(pool.open_returned(ticket,Reply::accepted)==b::PoolOpen::isolated);passed("unresolved request cannot become an accepted authorization");}
}
void no_scope_cases(){
    {static Pool pool;
        for(auto reply:{Reply::unavailable,Reply::busy,Reply::draining,Reply::wrong_cookie,Reply::reentrant}){
            b::PoolTicket ticket{};b::PoolOpenCall call{};assert(pool.claim(ticket,request(),5)&&pool.begin_open(ticket,call));
            assert(state_zero(*call.state)&&call.receipt->size==sizeof(b::Receipt));
            assert(pool.open_returned(ticket,reply)==b::PoolOpen::no_scope&&!pool.retained());
        }
        passed("five explicit normal NoScope replies recycle only their zero-state cell");}
    {static b::StatePool<3> pool;for(auto reply:{Reply::invalid,Reply::quiescent,static_cast<Reply>(999)}){
            b::PoolTicket ticket{};b::PoolOpenCall call{};assert(pool.claim(ticket,request(),5)&&pool.begin_open(ticket,call));
            assert(pool.open_returned(ticket,reply)==b::PoolOpen::isolated);}
        assert(pool.isolated()==3);passed("invalid/quiescent/unknown normal returns never imply NoScope");}
    {static b::StatePool<6> pool;for(unsigned which=0;which<6;++which){b::PoolTicket ticket{};b::PoolOpenCall call{};
            assert(pool.claim(ticket,request(),5)&&pool.begin_open(ticket,call));
            switch(which){case 0:call.state->storage[3]=std::byte{1};break;case 1:call.receipt->scope_id=1;break;
            case 2:call.receipt->flags=b::engine_calls_allowed;break;case 3:call.receipt->flags=4;break;
            case 4:call.receipt->size--;break;case 5:call.receipt->abi_version++;break;}
            assert(pool.open_returned(ticket,Reply::unavailable)==b::PoolOpen::isolated);}
        assert(pool.retained()==6);passed("NoScope with nonzero State/scope/flags or malformed header is retained");}
}
void close_cases(){
    {static b::StatePool<2> pool;for(auto end:{b::End::unresolved,static_cast<b::End>(999)}){
            b::PoolTicket ticket{};b::PoolOpenCall open{};active(pool,ticket,open);b::PoolCloseCall close{};
            assert(pool.begin_close(ticket,complete(),close));const auto frozen=*close.receipt;
            assert(pool.close_returned(ticket,end)==b::PoolClose::isolated);b::PoolDiagnostic d{};
            assert(pool.describe(ticket,d)&&b::pool_same_receipt(d.receipt,frozen)&&d.state==open.state);}
        passed("normal unresolved/unknown Close keeps State and frozen receipt");}
    {static Pool pool;b::PoolTicket ticket{};b::PoolOpenCall open{};active(pool,ticket,open);b::PoolCloseCall close{};
        assert(pool.begin_close(ticket,complete(),close));const auto original=*close.receipt;
        const_cast<b::Receipt*>(close.receipt)->revocation_generation++;
        assert(pool.close_returned(ticket,b::End::closed)==b::PoolClose::isolated);b::PoolDiagnostic d{};
        assert(pool.describe(ticket,d)&&b::pool_same_receipt(d.receipt,original));passed("mutated Close receipt cannot release slot despite End closed");}
    {static Pool pool;b::PoolTicket ticket{};b::PoolOpenCall open{};active(pool,ticket,open);b::PoolCloseCall close{};
        assert(pool.begin_close(ticket,complete(),close));const_cast<b::Finish*>(close.finish)->flags=0;
        assert(pool.close_returned(ticket,b::End::closed)==b::PoolClose::isolated);passed("mutated Close Finish cannot release slot");}
    {static b::StatePool<4> pool;for(unsigned which=0;which<4;++which){b::PoolTicket ticket{};b::PoolOpenCall open{};
            active(pool,ticket,open);auto finish=complete();if(which<3)finish.flags=which;else finish.outcome=b::Outcome::notification_fault;
            b::PoolCloseCall close{};assert(pool.begin_close(ticket,finish,close));
            assert(pool.close_returned(ticket,b::End::closed)==b::PoolClose::isolated);}
        passed("missing handback/stage or notification fault never becomes recycled clean");}
    {static Pool pool;b::PoolTicket ticket{};b::PoolOpenCall open{};active(pool,ticket,open);auto finish=complete();finish.outcome=static_cast<b::Outcome>(999);
        b::PoolCloseCall close{};assert(!pool.begin_close(ticket,finish,close)&&!close.state&&pool.isolated()==1);
        passed("malformed Close input isolates before any export call");}
    {static Pool pool;b::PoolTicket ticket{};b::PoolOpenCall open{};active(pool,ticket,open);auto finish=complete();finish.outcome=b::Outcome::original_exception;
        b::PoolCloseCall close{};assert(pool.begin_close(ticket,finish,close));assert(pool.close_returned(ticket,b::End::closed)==b::PoolClose::recycled);
        passed("properly closed original-exception bookkeeping may recycle");}
}
void ticket_cases(){
    {static Pool pool;b::PoolTicket ticket{};b::PoolOpenCall open{};active(pool,ticket,open);auto copied=ticket;
        b::PoolCloseCall bad{};assert(!pool.begin_close(copied,complete(),bad)&&pool.phase(ticket.index)==b::PoolPhase::active);
        copied.address=reinterpret_cast<std::uintptr_t>(&copied);
        assert(!pool.begin_close(copied,complete(),bad)&&pool.phase(ticket.index)==b::PoolPhase::active);
        b::PoolCloseCall close{};assert(pool.begin_close(ticket,complete(),close)&&pool.close_returned(ticket,b::End::closed)==b::PoolClose::recycled);
        b::PoolTicket next{};assert(pool.claim(next,request(),5)&&next.generation>ticket.generation);
        assert(!pool.cancel_unopened(ticket)&&pool.close_returned(ticket,b::End::closed)==b::PoolClose::invalid&&pool.phase(next.index)==b::PoolPhase::ready);
        assert(pool.cancel_unopened(next));passed("copied/repaired-copy/stale/duplicate tickets cannot retire another owner or generation");}
    {static b::StatePool<1,2> pool;std::uint64_t previous{};for(unsigned i=0;i<2;++i){b::PoolTicket ticket{};b::PoolOpenCall call{};
            assert(pool.claim(ticket,request(),5)&&ticket.generation>previous);previous=ticket.generation;
            assert(pool.begin_open(ticket,call)&&pool.open_returned(ticket,Reply::draining)==b::PoolOpen::no_scope);}
        b::PoolTicket exhausted{};assert(!pool.claim(exhausted,request(),5)&&pool.phase(0)==b::PoolPhase::exhausted&&!pool.retained());
        passed("bounded generation exhaustion retires slot without wrapping");}
    {static Pool pool;b::PoolTicket ticket{};assert(pool.claim(ticket,request(),5));
        assert(pool.open_returned(ticket,Reply::accepted)==b::PoolOpen::invalid&&pool.isolated()==1);
        b::PoolTicket other{};assert(!pool.claim(other,request(),5));passed("illegal exact-ticket transition is isolated and capacity fails closed");}
    {static Pool pool;auto invalid=request();invalid.flags=0;b::PoolTicket ticket{};
        assert(!pool.claim(ticket,invalid,5)&&!pool.claim(ticket,request(),0)&&!pool.retained());
        ticket.index=1;assert(!pool.claim(ticket,request(),5));passed("malformed Request/minimum/fresh Ticket reject before allocation");}
    {static b::StatePool<2> pool;static Pool foreign;b::PoolTicket ticket{};assert(pool.claim(ticket,request(),5));
        assert(!foreign.cancel_unopened(ticket)&&pool.cancel_unopened(ticket));b::PoolDiagnostic d{};
        assert(!pool.describe(ticket,d)&&!d.state&&pool.phase(99)==b::PoolPhase::invalid);passed("foreign pool and retired diagnostics never borrow an active slot");}
    {static b::StatePool<64> small;static b::StatePool<128> large;b::PoolTicket a{},c{};
        assert(small.claim(a,request(),5)&&large.claim(c,request(),5));b::PoolDiagnostic da{},dc{};
        assert(small.describe(a,da)&&large.describe(c,dc)&&reinterpret_cast<std::uintptr_t>(da.state)%alignof(b::State)==0);
        assert(small.cancel_unopened(a)&&large.cancel_unopened(c));passed("64/128-slot instantiations keep aligned State512 storage");}
}
void concurrent_case(){
    static b::StatePool<8> pool;std::array<b::PoolTicket,8> tickets{};std::array<std::thread,8> threads;
    std::mutex mutex;std::condition_variable condition;unsigned ready{};bool release{};
    for(unsigned i=0;i<8;++i)threads[i]=std::thread([&,i]{auto input=request();input.thread_id=100+i;
        assert(pool.claim(tickets[i],input,5));{std::lock_guard<std::mutex> lock(mutex);++ready;}condition.notify_all();
        {std::unique_lock<std::mutex> lock(mutex);assert(condition.wait_for(lock,std::chrono::seconds(5),[&]{return release;}));}
        b::PoolOpenCall call{};assert(pool.begin_open(tickets[i],call)&&pool.open_returned(tickets[i],Reply::draining)==b::PoolOpen::no_scope);
    });
    {std::unique_lock<std::mutex> lock(mutex);assert(condition.wait_for(lock,std::chrono::seconds(5),[&]{return ready==8;}));}
    assert(pool.retained()==8);for(unsigned i=0;i<8;++i)for(unsigned j=0;j<i;++j)assert(tickets[i].index!=tickets[j].index);
    b::PoolTicket full{};assert(!pool.claim(full,request(),5));{std::lock_guard<std::mutex> lock(mutex);release=true;}condition.notify_all();
    for(auto& thread:threads)thread.join();assert(!pool.retained()&&!pool.isolated());
    passed("eight bounded concurrent owners claim unique cells and retire normal NoScope independently");
}
}
int main(){seh_cases();receipt_cases();no_scope_cases();close_cases();ticket_cases();concurrent_case();
    std::printf("WeaponBoundaryStatePoolTest passed: %u cases; actual pool/ABI, artificial provider/TLS/SEH only\n",cases);}
