#pragma once
#include "weapon_hammer_of_dawn_interaction.h"

namespace gears_weapon {
// Effect debt is separate from the shared R2 edge ledger. A successful button
// release can still leave original beam/audio cleanup pending. One GT owner;
// all mutations are serial- and incarnation-scoped, including failed calls.
class HammerOfDawnUseOwner {
    HammerOfDawnIdentity identity_{};
    std::uint64_t serial_{},last_serial_{};
    bool press_invoked_{},press_returned_{},release_claimed_{},release_invoked_{},
         release_returned_{},fallback_claimed_{},fallback_invoked_{};
    bool matches(std::uint64_t serial,HammerOfDawnIdentity const& id) const noexcept {
        return serial_&&serial==serial_&&id==identity_;
    }
    void clear() noexcept {
        identity_={};serial_=0;
        press_invoked_=press_returned_=release_claimed_=release_invoked_=
            release_returned_=fallback_claimed_=fallback_invoked_=false;
    }
public:
    bool retained() const noexcept {return serial_!=0;}
    std::uint64_t serial() const noexcept {return serial_;}
    HammerOfDawnIdentity identity() const noexcept {return identity_;}
    bool press_invoked() const noexcept {return press_invoked_;}
    bool release_invoked() const noexcept {return release_invoked_;}
    bool call_unknown() const noexcept {
        return (press_invoked_&&!press_returned_)||(release_invoked_&&!release_returned_);
    }
    bool reserve(HammerOfDawnIdentity const& id,std::uint64_t serial) noexcept {
        if(retained()||!id.valid()||!serial||serial<=last_serial_)return false;
        identity_=id;serial_=last_serial_=serial;return true;
    }
    bool pressed(std::uint64_t serial,bool invoked,bool returned) noexcept {
        if(!serial_||serial!=serial_||press_invoked_||release_claimed_||returned&&!invoked)return false;
        if(!invoked){clear();return true;}
        press_invoked_=true;press_returned_=returned;return true;
    }
    bool begin_release(std::uint64_t serial,HammerOfDawnIdentity const& current) noexcept {
        if(!matches(serial,current)||!press_invoked_||release_claimed_||release_invoked_)return false;
        release_claimed_=true;return true;
    }
    bool released(std::uint64_t serial,bool invoked,bool returned) noexcept {
        if(!serial_||serial!=serial_||!release_claimed_||release_invoked_||returned&&!invoked)return false;
        if(!invoked){release_claimed_=false;return true;}
        release_invoked_=true;release_returned_=returned;return true;
    }
    bool begin_fallback(std::uint64_t serial,HammerOfDawnIdentity const& current,
                        bool original_still_active) noexcept {
        if(!matches(serial,current)||!release_invoked_||!original_still_active||fallback_claimed_||fallback_invoked_)
            return false;
        fallback_claimed_=true;return true;
    }
    bool fallback(std::uint64_t serial,bool invoked) noexcept {
        if(!serial_||serial!=serial_||!fallback_claimed_||fallback_invoked_)return false;
        if(!invoked){fallback_claimed_=false;return true;}
        fallback_invoked_=true;return true;
    }
    bool stopped(std::uint64_t serial,HammerOfDawnStopReadback const& proof) noexcept {
        if(!serial_||serial!=serial_||!press_invoked_||!release_invoked_||
           !proof.complete(identity_,serial_,proof.original_event_serial))return false;
        clear();return true;
    }
};
} // namespace gears_weapon
