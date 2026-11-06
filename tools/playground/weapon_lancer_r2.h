#pragma once
#include "../vr-common/original_r2_fire.h"
#include "../vr-common/weapon_hand_participant.h"
#include "../vr-common/controller_continuity_frame.h"

// PG-local input/ownership policy only. Caller authenticates each Frame and
// freshly certifies Game on the owning GT; Execute supplies the original call.
// Exclusive R2 source ownership remains a caller obligation. No engine reader,
// Native permission, retained Frame, pose or tracking sample lives here.
namespace gears_lancer_r2 {
namespace hand=gears_weapon_hand;
using Status=gears_weapon::OriginalR2Status;
struct Actors {hand::ObjectIdentity world{},controller{},pawn{},mesh{},weapon{};};
struct Game {Actors actors{};bool known{},lancer{},main_hand_held{},animation{},busy{},r2_known{},r2_held{};
    bool l2_known{},l2_held{},toggle_known{},toggle_targeting{};
    bool press_ready{true};
    // Caller must certify the exact looping pre-ammo gate before retaining
    // owned R2 without a muzzle. Neither flag grants a shot or source lease.
    bool retain_owned_for_muzzle{},defer_new_press_for_muzzle{};
};
// Optional exact-cohort publisher witness, read by the existing authenticated
// mapping owner. No action/pose permission or substitute input frame.
struct Continuity {gears_hand_continuity::TrackedInput tracking{};gears_hand_continuity::Publisher publisher{};};
enum class SourceStatus {unavailable,ready,inactive,stale,sync_invalid,regressed,cohort_changed,actors_mismatch,yielded,stopping};
inline bool same(Actors const& a,Actors const& b) noexcept {
    return hand::same(a.world,b.world)&&hand::same(a.controller,b.controller)&&hand::same(a.pawn,b.pawn)&&
        hand::same(a.mesh,b.mesh)&&hand::same(a.weapon,b.weapon);
}
inline bool valid(Actors const& a) noexcept {
    return hand::identity_valid(a.world)&&hand::identity_valid(a.controller)&&hand::identity_valid(a.pawn)&&
        hand::identity_valid(a.mesh)&&hand::identity_valid(a.weapon);
}
// Dispatch admission only, never input loss. A retained owner's other PC
// ticks are ignored; a freshly authenticated new owner is allowed through so
// the ownership policy can retain/fault an owed old release explicitly.
inline bool tick_admitted(bool quiet,std::uint64_t pc,std::uint64_t source_pc,std::uint64_t retained_pc) noexcept {
    return quiet&&pc&&(source_pc?pc==source_pc:!retained_pc||pc==retained_pc);
}
class Input {
    // One undispatched physical press may wait for the current muzzle path.
    // This is an intent deadline, not a source lifetime: EVERY later attempt
    // still requires a new <=100ms Frame, unchanged publisher/tracking/edge
    // counters, a trigger still held and the normal live muzzle reserve.
    // 680 lost the first post-execution press when the next current Frame
    // arrived133ms later: using100ms for both clocks cancelled the still-held
    // intent before that fresh frame could dispatch it.
    static constexpr std::int64_t pending_press_lifetime_ns=250000000;
    gears_weapon::OriginalR2Fire ledger_;
    Actors actors_{};hand::FrameKey cohort_{}; // frame is always zero: not a retained publication.
    std::uint64_t sequence_{};std::int64_t sync_{},observed_{};
    std::uint32_t source_{};bool actors_known_{},bound_{},sealed_{};
    SourceStatus status_{SourceStatus::unavailable};
    gears_hand_continuity::Publisher publisher_{};
    std::uint64_t stream_{},right_tracking_{};bool continuity_known_{},continuous_poll_gap_{};
    std::uint32_t observed_presses_{},observed_releases_{},pending_presses_{},pending_releases_{};
    std::int64_t pending_deadline_{};bool pending_press_{};
    bool same_continuity(Continuity const& c) const noexcept {
        return continuity_known_&&publisher_.pid==c.publisher.pid&&publisher_.start==c.publisher.start&&
            stream_==c.tracking.stream_epoch&&right_tracking_==c.tracking.tracking_epoch[1];
    }
    bool safe(Game const& game) const noexcept {
        return game.known&&game.lancer&&game.r2_known&&valid(game.actors)&&actors_known_&&same(actors_,game.actors);
    }
    gears_weapon::OriginalR2Context context(Game const& game,bool allow) const noexcept {
        return {game.actors.world.address,game.actors.controller.address,game.actors.weapon.address,
            game.known&&game.lancer&&game.r2_known&&valid(game.actors),game.r2_held,allow,safe(game)};
    }
    template<class Execute>Status seal(Game const& game,Execute&& execute) {
        bound_=continuity_known_=false;sealed_=true;pending_press_=false;
        if(ledger_.owns_press()){
            if(!safe(game))return Status::abort_required;
            // Empty source + press disabled invokes the legacy owned hand-back.
            return ledger_.advance({}, {},context(game,false),0,execute);
        }
        if(!game.known||!game.lancer||!game.r2_known||!valid(game.actors))return Status::rejected;
        if(!actors_known_||!same(actors_,game.actors)){
            ledger_.reset();actors_=game.actors;actors_known_=true;
        }
        return ledger_.yield(context(game,false),execute);
    }
public:
    bool owns_press() const noexcept {return ledger_.owns_press();}
    bool awaiting_release() const noexcept {return ledger_.awaiting_release();}
    bool has_actors() const noexcept {return actors_known_;}
    Actors const& actors() const noexcept {return actors_;}
    SourceStatus source_status() const noexcept {return status_;}
    bool continuous_poll_gap() const noexcept {return continuous_poll_gap_;}
    // Diagnostic scalars only; neither value extends source validity.
    std::int64_t last_sync_ns() const noexcept {return sync_;}
    std::int64_t last_observed_ns() const noexcept {return observed_;}
    // Caller has drained its callback lifetime. Never erase an owed release.
    bool reset_drained() noexcept {
        if(owns_press())return false;
        ledger_.reset();actors_={};cohort_={};sequence_=0;sync_=observed_=0;source_=0;
        actors_known_=bound_=sealed_=continuity_known_=continuous_poll_gap_=false;
        publisher_={};stream_=right_tracking_=0;status_=SourceStatus::unavailable;
        observed_presses_=observed_releases_=pending_presses_=pending_releases_=0;
        pending_deadline_=0;pending_press_=false;return true;
    }
    template<class Execute>Status yield(Game const& game,Execute&& execute) {
        status_=SourceStatus::yielded;return seal(game,execute);
    }
    template<class Execute>Status advance(hand::Frame const* frame,bool source_current,Game const& game,
        std::int64_t now,Execute&& execute,bool stopping=false,Continuity const* continuity=nullptr) {
        continuous_poll_gap_=false;
        if(stopping){status_=SourceStatus::stopping;
            if(!owns_press()){bound_=false;sealed_=true;pending_press_=false;return Status::accepted;}
            return seal(game,execute);}
        const auto reject=[&](SourceStatus why){status_=why;return seal(game,execute);};
        // This guard precedes every possible legacy release, including edges.
        if(owns_press()&&!safe(game))return reject(SourceStatus::actors_mismatch);
        if(!frame||!source_current)return reject(SourceStatus::unavailable);
        const auto& f=*frame;
        if(continuity&&(!continuity->publisher.pid||!continuity->publisher.start||
            !gears_hand_continuity::tracked_input_shape_valid(continuity->tracking)||
            !gears_hand_continuity::same_input(continuity->tracking.input,f.controllers)))
            return reject(SourceStatus::cohort_changed);
        if(!hand::frame_active(f,now))return reject(SourceStatus::inactive);
        constexpr std::int64_t age=100000000;
        if(now-f.now_ns>age)return reject(SourceStatus::stale);
        if(f.controllers.sync_ns>now||now-f.controllers.sync_ns>age)return reject(SourceStatus::sync_invalid);
        const Actors actual{f.world,f.controller,f.pawn,f.mesh,f.weapon};
        if(!game.known||!game.lancer||!game.r2_known||!valid(game.actors)||!same(actual,game.actors))
            return reject(SourceStatus::actors_mismatch);
        auto key=f.key;key.frame=0;
        const auto source=static_cast<std::uint32_t>(f.stereo.source);
        if(bound_&&(!hand::same(key,cohort_)||source!=source_||!same(actors_,actual)))
            return reject(SourceStatus::cohort_changed);
        if(bound_&&continuity_known_&&continuity&&!same_continuity(*continuity))
            return reject(SourceStatus::cohort_changed);
        if(bound_&&(now<observed_||f.controllers.sequence<sequence_||f.controllers.sync_ns<sync_||
           (f.controllers.sequence==sequence_&&f.controllers.sync_ns!=sync_)))return reject(SourceStatus::regressed);
        // Current sample age is checked above. Continuity measures producer
        // sync-to-sync spacing, not previous-sync-to-consumer time: otherwise
        // a fresh 70ms interval plus 40ms read latency is falsely sealed.
        if(bound_&&f.controllers.sequence>sequence_){
            if(f.controllers.sync_ns<=sync_)return reject(SourceStatus::sync_invalid);
            if(f.controllers.sync_ns-sync_>age){
                // Both witnesses certify EVERY producer sample: stream epoch
                // advances on a skipped/invalid/>100ms producer sample; right
                // tracking epoch advances on actual right-hand loss. A101ms
                // GT poll with that same certificate is not a source gap.
                // Permit at most two100ms consumer periods ONLY with this
                // exact certificate. Current input/stereo/source age remains
                // <=100ms above; no retained pose/ray/edge authorizes firing.
                // Longer pauses, missing/changed witnesses still seal it.
                if(!continuity||!same_continuity(*continuity)||now-observed_>2*age)
                    return reject(SourceStatus::sync_invalid);
                continuous_poll_gap_=true;
            }
        }
        if(!actors_known_||!same(actors_,actual)){
            // No owned press can pass the safe guard with a changed actor.
            ledger_.reset();actors_=actual;actors_known_=true;
        }
        const bool held_ready=game.main_hand_held&&!game.animation&&!game.busy&&f.controllers.hands[1].trigger_active==1;
        const bool allow=held_ready&&(game.press_ready||(ledger_.owns_press()&&game.retain_owned_for_muzzle));
        const bool prior_bound=bound_,prior_sealed=sealed_;
        const auto p=f.controllers.hands[1].pressed[gears_vr::control_trigger];
        const auto r=f.controllers.hands[1].released[gears_vr::control_trigger];
        const bool down=f.controllers.hands[1].trigger>=gears_vr::controller_analog_press_threshold;
        const bool delay_allowed=game.defer_new_press_for_muzzle&&held_ready&&!game.r2_held&&
            !ledger_.owns_press()&&!ledger_.awaiting_release()&&!prior_sealed&&prior_bound&&
            continuity&&same_continuity(*continuity)&&down;
        bool discard_pending{};
        if(pending_press_&&(!delay_allowed||p!=pending_presses_||r!=pending_releases_||now>pending_deadline_)){
            pending_press_=false;discard_pending=true;
        }
        if(sealed_){const auto sealed=ledger_.yield(context(game,false),execute);
            if(sealed==Status::abort_required)return sealed;sealed_=false;}
        cohort_=key;source_=source;bound_=true;sequence_=f.controllers.sequence;sync_=f.controllers.sync_ns;observed_=now;
        continuity_known_=continuity!=nullptr;
        if(continuity){publisher_=continuity->publisher;stream_=continuity->tracking.stream_epoch;
            right_tracking_=continuity->tracking.tracking_epoch[1];}
        const bool distinct_down=observed_presses_!=UINT32_MAX&&p==observed_presses_+1&&r==observed_releases_&&p>r;
        observed_presses_=p;observed_releases_=r;
        const auto defer=[&](){
            if(!pending_press_){
                // Original producer time bounds the pending edge. A later
                // Native publication never restarts or extends this clock.
                if(f.controllers.sync_ns>INT64_MAX-pending_press_lifetime_ns)return false;
                pending_press_=true;pending_presses_=p;pending_releases_=r;
                pending_deadline_=f.controllers.sync_ns+pending_press_lifetime_ns;
            }
            return now<=pending_deadline_;
        };
        status_=SourceStatus::ready;
        if(!discard_pending&&!game.press_ready&&delay_allowed&&(pending_press_||distinct_down)&&defer())
            return Status::deferred;
        if(held_ready&&!game.press_ready&&!ledger_.owns_press()&&!game.r2_held&&
           f.controllers.hands[1].trigger<=.35f){
            // A released trigger needs no firing ray. Idle intentionally skips
            // the expensive muzzle read; previously that also prevented the
            // ledger from observing UP after any source gap/reload hand-back,
            // so the next real press was consumed as its first baseline.
            // Seal first: the following existing ledger path can ONLY consume
            // this fresh UP baseline, including any unseen tap counters. It
            // cannot replay a press. Actual DOWN still requires press_ready.
            const auto sealed=ledger_.yield(context(game,false),execute);
            if(sealed==Status::abort_required)return sealed;
            return ledger_.advance(f.controllers,f.stereo,context(game,true),now,execute,source==1);
        }
        const auto result=ledger_.advance(f.controllers,f.stereo,context(game,allow&&!discard_pending),now,execute,source==1,
            delay_allowed&&!discard_pending);
        if(result==Status::deferred){
            if(!defer()){
                pending_press_=false;
                return ledger_.advance(f.controllers,f.stereo,context(game,false),now,execute,source==1);
            }
        }else pending_press_=false;
        return result;
    }
};
} // namespace gears_lancer_r2
