#pragma once
#include "left_hand_interaction_owner.h"
#include "weapon_hand_participant.h"

namespace gears_weapon_hand {
inline bool proposal_valid(Proposal const& p,Frame const& f) noexcept {
    if(p.size!=sizeof(Proposal)||p.abi_version!=version||!same(p.key,f.key)||!p.nonce||
       p.support>1||p.ammo>1||p.magazine>1||p.magazine_observation_pending>1)return false;
    if(p.magazine_observation_pending&&(p.support||p.ammo||p.magazine||
       p.incumbents[0].kind!=LeaseKind::none||p.incumbents[1].kind!=LeaseKind::none||
       !f.left_grip_down||!f.left_grip_known||!f.left_tracked))return false;
    for(auto const& lease:p.incumbents)
        if(!lease_valid(lease,f.key.lifecycle)||lease.kind==LeaseKind::cover)return false;
    return p.incumbents[0].kind==LeaseKind::none||p.incumbents[1].kind==LeaseKind::none||
        !same(p.incumbents[0],p.incumbents[1]);
}
inline gears_vr::LeftHandInteractionLease owner_lease(Lease const& lease) noexcept {
    return {static_cast<gears_vr::LeftHandInteraction>(lease.kind),lease.lifecycle,lease.id};
}
inline void append_weapon_proposal(Proposal const& p,gears_vr::LeftHandInteractionFrame& frame) noexcept {
    // The Native owner validates proposal_valid before calling this converter.
    // Preserve Native's real cover incumbent. Two distinct entries already
    // expose a conflict; a third can never turn that into a free hand.
    for(auto const& lease:p.incumbents)if(lease.kind!=LeaseKind::none){
        const auto incoming=owner_lease(lease);bool present{};
        for(auto const& old:frame.incumbents)
            if(old.kind==incoming.kind&&old.lifecycle==incoming.lifecycle&&old.id==incoming.id)present=true;
        if(!present)for(auto& slot:frame.incumbents)if(slot.kind==gears_vr::LeftHandInteraction::none){slot=incoming;break;}
    }
    frame.requests.support=frame.requests.support||p.support!=0;
    frame.requests.ammo=frame.requests.ammo||p.ammo!=0;frame.requests.magazine=frame.requests.magazine||p.magazine!=0;
    frame.requests.magazine_observation_pending=frame.requests.magazine_observation_pending||p.magazine_observation_pending!=0;
}
inline Decision owner_decision(Frame const& f,Proposal const& p,
        gears_vr::LeftHandInteractionDecision const& d,std::int64_t now_ns) noexcept {
    Decision out{};out.key=f.key;out.proposal_nonce=p.nonce;out.press_token=d.press_token;out.now_ns=now_ns;
    out.lease={static_cast<LeaseKind>(d.lease.kind),0,d.lease.lifecycle,d.lease.id};
    out.status=static_cast<DecisionStatus>(d.status);out.new_grant=d.new_grant?1u:0u;out.release_required=d.release_required?1u:0u;
    return out;
}
// Prevent a private owner enum change from silently changing the public ABI.
#define GEARS_HAND_ENUM_MATCH(name) static_assert(unsigned(LeaseKind::name)==unsigned(gears_vr::LeftHandInteraction::name))
GEARS_HAND_ENUM_MATCH(none);GEARS_HAND_ENUM_MATCH(support);GEARS_HAND_ENUM_MATCH(ammo);
GEARS_HAND_ENUM_MATCH(magazine);GEARS_HAND_ENUM_MATCH(cover);GEARS_HAND_ENUM_MATCH(other);
#undef GEARS_HAND_ENUM_MATCH
#define GEARS_HAND_STATUS_MATCH(name) static_assert(unsigned(DecisionStatus::name)==unsigned(gears_vr::LeftHandInteractionStatus::name))
GEARS_HAND_STATUS_MATCH(released);GEARS_HAND_STATUS_MATCH(retained);GEARS_HAND_STATUS_MATCH(granted);
GEARS_HAND_STATUS_MATCH(needs_release);GEARS_HAND_STATUS_MATCH(no_candidate);GEARS_HAND_STATUS_MATCH(incumbent_conflict);
GEARS_HAND_STATUS_MATCH(candidate_conflict);GEARS_HAND_STATUS_MATCH(inactive);GEARS_HAND_STATUS_MATCH(lifecycle_changed);
GEARS_HAND_STATUS_MATCH(invalid);GEARS_HAND_STATUS_MATCH(replayed);GEARS_HAND_STATUS_MATCH(ambiguous);GEARS_HAND_STATUS_MATCH(lease_lost);
#undef GEARS_HAND_STATUS_MATCH
} // namespace gears_weapon_hand
