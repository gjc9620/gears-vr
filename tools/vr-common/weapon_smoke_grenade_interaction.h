#pragma once
#include "body_slots.h"
#include "grenade_throw.h"
#include <cstdint>
#include <limits>
#include <optional>

namespace gears_weapon {

// Saved UnrealScript declarations (not authenticated retail native behavior):
// Weap_SmokeGrenade.ThrowGrenade delegates to Weap_Grenade.ThrowGrenade, whose
// successful original spawn precedes ConsumeAmmo. Proj_SmokeGrenade is distinct
// from Proj_FragGrenade; its GrenadeExplosion creates Emit_SmokeGrenade, whose
// original emitter owns smoke visibility, lifetime and spew/loop audio. See
// analysis/re-kb/{classes-v2,scripts}/WarfareGame, corresponding class/function
// exports. Several native tokens and WeaponProjectiles are unresolved. No mesh
// or animation establishes release, fuse, cancellation, gravity or launch ABI.
// This module accepts those facts only through a current-instance certificate.
inline constexpr char smoke_original_weapon_class[]="Weap_SmokeGrenade";
inline constexpr char smoke_original_projectile_class[]="Proj_SmokeGrenade";
inline constexpr char smoke_original_emitter_class[]="Emit_SmokeGrenade";
inline constexpr char smoke_original_ammo_class[]="WarAmmoType_GrenadeSmoke";

struct SmokeIdentity {
    std::uint64_t world{},pawn{},controller{},weapon{},world_generation{},
                  xr_session{},space_epoch{},input_epoch{},profile_generation{};
    bool valid() const noexcept {
        return world&&pawn&&controller&&weapon&&world_generation&&xr_session&&
            space_epoch&&input_epoch&&profile_generation;
    }
    bool operator==(SmokeIdentity const& b) const noexcept {
        return world==b.world&&pawn==b.pawn&&controller==b.controller&&weapon==b.weapon&&
            world_generation==b.world_generation&&xr_session==b.xr_session&&
            space_epoch==b.space_epoch&&input_epoch==b.input_epoch&&
            profile_generation==b.profile_generation;
    }
};
struct SmokeStamp {
    SmokeIdentity identity{};
    std::uint64_t binding{},sequence{},motion_epoch{};
    std::int64_t monotonic_ns{},game_tick{};
};
struct SmokeInventory {
    std::uint64_t revision{};
    std::uint32_t count{};
    bool count_known{};
    bool valid() const noexcept {return revision&&count_known;}
};
enum class SmokeReleaseInput : std::uint8_t { unknown,right_trigger_release_after_press,right_grip_release_after_hold };
// No local timer or cook duration is invented. In the first mode the original
// projectile decides when its fuse starts, including collision-dependent rules.
enum class SmokeFuseOwner : std::uint8_t { unknown,original_projectile,original_preparation };
struct SmokeFuse {
    std::uint64_t original_serial{};
    std::int64_t started_game_tick{};
    bool active{};
    bool valid() const noexcept {
        return active?(original_serial&&started_game_tick>=0):
            (!original_serial&&started_game_tick==0);
    }
    bool operator==(SmokeFuse const& b) const noexcept {
        return original_serial==b.original_serial&&started_game_tick==b.started_game_tick&&active==b.active;
    }
};
struct SmokeInstanceConfig {
    SmokeIdentity identity{};
    gears_vr::StereoSource input_source{gears_vr::StereoSource::RuntimeTracked};
    std::uint64_t config_serial{},original_weapon_type{},original_projectile_type{},
                  original_ammo_type{},original_emitter_type{},original_no_ammo_cue{},
                  grip_geometry_serial{},release_rule_serial{},fuse_rule_serial{},audio_rule_serial{},
                  prepare_entry{},launch_entry{},safety_entry{};
    SmokeReleaseInput release_input{SmokeReleaseInput::unknown};
    SmokeFuseOwner fuse_owner{SmokeFuseOwner::unknown};
    Vector centre_in_right_grip_metres{};
    // Gravity is an adapter-supplied original value. Other tracker defaults
    // are VR observation bounds, never original grenade/fuse parameters.
    GrenadeThrowConfig motion{0};
    std::int64_t request_lease_ns{100000000}; // VR lease, never an original fuse duration.
    bool current_instance_verified{},original_smoke_classes_verified{},
         grip_geometry_verified{},release_input_verified{},fuse_rule_verified{},
         original_prepare_no_ammo_change_verified{},original_launch_once_verified{},
         original_initial_pose_velocity_override_verified{},original_effects_preserved_verified{},
         non_throwing_safety_entry_verified{},original_gravity_and_units_verified{},
         original_smoke_emitter_path_verified{},original_throw_and_smoke_audio_verified{},
         original_no_ammo_cue_verified{};
    // The safety certificate includes the receipt rules below: an unarmed
    // preparation is reclaimed without spending; a live original fuse needs
    // its original non-throwing consequence and one original consume/spawn.
    // An instance without such an authenticated path cannot bind this module.
    bool valid() const noexcept {
        return identity.valid()&&gears_vr::stereo_source_known(input_source)&&config_serial&&original_weapon_type&&original_projectile_type&&
            original_ammo_type&&original_emitter_type&&original_no_ammo_cue&&grip_geometry_serial&&
            release_rule_serial&&fuse_rule_serial&&audio_rule_serial&&prepare_entry&&launch_entry&&safety_entry&&
            (release_input==SmokeReleaseInput::right_trigger_release_after_press||
             release_input==SmokeReleaseInput::right_grip_release_after_hold)&&
            (fuse_owner==SmokeFuseOwner::original_projectile||
             fuse_owner==SmokeFuseOwner::original_preparation)&&
            gears_vr::finite(centre_in_right_grip_metres)&&length(centre_in_right_grip_metres)<=.3&&
            motion.valid()&&request_lease_ns>0&&request_lease_ns<=1000000000&&
            current_instance_verified&&original_smoke_classes_verified&&grip_geometry_verified&&
            release_input_verified&&fuse_rule_verified&&original_prepare_no_ammo_change_verified&&
            original_launch_once_verified&&original_initial_pose_velocity_override_verified&&
            original_effects_preserved_verified&&non_throwing_safety_entry_verified&&
            original_gravity_and_units_verified&&original_smoke_emitter_path_verified&&
            original_throw_and_smoke_audio_verified&&original_no_ammo_cue_verified;
    }
};
struct SmokeWorldMapping {
    std::uint64_t motion_epoch{},reference_generation{};
    Pose frozen_reference{};
    gears_vr::Viewpoint game_base{};
    Vector pawn_velocity_world_units_per_second{};
    double units_per_metre{};
    bool current_instance_verified{};
};
struct SmokeRotationObservation {
    bool valid{},history_supplied{},history_current{};
    bool pitch_changed{},roll_changed{},yaw_changed{},stick_intent{},continuous{};
    bool locomotion_intent{},pitch_continuous{},current_stick_intent{};
    bool episode_before{},episode_after{},episode_continuation{},episode_reset{};
    std::uint64_t history_count{},consumer_gap_ns{};
    double old_pitch{},old_yaw{},old_roll{},new_pitch{},new_yaw{},new_roll{};
    double yaw_delta{},pitch_delta{},rotation_magnitude{};
};
struct SmokeFrame {
    SmokeStamp stamp{};
    SmokeInventory inventory{};
    gears_vr::ControllerInputFrame hands{};
    gears_vr::StereoSample stereo{};
    gears_hand_continuity::MotionHistory motion_history{};
    SmokeWorldMapping mapping{};
    std::uint64_t right_pocket_weapon{},weapon_type{},projectile_type{},ammo_type{},emitter_type{};
    std::uint64_t equipment_hold_generation{};
    // EquipmentModel must have confirmed the original equip before taking a
    // local visual reservation. This module never clones an inventory actor.
    bool original_equipped{},original_interaction_allowed{},original_owns_hands{};
};
enum class SmokeInputKind : std::uint8_t { none,right_grip_press,right_trigger_press,right_trigger_release,right_grip_release };
struct SmokeInput {
    SmokeInputKind kind{SmokeInputKind::none};
    std::uint64_t edge_serial{}; // One increasing serial across all right-hand edges.
    std::uint64_t hand_sequence{},input_epoch{};
    std::int64_t sync_ns{}; // Edge observation time, never XR predicted pose time.
    unsigned hand{1};
    BodySlot slot{BodySlot::none}; // body_slots.h result for this exact right-hand sample.
    bool continuous_input{}; // Recovery/baseline edges are never actions.
};
enum class SmokeTriggerObservation : std::uint8_t {baseline,duplicate,quiet,edge,invalid,ambiguous,exhausted};
// One normal-GT input observer, separate from the equipment Grip owner. The
// exact authenticated ring supplies the original trigger-edge pose/time.
// This classifier neither constructs a Frame nor prepares/spawns anything.
// Optional passive refusal context. Captured before clear()/advancing the
// previous row; it cannot supply an input edge or change observer admission.
struct SmokeTriggerDiagnostic {
    std::uint64_t sequence{},stream{},tracking{};
    std::int64_t sync_ns{};
    std::uint32_t pressed{},released{},active{};
    float level{};
    unsigned reason{};
    bool known{},cohort{};
};
class SmokeTriggerObserver {
    SmokeIdentity identity_{};std::uint64_t binding_{},hold_{},serial_{};
    gears_hand_continuity::TrackedInput previous_{};bool known_{};
public:
    void clear() noexcept {identity_={};binding_=hold_=0;previous_={};known_=false;}
    SmokeTriggerObservation observe(SmokeFrame const& f,SmokeInput& out,
        SmokeReleaseInput rule=SmokeReleaseInput::right_trigger_release_after_press,
        SmokeTriggerDiagnostic* diagnostic=nullptr) noexcept {
        out={};
        const bool grip=rule==SmokeReleaseInput::right_grip_release_after_hold;
        const auto control=grip?gears_vr::control_squeeze:gears_vr::control_trigger;
        if(diagnostic){
            *diagnostic={};diagnostic->known=known_;
            diagnostic->sequence=previous_.input.sequence;diagnostic->sync_ns=previous_.input.sync_ns;
            diagnostic->stream=previous_.stream_epoch;diagnostic->tracking=previous_.tracking_epoch[1];
            const auto& h=previous_.input.hands[1];
            diagnostic->pressed=h.pressed[control];diagnostic->released=h.released[control];
            diagnostic->active=grip?h.squeeze_active:h.trigger_active;diagnostic->level=grip?h.squeeze:h.trigger;
        }
        const auto refused=[&](SmokeTriggerObservation result,unsigned reason,bool reset) noexcept {
            if(diagnostic)diagnostic->reason=reason;
            if(reset)clear();
            return result;
        };
        if(!f.stamp.identity.valid()||!f.stamp.binding||!f.equipment_hold_generation||
           !f.motion_history.count||f.motion_history.count>gears_hand_continuity::motion_history_capacity)
            return refused(SmokeTriggerObservation::invalid,1,true);
        const auto& current=f.motion_history.frames[f.motion_history.count-1];
        if(!gears_hand_continuity::same_input(current.input,f.hands))return refused(SmokeTriggerObservation::invalid,2,true);
        if(!gears_hand_continuity::motion_history_valid(f.motion_history,current,1))return refused(SmokeTriggerObservation::invalid,3,true);
        if(f.stamp.monotonic_ns<current.input.sync_ns)return refused(SmokeTriggerObservation::invalid,4,true);
        const bool cohort=known_&&identity_==f.stamp.identity&&binding_==f.stamp.binding&&hold_==f.equipment_hold_generation&&
            previous_.stream_epoch==current.stream_epoch&&previous_.tracking_epoch[1]==current.tracking_epoch[1]&&
            gears_hand_continuity::detail::same_key(previous_.input,current.input);
        if(diagnostic)diagnostic->cohort=cohort;
        if(cohort&&gears_hand_continuity::same_tracking(previous_,current))return SmokeTriggerObservation::duplicate;
        const auto prior=previous_;identity_=f.stamp.identity;binding_=f.stamp.binding;hold_=f.equipment_hold_generation;
        previous_=current;known_=true;
        if(!cohort)return SmokeTriggerObservation::baseline;
        if(current.input.sequence<=prior.input.sequence||current.input.sync_ns<=prior.input.sync_ns)return refused(SmokeTriggerObservation::invalid,5,true);
        const auto& old=prior.input.hands[1];const auto& tip=current.input.hands[1];
        if((grip?tip.squeeze_active:tip.trigger_active)!=1||
           (grip?old.squeeze_active:old.trigger_active)!=1)return refused(SmokeTriggerObservation::invalid,6,true);
        if(tip.pressed[control]<old.pressed[control]||tip.released[control]<old.released[control])return refused(SmokeTriggerObservation::invalid,7,true);
        if(tip.pressed[control]==old.pressed[control]&&tip.released[control]==old.released[control])return SmokeTriggerObservation::quiet;
        const bool press=old.pressed[control]!=UINT32_MAX&&tip.pressed[control]==old.pressed[control]+1&&tip.released[control]==old.released[control];
        const bool release=old.released[control]!=UINT32_MAX&&tip.released[control]==old.released[control]+1&&tip.pressed[control]==old.pressed[control];
        if(press==release)return refused(SmokeTriggerObservation::ambiguous,8,false);
        for(unsigned i=0;i<f.motion_history.count;++i){const auto& row=f.motion_history.frames[i];const auto& h=row.input.hands[1];
            if(row.input.sequence<=prior.input.sequence)continue;
            const auto* before=i?&f.motion_history.frames[i-1]:&prior;
            const auto level=grip?h.squeeze:h.trigger;
            if(!gears_hand_continuity::motion_pair(*before,row,1,100000000)||(grip?h.squeeze_active:h.trigger_active)!=1||
               before->input.hands[1].pressed[control]!=old.pressed[control]||
               before->input.hands[1].released[control]!=old.released[control]||
               h.pressed[control]!=tip.pressed[control]||h.released[control]!=tip.released[control]||
               (press?level<.65f:level>.35f))continue;
            if(serial_==UINT64_MAX)return refused(SmokeTriggerObservation::exhausted,9,false);
            out.kind=grip?(press?SmokeInputKind::right_grip_press:SmokeInputKind::right_grip_release):
                (press?SmokeInputKind::right_trigger_press:SmokeInputKind::right_trigger_release);
            out.edge_serial=++serial_;out.hand_sequence=row.input.sequence;out.input_epoch=row.input.input_epoch;
            out.sync_ns=row.input.sync_ns;out.continuous_input=true;
            return SmokeTriggerObservation::edge;
        }
        // Ring overwritten, skipped edge, multiple cycles or boundary: never
        // invent the missing edge at the current hand position/poll clock.
        return refused(SmokeTriggerObservation::ambiguous,10,false);
    }
};
enum class SmokeAction : std::uint8_t { none,prepare_original,launch_original,preserve_original_on_interrupt };
enum class SmokeStage : std::uint8_t { unbound,idle,held,prepare_pending,prepared,launch_pending,safety_pending,interrupted };
enum class SmokeInterruptReason : std::uint8_t {none,stamp,frame_contract,grip_lost,history,missing_history,
    edge,mapping_boundary,grip_cycle,pending_release,pending_timeout,tracker,release_sequence,solution,issue};
enum class SmokeEvent : std::uint8_t { rejected,observed,no_ammo,grabbed,stowed,
    request_ready,prepared,thrown,motion_reset,interrupted,safety_required,safety_completed,safety_violation };
struct SmokeDecision {
    SmokeEvent event{SmokeEvent::rejected};
    bool request_original_no_ammo_sound{}; // Intent only; adapter plays original audio once.
    std::uint64_t audio_token{},original_cue{},binding{};
    SmokeIdentity identity{};
    // Dispatcher deduplicates audio_token. No cue is replayed across a bind.
    bool audio_for(SmokeIdentity const& current,std::uint64_t current_binding) const noexcept {
        return request_original_no_ammo_sound&&event==SmokeEvent::no_ammo&&audio_token&&original_cue&&
            binding==current_binding&&identity.valid()&&identity==current;
    }
};
struct SmokeRequest {
    SmokeIdentity identity{};
    std::uint64_t binding{},config_serial{},token{},held_serial{},original_preparation_serial{},
                  original_entry{},original_projectile_type{},original_ammo_type{},original_emitter_type{},
                  motion_epoch{},reference_generation{};
    SmokeAction action{SmokeAction::none};
    SmokeInventory before{};
    std::uint32_t expected_count{};
    SmokeFuse fuse{};
    std::int64_t authored_ns{},valid_until_ns{},authored_game_tick{};
    GrenadeThrowSolution tracked{};
    GameGrenadeLaunch launch{};
    bool executable_at(std::int64_t now,SmokeIdentity const& current,std::uint64_t current_binding,
                       std::uint64_t current_motion_epoch,std::uint64_t current_reference_generation) const noexcept {
        return identity.valid()&&identity==current&&binding&&binding==current_binding&&token&&
            config_serial&&original_entry&&held_serial&&before.valid()&&original_projectile_type&&
            original_ammo_type&&original_emitter_type&&action!=SmokeAction::none&&
            now>=authored_ns&&now<=valid_until_ns&&
            (action==SmokeAction::preserve_original_on_interrupt||
             (motion_epoch==current_motion_epoch&&reference_generation==current_reference_generation));
    }
};
// Deliver real original receipts before an ordinary inventory snapshot carrying
// their delta. Native event serials are increasing within the game-thread owner.
// Effect/audio booleans attest preservation of the original execution paths;
// they do not claim that later smoke rendering or audio output was observed.
struct SmokeOriginalReceipt {
    SmokeStamp stamp{};
    std::uint64_t request_token{},original_event_serial{},original_preparation_serial{},
                  original_projectile{},original_projectile_type{},config_serial{},original_entry{},
                  original_ammo_type{},original_emitter_type{},executed_motion_epoch{},executed_reference_generation{};
    std::int64_t executed_ns{};
    SmokeInventory inventory{};
    SmokeFuse fuse{};
    std::uint32_t original_projectiles_spawned{},original_ammo_consumptions{};
    bool accepted{},original_effect_applied_once{},original_fuse_preserved{},
         original_effects_preserved{},original_smoke_emitter_path_preserved{},original_audio_path_preserved{},
         tracked_initial_pose_and_velocity_applied{},native_safety_complete{},native_non_throwing_consequence{};
};

// One noncopyable game-thread owner. Requests are dispatched once and must be
// executed idempotently by token. A timeout/cancel NEVER retries an ambiguous
// native operation. Only an original receipt changes the ammo mirror.
// Launch changes only the original grenade's initial pose/velocity; adapter
// retains the actual smoke class, chain skinning/animation, bounce, fuse,
// emitter/visibility effects, throw/spew/loop audio and original inventory path,
// suppresses the original preview/velocity assignment for this one launch, and
// performs spawn + one ConsumeAmmo as one authenticated original operation.
// This header contains no game hook, projectile simulation or custom explosion.
class SmokeGrenadeInteraction {
    SmokeInstanceConfig config_{};
    SmokeInventory inventory_{};
    SmokeStage stage_{SmokeStage::unbound};
    SmokeRequest request_{};
    SmokeFuse fuse_{};
    GrenadeThrowTracker tracker_{};
    SmokeWorldMapping history_mapping_{};
    std::uint64_t next_token_{1},binding_{},sequence_{},last_edge_{},last_native_event_{},
                  held_serial_{},preparation_serial_{},last_preparation_serial_{},
                  motion_epoch_{},reference_generation_{},last_hand_sequence_{},trigger_press_sequence_{};
    std::int64_t ns_{},game_tick_{},last_hand_predicted_time_{},last_hand_sync_ns_{};
    bool dispatched_{},interrupt_after_receipt_{};
    bool history_mode_{};std::uint64_t motion_stream_{},motion_tracking_{};
    SmokeInterruptReason last_interrupt_reason_{};unsigned last_boundary_mask_{};
    bool yaw_episode_{};SmokeRotationObservation last_rotation_observation_{};
    std::uint64_t equipment_hold_generation_{};
    std::uint32_t grip_pressed_{},grip_released_{};
    bool grip_release() const noexcept {return config_.release_input==SmokeReleaseInput::right_grip_release_after_hold;}
    SmokeDecision prepare_grip(SmokeFrame const& f) noexcept {
        trigger_press_sequence_=f.hands.sequence;
        grip_pressed_=f.hands.hands[1].pressed[gears_vr::control_squeeze];
        grip_released_=f.hands.hands[1].released[gears_vr::control_squeeze];
        equipment_hold_generation_=f.equipment_hold_generation;
        if(!issue(SmokeAction::prepare_original))return {interrupt()};
        request_.valid_until_ns=(std::min)(request_.valid_until_ns,f.hands.sync_ns+config_.request_lease_ns);
        return {SmokeEvent::request_ready};
    }

    std::uint64_t token() noexcept {
        if(next_token_==(std::numeric_limits<std::uint64_t>::max)())return 0;
        return next_token_++;
    }
    bool stamp_valid(SmokeStamp const& s) const noexcept {
        return s.identity==config_.identity&&s.binding==binding_&&s.sequence>sequence_&&
            s.motion_epoch&&s.monotonic_ns>ns_&&s.game_tick>=game_tick_;
    }
    void consume(SmokeStamp const& s) noexcept {
        sequence_=s.sequence;ns_=s.monotonic_ns;game_tick_=s.game_tick;
    }
    bool count_matches(SmokeInventory const& i,std::uint32_t count,std::uint64_t revision) const noexcept {
        return i.valid()&&i.count==count&&i.revision>=revision;
    }
    static bool same_pose(Pose const& a,Pose const& b) noexcept {
        return a.position.x==b.position.x&&a.position.y==b.position.y&&a.position.z==b.position.z&&
            a.orientation.x==b.orientation.x&&a.orientation.y==b.orientation.y&&
            a.orientation.z==b.orientation.z&&a.orientation.w==b.orientation.w;
    }
    static bool same_motion_basis(SmokeWorldMapping const& a,SmokeWorldMapping const& b) noexcept {
        // The authenticated game origin may translate (walking/cover). It is
        // not an XR history sample and must never become fitted hand speed.
        // Reference, units and rotation remain discontinuity boundaries.
        return same_pose(a.frozen_reference,b.frozen_reference)&&a.units_per_metre==b.units_per_metre&&
            a.game_base.rotation.pitch==b.game_base.rotation.pitch&&
            a.game_base.rotation.yaw==b.game_base.rotation.yaw&&a.game_base.rotation.roll==b.game_base.rotation.roll;
    }
    void clear_local_hold() noexcept {
        tracker_.reset();held_serial_=preparation_serial_=trigger_press_sequence_=0;fuse_={};yaw_episode_=false;
    }
    bool issue(SmokeAction action) noexcept {
        const auto id=token();
        if(!id||ns_>(std::numeric_limits<std::int64_t>::max)()-config_.request_lease_ns)return false;
        request_={};request_.identity=config_.identity;request_.binding=binding_;
        request_.config_serial=config_.config_serial;request_.token=id;
        request_.held_serial=held_serial_;request_.original_preparation_serial=preparation_serial_;
        request_.original_projectile_type=config_.original_projectile_type;
        request_.original_ammo_type=config_.original_ammo_type;
        request_.original_emitter_type=config_.original_emitter_type;request_.action=action;
        request_.motion_epoch=motion_epoch_;request_.reference_generation=reference_generation_;
        request_.before=inventory_;request_.expected_count=inventory_.count;request_.fuse=fuse_;
        request_.authored_ns=ns_;request_.valid_until_ns=ns_+config_.request_lease_ns;
        request_.authored_game_tick=game_tick_;dispatched_=false;
        switch(action){
            case SmokeAction::prepare_original:request_.original_entry=config_.prepare_entry;
                stage_=SmokeStage::prepare_pending;break;
            case SmokeAction::launch_original:request_.original_entry=config_.launch_entry;
                request_.expected_count=inventory_.count-1;stage_=SmokeStage::launch_pending;break;
            case SmokeAction::preserve_original_on_interrupt:request_.original_entry=config_.safety_entry;
                if(fuse_.active)request_.expected_count=inventory_.count-1;
                stage_=SmokeStage::safety_pending;break;
            default:return false;
        }
        return true;
    }
    SmokeEvent interrupt() noexcept {
        tracker_.reset();yaw_episode_=false;last_rotation_observation_.episode_after=false;
        if(request_.token){
            if(dispatched_){interrupt_after_receipt_=true;return SmokeEvent::interrupted;}
            if(request_.action==SmokeAction::preserve_original_on_interrupt)return SmokeEvent::safety_required;
            // No adapter has taken this request: native execution is impossible.
            // Withdraw it before issuing safety, never dispatch an old throw.
            request_={};
        }
        if(preparation_serial_){
            if(issue(SmokeAction::preserve_original_on_interrupt))return SmokeEvent::safety_required;
            stage_=SmokeStage::interrupted;return SmokeEvent::safety_violation;
        }
        clear_local_hold();stage_=SmokeStage::interrupted;return SmokeEvent::interrupted;
    }
    SmokeDecision interrupted(SmokeInterruptReason reason) noexcept {
        last_interrupt_reason_=reason;return {interrupt()};
    }
    bool tracked(SmokeFrame const& f) const noexcept {
        auto const& h=f.hands;
        return h.session==config_.identity.xr_session&&h.space_epoch==config_.identity.space_epoch&&
            h.input_epoch==config_.identity.input_epoch&&h.profile_generation==config_.identity.profile_generation&&
            (grip_release()||h.hands[1].trigger_active==1)&&h.hands[1].squeeze_active==1&&
            h.sync_ns<=f.stamp.monotonic_ns&&f.stamp.monotonic_ns-h.sync_ns<=config_.motion.maximum_sample_gap_ns&&
            h.source==config_.input_source&&f.stereo.source==config_.input_source&&
            gears_vr::controller_frame_matches(h,f.stereo,f.stamp.monotonic_ns,
                config_.input_source==gears_vr::StereoSource::MetaOperatorSimulated)&&
            gears_vr::controller_pose_usable(h.hands[1].grip,h.hands[1].grip_flags,h.hands[1].grip_active);
    }
    bool mapping_valid(SmokeFrame const& f) const noexcept {
        auto const& m=f.mapping;
        return m.current_instance_verified&&m.motion_epoch==f.stamp.motion_epoch&&m.reference_generation&&
            gears_vr::stereo_pose_valid(m.frozen_reference)&&gears_vr::finite(m.game_base.position)&&
            std::isfinite(m.game_base.rotation.pitch)&&std::isfinite(m.game_base.rotation.yaw)&&
            std::isfinite(m.game_base.rotation.roll)&&gears_vr::finite(m.pawn_velocity_world_units_per_second)&&
            std::isfinite(m.units_per_metre)&&m.units_per_metre>=10&&m.units_per_metre<=500;
    }
public:
    SmokeGrenadeInteraction()=default;
    SmokeGrenadeInteraction(SmokeGrenadeInteraction const&)=delete;
    SmokeGrenadeInteraction& operator=(SmokeGrenadeInteraction const&)=delete;
    bool bind(SmokeInstanceConfig const& config,SmokeInventory const& inventory) noexcept {
        if(request_.token||held_serial_||preparation_serial_||!config.valid()||!inventory.valid())return false;
        const auto b=token();if(!b)return false;
        config_=config;inventory_=inventory;binding_=b;stage_=SmokeStage::idle;
        sequence_=last_edge_=last_native_event_=last_preparation_serial_=motion_epoch_=
            reference_generation_=last_hand_sequence_=trigger_press_sequence_=0;
        ns_=game_tick_=last_hand_predicted_time_=last_hand_sync_ns_=0;
        fuse_={};tracker_.reset();request_={};history_mapping_={};
        equipment_hold_generation_=0;grip_pressed_=grip_released_=0;
        history_mode_=false;motion_stream_=motion_tracking_=0;
        last_interrupt_reason_=SmokeInterruptReason::none;last_boundary_mask_=0;yaw_episode_=false;last_rotation_observation_={};
        dispatched_=interrupt_after_receipt_=false;return true;
    }
    SmokeDecision adopt_equipment_hold(SmokeFrame const& f) noexcept {
        if(stage_!=SmokeStage::idle||!f.equipment_hold_generation||!f.original_equipped)return {};
        if(grip_release()&&equipment_hold_generation_&&
           (f.equipment_hold_generation<=equipment_hold_generation_||
            f.hands.hands[1].pressed[gears_vr::control_squeeze]<=grip_pressed_))return {};
        const auto& right=f.hands.hands[1];
        if(grip_release()&&(right.squeeze_active!=1||right.squeeze<=.35f||
           !right.pressed[gears_vr::control_squeeze]))return {};
        const auto observed=update(f);
        if(stage_!=SmokeStage::idle)return observed;
        if(sequence_!=f.stamp.sequence||ns_!=f.stamp.monotonic_ns)return {};
        if(!inventory_.count){const auto id=token();if(!id)return {SmokeEvent::safety_violation};
            if(grip_release()){equipment_hold_generation_=f.equipment_hold_generation;
                grip_pressed_=right.pressed[gears_vr::control_squeeze];grip_released_=right.released[gears_vr::control_squeeze];}
            return {SmokeEvent::no_ammo,true,id,config_.original_no_ammo_cue,binding_,config_.identity};}
        held_serial_=token();if(!held_serial_)return {SmokeEvent::safety_violation};
        stage_=SmokeStage::held;
        return grip_release()?prepare_grip(f):SmokeDecision{SmokeEvent::grabbed};
    }
    std::uint64_t binding_token() const noexcept {return binding_;}
    SmokeInterruptReason last_interrupt_reason() const noexcept {return last_interrupt_reason_;}
    unsigned last_boundary_mask() const noexcept {return last_boundary_mask_;}
    SmokeRotationObservation const& last_rotation_observation() const noexcept {return last_rotation_observation_;}
    SmokeStage stage() const noexcept {return stage_;}
    SmokeInventory inventory() const noexcept {return inventory_;}
    SmokeFuse original_fuse() const noexcept {return fuse_;}
    unsigned reserved_count() const noexcept {return held_serial_?1u:0u;}
    unsigned available_count() const noexcept {return inventory_.count-reserved_count();}
    bool suppress_original_trajectory() const noexcept {return held_serial_!=0;}
    SmokeRequest const& pending_request() const noexcept {return request_;}
    std::optional<SmokeRequest> take_request() noexcept {
        if(!request_.token||dispatched_||ns_>request_.valid_until_ns)return std::nullopt;
        dispatched_=true;return request_;
    }
    // Includes switching away, pause, death or surrendering hands to animation.
    // Adapter must still reconcile an already dispatched request in its OLD
    // identity; it must never execute it on the new weapon/world instance.
    SmokeEvent cancel() noexcept {
        if(stage_==SmokeStage::unbound)return SmokeEvent::rejected;
        return interrupt();
    }
    SmokeDecision update(SmokeFrame const& f,SmokeInput const& input={}) noexcept {
        last_rotation_observation_={};
        if(stage_==SmokeStage::unbound||stage_==SmokeStage::interrupted)return {};
        if(f.stamp.binding!=binding_)return {};
        if(!stamp_valid(f.stamp)){
            // Late/replayed frames cannot release or refresh a velocity lease.
            return interrupted(SmokeInterruptReason::stamp);
        }
        const auto gap=ns_?f.stamp.monotonic_ns-ns_:0;
        consume(f.stamp);
        if(!count_matches(f.inventory,inventory_.count,inventory_.revision)||
           f.right_pocket_weapon!=config_.identity.weapon||!f.original_equipped||
           f.weapon_type!=config_.original_weapon_type||f.projectile_type!=config_.original_projectile_type||
           f.ammo_type!=config_.original_ammo_type||f.emitter_type!=config_.original_emitter_type||
           !f.original_interaction_allowed||f.original_owns_hands||!tracked(f)||!mapping_valid(f)||
           f.hands.sequence<=last_hand_sequence_||f.hands.predicted_time<=last_hand_predicted_time_||
           f.hands.sync_ns<=last_hand_sync_ns_){return interrupted(SmokeInterruptReason::frame_contract);}
        inventory_=f.inventory;last_hand_sequence_=f.hands.sequence;
        last_hand_predicted_time_=f.hands.predicted_time;last_hand_sync_ns_=f.hands.sync_ns;
        const auto release_kind=grip_release()?SmokeInputKind::right_grip_release:SmokeInputKind::right_trigger_release;
        if(grip_release()&&held_serial_&&
           (f.equipment_hold_generation!=equipment_hold_generation_||
            (f.hands.hands[1].squeeze<=.35f&&input.kind!=release_kind)))return interrupted(SmokeInterruptReason::grip_lost);
        const bool history_supplied=f.motion_history.count||f.motion_history.reserved;
        const gears_hand_continuity::TrackedInput* motion_current=nullptr;
        if(history_supplied){
            // Same publisher/epoch/counters/tracking use the existing100ms
            // source qualification, as the raw edge observer does. Latest
            // input freshness is still tracked()'s60ms. The tracker discards
            // motion across >60ms rather than cancelling an unlit held item.
            if(!f.motion_history.count||f.motion_history.count>gears_hand_continuity::motion_history_capacity)return interrupted(SmokeInterruptReason::history);
            motion_current=&f.motion_history.frames[f.motion_history.count-1];
            if(!gears_hand_continuity::same_input(motion_current->input,f.hands)||
               !gears_hand_continuity::motion_history_valid(f.motion_history,*motion_current,1,100000000))return interrupted(SmokeInterruptReason::history);
        }else if(history_mode_)return interrupted(SmokeInterruptReason::missing_history);
        bool edge=false;
        if(input.kind!=SmokeInputKind::none){
            const bool newer=input.edge_serial>last_edge_;
            if(input.hand==1&&newer)last_edge_=input.edge_serial;
            const gears_vr::ControllerInputFrame* edge_frame=&f.hands;
            if(history_supplied){edge_frame=nullptr;
                for(unsigned i=0;i<f.motion_history.count;++i){const auto& h=f.motion_history.frames[i].input;
                    if(h.sequence==input.hand_sequence&&h.sync_ns==input.sync_ns){edge_frame=&h;break;}}
            }
            const bool fresh=input.hand==1&&input.edge_serial&&newer&&input.continuous_input&&edge_frame&&
                input.hand_sequence==edge_frame->sequence&&input.input_epoch==f.hands.input_epoch&&
                input.sync_ns==edge_frame->sync_ns&&input.sync_ns<=ns_&&
                ns_-input.sync_ns<=config_.request_lease_ns;
            auto const& right=edge_frame?edge_frame->hands[1]:f.hands.hands[1];
            const bool grip_input=input.kind==SmokeInputKind::right_grip_press||input.kind==SmokeInputKind::right_grip_release;
            const auto control=grip_input?gears_vr::control_squeeze:gears_vr::control_trigger;
            const bool released=input.kind==SmokeInputKind::right_trigger_release||input.kind==SmokeInputKind::right_grip_release;
            // A later release/repress in the same ring cannot borrow an old
            // edge's pose. Exact producer counters remain the current ones.
            const bool same_edge_cycle=!history_supplied||
                (right.pressed[control]==f.hands.hands[1].pressed[control]&&
                 right.released[control]==f.hands.hands[1].released[control]&&
                 (released?right.released[control]:right.pressed[control])!=0);
            const bool trigger_valid=(input.kind!=SmokeInputKind::right_trigger_press&&
                input.kind!=SmokeInputKind::right_trigger_release)||
                (right.trigger_active==1&&
                 (input.kind==SmokeInputKind::right_trigger_press?right.trigger>=.65f:right.trigger<=.35f));
            const bool grip_valid=!grip_input||!grip_release()||
                (right.squeeze_active==1&&(released?right.squeeze<=.35f:right.squeeze>=.65f));
            if(!fresh||!same_edge_cycle||!trigger_valid||!grip_valid){
                // A release from a stale frame, recovery baseline or inactive
                // action cannot use this hold's accumulated velocity history.
                if(input.hand==1&&input.kind==release_kind&&held_serial_)
                    return interrupted(SmokeInterruptReason::edge);
                return {};
            }
            edge=true;
        }
        // Raw producer history determines release pose/speed. The current
        // certified Game frame supplies the body transform and base speed once.
        // Translation alone is not a motion-generation boundary.
        const double pitch_delta=motion_epoch_?std::remainder(
            f.mapping.game_base.rotation.pitch-history_mapping_.game_base.rotation.pitch,
            2.0*std::acos(-1.0)):0.0;
        const bool pitch_changed=motion_epoch_&&pitch_delta!=0.0;
        const bool roll_changed=motion_epoch_&&
            f.mapping.game_base.rotation.roll!=history_mapping_.game_base.rotation.roll;
        const double yaw_delta=motion_epoch_?std::remainder(
            f.mapping.game_base.rotation.yaw-history_mapping_.game_base.rotation.yaw,
            2.0*std::acos(-1.0)):0.0;
        const bool yaw_changed=motion_epoch_&&yaw_delta!=0.0;
        last_rotation_observation_.valid=motion_epoch_!=0;
        last_rotation_observation_.history_supplied=history_supplied;
        last_rotation_observation_.history_current=motion_current!=nullptr;
        last_rotation_observation_.history_count=f.motion_history.count;
        last_rotation_observation_.consumer_gap_ns=std::uint64_t(gap>0?gap:0);
        last_rotation_observation_.old_pitch=history_mapping_.game_base.rotation.pitch;
        last_rotation_observation_.old_yaw=history_mapping_.game_base.rotation.yaw;
        last_rotation_observation_.old_roll=history_mapping_.game_base.rotation.roll;
        last_rotation_observation_.new_pitch=f.mapping.game_base.rotation.pitch;
        last_rotation_observation_.new_yaw=f.mapping.game_base.rotation.yaw;
        last_rotation_observation_.new_roll=f.mapping.game_base.rotation.roll;
        last_rotation_observation_.yaw_delta=yaw_delta;
        last_rotation_observation_.pitch_delta=pitch_delta;
        bool yaw_stick_intent=false,locomotion_intent=false;
        if(grip_release()&&history_supplied&&motion_current){
            for(unsigned i=0;i<f.motion_history.count;++i){
                const auto& left=f.motion_history.frames[i].input.hands[0];
                if(left.stick_active==1&&std::isfinite(left.stick_x)&&std::isfinite(left.stick_y)&&
                   std::hypot(left.stick_x,left.stick_y)>=.2f)locomotion_intent=true;
                const auto& right=f.motion_history.frames[i].input.hands[1];
                if(right.stick_active==1&&std::isfinite(right.stick_x)&&std::abs(right.stick_x)>=.2f)yaw_stick_intent=true;
            }
        }
        const double rotation_magnitude=motion_epoch_?std::hypot(pitch_delta,yaw_delta):0.0;
        const bool pitch_continuous=grip_release()&&held_serial_&&history_supplied&&motion_current&&
            pitch_changed&&!roll_changed&&std::isfinite(pitch_delta)&&std::isfinite(yaw_delta)&&
            std::isfinite(rotation_magnitude)&&gap>0&&(locomotion_intent||yaw_stick_intent)&&
            rotation_magnitude<=.2&&rotation_magnitude<=8.0*double(gap)/1e9;
        const bool small_continuous_yaw=grip_release()&&held_serial_&&history_supplied&&motion_current&&
            (!pitch_changed||pitch_continuous)&&!roll_changed&&yaw_changed&&std::isfinite(yaw_delta)&&
            gap>0&&std::abs(yaw_delta)<=.2&&std::abs(yaw_delta)<=8.0*double(gap)/1e9&&yaw_stick_intent;
        const bool current_stick_intent=motion_current&&
            motion_current->input.hands[1].stick_active==1&&
            std::isfinite(motion_current->input.hands[1].stick_x)&&
            std::abs(motion_current->input.hands[1].stick_x)>=.2f;
        const bool episode_before=yaw_episode_;
        const bool bounded_turn=grip_release()&&held_serial_&&history_supplied&&motion_current&&
            (!pitch_changed||pitch_continuous)&&!roll_changed&&yaw_changed&&std::isfinite(yaw_delta)&&
            gap>0&&std::abs(yaw_delta)<=.2&&std::abs(yaw_delta)<=8.0*double(gap)/1e9;
        const bool episode_continuation=episode_before&&held_serial_&&bounded_turn;
        if(small_continuous_yaw&&held_serial_&&current_stick_intent)yaw_episode_=true;
        const bool accepted_yaw=small_continuous_yaw||episode_continuation;
        last_rotation_observation_.pitch_changed=pitch_changed;
        last_rotation_observation_.roll_changed=roll_changed;
        last_rotation_observation_.yaw_changed=yaw_changed;
        last_rotation_observation_.stick_intent=yaw_stick_intent;
        last_rotation_observation_.locomotion_intent=locomotion_intent;
        last_rotation_observation_.continuous=small_continuous_yaw;
        last_rotation_observation_.pitch_continuous=pitch_continuous;
        last_rotation_observation_.rotation_magnitude=rotation_magnitude;
        last_rotation_observation_.current_stick_intent=current_stick_intent;
        last_rotation_observation_.episode_before=episode_before;
        last_rotation_observation_.episode_after=yaw_episode_;
        last_rotation_observation_.episode_continuation=episode_continuation;
        const bool neutral_stable=episode_before&&!yaw_changed&&!current_stick_intent;
        last_rotation_observation_.episode_reset=neutral_stable;
        const bool rotation_changed=roll_changed||(pitch_changed&&!pitch_continuous)||(yaw_changed&&!accepted_yaw);
        const bool boundary=motion_epoch_&&(f.stamp.motion_epoch!=motion_epoch_||
            f.mapping.reference_generation!=reference_generation_||
            !same_pose(f.mapping.frozen_reference,history_mapping_.frozen_reference)||
            f.mapping.units_per_metre!=history_mapping_.units_per_metre||
            rotation_changed||
            (gap>config_.motion.maximum_sample_gap_ns&&!history_supplied)||
            (motion_current&&motion_stream_&&(motion_current->stream_epoch!=motion_stream_||motion_current->tracking_epoch[1]!=motion_tracking_)));
        // Existing mask meanings are preserved. Bit16 remains reserved and is
        // never reused; bit32 means an unaccepted rotation, not any rotation.
        last_boundary_mask_=!motion_epoch_?0u:
            (f.stamp.motion_epoch!=motion_epoch_?1u:0u)|
            (f.mapping.reference_generation!=reference_generation_?2u:0u)|
            (!same_pose(f.mapping.frozen_reference,history_mapping_.frozen_reference)?4u:0u)|
            (f.mapping.units_per_metre!=history_mapping_.units_per_metre?8u:0u)|
            (rotation_changed?32u:0u)|
            (motion_current&&motion_stream_&&motion_current->stream_epoch!=motion_stream_?64u:0u)|
            (motion_current&&motion_tracking_&&motion_current->tracking_epoch[1]!=motion_tracking_?128u:0u)|
            (gap>config_.motion.maximum_sample_gap_ns&&!history_supplied?256u:0u);
        if(neutral_stable||boundary)yaw_episode_=false;
        last_rotation_observation_.episode_after=yaw_episode_;
        motion_epoch_=f.stamp.motion_epoch;reference_generation_=f.mapping.reference_generation;
        history_mapping_=f.mapping;
        if(motion_current){history_mode_=true;motion_stream_=motion_current->stream_epoch;motion_tracking_=motion_current->tracking_epoch[1];}
        if(grip_release()&&held_serial_){const auto& right=f.hands.hands[1];
            const bool same_press=right.pressed[gears_vr::control_squeeze]==grip_pressed_;
            const bool same_release=right.released[gears_vr::control_squeeze]==grip_released_;
            const bool deliberate=edge&&input.kind==release_kind&&grip_released_!=UINT32_MAX&&right.released[gears_vr::control_squeeze]==grip_released_+1;
            if(!same_press||(!same_release&&!deliberate))return interrupted(SmokeInterruptReason::grip_cycle);}
        if(boundary){
            if(stage_==SmokeStage::held||stage_==SmokeStage::idle){
                tracker_.reset();return {SmokeEvent::motion_reset};
            }
            return interrupted(SmokeInterruptReason::mapping_boundary);
        }
        if(request_.token){
            // Releasing while native preparation is unresolved is cancellation,
            // never a buffered throw when a delayed receipt arrives.
            if(stage_==SmokeStage::prepare_pending&&edge&&
               (input.kind==release_kind||
                (input.kind==SmokeInputKind::right_grip_press&&input.slot==BodySlot::grenade)))
                return interrupted(SmokeInterruptReason::pending_release);
            if(ns_>request_.valid_until_ns)return interrupted(SmokeInterruptReason::pending_timeout);
            return {SmokeEvent::observed};
        }
        if(stage_==SmokeStage::idle){
            if(!edge||input.kind!=SmokeInputKind::right_grip_press||input.slot!=BodySlot::grenade)return {};
            if(grip_release()&&equipment_hold_generation_&&
               (f.equipment_hold_generation<=equipment_hold_generation_||
                f.hands.hands[1].pressed[gears_vr::control_squeeze]<=grip_pressed_))return {};
            if(!inventory_.count){
                const auto audio=token();
                if(!audio)return {SmokeEvent::safety_violation};
                return {SmokeEvent::no_ammo,true,audio,config_.original_no_ammo_cue,binding_,config_.identity};
            }
            held_serial_=token();if(!held_serial_)return {SmokeEvent::safety_violation};
            stage_=SmokeStage::held;
            if(grip_release()){
                if(!f.equipment_hold_generation)return {interrupt()};
                return prepare_grip(f);
            }
            return {SmokeEvent::grabbed};
        }
        if(edge&&input.kind==SmokeInputKind::right_grip_press&&input.slot==BodySlot::grenade){
            if(stage_==SmokeStage::held){clear_local_hold();stage_=SmokeStage::idle;return {SmokeEvent::stowed};}
            return {interrupt()};
        }
        if(stage_==SmokeStage::held){
            if(edge&&input.kind==SmokeInputKind::right_trigger_press){
                trigger_press_sequence_=f.hands.sequence;
                if(issue(SmokeAction::prepare_original))return {SmokeEvent::request_ready};
                return {interrupt()};
            }
            return {SmokeEvent::observed};
        }
        if(stage_!=SmokeStage::prepared)return {};
        const bool release=edge&&input.kind==release_kind;
        if(release&&(!trigger_press_sequence_||f.hands.sequence<=trigger_press_sequence_))return interrupted(SmokeInterruptReason::release_sequence);
        const auto solution=history_supplied?
            tracker_.advance_history(f.motion_history,f.hands,f.stereo,config_.identity.weapon,config_.identity.world,
                reference_generation_,ns_,trigger_press_sequence_,release?input.hand_sequence:0,config_.motion,
                config_.input_source==gears_vr::StereoSource::MetaOperatorSimulated,100000000):
            tracker_.advance(f.hands,f.stereo,config_.identity.weapon,config_.identity.world,
                reference_generation_,ns_,release,config_.motion,
                config_.input_source==gears_vr::StereoSource::MetaOperatorSimulated);
        if(!release){
            if(!tracker_.armed())return interrupted(SmokeInterruptReason::tracker);
            return {SmokeEvent::observed};
        }
        GameGrenadeLaunch launch{};
        if(!solution||!grenade_world_launch(*solution,f.mapping.frozen_reference,f.mapping.game_base,
            f.mapping.units_per_metre,launch,f.mapping.pawn_velocity_world_units_per_second))return interrupted(SmokeInterruptReason::solution);
        if(!issue(SmokeAction::launch_original))return interrupted(SmokeInterruptReason::issue);
        request_.tracked=*solution;request_.launch=launch;
        request_.valid_until_ns=(std::min)(request_.valid_until_ns,input.sync_ns+config_.request_lease_ns);
        return {SmokeEvent::request_ready};
    }
    // Reconcile a separately observed ORIGINAL debit only after our unlit
    // preparation actually stopped. This credits neither a VR projectile nor
    // a VR consume. Ordinary receipts retain their exact count/lease rules.
    bool retire_stopped_after_external_debit(SmokeOriginalReceipt const& r) noexcept {
        if(!grip_release()||config_.fuse_owner!=SmokeFuseOwner::original_projectile||
           stage_!=SmokeStage::safety_pending||!request_.token||!dispatched_||
           request_.action!=SmokeAction::preserve_original_on_interrupt||
           r.request_token!=request_.token||!stamp_valid(r.stamp)||
           r.original_event_serial<=last_native_event_||!r.accepted||
           r.config_serial!=request_.config_serial||r.original_entry!=request_.original_entry||
           r.original_ammo_type!=request_.original_ammo_type||r.original_emitter_type!=request_.original_emitter_type||
           !r.original_effect_applied_once||!r.native_safety_complete||
           !r.original_fuse_preserved||!r.original_effects_preserved||
           !r.original_smoke_emitter_path_preserved||!r.original_audio_path_preserved||
           !preparation_serial_||r.original_preparation_serial!=preparation_serial_||
           request_.original_preparation_serial!=preparation_serial_||
           !fuse_.valid()||fuse_.active||!(r.fuse==fuse_)||
           r.original_projectiles_spawned||r.original_projectile||r.original_ammo_consumptions||
           r.tracked_initial_pose_and_velocity_applied||r.native_non_throwing_consequence||
           r.executed_ns<request_.authored_ns||r.executed_ns>r.stamp.monotonic_ns||
           !request_.before.count||!r.inventory.valid()||
           r.inventory.revision<=request_.before.revision||
           r.inventory.count!=request_.before.count-1)return false;
        consume(r.stamp);last_native_event_=r.original_event_serial;
        inventory_=r.inventory;request_={};interrupt_after_receipt_=false;
        clear_local_hold();stage_=SmokeStage::interrupted;return true;
    }
    SmokeEvent acknowledge(SmokeOriginalReceipt const& r) noexcept {
        // Check the token FIRST: a stale callback with a newer transport stamp
        // must not consume the current transaction's sequence or inventory.
        if(!request_.token||!dispatched_||r.request_token!=request_.token||
           !stamp_valid(r.stamp)||r.original_event_serial<=last_native_event_||!r.fuse.valid())
            return SmokeEvent::rejected;
        auto const action=request_.action;
        if(!r.accepted){
            if(r.original_effect_applied_once||r.original_projectiles_spawned||r.original_projectile||
               r.original_ammo_consumptions||r.tracked_initial_pose_and_velocity_applied||
               !count_matches(r.inventory,request_.before.count,request_.before.revision)||
               !(r.fuse==fuse_))return SmokeEvent::safety_violation;
            consume(r.stamp);last_native_event_=r.original_event_serial;inventory_=r.inventory;
            // A rejected safety entry leaves the native obligation unresolved;
            // no new token, rebind or fake successful cancellation is allowed.
            if(action==SmokeAction::preserve_original_on_interrupt){
                stage_=SmokeStage::interrupted;return SmokeEvent::safety_violation;
            }
            request_={};
            return interrupt();
        }
        if(!request_.executable_at(r.executed_ns,r.stamp.identity,r.stamp.binding,
                                  r.executed_motion_epoch,r.executed_reference_generation)||
           r.config_serial!=request_.config_serial||r.original_entry!=request_.original_entry||
           r.original_ammo_type!=request_.original_ammo_type||r.original_emitter_type!=request_.original_emitter_type||
           r.executed_ns>r.stamp.monotonic_ns||
           !r.original_effect_applied_once||!r.original_fuse_preserved||!r.original_effects_preserved||
           !r.original_smoke_emitter_path_preserved||!r.original_audio_path_preserved||
           r.fuse.started_game_tick>r.stamp.game_tick)return SmokeEvent::safety_violation;
        bool ammo_spent=false;
        if(action==SmokeAction::prepare_original){
            if(!r.original_preparation_serial||r.original_preparation_serial<=last_preparation_serial_||
               r.original_projectiles_spawned||r.original_projectile||r.original_ammo_consumptions||
               r.tracked_initial_pose_and_velocity_applied||
               !count_matches(r.inventory,request_.before.count,request_.before.revision)||
               (config_.fuse_owner==SmokeFuseOwner::original_projectile&&r.fuse.active)||
               (config_.fuse_owner==SmokeFuseOwner::original_preparation&&
                   (!r.fuse.active||r.fuse.started_game_tick<request_.authored_game_tick)))
                return SmokeEvent::safety_violation;
        }else{
            if(r.original_preparation_serial!=preparation_serial_||
               (fuse_.active&&!(r.fuse==fuse_)))return SmokeEvent::safety_violation;
            ammo_spent=r.original_projectiles_spawned==1;
            if(action==SmokeAction::launch_original){
                if(!ammo_spent||!r.tracked_initial_pose_and_velocity_applied)return SmokeEvent::safety_violation;
            }else if(action==SmokeAction::preserve_original_on_interrupt){
                // A live original fuse cannot disappear into a free reusable
                // inventory item. Native consequences keep its old fuse and
                // consume exactly one original projectile; no VR throw occurs.
                if(!r.native_safety_complete||r.tracked_initial_pose_and_velocity_applied||
                   (fuse_.active&&!r.native_non_throwing_consequence)||(!fuse_.active&&ammo_spent)||
                   (!ammo_spent&&(fuse_.active||r.fuse.active)))
                    return SmokeEvent::safety_violation;
            }else return SmokeEvent::safety_violation;
            if(ammo_spent){
                if(!request_.before.count||!r.original_projectile||
                   r.original_projectile_type!=config_.original_projectile_type||r.original_ammo_consumptions!=1||
                   !count_matches(r.inventory,request_.before.count-1,request_.before.revision)||
                   r.inventory.revision<=request_.before.revision)return SmokeEvent::safety_violation;
            }else if(r.original_projectiles_spawned||r.original_projectile||r.original_ammo_consumptions||
                     !count_matches(r.inventory,request_.before.count,request_.before.revision))
                return SmokeEvent::safety_violation;
        }
        consume(r.stamp);last_native_event_=r.original_event_serial;inventory_=r.inventory;request_={};
        if(action==SmokeAction::prepare_original){
            preparation_serial_=last_preparation_serial_=r.original_preparation_serial;fuse_=r.fuse;
            if(interrupt_after_receipt_){interrupt_after_receipt_=false;return interrupt();}
            if(!tracker_.begin(config_.identity.weapon,config_.identity.world,1,reference_generation_,
                               config_.centre_in_right_grip_metres))return interrupt();
            stage_=SmokeStage::prepared;return SmokeEvent::prepared;
        }
        const bool interrupted=interrupt_after_receipt_;
        interrupt_after_receipt_=false;clear_local_hold();
        stage_=(action==SmokeAction::preserve_original_on_interrupt||interrupted)?
            SmokeStage::interrupted:SmokeStage::idle;
        return action==SmokeAction::launch_original?SmokeEvent::thrown:SmokeEvent::safety_completed;
    }
};
}
