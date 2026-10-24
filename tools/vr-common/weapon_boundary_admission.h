#pragma once
#include "weapon_action_boundary.h"
#include <atomic>
#include <limits>

// Local PG lifetime only. No Native scope, permission, engine call or arbiter
// is created here. The host retains its Native HMODULE until this channel is
// sealed, drained and detached, including Open's unsafe/no-scope outcome.
namespace gears_weapon_boundary {
struct Binding {
    std::uint64_t cookie{},native_started_qpc{};
    void* context{};Revoke revoke{};Open open{};Close close{};
};
class Admission {
public:
    struct Entry {
        Admission const* owner{};std::uintptr_t address{};
        std::uint64_t generation{};unsigned flags{};
    };
private:
    static constexpr std::uint64_t closing=std::uint64_t{1}<<63,count_mask=closing-1;
    enum EntryFlags:unsigned {claimed=1,release_attempted=2,released=4};
    std::atomic<std::uint64_t> state_{closing};
    std::atomic<bool> faulted_{};
    Binding binding_{};std::uint64_t generation_{};bool bound_{};
    void fault() noexcept {faulted_.store(true,std::memory_order_release);seal();}
    bool entry_shape(Entry const& entry) const noexcept {
        return entry.owner==this&&entry.address==reinterpret_cast<std::uintptr_t>(&entry)&&
            entry.generation&&(entry.flags&claimed)&&!(entry.flags&~7u);
    }
public:
    Admission()=default;
    Admission(Admission const&)=delete;Admission& operator=(Admission const&)=delete;
    // Host control operations are serialized off GT. All callback admission
    // and retirement share ONE closing-bit/count CAS; no count/alive Dekker
    // check can race a worker into clearing callback state or freeing Native.
    bool connect(Binding const& binding) noexcept {
        if(bound_||state_.load(std::memory_order_acquire)!=closing||faulted_.load(std::memory_order_acquire)||
           !binding.cookie||!binding.native_started_qpc||!binding.context||!binding.revoke||!binding.open||!binding.close)return false;
        if(generation_==(std::numeric_limits<std::uint64_t>::max)()){fault();return false;}
        ++generation_;binding_=binding;bound_=true;
        state_.store(0,std::memory_order_release);return true;
    }
    bool acquire(Entry& entry) noexcept {
        if(entry.owner||entry.address||entry.generation||entry.flags){fault();return false;}
        auto value=state_.load(std::memory_order_acquire);
        for(;;){
            if(value&closing)return false;
            if((value&count_mask)==count_mask){fault();return false;}
            if(state_.compare_exchange_weak(value,value+1,std::memory_order_acq_rel,std::memory_order_acquire))break;
        }
        entry.owner=this;entry.address=reinterpret_cast<std::uintptr_t>(&entry);
        entry.generation=generation_;entry.flags=claimed;return true;
    }
    Binding const* view(Entry const& entry) const noexcept {
        // No API pointer or non-atomic binding read before a claimed entry.
        return entry_shape(entry)&&!(entry.flags&release_attempted)&&entry.generation==generation_?&binding_:nullptr;
    }
    bool release(Entry& entry) noexcept {
        if(!entry_shape(entry)){fault();return false;}
        if(entry.flags&release_attempted)return (entry.flags&released)!=0;
        entry.flags|=release_attempted; // Failure must never steal a later lease on retry.
        if(entry.generation!=generation_){fault();return false;}
        auto value=state_.load(std::memory_order_acquire);
        do{if(!(value&count_mask)){fault();return false;}}
        while(!state_.compare_exchange_weak(value,value-1,std::memory_order_acq_rel,std::memory_order_acquire));
        entry.flags|=released;return true;
    }
    // Call before HandClient::begin_disconnect/finish_disconnect and before
    // looking at counts. No new Entry can pass after this linearization point.
    void seal() noexcept {state_.fetch_or(closing,std::memory_order_acq_rel);}
    void quarantine() noexcept {fault();}
    bool quiescent() const noexcept {
        return state_.load(std::memory_order_acquire)==closing&&!faulted_.load(std::memory_order_acquire);
    }
    bool detach() noexcept {
        if(!quiescent())return false;
        binding_={};bound_=false;return true;
    }
    std::uint64_t in_flight() const noexcept {return state_.load(std::memory_order_acquire)&count_mask;}
    bool faulted() const noexcept {return faulted_.load(std::memory_order_acquire);}
};
static_assert(std::is_standard_layout_v<Admission::Entry>&&std::is_trivially_copyable_v<Admission::Entry>);
} // namespace gears_weapon_boundary
