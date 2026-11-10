#pragma once
#include "weapon_hand_participant.h"

// HB's contribution to a read-consumer observation must not change halfway
// through the EXACT Frame that just granted a part. Shoulder supply remains
// the default; the initial latch must explicitly select magazine kind. POD only
// identifies that acquisition Frame. It cannot grant a lease or renew input.
namespace gears_hammerburst_supply_publication {
namespace hand=gears_weapon_hand;
struct Snapshot {
    hand::FrameKey key{};
    hand::Lease lease{};
    std::uint64_t sequence{},token{},input_epoch{},profile{},reference{};
    std::int64_t sync{};
    bool known{};
    void reset() noexcept {*this={};}
    bool remember(hand::Frame const& f,hand::Decision const& d,
                  hand::LeaseKind expected=hand::LeaseKind::ammo) noexcept {
        reset();
        if(!hand::same(f.key,d.key)||!d.proposal_nonce||!d.new_grant||d.release_required||
           (expected!=hand::LeaseKind::ammo&&expected!=hand::LeaseKind::magazine)||
           d.status!=hand::DecisionStatus::granted||d.lease.kind!=expected||
           !hand::lease_valid(d.lease,f.key.lifecycle)||d.lease.id!=f.press_token||
           d.press_token!=f.press_token||!f.left_grip_known||!f.left_grip_down||!f.left_tracked||
           !f.key.frame||!f.controllers.sequence||f.controllers.sync_ns<=0)return false;
        key=f.key;lease=d.lease;sequence=f.controllers.sequence;sync=f.controllers.sync_ns;
        token=f.press_token;input_epoch=f.controllers.input_epoch;profile=f.controllers.profile_generation;
        reference=f.reference_generation;known=true;return true;
    }
    bool same_acquisition_source(hand::Frame const& f,hand::Lease const& held,std::uint64_t item) const noexcept {
        return known&&hand::same(key,f.key)&&hand::same(lease,held)&&item==token&&f.press_token==token&&
            f.left_grip_known&&f.left_grip_down&&f.left_tracked&&
            f.controllers.sequence==sequence&&f.controllers.sync_ns==sync&&
            f.controllers.input_epoch==input_epoch&&f.controllers.profile_generation==profile&&
            f.reference_generation==reference;
    }
};
}
