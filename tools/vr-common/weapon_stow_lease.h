#pragma once
#include <cstdint>
#include <array>

namespace gears_weapon {
constexpr std::uint64_t stow_local_component_argument(std::array<unsigned char,10> const& bytes) noexcept {
    // Read-only classification of exactly LocalVariable(UProperty*) followed
    // by EndFunctionParms. Never a VM evaluator or a consumed FFrame.
    if(bytes[0]!=0x00||bytes[9]!=0x16)return 0;
    std::uint64_t property{};
    for(unsigned i=0;i<8;++i)property|=std::uint64_t(bytes[1+i])<<(i*8);
    return property;
}
enum class StowAttachment : std::uint8_t { unknown, original_hand, detached, other };
enum class StowPhase : std::uint8_t { unknown, active, equipping };
constexpr bool empty_pickup_touch_body(std::array<unsigned char,3> const& bytes,unsigned count,
    unsigned flags,unsigned return_offset,bool script_entry) noexcept {
    // Hash-verified retail WarDroppedPickup.Pickup.Touch: Return, Nothing,
    // EndOfScript. Never exempt native/unknown or non-void handlers.
    return script_entry&&!(flags&0x400)&&return_offset==0xffff&&count==3&&
        bytes==std::array<unsigned char,3>{0x04,0x0b,0x53};
}
// Visual holstering of an already-unheld new item may precede its equip timer.
// Never broaden explicit stow/switch/fire or admit a reload/unknown phase.
constexpr bool stow_phase_allowed(StowPhase phase,bool can_switch,bool busy,
                                  bool idle_unheld,bool clear_transition) noexcept {
    return can_switch&&clear_transition&&((phase==StowPhase::active&&!busy)||
        (phase==StowPhase::equipping&&idle_unheld));
}

// Retain an already-owned visibility interval across the equip -> Active
// boundary while the body's original blend still prevents normal holstering.
// This never grants a new hide, and unknown/special states restore normally.
constexpr bool retain_equip_visibility(StowPhase phase,bool can_switch,bool busy,
                                       bool clear_transition) noexcept {
    return clear_transition&&(phase==StowPhase::equipping||
        (phase==StowPhase::active&&(!can_switch||busy)));
}

// Performance scheduling only, never an engine/action permit. A newly current
// weapon must not inherit another weapon's cooldown, and a busy equip animation
// must not spend the first eligible attempt before it can actually be stowed.
template<class Key> class StowIdleCadence {
    Key key_{};bool have_{};
    std::int64_t observed_{},attempted_{},next_{};
public:
    bool attempt(Key const& key,std::int64_t now,bool eligible) noexcept {
        if(now<=0||now<observed_)return false;
        observed_=now;if(!eligible)return false;
        if(have_&&key==key_&&(now<=attempted_||now<next_))return false;
        key_=key;have_=true;attempted_=now;
        next_=now>INT64_MAX-500000000?INT64_MAX:now+500000000;
        return true;
    }
    void reset() noexcept {*this=StowIdleCadence{};}
};

// Key equality must cover the live object's generation, player, inventory and
// component identities. The backend authenticates them again on every read.
// This journal cannot infer ownership from a detached component. Its caller
// must restore before handing a relevant original mutation to the game, and
// must never reuse the journal across an unobserved action/world boundary.
template<class Key> struct StowState {
    Key key{};
    StowAttachment attachment{StowAttachment::unknown};
    bool ready{};
};

template<class Key> class WeaponStowLease {
    Key key_{};
    bool dirty_{},faulted_{},revoked_{};
    template<class Backend> bool fail(Backend& backend) {
        faulted_=true;
        restore(backend);
        return false;
    }
public:
    bool dirty() const noexcept {return dirty_;}
    bool faulted() const noexcept {return faulted_;}
    bool revoked() const noexcept {return revoked_;}
    void revoke() noexcept {if(dirty_)faulted_=revoked_=true;}
    Key const& key() const noexcept {return key_;}
    // Read-only witness for an ALREADY owned detach/visibility interval.
    // This cannot acquire, re-hide, renew a Frame or revive revoked rights.
    // Empty-hand presentation may remain hidden across animation/state
    // transitions that are ineligible to create a NEW holster operation.
    template<class Backend> bool retains_detached(Backend& backend) const {
        if(!dirty_||faulted_||revoked_)return false;
        StowState<Key> current{};
        return backend.read(current)&&dirty_&&!faulted_&&!revoked_&&
            current.ready&&current.key==key_&&current.attachment==StowAttachment::detached;
    }
    bool reset() noexcept {
        if(dirty_)return false;
        key_={};faulted_=revoked_=false;return true;
    }
    template<class Backend> bool acquire(Backend& backend) {
        if(dirty_||faulted_)return false;
        StowState<Key> before{};
        if(!backend.read(before)||!before.ready||
           before.attachment!=StowAttachment::original_hand)return false;
        key_=before.key;revoked_=false; // A new explicit, authenticated lease.
        // Even a failed reflected call may have detached the mesh.
        dirty_=true;
        const bool called=backend.set_holstered(true);
        StowState<Key> after{};
        if(!backend.read(after)||!after.ready||!(after.key==key_)||
           (after.attachment!=StowAttachment::detached&&
            after.attachment!=StowAttachment::original_hand))revoke();
        if(revoked_||after.attachment!=StowAttachment::detached||!called)return fail(backend);
        return true;
    }
    template<class Backend> bool relinquish(Backend& backend) {
        if(!dirty_)return true;
        // The adapter has already observed the original hand. End all writes
        // before this read can re-enter engine code; detached must never turn
        // that observation back into permission to call false.
        revoked_=true;
        StowState<Key> current{};
        if(!backend.read(current)||!current.ready||!(current.key==key_)||
           current.attachment!=StowAttachment::original_hand){faulted_=true;return false;}
        dirty_=false;
        return true;
    }
    template<class Backend> bool restore(Backend& backend) {
        if(!dirty_)return true;
        StowState<Key> current{};
        if(!backend.read(current)||!current.ready||!(current.key==key_)){
            revoke();return false;
        }
        if(current.attachment==StowAttachment::original_hand){dirty_=false;return true;}
        // Lost authentication/foreign state permanently revokes writes for
        // this lease. A later detached state must never reacquire ownership.
        // Read-only confirmation of the original hand may still clear dirty.
        if(revoked_)return false;
        // An unknown attachment or another bone/component is foreign state.
        // Never use the original helper to overwrite it.
        if(current.attachment!=StowAttachment::detached){revoke();return false;}
        const bool called=backend.set_holstered(false);
        if(!called)faulted_=true;
        StowState<Key> restored{};
        if(!backend.read(restored)||!restored.ready||!(restored.key==key_)||
           restored.attachment!=StowAttachment::original_hand){revoke();return false;}
        dirty_=false;
        return true;
    }
};
} // namespace gears_weapon
