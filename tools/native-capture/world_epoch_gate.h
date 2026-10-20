#pragma once
#include <atomic>
#include <cstdint>
#include <exception>
#include <limits>
#include <utility>

namespace gears_world {
// Callers manage recursive depth per thread: enter only at depth zero and keep
// that admission alive across nested callbacks. A rejected entry must not wait.
// This admission epoch advances on every successful close/open, including an
// aborted handoff; it is not automatically the renderer's committed world ID.
class EpochGate {
    static constexpr std::uint64_t closed_bit=std::uint64_t(1)<<31;
    static constexpr std::uint64_t readers_mask=closed_bit-1;
    std::atomic<std::uint64_t> state_{std::uint64_t(1)<<32};
    void leave(){state_.fetch_sub(1,std::memory_order_release);}
public:
    class Admission {
        friend class EpochGate;
        EpochGate* gate_{};
        std::uint32_t epoch_{};
        Admission(EpochGate* gate,std::uint32_t epoch):gate_(gate),epoch_(epoch){}
    public:
        Admission()=default;
        Admission(Admission const&)=delete;
        Admission& operator=(Admission const&)=delete;
        Admission(Admission&& other) noexcept:gate_(std::exchange(other.gate_,nullptr)),epoch_(other.epoch_){}
        Admission& operator=(Admission&&)=delete;
        ~Admission(){if(gate_)gate_->leave();}
        explicit operator bool()const{return gate_!=nullptr;}
        std::uint32_t epoch()const{return epoch_;}
    };
    struct CloseToken {EpochGate const* owner{};std::uint32_t epoch{};};
    Admission try_enter(){
        auto old=state_.load(std::memory_order_acquire);
        // Retry ordinary open-reader contention: dropping instrumentation here
        // would create unobserved GPU work even without a world transition.
        // This is lock-free, not wait-free. A closed writer is never waited on.
        for(;;){
            if((old&closed_bit)||(old&readers_mask)==readers_mask)return {};
            if(state_.compare_exchange_weak(old,old+1,std::memory_order_acq_rel,std::memory_order_acquire))
                return Admission(this,std::uint32_t(old>>32));
        }
    }
    bool try_close(CloseToken& token){
        token={};auto old=state_.load(std::memory_order_acquire);
        if((old&0xffffffffull)||std::uint32_t(old>>32)==UINT32_MAX)return false;
        if(!state_.compare_exchange_strong(old,old|closed_bit,std::memory_order_acq_rel))return false;
        token={this,std::uint32_t(old>>32)};return true;
    }
    bool open(CloseToken const& token){
        if(token.owner!=this||!token.epoch||token.epoch==UINT32_MAX)return false;
        auto expected=(std::uint64_t(token.epoch)<<32)|closed_bit;
        return state_.compare_exchange_strong(expected,std::uint64_t(token.epoch+1)<<32,std::memory_order_acq_rel);
    }
};
// One instance must be owned by one thread (normally an external thread_local).
// Pass that same instance through every recursive hook for this gate. There is
// deliberately no header-owned TLS or static state. Never share across threads.
struct ThreadState {
    std::uint32_t depth{},epoch{};
    bool admitted{};
    EpochGate const* gate{};
    const std::uint32_t maximum_depth;
    explicit ThreadState(std::uint32_t limit=(std::numeric_limits<std::uint32_t>::max)()):maximum_depth(limit){}
    ThreadState(ThreadState const&)=delete;
    ThreadState& operator=(ThreadState const&)=delete;
};
class RecursiveAdmission {
    ThreadState& state_;
    // Only an outer instance owns a lease. Declared after state_ so its release
    // occurs after the destructor body has matched and cleared thread state.
    EpochGate::Admission admission_;
    std::uint32_t entered_depth_{},epoch_{};
    bool admitted_{};
public:
    RecursiveAdmission(ThreadState& state,EpochGate& gate):state_(state),
        admission_(state.depth==0&&state.maximum_depth?gate.try_enter():EpochGate::Admission{}){
        if(state.depth){
            // A foreign gate or exhausted recursion cannot disturb the current
            // outer admission (including an outer rejection).
            if(state.gate!=&gate||state.depth>=state.maximum_depth)return;
            entered_depth_=++state.depth;admitted_=state.admitted;epoch_=state.epoch;
        }else if(state.maximum_depth){
            state.gate=&gate;state.depth=entered_depth_=1;
            state.admitted=admitted_=bool(admission_);state.epoch=epoch_=admission_.epoch();
        }
    }
    RecursiveAdmission(RecursiveAdmission const&)=delete;
    RecursiveAdmission& operator=(RecursiveAdmission const&)=delete;
    RecursiveAdmission(RecursiveAdmission&&)=delete;
    RecursiveAdmission& operator=(RecursiveAdmission&&)=delete;
    ~RecursiveAdmission(){
        if(!entered_depth_)return;
        // Non-LIFO destruction is a caller ownership error. Never silently
        // reopen a gate while a deeper callback still relies on its admission.
        if(state_.depth!=entered_depth_)std::terminate();
        if(--state_.depth==0){state_.gate=nullptr;state_.admitted=false;state_.epoch=0;}
    }
    explicit operator bool()const{return admitted_;}
    std::uint32_t epoch()const{return epoch_;}
};
}
