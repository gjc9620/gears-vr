#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include "weapon_action_boundary_state.h"
#include <cstdio>
#include <thread>
#ifdef NDEBUG
#error Boundary kernel test requires assertions enabled
#endif
namespace k=gears_weapon_boundary_state;namespace w=gears_weapon_boundary;namespace ctx=gears_native_context;
static unsigned checks{},failures{};
#define C(value) do{++checks;if(!(value)){++failures;std::printf("FAIL %u\n",unsigned(__LINE__));}}while(false)
static std::uintptr_t address(w::State const& s){return reinterpret_cast<std::uintptr_t>(&s);}
static bool zero(w::State const& s){const w::State z{};return std::memcmp(&s,&z,sizeof(s))==0;}
static w::Finish finished(w::Outcome outcome=w::Outcome::returned){w::Finish f{};f.outcome=outcome;f.flags=w::stage_closed|w::handback_complete;return f;}
struct Rig {
    k::Revocation rev{};k::Pool pool;ctx::Ledger ledger{};w::Request request{};k::Caller caller{{1,2,3},7,true};
    explicit Rig(std::uint64_t first=1):pool(rev,first){C(rev.publish(caller.auth));request.source=w::Source::process_event;request.flags=w::related;
        request.hand_cookie=1;request.native_started_qpc=2;request.context=3;request.object=4;request.function=8;request.thread_id=7;}
    k::Reply open(w::State& s,w::Receipt& r){return pool.open(request,s,r,caller,ledger,address(s));}
    w::End close(w::State& s,w::Receipt const& r,w::Finish f=finished()){return pool.close(s,r,f,ledger,caller.thread,caller.native_game_thread,address(s));}
};
static void lifecycle(){
    Rig f;w::State state{},nested{};w::Receipt receipt{},noauth{};C(f.open(state,receipt)==k::Reply::accepted);
    C(receipt.flags==3&&receipt.scope_id&&f.pool.counts().active==1);
    const auto top=reinterpret_cast<std::uintptr_t>(f.ledger.top),base=reinterpret_cast<std::uintptr_t>(&f.pool);
    C(top>=base&&top<base+sizeof(f.pool));C(f.open(nested,noauth)==k::Reply::reentrant);
    C(zero(nested)&&noauth.flags==0&&noauth.scope_id==0&&f.pool.counts().active==1);
    std::uint64_t generation{};C(f.rev.revoke(f.request,generation)==k::Reply::accepted&&generation>receipt.revocation_generation);
    f.pool.stop();C(f.rev.publish({}));C(f.close(state,receipt)==w::End::closed);C(zero(state)&&!f.ledger.top&&f.pool.counts().quiescent());
    C(f.open(nested,noauth)==k::Reply::draining&&zero(nested)&&!noauth.flags&&!noauth.scope_id);
}
static void unsafe_contexts(){
    for(unsigned kind=0;kind<unsigned(ctx::Kind::count);++kind){Rig f;ctx::Ticket token{};w::State s{};w::Receipt r{};
        C(ctx::enter(f.ledger,token,ctx::Kind(kind)));C(f.open(s,r)==k::Reply::reentrant);C(zero(s)&&!r.flags&&!r.scope_id&&f.pool.counts().quiescent());C(ctx::leave(f.ledger,token));}
    Rig f;w::State s{};w::Receipt r{};f.caller.native_game_thread=false;C(f.open(s,r)==k::Reply::unavailable);C(zero(s)&&!r.flags&&!r.scope_id);
    f.caller.native_game_thread=true;f.caller.auth.cookie=9;C(f.open(s,r)==k::Reply::wrong_cookie&&zero(s)&&!r.flags&&!r.scope_id);
}
static void invalid_closes(){
    for(unsigned mode=0;mode<17;++mode){Rig f;w::State s{};w::Receipt r{};C(f.open(s,r)==k::Reply::accepted);auto end=finished();ctx::Ledger other{};
        if(mode==0)++r.hand_cookie;if(mode==1)++r.native_started_qpc;if(mode==2)++r.scope_id;if(mode==3)++r.revocation_generation;
        if(mode==4)++r.thread_id;if(mode==5)end.flags=w::stage_closed;if(mode==6)end.outcome=w::Outcome::notification_fault;
        if(mode==7)end.outcome=w::Outcome(99);if(mode==8)s.storage.back()=std::byte{1};if(mode==9)f.caller.thread=8;if(mode==10)f.caller.native_game_thread=false;
        if(mode==12)f.ledger.poisoned=true;if(mode==13)f.ledger.depth[unsigned(ctx::Kind::callback)]=1;if(mode==14)f.ledger.depth[unsigned(ctx::Kind::restore)]=2;
        if(mode==15)end.size=0;if(mode==16)end.abi_version=2;
        const auto result=mode==11?f.pool.close(s,r,end,other,7,true,address(s)):f.close(s,r,end);
        C(result==w::End::unresolved);C(f.pool.counts().active==1&&f.pool.counts().uncertain==1&&!f.pool.counts().quiescent());
        f.pool.stop();C(f.rev.publish({})==false);C(!f.pool.counts().quiescent());}
    Rig duplicate;w::State s{};w::Receipt r{};C(duplicate.open(s,r)==k::Reply::accepted);C(duplicate.close(s,r)==w::End::closed);
    C(duplicate.close(s,r)==w::End::unresolved&&duplicate.pool.counts().unknown);
    Rig first,foreign;w::State t{};w::Receipt rt{};C(first.open(t,rt)==k::Reply::accepted);
    C(foreign.close(t,rt)==w::End::unresolved&&foreign.pool.counts().unknown);C(first.close(t,rt)==w::End::closed);
}
static void uncertain_isolation(){
    Rig f;w::State a{},b{};w::Receipt ra{},rb{};ctx::Ledger second{};auto request=f.request;request.thread_id=8;auto caller=f.caller;caller.thread=8;
    C(f.open(a,ra)==k::Reply::accepted);C(f.pool.open(request,b,rb,caller,second,address(b))==k::Reply::accepted);
    auto copy=a;C(f.close(copy,ra)==w::End::unresolved);C(f.pool.close(b,rb,finished(),second,8,true,address(b))==w::End::closed);
    C(f.pool.counts().active==1&&f.pool.counts().uncertain==1);C(f.close(a,ra)==w::End::unresolved);
    w::Receipt denied{};C(f.open(a,denied)==k::Reply::invalid&&zero(a)&&!denied.flags&&!denied.scope_id);C(f.pool.counts().uncertain==1);
    Rig lifo;w::State t{};w::Receipt rt{};ctx::Ticket nested{};C(lifo.open(t,rt)==k::Reply::accepted);
    C(ctx::enter(lifo.ledger,nested,ctx::Kind::callback));C(lifo.close(t,rt)==w::End::unresolved);C(ctx::leave(lifo.ledger,nested));C(!lifo.pool.counts().quiescent());
    Rig reopen;w::State live{},fresh{};w::Receipt live_receipt{},denial{};C(reopen.open(live,live_receipt)==k::Reply::accepted);
    C(reopen.open(live,denial)==k::Reply::invalid&&zero(live)&&!denial.flags&&!denial.scope_id);
    C(reopen.pool.counts().active==1&&reopen.pool.counts().uncertain==1);C(!reopen.rev.publish(reopen.caller.auth));
    C(reopen.open(fresh,denial)==k::Reply::draining&&zero(fresh)&&!denial.flags&&!denial.scope_id);
}
static void capacity(){
    Rig f;std::array<w::State,65> states{};std::array<w::Receipt,65> receipts{};std::array<ctx::Ledger,65> ledgers{};
    for(unsigned i=0;i<64;++i){auto request=f.request;request.thread_id=i+1;auto caller=f.caller;caller.thread=i+1;
        C(f.pool.open(request,states[i],receipts[i],caller,ledgers[i],address(states[i]))==k::Reply::accepted);}
    auto request=f.request;request.thread_id=65;auto caller=f.caller;caller.thread=65;
    C(f.pool.open(request,states[64],receipts[64],caller,ledgers[64],address(states[64]))==k::Reply::busy);C(zero(states[64])&&!receipts[64].flags&&!receipts[64].scope_id&&f.pool.counts().active==64);
    C(f.rev.publish({}));for(unsigned i=0;i<64;++i)C(f.pool.close(states[i],receipts[i],finished(),ledgers[i],i+1,true,address(states[i]))==w::End::closed);
    C(f.pool.counts().quiescent());C(f.rev.publish(caller.auth));C(f.pool.open(request,states[64],receipts[64],caller,ledgers[64],address(states[64]))==k::Reply::accepted);
    C(receipts[64].scope_id>receipts[63].scope_id);auto native_copy=states[64];
    C(f.pool.close(native_copy,receipts[64],finished(),ledgers[64],65,true,address(states[64]))==w::End::closed);C(f.pool.counts().quiescent());
}
static void revocations(){
    Rig f;const auto old=f.rev.freeze(f.caller.auth,77);C(f.rev.commit(old)&&f.rev.frame_current(old));std::atomic<unsigned> remaining{4},ready{};std::atomic<bool> okay{true},go{};std::array<std::thread,4> threads;
    for(auto& thread:threads)thread=std::thread([&]{++ready;while(!go.load())std::this_thread::yield();for(unsigned i=0;i<200;++i){std::uint64_t gen{};if(f.rev.revoke(f.request,gen)!=k::Reply::accepted||!gen)okay=false;}--remaining;});
    while(ready.load()!=4)std::this_thread::yield();go=true;
    while(remaining.load())(void)f.rev.commit(old);for(auto& thread:threads)thread.join();
    C(okay.load()&&f.rev.generation()==old.generation+800);C(!f.rev.commit(old)&&!f.rev.frame_current(old));
    const auto fresh=f.rev.freeze(f.caller.auth,77);C(f.rev.commit(fresh)&&f.rev.frame_current(fresh));C(!f.rev.frame_current({fresh.auth,78,fresh.generation}));
    auto unresolved=f.request;unresolved.flags|=w::unresolved;std::uint64_t gen{};C(f.rev.revoke(unresolved,gen)==k::Reply::draining&&f.rev.debt());C(!f.rev.publish(f.caller.auth)&&!f.rev.commit(fresh));
    k::Revocation overflow(k::maximum-1);C(overflow.publish(f.caller.auth));C(overflow.generation()==k::maximum);C(overflow.revoke(f.request,gen)==k::Reply::draining&&gen==0&&overflow.generation()==k::maximum&&overflow.debt());
    Rig ids(k::maximum);w::State s{};w::Receipt r{};C(ids.open(s,r)==k::Reply::invalid&&!ids.pool.counts().quiescent());
}
static Rig seh;
static unsigned seh_scope(){
    w::State s{};w::Receipt r{};
    __try {C(seh.open(s,r)==k::Reply::accepted);__try {RaiseException(0xe042c080,0,0,nullptr);}
        __finally {C(seh.close(s,r,finished(w::Outcome::original_exception))==w::End::closed);}}
    __except(EXCEPTION_EXECUTE_HANDLER){return GetExceptionCode();}
    return 0;
}
int main(){
    static_assert(std::is_trivially_copyable_v<w::State>&&std::is_trivially_destructible_v<w::State>);
    lifecycle();unsafe_contexts();invalid_closes();uncertain_isolation();capacity();revocations();C(seh_scope()==0xe042c080);C(seh.pool.counts().quiescent()&&!seh.ledger.top);
    std::printf("weapon_action_boundary_state: %u checks, %u failures; Native64 cells, frozen close, revocation, POD SEH\n",checks,failures);return failures?1:0;
}
