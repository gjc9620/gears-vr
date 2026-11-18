#pragma once
#include "weapon_hand_participant.h"
#include "weapon_pose_solver.h"
#include "weapon_support_volume.h"
#include <limits>
#include <optional>

namespace gears_weapon_hand {
enum class Grip:std::uint32_t {unknown,single,two,excluded};
enum class WeaponKind:std::uint32_t {unknown,firearm,hand_thrown};
struct GripGameState {
    // A live GT adapter must certify inventory ownership, animation state and
    // this exact mesh/asset mount. An IK or mesh restoration lease is not proof.
    bool known{},main_hand_held{},animation_owns_hands{},busy{},mount_known{};
    WeaponKind kind{};
    ObjectIdentity mount_asset{};
    std::uint64_t mount_revision{};
    gears_weapon::WeaponMount mount{};
    double acquire_metres{.10},release_metres{.18};
    std::array<Lease,2> object_incumbents{}; // Actual ammo/magazine/other objects.
    bool request_ammo{},request_magazine{}; // Independently certified candidates.
    bool magazine_observation_pending{}; // Unknown current part geometry, never contact.
    gears_weapon::SupportBox support_box{}; // Optional; default sphere remains unchanged.
    gears_weapon::SupportRegion support_region{}; // Optional acquisition only; local adapter state, not wire ABI.
};
struct GripSnapshot {
    Grip grip{};FrameKey key{};
    ObjectIdentity world{},controller{},pawn{},mesh{},weapon{};
    std::uint64_t input_session{},input_sequence{},space_epoch{},profile_generation{},input_epoch{},reference_generation{};
    std::uint64_t mount_revision{},press_token{};
    Lease left_hand{};
    std::int64_t committed_ns{},stereo_received_ns{},input_received_ns{};
    bool current(FrameKey const& context,ObjectIdentity const& current_weapon,std::int64_t now_ns) const noexcept {
        return grip!=Grip::unknown&&same(key,context)&&same(weapon,current_weapon)&&
            now_ns>=committed_ns&&stereo_received_ns>0&&input_received_ns>0&&
            now_ns>=stereo_received_ns&&now_ns>=input_received_ns&&
            now_ns-stereo_received_ns<=100000000&&now_ns-input_received_ns<=100000000;
    }
};
inline bool same_hand(gears_vr::ControllerHand const& a,gears_vr::ControllerHand const& b) noexcept {
    if(!gears_vr::stereo_same_pose(a.grip,b.grip)||!gears_vr::stereo_same_pose(a.aim,b.aim)||
       a.grip_flags!=b.grip_flags||a.aim_flags!=b.aim_flags||a.grip_active!=b.grip_active||a.aim_active!=b.aim_active||
       a.buttons!=b.buttons||a.buttons_active!=b.buttons_active||a.trigger!=b.trigger||a.squeeze!=b.squeeze||
       a.stick_x!=b.stick_x||a.stick_y!=b.stick_y||a.trigger_active!=b.trigger_active||
       a.squeeze_active!=b.squeeze_active||a.stick_active!=b.stick_active||
       !gears_vr::stereo_same_pose(a.last_squeeze_press_grip,b.last_squeeze_press_grip)||
       a.last_squeeze_press_flags!=b.last_squeeze_press_flags||a.last_squeeze_press_sequence!=b.last_squeeze_press_sequence)return false;
    for(unsigned i=0;i<gears_vr::control_count;++i)if(a.pressed[i]!=b.pressed[i]||a.released[i]!=b.released[i])return false;
    return true;
}
inline bool same_frame(Frame const& a,Frame const& b) noexcept {
    auto const& x=a.controllers;auto const& y=b.controllers;
    return a.size==b.size&&a.abi_version==b.abi_version&&a.flags==b.flags&&a.reserved==b.reserved&&
        same(a.key,b.key)&&a.now_ns==b.now_ns&&same(a.world,b.world)&&same(a.controller,b.controller)&&
        same(a.pawn,b.pawn)&&same(a.mesh,b.mesh)&&same(a.weapon,b.weapon)&&
        a.reference_generation==b.reference_generation&&a.stereo_status==b.stereo_status&&
        a.recentered==b.recentered&&a.heading_deferred==b.heading_deferred&&a.reserved2==b.reserved2&&
        a.press_token==b.press_token&&a.left_grip_down==b.left_grip_down&&a.left_grip_known==b.left_grip_known&&
        a.left_tracked==b.left_tracked&&a.interaction_control==b.interaction_control&&
        gears_vr::stereo_same_pose(a.reference,b.reference)&&gears_vr::stereo_same(a.stereo,b.stereo)&&
        x.source==y.source&&x.session==y.session&&x.sequence==y.sequence&&x.space_epoch==y.space_epoch&&
        x.profile_generation==y.profile_generation&&x.input_epoch==y.input_epoch&&x.predicted_time==y.predicted_time&&
        x.received_ns==y.received_ns&&x.sync_ns==y.sync_ns&&x.focused==y.focused&&
        same_hand(x.hands[0],y.hands[0])&&same_hand(x.hands[1],y.hands[1]);
}
inline bool same_proposal(Proposal const& a,Proposal const& b) noexcept {
    return a.size==b.size&&a.abi_version==b.abi_version&&same(a.key,b.key)&&a.nonce==b.nonce&&
        same(a.incumbents[0],b.incumbents[0])&&same(a.incumbents[1],b.incumbents[1])&&
        a.support==b.support&&a.ammo==b.ammo&&a.magazine==b.magazine&&
        a.magazine_observation_pending==b.magazine_observation_pending;
}
inline bool support_contact(Frame const& f,GripGameState const& game,bool retain,double* measured_distance=nullptr) noexcept {
    if(!game.mount_known||!identity_valid(game.mount_asset)||!game.mount_revision||!game.mount.valid()||
       !std::isfinite(game.acquire_metres)||!std::isfinite(game.release_metres)||game.acquire_metres<=0||
       game.acquire_metres>.15||game.release_metres<game.acquire_metres||game.release_metres>.25)return false;
    auto const& right=f.controllers.hands[1];auto const& left=f.controllers.hands[0];
    if(!gears_vr::controller_pose_usable(right.grip,right.grip_flags,right.grip_active)||
       !gears_vr::controller_pose_usable(left.grip,left.grip_flags,left.grip_active))return false;
    auto const orientation=gears_vr::normalized(gears_vr::multiply(right.grip.orientation,
        gears_vr::inverse(game.mount.right_grip_in_weapon.orientation)));
    auto const expected=gears_weapon::rotate(orientation,gears_weapon::difference(
        game.mount.support_grip_in_weapon,game.mount.right_grip_in_weapon.position));
    auto const actual=gears_weapon::difference(left.grip.position,right.grip.position);
    const auto e=gears_weapon::length(expected),a=gears_weapon::length(actual);
    const auto distance=gears_weapon::length(gears_weapon::difference(expected,actual));
    if(measured_distance&&std::isfinite(distance))*measured_distance=distance;
    const auto offset=gears_weapon::rotate(gears_vr::inverse(orientation),gears_weapon::difference(actual,expected));
    bool contact{};
    if(game.support_region.enabled){
        const gears_vr::Pose physical_grip_in_mount{
            {game.mount.support_grip_in_weapon.x+offset.x,game.mount.support_grip_in_weapon.y+offset.y,
             game.mount.support_grip_in_weapon.z+offset.z},
            gears_vr::normalized(gears_vr::multiply(gears_vr::inverse(orientation),left.grip.orientation))};
        const bool region_contact=retain?
            (game.support_region.snap_limits_enabled||distance<=game.release_metres):game.support_region.contains(offset);
        contact=region_contact&&game.support_region.snap(physical_grip_in_mount,retain);
    }else contact=game.support_box.enabled?game.support_box.contains(offset,retain):
        distance<=(retain?game.release_metres:game.acquire_metres);
    if(game.mount.close_support())
        return std::isfinite(a)&&std::isfinite(distance)&&contact&&
            distance<=game.mount.support_length_tolerance_metres;
    return std::isfinite(a)&&a>.10&&std::isfinite(distance)&&contact&&
        std::abs(a-e)<=game.mount.support_length_tolerance_metres&&
        gears_weapon::dot(expected,actual)/(e*a)>=std::cos(game.mount.maximum_support_correction_radians);
}

// Neutral auto-socket selection, not a hand lease or a retained tracking pose.
// A later Grip still needs CURRENT contact and the shared owner's real edge.
// This affinity may choose a known support socket over an unavailable unrelated
// part; it never chooses support on a cold held press or a known part request.
class SupportSocketPreference {
    FrameKey key_{};std::array<ObjectIdentity,6> objects_{};
    gears_vr::Pose reference_{};gears_weapon::WeaponMount mount_{};
    std::array<std::uint64_t,9> epochs_{};
    double acquire_{},release_{};bool selected_{};
    gears_weapon::SupportBox box_{};
    gears_weapon::SupportRegion region_{};
    static bool same_mount(gears_weapon::WeaponMount const& a,gears_weapon::WeaponMount const& b) noexcept {
        const auto vector=[](gears_vr::Vector p,gears_vr::Vector q){return p.x==q.x&&p.y==q.y&&p.z==q.z;};
        return gears_vr::stereo_same_pose(a.right_grip_in_weapon,b.right_grip_in_weapon)&&
            vector(a.support_grip_in_weapon,b.support_grip_in_weapon)&&vector(a.muzzle_in_weapon,b.muzzle_in_weapon)&&
            vector(a.bore_forward_in_weapon,b.bore_forward_in_weapon)&&
            a.support_length_tolerance_metres==b.support_length_tolerance_metres&&
            a.maximum_support_correction_radians==b.maximum_support_correction_radians;
    }
public:
    void reset() noexcept {selected_=false;}
    bool current_contact(Frame const& f,GripGameState const& game,std::uint64_t hold,std::int64_t now) const noexcept {
        if(!selected_||!hold||!frame_active(f,now)||!game.known||!game.main_hand_held||game.busy||
           game.animation_owns_hands||game.kind!=WeaponKind::firearm||!f.left_tracked||!f.left_grip_known||
           index_pinch(f)||game.request_magazine||game.request_ammo)return false;
        for(auto const& item:game.object_incumbents)if(item.kind!=LeaseKind::none)return false;
        const std::array<ObjectIdentity,6> objects{f.world,f.controller,f.pawn,f.mesh,f.weapon,game.mount_asset};
        const std::array<std::uint64_t,9> epochs{f.reference_generation,f.stereo.session,f.stereo.space_epoch,
            f.controllers.input_epoch,f.controllers.profile_generation,hold,game.mount_revision,
            std::uint64_t(f.stereo.source),std::uint64_t(f.controllers.source)};
        auto expected=key_;expected.frame=f.key.frame;
        if(f.key.frame<key_.frame||!same(expected,f.key)||epochs_!=epochs||
           !gears_vr::stereo_same_pose(reference_,f.reference)||!same_mount(mount_,game.mount)||
           acquire_!=game.acquire_metres||release_!=game.release_metres||!(box_==game.support_box)||
           !(region_==game.support_region))return false;
        for(std::size_t i=0;i<objects.size();++i)if(!same(objects_[i],objects[i]))return false;
        // The selected socket is identity only. Always evaluate TODAY's raw
        // hand pair against the unchanged retention geometry, never old poses.
        return support_contact(f,game,true);
    }
    bool observe(Frame const& f,GripGameState const& game,std::uint64_t hold,std::int64_t now,bool another_socket) noexcept {
        bool occupied{};for(auto const& item:game.object_incumbents)occupied|=item.kind!=LeaseKind::none;
        if(!hold||another_socket||occupied||!frame_active(f,now)||!game.known||!game.main_hand_held||game.busy||
           game.animation_owns_hands||game.kind!=WeaponKind::firearm||!f.left_tracked||!f.left_grip_known||
           index_pinch(f)||game.request_magazine||game.request_ammo){
            reset();return false;
        }
        const bool retained=current_contact(f,game,hold,now);
        if(!retained&&!support_contact(f,game,false)){reset();return false;}
        const auto& left=f.controllers.hands[0];
        const std::array<ObjectIdentity,6> objects{f.world,f.controller,f.pawn,f.mesh,f.weapon,game.mount_asset};
        const std::array<std::uint64_t,9> epochs{f.reference_generation,f.stereo.session,f.stereo.space_epoch,
            f.controllers.input_epoch,f.controllers.profile_generation,hold,game.mount_revision,
            std::uint64_t(f.stereo.source),std::uint64_t(f.controllers.source)};
        if(!f.left_grip_down&&left.squeeze_active==1&&!control_held(left,gears_vr::control_squeeze)){
            key_=f.key;objects_=objects;epochs_=epochs;reference_=f.reference;mount_=game.mount;
            acquire_=game.acquire_metres;release_=game.release_metres;box_=game.support_box;
            region_=game.support_region;selected_=true;return false;
        }
        if(!retained){reset();return false;}
        return f.left_grip_down&&control_held(left,gears_vr::control_squeeze);
    }
};

// A participant only: it never instantiates or updates LeftHandInteractionOwner.
// prepare is read-only with respect to the game; commit changes private grip
// state only after the shared Native arbiter has decided this exact frame.
class SupportGripParticipant {
    std::optional<Frame> last_;
    SupportSocketPreference socket_;
    Frame prepared_{};Proposal proposal_{};GripGameState game_{};
    Lease support_{};ObjectIdentity mount_asset_{};std::uint64_t mount_revision_{},nonce_{};
    GripSnapshot snapshot_{};bool pending_{};
    static bool same_lifetime(Frame const& a,Frame const& b) noexcept {
        return a.key.game_start==b.key.game_start&&a.key.game_pid==b.key.game_pid&&
            a.key.native_started_qpc==b.key.native_started_qpc&&a.key.registration_cookie==b.key.registration_cookie&&
            a.key.lifecycle==b.key.lifecycle&&a.key.world_generation==b.key.world_generation&&a.key.thread_id==b.key.thread_id&&
            same(a.world,b.world)&&same(a.controller,b.controller)&&same(a.pawn,b.pawn)&&same(a.mesh,b.mesh)&&same(a.weapon,b.weapon)&&
            a.controllers.session==b.controllers.session&&a.controllers.space_epoch==b.controllers.space_epoch&&
            a.controllers.profile_generation==b.controllers.profile_generation&&a.controllers.input_epoch==b.controllers.input_epoch&&
            a.reference_generation==b.reference_generation&&gears_vr::stereo_same_pose(a.reference,b.reference);
    }
    static bool main_valid(Frame const& f,GripGameState const& game,std::int64_t now_ns) noexcept {
        auto const& right=f.controllers.hands[1];
        return frame_active(f,now_ns)&&game.known&&game.main_hand_held&&!game.animation_owns_hands&&!game.busy&&
            f.weapon.address&&game.kind!=WeaponKind::unknown&&
            gears_vr::controller_pose_usable(right.grip,right.grip_flags,right.grip_active);
    }
    void clear() noexcept {support_={};snapshot_={};mount_asset_={};mount_revision_=0;socket_.reset();}
public:
    void invalidate_on_gt() noexcept {clear();pending_=false;}
    GripSnapshot snapshot() const noexcept {return snapshot_;}
    Reply prepare(Frame const& f,GripGameState const& game,Proposal& out) noexcept {
        out={};
        if(pending_||!frame_shape_valid(f)||!game.known||nonce_==(std::numeric_limits<std::uint64_t>::max)()){
            invalidate_on_gt();return Reply::invalid;
        }
        if(last_&&last_->key.game_pid==f.key.game_pid&&last_->key.game_start==f.key.game_start&&
           last_->key.native_started_qpc==f.key.native_started_qpc&&last_->key.registration_cookie==f.key.registration_cookie&&
           (f.key.lifecycle<last_->key.lifecycle||(f.key.lifecycle==last_->key.lifecycle&&f.key.frame<=last_->key.frame))){
            invalidate_on_gt();return Reply::invalid;
        }
        if(!last_||!same_lifetime(*last_,f)||!same(mount_asset_,game.mount_asset)||mount_revision_!=game.mount_revision)clear();
        last_=f;prepared_=f;game_=game;proposal_={};proposal_.key=f.key;proposal_.nonce=++nonce_;
        unsigned count{};
        for(auto const& lease:game.object_incumbents){
            if(!lease_valid(lease,f.key.lifecycle)||lease.kind==LeaseKind::cover||lease.kind==LeaseKind::support){
                invalidate_on_gt();return Reply::invalid;
            }
            if(lease.kind!=LeaseKind::none)proposal_.incumbents[count++]=lease;
        }
        if(count==2&&same(proposal_.incumbents[0],proposal_.incumbents[1])){
            invalidate_on_gt();return Reply::invalid;
        }
        const bool main=main_valid(f,game,f.now_ns);
        const bool logical=f.left_grip_known&&f.left_tracked&&f.left_grip_down;
        // Same identity-only affinity as the producer's socket preference.
        // mount_revision is the authenticated held-mount generation. A new
        // socket still needs acquisition contact; the old socket may use its
        // CURRENT retention contact on a new Grip. No proposal is authorized
        // until the existing real-input arbiter and commit checks succeed.
        socket_.observe(f,game,game.mount_revision,f.now_ns,false);
        const bool socket_contact=socket_.current_contact(f,game,game.mount_revision,f.now_ns);
        // The default spherical policy retains a genuine grasp until its
        // initiating control/identity/tracking lifetime ends. An explicitly
        // bounded box additionally requires CURRENT keep-volume contact.
        const bool retain=main&&logical&&support_.kind==LeaseKind::support&&
            support_.id==f.press_token&&lease_valid(support_,f.key.lifecycle)&&
            (!game.support_box.enabled||support_contact(f,game,true));
        if(retain&&count<proposal_.incumbents.size())proposal_.incumbents[count++]=support_;
        else if(!retain)support_={};
        // If all three exist, two are enough to expose an incumbent conflict;
        // never omit the real object holders in order to claim a free hand.
        if(main&&logical&&game.kind==WeaponKind::firearm){
            const bool pending=game.magazine_observation_pending&&f.left_grip_down&&f.left_grip_known&&f.left_tracked&&!count&&!retain&&
                !game.request_magazine&&!game.request_ammo;
            // A not-yet-observed part must not turn the same press into a
            // support grasp before its current fitted pair becomes available.
            proposal_.support=(retain||(!pending&&!game.request_magazine&&!game.request_ammo&&
                (support_contact(f,game,false)||socket_contact)))?1u:0u;
            proposal_.ammo=game.request_ammo?1u:0u;proposal_.magazine=game.request_magazine?1u:0u;
            proposal_.magazine_observation_pending=pending?1u:0u;
        }
        mount_asset_=game.mount_asset;mount_revision_=game.mount_revision;
        pending_=true;out=proposal_;return Reply::accepted;
    }
    Reply commit(Frame const& f,Proposal const& p,Decision const& d) noexcept {
        if(!pending_){clear();return Reply::invalid;}pending_=false;snapshot_={};
        if(!same_frame(f,prepared_)||!same_proposal(p,proposal_)||d.size!=sizeof(Decision)||d.abi_version!=version||
           !same(d.key,f.key)||d.proposal_nonce!=p.nonce||d.new_grant>1||d.release_required>1||d.reserved||
           !lease_valid(d.lease,f.key.lifecycle)||unsigned(d.status)>unsigned(DecisionStatus::lease_lost)||
           d.now_ns<f.now_ns||d.press_token!=f.press_token){clear();return Reply::invalid;}
        const bool owns=d.status==DecisionStatus::granted||d.status==DecisionStatus::retained;
        if(owns!=(d.lease.kind!=LeaseKind::none)||d.new_grant!=(d.status==DecisionStatus::granted?1u:0u)||
           (owns&&(!d.press_token||d.release_required||!f.left_grip_known||!f.left_tracked||!f.left_grip_down))){
            clear();return Reply::invalid;
        }
        const bool incumbent=same(p.incumbents[0],d.lease)||same(p.incumbents[1],d.lease);
        if(owns)for(auto const& lease:p.incumbents)if(lease.kind!=LeaseKind::none&&
           (d.lease.kind==LeaseKind::cover||!same(lease,d.lease))){clear();return Reply::invalid;}
        const bool retained_support=d.status==DecisionStatus::retained&&incumbent&&
            same(support_,d.lease)&&d.lease.id==f.press_token;
        if(d.lease.kind==LeaseKind::support&&(!main_valid(f,game_,d.now_ns)||!p.support||
           (!retained_support&&!support_contact(f,game_,false)&&
             !socket_.current_contact(f,game_,game_.mount_revision,d.now_ns))||
           (d.status==DecisionStatus::retained&&!retained_support)||
           (d.status==DecisionStatus::granted&&(incumbent||d.lease.id!=d.press_token||
             p.incumbents[0].kind!=LeaseKind::none||p.incumbents[1].kind!=LeaseKind::none||p.ammo||p.magazine)))){
            clear();return Reply::invalid;
        }
        if((d.lease.kind==LeaseKind::ammo||d.lease.kind==LeaseKind::magazine||d.lease.kind==LeaseKind::other)&&
           ((d.status==DecisionStatus::retained&&!incumbent)||
            (d.status==DecisionStatus::granted&&(incumbent||d.lease.id!=d.press_token||
              unsigned(p.support+p.ammo+p.magazine)!=1||d.lease.kind==LeaseKind::other||
              (d.lease.kind==LeaseKind::ammo?!p.ammo:!p.magazine))))){clear();return Reply::invalid;}
        Grip grip=Grip::unknown;
        if(!main_valid(f,game_,d.now_ns))support_={};
        else if(game_.kind==WeaponKind::hand_thrown)grip=Grip::excluded;
        else if(!f.left_grip_known||!f.left_tracked)support_={};
        else if(d.lease.kind==LeaseKind::support){
            support_=d.lease;grip=Grip::two;
        }else{
            support_={};
            if(owns||d.status==DecisionStatus::released||d.status==DecisionStatus::needs_release||
               d.status==DecisionStatus::no_candidate||d.status==DecisionStatus::lease_lost)grip=Grip::single;
        }
        snapshot_={grip,f.key,f.world,f.controller,f.pawn,f.mesh,f.weapon,
            f.controllers.session,f.controllers.sequence,f.controllers.space_epoch,f.controllers.profile_generation,
            f.controllers.input_epoch,f.reference_generation,game_.mount_revision,d.press_token,d.lease,
            d.now_ns,f.stereo.received_ns,f.controllers.received_ns};
        return Reply::accepted;
    }
};
} // namespace gears_weapon_hand
