#pragma once
#include "body_slots.h"
#include "grenade_throw.h"
#include <cstdint>
#include <limits>
#include <optional>

namespace gears_weapon {

// Saved declarations: Weap_FragGrenade.ThrowGrenade -> Weap_Grenade.ThrowGrenade
// spawns the original projectile, consumes ammo only after a successful spawn,
// and assigns a camera/SimulationDir-derived velocity. Proj_Grenade owns bounce,
// fuse and explosion; Proj_FragGrenade.EnableGrenadePhysics owns the chain spring.
// The exports contain unresolved native tokens: they do NOT authenticate retail
// release input, fuse activation, cancellation or a safe launch override.
// Mesh export SHA256 f015fe8af442469bd23f7b24c9cfc58b11b99085abeedc349236b8cbd4f2a7ac;
// analysis/bolo-animation-tracks.json records original GR_Prepare/airborne chains.
inline constexpr char bolo_original_weapon_class[]="Weap_FragGrenade";
inline constexpr char bolo_original_projectile_class[]="Proj_FragGrenade";

struct BoloIdentity {
    std::uint64_t world{},pawn{},controller{},weapon{},world_generation{},
                  xr_session{},space_epoch{},input_epoch{},profile_generation{};
    bool valid() const noexcept {
        return world&&pawn&&controller&&weapon&&world_generation&&xr_session&&
            space_epoch&&input_epoch&&profile_generation;
    }
    bool operator==(BoloIdentity const& b) const noexcept {
        return world==b.world&&pawn==b.pawn&&controller==b.controller&&weapon==b.weapon&&
            world_generation==b.world_generation&&xr_session==b.xr_session&&
            space_epoch==b.space_epoch&&input_epoch==b.input_epoch&&
            profile_generation==b.profile_generation;
    }
};
struct BoloStamp {
    BoloIdentity identity{};
    std::uint64_t binding{},sequence{},motion_epoch{};
    std::int64_t monotonic_ns{},game_tick{};
};
struct BoloInventory {
    std::uint64_t revision{};
    std::uint32_t count{};
    bool count_known{};
    bool valid() const noexcept {return revision&&count_known&&count<=10000;}
};
enum class BoloReleaseInput : std::uint8_t { unknown,right_trigger_release_after_press,right_grip_release_after_hold };
// No local timer or cook duration is invented. In the first mode the original
// projectile decides when its fuse starts, including collision-dependent rules.
enum class BoloFuseOwner : std::uint8_t { unknown,original_projectile,original_preparation };
struct BoloFuse {
    std::uint64_t original_serial{};
    std::int64_t started_game_tick{};
    bool active{};
    bool valid() const noexcept {
        return active?(original_serial&&started_game_tick>=0):
            (!original_serial&&started_game_tick==0);
    }
    bool operator==(BoloFuse const& b) const noexcept {
        return original_serial==b.original_serial&&started_game_tick==b.started_game_tick&&active==b.active;
    }
};
struct BoloInstanceConfig {
    BoloIdentity identity{};
    // One frozen provenance profile. A private simulator binding never
    // accepts physical packets, and production never silently accepts sim.
    gears_vr::StereoSource input_source{gears_vr::StereoSource::RuntimeTracked};
    std::uint64_t config_serial{},original_projectile_type{},grip_geometry_serial{},
                  release_rule_serial{},fuse_rule_serial{},prepare_entry{},launch_entry{},safety_entry{},
                  original_no_ammo_cue{},audio_rule_serial{};
    BoloReleaseInput release_input{BoloReleaseInput::unknown};
    BoloFuseOwner fuse_owner{BoloFuseOwner::unknown};
    Vector centre_in_right_grip_metres{};
    GrenadeThrowConfig motion{};
    std::int64_t request_lease_ns{100000000}; // VR lease, never an original fuse duration.
    bool current_instance_verified{},original_frag_classes_verified{},
         grip_geometry_verified{},release_input_verified{},fuse_rule_verified{},
         original_prepare_no_ammo_change_verified{},original_launch_once_verified{},
         original_initial_pose_velocity_override_verified{},original_effects_preserved_verified{},
         non_throwing_safety_entry_verified{},original_gravity_and_units_verified{},original_no_ammo_cue_verified{};
    bool valid() const noexcept {
        return identity.valid()&&gears_vr::stereo_source_known(input_source)&&config_serial&&original_projectile_type&&grip_geometry_serial&&
            release_rule_serial&&fuse_rule_serial&&prepare_entry&&launch_entry&&safety_entry&&
            original_no_ammo_cue&&audio_rule_serial&&
            (release_input==BoloReleaseInput::right_trigger_release_after_press||
             release_input==BoloReleaseInput::right_grip_release_after_hold)&&
            (fuse_owner==BoloFuseOwner::original_projectile||
             fuse_owner==BoloFuseOwner::original_preparation)&&
            gears_vr::finite(centre_in_right_grip_metres)&&length(centre_in_right_grip_metres)<=.3&&
            motion.valid()&&request_lease_ns>0&&request_lease_ns<=1000000000&&
            current_instance_verified&&original_frag_classes_verified&&grip_geometry_verified&&
            release_input_verified&&fuse_rule_verified&&original_prepare_no_ammo_change_verified&&
            original_launch_once_verified&&original_initial_pose_velocity_override_verified&&
            original_effects_preserved_verified&&non_throwing_safety_entry_verified&&
            original_gravity_and_units_verified&&original_no_ammo_cue_verified;
    }
};
struct BoloWorldMapping {
    std::uint64_t motion_epoch{},reference_generation{};
    Pose frozen_reference{};
    gears_vr::Viewpoint game_base{};
    Vector pawn_velocity_world_units_per_second{};
    double units_per_metre{};
    bool current_instance_verified{};
};
struct BoloFrame {
    BoloStamp stamp{};
    BoloInventory inventory{};
    gears_vr::ControllerInputFrame hands{};
    gears_vr::StereoSample stereo{};
    // Optional exact raw-producer ring, copied with the current publisher
    // bundle. These historical poses are never rebuilt as current Frames.
    gears_hand_continuity::MotionHistory motion_history{};
    BoloWorldMapping mapping{};
    std::uint64_t right_pocket_weapon{};
    // Current receipt of the existing equipment owner, not a new Grip edge.
    // The live adapter must authenticate the actual held binding/weapon and
    // its generation against this same fresh frame before supplying it.
    std::uint64_t equipment_hold_generation{};
    // EquipmentModel must have confirmed the original equip before taking a
    // local visual reservation. This module never clones an inventory actor.
    bool original_equipped{},original_interaction_allowed{},original_owns_hands{};
};
// Values already evaluated by the current update. Passive observation only;
// this is not an action grant or a second motion/source read.
struct BoloRotationObservation {
    bool valid{},history_supplied{},history_current{};
    bool pitch_changed{},roll_changed{},yaw_changed{},stick_intent{},continuous{};
    bool episode_before{},episode_after{},episode_continuation{},episode_reset{};
    bool current_stick_intent{};
    bool locomotion_intent{},pitch_continuous{};
    std::uint64_t history_count{},consumer_gap_ns{};
    double old_pitch{},old_yaw{},old_roll{},new_pitch{},new_yaw{},new_roll{},yaw_delta{};
    double pitch_delta{},rotation_magnitude{};
};
enum class BoloInputKind : std::uint8_t { none,right_grip_press,right_trigger_press,right_trigger_release,right_grip_release };
struct BoloInput {
    BoloInputKind kind{BoloInputKind::none};
    std::uint64_t edge_serial{}; // One increasing serial across all right-hand edges.
    std::uint64_t hand_sequence{},input_epoch{};
    std::int64_t sync_ns{}; // Exact edge observation frame, never XR predicted pose time.
    unsigned hand{1};
    BodySlot slot{BodySlot::none};
    bool continuous_input{}; // Recovery/baseline edges are never actions.
};
enum class BoloTriggerObservation : std::uint8_t {baseline,duplicate,quiet,edge,invalid,ambiguous,exhausted};
// One normal-GT input observer, separate from the equipment Grip owner. The
// exact authenticated ring supplies the original trigger-edge pose/time.
// This classifier neither constructs a Frame nor prepares/spawns anything.
// Optional passive refusal context. Captured before clear()/advancing the
// previous row; it cannot supply an input edge or change observer admission.
struct BoloTriggerDiagnostic {
    std::uint64_t sequence{},stream{},tracking{};
    std::int64_t sync_ns{};
    std::uint32_t pressed{},released{},active{};
    float level{};
    unsigned reason{};
    bool known{},cohort{};
};
class BoloTriggerObserver {
    BoloIdentity identity_{};std::uint64_t binding_{},hold_{},serial_{};
    gears_hand_continuity::TrackedInput previous_{};bool known_{};
public:
    void clear() noexcept {identity_={};binding_=hold_=0;previous_={};known_=false;}
    BoloTriggerObservation observe(BoloFrame const& f,BoloInput& out,
        BoloReleaseInput rule=BoloReleaseInput::right_trigger_release_after_press,
        BoloTriggerDiagnostic* diagnostic=nullptr) noexcept {
        out={};
        const bool grip=rule==BoloReleaseInput::right_grip_release_after_hold;
        const auto control=grip?gears_vr::control_squeeze:gears_vr::control_trigger;
        if(diagnostic){
            *diagnostic={};diagnostic->known=known_;
            diagnostic->sequence=previous_.input.sequence;diagnostic->sync_ns=previous_.input.sync_ns;
            diagnostic->stream=previous_.stream_epoch;diagnostic->tracking=previous_.tracking_epoch[1];
            const auto& h=previous_.input.hands[1];
            diagnostic->pressed=h.pressed[control];diagnostic->released=h.released[control];
            diagnostic->active=grip?h.squeeze_active:h.trigger_active;diagnostic->level=grip?h.squeeze:h.trigger;
        }
        const auto refused=[&](BoloTriggerObservation result,unsigned reason,bool reset) noexcept {
            if(diagnostic)diagnostic->reason=reason;
            if(reset)clear();
            return result;
        };
        if(!f.stamp.identity.valid()||!f.stamp.binding||!f.equipment_hold_generation||
           !f.motion_history.count||f.motion_history.count>gears_hand_continuity::motion_history_capacity)
            return refused(BoloTriggerObservation::invalid,1,true);
        const auto& current=f.motion_history.frames[f.motion_history.count-1];
        if(!gears_hand_continuity::same_input(current.input,f.hands))return refused(BoloTriggerObservation::invalid,2,true);
        if(!gears_hand_continuity::motion_history_valid(f.motion_history,current,1))return refused(BoloTriggerObservation::invalid,3,true);
        if(f.stamp.monotonic_ns<current.input.sync_ns)return refused(BoloTriggerObservation::invalid,4,true);
        const bool cohort=known_&&identity_==f.stamp.identity&&binding_==f.stamp.binding&&hold_==f.equipment_hold_generation&&
            previous_.stream_epoch==current.stream_epoch&&previous_.tracking_epoch[1]==current.tracking_epoch[1]&&
            gears_hand_continuity::detail::same_key(previous_.input,current.input);
        if(diagnostic)diagnostic->cohort=cohort;
        if(cohort&&gears_hand_continuity::same_tracking(previous_,current))return BoloTriggerObservation::duplicate;
        const auto prior=previous_;identity_=f.stamp.identity;binding_=f.stamp.binding;hold_=f.equipment_hold_generation;
        previous_=current;known_=true;
        if(!cohort)return BoloTriggerObservation::baseline;
        if(current.input.sequence<=prior.input.sequence||current.input.sync_ns<=prior.input.sync_ns)return refused(BoloTriggerObservation::invalid,5,true);
        const auto& old=prior.input.hands[1];const auto& tip=current.input.hands[1];
        if((grip?tip.squeeze_active:tip.trigger_active)!=1||
           (grip?old.squeeze_active:old.trigger_active)!=1)return refused(BoloTriggerObservation::invalid,6,true);
        if(tip.pressed[control]<old.pressed[control]||tip.released[control]<old.released[control])return refused(BoloTriggerObservation::invalid,7,true);
        if(tip.pressed[control]==old.pressed[control]&&tip.released[control]==old.released[control])return BoloTriggerObservation::quiet;
        const bool press=old.pressed[control]!=UINT32_MAX&&tip.pressed[control]==old.pressed[control]+1&&tip.released[control]==old.released[control];
        const bool release=old.released[control]!=UINT32_MAX&&tip.released[control]==old.released[control]+1&&tip.pressed[control]==old.pressed[control];
        if(press==release)return refused(BoloTriggerObservation::ambiguous,8,false);
        for(unsigned i=0;i<f.motion_history.count;++i){const auto& row=f.motion_history.frames[i];const auto& h=row.input.hands[1];
            if(row.input.sequence<=prior.input.sequence)continue;
            const auto* before=i?&f.motion_history.frames[i-1]:&prior;
            const auto level=grip?h.squeeze:h.trigger;
            if(!gears_hand_continuity::motion_pair(*before,row,1,100000000)||(grip?h.squeeze_active:h.trigger_active)!=1||
               before->input.hands[1].pressed[control]!=old.pressed[control]||
               before->input.hands[1].released[control]!=old.released[control]||
               h.pressed[control]!=tip.pressed[control]||h.released[control]!=tip.released[control]||
               (press?level<.65f:level>.35f))continue;
            if(serial_==UINT64_MAX)return refused(BoloTriggerObservation::exhausted,9,false);
            out.kind=grip?(press?BoloInputKind::right_grip_press:BoloInputKind::right_grip_release):
                (press?BoloInputKind::right_trigger_press:BoloInputKind::right_trigger_release);
            out.edge_serial=++serial_;out.hand_sequence=row.input.sequence;out.input_epoch=row.input.input_epoch;
            out.sync_ns=row.input.sync_ns;out.continuous_input=true;
            return BoloTriggerObservation::edge;
        }
        // Ring overwritten, skipped edge, multiple cycles or boundary: never
        // invent the missing edge at the current hand position/poll clock.
        return refused(BoloTriggerObservation::ambiguous,10,false);
    }
};
enum class BoloAction : std::uint8_t { none,prepare_original,launch_original,preserve_original_on_interrupt };
enum class BoloStage : std::uint8_t { unbound,idle,held,prepare_pending,prepared,launch_pending,safety_pending,interrupted };
enum class BoloEvent : std::uint8_t { rejected,observed,no_ammo,grabbed,stowed,
    request_ready,prepared,thrown,motion_reset,interrupted,safety_required,safety_completed,safety_violation };
struct BoloDecision {
    BoloEvent event{BoloEvent::rejected};
    bool request_original_no_ammo_sound{}; // Intent only; adapter plays original audio once.
    std::uint64_t audio_token{},original_cue{},binding{};
    BoloIdentity identity{};
    // Dispatcher deduplicates audio_token and rechecks the current identity.
    // The cue is supplied by current-instance certification, never guessed here.
    bool audio_for(BoloIdentity const& current,std::uint64_t current_binding) const noexcept {
        return request_original_no_ammo_sound&&event==BoloEvent::no_ammo&&audio_token&&original_cue&&
            binding==current_binding&&identity.valid()&&identity==current;
    }
};
struct BoloRequest {
    BoloIdentity identity{};
    std::uint64_t binding{},config_serial{},token{},held_serial{},original_preparation_serial{},
                  original_entry{},original_projectile_type{},motion_epoch{},reference_generation{};
    BoloAction action{BoloAction::none};
    BoloInventory before{};
    std::uint32_t expected_count{};
    BoloFuse fuse{};
    std::int64_t authored_ns{},valid_until_ns{},authored_game_tick{};
    std::int64_t original_input_sync_ns{};
    GrenadeThrowSolution tracked{};
    GameGrenadeLaunch launch{};
    bool executable_at(std::int64_t now,BoloIdentity const& current,std::uint64_t current_binding,
                       std::uint64_t current_motion_epoch,std::uint64_t current_reference_generation) const noexcept {
        return identity==current&&binding==current_binding&&token&&
            now>=authored_ns&&now<=valid_until_ns&&
            (action==BoloAction::preserve_original_on_interrupt||
             (motion_epoch==current_motion_epoch&&reference_generation==current_reference_generation));
    }
};
// Deliver real original receipts before an ordinary inventory snapshot carrying
// their delta. Native event serials are increasing within the game-thread owner.
struct BoloOriginalReceipt {
    BoloStamp stamp{};
    std::uint64_t request_token{},original_event_serial{},original_preparation_serial{},
                  original_projectile{},original_projectile_type{};
    std::int64_t executed_ns{};
    BoloInventory inventory{};
    BoloFuse fuse{};
    std::uint32_t original_projectiles_spawned{};
    bool accepted{},original_effect_applied_once{},original_fuse_preserved{},
         original_effects_preserved{},tracked_initial_pose_and_velocity_applied{},native_safety_complete{};
};

// One noncopyable game-thread owner. Requests are dispatched once and must be
// executed idempotently by token. A timeout/cancel NEVER retries an ambiguous
// native operation. Only an original receipt changes the ammo mirror.
// Launch changes only the original grenade's initial pose/velocity; adapter
// retains its class, chain skinning/animation, bounce, fuse, damage and effects,
// suppresses the original preview/velocity assignment for this one launch, and
// performs spawn + one ConsumeAmmo as one authenticated original operation.
// This header contains no game hook, projectile simulation or custom explosion.
class BoloGrenadeInteraction {
    GrenadeThrowObservation last_throw_observation_{};
    BoloRotationObservation last_rotation_observation_{};
    std::uint32_t last_boundary_flags_{}; // Bits 1..32 retained; 64 retired, 128 rotation.
    std::uint32_t last_update_refusal_{}; // Passive classification of the actual last update only.
    BoloInstanceConfig config_{};
    BoloInventory inventory_{};
    BoloStage stage_{BoloStage::unbound};
    BoloRequest request_{};
    BoloFuse fuse_{};
    GrenadeThrowTracker tracker_{};
    Pose history_reference_{};
    BoloWorldMapping history_mapping_{};
    std::uint64_t next_token_{1},binding_{},sequence_{},last_edge_{},last_native_event_{},
                  held_serial_{},preparation_serial_{},last_preparation_serial_{},
                  motion_epoch_{},reference_generation_{},last_hand_sequence_{},
                  trigger_press_sequence_{},trigger_press_hold_{},equipment_hold_generation_{};
    std::uint64_t motion_stream_{},motion_tracking_{};
    std::uint32_t trigger_pressed_counter_{},trigger_released_counter_{};
    std::int64_t ns_{},game_tick_{},last_hand_predicted_time_{},last_hand_sync_ns_{},trigger_press_sync_ns_{};
    double history_units_per_metre_{};
    bool dispatched_{},interrupt_after_receipt_{},history_mode_{},yaw_episode_{};
    bool grip_release() const noexcept {return config_.release_input==BoloReleaseInput::right_grip_release_after_hold;}
    auto hold_control() const noexcept {return grip_release()?gears_vr::control_squeeze:gears_vr::control_trigger;}
    BoloInputKind press_kind() const noexcept {return grip_release()?BoloInputKind::right_grip_press:BoloInputKind::right_trigger_press;}
    BoloInputKind release_kind() const noexcept {return grip_release()?BoloInputKind::right_grip_release:BoloInputKind::right_trigger_release;}
    BoloDecision prepare_hold(BoloFrame const& f,std::uint64_t sequence,std::int64_t sync) noexcept {
        trigger_press_sequence_=sequence;trigger_press_hold_=held_serial_;trigger_press_sync_ns_=sync;
        trigger_pressed_counter_=f.hands.hands[1].pressed[hold_control()];
        trigger_released_counter_=f.hands.hands[1].released[hold_control()];
        if(!issue(BoloAction::prepare_original))return {interrupt()};
        request_.original_input_sync_ns=sync;
        request_.valid_until_ns=(std::min)(request_.valid_until_ns,sync+config_.request_lease_ns);
        return {BoloEvent::request_ready};
    }

    std::uint64_t token() noexcept {
        if(next_token_==(std::numeric_limits<std::uint64_t>::max)())return 0;
        return next_token_++;
    }
    bool stamp_valid(BoloStamp const& s) const noexcept {
        return s.identity==config_.identity&&s.binding==binding_&&s.sequence>sequence_&&
            s.motion_epoch&&s.monotonic_ns>ns_&&s.game_tick>=game_tick_;
    }
    void consume(BoloStamp const& s) noexcept {
        sequence_=s.sequence;ns_=s.monotonic_ns;game_tick_=s.game_tick;
    }
    bool count_matches(BoloInventory const& i,std::uint32_t count,std::uint64_t revision) const noexcept {
        return i.valid()&&i.count==count&&i.revision>=revision;
    }
    static bool same_pose(Pose const& a,Pose const& b) noexcept {
        return a.position.x==b.position.x&&a.position.y==b.position.y&&a.position.z==b.position.z&&
            a.orientation.x==b.orientation.x&&a.orientation.y==b.orientation.y&&
            a.orientation.z==b.orientation.z&&a.orientation.w==b.orientation.w;
    }
    void clear_local_hold() noexcept {
        tracker_.reset();held_serial_=preparation_serial_=trigger_press_sequence_=trigger_press_hold_=0;
        trigger_press_sync_ns_=0;trigger_pressed_counter_=trigger_released_counter_=0;equipment_hold_generation_=0;fuse_={};
        yaw_episode_=false;
    }
    bool issue(BoloAction action) noexcept {
        const auto id=token();
        if(!id||ns_>(std::numeric_limits<std::int64_t>::max)()-config_.request_lease_ns)return false;
        request_={};request_.identity=config_.identity;request_.binding=binding_;
        request_.config_serial=config_.config_serial;request_.token=id;
        request_.held_serial=held_serial_;request_.original_preparation_serial=preparation_serial_;
        request_.original_projectile_type=config_.original_projectile_type;request_.action=action;
        request_.motion_epoch=motion_epoch_;request_.reference_generation=reference_generation_;
        request_.before=inventory_;request_.expected_count=inventory_.count;request_.fuse=fuse_;
        request_.authored_ns=ns_;request_.valid_until_ns=ns_+config_.request_lease_ns;
        request_.authored_game_tick=game_tick_;dispatched_=false;
        switch(action){
            case BoloAction::prepare_original:request_.original_entry=config_.prepare_entry;
                stage_=BoloStage::prepare_pending;break;
            case BoloAction::launch_original:request_.original_entry=config_.launch_entry;
                request_.expected_count=inventory_.count-1;stage_=BoloStage::launch_pending;break;
            case BoloAction::preserve_original_on_interrupt:request_.original_entry=config_.safety_entry;
                stage_=BoloStage::safety_pending;break;
            default:return false;
        }
        return true;
    }
    BoloEvent interrupt() noexcept {
        yaw_episode_=false;
        last_rotation_observation_.episode_after=false;
        tracker_.reset();trigger_press_sequence_=trigger_press_hold_=0;trigger_press_sync_ns_=0;
        if(request_.token){
            if(dispatched_){interrupt_after_receipt_=true;return BoloEvent::interrupted;}
            if(request_.action==BoloAction::preserve_original_on_interrupt)return BoloEvent::safety_required;
            // No adapter has taken this request: native execution is impossible.
            // Withdraw it before issuing safety, never dispatch an old throw.
            request_={};
        }
        if(preparation_serial_){
            if(issue(BoloAction::preserve_original_on_interrupt))return BoloEvent::safety_required;
            stage_=BoloStage::interrupted;return BoloEvent::safety_violation;
        }
        clear_local_hold();stage_=BoloStage::interrupted;return BoloEvent::interrupted;
    }
    bool tracked_cohort(BoloFrame const& f) const noexcept {
        auto const& h=f.hands;
        return h.session==config_.identity.xr_session&&h.space_epoch==config_.identity.space_epoch&&
            h.input_epoch==config_.identity.input_epoch&&h.profile_generation==config_.identity.profile_generation&&
            h.source==config_.input_source&&f.stereo.source==config_.input_source&&
            gears_vr::controller_frame_matches(h,f.stereo,f.stamp.monotonic_ns,
                config_.input_source==gears_vr::StereoSource::MetaOperatorSimulated)&&
            gears_vr::controller_pose_usable(h.hands[1].grip,h.hands[1].grip_flags,h.hands[1].grip_active);
    }
    bool tracked(BoloFrame const& f) const noexcept {
        return f.hands.sync_ns<=f.stamp.monotonic_ns&&
            f.stamp.monotonic_ns-f.hands.sync_ns<=config_.motion.maximum_sample_gap_ns&&tracked_cohort(f);
    }
    bool mapping_valid(BoloFrame const& f) const noexcept {
        auto const& m=f.mapping;
        return m.current_instance_verified&&m.motion_epoch==f.stamp.motion_epoch&&m.reference_generation&&
            gears_vr::stereo_pose_valid(m.frozen_reference)&&gears_vr::finite(m.game_base.position)&&
            std::isfinite(m.game_base.rotation.pitch)&&std::isfinite(m.game_base.rotation.yaw)&&
            std::isfinite(m.game_base.rotation.roll)&&gears_vr::finite(m.pawn_velocity_world_units_per_second)&&
            std::isfinite(m.units_per_metre)&&m.units_per_metre>=10&&m.units_per_metre<=500;
    }
public:
    BoloGrenadeInteraction()=default;
    BoloGrenadeInteraction(BoloGrenadeInteraction const&)=delete;
    BoloGrenadeInteraction& operator=(BoloGrenadeInteraction const&)=delete;
    bool bind(BoloInstanceConfig const& config,BoloInventory const& inventory) noexcept {
        if(request_.token||held_serial_||preparation_serial_||!config.valid()||!inventory.valid())return false;
        const auto b=token();if(!b)return false;
        config_=config;inventory_=inventory;binding_=b;stage_=BoloStage::idle;
        sequence_=last_edge_=last_native_event_=last_preparation_serial_=motion_epoch_=
            reference_generation_=last_hand_sequence_=trigger_press_sequence_=trigger_press_hold_=0;
        ns_=game_tick_=last_hand_predicted_time_=last_hand_sync_ns_=trigger_press_sync_ns_=0;
        equipment_hold_generation_=0;
        motion_stream_=motion_tracking_=0;history_mode_=false;yaw_episode_=false;last_boundary_flags_=0;
        fuse_={};tracker_.reset();request_={};history_mapping_={};
        dispatched_=interrupt_after_receipt_=false;return true;
    }
    std::uint64_t binding_token() const noexcept {return binding_;}
    BoloStage stage() const noexcept {return stage_;}
    BoloInventory inventory() const noexcept {return inventory_;}
    BoloFuse original_fuse() const noexcept {return fuse_;}
    unsigned reserved_count() const noexcept {return held_serial_?1u:0u;}
    bool suppress_original_trajectory() const noexcept {return held_serial_!=0;}
    BoloRequest const& pending_request() const noexcept {return request_;}
    // Equip animation can finish after the physical pocket Grip has already
    // been consumed by EquipmentModel. Adopt its current confirmed hold; do
    // not require/replay that old press or create a second equipment arbiter.
    // Grip mode prepares only while the confirmed equipment hold is physically
    // held in this exact fresh frame. A release during equip never throws.
    // Launch still needs its own subsequent producer release edge and receipt.
    BoloDecision adopt_equipment_hold(BoloFrame const& f) noexcept {
        if(stage_!=BoloStage::idle||!f.equipment_hold_generation||!inventory_.count||
           request_.token||held_serial_||!stamp_valid(f.stamp))return {};
        const auto& right=f.hands.hands[1];
        if(grip_release()&&(right.squeeze_active!=1||right.squeeze<=.35f||
           !right.pressed[gears_vr::control_squeeze]))return {};
        // Before any hold/preparation/token exists, a current certified
        // equipment hold may wait for fresh input. This grants nothing from
        // the delayed cohort and never resumes an interrupted preparation.
        // The next adoption still validates its own fresh input and live Grip.
        if(grip_release()&&f.hands.sync_ns<=f.stamp.monotonic_ns&&
           f.stamp.monotonic_ns-f.hands.sync_ns>config_.motion.maximum_sample_gap_ns&&
           f.stamp.monotonic_ns-f.hands.sync_ns<=100000000&&
           tracked_cohort(f)&&mapping_valid(f)&&
           count_matches(f.inventory,inventory_.count,inventory_.revision)&&
           f.right_pocket_weapon==config_.identity.weapon&&f.original_equipped&&
           f.original_interaction_allowed&&!f.original_owns_hands)return {};
        const auto observed=update(f);
        // Preserve the rejection produced while validating this first hold.
        // The live owner must retire an interrupted adoption immediately.
        if(stage_!=BoloStage::idle)return observed;
        if(sequence_!=f.stamp.sequence||ns_!=f.stamp.monotonic_ns)return {};
        held_serial_=token();if(!held_serial_)return {BoloEvent::safety_violation};
        equipment_hold_generation_=f.equipment_hold_generation;
        stage_=BoloStage::held;
        if(grip_release())return prepare_hold(f,f.hands.sequence,f.hands.sync_ns);
        return {BoloEvent::grabbed};
    }
    std::optional<BoloRequest> take_request() noexcept {
        if(!request_.token||dispatched_)return std::nullopt;
        dispatched_=true;return request_;
    }
    // Includes switching away, pause, death or surrendering hands to animation.
    // Adapter must still reconcile an already dispatched request in its OLD
    // identity; it must never execute it on the new weapon/world instance.
    BoloEvent cancel() noexcept {
        if(stage_==BoloStage::unbound)return BoloEvent::rejected;
        return interrupt();
    }
    GrenadeThrowObservation const& last_throw_observation() const noexcept {return last_throw_observation_;}
    BoloRotationObservation const& last_rotation_observation() const noexcept {return last_rotation_observation_;}
    std::uint32_t last_update_refusal() const noexcept {return last_update_refusal_;}
    std::uint32_t last_boundary_flags() const noexcept {return last_boundary_flags_;}
    BoloDecision update(BoloFrame const& f,BoloInput const& input={}) noexcept {
        last_update_refusal_=0;last_boundary_flags_=0;last_throw_observation_={};last_rotation_observation_={};
        const auto refuse_update=[this](std::uint32_t reason) noexcept {
            last_update_refusal_=reason;return BoloDecision{interrupt()};
        };
        if(stage_==BoloStage::unbound||stage_==BoloStage::interrupted)return {};
        if(f.stamp.binding!=binding_)return {};
        if(!stamp_valid(f.stamp)){
            // Late/replayed frames cannot release or refresh a velocity lease.
            return refuse_update(1);
        }
        const auto gap=ns_?f.stamp.monotonic_ns-ns_:0;
        consume(f.stamp);
        if(!count_matches(f.inventory,inventory_.count,inventory_.revision)||
           f.right_pocket_weapon!=config_.identity.weapon||!f.original_equipped||
           (equipment_hold_generation_&&f.equipment_hold_generation!=equipment_hold_generation_)||
           !f.original_interaction_allowed||f.original_owns_hands||!tracked(f)||!mapping_valid(f)||
           f.hands.sequence<=last_hand_sequence_||f.hands.predicted_time<=last_hand_predicted_time_||
           f.hands.sync_ns<=last_hand_sync_ns_){return refuse_update(2);}
        inventory_=f.inventory;last_hand_sequence_=f.hands.sequence;
        last_hand_predicted_time_=f.hands.predicted_time;last_hand_sync_ns_=f.hands.sync_ns;
        const bool history_supplied=f.motion_history.count||f.motion_history.reserved;
        const gears_hand_continuity::TrackedInput* motion_current=nullptr;
        if(history_supplied){
            if(!f.motion_history.count||f.motion_history.count>gears_hand_continuity::motion_history_capacity)return refuse_update(3);
            motion_current=&f.motion_history.frames[f.motion_history.count-1];
            if(!gears_hand_continuity::same_input(motion_current->input,f.hands)||
               !gears_hand_continuity::motion_history_valid(f.motion_history,*motion_current,1,config_.motion.maximum_sample_gap_ns))return refuse_update(4);
        }else if(history_mode_)return refuse_update(5);
        bool edge=false;
        if(input.kind!=BoloInputKind::none){
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
            const bool grip_input=input.kind==BoloInputKind::right_grip_press||input.kind==BoloInputKind::right_grip_release;
            const auto control=grip_input?gears_vr::control_squeeze:gears_vr::control_trigger;
            const bool released=input.kind==BoloInputKind::right_trigger_release||input.kind==BoloInputKind::right_grip_release;
            // A later release/repress in the same ring cannot borrow an old
            // edge's pose. Exact producer counters remain the current ones.
            const bool same_edge_cycle=!history_supplied||
                (right.pressed[control]==f.hands.hands[1].pressed[control]&&
                 right.released[control]==f.hands.hands[1].released[control]&&
                 (released?right.released[control]:right.pressed[control])!=0);
            const bool trigger_valid=(input.kind!=BoloInputKind::right_trigger_press&&
                input.kind!=BoloInputKind::right_trigger_release)||
                (right.trigger_active==1&&
                 (input.kind==BoloInputKind::right_trigger_press?right.trigger>=.65f:right.trigger<=.35f));
            const bool grip_valid=!grip_input||!grip_release()||
                (right.squeeze_active==1&&(released?right.squeeze<=.35f:right.squeeze>=.65f));
            if(!fresh||!same_edge_cycle||!trigger_valid||!grip_valid){
                // A release from a stale frame, recovery baseline or inactive
                // action cannot use this hold's accumulated velocity history.
                if(input.hand==1&&input.kind==release_kind()&&held_serial_)
                    return refuse_update(6);
                return {};
            }
            edge=true;
        }
        // Right Grip belongs to the equipment owner after an external hold
        // was adopted. Its later confirmed generation/identity change above
        // cancels us; a duplicate raw Grip must not toggle our local state.
        if(equipment_hold_generation_&&!grip_release()&&input.kind==BoloInputKind::right_grip_press)edge=false;
        // Raw producer history determines release pose and hand speed. The
        // current certified Game frame supplies the body transform and base
        // speed once. Body translation does not invalidate a genuine, fresh
        // historical edge within the same reference/motion generation.
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
        // Bounded mapping filter, not an action grant or a native turn certificate.
        // Producer history stays in tracking space. Release maps it through the
        // current certified Game basis once; every other hard boundary stays below.
        // Intent must be present in this same already-authenticated raw history;
        // base/head rotation never fabricates movement intent.
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
        // The Game-thread consumer interval is not a raw-sample gap. Complete
        // authenticated history below already validates each raw sample and
        // latest freshness; only the measured positive consumer interval and
        // rate bound apply here. A missing-history GT gap remains a boundary.
        const double rotation_magnitude=motion_epoch_?std::hypot(pitch_delta,yaw_delta):0.0;
        const bool pitch_continuous=grip_release()&&held_serial_&&history_supplied&&motion_current&&
            pitch_changed&&!roll_changed&&std::isfinite(pitch_delta)&&std::isfinite(yaw_delta)&&
            std::isfinite(rotation_magnitude)&&gap>0&&(locomotion_intent||yaw_stick_intent)&&
            rotation_magnitude<=.2&&rotation_magnitude<=8.0*double(gap)/1e9;
        const bool small_continuous_yaw=grip_release()&&held_serial_&&history_supplied&&motion_current&&
            (!pitch_changed||pitch_continuous)&&!roll_changed&&yaw_changed&&std::isfinite(yaw_delta)&&
            gap>0&&
            std::abs(yaw_delta)<=.2&&std::abs(yaw_delta)<=8.0*double(gap)/1e9&&yaw_stick_intent;
        const bool current_stick_intent=motion_current&&
            motion_current->input.hands[1].stick_active==1&&
            std::isfinite(motion_current->input.hands[1].stick_x)&&
            std::abs(motion_current->input.hands[1].stick_x)>=.2f;
        const bool episode_before=yaw_episode_;
        const bool bounded_turn=grip_release()&&held_serial_&&history_supplied&&motion_current&&
            (!pitch_changed||pitch_continuous)&&!roll_changed&&yaw_changed&&std::isfinite(yaw_delta)&&
            gap>0&&std::abs(yaw_delta)<=.2&&std::abs(yaw_delta)<=8.0*double(gap)/1e9;
        // Continuation only while the same hold still supplies fresh valid raw
        // history. It never renews a request, lease or input edge.
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
            !same_pose(f.mapping.frozen_reference,history_reference_)||
            f.mapping.units_per_metre!=history_units_per_metre_||
            rotation_changed||
            (gap>config_.motion.maximum_sample_gap_ns&&!history_supplied)||
            (motion_current&&motion_stream_&&(motion_current->stream_epoch!=motion_stream_||motion_current->tracking_epoch[1]!=motion_tracking_)));
        // Preserve existing diagnostic bit meanings for recorded failures.
        if(boundary)last_boundary_flags_=
            (f.stamp.motion_epoch!=motion_epoch_?1u:0u)|
            (f.mapping.reference_generation!=reference_generation_?2u:0u)|
            (!same_pose(f.mapping.frozen_reference,history_reference_)?4u:0u)|
            (f.mapping.units_per_metre!=history_units_per_metre_?8u:0u)|
            (rotation_changed?128u:0u)|
            (gap>config_.motion.maximum_sample_gap_ns&&!history_supplied?16u:0u)|
            (motion_current&&motion_stream_&&motion_current->stream_epoch!=motion_stream_?32u:0u)|
            (motion_current&&motion_stream_&&motion_current->tracking_epoch[1]!=motion_tracking_?64u:0u);
        if(neutral_stable||boundary)yaw_episode_=false;
        last_rotation_observation_.episode_after=yaw_episode_;
        motion_epoch_=f.stamp.motion_epoch;reference_generation_=f.mapping.reference_generation;
        history_reference_=f.mapping.frozen_reference;history_units_per_metre_=f.mapping.units_per_metre;
        history_mapping_=f.mapping;
        if(motion_current){history_mode_=true;motion_stream_=motion_current->stream_epoch;motion_tracking_=motion_current->tracking_epoch[1];}
        const auto& held_right=f.hands.hands[1];
        if(trigger_press_sequence_&&(grip_release()?held_right.squeeze_active:held_right.trigger_active)!=1)return refuse_update(7);
        if(history_mode_&&trigger_press_sequence_){
            const auto& right=f.hands.hands[1];
            const bool same_cycle=right.pressed[hold_control()]==trigger_pressed_counter_;
            const bool same_release=right.released[hold_control()]==trigger_released_counter_;
            const bool deliberate_release=edge&&input.kind==release_kind()&&
                trigger_released_counter_!=UINT32_MAX&&right.released[hold_control()]==trigger_released_counter_+1;
            if(!same_cycle||(!same_release&&!deliberate_release))return refuse_update(8);
        }
        if(trigger_press_sequence_&&(stage_==BoloStage::prepared||stage_==BoloStage::prepare_pending)&&
           (grip_release()?held_right.squeeze:held_right.trigger)<=.35f&&
           !(edge&&input.kind==release_kind()))return refuse_update(9);
        if(boundary){
            if(stage_==BoloStage::held||stage_==BoloStage::idle){
                tracker_.reset();return {BoloEvent::motion_reset};
            }
            return refuse_update(10);
        }
        if(request_.token){
            // Releasing while native preparation is unresolved is cancellation,
            // never a buffered throw when a delayed receipt arrives.
            if(stage_==BoloStage::prepare_pending&&edge&&
               (input.kind==release_kind()||
                (input.kind==BoloInputKind::right_grip_press&&input.slot==BodySlot::grenade)))
                return refuse_update(11);
            if(ns_>request_.valid_until_ns)return refuse_update(12);
            return {BoloEvent::observed};
        }
        if(stage_==BoloStage::idle){
            if(!edge||input.kind!=BoloInputKind::right_grip_press||input.slot!=BodySlot::grenade)return {};
            if(!inventory_.count){
                const auto audio=token();if(!audio)return {BoloEvent::safety_violation};
                return {BoloEvent::no_ammo,true,audio,config_.original_no_ammo_cue,binding_,config_.identity};
            }
            held_serial_=token();if(!held_serial_)return {BoloEvent::safety_violation};
            stage_=BoloStage::held;return {BoloEvent::grabbed};
        }
        if(!grip_release()&&edge&&input.kind==BoloInputKind::right_grip_press&&input.slot==BodySlot::grenade){
            if(stage_==BoloStage::held){clear_local_hold();stage_=BoloStage::idle;return {BoloEvent::stowed};}
            return refuse_update(13);
        }
        if(stage_==BoloStage::held){
            if(edge&&input.kind==press_kind())return prepare_hold(f,input.hand_sequence,input.sync_ns);
            return {BoloEvent::observed};
        }
        if(stage_!=BoloStage::prepared)return {};
        const bool release=edge&&input.kind==release_kind();
        if(release&&(!trigger_press_sequence_||trigger_press_hold_!=held_serial_||
           input.hand_sequence<=trigger_press_sequence_||input.sync_ns<=trigger_press_sync_ns_))
            return refuse_update(14);
        const auto solution=history_supplied?
            tracker_.advance_history(f.motion_history,f.hands,f.stereo,config_.identity.weapon,config_.identity.world,
                reference_generation_,ns_,trigger_press_sequence_,release?input.hand_sequence:0,config_.motion,
                config_.input_source==gears_vr::StereoSource::MetaOperatorSimulated,0,&last_throw_observation_):
            tracker_.advance(f.hands,f.stereo,config_.identity.weapon,config_.identity.world,
                reference_generation_,ns_,release,config_.motion,
                config_.input_source==gears_vr::StereoSource::MetaOperatorSimulated);
        if(!release){
            if(!tracker_.armed())return refuse_update(15);
            return {BoloEvent::observed};
        }
        GameGrenadeLaunch launch{};
        if(!solution)return refuse_update(16);
        if(!grenade_world_launch(*solution,f.mapping.frozen_reference,f.mapping.game_base,
            f.mapping.units_per_metre,launch,f.mapping.pawn_velocity_world_units_per_second))return refuse_update(18);
        if(!issue(BoloAction::launch_original))return refuse_update(17);
        request_.original_input_sync_ns=input.sync_ns;
        request_.valid_until_ns=(std::min)(request_.valid_until_ns,input.sync_ns+config_.request_lease_ns);
        request_.tracked=*solution;request_.launch=launch;return {BoloEvent::request_ready};
    }
    // A separate original action (for example melee attachment) can debit the
    // same weapon while VR owns an unlit preparation. Retire that stopped
    // preparation without crediting our stop with a projectile or a debit.
    // Ordinary prepare/launch/safety receipts keep their strict count rules.
    bool retire_stopped_after_external_debit(BoloOriginalReceipt const& r) noexcept {
        if(!grip_release()||config_.fuse_owner!=BoloFuseOwner::original_projectile||
           stage_!=BoloStage::safety_pending||!request_.token||!dispatched_||
           request_.action!=BoloAction::preserve_original_on_interrupt||
           r.request_token!=request_.token||!stamp_valid(r.stamp)||
           r.original_event_serial<=last_native_event_||!r.accepted||
           !r.original_effect_applied_once||!r.native_safety_complete||
           !r.original_fuse_preserved||!r.original_effects_preserved||
           !preparation_serial_||r.original_preparation_serial!=preparation_serial_||
           request_.original_preparation_serial!=preparation_serial_||
           !fuse_.valid()||fuse_.active||!(r.fuse==fuse_)||
           r.original_projectiles_spawned||r.original_projectile||
           r.tracked_initial_pose_and_velocity_applied||
           r.executed_ns<request_.authored_ns||r.executed_ns>r.stamp.monotonic_ns||
           !request_.before.count||!r.inventory.valid()||
           r.inventory.revision<=request_.before.revision||
           r.inventory.count!=request_.before.count-1)return false;
        consume(r.stamp);last_native_event_=r.original_event_serial;
        inventory_=r.inventory;request_={};interrupt_after_receipt_=false;
        clear_local_hold();stage_=BoloStage::interrupted;return true;
    }
    BoloEvent acknowledge(BoloOriginalReceipt const& r) noexcept {
        // Check the token FIRST: a stale callback with a newer transport stamp
        // must not consume the current transaction's sequence or inventory.
        if(!request_.token||!dispatched_||r.request_token!=request_.token||
           !stamp_valid(r.stamp)||r.original_event_serial<=last_native_event_||!r.fuse.valid())
            return BoloEvent::rejected;
        auto const action=request_.action;
        if(!r.accepted){
            if(r.original_effect_applied_once||r.original_projectiles_spawned||r.original_projectile||
               !count_matches(r.inventory,request_.before.count,request_.before.revision)||
               !(r.fuse==fuse_))return BoloEvent::safety_violation;
            consume(r.stamp);last_native_event_=r.original_event_serial;inventory_=r.inventory;
            // A rejected safety entry leaves the native obligation unresolved;
            // no new token, rebind or fake successful cancellation is allowed.
            if(action==BoloAction::preserve_original_on_interrupt){
                stage_=BoloStage::interrupted;return BoloEvent::safety_violation;
            }
            request_={};
            return interrupt();
        }
        // A non-throwing release of an already-owned, unlit preparation may
        // finish after a pause/source outage. It grants no new input, motion,
        // item or projectile. Positive prepare/launch and a live-fuse safety
        // operation retain their original request deadline unchanged.
        const bool release_only=action==BoloAction::preserve_original_on_interrupt&&
            !fuse_.active&&!r.fuse.active&&!r.original_projectiles_spawned&&!r.original_projectile&&r.native_safety_complete;
        const bool execution_time=r.executed_ns>=request_.authored_ns&&r.executed_ns<=r.stamp.monotonic_ns&&
            (release_only||request_.executable_at(r.executed_ns,r.stamp.identity,r.stamp.binding,
                                  request_.motion_epoch,request_.reference_generation));
        if(!execution_time||
           !r.original_effect_applied_once||!r.original_fuse_preserved||!r.original_effects_preserved||
           r.fuse.started_game_tick>r.stamp.game_tick)return BoloEvent::safety_violation;
        bool ammo_spent=false;
        if(action==BoloAction::prepare_original){
            if(!r.original_preparation_serial||r.original_preparation_serial<=last_preparation_serial_||
               r.original_projectiles_spawned||r.original_projectile||
               !count_matches(r.inventory,request_.before.count,request_.before.revision)||
               (config_.fuse_owner==BoloFuseOwner::original_projectile&&r.fuse.active)||
               (config_.fuse_owner==BoloFuseOwner::original_preparation&&
                   (!r.fuse.active||r.fuse.started_game_tick<request_.authored_game_tick)))
                return BoloEvent::safety_violation;
        }else{
            if(r.original_preparation_serial!=preparation_serial_||
               (fuse_.active&&!(r.fuse==fuse_)))return BoloEvent::safety_violation;
            ammo_spent=r.original_projectiles_spawned==1;
            if(action==BoloAction::launch_original){
                if(!ammo_spent||!r.tracked_initial_pose_and_velocity_applied)return BoloEvent::safety_violation;
            }else if(action==BoloAction::preserve_original_on_interrupt){
                // A live original fuse cannot disappear into a free reusable
                // inventory item. Native consequences keep its old fuse and
                // consume exactly one original projectile; no VR throw occurs.
                if(!r.native_safety_complete||(!fuse_.active&&ammo_spent)||
                   (!ammo_spent&&(fuse_.active||r.fuse.active)))
                    return BoloEvent::safety_violation;
            }else return BoloEvent::safety_violation;
            if(ammo_spent){
                if(!request_.before.count||!r.original_projectile||
                   r.original_projectile_type!=config_.original_projectile_type||
                   !count_matches(r.inventory,request_.before.count-1,request_.before.revision)||
                   r.inventory.revision<=request_.before.revision)return BoloEvent::safety_violation;
            }else if(r.original_projectiles_spawned||r.original_projectile||
                     !count_matches(r.inventory,request_.before.count,request_.before.revision))
                return BoloEvent::safety_violation;
        }
        consume(r.stamp);last_native_event_=r.original_event_serial;inventory_=r.inventory;request_={};
        if(action==BoloAction::prepare_original){
            preparation_serial_=last_preparation_serial_=r.original_preparation_serial;fuse_=r.fuse;
            if(interrupt_after_receipt_){interrupt_after_receipt_=false;return interrupt();}
            if(!tracker_.begin(config_.identity.weapon,config_.identity.world,1,reference_generation_,
                               config_.centre_in_right_grip_metres))return interrupt();
            stage_=BoloStage::prepared;return BoloEvent::prepared;
        }
        const bool interrupted=interrupt_after_receipt_;
        interrupt_after_receipt_=false;clear_local_hold();
        stage_=(action==BoloAction::preserve_original_on_interrupt||interrupted)?
            BoloStage::interrupted:BoloStage::idle;
        return action==BoloAction::launch_original?BoloEvent::thrown:BoloEvent::safety_completed;
    }
};
}
