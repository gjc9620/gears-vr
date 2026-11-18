#include "../vr-common/weapon_boomshot_guide.h"
namespace boomshot_guide {
namespace hand=gears_weapon_hand;namespace vr=gears_vr;namespace guide=gears_boomshot_guide;
namespace inv=lancerreload_inventory;namespace physical=gears_weapon::boomshot_insertion;
using Owner=boomshot_reload_owner::Owner;using Part=Owner::GuidePart;
hand::FrameKey drawn_source{};bool drawn_known{};unsigned rows{};guide::Step reported{};
#ifndef BOOMSHOT_GUIDE_TRANSPORT_BOUNDARY_FIXTURE
bool transport_current(hand::Frame const& f) noexcept {
    return weaponhand::current_gt(nullptr)&&nativeequipment::domain_ready()&&
        weaponhand::client.registration_alive(f.key.registration_cookie,f.key.native_started_qpc)&&
        hand::frame_active(f,weaponhand::clock_ns(nullptr))&&unified_weapon_fire::manual_profile()&&
        vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)&&
        nativelancerfollow::bound_key_known&&nativelancerfollow::mount.bound()&&
        hand::same(f.weapon,nativelancerfollow::bound_key.weapon)&&
        nativelancerfollow::bound_key.hold_generation==vrequip::model.hold_generation();
}
bool current(hand::Frame const& f) noexcept{return transport_current(f)&&nativeequipment::current(nullptr,f);}
#endif
bool neutral_counts(hand::Frame const& f,inv::Counts& counts) noexcept {
    return boomshot_inspection_clip::fire_neutral(f)&&inv::read(f,counts)&&counts.capacity==1&&
        counts.used>=0&&counts.used<=1&&counts.reserve>=0&&counts.reserve<=1000000;
}
bool select(hand::Frame const& f,hand::GripGameState const& game,hand::Decision const& d,
    gears_weapon::BoomshotMechanismGrant& grant,boomshot_skin_source::Source const& source,
    physical::Geometry const& geometry,guide::View& out) noexcept {
    out={};Owner::GuideState state{};
    if(!game.known||!game.main_hand_held||game.kind!=hand::WeaponKind::firearm||game.busy||game.animation_owns_hands||
       !f.left_tracked||!f.left_grip_known||!boomshot_skin::decision(f,d)||!boomshot_reload_owner::owner.guide_state(f,state))return false;
    if(f.left_grip_down){
        const auto purpose=state.part==Part::opening?gears_weapon::BoomshotContact::opening:
            state.part==Part::closing?gears_weapon::BoomshotContact::closing:gears_weapon::BoomshotContact::take_carrier;
        if(!grant.begin_allowed(f,d,purpose,weaponhand::clock_ns(nullptr))&&
           !grant.retained(f,d,purpose,weaponhand::clock_ns(nullptr)))return false;
    }else{
        if(d.lease.kind!=hand::LeaseKind::none||
           (d.status!=hand::DecisionStatus::no_candidate&&d.status!=hand::DecisionStatus::released))return false;
        for(const auto& incumbent:game.object_incumbents)if(incumbent.kind!=hand::LeaseKind::none)return false;
    }
    if(state.part==Part::opening&&!boomshot_reload_owner::owner.pending()){
        inv::Counts counts{};double progress{};
        const auto local=guide::m::mul(guide::m::rotate(vr::inverse(source.root.orientation),
            guide::m::sub(source.barrel.position,source.root.position)),1/source.root_scale);
        if(!neutral_counts(f,counts)||!gears_weapon::boomshot_barrel::progress(local,progress)||progress>.015||
           (counts.used==1&&(counts.infinite||counts.reserve)&&!physical::valid(geometry)))return false;
    }
    if(state.part==Part::shoulder){
        inv::Counts counts{};
        if(!neutral_counts(f,counts)||counts.used!=1||(!counts.infinite&&!counts.reserve)||!physical::valid(geometry))return false;
    }
    if(state.part==Part::entry){
        Owner::VisualState held{};
        if(!f.left_grip_down||!physical::valid(geometry)||!boomshot_reload_owner::owner.visual_state(f,held)||
           !boomshot_carrier_original::owner.holding(f,held.serial,held.item))return false;
    }
    switch(state.part){
    case Part::opening:out.step=guide::Step::opening;break;
    case Part::closing:out.step=guide::Step::closing;break;
    case Part::shoulder:out.step=guide::Step::shoulder;break;
    case Part::entry:out={guide::Step::entry,state.entered};break;
    default:return false;
    }
    return true;
}
}
#ifndef BOOMSHOT_GUIDE_TRANSPORT_BOUNDARY_FIXTURE
#include "weapon_boomshot_guide_transport.inl"
#endif
namespace boomshot_guide {
void tick(hand::Frame const& f,hand::GripGameState const& game,hand::Decision const& d,
    gears_weapon::BoomshotMechanismGrant& grant) noexcept {
    const auto began=weaponhand::clock_ns(nullptr);
    if((drawn_known&&hand::same(drawn_source,f.key))||!current(f))return;
    boomshot_skin_source::Source source{};physical::Geometry geometry{};guide::View view{};
    if(!boomshot_skin_source::read(f,source))return;
    boomshot_seating::read(f,geometry);
    const bool have_hint=select(f,game,d,grant,source,geometry,view);
    boomshot_endpoint::Kind endpoint_kind{};const bool have_result=boomshot_endpoint::view(f,endpoint_kind);
    if(!have_hint&&!have_result)return;
    vr::Pose shoulder{};
    if(view.step==guide::Step::shoulder){vr::Vector raw{};const vr::Pose body{f.stereo.head.position,f.reference.orientation};
        if(!gears_weapon::shoulder_ammo_center(nativeequipment::calibration,body,raw)||
           !vr::rig_tracking::pose(source.rig.base_view,source.rig.reference,{raw,body.orientation},source.units,shoulder))return;
    }
    guide::Shape shape{};
    if(have_hint&&!guide::make(view,source.root,source.barrel,source.root_scale,source.units,shoulder,geometry,shape))return;
    if(have_result&&(source.rig.world_pose_mask&gears_rig_tracking::head_pose)){const auto result=endpoint_kind==boomshot_endpoint::Kind::opened?guide::Result::opened:
        endpoint_kind==boomshot_endpoint::Kind::seated?guide::Result::seated:guide::Result::closed;
        guide::result(result,source.root,source.barrel,source.root_scale,source.units,geometry,shape,source.rig.world_head.orientation);
    }
    if(!shape.count||!current(f))return;
    const bool complete=draw(f,shape,began);
    if(log_file&&rows<24&&reported!=view.step){
        fwprintf(log_file,L"boomshot_guide frame=%llu step=%u entered=%u lines=%u complete=%u one_frame=1 cosmetic_only=1\n",
            f.key.frame,unsigned(view.step),unsigned(view.entered),shape.count,unsigned(complete));fflush(log_file);++rows;reported=view.step;
    }
}
}
