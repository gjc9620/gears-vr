// Bow-specific original ABI survey and hidden component warming on the normal
// GT dispatcher. Neither path issues original fire/reload calls. The sole
// R2 dispatcher separately owns torquebow_input; its active journal must not
// pay for repeated read-only ABI surveys after it has bound the real actor.
namespace torquebow_runtime {
namespace hand=gears_weapon_hand;
// Shared observe_game invokes this for BOTH source-current checks and Native
// proposals. Prepared geometry only: no Engine/Native calls or broker reentry.
bool fill_support_mount(hand::Frame const& f,hand::GripGameState& game) noexcept {
    const auto& backend=torquebow_input::backend;
    if(!game.known||!game.main_hand_held||game.busy||game.animation_owns_hands||game.kind!=hand::WeaponKind::firearm||
       hand::index_pinch(f)||!backend.geometry_current(f)||!backend.support_known||
       f.key.frame<=backend.geometry_parent.key.frame||!gears_weapon_two_hand::geometry(backend.support_grip))return false;
    const auto& p=backend.support_grip.position;
    game.mount={};game.mount.support_grip_in_weapon={p.y,p.z,-p.x};
    game.mount.support_length_tolerance_metres=.08;game.mount.maximum_support_correction_radians=.6;
    game.acquire_metres=.055;game.release_metres=.08;game.mount_asset=nativelancerfollow::bound_key.asset;
    game.mount_revision=backend.geometry_hold;game.mount_known=game.mount.valid();return game.mount_known;
}
struct RoutePublication {hand::Frame source{};hand::GripGameState routes{};bool known{};};
RoutePublication route_proposal{},route_committed{};
void apply_routes(hand::GripGameState const& saved,hand::GripGameState& game) noexcept {
    game.request_ammo=saved.request_ammo;game.request_magazine=saved.request_magazine;
    game.magazine_observation_pending=saved.magazine_observation_pending;game.object_incumbents=saved.object_incumbents;
}
// Native's existing arbitration callback: only its supplied Frame and already
// prepared POD state. No Engine, Native or HandClient calls/reentry here.
void propose_live(hand::Frame const& f,hand::GripGameState& game) noexcept {
    if(!game.known||!game.main_hand_held||game.busy||game.animation_owns_hands||
       game.kind!=hand::WeaponKind::firearm||hand::index_pinch(f)||!f.left_tracked||!f.left_grip_known||!f.left_grip_down||
       !torquebow_input::driver.matches_source(f,vrequip::model.hold_generation()))return;
    const auto lease=torquebow_input::backend.reload_lease;
    if(torquebow_input::backend.reload_owned()&&lease.id==f.press_token&&hand::lease_valid(lease,f.key.lifecycle)){
        for(auto& slot:game.object_incumbents){if(hand::same(slot,lease))return;
            if(slot.kind==hand::LeaseKind::none){slot=lease;return;}}return;
    }
    const auto& model=torquebow_input::driver.interaction();
    if((model.can_begin_reload(f.now_ns)||(model.stage()==gears_weapon::TorqueBowStage::reload_steps&&!model.held_arrow_serial()))&&
       torquebow_arrow_visual::warmed()&&torquebow_arrow_visual::same_parent(f)&&torquebow_input::shoulder(f))game.request_ammo=true;
}
void propose(hand::Frame const& f,hand::GripGameState& game) noexcept {
    if(route_committed.known&&hand::same_frame(route_committed.source,f)){apply_routes(route_committed.routes,game);return;}
    if(route_proposal.known&&hand::same_frame(route_proposal.source,f)){apply_routes(route_proposal.routes,game);return;}
    propose_live(f,game);route_proposal={f,game,true};
}
void committed(hand::Frame const& f,hand::Decision const& d) noexcept {
    if(route_proposal.known&&hand::same_frame(route_proposal.source,f)&&hand::same(f.key,d.key))route_committed=route_proposal;
}
hand::ObjectIdentity last_weapon{};unsigned last_gate{~0u},reports{};
std::uint64_t last_sequence{};std::int64_t next_preparation{};
bool pending_work() noexcept {return torquebow_arrow_visual::pending()&&weapon_part_assets::find(torquebow_arrow_visual::owner.handle);}
void service_unselected(uptr pc,uptr selected) noexcept {
    if(torquebow_arrow_visual::pending()&&torquebow_arrow_visual::controller()==pc&&
       (weapon_part_assets::stop_requested.load(std::memory_order_acquire)||
        selected!=torquebow_arrow_visual::owner.parent.weapon.address||
        vrequip::model.hold_generation()!=torquebow_arrow_visual::owner.hold))torquebow_arrow_visual::retire();
}
bool tick(uptr pc) noexcept {
    const auto selected=uptr(vrequip::model.held_weapon());
    service_unselected(pc,selected);
    if(weapon_part_assets::stop_requested.load(std::memory_order_acquire))return false;
    // Hidden preparation must not spend an already owned original Charge's
    // source budget or run a constructor inside that action's service turn.
    if(torquebow_input::pending()||torquebow_input::driver.interaction().has_fire_attempt())return true;
    if(!selected||!playground::instance(selected,L"Weap_Bow_base")||
       !weaponhand::current_gt(nullptr)||!nativeequipment::domain_ready())return false;
    const auto now=weaponhand::clock_ns(nullptr);
    if(now<next_preparation)return false;
    hand::Frame f{};hand::GripGameState game{};
    if(!weaponhand::client.copy_frame_on_gt(f,game)||f.controller.address!=pc||f.weapon.address!=selected||
       !nativeequipment::current(nullptr,f)||!game.known||!game.main_hand_held||
       game.busy||game.animation_owns_hands||f.controllers.sequence==last_sequence)return false;
    last_sequence=f.controllers.sequence;next_preparation=now+250000000;
    // Hidden construction is not an input or cached action certificate. A
    // completed warm component needs no repeated ABI/material surveys while
    // idle. show() independently rereads the original appearance and source.
    if(!torquebow_arrow_visual::warmed()){
        gears_committed_interaction::Frame avatar{};
        if(handrig::read_interaction(avatar)){
            torquebow_input::backend.warm_result=2+unsigned(torquebow_arrow_visual::prepare(f,avatar));
        }else torquebow_input::backend.warm_result=1;
    }
    if(torquebow_input::pending()||torquebow_input::driver.bound())return true;
    if(!torquebow_input::backend.geometry_current(f)||
       torquebow_input::backend.feed_calibration.instance_serial!=torquebow_input::backend.config_serial)
        torquebow_input::backend.prepare_geometry(f);
    // Bind the existing NON-ACTION CanReload veto before Bow can ever obtain
    // its independent projectile source. This neither opens a physical reload
    // permission nor changes ammo; the same original actor stays vetoed stowed.
    lancerreload_manual::prepare(f);
    torquebow_original::Preparation p{};const bool ready=torquebow_original::prepare(f,p);
    if(!hand::same(last_weapon,f.weapon)||p.gate!=last_gate){
        last_weapon=f.weapon;last_gate=p.gate;
        if(log_file&&reports++<16){
            fwprintf(log_file,L"torquebow_original_preparation weapon=%llx frame=%llu sequence=%llu ready=%u gate=%u clear=%llx ended=%llx state=%llx transition=%llx transition_bytes=%u pending_property=%llx pending_offset=%u pending_mask=%u capacity=%d used=%d reserve=%d elapsed_ns=%lld input_age_begin_ns=%lld input_age_end_ns=%lld source_check_known=%u source_current=%u withdrawal_frame_sequence_match=%u withdrawal_candidate_failure=%u withdrawal_candidate_difference=%u preparation_only=1\n",
                f.weapon.address,f.key.frame,f.controllers.sequence,unsigned(ready),p.gate,p.clear_pending,
                p.charge_ended,p.get_state,p.goto_state,p.goto_bytes,p.pending_end_property,p.pending_offset,
                p.pending_mask,p.inventory.capacity,p.inventory.used,p.inventory.reserve,p.finished_ns-p.started_ns,
                p.input_age_begin_ns,p.input_age_end_ns,p.source_check_known,p.source_current,p.withdrawal_frame_sequence_match,
                p.withdrawal_candidate_failure,p.withdrawal_candidate_difference);fflush(log_file);
        }
    }
    return ready;
}
}
