#pragma once
#include "weapon_gnasher_producer_segment.h"
#include "../vr-common/rig_tracking_math.h"

namespace gears_gnasher_producer_segment {
// Cancel the common XR/world basis before interpreting contact geometry.
// No past raw pose is mapped through a current camera/reference transform.
inline bool lever_motion(Segment const& segment,gears_weapon::GnasherMechanicalConfig const& config,
    gears_weapon::GnasherIdentity const& identity,gears_weapon::GnasherLeverMotion& out) noexcept {
    out={};
    if(!config.valid()||!identity.valid()||segment.count<2||segment.count>segment.points.size()||
       segment.count>out.points.size())return false;
    gears_weapon::GnasherLeverMotion result{};
    for(unsigned i=0;i<segment.count;++i){
        auto const& input=segment.points[i].input;
        auto const& left=input.hands[0].grip;auto const& right=input.hands[1].grip;
        if(!c::tracked_input_shape_valid(segment.points[i])||
           input.session!=identity.session||input.space_epoch!=identity.space_epoch||
           input.input_epoch!=identity.input_epoch||input.profile_generation!=identity.profile_generation)return false;
        const auto inverse=vr::inverse(left.orientation);
        vr::Pose relative{};
        relative.position=vr::xr_to_ue(gears_weapon::rotate(inverse,
            gears_weapon::difference(right.position,left.position)));
        if(!vr::rig_tracking::detail::quaternion(vr::xr_to_ue(vr::rotation_matrix(
            vr::multiply(inverse,right.orientation))),relative.orientation))return false;
        vr::Pose mathematical_main{};double angle{};
        if(!gears_weapon::gnasher_lever_geometry(config,vr::Pose{},relative,mathematical_main,angle))return false;
        result.points[i]={identity.weapon,identity.world,identity.space_epoch,input.sequence,input.sync_ns,
            angle,true,true,true};
    }
    result.count=segment.count;out=result;return true;
}
}
