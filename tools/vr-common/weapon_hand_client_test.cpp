#include "weapon_hand_client.h"
#include "weapon_hand_owner_adapter.h"
#include <cassert>
#include <condition_variable>
#include <cstdio>
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
    std::int64_t now=1001000000;bool metadata_known=true,reenter{},invalidate_read{};
    unsigned reads{};HandClient* client{};Registration registration{};
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
        if(h.invalidate_read){h.invalidate_read=false;h.invalidate();}
        if(!h.metadata_known||!same(f.world,h.live.world)||!same(f.controller,h.live.controller)||
           !same(f.pawn,h.live.pawn)||!same(f.mesh,h.live.mesh)||!same(f.weapon,h.live.weapon))return false;
        out=h.game;return true;
    }
    static std::uint32_t thread(void*) noexcept {return local_thread==55?55u:0u;}
    static std::int64_t clock(void* p) noexcept {return static_cast<Host*>(p)->now;}
    ClientAdapter adapter() noexcept {return {this,observe,thread,clock};}
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
};
}
int main(){
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
    std::printf("WeaponHandClientTest passed: %u cases; local host/adapter only\n",cases);
}
