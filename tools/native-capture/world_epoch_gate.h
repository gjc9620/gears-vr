#pragma once
#include <atomic>
#include <cstdint>
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
        // One attempt: contention and closed gates both reject without waiting.
        if((old&closed_bit)||(old&readers_mask)==readers_mask)return {};
        if(!state_.compare_exchange_strong(old,old+1,std::memory_order_acq_rel))return {};
        return Admission(this,std::uint32_t(old>>32));
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
}
