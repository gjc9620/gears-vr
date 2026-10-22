#pragma once
#include <cmath>
#include <cstdint>
#include <limits>

namespace gears_weapon {

// Evidence is static, not retail runtime acceptance. Saved declarations:
// analysis/re-kb/scripts/WarfareGame/HammerOfDawnBeamManagerBase functions
// IsAValidHODTarget (native), BeginBeamWarmup (unresolved parameter type),
// SetEnabled (unresolved native tokens). They establish no usable call ABI,
// outdoor/target rule, charge duration, ammunition cost or cancellation path.
// analysis/weapon-map-bone-audit-corrected.json identifies cog_hod_mesh/Trigger,
// animation_verified=false; mesh export SHA256:
// e8d29e9e13040bd422a58d446656be47b32e695541d653d00b65e245219eed54.
// Its independent Trigger geometry does not establish a reload or hand motion.
// No magazine, shoulder ammunition, invented timer, damage or recoil is here.

struct HammerOfDawnIdentity {
    std::uint64_t world{},world_generation{},pawn{},controller{},weapon{},
                  weapon_generation{},xr_session{},space_epoch{},input_epoch{};
    bool valid() const noexcept {
        return world&&world_generation&&pawn&&controller&&weapon&&weapon_generation&&
            xr_session&&space_epoch&&input_epoch;
    }
};
inline bool operator==(HammerOfDawnIdentity const& a,HammerOfDawnIdentity const& b) noexcept {
    return a.world==b.world&&a.world_generation==b.world_generation&&a.pawn==b.pawn&&
        a.controller==b.controller&&a.weapon==b.weapon&&a.weapon_generation==b.weapon_generation&&
        a.xr_session==b.xr_session&&a.space_epoch==b.space_epoch&&a.input_epoch==b.input_epoch;
}
struct HammerOfDawnStamp {
    HammerOfDawnIdentity identity{};
    std::uint64_t binding{},sequence{},frame_ticket{};
    std::int64_t monotonic_ns{};
};
struct HammerOfDawnVector {double x{},y{},z{};};
struct HammerOfDawnRay {HammerOfDawnVector origin{},direction{};};
inline bool hammer_of_dawn_ray_valid(HammerOfDawnRay const& ray) noexcept {
    auto finite=[](HammerOfDawnVector const& v) noexcept {
        return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);
    };
    const auto& d=ray.direction;
    const double squared=d.x*d.x+d.y*d.y+d.z*d.z;
    return finite(ray.origin)&&finite(d)&&std::isfinite(squared)&&
        std::abs(squared-1.0)<=0.00001;
}
enum class HammerOfDawnResourceRule : std::uint8_t { unverified,original_managed,no_consumable };
struct HammerOfDawnConfig {
    HammerOfDawnIdentity identity{};
    // The adapter allocates a lifetime-unique binding; never reuse it on rebind.
    std::uint64_t binding{},instance_config{},pose_mapping{},resource_rule{};
    std::int64_t maximum_pose_age_ns{100000000}; // XR lease only.
    HammerOfDawnResourceRule resources{HammerOfDawnResourceRule::unverified};
    bool current_instance_verified{},original_class_verified{},
         original_l_hold_semantics_verified{},original_r_semantics_verified{},
         original_release_semantics_verified{},release_is_non_firing_verified{},
         non_firing_cancel_verified{},original_conditions_and_target_check_verified{},
         original_effects_and_recoil_preserved{},no_regular_reload_verified{};
    bool valid() const noexcept {
        return identity.valid()&&binding&&instance_config&&pose_mapping&&resource_rule&&
            current_instance_verified&&original_class_verified&&maximum_pose_age_ns>0&&
            maximum_pose_age_ns<=1000000000&&
            (resources==HammerOfDawnResourceRule::unverified||
             resources==HammerOfDawnResourceRule::original_managed||
             resources==HammerOfDawnResourceRule::no_consumable);
    }
};
// Opaque original resource units, never loaded/reserve ammunition. An accepted
// resource receipt is the only writer; there is no local assumed per-shot cost.
struct HammerOfDawnResources {std::uint64_t revision{},rule{};std::uint32_t units{};};
enum class HammerOfDawnPhase : std::uint8_t {
    idle,designating,warming,illuminating,cooldown,unavailable
};
struct HammerOfDawnOriginalState {
    std::uint64_t revision{};
    HammerOfDawnPhase phase{HammerOfDawnPhase::unavailable};
    // The original condition/resource gates must describe the current phase:
    // beginning a use and continuing an already paid-for beam are not assumed
    // to have the same resource test. The adapter certifies the applicable rule.
    bool current_instance_verified{},equipped{},ordinary_player{},input_allowed{},
         manager_enabled{},conditions_allow_use{},resources_allow_use{},
         aim_input_down{},fire_input_down{},aiming{},animation_owns_hands{};
};
struct HammerOfDawnPose {
    HammerOfDawnRay ray{}; // Current gun indicator/muzzle, in game world units.
    std::uint64_t frame_ticket{},mapping{};
    std::int64_t measured_ns{};
    bool right_tracked{},left_tracked{},right_weapon_held{},
         origin_and_axis_from_weapon{},grip_to_indicator_clear{};
};
// A target may be a surface, so target_certificate is not necessarily an actor.
// This certifies the ORIGINAL target query on this exact gun-ray ticket; an HMD
// forward trace, a HUD cache or a bone name cannot supply this proof.
struct HammerOfDawnTarget {
    std::uint64_t frame_ticket{},target_certificate{},original_check_serial{};
    bool original_conditions_passed{},original_target_valid{},occlusion_clear{},
         checked_from_same_weapon_ray{};
};
struct HammerOfDawnSample {
    HammerOfDawnStamp stamp{};
    HammerOfDawnOriginalState original{};
    HammerOfDawnResources resources{};
    HammerOfDawnPose pose{};
    HammerOfDawnTarget target{};
    bool left_trigger_down{},right_trigger_down{};
};
enum class HammerOfDawnInput : std::uint8_t { aim,fire };
struct HammerOfDawnEdge {
    std::uint64_t serial{};
    HammerOfDawnInput input{HammerOfDawnInput::fire};
    bool down{},continuous_input{};
};
enum class HammerOfDawnAction : std::uint8_t {
    none,press_original_aim,release_original_aim,press_original_fire,
    release_original_fire,update_original_designation,cancel_original_inputs
};
enum class HammerOfDawnHands : std::uint8_t { vr,original_animation,await_tracking };
enum class HammerOfDawnEvent : std::uint8_t {
    rejected,observed,request_ready,input_released,release_deferred,
    cancel_required,cancelled,original_accepted,original_rejected,
    original_effect_recorded,resource_recorded,hands_restored,
    unsupported_reload,unverified_reload,interrupted,safety_violation
};
struct HammerOfDawnRequest {
    HammerOfDawnStamp stamp{};
    std::uint64_t token{},instance_config{},use_cycle{},target_certificate{},
                  original_state_revision{},original_target_check{},pose_mapping{};
    HammerOfDawnAction action{HammerOfDawnAction::none};
    HammerOfDawnRay ray{};
    HammerOfDawnResources before{};
    std::int64_t valid_until_ns{};
    bool requires_pose{},owned_aim{},owned_fire{};
};
struct HammerOfDawnReceipt {
    HammerOfDawnStamp stamp{};
    std::uint64_t request_token{},instance_config{},original_event_serial{};
    HammerOfDawnOriginalState original{};
    HammerOfDawnResources resources{};
    bool accepted{},original_resource_change_verified{};
};
struct HammerOfDawnEffectReceipt {
    HammerOfDawnStamp stamp{};
    std::uint64_t instance_config{},use_cycle{},original_event_serial{},effect_serial{};
    HammerOfDawnResources resources{};
    bool original_effect_observed{},original_effects_and_recoil_preserved{},
         original_resource_change_verified{};
};

// One noncopyable game-thread owner. The live adapter must revalidate identity,
// state, current pose/target and request lease immediately before the ORIGINAL
// input call, and execute each token once. Never substitute StartFire/EndFire or
// an unverified stop call. A dispatched call always needs a receipt, even after
// interruption. Process original effect receipts before snapshots containing
// their resource delta. No method here calls the game or spawns a visible effect.
class HammerOfDawnInteraction {
    HammerOfDawnConfig config_{};
    HammerOfDawnOriginalState original_{};
    HammerOfDawnResources resources_{};
    HammerOfDawnPose pose_{};
    HammerOfDawnTarget target_{};
    HammerOfDawnRequest pending_{};
    std::uint64_t sequence_{},next_token_{1},last_original_event_{},last_effect_{},
        aim_edge_{},fire_edge_{},r3_edge_{},use_cycle_{},last_pose_ticket_{},last_target_check_{},
        last_designation_ticket_{};
    std::int64_t now_ns_{};
    HammerOfDawnHands hands_{HammerOfDawnHands::await_tracking};
    bool bound_{},dispatched_{},owns_aim_{},owns_fire_{},aim_armed_{},fire_armed_{},
        aim_level_{},fire_level_{},release_aim_{},release_fire_{},cancel_needed_{},
        interrupted_{},tracking_valid_{},lost_during_use_{},cycle_dispatched_{};

    bool accepts(HammerOfDawnStamp const& s) const noexcept {
        return bound_&&s.identity==config_.identity&&s.binding==config_.binding&&
            s.sequence>sequence_&&s.monotonic_ns>now_ns_&&s.monotonic_ns>0;
    }
    void consume(HammerOfDawnStamp const& s) noexcept {
        sequence_=s.sequence;now_ns_=s.monotonic_ns;
    }
    bool resource_valid(HammerOfDawnResources const& r) const noexcept {
        return r.revision&&r.rule==config_.resource_rule&&
            (config_.resources!=HammerOfDawnResourceRule::no_consumable||!r.units);
    }
    bool resource_receipt(HammerOfDawnResources const& r,bool certified) const noexcept {
        return resource_valid(r)&&r.revision>=resources_.revision&&
            (r.units==resources_.units||
             (certified&&config_.resources==HammerOfDawnResourceRule::original_managed&&
              r.revision>resources_.revision));
    }
    bool state_valid(HammerOfDawnOriginalState const& s) const noexcept {
        return s.revision>=original_.revision&&s.revision&&s.current_instance_verified&&
            (s.phase==HammerOfDawnPhase::idle||s.phase==HammerOfDawnPhase::designating||
             s.phase==HammerOfDawnPhase::warming||s.phase==HammerOfDawnPhase::illuminating||
             s.phase==HammerOfDawnPhase::cooldown||s.phase==HammerOfDawnPhase::unavailable);
    }
    bool fresh(std::int64_t ns) const noexcept {
        return tracking_valid_&&pose_.measured_ns>0&&ns>=pose_.measured_ns&&
            ns-pose_.measured_ns<=config_.maximum_pose_age_ns;
    }
    bool original_input_allowed() const noexcept {
        return original_.current_instance_verified&&original_.equipped&&original_.ordinary_player&&
            original_.input_allowed&&!original_.animation_owns_hands;
    }
    bool target_valid() const noexcept {
        return target_.frame_ticket==pose_.frame_ticket&&target_.target_certificate&&
            target_.original_check_serial&&target_.original_conditions_passed&&
            target_.original_target_valid&&target_.occlusion_clear&&
            target_.checked_from_same_weapon_ray;
    }
    bool use_allowed(std::int64_t ns) const noexcept {
        return !interrupted_&&hands_==HammerOfDawnHands::vr&&fresh(ns)&&
            original_input_allowed()&&original_.manager_enabled&&original_.conditions_allow_use&&
            original_.resources_allow_use&&target_valid()&&
            config_.original_r_semantics_verified&&config_.original_release_semantics_verified&&
            config_.non_firing_cancel_verified&&config_.original_conditions_and_target_check_verified&&
            config_.original_effects_and_recoil_preserved&&
            config_.resources!=HammerOfDawnResourceRule::unverified;
    }
    bool queue(HammerOfDawnAction action,HammerOfDawnStamp const& s) noexcept {
        if(pending_.token||next_token_==std::numeric_limits<std::uint64_t>::max()){
            interrupted_=true;return false;
        }
        pending_={};pending_.stamp=s;pending_.token=next_token_++;
        pending_.instance_config=config_.instance_config;pending_.use_cycle=use_cycle_;
        pending_.original_state_revision=original_.revision;
        pending_.original_target_check=target_.original_check_serial;
        pending_.pose_mapping=config_.pose_mapping;
        pending_.action=action;pending_.ray=pose_.ray;
        pending_.target_certificate=target_.target_certificate;pending_.before=resources_;
        pending_.owned_aim=owns_aim_;pending_.owned_fire=owns_fire_;
        pending_.requires_pose=action==HammerOfDawnAction::press_original_aim||
            action==HammerOfDawnAction::press_original_fire||
            action==HammerOfDawnAction::update_original_designation||
            (action==HammerOfDawnAction::release_original_fire&&!config_.release_is_non_firing_verified);
        const auto limit=std::numeric_limits<std::int64_t>::max()-config_.maximum_pose_age_ns;
        pending_.valid_until_ns=pose_.measured_ns>limit?std::numeric_limits<std::int64_t>::max():
            pose_.measured_ns+config_.maximum_pose_age_ns;
        dispatched_=false;return true;
    }
    HammerOfDawnEvent reconcile(HammerOfDawnStamp const& s) noexcept {
        if(pending_.token)return cancel_needed_?HammerOfDawnEvent::cancel_required:
            HammerOfDawnEvent::release_deferred;
        if(cancel_needed_){
            if(!owns_aim_&&!owns_fire_){cancel_needed_=false;return HammerOfDawnEvent::cancelled;}
            if(!config_.non_firing_cancel_verified){interrupted_=true;return HammerOfDawnEvent::cancel_required;}
            return queue(HammerOfDawnAction::cancel_original_inputs,s)?
                HammerOfDawnEvent::cancel_required:HammerOfDawnEvent::interrupted;
        }
        if(release_fire_&&owns_fire_){
            if(!config_.release_is_non_firing_verified&&!use_allowed(s.monotonic_ns))return lose_tracking();
            return queue(HammerOfDawnAction::release_original_fire,s)?
                HammerOfDawnEvent::request_ready:HammerOfDawnEvent::interrupted;
        }
        if(release_aim_&&owns_aim_)return queue(HammerOfDawnAction::release_original_aim,s)?
            HammerOfDawnEvent::request_ready:HammerOfDawnEvent::interrupted;
        return HammerOfDawnEvent::observed;
    }
    HammerOfDawnEvent lose_tracking() noexcept {
        tracking_valid_=aim_armed_=fire_armed_=false;
        if(hands_!=HammerOfDawnHands::original_animation)hands_=HammerOfDawnHands::await_tracking;
        if(owns_fire_||(pending_.token&&(pending_.action==HammerOfDawnAction::press_original_fire||
            pending_.action==HammerOfDawnAction::update_original_designation)))lost_during_use_=true;
        if(pending_.token&&!dispatched_)pending_={};
        cancel_needed_=owns_aim_||owns_fire_||pending_.token;
        const HammerOfDawnStamp stamp{config_.identity,config_.binding,sequence_,pose_.frame_ticket,now_ns_};
        return cancel_needed_?reconcile(stamp):HammerOfDawnEvent::observed;
    }
public:
    HammerOfDawnInteraction()=default;
    HammerOfDawnInteraction(HammerOfDawnInteraction const&)=delete;
    HammerOfDawnInteraction& operator=(HammerOfDawnInteraction const&)=delete;
    bool bind(HammerOfDawnConfig const& c,HammerOfDawnResources const& r) noexcept {
        if(bound_||!c.valid()||!r.revision||r.rule!=c.resource_rule||
           (c.resources==HammerOfDawnResourceRule::no_consumable&&r.units))return false;
        config_=c;resources_=r;bound_=true;return true;
    }
    HammerOfDawnHands hands() const noexcept {return hands_;}
    HammerOfDawnPhase original_phase() const noexcept {return original_.phase;}
    HammerOfDawnResources resources() const noexcept {return resources_;}
    bool owns_aim() const noexcept {return owns_aim_;}
    bool owns_fire() const noexcept {return owns_fire_;}
    bool interrupted() const noexcept {return interrupted_;}
    bool has_pending_request() const noexcept {return pending_.token!=0;}
    HammerOfDawnEvent interrupt() noexcept {interrupted_=true;return lose_tracking();}
    HammerOfDawnEvent expire_pose(std::int64_t ns) noexcept {
        if(!bound_||ns<now_ns_)return HammerOfDawnEvent::rejected;
        return fresh(ns)?HammerOfDawnEvent::observed:lose_tracking();
    }
    HammerOfDawnEvent observe(HammerOfDawnSample const& s) noexcept {
        if(!accepts(s.stamp)){
            if(bound_&&(!(s.stamp.identity==config_.identity)||s.stamp.binding!=config_.binding))
                return interrupt();
            return HammerOfDawnEvent::rejected;
        }
        if(!state_valid(s.original)||!resource_receipt(s.resources,false))return interrupt();
        const bool old_fresh=fresh(s.stamp.monotonic_ns);
        consume(s.stamp);resources_=s.resources;original_=s.original;
        aim_level_=s.left_trigger_down;fire_level_=s.right_trigger_down;
        const bool valid=s.pose.frame_ticket&&s.pose.frame_ticket==s.stamp.frame_ticket&&
            s.pose.frame_ticket>last_pose_ticket_&&s.pose.mapping==config_.pose_mapping&&
            s.pose.measured_ns>0&&s.pose.measured_ns<=s.stamp.monotonic_ns&&
            s.stamp.monotonic_ns-s.pose.measured_ns<=config_.maximum_pose_age_ns&&
            s.pose.right_tracked&&s.pose.right_weapon_held&&s.pose.origin_and_axis_from_weapon&&
            s.pose.grip_to_indicator_clear&&hammer_of_dawn_ray_valid(s.pose.ray);
        if(original_.animation_owns_hands){
            hands_=HammerOfDawnHands::original_animation;return lose_tracking();
        }
        if(!valid)return lose_tracking();
        if(!old_fresh&&(owns_fire_||owns_aim_||pending_.token)){
            lose_tracking(); // A new pose cannot erase an intervening lease gap.
        }
        pose_=s.pose;last_pose_ticket_=s.pose.frame_ticket;tracking_valid_=true;
        target_=s.target;
        if(target_.original_check_serial<=last_target_check_)target_={};
        else last_target_check_=target_.original_check_serial;
        if((owns_fire_||owns_aim_)&&!original_input_allowed())return lose_tracking();
        if(owns_aim_&&(!pose_.left_tracked||!original_.aim_input_down))return lose_tracking();
        if(owns_fire_&&(!original_.fire_input_down||!use_allowed(s.stamp.monotonic_ns))){
            if(!original_.fire_input_down)interrupted_=true;
            return lose_tracking();
        }
        const auto request_event=reconcile(s.stamp);
        if(pending_.token||cancel_needed_)return request_event;
        const bool restored=hands_!=HammerOfDawnHands::vr;
        if(pose_.left_tracked)hands_=HammerOfDawnHands::vr;
        if(!aim_level_&&!owns_aim_)aim_armed_=true;
        if(!fire_level_&&!owns_fire_)fire_armed_=true;
        if(restored&&hands_==HammerOfDawnHands::vr)return HammerOfDawnEvent::hands_restored;
        return HammerOfDawnEvent::observed;
    }
    HammerOfDawnEvent edge(HammerOfDawnStamp const& s,HammerOfDawnEdge const& e) noexcept {
        if(!accepts(s)||interrupted_||!e.serial||
           (e.input!=HammerOfDawnInput::aim&&e.input!=HammerOfDawnInput::fire))return HammerOfDawnEvent::rejected;
        const bool aim=e.input==HammerOfDawnInput::aim;
        auto& last=aim?aim_edge_:fire_edge_;
        if(e.serial<=last||s.frame_ticket!=pose_.frame_ticket||
           e.down!=(aim?aim_level_:fire_level_))return HammerOfDawnEvent::rejected;
        if(!e.continuous_input||!fresh(s.monotonic_ns)||(aim&&!pose_.left_tracked)){
            consume(s);last=e.serial;return lose_tracking();
        }
        if(e.down){
            if(pending_.token||cancel_needed_||!original_input_allowed()||hands_!=HammerOfDawnHands::vr)
                return HammerOfDawnEvent::rejected;
            if(aim){
                if(!aim_armed_||owns_aim_||original_.aim_input_down||
                   (original_.fire_input_down&&!owns_fire_)||
                   !config_.original_l_hold_semantics_verified||!config_.non_firing_cancel_verified)
                    return HammerOfDawnEvent::rejected;
            }else if(!fire_armed_||owns_fire_||original_.fire_input_down||
                     (original_.aim_input_down&&!owns_aim_)||
                     original_.phase!=HammerOfDawnPhase::idle||!use_allowed(s.monotonic_ns)||
                     use_cycle_==std::numeric_limits<std::uint64_t>::max())return HammerOfDawnEvent::rejected;
            consume(s);last=e.serial;
            if(aim){aim_armed_=false;release_aim_=false;}
            else {fire_armed_=false;release_fire_=false;lost_during_use_=false;
                cycle_dispatched_=false;++use_cycle_;}
            return queue(aim?HammerOfDawnAction::press_original_aim:
                HammerOfDawnAction::press_original_fire,s)?HammerOfDawnEvent::request_ready:
                HammerOfDawnEvent::interrupted;
        }
        consume(s);last=e.serial;
        if(aim){release_aim_=true;aim_armed_=true;}
        else {release_fire_=true;fire_armed_=true;}
        const auto press=aim?HammerOfDawnAction::press_original_aim:HammerOfDawnAction::press_original_fire;
        if(pending_.action==press&&!dispatched_){
            pending_={};
            if(aim)release_aim_=false;else release_fire_=false;
            return HammerOfDawnEvent::input_released;
        }
        return reconcile(s);
    }
    HammerOfDawnEvent update_designation(HammerOfDawnStamp const& s) noexcept {
        if(!accepts(s)||pending_.token||cancel_needed_||!owns_fire_||!fire_level_||
           s.frame_ticket!=pose_.frame_ticket||s.frame_ticket<=last_designation_ticket_||
           !use_allowed(s.monotonic_ns))return HammerOfDawnEvent::rejected;
        consume(s);last_designation_ticket_=s.frame_ticket;
        return queue(HammerOfDawnAction::update_original_designation,s)?
            HammerOfDawnEvent::request_ready:HammerOfDawnEvent::interrupted;
    }
    HammerOfDawnEvent begin_r3(HammerOfDawnStamp const& s,std::uint64_t edge_serial) noexcept {
        if(!accepts(s)||!edge_serial||edge_serial<=r3_edge_)return HammerOfDawnEvent::rejected;
        consume(s);r3_edge_=edge_serial;
        return config_.no_regular_reload_verified?HammerOfDawnEvent::unsupported_reload:
            HammerOfDawnEvent::unverified_reload;
    }
    bool take_request(HammerOfDawnRequest& out,std::int64_t ns) noexcept {
        if(!pending_.token||dispatched_||ns<pending_.stamp.monotonic_ns)return false;
        if(pending_.requires_pose&&(!fresh(ns)||ns>pending_.valid_until_ns||
           pending_.stamp.frame_ticket!=pose_.frame_ticket||!original_input_allowed()||
           ((pending_.action==HammerOfDawnAction::press_original_fire||
             pending_.action==HammerOfDawnAction::update_original_designation)&&!use_allowed(ns)))){
            lose_tracking();return false;
        }
        out=pending_;dispatched_=true;
        if(pending_.action==HammerOfDawnAction::press_original_fire)cycle_dispatched_=true;
        return true;
    }
    HammerOfDawnEvent acknowledge(HammerOfDawnReceipt const& r) noexcept {
        if(!accepts(r.stamp)||!pending_.token||!dispatched_||r.request_token!=pending_.token||
           r.instance_config!=config_.instance_config||!r.original_event_serial||
           r.original_event_serial<=last_original_event_||!state_valid(r.original))
            return HammerOfDawnEvent::rejected;
        if(!resource_receipt(r.resources,r.original_resource_change_verified))return interrupt();
        const auto action=pending_.action;
        consume(r.stamp);last_original_event_=r.original_event_serial;resources_=r.resources;
        original_=r.original;pending_={};dispatched_=false;
        if(r.accepted){
            if(action==HammerOfDawnAction::press_original_aim)owns_aim_=true;
            if(action==HammerOfDawnAction::press_original_fire)owns_fire_=true;
            if(action==HammerOfDawnAction::release_original_aim){owns_aim_=false;release_aim_=false;}
            if(action==HammerOfDawnAction::release_original_fire){owns_fire_=false;release_fire_=false;}
            if(action==HammerOfDawnAction::cancel_original_inputs){
                if((owns_aim_&&original_.aim_input_down)||(owns_fire_&&original_.fire_input_down)||
                   (owns_fire_&&(original_.phase==HammerOfDawnPhase::designating||
                    original_.phase==HammerOfDawnPhase::warming||
                    original_.phase==HammerOfDawnPhase::illuminating))){
                    interrupted_=true;return HammerOfDawnEvent::cancel_required;
                }
                owns_aim_=owns_fire_=release_aim_=release_fire_=cancel_needed_=false;
                return HammerOfDawnEvent::cancelled;
            }
            if((owns_aim_&&!original_.aim_input_down)||(owns_fire_&&!original_.fire_input_down)||
               (action==HammerOfDawnAction::release_original_fire&&original_.fire_input_down)||
               (action==HammerOfDawnAction::release_original_aim&&original_.aim_input_down)){
                interrupted_=true;return lose_tracking();
            }
        }else if(action==HammerOfDawnAction::cancel_original_inputs){
            interrupted_=true;return HammerOfDawnEvent::cancel_required;
        }else if((action==HammerOfDawnAction::press_original_fire&&original_.fire_input_down)||
                 (action==HammerOfDawnAction::press_original_aim&&original_.aim_input_down)){
            // A rejected call cannot silently acquire someone else's input.
            interrupted_=true;return HammerOfDawnEvent::safety_violation;
        }else if(action==HammerOfDawnAction::release_original_fire||
                 action==HammerOfDawnAction::release_original_aim){
            interrupted_=true;return lose_tracking();
        }
        if(cancel_needed_||!fresh(r.stamp.monotonic_ns)||original_.animation_owns_hands){
            if(original_.animation_owns_hands)hands_=HammerOfDawnHands::original_animation;
            return lose_tracking();
        }
        const auto next=reconcile(r.stamp);
        if(pending_.token||cancel_needed_)return next;
        return r.accepted?HammerOfDawnEvent::original_accepted:HammerOfDawnEvent::original_rejected;
    }
    HammerOfDawnEvent record_original_effect(HammerOfDawnEffectReceipt const& r) noexcept {
        if(!accepts(r.stamp)||!cycle_dispatched_||!use_cycle_||r.use_cycle!=use_cycle_||
           r.instance_config!=config_.instance_config||!r.original_event_serial||
           r.original_event_serial<=last_original_event_||!r.effect_serial||r.effect_serial<=last_effect_||
           !r.original_effect_observed||!r.original_effects_and_recoil_preserved)
            return HammerOfDawnEvent::rejected;
        if(!resource_receipt(r.resources,r.original_resource_change_verified))return interrupt();
        if(!fresh(r.stamp.monotonic_ns))lose_tracking();
        consume(r.stamp);last_original_event_=r.original_event_serial;last_effect_=r.effect_serial;
        resources_=r.resources;
        if(lost_during_use_){interrupted_=true;return HammerOfDawnEvent::safety_violation;}
        return HammerOfDawnEvent::original_effect_recorded;
    }
};
}
