#pragma once
#include "weapon_hand_participant.h"
#include "controller_continuity_frame.h"

namespace gears_lancer_saw_input {
namespace hand=gears_weapon_hand;
struct Actors {
    hand::ObjectIdentity world{},controller{},pawn{},weapon{};
};
inline bool same(Actors const& a,Actors const& b) noexcept {
    return hand::same(a.world,b.world)&&hand::same(a.controller,b.controller)&&
        hand::same(a.pawn,b.pawn)&&hand::same(a.weapon,b.weapon);
}
struct Observation {
    Actors actors{};
    bool controller_known{},button_known{},button_held{},actors_current{},
         current_lancer{},can_start{},doing_special{},motor_running{},doing_melee{};
    unsigned special_move{},fire_mode{};
    bool fire_mode_known{};
};
struct Call {bool invoked{},succeeded{};};
inline bool ready_motion_context(Observation const& o) noexcept {
    // Scope only: an adapter still needs its exact fresh owner/hold/source.
    // The equipment veto during33 must not starve held-gun/contact updates;
    // paired34 and all unrelated animation remain original-owned.
    return o.actors_current&&o.current_lancer&&o.controller_known&&o.button_known&&
        o.button_held&&o.special_move==33&&o.motor_running&&o.doing_melee&&!o.doing_special;
}
// Liveness of an ALREADY journalled original B hold, not a Frame, pose or
// permission to start melee/fire/equipment. A slow game Tick must qualify the
// actual latest authenticated publisher sample, never renew its last Frame.
class HoldInput {
    gears_hand_continuity::Publisher publisher_{};
    gears_hand_continuity::TrackedInput pin_{};
    std::uint64_t sequence_{};std::int64_t sync_{};bool bound_{};
    unsigned control_{gears_vr::control_squeeze};
    bool release_tail_enabled_{};
    std::int64_t release_tail_deadline_{};
    std::uint32_t last_presses_{},last_releases_{};
public:
    void reset() noexcept {*this={};}
    std::int64_t sync_ns() const noexcept {return sync_;}
    bool release_tail_started() const noexcept {return release_tail_deadline_>0;}
    bool release_tail_expired(std::int64_t now) const noexcept {
        return release_tail_deadline_>0&&now>=release_tail_deadline_;
    }
    bool refresh(gears_vr::StereoSample const& stereo,gears_hand_continuity::TrackedInput const& latest,
        gears_hand_continuity::Publisher publisher,std::int64_t now,bool allow_sim) noexcept {
        const auto& c=latest.input;const auto& p=pin_.input;
        if(!bound_||!publisher.pid||!publisher.start||publisher.pid!=publisher_.pid||publisher.start!=publisher_.start||
           !gears_hand_continuity::tracked_input_shape_valid(latest)||
           latest.stream_epoch!=pin_.stream_epoch||latest.tracking_epoch[0]!=pin_.tracking_epoch[0]||
           latest.tracking_epoch[1]!=pin_.tracking_epoch[1]||c.source!=p.source||c.session!=p.session||
           c.space_epoch!=p.space_epoch||c.input_epoch!=p.input_epoch||c.profile_generation!=p.profile_generation||
           c.sequence<sequence_||c.sync_ns<sync_||!gears_vr::stereo_geometry_valid(stereo)||
           !gears_vr::stereo_render_active(stereo)||!gears_vr::stereo_tracking_valid(stereo,allow_sim)||
           !gears_vr::controller_frame_matches(c,stereo,now,allow_sim)||
           now<stereo.received_ns||now-stereo.received_ns>100000000||now<c.sync_ns||now-c.sync_ns>100000000||
           !gears_vr::controller_pose_usable(c.hands[1].grip,c.hands[1].grip_flags,c.hands[1].grip_active)||
           !gears_vr::controller_pose_usable(c.hands[0].grip,c.hands[0].grip_flags,c.hands[0].grip_active))return false;
        const auto& left=c.hands[0];
        const bool control_active=control_==gears_vr::control_trigger?left.trigger_active==1:left.squeeze_active==1;
        const bool down=control_active&&(control_==gears_vr::control_trigger?left.trigger:left.squeeze)>
            gears_vr::controller_analog_release_threshold;
        const auto presses=left.pressed[control_],releases=left.released[control_];
        if(!control_active||presses<last_presses_||releases<last_releases_||
           (c.sequence==sequence_&&(c.sync_ns!=sync_||presses!=last_presses_||releases!=last_releases_)))return false;
        const bool unchanged=presses==p.hands[0].pressed[control_]&&releases==p.hands[0].released[control_];
        if(!release_tail_deadline_&&(!down||!unchanged)){
            // Only the already-started original motor opts into this tail.
            // A real cumulative release, not a low analog value/timeout,
            // starts it ONCE. Regrasping cannot renew the three-second end.
            if(!release_tail_enabled_||releases<=p.hands[0].released[control_]||
               c.sync_ns>INT64_MAX-3000000000LL)return false;
            release_tail_deadline_=c.sync_ns+3000000000LL;
        }
        if(release_tail_expired(now))return false;
        sequence_=c.sequence;sync_=c.sync_ns;last_presses_=presses;last_releases_=releases;return true;
    }
    bool bind(gears_vr::StereoSample const& stereo,gears_hand_continuity::TrackedInput const& input,
        gears_hand_continuity::Publisher publisher,std::int64_t now,bool allow_sim,unsigned control=gears_vr::control_squeeze,
        bool release_tail=false) noexcept {
        reset();if(control!=gears_vr::control_squeeze&&control!=gears_vr::control_trigger)return false;
        control_=control;publisher_=publisher;pin_=input;bound_=true;release_tail_enabled_=release_tail;
        sequence_=input.input.sequence;sync_=input.input.sync_ns;
        last_presses_=input.input.hands[0].pressed[control];last_releases_=input.input.hands[0].released[control];
        if(refresh(stereo,input,publisher,now,allow_sim))return true;
        reset();return false;
    }
};
enum class Status {idle,not_started,owned,released,deferred_animation,cleanup_debt,release_pending};
// Only the normal-GT adapter may invoke callbacks, after its actual original
// context checks. The ledger is established BEFORE a possible press. Failed
// calls cannot erase it or release an unrelated new pawn's key.
class Button {
    Actors owner_{};bool owed_{},execution_seen_{},release_dispatched_{};
public:
    bool owed() const noexcept {return owed_;}
    Actors const& owner() const noexcept {return owner_;}
    void reset_after_proven_up() noexcept {*this={};}
    template<class Press>
    Status begin(Observation const& o,bool current_start_permit,Press press) {
        if(owed_)return Status::cleanup_debt;
        if(!current_start_permit||!o.controller_known||!o.button_known||o.button_held||
           !o.actors_current||!o.current_lancer||!o.can_start||o.special_move||o.doing_special)
            return Status::not_started;
        owner_=o.actors;owed_=true;
        const auto call=press();
        if(!call.invoked){reset_after_proven_up();return Status::not_started;}
        return call.succeeded?Status::owned:Status::cleanup_debt;
    }
    template<class Release>
    Status service(Observation const& o,bool stop,Release release) {
        if(!owed_)return Status::idle;
        // Original input reset / owner button release may prove the exact old
        // controller up even after travel changed its pawn. No call is made.
        if(o.controller_known&&hand::same(o.actors.controller,owner_.controller)&&o.button_known&&!o.button_held){
            reset_after_proven_up();return Status::released;
        }
        if(!o.controller_known||!o.button_known||!o.actors_current||!same(o.actors,owner_))return Status::cleanup_debt;
        execution_seen_=execution_seen_||o.special_move==34;
        if(o.special_move==34||o.doing_special)return Status::deferred_animation;
        if(!stop&&!execution_seen_)return Status::owned;
        // Ready33 may still have bDoingMeleeAttack set while B is held. Waiting
        // for that flag to clear before releasing would deadlock the hold.
        if(o.special_move!=0&&o.special_move!=33)return Status::deferred_animation;
        if(release_dispatched_)return Status::cleanup_debt;
        const auto call=release();
        if(!call.invoked)return Status::cleanup_debt;
        release_dispatched_=true;
        return call.succeeded?Status::release_pending:Status::cleanup_debt;
    }
};
// Releasing original B and leaving original MeleeAttacking are different
// obligations. StopMeleeAttack(false) only clears bWantsToMelee; the original
// force-stop path calls StopFire(2), which runs MeleeAttacking.EndFire/EndState.
// This ledger grants no equipment action and never supplies a new input edge.
class ReadyStop {
    Actors owner_{};bool owed_{},dispatched_{};
public:
    bool owed() const noexcept {return owed_;}
    Actors const& owner() const noexcept {return owner_;}
    void arm(Actors const& owner) noexcept {if(!owed_){owner_=owner;owed_=true;dispatched_=false;}}
    template<class Stop>
    Status service(Observation const& o,Stop stop) {
        if(!owed_)return Status::idle;
        if(!o.controller_known||!o.button_known||o.button_held||!o.actors_current||
           !o.current_lancer||!same(o.actors,owner_)||!o.fire_mode_known)return Status::cleanup_debt;
        // Never interrupt the real victim execution or an unrelated move.
        if(o.special_move==34||o.doing_special||(o.special_move&&o.special_move!=33))return Status::deferred_animation;
        if(!o.special_move&&!o.motor_running&&!o.doing_melee&&o.fire_mode!=9){
            *this={};return Status::released;
        }
        if(dispatched_)return Status::cleanup_debt;
        const auto call=stop();
        if(!call.invoked)return Status::cleanup_debt;
        dispatched_=true;
        return call.succeeded?Status::release_pending:Status::cleanup_debt;
    }
};
} // namespace gears_lancer_saw_input
