#pragma once
#include "cover_controller_adapter.h"
#include "cover_view.h"
#include "left_hand_interaction_owner.h"
#include "left_interaction_control.h"

namespace gears_vr {
struct CoverInputOwnerFrame {
    // One native GT owner frame. Exactly one call per frame; rendering retries
    // consume its committed result instead of arbitrating the hand again.
    std::uint64_t lifecycle=0,frame=0;
    CoverObservation cover;
    CoverViewAnchor anchor;      // Wall/body/virtual-drag basis.
    CoverViewAnchor view_anchor; // Explicit continuous view; same neutral eye.
    StereoResult stereo;
    ControllerInputFrame const* controllers=nullptr; // exact stereo match only
    std::array<LeftHandInteractionLease,2> weapon_incumbents{};
    LeftHandInteractionRequests weapon_requests;
    CoverHeightKey const* height_curve=nullptr;
    unsigned height_key_count=0;
    double units_per_metre=100;
    std::int64_t now_ns=0;
    bool allow_sim=false,owner_context_valid=false,allow_index_pinch=false;
};
struct CoverInputOwnerResult {
    CoverResult interaction;
    CoverControllerStatus controller_status=CoverControllerStatus::unavailable;
    LeftHandInteractionDecision left_hand;
    CoverViewResult view;
    CoverViewAnchor wall_anchor{},view_anchor{}; // Frozen only with view_valid.
    StereoResult tracking{}; // Exact selected sample/reference used for this view.
    CoverPhysicalDisplacement physical;
    double requested_right_metres=0,requested_up_metres=0,requested_up_weight=0;
    // A failed hand transaction may still re-certify a held-offset/physical-HMD
    // view in its CURRENT Scene. accepted never authorizes that failed lease;
    // view_valid/body_weight_valid independently certify these pose outputs.
    bool accepted=false,view_valid=false,body_weight_valid=false;
};

class CoverInputOwner;
class CoverInputOwnerTicket {
public:
    explicit operator bool() const noexcept {return origin_&&serial_;}
    friend bool operator==(CoverInputOwnerTicket a,CoverInputOwnerTicket b) noexcept {
        return a.origin_==b.origin_&&a.serial_==b.serial_;
    }
    friend bool operator!=(CoverInputOwnerTicket a,CoverInputOwnerTicket b) noexcept {return !(a==b);}
private:
    friend class CoverInputOwner;
    CoverInputOwner const* origin_=nullptr;
    std::uint64_t serial_=0;
};
struct CoverInputOwnerDecoded {
    CoverInputOwnerTicket ticket;
    std::uint64_t press_token=0,continuity_generation=0;
    LeftInteractionControl interaction_control{LeftInteractionControl::grip};
    CoverControllerStatus controller_status=CoverControllerStatus::unavailable;
    // These are conservative logical-input certifications, not raw tracking
    // diagnostics. Unknown/needs-release/barrier samples expose all three false.
    bool left_grip_down=false,left_grip_known=false,left_tracked=false,accepted=false;
};
enum class CoverWeaponState {ready,absent_quiescent,blocked};
struct CoverWeaponContribution {
    // Busy, unavailable, invalid, draining and failed callbacks are BLOCKED,
    // never an empty hand. Absent requires no participant or cleanup debt.
    CoverWeaponState state=CoverWeaponState::blocked;
    std::array<LeftHandInteractionLease,2> incumbents{};
    LeftHandInteractionRequests requests;
};
struct CoverInputOwnerCandidate {
    LeftHandInteractionDecision left_hand;
    CoverControllerStatus controller_status=CoverControllerStatus::unavailable;
    bool accepted=false;
};

// Cover's participant in the sole GT left-hand owner. The supplied owner is
// shared with weapon requests/results; no second arbiter lives in this class.
// It changes only private interaction state, never engine/actor/mesh memory.
// All calls are serialized on the owner GT; that same owner outlives this
// object. Native freezes its full FrameKey/cookie and holds ONE registration
// admission across begin -> prepare -> arbitration -> commit -> complete,
// including the gap between callbacks. Stop/drain must retain that admission,
// module pin, context and restoration debt until the transaction has ended.
// Rendering retries read a completed result; they do not repeat PG callbacks.
// Run this outside cover too, with cover_valid=false but live input epochs.
class CoverInputOwner {
public:
    static constexpr unsigned maximum_height_keys=32;
    CoverInputOwner()=default;
    CoverInputOwner(CoverInputOwner const&)=delete;
    CoverInputOwner& operator=(CoverInputOwner const&)=delete;

    CoverInputOwnerDecoded begin(CoverInputOwnerFrame const& frame,LeftHandInteractionOwner& owner){
        if(!bind(owner))return {};
        if(!frame.lifecycle||!frame.frame||frame.cover.sequence!=frame.frame||frame.now_ns<=0||
           frame.lifecycle<lifecycle_||(frame.lifecycle==lifecycle_&&frame.frame<frame_)){
            abort_current(owner);return {};
        }
        if(frame.lifecycle==lifecycle_&&frame.frame==frame_){
            if(snapshot_valid_&&same_snapshot(frame))return decoded_;
            abort_current(owner);return {};
        }
        if(phase_==Phase::decoded||phase_==Phase::decided)abort_current(owner);
        lifecycle_=frame.lifecycle;frame_=frame.frame;
        phase_=Phase::aborted;snapshot_valid_=false;decoded_={};candidate_={};result_={};
        if(frame.height_key_count>maximum_height_keys||(!frame.height_curve&&frame.height_key_count)||
           serial_==(std::numeric_limits<std::uint64_t>::max)()){
            abort_current(owner);return {};
        }
        frozen_=frame;has_controllers_=frame.controllers!=nullptr;has_curve_=frame.height_curve!=nullptr;
        if(has_controllers_)controllers_=*frame.controllers;
        for(unsigned i=0;i<frame.height_key_count;++i)height_curve_[i]=frame.height_curve[i];
        frozen_.controllers=nullptr;frozen_.height_curve=nullptr;snapshot_valid_=true;
        barrier_=barrier_next_;barrier_next_=false;
        decoded_.ticket.origin_=this;decoded_.ticket.serial_=++serial_;phase_=Phase::decoded;
        hand_=adapter_.update(has_controllers_?&controllers_:nullptr,frame.stereo.sample,
            {frame.cover.epochs.session,frame.cover.epochs.space},frame.now_ns,frame.allow_sim);
        const auto pinch=pinch_adapter_.update(has_controllers_?&controllers_:nullptr,frame.stereo.sample,
            {frame.cover.epochs.session,frame.cover.epochs.space},frame.now_ns,frame.allow_sim);
        const auto selection=control_selector_.update(
            {hand_.hand.input_active,hand_.hand.lgrip_down},{pinch.hand.input_active,pinch.hand.lgrip_down},
            frame.allow_index_pinch&&!frame.cover.cover_valid);
        if(selection.control==LeftInteractionControl::pinch)hand_=pinch;
        hand_.hand.input_active=hand_.hand.input_active&&selection.known;
        hand_.hand.lgrip_down=selection.down;
        decoded_.interaction_control=selection.control;
        const bool tracked=hand_.hand.input_active&&hand_.hand.position_valid&&hand_.hand.position_tracked;
        const bool down=tracked&&hand_.hand.lgrip_down;
        if(!tracked)previous_down_=true; // An interruption cannot manufacture a new press.
        else{
            if(down&&!previous_down_){
                if(press_token_==(std::numeric_limits<std::uint64_t>::max)()){
                    abort_current(owner);return {};
                }
                ++press_token_;
            }
            previous_down_=down;
        }
        decoded_.press_token=press_token_;decoded_.continuity_generation=hand_.continuity_generation;
        decoded_.controller_status=hand_.status;decoded_.accepted=true;
        decoded_.left_grip_known=decoded_.left_tracked=tracked&&!barrier_;
        decoded_.left_grip_down=down&&!barrier_;
        return decoded_;
    }

    CoverInputOwnerCandidate update_with_decoded(CoverInputOwnerTicket ticket,
        CoverWeaponContribution const& weapon,LeftHandInteractionOwner& owner){
        if(!bind(owner)||!current(ticket))return {};
        if(phase_==Phase::decided||phase_==Phase::committed){
            if(same(weapon,weapon_))return candidate_;
            abort_current(owner);return {};
        }
        if(phase_!=Phase::decoded)return {};
        weapon_=weapon;candidate_={};candidate_.controller_status=hand_.status;
        pending_interaction_=interaction_;pending_lease_=cover_lease_;
        auto requests=owner_frame();
        if(!valid(weapon)){
            requests.context_valid=false;candidate_.left_hand=owner.update(requests);
            phase_=Phase::decided;return candidate_;
        }
        auto const& frame=frozen_;auto hand=hand_.hand;
        auto cover=frame.cover;
        if(cover.cover_valid){
            CoverViewResult base{};CoverTrackingBasis axes{};
            if(prepare_cover_view(frame.anchor,frame.view_anchor,{},frame.units_per_metre,base)&&
               cover_tracking_basis(frame.stereo.reference,frame.view_anchor,frame.anchor,axes)){
                cover.right_axis_tracking=axes.right;cover.up_axis_tracking=axes.up;
            }else{
                // Missing cover/view geometry withdraws only Cover's proposal;
                // the same decoded input can still serve a valid weapon request.
                cover.interaction_enabled=false;
            }
        }
        if(barrier_)hand.input_active=hand.position_valid=hand.position_tracked=false;
        unsigned external_count{};
        for(auto const& lease:weapon.incumbents)if(lease.kind!=LeftHandInteraction::none)++external_count;
        CoverOwnership ownership{};
        if(external_count)ownership.owner=CoverLeftHandOwner::other;
        else if(cover_lease_.kind==LeftHandInteraction::cover)
            ownership={CoverLeftHandOwner::cover,cover_lease_.id};
        auto proposal=pending_interaction_.update(cover,hand,ownership,frame.now_ns);
        requests.incumbents=weapon.incumbents;requests.requests=weapon.requests;
        // The cover request is always derived here, never accepted from the
        // weapon side. A held body offset has no hand lease after release.
        requests.requests.cover_request_id=proposal.request_left_hand?proposal.request_id:0;
        if(proposal.owns_left_hand&&cover_lease_.kind==LeftHandInteraction::cover){
            for(auto& incumbent:requests.incumbents)if(incumbent.kind==LeftHandInteraction::none){incumbent=cover_lease_;break;}
        }
        candidate_.left_hand=owner.update(requests);
        if(proposal.request_left_hand&&candidate_.left_hand.lease.kind==LeftHandInteraction::cover&&
           candidate_.left_hand.lease.id==proposal.request_id){
            proposal=pending_interaction_.update(cover,hand,
                {CoverLeftHandOwner::cover,proposal.request_id},frame.now_ns);
        }else if(proposal.owns_left_hand&&candidate_.left_hand.lease.kind!=LeftHandInteraction::cover){
            proposal=pending_interaction_.update(frame.cover,hand,{CoverLeftHandOwner::other,0},frame.now_ns);
        }
        pending_lease_=proposal.owns_left_hand?candidate_.left_hand.lease:LeftHandInteractionLease{};
        pending_result_={};pending_result_.interaction=proposal;pending_result_.left_hand=candidate_.left_hand;
        pending_result_.controller_status=hand_.status;pending_result_.accepted=true;
        build_view(pending_result_);candidate_.accepted=true;phase_=Phase::decided;
        return candidate_;
    }

    CoverInputOwnerResult complete(CoverInputOwnerTicket ticket,bool committed,LeftHandInteractionOwner& owner){
        if(!bind(owner)||!current(ticket))return {};
        if(phase_==Phase::aborted||(phase_==Phase::committed&&committed))return result_;
        if(phase_==Phase::committed){abort_current(owner);return result_;}
        if(!committed||phase_!=Phase::decided||!candidate_.accepted){abort_current(owner,true);return result_;}
        interaction_=pending_interaction_;cover_lease_=pending_lease_;result_=pending_result_;
        phase_=Phase::committed;return result_;
    }

    // Native must call this serialized barrier for recenter/reference, world,
    // pawn or registration-cookie changes, even OUTSIDE cover. The input
    // adapter only knows session/space. It is not reset; tokens never rewind.
    // Off-GT stop first closes atomic admission, then drains/serializes here.
    void invalidate(LeftHandInteractionOwner& owner){
        if(!bind(owner))return;
        abort_current(owner);barrier_next_=true;
    }

    // Compatibility path: synchronous callers supply already-final weapon
    // data. Preserve the old duplicate-update rejection, including retries
    // after completion; new callers use begin/complete's exact cached result.
    CoverInputOwnerResult update(CoverInputOwnerFrame const& frame,LeftHandInteractionOwner& owner){
        if(!frame.lifecycle||!frame.frame||frame.cover.sequence!=frame.frame||frame.now_ns<=0||
           frame.lifecycle<lifecycle_||(frame.lifecycle==lifecycle_&&frame.frame<=frame_))return {};
        auto decoded=begin(frame,owner);if(!decoded.accepted)return {};
        CoverWeaponContribution weapon{};weapon.state=CoverWeaponState::ready;
        weapon.incumbents=frame.weapon_incumbents;weapon.requests=frame.weapon_requests;
        // The legacy path always derived this field itself.
        weapon.requests.cover_request_id=0;
        const auto candidate=update_with_decoded(decoded.ticket,weapon,owner);
        return complete(decoded.ticket,candidate.accepted,owner);
    }

private:
    enum class Phase {empty,decoded,decided,committed,aborted};
    bool bind(LeftHandInteractionOwner& owner){
        if(!owner_)owner_=&owner;
        if(owner_==&owner)return true;
        abort_current(*owner_);owner.invalidate();return false;
    }
    bool current(CoverInputOwnerTicket ticket) const {return ticket&&ticket==decoded_.ticket;}
    LeftHandInteractionFrame owner_frame() const {
        LeftHandInteractionFrame frame{};
        frame.lifecycle=lifecycle_;frame.frame=frame_;frame.press_token=press_token_;
        frame.context_valid=frozen_.owner_context_valid&&!barrier_;
        frame.tracked=decoded_.left_tracked;frame.grip_down=decoded_.left_grip_down;
        frame.now_ns=frozen_.now_ns;
        return frame;
    }
    void abort_current(LeftHandInteractionOwner& owner,bool recertify_current_scene=false){
        const bool current_scene=recertify_current_scene&&snapshot_valid_&&decoded_.accepted&&
            (phase_==Phase::decoded||phase_==Phase::decided)&&frozen_.owner_context_valid&&
            frozen_.cover.observed_ns>0&&frozen_.cover.observed_ns<=frozen_.now_ns&&
            frozen_.now_ns-frozen_.cover.observed_ns<=100000000;
        // An abandoned begin must consume an inactive owner frame; otherwise
        // the next held sample could become a delayed rising edge.
        if(phase_==Phase::decoded){auto frame=owner_frame();frame.context_valid=false;owner.update(frame);}
        result_={};result_.controller_status=hand_.status;
        result_.interaction=interaction_.suspend();result_.left_hand=owner.invalidate();
        if(current_scene){
            // Re-certify ONLY committed offset against this fresh Scene, not
            // pending_interaction_'s tentative drag. The normal kernel clears
            // it for a new world/pawn/cover or cover exit. Bounds that exclude
            // the held offset withdraw certification without moving it.
            // Disable interaction in this copy so certification stops BEFORE
            // touching the hand stream; a synthetic inactive sample must not
            // poison a later Scene reusing the same valid XR release packet.
            auto certified=interaction_;auto cover=frozen_.cover;auto hand=hand_.hand;
            cover.interaction_enabled=false;
            hand.input_active=hand.position_valid=hand.position_tracked=hand.lgrip_down=false;
            result_.interaction=certified.update(cover,hand,{CoverLeftHandOwner::other,0},frozen_.now_ns);
            if(result_.interaction.offset_certified||result_.interaction.status==CoverStatus::no_cover)
                interaction_=certified;
            // Stereo time/geometry/reference validation uses only frozen time.
            // Invalidations, mutated snapshots, old tickets and abandoned
            // frames never enter this path or rebuild a cached world/view.
            build_view(result_);
        }
        cover_lease_={};candidate_={};decoded_.accepted=false;phase_=Phase::aborted;
    }
    bool valid(CoverWeaponContribution const& weapon) const {
        if(weapon.state!=CoverWeaponState::ready&&weapon.state!=CoverWeaponState::absent_quiescent)return false;
        if(weapon.requests.cover_request_id)return false;
        for(auto const& lease:weapon.incumbents){
            if(unsigned(lease.kind)>unsigned(LeftHandInteraction::other)||lease.kind==LeftHandInteraction::cover)return false;
            if(lease.kind==LeftHandInteraction::none){if(lease.lifecycle||lease.id)return false;}
            else if(lease.lifecycle!=lifecycle_||!lease.id||weapon.state==CoverWeaponState::absent_quiescent)return false;
        }
        return weapon.state!=CoverWeaponState::absent_quiescent||
            (!weapon.requests.support&&!weapon.requests.ammo&&!weapon.requests.magazine&&!weapon.requests.magazine_observation_pending);
    }
    void build_view(CoverInputOwnerResult& result) const {
        auto const& frame=frozen_;auto const& proposal=result.interaction;
        // Losing the left hand releases its grip, while a freshly re-certified
        // held offset and live HMD still produce a valid camera/body target.
        if(!proposal.offset_certified||!frame.stereo||!frame.stereo.reference_generation||
           !stereo_render_active(frame.stereo.sample)||!stereo_tracking_valid(frame.stereo.sample,frame.allow_sim)||
           !stereo_geometry_valid(frame.stereo.sample)||frame.stereo.sample.received_ns<=0||
           frame.stereo.sample.received_ns>frame.now_ns||frame.now_ns-frame.stereo.sample.received_ns>100000000)return;
        CoverPhysicalDisplacement wall_physical{};
        if(!prepare_cover_view(frame.anchor,frame.view_anchor,proposal.offset,frame.units_per_metre,result.view)||
           !cover_physical_displacement(frame.stereo.reference,frame.stereo.sample.head,result.physical)||
           !cover_physical_in_wall(frame.anchor,frame.view_anchor,result.physical,wall_physical))return;
        result.wall_anchor=frame.anchor;result.view_anchor=frame.view_anchor;
        result.tracking=frame.stereo;
        result.view_valid=true;
        result.requested_right_metres=proposal.offset.right+wall_physical.right;
        result.requested_up_metres=proposal.offset.up+result.physical.up;
        result.body_weight_valid=cover_up_weight(has_curve_?height_curve_.data():nullptr,frame.height_key_count,
            result.requested_up_metres,result.requested_up_weight);
    }

    template<class T> static bool number_same(T a,T b){return std::memcmp(&a,&b,sizeof(T))==0;}
    static bool same(Vector a,Vector b){return number_same(a.x,b.x)&&number_same(a.y,b.y)&&number_same(a.z,b.z);}
    static bool same(Pose const& a,Pose const& b){
        return same(a.position,b.position)&&number_same(a.orientation.x,b.orientation.x)&&
            number_same(a.orientation.y,b.orientation.y)&&number_same(a.orientation.z,b.orientation.z)&&
            number_same(a.orientation.w,b.orientation.w);
    }
    static bool same(ControllerHand const& a,ControllerHand const& b){
        if(!same(a.grip,b.grip)||!same(a.aim,b.aim)||a.grip_flags!=b.grip_flags||a.aim_flags!=b.aim_flags||
           a.grip_active!=b.grip_active||a.aim_active!=b.aim_active||a.buttons!=b.buttons||a.buttons_active!=b.buttons_active||
           !number_same(a.trigger,b.trigger)||!number_same(a.squeeze,b.squeeze)||!number_same(a.stick_x,b.stick_x)||
           !number_same(a.stick_y,b.stick_y)||a.trigger_active!=b.trigger_active||a.squeeze_active!=b.squeeze_active||
           a.stick_active!=b.stick_active||!same(a.last_squeeze_press_grip,b.last_squeeze_press_grip)||
           a.last_squeeze_press_flags!=b.last_squeeze_press_flags||a.last_squeeze_press_sequence!=b.last_squeeze_press_sequence)return false;
        for(unsigned i=0;i<control_count;++i)if(a.pressed[i]!=b.pressed[i]||a.released[i]!=b.released[i])return false;
        return true;
    }
    static bool same(ControllerInputFrame const& a,ControllerInputFrame const& b){
        return a.source==b.source&&a.session==b.session&&a.sequence==b.sequence&&a.space_epoch==b.space_epoch&&
            a.profile_generation==b.profile_generation&&a.input_epoch==b.input_epoch&&a.predicted_time==b.predicted_time&&
            a.received_ns==b.received_ns&&a.sync_ns==b.sync_ns&&a.focused==b.focused&&
            same(a.hands[0],b.hands[0])&&same(a.hands[1],b.hands[1]);
    }
    static bool same(StereoResult const& a,StereoResult const& b){
        if(a.status!=b.status||a.reference_generation!=b.reference_generation||a.recentered!=b.recentered||
           a.heading_deferred!=b.heading_deferred||!same(a.reference,b.reference))return false;
        auto const& x=a.sample;auto const& y=b.sample;
        if(x.source!=y.source||x.session!=y.session||x.sequence!=y.sequence||x.received_ns!=y.received_ns||
           x.predicted_time!=y.predicted_time||x.head_flags!=y.head_flags||x.view_flags!=y.view_flags||
           x.focused!=y.focused||x.should_render!=y.should_render||x.space_epoch!=y.space_epoch||
           x.visible!=y.visible||!same(x.head,y.head))return false;
        for(unsigned i=0;i<2;++i){auto const& p=x.eyes[i];auto const& q=y.eyes[i];
            if(!same(p.pose,q.pose)||!number_same(p.fov.left,q.fov.left)||!number_same(p.fov.right,q.fov.right)||
               !number_same(p.fov.up,q.fov.up)||!number_same(p.fov.down,q.fov.down))return false;}
        return true;
    }
    static bool same(CoverObservation const& a,CoverObservation const& b){
        auto const& x=a.epochs;auto const& y=b.epochs;
        return a.sequence==b.sequence&&a.observed_ns==b.observed_ns&&a.cover_valid==b.cover_valid&&
            a.interaction_enabled==b.interaction_enabled&&x.world==y.world&&x.pawn==y.pawn&&x.cover==y.cover&&
            x.slot==y.slot&&x.session==y.session&&x.space==y.space&&x.reference==y.reference&&
            number_same(a.range.min_right,b.range.min_right)&&number_same(a.range.max_right,b.range.max_right)&&
            number_same(a.range.min_up,b.range.min_up)&&number_same(a.range.max_up,b.range.max_up)&&
            same(a.right_axis_tracking,b.right_axis_tracking)&&same(a.up_axis_tracking,b.up_axis_tracking)&&
            a.edge.id==b.edge.id&&a.edge.kind==b.edge.kind&&a.edge.allowed==b.edge.allowed&&
            a.edge.within_acquisition_reach==b.edge.within_acquisition_reach;
    }
    static bool same(LeftHandInteractionLease a,LeftHandInteractionLease b){
        return a.kind==b.kind&&a.lifecycle==b.lifecycle&&a.id==b.id;
    }
    static bool same(LeftHandInteractionRequests const& a,LeftHandInteractionRequests const& b){
        return a.cover_request_id==b.cover_request_id&&a.support==b.support&&a.ammo==b.ammo&&a.magazine==b.magazine&&
            a.magazine_observation_pending==b.magazine_observation_pending;
    }
    static bool same(CoverWeaponContribution const& a,CoverWeaponContribution const& b){
        return a.state==b.state&&same(a.incumbents[0],b.incumbents[0])&&same(a.incumbents[1],b.incumbents[1])&&same(a.requests,b.requests);
    }
    bool same_snapshot(CoverInputOwnerFrame const& frame) const {
        auto const& f=frozen_;
        if(frame.lifecycle!=f.lifecycle||frame.frame!=f.frame||!same(frame.cover,f.cover)||
           !same(frame.anchor.neutral_eye_world,f.anchor.neutral_eye_world)||!same(frame.anchor.forward_world,f.anchor.forward_world)||
           !same(frame.anchor.right_world,f.anchor.right_world)||!same(frame.stereo,f.stereo)||
           !same(frame.view_anchor.neutral_eye_world,f.view_anchor.neutral_eye_world)||
           !same(frame.view_anchor.forward_world,f.view_anchor.forward_world)||
           !same(frame.view_anchor.right_world,f.view_anchor.right_world)||
           (frame.controllers!=nullptr)!=has_controllers_||(has_controllers_&&!same(*frame.controllers,controllers_))||
           !same(frame.weapon_incumbents[0],f.weapon_incumbents[0])||!same(frame.weapon_incumbents[1],f.weapon_incumbents[1])||
           !same(frame.weapon_requests,f.weapon_requests)||(frame.height_curve!=nullptr)!=has_curve_||
           frame.height_key_count!=f.height_key_count||!number_same(frame.units_per_metre,f.units_per_metre)||
           frame.now_ns!=f.now_ns||frame.allow_sim!=f.allow_sim||frame.owner_context_valid!=f.owner_context_valid||
           frame.allow_index_pinch!=f.allow_index_pinch)return false;
        for(unsigned i=0;i<f.height_key_count;++i)
            if(!number_same(frame.height_curve[i].u,height_curve_[i].u)||
               !number_same(frame.height_curve[i].height_metres,height_curve_[i].height_metres))return false;
        return true;
    }
    CoverControllerAdapter adapter_;
    CoverControllerAdapter pinch_adapter_{control_trigger};
    LeftControlSelector control_selector_;
    CoverInteraction interaction_,pending_interaction_;
    LeftHandInteractionLease cover_lease_,pending_lease_;
    LeftHandInteractionOwner* owner_=nullptr;
    CoverInputOwnerFrame frozen_;
    ControllerInputFrame controllers_;
    std::array<CoverHeightKey,maximum_height_keys> height_curve_{};
    CoverControllerResult hand_;
    CoverWeaponContribution weapon_;
    CoverInputOwnerDecoded decoded_;
    CoverInputOwnerCandidate candidate_;
    CoverInputOwnerResult result_,pending_result_;
    std::uint64_t lifecycle_=0,frame_=0,press_token_=0,serial_=0;
    Phase phase_=Phase::empty;
    bool previous_down_=true,snapshot_valid_=false,has_controllers_=false,has_curve_=false;
    bool barrier_=false,barrier_next_=false;
};
} // namespace gears_vr
