// Bow's original R2 journal. The existing shared R2 dispatcher is its sole
// caller. This module owns no raw controller producer, hook or pose writer.
// Every original call is journalled BEFORE dispatch; soft loss may only stop
// the old journal through its cleanup anchor, never create or release a shot.
namespace torquebow_input_owner {
namespace hand=gears_weapon_hand;namespace original_call=torquebow_buttons;
using Result=gears_weapon::TorqueBowCallResult;using Route=gears_weapon::TorqueBowCancelRoute;
enum class Phase:unsigned {idle,reserved,press_inflight,down,release_inflight,await_neutral,uncertain,cleaning};
struct Journal {
    hand::Frame source{};torquebow_cleanup::Anchor cleanup{};
    original_call::Entry press{},release{};lancerreload_inventory::Counts before{};
    std::uint64_t serial{},hold{},accounted_spawn_call{};Phase phase{};
    bool may_have_called{},injection_fenced{},cleanup_complete{};
};
Journal journal;std::uint64_t next_serial{1};std::atomic<bool> outstanding{};
bool owns(hand::Frame const& captured,std::uint64_t serial) noexcept {
    return journal.serial&&serial==journal.serial&&journal.phase!=Phase::idle&&
        hand::same_frame(captured,journal.source);
}
bool same_admission(hand::Frame const& f) noexcept {
    const auto& a=journal.source;
    return journal.serial&&unified_weapon_fire::same_context(a.key,f.key)&&
        hand::same(a.world,f.world)&&hand::same(a.controller,f.controller)&&hand::same(a.pawn,f.pawn)&&
        hand::same(a.mesh,f.mesh)&&hand::same(a.weapon,f.weapon)&&a.reference_generation==f.reference_generation&&
        gears_vr::stereo_same_pose(a.reference,f.reference)&&a.controllers.session==f.controllers.session&&
        a.stereo.source==f.stereo.source&&a.controllers.source==f.controllers.source&&
        a.controllers.space_epoch==f.controllers.space_epoch&&a.controllers.profile_generation==f.controllers.profile_generation&&
        a.controllers.input_epoch==f.controllers.input_epoch&&f.controllers.sequence>=a.controllers.sequence&&
        f.controllers.sync_ns>=a.controllers.sync_ns;
}
bool same_hold(hand::Frame const& f) noexcept {
    return same_admission(f)&&journal.hold==vrequip::model.hold_generation();
}
bool button_owns(hand::Frame const& f,std::uint64_t serial) noexcept {
    return serial&&serial==journal.serial&&!journal.injection_fenced&&same_hold(f)&&
        (journal.phase==Phase::press_inflight||journal.phase==Phase::release_inflight);
}
std::uint64_t projectile_owner(hand::Frame const& f) noexcept {
    // Admission for the existing projectile-return consumer, never permission
    // to invoke ProjectileFire. A prepared ray alone cannot adopt an external
    // Bow shot. The in-flight phase is journalled before original ButtonPress.
    if(!journal.may_have_called||journal.injection_fenced||!same_hold(f))return 0;
    return journal.phase==Phase::press_inflight||journal.phase==Phase::down||
        journal.phase==Phase::release_inflight||journal.phase==Phase::await_neutral?journal.serial:0;
}
bool muzzle_ready(hand::Frame const& f) noexcept {return torquebow_fire::ready_for(f,20000000);}
bool stop_local(std::uint64_t serial) noexcept {
    if(!serial||serial!=journal.serial)return false;
    journal.injection_fenced=true;torquebow_fire::clear_source();return true;
}
bool read(torquebow_state::Observed& out) noexcept {
    return journal.serial&&torquebow_state::read_owned(journal.source,out,journal.cleanup,journal.serial,owns);
}
bool retire_neutral() noexcept {
    if(!journal.serial||torquebow_charge::cancel_owner.pending())return false;
    if(journal.phase==Phase::cleaning&&!journal.cleanup_complete)return false;
    if(journal.may_have_called){torquebow_state::Observed actual{};if(!read(actual)||!actual.stopped())return false;}
    torquebow_fire::SpawnReceipt unaccounted{};
    if(torquebow_fire::peek_spawn(unaccounted))return false;
    const auto serial=journal.serial;
    auto& owner=torquebow_charge::cancel_owner;
    if(owner.phase()==gears_weapon::TorqueBowCancelPhase::complete&&!owner.retire_completed(serial))return false;
    if(!journal.cleanup.retire(serial,true))return false;
    journal={};outstanding.store(false,std::memory_order_release);return true;
}
bool reserve_before_press(hand::Frame const& f) noexcept {
    if(journal.serial||next_serial==UINT64_MAX||!nativeequipment::current(nullptr,f)||
       !torquebow_input_fence::available()||!muzzle_ready(f))return false;
    torquebow_fire::SpawnReceipt pending_spawn{};
    if(torquebow_fire::peek_spawn(pending_spawn))return false; // Previous real spawn needs accounting first.
    Journal next{};torquebow_state::Observed actual{};Name active{};bool probe{};
    if(!torquebow_state::read(f,actual)||!actual.active||!actual.stopped()||actual.inventory.capacity!=1||actual.inventory.used!=0||actual.inventory.infinite||
       !original_call::bind(f.controller,true,next.press)||!original_call::bind(f.controller,false,next.release)||
       !torquebow_cancel_live::active_name(f.weapon.address,active)||
       !torquebow_state::tick_enabled(f.weapon.address,probe)||
       !nativeequipment::current(nullptr,f))return false;
    next.source=f;next.serial=next_serial++;next.hold=vrequip::model.hold_generation();next.before=actual.inventory;
    next.phase=Phase::reserved;journal=next;
    if(!journal.cleanup.capture_before_press(f,journal.serial,owns)){journal={};return false;}
    outstanding.store(true,std::memory_order_release);return true;
}
Result press(hand::Frame const& f) noexcept {
    if(journal.phase!=Phase::reserved||!hand::same_frame(f,journal.source)||journal.injection_fenced)return Result::rejected;
    journal.phase=Phase::press_inflight;journal.may_have_called=true;
    const auto result=original_call::dispatch(f,journal.press,journal.serial,button_owns,muzzle_ready);
    if(result==Result::rejected){journal.phase=Phase::reserved;journal.may_have_called=false;}
    else journal.phase=result==Result::accepted?Phase::down:Phase::uncertain;
    return result;
}
Result normal_release(hand::Frame const& f) noexcept {
    // A release after a failed/unknown begin is NOT a recovery operation.
    // The caller must use cancel(), which cannot emit EndFire/CheckFireShot.
    if(journal.phase!=Phase::down||journal.injection_fenced||!same_hold(f))return Result::rejected;
    journal.phase=Phase::release_inflight;
    const auto result=original_call::dispatch(f,journal.release,journal.serial,button_owns,muzzle_ready);
    journal.phase=result==Result::accepted?Phase::await_neutral:result==Result::rejected?Phase::down:Phase::uncertain;
    return result;
}
struct StopReceipt {
    bool complete{},began_in_charge{},ammo_unchanged_from_press{};
    lancerreload_inventory::Counts after{};
};
bool cancel(Route route,StopReceipt& receipt) noexcept {
    receipt={};if(!journal.serial){receipt.complete=true;return true;}
    if(!stop_local(journal.serial))return false;
    if(!journal.may_have_called){receipt.complete=retire_neutral();return receipt.complete;}
    journal.phase=Phase::cleaning;
    auto& owner=torquebow_charge::cancel_owner;
    if(!journal.cleanup_complete){
        if(!owner.pending()&&owner.phase()!=gears_weapon::TorqueBowCancelPhase::complete){
            if(!owner.begin(journal.source,journal.cleanup,journal.serial,owns,stop_local,route,journal.accounted_spawn_call!=0))return false;
        }
        if(!owner.drain()||owner.phase()!=gears_weapon::TorqueBowCancelPhase::complete)return false;
        journal.cleanup_complete=true;
    }
    torquebow_state::Observed actual{};
    if(!read(actual)||!actual.stopped())return false;
    receipt.after=actual.inventory;
    receipt.ammo_unchanged_from_press=actual.inventory==journal.before;
    // Only the original Charge.EndState return can certify an actual Charge
    // exit. A refused/pre-Charge owned press uses zero Charge exit calls.
    receipt.began_in_charge=owner.began_in_charge();
    receipt.complete=retire_neutral();return receipt.complete;
}
std::uint64_t serial() noexcept {return journal.serial;}
bool pending() noexcept {return outstanding.load(std::memory_order_acquire);}

enum class ShotStatus:unsigned {none,waiting_for_ammo,ready,unavailable,source_mismatch,
    inventory_mismatch,model_rejected,receipt_retirement,recorded};
struct ShotObservation {
    torquebow_fire::SpawnReceipt spawn{};
    lancerreload_inventory::Counts before{},after{};
};
ShotStatus inspect_original_shot(ShotObservation& out) noexcept {
    out={};if(!torquebow_fire::peek_spawn(out.spawn))return ShotStatus::none;
    const auto& spawn=out.spawn;
    if(!journal.serial||spawn.input_serial!=journal.serial||!same_admission(spawn.source)||
       !spawn.call||!spawn.shot||!hand::identity_valid(spawn.projectile)||
       !hand::identity_valid(spawn.projectile_class)||spawn.projectile.klass!=spawn.projectile_class.address)
        return ShotStatus::source_mismatch;
    if(journal.accounted_spawn_call)return journal.accounted_spawn_call==spawn.call?
        ShotStatus::receipt_retirement:ShotStatus::source_mismatch;
    out.before=journal.before;
    torquebow_state::Observed actual{};
    if(!read(actual))return ShotStatus::unavailable;
    out.after=actual.inventory;
    if(out.before.capacity!=1||out.before.used||out.before.reserve<0||out.before.infinite)return ShotStatus::inventory_mismatch;
    if(out.after==out.before)return ShotStatus::waiting_for_ammo;
    if(out.after.capacity!=1||out.after.used!=1||out.after.reserve!=out.before.reserve||out.after.infinite)
        return ShotStatus::inventory_mismatch;
    return ShotStatus::ready;
}
ShotStatus record_original_shot(gears_weapon::TorqueBowInteraction& model,
    gears_weapon::TorqueBowStamp const& stamp,std::uint64_t event_serial,
    gears_weapon::TorqueBowEvent& event) noexcept {
    event=gears_weapon::TorqueBowEvent::rejected;
    ShotObservation shot_evidence{};const auto status=inspect_original_shot(shot_evidence);
    if(status==ShotStatus::receipt_retirement){
        // The model already accepted this actual projectile. An interrupted
        // queue retirement cannot submit the ammo delta to that model twice.
        return torquebow_fire::consume_spawn(shot_evidence.spawn.call)?ShotStatus::recorded:status;
    }
    if(status!=ShotStatus::ready)return status;
    const auto& source=journal.source;
    const gears_weapon::TorqueBowIdentity expected{source.world.address,source.pawn.address,source.controller.address,
        source.weapon.address,source.key.world_generation,source.controllers.session,source.controllers.space_epoch,
        source.controllers.input_epoch};
    const auto before=model.inventory();
    if(!(stamp.identity==expected)||before.revision==UINT64_MAX||before.loaded!=1||
       before.reserve!=unsigned(shot_evidence.before.reserve))return ShotStatus::model_rejected;
    auto after=before;++after.revision;after.loaded=0;
    event=model.record_original_shot({stamp,event_serial,shot_evidence.spawn.shot,after,true});
    if(event!=gears_weapon::TorqueBowEvent::shot_recorded&&event!=gears_weapon::TorqueBowEvent::safety_violation)
        return ShotStatus::model_rejected;
    journal.accounted_spawn_call=shot_evidence.spawn.call;
    return torquebow_fire::consume_spawn(shot_evidence.spawn.call)?ShotStatus::recorded:ShotStatus::receipt_retirement;
}
}
