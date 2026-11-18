#pragma once
#include "weapon_gnasher_mechanics.h"

namespace gears_weapon {
// Read-consumer only. The common inventory backend remains the sole writer.
// A root CustomFire return is a possible shot; only post-shot original counts
// confirm it. Multiple pellet calls never issue independent cycle obligations.
class GnasherFireCycle {
    GnasherInventory before_{};
    GnasherShotReceipt receipt_{};
    std::uint64_t last_event_{},pending_event_{},cycle_token_{},last_insert_token_{};
    std::int64_t returned_ns_{},game_tick_{};
    bool bound_{},debt_{},confirmed_{},faulted_{},reload_debt_{};
public:
    bool bind(GnasherInventory const& inventory) noexcept {
        if(bound_||!inventory.identity.valid()||!inventory.revision)return false;
        before_=inventory;bound_=true;return true;
    }
    bool prepare(GnasherInventory const& observed) noexcept {
        if(!bound_||faulted_||debt_||pending_event_||!(observed.identity==before_.identity)||
           observed.revision<before_.revision||observed.loaded!=before_.loaded||
           observed.reserve!=before_.reserve)return false;
        before_=observed;return true;
    }
    // Fresh input/hold binding after the SAME original actor's inventory was
    // independently read back. Retain the unpaid cycle and historical shot;
    // this transfers no controller edge, pose or mechanical stroke.
    bool rebind_observation(GnasherInventory const& observed) noexcept {
        const auto& a=before_.identity;const auto& b=observed.identity;
        if(!bound_||faulted_||pending_event_||cycle_token_||!b.valid()||
           a.world!=b.world||a.pawn!=b.pawn||a.weapon!=b.weapon||a.world_generation!=b.world_generation||
           observed.revision<before_.revision||observed.loaded!=before_.loaded||observed.reserve!=before_.reserve)return false;
        before_=observed;return true;
    }
    bool returned(std::uint64_t event,std::int64_t now_ns,std::int64_t game_tick) noexcept {
        if(!bound_||!event||event<=last_event_||now_ns<=0||game_tick<0)return false;
        last_event_=event;
        if(debt_||pending_event_||!before_.loaded){faulted_=true;debt_=true;return false;}
        pending_event_=event;returned_ns_=now_ns;game_tick_=game_tick;debt_=true;return true;
    }
    // The exact original root returned, but its tracking/source cohort could
    // not be certified. Deny further fire without inventing a shot receipt or
    // permitting a count/cycle ACK to erase the unresolved original action.
    bool unresolved_return(std::uint64_t event) noexcept {
        if(!bound_||!event||event<=last_event_)return false;
        last_event_=event;debt_=faulted_=true;return true;
    }
    bool confirm(GnasherInventory const& observed) noexcept {
        if(!bound_||faulted_||!pending_event_||!(observed.identity==before_.identity)||
           observed.revision<=before_.revision)return false;
        if(!before_.loaded||observed.loaded!=before_.loaded-1||observed.reserve!=before_.reserve){
            faulted_=true;return false;
        }
        receipt_={observed.identity,pending_event_,returned_ns_,game_tick_,observed};
        before_=observed;pending_event_=0;confirmed_=true;return true;
    }
    // An owned original one-round insertion updates the current count ledger,
    // while preserving any HISTORICAL shot receipt and its unpaid cycle.
    // This is a receipt consumer; it never dispatches an inventory setter.
    bool acknowledge_insert(GnasherInventoryRequest const& request,
                            GnasherInventoryReceipt const& receipt) noexcept {
        if(!bound_||faulted_||pending_event_||cycle_token_||!request.token||
           request.token<=last_insert_token_||!request.round_serial||!receipt.accepted||
           receipt.request_token!=request.token||!(request.identity==before_.identity)||
           !(receipt.observed.identity==before_.identity)||request.before_revision!=before_.revision||
           request.before_loaded!=before_.loaded||request.before_reserve!=before_.reserve||
           !before_.reserve||before_.loaded>=10000||request.after_loaded!=before_.loaded+1||
           request.after_reserve!=before_.reserve-1||receipt.observed.revision<=before_.revision||
           receipt.observed.loaded!=request.after_loaded||receipt.observed.reserve!=request.after_reserve)return false;
        before_=receipt.observed;last_insert_token_=request.token;reload_debt_=debt_=true;return true;
    }
    bool can_request_cycle() const noexcept {
        return bound_&&!faulted_&&debt_&&!pending_event_&&!cycle_token_&&(confirmed_||reload_debt_);
    }
    bool request_cycle(std::uint64_t shot,std::uint64_t token) noexcept {
        if(!can_request_cycle()||!token||shot!=(confirmed_?receipt_.original_event_sequence:0))return false;
        cycle_token_=token;return true;
    }
    // The adapter calls this only after that token's original completion
    // returned normally and the same original inventory was read back.
    bool acknowledge_cycle(std::uint64_t token,GnasherInventory const& observed) noexcept {
        if(faulted_||!debt_||(!confirmed_&&!reload_debt_)||!cycle_token_||token!=cycle_token_||
           !(observed.identity==before_.identity)||observed.revision<before_.revision||
           observed.loaded!=before_.loaded||observed.reserve!=before_.reserve)return false;
        before_=observed;cycle_token_=0;debt_=confirmed_=reload_debt_=false;return true;
    }
    bool blocked() const noexcept{return debt_||pending_event_||faulted_;}
    bool pending() const noexcept{return pending_event_!=0;}
    bool confirmed() const noexcept{return confirmed_;}
    bool reload_cycle_pending() const noexcept{return reload_debt_;}
    bool faulted() const noexcept{return faulted_;}
    GnasherShotReceipt receipt() const noexcept{return receipt_;}
    GnasherInventory inventory() const noexcept{return before_;}
};
}
