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
        if(h.reenter_frame){h.reenter_frame=false;Frame copy=h.frame;
            assert(!h.client->copy_frame_on_gt(copy)&&same_frame(copy,Frame{}));}
        if(h.reenter_source){h.reenter_source=false;assert(!h.client->source_current_on_gt(f.key));}
        if(h.invalidate_read){h.invalidate_read=false;h.invalidate();}
        if(!h.metadata_known||!same(f.world,h.live.world)||!same(f.controller,h.live.controller)||
           !same(f.pawn,h.live.pawn)||!same(f.mesh,h.live.mesh)||!same(f.weapon,h.live.weapon))return false;
        out=h.game;if(h.advance_read){h.now+=h.advance_read;h.advance_read=0;}return true;
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
    bool copy(Frame& out){
        GT gt;GripGameState observed=game;const bool copied=client->copy_frame_on_gt(out,observed);
        assert(same_game(observed,copied?game:GripGameState{}));return copied;
    }
    bool current(FrameKey const& expected){GT gt;return client->source_current_on_gt(expected);}
    bool current_game(FrameKey const& expected,GripGameState& out){GT gt;return client->source_game_on_gt(expected,out);}
};
unsigned frame_copy_cases(){
    unsigned cases{};
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
unsigned source_current_cases(){
    unsigned cases{};
    {Host h;HandClient c(h.adapter());h.connect(c);h.game.main_hand_held=false;
        assert(h.step()==Grip::unknown);const auto key=h.frame.key;
        h.game.main_hand_held=h.game.busy=h.game.animation_owns_hands=true;
        const auto reads=h.reads;assert(h.current(key)&&h.reads==reads+1&&c.armed());
        assert(h.current(key));Frame out=h.frame;
        // Source validation must not rewrite the originally committed game.
        assert(!h.copy(out)&&same_frame(out,Frame{}));++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();h.game.main_hand_held=false;h.game.busy=true;
        assert(h.current(h.frame.key)&&c.armed());
        // It must not refresh/authorize an existing frozen two-hand snapshot.
        assert(!h.snapshot()&&!c.armed());++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();const auto old=h.frame.key;
        h.next();h.step();assert(!h.current(old)&&c.armed());Frame out{};
        assert(h.current(h.frame.key)&&h.copy(out)&&same_frame(out,h.frame));++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();const auto key=h.frame.key;
        for(unsigned field=0;field<8;++field){auto wrong=key;
            switch(field){case 0:++wrong.game_start;break;case 1:++wrong.native_started_qpc;break;case 2:++wrong.registration_cookie;break;
                case 3:++wrong.lifecycle;break;case 4:++wrong.frame;break;case 5:++wrong.world_generation;break;
                case 6:++wrong.game_pid;break;case 7:++wrong.thread_id;break;}
            const auto reads=h.reads;assert(!h.current(wrong)&&c.armed()&&h.reads==reads);
            Frame out{};assert(h.copy(out)&&same_frame(out,h.frame));}
        assert(h.current(key));++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();
        assert(!c.source_current_on_gt(h.frame.key)&&c.armed());Frame out{};
        assert(h.current(h.frame.key)&&h.copy(out));++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);assert(!h.current(h.frame.key));
        h.step();h.next();Proposal proposal{};{GT gt;assert(h.prepare(proposal)==Reply::accepted);}
        assert(!h.current(h.frame.key));{GT gt;assert(h.commit(proposal)==Reply::accepted);}
        assert(h.current(h.frame.key));++cases;}
    for(unsigned changed=0;changed<5;++changed){
        Host h;HandClient c(h.adapter());h.connect(c);h.ready();
        ObjectIdentity* identities[]={&h.live.world,&h.live.controller,&h.live.pawn,&h.live.mesh,&h.live.weapon};
        ++identities[changed]->registry_index;
        assert(!h.current(h.frame.key)&&!c.armed());++cases;
    }
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();h.game.known=false;
        assert(!h.current(h.frame.key)&&!c.armed());++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();h.invalidate_read=true;
        assert(!h.current(h.frame.key)&&!c.armed());assert(!h.current(h.frame.key));++cases;}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();h.reenter_source=true;
        assert(!h.current(h.frame.key)&&!c.armed());++cases;}
    for(bool stale_stereo:{false,true}){
        Host h;HandClient c(h.adapter());h.connect(c);
        if(stale_stereo){h.frame.controllers.received_ns=h.frame.now_ns;h.frame.controllers.sync_ns=h.frame.now_ns-1;}
        else h.frame.stereo.received_ns=h.frame.now_ns;
        h.step();h.now=h.frame.now_ns+91000000;const auto reads=h.reads;
        assert(!h.current(h.frame.key)&&!c.armed()&&h.reads==reads);++cases;
    }
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();const auto original=h.frame;
        h.advance_read=91000000;assert(!h.current(original.key));
        assert(same_frame(h.frame,original));++cases;}
    return cases;
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
    const auto frame_cases=frame_copy_cases();
    const auto source_cases=source_current_cases();
    const auto game_cases=source_game_cases();
    std::printf("WeaponHandClientTest passed: %u existing + %u Frame-copy + %u source-current cases; local host/adapter only, no equipment/write authority\n",cases,frame_cases,source_cases);
    std::printf("Source game-state output: %u cases; no publication or grant\n",game_cases);
}
