#pragma once
#include "weapon_support_grip.h"
#include <atomic>
#include <cstring>

namespace gears_weapon_hand {
// The adapter runs on the certified game thread. It must freshly validate the
// Frame's objects and held equipment without calling the engine, allocating,
// waiting, or entering Native. A cached pose/visible IK lease is insufficient.
struct ClientAdapter {
    void* context{};
    bool (*observe)(void*,Frame const&,std::int64_t,GripGameState&) noexcept{};
    std::uint32_t (*game_thread)(void*) noexcept{};
    std::int64_t (*clock_ns)(void*) noexcept{};
};
enum class ClientPhase:std::uint32_t {off,connecting,active,draining,quarantined};
struct ClientSession {std::uint64_t game_start{};std::uint32_t game_pid{};};
inline bool same_game(GripGameState const& a,GripGameState const& b) noexcept {
    auto const& x=a.mount;auto const& y=b.mount;
    const auto vector=[](gears_vr::Vector p,gears_vr::Vector q){return p.x==q.x&&p.y==q.y&&p.z==q.z;};
    return a.known==b.known&&a.main_hand_held==b.main_hand_held&&a.animation_owns_hands==b.animation_owns_hands&&
        a.busy==b.busy&&a.mount_known==b.mount_known&&a.kind==b.kind&&same(a.mount_asset,b.mount_asset)&&
        a.mount_revision==b.mount_revision&&gears_vr::stereo_same_pose(x.right_grip_in_weapon,y.right_grip_in_weapon)&&
        vector(x.support_grip_in_weapon,y.support_grip_in_weapon)&&vector(x.muzzle_in_weapon,y.muzzle_in_weapon)&&
        vector(x.bore_forward_in_weapon,y.bore_forward_in_weapon)&&
        x.support_length_tolerance_metres==y.support_length_tolerance_metres&&
        x.maximum_support_correction_radians==y.maximum_support_correction_radians&&
        a.acquire_metres==b.acquire_metres&&a.release_metres==b.release_metres&&
        same(a.object_incumbents[0],b.object_incumbents[0])&&same(a.object_incumbents[1],b.object_incumbents[1])&&
        a.request_ammo==b.request_ammo&&a.request_magazine==b.request_magazine;
}

// Weapon-side callback storage, not a host/arbiter or a loader. Control calls
// are externally serialized on a non-GT thread; callback state is GT-only.
// Invalidations touch atomics only. No destructor assumes that Native drained.
class HandClient {
    struct Gate {
        std::atomic_flag& flag;bool held;
        explicit Gate(std::atomic_flag& f) noexcept:flag(f),held(!f.test_and_set(std::memory_order_acquire)){}
        ~Gate(){if(held)flag.clear(std::memory_order_release);}
    };
    ClientAdapter adapter_{};ClientSession session_{};
    std::atomic<ClientPhase> phase_{ClientPhase::off};
    std::atomic<std::uint64_t> cookie_{},native_{},epoch_{1},published_{};
    std::atomic<bool> terminal_{};
    std::atomic_flag gate_=ATOMIC_FLAG_INIT;
    SupportGripParticipant participant_;
    Frame committed_{};GripGameState prepared_game_{},committed_game_{};
    std::uint64_t seen_epoch_{},prepared_epoch_{};bool pending_{};
    bool adapter_valid() const noexcept {return adapter_.observe&&adapter_.game_thread&&adapter_.clock_ns;}
    void disarm() noexcept {
        published_.store(0,std::memory_order_release);
        auto value=epoch_.load(std::memory_order_acquire);
        for(;;){
            if(value==(std::numeric_limits<std::uint64_t>::max)()){
                terminal_.store(true,std::memory_order_release);return;
            }
            if(epoch_.compare_exchange_weak(value,value+1,std::memory_order_acq_rel))return;
        }
    }
    void clear_gt() noexcept {
        participant_.invalidate_on_gt();committed_={};prepared_game_={};committed_game_={};
        pending_=false;prepared_epoch_=0;published_.store(0,std::memory_order_release);
    }
    Reply fail_gt(Reply reason) noexcept {clear_gt();return reason;}
    bool active(std::uint64_t epoch) const noexcept {
        return phase_.load(std::memory_order_acquire)==ClientPhase::active&&!terminal_.load(std::memory_order_acquire)&&
            epoch_.load(std::memory_order_acquire)==epoch;
    }
    bool on_gt() const noexcept {return adapter_valid()&&adapter_.game_thread(adapter_.context)!=0;}
    bool key_valid(Frame const& f) const noexcept {
        return f.key.game_pid==session_.game_pid&&f.key.game_start==session_.game_start&&
            f.key.registration_cookie==cookie_.load(std::memory_order_acquire)&&
            f.key.native_started_qpc==native_.load(std::memory_order_acquire)&&
            f.key.thread_id==adapter_.game_thread(adapter_.context);
    }
    bool fresh(Frame const& f,std::int64_t now) const noexcept {
        return now>=f.now_ns&&now-f.now_ns<=100000000&&frame_active(f,now);
    }
    Reply prepare(Frame const* frame,Proposal* out) noexcept {
        if(!frame||!out)return Reply::invalid;
        *out={};
        if(phase_.load(std::memory_order_acquire)!=ClientPhase::active||!on_gt())return Reply::unavailable;
        Gate gate(gate_);if(!gate.held){disarm();return Reply::busy;}
        const auto epoch=epoch_.load(std::memory_order_acquire);
        if(seen_epoch_!=epoch){clear_gt();seen_epoch_=epoch;}
        if(!active(epoch)||!key_valid(*frame))return fail_gt(Reply::unavailable);
        auto now=adapter_.clock_ns(adapter_.context);GripGameState game{};
        if(!fresh(*frame,now)||!adapter_.observe(adapter_.context,*frame,now,game)||!game.known)
            return fail_gt(Reply::unavailable);
        now=adapter_.clock_ns(adapter_.context);
        if(!active(epoch)||!fresh(*frame,now))return fail_gt(Reply::unavailable);
        const auto reply=participant_.prepare(*frame,game,*out);
        if(reply!=Reply::accepted)return fail_gt(reply);
        if(!active(epoch)){*out={};return fail_gt(Reply::unavailable);}
        prepared_game_=game;prepared_epoch_=epoch;pending_=true;
        // A previous frame must not remain consumable between prepare/commit.
        published_.store(0,std::memory_order_release);return Reply::accepted;
    }
    Reply commit(Frame const* frame,Proposal const* proposal,Decision const* decision) noexcept {
        if(!frame||!proposal||!decision)return Reply::invalid;
        if(phase_.load(std::memory_order_acquire)!=ClientPhase::active||!on_gt())return Reply::unavailable;
        Gate gate(gate_);if(!gate.held){disarm();return Reply::busy;}
        const auto epoch=epoch_.load(std::memory_order_acquire);
        if(!pending_||prepared_epoch_!=epoch||!active(epoch)||!key_valid(*frame))return fail_gt(Reply::unavailable);
        pending_=false;auto now=adapter_.clock_ns(adapter_.context);GripGameState game{};
        if(!fresh(*frame,now)||decision->now_ns>now||
           !adapter_.observe(adapter_.context,*frame,now,game)||!same_game(game,prepared_game_))
            return fail_gt(Reply::unavailable);
        now=adapter_.clock_ns(adapter_.context);
        if(!active(epoch)||!fresh(*frame,now))return fail_gt(Reply::unavailable);
        const auto reply=participant_.commit(*frame,*proposal,*decision);
        if(reply!=Reply::accepted)return fail_gt(reply);
        committed_=*frame;committed_game_=game;published_.store(epoch,std::memory_order_release);
        if(!active(epoch))return fail_gt(Reply::unavailable);
        return Reply::accepted;
    }
    void invalidate(Invalidation const* message) noexcept {
        if(!message||message->size!=sizeof(Invalidation)||message->abi_version!=version||message->reserved||
           unsigned(message->reason)>unsigned(InvalidateReason::callback_fault))return;
        const auto phase=phase_.load(std::memory_order_acquire);
        if(phase==ClientPhase::off)return;
        if(phase!=ClientPhase::connecting&&
           (message->registration_cookie!=cookie_.load(std::memory_order_acquire)||
            message->native_started_qpc!=native_.load(std::memory_order_acquire)))return;
        if(message->reason==InvalidateReason::native_stopping||message->reason==InvalidateReason::unregistering||
           message->reason==InvalidateReason::callback_fault)terminal_.store(true,std::memory_order_release);
        disarm();
    }
    static std::uint32_t prepare_callback(void* p,Frame const* f,Proposal* out) noexcept {
        return p?unsigned(static_cast<HandClient*>(p)->prepare(f,out)):unsigned(Reply::invalid);
    }
    static std::uint32_t commit_callback(void* p,Frame const* f,Proposal const* proposal,Decision const* d) noexcept {
        return p?unsigned(static_cast<HandClient*>(p)->commit(f,proposal,d)):unsigned(Reply::invalid);
    }
    static void invalidate_callback(void* p,Invalidation const* m) noexcept {
        if(p)static_cast<HandClient*>(p)->invalidate(m);
    }
public:
    explicit HandClient(ClientAdapter adapter) noexcept:adapter_(adapter){}
    HandClient(HandClient const&)=delete;HandClient& operator=(HandClient const&)=delete;
    ClientPhase phase() const noexcept {return phase_.load(std::memory_order_acquire);}
    std::uint64_t cookie() const noexcept {return cookie_.load(std::memory_order_acquire);}
    std::uint64_t native_started_qpc() const noexcept {return native_.load(std::memory_order_acquire);}
    // An already-issued shot retains its frozen grip after soft input loss.
    // A terminal registration may never authorize a new packet/write. This is
    // an admission check; final stop still drains enclosing PG callbacks.
    bool registration_alive(std::uint64_t cookie,std::uint64_t native) const noexcept {
        return cookie&&native&&phase()==ClientPhase::active&&!terminal_.load(std::memory_order_acquire)&&
            cookie_.load(std::memory_order_acquire)==cookie&&native_.load(std::memory_order_acquire)==native;
    }
    bool armed() const noexcept {
        const auto p=published_.load(std::memory_order_acquire);return p&&active(p);
    }
    Registration registration() noexcept {
        Registration r{};r.context=this;r.prepare=prepare_callback;r.commit=commit_callback;r.invalidate=invalidate_callback;return r;
    }
    // Call before invoking Native register; callbacks can arrive before that
    // export returns, but cannot publish until its receipt has been validated.
    Reply begin_connect(ClientSession session) noexcept {
        if(on_gt())return Reply::reentrant;
        if(phase()!=ClientPhase::off)return Reply::busy;
        Gate gate(gate_);if(!gate.held)return Reply::busy;
        if(!adapter_valid()||!session.game_pid||!session.game_start)return Reply::invalid;
        clear_gt();participant_=SupportGripParticipant{};session_=session;seen_epoch_=0;
        terminal_.store(false,std::memory_order_release);
        cookie_.store(0,std::memory_order_release);native_.store(0,std::memory_order_release);
        phase_.store(ClientPhase::connecting,std::memory_order_release);return Reply::accepted;
    }
    // A fault/invalid accepted receipt leaves storage quarantined: no guessed
    // cookie and no unload. The external loader retains Native and this module.
    Reply connected(Reply reply,RegistrationResult const& receipt,bool call_completed=true) noexcept {
        if(phase()!=ClientPhase::connecting)return Reply::invalid;
        if(call_completed&&reply!=Reply::accepted&&!receipt.cookie&&
           (reply==Reply::unavailable||reply==Reply::busy||reply==Reply::invalid)){
            phase_.store(ClientPhase::off,std::memory_order_release);return reply;
        }
        if(!call_completed||reply!=Reply::accepted||receipt.size!=sizeof(receipt)||receipt.abi_version!=version||
           !receipt.cookie||!receipt.native_started_qpc){
            phase_.store(ClientPhase::quarantined,std::memory_order_release);disarm();return Reply::invalid;
        }
        cookie_.store(receipt.cookie,std::memory_order_release);native_.store(receipt.native_started_qpc,std::memory_order_release);
        phase_.store(terminal_.load(std::memory_order_acquire)?ClientPhase::draining:ClientPhase::active,std::memory_order_release);
        return phase()==ClientPhase::active?Reply::accepted:Reply::draining;
    }
    Reply begin_disconnect() noexcept {
        if(on_gt())return Reply::reentrant;
        if(phase()==ClientPhase::off)return Reply::quiescent;
        if(phase()==ClientPhase::connecting||phase()==ClientPhase::quarantined)return Reply::unavailable;
        phase_.store(ClientPhase::draining,std::memory_order_release);disarm();return Reply::draining;
    }
    // Native's exact-cookie quiescent receipt is a PRECONDITION. This resets
    // private state; it is NOT permission to destroy storage or unload code.
    // All local readers must be inside the enclosing PG hook callback lease.
    // Keep this object alive until hook admission is stopped AND that separate
    // callback drain is proven, including readers not yet holding gate_.
    Reply finish_disconnect(std::uint64_t quiescent_cookie) noexcept {
        if(on_gt())return Reply::reentrant;
        if(phase()!=ClientPhase::draining||!quiescent_cookie||quiescent_cookie!=cookie())return Reply::wrong_cookie;
        Gate gate(gate_);if(!gate.held)return Reply::draining;
        clear_gt();participant_=SupportGripParticipant{};cookie_.store(0);native_.store(0);
        phase_.store(ClientPhase::off,std::memory_order_release);return Reply::quiescent;
    }
    // Requires the enclosing PG hook callback lifetime lease even on an early
    // return, just like snapshot_on_gt. Copy only the current committed Native
    // data after a fresh GT identity/game observation. No held/support lease is
    // required: first-grab, busy and animation-owned states are observable.
    // This does not grant equipment calls, ownership, or pose-write permission.
    bool copy_frame_on_gt(Frame& out) noexcept {
        out={};if(!on_gt()||!armed())return false;
        Gate gate(gate_);if(!gate.held){disarm();return false;}
        const auto epoch=epoch_.load(std::memory_order_acquire);
        if(!active(epoch)||published_.load(std::memory_order_acquire)!=epoch)return false;
        auto now=adapter_.clock_ns(adapter_.context);GripGameState current{};
        if(!key_valid(committed_)||!fresh(committed_,now)||
           !adapter_.observe(adapter_.context,committed_,now,current)||!same_game(current,committed_game_)){
            clear_gt();return false;
        }
        now=adapter_.clock_ns(adapter_.context);
        if(!active(epoch)||published_.load(std::memory_order_acquire)!=epoch||!fresh(committed_,now))return false;
        std::memcpy(&out,&committed_,sizeof(out));return true;
    }
    // Same callback lifetime lease requirement. Revalidate the exact existing
    // publication after a caller changes its software equipment model. Fresh
    // observe must still certify the committed actors, but held/busy state may
    // differ. Do not change committed_game_, refresh a grip or return data.
    // A stale caller key cannot revoke a different healthy current publication.
    // This source-current witness is not equipment or pose-write permission.
    bool source_current_on_gt(FrameKey const& expected) noexcept {
        if(!on_gt()||!armed())return false;
        Gate gate(gate_);if(!gate.held){disarm();return false;}
        const auto epoch=epoch_.load(std::memory_order_acquire);
        if(!active(epoch)||published_.load(std::memory_order_acquire)!=epoch||!same(committed_.key,expected))return false;
        auto now=adapter_.clock_ns(adapter_.context);GripGameState current{};
        if(!key_valid(committed_)||!fresh(committed_,now)||
           !adapter_.observe(adapter_.context,committed_,now,current)||!current.known){clear_gt();return false;}
        now=adapter_.clock_ns(adapter_.context);
        return active(epoch)&&published_.load(std::memory_order_acquire)==epoch&&fresh(committed_,now);
    }
    // Requires the enclosing PG hook callback lifetime lease even on an early
    // return. Freeze this at the outer actual fire invocation. Do not reread it
    // independently for each pellet/target, and never use it as a throw scale.
    bool snapshot_on_gt(GripSnapshot& out) noexcept {
        out={};if(!on_gt()||!armed())return false;
        Gate gate(gate_);if(!gate.held){disarm();return false;}
        const auto epoch=epoch_.load(std::memory_order_acquire);
        if(!active(epoch)||published_.load(std::memory_order_acquire)!=epoch)return false;
        auto now=adapter_.clock_ns(adapter_.context);GripGameState current{};
        if(!key_valid(committed_)||!fresh(committed_,now)||
           !adapter_.observe(adapter_.context,committed_,now,current)||!same_game(current,committed_game_)){
            clear_gt();return false;
        }
        now=adapter_.clock_ns(adapter_.context);auto snapshot=participant_.snapshot();
        if(!active(epoch)||published_.load(std::memory_order_acquire)!=epoch||!fresh(committed_,now)||
           !snapshot.current(committed_.key,committed_.weapon,now))return false;
        out=snapshot;return true;
    }
};
} // namespace gears_weapon_hand
