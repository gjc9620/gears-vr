#pragma once
#include "weapon_boltok_reload.h"

namespace gears_weapon {
// Local mechanical display mapping, NOT six chambers read from native ammo.
// The authoritative total remains the original game's inventory. This ledger
// changes a hole only after a matching, once-applied original inventory receipt.
// It grants no hand ownership, contact, opening motion, or firing permission.
class BoltokCylinder {
    BoltokInventory inventory_{};
    BoltokInventoryRequest pending_{};
    std::uint64_t last_token_{},last_event_{};
    unsigned mask_{},slot_{6};
    bool bound_{},open_{},reconcile_{};
    static unsigned count(unsigned mask) noexcept {
        unsigned result=0;for(;mask;mask>>=1)result+=mask&1u;return result;
    }
    bool same(BoltokInventory const& v) const noexcept {
        return v.valid()&&v.identity==inventory_.identity&&v.capacity==6&&
            v.ammunition_type==inventory_.ammunition_type;
    }
public:
    bool bind(BoltokInventory const& v) noexcept {
        if(!v.valid()||v.capacity!=6||(bound_&&!reconcile_)||
           (bound_&&v.identity.binding_generation<=inventory_.identity.binding_generation))return false;
        inventory_=v;mask_=(1u<<v.loaded)-1u;pending_={};slot_=6;
        bound_=true;open_=reconcile_=false;last_token_=last_event_=0;return true;
    }
    // Called only AFTER the shared mechanical protocol accepts actual travel.
    bool opened() noexcept {
        if(!bound_||reconcile_||open_||pending_.token)return false;
        open_=true;return true;
    }
    bool closed() noexcept {
        if(!bound_||reconcile_||!open_||pending_.token)return false;
        open_=false;return true;
    }
    bool prepare(unsigned slot,BoltokInventoryRequest const& r) noexcept {
        if(!bound_||reconcile_||!open_||pending_.token||slot>=6||(mask_&(1u<<slot))||
           !(r.identity==inventory_.identity)||!r.token||r.token<=last_token_||
           !r.item_serial||!r.original_reload_serial||!r.plan_generation||
           r.before_revision!=inventory_.revision||r.before_loaded!=inventory_.loaded||
           r.before_reserve!=inventory_.reserve||!r.before_reserve||r.before_loaded>=6||
           r.after_loaded!=r.before_loaded+1||r.after_reserve!=r.before_reserve-1)return false;
        pending_=r;slot_=slot;last_token_=r.token;return true;
    }
    bool acknowledge(BoltokOriginalReceipt const& r) noexcept {
        if(!pending_.token||reconcile_||r.token!=pending_.token||
           r.original_reload_serial!=pending_.original_reload_serial||
           r.plan_generation!=pending_.plan_generation||!r.original_event_sequence||
           r.original_event_sequence<=last_event_)return false;
        if(!same(r.observed)||r.observed.revision<inventory_.revision){reconcile_=true;return false;}
        const bool applied=r.accepted&&r.original_effect_applied_once&&
            r.observed.revision>inventory_.revision&&r.observed.loaded==pending_.after_loaded&&
            r.observed.reserve==pending_.after_reserve;
        const bool denied=!r.accepted&&!r.original_effect_applied_once&&
            r.observed.loaded==inventory_.loaded&&r.observed.reserve==inventory_.reserve;
        if(!applied&&!denied){reconcile_=true;return false;}
        if(applied)mask_|=1u<<slot_;
        inventory_=r.observed;last_event_=r.original_event_sequence;pending_={};slot_=6;
        return applied;
    }
    // Apply only a shot already authenticated by the original-fire protocol.
    // Removal order is this local display's indexing convention, not native data.
    bool shot(BoltokInventory const& v,unsigned cost) noexcept {
        if(!bound_||reconcile_||open_||pending_.token||!same(v)||!cost||
           cost>inventory_.loaded||v.revision<=inventory_.revision||
           v.loaded!=inventory_.loaded-cost||v.reserve!=inventory_.reserve)return false;
        for(unsigned i=0;i<6&&cost;++i)if(mask_&(1u<<i)){mask_&=~(1u<<i);--cost;}
        inventory_=v;return count(mask_)==v.loaded;
    }
    void interrupt() noexcept {if(bound_)reconcile_=true;}
    unsigned live_mask() const noexcept {return mask_;}
    bool pending() const noexcept {return pending_.token!=0;}
    bool needs_reconciliation() const noexcept {return reconcile_;}
};
}
