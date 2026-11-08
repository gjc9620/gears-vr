#pragma once
#include "weapon_support_grip.h"
#include "weapon_equipment_observation.h"
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
    // Optional synchronous receipt of an accepted decision. This runs under
    // the client gate: no reentry, engine calls, pose or inventory writes.
    // Any staged incumbent must apply only to a LATER owner frame, preserving
    // the exact prepared/committed observation of this frame.
    void (*committed)(void*,Frame const&,Decision const&) noexcept{};
    // Optional source-liveness observation with the SAME fresh actor/link/
    // lifecycle checks as observe, but without calculating new contact
    // candidates. Display revalidation consumes no proposal or hand grant.
    // Omitted adapters retain the original full-observation behavior.
    bool (*observe_source)(void*,Frame const&,std::int64_t,GripGameState&) noexcept{};
};
enum class ClientPhase:std::uint32_t {off,connecting,active,draining,quarantined};
struct ClientSession {std::uint64_t game_start{};std::uint32_t game_pid{};};
// Optional caller-owned failure observation. Never a Frame, source, holder or
// permission. It classifies the SAME existing checks without another read.
enum class FrameReadFailure:std::uint32_t {none,thread_or_unarmed,gate,publication,key,
    stale_before,observe,game_changed,stale_or_changed_after,support_prepare,implicit_clear};
struct CandidateObservation {
    // Diagnostic bits:1 association known,2 support contact,4 mount known,
    //8 mount valid,16 magazine pending,32 magazine request,64 ammo request.
    std::uint32_t flags{},decision{},lease{};
    double support_distance_metres{-1};
};
inline CandidateObservation candidate_observation(Frame const& f,GripGameState const& game,Decision const& d) noexcept {
    CandidateObservation result{};
    if(!f.key.frame||!same(f.key,d.key))return result;
    const bool contact=support_contact(f,game,false,&result.support_distance_metres);
    result.flags=1u|(unsigned(contact)<<1)|(unsigned(game.mount_known)<<2)|(unsigned(game.mount.valid())<<3)|
        (unsigned(game.magazine_observation_pending)<<4)|(unsigned(game.request_magazine)<<5)|(unsigned(game.request_ammo)<<6);
    result.decision=unsigned(d.status);result.lease=unsigned(d.lease.kind);return result;
}
struct FrameReadObservation {
    FrameReadFailure failure{};std::uint32_t game_difference{};
    std::uint64_t frame{},sequence{},press_token{};
    std::int64_t input_age_ns{-1};std::uint32_t left_grip_down{};
    std::uint32_t frame_difference{},game_flags{};
    CandidateObservation candidate{}; // Existing failure receipt only; never a permit.
};
// Lifetime of an already journalled ORIGINAL action through a normal pose/
// animation handoff. This is neither a pose/input Frame nor an action grant.
struct ExistingActionLifetime {EquipmentIdentity parent{};std::uint64_t epoch{};};
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
        a.request_ammo==b.request_ammo&&a.request_magazine==b.request_magazine&&
        a.magazine_observation_pending==b.magazine_observation_pending;
}
inline bool same_support_observation(Frame const& f,Decision const& d,
    GripGameState const& current,GripGameState const& committed) noexcept {
    if(same_game(current,committed))return true;
    // Hover proposals belong to acquisition. A freshly revalidated, already
    // exclusive support grasp must not disappear because another part enters
    // a generous hover volume during an original shot. Do not reuse this for
    // prepare/commit, part grants, conflicting incumbents or changed game,
    // mount, animation, identity, input or tracking state.
    if(d.lease.kind!=LeaseKind::support||d.release_required||
       (d.status!=DecisionStatus::granted&&d.status!=DecisionStatus::retained)||
       !same(f.key,d.key)||!f.press_token||d.lease.id!=f.press_token||d.press_token!=f.press_token||
       !lease_valid(d.lease,f.key.lifecycle)||!f.left_grip_known||!f.left_tracked||!f.left_grip_down||
       !current.known||!current.main_hand_held||current.animation_owns_hands||current.busy||
       current.kind!=WeaponKind::firearm||!current.mount_known)return false;
    auto compared=current;
    compared.request_ammo=committed.request_ammo;compared.request_magazine=committed.request_magazine;
    compared.magazine_observation_pending=committed.magazine_observation_pending;
    return same_game(compared,committed);
}
inline bool same_pose_consumer_observation(Frame const& f,Decision const& d,
    GripGameState const& current,GripGameState const& committed) noexcept {
    if(same_support_observation(f,d,current,committed))return true;
    //67624/3088: an installed-mag pending marker changed after a no-lease
    // publication or while the same C part was ALREADY held. It is not the
    // main hand's or that incumbent's liveness. Keep the actual observation
    // and frozen decision; this cannot grant a part or renew a press.
    // Prepare/commit remains strict. A part must have the exact SAME sole
    // certified incumbent, not a first grant or an omitted/conflicting lease.
    if(!same(f.key,d.key)||!d.proposal_nonce||!f.press_token||d.press_token!=f.press_token||
       d.new_grant||!f.left_grip_known||!f.left_tracked||!f.left_grip_down||
       !current.known||!current.main_hand_held||current.animation_owns_hands||current.busy||
       current.kind!=WeaponKind::firearm||!current.mount_known)return false;
    unsigned holders{};
    for(unsigned i=0;i<2;++i)if(!same(current.object_incumbents[i],Lease{})){
        if(!same(current.object_incumbents[i],d.lease)||++holders>1)return false;
    }
    if(same(d.lease,Lease{})){
        if(holders)return false;
    }else if(d.lease.kind!=LeaseKind::magazine||d.status!=DecisionStatus::retained||
             d.release_required||!lease_valid(d.lease,f.key.lifecycle)||d.lease.id!=f.press_token||holders!=1)return false;
    auto compared=current;
    // 69680 difference18: once the exact exclusive magazine-route part is
    // ALREADY retained, entering/leaving its generous acquisition volume is
    // not loss of that item's liveness. Keep this out of the no-lease branch
    // and the first-grant/prepare/commit paths; returned data remains actual.
    if(holders==1)compared.request_magazine=committed.request_magazine;
    compared.magazine_observation_pending=committed.magazine_observation_pending;
    return same_game(compared,committed);
}
inline unsigned game_difference(GripGameState const& current,GripGameState const& expected) noexcept {
    auto compared=current;
    unsigned difference=unsigned(current.request_ammo!=expected.request_ammo)|
        (unsigned(current.request_magazine!=expected.request_magazine)<<1)|
        (unsigned(current.magazine_observation_pending!=expected.magazine_observation_pending)<<4)|
        (unsigned(!same(current.object_incumbents[0],expected.object_incumbents[0])||
            !same(current.object_incumbents[1],expected.object_incumbents[1]))<<2);
    compared.request_ammo=expected.request_ammo;compared.request_magazine=expected.request_magazine;
    compared.magazine_observation_pending=expected.magazine_observation_pending;
    compared.object_incumbents=expected.object_incumbents;
    if(!same_game(compared,expected))difference|=8;
    return difference;
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
    Decision committed_decision_{};
    FrameReadObservation last_withdrawal_{};
    std::atomic<std::uint32_t> last_disarm_reason_{};
    std::uint64_t seen_epoch_{},prepared_epoch_{};bool pending_{};
    bool adapter_valid() const noexcept {return adapter_.observe&&adapter_.game_thread&&adapter_.clock_ns;}
    void disarm(std::uint32_t reason=1) noexcept {
        // Read-only failure classification. No validity is extended and no
        // caller may use this diagnostic to recover a retired publication.
        // Native notices use0x100+reason; local disarms use0x200+the frozen
        // source line, distinguishing gate reentry from drain/quarantine.
        last_disarm_reason_.store(reason,std::memory_order_release);
        published_.store(0,std::memory_order_release);
        auto value=epoch_.load(std::memory_order_acquire);
        for(;;){
            if(value==(std::numeric_limits<std::uint64_t>::max)()){
                terminal_.store(true,std::memory_order_release);return;
            }
            if(epoch_.compare_exchange_weak(value,value+1,std::memory_order_acq_rel))return;
        }
    }
    void clear_gt(FrameReadFailure reason=FrameReadFailure::none,unsigned difference=0) noexcept {
        if(reason==FrameReadFailure::none&&committed_decision_.lease.kind==LeaseKind::support)
            reason=FrameReadFailure::implicit_clear;
        if(reason!=FrameReadFailure::none&&committed_.key.frame){
            const auto now=adapter_.clock_ns(adapter_.context);
            last_withdrawal_={reason,difference,committed_.key.frame,committed_.controllers.sequence,
                committed_.press_token,committed_.controllers.sync_ns>0&&now>=committed_.controllers.sync_ns?
                    now-committed_.controllers.sync_ns:-1,committed_.left_grip_down};
            last_withdrawal_.candidate=candidate_observation(committed_,committed_game_,committed_decision_);
        }
        participant_.invalidate_on_gt();committed_={};prepared_game_={};committed_game_={};committed_decision_={};
        pending_=false;prepared_epoch_=0;published_.store(0,std::memory_order_release);
    }
    void withdraw_expired_pose_gt() noexcept {
        if(participant_.snapshot().grip!=Grip::two||committed_decision_.lease.kind!=LeaseKind::support){
            clear_gt(FrameReadFailure::stale_before);return;
        }
        const auto now=adapter_.clock_ns(adapter_.context);
        last_withdrawal_={FrameReadFailure::stale_before,0,committed_.key.frame,committed_.controllers.sequence,
            committed_.press_token,now>=committed_.controllers.sync_ns?now-committed_.controllers.sync_ns:-1,
            committed_.left_grip_down};
        last_withdrawal_.candidate=candidate_observation(committed_,committed_game_,committed_decision_);
        // Expired render data grants NO Frame, snapshot, pose or action. Keep
        // only the participant's logical support incumbent for the NEXT
        // independently fresh prepare/commit. It must revalidate all object,
        // mount, reference, input/tracking epochs, held token and game guards;
        // a genuine release or invalidation still clears it normally.
        committed_={};prepared_game_={};committed_game_={};committed_decision_={};
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
    template<class Reader>
    bool equipment_locked(EquipmentIdentity const& expected,Reader& reader,EquipmentObservation& out) noexcept {
        const auto epoch=epoch_.load(std::memory_order_acquire);
        const auto current=[&]() noexcept {
            constexpr auto required=scene_owned|input_matched|gameplay_interactable;
            return on_gt()&&active(epoch)&&published_.load(std::memory_order_acquire)==epoch&&
                key_valid(committed_)&&frame_shape_valid(committed_)&&
                (committed_.flags&required)==required&&!committed_.recentered&&!committed_.heading_deferred&&
                equipment_matches(committed_,expected);
        };
        // Historical association only. The reader independently qualifies the
        // actual typed tracking source; no old Frame freshness is renewed.
        if(!equipment_identity_valid(expected)||!current())return false;
        const auto begin=adapter_.clock_ns(adapter_.context);
        if(begin<=0||!reader(expected,current,out)||!current())return false;
        const auto end=adapter_.clock_ns(adapter_.context);
        if(end<begin||std::uint64_t(end-begin)>50000000||!current())return false;
        out.begin_ns=begin;out.end_ns=end;return true;
    }
    Reply prepare(Frame const* frame,Proposal* out) noexcept {
        if(!frame||!out)return Reply::invalid;
        *out={};
        if(phase_.load(std::memory_order_acquire)!=ClientPhase::active||!on_gt())return Reply::unavailable;
        Gate gate(gate_);if(!gate.held){disarm(0x200u+__LINE__);return Reply::busy;}
        const auto epoch=epoch_.load(std::memory_order_acquire);
        if(seen_epoch_!=epoch){clear_gt();seen_epoch_=epoch;}
        if(!active(epoch)||!key_valid(*frame))return fail_gt(Reply::unavailable);
        auto now=adapter_.clock_ns(adapter_.context);GripGameState game{};
        if(!fresh(*frame,now)||!adapter_.observe(adapter_.context,*frame,now,game)||!game.known)
            return fail_gt(Reply::unavailable);
        now=adapter_.clock_ns(adapter_.context);
        if(!active(epoch)||!fresh(*frame,now))return fail_gt(Reply::unavailable);
        const bool support_was_held=participant_.snapshot().grip==Grip::two;
        const auto reply=participant_.prepare(*frame,game,*out);
        if(support_was_held&&reply==Reply::accepted&&
           out->incumbents[0].kind!=LeaseKind::support&&out->incumbents[1].kind!=LeaseKind::support){
            // Data from this exact existing prepare only. A withdrawal cannot
            // be explained by a later single-hand snapshot; retain which
            // lifetime/game fields actually differed before the old grant
            // disappeared. This grants no source, input or held authority.
            auto const& old=committed_;auto const& f=*frame;
            const unsigned changed=unsigned(!gears_vr::stereo_same_pose(old.reference,f.reference))|
                (unsigned(old.reference_generation!=f.reference_generation)<<1)|
                (unsigned(old.controllers.session!=f.controllers.session||old.controllers.space_epoch!=f.controllers.space_epoch||
                    old.controllers.profile_generation!=f.controllers.profile_generation||old.controllers.input_epoch!=f.controllers.input_epoch)<<2)|
                (unsigned(old.key.lifecycle!=f.key.lifecycle||old.key.world_generation!=f.key.world_generation||
                    old.key.registration_cookie!=f.key.registration_cookie||old.key.native_started_qpc!=f.key.native_started_qpc)<<3)|
                (unsigned(!same(old.world,f.world)||!same(old.controller,f.controller)||!same(old.pawn,f.pawn)||
                    !same(old.mesh,f.mesh)||!same(old.weapon,f.weapon))<<4)|
                (unsigned(!same(committed_game_.mount_asset,game.mount_asset)||committed_game_.mount_revision!=game.mount_revision)<<5);
            last_withdrawal_={FrameReadFailure::support_prepare,game_difference(game,committed_game_),f.key.frame,
                f.controllers.sequence,f.press_token,now>=f.controllers.sync_ns?now-f.controllers.sync_ns:-1,
                f.left_grip_down,changed,unsigned(game.known)|(unsigned(game.main_hand_held)<<1)|
                    (unsigned(game.animation_owns_hands)<<2)|(unsigned(game.busy)<<3)|(unsigned(game.mount_known)<<4)};
        }
        if(reply!=Reply::accepted)return fail_gt(reply);
        if(!active(epoch)){*out={};return fail_gt(Reply::unavailable);}
        prepared_game_=game;prepared_epoch_=epoch;pending_=true;
        // A previous frame must not remain consumable between prepare/commit.
        published_.store(0,std::memory_order_release);return Reply::accepted;
    }
    Reply commit(Frame const* frame,Proposal const* proposal,Decision const* decision) noexcept {
        if(!frame||!proposal||!decision)return Reply::invalid;
        if(phase_.load(std::memory_order_acquire)!=ClientPhase::active||!on_gt())return Reply::unavailable;
        Gate gate(gate_);if(!gate.held){disarm(0x200u+__LINE__);return Reply::busy;}
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
        committed_=*frame;committed_game_=game;committed_decision_=*decision;published_.store(epoch,std::memory_order_release);
        if(!active(epoch))return fail_gt(Reply::unavailable);
        if(adapter_.committed)adapter_.committed(adapter_.context,*frame,*decision);
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
        disarm(0x100u+unsigned(message->reason));
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
    // Query-local metadata lifetime, independent of the legacy published Hand
    // Frame. This number is not a pose/input/action permit. Explicit revoke,
    // reentry, drain or reconnect still invalidates a read in progress.
    std::uint64_t metadata_generation_on_gt() const noexcept {
        const auto epoch=epoch_.load(std::memory_order_acquire);
        return on_gt()&&active(epoch)&&cookie_.load(std::memory_order_acquire)&&
            native_.load(std::memory_order_acquire)?epoch:0;
    }
    std::uint32_t last_disarm_reason() const noexcept {return last_disarm_reason_.load(std::memory_order_acquire);}
    Registration registration() noexcept {
        Registration r{};r.context=this;r.prepare=prepare_callback;r.commit=commit_callback;r.invalidate=invalidate_callback;return r;
    }
    bool capture_existing_action_lifetime_on_gt(FrameKey const& expected,ExistingActionLifetime& out) noexcept {
        out={};if(!on_gt()||!armed())return false;
        Gate gate(gate_);if(!gate.held){disarm(0x200u+__LINE__);return false;}
        const auto epoch=epoch_.load(std::memory_order_acquire);const auto& f=committed_;
        if(!active(epoch)||published_.load(std::memory_order_acquire)!=epoch||!same(f.key,expected)||
           !key_valid(f)||!fresh(f,adapter_.clock_ns(adapter_.context)))return false;
        out={{f.key,f.world,f.controller,f.pawn,f.mesh,f.weapon,f.reference,f.reference_generation},epoch};return true;
    }
    bool existing_action_lifetime_on_gt(ExistingActionLifetime const& action,std::uint32_t* failure=nullptr) noexcept {
        if(failure)*failure=0;
        const auto reject=[&](std::uint32_t why) noexcept {if(failure)*failure=why;return false;};
        if(!on_gt()||!action.epoch||!equipment_identity_valid(action.parent))return reject(1);
        Gate gate(gate_);if(!gate.held){disarm(0x200u+__LINE__);return reject(2);}
        Frame association{};association.key=action.parent.key;
        if(!active(action.epoch))return reject(3);
        if(!key_valid(association))return reject(4);
        // clear_gt can retire a grip publication when the ORIGINAL action
        // changes busy/animation state. Explicit Native invalidation advances
        // epoch and always revokes this witness. A published new ancestry may
        // advance its frame, but cannot change the frozen owner/reference.
        if(committed_.key.frame){auto expected=action.parent;expected.key.frame=committed_.key.frame;
            if(committed_.key.frame<action.parent.key.frame)return reject(5);
            if(!equipment_matches(committed_,expected)){
                // Classify existing POD only; no renewed lease or game read.
                if(!same(committed_.key,expected.key))return reject(6);
                if(committed_.reference_generation!=expected.reference_generation)return reject(8);
                if(!gears_vr::stereo_same_pose(committed_.reference,expected.reference))return reject(9);
                return reject(7);
            }}
        return active(action.epoch)?true:reject(3);
    }
    // Existing parent callback lifetime is required, including early returns.
    // A fixed PG metadata continuation executes synchronously under one Gate;
    // it may not recursively call another Client method or enter Native.
    // No Frame, publication, participant state or reusable permit is returned.
    template<class Reader>
    bool observe_equipment_on_gt(EquipmentIdentity const& expected,Reader& reader,EquipmentObservation& out) noexcept {
        out={};
        if(!on_gt()||!armed())return false;
        if(gate_.test_and_set(std::memory_order_acquire)){disarm(0x200u+__LINE__);return false;}
        bool okay{};
#if defined(_MSC_VER)
        __try{okay=equipment_locked(expected,reader,out);}
        __finally{if(!okay)out={};gate_.clear(std::memory_order_release);}
#else
        okay=equipment_locked(expected,reader,out);gate_.clear(std::memory_order_release);
#endif
        if(!okay)out={};return okay;
    }
    // The typed current-body source caller authenticates its own immutable
    // Scene association, fresh tracking/eye basis and before/after actor.
    // Qualify ONLY metadata against this registration generation. Do not
    // consume, republish or rearm a missing/expired legacy Hand Frame.
    template<class Reader>
    bool observe_registered_equipment_on_gt(EquipmentIdentity const& expected,std::uint64_t generation,
        Reader& reader,EquipmentObservation& out) noexcept {
        out={};
        if(!generation||!equipment_identity_valid(expected)||metadata_generation_on_gt()!=generation)return false;
        if(gate_.test_and_set(std::memory_order_acquire)){disarm(0x200u+__LINE__);return false;}
        bool okay{};
        EquipmentIdentity frozen_identity{};std::memcpy(&frozen_identity,&expected,sizeof frozen_identity);
        const auto current=[&]() noexcept {
            auto const& k=expected.key;
            return metadata_generation_on_gt()==generation&&!std::memcmp(&expected,&frozen_identity,sizeof expected)&&
                k.game_pid==session_.game_pid&&k.game_start==session_.game_start&&
                k.thread_id==adapter_.game_thread(adapter_.context)&&
                registration_alive(k.registration_cookie,k.native_started_qpc);
        };
        const auto read=[&]() noexcept {
            if(!current())return false;
            const auto begin=adapter_.clock_ns(adapter_.context);
            if(begin<=0||!reader(expected,current,out)||!current())return false;
            const auto end=adapter_.clock_ns(adapter_.context);
            if(end<begin||std::uint64_t(end-begin)>50000000||!current())return false;
            out.begin_ns=begin;out.end_ns=end;return true;
        };
#if defined(_MSC_VER)
        __try{okay=read();}
        __finally{if(!okay)out={};gate_.clear(std::memory_order_release);}
#else
        okay=read();if(!okay)out={};gate_.clear(std::memory_order_release);
#endif
        return okay;
    }
    // Motion observation only, for the exact still-published parent of a
    // completed Native pose. Do not re-query pose-derived candidates here:
    // that may revoke a publication while merely recording its actual motion.
    // This neither renews the old input/decision nor certifies current game
    // state, and must never replace copy_frame_on_gt in an action/writer.
    bool copy_retained_item_input_on_gt(EquipmentIdentity const& expected,RetainedItemInput& out) noexcept {
        out={};if(!on_gt()||!armed()||!equipment_identity_valid(expected))return false;
        Gate gate(gate_);if(!gate.held){disarm(0x200u+__LINE__);return false;}
        const auto epoch=epoch_.load(std::memory_order_acquire);
        constexpr auto required=scene_owned|input_matched|gameplay_interactable;
        const auto& f=committed_;const auto& d=committed_decision_;
        if(!active(epoch)||published_.load(std::memory_order_acquire)!=epoch||pending_||
           !key_valid(f)||!frame_shape_valid(f)||(f.flags&required)!=required||f.recentered||f.heading_deferred||
           !equipment_matches(f,expected)||!same(d.key,f.key)||d.status!=DecisionStatus::retained||
           d.release_required||!lease_valid(d.lease,f.key.lifecycle)||
           (d.lease.kind!=LeaseKind::magazine&&d.lease.kind!=LeaseKind::ammo)||
           !f.left_tracked||!f.left_grip_known||!f.left_grip_down||
           !f.press_token||d.press_token!=f.press_token||d.lease.id!=f.press_token)return false;
        const auto now=adapter_.clock_ns(adapter_.context);
        if(now<f.now_ns||d.now_ns<f.now_ns||d.now_ns>now||f.controllers.sync_ns<=0||f.controllers.sync_ns>now)return false;
        const auto& left=f.controllers.hands[0];
        RetainedItemInput candidate{expected,d.lease,f.press_token,f.controllers.sequence,f.controllers.source,
            f.controllers.session,f.controllers.space_epoch,f.controllers.input_epoch,f.controllers.profile_generation,
            f.controllers.sync_ns,d.now_ns,left.pressed[left_control(f)],left.released[left_control(f)],f.interaction_control};
        if(!active(epoch)||published_.load(std::memory_order_acquire)!=epoch)return false;
        out=candidate;return true;
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
            phase_.store(ClientPhase::quarantined,std::memory_order_release);disarm(0x200u+__LINE__);return Reply::invalid;
        }
        cookie_.store(receipt.cookie,std::memory_order_release);native_.store(receipt.native_started_qpc,std::memory_order_release);
        phase_.store(terminal_.load(std::memory_order_acquire)?ClientPhase::draining:ClientPhase::active,std::memory_order_release);
        return phase()==ClientPhase::active?Reply::accepted:Reply::draining;
    }
    Reply begin_disconnect() noexcept {
        if(on_gt())return Reply::reentrant;
        if(phase()==ClientPhase::off)return Reply::quiescent;
        if(phase()==ClientPhase::connecting||phase()==ClientPhase::quarantined)return Reply::unavailable;
        phase_.store(ClientPhase::draining,std::memory_order_release);disarm(0x200u+__LINE__);return Reply::draining;
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
        GripGameState ignored{};return copy_frame_on_gt(out,ignored);
    }
    // Return this call's actual fresh observation together with its Frame.
    // Both are caller-owned, synchronous data, not a reusable source permit.
    bool copy_frame_on_gt(Frame& out,GripGameState& observed) noexcept {
        Decision ignored{};return copy_frame_on_gt(out,observed,ignored);
    }
    // The actual sole-Native-owner decision from this same committed frame.
    // Consumers cannot infer ammo/magazine ownership from Grip::single: it
    // also represents no candidate and released ownership. No new arbiter.
    bool copy_frame_on_gt(Frame& out,GripGameState& observed,Decision& decision,
        FrameReadObservation* observation=nullptr) noexcept {
        if(observation)*observation={};
        const auto fail=[&](FrameReadFailure reason,std::uint32_t difference=0)noexcept {
            if(observation){observation->failure=reason;observation->game_difference=difference;}return false;
        };
        out={};observed={};decision={};if(!on_gt()||!armed())return fail(FrameReadFailure::thread_or_unarmed);
        Gate gate(gate_);if(!gate.held){disarm(0x200u+__LINE__);return fail(FrameReadFailure::gate);}
        const auto epoch=epoch_.load(std::memory_order_acquire);
        if(!active(epoch)||published_.load(std::memory_order_acquire)!=epoch)return fail(FrameReadFailure::publication);
        auto now=adapter_.clock_ns(adapter_.context);GripGameState current{};
        if(observation){
            observation->frame=committed_.key.frame;observation->sequence=committed_.controllers.sequence;
            observation->press_token=committed_.press_token;observation->left_grip_down=committed_.left_grip_down;
            observation->input_age_ns=committed_.controllers.sync_ns>0&&now>=committed_.controllers.sync_ns?
                now-committed_.controllers.sync_ns:-1;
            observation->candidate=candidate_observation(committed_,committed_game_,committed_decision_);
        }
        if(!key_valid(committed_)){clear_gt(FrameReadFailure::key);return fail(FrameReadFailure::key);}
        if(!fresh(committed_,now)){withdraw_expired_pose_gt();return fail(FrameReadFailure::stale_before);}
        if(!adapter_.observe(adapter_.context,committed_,now,current)){clear_gt(FrameReadFailure::observe);return fail(FrameReadFailure::observe);}
        if(!same_pose_consumer_observation(committed_,committed_decision_,current,committed_game_)){
            // Classify candidates, actual item incumbents, and remaining
            // live-game/mount fields separately. Returned game data remains
            // the actual observation, including acquisition hover changes.
            const unsigned difference=game_difference(current,committed_game_);
            clear_gt(FrameReadFailure::game_changed,difference);return fail(FrameReadFailure::game_changed,difference);
        }
        now=adapter_.clock_ns(adapter_.context);
        if(!active(epoch)||published_.load(std::memory_order_acquire)!=epoch||!fresh(committed_,now))return fail(FrameReadFailure::stale_or_changed_after);
        std::memcpy(&out,&committed_,sizeof(out));observed=current;decision=committed_decision_;return true;
    }
    // Same callback lifetime lease requirement. Revalidate the exact existing
    // publication after a caller changes its software equipment model. Fresh
    // observe must still certify the committed actors, but held/busy state may
    // differ. Do not change committed_game_, refresh a grip or return a Frame.
    // A stale caller key cannot revoke a different healthy current publication.
    // This source-current witness is not equipment or pose-write permission.
private:
    bool source_observation_on_gt(FrameKey const& expected,GripGameState& out,bool source_only) noexcept {
        out={};
        if(!on_gt()||!armed())return false;
        Gate gate(gate_);if(!gate.held){disarm(0x200u+__LINE__);return false;}
        const auto epoch=epoch_.load(std::memory_order_acquire);
        if(!active(epoch)||published_.load(std::memory_order_acquire)!=epoch||!same(committed_.key,expected))return false;
        auto now=adapter_.clock_ns(adapter_.context);GripGameState current{};
        const auto observe=source_only&&adapter_.observe_source?adapter_.observe_source:adapter_.observe;
        if(!key_valid(committed_)){clear_gt(FrameReadFailure::key);return false;}
        if(!fresh(committed_,now)){withdraw_expired_pose_gt();return false;}
        if(!observe(adapter_.context,committed_,now,current)||!current.known){clear_gt(FrameReadFailure::observe);return false;}
        now=adapter_.clock_ns(adapter_.context);
        if(!active(epoch)||published_.load(std::memory_order_acquire)!=epoch||!fresh(committed_,now))return false;
        out=current;return true;
    }
public:
    FrameReadObservation withdrawal_on_gt() const noexcept {return on_gt()?last_withdrawal_:FrameReadObservation{};}
    bool source_game_on_gt(FrameKey const& expected,GripGameState& out) noexcept {
        return source_observation_on_gt(expected,out,false);
    }
    // Reuse the same witness when only liveness is requested. The optional
    // current game data above is not a new publication or a held/support grant.
    bool source_current_on_gt(FrameKey const& expected) noexcept {
        GripGameState ignored{};return source_observation_on_gt(expected,ignored,true);
    }
    // Requires the enclosing PG hook callback lifetime lease even on an early
    // return. Freeze this at the outer actual fire invocation. Do not reread it
    // independently for each pellet/target, and never use it as a throw scale.
    bool snapshot_on_gt(GripSnapshot& out) noexcept {
        out={};if(!on_gt()||!armed())return false;
        Gate gate(gate_);if(!gate.held){disarm(0x200u+__LINE__);return false;}
        const auto epoch=epoch_.load(std::memory_order_acquire);
        if(!active(epoch)||published_.load(std::memory_order_acquire)!=epoch)return false;
        auto now=adapter_.clock_ns(adapter_.context);GripGameState current{};
        if(!key_valid(committed_)){clear_gt(FrameReadFailure::key);return false;}
        if(!fresh(committed_,now)){withdraw_expired_pose_gt();return false;}
        if(!adapter_.observe(adapter_.context,committed_,now,current)){clear_gt(FrameReadFailure::observe);return false;}
        if(!same_pose_consumer_observation(committed_,committed_decision_,current,committed_game_)){
            clear_gt(FrameReadFailure::game_changed,game_difference(current,committed_game_));return false;}
        now=adapter_.clock_ns(adapter_.context);auto snapshot=participant_.snapshot();
        if(!active(epoch)||published_.load(std::memory_order_acquire)!=epoch||!fresh(committed_,now)||
           !snapshot.current(committed_.key,committed_.weapon,now))return false;
        out=snapshot;return true;
    }
};
} // namespace gears_weapon_hand
