#pragma once
#include "weapon_torque_bow_arrow_presentation.h"
#include "weapon_torque_bow_arrow_tracking_fixture.h"
// Coherent synthetic RawRig. We move only the fixed gun mount to place the
// original slot around this real input-derived tip. No fake world-grip bytes.
namespace bow_arrow_presentation_fixture {
namespace p=gears_torque_bow_arrow_presentation;namespace am=p::am;namespace vr=p::vr;namespace w=p::w;
struct Fixture:bow_arrow_tracking_fixture::Fixture {
    am::Mount arrow{};w::TorqueBowFeedCalibration feed{};w::WeaponMeshFollow fixed;
    explicit Fixture(unsigned source=0,double units=100):bow_arrow_tracking_fixture::Fixture(source){
        rig.units_per_metre=units;recompute();
        BOW_ARROW_CHECK(am::author_original({true,{.01,.02,.03},{0,0,0,1}},.3,.3,units,arrow));
        BOW_ARROW_CHECK(w::author_torque_bow_feed(37,.3,arrow,feed));
    }
    bool place(double depth,double lateral,double angle=0) noexcept {
        p::tracking::Sample raw{};if(!p::tracking::sample(hand,rig,arrow,bow_arrow_tracking_fixture::now,raw))return false;
        const vr::Quaternion error{0,0,std::sin(angle/2),std::cos(angle/2)};
        const auto root_q=vr::normalized(vr::multiply(vr::multiply(raw.tip_world.orientation,error),vr::inverse(feed.mount.port_in_weapon.orientation)));
        const auto root_p=am::sub(am::sub(raw.tip_world.position,am::rotate(root_q,am::mul({depth,lateral,0},rig.units_per_metre))),
            am::rotate(root_q,am::mul(feed.mount.port_in_weapon.position,rig.units_per_metre)));
        const auto mesh_q=vr::normalized(vr::multiply(root_q,vr::Quaternion{0,0,-.7071067811865475244,.7071067811865475244}));
        const auto basis=vr::rotation_matrix(mesh_q);std::array<float,16> matrix{};
        for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)matrix[i*4+j]=float(basis[j][i]*.3);
        matrix[12]=float(root_p.x);matrix[13]=float(root_p.y);matrix[14]=float(root_p.z);matrix[15]=1;
        const auto& right=rig.world_grip[1];fixed.reset();
        return fixed.bind({right.position,vr::ue_angles(vr::rotation_matrix(right.orientation))},matrix);
    }
    bool make(bool keep,p::Result& out,std::int64_t stamp=bow_arrow_tracking_fixture::now) noexcept {
        return p::make(hand,rig,arrow,{feed,fixed,11},stamp,keep,out);
    }
    void move_left_world(vr::Vector delta,std::int64_t stamp) noexcept {
        // Invert the existing XR->UE point mapping. This creates a NEW input
        // record/scene, not a renewed receipt for the previous controller pack.
        const auto& b=rig.base_view;delta=am::mul(delta,1/rig.units_per_metre);
        const vr::Vector local{b[0]*delta.x+b[4]*delta.y+b[8]*delta.z,
            b[1]*delta.x+b[5]*delta.y+b[9]*delta.z,-b[2]*delta.x-b[6]*delta.y-b[10]*delta.z};
        const auto xr=am::rotate(rig.reference.orientation,local);
        rig.controllers.hands[0].grip.position=am::add(rig.controllers.hands[0].grip.position,xr);
        ++rig.controllers.sequence;rig.stereo.sequence=rig.controllers.sequence;
        rig.controllers.sync_ns=stamp-4000000;rig.controllers.received_ns=stamp-3000000;rig.stereo.received_ns=stamp-2000000;
        ++rig.scene_invocation;++rig.publication_sequence;rig.published_ns=stamp-1000000;
        hand.key.frame=rig.scene_invocation;hand.now_ns=stamp-1000000;hand.controllers=rig.controllers;hand.stereo=rig.stereo;
        recompute();
    }
};
}
