#include "weapon_hand_client.h"
#include "weapon_hand_owner_adapter.h"
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#ifdef NDEBUG
#error WeaponHandClientTest requires active assertions
#endif
using namespace gears_weapon_hand;
namespace {
thread_local std::uint32_t local_thread=99;
struct GT {GT(){local_thread=55;}~GT(){local_thread=99;}};
// Explicit local adapter/Native model, not game memory, a DLL pin, or headset
// acceptance. Run the real client, support kernel and sole shared owner.
struct Host {
    Frame frame{},live{};GripGameState game{};
    std::int64_t now=1001000000,advance_read{};bool metadata_known=true,reenter{},reenter_frame{},reenter_source{},invalidate_read{};
    unsigned reads{},source_reads{};HandClient* client{};Registration registration{};
    bool stage_ammo{},invalidate_notification{};
    unsigned notifications{};Lease staged{};std::uint64_t staged_after{};
    std::mutex mutex;std::condition_variable changed;bool block{},entered{},release{};
    gears_vr::LeftHandInteractionOwner owner;
    static ObjectIdentity object(std::uint64_t p){return {p,0x8000,0x9000,unsigned(p/8),7,0,1};}
    Host(){
        frame.key={11,22,33,1,1,1,44,55};frame.flags=scene_owned|input_matched|gameplay_interactable;
        frame.now_ns=1000000000;frame.world=object(0x1000);frame.controller=object(0x2000);
        frame.pawn=object(0x3000);frame.mesh=object(0x4000);frame.weapon=object(0x5000);
        frame.reference_generation=1;frame.stereo_status=unsigned(gears_vr::StereoStatus::accepted);
        auto& s=frame.stereo;s.session=1;s.sequence=100;s.received_ns=frame.now_ns-10000000;
        s.predicted_time=123;s.head_flags=s.view_flags=15;s.focused=s.visible=s.should_render=1;
        s.head={{0,1.6,0},{0,0,0,1}};s.eyes[0].pose.position={-.032,1.6,0};s.eyes[1].pose.position={.032,1.6,0};
        for(auto& eye:s.eyes){eye.pose.orientation={0,0,0,1};eye.fov={-.8,.8,.8,-.8};}
        auto& c=frame.controllers;c.session=1;c.sequence=100;c.space_epoch=s.space_epoch;c.predicted_time=123;c.focused=1;
        c.received_ns=s.received_ns;c.sync_ns=c.received_ns-1;
        for(auto& hand:c.hands){hand.grip_active=1;hand.grip_flags=15;}
        c.hands[1].grip.position={0,1,0};c.hands[0].grip.position={0,1,-.5};
        frame.left_grip_known=frame.left_tracked=1;
        game.known=game.main_hand_held=game.mount_known=true;game.kind=WeaponKind::firearm;
        game.mount_asset=object(0x6000);game.mount_revision=1;
        game.mount.support_grip_in_weapon={0,0,-.5};game.mount.muzzle_in_weapon={0,0,-1};
        live=frame;assert(frame_active(frame,now));
    }
    static bool observe(void* p,Frame const& f,std::int64_t,GripGameState& out) noexcept {
        auto& h=*static_cast<Host*>(p);++h.reads;
        if(h.block){std::unique_lock<std::mutex> lock(h.mutex);h.entered=true;h.changed.notify_all();
            h.changed.wait(lock,[&]{return h.release;});}
        if(h.reenter){h.reenter=false;Proposal proposal{};
            assert(h.registration.prepare(h.registration.context,&f,&proposal)==unsigned(Reply::busy));}
        if(h.reenter_frame){h.reenter_frame=false;Frame copy=h.frame;
            assert(!h.client->copy_frame_on_gt(copy)&&same_frame(copy,Frame{}));}
        if(h.reenter_source){h.reenter_source=false;assert(!h.client->source_current_on_gt(f.key));}
        if(h.invalidate_read){h.invalidate_read=false;h.invalidate();}
        if(!h.metadata_known||!same(f.world,h.live.world)||!same(f.controller,h.live.controller)||
           !same(f.pawn,h.live.pawn)||!same(f.mesh,h.live.mesh)||!same(f.weapon,h.live.weapon))return false;
        out=h.game;
        if(h.stage_ammo&&h.staged.kind==LeaseKind::ammo&&f.key.frame>h.staged_after&&
           h.staged.lifecycle==f.key.lifecycle&&h.staged.id==f.press_token&&f.left_grip_down){
            out.object_incumbents[0]=h.staged;out.request_ammo=false;
        }
        if(h.advance_read){h.now+=h.advance_read;h.advance_read=0;}return true;
    }
    static std::uint32_t thread(void*) noexcept {return local_thread==55?55u:0u;}
    static std::int64_t clock(void* p) noexcept {return static_cast<Host*>(p)->now;}
    static void committed(void* p,Frame const& f,Decision const& d) noexcept {
        auto& h=*static_cast<Host*>(p);++h.notifications;
        if(h.stage_ammo&&d.status==DecisionStatus::granted&&d.lease.kind==LeaseKind::ammo){
            h.staged=d.lease;h.staged_after=f.key.frame;
        }
        if(h.invalidate_notification)h.invalidate();
    }
    static bool observe_source(void* p,Frame const& f,std::int64_t now,GripGameState& out) noexcept {
        ++static_cast<Host*>(p)->source_reads;return observe(p,f,now,out);
    }
    ClientAdapter adapter(bool source_only=false) noexcept {
        return {this,observe,thread,clock,committed,source_only?observe_source:nullptr};
    }
    void connect(HandClient& c,std::uint64_t cookie=33){
        client=&c;registration=c.registration();assert(c.begin_connect({11,44})==Reply::accepted);
        RegistrationResult receipt{};receipt.cookie=cookie;receipt.native_started_qpc=22;
        assert(c.connected(Reply::accepted,receipt)==Reply::accepted);frame.key.registration_cookie=cookie;
    }
    void invalidate(InvalidateReason reason=InvalidateReason::context_lost,std::uint64_t cookie=0){
        Invalidation message{};message.registration_cookie=cookie?cookie:frame.key.registration_cookie;
        message.native_started_qpc=22;message.reason=reason;registration.invalidate(registration.context,&message);
    }
    void next(bool down=false){
        ++frame.key.frame;++frame.stereo.sequence;frame.controllers.sequence=frame.stereo.sequence;
        frame.now_ns+=10000000;now=frame.now_ns+1000000;frame.stereo.received_ns+=10000000;
        frame.controllers.received_ns=frame.stereo.received_ns;frame.controllers.sync_ns=frame.controllers.received_ns-1;
        if(down&&!frame.left_grip_down)++frame.press_token;
        frame.left_grip_down=down?1u:0u;
    }
    Reply prepare(Proposal& p){return Reply(registration.prepare(registration.context,&frame,&p));}
    Reply commit(Proposal const& p){
        gears_vr::LeftHandInteractionFrame input{};input.lifecycle=frame.key.lifecycle;input.frame=frame.key.frame;
        input.press_token=frame.press_token;input.context_valid=true;input.tracked=frame.left_tracked!=0;
        input.grip_down=frame.left_grip_down!=0;append_weapon_proposal(p,input);
        const auto decision=owner_decision(frame,p,owner.update(input),now);
        return Reply(registration.commit(registration.context,&frame,&p,&decision));
    }
    Grip step(){
        GT gt;Proposal p{};assert(prepare(p)==Reply::accepted);assert(commit(p)==Reply::accepted);
        GripSnapshot snapshot{};return client->snapshot_on_gt(snapshot)?snapshot.grip:Grip::unknown;
    }
    void ready(){step();next();assert(step()==Grip::single);next(true);assert(step()==Grip::two);}
    bool snapshot(){GT gt;GripSnapshot s{};return client->snapshot_on_gt(s);}
    bool copy(Frame& out){
        GT gt;GripGameState observed=game;const bool copied=client->copy_frame_on_gt(out,observed);
        assert(same_game(observed,copied?game:GripGameState{}));return copied;
    }
    bool current(FrameKey const& expected){GT gt;return client->source_current_on_gt(expected);}
    bool current_game(FrameKey const& expected,GripGameState& out){GT gt;return client->source_game_on_gt(expected,out);}
};
unsigned candidate_failure_observation_cases(){
    unsigned cases{};
    for(bool pending:{false,true}){
        Host h;HandClient c(h.adapter());h.connect(c);h.step();h.next();h.step();
        h.game.magazine_observation_pending=pending;
        if(!pending)h.frame.controllers.hands[0].grip.position.x=2.;
        h.next(true);h.step();
        // Expiry STILL rejects and clears the actual publication. The same
        // failing read now explains the committed empty proposal using POD;
        // observing a known contact cannot refresh or grant the expired Frame.
        h.now=(std::max)(h.frame.controllers.received_ns,h.frame.stereo.received_ns)+100000001;
        Frame out{};GripGameState observed{};Decision decision{};FrameReadObservation failure{};
        {GT gt;assert(!c.copy_frame_on_gt(out,observed,decision,&failure));
            assert(failure.failure==FrameReadFailure::stale_before);
            assert(c.withdrawal_on_gt().candidate.flags==failure.candidate.flags);}
        assert(!c.armed()&&same_frame(out,Frame{})&&same_game(observed,GripGameState{}));
        assert(failure.candidate.decision==unsigned(DecisionStatus::no_candidate)&&!failure.candidate.lease);
        assert(failure.candidate.flags==(pending?31u:13u));
        assert(pending?failure.candidate.support_distance_metres<1e-9:failure.candidate.support_distance_metres>1.);
        ++cases;
    }
    assert(!candidate_observation(Frame{},GripGameState{},Decision{}).flags);return cases+1;
}
unsigned single_hand_pending_marker_cases(){
    //67624: no left lease existed when pending-mag observation changed.
    // That acquisition-only marker must not revoke the current main hand.
    // It still cannot supply a part grant, turn a held miss into a press, or
    // relax the actual prepare/commit comparison.
    unsigned cases{};
    const auto single=[](Host& h,HandClient& c,bool pending){
        h.connect(c);h.frame.controllers.hands[0].grip.position.x=2.;
        h.step();h.next();h.step();h.game.magazine_observation_pending=pending;
        h.next(true);assert(h.step()==Grip::single);
    };
    for(bool pending:{false,true})for(bool snapshot_first:{false,true}){
        Host h;HandClient c(h.adapter());single(h,c,pending);
        h.game.magazine_observation_pending=!pending;
        if(snapshot_first)assert(h.snapshot());
        Frame out{};GripGameState game{};Decision decision{};
        {GT gt;assert(c.copy_frame_on_gt(out,game,decision));}
        assert(same_frame(out,h.frame)&&same_game(game,h.game)&&c.armed());
        assert(decision.lease.kind==LeaseKind::none&&!decision.new_grant);
        // No second edge: observing a different hover cannot acquire a part.
        h.game.magazine_observation_pending=false;h.game.request_magazine=true;
        h.next(true);assert(h.step()==Grip::single);
        {GT gt;assert(c.copy_frame_on_gt(out,game,decision));}
        assert(decision.lease.kind==LeaseKind::none&&!decision.new_grant);
        h.next(false);h.step();h.next(true);h.step();
        {GT gt;assert(c.copy_frame_on_gt(out,game,decision));}
        assert(decision.lease.kind==LeaseKind::magazine&&decision.new_grant);++cases;
    }
    for(unsigned change=0;change<8;++change){
        Host h;HandClient c(h.adapter());single(h,c,false);
        h.game.magazine_observation_pending=true;
        if(change==0)h.game.known=false;
        if(change==1)h.game.main_hand_held=false;
        if(change==2)h.game.animation_owns_hands=true;
        if(change==3)h.game.busy=true;
        if(change==4)++h.game.mount_revision;
        if(change==5)h.game.object_incumbents[0]={LeaseKind::magazine,0,h.frame.key.lifecycle,7};
        if(change==6)h.live.weapon=Host::object(0x5800);
        if(change==7)h.now+=100000001;
        Frame out{};GripGameState game{};Decision decision{};
        {GT gt;assert(!c.copy_frame_on_gt(out,game,decision));}
        assert(same_frame(out,Frame{})&&decision.lease.kind==LeaseKind::none&&!c.armed());++cases;
    }
    for(bool pending:{false,true}){
        Host h;HandClient c(h.adapter());single(h,c,pending);
        h.next(true);GT gt;Proposal proposal{};
        assert(h.prepare(proposal)==Reply::accepted);
        const auto notifications=h.notifications;h.game.magazine_observation_pending=!pending;
        assert(h.commit(proposal)!=Reply::accepted&&h.notifications==notifications&&!c.armed());++cases;
    }
    for(bool pending:{false,true})for(bool snapshot_first:{false,true}){
        Host h;HandClient c(h.adapter());single(h,c,false);
        h.next(false);h.step();h.game.request_magazine=true;h.next(true);h.step();
        Frame out{};GripGameState game{};Decision decision{};
        {GT gt;assert(c.copy_frame_on_gt(out,game,decision));}
        h.game.request_magazine=false;h.game.object_incumbents[0]=decision.lease;
        h.game.magazine_observation_pending=pending;h.next(true);h.step();
        h.game.magazine_observation_pending=!pending;
        if(snapshot_first)assert(h.snapshot());
        {GT gt;assert(c.copy_frame_on_gt(out,game,decision));}
        assert(decision.status==DecisionStatus::retained&&!decision.new_grant&&
            same(decision.lease,h.game.object_incumbents[0])&&c.armed());++cases;
    }
    // 69680 frame736: request_magazine AND pending changed (difference18).
    // Reproduce the retained-part case, in both directions and through both
    // consumers; this does not infer that original frame's decision status.
    for(bool hover:{false,true})for(bool pending:{false,true})for(bool snapshot_first:{false,true}){
        Host h;HandClient c(h.adapter());single(h,c,false);
        h.next(false);h.step();h.game.request_magazine=true;h.next(true);h.step();
        Frame out{};GripGameState game{};Decision decision{};
        {GT gt;assert(c.copy_frame_on_gt(out,game,decision));}
        h.game.object_incumbents[0]=decision.lease;h.game.request_magazine=hover;
        h.game.magazine_observation_pending=pending;h.next(true);h.step();
        h.game.request_magazine=!hover;h.game.magazine_observation_pending=!pending;
        if(snapshot_first)assert(h.snapshot());
        {GT gt;assert(c.copy_frame_on_gt(out,game,decision));}
        assert(decision.status==DecisionStatus::retained&&!decision.new_grant&&
            same_frame(out,h.frame)&&same_game(game,h.game)&&
            same(decision.lease,h.game.object_incumbents[0])&&c.armed());++cases;
    }
    {
        Host h;HandClient c(h.adapter());single(h,c,false);
        h.next(false);h.step();h.game.request_magazine=true;h.next(true);h.step();
        Frame out{};GripGameState game{};Decision decision{};
        {GT gt;assert(c.copy_frame_on_gt(out,game,decision));}
        assert(decision.new_grant);
        auto committed=game;committed.object_incumbents[0]=decision.lease;
        auto changed=committed;changed.request_magazine=false;changed.magazine_observation_pending=true;
        assert(!same_pose_consumer_observation(out,decision,changed,committed));++cases;
    }
    for(unsigned changed=0;changed<5;++changed){
        Host h;HandClient c(h.adapter());single(h,c,false);
        h.next(false);h.step();h.game.request_magazine=true;h.next(true);h.step();
        Frame out{};GripGameState game{};Decision decision{};
        {GT gt;assert(c.copy_frame_on_gt(out,game,decision));}
        h.game.request_magazine=false;h.game.object_incumbents[0]=decision.lease;h.next(true);h.step();
        h.game.magazine_observation_pending=true;h.game.request_magazine=true;
        if(changed==0)h.game.object_incumbents={};
        if(changed==1)++h.game.object_incumbents[0].id;
        if(changed==2)h.game.object_incumbents[1]=h.game.object_incumbents[0];
        if(changed==3)++h.game.mount_revision;
        if(changed==4)h.now+=100000001;
        {GT gt;assert(!c.copy_frame_on_gt(out,game,decision));}
        assert(!c.armed()&&same_frame(out,Frame{}));++cases;
    }
    return cases;
}
unsigned frame_copy_cases(){
    unsigned cases{};
    for(unsigned changed=0;changed<7;++changed){
        Host h;HandClient c(h.adapter());h.connect(c);h.ready();
        h.now+=100000001;
        Frame out{};GripGameState game{};Decision decision{};
        {GT gt;assert(!c.copy_frame_on_gt(out,game,decision));}
        assert(!c.armed()&&!h.snapshot()&&same_frame(out,Frame{}));
        // A real later publication, never a refreshed timestamp on old data.
        h.frame.now_ns+=120000000;h.frame.stereo.received_ns+=120000000;
        h.frame.controllers.received_ns+=120000000;h.frame.controllers.sync_ns+=120000000;
        if(changed==1)h.next(false);else h.next(true);
        if(changed==2){h.frame.weapon=Host::object(0x5800);h.live=h.frame;}
        if(changed==3)++h.frame.controllers.input_epoch;
        if(changed==4)++h.frame.reference_generation;
        if(changed==5)++h.game.mount_revision;
        if(changed==6){h.next(false);h.next(true);}
        const auto grip=h.step();
        if(changed==0)assert(grip==Grip::two);else assert(grip!=Grip::two);
        ++cases;
    }
    for(unsigned changed=0;changed<2;++changed){
        Host h;HandClient c(h.adapter());h.connect(c);h.ready();
        if(changed==0)h.game.request_ammo=true;else h.game.request_magazine=true;
        // Actual shot callers enter through snapshot without a preceding
        // copy_frame call. Neither a held trigger nor a hover proposal is a
        // new support acquisition, and neither may drop the existing grasp.
        assert(h.snapshot());h.next(true);assert(h.step()==Grip::two);++cases;
    }
    for(unsigned changed=0;changed<5;++changed){
        Host h;HandClient c(h.adapter());h.connect(c);h.ready();
        Frame out=h.frame;GripGameState game=h.game;Decision decision{};FrameReadObservation observation{};
        {GT gt;assert(c.copy_frame_on_gt(out,game,decision,&observation));}
        assert(observation.failure==FrameReadFailure::none&&!observation.game_difference);
        if(changed==0)h.game.request_ammo=true;
        if(changed==1)h.game.request_magazine=true;
        if(changed==2)h.game.object_incumbents[0]={LeaseKind::magazine,0,h.frame.key.lifecycle,7};
        if(changed==3)h.game.busy=true;
        if(changed==4)h.now+=100000001;
        if(changed<2){
            {GT gt;assert(c.copy_frame_on_gt(out,game,decision,&observation));}
            assert(same_frame(out,h.frame)&&same_game(game,h.game)&&decision.lease.kind==LeaseKind::support&&c.armed());
            assert(h.snapshot());h.next(true);assert(h.step()==Grip::two);++cases;continue;
        }
        {GT gt;assert(!c.copy_frame_on_gt(out,game,decision,&observation));}
        assert(same_frame(out,Frame{})&&same_game(game,GripGameState{})&&!decision.proposal_nonce&&!c.armed());
        assert(observation.failure==(changed==4?FrameReadFailure::stale_before:FrameReadFailure::game_changed));
        assert(observation.game_difference==(changed==4?0u:1u<<changed));++cases;
    }
    {Host h;h.stage_ammo=true;HandClient c(h.adapter());h.connect(c);h.game.mount_known=false;
        h.step();h.next();h.step();h.game.request_ammo=true;h.next(true);
        assert(h.step()==Grip::single);
        Frame f{};GripGameState g{};Decision d{};
        {GT gt;assert(c.copy_frame_on_gt(f,g,d));}
        assert(d.status==DecisionStatus::granted&&g.request_ammo&&g.object_incumbents[0].kind==LeaseKind::none);
        // No PlayerTick consumer acknowledged the grant. Its synchronous
        // receipt alone must already be visible to the NEXT arbitration.
        h.next(true);assert(h.step()==Grip::single);
        {GT gt;assert(c.copy_frame_on_gt(f,g,d));}
        assert(d.status==DecisionStatus::retained&&same(g.object_incumbents[0],h.staged));
        h.next(false);h.step();
        {GT gt;assert(c.copy_frame_on_gt(f,g,d));}
        assert(d.status==DecisionStatus::released&&d.lease.kind==LeaseKind::none);++cases;
    }
    {Host h;HandClient c(h.adapter());h.connect(c);GT gt;Proposal p{};
        assert(h.prepare(p)==Reply::accepted);h.metadata_known=false;
        assert(h.commit(p)!=Reply::accepted&&h.notifications==0);++cases;
    }
    {Host h;HandClient c(h.adapter());h.connect(c);GT gt;Proposal p{};
        assert(h.prepare(p)==Reply::accepted);h.invalidate_notification=true;
        assert(h.commit(p)!=Reply::accepted&&h.notifications==1);
        Frame f{};assert(!c.copy_frame_on_gt(f));++cases;
    }
    for(bool magazine:{false,true}){
        Host h;HandClient c(h.adapter());h.connect(c);h.game.mount_known=false;
        h.step();h.next();h.step();
        h.game.request_ammo=!magazine;h.game.request_magazine=magazine;
        h.next(true);assert(h.step()==Grip::single);
        Frame out{};GripGameState game{};Decision decision{};
        {GT gt;assert(c.copy_frame_on_gt(out,game,decision));}
        assert(same(out.key,decision.key)&&decision.status==DecisionStatus::granted&&
            decision.lease.kind==(magazine?LeaseKind::magazine:LeaseKind::ammo)&&
            decision.lease.id==out.press_token&&decision.proposal_nonce);
        h.game.object_incumbents[0]=decision.lease;
        h.game.request_ammo=h.game.request_magazine=false;
        h.frame.controllers.hands[0].grip.position.x+=.3;
        h.next(true);assert(h.step()==Grip::single);
        {GT gt;assert(c.copy_frame_on_gt(out,game,decision));}
        assert(decision.status==DecisionStatus::retained&&
            same(decision.lease,h.game.object_incumbents[0]));
        h.game.object_incumbents={};
        h.next(false);h.step();
        {GT gt;assert(c.copy_frame_on_gt(out,game,decision));}
        assert(decision.lease.kind==LeaseKind::none);
        h.invalidate();
        {GT gt;assert(!c.copy_frame_on_gt(out,game,decision));}
        assert(same_frame(out,Frame{})&&decision.lease.kind==LeaseKind::none&&!decision.proposal_nonce);
        ++cases;
    }
    for(bool unarmed:{false,true}){
        Host h;HandClient c(h.adapter());h.connect(c);h.game.main_hand_held=h.game.mount_known=false;
        if(unarmed){h.frame.weapon=h.live.weapon={};h.game.kind=WeaponKind::unknown;}
        assert(h.step()==Grip::unknown&&c.armed());
        Frame out{};const auto reads=h.reads;assert(h.copy(out));
        assert(h.reads==reads+1&&same_frame(out,h.frame)&&!std::memcmp(&out,&h.frame,sizeof(out)));
        assert(out.now_ns==h.frame.now_ns&&out.stereo.received_ns==h.frame.stereo.received_ns&&
            out.controllers.received_ns==h.frame.controllers.received_ns&&out.controllers.sync_ns==h.frame.controllers.sync_ns);
        Frame legacy{};const auto legacy_reads=h.reads;
        {GT gt;assert(c.copy_frame_on_gt(legacy));}
        assert(h.reads==legacy_reads+1&&same_frame(legacy,out));
        // Data availability does not establish a held weapon or support grant.
        assert(!h.snapshot()&&c.armed());++cases;
    }
    {Host h;HandClient c(h.adapter());h.connect(c);h.game.busy=h.game.animation_owns_hands=true;
        assert(h.step()==Grip::unknown);Frame out{};assert(h.copy(out)&&same_frame(out,h.frame)&&!h.snapshot());
        h.game.busy=h.game.animation_owns_hands=false;out=h.frame;
        assert(!h.copy(out)&&same_frame(out,Frame{}));
        h.next();assert(h.step()==Grip::single&&h.copy(out));++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);Frame out=h.frame;
        assert(!h.copy(out)&&same_frame(out,Frame{}));h.step();assert(h.copy(out));
        h.next();Proposal proposal{};{GT gt;assert(h.prepare(proposal)==Reply::accepted);}
        out=h.frame;const auto reads=h.reads;
        assert(!h.copy(out)&&same_frame(out,Frame{})&&h.reads==reads);
        {GT gt;assert(h.commit(proposal)==Reply::accepted);}
        assert(h.copy(out)&&same_frame(out,h.frame));++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();Frame out=h.frame;
        assert(!c.copy_frame_on_gt(out)&&same_frame(out,Frame{})&&c.armed());
        assert(h.copy(out)&&same_frame(out,h.frame)&&h.snapshot());++cases;}
    for(auto reason:{InvalidateReason::context_lost,InvalidateReason::input_lost}){
        Host h;HandClient c(h.adapter());h.connect(c);h.ready();Frame out{};
        h.invalidate(reason,32);assert(h.copy(out));h.invalidate(reason);out=h.frame;
        assert(!h.copy(out)&&same_frame(out,Frame{})&&!c.armed());
        h.next();h.step();assert(h.copy(out));++cases;
    }
    for(auto reason:{InvalidateReason::native_stopping,InvalidateReason::unregistering,InvalidateReason::callback_fault}){
        Host h;HandClient c(h.adapter());h.connect(c);h.ready();h.invalidate(reason);Frame out=h.frame;
        assert(!h.copy(out)&&same_frame(out,Frame{})&&!c.armed());
        h.next();Proposal proposal{};{GT gt;assert(h.prepare(proposal)==Reply::unavailable);}++cases;
    }
    for(unsigned changed=0;changed<5;++changed){
        Host h;HandClient c(h.adapter());h.connect(c);h.ready();
        ObjectIdentity* identities[]={&h.live.world,&h.live.controller,&h.live.pawn,&h.live.mesh,&h.live.weapon};
        ++identities[changed]->name_number;Frame out=h.frame;
        assert(!h.copy(out)&&same_frame(out,Frame{})&&!c.armed());
        h.live=h.frame;assert(!h.copy(out));++cases;
    }
    for(bool unknown:{false,true}){
        Host h;HandClient c(h.adapter());h.connect(c);h.ready();
        if(unknown)h.metadata_known=false;else ++h.game.mount_revision;
        Frame out=h.frame;assert(!h.copy(out)&&same_frame(out,Frame{})&&!c.armed());++cases;
    }
    for(unsigned invalid_key=0;invalid_key<5;++invalid_key){
        Host h;HandClient c(h.adapter());h.connect(c);h.ready();h.next();
        switch(invalid_key){case 0:++h.frame.key.registration_cookie;break;case 1:++h.frame.key.native_started_qpc;break;
            case 2:++h.frame.key.game_start;break;case 3:++h.frame.key.game_pid;break;case 4:++h.frame.key.thread_id;break;}
        Proposal proposal{};{GT gt;assert(h.prepare(proposal)==Reply::unavailable);}
        Frame out=h.frame;assert(!h.copy(out)&&same_frame(out,Frame{})&&!c.armed());++cases;
    }
    for(bool stale_stereo:{false,true}){
        Host h;HandClient c(h.adapter());h.connect(c);
        if(stale_stereo){h.frame.controllers.received_ns=h.frame.now_ns;h.frame.controllers.sync_ns=h.frame.now_ns-1;}
        else h.frame.stereo.received_ns=h.frame.now_ns;
        h.step();h.now=h.frame.now_ns+91000000;assert(h.now-h.frame.now_ns<=100000000);
        Frame out=h.frame;const auto reads=h.reads;
        assert(!h.copy(out)&&same_frame(out,Frame{})&&h.reads==reads);++cases;
    }
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();h.advance_read=91000000;
        Frame out=h.frame;const auto reads=h.reads;
        assert(!h.copy(out)&&same_frame(out,Frame{})&&h.reads==reads+1);
        assert(!h.copy(out));++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();h.invalidate_read=true;Frame out=h.frame;
        assert(!h.copy(out)&&same_frame(out,Frame{})&&!c.armed());assert(!h.copy(out));++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();h.reenter_frame=true;Frame out=h.frame;
        assert(!h.copy(out)&&same_frame(out,Frame{})&&!c.armed());assert(!h.copy(out));++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();h.block=true;Frame out=h.frame;bool copied=true;GripGameState observed=h.game;
        std::thread worker([&]{GT gt;copied=c.copy_frame_on_gt(out,observed);});
        {std::unique_lock<std::mutex> lock(h.mutex);assert(h.changed.wait_for(lock,std::chrono::seconds(2),[&]{return h.entered;}));}
        assert(c.begin_disconnect()==Reply::draining&&c.finish_disconnect(33)==Reply::draining);
        {std::lock_guard<std::mutex> lock(h.mutex);h.release=true;}h.changed.notify_all();worker.join();
        assert(!copied&&same_frame(out,Frame{})&&same_game(observed,GripGameState{})&&c.finish_disconnect(33)==Reply::quiescent);
        out=h.frame;assert(!h.copy(out)&&same_frame(out,Frame{}));++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();Frame out{};
        assert(h.copy(out)&&c.armed());h.invalidate(InvalidateReason::context_lost);
        assert(!c.armed()&&c.registration_alive(33,22)); // registration does not keep a copied observation current.
        GripGameState observed=h.game;{GT gt;assert(!c.copy_frame_on_gt(out,observed));}
        assert(same_frame(out,Frame{})&&same_game(observed,GripGameState{}));++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();Frame out=h.frame;GripGameState observed=h.game;
        assert(!c.copy_frame_on_gt(out,observed)&&same_frame(out,Frame{})&&same_game(observed,GripGameState{})&&c.armed());++cases;}
    return cases;
}
unsigned source_current_cases(bool source_only=false){
    unsigned cases{};
    {Host h;HandClient c(h.adapter(source_only));h.connect(c);h.game.main_hand_held=false;
        assert(h.step()==Grip::unknown);const auto key=h.frame.key;
        h.game.main_hand_held=h.game.busy=h.game.animation_owns_hands=true;
        const auto reads=h.reads;assert(h.current(key)&&h.reads==reads+1&&c.armed());
        assert(h.current(key));Frame out=h.frame;
        // Source validation must not rewrite the originally committed game.
        assert(!h.copy(out)&&same_frame(out,Frame{}));++cases;}
    {Host h;HandClient c(h.adapter(source_only));h.connect(c);h.ready();h.game.main_hand_held=false;h.game.busy=true;
        assert(h.current(h.frame.key)&&c.armed());
        // It must not refresh/authorize an existing frozen two-hand snapshot.
        assert(!h.snapshot()&&!c.armed());++cases;}
    {Host h;HandClient c(h.adapter(source_only));h.connect(c);h.ready();const auto old=h.frame.key;
        h.next();h.step();assert(!h.current(old)&&c.armed());Frame out{};
        assert(h.current(h.frame.key)&&h.copy(out)&&same_frame(out,h.frame));++cases;}
    {Host h;HandClient c(h.adapter(source_only));h.connect(c);h.ready();const auto key=h.frame.key;
        for(unsigned field=0;field<8;++field){auto wrong=key;
            switch(field){case 0:++wrong.game_start;break;case 1:++wrong.native_started_qpc;break;case 2:++wrong.registration_cookie;break;
                case 3:++wrong.lifecycle;break;case 4:++wrong.frame;break;case 5:++wrong.world_generation;break;
                case 6:++wrong.game_pid;break;case 7:++wrong.thread_id;break;}
            const auto reads=h.reads;assert(!h.current(wrong)&&c.armed()&&h.reads==reads);
            Frame out{};assert(h.copy(out)&&same_frame(out,h.frame));}
        assert(h.current(key));++cases;}
    {Host h;HandClient c(h.adapter(source_only));h.connect(c);h.ready();
        assert(!c.source_current_on_gt(h.frame.key)&&c.armed());Frame out{};
        assert(h.current(h.frame.key)&&h.copy(out));++cases;}
    {Host h;HandClient c(h.adapter(source_only));h.connect(c);assert(!h.current(h.frame.key));
        h.step();h.next();Proposal proposal{};{GT gt;assert(h.prepare(proposal)==Reply::accepted);}
        assert(!h.current(h.frame.key));{GT gt;assert(h.commit(proposal)==Reply::accepted);}
        assert(h.current(h.frame.key));++cases;}
    for(unsigned changed=0;changed<5;++changed){
        Host h;HandClient c(h.adapter(source_only));h.connect(c);h.ready();
        ObjectIdentity* identities[]={&h.live.world,&h.live.controller,&h.live.pawn,&h.live.mesh,&h.live.weapon};
        ++identities[changed]->registry_index;
        assert(!h.current(h.frame.key)&&!c.armed());++cases;
    }
    {Host h;HandClient c(h.adapter(source_only));h.connect(c);h.ready();h.game.known=false;
        assert(!h.current(h.frame.key)&&!c.armed());++cases;}
    {Host h;HandClient c(h.adapter(source_only));h.connect(c);h.ready();h.invalidate_read=true;
        assert(!h.current(h.frame.key)&&!c.armed());assert(!h.current(h.frame.key));++cases;}
    {Host h;HandClient c(h.adapter(source_only));h.connect(c);h.ready();h.reenter_source=true;
        assert(!h.current(h.frame.key)&&!c.armed());++cases;}
    for(bool stale_stereo:{false,true}){
        Host h;HandClient c(h.adapter(source_only));h.connect(c);
        if(stale_stereo){h.frame.controllers.received_ns=h.frame.now_ns;h.frame.controllers.sync_ns=h.frame.now_ns-1;}
        else h.frame.stereo.received_ns=h.frame.now_ns;
        h.step();h.now=h.frame.now_ns+91000000;const auto reads=h.reads;
        assert(!h.current(h.frame.key)&&!c.armed()&&h.reads==reads);++cases;
    }
    {Host h;HandClient c(h.adapter(source_only));h.connect(c);h.ready();const auto original=h.frame;
        h.advance_read=91000000;assert(!h.current(original.key));
        assert(same_frame(h.frame,original));++cases;}
    return cases;
}
unsigned existing_action_lifetime_cases(){
    unsigned cases{};
    {Host h;HandClient c(h.adapter(true));h.connect(c);ExistingActionLifetime life{};
        {GT gt;assert(!c.capture_existing_action_lifetime_on_gt(h.frame.key,life)&&!life.epoch);}
        h.ready();{GT gt;assert(c.capture_existing_action_lifetime_on_gt(h.frame.key,life));
            assert(c.existing_action_lifetime_on_gt(life));}
        h.game.busy=h.game.animation_owns_hands=true;Frame copy{};
        assert(!h.copy(copy)&&!c.armed()&&same_frame(copy,Frame{}));
        {GT gt;assert(c.existing_action_lifetime_on_gt(life));}
        assert(!h.snapshot()); // Lifetime alone never restores a pose/grip grant.
        h.invalidate();{GT gt;assert(!c.existing_action_lifetime_on_gt(life));}++cases;}
    {Host h;HandClient c(h.adapter(true));h.connect(c);h.ready();ExistingActionLifetime life{};
        {GT gt;assert(c.capture_existing_action_lifetime_on_gt(h.frame.key,life));}
        h.next();h.step();{GT gt;assert(c.existing_action_lifetime_on_gt(life));}
        for(unsigned changed=0;changed<8;++changed){auto wrong=life;
            switch(changed){
            case 0:++wrong.epoch;break;case 1:++wrong.parent.key.registration_cookie;break;
            case 2:++wrong.parent.key.lifecycle;break;case 3:++wrong.parent.key.world_generation;break;
            case 4:++wrong.parent.pawn.registry_index;break;case 5:++wrong.parent.weapon.registry_index;break;
            case 6:++wrong.parent.reference_generation;break;case 7:wrong.parent.key.frame=h.frame.key.frame+1;break;
            }
            GT gt;assert(!c.existing_action_lifetime_on_gt(wrong));++cases;
        }
        ++h.frame.reference_generation;h.next();h.step();
        {GT gt;assert(!c.existing_action_lifetime_on_gt(life));}++cases;}
    {Host h;HandClient c(h.adapter(true));h.connect(c);h.ready();ExistingActionLifetime life{};
        {GT gt;assert(c.capture_existing_action_lifetime_on_gt(h.frame.key,life));}
        h.invalidate();h.next();h.step();
        {GT gt;assert(!c.existing_action_lifetime_on_gt(life));}++cases;}
    {Host h;HandClient c(h.adapter(true));h.connect(c);h.ready();ExistingActionLifetime life{};
        {GT gt;assert(c.capture_existing_action_lifetime_on_gt(h.frame.key,life));}
        assert(!c.existing_action_lifetime_on_gt(life));
        assert(c.begin_disconnect()==Reply::draining);
        {GT gt;assert(!c.existing_action_lifetime_on_gt(life));}++cases;}
    return cases;
}
unsigned source_observer_routing_cases(){
    Host h;HandClient c(h.adapter(true));h.connect(c);h.ready();
    const auto reads=h.reads,source_reads=h.source_reads;
    assert(h.current(h.frame.key)&&h.reads==reads+1&&h.source_reads==source_reads+1);
    Frame copy{};assert(h.copy(copy)&&h.reads==reads+2&&h.source_reads==source_reads+1);
    GripGameState game{};
    assert(h.current_game(h.frame.key,game)&&h.reads==reads+3&&h.source_reads==source_reads+1);
    assert(game.known&&same_game(game,h.game)&&same_frame(copy,h.frame));
    auto wrong=h.frame.key;++wrong.frame;
    assert(!h.current(wrong)&&c.armed()&&h.reads==reads+3&&h.source_reads==source_reads+1);
    return 1;
}
unsigned source_game_cases(){
    unsigned cases{};
    {Host h;HandClient c(h.adapter());h.connect(c);h.game.main_hand_held=false;h.step();
        h.game.main_hand_held=h.game.busy=h.game.animation_owns_hands=true;GripGameState out{};const auto before=h.reads;
        assert(h.current_game(h.frame.key,out)&&h.reads==before+1&&out.known&&out.main_hand_held&&out.busy&&out.animation_owns_hands);
        assert(c.armed());Frame copy{};assert(!h.copy(copy));++cases;}
    for(unsigned failure=0;failure<6;++failure){
        Host h;HandClient c(h.adapter());h.connect(c);h.ready();GripGameState out=h.game;auto key=h.frame.key;
        if(failure==0)++key.frame;
        if(failure==1)h.metadata_known=false;
        if(failure==2)h.invalidate_read=true;
        if(failure==3)h.advance_read=110000000;
        if(failure==4)++h.live.weapon.registry_index;
        if(failure==5)h.game.known=false;
        assert(!h.current_game(key,out)&&!out.known&&!out.main_hand_held&&!out.mount_revision&&!out.mount_asset.address);
        if(failure==0)assert(c.armed());++cases;
    }
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();GripGameState out=h.game;
        assert(!c.source_game_on_gt(h.frame.key,out)&&!out.known&&c.armed());++cases;}
    return cases;
}
}
unsigned retained_input_cases(){
    unsigned cases{};
    for(bool magazine:{false,true}){
        Host h;HandClient c(h.adapter());h.connect(c);h.game.mount_known=false;
        h.step();h.next();h.step();h.game.request_ammo=!magazine;h.game.request_magazine=magazine;
        h.next(true);h.step();Frame f{};GripGameState g{};Decision d{};
        {GT gt;assert(c.copy_frame_on_gt(f,g,d));}
        EquipmentIdentity expected{f.key,f.world,f.controller,f.pawn,f.mesh,f.weapon,f.reference,f.reference_generation};
        RetainedItemInput out{};const auto before_grant=h.reads;
        {GT gt;assert(!c.copy_retained_item_input_on_gt(expected,out));} // Not yet retained.
        assert(h.reads==before_grant&&!out.press_token);
        h.game.object_incumbents[0]=d.lease;h.game.request_ammo=h.game.request_magazine=false;
        h.next(true);h.step();expected.key=h.frame.key;
        const auto reads=h.reads;
        {GT gt;assert(c.copy_retained_item_input_on_gt(expected,out));}
        assert(h.reads==reads&&same(out.parent.key,h.frame.key)&&same(out.lease,d.lease)&&
            out.press_token==h.frame.press_token&&out.original_input_ns==h.frame.controllers.sync_ns&&
            out.decision_ns==h.now);
        // Old association is not renewed, nor passed off as a current Frame.
        // Fresh actual Native input must independently match it at the caller.
        h.now+=500000000;h.metadata_known=false;
        {GT gt;assert(c.copy_retained_item_input_on_gt(expected,out));}
        assert(h.reads==reads&&out.original_input_ns==h.frame.controllers.sync_ns);
        auto wrong=expected;++wrong.weapon.name_number;
        {GT gt;assert(!c.copy_retained_item_input_on_gt(wrong,out));}
        assert(!out.press_token&&c.armed());
        assert(!c.copy_retained_item_input_on_gt(expected,out)&&!out.press_token&&c.armed());
        h.invalidate();{GT gt;assert(!c.copy_retained_item_input_on_gt(expected,out));}
        assert(!out.press_token&&!c.armed());++cases;
    }
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();
        EquipmentIdentity expected{h.frame.key,h.frame.world,h.frame.controller,h.frame.pawn,h.frame.mesh,
            h.frame.weapon,h.frame.reference,h.frame.reference_generation};RetainedItemInput out{};
        {GT gt;assert(!c.copy_retained_item_input_on_gt(expected,out));} // Support is not an item.
        h.next(true);Proposal p{};{GT gt;assert(h.prepare(p)==Reply::accepted);
            assert(!c.copy_retained_item_input_on_gt(expected,out));}
        assert(!out.press_token);++cases;
    }
    return cases;
}
unsigned publication_age_observation_cases(){
    unsigned cases{};
    for(bool support:{false,true})for(auto delay:{0LL,20000000LL,90000000LL,100000001LL}){
        Host h;HandClient c(h.adapter());h.connect(c);
        if(support)h.ready();else {h.step();h.next();assert(h.step()==Grip::single);}
        const auto committed_check=h.now,original_sync=h.frame.controllers.sync_ns;
        const auto expected_age=committed_check-original_sync;
        h.now+=delay;const auto reads=h.reads;
        Frame out{};GripGameState game{};Decision decision{};FrameReadObservation observation{};
        bool copied{};{GT gt;copied=c.copy_frame_on_gt(out,game,decision,&observation);}
        assert(observation.commit_check_input_age_ns==expected_age);
        assert(observation.since_commit_check_ns==delay);
        assert(observation.input_age_ns==expected_age+delay);
        assert(h.frame.controllers.sync_ns==original_sync);
        if(delay<=20000000){assert(copied&&observation.failure==FrameReadFailure::none&&h.reads==reads+1);}
        else {assert(!copied&&observation.failure==FrameReadFailure::stale_before&&h.reads==reads);
            assert(same_frame(out,Frame{}));
            observation={};{GT gt;assert(!c.copy_frame_on_gt(out,game,decision,&observation));}
            assert(observation.failure==FrameReadFailure::thread_or_unarmed&&
                observation.commit_check_input_age_ns==-1&&observation.since_commit_check_ns==-1);
            h.frame.now_ns+=delay;h.frame.stereo.received_ns+=delay;
            h.frame.controllers.received_ns+=delay;h.frame.controllers.sync_ns+=delay;
            h.next();h.step();const auto check=h.now;
            h.now+=2000000;{GT gt;assert(c.copy_frame_on_gt(out,game,decision,&observation));}
            assert(observation.commit_check_input_age_ns==check-h.frame.controllers.sync_ns&&
                observation.since_commit_check_ns==2000000);
        }
        ++cases;
    }
    return cases;
}
unsigned publication_failure_classification_cases(){
    unsigned cases{};
    const auto metadata=[](HandClient& client){
        if(local_thread==55)return client.metadata_generation_on_gt();
        GT gt;return client.metadata_generation_on_gt();
    };
    // Static classification receipt for the failure returned to HB consumers.
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();
        const auto generation=h.snapshot()?metadata(c):0;assert(generation);
        const auto reason=c.last_disarm_reason();h.next();Proposal p{};
        {GT gt;assert(h.prepare(p)==Reply::accepted);Frame out=h.frame;GripGameState game=h.game;Decision decision{};
            FrameReadObservation observation{};assert(!c.copy_frame_on_gt(out,game,decision,&observation));
            assert(observation.failure==FrameReadFailure::thread_or_unarmed);assert(!c.armed());
            assert(same_frame(out,Frame{})&&same_game(game,GripGameState{})&&!decision.proposal_nonce);
            assert(metadata(c)==generation&&c.last_disarm_reason()==reason);
            assert(!c.source_current_on_gt(h.frame.key));}
        {GT gt;assert(h.commit(p)==Reply::accepted);Frame out{};GripGameState game{};Decision decision{};
            assert(c.copy_frame_on_gt(out,game,decision));assert(same_frame(out,h.frame)&&same_game(game,h.game)&&c.armed());}
        ++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();const auto reason=c.last_disarm_reason();
        const auto epoch=metadata(c);assert(epoch);h.metadata_known=false;
        Frame out=h.frame;GripGameState game=h.game;Decision decision{};FrameReadObservation observation{};
        {GT gt;assert(!c.copy_frame_on_gt(out,game,decision,&observation));}
        // An observe failure clears publication without incrementing the epoch.
        assert(observation.failure==FrameReadFailure::observe);assert(!c.armed());
        assert(same_frame(out,Frame{})&&same_game(game,GripGameState{})&&!decision.proposal_nonce);
        assert(metadata(c)==epoch&&c.last_disarm_reason()==reason);
        {GT gt;Frame retry{};GripGameState retry_game{};Decision retry_decision{};
            assert(!c.copy_frame_on_gt(retry,retry_game,retry_decision));}
        {GT gt;assert(!c.source_current_on_gt(h.frame.key));}
        h.metadata_known=true;h.next();h.step();assert(c.armed());++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();
        const auto epoch=metadata(c);assert(epoch);Frame old=h.frame;
        h.invalidate(InvalidateReason::input_lost);
        Frame out=h.frame;GripGameState game=h.game;Decision decision{};FrameReadObservation observation{};
        {GT gt;assert(!c.copy_frame_on_gt(out,game,decision,&observation));}
        assert(observation.failure==FrameReadFailure::thread_or_unarmed);
        assert(!c.armed()&&c.last_disarm_reason()==257u&&metadata(c)!=epoch);
        assert(same_frame(out,Frame{})&&same_game(game,GripGameState{})&&!decision.proposal_nonce);
        {GT gt;assert(!c.source_current_on_gt(old.key));RetainedItemInput retained{};
            EquipmentIdentity identity{old.key,old.world,old.controller,old.pawn,old.mesh,old.weapon,old.reference,old.reference_generation};
            assert(!c.copy_retained_item_input_on_gt(identity,retained)&&!retained.press_token);}
        ++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();Frame out{};FrameReadObservation observation{};
        auto copy_off=[&]{GripGameState game{};Decision decision{};
            assert(!c.copy_frame_on_gt(out,game,decision,&observation));};
        copy_off();assert(observation.failure==FrameReadFailure::thread_or_unarmed&&c.armed());
        const auto epoch=metadata(c);assert(epoch);
        assert(c.begin_disconnect()==Reply::draining);copy_off();
        assert(observation.failure==FrameReadFailure::thread_or_unarmed&&!c.armed());
        assert(metadata(c)==0);assert(c.finish_disconnect(33)==Reply::quiescent);
        copy_off();assert(observation.failure==FrameReadFailure::thread_or_unarmed&&!c.armed());++cases;}
    {Host h;HandClient c(h.adapter());h.client=&c;h.registration=c.registration();Frame out{};GripGameState game{};
        Decision decision{};FrameReadObservation observation{};
        assert(!c.copy_frame_on_gt(out,game,decision,&observation));
        assert(observation.failure==FrameReadFailure::thread_or_unarmed&&c.phase()==ClientPhase::off);
        assert(same_frame(out,Frame{})&&same_game(game,GripGameState{})&&!decision.proposal_nonce);
        assert(metadata(c)==0);++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();Frame out{};GripGameState game{};Decision decision{};
        FrameReadObservation observation{};assert(c.armed());
        assert(!c.copy_frame_on_gt(out,game,decision,&observation)); // Deliberately outside GT.
        assert(observation.failure==FrameReadFailure::thread_or_unarmed&&c.armed());
        assert(same_frame(out,Frame{})&&same_game(game,GripGameState{})&&!decision.proposal_nonce);++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();const auto epoch=metadata(c);assert(epoch);
        h.invalidate(InvalidateReason::native_stopping);Frame out{};GripGameState game{};Decision decision{};
        FrameReadObservation observation{};{GT gt;assert(!c.copy_frame_on_gt(out,game,decision,&observation));}
        assert(observation.failure==FrameReadFailure::thread_or_unarmed&&!c.armed());
        assert(c.last_disarm_reason()==258u&&!c.registration_alive(33,22));
        assert(same_frame(out,Frame{})&&same_game(game,GripGameState{})&&!decision.proposal_nonce);
        h.next();Proposal proposal{};{GT gt;assert(h.prepare(proposal)==Reply::unavailable);}++cases;}
    return cases;
}
int main(){
    { GripGameState before{},after{};assert(same_game(before,after));
      after.support_box.enabled=true;after.support_box.acquire_half={.05,.05,.05};
      after.support_box.retain_half={.1,.1,.1};assert(!same_game(before,after));
      after=before;after.support_region.enabled=true;assert(!same_game(before,after));
      before=after;after.support_region.volume.local.position.x=.01;assert(!same_game(before,after));
      after=before;after.support_region.scale=2;assert(!same_game(before,after));
    }

    const auto publication_age_cases=publication_age_observation_cases();
    const auto publication_failure_cases=publication_failure_classification_cases();
    assert(publication_failure_cases==7);
    std::printf("Publication age observation: %u cases; original clocks and admission unchanged\n",publication_age_cases);
    std::printf("Publication failure classification: %u cases; empty output and ThreadOrUnarmed are not a gesture/publication grant\n",publication_failure_cases);
    unsigned cases{};
    {Host h;HandClient c(h.adapter());h.client=&c;h.registration=c.registration();
        assert(c.begin_connect({11,44})==Reply::accepted);
        {GT gt;Proposal p{};assert(h.prepare(p)==Reply::unavailable);assert(!c.armed());}
        RegistrationResult r{};r.cookie=33;r.native_started_qpc=22;assert(c.connected(Reply::accepted,r)==Reply::accepted);
        h.ready();h.next(true);assert(h.step()==Grip::two);h.next(false);assert(h.step()==Grip::single);++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();h.next(true);Proposal p{};
        {GT gt;assert(h.prepare(p)==Reply::accepted);}h.invalidate();
        {GT gt;assert(h.commit(p)==Reply::unavailable);}assert(!c.armed()&&!h.snapshot());++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();h.invalidate(InvalidateReason::context_lost,32);
        assert(c.armed()&&h.snapshot());h.invalidate(InvalidateReason::input_lost);assert(!c.armed()&&!h.snapshot());++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();h.live.weapon=Host::object(0x5010);
        assert(!h.snapshot()&&!c.armed());++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();h.metadata_known=false;
        assert(!h.snapshot());h.metadata_known=true;assert(!h.snapshot());++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();h.now+=100000001;assert(!h.snapshot());++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();h.game.kind=WeaponKind::hand_thrown;
        h.next(false);assert(h.step()==Grip::excluded);++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();h.next(true);h.reenter=true;Proposal p{};
        {GT gt;assert(h.prepare(p)==Reply::unavailable);}assert(!c.armed());++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();h.next(true);Proposal p{};
        {GT gt;assert(h.prepare(p)==Reply::accepted);h.invalidate_read=true;assert(h.commit(p)==Reply::unavailable);}
        assert(!c.armed());++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();h.next(true);h.block=true;Reply result=Reply::invalid;
        std::thread worker([&]{GT gt;Proposal p{};result=h.prepare(p);});
        {std::unique_lock<std::mutex> lock(h.mutex);h.changed.wait(lock,[&]{return h.entered;});}
        assert(c.begin_disconnect()==Reply::draining);
        // The host must itself drain its transaction. Even with a prematurely
        // supplied receipt, local state cannot reset while this Gate is held.
        assert(c.finish_disconnect(33)==Reply::draining);
        {std::lock_guard<std::mutex> lock(h.mutex);h.release=true;}h.changed.notify_all();worker.join();
        assert(result==Reply::unavailable);assert(c.finish_disconnect(33)==Reply::quiescent);
        assert(c.phase()==ClientPhase::off&&!h.snapshot());++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();
        {GT gt;assert(c.begin_disconnect()==Reply::reentrant);assert(c.finish_disconnect(33)==Reply::reentrant);}
        assert(c.begin_disconnect()==Reply::draining);assert(c.finish_disconnect(32)==Reply::wrong_cookie);
        assert(c.finish_disconnect(33)==Reply::quiescent);h.connect(c,34);h.next(false);h.step();
        h.next(false);assert(h.step()==Grip::single);h.invalidate(InvalidateReason::native_stopping,33);
        assert(c.armed());assert(c.begin_disconnect()==Reply::draining);assert(c.finish_disconnect(34)==Reply::quiescent);++cases;}
    {Host h;HandClient c(h.adapter());assert(c.begin_connect({11,44})==Reply::accepted);
        RegistrationResult r{};assert(c.connected(Reply::unavailable,r)==Reply::unavailable);assert(c.phase()==ClientPhase::off);
        assert(c.begin_connect({11,44})==Reply::accepted);assert(c.connected(Reply::accepted,r)==Reply::invalid);
        assert(c.phase()==ClientPhase::quarantined&&c.begin_disconnect()==Reply::unavailable);++cases;}
    {Host h;HandClient c(h.adapter());assert(c.begin_connect({11,44})==Reply::accepted);RegistrationResult r{};
        assert(c.connected(Reply::invalid,r,false)==Reply::invalid);assert(c.phase()==ClientPhase::quarantined);++cases;}
    {Host h;HandClient c(h.adapter());h.client=&c;h.registration=c.registration();assert(c.begin_connect({11,44})==Reply::accepted);
        h.invalidate(InvalidateReason::native_stopping);RegistrationResult r{};r.cookie=33;r.native_started_qpc=22;
        assert(c.connected(Reply::accepted,r)==Reply::draining);assert(!c.armed());
        assert(c.finish_disconnect(33)==Reply::quiescent);++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();assert(c.registration_alive(33,22));
        h.invalidate(InvalidateReason::input_lost);assert(!c.armed()&&c.registration_alive(33,22));
        h.invalidate(InvalidateReason::native_stopping);assert(c.phase()==ClientPhase::active&&!c.registration_alive(33,22));
        assert(c.begin_disconnect()==Reply::draining&&!c.registration_alive(33,22));
        assert(c.finish_disconnect(33)==Reply::quiescent);++cases;}
    const auto candidate_failure_cases=candidate_failure_observation_cases();
    const auto pending_marker_cases=single_hand_pending_marker_cases();
    const auto frame_cases=frame_copy_cases();
    const auto source_cases=source_current_cases();
    const auto source_only_cases=source_current_cases(true);
    const auto routing_cases=source_observer_routing_cases();
    const auto action_lifetime_cases=existing_action_lifetime_cases();
    const auto game_cases=source_game_cases();
    const auto motion_cases=retained_input_cases();
    std::printf("WeaponHandClientTest passed: %u existing + %u Frame-copy + %u source-current cases; local host/adapter only, no equipment/write authority\n",cases,frame_cases,source_cases);
    std::printf("Single-hand pending marker: %u cases; no left grant, fresh repress and strict commit required\n",pending_marker_cases);
    std::printf("Source-only observer: %u identical liveness rejection cases; no contact proposals\n",source_only_cases);
    std::printf("Existing original-action lifetime: %u cases; explicit epoch invalidation, ancestry and no new pose authority\n",action_lifetime_cases);
    std::printf("Source observer routing: %u case; full Frame/game reads retain candidate observation\n",routing_cases);
    std::printf("Source game-state output: %u cases; no publication or grant\n",game_cases);
    std::printf("Retained motion ancestry: %u cases; original clocks, no fresh Frame/game/write authority\n",motion_cases);
    std::printf("Expired empty-proposal receipt: %u cases; known contact/pending versus known miss, no renewed permission\n",candidate_failure_cases);
}
