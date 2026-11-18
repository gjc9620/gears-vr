#pragma once
#include "weapon_lancer_blade_outline.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace gears_weapon {

// The original Lancer owns the execution, damage, sounds and recovery. This
// model owns only the VR starter gesture and the decision to hand a verified
// blade contact to the original melee state. The live adapter must suppress
// the original forward-facing CheckForMeleeTarget timer while it owns that
// state; otherwise the game can acquire a victim before physical contact.
enum class LancerSawPhase : std::uint8_t {
    idle, pulling, waiting_for_hold, running, waiting_for_execution, animation, deploying, rail_off
};
// An idle, actually observed release is input history, not a held action or
// cached pose. Repeated neutral callbacks must not erase it before the real
// next press. Tracking/input uncertainty or any active grasp still resets.
inline bool preserve_lancer_neutral_witness(bool tracked,bool known,bool down,
    bool has_grasp,LancerSawPhase phase) noexcept {
    return tracked&&known&&!down&&!has_grasp&&phase==LancerSawPhase::idle;
}
enum class LancerSawEvent : std::uint8_t {
    none, pull_began, request_original_hold, hold_started,
    request_original_acquire, animation_handoff, request_original_release,
    restored, interrupted
};
struct LancerSawDecision {
    LancerSawEvent event{LancerSawEvent::none};
    LancerSawPhase phase{LancerSawPhase::idle};
    std::uint64_t victim{};
    bool press_original_melee{},release_original_melee{};
    bool animation_owns_hands{},restore_6dof{},unexpected_original_target{};
};

// AcquiredVictim does not re-run these original checks. The game-thread
// adapter must take the same-tick pawn/weapon snapshot, call both original
// validators, and only then hand off the exact contacted victim once.
struct LancerSawOriginalTargetGate {
    bool authoritative{},attacker_alive{},original_hold_move{},
         attacker_not_victim{},no_current_victim{},target_live{},
         target_controller_valid{},pawn_target_valid{},weapon_target_valid{},
         adhesion_clear_or_same{},blade_path_unobstructed{};
    bool valid() const noexcept {
        return authoritative&&attacker_alive&&original_hold_move&&
            attacker_not_victim&&no_current_victim&&target_live&&
            target_controller_valid&&pawn_target_valid&&weapon_target_valid&&
            adhesion_clear_or_same&&blade_path_unobstructed;
    }
};

// Positions and radii here are original game-world units. The caller must
// verify the current gun component and use its same-tick transform to build
// the visible chain outline. The two chainsaw bone pivots sit *inside* the
// saw housing and must never be substituted for its cutting edge. The target
// capsule and original validity/occlusion results come from the game thread.
struct LancerSawContact {
    std::uint64_t victim{};
    LancerBladeOutline blade{};
    Vector target_a_raw{},target_b_raw{};
    double blade_radius_raw{1.5},target_radius_raw{};
    LancerSawOriginalTargetGate original{};
};

// XR local positions are in metres. This sample must already be tied to the
// same certified XR ticket as both hands, and the game state must be read on
// the game thread. A pull cannot be inferred from a single spatial overlap.
struct LancerSawSample {
    std::uint64_t world{},pawn{},weapon{},session{},space_epoch{},input_epoch{},
                  world_generation{},sequence{},created_ms{};
    // Optional ACTUAL XR continuity companion, never inferred from consumer
    // sequence gaps. The live adapter still validates this sample's original
    // acquisition/input age at <=100ms before constructing it.
    std::uint64_t producer_stream{},tracking_epoch[2]{};
    Vector left_palm_m{},starter_anchor_m{},pull_axis_m{1,0,0};
    // Original bone4 is a hinged lever. These are actual same-frame geometry,
    // never an angle converted into an invented linear hand position.
    Vector starter_pivot_m{};
    Vector starter_grasp_m{};
    double starter_actual_angle{};
    double starter_actual_pull_m{},starter_travel_m{};
    Vector rail_hand_m{}; // Physical grip relative to the same current weapon, in metres.
    bool original_handle{},original_slide{},local_rail{},require_full_slide{};
    // Current adapter-owned geometry witness only; never an input/grant.
    bool authored_acquisition_enabled{},authored_acquisition_contact{};
    bool lancer_active{},right_owns_weapon{},right_tracked{},left_tracked{},
         left_grip_down{},game_can_melee{},original_hold{},
         original_execution{},original_ready{},original_button_held{};
    std::uint64_t original_victim{};
    LancerSawContact contact{};
};

// Input-only continuity of an ALREADY acquired physical pull. It carries no
// hand position, blade contact, target or new action permission. The live
// adapter obtains it from the existing exact-frame authenticated reader.
struct LancerSawInputContinuity {
    std::uint64_t world{},pawn{},weapon{},session{},space_epoch{},input_epoch{},world_generation{},
        producer_stream{},tracking_epoch[2]{},publisher_pid{},publisher_start{},profile{},
        sequence{},created_ms{},observed_ms{};
    std::uint32_t presses{},releases{};
    bool focused{},left_tracked{},right_tracked{},left_grip_down{};
};

inline double closest_segment_points(Vector p0,Vector p1,Vector q0,Vector q1,
    Vector& on_p,Vector& on_q) noexcept {
    const auto u=difference(p1,p0),v=difference(q1,q0),w=difference(p0,q0);
    const double a=dot(u,u),b=dot(u,v),c=dot(v,v),d=dot(u,w),e=dot(v,w);
    const double epsilon=1e-9;
    double s=0,t=0;
    if(a<=epsilon&&c<=epsilon){on_p=p0;on_q=q0;return dot(w,w);}
    if(a<=epsilon)t=(std::clamp)(e/c,0.,1.);
    else if(c<=epsilon)s=(std::clamp)(-d/a,0.,1.);
    else {
        const double denom=a*c-b*b;
        if(denom>epsilon)s=(std::clamp)((b*e-c*d)/denom,0.,1.);
        t=(b*s+e)/c;
        if(t<0){t=0;s=(std::clamp)(-d/a,0.,1.);}
        else if(t>1){t=1;s=(std::clamp)((b-d)/a,0.,1.);}
    }
    const auto gap=difference(sum(w,scaled(u,s)),scaled(v,t));
    on_p=sum(p0,scaled(u,s));on_q=sum(q0,scaled(v,t));
    return dot(gap,gap);
}
inline double segment_distance_squared(Vector p0,Vector p1,Vector q0,Vector q1) noexcept {
    Vector on_p{},on_q{};return closest_segment_points(p0,p1,q0,q1,on_p,on_q);
}
inline bool lancer_saw_blade_geometry(LancerSawContact const& contact) noexcept {
    if(!contact.victim||!contact.blade.verified_current_mesh||
       !gears_vr::finite(contact.target_a_raw)||
       !gears_vr::finite(contact.target_b_raw)||
       !std::isfinite(contact.blade_radius_raw)||
       !std::isfinite(contact.target_radius_raw)||
       contact.blade_radius_raw<0||contact.blade_radius_raw>20||
       contact.target_radius_raw<=0||contact.target_radius_raw>200)
        return false;
    const double radius=contact.blade_radius_raw+contact.target_radius_raw;
    for(std::size_t i=0;i<contact.blade.world_raw.size();++i){
        const auto a=contact.blade.world_raw[i];
        const auto b=contact.blade.world_raw[(i+1)%contact.blade.world_raw.size()];
        if(!gears_vr::finite(a)||!gears_vr::finite(b)||
           length(difference(a,b))<.5||length(difference(a,b))>80)
            return false;
    }
    for(std::size_t i=0;i<contact.blade.world_raw.size();++i){
        const auto a=contact.blade.world_raw[i];
        const auto b=contact.blade.world_raw[(i+1)%contact.blade.world_raw.size()];
        if(segment_distance_squared(a,b,contact.target_a_raw,
                                    contact.target_b_raw)<=radius*radius)
            return true;
    }
    return false;
}
inline bool lancer_saw_blade_contact(LancerSawContact const& contact) noexcept {
    return contact.original.valid()&&lancer_saw_blade_geometry(contact);
}

class LancerChainsawInteraction {
    LancerSawPhase phase_{LancerSawPhase::idle};
    std::uint64_t world_{},pawn_{},weapon_{},session_{},space_epoch_{},
                  input_epoch_{},world_generation_{},sequence_{},last_ms_{},
                  phase_start_ms_{},victim_{};
    std::uint64_t producer_stream_{},tracking_epoch_[2]{};
    std::uint64_t input_sequence_{},input_ms_{},input_geometry_begin_ms_{},input_publisher_pid_{},
        input_publisher_start_{},input_profile_{},bridged_samples_{},bridged_input_polls_{};
    std::uint32_t input_presses_{},input_releases_{};
    bool input_chain_{};
    Vector start_left_{},start_anchor_{},start_relative_{},start_axis_{},
           last_left_{},last_anchor_{};
    bool previous_left_grip_{},owns_original_melee_{},release_requested_{};
    double pull_metres_{},handle_angle_{},initial_handle_angle_{},initial_slide_{},lowest_slide_{};
    bool original_handle_{},original_slide_{},local_rail_{};
    Vector start_rail_{};
    Vector initial_radial_{};

    void clear() noexcept {*this={};}
    bool identity_matches(LancerSawSample const& s) const noexcept {
        return s.world==world_&&s.pawn==pawn_&&s.weapon==weapon_&&
            s.session==session_&&s.space_epoch==space_epoch_&&
            s.input_epoch==input_epoch_&&s.world_generation==world_generation_&&
            s.producer_stream==producer_stream_&&s.tracking_epoch[0]==tracking_epoch_[0]&&s.tracking_epoch[1]==tracking_epoch_[1];
    }
    void bind(LancerSawSample const& s) noexcept {
        world_=s.world;pawn_=s.pawn;weapon_=s.weapon;session_=s.session;
        space_epoch_=s.space_epoch;input_epoch_=s.input_epoch;
        world_generation_=s.world_generation;
        producer_stream_=s.producer_stream;tracking_epoch_[0]=s.tracking_epoch[0];tracking_epoch_[1]=s.tracking_epoch[1];
    }
    LancerSawDecision interrupted() noexcept {
        const bool active=phase_!=LancerSawPhase::idle;
        const bool release=owns_original_melee_;
        clear();
        if(!active)return {};
        return {LancerSawEvent::interrupted,LancerSawPhase::idle,0,false,release};
    }
    bool rail_position(LancerSawSample const& s,double& travel,double& lateral) const noexcept {
        if(s.local_rail!=local_rail_)return false;
        const auto relative=local_rail_?difference(s.rail_hand_m,start_rail_):
            difference(difference(s.left_palm_m,s.starter_anchor_m),start_relative_);
        travel=local_rail_?relative.y:dot(relative,start_axis_);
        lateral=local_rail_?std::hypot(relative.x,relative.z):length(difference(relative,scaled(start_axis_,travel)));
        travel+=initial_slide_;
        // Sliding socket: project the current real hand onto its acquired
        // rail, rather than measuring a sphere from the old grab origin.
        // Axial overtravel never releases this held part. The real mechanism
        // stays clamped; radial departure, release and source guards remain.
        return gears_vr::finite(relative)&&std::isfinite(travel)&&std::isfinite(lateral)&&
            lateral<=.18;
    }
public:
    void reset() noexcept {clear();}
    LancerSawPhase phase() const noexcept {return phase_;}
    double pull_metres() const noexcept {return pull_metres_;}
    double handle_angle() const noexcept {return handle_angle_;}
    std::uint64_t bridged_samples() const noexcept {return bridged_samples_;}
    std::uint64_t bridged_input_polls() const noexcept {return bridged_input_polls_;}
    LancerSawDecision observe_input(LancerSawInputContinuity const& w,bool producer_continuity_verified=false) noexcept {
        if(phase_!=LancerSawPhase::pulling&&phase_!=LancerSawPhase::deploying&&
           !(original_slide_&&(phase_==LancerSawPhase::waiting_for_hold||phase_==LancerSawPhase::running||phase_==LancerSawPhase::rail_off)))return {};
        if(!w.publisher_pid||!w.publisher_start||!w.profile||!w.sequence||!w.created_ms||
           w.observed_ms<w.created_ms||w.observed_ms-w.created_ms>100||
           !w.producer_stream||!w.tracking_epoch[0]||!w.tracking_epoch[1]||
           w.world!=world_||w.pawn!=pawn_||w.weapon!=weapon_||w.session!=session_||
           w.space_epoch!=space_epoch_||w.input_epoch!=input_epoch_||w.world_generation!=world_generation_||
           w.producer_stream!=producer_stream_||w.tracking_epoch[0]!=tracking_epoch_[0]||w.tracking_epoch[1]!=tracking_epoch_[1]||
           !w.focused||!w.left_tracked||!w.right_tracked||!w.left_grip_down||
           w.presses<=w.releases||w.presses-w.releases!=1||
           w.created_ms<phase_start_ms_||(!original_slide_&&w.created_ms-phase_start_ms_>1500))return interrupted();
        // The live adapter may separately authenticate the already-acquired
        // hold against the actual XR producer's per-sync stream/tracking
        // epochs and unchanged press/release counters. That proof distinguishes
        // a slow GAME consumer from an XR publication/tracking gap. It supplies
        // no pose or progress; every current-input age/key check above remains.
        const bool consumer_bridge=original_slide_&&producer_continuity_verified;
        const auto previous_input_ms=input_chain_?input_ms_:last_ms_;
        if(!input_chain_){
            if(w.created_ms<last_ms_)return {}; // older than the last real geometry; not a bridge
            if(w.created_ms-last_ms_>250&&!consumer_bridge)return interrupted();
            input_chain_=true;input_geometry_begin_ms_=last_ms_;
            input_publisher_pid_=w.publisher_pid;input_publisher_start_=w.publisher_start;input_profile_=w.profile;
            input_presses_=w.presses;input_releases_=w.releases;
        }else{
            if(w.publisher_pid!=input_publisher_pid_||w.publisher_start!=input_publisher_start_||w.profile!=input_profile_||
               w.presses!=input_presses_||w.releases!=input_releases_||w.sequence<input_sequence_||w.created_ms<input_ms_||
               (w.created_ms-input_ms_>250&&!consumer_bridge))return interrupted();
            if(w.sequence==input_sequence_){
                if(w.created_ms!=input_ms_)return interrupted();
                return {}; // observing the same old input does not renew it
            }
            if(w.created_ms==input_ms_)return interrupted();
        }
        if(consumer_bridge&&w.created_ms-previous_input_ms>250)++bridged_input_polls_;
        input_sequence_=w.sequence;input_ms_=w.created_ms;
        return {}; // no gesture progress, new grasp, original B, shot or inventory change
    }
    bool animation_owns_hands() const noexcept {
        return phase_==LancerSawPhase::animation;
    }
    bool suppress_original_auto_target() const noexcept {
        return owns_original_melee_&&
            (phase_==LancerSawPhase::waiting_for_hold||
             phase_==LancerSawPhase::running||
             phase_==LancerSawPhase::waiting_for_execution);
    }

    LancerSawDecision observe(LancerSawSample const& s,bool committed_new_grasp=false) noexcept {
        // Keep the original paired animation in charge through an XR tracking
        // outage or recenter. Releasing B at the entry edge can call the
        // original EndFire and cancel the attack. Wait until the original
        // melee state and attack special move have both exited, then release
        // our button once and wait for the game to confirm it is clear.
        if(phase_==LancerSawPhase::animation){
            if(!s.world||s.world!=world_||s.pawn!=pawn_||s.weapon!=weapon_)
                return interrupted();
            if(s.original_ready&&!s.original_execution&&!s.original_hold){
                if(owns_original_melee_){
                    if(s.original_button_held){
                        if(!release_requested_){
                            release_requested_=true;
                            return {LancerSawEvent::request_original_release,
                                    phase_,victim_,false,true,true};
                        }
                        return {LancerSawEvent::none,phase_,victim_,false,false,true};
                    }
                    owns_original_melee_=false;
                    release_requested_=false;
                }
                if(!s.original_button_held){
                    if(!s.lancer_active||!s.right_owns_weapon){
                        clear();
                        return {LancerSawEvent::interrupted,LancerSawPhase::idle};
                    }
                    if(s.right_tracked){
                        clear();
                        return {LancerSawEvent::restored,LancerSawPhase::idle,0,
                                false,false,false,true};
                    }
                }
            }
            return {LancerSawEvent::none,phase_,victim_,false,false,true};
        }
        // An original attack may begin in the same game tick that XR tracking
        // expires or recenters. The game-owned paired animation wins that
        // race; its identity is checked against the already bound game actors.
        if(s.original_execution&&phase_!=LancerSawPhase::idle){
            if(s.world!=world_||s.pawn!=pawn_||s.weapon!=weapon_)
                return interrupted();
            const bool unexpected=phase_!=LancerSawPhase::waiting_for_execution||
                (s.original_victim&&s.original_victim!=victim_);
            phase_=LancerSawPhase::animation;
            if(s.original_victim)victim_=s.original_victim;
            return {LancerSawEvent::animation_handoff,phase_,victim_,false,
                    false,true,false,unexpected};
        }
        const bool witnessed=s.producer_stream&&s.tracking_epoch[0]&&s.tracking_epoch[1];
        if((phase_==LancerSawPhase::waiting_for_hold||phase_==LancerSawPhase::running)&&
           (!s.left_tracked||!s.left_grip_down))return interrupted();
        const bool partial_witness=(s.producer_stream||s.tracking_epoch[0]||s.tracking_epoch[1])&&!witnessed;
        if(partial_witness||!s.world||!s.pawn||!s.weapon||!s.session||!s.space_epoch||
           !s.input_epoch||!s.world_generation||!s.sequence||!s.created_ms||
           !s.lancer_active||!s.right_owns_weapon||!s.right_tracked||
           ((phase_==LancerSawPhase::idle||phase_==LancerSawPhase::pulling||phase_==LancerSawPhase::deploying||
             (original_slide_&&(phase_==LancerSawPhase::waiting_for_hold||phase_==LancerSawPhase::running||phase_==LancerSawPhase::rail_off)))&&
            (!gears_vr::finite(s.left_palm_m)||
             !gears_vr::finite(s.starter_anchor_m)||
             !gears_vr::finite(s.pull_axis_m)||
             std::abs(length(s.pull_axis_m)-1.)>.1||
             (s.original_handle&&(!gears_vr::finite(s.starter_pivot_m)||
              !std::isfinite(s.starter_actual_angle)||s.starter_actual_angle<-.03||s.starter_actual_angle>1.60))||
             (s.original_slide&&(!s.original_handle||!gears_vr::finite(s.starter_grasp_m)||
              (s.local_rail&&!gears_vr::finite(s.rail_hand_m))||!std::isfinite(s.starter_travel_m)||
              s.starter_travel_m<.01||s.starter_travel_m>.3||!std::isfinite(s.starter_actual_pull_m)||
              s.starter_actual_pull_m<-.002||s.starter_actual_pull_m>s.starter_travel_m+.003)))))
            return interrupted();
        const bool first_sample=!world_;
        if(first_sample){
            bind(s);
            // Native's actual new part grant is an independent grab event,
            // not an invented released input. Ordinary cold-held samples
            // still establish history only and never start a gesture.
            if(!committed_new_grasp){
                sequence_=s.sequence;last_ms_=s.created_ms;
                previous_left_grip_=s.left_grip_down;return {};
            }
        }
        // Slow game callbacks are not producer tracking loss. The companion
        // advances on missing source frames or actual hand-tracking loss and
        // is identity-matched above. Permit bounded consumer spacing ONLY
        // with that witness; retain the legacy100ms spacing otherwise.
        const bool input_bridge=witnessed&&(phase_==LancerSawPhase::pulling||phase_==LancerSawPhase::deploying||
            (original_slide_&&(phase_==LancerSawPhase::waiting_for_hold||phase_==LancerSawPhase::running||phase_==LancerSawPhase::rail_off)))&&input_chain_&&
            input_geometry_begin_ms_<=last_ms_&&input_ms_>=last_ms_&&
            (input_ms_<=s.created_ms?s.created_ms-input_ms_<=250:input_ms_-s.created_ms<=100);
        if(!first_sample){
            if(!identity_matches(s)||s.sequence<sequence_||s.created_ms<last_ms_||
               (s.created_ms-last_ms_>(witnessed?250u:100u)&&!input_bridge))return interrupted();
            if(s.sequence==sequence_)return {LancerSawEvent::none,phase_};
            if(s.created_ms==last_ms_)return interrupted();
            if(s.created_ms-last_ms_>250&&input_bridge)++bridged_samples_;
        }
        const double frame_seconds=first_sample?0.:double(s.created_ms-last_ms_)/1000.;
        sequence_=s.sequence;last_ms_=s.created_ms;
        const bool grip_pressed=s.left_grip_down&&(!previous_left_grip_||committed_new_grasp);
        previous_left_grip_=s.left_grip_down;

        if(phase_==LancerSawPhase::idle){
            if(grip_pressed&&s.left_tracked&&s.game_can_melee&&
               !s.original_hold&&!s.original_execution&&
               (s.original_slide&&s.authored_acquisition_enabled?s.authored_acquisition_contact:
                length(difference(s.left_palm_m,s.original_slide?s.starter_grasp_m:s.starter_anchor_m))<=(s.original_slide?.14:.07))){
                phase_=LancerSawPhase::pulling;phase_start_ms_=s.created_ms;
                start_left_=s.left_palm_m;start_anchor_=s.starter_anchor_m;
                start_relative_=difference(s.left_palm_m,s.starter_anchor_m);
                start_axis_=s.pull_axis_m;last_left_=s.left_palm_m;
                last_anchor_=s.starter_anchor_m;
                original_handle_=s.original_handle;
                original_slide_=s.original_slide;
                local_rail_=s.local_rail;start_rail_=s.rail_hand_m;
                if(original_slide_){
                    // Grip release now locks the actual slider. A new grasp
                    // starts at THAT position, never teleports it to rest.
                    initial_slide_=lowest_slide_=pull_metres_=s.starter_actual_pull_m;
                    if(s.starter_actual_angle<1.56)phase_=LancerSawPhase::deploying;
                }else if(original_handle_){
                    initial_radial_=difference(s.left_palm_m,s.starter_pivot_m);
                    const auto radial=difference(initial_radial_,scaled(start_axis_,dot(initial_radial_,start_axis_)));
                    if(std::abs(s.starter_actual_angle)>.03||length(radial)<.04||length(radial)>.13)return interrupted();
                    const auto rest=difference(s.starter_anchor_m,s.starter_pivot_m);
                    const auto cross=Vector{rest.y*radial.z-rest.z*radial.y,rest.z*radial.x-rest.x*radial.z,rest.x*radial.y-rest.y*radial.x};
                    initial_handle_angle_=std::atan2(dot(cross,start_axis_),dot(rest,radial));
                }
                return {LancerSawEvent::pull_began,phase_};
            }
            return {};
        }
        if(phase_==LancerSawPhase::deploying){
            if(!s.original_slide||!s.left_tracked||!s.left_grip_down||!s.game_can_melee||
               std::abs(s.starter_actual_pull_m-initial_slide_)>.003)return interrupted();
            // The REAL new Grip acquired the folded bar. Its unfolding is
            // cosmetic setup on an ALREADY authenticated grasp. Do not demand
            // that the player chase its animated end with a second2.5cm grab.
            // Keep the ORIGINAL physical grasp baseline. A normal player
            // pushes while the cosmetic bar unfolds; resetting here erased
            // that genuine motion and required a second push. Cosmetic
            // unfolding itself still changes none of these physical values.
            if(s.starter_actual_angle>=1.56){
                phase_=LancerSawPhase::pulling;
                last_left_=s.left_palm_m;last_anchor_=s.starter_anchor_m;
            }
            return {LancerSawEvent::none,phase_};
        }
        if(phase_==LancerSawPhase::pulling||phase_==LancerSawPhase::rail_off){
            if(phase_==LancerSawPhase::rail_off&&s.original_button_held)return {LancerSawEvent::none,phase_};
            if(s.original_handle!=original_handle_||s.original_slide!=original_slide_||!s.left_tracked||!s.left_grip_down||!s.game_can_melee||
               (!original_slide_&&s.created_ms-phase_start_ms_>1500)||
               (!local_rail_&&(dot(s.pull_axis_m,start_axis_)<.94||length(difference(s.starter_anchor_m,start_anchor_))>.10))||
               length(difference(s.left_palm_m,last_left_))>3.*frame_seconds||
               length(difference(s.starter_anchor_m,last_anchor_))>
                   1.5*frame_seconds)
                return interrupted();
            last_left_=s.left_palm_m;last_anchor_=s.starter_anchor_m;
            if(original_slide_){
                double travel{},lateral{};if(!rail_position(s,travel,lateral))return interrupted();
                const double required=s.starter_travel_m*(s.require_full_slide?.98:.625);
                // A quicker push waits for actual mechanical travel and
                // dwell; it must not discard a legitimate retained grasp.
                if(lateral>.18)return interrupted();
                pull_metres_=std::clamp(travel,0.,s.starter_travel_m);
                lowest_slide_=(std::min)(lowest_slide_,(std::min)(pull_metres_,s.starter_actual_pull_m));
                const double required_new=(std::min)(required*.7,(std::max)(.01,required-initial_slide_));
                const double physical_forward=local_rail_?travel-lowest_slide_:
                    dot(difference(s.left_palm_m,start_left_),start_axis_);
                // Physical forward travel AND the independently evaluated
                // C-bone must reach the on gate. Unfolding cannot supply it.
                if(travel>=required&&travel-lowest_slide_>=required_new&&
                   physical_forward>=required_new&&
                   s.starter_actual_pull_m-lowest_slide_>=required_new&&
                   s.starter_actual_pull_m>=required&&s.created_ms-phase_start_ms_>=160){
                    phase_=LancerSawPhase::waiting_for_hold;phase_start_ms_=s.created_ms;owns_original_melee_=true;
                    return {LancerSawEvent::request_original_hold,phase_,0,true};
                }
                return {LancerSawEvent::none,phase_};
            }
            if(original_handle_){
                const auto current=difference(s.left_palm_m,s.starter_pivot_m);
                const auto a=difference(initial_radial_,scaled(start_axis_,dot(initial_radial_,start_axis_)));
                const auto b=difference(current,scaled(s.pull_axis_m,dot(current,s.pull_axis_m)));
                const auto rest=difference(s.starter_anchor_m,s.starter_pivot_m);
                const auto cross=Vector{rest.y*b.z-rest.z*b.y,rest.z*b.x-rest.x*b.z,rest.x*b.y-rest.y*b.x};
                const double angle=std::atan2(dot(cross,s.pull_axis_m),dot(rest,b))-initial_handle_angle_;
                if(std::abs(length(a)-length(b))>.03||
                   std::abs(dot(current,s.pull_axis_m)-dot(initial_radial_,start_axis_))>.03||
                   angle<-.08||angle>1.60)return interrupted();
                handle_angle_=std::clamp(angle,0.,1.5707963267948966);
                if(handle_angle_>=1.30&&s.created_ms-phase_start_ms_<160)return interrupted();
                // Hand travel AND the original evaluated bone must reach the
                // deployed lever. Submitted display commands cannot start B.
                if(handle_angle_>=1.30&&s.starter_actual_angle>=1.27&&s.created_ms-phase_start_ms_>=160){
                    phase_=LancerSawPhase::waiting_for_hold;phase_start_ms_=s.created_ms;owns_original_melee_=true;
                    return {LancerSawEvent::request_original_hold,phase_,0,true};
                }
                return {LancerSawEvent::none,phase_};
            }
            const auto relative=difference(
                difference(s.left_palm_m,s.starter_anchor_m),start_relative_);
            const double travel=dot(relative,start_axis_);
            const double hand_travel=dot(difference(s.left_palm_m,start_left_),
                                         start_axis_);
            const double lateral=length(difference(relative,scaled(start_axis_,travel)));
            if(travel<-.03||lateral>.08||hand_travel<-.03)
                return interrupted();
            if(travel>=.18&&s.created_ms-phase_start_ms_<160)
                return interrupted();
            pull_metres_=(std::clamp)(travel,0.,.3);
            if(travel>=.18&&hand_travel>=.14&&
               s.created_ms-phase_start_ms_>=160){
                phase_=LancerSawPhase::waiting_for_hold;
                phase_start_ms_=s.created_ms;owns_original_melee_=true;
                return {LancerSawEvent::request_original_hold,phase_,0,true};
            }
            return {LancerSawEvent::none,phase_};
        }
        if(original_slide_&&(phase_==LancerSawPhase::waiting_for_hold||phase_==LancerSawPhase::running)){
            double travel{},lateral{};
            if(!s.left_tracked||!s.left_grip_down||!rail_position(s,travel,lateral))return interrupted();
            pull_metres_=std::clamp(travel,0.,s.starter_travel_m);
            if(travel<=s.starter_travel_m*.30&&s.starter_actual_pull_m<=s.starter_travel_m*.33){
                phase_=LancerSawPhase::rail_off;phase_start_ms_=s.created_ms;owns_original_melee_=false;
                last_left_=s.left_palm_m;last_anchor_=s.starter_anchor_m;
                return {LancerSawEvent::request_original_release,phase_,0,false,true};
            }
        }
        if(phase_==LancerSawPhase::waiting_for_hold){
            if(s.original_hold){
                phase_=LancerSawPhase::running;
                return {LancerSawEvent::hold_started,phase_};
            }
            if(s.created_ms-phase_start_ms_>700)return interrupted();
            return {LancerSawEvent::none,phase_};
        }
        if(phase_==LancerSawPhase::running){
            // CanDoSpecialMeleeAttack is an entry gate. It may turn false
            // once the original weapon is already in its held melee state.
            if(!s.original_hold)return interrupted();
            if(lancer_saw_blade_contact(s.contact)){
                victim_=s.contact.victim;
                phase_=LancerSawPhase::waiting_for_execution;
                phase_start_ms_=s.created_ms;
                return {LancerSawEvent::request_original_acquire,phase_,victim_};
            }
            return {LancerSawEvent::none,phase_};
        }
        if(phase_==LancerSawPhase::waiting_for_execution){
            // The original hold can end one update before the paired attack
            // becomes observable; do not seize the hands during that gap.
            if(s.original_ready||s.created_ms-phase_start_ms_>700)
                return interrupted();
            return {LancerSawEvent::none,phase_,victim_};
        }
        return interrupted();
    }
};

} // namespace gears_weapon
