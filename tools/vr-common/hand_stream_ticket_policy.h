#pragma once
#include <cmath>
#include <cstdint>
#include <limits>

// Ticket/permission bookkeeping only: no geometry, engine calls, writes or ABI.
namespace gears_hand_stream {
constexpr unsigned left_hand=1, right_hand=2, both_hands=3;
constexpr std::uint64_t maximum_age_ms=100;
struct GameKey {
    std::uint64_t world{},controller{},pawn{},mesh{},weapon{}; // weapon may be zero.
};
struct StreamKey {
    std::uint64_t session{},space_epoch{},input_epoch{},world_generation{};
};
struct Ticket {
    StreamKey stream{};
    std::uint64_t serial{},sequence{},created_ms{},controller{},pawn{},weapon{};
    double units_per_metre{};
    unsigned valid_mask{}; // Bit 0 left, bit 1 right; not skeletal ownership.
};
struct Context {
    GameKey game{};
    std::uint64_t expected_native_world_generation{},action_epoch{},observed_ms{};
    bool idle_ready{},clean{};
};
struct Plan {
    Context context{};Ticket ticket{};
    std::uint64_t revision{},observed_now{};
    unsigned accept_mask{},bind_mask{},reset_binding_mask{},restore_mask{},baseline_mask{};
    bool has_ticket{};
};
enum class ObserveReason : unsigned {
    context_identity,context_readiness,binding_age,not_ready,no_receipt,
    stream_changed,stream_invalid,ticket_shape,ticket_owner,ticket_world,
    not_newer,ticket_age,scale_changed,tracking_loss,revision_exhausted,count
};
constexpr unsigned observe_reason_count=static_cast<unsigned>(ObserveReason::count);
struct ObserveAge {
    std::uint64_t ms{}; // Absolute distance; future means the stamp is ahead of now.
    bool known{},future{};
};
struct ObserveDiagnostic {
    std::uint64_t observed_now{};
    ObserveAge old_ticket_age[2]{},context_age{},previous_context_age{},new_ticket_age{};
    unsigned bound_before{},bound_after{};
    // Masks describe branches actually reached, including lease-only yields
    // (no_receipt/not_newer). Several reasons can describe the same hand.
    unsigned reason_masks[observe_reason_count]{};
    unsigned mask(ObserveReason reason) const noexcept {
        const auto index=static_cast<unsigned>(reason);
        return index<observe_reason_count?reason_masks[index]:0u;
    }
};
inline ObserveAge observe_age(std::uint64_t stamp,std::uint64_t now,bool known=true) noexcept {
    return known?ObserveAge{stamp>now?stamp-now:now-stamp,true,stamp>now}:ObserveAge{};
}
inline bool same(GameKey const& a,GameKey const& b) noexcept {
    return a.world==b.world&&a.controller==b.controller&&a.pawn==b.pawn&&
        a.mesh==b.mesh&&a.weapon==b.weapon;
}
inline bool same(StreamKey const& a,StreamKey const& b) noexcept {
    return a.session==b.session&&a.space_epoch==b.space_epoch&&
        a.input_epoch==b.input_epoch&&a.world_generation==b.world_generation;
}
inline bool recent(std::uint64_t stamp,std::uint64_t now) noexcept {
    return stamp<=now&&now-stamp<=maximum_age_ms;
}

class Policy {
    Context context_{};StreamKey stream_{};Plan current_{};
    ObserveDiagnostic diagnostic_{};
    std::uint64_t revision_{},serial_{},sequence_{},last_present_ms_[2]{};
    double units_{};
    unsigned bound_{},baseline_pending_{both_hands},last_valid_{};
    bool context_known_{},stream_known_{},ready_{};
    static bool game_valid(GameKey const& game) noexcept {
        return game.world&&game.controller&&game.pawn&&game.mesh;
    }
    static bool stream_valid(StreamKey const& stream) noexcept {
        return stream.session&&stream.space_epoch&&stream.input_epoch&&stream.world_generation;
    }
    static bool identity(Context const& a,Context const& b) noexcept {
        return same(a.game,b.game)&&
            a.expected_native_world_generation==b.expected_native_world_generation&&
            a.action_epoch==b.action_epoch;
    }
    static bool context_same(Context const& a,Context const& b) noexcept {
        return identity(a,b)&&a.observed_ms==b.observed_ms&&
            a.idle_ready==b.idle_ready&&a.clean==b.clean;
    }
    static bool ticket_same(Ticket const& a,Ticket const& b) noexcept {
        return same(a.stream,b.stream)&&a.serial==b.serial&&a.sequence==b.sequence&&
            a.created_ms==b.created_ms&&a.controller==b.controller&&a.pawn==b.pawn&&
            a.weapon==b.weapon&&a.units_per_metre==b.units_per_metre&&a.valid_mask==b.valid_mask;
    }
    bool current(Plan const& plan) const noexcept {
        return plan.revision&&plan.revision==current_.revision&&
            plan.observed_now==current_.observed_now&&plan.has_ticket==current_.has_ticket&&
            context_same(plan.context,current_.context)&&ticket_same(plan.ticket,current_.ticket)&&
            plan.accept_mask==current_.accept_mask&&plan.bind_mask==current_.bind_mask&&
            plan.reset_binding_mask==current_.reset_binding_mask&&
            plan.restore_mask==current_.restore_mask&&plan.baseline_mask==current_.baseline_mask;
    }
    void invalidate(Plan& plan,unsigned mask) noexcept {
        bound_&=~mask;baseline_pending_|=mask;
        plan.reset_binding_mask|=mask;plan.restore_mask|=mask;
    }
    void diagnose(ObserveReason reason,unsigned mask=both_hands) noexcept {
        diagnostic_.reason_masks[static_cast<unsigned>(reason)]|=mask;
    }
    Plan seal(Plan plan) noexcept {
        // No accepted new pose means yield the corresponding active lease,
        // while an otherwise recent geometry binding may remain available.
        plan.restore_mask|=both_hands&~plan.accept_mask;
        diagnostic_.bound_after=bound_;
        current_=plan;return plan;
    }
public:
    unsigned bound_mask() const noexcept {return bound_;}
    // The last observe only. Ages are sampled before any invalidation or
    // renewal; old_ticket_age is known only for hands bound on entry. The
    // bound_after value precedes any later commit_binding. No permission state
    // is read back from this diagnostic, and authorize never changes it.
    ObserveDiagnostic const& last_observe() const noexcept {return diagnostic_;}
    // Pass only this frame's successful source read. nullptr never reuses the
    // stored ticket. Every observation also revokes all earlier plans.
    Plan observe(Context const& context,Ticket const* fresh,std::uint64_t now) noexcept {
        diagnostic_={};diagnostic_.observed_now=now;diagnostic_.bound_before=bound_;
        diagnostic_.context_age=observe_age(context.observed_ms,now);
        diagnostic_.previous_context_age=observe_age(context_.observed_ms,now,context_known_);
        diagnostic_.new_ticket_age=observe_age(fresh?fresh->created_ms:0,now,fresh!=nullptr);
        for(unsigned hand=0;hand<2;++hand)
            diagnostic_.old_ticket_age[hand]=observe_age(last_present_ms_[hand],now,(bound_&(1u<<hand))!=0);
        Plan plan{};plan.context=context;plan.observed_now=now;
        plan.has_ticket=fresh!=nullptr;if(fresh)plan.ticket=*fresh;
        if(revision_==(std::numeric_limits<std::uint64_t>::max)()){
            diagnose(ObserveReason::revision_exhausted);
            invalidate(plan,both_hands);return seal(plan); // Never wrap a plan identity.
        }
        plan.revision=++revision_;
        const bool ready=game_valid(context.game)&&recent(context.observed_ms,now)&&
            context.idle_ready&&context.clean;
        const bool context_changed=!context_known_||!identity(context_,context);
        if(context_changed||ready!=ready_){
            if(context_changed)diagnose(ObserveReason::context_identity);
            if(ready!=ready_)diagnose(ObserveReason::context_readiness);
            invalidate(plan,both_hands);
        }
        context_=context;context_known_=true;ready_=ready;
        // Check the old binding BEFORE a new ticket can renew its age. A long
        // gap requires a new baseline and a later receipt, even on immediate read success.
        for(unsigned hand=0;hand<2;++hand)
            if((bound_&(1u<<hand))&&!recent(last_present_ms_[hand],now)){
                diagnose(ObserveReason::binding_age,1u<<hand);invalidate(plan,1u<<hand);
            }
        if(!ready){diagnose(ObserveReason::not_ready);invalidate(plan,both_hands);}
        if(!fresh){diagnose(ObserveReason::no_receipt);return seal(plan);}
        const auto& ticket=*fresh;
        const bool owner=game_valid(context.game)&&ticket.controller==context.game.controller&&
            ticket.pawn==context.game.pawn&&ticket.weapon==context.game.weapon;
        const bool scale=std::isfinite(ticket.units_per_metre)&&ticket.units_per_metre>0.;
        const bool shape=stream_valid(ticket.stream)&&ticket.serial&&ticket.sequence&&
            !(ticket.valid_mask&~both_hands)&&scale;
        const bool world=ticket.stream.world_generation==context.expected_native_world_generation;
        const bool changed=!stream_known_||!same(stream_,ticket.stream);
        if(changed){
            diagnose(ObserveReason::stream_changed);
            if(!stream_valid(ticket.stream)){
                diagnose(ObserveReason::stream_invalid);invalidate(plan,both_hands);return seal(plan);
            }
            stream_=ticket.stream;stream_known_=true;serial_=sequence_=0;last_valid_=0;
            units_=scale?ticket.units_per_metre:0.;invalidate(plan,both_hands);
        }
        // A rejected read still consumes its same-stream cursors. In particular,
        // a future receipt cannot become writable when the clock catches up.
        const bool newer=ticket.serial>serial_&&ticket.sequence>sequence_;
        if(ticket.serial>serial_)serial_=ticket.serial;
        if(ticket.sequence>sequence_)sequence_=ticket.sequence;
        if(!shape||!owner||!world){
            if(!shape)diagnose(ObserveReason::ticket_shape);
            if(!owner)diagnose(ObserveReason::ticket_owner);
            if(!world)diagnose(ObserveReason::ticket_world);
            invalidate(plan,both_hands);return seal(plan);
        }
        if(!newer){diagnose(ObserveReason::not_newer);return seal(plan);}
        if(!recent(ticket.created_ms,now)){
            diagnose(ObserveReason::ticket_age);invalidate(plan,both_hands);return seal(plan);
        }
        if(units_!=ticket.units_per_metre){
            diagnose(ObserveReason::scale_changed);units_=ticket.units_per_metre;invalidate(plan,both_hands);
        }
        const auto lost=ticket.valid_mask?(last_valid_&~ticket.valid_mask):both_hands;
        diagnose(ObserveReason::tracking_loss,lost);
        invalidate(plan,lost);last_valid_=ticket.valid_mask;
        if(!ready)return seal(plan);
        // A loss receipt seals the missing hand too. Its return needs a newer
        // receipt; it does not force the still-tracked hand through a baseline.
        plan.baseline_mask=baseline_pending_;baseline_pending_=0;
        plan.accept_mask=ticket.valid_mask&~plan.baseline_mask;
        plan.bind_mask=plan.accept_mask&~bound_;
        for(unsigned hand=0;hand<2;++hand)
            if(ticket.valid_mask&(1u<<hand))last_present_ms_[hand]=ticket.created_ms;
        return seal(plan);
    }
    // Commit only after the separate geometry binder actually succeeded for
    // these hands. Binding a baseline, old plan, already-bound hand or a hand
    // omitted from bind_mask is rejected as a whole.
    bool commit_binding(Plan const& plan,unsigned mask) noexcept {
        if(!current(plan)||!mask||(mask&~plan.bind_mask)||(mask&bound_))return false;
        bound_|=mask;return true;
    }
    // This permission is for a write immediately following the caller's fresh
    // context check. It is not animation ownership or a dirty-journal assertion.
    // restore_mask means "yield these hands"; the driver inspects its journals.
    unsigned authorize(Plan const& plan,Context const& current_context,std::uint64_t now) const noexcept {
        if(!current(plan)||!plan.has_ticket||!ready_||!plan.accept_mask||
           now<plan.observed_now||!identity(plan.context,current_context)||
           !current_context.idle_ready||!current_context.clean||
           current_context.observed_ms<plan.context.observed_ms||
           !recent(plan.context.observed_ms,now)||!recent(current_context.observed_ms,now)||
           !recent(plan.ticket.created_ms,now))return 0;
        return plan.accept_mask&bound_;
    }
};
} // namespace gears_hand_stream
