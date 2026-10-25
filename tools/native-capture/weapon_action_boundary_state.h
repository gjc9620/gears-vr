#pragma once
#include "../vr-common/weapon_action_boundary.h"
#include "native_action_context.h"
#include <atomic>
#include <cstring>

// Native-only kernel. Caller serializes Pool and publication/commit writers
// under Broker admission. Revoke alone may race. State is a safely copied POD;
// its separately supplied original PG address is compared, never dereferenced.
// Pool/ledger storage must outlive nonquiescent cells. No pin/engine API.
namespace gears_weapon_boundary_state {
namespace wire=gears_weapon_boundary;namespace ctx=gears_native_context;
using Reply=gears_weapon_hand::Reply;
constexpr auto maximum=(std::numeric_limits<std::uint64_t>::max)();
struct Auth {std::uint64_t cookie{},native{},context{};};
inline bool same(Auth a,Auth b) noexcept {return a.cookie==b.cookie&&a.native==b.native&&a.context==b.context;}
struct Caller {Auth auth{};std::uint32_t thread{};bool native_game_thread{};};
struct Stamp {Auth auth{};std::uint64_t frame{},generation{};};
class Revocation {
    std::atomic<std::uint64_t> generation_,auth_seq_{},cookie_{},native_{},context_{},commit_seq_{},frame_{},committed_{};
    std::atomic<bool> debt_{};
    std::uint64_t advance() noexcept {
        auto value=generation_.load(std::memory_order_acquire);
        do{if(value==maximum){poison();return 0;}}
        while(!generation_.compare_exchange_weak(value,value+1,std::memory_order_acq_rel));
        return value+1;
    }
public:
    explicit Revocation(std::uint64_t first=1) noexcept:generation_(first){if(!first)poison();}
    void poison() noexcept {debt_.store(true,std::memory_order_release);}
    bool debt() const noexcept {return debt_.load(std::memory_order_acquire);}
    std::uint64_t generation() const noexcept {return generation_.load(std::memory_order_acquire);}
    bool matches(Auth a) const noexcept {
        const auto seq=auth_seq_.load(std::memory_order_acquire);if(seq&1)return false;
        const Auth b{cookie_.load(),native_.load(),context_.load()};
        return a.cookie&&a.native&&a.context&&same(a,b)&&seq==auth_seq_.load(std::memory_order_acquire);
    }
    bool publish(Auth a) noexcept {
        const auto seq=auth_seq_.load();if(seq>=maximum-1){poison();return false;}
        auth_seq_.store(seq+1,std::memory_order_release);cookie_.store(a.cookie);native_.store(a.native);context_.store(a.context);
        committed_.store(0,std::memory_order_release);const auto value=advance();
        auth_seq_.store(seq+2,std::memory_order_release);return value&&!debt();
    }
    Reply revoke(wire::Request const& r,std::uint64_t& output) noexcept {
        output=0;if(!wire::request_valid(r))return Reply::invalid;
        if(!matches({r.hand_cookie,r.native_started_qpc,r.context}))return Reply::wrong_cookie;
        output=advance();if(r.flags&wire::unresolved)poison();
        return output&&!debt()?Reply::accepted:Reply::draining;
    }
    Stamp freeze(Auth a,std::uint64_t frame) const noexcept {return {a,frame,generation()};}
    bool commit(Stamp s) noexcept {
        if(!s.frame||debt()||!matches(s.auth)||s.generation!=generation())return false;
        const auto seq=commit_seq_.load();if(seq>=maximum-1){poison();return false;}
        commit_seq_.store(seq+1,std::memory_order_release);frame_.store(s.frame);committed_.store(s.generation);
        commit_seq_.store(seq+2,std::memory_order_release);
        return !debt()&&s.generation==generation()&&matches(s.auth);
    }
    // Cache-stamp comparison only, never an ambient engine/field permission.
    bool frame_current(Stamp s) const noexcept {
        const auto seq=commit_seq_.load(std::memory_order_acquire);if(seq&1)return false;
        const auto frame=frame_.load(),generation=committed_.load();
        return s.frame&&frame==s.frame&&generation==s.generation&&seq==commit_seq_.load(std::memory_order_acquire)&&
            !debt()&&s.generation==this->generation()&&matches(s.auth);
    }
};
static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
struct Descriptor {std::uintptr_t pool{};std::uint64_t id{},request_hash{};std::uint32_t slot{},magic{};};
struct Cell {
    ctx::Ticket ticket{};ctx::Ledger* ledger{};std::uintptr_t state{};std::uint64_t id{};
    wire::Request request{};wire::Receipt receipt{};bool active{},debt{};
};
struct Counts {unsigned active{},uncertain{};bool unknown{},revocation{};
    bool quiescent() const noexcept {return !active&&!uncertain&&!unknown&&!revocation;}};
class Pool {
    std::array<Cell,64> cells_{};Revocation& revocation_;std::uint64_t next_;bool unknown_{},stopping_{};
    static std::uint64_t hash(wire::Request const& r) noexcept {
        std::array<unsigned char,sizeof(r)> bytes{};std::memcpy(bytes.data(),&r,sizeof(r));std::uint64_t value=1469598103934665603ull;
        for(auto byte:bytes)value=(value^byte)*1099511628211ull;return value;
    }
    Descriptor token(unsigned slot,Cell const& c) const noexcept {return {reinterpret_cast<std::uintptr_t>(this),c.id,hash(c.request),slot,0x425631u};}
    wire::State encoded(unsigned slot,Cell const& c) const noexcept {wire::State value{};const auto d=token(slot,c);std::memcpy(value.storage.data(),&d,sizeof(d));return value;}
    Cell* identify(wire::State const& state,std::uintptr_t address) noexcept {
        for(auto& c:cells_)if((c.active||c.debt)&&c.state==address)return &c;
        Descriptor d{};std::memcpy(&d,state.storage.data(),sizeof(d));
        if(d.pool!=reinterpret_cast<std::uintptr_t>(this)||d.slot>=cells_.size())return nullptr;
        auto& c=cells_[d.slot];return (c.active||c.debt)&&c.id==d.id?&c:nullptr;
    }
    wire::End bad(Cell* c) noexcept {if(c)c->debt=true;else unknown_=true;revocation_.poison();return wire::End::unresolved;}
public:
    explicit Pool(Revocation& revocation,std::uint64_t first=1) noexcept:revocation_(revocation),next_(first){}
    Pool(Pool const&)=delete;Pool& operator=(Pool const&)=delete;
    Counts counts() const noexcept {Counts n{};for(auto const& c:cells_){n.active+=c.active;n.uncertain+=c.debt;}n.unknown=unknown_;n.revocation=revocation_.debt();return n;}
    void stop() noexcept {stopping_=true;}
    Reply open(wire::Request const& r,wire::State& state,wire::Receipt& receipt,Caller caller,ctx::Ledger& ledger,std::uintptr_t address) noexcept {
        auto* old=identify(state,address);const wire::State zero{};const bool dirty=std::memcmp(&state,&zero,sizeof(state))!=0;
        state={};receipt={};
        // Reopen of an admitted/uncertain descriptor is NOT a clean NoAuth reply.
        if(old||dirty){(void)bad(old);return Reply::invalid;}
        if(!address||(address&7)){(void)bad(nullptr);return Reply::invalid;}
        if(!wire::request_valid(r))return Reply::invalid;
        if(stopping_||revocation_.debt())return Reply::draining;
        if(!same(caller.auth,{r.hand_cookie,r.native_started_qpc,r.context})||!revocation_.matches(caller.auth))return Reply::wrong_cookie;
        if(r.flags&wire::unresolved){(void)bad(nullptr);return Reply::invalid;}
        if(!caller.native_game_thread||!caller.thread||caller.thread!=r.thread_id)return Reply::unavailable;
        if(!ctx::engine_context_clear(ledger,true))return Reply::reentrant;
        if(!next_||next_==maximum){(void)bad(nullptr);return Reply::invalid;}
        for(unsigned slot=0;slot<cells_.size();++slot){auto& c=cells_[slot];if(c.active||c.debt)continue;
            c.request=r;c.id=next_++;c.ledger=&ledger;c.state=address;
            if(!ctx::enter(ledger,c.ticket,ctx::Kind::restore)){(void)bad(&c);return Reply::invalid;}
            c.active=true;c.receipt.hand_cookie=r.hand_cookie;c.receipt.native_started_qpc=r.native_started_qpc;
            c.receipt.scope_id=c.id;c.receipt.revocation_generation=revocation_.generation();c.receipt.thread_id=caller.thread;
            c.receipt.flags=wire::scope_active|wire::engine_calls_allowed;state=encoded(slot,c);receipt=c.receipt;return Reply::accepted;
        }
        return Reply::busy;
    }
    wire::End close(wire::State& state,wire::Receipt const& receipt,wire::Finish const& finish,ctx::Ledger& ledger,
                    std::uint32_t native_thread,bool authenticated_gt,std::uintptr_t address) noexcept {
        auto* c=identify(state,address);if(!c)return bad(nullptr);
        const auto slot=unsigned(c-cells_.data());const auto canonical=encoded(slot,*c);
        // GT attestation may use the frozen Native GT binding after stop.
        // Never require current published auth: unpublish/Revoke may have happened.
        if(c->debt||ledger.poisoned||c->state!=address||c->ledger!=&ledger||!authenticated_gt||native_thread!=c->request.thread_id||
           std::memcmp(&state,&canonical,sizeof(state))||std::memcmp(&receipt,&c->receipt,sizeof(receipt))||
           finish.size!=sizeof(finish)||finish.abi_version!=wire::version||finish.flags!=(wire::stage_closed|wire::handback_complete)||
           (finish.outcome!=wire::Outcome::returned&&finish.outcome!=wire::Outcome::original_exception)||ledger.top!=&c->ticket)return bad(c);
        for(unsigned i=0;i<ledger.depth.size();++i)if(ledger.depth[i]!=(i==unsigned(ctx::Kind::restore)?1u:0u))return bad(c);
        if(!ctx::leave(ledger,c->ticket))return bad(c);
        state={};*c={};return wire::End::closed; // No Native reference to the retired PG State.
    }
};
static_assert(std::is_trivially_copyable_v<Cell>&&std::is_trivially_destructible_v<Cell>);
static_assert(!std::is_copy_constructible_v<Pool>&&!std::is_move_constructible_v<Pool>);
} // namespace gears_weapon_boundary_state
