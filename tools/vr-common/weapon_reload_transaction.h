#pragma once
#include <algorithm>
#include <cstdint>
#include <limits>

namespace gears_weapon {
// Identity is the existing weapon-specific identity, not a second XR protocol.
// This pure ledger accepts only receipts authenticated by the game-thread owner.
// A populated value does not itself certify a native call or physical gesture.
enum class ReloadTxnMode : std::uint8_t { magazine, round };
enum class ReloadTxnStage : std::uint8_t {
    unbound, ready, open, holding, inventory_pending, closing,
    completion_pending, complete, interrupted, reconciliation_required
};
enum class ReloadTxnResult : std::uint8_t { none, inspection, ordinary, perfect };
constexpr ReloadTxnResult physical_reload_result(std::uint32_t target,std::uint32_t inserted,
    std::int64_t started,std::int64_t finished,std::int64_t perfect_end) noexcept {
    if(inserted>target||started<0||finished<started||perfect_end<0)return ReloadTxnResult::none;
    if(!inserted)return ReloadTxnResult::inspection;
    return inserted==target&&perfect_end>0&&finished-started<=perfect_end ?
        ReloadTxnResult::perfect:ReloadTxnResult::ordinary;
}
// Visual six-chamber bookkeeping. Native inventory provides a total, not six
// physical chamber bits. A fresh instance uses canonical slots; known same-
// instance state can be retained if its count matches the authoritative total.
class ReloadSixChambers {
    std::uint8_t occupied_{},next_shot_{};
public:
    constexpr unsigned count() const noexcept {
        unsigned n=0;for(unsigned i=0;i<6;++i)n+=(occupied_>>i)&1u;return n;
    }
    constexpr std::uint8_t mask() const noexcept {return occupied_;}
    constexpr bool bind(unsigned loaded) noexcept {
        if(loaded>6)return false;
        occupied_=static_cast<std::uint8_t>((1u<<loaded)-1u);next_shot_=0;return true;
    }
    constexpr bool vacant(unsigned slot) const noexcept {return slot<6&&!(occupied_&(1u<<slot));}
    constexpr bool occupy(unsigned slot) noexcept {
        if(!vacant(slot))return false;
        occupied_=static_cast<std::uint8_t>(occupied_|(1u<<slot));return true;
    }
    constexpr bool after_confirmed_shot() noexcept {
        for(unsigned i=0;i<6;++i){
            const unsigned slot=(next_shot_+i)%6;
            if(!vacant(slot)){
                occupied_=static_cast<std::uint8_t>(occupied_&~(1u<<slot));
                next_shot_=static_cast<std::uint8_t>((slot+1)%6);return true;
            }
        }
        return false;
    }
};
enum class ReloadTxnCall : std::uint8_t { none, insert, finish, exit_without_fill };
enum class ReloadIntentSource : std::uint8_t {
    magazine_pull, mechanism_open, take_ammunition, auxiliary_button
};
template<class Identity> struct ReloadTxnInventory {
    Identity identity{};
    std::uint64_t revision{};
    std::uint32_t capacity{}, loaded{}, reserve{};
    bool infinite_reserve{}; // Original HasInfiniteSpareAmmo, not a sentinel count.
    constexpr bool valid() const noexcept {
        return identity.valid() && revision && capacity && capacity<=10000 &&
            loaded<=capacity && reserve<=1000000;
    }
};
template<class Identity> struct ReloadTxnRequest {
    Identity identity{};
    std::uint64_t token{}, reload_serial{}, item_serial{};
    ReloadTxnCall call{ReloadTxnCall::none};
    ReloadTxnInventory<Identity> before{}, expected{};
    ReloadTxnResult result{ReloadTxnResult::none};
};
template<class Identity> struct ReloadTxnReceipt {
    std::uint64_t token{};
    bool accepted{};
    ReloadTxnInventory<Identity> observed{};
    ReloadTxnResult result_applied{ReloadTxnResult::none};
};
template<class Identity> class ReloadTransaction {
    using Inventory=ReloadTxnInventory<Identity>;
    using Request=ReloadTxnRequest<Identity>;
    Inventory inventory_{};
    Request pending_{};
    ReloadTxnStage stage_{ReloadTxnStage::unbound};
    ReloadTxnMode mode_{ReloadTxnMode::round};
    ReloadTxnResult result_{ReloadTxnResult::none};
    std::uint64_t next_token_{1}, last_intent_{}, last_item_{}, last_reload_{},
        reload_serial_{}, old_item_{}, item_{};
    std::uint32_t target_{}, inserted_{}, held_{};
    std::int64_t started_tick_{},closed_tick_{},closing_window_{};
    bool dispatched_{}, cancel_requested_{}, old_held_{}, feed_present_{true},
        closed_{true}, original_restored_{};

    constexpr bool same_counts(Inventory const& a,Inventory const& b) const noexcept {
        return a.identity==b.identity && a.capacity==b.capacity &&
            a.loaded==b.loaded && a.reserve==b.reserve && a.infinite_reserve==b.infinite_reserve;
    }
    constexpr bool active() const noexcept {
        return stage_!=ReloadTxnStage::unbound && stage_!=ReloadTxnStage::ready &&
            stage_!=ReloadTxnStage::complete;
    }
    constexpr bool queue(ReloadTxnCall call,ReloadTxnResult result) noexcept {
        if(pending_.token || next_token_==std::numeric_limits<std::uint64_t>::max())return false;
        pending_={inventory_.identity,next_token_++,reload_serial_,item_,call,
            inventory_,inventory_,result};
        // expected.revision is not a fabricated future native revision.
        pending_.expected.revision=0;dispatched_=false;return true;
    }
    constexpr void release_reservation() noexcept {held_=0;item_=0;}
public:
    // No reset/rebind API may erase an outstanding original call.
    constexpr bool bind(Inventory const& value) noexcept {
        if(stage_!=ReloadTxnStage::unbound || !value.valid())return false;
        inventory_=value;stage_=ReloadTxnStage::ready;return true;
    }
    constexpr bool sync_idle(Inventory const& value) noexcept {
        if((stage_!=ReloadTxnStage::ready && stage_!=ReloadTxnStage::complete) ||
           !value.valid() || !(value.identity==inventory_.identity) ||
           value.capacity!=inventory_.capacity || value.revision<inventory_.revision ||
           (value.revision==inventory_.revision && !same_counts(value,inventory_)))return false;
        inventory_=value;return true;
    }
    // serial is the accepted original reload, except inspection (q=0) where
    // ForceReload must NOT be invoked and serial must be zero. Intent is a fresh
    // physical/auxiliary action token, never a synthetic R3 edge.
    constexpr bool begin(Inventory const& inventory_observation,std::uint64_t intent,
        ReloadIntentSource source,std::uint64_t serial,ReloadTxnMode mode,
        std::uint32_t maximum_per_cycle,std::int64_t tick,std::uint64_t old_item=0) noexcept {
        if(active() || stage_==ReloadTxnStage::unbound || !intent || intent<=last_intent_ ||
           !inventory_observation.valid() || !same_counts(inventory_observation,inventory_) ||
           inventory_observation.revision<inventory_.revision || tick<0 || !maximum_per_cycle ||
           maximum_per_cycle>inventory_observation.capacity ||
           (mode!=ReloadTxnMode::magazine && mode!=ReloadTxnMode::round) ||
           (mode==ReloadTxnMode::magazine ? (!old_item || old_item<=last_item_) : old_item!=0))return false;
        if(source!=ReloadIntentSource::magazine_pull && source!=ReloadIntentSource::mechanism_open &&
           source!=ReloadIntentSource::take_ammunition && source!=ReloadIntentSource::auxiliary_button)return false;
        const auto deficit=inventory_observation.capacity-inventory_observation.loaded;
        const auto target=std::min(inventory_observation.infinite_reserve?deficit:std::min(deficit,inventory_observation.reserve),maximum_per_cycle);
        if(target ? (!serial || serial<=last_reload_) : serial!=0)return false;
        inventory_=inventory_observation;last_intent_=intent;reload_serial_=serial;
        if(serial)last_reload_=serial;
        mode_=mode;target_=target;inserted_=held_=0;started_tick_=tick;item_=0;
        closed_tick_=closing_window_=0;
        old_item_=old_item;old_held_=old_item!=0;
        if(old_item)last_item_=old_item;
        feed_present_=mode==ReloadTxnMode::round;closed_=false;
        original_restored_=cancel_requested_=false;result_=ReloadTxnResult::none;
        stage_=ReloadTxnStage::open;return true;
    }
    constexpr bool stow_original(std::uint64_t item) noexcept {
        if(stage_!=ReloadTxnStage::open || !old_held_ || item!=old_item_)return false;
        old_held_=false;return true; // The old remaining rounds never leave native L.
    }
    constexpr bool restore_original(std::uint64_t item) noexcept {
        if(stage_!=ReloadTxnStage::open || mode_!=ReloadTxnMode::magazine || inserted_ ||
           held_ || pending_.token || !item || item!=old_item_)return false;
        old_held_=false;feed_present_=true;original_restored_=true;
        stage_=ReloadTxnStage::closing;return true;
    }
    constexpr bool reserve_item(std::uint64_t item) noexcept {
        if(stage_!=ReloadTxnStage::open || old_held_ || held_ || pending_.token ||
           !item || item<=last_item_ || inserted_>=target_)return false;
        const auto quantity=mode_==ReloadTxnMode::magazine ? target_-inserted_ : 1u;
        if(!inventory_.infinite_reserve&&quantity>inventory_.reserve)return false;
        held_=quantity;item_=last_item_=item;stage_=ReloadTxnStage::holding;return true;
    }
    constexpr bool release_item(std::uint64_t item) noexcept {
        if(stage_!=ReloadTxnStage::holding || !item || item!=item_)return false;
        release_reservation();stage_=ReloadTxnStage::open;return true;
    }
    constexpr bool request_insertion(std::uint64_t item) noexcept {
        if(stage_!=ReloadTxnStage::holding || !item || item!=item_ || !held_ ||
           !queue(ReloadTxnCall::insert,ReloadTxnResult::none))return false;
        pending_.expected.loaded+=held_;
        if(!inventory_.infinite_reserve)pending_.expected.reserve-=held_;
        stage_=ReloadTxnStage::inventory_pending;return true;
    }
    constexpr bool take_request(Request& out) noexcept {
        if(!pending_.token || dispatched_ || stage_==ReloadTxnStage::reconciliation_required)return false;
        out=pending_;dispatched_=true;return true;
    }
    // Read-only recovery evidence; never permits dispatching the call again.
    constexpr bool dispatched_request(Request& out) const noexcept {
        if(!pending_.token||!dispatched_)return false;
        out=pending_;return true;
    }
    // A missing/ambiguous call outcome retains the request. No automatic retry.
    constexpr bool require_reconciliation() noexcept {
        if(!pending_.token || !dispatched_)return false;
        cancel_requested_=true;stage_=ReloadTxnStage::reconciliation_required;return true;
    }
    constexpr bool acknowledge(ReloadTxnReceipt<Identity> const& receipt) noexcept {
        if(!pending_.token || !dispatched_ || receipt.token!=pending_.token ||
           !receipt.observed.valid() || !(receipt.observed.identity==inventory_.identity))return false;
        const bool insertion=pending_.call==ReloadTxnCall::insert;
        const auto& expected=receipt.accepted ? pending_.expected : pending_.before;
        const auto expected_result=receipt.accepted && pending_.result!=ReloadTxnResult::inspection ?
            pending_.result : ReloadTxnResult::none;
        if(!same_counts(receipt.observed,expected) ||
           receipt.observed.revision<pending_.before.revision ||
           (receipt.accepted && insertion && receipt.observed.revision<=pending_.before.revision) ||
           receipt.result_applied!=expected_result){
            cancel_requested_=true;stage_=ReloadTxnStage::reconciliation_required;return false;
        }
        inventory_=receipt.observed;
        const auto call_result=pending_.result;
        pending_={};dispatched_=false;
        if(insertion){
            if(receipt.accepted){inserted_+=held_;if(mode_==ReloadTxnMode::magazine)feed_present_=true;}
            release_reservation();
            stage_=cancel_requested_ ? ReloadTxnStage::interrupted :
                (inserted_==target_ ? ReloadTxnStage::closing : ReloadTxnStage::open);
        }else{
            result_=receipt.accepted ? call_result : ReloadTxnResult::none;
            stage_=receipt.accepted && !cancel_requested_ ? ReloadTxnStage::complete : ReloadTxnStage::interrupted;
        }
        return true;
    }
    constexpr bool request_close() noexcept {
        if(stage_!=ReloadTxnStage::open || held_ || old_held_ || pending_.token || !feed_present_)return false;
        stage_=ReloadTxnStage::closing;return true; // target_ is intentionally preserved.
    }
    // Called after physical closure is authenticated. The original adapter must
    // use a non-refilling exit for inspection, and attest the reward actually applied.
    constexpr bool confirm_closed(std::int64_t tick,std::int64_t perfect_end_ticks=0) noexcept {
        if(stage_!=ReloadTxnStage::closing || pending_.token || !feed_present_ ||
           tick<started_tick_ || perfect_end_ticks<0)return false;
        // A definitively rejected native completion can be retried after
        // reconciliation, but never changes the actual physical closure time.
        if(closed_ && perfect_end_ticks!=closing_window_)return false;
        if(!closed_){closed_tick_=tick;closing_window_=perfect_end_ticks;}
        closed_=true;
        if(!reload_serial_){result_=ReloadTxnResult::inspection;stage_=ReloadTxnStage::complete;return true;}
        const bool inspection=original_restored_ || !inserted_;
        const auto result=inspection ? ReloadTxnResult::inspection :
            physical_reload_result(target_,inserted_,started_tick_,closed_tick_,closing_window_);
        if(!queue(inspection ? ReloadTxnCall::exit_without_fill : ReloadTxnCall::finish,result))return false;
        stage_=ReloadTxnStage::completion_pending;return true;
    }
    constexpr bool interrupt() noexcept {
        if(!active())return false;
        cancel_requested_=true; // Original magazine still needs explicit stow/restore after recovery.
        if(pending_.token && dispatched_)return true; // A late commit still belongs to us.
        pending_={};release_reservation();stage_=ReloadTxnStage::interrupted;return true;
    }
    // Resume only the SAME original reload ownership, after its native state and
    // inventory have been reconciled. A new world/binding cannot revive this ledger.
    constexpr bool resume(Inventory const& inventory_observation,std::uint64_t fresh_intent,
                          std::uint64_t same_reload_serial) noexcept {
        if(stage_!=ReloadTxnStage::interrupted || pending_.token || !inventory_observation.valid() ||
           !same_counts(inventory_observation,inventory_) || inventory_observation.revision<inventory_.revision ||
           !fresh_intent || fresh_intent<=last_intent_ || same_reload_serial!=reload_serial_)return false;
        inventory_=inventory_observation;last_intent_=fresh_intent;cancel_requested_=false;
        if(result_!=ReloadTxnResult::none && closed_){stage_=ReloadTxnStage::complete;return true;}
        stage_=(feed_present_ && (closed_ || original_restored_ || inserted_==target_ ||
            mode_==ReloadTxnMode::magazine)) ? ReloadTxnStage::closing : ReloadTxnStage::open;
        return true;
    }
    // Adapter-only rebind after a suspended SAME weapon is held again. The
    // adapter must authenticate object/lifecycle identity and the new hold;
    // this ledger proves no pending dispatch/reservation or inventory change
    // is hidden by that transition. Keep target, commits and timing intact.
    constexpr bool rebind_suspended(Identity const& expected_old,Inventory const& next) noexcept {
        if(stage_!=ReloadTxnStage::interrupted||pending_.token||held_||old_held_||
           !(inventory_.identity==expected_old)||!next.valid()||
           next.revision<inventory_.revision||next.capacity!=inventory_.capacity||
           next.loaded!=inventory_.loaded||next.reserve!=inventory_.reserve||
           next.infinite_reserve!=inventory_.infinite_reserve)return false;
        inventory_=next;return true;
    }
    // Private bookkeeping only for the SAME retained old magazine after an
    // input lifecycle boundary. The adapter proves the unchanged real item,
    // weapon/hold/session and counts, and STILL requires a tracked release /
    // new grasp before any insertion. This neither interrupts/stows the old
    // item nor grants an input edge, reservation, inventory write or closure.
    constexpr bool rebind_retained_original(Identity const& expected_old,
            Inventory const& next,std::uint64_t old_item) noexcept {
        if(stage_!=ReloadTxnStage::open||mode_!=ReloadTxnMode::magazine||
           pending_.token||held_||!old_held_||!old_item||old_item!=old_item_||
           feed_present_||inserted_||cancel_requested_||
           !(inventory_.identity==expected_old)||!next.valid()||
           next.revision<inventory_.revision||next.capacity!=inventory_.capacity||
           next.loaded!=inventory_.loaded||next.reserve!=inventory_.reserve||
           next.infinite_reserve!=inventory_.infinite_reserve)return false;
        inventory_=next;return true;
    }
    constexpr ReloadTxnStage stage() const noexcept {return stage_;}
    constexpr Inventory inventory() const noexcept {return inventory_;}
    constexpr ReloadTxnResult result() const noexcept {return result_;}
    constexpr std::uint32_t reserved() const noexcept {return held_;}
    // Finite stored reserve remains honest even under the original infinite
    // policy. Callers consult inventory().infinite_reserve for availability.
    constexpr std::uint32_t available_reserve() const noexcept {
        return inventory_.infinite_reserve?inventory_.reserve:inventory_.reserve-held_;
    }
    constexpr std::uint32_t target() const noexcept {return target_;}
    constexpr std::uint32_t inserted() const noexcept {return inserted_;}
    constexpr bool pending() const noexcept {return pending_.token!=0;}
    constexpr ReloadTxnCall pending_call() const noexcept {return pending_.call;}
    constexpr bool feed_present() const noexcept {return feed_present_;}
    constexpr bool mechanical_ready() const noexcept {
        return (stage_==ReloadTxnStage::ready || stage_==ReloadTxnStage::complete) &&
            feed_present_ && closed_ && !pending_.token;
    }
};
}
