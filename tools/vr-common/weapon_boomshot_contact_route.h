#pragma once
#include "weapon_support_grip.h"

namespace gears_weapon {
enum class BoomshotContact : unsigned {none,opening,closing,take_carrier};
inline gears_weapon_hand::LeaseKind boomshot_contact_lease(BoomshotContact purpose) noexcept {
    return purpose==BoomshotContact::take_carrier?gears_weapon_hand::LeaseKind::ammo:
        purpose==BoomshotContact::opening||purpose==BoomshotContact::closing?gears_weapon_hand::LeaseKind::magazine:
        gears_weapon_hand::LeaseKind::none;
}
class BoomshotContactRoute;
// Receipt of a Native commit, not another input/lease issuer. The ordinary GT
// consumer must still prove current actors, counts, geometry and animation.
class BoomshotMechanismGrant {
    friend class BoomshotContactRoute;
    using Frame=gears_weapon_hand::Frame;using Decision=gears_weapon_hand::Decision;
    Frame origin_{};Decision decision_{};BoomshotContact purpose_{};bool known_{},started_{};
    static bool usable(Frame const& f,std::int64_t now) noexcept {
        return gears_weapon_hand::frame_active(f,now)&&now-f.now_ns<=100000000&&
            !f.recentered&&!f.heading_deferred&&f.left_tracked==1&&f.left_grip_known==1&&f.left_grip_down==1&&f.press_token;
    }
    bool cohort(Frame const& f,std::int64_t now) const noexcept {
        namespace h=gears_weapon_hand;
        if(!known_||!usable(f,now)||f.key.frame<origin_.key.frame||f.controllers.sequence<origin_.controllers.sequence||
           f.controllers.sync_ns<origin_.controllers.sync_ns||f.press_token!=origin_.press_token||
           f.interaction_control!=origin_.interaction_control)return false;
        auto key=origin_.key;key.frame=f.key.frame;
        if(!h::same(key,f.key)||!h::same(origin_.world,f.world)||!h::same(origin_.controller,f.controller)||
           !h::same(origin_.pawn,f.pawn)||!h::same(origin_.mesh,f.mesh)||!h::same(origin_.weapon,f.weapon)||
           origin_.reference_generation!=f.reference_generation||!gears_vr::stereo_same_pose(origin_.reference,f.reference)||
           origin_.stereo.source!=f.stereo.source||origin_.stereo.session!=f.stereo.session||origin_.stereo.space_epoch!=f.stereo.space_epoch||
           origin_.controllers.input_epoch!=f.controllers.input_epoch||origin_.controllers.profile_generation!=f.controllers.profile_generation)return false;
        const auto control=h::left_control(f);auto const& before=origin_.controllers.hands[0];auto const& current=f.controllers.hands[0];
        const auto active=[&](gears_vr::ControllerHand const& hand){return control==gears_vr::control_trigger?hand.trigger_active:hand.squeeze_active;};
        return active(before)==1&&active(current)==1&&before.pressed[control]==current.pressed[control]&&
            before.released[control]==current.released[control]&&h::control_held(before,control)&&h::control_held(current,control);
    }
    bool selected(Frame const& f,Decision const& d,BoomshotContact purpose,std::int64_t now) const noexcept {
        namespace h=gears_weapon_hand;
        if(!cohort(f,now)||purpose==BoomshotContact::none||purpose!=purpose_||d.size!=sizeof d||d.abi_version!=h::version||
           d.reserved||d.release_required||!d.proposal_nonce||d.press_token!=f.press_token||!h::same(f.key,d.key)||
           !h::same(d.lease,decision_.lease)||!h::lease_valid(d.lease,f.key.lifecycle)||d.lease.id!=f.press_token||
           d.now_ns<f.now_ns||d.now_ns>now||now-d.now_ns>100000000)return false;
        if(f.key.frame==origin_.key.frame)return h::same_frame(origin_,f)&&d.new_grant==1&&
            d.status==h::DecisionStatus::granted&&d.proposal_nonce==decision_.proposal_nonce;
        return d.new_grant==0&&d.status==h::DecisionStatus::retained;
    }
public:
    BoomshotMechanismGrant()=default;
    BoomshotMechanismGrant(BoomshotMechanismGrant const&)=delete;
    BoomshotMechanismGrant& operator=(BoomshotMechanismGrant const&)=delete;
    bool begin_allowed(Frame const& f,Decision const& d,BoomshotContact purpose,std::int64_t now) const noexcept {
        return !started_&&selected(f,d,purpose,now)&&now>=origin_.controllers.sync_ns&&
            now-origin_.controllers.sync_ns<=250000000;
    }
    bool started(Frame const& f,Decision const& d,BoomshotContact purpose,std::int64_t now) noexcept {
        if(!begin_allowed(f,d,purpose,now))return false;started_=true;return true;
    }
    bool retained(Frame const& f,Decision const& d,BoomshotContact purpose,std::int64_t now) const noexcept {
        return started_&&selected(f,d,purpose,now);
    }
    bool append_incumbent(Frame const& f,BoomshotContact purpose,std::int64_t now,
        std::array<gears_weapon_hand::Lease,2>& slots) const noexcept {
        namespace h=gears_weapon_hand;
        if(!cohort(f,now)||purpose!=purpose_||purpose==BoomshotContact::none||f.key.frame<=origin_.key.frame||
           (!started_&&(now<origin_.controllers.sync_ns||now-origin_.controllers.sync_ns>250000000)))return false;
        for(auto const& slot:slots)if(!h::lease_valid(slot,f.key.lifecycle))return false;
        if(slots[0].kind!=h::LeaseKind::none&&slots[1].kind!=h::LeaseKind::none&&h::same(slots[0],slots[1]))return false;
        for(auto const& slot:slots)if(h::same(slot,decision_.lease))return true;
        for(auto& slot:slots)if(slot.kind==h::LeaseKind::none){slot=decision_.lease;return true;}
        return false;
    }
    bool active()const noexcept{return known_;}
    void clear()noexcept{origin_={};decision_={};purpose_={};known_=started_=false;}
};
class BoomshotContactRoute {
    using Frame=gears_weapon_hand::Frame;using Decision=gears_weapon_hand::Decision;
    Frame proposed_{};BoomshotContact purpose_{};bool known_{},conflict_{};
    Frame committed_{};BoomshotContact committed_purpose_{};bool commit_known_{};
    gears_weapon_hand::FrameKey last_key_{};std::uint64_t last_token_{};bool accepted_{};
    BoomshotMechanismGrant grant_{};
    static bool same_domain(gears_weapon_hand::FrameKey a,gears_weapon_hand::FrameKey b)noexcept {
        a.frame=b.frame;return gears_weapon_hand::same(a,b);
    }
public:
    BoomshotContact propose(Frame const& f,BoomshotContact purpose,std::int64_t now) noexcept {
        namespace h=gears_weapon_hand;
        if(commit_known_&&h::same(committed_.key,f.key)){
            // Engine work after commit cannot withdraw THIS frozen proposal.
            // Changed source bytes still poison it; no next-frame permission.
            if(!h::same_frame(committed_,f)||!BoomshotMechanismGrant::usable(f,now))conflict_=true;
            if(conflict_)return BoomshotContact::none;
            proposed_=f;purpose_=committed_purpose_;known_=true;return purpose_;
        }
        if(known_&&h::same(proposed_.key,f.key)){
            if(!h::same_frame(proposed_,f)||purpose!=purpose_||!BoomshotMechanismGrant::usable(f,now))conflict_=true;
            return conflict_?BoomshotContact::none:purpose_;
        }
        if(known_&&same_domain(proposed_.key,f.key)&&f.key.frame<proposed_.key.frame)return BoomshotContact::none;
        proposed_=f;purpose_=purpose;known_=true;conflict_=false;
        if(!BoomshotMechanismGrant::usable(f,now)||(boomshot_contact_lease(purpose)==h::LeaseKind::none))purpose_=BoomshotContact::none;
        return purpose_;
    }
    bool remember(Frame const& f,Decision const& d,BoomshotContact purpose,std::int64_t now) noexcept {
        namespace h=gears_weapon_hand;
        if(!known_||conflict_||purpose==BoomshotContact::none||purpose!=purpose_||!h::same_frame(proposed_,f)||
           !BoomshotMechanismGrant::usable(f,now)||d.size!=sizeof d||d.abi_version!=h::version||d.reserved||
           !h::same(f.key,d.key)||d.new_grant!=1||d.release_required||d.status!=h::DecisionStatus::granted||
           d.press_token!=f.press_token||!d.proposal_nonce||d.lease.kind!=boomshot_contact_lease(purpose)||
           !h::lease_valid(d.lease,f.key.lifecycle)||d.lease.id!=f.press_token||d.now_ns<f.now_ns||d.now_ns>now)return false;
        if(accepted_&&same_domain(last_key_,f.key)&&(f.key.frame<=last_key_.frame||f.press_token<=last_token_)){
            // A repeated notification cannot undo the receipt's started bit.
            return grant_.known_&&h::same_frame(grant_.origin_,f)&&h::same(grant_.decision_.lease,d.lease)&&
                grant_.decision_.proposal_nonce==d.proposal_nonce&&grant_.purpose_==purpose;
        }
        grant_.origin_=f;grant_.decision_=d;grant_.purpose_=purpose;grant_.known_=true;grant_.started_=false;
        committed_=f;committed_purpose_=purpose;commit_known_=true;
        last_key_=f.key;last_token_=f.press_token;accepted_=true;return true;
    }
    BoomshotMechanismGrant& grant()noexcept{return grant_;}
    void reset()noexcept {proposed_={};purpose_={};known_=conflict_=false;grant_.clear();} // Keep replay floor and frozen commit.
};
}
