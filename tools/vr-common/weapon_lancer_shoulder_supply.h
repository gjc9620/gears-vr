#pragma once
#include "body_slots.h"

namespace gears_lancer_shoulder_supply {
struct Parameters {
    gears_vr::Vector adjust{};double scale{1};
    bool valid() const noexcept {
        return gears_vr::finite(adjust)&&std::hypot(std::hypot(adjust.x,adjust.y),adjust.z)<=.5&&
            std::isfinite(scale)&&scale>=.5&&scale<=2.;
    }
};
inline bool center(Parameters const& p,gears_vr::Pose const& body,gears_vr::Vector& out) noexcept {
    out={};gears_vr::Vector base{};
    if(!p.valid()||!gears_weapon::shoulder_ammo_center(gears_weapon::default_body_slot_layout(),body,base))return false;
    const auto shift=gears_vr::transform(gears_vr::rotation_matrix(body.orientation),p.adjust);
    out={base.x+shift.x,base.y+shift.y,base.z+shift.z};return gears_vr::finite(out);
}
// Workbench-authored supply layout, not an armspan measurement. Apply ONLY
// to the empty magazine's left-shoulder supply request. Do not edit global
// calibration or either right-controller weapon-slot classification.
inline bool contains(Parameters const& p,gears_vr::Pose const& body,gears_vr::Vector hand) noexcept {
    if(!p.valid()||!gears_vr::stereo_pose_valid(body)||!gears_vr::finite(hand))return false;
    const auto axes=gears_vr::rotation_matrix(body.orientation);
    const auto shift=gears_vr::transform(axes,p.adjust);
    if(p.scale==1.){
        auto shifted=body;shifted.position={body.position.x+shift.x,body.position.y+shift.y,body.position.z+shift.z};
        return gears_weapon::body_slot(gears_weapon::default_body_slot_layout(),shifted,hand,0)==
            gears_weapon::BodySlot::shoulder_ammo;
    }
    gears_vr::Vector center{};
    if(!gears_weapon::shoulder_ammo_center(gears_weapon::default_body_slot_layout(),body,center))return false;
    const auto local=gears_vr::transform(gears_vr::transpose(axes),
        {hand.x-center.x-shift.x,hand.y-center.y-shift.y,hand.z-center.z-shift.z});
    const double side=.2045*p.scale,reach=.24*p.scale;
    return local.x*local.x/(side*side)+(local.y*local.y+local.z*local.z)/(reach*reach)<=1.;
}
}
