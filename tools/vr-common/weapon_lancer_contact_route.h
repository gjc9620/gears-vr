#pragma once
#include "weapon_hand_participant.h"

namespace gears_weapon {
// A purpose tag for Native's existing magazine/mechanism lease, not another
// arbiter. The lease alone cannot tell a magazine, receiver and starter apart.
// Only the exact proposal and grant may dispatch the corresponding consumer.
enum class LancerContact : unsigned {none=0,magazine=1,charging=2,starter=4,conflict=8};
// Report an ALREADY granted part, independently of hover/other-part geometry.
// The caller still authenticates the weapon identity and current game context;
// Native remains the sole arbiter. This cannot propose a new acquisition or
// supply a pose, physical stroke, input edge or motor-start permission.
inline bool retain_lancer_part_incumbent(gears_weapon_hand::Frame const& f,
    gears_weapon_hand::Lease const& held,std::uint64_t granted_frame,bool identity_matches,
    std::array<gears_weapon_hand::Lease,2>& incumbents) noexcept {
    namespace h=gears_weapon_hand;
    if(!identity_matches||!granted_frame||f.key.frame<=granted_frame||
       !f.left_tracked||!f.left_grip_known||!f.left_grip_down||
       held.kind!=h::LeaseKind::magazine||held.id!=f.press_token||
       !h::lease_valid(held,f.key.lifecycle))return false;
    for(auto const& incumbent:incumbents)if(h::same(incumbent,held))return true;
    for(auto& incumbent:incumbents)if(incumbent.kind==h::LeaseKind::none){incumbent=held;return true;}
    return false; // Never overwrite another owner or hide an incumbent conflict.
}
class LancerContactRoute {
    using Frame=gears_weapon_hand::Frame;
    using Decision=gears_weapon_hand::Decision;
    struct Source {
        gears_weapon_hand::FrameKey key{};
        gears_weapon_hand::ObjectIdentity world{},controller{},pawn{},mesh{},weapon{};
        std::uint64_t token{},sequence{},input_epoch{},session{},space{},reference{};
        std::int64_t sync_ns{};
    } source_{};
    unsigned candidates_{};
    LancerContact purpose_{};
    bool known_{};
    static Source source(Frame const& f) noexcept {
        return {f.key,f.world,f.controller,f.pawn,f.mesh,f.weapon,f.press_token,
            f.controllers.sequence,f.controllers.input_epoch,f.stereo.session,
            f.stereo.space_epoch,f.reference_generation,f.controllers.sync_ns};
    }
    static bool same_source(Source const& a,Frame const& b) noexcept {
        namespace h=gears_weapon_hand;
        return h::same(a.key,b.key)&&h::same(a.world,b.world)&&
            h::same(a.controller,b.controller)&&h::same(a.pawn,b.pawn)&&
            h::same(a.mesh,b.mesh)&&h::same(a.weapon,b.weapon)&&
            a.token==b.press_token&&a.sequence==b.controllers.sequence&&
            a.sync_ns==b.controllers.sync_ns&&a.input_epoch==b.controllers.input_epoch&&
            a.session==b.stereo.session&&a.space==b.stereo.space_epoch&&
            a.reference==b.reference_generation;
    }
public:
    void reset() noexcept {*this={};}
    LancerContact propose(Frame const& f,unsigned candidates) noexcept {
        namespace h=gears_weapon_hand;
        // Native authenticates the callback; this tag never extends freshness
        // or makes a controller input actionable on its own.
        const bool usable=f.key.frame&&f.key.lifecycle&&f.press_token&&
            f.left_tracked==1&&f.left_grip_known==1&&f.left_grip_down==1;
        if(known_&&h::same(source_.key,f.key)){
            if(!same_source(source_,f)||candidates_!=candidates||!usable)
                purpose_=LancerContact::conflict;
            return purpose_;
        }
        // A delayed observation cannot replace the current frame's dispatch.
        if(known_&&source_.key.registration_cookie==f.key.registration_cookie&&
           source_.key.native_started_qpc==f.key.native_started_qpc&&
           source_.key.lifecycle==f.key.lifecycle&&f.key.frame<source_.key.frame)
            return LancerContact::conflict;
        source_=source(f);candidates_=candidates;known_=true;
        const bool single=candidates&&(candidates&(candidates-1))==0;
        purpose_=!usable||!candidates?LancerContact::none:
            (!single||(candidates&~7u)?LancerContact::conflict:static_cast<LancerContact>(candidates));
        return purpose_;
    }
    bool grants(Frame const& f,Decision const& d,LancerContact purpose) const noexcept {
        namespace h=gears_weapon_hand;
        return known_&&purpose!=LancerContact::none&&purpose!=LancerContact::conflict&&
            purpose_==purpose&&same_source(source_,f)&&f.left_tracked==1&&
            f.left_grip_known==1&&f.left_grip_down==1&&f.press_token&&
            h::same(d.key,f.key)&&d.new_grant==1&&!d.release_required&&
            d.status==h::DecisionStatus::granted&&d.lease.kind==h::LeaseKind::magazine&&
            h::lease_valid(d.lease,f.key.lifecycle)&&d.lease.id==f.press_token&&
            d.press_token==f.press_token;
    }
};
} // namespace gears_weapon
