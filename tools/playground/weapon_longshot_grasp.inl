// Longshot chooses part geometry/preset; the optional generic V3/V4 publication
// uses the existing Native Broker and sole C0. No gameplay action is supplied.
#include "../vr-common/hand_authored_grasp.h"
#include "../vr-common/weapon_longshot_round_grasp_profile.h"
#include "../vr-common/weapon_longshot_bolt_grasp_profile.h"
namespace longshot_grasp {
namespace api=gears_hand_part_grasp;namespace hand=gears_weapon_hand;
namespace runtime=longshot_runtime;namespace geometry=gears_weapon_longshot_grasp;
using P=gears_vr::Pose;using V=gears_vr::Vector;
hand::Frame nearby_source{};std::uint64_t nearby_hold{};bool nearby{};
bool same_nearby(hand::Frame const& f) noexcept {
    const auto& old=nearby_source;
    return nearby&&nearby_hold==nativelancerfollow::bound_key.hold_generation&&
        old.controllers.sync_ns>0&&f.controllers.sync_ns>=old.controllers.sync_ns&&
        f.controllers.sync_ns-old.controllers.sync_ns<=100000000&&
        old.key.game_pid==f.key.game_pid&&old.key.thread_id==f.key.thread_id&&old.key.game_start==f.key.game_start&&
        old.key.native_started_qpc==f.key.native_started_qpc&&old.key.registration_cookie==f.key.registration_cookie&&
        old.key.lifecycle==f.key.lifecycle&&old.key.world_generation==f.key.world_generation&&
        hand::same(old.world,f.world)&&hand::same(old.controller,f.controller)&&hand::same(old.pawn,f.pawn)&&
        hand::same(old.mesh,f.mesh)&&hand::same(old.weapon,f.weapon)&&old.reference_generation==f.reference_generation&&
        old.stereo.session==f.stereo.session&&old.stereo.space_epoch==f.stereo.space_epoch&&
        old.controllers.input_epoch==f.controllers.input_epoch&&old.controllers.profile_generation==f.controllers.profile_generation;
}
bool tick(hand::Frame const& f,hand::GripGameState const& game,hand::Decision const& decision) noexcept {
    const auto& plan=nativelancerfollow::plan;
    if(!runtime::selected(f)||!game.known||!game.main_hand_held||!plan.valid||
       !(plan.key==nativelancerfollow::bound_key)||!hand::same_frame(plan.source,f)||!nativeequipment::current(nullptr,f)||
       !f.left_tracked||!f.left_grip_known||!hand::same(decision.key,f.key)){
        nearby=false;return false;
    }
    const auto mask=gears_rig_tracking::left_grip_pose|gears_rig_tracking::right_grip_pose;
    const double units=plan.prepared.units_per_metre;
    if((plan.prepared.world_pose_mask&mask)!=mask||!std::isfinite(units)||units<10||units>500)return false;
    api::Request q{};q.pose.source=f;q.pose.kind=gears_hand_object_grasp::Kind::magazine;
    const auto& physical=plan.prepared.world_grip[0];
    if(!gears_vr::stereo_pose_valid(physical)||!gears_vr::stereo_pose_valid(plan.prepared.world_grip[1]))return false;
    if(runtime::stage==runtime::Stage::holding_round){
        if(!longshot_round::item_serial||!longshot_round::current(f)||
           !longshot_round::granted(f,decision)||!hand::same(longshot_round::held_lease,decision.lease))return false;
        // This mount/profile and displayed cartridge share ONE result. The
        // insertion owner separately measures its untouched physical tip.
        P round_grip{};
        q.preset=api::Preset::precision;
        q.pose.grip=longshot_round::presentation_grip(f,decision,round_grip)?round_grip:physical;
        if(longshot_round::authored_pinch){
            namespace authored=gears_hand_authored_grasp;
            namespace pinch=gears_weapon_longshot_round_grasp;
            authored::Request request{};request.source=f;
            request.geometry_signature=gears_hand_authored_mount::marcus115_signature;request.profile_id=pinch::profile_id;
            request.side=authored::Side::left;request.mode=authored::Mode::left_part;request.kind=q.pose.kind;
            request.grip=q.pose.grip;request.reference=pinch::reference;request.closed=pinch::closed;
            const auto publish=lancersaw_visual::function<authored::Publish>(authored::publish_export);
            // This item latches ONE mount/skin pair. Losing V4 cannot silently
            // publish the old V3 fingers around its differently oriented case.
            return publish&&nativeequipment::current(nullptr,f)&&publish(&request);
        }
    }else{
        const auto stage=runtime::stage;
        if(stage!=runtime::Stage::idle&&stage!=runtime::Stage::acquiring&&stage!=runtime::Stage::opening&&
           stage!=runtime::Stage::waiting_close&&stage!=runtime::Stage::closing)return false;
        if(!runtime::proposal_known||!gears_vr::stereo_pose_valid(runtime::root_in_right)||
           !(runtime::proposal_identity==lancerreload_inventory::identity(f))||
           runtime::proposal_reference!=f.reference_generation||runtime::contact_frame!=f.key.frame||
           runtime::contact_sync_ns!=f.controllers.sync_ns)return false;
        // Bind_contact already read these current bone/root values. Rebuild
        // them from the same unsteered right grip; do not read another rig or
        // game matrix, nor use the foregrip's aimed/steered right endpoint.
        const auto& right=plan.prepared.world_grip[1];
        const P root{longshot_bolt::point(right,runtime::root_in_right.position),
            gears_vr::normalized(gears_vr::multiply(right.orientation,runtime::root_in_right.orientation))};
        const double scale=runtime::root_scale;
        if(!std::isfinite(scale)||scale<=.1||scale>=10)return false;
        const auto contact=longshot_bolt::point(root,runtime::knob_local,scale*units);
        const auto pivot=longshot_bolt::point(root,runtime::bolt_pivot_local,scale*units);
        const double distance=std::hypot(std::hypot(physical.position.x-contact.x,
            physical.position.y-contact.y),physical.position.z-contact.z)/units;
        const bool active=longshot_bolt::granted(f,decision);
        if(active){if(!std::isfinite(distance)||distance>.07){nearby=false;return false;}}
        else{
            const auto& input=f.controllers.hands[0];
            if(game.busy||game.animation_owns_hands||decision.lease.kind!=hand::LeaseKind::none||
               input.squeeze_active!=1||input.trigger_active!=1||
               hand::control_held(input,gears_vr::control_squeeze)||hand::control_held(input,gears_vr::control_trigger)||
               !std::isfinite(distance)||distance>(same_nearby(f)?.07:.05)){
                nearby=false;return false;
            }
            q.pose.proximity=1;
        }
        // The actual knob-head centre and current pivot supply the stem.
        // The pad table and wrist mount are ONE fitted original-surface pair;
        // keep the receiver clear instead of rolling an old cylinder fist.
        auto axis=geometry::sub(contact,pivot);
        const auto forward=gears_vr::transform(gears_vr::rotation_matrix(root.orientation),{1,0,0});
        axis=geometry::sub(axis,geometry::scale(forward,geometry::dot(axis,forward)));
        gears_hand_authored_mount::Mount anatomy{};
        if(!gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
            gears_hand_authored_mount::Side::left,1,units,anatomy)||
           !gears_weapon_longshot_bolt_grasp::grip(contact,forward,axis,anatomy.calibration.grip_to_wrist_metres,
              anatomy.calibration.grip_to_wrist_rotation,units,scale,q.pose.grip))return false;
        namespace authored=gears_hand_authored_grasp;
        namespace fitted=gears_weapon_longshot_bolt_grasp;
        authored::Request request{};request.source=f;
        request.geometry_signature=gears_hand_authored_mount::marcus115_signature;request.profile_id=fitted::profile_id;
        request.side=authored::Side::left;request.mode=authored::Mode::left_part;request.kind=q.pose.kind;
        request.grip=q.pose.grip;request.proximity=q.pose.proximity;
        request.reference=fitted::reference;request.closed=fitted::closed;
        const auto publish=lancersaw_visual::function<authored::Publish>(authored::publish_export);
        const bool accepted=publish&&nativeequipment::current(nullptr,f)&&publish(&request);
        if(accepted){nearby=true;nearby_source=f;nearby_hold=nativelancerfollow::bound_key.hold_generation;}
        return accepted;
    }
    const auto publish=lancersaw_visual::function<api::Publish>(api::publish_export);
    const bool accepted=publish&&nativeequipment::current(nullptr,f)&&publish(&q);
    return accepted;
}
}
