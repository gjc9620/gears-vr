// Ordinary GT publication after a real part lease. The destination is the
// existing Native C0 skin solver, never a controller/input transform.
#include "../vr-common/weapon_lancer_grasp_pose.h"
namespace lancer_grasp {
namespace api=gears_hand_object_grasp;namespace hand=gears_weapon_hand;
using P=gears_vr::Pose;using Q=gears_vr::Quaternion;using V=gears_vr::Vector;
std::uint64_t magazine_token{};hand::ObjectIdentity magazine_weapon{};
hand::FrameKey magazine_key{};std::uint64_t magazine_reference{},magazine_session{},magazine_space{},
    magazine_input{},magazine_profile{},magazine_hold{},magazine_stream{},magazine_tracking{};
bool magazine_known{};
gears_weapon_lancer_grasp::PartFit part_fit;
hand::Frame part_source{};api::Kind part_kind{};
unsigned part_pose_gate{};
hand::ObjectIdentity part_asset{},part_component{};
std::array<std::uint64_t,4> part_epochs{};
bool same_magazine_capture(hand::Frame const& f,gears_committed_interaction::Frame const& actual) noexcept {
    return magazine_known&&magazine_token==f.press_token&&hand::same(magazine_weapon,f.weapon)&&
        magazine_key.registration_cookie==f.key.registration_cookie&&magazine_key.lifecycle==f.key.lifecycle&&
        magazine_key.game_start==f.key.game_start&&magazine_key.native_started_qpc==f.key.native_started_qpc&&
        magazine_key.world_generation==f.key.world_generation&&magazine_reference==f.reference_generation&&
        magazine_session==f.stereo.session&&magazine_space==f.stereo.space_epoch&&
        magazine_input==f.controllers.input_epoch&&magazine_profile==f.controllers.profile_generation&&
        magazine_hold==actual.right.hold_generation&&magazine_stream==actual.right.producer.stream&&
        magazine_tracking==actual.right.producer.tracking[0];
}
bool grip_for_wrist(P const& wrist,double units,double model_scale,P& grip) noexcept {
    gears_hand_authored_mount::Mount m{};
    if(!gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
       gears_hand_authored_mount::Side::left,model_scale,units,m))return false;
    grip.orientation=gears_vr::normalized(gears_vr::multiply(wrist.orientation,gears_vr::inverse(m.calibration.grip_to_wrist_rotation)));
    const auto d=gears_vr::transform(gears_vr::rotation_matrix(grip.orientation),m.calibration.grip_to_wrist_metres);
    grip.position={wrist.position.x-d.x*units,wrist.position.y-d.y*units,wrist.position.z-d.z*units};
    return gears_vr::stereo_pose_valid(grip);
}
bool part_pose(hand::Frame const& f,api::Kind kind,P& grip) noexcept {
    part_pose_gate=1;
    lancersaw_visual::Observation actual_part{};
    if(kind==api::Kind::starter){if(!lancersaw_visual::observe(f,actual_part))return false;}
    else{
        vrequip::MeshPoseProbe probe{};
        if(!vrequip::mesh_pose_probe(f.weapon.address,probe)||
           probe.mesh!=lancer_charging::geometry.component.address||probe.asset!=lancer_charging::geometry.asset.address)return false;
        std::memcpy(actual_part.gun.data(),probe.local_to_world,sizeof(probe.local_to_world));
    }
    part_pose_gate=2;P gun{};double scale{};if(!hammerburst_runtime::matrix(actual_part.gun,gun,scale))return false;
    V contact{};Q delta{};
    if(kind==api::Kind::starter){
        contact=actual_part.grasp;delta={0,0,std::sin(actual_part.angle*.5),std::cos(actual_part.angle*.5)};
    }else{
        if(!lancer_charging::current_geometry(f))return false;
        const auto& g=lancer_charging::geometry.slide;const double distance=lancer_charging::last_observed;
        if(!std::isfinite(distance)||distance<0||distance>g.travel+.001)return false;
        const double units=nativelancerfollow::bound_units;
        const auto p=gears_weapon::lancer_front::contact_raw;
        contact=gears_weapon::lancer_guide::point(actual_part.gun,
            {p.x+g.axis.x*distance*units,p.y+g.axis.y*distance*units,p.z+g.axis.z*distance*units});
    }
    gears_committed_interaction::Frame hand{};
    part_pose_gate=3;
    if(!lancerreload_visual::interaction(f,hand))return false;
    part_pose_gate=4;
    P physical_wrist{};if(!lancerreload_visual::physical_wrist(hand,physical_wrist))return false;
    const std::array<std::uint64_t,4> epochs{{hand.right.hold_generation,hand.right.producer.stream,
        hand.right.producer.tracking[0],hand.right.producer.tracking[1]}};
    const auto& old=part_source;
    const bool same=part_fit.known&&part_kind==kind&&epochs==part_epochs&&
        old.key.game_pid==f.key.game_pid&&old.key.game_start==f.key.game_start&&
        old.key.native_started_qpc==f.key.native_started_qpc&&old.key.registration_cookie==f.key.registration_cookie&&
        old.key.lifecycle==f.key.lifecycle&&old.key.world_generation==f.key.world_generation&&
        old.press_token==f.press_token&&old.interaction_control==f.interaction_control&&
        hand::same(old.world,f.world)&&hand::same(old.controller,f.controller)&&hand::same(old.pawn,f.pawn)&&
        hand::same(old.mesh,f.mesh)&&hand::same(old.weapon,f.weapon)&&old.reference_generation==f.reference_generation&&
        old.stereo.session==f.stereo.session&&old.stereo.space_epoch==f.stereo.space_epoch&&
        old.controllers.input_epoch==f.controllers.input_epoch&&old.controllers.profile_generation==f.controllers.profile_generation&&
        hand::same(part_asset,nativelancerfollow::bound_key.asset)&&hand::same(part_component,nativelancerfollow::bound_key.component);
    part_pose_gate=5;
    if(!part_fit.solve(!same,kind,gun.orientation,delta,contact,physical_wrist,
        hand.model_scale,hand.right.units_per_metre,grip))return false;
    part_source=f;part_kind=kind;part_epochs=epochs;
    part_asset=nativelancerfollow::bound_key.asset;part_component=nativelancerfollow::bound_key.component;
    return true;
}
void report_grasp_attempt(hand::Frame const& f,api::Kind kind,bool published,unsigned gate,unsigned part_gate) noexcept {
    static unsigned previous{~0u},rows{};
    const unsigned state=unsigned(kind)*4096+gate*64+part_gate;
    if(log_file&&rows<16&&previous!=state){fwprintf(log_file,
        L"lancer_context_grasp kind=%u published=%u frame=%llu visual_only=1 gate=%u part_gate=%u age_ns=%lld\n",
        unsigned(kind),unsigned(published),f.key.frame,gate,part_gate,
        weaponhand::clock_ns(nullptr)-f.controllers.sync_ns);fflush(log_file);++rows;}previous=state;
}
void tick(hand::Frame const& f,hand::Decision const& decision) noexcept try {
    struct Attempt {
        hand::Frame const& frame;api::Kind kind{};bool published{};unsigned gate{1},part_gate{};
        ~Attempt(){report_grasp_attempt(frame,kind,published,gate,part_gate);}
    } attempt{f};
    if(!f.left_tracked||!f.left_grip_known||!f.left_grip_down||decision.release_required||
       (decision.status!=hand::DecisionStatus::granted&&decision.status!=hand::DecisionStatus::retained)||
       !hand::lease_valid(decision.lease,f.key.lifecycle)||decision.lease.id!=f.press_token||
       (decision.lease.kind!=hand::LeaseKind::magazine&&decision.lease.kind!=hand::LeaseKind::ammo&&
        decision.lease.kind!=hand::LeaseKind::support)){
        magazine_known=false;part_fit.reset();return;
    }
    if(decision.lease.kind==hand::LeaseKind::support){
        attempt.kind=api::Kind::support;attempt.gate=2;
        magazine_known=false;part_fit.reset();P geometry{};
        if(!nativelancerfollow::support_geometry(f,geometry))return;
        const auto call=lancersaw_visual::function<api::Publish>(api::publish_export);
        attempt.gate=3;if(!call||!nativeequipment::current(nullptr,f))return;
        api::Request q{};q.source=f;q.kind=api::Kind::support;q.grip=geometry;
        attempt.published=call(&q);attempt.gate=attempt.published?0u:33u;return;
    }
    api::Kind kind{};
    if(lancersaw_starter::same_hold(f))kind=api::Kind::starter;
    else if(lancer_charging::current_geometry(f)&&lancer_charging::required(f)&&
        lancer_charging::pending_identity==lancerreload_inventory::identity(f)&&
        hand::same(lancer_charging::held,decision.lease))kind=api::Kind::loading;
    else kind=api::Kind::magazine;
    attempt.kind=kind;attempt.gate=10;
    P goal{};
    if(kind==api::Kind::magazine){
        part_fit.reset();
        gears_committed_interaction::Frame actual{};if(!lancerreload_visual::interaction(f,actual))return;
        const auto& physical=actual.physical_left_grip;
        // Re-evaluate the actual item's Grip Point on every fresh frame.
        // Insertion assistance and the seated extraction stop can move the
        // visible magazine relative to the raw controller. A cached offset
        // on the controller would leave its hand behind during that motion.
        const auto component=playground::pointer(f.weapon.address,L"MagazineMesh");
        std::array<float,16> matrix{},after{};P magazine{};double size{};gears_vr::Viewpoint wrist{};Q wrist_q{};
        if(!playground::instance(component,L"StaticMeshComponent")||
           !copy(matrix.data(),component+0xa0,sizeof matrix)||!hammerburst_runtime::matrix(matrix,magazine,size)||
           !gears_weapon::lancer_left_for_magazine({magazine.position,gears_vr::ue_angles(gears_vr::rotation_matrix(magazine.orientation))},wrist,actual.model_scale)||
           !copy(after.data(),component+0xa0,sizeof after)||matrix!=after||
           playground::pointer(f.weapon.address,L"MagazineMesh")!=component||
           !gears_vr::rig_tracking::detail::quaternion(gears_vr::ue_matrix(wrist.rotation),wrist_q)||
           !grip_for_wrist({wrist.position,wrist_q},actual.right.units_per_metre,actual.model_scale,goal))return;
        if(!same_magazine_capture(f,actual)){
            magazine_known=false;
            // A newly granted ammo fetch can precede creation/attachment of
            // the replacement magazine. Do not permanently cache the distant
            // old component's pose. Retry only on a later fresh actual pair.
            gears_hand_authored_mount::Mount model{};P bounded_visual{};
            if(!gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
               gears_hand_authored_mount::Side::left,actual.model_scale,actual.right.units_per_metre,model)||
               !api::apply({kind,goal,1},physical,model.calibration,actual.right.units_per_metre,bounded_visual))return;
            magazine_token=f.press_token;magazine_weapon=f.weapon;magazine_known=true;
            magazine_key=f.key;magazine_reference=f.reference_generation;magazine_session=f.stereo.session;
            magazine_space=f.stereo.space_epoch;magazine_input=f.controllers.input_epoch;
            magazine_profile=f.controllers.profile_generation;magazine_hold=actual.right.hold_generation;
            magazine_stream=actual.right.producer.stream;magazine_tracking=actual.right.producer.tracking[0];
        }
    }else if(!part_pose(f,kind,goal)){attempt.gate=20;attempt.part_gate=part_pose_gate;return;}
    const auto call=lancersaw_visual::function<api::Publish>(api::publish_export);
    attempt.gate=3;if(!call||!nativeequipment::current(nullptr,f))return;
    api::Request q{};q.source=f;q.kind=kind;q.grip=goal;const bool published=call(&q);
    attempt.published=published;attempt.gate=published?0u:33u;
}catch(...){}
}
