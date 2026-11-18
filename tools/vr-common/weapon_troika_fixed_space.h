#pragma once
#include "weapon_troika_interaction.h"
#include "controller_input_frame.h"
#include "rig_tracking_math.h"
namespace gears_weapon {
// Math only. Caller must authenticate the original mounted owner and the
// Native entry calibration. This is NOT an ordinary hand Frame or a fire grant.
struct TroikaSpaceCalibration {
    gears_vr::StereoMatrix entry_base_view{};
    gears_vr::Pose entry_reference{};
    double units_per_metre{};
    std::uint64_t reference_generation{};
};
struct TroikaMappedHands {
    std::array<gears_vr::Pose,2> world{};
    std::uint64_t input_sequence{};
    std::int64_t input_sync_ns{},input_received_ns{},stereo_received_ns{};
    unsigned tracked_mask{};
};
class TroikaFixedSpace {
    TroikaIdentity owner_{};TroikaSpaceCalibration calibration_{};bool bound_{};
public:
    bool bind(TroikaIdentity const& owner,TroikaSpaceCalibration const& c) noexcept {
        if(bound_||!owner.valid()||!c.reference_generation||!gears_vr::stereo_rigid(c.entry_base_view)||
           !gears_vr::stereo_pose_valid(c.entry_reference)||!std::isfinite(c.units_per_metre)||
           c.units_per_metre<10||c.units_per_metre>500)return false;
        owner_=owner;calibration_=c;bound_=true;return true;
    }
    void reset() noexcept {bound_=false;owner_={};calibration_={};}
    bool map(TroikaIdentity const& owner,std::uint64_t reference_generation,
        gears_vr::ControllerInputFrame const& input,gears_vr::StereoSample const& stereo,
        std::int64_t now,TroikaMappedHands& out,bool allow_sim=false) const noexcept {
        if(!bound_||!(owner==owner_)||reference_generation!=calibration_.reference_generation||
           input.session!=owner.xr_session||input.space_epoch!=owner.space_epoch||
           input.input_epoch!=owner.input_epoch||input.profile_generation!=owner.profile_generation||
           now<=0||stereo.received_ns<=0||stereo.received_ns>now||now-stereo.received_ns>100000000||
           input.sync_ns<=0||input.sync_ns>now||now-input.sync_ns>100000000||
           !gears_vr::stereo_render_active(stereo)||!gears_vr::stereo_tracking_valid(stereo,allow_sim)||
           !gears_vr::stereo_geometry_valid(stereo)||!gears_vr::controller_frame_matches(input,stereo,now,allow_sim))return false;
        TroikaMappedHands next{};next.input_sequence=input.sequence;
        next.input_sync_ns=input.sync_ns;next.input_received_ns=input.received_ns;next.stereo_received_ns=stereo.received_ns;
        for(unsigned i=0;i<2;++i){const auto& hand=input.hands[i];
            if(!gears_vr::controller_pose_usable(hand.grip,hand.grip_flags,hand.grip_active))continue;
            if(!gears_vr::rig_tracking::pose(calibration_.entry_base_view,calibration_.entry_reference,
               hand.grip,calibration_.units_per_metre,next.world[i]))return false;
            next.tracked_mask|=1u<<i;
        }
        // Original timestamps are copied, never renewed. No current HMD or
        // Controller.Rotation is used to rotate the entry-fixed hand basis.
        out=next;return true;
    }
};
} // namespace gears_weapon
