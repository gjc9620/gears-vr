#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace gears_hand_ik {
constexpr float trial_strength=.1f;
constexpr float trial_offset_units=1.f;
constexpr std::uint64_t minimum_stable_ms=250,maximum_pulse_ms=400,recovery_ms=250;
inline bool unload_allowed(bool restored_or_never_written,bool queued,unsigned status) noexcept {
    return restored_or_never_written&&!queued&&
        (status==0||status==4||status==5||status==6||status==7);
}
using Triple=std::array<float,3>;
inline bool finite(Triple const& values) noexcept {
    for(auto value:values)if(!std::isfinite(value))return false;
    return true;
}
inline bool exact(Triple const& a,Triple const& b) noexcept {
    return std::memcmp(a.data(),b.data(),sizeof(a))==0;
}
inline bool zero_strength(Triple const& values) noexcept {
    return finite(values)&&values[0]==0&&values[1]==0&&values[2]==0;
}
// The caller compares every instance, link, space, flag and target of all
// eight original controls. A mismatched/unknown observation starts over.
class StabilityGate {
    unsigned ticks_{};
    std::uint64_t began_{},previous_{};
public:
    void reset() noexcept {ticks_=0;began_=previous_=0;}
    unsigned ticks() const noexcept {return ticks_;}
    bool observe(std::uint64_t now,bool qualified,bool same_as_previous) noexcept {
        if(!qualified){reset();return false;}
        if(!ticks_||!same_as_previous||now<previous_){ticks_=1;began_=now;}
        else if(now>previous_)++ticks_;
        previous_=now;
        return ticks_>=8&&now>=began_&&now-began_>=minimum_stable_ms;
    }
};
// No fuzzy equality on rollback: even a small external change relinquishes
// ownership. Never restore a stale target over an original-game new value.
struct OwnedWrite {
    Triple original_target{},written_target{},written_strength{{trial_strength,trial_strength,0}};
    bool can_restore(Triple const& current_strength,Triple const& current_target) const noexcept {
        return finite(current_target)&&exact(current_target,written_target)&&
            exact(current_strength,written_strength);
    }
};
inline bool pulse_in_time(std::uint64_t wrote,std::uint64_t observed) noexcept {
    return observed>=wrote&&observed-wrote<=maximum_pulse_ms;
}
// Bone getters are not a request to force animation evaluation. Without a
// naturally advancing render timestamp and directional movement followed by
// return, a restored setter trial remains inconclusive.
inline bool directional_effect(Triple const& before,Triple const& after,
                               Triple const& left_before,Triple const& left_after,
                               bool natural_render_progress) noexcept {
    if(!natural_render_progress||!finite(before)||!finite(after)||
       !finite(left_before)||!finite(left_after))return false;
    const auto x=after[0]-before[0],y=after[1]-before[1],z=after[2]-before[2];
    const auto left=std::hypot(std::hypot(left_after[0]-left_before[0],
        left_after[1]-left_before[1]),left_after[2]-left_before[2]);
    return x>=.01f&&x<=.5f&&std::abs(y)<=.02f&&std::abs(z)<=.02f&&left<=.01f;
}
inline bool returned(Triple const& before,Triple const& after) noexcept {
    return finite(before)&&finite(after)&&std::hypot(std::hypot(before[0]-after[0],
        before[1]-after[1]),before[2]-after[2])<=.02f;
}
} // namespace gears_hand_ik
