#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace gears_weapon {

// Static evidence only: analysis/re-kb/scripts/WarfareGame contains
// Turret_TroikaCabal_base.AttachDriver/ClaimTurret/UnclaimTurret and
// Weap_Troika_base.InstantFire/WarmingUp.EndFire/Reloading.BeginState.
// Signatures/native tokens remain unresolved; these exports are NOT a call ABI,
// mount/exit animation certificate, reload recipe, heat rule or firing-ray proof.
// analysis/troika-animation-tracks.json: MP_WarMachine.war SHA256
// 55f40ca44051413fe6db19816f97b09849c5e104319d93e9d5d159cbe6fb2840;
// Spin has 10 rotation keys on EACH spinner. The audited turret mesh export
// SHA256 7baac05a7bb59b5a60e5f1b409deb2bed223e17c953ad3c3d400619e59adc037
// has handle_L/R and muzzle_L/R bones. Bone names do not certify live grips,
// alternating barrels, authoritative firing rays, animation travel or recoil.
// This fixed turret has no body slot, shoulder magazine or assumed R3 action.

struct TroikaIdentity {
    std::uint64_t world{},world_generation{},pawn{},controller{},turret{},
        turret_generation{},weapon{},weapon_generation{},xr_session{},space_epoch{},input_epoch{},
        profile_generation{};
    bool valid() const noexcept {
        return world&&world_generation&&pawn&&controller&&turret&&turret_generation&&
            weapon&&weapon_generation&&xr_session&&space_epoch&&input_epoch&&profile_generation;
    }
};
inline bool operator==(TroikaIdentity const& a,TroikaIdentity const& b) noexcept {
    return a.world==b.world&&a.world_generation==b.world_generation&&a.pawn==b.pawn&&
        a.controller==b.controller&&a.turret==b.turret&&a.turret_generation==b.turret_generation&&
        a.weapon==b.weapon&&a.weapon_generation==b.weapon_generation&&a.xr_session==b.xr_session&&
        a.space_epoch==b.space_epoch&&a.input_epoch==b.input_epoch&&
        a.profile_generation==b.profile_generation;
}
struct TroikaStamp {
    TroikaIdentity identity{};
    std::uint64_t binding{},sequence{};
    std::int64_t monotonic_ns{};
};
struct TroikaVector {double x{},y{},z{};};
struct TroikaRotation {double x{},y{},z{},w{1};};
struct TroikaTransform {TroikaVector position{};TroikaRotation rotation{};};
struct TroikaRay {TroikaVector origin{},direction{};};
inline bool troika_finite(TroikaVector const& v) noexcept {
    return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);
}
inline double troika_distance_squared(TroikaVector const& a,TroikaVector const& b) noexcept {
    const double x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;return x*x+y*y+z*z;
}
inline bool troika_rotation_valid(TroikaRotation const& q) noexcept {
    const double n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    return std::isfinite(n)&&std::abs(n-1.0)<=0.00001;
}
inline bool troika_transform_valid(TroikaTransform const& p) noexcept {
    return troika_finite(p.position)&&troika_rotation_valid(p.rotation);
}
inline bool troika_ray_valid(TroikaRay const& ray) noexcept {
    return troika_finite(ray.origin)&&troika_finite(ray.direction)&&
        std::abs(troika_distance_squared(ray.direction,{})-1.0)<=0.00001;
}
inline TroikaRotation troika_multiply(TroikaRotation const& a,TroikaRotation const& b) noexcept {
    return {a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
        a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
        a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,
        a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};
}
inline TroikaTransform troika_compose(TroikaTransform const& world,TroikaTransform const& local) noexcept {
    const auto& q=world.rotation;const auto& v=local.position;
    const TroikaVector t{2*(q.y*v.z-q.z*v.y),2*(q.z*v.x-q.x*v.z),2*(q.x*v.y-q.y*v.x)};
    return {{world.position.x+v.x+q.w*t.x+q.y*t.z-q.z*t.y,
        world.position.y+v.y+q.w*t.y+q.z*t.x-q.x*t.z,
        world.position.z+v.z+q.w*t.z+q.x*t.y-q.y*t.x},
        troika_multiply(q,local.rotation)};
}
inline bool troika_grip_matches(TroikaTransform const& turret,TroikaTransform const& local,
    TroikaTransform const& hand,double distance_m,double angle_rad) noexcept {
    if(!troika_transform_valid(turret)||!troika_transform_valid(local)||!troika_transform_valid(hand)||
       !std::isfinite(distance_m)||distance_m<=0||!std::isfinite(angle_rad)||angle_rad<=0||
       angle_rad>3.141592653589793)
        return false;
    const auto expected=troika_compose(turret,local);
    const auto& a=expected.rotation;const auto& b=hand.rotation;
    const double dot=std::abs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w);
    return troika_distance_squared(expected.position,hand.position)<=distance_m*distance_m&&
        dot>=std::cos(angle_rad*0.5);
}

struct TroikaMechanicalStep {std::uint64_t component{},action{},motion_spec{};};
struct TroikaConfig {
    TroikaIdentity identity{};
    // All IDs/certificates belong to THIS instance; binding must never be reused.
    std::uint64_t binding{},instance_config{},pose_mapping{},resource_rule{},
        left_handle{},right_handle{},left_muzzle{},right_muzzle{},mechanical_plan{};
    TroikaVector fixed_pivot_world_m{};
    TroikaTransform left_handle_local_m{},right_handle_local_m{};
    // No retail calibration is supplied. The adapter certifies axes, metres,
    // yaw/pitch mapping, original travel limits and contact tolerances together.
    double minimum_yaw_rad{},maximum_yaw_rad{},minimum_pitch_rad{},maximum_pitch_rad{},
        grip_distance_m{},grip_angle_rad{},anchor_tolerance_m{};
    std::int64_t maximum_pose_age_ns{100000000}; // XR lease, never original warmup/heat timing.
    bool current_instance_verified{},fixed_turret_verified{},geometry_verified{},
        original_mount_verified{},original_unmount_verified{},original_aim_verified{},
        original_fire_verified{},non_firing_stop_verified{},original_effects_preserved{},
        resource_rules_verified{},heat_rules_verified{},reload_rules_verified{},
        manual_mechanical_plan_verified{};
    std::uint32_t step_count{};
    std::array<TroikaMechanicalStep,8> steps{};
    bool valid() const noexcept {
        if(!identity.valid()||!binding||!instance_config||!pose_mapping||!resource_rule||
           !current_instance_verified||!fixed_turret_verified||!geometry_verified||
           !left_handle||!right_handle||left_handle==right_handle||!left_muzzle||!right_muzzle||
           left_muzzle==right_muzzle||!troika_finite(fixed_pivot_world_m)||
           !troika_transform_valid(left_handle_local_m)||!troika_transform_valid(right_handle_local_m)||
           troika_distance_squared(left_handle_local_m.position,right_handle_local_m.position)<0.000001||
           !std::isfinite(minimum_yaw_rad)||!std::isfinite(maximum_yaw_rad)||
           !std::isfinite(minimum_pitch_rad)||!std::isfinite(maximum_pitch_rad)||
           minimum_yaw_rad>=maximum_yaw_rad||minimum_pitch_rad>=maximum_pitch_rad||
           !std::isfinite(grip_distance_m)||grip_distance_m<=0||grip_distance_m>1||
           !std::isfinite(grip_angle_rad)||grip_angle_rad<=0||grip_angle_rad>3.141592653589793||
           !std::isfinite(anchor_tolerance_m)||anchor_tolerance_m<=0||anchor_tolerance_m>1||
           maximum_pose_age_ns<=0||maximum_pose_age_ns>1000000000)return false;
        if(!manual_mechanical_plan_verified)return !step_count&&!mechanical_plan;
        if(!reload_rules_verified||!mechanical_plan||!step_count||step_count>steps.size())return false;
        for(std::uint32_t i=0;i<step_count;++i)
            if(!steps[i].component||!steps[i].action||!steps[i].motion_spec)return false;
        return true;
    }
};
// Opaque original resource values, with no assumed cost/capacity/heat threshold.
// Original shots, cooling, scripted changes and mechanics must carry receipts.
struct TroikaResources {
    std::uint64_t revision{},rule{};
    std::uint32_t ammunition{};
    double heat{};
};
enum class TroikaPhase : std::uint8_t { detached,entering,mounted,leaving,unavailable };
struct TroikaOriginalState {
    std::uint64_t revision{},rule{},animation_serial{},service_serial{};
    TroikaPhase phase{TroikaPhase::unavailable};
    // These original gates describe the CURRENT phase: beginning a firing cycle
    // and continuing native warmup/fire need not use the same original check.
    bool current_instance_verified{},driver_is_this_pawn{},input_allowed{},can_mount{},can_unmount{},
        can_fire{},resources_allow_fire{},heat_allows_fire{},fire_input_down{},
        animation_owns_hands{},servicing{};
};
struct TroikaMuzzle {std::uint64_t identity{};TroikaRay ray{};};
struct TroikaPose {
    TroikaTransform turret_world_m{},left_hand_world_m{},right_hand_world_m{};
    std::array<TroikaMuzzle,2> muzzles{}; // [left,right], never alternated locally.
    std::uint64_t frame_ticket{},mapping{};
    std::int64_t measured_ns{};
    double yaw_rad{},pitch_rad{};
    bool left_tracked{},right_tracked{},left_grip_down{},right_grip_down{},
        original_pivot_and_constraints_verified{},muzzle_axis_relation_verified{},mount_contact_verified{};
};
struct TroikaSample {
    TroikaStamp stamp{};
    TroikaOriginalState original{};
    TroikaResources resources{};
    TroikaPose pose{};
    bool trigger_down{},interact_down{},original_resource_change_verified{};
    std::uint64_t resource_event_serial{};
};
enum class TroikaInput : std::uint8_t { fire,interact };
struct TroikaEdge {std::uint64_t serial{};TroikaInput input{TroikaInput::fire};bool down{},continuous_input{};};
enum class TroikaAction : std::uint8_t {
    none,mount_original,unmount_original,aim_original,press_original_fire,stop_original_fire,mechanical_original
};
enum class TroikaHands : std::uint8_t { vr,original_animation,await_tracking };
enum class TroikaEvent : std::uint8_t {
    rejected,observed,request_ready,original_accepted,original_rejected,hands_restored,
    animation_handoff,stop_required,interrupted,shot_recorded,mechanical_started,mechanical_advanced,
    unsupported_mechanism,safety_violation
};
struct TroikaRequest {
    TroikaStamp stamp{};
    std::uint64_t token{},instance_config{},state_revision{},resource_revision{},frame_ticket{},
        pose_mapping{},fire_cycle{},mechanical_plan{},service_serial{},component{},action{},motion_spec{};
    TroikaAction kind{TroikaAction::none};
    double yaw_rad{},pitch_rad{};
    std::array<TroikaMuzzle,2> muzzles{};
    std::int64_t valid_until_ns{};
};
struct TroikaReceipt {
    TroikaStamp stamp{};
    std::uint64_t token{},instance_config{},original_event_serial{};
    TroikaOriginalState original{};
    TroikaResources resources{};
    bool accepted{},original_resource_change_verified{},mechanical_completion_verified{};
};
struct TroikaAnimationReceipt {
    TroikaStamp stamp{};
    std::uint64_t instance_config{},request_token{},animation_serial{},original_event_serial{};
    TroikaOriginalState original{};
    bool original_animation_completed{},original_input_restored{};
};
struct TroikaShotReceipt {
    TroikaStamp stamp{};
    std::uint64_t instance_config{},fire_cycle{},shot_serial{},muzzle{},original_event_serial{};
    TroikaRay authoritative_firing_ray{};
    TroikaResources resources{};
    bool original_shot_observed{},original_muzzle_identity_verified{},original_ray_verified{},
        original_damage_spread_recoil_effects_preserved{},original_resource_change_verified{};
};
struct TroikaMechanicalStart {
    std::uint64_t instance_config{},plan{},service_serial{},original_event_serial{};
    bool original_start_verified{},automatic_completion_suspended{};
};
struct TroikaMechanicalMotion {
    std::uint64_t instance_config{},plan{},service_serial{},component{},action{},motion_spec{},
        motion_serial{},motion_begin_sequence{},frame_ticket{};
    bool original_geometry_verified{},continuous{},complete{},correct_hand_contacts{};
};
struct TroikaGuide {std::uint64_t component{},action{},motion_spec{};std::uint32_t step{};};

// Single game-thread owner. No game calls, ammunition changes, damage, recoil or
// muzzle flashes are produced here. The live adapter must revalidate each token
// immediately before the ORIGINAL call, execute it once, and provide a receipt
// even after tracking loss/recenter/switch. Never discard an in-flight press.
// It must drain original shot/resource receipts before a later firing cycle.
// Recenter, pawn/world/equipment change creates a NEW binding only after this
// owner's stop/animation work is settled. HMD forward is never an aiming input.
class TroikaInteraction {
    TroikaConfig config_{};TroikaResources resources_{};TroikaOriginalState original_{};
    TroikaPose pose_{};TroikaRequest pending_{};TroikaShotReceipt last_shot_{};
    std::uint64_t sequence_{},next_token_{1},last_event_{},last_resource_event_{},last_shot_serial_{},
        fire_edge_{},interact_edge_{},fire_cycle_{},last_pose_ticket_{},animation_{},
        transition_token_{},service_serial_{},service_start_sequence_{},last_motion_{},last_animation_serial_{};
    std::int64_t now_ns_{};
    std::uint32_t step_{};
    TroikaHands hands_{TroikaHands::await_tracking};
    TroikaPhase transition_target_{TroikaPhase::unavailable};
    bool bound_{},dispatched_{},owns_fire_{},stop_needed_{},exit_requested_{},terminal_{},
        tracking_good_{},fire_armed_{},interact_armed_{},fire_level_{},interact_level_{},
        fire_edge_level_{},interact_edge_level_{},service_active_{},unresolved_transition_{};

    bool accepts(TroikaStamp const& s) const noexcept {
        return bound_&&s.identity==config_.identity&&s.binding==config_.binding&&
            s.sequence>sequence_&&s.monotonic_ns>now_ns_&&s.monotonic_ns>0;
    }
    void consume(TroikaStamp const& s) noexcept {sequence_=s.sequence;now_ns_=s.monotonic_ns;}
    bool state_valid(TroikaOriginalState const& s) const noexcept {
        if(!s.current_instance_verified||!s.revision||s.revision<=original_.revision||s.rule!=config_.resource_rule)
            return false;
        switch(s.phase){
            case TroikaPhase::detached:if(s.driver_is_this_pawn)return false;break;
            case TroikaPhase::entering:case TroikaPhase::mounted:case TroikaPhase::leaving:break;
            case TroikaPhase::unavailable:break;
            default:return false;
        }
        return (!s.animation_owns_hands||(s.animation_serial&&
            (s.animation_serial==animation_||s.animation_serial>last_animation_serial_)))&&
            (!s.servicing||s.service_serial);
    }
    bool resources_valid(TroikaResources const& r,bool certified) const noexcept {
        return r.rule==config_.resource_rule&&r.revision&&r.revision>=resources_.revision&&
            std::isfinite(r.heat)&&r.heat>=0&&
            ((r.ammunition==resources_.ammunition&&r.heat==resources_.heat)||
             (certified&&r.revision>resources_.revision));
    }
    bool pose_valid(TroikaPose const& p,std::int64_t ns) const noexcept {
        return p.frame_ticket&&p.mapping==config_.pose_mapping&&p.measured_ns>0&&p.measured_ns<=ns&&
            ns-p.measured_ns<=config_.maximum_pose_age_ns&&p.left_tracked&&p.right_tracked&&
            p.original_pivot_and_constraints_verified&&p.muzzle_axis_relation_verified&&
            troika_transform_valid(p.turret_world_m)&&troika_transform_valid(p.left_hand_world_m)&&
            troika_transform_valid(p.right_hand_world_m)&&
            troika_distance_squared(p.turret_world_m.position,config_.fixed_pivot_world_m)<=
                config_.anchor_tolerance_m*config_.anchor_tolerance_m&&
            std::isfinite(p.yaw_rad)&&std::isfinite(p.pitch_rad)&&
            p.yaw_rad>=config_.minimum_yaw_rad&&p.yaw_rad<=config_.maximum_yaw_rad&&
            p.pitch_rad>=config_.minimum_pitch_rad&&p.pitch_rad<=config_.maximum_pitch_rad&&
            p.muzzles[0].identity==config_.left_muzzle&&p.muzzles[1].identity==config_.right_muzzle&&
            troika_ray_valid(p.muzzles[0].ray)&&troika_ray_valid(p.muzzles[1].ray);
    }
    bool fresh(std::int64_t ns) const noexcept {
        return tracking_good_&&ns>=now_ns_&&pose_valid(pose_,ns);
    }
    bool right_handle() const noexcept {
        return pose_.right_grip_down&&troika_grip_matches(pose_.turret_world_m,config_.right_handle_local_m,
            pose_.right_hand_world_m,config_.grip_distance_m,config_.grip_angle_rad);
    }
    bool both_handles() const noexcept {
        return right_handle()&&pose_.left_grip_down&&troika_grip_matches(pose_.turret_world_m,
            config_.left_handle_local_m,pose_.left_hand_world_m,config_.grip_distance_m,config_.grip_angle_rad);
    }
    bool drive_allowed(std::int64_t ns) const noexcept {
        return !terminal_&&!exit_requested_&&!animation_&&hands_==TroikaHands::vr&&fresh(ns)&&
            original_.phase==TroikaPhase::mounted&&original_.driver_is_this_pawn&&original_.input_allowed&&
            !original_.animation_owns_hands&&!original_.servicing&&config_.original_aim_verified&&both_handles();
    }
    bool fire_allowed(std::int64_t ns) const noexcept {
        return drive_allowed(ns)&&config_.original_fire_verified&&config_.non_firing_stop_verified&&
            config_.original_effects_preserved&&config_.resource_rules_verified&&config_.heat_rules_verified&&
            config_.reload_rules_verified&&original_.can_fire&&original_.resources_allow_fire&&
            original_.heat_allows_fire;
    }
    bool fire_in_flight() const noexcept {
        return owns_fire_||(pending_.token&&dispatched_&&pending_.kind==TroikaAction::press_original_fire);
    }
    bool queue(TroikaAction kind,TroikaStamp const& s) noexcept {
        if(pending_.token||next_token_==std::numeric_limits<std::uint64_t>::max())return false;
        pending_={};pending_.stamp=s;pending_.token=next_token_++;pending_.kind=kind;
        pending_.instance_config=config_.instance_config;pending_.state_revision=original_.revision;
        pending_.resource_revision=resources_.revision;pending_.frame_ticket=pose_.frame_ticket;
        pending_.pose_mapping=config_.pose_mapping;pending_.yaw_rad=pose_.yaw_rad;pending_.pitch_rad=pose_.pitch_rad;
        pending_.muzzles=pose_.muzzles;pending_.fire_cycle=kind==TroikaAction::press_original_fire?pending_.token:fire_cycle_;
        const auto limit=std::numeric_limits<std::int64_t>::max()-config_.maximum_pose_age_ns;
        pending_.valid_until_ns=pose_.measured_ns>limit?std::numeric_limits<std::int64_t>::max():
            pose_.measured_ns+config_.maximum_pose_age_ns;
        dispatched_=false;return true;
    }
    void invalidate_tracking() noexcept {
        // Latch cancellation before clearing an undispatched request. Recovery
        // before a native receipt arrives must never revive a dispatched press.
        if(fire_in_flight())stop_needed_=true;
        tracking_good_=false;fire_armed_=interact_armed_=false;hands_=animation_?
            TroikaHands::original_animation:TroikaHands::await_tracking;
        if(pending_.token&&!dispatched_&&pending_.kind!=TroikaAction::stop_original_fire)pending_={};
        if(owns_fire_)stop_needed_=true;
        service_active_=false;
    }
    TroikaEvent reconcile(TroikaStamp const& s) noexcept {
        if(pending_.token)return stop_needed_?TroikaEvent::stop_required:TroikaEvent::observed;
        if(stop_needed_&&owns_fire_){
            return queue(TroikaAction::stop_original_fire,s)?TroikaEvent::request_ready:TroikaEvent::safety_violation;
        }
        if(exit_requested_&&!owns_fire_&&!animation_&&!terminal_&&original_.phase==TroikaPhase::mounted&&
           original_.driver_is_this_pawn&&original_.can_unmount&&original_.input_allowed&&config_.original_unmount_verified)
            return queue(TroikaAction::unmount_original,s)?TroikaEvent::request_ready:TroikaEvent::safety_violation;
        return terminal_?TroikaEvent::interrupted:TroikaEvent::observed;
    }
    void adopt_state(TroikaOriginalState const& s) noexcept {
        original_=s;
        if(s.animation_owns_hands){
            animation_=s.animation_serial;hands_=TroikaHands::original_animation;
            if(animation_>last_animation_serial_)last_animation_serial_=animation_;
            fire_armed_=interact_armed_=false;service_active_=false;
        }
        if(fire_in_flight()&&(!fire_allowed(now_ns_)||!fire_level_))stop_needed_=true;
    }
public:
    TroikaInteraction()=default;
    TroikaInteraction(TroikaInteraction const&)=delete;
    TroikaInteraction& operator=(TroikaInteraction const&)=delete;
    bool bind(TroikaConfig const& c,TroikaResources const& r) noexcept {
        if(bound_||!c.valid()||!r.revision||r.rule!=c.resource_rule||!std::isfinite(r.heat)||r.heat<0)return false;
        config_=c;resources_=r;bound_=true;return true;
    }
    TroikaHands hands() const noexcept {return hands_;}
    TroikaResources resources() const noexcept {return resources_;}
    TroikaShotReceipt const& last_shot() const noexcept {return last_shot_;}
    bool owns_fire() const noexcept {return owns_fire_;}
    bool interrupted() const noexcept {return terminal_;}
    bool has_pending_request() const noexcept {return pending_.token!=0;}
    bool settled() const noexcept {return !owns_fire_&&!pending_.token&&!animation_&&!unresolved_transition_;}
    TroikaEvent interrupt() noexcept {
        terminal_=true;exit_requested_=false;invalidate_tracking();return owns_fire_||
            (pending_.token&&dispatched_)?TroikaEvent::stop_required:TroikaEvent::interrupted;
    }
    TroikaEvent poll(TroikaStamp const& s) noexcept {
        if(!accepts(s))return TroikaEvent::rejected;
        consume(s);if(!fresh(now_ns_))invalidate_tracking();return reconcile(s);
    }
    TroikaEvent observe(TroikaSample const& s) noexcept {
        if(!accepts(s.stamp)||!state_valid(s.original)||
           !resources_valid(s.resources,s.original_resource_change_verified)||
           ((s.resources.ammunition!=resources_.ammunition||s.resources.heat!=resources_.heat)&&
            (!s.resource_event_serial||s.resource_event_serial<=last_resource_event_)))return TroikaEvent::rejected;
        consume(s.stamp);resources_=s.resources;
        if(s.resource_event_serial>last_resource_event_)last_resource_event_=s.resource_event_serial;
        fire_level_=s.trigger_down;interact_level_=s.interact_down;
        const bool restored=!tracking_good_;
        tracking_good_=!terminal_&&s.pose.frame_ticket>last_pose_ticket_&&
            s.pose.measured_ns>=pose_.measured_ns&&pose_valid(s.pose,now_ns_);
        if(s.pose.frame_ticket>last_pose_ticket_)last_pose_ticket_=s.pose.frame_ticket;
        pose_=s.pose;adopt_state(s.original);
        if(!tracking_good_)invalidate_tracking();
        else if(!animation_&&!original_.animation_owns_hands){
            hands_=TroikaHands::vr;
            if(!fire_level_)fire_armed_=true;
            if(!interact_level_)interact_armed_=true;
        }
        if(fire_in_flight()&&(!fire_allowed(now_ns_)||!fire_level_))stop_needed_=true;
        const auto e=reconcile(s.stamp);
        if(e!=TroikaEvent::observed)return e;
        return original_.animation_owns_hands?TroikaEvent::animation_handoff:
            restored&&tracking_good_&&!animation_?TroikaEvent::hands_restored:TroikaEvent::observed;
    }
    TroikaEvent edge(TroikaStamp const& s,TroikaEdge const& e) noexcept {
        if(!accepts(s)||!e.serial||!e.continuous_input||
           (e.input!=TroikaInput::fire&&e.input!=TroikaInput::interact))return TroikaEvent::rejected;
        auto& serial=e.input==TroikaInput::fire?fire_edge_:interact_edge_;
        auto& level=e.input==TroikaInput::fire?fire_edge_level_:interact_edge_level_;
        auto& armed=e.input==TroikaInput::fire?fire_armed_:interact_armed_;
        if(e.serial<=serial||level==e.down||e.down!=(e.input==TroikaInput::fire?fire_level_:interact_level_))
            return TroikaEvent::rejected;
        consume(s);serial=e.serial;level=e.down;
        if(!e.down){armed=tracking_good_&&!terminal_&&!animation_;if(e.input==TroikaInput::fire&&fire_in_flight())
            stop_needed_=true;return reconcile(s);}
        const bool was_armed=armed;armed=false;
        if(!was_armed||terminal_||animation_||!fresh(now_ns_))return TroikaEvent::rejected;
        if(e.input==TroikaInput::fire){
            if(pending_.token||owns_fire_||original_.fire_input_down||!fire_allowed(now_ns_))return TroikaEvent::rejected;
            return queue(TroikaAction::press_original_fire,s)?TroikaEvent::request_ready:TroikaEvent::rejected;
        }
        if(!original_.input_allowed)return TroikaEvent::rejected;
        if(original_.phase==TroikaPhase::mounted&&original_.driver_is_this_pawn&&
           original_.can_unmount&&config_.original_unmount_verified){
            exit_requested_=true;fire_armed_=false;service_active_=false;
            if(pending_.token&&!dispatched_)pending_={};if(owns_fire_)stop_needed_=true;
            return reconcile(s);
        }
        if(original_.phase!=TroikaPhase::detached||!original_.can_mount||!config_.original_mount_verified||
           !pose_.mount_contact_verified||pending_.token)return TroikaEvent::rejected;
        return queue(TroikaAction::mount_original,s)?TroikaEvent::request_ready:TroikaEvent::rejected;
    }
    TroikaEvent update_aim(TroikaStamp const& s) noexcept {
        if(!accepts(s)||pending_.token||!drive_allowed(s.monotonic_ns))return TroikaEvent::rejected;
        consume(s);return queue(TroikaAction::aim_original,s)?TroikaEvent::request_ready:TroikaEvent::rejected;
    }
    bool take_request(TroikaRequest& out,std::int64_t ns) noexcept {
        if(!pending_.token||dispatched_||ns<now_ns_)return false;
        const auto kind=pending_.kind;
        bool valid=kind==TroikaAction::stop_original_fire;
        if(!valid&&kind==TroikaAction::unmount_original)
            valid=!terminal_&&!owns_fire_&&!animation_&&original_.phase==TroikaPhase::mounted&&
                original_.driver_is_this_pawn&&original_.can_unmount&&original_.input_allowed&&config_.original_unmount_verified;
        else if(!valid){
            valid=!terminal_&&!animation_&&fresh(ns)&&ns<=pending_.valid_until_ns&&
                pending_.state_revision==original_.revision&&pending_.resource_revision==resources_.revision&&
                pending_.frame_ticket==pose_.frame_ticket;
            if(kind==TroikaAction::press_original_fire)valid=valid&&fire_level_&&fire_allowed(ns)&&!original_.fire_input_down;
            else if(kind==TroikaAction::aim_original)valid=valid&&drive_allowed(ns);
            else if(kind==TroikaAction::mount_original)valid=valid&&original_.phase==TroikaPhase::detached&&
                original_.can_mount&&original_.input_allowed&&pose_.mount_contact_verified&&config_.original_mount_verified;
            else if(kind==TroikaAction::mechanical_original)valid=valid&&service_active_&&right_handle()&&
                original_.servicing&&original_.service_serial==service_serial_;
        }
        if(!valid){pending_={};dispatched_=false;if(owns_fire_)stop_needed_=true;return false;}
        out=pending_;dispatched_=true;now_ns_=ns;return true;
    }
    TroikaEvent acknowledge(TroikaReceipt const& r) noexcept {
        if(!accepts(r.stamp)||!pending_.token||!dispatched_||r.token!=pending_.token||
           r.instance_config!=config_.instance_config||r.original_event_serial<=last_event_)return TroikaEvent::rejected;
        consume(r.stamp);last_event_=r.original_event_serial;
        const auto request=pending_;pending_={};dispatched_=false;
        // An accepted press may already own a live native firing input even if
        // the rest of the receipt is malformed. Retain that lease for cleanup.
        if(r.accepted&&request.kind==TroikaAction::press_original_fire){owns_fire_=true;fire_cycle_=request.fire_cycle;}
        if(r.accepted&&(request.kind==TroikaAction::mount_original||request.kind==TroikaAction::unmount_original))
            unresolved_transition_=true;
        bool valid=state_valid(r.original)&&resources_valid(r.resources,r.original_resource_change_verified);
        if(r.accepted){
            if(request.kind==TroikaAction::press_original_fire)valid=valid&&r.original.fire_input_down;
            else if(request.kind==TroikaAction::stop_original_fire)valid=valid&&!r.original.fire_input_down;
            else if(request.kind==TroikaAction::mount_original||request.kind==TroikaAction::unmount_original)
                valid=valid&&r.original.animation_owns_hands&&r.original.animation_serial&&
                    r.original.driver_is_this_pawn&&
                    r.original.phase==(request.kind==TroikaAction::mount_original?TroikaPhase::entering:TroikaPhase::leaving);
            else if(request.kind==TroikaAction::mechanical_original)
                valid=valid&&(step_+1<config_.step_count?
                    r.original.servicing&&r.original.service_serial==service_serial_:
                    !r.original.servicing&&r.mechanical_completion_verified);
        }
        if(!valid){terminal_=true;invalidate_tracking();reconcile(r.stamp);return TroikaEvent::safety_violation;}
        resources_=r.resources;
        if(!r.accepted&&request.kind==TroikaAction::press_original_fire&&!owns_fire_)stop_needed_=false;
        if(r.accepted&&request.kind==TroikaAction::stop_original_fire){owns_fire_=false;stop_needed_=false;}
        if(r.accepted&&(request.kind==TroikaAction::mount_original||request.kind==TroikaAction::unmount_original)){
            unresolved_transition_=false;
            transition_token_=request.token;transition_target_=request.kind==TroikaAction::mount_original?
                TroikaPhase::mounted:TroikaPhase::detached;
        }
        adopt_state(r.original);
        if(r.accepted&&request.kind==TroikaAction::mechanical_original){
            ++step_;if(step_==config_.step_count)service_active_=false;
        }
        if(owns_fire_&&(terminal_||!fire_level_||!fire_allowed(now_ns_)||exit_requested_))stop_needed_=true;
        const auto next=reconcile(r.stamp);
        if(next!=TroikaEvent::observed)return next;
        return r.accepted?TroikaEvent::original_accepted:TroikaEvent::original_rejected;
    }
    TroikaEvent animation_finished(TroikaAnimationReceipt const& r) noexcept {
        if(!accepts(r.stamp)||!animation_||r.instance_config!=config_.instance_config||
           r.animation_serial!=animation_||r.request_token!=transition_token_||
           r.original_event_serial<=last_event_||!state_valid(r.original)||
           !r.original_animation_completed||!r.original_input_restored||r.original.animation_owns_hands||
           (transition_token_&&r.original.phase!=transition_target_))return TroikaEvent::rejected;
        consume(r.stamp);last_event_=r.original_event_serial;animation_=transition_token_=0;
        transition_target_=TroikaPhase::unavailable;exit_requested_=false;original_=r.original;
        invalidate_tracking();return reconcile(r.stamp);
    }
    TroikaEvent record_original_shot(TroikaShotReceipt const& r) noexcept {
        if(!accepts(r.stamp)||r.instance_config!=config_.instance_config||!fire_cycle_||r.fire_cycle!=fire_cycle_||
           !r.shot_serial||r.shot_serial<=last_shot_serial_||r.original_event_serial<=last_event_||
           (r.muzzle!=config_.left_muzzle&&r.muzzle!=config_.right_muzzle)||
           !r.original_shot_observed||!r.original_muzzle_identity_verified||!r.original_ray_verified||
           !r.original_damage_spread_recoil_effects_preserved||!troika_ray_valid(r.authoritative_firing_ray)||
           !resources_valid(r.resources,r.original_resource_change_verified))return TroikaEvent::rejected;
        consume(r.stamp);last_event_=r.original_event_serial;last_shot_serial_=r.shot_serial;
        last_shot_=r;resources_=r.resources;return TroikaEvent::shot_recorded;
    }
    // There is deliberately no R3/default reload entry. This begins only after
    // an original current-instance service start, with automatic fill suspended.
    TroikaEvent begin_mechanical(TroikaStamp const& s,TroikaMechanicalStart const& r) noexcept {
        if(!accepts(s))return TroikaEvent::rejected;
        if(!config_.manual_mechanical_plan_verified)return TroikaEvent::unsupported_mechanism;
        if(terminal_||animation_||exit_requested_||pending_.token||owns_fire_||service_active_||!fresh(s.monotonic_ns)||
           !right_handle()||!original_.servicing||!original_.input_allowed||
           original_.phase!=TroikaPhase::mounted||!original_.driver_is_this_pawn||
           r.instance_config!=config_.instance_config||r.plan!=config_.mechanical_plan||
           !r.service_serial||r.service_serial!=original_.service_serial||r.service_serial<=service_serial_||
           r.original_event_serial<=last_event_||!r.original_start_verified||!r.automatic_completion_suspended)
            return TroikaEvent::rejected;
        consume(s);last_event_=r.original_event_serial;service_serial_=r.service_serial;
        service_start_sequence_=s.sequence;step_=0;service_active_=true;return TroikaEvent::mechanical_started;
    }
    TroikaGuide guide(std::int64_t ns) const noexcept {
        if(!service_active_||terminal_||animation_||!fresh(ns)||!right_handle()||
           !original_.input_allowed||original_.phase!=TroikaPhase::mounted||!original_.driver_is_this_pawn||
           !original_.servicing||original_.service_serial!=service_serial_||step_>=config_.step_count)return {};
        const auto& p=config_.steps[step_];return {p.component,p.action,p.motion_spec,step_};
    }
    TroikaEvent mechanical_motion(TroikaStamp const& s,TroikaMechanicalMotion const& r) noexcept {
        const auto g=guide(s.monotonic_ns);
        if(!accepts(s)||!g.component||pending_.token||r.instance_config!=config_.instance_config||
           r.plan!=config_.mechanical_plan||r.service_serial!=service_serial_||r.component!=g.component||
           r.action!=g.action||r.motion_spec!=g.motion_spec||!r.motion_serial||r.motion_serial<=last_motion_||
           r.motion_begin_sequence<service_start_sequence_||r.motion_begin_sequence>s.sequence||
           r.frame_ticket!=pose_.frame_ticket||!r.original_geometry_verified||!r.continuous||
           !r.complete||!r.correct_hand_contacts)return TroikaEvent::rejected;
        consume(s);last_motion_=r.motion_serial;
        if(!queue(TroikaAction::mechanical_original,s))return TroikaEvent::rejected;
        pending_.mechanical_plan=config_.mechanical_plan;pending_.service_serial=service_serial_;
        pending_.component=g.component;pending_.action=g.action;pending_.motion_spec=g.motion_spec;
        return TroikaEvent::request_ready;
    }
};

} // namespace gears_weapon
