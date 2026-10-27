#pragma once
#include <cstdint>
#include <limits>
#include <type_traits>

namespace gears_hand_session {
constexpr std::uint64_t recording_ms=15000;
constexpr unsigned recording_leases=256,event_limit=1024;

// Lifetime only: none of these methods authenticates a pose or grants a mask.
class Session {
    std::uint64_t began_{};
    unsigned recorded_{},events_{};
    bool begun_{},ended_{},recording_closed_{};
public:
    void begin(std::uint64_t now) noexcept {began_=now;begun_=true;}
    void end() noexcept {ended_=true;recording_closed_=true;}
    bool running(std::uint64_t now,bool stop,bool shutdown) const noexcept {
        return begun_&&!ended_&&!stop&&!shutdown&&now>=began_;
    }
    bool recording(std::uint64_t now) noexcept {
        if(!begun_||recording_closed_)return false;
        if(now<began_||now-began_>recording_ms||recorded_>=recording_leases){
            recording_closed_=true;return false;
        }
        return true;
    }
    bool record_lease(std::uint64_t now) noexcept {
        if(!recording(now))return false;
        ++recorded_;return true;
    }
    bool diagnostic_event(std::uint64_t now) noexcept {
        if(!recording(now)||events_==event_limit)return false;
        ++events_;return true;
    }
    unsigned recorded_leases() const noexcept {return recorded_;}
    unsigned diagnostic_events() const noexcept {return events_;}
};

// The continuous writer has a finite per-Plan cutoff from original stamps.
// Overflow is a rejection; neither receiving now nor a renewed session age is
// a substitute for any one of the three authenticated observation times.
inline bool write_until(std::uint64_t ticket,std::uint64_t plan_context,
                        std::uint64_t current_context,std::uint64_t& out) noexcept {
    out=0;
    constexpr auto last=std::numeric_limits<std::uint64_t>::max()-100;
    if(ticket>last||plan_context>last||current_context>last)return false;
    auto earliest=ticket<plan_context?ticket:plan_context;
    earliest=earliest<current_context?earliest:current_context;
    out=earliest+100;return true;
}
template<class T> bool saturating_add(T& value,T amount=T{1}) noexcept {
    static_assert(std::is_unsigned_v<T>);
    const auto maximum=std::numeric_limits<T>::max();
    if(amount>maximum-value){value=maximum;return false;}
    value+=amount;return true;
}
inline unsigned status_leases(unsigned total) noexcept {return total>65535u?65535u:total;}
} // namespace gears_hand_session
