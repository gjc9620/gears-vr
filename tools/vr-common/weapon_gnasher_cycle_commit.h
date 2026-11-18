#pragma once
#include "weapon_gnasher_fire.h"
#include "weapon_gnasher_cycle_readback.h"
namespace gears_weapon {
// A fresh current hold has independently re-read the original same actor and
// counts. Publish both private count consumers together without importing any
// former hand lease, motion sample, trigger edge or four-second clock reset.
inline bool gnasher_resume_held_consumers(GnasherMechanicalInteraction& mechanical,
    GnasherFireCycle& fire,GnasherInventory const& observed) noexcept {
    auto next_mechanical=mechanical;auto next_fire=fire;
    if(!next_mechanical.resume_hold(observed)||!next_fire.rebind_observation(observed))return false;
    mechanical=next_mechanical;fire=next_fire;return true;
}
// A known rejected/compensated insertion may release only its own proposal.
// Counts must be unchanged; preserving a prior shot debt is independent from
// releasing the still-held round for a NEW physical insertion gesture.
inline bool gnasher_reject_inserted_round(GnasherMechanicalInteraction& mechanical,
    GnasherFireCycle& fire,GnasherInventoryReceipt const& receipt) noexcept {
    const auto request=mechanical.inventory_request();
    if(receipt.accepted||!request.token||request.token!=receipt.request_token||
       receipt.observed.loaded!=request.before_loaded||receipt.observed.reserve!=request.before_reserve)return false;
    auto next_mechanical=mechanical;auto next_fire=fire;
    if(next_mechanical.acknowledge_inventory(receipt).event!=GnasherMechanicalEvent::rejected||
       next_mechanical.stage()==GnasherMechanicalStage::unbound||
       !next_fire.rebind_observation(receipt.observed))return false;
    mechanical=next_mechanical;fire=next_fire;return true;
}
// Original setters have already returned and their actual counts were read
// back. Publish the one-round receipt to both consumers or neither; do not
// erase a historical shot receipt, replay an insertion or clear cycle debt.
inline bool gnasher_commit_inserted_round(GnasherMechanicalInteraction& mechanical,
    GnasherFireCycle& fire,GnasherInventoryReceipt const& receipt) noexcept {
    const auto request=mechanical.inventory_request();
    if(!receipt.accepted||!request.token||request.token!=receipt.request_token)return false;
    auto next_mechanical=mechanical;auto next_fire=fire;
    if(next_mechanical.acknowledge_inventory(receipt).event!=GnasherMechanicalEvent::inventory_committed||
       !next_fire.acknowledge_insert(request,receipt))return false;
    mechanical=next_mechanical;fire=next_fire;return true;
}
// Commit both PRIVATE read-consumer states together. The engine's inventory
// backend is unchanged. The caller supplies a certified original endpoint ACK.
inline bool gnasher_commit_closed_cycle(GnasherMechanicalInteraction& mechanical,
    GnasherFireCycle& fire,GnasherCycleReceipt const& receipt,
    GnasherInventory const& observed) noexcept {
    const auto request=mechanical.cycle_request();
    if(!receipt.accepted||!request.token||request.token!=receipt.request_token||
       !(request.identity==receipt.identity))return false;
    auto next_mechanical=mechanical;auto next_fire=fire;
    if(!next_fire.can_request_cycle()||
       !next_fire.request_cycle(next_fire.confirmed()?next_fire.receipt().original_event_sequence:0,request.token)||
       !next_fire.acknowledge_cycle(request.token,observed))return false;
    if(next_mechanical.acknowledge_cycle(receipt,observed).event!=GnasherMechanicalEvent::cycle_accepted)return false;
    mechanical=next_mechanical;fire=next_fire;return true;
}
}
