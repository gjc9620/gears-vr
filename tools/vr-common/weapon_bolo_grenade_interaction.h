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
enum class BoloReleaseInput : std::uint8_t { unknown,right_trigger_release_after_press };
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
            release_input==BoloReleaseInput::right_trigger_release_after_press&&
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
enum class BoloInputKind : std::uint8_t { none,right_grip_press,right_trigger_press,right_trigger_release };
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
class BoloTriggerObserver {
    BoloIdentity identity_{};std::uint64_t binding_{},hold_{},serial_{};
    gears_hand_continuity::TrackedInput previous_{};bool known_{};
public:
    void clear() noexcept {identity_={};binding_=hold_=0;previous_={};known_=false;}
    BoloTriggerObservation observe(BoloFrame const& f,BoloInput& out) noexcept {
        out={};
        if(!f.stamp.identity.valid()||!f.stamp.binding||!f.equipment_hold_generation||
           !f.motion_history.count||f.motion_history.count>gears_hand_continuity::motion_history_capacity){clear();return BoloTriggerObservation::invalid;}
        const auto& current=f.motion_history.frames[f.motion_history.count-1];
        if(!gears_hand_continuity::same_input(current.input,f.hands)||
           !gears_hand_continuity::motion_history_valid(f.motion_history,current,1)||
           f.stamp.monotonic_ns<current.input.sync_ns){clear();return BoloTriggerObservation::invalid;}
        const bool cohort=known_&&identity_==f.stamp.identity&&binding_==f.stamp.binding&&hold_==f.equipment_hold_generation&&
            previous_.stream_epoch==current.stream_epoch&&previous_.tracking_epoch[1]==current.tracking_epoch[1]&&
            gears_hand_continuity::detail::same_key(previous_.input,current.input);
        if(cohort&&gears_hand_continuity::same_tracking(previous_,current))return BoloTriggerObservation::duplicate;
        const auto prior=previous_;identity_=f.stamp.identity;binding_=f.stamp.binding;hold_=f.equipment_hold_generation;
        previous_=current;known_=true;
        if(!cohort)return BoloTriggerObservation::baseline;
        if(current.input.sequence<=prior.input.sequence||current.input.sync_ns<=prior.input.sync_ns){clear();return BoloTriggerObservation::invalid;}
        constexpr auto control=gears_vr::control_trigger;
        const auto& old=prior.input.hands[1];const auto& tip=current.input.hands[1];
        if(tip.trigger_active!=1||old.trigger_active!=1){clear();return BoloTriggerObservation::invalid;}
        if(tip.pressed[control]<old.pressed[control]||tip.released[control]<old.released[control]){clear();return BoloTriggerObservation::invalid;}
        if(tip.pressed[control]==old.pressed[control]&&tip.released[control]==old.released[control])return BoloTriggerObservation::quiet;
        const bool press=old.pressed[control]!=UINT32_MAX&&tip.pressed[control]==old.pressed[control]+1&&tip.released[control]==old.released[control];
        const bool release=old.released[control]!=UINT32_MAX&&tip.released[control]==old.released[control]+1&&tip.pressed[control]==old.pressed[control];
        if(press==release)return BoloTriggerObservation::ambiguous;
        for(unsigned i=0;i<f.motion_history.count;++i){const auto& row=f.motion_history.frames[i];const auto& h=row.input.hands[1];
            if(row.input.sequence<=prior.input.sequence)continue;
            const auto* before=i?&f.motion_history.frames[i-1]:&prior;
            if(!gears_hand_continuity::motion_pair(*before,row,1,100000000)||h.trigger_active!=1||
               before->input.hands[1].pressed[control]!=old.pressed[control]||
               before->input.hands[1].released[control]!=old.released[control]||
               h.pressed[control]!=tip.pressed[control]||h.released[control]!=tip.released[control]||
               (press?h.trigger<.65f:h.trigger>.35f))continue;
            if(serial_==UINT64_MAX)return BoloTriggerObservation::exhausted;
            out.kind=press?BoloInputKind::right_trigger_press:BoloInputKind::right_trigger_release;
            out.edge_serial=++serial_;out.hand_sequence=row.input.sequence;out.input_epoch=row.input.input_epoch;
            out.sync_ns=row.input.sync_ns;out.continuous_input=true;
            return BoloTriggerObservation::edge;
        }
        // Ring overwritten, skipped edge, multiple cycles or boundary: never
        // invent the missing edge at the current hand position/poll clock.
        return BoloTriggerObservation::ambiguous;
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
    BoloInstanceConfig config_{};
    BoloInventory inventory_{};
    BoloStage stage_{BoloStage::unbound};
    BoloRequest request_{};
    BoloFuse fuse_{};
    GrenadeThrowTracker tracker_{};
    Pose history_reference_{};
    std::uint64_t next_token_{1},binding_{},sequence_{},last_edge_{},last_native_event_{},
                  held_serial_{},preparation_serial_{},last_preparation_serial_{},
                  motion_epoch_{},reference_generation_{},last_hand_sequence_{},
                  trigger_press_sequence_{},trigger_press_hold_{},equipment_hold_generation_{};
    std::uint64_t motion_stream_{},motion_tracking_{};
    std::uint32_t trigger_pressed_counter_{},trigger_released_counter_{};
    std::int64_t ns_{},game_tick_{},last_hand_predicted_time_{},last_hand_sync_ns_{},trigger_press_sync_ns_{};
    double history_units_per_metre_{};
    bool dispatched_{},interrupt_after_receipt_{},history_mode_{};

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
    bool tracked(BoloFrame const& f) const noexcept {
        auto const& h=f.hands;
        return h.session==config_.identity.xr_session&&h.space_epoch==config_.identity.space_epoch&&
            h.input_epoch==config_.identity.input_epoch&&h.profile_generation==config_.identity.profile_generation&&
            h.sync_ns<=f.stamp.monotonic_ns&&
            f.stamp.monotonic_ns-h.sync_ns<=config_.motion.maximum_sample_gap_ns&&
            h.source==config_.input_source&&f.stereo.source==config_.input_source&&
            gears_vr::controller_frame_matches(h,f.stereo,f.stamp.monotonic_ns,
                config_.input_source==gears_vr::StereoSource::MetaOperatorSimulated)&&
            gears_vr::controller_pose_usable(h.hands[1].grip,h.hands[1].grip_flags,h.hands[1].grip_active);
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
        motion_stream_=motion_tracking_=0;history_mode_=false;
        fuse_={};tracker_.reset();request_={};
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
    // This grants neither preparation nor launch: both still require their
    // own real fresh trigger edges and original-operation receipts.
    BoloDecision adopt_equipment_hold(BoloFrame const& f) noexcept {
        if(stage_!=BoloStage::idle||!f.equipment_hold_generation||!inventory_.count||
           request_.token||held_serial_||!stamp_valid(f.stamp))return {};
        (void)update(f);
        if(stage_!=BoloStage::idle||sequence_!=f.stamp.sequence||ns_!=f.stamp.monotonic_ns)return {};
        held_serial_=token();if(!held_serial_)return {BoloEvent::safety_violation};
        equipment_hold_generation_=f.equipment_hold_generation;
        stage_=BoloStage::held;return {BoloEvent::grabbed};
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
    BoloDecision update(BoloFrame const& f,BoloInput const& input={}) noexcept {
        if(stage_==BoloStage::unbound||stage_==BoloStage::interrupted)return {};
        if(f.stamp.binding!=binding_)return {};
        if(!stamp_valid(f.stamp)){
            // Late/replayed frames cannot release or refresh a velocity lease.
            return {interrupt()};
        }
        const auto gap=ns_?f.stamp.monotonic_ns-ns_:0;
        consume(f.stamp);
        if(!count_matches(f.inventory,inventory_.count,inventory_.revision)||
           f.right_pocket_weapon!=config_.identity.weapon||!f.original_equipped||
           (equipment_hold_generation_&&f.equipment_hold_generation!=equipment_hold_generation_)||
           !f.original_interaction_allowed||f.original_owns_hands||!tracked(f)||!mapping_valid(f)||
           f.hands.sequence<=last_hand_sequence_||f.hands.predicted_time<=last_hand_predicted_time_||
           f.hands.sync_ns<=last_hand_sync_ns_){return {interrupt()};}
        inventory_=f.inventory;last_hand_sequence_=f.hands.sequence;
        last_hand_predicted_time_=f.hands.predicted_time;last_hand_sync_ns_=f.hands.sync_ns;
        const bool history_supplied=f.motion_history.count||f.motion_history.reserved;
        const gears_hand_continuity::TrackedInput* motion_current=nullptr;
        if(history_supplied){
            if(!f.motion_history.count||f.motion_history.count>gears_hand_continuity::motion_history_capacity)return {interrupt()};
            motion_current=&f.motion_history.frames[f.motion_history.count-1];
            if(!gears_hand_continuity::same_input(motion_current->input,f.hands)||
               !gears_hand_continuity::motion_history_valid(f.motion_history,*motion_current,1,config_.motion.maximum_sample_gap_ns))return {interrupt()};
        }else if(history_mode_)return {interrupt()};
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
            const auto control=input.kind==BoloInputKind::right_grip_press?gears_vr::control_squeeze:gears_vr::control_trigger;
            // A later release/repress in the same ring cannot borrow an old
            // edge's pose. Exact producer counters remain the current ones.
            const bool same_edge_cycle=!history_supplied||
                (right.pressed[control]==f.hands.hands[1].pressed[control]&&
                 right.released[control]==f.hands.hands[1].released[control]&&
                 (input.kind==BoloInputKind::right_trigger_release?right.released[control]:right.pressed[control])!=0);
            const bool trigger_valid=(input.kind!=BoloInputKind::right_trigger_press&&
                input.kind!=BoloInputKind::right_trigger_release)||
                (right.trigger_active==1&&
                 (input.kind==BoloInputKind::right_trigger_press?right.trigger>=.65f:right.trigger<=.35f));
            if(!fresh||!same_edge_cycle||!trigger_valid){
                // A release from a stale frame, recovery baseline or inactive
                // action cannot use this hold's accumulated velocity history.
                if(input.hand==1&&input.kind==BoloInputKind::right_trigger_release&&held_serial_)
                    return {interrupt()};
                return {};
            }
            edge=true;
        }
        // Right Grip belongs to the equipment owner after an external hold
        // was adopted. Its later confirmed generation/identity change above
        // cancels us; a duplicate raw Grip must not toggle our local state.
        if(equipment_hold_generation_&&input.kind==BoloInputKind::right_grip_press)edge=false;
        const bool boundary=motion_epoch_&&(f.stamp.motion_epoch!=motion_epoch_||
            f.mapping.reference_generation!=reference_generation_||
            !same_pose(f.mapping.frozen_reference,history_reference_)||
            f.mapping.units_per_metre!=history_units_per_metre_||
            (gap>config_.motion.maximum_sample_gap_ns&&!history_supplied)||
            (motion_current&&motion_stream_&&(motion_current->stream_epoch!=motion_stream_||motion_current->tracking_epoch[1]!=motion_tracking_)));
        motion_epoch_=f.stamp.motion_epoch;reference_generation_=f.mapping.reference_generation;
        history_reference_=f.mapping.frozen_reference;history_units_per_metre_=f.mapping.units_per_metre;
        if(motion_current){history_mode_=true;motion_stream_=motion_current->stream_epoch;motion_tracking_=motion_current->tracking_epoch[1];}
        if(trigger_press_sequence_&&f.hands.hands[1].trigger_active!=1)return {interrupt()};
        if(history_mode_&&trigger_press_sequence_){
            const auto& right=f.hands.hands[1];
            const bool same_cycle=right.pressed[gears_vr::control_trigger]==trigger_pressed_counter_;
            const bool same_release=right.released[gears_vr::control_trigger]==trigger_released_counter_;
            const bool deliberate_release=edge&&input.kind==BoloInputKind::right_trigger_release&&
                trigger_released_counter_!=UINT32_MAX&&right.released[gears_vr::control_trigger]==trigger_released_counter_+1;
            if(!same_cycle||(!same_release&&!deliberate_release))return {interrupt()};
        }
        if(trigger_press_sequence_&&(stage_==BoloStage::prepared||stage_==BoloStage::prepare_pending)&&
           f.hands.hands[1].trigger<=.35f&&
           !(edge&&input.kind==BoloInputKind::right_trigger_release))return {interrupt()};
        if(boundary){
            if(stage_==BoloStage::held||stage_==BoloStage::idle){
                tracker_.reset();return {BoloEvent::motion_reset};
            }
            return {interrupt()};
        }
        if(request_.token){
            // Releasing while native preparation is unresolved is cancellation,
            // never a buffered throw when a delayed receipt arrives.
            if(stage_==BoloStage::prepare_pending&&edge&&
               (input.kind==BoloInputKind::right_trigger_release||
                (input.kind==BoloInputKind::right_grip_press&&input.slot==BodySlot::grenade)))
                return {interrupt()};
            if(ns_>request_.valid_until_ns)return {interrupt()};
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
        if(edge&&input.kind==BoloInputKind::right_grip_press&&input.slot==BodySlot::grenade){
            if(stage_==BoloStage::held){clear_local_hold();stage_=BoloStage::idle;return {BoloEvent::stowed};}
            return {interrupt()};
        }
        if(stage_==BoloStage::held){
            if(edge&&input.kind==BoloInputKind::right_trigger_press){
                trigger_press_sequence_=input.hand_sequence;trigger_press_hold_=held_serial_;
                trigger_press_sync_ns_=input.sync_ns;
                trigger_pressed_counter_=f.hands.hands[1].pressed[gears_vr::control_trigger];
                trigger_released_counter_=f.hands.hands[1].released[gears_vr::control_trigger];
                if(issue(BoloAction::prepare_original)){
                    request_.original_input_sync_ns=input.sync_ns;
                    request_.valid_until_ns=(std::min)(request_.valid_until_ns,input.sync_ns+config_.request_lease_ns);
                    return {BoloEvent::request_ready};}
                return {interrupt()};
            }
            return {BoloEvent::observed};
        }
        if(stage_!=BoloStage::prepared)return {};
        const bool release=edge&&input.kind==BoloInputKind::right_trigger_release;
        if(release&&(!trigger_press_sequence_||trigger_press_hold_!=held_serial_||
           input.hand_sequence<=trigger_press_sequence_||input.sync_ns<=trigger_press_sync_ns_))
            return {interrupt()};
        const auto solution=history_supplied?
            tracker_.advance_history(f.motion_history,f.hands,f.stereo,config_.identity.weapon,config_.identity.world,
                reference_generation_,ns_,trigger_press_sequence_,release?input.hand_sequence:0,config_.motion,
                config_.input_source==gears_vr::StereoSource::MetaOperatorSimulated):
            tracker_.advance(f.hands,f.stereo,config_.identity.weapon,config_.identity.world,
                reference_generation_,ns_,release,config_.motion,
                config_.input_source==gears_vr::StereoSource::MetaOperatorSimulated);
        if(!release){
            if(!tracker_.armed())return {interrupt()};
            return {BoloEvent::observed};
        }
        GameGrenadeLaunch launch{};
        if(!solution||!grenade_world_launch(*solution,f.mapping.frozen_reference,f.mapping.game_base,
            f.mapping.units_per_metre,launch,f.mapping.pawn_velocity_world_units_per_second))return {interrupt()};
        if(!issue(BoloAction::launch_original))return {interrupt()};
        request_.original_input_sync_ns=input.sync_ns;
        request_.valid_until_ns=(std::min)(request_.valid_until_ns,input.sync_ns+config_.request_lease_ns);
        request_.tracked=*solution;request_.launch=launch;return {BoloEvent::request_ready};
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
