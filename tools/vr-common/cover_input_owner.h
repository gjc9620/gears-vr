#pragma once
#include "cover_controller_adapter.h"
#include "cover_view.h"
#include "left_hand_interaction_owner.h"

namespace gears_vr {
struct CoverInputOwnerFrame {
    // One native GT owner frame. Exactly one call per frame; rendering retries
    // consume its committed result instead of arbitrating the hand again.
    std::uint64_t lifecycle=0,frame=0;
    CoverObservation cover;
    CoverViewAnchor anchor;
    StereoResult stereo;
    ControllerInputFrame const* controllers=nullptr; // exact stereo match only
    std::array<LeftHandInteractionLease,2> weapon_incumbents{};
    LeftHandInteractionRequests weapon_requests;
    CoverHeightKey const* height_curve=nullptr;
    unsigned height_key_count=0;
    double units_per_metre=100;
    std::int64_t now_ns=0;
    bool allow_sim=false,owner_context_valid=false;
};
struct CoverInputOwnerResult {
    CoverResult interaction;
    CoverControllerStatus controller_status=CoverControllerStatus::unavailable;
    LeftHandInteractionDecision left_hand;
    CoverViewResult view;
    CoverPhysicalDisplacement physical;
    double requested_right_metres=0,requested_up_metres=0,requested_up_weight=0;
    bool accepted=false,view_valid=false,body_weight_valid=false;
};

// Cover's participant in the sole GT left-hand owner. The supplied owner is
// shared with weapon requests/results; no second arbiter lives in this class.
// It changes only private interaction state, never engine/actor/mesh memory.
class CoverInputOwner {
public:
    CoverInputOwnerResult update(CoverInputOwnerFrame const& frame,LeftHandInteractionOwner& owner){
        CoverInputOwnerResult result{};
        if(!frame.lifecycle||!frame.frame||frame.cover.sequence!=frame.frame||frame.now_ns<=0||
           frame.lifecycle<lifecycle_||(frame.lifecycle==lifecycle_&&frame.frame<=frame_))return result;
        lifecycle_=frame.lifecycle;frame_=frame.frame;
        // External participants may report only their actual committed weapon
        // leases. A cover lease can be minted solely by our two-stage grant.
        for(auto const& lease:frame.weapon_incumbents)
            if(lease.kind==LeftHandInteraction::cover)return result;
        const auto hand=adapter_.update(frame.controllers,frame.stereo.sample,
            {frame.cover.epochs.session,frame.cover.epochs.space},frame.now_ns,frame.allow_sim);
        result.controller_status=hand.status;
        const bool tracked=hand.hand.input_active&&hand.hand.position_valid&&hand.hand.position_tracked;
        const bool down=tracked&&hand.hand.lgrip_down;
        if(!tracked)previous_down_=true; // An interruption cannot manufacture a new press.
        else{
            if(down&&!previous_down_){
                if(press_token_==(std::numeric_limits<std::uint64_t>::max)())return result;
                ++press_token_;
            }
            previous_down_=down;
        }
        unsigned external_count{};
        for(auto const& lease:frame.weapon_incumbents)if(lease.kind!=LeftHandInteraction::none)++external_count;
        CoverOwnership ownership{};
        if(external_count)ownership.owner=CoverLeftHandOwner::other;
        else if(cover_lease_.kind==LeftHandInteraction::cover)
            ownership={CoverLeftHandOwner::cover,cover_lease_.id};
        auto proposal=interaction_.update(frame.cover,hand.hand,ownership,frame.now_ns);
        LeftHandInteractionFrame requests{};
        requests.lifecycle=frame.lifecycle;requests.frame=frame.frame;requests.press_token=press_token_;
        requests.context_valid=frame.owner_context_valid;
        requests.tracked=tracked;requests.grip_down=down;requests.incumbents=frame.weapon_incumbents;
        requests.requests=frame.weapon_requests;
        // The cover request is always derived here, never accepted from the
        // weapon side. A held body offset has no hand lease after release.
        requests.requests.cover_request_id=proposal.request_left_hand?proposal.request_id:0;
        if(proposal.owns_left_hand&&cover_lease_.kind==LeftHandInteraction::cover){
            for(auto& incumbent:requests.incumbents)if(incumbent.kind==LeftHandInteraction::none){incumbent=cover_lease_;break;}
        }
        result.left_hand=owner.update(requests);
        if(proposal.request_left_hand&&result.left_hand.lease.kind==LeftHandInteraction::cover&&
           result.left_hand.lease.id==proposal.request_id){
            proposal=interaction_.update(frame.cover,hand.hand,
                {CoverLeftHandOwner::cover,proposal.request_id},frame.now_ns);
        }else if(proposal.owns_left_hand&&result.left_hand.lease.kind!=LeftHandInteraction::cover){
            proposal=interaction_.update(frame.cover,hand.hand,{CoverLeftHandOwner::other,0},frame.now_ns);
        }
        cover_lease_=proposal.owns_left_hand?result.left_hand.lease:LeftHandInteractionLease{};
        result.interaction=proposal;result.accepted=true;
        // Losing the left hand releases its grip, while a freshly re-certified
        // held offset and live HMD still produce a valid camera/body target.
        if(!proposal.offset_certified||!frame.stereo||!frame.stereo.reference_generation||
           !stereo_render_active(frame.stereo.sample)||!stereo_tracking_valid(frame.stereo.sample,frame.allow_sim)||
           !stereo_geometry_valid(frame.stereo.sample)||frame.stereo.sample.received_ns<=0||
           frame.stereo.sample.received_ns>frame.now_ns||frame.now_ns-frame.stereo.sample.received_ns>100000000)return result;
        if(!prepare_cover_view(frame.anchor,proposal.offset,frame.units_per_metre,result.view)||
           !cover_physical_displacement(frame.stereo.reference,frame.stereo.sample.head,result.physical))return result;
        result.view_valid=true;
        result.requested_right_metres=proposal.offset.right+result.physical.right;
        result.requested_up_metres=proposal.offset.up+result.physical.up;
        result.body_weight_valid=cover_up_weight(frame.height_curve,frame.height_key_count,
            result.requested_up_metres,result.requested_up_weight);
        return result;
    }
private:
    CoverControllerAdapter adapter_;
    CoverInteraction interaction_;
    LeftHandInteractionLease cover_lease_;
    std::uint64_t lifecycle_=0,frame_=0,press_token_=0;
    bool previous_down_=true;
};
} // namespace gears_vr
