#pragma once
#include "rig_tracking_frame.h"
#include "controller_continuity_frame.h"
#include "hand_stream_ticket_policy.h"

// A typed, local consumer of an authenticated Native Rig read. This is only
// ticket/binding bookkeeping; it grants no engine scope, writer or bone lease.
// All calls belong to the owning GT. Invalidation from another thread must be
// marshalled by the owner, just as with the existing hand-stream policy.
namespace gears_hand_rig {
using gears_hand_stream::both_hands;
using gears_hand_stream::ObserveReason;
using gears_rig_tracking::Frame;
struct RigCohort {
    gears_weapon_hand::ObjectIdentity world{},controller{},pawn{},mesh{},weapon{};
};
struct RigBinding {
    gears_rig_tracking::NativeKey native{};
    std::uint32_t expected_source{},publisher_pid{};
    std::uint64_t publisher_start{};
};
struct RigContext : gears_hand_stream::Context {
    // These are fresh local object identities, not addresses copied from Rig.
    RigCohort objects{};
    std::uint32_t game_thread{};
    // Local cache/binding generation; never relabel an XR or Native epoch.
    std::uint64_t input_revision{};
};
struct RigPlan : gears_hand_stream::Plan {
    RigContext rig_context{};
    Frame frame{}; // Complete, unmodified receipt; never stored in Certificate.
    std::int64_t observed_ns{};
    std::uint64_t source_age_ns[2]{}; // stereo, controllers at local observation.
    bool has_frame{},raw_fresh{};
    // Inherited Ticket is diagnostic only. created_ms is the local receipt
    // observation (observed_now), NOT Native uptime or either source's birth.
    // Both genuine source timestamps remain in frame and gate every authorize.
};
inline bool same(RigCohort const& a,RigCohort const& b) noexcept {
    return gears_weapon_hand::same(a.world,b.world)&&gears_weapon_hand::same(a.controller,b.controller)&&
        gears_weapon_hand::same(a.pawn,b.pawn)&&gears_weapon_hand::same(a.mesh,b.mesh)&&
        gears_weapon_hand::same(a.weapon,b.weapon);
}
inline RigCohort cohort(Frame const& f) noexcept {return {f.world,f.controller,f.pawn,f.mesh,f.weapon};}
inline bool same_frame(Frame const& a,Frame const& b) noexcept {
    using gears_vr::stereo_same_pose;
    if(a.size!=b.size||a.abi_version!=b.abi_version||a.status!=b.status||a.flags!=b.flags||
       !gears_rig_tracking::same(a.native,b.native)||a.publication_sequence!=b.publication_sequence||
       a.scene_invocation!=b.scene_invocation||a.world_generation!=b.world_generation||
       a.binding_generation!=b.binding_generation||a.reference_generation!=b.reference_generation||
       a.published_ns!=b.published_ns||a.scene_qpc!=b.scene_qpc||a.qpc_frequency!=b.qpc_frequency||
       a.observed.stream!=b.observed.stream||a.observed.reference!=b.observed.reference||
       a.observed.head_tracking!=b.observed.head_tracking||a.producer.stream!=b.producer.stream||
       a.native_reference.epoch!=b.native_reference.epoch||a.native_reference.flags!=b.native_reference.flags||
       a.native_reference.reserved!=b.native_reference.reserved||
       a.controller_publisher_pid!=b.controller_publisher_pid||a.controller_publisher_start!=b.controller_publisher_start||
       a.view_policy!=b.view_policy||!same(cohort(a),cohort(b))||
       !gears_weapon_hand::same(a.cover_link,b.cover_link)||a.cover_slot!=b.cover_slot||a.cover_type!=b.cover_type||
       a.units_per_metre!=b.units_per_metre||a.source_view!=b.source_view||a.base_view!=b.base_view||
       !stereo_same_pose(a.reference,b.reference)||!gears_vr::stereo_same(a.stereo,b.stereo)||
       !gears_hand_continuity::same_input(a.controllers,b.controllers)||a.world_pose_mask!=b.world_pose_mask||
       a.reserved!=b.reserved||!stereo_same_pose(a.world_head,b.world_head))return false;
    for(unsigned i=0;i<2;++i)
        if(a.observed.hand_tracking[i]!=b.observed.hand_tracking[i]||a.producer.tracking[i]!=b.producer.tracking[i]||
           !stereo_same_pose(a.world_grip[i],b.world_grip[i]))return false;
    return true;
}
class RigPolicy {
    RigBinding binding_{};
    RigContext context_{};
    RigPlan current_{};
    Frame previous_{};
    gears_hand_stream::ObserveDiagnostic diagnostic_{};
    std::uint64_t revision_{},last_present_ms_[2]{},last_observed_ms_{};
    std::int64_t last_observed_ns_{};
    unsigned bound_{},baseline_pending_{both_hands},last_valid_{};
    bool enabled_{},used_{},failed_{},stopped_{},context_known_{},known_{},ready_{};
    static bool context_identity(RigContext const& a,RigContext const& b) noexcept {
        return gears_hand_stream::same(a.game,b.game)&&a.expected_native_world_generation==b.expected_native_world_generation&&
            a.action_epoch==b.action_epoch&&same(a.objects,b.objects)&&a.game_thread==b.game_thread&&
            a.input_revision==b.input_revision;
    }
    static bool context_same(RigContext const& a,RigContext const& b) noexcept {
        return context_identity(a,b)&&a.observed_ms==b.observed_ms&&a.idle_ready==b.idle_ready&&a.clean==b.clean;
    }
    bool context_valid(RigContext const& c) const noexcept {
        auto const& o=c.objects;
        return c.expected_native_world_generation&&c.input_revision&&c.game_thread==binding_.native.game_thread&&
            gears_weapon_hand::identity_valid(o.world)&&gears_weapon_hand::identity_valid(o.controller)&&
            gears_weapon_hand::identity_valid(o.pawn)&&gears_weapon_hand::identity_valid(o.mesh)&&
            gears_weapon_hand::identity_valid(o.weapon,true)&&c.game.world==o.world.address&&
            c.game.controller==o.controller.address&&c.game.pawn==o.pawn.address&&
            c.game.mesh==o.mesh.address&&c.game.weapon==o.weapon.address;
    }
    static bool legacy_context_same(gears_hand_stream::Context const& a,gears_hand_stream::Context const& b) noexcept {
        return gears_hand_stream::same(a.game,b.game)&&a.expected_native_world_generation==b.expected_native_world_generation&&
            a.action_epoch==b.action_epoch&&a.observed_ms==b.observed_ms&&a.idle_ready==b.idle_ready&&a.clean==b.clean;
    }
    static bool ticket_same(gears_hand_stream::Ticket const& a,gears_hand_stream::Ticket const& b) noexcept {
        return gears_hand_stream::same(a.stream,b.stream)&&a.serial==b.serial&&a.sequence==b.sequence&&
            a.created_ms==b.created_ms&&a.controller==b.controller&&a.pawn==b.pawn&&a.weapon==b.weapon&&
            a.units_per_metre==b.units_per_metre&&a.valid_mask==b.valid_mask&&
            gears_hand_continuity::same_certificate(a.certificate,b.certificate);
    }
    bool current(RigPlan const& p) const noexcept {
        return p.revision&&p.revision==current_.revision&&p.observed_now==current_.observed_now&&
            p.has_ticket==current_.has_ticket&&legacy_context_same(p.context,current_.context)&&
            ticket_same(p.ticket,current_.ticket)&&p.accept_mask==current_.accept_mask&&p.bind_mask==current_.bind_mask&&
            p.reset_binding_mask==current_.reset_binding_mask&&p.restore_mask==current_.restore_mask&&
            p.baseline_mask==current_.baseline_mask&&context_same(p.rig_context,current_.rig_context)&&
            p.has_frame==current_.has_frame&&p.raw_fresh==current_.raw_fresh&&p.observed_ns==current_.observed_ns&&
            p.source_age_ns[0]==current_.source_age_ns[0]&&p.source_age_ns[1]==current_.source_age_ns[1]&&
            same_frame(p.frame,current_.frame);
    }
    void diagnose(ObserveReason why,unsigned mask=both_hands) noexcept {
        diagnostic_.reason_masks[static_cast<unsigned>(why)]|=mask;
    }
    void invalidate(RigPlan& p,unsigned mask=both_hands) noexcept {
        bound_&=~mask;baseline_pending_|=mask;p.reset_binding_mask|=mask;p.restore_mask|=mask;
    }
    RigPlan seal(RigPlan p) noexcept {
        p.restore_mask|=both_hands&~p.accept_mask;diagnostic_.bound_after=bound_;current_=p;return p;
    }
    RigPlan reject(RigPlan p,ObserveReason why) noexcept {
        // A malformed/replayed incarnation cannot become valid as clocks or
        // contexts catch up. A new owner must explicitly create a new Policy.
        failed_=true;diagnose(why);invalidate(p);return seal(p);
    }
    bool envelope(Frame const& f,std::int64_t now_ns) const noexcept {
        constexpr auto required=gears_rig_tracking::controllers_matched|gears_rig_tracking::producer_continuity_available;
        return gears_rig_tracking::same(f.native,binding_.native)&&
            static_cast<std::uint32_t>(f.stereo.source)==binding_.expected_source&&
            f.controller_publisher_pid==binding_.publisher_pid&&f.controller_publisher_start==binding_.publisher_start&&
            (f.flags&required)==required&&
            gears_rig_tracking::frame_valid(f,now_ns,gears_rig_tracking::maximum_age_ns,binding_.expected_source==1)&&
            gears_vr::stereo_render_active(f.stereo)&&gears_vr::stereo_geometry_valid(f.stereo)&&
            now_ns>=f.controllers.received_ns&&now_ns-f.controllers.received_ns<=100000000;
    }
    static bool raw_same(Frame const& a,Frame const& b) noexcept {
        return gears_vr::stereo_same(a.stereo,b.stereo)&&gears_hand_continuity::same_input(a.controllers,b.controllers);
    }
    static bool counters_regress(Frame const& f,Frame const& old) noexcept {
        if(f.world_generation<old.world_generation||f.binding_generation<old.binding_generation||
           f.producer.stream<old.producer.stream||f.native_reference.epoch<old.native_reference.epoch||
           f.observed.stream<old.observed.stream||f.observed.reference<old.observed.reference||
           f.observed.head_tracking<old.observed.head_tracking||
           f.stereo.received_ns<old.stereo.received_ns||f.controllers.received_ns<old.controllers.received_ns||
           f.controllers.sync_ns<old.controllers.sync_ns)return true;
        for(unsigned i=0;i<2;++i)if(f.producer.tracking[i]<old.producer.tracking[i]||
            f.observed.hand_tracking[i]<old.observed.hand_tracking[i])return true;
        return false;
    }
public:
    // The owner obtains publisher identity from the first actual Native API
    // read after checking NativeKey/source/age/producer/reference. That first
    // receipt still passes observe and is only a baseline, never a write.
    bool enable(RigBinding const& binding) noexcept {
        auto const& n=binding.native;
        if(used_||enabled_||stopped_||!n.game_pid||!n.game_thread||!n.game_start||!n.native_started_qpc||
           !n.registration_cookie||binding.expected_source>1||!binding.publisher_pid||!binding.publisher_start)return false;
        binding_=binding;enabled_=true;return true;
    }
    bool failed() const noexcept {return failed_;}
    bool stopped() const noexcept {return stopped_;}
    unsigned bound_mask() const noexcept {return bound_;}
    std::uint64_t publication_cursor() const noexcept {return known_?previous_.publication_sequence:0;}
    std::uint64_t raw_sequence_cursor() const noexcept {return known_?previous_.stereo.sequence:0;}
    gears_hand_stream::ObserveDiagnostic const& last_observe() const noexcept {return diagnostic_;}
    gears_hand_stream::ContinuitySnapshot continuity_snapshot(std::uint64_t now_ms) const noexcept {
        gears_hand_stream::ContinuitySnapshot s{};s.context=context_;s.context_known=context_known_;
        s.stream_known=known_;s.ready=ready_&&!stopped_&&!failed_;s.bound_mask=bound_;
        if(known_){s.stream={previous_.stereo.session,previous_.stereo.space_epoch,previous_.controllers.input_epoch,
                          previous_.world_generation};s.serial=previous_.publication_sequence;s.sequence=previous_.stereo.sequence;
            s.units_per_metre=previous_.units_per_metre;}
        for(unsigned i=0;i<2;++i){if(bound_&(1u<<i))s.source_stamp[i]=last_present_ms_[i];
            s.old_ticket_age[i]=gears_hand_stream::observe_age(last_present_ms_[i],now_ms,(bound_&(1u<<i))!=0);}
        s.context_age=gears_hand_stream::observe_age(context_.observed_ms,now_ms,context_known_);return s;
    }
    // No journal lives here. Revocation/stop prohibit forward work but cannot
    // erase the caller's already-owned restore obligations or require new Rig.
    void revoke() noexcept {current_.revision=0;bound_=0;baseline_pending_=both_hands;ready_=false;}
    void stop() noexcept {stopped_=true;revoke();}
    RigPlan observe(RigContext const& c,Frame const* fresh,std::uint64_t now_ms,std::int64_t now_ns) noexcept {
        used_=true;current_.revision=0;
        diagnostic_={};diagnostic_.observed_now=now_ms;diagnostic_.bound_before=bound_;
        diagnostic_.context_age=gears_hand_stream::observe_age(c.observed_ms,now_ms);
        diagnostic_.previous_context_age=gears_hand_stream::observe_age(context_.observed_ms,now_ms,context_known_);
        diagnostic_.new_ticket_age=gears_hand_stream::observe_age(now_ms,now_ms,fresh!=nullptr);
        for(unsigned i=0;i<2;++i)diagnostic_.old_ticket_age[i]=
            gears_hand_stream::observe_age(last_present_ms_[i],now_ms,(bound_&(1u<<i))!=0);
        RigPlan p{};p.context=c;p.rig_context=c;p.observed_now=now_ms;p.observed_ns=now_ns;
        p.has_frame=fresh!=nullptr;if(fresh)p.frame=*fresh;
        if(revision_==(std::numeric_limits<std::uint64_t>::max)())return reject(p,ObserveReason::revision_exhausted);
        p.revision=++revision_;
        const bool identity_changed=!context_known_||!context_identity(context_,c);
        const bool flags_changed=context_known_&&(context_.idle_ready!=c.idle_ready||context_.clean!=c.clean);
        const bool clock_bad=now_ns<=0||c.observed_ms>now_ms||(context_known_&&
            (context_.observed_ms>now_ms||now_ms<last_observed_ms_||now_ns<last_observed_ns_));
        const bool structural=context_valid(c)&&c.idle_ready&&c.clean;
        if(identity_changed){diagnose(ObserveReason::context_identity);invalidate(p);}
        if(flags_changed||clock_bad){diagnose(ObserveReason::context_readiness);invalidate(p);}
        if(!structural){diagnose(ObserveReason::not_ready);invalidate(p);}
        ready_=structural&&!clock_bad&&gears_hand_stream::recent(c.observed_ms,now_ms);
        context_=c;context_known_=true;last_observed_ms_=now_ms;last_observed_ns_=now_ns;
        for(unsigned i=0;i<2;++i)if(bound_&(1u<<i)){
            // Certified geometry survives historical age only. Neither these
            // diagnostic stamps nor a new Scene can renew raw-source freshness.
            if(!gears_hand_stream::recent(last_present_ms_[i],now_ms))diagnose(ObserveReason::binding_age,1u<<i);
            if(last_present_ms_[i]>now_ms)invalidate(p,1u<<i);
        }
        if(!ready_)diagnose(ObserveReason::not_ready);
        if(!enabled_||stopped_){diagnose(ObserveReason::not_ready);invalidate(p);return seal(p);}
        if(failed_)return reject(p,ObserveReason::not_newer);
        if(!fresh){diagnose(ObserveReason::no_receipt);return seal(p);}
        auto const& f=*fresh;
        if(!envelope(f,now_ns))return reject(p,ObserveReason::ticket_shape);
        if(!context_valid(c)||!same(cohort(f),c.objects))return reject(p,ObserveReason::ticket_owner);
        if(f.world_generation!=c.expected_native_world_generation)return reject(p,ObserveReason::ticket_world);
        p.source_age_ns[0]=std::uint64_t(now_ns-f.stereo.received_ns);
        p.source_age_ns[1]=std::uint64_t(now_ns-f.controllers.received_ns);
        p.ticket.stream={f.stereo.session,f.stereo.space_epoch,f.controllers.input_epoch,f.world_generation};
        p.ticket.serial=f.publication_sequence;p.ticket.sequence=f.stereo.sequence;p.ticket.created_ms=now_ms;
        p.ticket.controller=f.controller.address;p.ticket.pawn=f.pawn.address;p.ticket.weapon=f.weapon.address;
        p.ticket.units_per_metre=f.units_per_metre;p.ticket.valid_mask=(f.world_pose_mask>>1)&both_hands;
        bool duplicate_raw=false;
        if(known_){
            auto const& old=previous_;
            if(f.publication_sequence<old.publication_sequence)return reject(p,ObserveReason::not_newer);
            if(f.publication_sequence==old.publication_sequence){
                if(!same_frame(f,old))return reject(p,ObserveReason::not_newer);
                diagnose(ObserveReason::not_newer);return seal(p); // Same Scene is not a fresh receipt.
            }
            if(f.scene_invocation<=old.scene_invocation||f.scene_qpc<old.scene_qpc||
               f.qpc_frequency!=old.qpc_frequency||f.published_ns<old.published_ns||counters_regress(f,old))
                return reject(p,ObserveReason::not_newer);
            if(f.native_reference.epoch==old.native_reference.epoch&&
               (f.reference_generation!=old.reference_generation||!gears_vr::stereo_same_pose(f.reference,old.reference)))
                return reject(p,ObserveReason::not_newer);
            const bool same_session=f.stereo.session==old.stereo.session;
            if(!same_session&&f.producer.stream<=old.producer.stream)return reject(p,ObserveReason::not_newer);
            if(same_session){
                if(f.stereo.sequence<old.stereo.sequence||f.stereo.space_epoch<old.stereo.space_epoch||
                   f.controllers.input_epoch<old.controllers.input_epoch||f.controllers.profile_generation<old.controllers.profile_generation||
                   f.stereo.predicted_time<old.stereo.predicted_time)return reject(p,ObserveReason::not_newer);
                duplicate_raw=f.stereo.sequence==old.stereo.sequence;
                if(duplicate_raw&&!raw_same(f,old))return reject(p,ObserveReason::not_newer);
            }
            const bool global=!same_session||f.stereo.space_epoch!=old.stereo.space_epoch||
                f.controllers.input_epoch!=old.controllers.input_epoch||f.controllers.profile_generation!=old.controllers.profile_generation||
                f.producer.stream!=old.producer.stream||f.native_reference.epoch!=old.native_reference.epoch||
                f.observed.stream!=old.observed.stream||f.observed.reference!=old.observed.reference||
                f.observed.head_tracking!=old.observed.head_tracking||f.world_generation!=old.world_generation||
                f.binding_generation!=old.binding_generation||!same(cohort(f),cohort(old))||
                f.view_policy!=old.view_policy||f.cover_type!=old.cover_type||f.cover_slot!=old.cover_slot||
                !gears_weapon_hand::same(f.cover_link,old.cover_link);
            if(global){diagnose(ObserveReason::stream_changed);invalidate(p);}
            if(f.units_per_metre!=old.units_per_metre){diagnose(ObserveReason::scale_changed);invalidate(p);}
            for(unsigned i=0;i<2;++i)if(f.producer.tracking[i]!=old.producer.tracking[i]||
                f.observed.hand_tracking[i]!=old.observed.hand_tracking[i]){
                diagnose(ObserveReason::tracking_loss,1u<<i);invalidate(p,1u<<i);
            }
        }else {diagnose(ObserveReason::stream_changed);invalidate(p);}
        const auto valid=p.ticket.valid_mask;
        const auto lost=valid?(last_valid_&~valid):both_hands;
        diagnose(ObserveReason::tracking_loss,lost);invalidate(p,lost);
        // Consume the Scene AFTER checking all boundaries, even when its XR
        // sample repeats. A repeat never clears pending baselines or advances
        // the last genuinely fresh source/receipt stamps.
        previous_=f;known_=true;last_valid_=valid;
        if(duplicate_raw){diagnose(ObserveReason::not_newer);return seal(p);}
        p.raw_fresh=true;p.has_ticket=true;
        if(!ready_)return seal(p);
        p.baseline_mask=baseline_pending_;baseline_pending_=0;
        p.accept_mask=valid&~p.baseline_mask;p.bind_mask=p.accept_mask&~bound_;
        for(unsigned i=0;i<2;++i)if(valid&(1u<<i))last_present_ms_[i]=now_ms;
        return seal(p);
    }
    bool commit_binding(RigPlan const& p,unsigned mask) noexcept {
        if(!enabled_||stopped_||failed_||!ready_||!current(p)||!p.has_ticket||!p.raw_fresh||
           !mask||(mask&~p.bind_mask)||(mask&bound_))return false;
        bound_|=mask;return true;
    }
    // live must come from a successful NEW read at this write boundary. A read
    // failure is nullptr, never a cached Frame. Reader minimum publication is
    // merely a lower bound (usually p.frame.publication_sequence-1); the whole
    // returned Frame, including publication/Scene/base/cohort, must equal p.
    // A mismatch retires this plan so it cannot revive through a cached retry.
    unsigned authorize(RigPlan const& p,RigContext const& c,Frame const* live,
                       std::uint64_t now_ms,std::int64_t now_ns) noexcept {
        if(!enabled_||stopped_||failed_||!current(p)||!p.has_ticket||!p.raw_fresh||!ready_||!p.accept_mask)return 0;
        if(!context_identity(p.rig_context,c)||!context_valid(c)||!c.idle_ready||!c.clean){
            // An input/binding/action/object change seen at the write boundary
            // cannot keep geometry merely because observe has not run yet.
            revoke();return 0;
        }
        if(!live||now_ms<p.observed_now||now_ns<p.observed_ns||c.observed_ms<p.context.observed_ms||
           !gears_hand_stream::recent(p.context.observed_ms,now_ms)||!gears_hand_stream::recent(c.observed_ms,now_ms)||
           !gears_hand_stream::recent(p.ticket.created_ms,now_ms)||!envelope(*live,now_ns)||!same_frame(p.frame,*live)){
            current_.revision=0;return 0;
        }
        return p.accept_mask&bound_;
    }
};
static_assert(std::is_trivially_copyable_v<RigPlan>&&std::is_trivially_copyable_v<RigPolicy>);
} // namespace gears_hand_rig
