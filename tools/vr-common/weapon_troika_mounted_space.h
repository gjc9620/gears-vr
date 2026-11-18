#pragma once
#include "weapon_troika_fixed_space.h"
#include "troika_mounted_source.h"
#include <cstring>

namespace gears_weapon {
// Geometric lifetime only. The caller authenticates this original mount before
// and after use. This record is never a hand lease, C0 grant or fire permit.
struct TroikaMountKey {
    gears_rig_tracking::NativeKey native{};
    gears_weapon_hand::ObjectIdentity world{},controller{},driver{},turret{},weapon{};
    std::uint64_t binding{},token{};
};
inline bool troika_same_mount(TroikaMountKey const& a,TroikaMountKey const& b) noexcept {
    namespace h=gears_weapon_hand;
    return gears_rig_tracking::same(a.native,b.native)&&h::same(a.world,b.world)&&h::same(a.controller,b.controller)&&
        h::same(a.driver,b.driver)&&h::same(a.turret,b.turret)&&h::same(a.weapon,b.weapon)&&a.binding==b.binding&&a.token==b.token;
}
struct TroikaMountedMapping {
    TroikaIdentity identity{};TroikaMappedHands hands{};gears_vr::Pose world_head{};
    std::uint64_t publication_sequence{},source_ticket{};
    double units_per_metre{};
    bool new_publication{},continuity_changed{};
};
class TroikaMountedSpace {
    TroikaMountKey owner_{};TroikaSpaceCalibration calibration_{};
    gears_weapon_hand::ObjectIdentity driver_mesh_{};
    gears_rig_tracking::NativeReference reference_{};
    gears_vr::StereoSource source_{};
    std::uint64_t entry_world_generation_{},xr_session_{},space_epoch_{},publisher_start_{};
    std::uint32_t publisher_pid_{};
    gears_troika_source::Frame previous_{};unsigned previous_mask_{};
    bool captured_{},have_previous_{},interrupted_{true};
public:
    // Return the authentic PRE-HMD entry basis, not world_head. The renderer
    // composes this exact source's eyes once through stereo_matrices. A turret
    // camera/rotation is deliberately not an input to the fixed-space view.
    bool render_base(TroikaMountKey const& owner,gears_troika_source::Frame const& source,
        std::int64_t now,gears_vr::StereoMatrix& output) noexcept {
        TroikaMountedMapping proof{};
        if(!map(owner,source,now,proof))return false;
        output=calibration_.entry_base_view;return true;
    }
    bool map_press(TroikaMountKey const& owner,gears_troika_source::Frame const& source,unsigned side,
        std::int64_t now,gears_vr::Pose& output) noexcept {
        output={};TroikaMountedMapping proof{};
        if(side>1||!map(owner,source,now,proof))return false;
        auto const& hand=source.controllers.hands[side];
        return hand.last_squeeze_press_sequence&&hand.last_squeeze_press_sequence<=source.controllers.sequence&&
            gears_vr::controller_pose_usable(hand.last_squeeze_press_grip,hand.last_squeeze_press_flags,1)&&
            gears_vr::rig_tracking::pose(calibration_.entry_base_view,calibration_.entry_reference,
                hand.last_squeeze_press_grip,calibration_.units_per_metre,output);
    }
    void reset() noexcept {*this=TroikaMountedSpace{};}
    void interrupt() noexcept {interrupted_=true;}
    bool capture(TroikaMountKey const& owner,gears_rig_tracking::Frame const& entry,std::int64_t now) noexcept {
        namespace r=gears_rig_tracking;namespace h=gears_weapon_hand;
        const bool simulated=entry.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated;
        if(captured_||!owner.binding||!owner.token||!h::identity_valid(owner.turret)||!h::identity_valid(owner.weapon)||
           !r::frame_valid(entry,now,r::maximum_age_ns,simulated)||!r::same(owner.native,entry.native)||
           !h::same(owner.world,entry.world)||!h::same(owner.controller,entry.controller)||!h::same(owner.driver,entry.pawn)||
           owner.turret.address==owner.driver.address||!(entry.flags&r::producer_continuity_available))return false;
        TroikaSpaceCalibration calibration{entry.base_view,entry.reference,entry.units_per_metre,entry.reference_generation};
        TroikaIdentity identity{owner.world.address,entry.world_generation,owner.driver.address,owner.controller.address,
            owner.turret.address,owner.binding,owner.weapon.address,owner.binding,entry.controllers.session,
            entry.controllers.space_epoch,entry.controllers.input_epoch,entry.controllers.profile_generation};
        TroikaFixedSpace proof;if(!proof.bind(identity,calibration))return false;
        owner_=owner;calibration_=calibration;driver_mesh_=entry.mesh;reference_=entry.native_reference;
        source_=entry.stereo.source;entry_world_generation_=entry.world_generation;xr_session_=entry.stereo.session;
        space_epoch_=entry.stereo.space_epoch;publisher_pid_=entry.controller_publisher_pid;publisher_start_=entry.controller_publisher_start;
        captured_=true;have_previous_=false;interrupted_=true;return true;
    }
    bool map(TroikaMountKey const& owner,gears_troika_source::Frame const& source,std::int64_t now,TroikaMountedMapping& output) noexcept {
        namespace h=gears_weapon_hand;namespace v=gears_vr;namespace r=gears_rig_tracking;
        const auto fail=[&](){interrupted_=true;return false;};
        if(!captured_||!troika_same_mount(owner_,owner)||!gears_troika_source::frame_valid(source,now,r::maximum_age_ns,
           source_==v::StereoSource::MetaOperatorSimulated)||!r::same(source.native,owner_.native)||
           !h::same(source.world,owner_.world)||!h::same(source.controller,owner_.controller)||!h::same(source.driver,owner_.driver)||
           !h::same(source.turret,owner_.turret)||!h::same(source.weapon,owner_.weapon)||!h::same(source.driver_mesh,driver_mesh_)||
           source.world_generation<entry_world_generation_||source.stereo.source!=source_||source.stereo.session!=xr_session_||
           source.stereo.space_epoch!=space_epoch_||source.reference_generation!=calibration_.reference_generation||
           !v::stereo_same_pose(source.reference,calibration_.entry_reference)||source.native_reference.epoch!=reference_.epoch||
           source.publisher_pid!=publisher_pid_||source.publisher_start!=publisher_start_)return fail();
        if(have_previous_&&(source.publication_sequence<previous_.publication_sequence||
           (source.publication_sequence==previous_.publication_sequence&&std::memcmp(&source,&previous_,sizeof source))))return fail();
        TroikaMountedMapping next{};next.identity={owner_.world.address,source.world_generation,owner_.driver.address,owner_.controller.address,
            owner_.turret.address,owner_.binding,owner_.weapon.address,owner_.binding,xr_session_,space_epoch_,
            source.controllers.input_epoch,source.controllers.profile_generation};
        // Input/focus/profile boundaries do not rotate the physical entry
        // basis. They produce a new data identity/continuity boundary; the
        // action consumer must stop and require fresh grip/trigger edges.
        TroikaFixedSpace mapper;
        if(!mapper.bind(next.identity,calibration_)||!mapper.map(next.identity,source.reference_generation,
           source.controllers,source.stereo,now,next.hands,source_==v::StereoSource::MetaOperatorSimulated)||
           !v::rig_tracking::pose(calibration_.entry_base_view,calibration_.entry_reference,source.stereo.head,
               calibration_.units_per_metre,next.world_head))return fail();
        next.publication_sequence=source.publication_sequence;next.source_ticket=source.source_ticket;
        next.units_per_metre=calibration_.units_per_metre;
        next.new_publication=!have_previous_||source.publication_sequence>previous_.publication_sequence;
        next.continuity_changed=next.new_publication&&(interrupted_||!have_previous_||
            source.world_generation!=previous_.world_generation||source.native_reference_revision!=previous_.native_reference_revision||
            source.controllers.input_epoch!=previous_.controllers.input_epoch||source.controllers.profile_generation!=previous_.controllers.profile_generation||
            source.producer.stream!=previous_.producer.stream||source.producer.tracking[0]!=previous_.producer.tracking[0]||
            source.producer.tracking[1]!=previous_.producer.tracking[1]||next.hands.tracked_mask!=previous_mask_);
        if(next.new_publication){std::memcpy(&previous_,&source,sizeof source);previous_mask_=next.hands.tracked_mask;have_previous_=true;interrupted_=false;}
        output=next;return true;
    }
};
} // namespace gears_weapon
