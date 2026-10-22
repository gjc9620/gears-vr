#pragma once
#include "weapon_pose_solver.h"
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
    idle, pulling, waiting_for_hold, running, waiting_for_execution, animation
};
enum class LancerSawEvent : std::uint8_t {
    none, pull_began, request_original_hold, hold_started,
    request_original_acquire, animation_handoff, restored, interrupted
};
struct LancerSawDecision {
    LancerSawEvent event{LancerSawEvent::none};
    LancerSawPhase phase{LancerSawPhase::idle};
    std::uint64_t victim{};
    bool press_original_melee{},release_original_melee{};
    bool animation_owns_hands{},restore_6dof{},unexpected_original_target{};
};

// Positions and radii here are original game-world units. The caller derives
// blade endpoints from the current Lancer mesh bones and the target capsule
// from the original collision/validity query. A view-center enemy is not a
// contact merely because it is visible.
struct LancerSawContact {
    std::uint64_t victim{};
    Vector blade_base_raw{},blade_tip_raw{},target_a_raw{},target_b_raw{};
    double blade_radius_raw{6},target_radius_raw{};
    bool original_target_valid{},unobstructed{};
};

// XR local positions are in metres. This sample must already be tied to the
// same certified XR ticket as both hands, and the game state must be read on
// the game thread. A pull cannot be inferred from a single spatial overlap.
struct LancerSawSample {
    std::uint64_t world{},pawn{},weapon{},session{},space_epoch{},input_epoch{},
                  world_generation{},sequence{},created_ms{};
    Vector left_palm_m{},starter_anchor_m{},pull_axis_m{1,0,0};
    bool lancer_active{},right_owns_weapon{},right_tracked{},left_tracked{},
         left_grip_down{},game_can_melee{},original_hold{},
         original_execution{},original_ready{};
    std::uint64_t original_victim{};
    LancerSawContact contact{};
};

inline double segment_distance_squared(Vector p0,Vector p1,
                                       Vector q0,Vector q1) noexcept {
    const auto u=difference(p1,p0),v=difference(q1,q0),w=difference(p0,q0);
    const double a=dot(u,u),b=dot(u,v),c=dot(v,v),d=dot(u,w),e=dot(v,w);
    const double epsilon=1e-9;
    double s=0,t=0;
    if(a<=epsilon&&c<=epsilon)return dot(w,w);
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
    return dot(gap,gap);
}
inline bool lancer_saw_blade_contact(LancerSawContact const& contact) noexcept {
    if(!contact.victim||!contact.original_target_valid||!contact.unobstructed||
       !gears_vr::finite(contact.blade_base_raw)||
       !gears_vr::finite(contact.blade_tip_raw)||
       !gears_vr::finite(contact.target_a_raw)||
       !gears_vr::finite(contact.target_b_raw)||
       !std::isfinite(contact.blade_radius_raw)||
       !std::isfinite(contact.target_radius_raw)||
       contact.blade_radius_raw<0||contact.blade_radius_raw>20||
       contact.target_radius_raw<=0||contact.target_radius_raw>200||
       length(difference(contact.blade_tip_raw,contact.blade_base_raw))<5)
        return false;
    const double radius=contact.blade_radius_raw+contact.target_radius_raw;
    return segment_distance_squared(contact.blade_base_raw,contact.blade_tip_raw,
                                    contact.target_a_raw,contact.target_b_raw)<=
           radius*radius;
}

class LancerChainsawInteraction {
    LancerSawPhase phase_{LancerSawPhase::idle};
    std::uint64_t world_{},pawn_{},weapon_{},session_{},space_epoch_{},
                  input_epoch_{},world_generation_{},sequence_{},last_ms_{},
                  phase_start_ms_{},victim_{};
    Vector start_left_{},start_anchor_{},start_relative_{},start_axis_{},
           last_left_{},last_anchor_{};
    bool previous_left_grip_{},owns_original_melee_{};

    void clear() noexcept {*this={};}
    bool identity_matches(LancerSawSample const& s) const noexcept {
        return s.world==world_&&s.pawn==pawn_&&s.weapon==weapon_&&
            s.session==session_&&s.space_epoch==space_epoch_&&
            s.input_epoch==input_epoch_&&s.world_generation==world_generation_;
    }
    void bind(LancerSawSample const& s) noexcept {
        world_=s.world;pawn_=s.pawn;weapon_=s.weapon;session_=s.session;
        space_epoch_=s.space_epoch;input_epoch_=s.input_epoch;
        world_generation_=s.world_generation;
    }
    LancerSawDecision interrupted() noexcept {
        const bool active=phase_!=LancerSawPhase::idle;
        const bool release=owns_original_melee_;
        clear();
        if(!active)return {};
        return {LancerSawEvent::interrupted,LancerSawPhase::idle,0,false,release};
    }
public:
    void reset() noexcept {clear();}
    LancerSawPhase phase() const noexcept {return phase_;}
    bool animation_owns_hands() const noexcept {
        return phase_==LancerSawPhase::animation;
    }
    bool suppress_original_auto_target() const noexcept {
        return owns_original_melee_&&
            (phase_==LancerSawPhase::waiting_for_hold||
             phase_==LancerSawPhase::running||
             phase_==LancerSawPhase::waiting_for_execution);
    }

    LancerSawDecision observe(LancerSawSample const& s) noexcept {
        // Keep the original paired animation in charge through an XR tracking
        // outage or recenter. Only the original game can declare it finished.
        if(phase_==LancerSawPhase::animation){
            if(!s.world||s.world!=world_||s.pawn!=pawn_||s.weapon!=weapon_)
                return interrupted();
            if(s.original_ready&&!s.original_execution&&!s.original_hold&&
               s.lancer_active&&s.right_owns_weapon&&s.right_tracked){
                clear();
                return {LancerSawEvent::restored,LancerSawPhase::idle,0,
                        false,false,false,true};
            }
            return {LancerSawEvent::none,phase_,victim_,false,false,true};
        }
        if(!s.world||!s.pawn||!s.weapon||!s.session||!s.space_epoch||
           !s.input_epoch||!s.world_generation||!s.sequence||!s.created_ms||
           !s.lancer_active||!s.right_owns_weapon||!s.right_tracked||
           ((phase_==LancerSawPhase::idle||phase_==LancerSawPhase::pulling)&&
            (!gears_vr::finite(s.left_palm_m)||
             !gears_vr::finite(s.starter_anchor_m)||
             !gears_vr::finite(s.pull_axis_m)||
             std::abs(length(s.pull_axis_m)-1.)>.1)))
            return interrupted();
        if(!world_){
            bind(s);sequence_=s.sequence;last_ms_=s.created_ms;
            previous_left_grip_=s.left_grip_down;
            return {};
        }
        if(!identity_matches(s)||s.sequence<sequence_||s.created_ms<last_ms_||
           s.created_ms-last_ms_>100)return interrupted();
        if(s.sequence==sequence_)return {LancerSawEvent::none,phase_};
        if(s.created_ms==last_ms_)return interrupted();
        const double frame_seconds=double(s.created_ms-last_ms_)/1000.;
        sequence_=s.sequence;last_ms_=s.created_ms;
        const bool grip_pressed=s.left_grip_down&&!previous_left_grip_;
        previous_left_grip_=s.left_grip_down;

        // Once the original paired execution starts, it controls both arms
        // until its own weapon/special-move completion is observed.
        if(s.original_execution&&phase_!=LancerSawPhase::idle&&
           phase_!=LancerSawPhase::animation){
            const bool unexpected=phase_!=LancerSawPhase::waiting_for_execution||
                (s.original_victim&&s.original_victim!=victim_);
            phase_=LancerSawPhase::animation;victim_=s.original_victim;
            const bool release=owns_original_melee_;owns_original_melee_=false;
            return {LancerSawEvent::animation_handoff,phase_,victim_,false,
                    release,true,false,unexpected};
        }
        if(phase_==LancerSawPhase::idle){
            if(grip_pressed&&s.left_tracked&&s.game_can_melee&&
               !s.original_hold&&!s.original_execution&&
               length(difference(s.left_palm_m,s.starter_anchor_m))<=.07){
                phase_=LancerSawPhase::pulling;phase_start_ms_=s.created_ms;
                start_left_=s.left_palm_m;start_anchor_=s.starter_anchor_m;
                start_relative_=difference(s.left_palm_m,s.starter_anchor_m);
                start_axis_=s.pull_axis_m;last_left_=s.left_palm_m;
                last_anchor_=s.starter_anchor_m;
                return {LancerSawEvent::pull_began,phase_};
            }
            return {};
        }
        if(phase_==LancerSawPhase::pulling){
            if(!s.left_tracked||!s.left_grip_down||!s.game_can_melee||
               s.created_ms-phase_start_ms_>1500||
               dot(s.pull_axis_m,start_axis_)<.94||
               length(difference(s.starter_anchor_m,start_anchor_))>.10||
               length(difference(s.left_palm_m,last_left_))>3.*frame_seconds||
               length(difference(s.starter_anchor_m,last_anchor_))>
                   1.5*frame_seconds)
                return interrupted();
            last_left_=s.left_palm_m;last_anchor_=s.starter_anchor_m;
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
            if(travel>=.18&&hand_travel>=.14&&
               s.created_ms-phase_start_ms_>=160){
                phase_=LancerSawPhase::waiting_for_hold;
                phase_start_ms_=s.created_ms;owns_original_melee_=true;
                return {LancerSawEvent::request_original_hold,phase_,0,true};
            }
            return {LancerSawEvent::none,phase_};
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
            if(!s.original_hold||!s.game_can_melee)return interrupted();
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
