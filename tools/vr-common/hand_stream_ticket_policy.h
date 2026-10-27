#pragma once
#include "hand_continuity_certificate.h"
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
    gears_hand_continuity::Certificate certificate{};
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
constexpr unsigned transport_stream_capacity=8;
struct TransportResult {
    unsigned refreshed_mask{},reset_binding_mask{};
    // count means no rejection/revocation reason. Mixed results keep the first
    // reason, including old binding age before a remaining hand is refreshed.
    ObserveReason reason{ObserveReason::count};
    bool capacity_exhausted{}; // Permanent fence failure; reason is not_newer.
};
struct ObserveAge {
    std::uint64_t ms{}; // Absolute distance; future means the stamp is ahead of now.
    bool known{},future{};
};
struct ContinuitySnapshot {
    // Historical observe state only, never a fresh context or write permission.
    Context context{};StreamKey stream{};
    std::uint64_t serial{},sequence{}; // Already-consumed stream cursors.
    double units_per_metre{};
    unsigned bound_mask{};
    bool context_known{},stream_known{},ready{}; // Historical ready_, not recomputed.
    std::uint64_t source_stamp[2]{}; // Known iff the corresponding old_ticket_age is known.
    ObserveAge old_ticket_age[2]{},context_age{};
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
    struct TransportFence {
        StreamKey stream{};
        std::uint64_t serial{},sequence{},writer_serial{},writer_sequence{};
        bool known{};
    };
    Context context_{};StreamKey stream_{};Plan current_{};
    ObserveDiagnostic diagnostic_{};
    std::uint64_t revision_{},serial_{},sequence_{},last_present_ms_[2]{};
    double units_{};
    unsigned bound_{},baseline_pending_{both_hands},last_valid_{};
    bool context_known_{},stream_known_{},ready_{};
    TransportFence transport_fences_[transport_stream_capacity]{};
    Ticket transport_writer_ticket_{};
    bool transport_enabled_{},transport_exhausted_{},transport_writer_known_{};
    gears_hand_continuity::Certificate certified_{};
    std::uint64_t expected_native_qpc_{};
    std::int64_t certified_observed_ns_{};
    std::uint32_t expected_source_{};
    bool used_{},certified_enabled_{},certified_known_{},certified_failed_{};
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
            a.weapon==b.weapon&&a.units_per_metre==b.units_per_metre&&a.valid_mask==b.valid_mask&&
            gears_hand_continuity::same_certificate(a.certificate,b.certificate);
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
    void invalidate(TransportResult& result,unsigned mask) noexcept {
        bound_&=~mask;baseline_pending_|=mask;result.reset_binding_mask|=mask;
    }
    static void reason(TransportResult& result,ObserveReason value) noexcept {
        if(result.reason==ObserveReason::count)result.reason=value;
    }
    TransportFence* transport_fence(StreamKey const& stream) noexcept {
        for(auto& fence:transport_fences_)
            if(fence.known&&same(fence.stream,stream))return &fence;
        if(!transport_exhausted_)
            for(auto& fence:transport_fences_)
                if(!fence.known){fence.known=true;fence.stream=stream;return &fence;}
        transport_exhausted_=true;transport_writer_known_=false;return nullptr;
    }
    static void consume_writer(TransportFence& fence,std::uint64_t serial,std::uint64_t sequence) noexcept {
        if(serial>fence.writer_serial)fence.writer_serial=serial;
        if(sequence>fence.writer_sequence)fence.writer_sequence=sequence;
    }
    bool stamp_regresses(Ticket const& ticket) const noexcept {
        for(unsigned hand=0;hand<2;++hand)
            if((bound_&ticket.valid_mask&(1u<<hand))&&ticket.created_ms<last_present_ms_[hand])return true;
        return false;
    }
    void diagnose(ObserveReason reason,unsigned mask=both_hands) noexcept {
        diagnostic_.reason_masks[static_cast<unsigned>(reason)]|=mask;
    }
    void certified_diagnostic(Context const& context,Ticket const* fresh,std::uint64_t now) noexcept {
        diagnostic_={};diagnostic_.observed_now=now;diagnostic_.bound_before=bound_;
        diagnostic_.context_age=observe_age(context.observed_ms,now);
        diagnostic_.previous_context_age=observe_age(context_.observed_ms,now,context_known_);
        diagnostic_.new_ticket_age=observe_age(fresh?fresh->created_ms:0,now,fresh!=nullptr);
        for(unsigned hand=0;hand<2;++hand)
            diagnostic_.old_ticket_age[hand]=observe_age(last_present_ms_[hand],now,(bound_&(1u<<hand))!=0);
    }
    Plan blocked_observation(Context const& context,Ticket const* fresh,std::uint64_t now) noexcept {
        current_.revision=0;certified_diagnostic(context,fresh,now);
        Plan plan{};plan.context=context;plan.observed_now=now;
        diagnose(ObserveReason::not_ready);invalidate(plan,both_hands);return seal(plan);
    }
    Plan reject_certified(Plan plan,ObserveReason why) noexcept {
        // One bounded, permanent fence covers even malformed/future attempts;
        // no rejected incarnation can return through another stream or clock.
        certified_failed_=true;diagnose(why);invalidate(plan,both_hands);return seal(plan);
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
    bool certified_failed() const noexcept {return certified_failed_;}
    // Opt in once, before either observation path or transport has been used.
    // A read-only snapshot does not consume this initial state.
    bool enable_certified_calibration(std::uint64_t expected_native_qpc,std::uint32_t expected_source) noexcept {
        if(used_||certified_enabled_||!expected_native_qpc||expected_source>1)return false;
        expected_native_qpc_=expected_native_qpc;expected_source_=expected_source;
        certified_enabled_=true;return true;
    }
    // The last observe only. Ages are sampled before any invalidation or
    // renewal; old_ticket_age is known only for hands bound on entry. The
    // bound_after value precedes any later commit_binding. No permission state
    // is read back from this diagnostic, and authorize never changes it.
    ObserveDiagnostic const& last_observe() const noexcept {return diagnostic_;}
    // Sample ages against now without observing a receipt, renewing timestamps,
    // consuming cursors, invalidating plans or changing the historical state.
    ContinuitySnapshot continuity_snapshot(std::uint64_t now) const noexcept {
        ContinuitySnapshot snapshot{};
        snapshot.context=context_;snapshot.stream=stream_;
        snapshot.serial=serial_;snapshot.sequence=sequence_;
        snapshot.units_per_metre=units_;snapshot.bound_mask=bound_;
        snapshot.context_known=context_known_;snapshot.stream_known=stream_known_;snapshot.ready=ready_;
        for(unsigned hand=0;hand<2;++hand){
            if(bound_&(1u<<hand))snapshot.source_stamp[hand]=last_present_ms_[hand];
            snapshot.old_ticket_age[hand]=observe_age(last_present_ms_[hand],now,(bound_&(1u<<hand))!=0);
        }
        snapshot.context_age=observe_age(context_.observed_ms,now,context_known_);
        return snapshot;
    }
    // Continuity only: never returns a Plan or grants a write/binding. The
    // caller still restores its journals and makes a regular fresh observation.
    TransportResult refresh_bound_transport(Ticket const& ticket,std::uint64_t action_epoch,std::uint64_t now) noexcept {
        used_=true;
        current_.revision=0; // Incrementing revision_ alone would leave current_ usable.
        TransportResult result{};
        if(certified_enabled_){
            reason(result,ObserveReason::not_ready);invalidate(result,both_hands);return result;
        }
        for(unsigned hand=0;hand<2;++hand)
            if((bound_&(1u<<hand))&&!recent(last_present_ms_[hand],now)){
                reason(result,ObserveReason::binding_age);invalidate(result,1u<<hand);
            }
        if(!transport_enabled_){
            transport_enabled_=true;
            if(stream_known_){
                auto* previous=transport_fence(stream_);
                if(previous)consume_writer(*previous,serial_,sequence_);
            }
        }
        auto* fence=transport_exhausted_?nullptr:transport_fence(ticket.stream);
        if(!fence){
            result.capacity_exhausted=true;reason(result,ObserveReason::not_newer);
            invalidate(result,both_hands);return result;
        }
        const bool newer=ticket.serial>fence->serial&&ticket.sequence>fence->sequence;
        if(ticket.serial>fence->serial)fence->serial=ticket.serial;
        if(ticket.sequence>fence->sequence)fence->sequence=ticket.sequence;
        // Do not let a duplicate clear a prior rejection or replace the one
        // legitimate same-ticket writer exception. A later attempted cursor
        // supersedes that exception even when the attempt is rejected.
        if(newer||!ticket_same(ticket,transport_writer_ticket_))transport_writer_known_=false;
        const auto reject=[&](ObserveReason why) noexcept {
            reason(result,why);transport_writer_known_=false;invalidate(result,both_hands);
        };
        if(revision_==(std::numeric_limits<std::uint64_t>::max)()){
            reject(ObserveReason::revision_exhausted);return result;
        }
        if(!context_known_||!game_valid(context_.game)||action_epoch!=context_.action_epoch){
            reject(ObserveReason::context_identity);return result;
        }
        if(!ready_||!context_.idle_ready||!context_.clean){reject(ObserveReason::not_ready);return result;}
        if(!stream_valid(ticket.stream)){reject(ObserveReason::stream_invalid);return result;}
        if(!stream_known_||!same(stream_,ticket.stream)){reject(ObserveReason::stream_changed);return result;}
        const bool scale=std::isfinite(ticket.units_per_metre)&&ticket.units_per_metre>0.;
        if(!ticket.serial||!ticket.sequence||(ticket.valid_mask&~both_hands)||!scale){
            reject(ObserveReason::ticket_shape);return result;
        }
        if(ticket.controller!=context_.game.controller||ticket.pawn!=context_.game.pawn||ticket.weapon!=context_.game.weapon){
            reject(ObserveReason::ticket_owner);return result;
        }
        if(ticket.stream.world_generation!=context_.expected_native_world_generation){
            reject(ObserveReason::ticket_world);return result;
        }
        if(units_!=ticket.units_per_metre){reject(ObserveReason::scale_changed);return result;}
        if(!newer||ticket.serial<=fence->writer_serial||ticket.sequence<=fence->writer_sequence){
            reason(result,ObserveReason::not_newer);return result;
        }
        if(!recent(ticket.created_ms,now)||stamp_regresses(ticket)){reject(ObserveReason::ticket_age);return result;}
        const auto lost=both_hands&~ticket.valid_mask;
        if(lost){reason(result,ObserveReason::tracking_loss);invalidate(result,lost);}
        result.refreshed_mask=bound_&ticket.valid_mask;
        for(unsigned hand=0;hand<2;++hand)
            if(result.refreshed_mask&(1u<<hand))last_present_ms_[hand]=ticket.created_ms;
        transport_writer_ticket_=ticket;transport_writer_known_=true;
        return result;
    }
    // Pass only this frame's successful source read. nullptr never reuses the
    // stored ticket. Every observation also revokes all earlier plans.
    Plan observe(Context const& context,Ticket const* fresh,std::uint64_t now) noexcept {
        used_=true;
        if(certified_enabled_)return blocked_observation(context,fresh,now);
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
            transport_writer_known_=false;
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
        if(transport_exhausted_){diagnose(ObserveReason::not_newer);invalidate(plan,both_hands);return seal(plan);}
        if(!fresh){diagnose(ObserveReason::no_receipt);return seal(plan);}
        const auto& ticket=*fresh;
        auto* fence=transport_enabled_?transport_fence(ticket.stream):nullptr;
        if(transport_exhausted_){diagnose(ObserveReason::not_newer);invalidate(plan,both_hands);return seal(plan);}
        const bool owner=game_valid(context.game)&&ticket.controller==context.game.controller&&
            ticket.pawn==context.game.pawn&&ticket.weapon==context.game.weapon;
        const bool scale=std::isfinite(ticket.units_per_metre)&&ticket.units_per_metre>0.;
        const bool shape=stream_valid(ticket.stream)&&ticket.serial&&ticket.sequence&&
            !(ticket.valid_mask&~both_hands)&&scale;
        const bool world=ticket.stream.world_generation==context.expected_native_world_generation;
        const bool changed=!stream_known_||!same(stream_,ticket.stream);
        if(changed){
            transport_writer_known_=false;
            diagnose(ObserveReason::stream_changed);
            if(!stream_valid(ticket.stream)){
                diagnose(ObserveReason::stream_invalid);invalidate(plan,both_hands);return seal(plan);
            }
            stream_=ticket.stream;stream_known_=true;serial_=sequence_=0;last_valid_=0;
            units_=scale?ticket.units_per_metre:0.;invalidate(plan,both_hands);
        }
        // A rejected read still consumes its same-stream cursors. In particular,
        // a future receipt cannot become writable when the clock catches up.
        const bool newer=ticket.serial>serial_&&ticket.sequence>sequence_&&
            (!fence||(ticket.serial>fence->writer_serial&&ticket.sequence>fence->writer_sequence));
        if(ticket.serial>serial_)serial_=ticket.serial;
        if(ticket.sequence>sequence_)sequence_=ticket.sequence;
        if(fence)consume_writer(*fence,ticket.serial,ticket.sequence);
        if(!shape||!owner||!world){
            transport_writer_known_=false;
            if(!shape)diagnose(ObserveReason::ticket_shape);
            if(!owner)diagnose(ObserveReason::ticket_owner);
            if(!world)diagnose(ObserveReason::ticket_world);
            invalidate(plan,both_hands);return seal(plan);
        }
        if(!newer){diagnose(ObserveReason::not_newer);return seal(plan);}
        const bool same_transport=transport_writer_known_&&ticket_same(ticket,transport_writer_ticket_);
        if(fence&&!(ticket.serial>fence->serial&&ticket.sequence>fence->sequence)&&!same_transport){
            diagnose(ObserveReason::not_newer);return seal(plan);
        }
        transport_writer_known_=false; // A legal transport ticket can enter the writer path only once.
        if(!recent(ticket.created_ms,now)||(transport_enabled_&&stamp_regresses(ticket))){
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
    // Certified continuity may retain geometry through historical age alone.
    // Only a newly authenticated receipt may grant this observation a write.
    Plan observe_certified(Context const& context,Ticket const* fresh,std::uint64_t now_ms,std::int64_t now_ns) noexcept {
        used_=true;
        if(!certified_enabled_)return blocked_observation(context,fresh,now_ms);
        current_.revision=0;certified_diagnostic(context,fresh,now_ms);certified_observed_ns_=now_ns;
        Plan plan{};plan.context=context;plan.observed_now=now_ms;
        plan.has_ticket=fresh!=nullptr;if(fresh)plan.ticket=*fresh;
        if(revision_==(std::numeric_limits<std::uint64_t>::max)())
            return reject_certified(plan,ObserveReason::revision_exhausted);
        plan.revision=++revision_;
        const bool changed=!context_known_||!identity(context_,context);
        const bool flags_changed=context_known_&&(context_.idle_ready!=context.idle_ready||context_.clean!=context.clean);
        const bool clock_bad=context.observed_ms>now_ms||(context_known_&&context_.observed_ms>now_ms)||now_ns<=0;
        const bool structural_ready=game_valid(context.game)&&context.idle_ready&&context.clean;
        if(changed){diagnose(ObserveReason::context_identity);invalidate(plan,both_hands);}
        if(flags_changed||clock_bad){diagnose(ObserveReason::context_readiness);invalidate(plan,both_hands);}
        if(!structural_ready){diagnose(ObserveReason::not_ready);invalidate(plan,both_hands);}
        ready_=structural_ready&&!clock_bad&&recent(context.observed_ms,now_ms);
        context_=context;context_known_=true;
        for(unsigned hand=0;hand<2;++hand)if(bound_&(1u<<hand)){
            if(!recent(last_present_ms_[hand],now_ms))diagnose(ObserveReason::binding_age,1u<<hand);
            if(last_present_ms_[hand]>now_ms)invalidate(plan,1u<<hand);
        }
        if(!ready_)diagnose(ObserveReason::not_ready);
        if(certified_failed_)return reject_certified(plan,ObserveReason::not_newer);
        if(!fresh){diagnose(ObserveReason::no_receipt);return seal(plan);}
        auto const& ticket=*fresh;auto const& certificate=ticket.certificate;
        const bool newer_serial=ticket.serial>serial_;
        if(ticket.serial>serial_)serial_=ticket.serial; // Global lane cursor; never reset on a stream change.
        const bool scale=std::isfinite(ticket.units_per_metre)&&ticket.units_per_metre>0.;
        if(!stream_valid(ticket.stream)||!ticket.serial||!ticket.sequence||
           (ticket.valid_mask&~both_hands)||!scale||!gears_hand_continuity::certificate_shape_valid(certificate))
            return reject_certified(plan,ObserveReason::ticket_shape);
        if(!game_valid(context.game)||ticket.controller!=context.game.controller||ticket.pawn!=context.game.pawn||
           ticket.weapon!=context.game.weapon||certificate.native_started_qpc!=expected_native_qpc_||
           (certified_known_&&(certificate.xr_publisher_pid!=certified_.xr_publisher_pid||
                               certificate.xr_publisher_start!=certified_.xr_publisher_start)))
            return reject_certified(plan,ObserveReason::ticket_owner);
        if(ticket.stream.world_generation!=context.expected_native_world_generation)
            return reject_certified(plan,ObserveReason::ticket_world);
        if(certificate.source!=expected_source_)return reject_certified(plan,ObserveReason::stream_invalid);
        const bool stream_advanced=certified_known_&&certificate.stream_epoch>certified_.stream_epoch;
        const bool reference_advanced=certified_known_&&certificate.reference_epoch>certified_.reference_epoch;
        const bool raw_restart=stream_advanced&&ticket.stream.session!=stream_.session;
        if(!newer_serial||(certified_known_&&!raw_restart&&ticket.sequence<=sequence_))
            return reject_certified(plan,ObserveReason::not_newer);
        if(certified_known_&&(certificate.stream_epoch<certified_.stream_epoch||
           certificate.reference_epoch<certified_.reference_epoch||
           certificate.tracking_epoch[0]<certified_.tracking_epoch[0]||
           certificate.tracking_epoch[1]<certified_.tracking_epoch[1]||
           (!reference_advanced&&certificate.reference_generation!=certified_.reference_generation)||
           certificate.stereo_received_ns<certified_.stereo_received_ns||
           certificate.input_received_ns<certified_.input_received_ns))
            return reject_certified(plan,ObserveReason::not_newer);
        if(!recent(ticket.created_ms,now_ms)||stamp_regresses(ticket)||
           !gears_hand_continuity::certificate_recent(certificate,now_ns))
            return reject_certified(plan,ObserveReason::ticket_age);
        const bool global=!certified_known_||!same(stream_,ticket.stream)||stream_advanced||reference_advanced||
            certificate.source!=certified_.source||certificate.profile_generation!=certified_.profile_generation;
        if(global){diagnose(ObserveReason::stream_changed);invalidate(plan,both_hands);}
        if(certified_known_&&units_!=ticket.units_per_metre){
            diagnose(ObserveReason::scale_changed);invalidate(plan,both_hands);
        }
        if(certified_known_)for(unsigned hand=0;hand<2;++hand)
            if(certificate.tracking_epoch[hand]>certified_.tracking_epoch[hand]){
                diagnose(ObserveReason::tracking_loss,1u<<hand);invalidate(plan,1u<<hand);
            }
        const auto lost=ticket.valid_mask?(last_valid_&~ticket.valid_mask):both_hands;
        diagnose(ObserveReason::tracking_loss,lost);invalidate(plan,lost);
        certified_=certificate;certified_known_=true;stream_=ticket.stream;stream_known_=true;
        sequence_=ticket.sequence;units_=ticket.units_per_metre;last_valid_=ticket.valid_mask;
        if(!ready_)return seal(plan); // Consume the fresh receipt, but never use an aged/unready context to write.
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
        if(certified_enabled_||!current(plan)||!plan.has_ticket||!ready_||!plan.accept_mask||
           now<plan.observed_now||!identity(plan.context,current_context)||
           !current_context.idle_ready||!current_context.clean||
           current_context.observed_ms<plan.context.observed_ms||
           !recent(plan.context.observed_ms,now)||!recent(current_context.observed_ms,now)||
           !recent(plan.ticket.created_ms,now))return 0;
        return plan.accept_mask&bound_;
    }
    unsigned authorize_certified(Plan const& plan,Context const& current_context,std::uint64_t now_ms,std::int64_t now_ns) const noexcept {
        if(!certified_enabled_||certified_failed_||!certified_known_||!current(plan)||!plan.has_ticket||!ready_||
           !plan.accept_mask||now_ms<plan.observed_now||now_ns<certified_observed_ns_||
           !identity(plan.context,current_context)||!current_context.idle_ready||!current_context.clean||
           current_context.observed_ms<plan.context.observed_ms||!recent(plan.context.observed_ms,now_ms)||
           !recent(current_context.observed_ms,now_ms)||!recent(plan.ticket.created_ms,now_ms)||
           !gears_hand_continuity::same_certificate(plan.ticket.certificate,certified_)||
           !gears_hand_continuity::certificate_recent(plan.ticket.certificate,now_ns))return 0;
        return plan.accept_mask&bound_;
    }
};
} // namespace gears_hand_stream
