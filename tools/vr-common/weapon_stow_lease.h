#pragma once
#include <cstdint>

namespace gears_weapon {
enum class StowAttachment : std::uint8_t { unknown, original_hand, detached, other };

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
    bool reset() noexcept {
        if(dirty_)return false;
        key_={};faulted_=revoked_=false;return true;
    }
    template<class Backend> bool acquire(Backend& backend) {
        if(dirty_||faulted_)return false;
        StowState<Key> before{};
        if(!backend.read(before)||!before.ready||
           before.attachment!=StowAttachment::original_hand)return false;
        key_=before.key;
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
