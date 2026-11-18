#include "../vr-common/weapon_boomshot_passive_grasp.h"
#ifndef BOOMSHOT_SKIN_SOURCE_BOUNDARY_FIXTURE
#include "weapon_boomshot_skin_source.inl"
#endif
// Cosmetic selection/publication only. The original action owner and Native
// Broker independently retain all grants, counts, input and fixed-arm limits.
namespace boomshot_skin {
namespace hand=gears_weapon_hand;namespace vr=gears_vr;namespace authored=gears_boomshot_grasp_pose;
namespace source=boomshot_skin_source;namespace mount=gears_boomshot_hand_mount;
using Part=authored::Part;using VisualPart=boomshot_reload_owner::Owner::VisualPart;
namespace passive=gears_boomshot_passive_grasp;
passive::Selector passive_selection;
passive::Scope passive_scope(hand::Frame const& f,source::Source const& current)noexcept{
    passive::Scope value{};value.key=f.key;value.objects={f.world,f.controller,f.pawn,f.mesh,f.weapon,current.pose.asset};
    const auto& r=current.rig;
    value.boundaries={nativelancerfollow::bound_key.hold_generation,f.reference_generation,r.binding_generation,
        r.native_reference.epoch,r.reference_generation,r.producer.stream,r.producer.tracking[0],r.producer.tracking[1],
        r.observed.stream,r.observed.reference,r.observed.head_tracking,r.observed.hand_tracking[0],r.observed.hand_tracking[1],
        f.stereo.session,f.stereo.space_epoch,f.controllers.input_epoch,f.controllers.profile_generation};
    value.units=current.units;value.root_scale=current.root_scale;value.hand_scale=current.hand_scale;
    value.source=unsigned(f.stereo.source);value.control=f.interaction_control;return value;
}
bool support_geometry(hand::Frame const& f,vr::Pose& out)noexcept{return source::support_relative(f,out);}
bool fill_support_mount(hand::Frame const& f,hand::GripGameState& game)noexcept{
    if(!game.known||!game.main_hand_held||game.busy||game.animation_owns_hands||game.kind!=hand::WeaponKind::firearm||
       !f.left_tracked||!f.left_grip_known||
       hand::index_pinch(f))return false;
    // Publish the settled geometry before the press, so the ordinary broker
    // can select this socket on the player's first Grip edge.
    vr::Pose relative{};
    if(!source::support_relative(f,relative))return false;
    game.mount={};game.support_box={};
    game.mount.support_grip_in_weapon={relative.position.y,relative.position.z,-relative.position.x};
    game.mount.support_length_tolerance_metres=.22;game.mount.maximum_support_correction_radians=.8;
    game.acquire_metres=.12;game.release_metres=.20;game.mount_asset=nativelancerfollow::bound_key.asset;
    game.mount_revision=nativelancerfollow::bound_key.hold_generation;game.mount_known=game.mount.valid();return game.mount_known;
}
bool decision(hand::Frame const& f,hand::Decision const& d)noexcept{
    const auto observed_now=weaponhand::clock_ns(nullptr);
    return d.size==sizeof d&&d.abi_version==hand::version&&!d.reserved&&d.new_grant<=1&&!d.release_required&&
        d.proposal_nonce&&hand::same(f.key,d.key)&&d.press_token==f.press_token&&hand::lease_valid(d.lease,f.key.lifecycle)&&
        d.now_ns>=f.now_ns&&d.now_ns<=observed_now&&observed_now-d.now_ns<=100000000;
}
bool publish_left(hand::Frame const& f,hand::Decision const& d,gears_weapon::BoomshotMechanismGrant& grant)noexcept{
    const auto withdrawn=[]()noexcept{passive_selection.lost();return false;};
    if(!weaponhand::current_gt(nullptr)||!decision(f,d)||!f.left_tracked||!f.left_grip_known)return withdrawn();
    // Held carrier fingers are selected by the SAME current Native broker/C0 writer; no historical world wrist goal is published.
    if(f.left_grip_down&&boomshot_carrier_original::owner.active())return true;
    source::Source current{};if(!source::read(f,current))return withdrawn();
    if(f.left_grip_down){
        passive_selection.lost();
        boomshot_reload_owner::Owner::VisualState state{};vr::Pose target{};Part part{};
        if(boomshot_reload_owner::owner.visual_state(f,state)){
            const bool carrier=state.part==VisualPart::carrier||state.part==VisualPart::carrier_seating;
            const auto purpose=carrier?(boomshot_reload_owner::owner.removing_old_carrier()?gears_weapon::BoomshotContact::opening:gears_weapon::BoomshotContact::take_carrier):
                state.part==VisualPart::closing?gears_weapon::BoomshotContact::closing:gears_weapon::BoomshotContact::opening;
            if(!grant.begin_allowed(f,d,purpose,weaponhand::clock_ns(nullptr))&&
               !grant.retained(f,d,purpose,weaponhand::clock_ns(nullptr)))return false;
            if(carrier){
                vr::Pose actual{};double scale{};
                if(!boomshot_carrier_original::owner.visual_pose(f,state.serial,state.item,actual,scale)||
                   !vr::stereo_pose_valid(actual)||!std::isfinite(scale)||scale<.01||scale>10)return false;
                // M0 currently follows the ORIGINAL LeftHand socket. Preserve
                // the RAW controller target and author fingers only; the
                // captured ammo wrist relation must be wired together with
                // physical sample_raw() in the next carrier step. Feeding the
                // fitted component pose back here would duplicate the old
                // socket rotation and create hand/attachment feedback.
                target=current.rig.world_grip[0];
                part=state.part==VisualPart::carrier_seating?Part::carrier_seating:Part::carrier;
            }else{
                const bool closing=state.part==VisualPart::closing;
                if(!source::mechanism_world(current,closing,target))return false;
                part=closing?Part::mechanism_close:Part::mechanism_open;
            }
        }else{
            if(boomshot_reload_owner::owner.blocks_fire(f)||hand::index_pinch(f)||d.lease.kind!=hand::LeaseKind::support||
               d.lease.id!=f.press_token||(d.status!=hand::DecisionStatus::granted&&d.status!=hand::DecisionStatus::retained)||
               !source::relative_support(current,target))return false;
            part=Part::support;
        }
        return boomshot_grasp::publish_marcus115(f,part,target,false);
    }
    if(d.lease.kind!=hand::LeaseKind::none||f.controllers.hands[0].squeeze_active!=1||
       hand::control_held(f.controllers.hands[0],vr::control_squeeze))return withdrawn();
    const bool debt=boomshot_reload_owner::owner.blocks_fire(f);
    const auto purpose=debt?boomshot_reload_owner::owner.contact(f):gears_weapon::BoomshotContact::opening;
    vr::Pose mechanism{},support{};bool have_mechanism=false,have_support=false;
    if(purpose==gears_weapon::BoomshotContact::opening||purpose==gears_weapon::BoomshotContact::closing)
        have_mechanism=source::mechanism_world(current,purpose==gears_weapon::BoomshotContact::closing,mechanism);
    if(!debt)have_support=source::support_world(current,support);
    const auto distance=[&](vr::Pose const& target)noexcept{
        const auto delta=mount::sub(target.position,current.rig.world_grip[0].position);return std::sqrt(mount::dot(delta,delta))/current.units;
    };
    const auto md=have_mechanism?distance(mechanism):1e9,sd=have_support?distance(support):1e9;
    const auto selected=passive_selection.advance(passive_scope(f,current),f.now_ns,
        !have_mechanism?passive::Part::none:purpose==gears_weapon::BoomshotContact::closing?passive::Part::closing:passive::Part::opening,md,sd);
    if(selected==passive::Part::none)return false;
    const bool accepted=selected==passive::Part::support?
        boomshot_grasp::publish_marcus115(f,Part::support,support,true):
        boomshot_grasp::publish_marcus115(f,selected==passive::Part::closing?Part::mechanism_close:Part::mechanism_open,mechanism,true);
    if(!accepted)passive_selection.lost();return accepted;
}
}
