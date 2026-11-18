#include "../vr-common/weapon_boomshot_feedback.h"
#ifndef BOOMSHOT_FEEDBACK_SOUND_BOUNDARY_FIXTURE
#include "weapon_boomshot_empty_sound.inl"
#endif
// Runs on the existing normal GT after mechanics, before optional guide draw.
// A consumed physical shoulder edge may request the original empty cue once.
namespace boomshot_feedback {
namespace hand=gears_weapon_hand;namespace vr=gears_vr;namespace inv=lancerreload_inventory;
gears_boomshot_feedback::Intent intent;unsigned rows{};
void lost() noexcept {intent.lost();}
#ifndef BOOMSHOT_FEEDBACK_CURRENT_BOUNDARY_FIXTURE
bool current(hand::Frame const& f) noexcept {
    return weaponhand::current_gt(nullptr)&&unified_weapon_fire::manual_profile()&&nativeequipment::domain_ready()&&
        weaponhand::client.registration_alive(f.key.registration_cookie,f.key.native_started_qpc)&&
        hand::frame_active(f,weaponhand::clock_ns(nullptr))&&
        vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)&&
        nativelancerfollow::bound_key_known&&nativelancerfollow::mount.bound()&&
        hand::same(f.weapon,nativelancerfollow::bound_key.weapon)&&
        nativelancerfollow::bound_key.hold_generation==vrequip::model.hold_generation()&&nativeequipment::current(nullptr,f);
}
#endif
#ifndef BOOMSHOT_FEEDBACK_STATE_BOUNDARY_FIXTURE
bool active(hand::Frame const& f) noexcept {
    if(!current(f))return false;
    playground::Args state(f.weapon.address,L"GetStateName");hand::ObjectIdentity function{};
    if(!state.valid||!registered(state.r.function)||!playground::named(get<uptr>(state.r.function+0x50),L"Function")||
       state.r.size!=8||!vrequip::result_property(state,L"NameProperty",8))return false;
    const auto owner=get<uptr>(state.r.function+0x40),package=owner?get<uptr>(owner+0x40):0;
    if(!registered(owner)||!registered(package)||!playground::named(owner,L"Object")||!playground::named(package,L"Core")||
       get<uptr>(package+0x40)||!(get<unsigned>(state.r.function+0xf4)&0x400)||
       !weaponhand::executable(reinterpret_cast<FARPROC>(get<uptr>(state.r.function+0x114)),GetModuleHandleW(nullptr))||
       !nativeequipment::identify(nullptr,state.r.function,function)||!current(f)||!state.run())return false;
    const auto value=state.output<Name>();namespace meta=gripdamage_metadata;
    meta::Table objects{},names{};meta::NameText label{};hand::ObjectIdentity closing{};
    return !value.number&&meta::tables(base,objects,names)&&meta::fresh_name(names,value.index,label)&&
        meta::exact_text(label,L"Active")&&nativeequipment::identify(nullptr,state.r.function,closing)&&
        hand::same(function,closing)&&current(f);
}
#endif
bool shoulder(hand::Frame const& f) noexcept {
    return boomshot_reload_owner::Owner::shoulder(f)&&gears_weapon::body_slot(nativeequipment::calibration,
        {f.stereo.head.position,f.reference.orientation},f.controllers.hands[0].last_squeeze_press_grip.position,0)==gears_weapon::BodySlot::shoulder_ammo;
}
bool quiet(hand::Frame const& f,inv::Counts& out) noexcept {
    out={};bool reloading{};unsigned char mode{};
    return current(f)&&!boomshot_reload_owner::owner.pending()&&!boomshot_sessions::blocks_fire(f)&&
        !boomshot_carrier_original::owner.active()&&!boomshot_clip::serial&&!boomshot_inspection_clip::owner.active()&&
        !lancerreload_state::lease.acquired&&boomshot_inspection_clip::fire_neutral(f)&&active(f)&&
        inv::read(f,out)&&out.capacity==1&&out.used>=0&&out.used<=1&&!out.reserve&&!out.infinite&&
        lancerreload_state::bool_call(f.weapon.address,L"IsReloading",reloading)&&!reloading&&
        lancerreload_state::byte_value(f.weapon.address,L"CurrentFireMode",mode)&&mode==0&&current(f);
}
void sample(hand::Frame const& f,hand::GripGameState const& game,hand::Decision const& d) noexcept {
    if(!weaponhand::current_gt(nullptr)||!current(f)){lost();return;}
    bool eligible=game.known&&game.main_hand_held&&game.kind==hand::WeaponKind::firearm&&!game.busy&&!game.animation_owns_hands&&
        boomshot_skin::decision(f,d)&&!d.new_grant&&d.status==hand::DecisionStatus::no_candidate&&d.lease.kind==hand::LeaseKind::none&&
        !game.request_magazine&&!game.request_ammo&&!game.magazine_observation_pending&&
        !boomshot_reload_owner::owner.pending()&&!boomshot_sessions::blocks_fire(f);
    for(const auto& incumbent:game.object_incumbents)eligible=eligible&&incumbent.kind==hand::LeaseKind::none;
    eligible=eligible&&shoulder(f);
    // All current physical edges (including outside/nonempty/refused ones) are
    // consumed before any original getters, reflection or effect dispatch.
    if(!intent.advance(f,vrequip::model.hold_generation(),weaponhand::clock_ns(nullptr),eligible))return;
    inv::Counts before{},closing_counts{},after{};boomshot_empty_sound::Entry first{},closing{};
    if(!quiet(f,before)||!boomshot_empty_sound::read(f,first)||!quiet(f,closing_counts)||!(before==closing_counts)||
       !boomshot_empty_sound::read(f,closing)||!boomshot_empty_sound::same(first,closing)||!current(f))return;
    Request call{};call.object=f.weapon.address;call.function=first.function.address;call.size=0;
    const bool returned=invoke(&call);
    const bool verified=returned&&quiet(f,after)&&before==after;
    if(log_file&&rows<16){++rows;
        fwprintf(log_file,L"boomshot_no_ammo press=%llu frame=%llu sequence=%llu original_sync_ns=%lld returned=%u unchanged_counts_verified=%u used=%d reserve=%d cue=%llx no_fire_reload=1 no_retry=1\n",
            f.press_token,f.key.frame,f.controllers.sequence,f.controllers.sync_ns,unsigned(returned),unsigned(verified),
            before.used,before.reserve,first.cue.address);fflush(log_file);
    }
}
}
