#include "cover_input_owner.h"
#include "weapon_hand_owner_adapter.h"
#include "../native-capture/weapon_hand_broker.h"
#include <cstdio>
using namespace gears_vr;
static unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
static bool close(double a,double b){return std::abs(a-b)<1e-8;}
static bool no_lease(LeftHandInteractionDecision const& d){return d.lease.kind==LeftHandInteraction::none&&!d.new_grant;}
static CoverWeaponContribution absent(){CoverWeaponContribution w{};w.state=CoverWeaponState::absent_quiescent;return w;}
static CoverWeaponContribution ready(){CoverWeaponContribution w{};w.state=CoverWeaponState::ready;return w;}

struct Fixture {
    LeftHandInteractionOwner owner;CoverInputOwner kernel;ControllerEdgeCounter edges;
    ControllerInputFrame hands;CoverInputOwnerFrame frame;CoverInputOwnerDecoded decoded;
    CoverHeightKey curve[3]{{0,0},{.5,.3},{1,.7}};
    Fixture(){
        frame.lifecycle=1;frame.frame=1;frame.now_ns=1000000000;frame.units_per_metre=100;
        frame.cover.sequence=1;frame.cover.observed_ns=frame.now_ns;frame.cover.epochs={1,1,1,1,1,1,1};
        frame.cover.cover_valid=frame.cover.interaction_enabled=frame.owner_context_valid=true;
        frame.cover.range={-.6,.5,0,.7};frame.cover.edge={17,CoverEdgeKind::top,true,true};
        frame.anchor={{1000,-2000,-40},{1,0,0},{0,1,0}};frame.view_anchor=frame.anchor;
        frame.height_curve=curve;frame.height_key_count=3;
        frame.stereo.status=StereoStatus::accepted;frame.stereo.reference_generation=1;
        frame.stereo.reference={{0,1.6,0},{}};
        auto& s=frame.stereo.sample;s.session=41;s.sequence=1;s.space_epoch=2;
        s.focused=1;s.visible=1;s.should_render=1;s.view_flags=s.head_flags=15;
        s.head={{0,1.6,0},{}};s.eyes[0].pose={{-.032,1.6,0},{}};s.eyes[1].pose={{.032,1.6,0},{}};
        for(auto& eye:s.eyes)eye.fov={-.7,.7,.7,-.7};
        for(auto& hand:hands.hands){hand.grip_active=1;hand.grip_flags=15;hand.squeeze_active=1;}
        hands.hands[0].grip.position={.2,1.2,-.4};frame.controllers=&hands;publish();
    }
    void publish(){
        auto& s=frame.stereo.sample;s.received_ns=frame.now_ns;s.predicted_time=frame.now_ns+20000000;
        hands.source=s.source;hands.session=s.session;hands.sequence=s.sequence;hands.space_epoch=s.space_epoch;
        hands.focused=1;hands.received_ns=frame.now_ns;hands.sync_ns=frame.now_ns-1000;hands.predicted_time=s.predicted_time;
        edges.update(hands);C(controller_frame_shape_valid(hands));
    }
    void advance(float squeeze){
        ++frame.frame;++frame.cover.sequence;++frame.stereo.sample.sequence;frame.now_ns+=10000000;
        frame.cover.observed_ns=frame.now_ns;hands.hands[0].squeeze=squeeze;publish();
    }
    CoverInputOwnerDecoded begin(){decoded=kernel.begin(frame,owner);return decoded;}
    CoverInputOwnerResult run(CoverWeaponContribution weapon=absent()){
        const auto d=begin();C(d.accepted);
        const auto candidate=kernel.update_with_decoded(d.ticket,weapon,owner);
        return kernel.complete(d.ticket,candidate.accepted,owner);
    }
    CoverInputOwnerResult next(float squeeze,CoverWeaponContribution weapon=absent()){
        advance(squeeze);return run(weapon);
    }
};

static void held_offset(Fixture& f,bool lateral=false){
    if(lateral)f.frame.cover.edge.kind=CoverEdgeKind::left;
    f.run();f.next(.8f);
    if(lateral)f.hands.hands[0].grip.position.x-=.2;
    else f.hands.hands[0].grip.position.y-=.3;
    f.next(.8f);auto r=f.next(0);
    C(!r.interaction.owns_left_hand&&close(lateral?r.interaction.offset.right:r.interaction.offset.up,lateral?.2:.3));
}
static void head_delta(Fixture& f,double right,double up){
    auto& s=f.frame.stereo.sample;s.head.position={right,1.6+up,0};
    s.eyes[0].pose.position={right-.032,1.6+up,0};s.eyes[1].pose.position={right+.032,1.6+up,0};
}

static void exact_snapshot_and_opaque_ticket(){
    Fixture f;auto d=f.begin();C(d.accepted&&d.left_grip_known&&d.left_tracked&&!d.left_grip_down&&!d.press_token);
    auto copied_hands=f.hands;CoverHeightKey copied_curve[3];
    for(unsigned i=0;i<3;++i)copied_curve[i]=f.curve[i];
    auto copy=f.frame;copy.controllers=&copied_hands;copy.height_curve=copied_curve;
    auto repeated=f.kernel.begin(copy,f.owner);C(repeated.accepted&&repeated.ticket==d.ticket);
    C(repeated.press_token==d.press_token&&repeated.continuity_generation==d.continuity_generation);
    auto candidate=f.kernel.update_with_decoded(d.ticket,absent(),f.owner);C(candidate.accepted);
    C(f.kernel.update_with_decoded(d.ticket,absent(),f.owner).left_hand.status==candidate.left_hand.status);
    C(f.kernel.complete(d.ticket,true,f.owner).accepted);
    C(f.kernel.complete(d.ticket,true,f.owner).accepted); // Read-only completed retry.
    C(!f.kernel.update(f.frame,f.owner).accepted); // Old update retains duplicate rejection.

    f.advance(.8f);d=f.begin();C(d.press_token==1&&d.left_grip_down);
    Fixture other;auto other_ticket=other.begin().ticket;
    C(other_ticket!=d.ticket&&!f.kernel.complete(other_ticket,true,f.owner).accepted);
    candidate=f.kernel.update_with_decoded(d.ticket,absent(),f.owner);C(candidate.accepted&&candidate.left_hand.new_grant);
    C(f.kernel.complete(d.ticket,true,f.owner).interaction.owns_left_hand);
    f.hands.hands[0].grip.position.y=.9;f.advance(.8f);const auto selected_tracking=f.frame.stereo;d=f.begin();
    // No pointer survives begin: proposal/height/view use copied data even if
    // the caller's storage is overwritten before arbitration.
    f.hands.hands[0].grip.position.y=-9;f.curve[1].height_metres=.6;f.frame.anchor.neutral_eye_world.z=9000;
    f.frame.stereo.sample.head.position.y+=7;
    for(auto& eye:f.frame.stereo.sample.eyes)eye.pose.position.y+=7;
    f.frame.cover.edge.allowed=false;
    candidate=f.kernel.update_with_decoded(d.ticket,absent(),f.owner);C(candidate.accepted);
    auto r=f.kernel.complete(d.ticket,true,f.owner);
    C(close(r.interaction.offset.up,.3)&&close(r.requested_up_weight,.5)&&close(r.view.base_eye_world.z,-10));
    C(stereo_same(r.tracking.sample,selected_tracking.sample)&&stereo_same_pose(r.tracking.reference,selected_tracking.reference));

    Fixture bounded;CoverHeightKey curve[33]{};bounded.frame.height_curve=curve;bounded.frame.height_key_count=33;
    C(!bounded.begin().accepted);bounded.advance(0);bounded.frame.height_key_count=32;
    C(bounded.begin().accepted);bounded.kernel.complete(bounded.decoded.ticket,false,bounded.owner);
    bounded.advance(0);bounded.frame.height_curve=nullptr;bounded.frame.height_key_count=1;C(!bounded.begin().accepted);
}

static void snapshot_mutations_abort(){
    for(unsigned mutation=0;mutation<21;++mutation){
        Fixture f;f.run();f.advance(.8f);auto d=f.begin();C(d.accepted&&d.press_token==1);
        switch(mutation){
        case 0:++f.frame.now_ns;break;
        case 1:++f.frame.cover.epochs.world;break;
        case 2:++f.frame.cover.epochs.pawn;break;
        case 3:++f.frame.stereo.reference_generation;break;
        case 4:f.frame.stereo.reference.position.x+=.1;break;
        case 5:f.frame.stereo.recentered=true;break;
        case 6:++f.hands.input_epoch;break;
        case 7:++f.hands.hands[0].pressed[control_squeeze];break;
        case 8:f.frame.cover.edge.allowed=false;break;
        case 9:f.frame.anchor.right_world.x+=.1;break;
        case 10:f.frame.units_per_metre=101;break;
        case 11:f.curve[1].height_metres=.4;break;
        case 12:f.frame.owner_context_valid=false;break;
        case 13:f.frame.controllers=nullptr;break;
        case 14:f.frame.weapon_requests.ammo=true;break;
        case 15:f.frame.stereo.sample.eyes[1].fov.left=-.6;break;
        case 16:++f.frame.cover.epochs.session;break;
        case 17:f.frame.allow_sim=true;break;
        case 18:f.frame.view_anchor.neutral_eye_world.x+=.1;break;
        case 19:f.frame.view_anchor.forward_world.x+=.1;break;
        default:f.frame.view_anchor.right_world.y+=.1;break;
        }
        C(!f.begin().accepted);const auto rejected=f.kernel.complete(d.ticket,true,f.owner);
        C(!rejected.accepted&&!rejected.view_valid&&!rejected.body_weight_valid);
        C(!f.kernel.update_with_decoded(d.ticket,absent(),f.owner).accepted);
    }
    Fixture f;f.run();f.advance(.8f);auto d=f.begin();
    auto candidate=f.kernel.update_with_decoded(d.ticket,absent(),f.owner);C(candidate.left_hand.new_grant);
    auto changed=ready();changed.requests.support=true;
    C(!f.kernel.update_with_decoded(d.ticket,changed,f.owner).accepted);
    C(!f.kernel.complete(d.ticket,true,f.owner).accepted);
}

static void prepare_busy_and_abandoned_frames(){
    Fixture f;f.run();f.advance(.8f);auto d=f.begin();
    CoverWeaponContribution blocked{};
    auto candidate=f.kernel.update_with_decoded(d.ticket,blocked,f.owner);
    C(!candidate.accepted&&no_lease(candidate.left_hand)&&candidate.left_hand.release_required);
    C(!f.kernel.complete(d.ticket,true,f.owner).accepted);
    auto r=f.next(.8f,ready());C(r.accepted&&!r.interaction.owns_left_hand&&no_lease(r.left_hand));
    C(f.decoded.press_token==d.press_token);
    r=f.next(0,ready());C(r.left_hand.status==LeftHandInteractionStatus::released);
    r=f.next(.8f,ready());C(r.interaction.owns_left_hand&&r.left_hand.press_token==d.press_token+1);

    for(bool after_decision:{false,true}){
        Fixture abandoned;abandoned.run();abandoned.advance(.8f);const auto old=abandoned.begin();
        if(after_decision)C(abandoned.kernel.update_with_decoded(old.ticket,absent(),abandoned.owner).left_hand.new_grant);
        abandoned.advance(.8f);const auto next=abandoned.begin();C(next.accepted&&next.ticket!=old.ticket);
        C(next.press_token==old.press_token);
        candidate=abandoned.kernel.update_with_decoded(next.ticket,absent(),abandoned.owner);
        C(candidate.accepted&&no_lease(candidate.left_hand));
        C(!abandoned.kernel.complete(old.ticket,true,abandoned.owner).accepted);
        C(!abandoned.kernel.complete(next.ticket,true,abandoned.owner).interaction.owns_left_hand);
        abandoned.next(0);C(abandoned.next(.8f).interaction.owns_left_hand);
    }
}

static void failed_commit_keeps_only_committed_offset(){
    Fixture f;f.run();C(f.next(.8f).interaction.owns_left_hand);
    f.hands.hands[0].grip.position.y=1.;auto r=f.next(.8f);C(close(r.interaction.offset.up,.2));
    f.hands.hands[0].grip.position.y=.8;f.advance(.8f);auto d=f.begin();
    auto candidate=f.kernel.update_with_decoded(d.ticket,absent(),f.owner);
    C(candidate.accepted&&candidate.left_hand.lease.kind==LeftHandInteraction::cover);
    r=f.kernel.complete(d.ticket,false,f.owner);
    C(!r.accepted&&r.view_valid&&!r.interaction.owns_left_hand&&close(r.interaction.offset.up,.2));
    C(r.interaction.offset_certified&&no_lease(r.left_hand)&&r.left_hand.release_required);
    C(r.body_weight_valid&&close(r.requested_up_metres,.2)&&close(r.view.base_eye_world.z,-20));
    C(!f.kernel.complete(d.ticket,true,f.owner).accepted);
    f.hands.hands[0].grip.position.y=.7;r=f.next(.8f);
    C(!r.interaction.owns_left_hand&&close(r.interaction.offset.up,.2));
    f.next(0);r=f.next(.8f);C(r.interaction.owns_left_hand&&close(r.interaction.offset.up,.2));
    C(r.left_hand.press_token==d.press_token+1);
    f.hands.hands[0].grip.position.y=.6;r=f.next(.8f);C(close(r.interaction.offset.up,.3));
}

static void blocked_hand_keeps_held_offset_and_live_head(){
    for(bool lateral:{false,true}){
        Fixture f;held_offset(f,lateral);const auto token=f.decoded.press_token;
        head_delta(f,.15,.2);auto r=f.next(0,CoverWeaponContribution{});
        C(!r.accepted&&r.view_valid&&r.body_weight_valid&&r.interaction.offset_certified);
        C(no_lease(r.left_hand)&&!r.interaction.owns_left_hand&&!r.interaction.request_left_hand);
        C(close(r.interaction.offset.up,lateral?0:.3)&&close(r.interaction.offset.right,lateral?.2:0));
        C(close(r.physical.right,.15)&&close(r.physical.up,.2));
        C(close(r.requested_up_metres,lateral?.2:.5)&&close(r.requested_right_metres,lateral?.35:.15));
        // Physical HMD remains for stereo composition; the base contains only
        // the held virtual displacement, never a second physical displacement.
        C(close(r.view.base_eye_world.z,lateral?-40:-10)&&close(r.view.base_eye_world.y,lateral?-1980:-2000));
        f.frame.controllers=nullptr;head_delta(f,-.25,.35);r=f.next(0,CoverWeaponContribution{});
        C(!r.accepted&&r.view_valid&&r.body_weight_valid&&!f.decoded.left_tracked);
        C(close(r.requested_up_metres,lateral?.35:.65)&&close(r.requested_right_metres,lateral?-.05:-.25));
        C(f.decoded.press_token==token&&no_lease(r.left_hand));
        // Same-ticket reads neither refresh the timestamp nor incorporate a
        // changed caller HMD. Mutating that frozen frame explicitly revokes it.
        const auto ticket=f.decoded.ticket;head_delta(f,4,5);
        auto cached=f.kernel.complete(ticket,false,f.owner);C(cached.view_valid&&close(cached.physical.up,.35));
        C(!f.begin().accepted);C(!f.kernel.complete(ticket,false,f.owner).view_valid);
    }

    Fixture release;held_offset(release);head_delta(release,.1,.2);
    auto r=release.next(0,CoverWeaponContribution{});C(!r.accepted&&r.view_valid);
    // A new Scene may use the exact same XR release packet. Recertification
    // did not record a synthetic inactive hand with that packet's sequence.
    ++release.frame.frame;++release.frame.cover.sequence;release.frame.now_ns+=10000000;
    release.frame.cover.observed_ns=release.frame.now_ns;r=release.run();
    C(r.left_hand.status==LeftHandInteractionStatus::released&&r.interaction.status==CoverStatus::okay);
    r=release.next(.8f);C(r.interaction.owns_left_hand&&close(r.interaction.offset.up,.3));
}

static void failed_transaction_recertifies_current_context_only(){
    for(unsigned change=0;change<5;++change){
        Fixture f;held_offset(f);head_delta(f,.15,.1);
        if(change==0)++f.frame.cover.epochs.world;
        if(change==1)++f.frame.cover.epochs.pawn;
        if(change==2)++f.frame.cover.epochs.cover;
        if(change==3)f.frame.cover.range.max_up=.1;
        if(change==4)f.frame.cover.cover_valid=false;
        f.frame.anchor.neutral_eye_world={200,300,400};
        f.frame.view_anchor.neutral_eye_world=f.frame.anchor.neutral_eye_world;
        auto r=f.next(0,CoverWeaponContribution{});
        C(!r.accepted&&no_lease(r.left_hand)&&!r.interaction.owns_left_hand);
        C(close(r.interaction.offset.up,change==3?.3:0));
        if(change==4){
            C(!r.view_valid&&!r.body_weight_valid&&r.interaction.status==CoverStatus::no_cover);
            // A failed participant cannot hide a certified cover exit and
            // resurrect the old offset on reentry under the same epochs.
            f.frame.cover.cover_valid=true;r=f.next(0,CoverWeaponContribution{});
            C(r.view_valid&&close(r.interaction.offset.up,0)&&close(r.requested_up_metres,.1));
        }else if(change==3){
            C(!r.view_valid&&!r.body_weight_valid&&!r.interaction.offset_certified);
            C(r.interaction.status==CoverStatus::range_invalidated);
            // A failed participant cannot turn a range clamp into a newly
            // certified body/view position. Restore the old legal range only.
            f.frame.cover.range.max_up=.7;r=f.next(0,CoverWeaponContribution{});
            C(r.view_valid&&close(r.interaction.offset.up,.3)&&close(r.requested_up_metres,.4));
        }else{
            C(r.view_valid&&r.body_weight_valid&&r.interaction.offset_certified);
            C(close(r.view.base_eye_world.z,400)&&close(r.requested_up_metres,.1));
            C(close(r.view.base_eye_world.x,200)&&close(r.view.base_eye_world.y,300));
        }
    }
    for(unsigned invalid=0;invalid<10;++invalid){
        Fixture f;held_offset(f);f.advance(0);
        switch(invalid){
        case 0:f.frame.cover.observed_ns=f.frame.now_ns-100000001;break;
        case 1:f.frame.cover.observed_ns=f.frame.now_ns+1;break;
        case 2:f.frame.stereo.sample.received_ns=f.frame.now_ns-100000001;break;
        case 3:f.frame.stereo.sample.received_ns=f.frame.now_ns+1;break;
        case 4:f.frame.owner_context_valid=false;break;
        case 5:f.frame.stereo.sample.head_flags=0;break;
        case 6:f.frame.stereo.reference_generation=0;break;
        case 7:f.frame.anchor.forward_world={};break;
        case 8:f.frame.cover.range.max_up=-.1;break;
        default:f.frame.cover.epochs.world=0;break;
        }
        const auto r=f.run(CoverWeaponContribution{});
        C(!r.accepted&&!r.view_valid&&!r.body_weight_valid&&no_lease(r.left_hand));
    }
    Fixture boundary;held_offset(boundary);boundary.advance(0);boundary.frame.now_ns+=100000000;
    boundary.frame.cover.observed_ns=boundary.frame.stereo.sample.received_ns=boundary.frame.now_ns-100000000;
    C(boundary.run(CoverWeaponContribution{}).view_valid); // The specified 100ms bound is inclusive.
    const auto ticket=boundary.decoded.ticket;boundary.kernel.invalidate(boundary.owner);
    C(!boundary.kernel.complete(ticket,false,boundary.owner).view_valid); // No new Scene proof.
    boundary.advance(0);const auto current=boundary.begin();
    C(!boundary.kernel.complete(ticket,false,boundary.owner).view_valid); // Old ticket cannot use a new Scene.
    boundary.kernel.complete(current.ticket,false,boundary.owner);
}

static void raw_epoch_counters_do_not_become_tokens(){
    for(unsigned boundary=0;boundary<3;++boundary){
        Fixture f;f.run();f.next(.8f);f.next(0);auto r=f.next(.8f);
        C(r.interaction.owns_left_hand&&f.decoded.press_token==2&&f.hands.hands[0].pressed[control_squeeze]==2);
        if(boundary==0){++f.hands.profile_generation;f.edges.profile_changed();}
        if(boundary==1){++f.frame.stereo.sample.space_epoch;++f.frame.cover.epochs.space;}
        if(boundary==2){++f.frame.stereo.sample.session;++f.frame.cover.epochs.session;}
        r=f.next(.8f);
        C(!r.interaction.owns_left_hand&&!f.decoded.left_grip_known&&f.decoded.press_token==2);
        C(f.hands.hands[0].pressed[control_squeeze]==0); // The producer reset; Native did not.
        r=f.next(.8f);C(no_lease(r.left_hand)&&f.decoded.press_token==2);
        f.next(0);r=f.next(.8f);
        C(r.interaction.owns_left_hand&&f.decoded.press_token==3&&f.hands.hands[0].pressed[control_squeeze]==1);
    }
}

static void outside_cover_outage_and_global_barriers(){
    Fixture f;f.frame.cover.cover_valid=false;auto weapon=ready();weapon.requests.support=true;
    auto r=f.run(weapon);C(r.accepted&&!r.view_valid&&r.left_hand.status==LeftHandInteractionStatus::released);
    r=f.next(.8f,weapon);C(r.left_hand.lease.kind==LeftHandInteraction::support&&r.left_hand.new_grant);
    weapon.incumbents[0]=r.left_hand.lease;r=f.next(.8f,weapon);
    C(r.left_hand.status==LeftHandInteractionStatus::retained&&!r.interaction.owns_left_hand);
    const auto old_token=f.decoded.press_token;
    f.frame.controllers=nullptr;r=f.next(.8f,weapon);
    C(no_lease(r.left_hand)&&!f.decoded.left_grip_known&&!f.decoded.left_grip_down&&!f.decoded.left_tracked);
    f.frame.controllers=&f.hands;r=f.next(.8f,weapon);C(no_lease(r.left_hand));
    weapon.incumbents={};f.next(0,weapon);r=f.next(.8f,weapon);
    C(r.left_hand.lease.kind==LeftHandInteraction::support&&f.decoded.press_token==old_token+1);
    weapon.incumbents[0]=r.left_hand.lease;

    // Recenter is a global owner barrier, not just a cover-axis update.
    f.kernel.invalidate(f.owner);++f.frame.stereo.reference_generation;++f.frame.cover.epochs.reference;
    r=f.next(.8f,weapon);C(no_lease(r.left_hand)&&!f.decoded.left_tracked);
    r=f.next(.8f,weapon);C(no_lease(r.left_hand)&&f.decoded.left_tracked);
    weapon.incumbents={};f.next(0,weapon);r=f.next(.8f,weapon);C(r.left_hand.new_grant);

    // World/pawn/callback-cookie changes use this same explicit barrier;
    // Native additionally changes lifecycle so a lease cannot cross a cookie.
    const auto token_before_lifecycle=f.decoded.press_token;weapon.incumbents[0]=r.left_hand.lease;
    f.kernel.invalidate(f.owner);++f.frame.lifecycle;++f.frame.cover.epochs.world;++f.frame.cover.epochs.pawn;
    r=f.next(.8f,weapon);C(!r.accepted&&no_lease(r.left_hand)); // Old-lifecycle incumbent is invalid.
    weapon.incumbents={};r=f.next(.8f,weapon);C(no_lease(r.left_hand));
    f.next(0,weapon);r=f.next(.8f,weapon);
    C(r.left_hand.new_grant&&r.left_hand.lease.lifecycle==f.frame.lifecycle&&f.decoded.press_token==token_before_lifecycle+1);
}

namespace transport {
namespace w=gears_weapon_hand;
constexpr std::uint32_t worker=1,gt=2;
struct Endpoint {
    w::Broker* broker=nullptr;
    std::int64_t stamp=1000000000;
    std::uint32_t thread_id=worker;
    unsigned pins=0,unpins=0,prepares=0,commits=0,invalidates=0;
    bool allow_unpin=true,restore_pending=false;
    w::Reply prepare_reply=w::Reply::accepted,commit_reply=w::Reply::accepted;
    w::Frame observed{};
    static bool pin(void* context,w::Registration const&,w::ModulePin& out) noexcept {
        ++static_cast<Endpoint*>(context)->pins;out={0x8000,0x8000};return true;
    }
    static bool unpin(void* context,w::ModulePin value) noexcept {
        auto& e=*static_cast<Endpoint*>(context);++e.unpins;
        C(value.handle==0x8000&&e.thread_id==worker);
        C(!e.broker->pending_callbacks()&&!e.broker->pending_transactions());return e.allow_unpin;
    }
    static std::int64_t clock(void* context) noexcept {return static_cast<Endpoint*>(context)->stamp;}
    static std::uint32_t thread(void* context) noexcept {return static_cast<Endpoint*>(context)->thread_id;}
    static std::uint32_t prepare(void* context,w::Frame const* frame,w::Proposal* proposal) noexcept {
        auto& e=*static_cast<Endpoint*>(context);++e.prepares;e.observed=*frame;
        C(e.broker->pending_callbacks()==1&&e.broker->pending_transactions()==1);
        *proposal={};proposal->key=frame->key;proposal->nonce=frame->key.frame+10;proposal->support=1;
        return unsigned(e.prepare_reply);
    }
    static std::uint32_t commit(void* context,w::Frame const*,w::Proposal const*,w::Decision const*) noexcept {
        auto& e=*static_cast<Endpoint*>(context);++e.commits;
        C(e.broker->pending_callbacks()==1&&e.broker->pending_transactions()==1);return unsigned(e.commit_reply);
    }
    static void invalidate(void* context,w::Invalidation const*) noexcept {
        auto& e=*static_cast<Endpoint*>(context);++e.invalidates;
        C(!e.broker->authorized()&&e.broker->pending_callbacks()==1);
        // This void callback can only disarm publication; it cannot certify
        // or perform a GT resource journal restoration.
    }
    w::BrokerBackend backend(){return {this,pin,unpin,clock,thread,
        w::weapon_broker_win32::prepare,w::weapon_broker_win32::commit,w::weapon_broker_win32::invalidate};}
};
struct AbortTransaction {
    Fixture& f;w::Broker& broker;CoverInputOwnerTicket ticket;w::FrameKey key;std::uint64_t nonce;
    ~AbortTransaction(){
        C(!f.kernel.complete(ticket,false,f.owner).accepted);
        C(broker.abort_frame(key,nonce)==w::Reply::quiescent);
    }
};
static w::Frame frame(Fixture const& f,CoverInputOwnerDecoded const& decoded,std::uint64_t cookie){
    w::Frame out{};out.flags=w::scene_owned|w::input_matched|w::gameplay_interactable;
    out.key={400,500,cookie,f.frame.lifecycle,f.frame.frame,f.frame.cover.epochs.world,42,gt};
    out.now_ns=f.frame.now_ns;out.world={0x1000,0,0x9000,1,2,0,1};
    out.controller={0x2000,0x1000,0x9000,2,3,0,1};out.pawn={0x3000,0x1000,0x9000,3,4,0,1};
    out.mesh={0x4000,0x3000,0x9000,4,5,0,1};out.controllers=f.hands;out.stereo=f.frame.stereo.sample;
    out.reference=f.frame.stereo.reference;out.reference_generation=f.frame.stereo.reference_generation;
    out.stereo_status=unsigned(f.frame.stereo.status);out.press_token=decoded.press_token;
    out.left_grip_down=decoded.left_grip_down;out.left_grip_known=decoded.left_grip_known;out.left_tracked=decoded.left_tracked;
    C(w::frame_active(out,out.now_ns));return out;
}
static CoverWeaponContribution contribution(w::Frame const& f,w::Proposal const& p){
    C(w::proposal_valid(p,f));LeftHandInteractionFrame merged{};w::append_weapon_proposal(p,merged);
    auto result=ready();result.incumbents=merged.incumbents;result.requests=merged.requests;return result;
}
static CoverInputOwnerResult run(Fixture& f,Endpoint& endpoint,w::Broker& broker,std::uint64_t cookie){
    endpoint.thread_id=gt;endpoint.stamp=f.frame.now_ns;const auto decoded=f.begin();C(decoded.accepted);
    const auto wire=frame(f,decoded,cookie);w::Proposal proposal{};
    const bool prepared=broker.prepare(wire,proposal)==w::Reply::accepted;
    const auto weapon=prepared?contribution(wire,proposal):CoverWeaponContribution{};
    const auto candidate=f.kernel.update_with_decoded(decoded.ticket,weapon,f.owner);
    const bool committed=prepared&&candidate.accepted&&
        broker.commit(wire,proposal,w::owner_decision(wire,proposal,candidate.left_hand,wire.now_ns))==w::Reply::accepted;
    const auto result=f.kernel.complete(decoded.ticket,committed,f.owner);
    if(broker.pending_transactions())broker.abort_frame(wire.key,proposal.nonce);
    return result;
}
} // namespace transport

static void broker_prepare_unavailable_and_commit_busy_keep_physical_pose(){
    namespace w=gears_weapon_hand;using namespace transport;
    Endpoint endpoint;w::Broker broker{{42,worker,400,500},endpoint.backend()};endpoint.broker=&broker;
    w::Registration registration{};registration.context=&endpoint;registration.prepare=Endpoint::prepare;
    registration.commit=Endpoint::commit;registration.invalidate=Endpoint::invalidate;w::RegistrationResult registered{};
    C(broker.register_participant(&registration,&registered)==w::Reply::accepted);
    Fixture f;C(run(f,endpoint,broker,registered.cookie).accepted);
    f.advance(.8f);C(run(f,endpoint,broker,registered.cookie).interaction.owns_left_hand);
    f.hands.hands[0].grip.position.y=.9;f.advance(.8f);
    C(close(run(f,endpoint,broker,registered.cookie).interaction.offset.up,.3));
    f.advance(0);C(close(run(f,endpoint,broker,registered.cookie).interaction.offset.up,.3));
    const auto previous_commits=endpoint.commits;
    endpoint.prepare_reply=w::Reply::unavailable;head_delta(f,.1,.2);f.advance(0);
    auto result=run(f,endpoint,broker,registered.cookie);
    C(!result.accepted&&result.view_valid&&result.body_weight_valid&&no_lease(result.left_hand));
    C(close(result.interaction.offset.up,.3)&&close(result.requested_up_metres,.5)&&close(result.requested_up_weight,.75));
    C(close(result.requested_right_metres,.1)&&endpoint.commits==previous_commits&&!broker.pending());
    endpoint.prepare_reply=w::Reply::accepted;endpoint.commit_reply=w::Reply::busy;
    head_delta(f,-.2,.3);f.advance(0);result=run(f,endpoint,broker,registered.cookie);
    C(!result.accepted&&result.view_valid&&result.body_weight_valid&&no_lease(result.left_hand));
    C(close(result.interaction.offset.up,.3)&&close(result.requested_up_metres,.6)&&close(result.requested_up_weight,.875));
    C(close(result.requested_right_metres,-.2)&&endpoint.commits==previous_commits+1&&!broker.pending_transactions());
    endpoint.thread_id=worker;C(broker.unregister(registered.cookie)==w::Reply::quiescent);
    C(broker.collect_on_owner_worker()==w::Reply::quiescent);
}

static void unregister_between_prepare_and_commit(){
    namespace w=gears_weapon_hand;using namespace transport;
    Endpoint endpoint;w::Broker broker{{42,worker,400,500},endpoint.backend()};endpoint.broker=&broker;
    w::Registration registration{};registration.context=&endpoint;registration.prepare=Endpoint::prepare;
    registration.commit=Endpoint::commit;registration.invalidate=Endpoint::invalidate;w::RegistrationResult registered{};
    C(broker.register_participant(&registration,&registered)==w::Reply::accepted&&endpoint.pins==1);
    Fixture f;endpoint.thread_id=gt;
    auto d=f.begin();auto wire=frame(f,d,registered.cookie);w::Proposal proposal{};
    C(broker.prepare(wire,proposal)==w::Reply::accepted);
    auto weapon=contribution(wire,proposal);auto candidate=f.kernel.update_with_decoded(d.ticket,weapon,f.owner);
    C(candidate.accepted&&no_lease(candidate.left_hand));
    auto decision=w::owner_decision(wire,proposal,candidate.left_hand,wire.now_ns);
    C(broker.commit(wire,proposal,decision)==w::Reply::accepted);
    C(f.kernel.complete(d.ticket,true,f.owner).accepted);

    f.advance(.8f);endpoint.stamp=f.frame.now_ns;d=f.begin();wire=frame(f,d,registered.cookie);
    C(d.press_token==1&&d.left_grip_known&&d.left_grip_down);
    C(broker.prepare(wire,proposal)==w::Reply::accepted);
    C(endpoint.observed.press_token==d.press_token&&endpoint.observed.left_grip_down==1);
    {
        AbortTransaction cleanup{f,broker,d.ticket,wire.key,proposal.nonce};
        weapon=contribution(wire,proposal);candidate=f.kernel.update_with_decoded(d.ticket,weapon,f.owner);
        C(candidate.accepted&&candidate.left_hand.new_grant&&candidate.left_hand.lease.kind==LeftHandInteraction::cover);
        decision=w::owner_decision(wire,proposal,candidate.left_hand,wire.now_ns);
        C(broker.pending_callbacks()==0&&broker.pending_transactions()==1&&broker.pin_retained());
        endpoint.thread_id=worker;endpoint.restore_pending=true;
        C(broker.unregister(registered.cookie)==w::Reply::draining);
        C(endpoint.invalidates==1&&endpoint.restore_pending);
        C(broker.collect_on_owner_worker()==w::Reply::draining&&endpoint.unpins==0);
        C(broker.pin_retained()&&broker.cookie()==registered.cookie&&broker.pending_transactions()==1);
        endpoint.thread_id=gt;f.kernel.invalidate(f.owner);
        C(broker.commit(wire,proposal,decision)==w::Reply::draining&&endpoint.commits==1);
        C(broker.pending_transactions()==1&&!f.kernel.complete(d.ticket,true,f.owner).accepted);
    } // Actual broker admission is dropped only by this GT abort.
    C(!broker.pending_transactions()&&broker.quiescent()&&broker.pin_retained());
    C(broker.collect_on_owner_worker()==w::Reply::invalid&&endpoint.unpins==0);
    endpoint.thread_id=worker;endpoint.allow_unpin=false;
    C(broker.collect_on_owner_worker()==w::Reply::unavailable&&broker.pin_retained());
    C(broker.cookie()==registered.cookie&&endpoint.restore_pending);
    endpoint.allow_unpin=true;C(broker.collect_on_owner_worker()==w::Reply::quiescent);
    C(broker.availability()==w::Availability::absent_quiescent&&!broker.pin_retained()&&endpoint.restore_pending);
    // Callback quiescence does not discharge the separate PG resource journal.
    auto r=f.next(.8f,CoverWeaponContribution{});C(!r.accepted&&no_lease(r.left_hand));
    endpoint.restore_pending=false;r=f.next(.8f,absent());C(!r.interaction.owns_left_hand);
    f.next(0);C(f.next(.8f).interaction.owns_left_hand);
}

static void owner_invalidate_poison_and_contribution_validation(){
    LeftHandInteractionOwner owner;LeftHandInteractionFrame frame{};
    frame.lifecycle=1;frame.frame=1;frame.context_valid=frame.tracked=true;owner.update(frame);
    ++frame.frame;++frame.press_token;frame.grip_down=true;frame.requests.support=true;
    auto grant=owner.update(frame);C(grant.new_grant);
    auto stopped=owner.invalidate();C(no_lease(stopped)&&stopped.release_required&&stopped.press_token==grant.press_token);
    C(owner.update(frame).status==LeftHandInteractionStatus::replayed);
    ++frame.frame;frame.incumbents[0]=grant.lease;C(no_lease(owner.update(frame)));
    ++frame.frame;frame.grip_down=false;frame.incumbents={};C(owner.update(frame).status==LeftHandInteractionStatus::released);
    ++frame.frame;++frame.press_token;frame.grip_down=true;C(owner.update(frame).new_grant);
    for(unsigned bad=0;bad<5;++bad){
        Fixture f;f.run();f.advance(.8f);auto d=f.begin();auto weapon=ready();
        if(bad==0)weapon.incumbents[0]={LeftHandInteraction::cover,1,91};
        if(bad==1)weapon.incumbents[0]={LeftHandInteraction::support,2,91};
        if(bad==2)weapon.requests.cover_request_id=91;
        if(bad==3){weapon.state=CoverWeaponState::absent_quiescent;weapon.requests.support=true;}
        if(bad==4)weapon.incumbents[0]={LeftHandInteraction::none,1,91};
        C(!f.kernel.update_with_decoded(d.ticket,weapon,f.owner).accepted);
        C(!f.kernel.complete(d.ticket,true,f.owner).accepted);
    }
}

static void clipping_and_range_loss_use_actual_completed_input_state(){
    Fixture f;f.run();f.next(.8f);
    f.hands.hands[0].grip.position.y=-1.2;
    auto r=f.next(.8f);
    C(r.accepted&&r.view_valid&&r.interaction.owns_left_hand&&
      r.interaction.status==CoverStatus::clipped&&close(r.interaction.offset.up,.7));
    f.hands.hands[0].grip.position.y+=.01;r=f.next(.8f);
    C(r.view_valid&&r.interaction.owns_left_hand&&close(r.interaction.offset.up,.69));
    f.next(0);f.frame.cover.range.max_up=.2;r=f.next(0);
    C(!r.view_valid&&!r.body_weight_valid&&!r.interaction.offset_certified&&
      !r.interaction.owns_left_hand&&close(r.interaction.offset.up,.69));
    C(r.interaction.status==CoverStatus::range_invalidated&&no_lease(r.left_hand));
    f.frame.cover.range.max_up=.7;r=f.next(.8f);
    C(r.view_valid&&close(r.interaction.offset.up,.69)&&!r.interaction.owns_left_hand);
    f.next(0);r=f.next(.8f);C(r.interaction.owns_left_hand&&close(r.interaction.offset.up,.69));

    Fixture failed;failed.run();failed.next(.8f);
    failed.hands.hands[0].grip.position.y=-1.2;failed.advance(.8f);
    auto decoded=failed.begin();
    C(failed.kernel.update_with_decoded(decoded.ticket,absent(),failed.owner).accepted);
    r=failed.kernel.complete(decoded.ticket,false,failed.owner);
    C(!r.accepted&&!r.interaction.owns_left_hand&&close(r.interaction.offset.up,0));
    failed.next(0);r=failed.next(.8f);C(r.interaction.owns_left_hand&&close(r.interaction.offset.up,0));
    failed.hands.hands[0].grip.position.y-=.01;r=failed.next(.8f);
    C(close(r.interaction.offset.up,.01)); // Aborted clip did not install its offset or new origin.
}

int main(){
    exact_snapshot_and_opaque_ticket();snapshot_mutations_abort();prepare_busy_and_abandoned_frames();
    failed_commit_keeps_only_committed_offset();blocked_hand_keeps_held_offset_and_live_head();
    failed_transaction_recertifies_current_context_only();raw_epoch_counters_do_not_become_tokens();outside_cover_outage_and_global_barriers();
    broker_prepare_unavailable_and_commit_busy_keep_physical_pose();unregister_between_prepare_and_commit();
    owner_invalidate_poison_and_contribution_validation();
    clipping_and_range_loss_use_actual_completed_input_state();
    std::printf("cover_input_transaction: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
