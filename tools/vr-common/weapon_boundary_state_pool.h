#pragma once
#include <cstddef>
#include "weapon_action_boundary.h"
#include <atomic>
#include <limits>

// PG storage only, not a Native permission/provider or module-lifetime proof.
// Keep the nonmovable pool at a stable address until Admission/code/Native are
// safely drained. An isolated slot has no recovery/reset API. Before releasing
// its Admission Entry, the caller must quarantine Admission while still pinned.
namespace gears_weapon_boundary {
struct PoolTicket {
    std::uintptr_t owner,address;
    std::uint64_t generation;
    std::uint32_t index,reserved;
};
static_assert(std::is_trivial_v<PoolTicket>&&std::is_standard_layout_v<PoolTicket>);
enum class PoolPhase:std::uint64_t {free,held,ready,opening,active,closing,isolated,exhausted,invalid};
enum class PoolOpen:std::uint32_t {active,no_scope,isolated,invalid};
enum class PoolClose:std::uint32_t {recycled,isolated,invalid};
struct PoolOpenCall {Request const* request{};State* state{};Receipt* receipt{};};
struct PoolCloseCall {State* state{};Receipt const* receipt{};Finish const* finish{};};
struct PoolDiagnostic {
    PoolPhase phase{PoolPhase::invalid};std::uint64_t generation{},minimum_revocation_generation{};
    std::uint32_t attempted{};bool receipt_frozen{};
    Request request{};Receipt receipt{};State const* state{};
};

// Provider contract: these NORMAL replies mean this exact cell was never
// admitted and has no Native references. They say nothing about registration
// quiescence. Keep this explicit list synchronized with the audited provider.
inline bool pool_no_scope_reply(gears_weapon_hand::Reply reply) noexcept {
    using Reply=gears_weapon_hand::Reply;
    return reply==Reply::unavailable||reply==Reply::busy||reply==Reply::draining||
        reply==Reply::wrong_cookie||reply==Reply::reentrant;
}
inline bool pool_same_receipt(Receipt const& a,Receipt const& b) noexcept {
    return a.size==b.size&&a.abi_version==b.abi_version&&a.hand_cookie==b.hand_cookie&&
        a.native_started_qpc==b.native_started_qpc&&a.scope_id==b.scope_id&&
        a.revocation_generation==b.revocation_generation&&a.thread_id==b.thread_id&&a.flags==b.flags;
}
inline bool pool_same_request(Request const& a,Request const& b) noexcept {
    return a.size==b.size&&a.abi_version==b.abi_version&&a.source==b.source&&a.flags==b.flags&&
        a.hand_cookie==b.hand_cookie&&a.native_started_qpc==b.native_started_qpc&&a.context==b.context&&
        a.object==b.object&&a.function==b.function&&a.thread_id==b.thread_id&&a.reserved==b.reserved;
}
inline bool pool_finish_shape(Finish const& f) noexcept {
    return f.size==sizeof(Finish)&&f.abi_version==version&&!(f.flags&~(stage_closed|handback_complete))&&
        (f.outcome==Outcome::returned||f.outcome==Outcome::original_exception||f.outcome==Outcome::notification_fault);
}
inline bool pool_same_finish(Finish const& a,Finish const& b) noexcept {
    return a.size==b.size&&a.abi_version==b.abi_version&&a.outcome==b.outcome&&a.flags==b.flags;
}

// A lower GenerationLimit is useful for bounded exhaustion tests; it can only
// make admission fail earlier. The default never wraps its 61-bit identity.
template<std::size_t N,std::uint64_t GenerationLimit=((std::numeric_limits<std::uint64_t>::max)()>>3)>
class StatePool {
    static_assert(N&&N<=(std::numeric_limits<std::uint32_t>::max)());
    static_assert(GenerationLimit&&GenerationLimit<=((std::numeric_limits<std::uint64_t>::max)()>>3));
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free&&std::atomic<std::uintptr_t>::is_always_lock_free&&
        std::atomic<std::uint32_t>::is_always_lock_free);
    static constexpr std::uint64_t phase_mask=7;
    struct Slot {
        State state{};
        Request request{},original_request{};
        Receipt output{},frozen{},close_receipt{};
        Finish finish{},original_finish{};
        std::uint64_t minimum{};
        std::atomic<std::uint64_t> word{0};
        std::atomic<std::uintptr_t> ticket_address{0};
        std::atomic<std::uint32_t> attempted{0};
        bool receipt_frozen{};std::array<std::byte,3> reserved{};
    };
    static_assert(sizeof(Slot)==848&&alignof(Slot)==alignof(State),"Explicit local layout, never a wire ABI");
    std::array<Slot,N> slots_{};
    static std::uint64_t word(std::uint64_t generation,PoolPhase phase) noexcept {
        return (generation<<3)|static_cast<std::uint64_t>(phase);
    }
    static PoolPhase phase_of(std::uint64_t value) noexcept {return static_cast<PoolPhase>(value&phase_mask);}
    static bool zero(State const& state) noexcept {
        for(auto byte:state.storage)if(byte!=std::byte{})return false;
        return true;
    }
    bool shape(PoolTicket const& ticket) const noexcept {
        return ticket.owner==reinterpret_cast<std::uintptr_t>(this)&&ticket.address==reinterpret_cast<std::uintptr_t>(&ticket)&&
            ticket.generation&&ticket.generation<=GenerationLimit&&ticket.index<N&&!ticket.reserved;
    }
    Slot const* find(PoolTicket const& ticket,std::uint64_t& value) const noexcept {
        if(!shape(ticket))return nullptr;
        auto const& slot=slots_[ticket.index];value=slot.word.load(std::memory_order_acquire);
        if((value>>3)!=ticket.generation||slot.ticket_address.load(std::memory_order_acquire)!=ticket.address)return nullptr;
        return &slot;
    }
    Slot* take(PoolTicket const& ticket,PoolPhase expected) noexcept {
        std::uint64_t slot_word{};if(!find(ticket,slot_word))return nullptr;
        auto& slot=slots_[ticket.index];auto value=word(ticket.generation,expected);
        if(slot.word.compare_exchange_strong(value,word(ticket.generation,PoolPhase::held),std::memory_order_acq_rel,std::memory_order_acquire))return &slot;
        // An illegal transition of this exact live ticket cannot reopen/free
        // anything. Copied/stale tickets never reach this mutation.
        const auto phase=phase_of(value);
        if((value>>3)==ticket.generation&&phase!=PoolPhase::free&&phase!=PoolPhase::isolated&&phase!=PoolPhase::exhausted)
            (void)slot.word.compare_exchange_strong(value,word(ticket.generation,PoolPhase::isolated),std::memory_order_acq_rel,std::memory_order_acquire);
        return nullptr;
    }
    static bool publish(Slot& slot,std::uint64_t generation,PoolPhase phase) noexcept {
        auto expected=word(generation,PoolPhase::held);
        return slot.word.compare_exchange_strong(expected,word(generation,phase),std::memory_order_acq_rel,std::memory_order_acquire);
    }
    static PoolOpen reject_open(Slot& slot,std::uint64_t generation) noexcept {
        (void)publish(slot,generation,PoolPhase::isolated);return PoolOpen::isolated;
    }
    static bool active_receipt(Slot const& slot) noexcept {
        auto const& r=slot.output;auto const& request=slot.original_request;
        return !(request.flags&unresolved)&&r.size==sizeof(Receipt)&&r.abi_version==version&&r.hand_cookie==request.hand_cookie&&
            r.native_started_qpc==request.native_started_qpc&&r.thread_id==request.thread_id&&r.scope_id&&
            r.revocation_generation&&r.revocation_generation>=slot.minimum&&r.flags==(scope_active|engine_calls_allowed);
    }
    static void diagnostic(Slot const& slot,std::uint64_t value,PoolDiagnostic& out) noexcept {
        out={};out.phase=phase_of(value);out.generation=value>>3;out.minimum_revocation_generation=slot.minimum;
        out.attempted=slot.attempted.load(std::memory_order_acquire);out.receipt_frozen=slot.receipt_frozen;
        out.request=slot.original_request;if(slot.receipt_frozen)out.receipt=slot.frozen;out.state=&slot.state;
    }
public:
    using Ticket=PoolTicket;
    StatePool()=default;
    StatePool(StatePool const&)=delete;StatePool& operator=(StatePool const&)=delete;
    StatePool(StatePool&&)=delete;StatePool& operator=(StatePool&&)=delete;
    // Tickets are single-owner stable-address POD journals. Do not share one
    // ticket concurrently, repair a copied ticket, or retain API pointers after
    // recycling. Distinct tickets may claim/retire different slots concurrently.
    bool claim(Ticket& ticket,Request const& request,std::uint64_t minimum_generation) noexcept {
        if(ticket.owner||ticket.address||ticket.generation||ticket.index||ticket.reserved||!request_valid(request)||!minimum_generation)return false;
        for(std::size_t i=0;i<N;++i){
            auto& slot=slots_[i];auto value=slot.word.load(std::memory_order_acquire);
            if(phase_of(value)!=PoolPhase::free)continue;
            const auto generation=value>>3;
            if(generation==GenerationLimit){
                (void)slot.word.compare_exchange_strong(value,word(generation,PoolPhase::exhausted),std::memory_order_acq_rel,std::memory_order_acquire);
                continue;
            }
            const auto next=generation+1;
            if(!slot.word.compare_exchange_strong(value,word(next,PoolPhase::held),std::memory_order_acq_rel,std::memory_order_acquire))continue;
            slot.state={};slot.request=request;slot.original_request=request;slot.output={};slot.frozen={};slot.close_receipt={};
            slot.finish={};slot.original_finish={};slot.minimum=minimum_generation;slot.receipt_frozen=false;
            slot.attempted.store(0,std::memory_order_relaxed);
            slot.ticket_address.store(reinterpret_cast<std::uintptr_t>(&ticket),std::memory_order_release);
            ticket={reinterpret_cast<std::uintptr_t>(this),reinterpret_cast<std::uintptr_t>(&ticket),next,static_cast<std::uint32_t>(i),0};
            if(!publish(slot,next,PoolPhase::ready))return false;
            return true;
        }
        return false;
    }
    bool begin_open(Ticket const& ticket,PoolOpenCall& call) noexcept {
        call={};auto* slot=take(ticket,PoolPhase::ready);if(!slot)return false;
        slot->attempted.fetch_or(1,std::memory_order_relaxed);
        if(!pool_same_request(slot->request,slot->original_request)||!zero(slot->state)||
           slot->output.size!=sizeof(Receipt)||slot->output.abi_version!=version||slot->output.flags||slot->output.scope_id){
            (void)publish(*slot,ticket.generation,PoolPhase::isolated);return false;
        }
        if(!publish(*slot,ticket.generation,PoolPhase::opening))return false;
        call={&slot->request,&slot->state,&slot->output};return true;
    }
    // Call ONLY after a normal Open return. Any SEH/C++/unknown completion uses
    // isolate(), even when State looks zero or Native may already have unlinked.
    PoolOpen open_returned(Ticket const& ticket,gears_weapon_hand::Reply reply) noexcept {
        auto* slot=take(ticket,PoolPhase::opening);if(!slot)return PoolOpen::invalid;
        slot->frozen=slot->output;slot->receipt_frozen=true; // Also retain malformed normal output for diagnostics.
        if(!pool_same_request(slot->request,slot->original_request))return reject_open(*slot,ticket.generation);
        if(reply==gears_weapon_hand::Reply::accepted){
            if(!active_receipt(*slot))return reject_open(*slot,ticket.generation);
            return publish(*slot,ticket.generation,PoolPhase::active)?PoolOpen::active:PoolOpen::isolated;
        }
        if(pool_no_scope_reply(reply)&&zero(slot->state)&&slot->output.size==sizeof(Receipt)&&
           slot->output.abi_version==version&&!slot->output.flags&&!slot->output.scope_id)
            return publish(*slot,ticket.generation,PoolPhase::free)?PoolOpen::no_scope:PoolOpen::isolated;
        return reject_open(*slot,ticket.generation);
    }
    bool begin_close(Ticket const& ticket,Finish const& finish,PoolCloseCall& call) noexcept {
        call={};auto* slot=take(ticket,PoolPhase::active);if(!slot)return false;
        // Open's output buffer may be overwritten by a caller after acceptance;
        // never use it again. Close gets a stable copy of the frozen receipt.
        if(!slot->receipt_frozen||!pool_finish_shape(finish)){
            (void)publish(*slot,ticket.generation,PoolPhase::isolated);return false;
        }
        slot->close_receipt=slot->frozen;slot->finish=finish;slot->original_finish=finish;
        slot->attempted.fetch_or(2,std::memory_order_relaxed);
        if(!publish(*slot,ticket.generation,PoolPhase::closing))return false;
        call={&slot->state,&slot->close_receipt,&slot->finish};return true;
    }
    // Provider End::closed MUST guarantee all references to this exact State
    // are gone. Local validation cannot prove Native unlink/TLS correctness.
    PoolClose close_returned(Ticket const& ticket,End end) noexcept {
        auto* slot=take(ticket,PoolPhase::closing);if(!slot)return PoolClose::invalid;
        const bool complete=slot->original_finish.flags==(stage_closed|handback_complete)&&
            slot->original_finish.outcome!=Outcome::notification_fault;
        if(end!=End::closed||!complete||!pool_same_receipt(slot->close_receipt,slot->frozen)||
           !pool_same_finish(slot->finish,slot->original_finish)){
            (void)publish(*slot,ticket.generation,PoolPhase::isolated);return PoolClose::isolated;
        }
        return publish(*slot,ticket.generation,PoolPhase::free)?PoolClose::recycled:PoolClose::isolated;
    }
    bool cancel_unopened(Ticket const& ticket) noexcept {
        auto* slot=take(ticket,PoolPhase::ready);if(!slot)return false;
        if(!zero(slot->state)||!pool_same_request(slot->request,slot->original_request)){
            (void)publish(*slot,ticket.generation,PoolPhase::isolated);return false;
        }
        return publish(*slot,ticket.generation,PoolPhase::free);
    }
    bool isolate(Ticket const& ticket) noexcept {
        std::uint64_t value{};if(!find(ticket,value))return false;
        const auto phase=phase_of(value);if(phase==PoolPhase::isolated)return true;
        if(phase==PoolPhase::free||phase==PoolPhase::exhausted)return false;
        return slots_[ticket.index].word.compare_exchange_strong(value,word(ticket.generation,PoolPhase::isolated),std::memory_order_acq_rel,std::memory_order_acquire);
    }
    PoolPhase phase(std::size_t index) const noexcept {
        return index<N?phase_of(slots_[index].word.load(std::memory_order_acquire)):PoolPhase::invalid;
    }
    // Same-ticket diagnostics need the single owner, outside an export call.
    bool describe(Ticket const& ticket,PoolDiagnostic& out) const noexcept {
        out={};std::uint64_t value{};auto const* slot=find(ticket,value);if(!slot)return false;
        const auto phase=phase_of(value);
        if(phase!=PoolPhase::ready&&phase!=PoolPhase::active&&phase!=PoolPhase::closing&&phase!=PoolPhase::isolated)return false;
        diagnostic(*slot,value,out);return true;
    }
    // Worker-only, after PG Admission is sealed/drained; never a concurrent
    // State-byte snapshot. Isolated private metadata is retained, not reset.
    bool describe_isolated(std::size_t index,PoolDiagnostic& out) const noexcept {
        out={};if(index>=N)return false;auto const& slot=slots_[index];const auto value=slot.word.load(std::memory_order_acquire);
        if(phase_of(value)!=PoolPhase::isolated)return false;diagnostic(slot,value,out);return true;
    }
    std::size_t retained() const noexcept {
        std::size_t count{};for(auto const& slot:slots_){const auto phase=phase_of(slot.word.load(std::memory_order_acquire));
            if(phase!=PoolPhase::free&&phase!=PoolPhase::exhausted)++count;}return count;
    }
    std::size_t isolated() const noexcept {
        std::size_t count{};for(auto const& slot:slots_)if(phase_of(slot.word.load(std::memory_order_acquire))==PoolPhase::isolated)++count;return count;
    }
};
} // namespace gears_weapon_boundary
