#pragma once
#include <cstdint>
#include <limits>

namespace gears_weapon {
enum class StowPendingResult : std::uint8_t { waiting,ready,cancelled };

// Single-consumer bounded wait only; no game calls, ownership or journal.
// Key equality must cover every identity the caller requires. The caller must
// authenticate attachment/flags and restrict can_wait to permitted game states.
template<class Key> class EquipmentStowPending {
    Key key_{};
    std::uint64_t last_ms_{},deadline_ms_{};
    bool active_{};
public:
    static constexpr std::uint64_t maximum_timeout_ms=1000;

    bool active() const noexcept {return active_;}
    Key const& key() const noexcept {return key_;} // Current/last key; meaningful for writes only while active.

    bool begin(Key const& key,std::uint64_t now_ms,std::uint64_t timeout_ms) {
        if(active_||!timeout_ms||timeout_ms>maximum_timeout_ms||
           now_ms>std::numeric_limits<std::uint64_t>::max()-timeout_ms)return false;
        key_=key;last_ms_=now_ms;deadline_ms_=now_ms+timeout_ms;active_=true;
        return true;
    }

    StowPendingResult poll(bool key_known,Key const& current_key,std::uint64_t now_ms,
                           bool ready,bool can_wait,bool stopped) {
        if(!active_)return StowPendingResult::cancelled;
        // Readiness must never outrank lost authentication, a stopped owner,
        // a disallowed state or an expired/wrapped/non-monotonic clock.
        if(!key_known||!(current_key==key_)||now_ms<last_ms_||
           now_ms>=deadline_ms_||stopped||!can_wait){
            active_=false;return StowPendingResult::cancelled;
        }
        last_ms_=now_ms; // Same-now polls are allowed; the deadline never extends.
        if(ready){active_=false;return StowPendingResult::ready;}
        return StowPendingResult::waiting;
    }

    // Lifecycle only, after the caller has drained its callbacks/completion owner.
    void reset() {key_={};last_ms_=deadline_ms_=0;active_=false;}
};
}
