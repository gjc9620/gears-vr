#pragma once
#include "weapon_mechanism_geometry.h"
#include "weapon_reload_geometry.h"
#include "gnasher_lever_motion.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace gears_weapon {

// These identities belong to the same certified game-thread/XR observation.
// A recycled weapon pointer is insufficient to continue a physical stroke.
struct GnasherIdentity {
    std::uint64_t world{},pawn{},weapon{},world_generation{},session{},space_epoch{},
                  input_epoch{},profile_generation{};
    bool valid() const noexcept {
        return world&&pawn&&weapon&&world_generation&&session&&space_epoch&&
            input_epoch&&profile_generation;
    }
    bool operator==(GnasherIdentity const& b) const noexcept {
        return world==b.world&&pawn==b.pawn&&weapon==b.weapon&&
            world_generation==b.world_generation&&session==b.session&&
            space_epoch==b.space_epoch&&input_epoch==b.input_epoch&&
            profile_generation==b.profile_generation;
    }
};

// All contact transforms are external calibration inputs. The existing
// original pivot/axis and SG_Chamber travel do not establish an XR grip.
// In particular, neither the pawn hand-bone origin nor a default identity
// quaternion certifies the controller's contact point or resting orientation.
struct GnasherMechanicalConfig {
    std::uint64_t calibration_id{};
    bool lever_calibrated{},feed_calibrated{};
    std::uint32_t capacity{};
    std::int64_t game_ticks_per_second{}; // Authenticated game clock; 0 = unknown, ordinary only.
    LeverMount lever{};
    Quaternion right_grip_home_in_main{};
    Vector right_contact_in_grip{};
    FeedPortMount feed{};
    LeverTravel travel{original_gnasher_lever_travel};
    double maximum_grip_orientation_error_radians{.45};
    double maximum_hand_speed_metres_per_second{5.};
    double maximum_hand_rotation_radians_per_second{18.};
    double regrasp_angle_tolerance_radians{.02};
    bool valid() const noexcept {
        return calibration_id&&lever_calibrated&&capacity&&capacity<=10000&&
            game_ticks_per_second>=0&&game_ticks_per_second<=1000000000&&lever.valid()&&travel.valid()&&
            gears_vr::stereo_pose_valid({{},right_grip_home_in_main})&&
            gears_vr::finite(right_contact_in_grip)&&
            length(right_contact_in_grip)<.5&&
            std::isfinite(maximum_grip_orientation_error_radians)&&
            maximum_grip_orientation_error_radians>.01&&
            maximum_grip_orientation_error_radians<1.&&
            std::isfinite(maximum_hand_speed_metres_per_second)&&
            maximum_hand_speed_metres_per_second>.1&&
            maximum_hand_speed_metres_per_second<15.&&
            std::isfinite(maximum_hand_rotation_radians_per_second)&&
            maximum_hand_rotation_radians_per_second>.1&&
            maximum_hand_rotation_radians_per_second<40.&&
            std::isfinite(regrasp_angle_tolerance_radians)&&regrasp_angle_tolerance_radians>=0&&
            regrasp_angle_tolerance_radians<=.05&&
            (!feed_calibrated||feed.valid());
    }
};

// Reserve includes a round held by the left hand until its insertion is
// acknowledged. Merely grabbing/releasing a visual round never edits ammo.
struct GnasherInventory {
    GnasherIdentity identity{};
    std::uint64_t revision{};
    std::uint32_t loaded{},reserve{};
};
struct GnasherMechanicalSample {
    GnasherIdentity identity{};
    std::uint64_t sequence{};
    std::int64_t monotonic_ns{},game_tick{};
    Pose left_grip{},right_grip{};
    std::uint64_t left_round_serial{};
    bool left_tracked{},right_tracked{},left_front_held{},right_handle_held{};
    bool original_animation_active{},original_ready{};
    std::uint64_t support_grasp_token{}; // Actual shared support lease; never an XR counter.
    std::uint64_t reference_generation{};
};

// Shared geometry for the CURRENT world pair and producer-relative motion.
// A relative pair is mathematical calibration data, never a world pose/Frame.
inline bool gnasher_lever_geometry(GnasherMechanicalConfig const& config,
    Pose const& left,Pose const& right,Pose& main,double& angle) noexcept {
    if(!gears_vr::stereo_pose_valid(left)||!gears_vr::stereo_pose_valid(right))return false;
    main.orientation=gears_vr::normalized(gears_vr::multiply(left.orientation,
        gears_vr::inverse(config.lever.left_front_grip_in_main.orientation)));
    main.position=difference(left.position,rotate(main.orientation,config.lever.left_front_grip_in_main.position));
    const auto contact=sum(right.position,rotate(right.orientation,config.right_contact_in_grip));
    if(!lever_angle_from_hands(main,contact,config.lever,angle)||
       angle>config.travel.maximum_angle_radians)return false;
    const auto half=angle*.5;const auto axis=config.lever.hinge_axis_in_main;
    const Quaternion rotation{axis.x*std::sin(half),axis.y*std::sin(half),axis.z*std::sin(half),std::cos(half)};
    const auto expected=gears_vr::normalized(gears_vr::multiply(main.orientation,
        gears_vr::multiply(rotation,config.right_grip_home_in_main)));
    const auto a=gears_vr::normalized(expected),b=gears_vr::normalized(right.orientation);
    const auto dot=std::abs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w);
    return 2*std::acos((std::clamp)(dot,0.,1.))<=config.maximum_grip_orientation_error_radians;
}

// Adapter supplies this only after exact raw history AND current Native/
// reference/hold/support closure. It cannot authorize past poses or calls.
struct GnasherProducerMotion {
    GnasherIdentity identity{};
    std::uint64_t reference_generation{},support_grasp_token{};
    GnasherLeverMotion lever{};
};

inline bool gnasher_mechanical_sample_from_hands(
    gears_vr::ControllerInputFrame const& hands,gears_vr::StereoSample const& stereo,
    GnasherIdentity const& identity,std::int64_t now_ns,std::int64_t game_tick,
    bool left_front_held,bool right_handle_held,std::uint64_t left_round_serial,
    bool original_animation_active,bool original_ready,
    GnasherMechanicalSample& out,bool allow_sim=false) noexcept {
    if(!identity.valid()||identity.session!=hands.session||
       identity.space_epoch!=hands.space_epoch||identity.input_epoch!=hands.input_epoch||
       identity.profile_generation!=hands.profile_generation||game_tick<0||
       !gears_vr::controller_frame_matches(hands,stereo,now_ns,allow_sim))return false;
    out={identity,hands.sequence,hands.sync_ns,game_tick,hands.hands[0].grip,
        hands.hands[1].grip,left_round_serial,
        gears_vr::controller_pose_usable(hands.hands[0].grip,
            hands.hands[0].grip_flags,hands.hands[0].grip_active),
        gears_vr::controller_pose_usable(hands.hands[1].grip,
            hands.hands[1].grip_flags,hands.hands[1].grip_active),
        left_front_held,right_handle_held,original_animation_active,original_ready};
    return true;
}

enum class GnasherMechanicalStage : std::uint8_t {
    unbound,ready,feed_port,holding_round,inventory_commit,lever,cycle_handoff
};
enum class GnasherMechanicalEvent : std::uint8_t {
    none,rejected,invalidated,reload_started,round_grabbed,round_released,
    request_inventory_commit,inventory_committed,request_original_cycle,
    cycle_accepted,shot_recorded,reload_cancelled,animation_handoff,restored
};
struct GnasherMechanicalDecision {
    GnasherMechanicalEvent event{GnasherMechanicalEvent::none};
    ReloadEvent reload_event{ReloadEvent::rejected};
    std::uint64_t request_token{};
    bool drive_lever{},animation_owns_hands{},restore_6dof{},lever_returning{};
    double lever_angle_radians{};
    Pose main_pose{};
};
struct GnasherInventoryRequest {
    GnasherIdentity identity{};
    std::uint64_t token{},round_serial{},before_revision{};
    std::uint32_t before_loaded{},before_reserve{},after_loaded{},after_reserve{};
    std::uint64_t input_sequence{};std::int64_t physical_completion_ns{};
    std::uint64_t reference_generation{};
};
struct GnasherInventoryReceipt {
    std::uint64_t request_token{};
    bool accepted{};
    GnasherInventory observed{};
};
struct GnasherCycleRequest {
    GnasherIdentity identity{};
    std::uint64_t token{};
    std::int64_t physical_completion_game_tick{};
    MechanismAction action{MechanismAction::none};
    ReloadEvent completion{ReloadEvent::rejected}; // Proposal; reward only after cycle_accepted.
    std::uint64_t input_sequence{};std::int64_t physical_completion_ns{};
};
// An accepted handoff is not an animation-completion or ammunition receipt.
// Original SG_Chamber/reload notifications, firing, damage, recoil and sounds
// remain the game's responsibility. The adapter must suppress the original
// timed refill while a physical reload owns the transaction.
struct GnasherCycleReceipt {
    GnasherIdentity identity{};
    std::uint64_t request_token{},original_event_sequence{};
    bool accepted{};
};
struct GnasherShotReceipt {
    GnasherIdentity identity{};
    std::uint64_t original_event_sequence{};
    std::int64_t monotonic_ns{},game_tick{};
    GnasherInventory observed{}; // Same original event's actual post-shot counts.
};
struct GnasherShotAcceptance {
    GnasherIdentity identity{};std::uint64_t original_event_sequence{},source_sequence{};
    std::int64_t source_sync_ns{},source_admitted_ns{};
    GnasherInventory before{};bool accepted{};
};
struct GnasherReloadAcceptance {
    GnasherIdentity identity{};std::uint64_t source_sequence{};
    std::int64_t source_sync_ns{},source_admitted_ns{},original_start_game_tick{};
    GnasherInventory observed{};bool accepted{};
};

class GnasherMechanicalInteraction {
    GnasherIdentity identity_{};
    GnasherMechanicalConfig config_{};
    GnasherInventory inventory_{};
    GnasherLever lever_{};
    FeedInsertionStroke feed_{};
    GnasherInventoryRequest inventory_request_{};
    GnasherCycleRequest cycle_request_{};
    std::uint64_t next_token_{1},last_round_serial_{},held_round_{},sequence_{},
                  last_shot_event_{},last_cycle_event_{},last_support_token_{},last_reference_generation_{};
    std::int64_t last_ns_{},last_game_tick_{},reload_start_tick_{};
    Pose previous_left_{},previous_right_{};
    std::uint32_t target_{},inserted_{};
    unsigned observation_rejection_{}; // Last actual observe result; never admission.
    bool bound_{},reload_active_{},mechanism_pending_{},pose_continuity_{},feed_armed_{},
         tracked_{},ready_{},animation_owns_hands_{},waiting_recovery_{},close_requested_{},hold_suspended_{};

    GnasherMechanicalDecision result(GnasherMechanicalEvent event,
        ReloadEvent reload=ReloadEvent::rejected,std::uint64_t token=0) const noexcept {
        GnasherMechanicalDecision d{};
        d.event=event;d.reload_event=reload;d.request_token=token;
        d.animation_owns_hands=animation_owns_hands_||waiting_recovery_;return d;
    }
    bool identity_matches(GnasherIdentity const& identity) const noexcept {
        return bound_&&identity==identity_;
    }
    bool inventory_matches(GnasherInventory const& inventory) const noexcept {
        return identity_matches(inventory.identity)&&inventory.revision&&
            inventory.loaded==inventory_.loaded&&inventory.reserve==inventory_.reserve;
    }
    std::uint64_t token() noexcept {
        if(next_token_==std::numeric_limits<std::uint64_t>::max())return 0;
        return next_token_++;
    }
    void break_stroke() noexcept {lever_.reset();last_support_token_=0;feed_.reset();feed_armed_=false;}
    GnasherMechanicalDecision invalidate() noexcept {
        reset();return result(GnasherMechanicalEvent::invalidated);
    }
    static double orientation_distance(Quaternion a,Quaternion b) noexcept {
        const double norm=std::sqrt((a.x*a.x+a.y*a.y+a.z*a.z+a.w*a.w)*
            (b.x*b.x+b.y*b.y+b.z*b.z+b.w*b.w));
        const double cosine=std::abs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w)/norm;
        return 2*std::acos((std::clamp)(cosine,0.,1.));
    }
    bool continuous(Pose const& a,Pose const& b,double seconds) const noexcept {
        return length(difference(a.position,b.position))<=
                config_.maximum_hand_speed_metres_per_second*seconds&&
            orientation_distance(a.orientation,b.orientation)<=
                config_.maximum_hand_rotation_radians_per_second*seconds;
    }
    Pose main_from_right(Pose const& right) const noexcept {
        Pose main{};
        main.orientation=gears_vr::normalized(gears_vr::multiply(right.orientation,
            gears_vr::inverse(config_.right_grip_home_in_main)));
        const auto contact=sum(right.position,rotate(right.orientation,
            config_.right_contact_in_grip));
        main.position=difference(contact,rotate(main.orientation,
            config_.lever.right_handle_home_in_main));return main;
    }
    bool lever_geometry(GnasherMechanicalSample const& sample,
                        Pose& main,double& angle) const noexcept {
        return gnasher_lever_geometry(config_,sample.left_grip,sample.right_grip,main,angle);
    }

    bool fresh(std::int64_t now_ns) const noexcept {
        return tracked_&&last_ns_>0&&now_ns>=last_ns_&&
            now_ns-last_ns_<=config_.travel.maximum_sample_gap_ns;
    }
public:
    // Reset invalidates proposals. Tokens and round serials remain monotonic
    // across a rebind so a delayed receipt cannot satisfy a later request.
    void reset() noexcept {
        const auto next=next_token_,round=last_round_serial_;
        *this=GnasherMechanicalInteraction{};next_token_=next;last_round_serial_=round;
    }
    bool bind(GnasherIdentity const& identity,GnasherMechanicalConfig const& config,
              GnasherInventory const& inventory) noexcept {
        if(bound_||!identity.valid()||!config.valid()||!(inventory.identity==identity)||
           !inventory.revision||inventory.loaded>config.capacity||inventory.reserve>1000000)
            return false;
        identity_=identity;config_=config;inventory_=inventory;bound_=true;return true;
    }
    // Initialize a newly calibrated consumer from an ALREADY authenticated
    // original shot/count receipt. Never synthesize a pre-shot count or a
    // fresh tracking sample; the pending cycle survives the stock timer.
    bool bind_after_original_shot(GnasherMechanicalConfig const& config,
                                  GnasherShotReceipt const& receipt) noexcept {
        if(bound_||!receipt.original_event_sequence||receipt.monotonic_ns<=0||receipt.game_tick<0||
           !(receipt.identity==receipt.observed.identity)||!bind(receipt.identity,config,receipt.observed))return false;
        last_shot_event_=receipt.original_event_sequence;last_ns_=receipt.monotonic_ns;
        last_game_tick_=receipt.game_tick;mechanism_pending_=waiting_recovery_=true;
        return true;
    }
    // A failed controller-frame factory must call this too. It interrupts
    // gesture history without cancelling an original animation or refunding
    // already committed ammunition.
    void tracking_lost() noexcept {
        tracked_=false;pose_continuity_=false;lever_.suspend();feed_.reset();feed_armed_=false;
    }
    GnasherMechanicalStage stage() const noexcept {
        if(!bound_)return GnasherMechanicalStage::unbound;
        if(inventory_request_.token)return GnasherMechanicalStage::inventory_commit;
        if(cycle_request_.token)return GnasherMechanicalStage::cycle_handoff;
        if(reload_active_&&!close_requested_&&inserted_<target_)
            return held_round_?GnasherMechanicalStage::holding_round:
                GnasherMechanicalStage::feed_port;
        return mechanism_pending_?GnasherMechanicalStage::lever:GnasherMechanicalStage::ready;
    }
    GuidePart guide() const noexcept {
        switch(stage()){
            case GnasherMechanicalStage::feed_port:return GuidePart::left_shoulder;
            case GnasherMechanicalStage::holding_round:return GuidePart::feed_port;
            case GnasherMechanicalStage::lever:return GuidePart::lever_handle;
            default:return GuidePart::none;
        }
    }
    bool can_request_fire(std::int64_t now_ns) const noexcept {
        return bound_&&fresh(now_ns)&&ready_&&!reload_active_&&!mechanism_pending_&&
            !inventory_request_.token&&!cycle_request_.token&&!animation_owns_hands_&&
            !waiting_recovery_&&inventory_.loaded>0;
    }
    // The live fire adapter must also compare a fresh original inventory
    // readback. Animation readiness, the tracked pose and actual usable ammo
    // are independent requirements; a prior model count is not that readback.
    bool can_request_fire(std::int64_t now_ns,GnasherInventory const& observed) const noexcept {
        return inventory_matches(observed)&&observed.revision==inventory_.revision&&
            can_request_fire(now_ns);
    }
    // Read-only diagnostic of the independent original fire prerequisites.
    // Never consumes a sample or changes the normal admission predicate.
    unsigned observation_rejection() const noexcept {return observation_rejection_;}
    unsigned fire_readiness_mask(std::int64_t now_ns,GnasherInventory const& observed) const noexcept {
        return (bound_?1u:0u)|(tracked_?2u:0u)|
            ((last_ns_>0&&now_ns>=last_ns_&&now_ns-last_ns_<=config_.travel.maximum_sample_gap_ns)?4u:0u)|
            (ready_?8u:0u)|(!reload_active_?16u:0u)|(!mechanism_pending_?32u:0u)|
            (!inventory_request_.token?64u:0u)|(!cycle_request_.token?128u:0u)|
            (!animation_owns_hands_?256u:0u)|(!waiting_recovery_?512u:0u)|
            (inventory_.loaded>0?1024u:0u)|(inventory_matches(observed)?2048u:0u)|
            (observed.revision==inventory_.revision?4096u:0u);
    }
    bool animation_owns_hands() const noexcept {
        return animation_owns_hands_||waiting_recovery_;
    }
    GnasherInventory inventory() const noexcept {return inventory_;}
    bool source_matches(std::uint64_t sequence,std::int64_t sync_ns) const noexcept {
        return bound_&&sequence&&sequence==sequence_&&sync_ns>0&&sync_ns==last_ns_;
    }
    std::uint32_t capacity() const noexcept {return config_.capacity;}
    std::int64_t game_ticks_per_second() const noexcept {return config_.game_ticks_per_second;}
    bool reload_active() const noexcept {return reload_active_;}
    bool hold_suspended() const noexcept {return hold_suspended_;}
    std::int64_t reload_started_game_tick() const noexcept {return reload_start_tick_;}
    // Parking has no new gesture authority. Resolve dispatched inventory and
    // final-cycle proposals, and release the actual held item, before parking.
    // Keep the logical reload clock/counts and the frozen physical lever phase.
    bool suspend_hold() noexcept {
        if(!bound_||held_round_||inventory_request_.token||cycle_request_.token)return false;
        hold_suspended_=true;tracking_lost();ready_=false;return true;
    }
    bool resume_hold(GnasherInventory const& observed) noexcept {
        if(!hold_suspended_||!inventory_matches(observed)||observed.revision<inventory_.revision)return false;
        inventory_.revision=observed.revision;hold_suspended_=false;
        tracking_lost();ready_=false;return true; // No input, pose or time renewed.
    }
    std::uint32_t target() const noexcept {return target_;}
    std::uint32_t inserted() const noexcept {return inserted_;}
    std::uint64_t held_round_serial() const noexcept {return held_round_;}
    bool mechanism_pending() const noexcept {return mechanism_pending_;}
    GnasherInventoryRequest inventory_request() const noexcept {return inventory_request_;}
    GnasherCycleRequest cycle_request() const noexcept {return cycle_request_;}
    // The live owner may reject only a proposal that has not entered its
    // original readback/exit gate. This never pays the shot/reload debt or
    // changes target/counts/clock, and its consumed token cannot be reused.
    bool reject_unpublished_cycle(std::uint64_t token) noexcept {
        if(!token||cycle_request_.token!=token||inventory_request_.token)return false;
        cycle_request_={};break_stroke();return true;
    }

    GnasherMechanicalDecision begin_reload(GnasherIdentity const& identity,
        std::int64_t now_ns,std::int64_t game_tick) noexcept {
        if(!identity_matches(identity)||!fresh(now_ns)||!ready_||game_tick<last_game_tick_||
           reload_active_||inventory_request_.token||cycle_request_.token||
           animation_owns_hands()||!config_.feed_calibrated)
            return result(GnasherMechanicalEvent::rejected);
        if(inventory_.loaded==config_.capacity)
            return result(GnasherMechanicalEvent::rejected,ReloadEvent::full);
        if(!inventory_.reserve)
            return result(GnasherMechanicalEvent::rejected,ReloadEvent::no_ammo);
        target_=(std::min)({3u,config_.capacity-inventory_.loaded,inventory_.reserve});
        inserted_=0;close_requested_=false;reload_start_tick_=last_game_tick_=game_tick;reload_active_=true;
        break_stroke();return result(GnasherMechanicalEvent::reload_started,ReloadEvent::started);
    }
    // Receipt for the original action already admitted from THIS raw sample.
    // Dispatcher latency may delay this receipt; it never refreshes tracking
    // or moves the four-second start away from original ReloadStartTime.
    GnasherMechanicalDecision accept_original_reload(GnasherReloadAcceptance const& acceptance) noexcept {
        if(!acceptance.accepted||!identity_matches(acceptance.identity)||
           acceptance.source_sequence!=sequence_||acceptance.source_sync_ns!=last_ns_||
           !inventory_matches(acceptance.observed)||acceptance.observed.revision<inventory_.revision||
           acceptance.source_admitted_ns<acceptance.source_sync_ns||
           acceptance.source_admitted_ns-acceptance.source_sync_ns>100000000)
            return result(GnasherMechanicalEvent::rejected);
        auto next=*this;next.inventory_.revision=acceptance.observed.revision;
        const auto decision=next.begin_reload(acceptance.identity,acceptance.source_admitted_ns,
            acceptance.original_start_game_tick);
        if(decision.event==GnasherMechanicalEvent::reload_started)*this=next;
        return decision;
    }
    // Serial is a monotonically issued visual-ammo instance token, never an
    // object address. The adapter verifies the left-shoulder source and ammo
    // type before requesting this reservation.
    GnasherMechanicalDecision grab_round(GnasherIdentity const& identity,
        std::uint64_t serial,std::int64_t now_ns) noexcept {
        if(!identity_matches(identity)||stage()!=GnasherMechanicalStage::feed_port||
           !fresh(now_ns)||!ready_||animation_owns_hands()||!serial||serial<=last_round_serial_)
            return result(GnasherMechanicalEvent::rejected);
        held_round_=last_round_serial_=serial;feed_.reset();feed_armed_=false;
        return result(GnasherMechanicalEvent::round_grabbed,ReloadEvent::grabbed);
    }
    GnasherMechanicalDecision release_round(GnasherIdentity const& identity,
        std::uint64_t serial) noexcept {
        if(!identity_matches(identity)||!held_round_||held_round_!=serial||inventory_request_.token)
            return result(GnasherMechanicalEvent::rejected);
        held_round_=0;feed_.reset();feed_armed_=false;
        return result(GnasherMechanicalEvent::round_released,ReloadEvent::released);
    }
    // Called for an authenticated fresh support-grab intent, never passive snap.
    // Keep the original target/clock and reject unsettled held ammunition.
    GnasherMechanicalDecision request_close(GnasherIdentity const& identity,
        std::int64_t now_ns) noexcept {
        if(!identity_matches(identity)||!fresh(now_ns)||!ready_||!reload_active_||close_requested_||
           held_round_||inventory_request_.token||cycle_request_.token||animation_owns_hands())
            return result(GnasherMechanicalEvent::rejected);
        close_requested_=true;break_stroke();
        if(!mechanism_pending_){reload_active_=false;return result(GnasherMechanicalEvent::reload_cancelled,ReloadEvent::cancelled);}
        return result(GnasherMechanicalEvent::none,ReloadEvent::mechanism_needed);
    }
    GnasherMechanicalDecision cancel_reload(GnasherIdentity const& identity) noexcept {
        if(!identity_matches(identity)||!reload_active_||inventory_request_.token||cycle_request_.token)
            return result(GnasherMechanicalEvent::rejected);
        reload_active_=false;held_round_=0;break_stroke();
        return result(GnasherMechanicalEvent::reload_cancelled,ReloadEvent::cancelled);
    }
    GnasherMechanicalDecision acknowledge_inventory(GnasherInventoryReceipt const& receipt) noexcept {
        const auto request=inventory_request_;
        if(!request.token||request.token!=receipt.request_token||
           !identity_matches(receipt.observed.identity))
            return result(GnasherMechanicalEvent::rejected);
        if(!receipt.accepted){
            if(!inventory_matches(receipt.observed)||receipt.observed.revision<inventory_.revision)
                return invalidate();
            inventory_.revision=receipt.observed.revision;inventory_request_={};
            feed_.reset();feed_armed_=false;
            return result(GnasherMechanicalEvent::rejected);
        }
        if(receipt.observed.revision<=request.before_revision||
           receipt.observed.loaded!=request.after_loaded||
           receipt.observed.reserve!=request.after_reserve)return invalidate();
        inventory_=receipt.observed;inventory_request_={};held_round_=0;++inserted_;
        mechanism_pending_=true;break_stroke();
        return result(GnasherMechanicalEvent::inventory_committed,
            inserted_==target_?ReloadEvent::mechanism_needed:ReloadEvent::inserted);
    }
    GnasherMechanicalDecision accept_original_shot(GnasherShotReceipt const& receipt,
        GnasherShotAcceptance const& accepted) noexcept {
        if(!identity_matches(receipt.identity)||!identity_matches(receipt.observed.identity)||
           !receipt.original_event_sequence||receipt.original_event_sequence<=last_shot_event_)
            return result(GnasherMechanicalEvent::rejected);
        if(!accepted.accepted||!identity_matches(accepted.identity)||
           accepted.original_event_sequence!=receipt.original_event_sequence||
           !source_matches(accepted.source_sequence,accepted.source_sync_ns)||
           !inventory_matches(accepted.before)||accepted.before.revision!=inventory_.revision||
           accepted.source_admitted_ns<accepted.source_sync_ns||
           accepted.source_admitted_ns-accepted.source_sync_ns>100000000||
           receipt.monotonic_ns<accepted.source_admitted_ns)return result(GnasherMechanicalEvent::rejected);
        // A new authenticated original shot with divergent ammo or an illegal
        // gate means the model lost synchronization. Repeated old receipts
        // are harmless; a new unexpected shot must fail closed.
        // This is a receipt for an already admitted original call. Returning
        // late cannot renew input, but does not erase its genuine -1 either.
        if(!can_request_fire(accepted.source_admitted_ns)||receipt.game_tick<last_game_tick_||
           receipt.observed.revision<=inventory_.revision||
           receipt.observed.loaded!=inventory_.loaded-1||
           receipt.observed.reserve!=inventory_.reserve)
            return invalidate();
        inventory_=receipt.observed;last_shot_event_=receipt.original_event_sequence;
        last_game_tick_=receipt.game_tick;mechanism_pending_=true;ready_=false;
        waiting_recovery_=true;break_stroke();
        return result(GnasherMechanicalEvent::shot_recorded);
    }
    GnasherMechanicalDecision after_original_shot(GnasherShotReceipt const& receipt) noexcept {
        const GnasherShotAcceptance accepted{identity_,receipt.original_event_sequence,sequence_,last_ns_,
            receipt.monotonic_ns,inventory_,true};
        return accept_original_shot(receipt,accepted);
    }
    GnasherMechanicalDecision acknowledge_cycle(GnasherCycleReceipt const& receipt,
        GnasherInventory const& observed) noexcept {
        if(!identity_matches(receipt.identity)||!cycle_request_.token||
           receipt.request_token!=cycle_request_.token||!receipt.original_event_sequence||
           receipt.original_event_sequence<=last_cycle_event_)
            return result(GnasherMechanicalEvent::rejected);
        if(!inventory_matches(observed)||observed.revision<inventory_.revision)return invalidate();
        last_cycle_event_=receipt.original_event_sequence;inventory_.revision=observed.revision;
        const auto completion=cycle_request_.completion;cycle_request_={};break_stroke();
        ready_=false;waiting_recovery_=true;
        if(!receipt.accepted)return result(GnasherMechanicalEvent::rejected);
        mechanism_pending_=false;reload_active_=false;
        return result(GnasherMechanicalEvent::cycle_accepted,completion);
    }

    GnasherMechanicalDecision observe(GnasherMechanicalSample const& sample,
        GnasherProducerMotion const* motion=nullptr) noexcept {
        if(!bound_){observation_rejection_=1;return result(GnasherMechanicalEvent::rejected);}
        if(hold_suspended_){observation_rejection_=2;return result(GnasherMechanicalEvent::rejected);}
        if(!identity_matches(sample.identity)){auto d=invalidate();observation_rejection_=3;return d;}
        if(!sample.sequence||sample.sequence<=sequence_||sample.monotonic_ns<=0||
           sample.monotonic_ns<=last_ns_||sample.game_tick<last_game_tick_){
            observation_rejection_=4;tracking_lost();break_stroke();return result(GnasherMechanicalEvent::rejected);
        }
        const auto gap=last_ns_?sample.monotonic_ns-last_ns_:0;
        auto motion_trial=lever_;auto motion_result=GnasherMotionResult::rejected;
        if(motion&&pose_continuity_&&tracked_&&ready_&&gap>config_.travel.maximum_sample_gap_ns&&
           stage()==GnasherMechanicalStage::lever&&!animation_owns_hands_&&!waiting_recovery_&&
           !held_round_&&!inventory_request_.token&&!cycle_request_.token&&
           !sample.original_animation_active&&sample.original_ready&&
           sample.left_tracked&&sample.right_tracked&&sample.left_front_held&&sample.right_handle_held&&
           !sample.left_round_serial&&sample.support_grasp_token&&
           sample.support_grasp_token==last_support_token_&&
           motion->support_grasp_token==last_support_token_&&motion->identity==identity_&&
           last_reference_generation_&&sample.reference_generation==last_reference_generation_&&
           motion->reference_generation==last_reference_generation_&&
           motion->lever.count>=2&&motion->lever.count<=motion->lever.points.size()){
            auto const& first=motion->lever.points[0];
            auto const& tail=motion->lever.points[motion->lever.count-1];
            Pose main{};double angle{};
            if(first.sequence==sequence_&&first.monotonic_ns==last_ns_&&
               tail.sequence==sample.sequence&&tail.monotonic_ns==sample.monotonic_ns&&
               lever_geometry(sample,main,angle)&&std::abs(angle-tail.lever_angle_radians)<=.0002)
                motion_result=observe_gnasher_motion(motion_trial,config_.travel,motion->lever);
        }
        sequence_=sample.sequence;last_ns_=sample.monotonic_ns;last_game_tick_=sample.game_tick;
        last_reference_generation_=sample.reference_generation;
        if(sample.original_animation_active){
            observation_rejection_=5;const bool entered=!animation_owns_hands_;
            animation_owns_hands_=true;ready_=false;tracking_lost();break_stroke();
            return result(entered?GnasherMechanicalEvent::animation_handoff:
                GnasherMechanicalEvent::none);
        }
        const bool pose_valid=gears_vr::stereo_pose_valid(sample.left_grip)&&
            gears_vr::stereo_pose_valid(sample.right_grip);
        if(!pose_valid){observation_rejection_=6;tracking_lost();break_stroke();ready_=false;return result(GnasherMechanicalEvent::rejected);}
        const bool valid=sample.left_tracked&&sample.right_tracked&&sample.right_handle_held;
        if(!valid){observation_rejection_=7;tracking_lost();ready_=false;return result(GnasherMechanicalEvent::rejected);}
        if(pose_continuity_&&gap>config_.travel.maximum_sample_gap_ns&&
           motion_result==GnasherMotionResult::rejected){
            observation_rejection_=8;tracking_lost();ready_=false;
            // A new authenticated pose can baseline an idle loaded gun after
            // sparse GT consumption. This consumes no stroke, grant or debt.
            // Active mechanics still suspend below; fire independently checks
            // this sample's ORIGINAL timestamp and exact inventory readback.
            if(!reload_active_&&!mechanism_pending_&&!held_round_&&!inventory_request_.token&&
               !cycle_request_.token&&!animation_owns_hands_&&!waiting_recovery_&&
               inventory_.loaded>0&&sample.original_ready){
                previous_left_=sample.left_grip;previous_right_=sample.right_grip;
                pose_continuity_=tracked_=ready_=true;observation_rejection_=0;
                return result(GnasherMechanicalEvent::none);
            }
            return result(GnasherMechanicalEvent::rejected);
        }
        if(pose_continuity_&&!lever_.suspended()&&(!continuous(previous_left_,sample.left_grip,double(gap)/1e9)||
           !continuous(previous_right_,sample.right_grip,double(gap)/1e9))){
            observation_rejection_=9;tracking_lost();break_stroke();ready_=false;return result(GnasherMechanicalEvent::rejected);
        }
        observation_rejection_=0;
        previous_left_=sample.left_grip;previous_right_=sample.right_grip;
        pose_continuity_=tracked_=true;ready_=sample.original_ready;
        if(!ready_){break_stroke();return result(GnasherMechanicalEvent::none);}
        if(animation_owns_hands_||waiting_recovery_){
            animation_owns_hands_=waiting_recovery_=false;break_stroke();
            auto d=result(GnasherMechanicalEvent::restored);d.restore_6dof=true;return d;
        }
        if(inventory_request_.token||cycle_request_.token)
            return result(GnasherMechanicalEvent::none);
        if(stage()==GnasherMechanicalStage::holding_round){
            lever_.reset();
            if(!config_.feed_calibrated||sample.left_front_held||
               sample.left_round_serial!=held_round_){
                feed_.reset();feed_armed_=false;return result(GnasherMechanicalEvent::rejected);}
            if(!feed_armed_)feed_armed_=feed_.arm(identity_.weapon,held_round_,
                identity_.world,identity_.space_epoch);
            const FeedSample feed_sample{identity_.weapon,held_round_,identity_.world,
                identity_.space_epoch,sample.sequence,sample.monotonic_ns,
                main_from_right(sample.right_grip),sample.left_grip,true,true};
            const auto inserted=feed_.observe(feed_sample,config_.feed);
            if(inserted==FeedResult::rejected){
                feed_.reset();feed_armed_=false;return result(GnasherMechanicalEvent::rejected);}
            if(inserted!=FeedResult::inserted)return result(GnasherMechanicalEvent::none);
            feed_armed_=false;
            if(!inventory_.reserve||inventory_.loaded>=config_.capacity)return invalidate();
            const auto request_token=token();if(!request_token)return invalidate();
            inventory_request_={identity_,request_token,held_round_,inventory_.revision,
                inventory_.loaded,inventory_.reserve,inventory_.loaded+1,inventory_.reserve-1,
                sample.sequence,sample.monotonic_ns,sample.reference_generation};
            return result(GnasherMechanicalEvent::request_inventory_commit,
                ReloadEvent::inserted,request_token);
        }
        if(stage()!=GnasherMechanicalStage::lever){lever_.reset();return result(GnasherMechanicalEvent::none);}
        feed_.reset();
        if(!sample.left_front_held||sample.left_round_serial||!sample.support_grasp_token){
            lever_.suspend();return result(GnasherMechanicalEvent::rejected);}
        Pose main{};double angle{};
        if(!lever_geometry(sample,main,angle)){
            if(!lever_.suspended()){lever_.reset();last_support_token_=0;}
            return result(GnasherMechanicalEvent::rejected);}
        if(lever_.phase()!=LeverPhase::unarmed&&sample.support_grasp_token!=last_support_token_)lever_.suspend();
        const LeverPoseSample lever_sample{identity_.weapon,identity_.world,identity_.space_epoch,
            sample.sequence,sample.monotonic_ns,angle,true,true,true};
        if(lever_.suspended()){
            if(sample.support_grasp_token<=last_support_token_||
               !lever_.reattach(lever_sample,config_.travel,config_.regrasp_angle_tolerance_radians))
                return result(GnasherMechanicalEvent::rejected);
            last_support_token_=sample.support_grasp_token;
            // A regrasp itself never moves the mechanism or emits completion.
            return result(GnasherMechanicalEvent::none);
        }
        if(lever_.phase()==LeverPhase::unarmed){
            lever_.arm(identity_.weapon,identity_.world,identity_.space_epoch);
            last_support_token_=sample.support_grasp_token;
        }
        bool complete{};
        if(motion_result!=GnasherMotionResult::rejected){
            lever_=motion_trial;complete=motion_result==GnasherMotionResult::completed;
        }else complete=lever_.observe(lever_sample,config_.travel);
        auto d=result(GnasherMechanicalEvent::none);d.drive_lever=true;
        d.main_pose=main;d.lever_angle_radians=angle;
        d.lever_returning=complete||lever_.phase()==LeverPhase::return_home;
        if(!complete)return d;
        const auto request_token=token();if(!request_token)return invalidate();
        const auto completion=reload_active_?
            (inserted_==target_&&inserted_>0&&config_.game_ticks_per_second>0&&
             sample.game_tick>=reload_start_tick_&&
             sample.game_tick-reload_start_tick_<=4*config_.game_ticks_per_second?
                ReloadEvent::perfect:ReloadEvent::ordinary):ReloadEvent::cycled;
        cycle_request_={identity_,request_token,sample.game_tick,
            MechanismAction::lever_down_return,completion,sample.sequence,sample.monotonic_ns};
        d.event=GnasherMechanicalEvent::request_original_cycle;d.reload_event=completion;
        d.request_token=request_token;return d;
    }
};
}
