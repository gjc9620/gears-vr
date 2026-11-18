// Single game-thread consumer for the Native-owned left hand. The integrated
// candidate is under controlled runtime verification, not yet packaged.
namespace lancerreload_runtime {
namespace hand=gears_weapon_hand;
namespace inv=lancerreload_inventory;
std::atomic<std::uint64_t> configured_policy{gears_weapon::LancerReloadPolicy{}.packed()};
gears_weapon::LancerReloadPolicy opening_policy;
bool ui_config_pending{};
bool completing_magazine_seat{};
std::uint64_t parked_display_hold{};
enum class Stage { idle,extracting,holding_original,waiting_ammo,holding_new,recovering,completing,parked,charging };
Stage stage{};
inv::ObservedInventory observer;
inv::Transaction transaction;
gears_weapon::LancerMagazineExtractionStroke extraction;
gears_weapon::LancerMagazineInsertionStroke insertion;
gears_weapon::LancerMotionSamples motion_samples;
// Existing grasp/input pin only; never a replacement pose/action Frame.
hand::Frame magazine_parent{};hand::ExistingActionLifetime magazine_lifetime{};
gears_lancer_saw_input::HoldInput magazine_input;
std::uint64_t magazine_hold{};bool magazine_pin_known{};
unsigned magazine_pose_gap_checks{};
// Evidence of the FIRST rejected decision during an already journalled grasp.
// POD only: no new read/lock/lease query, action, collector or callback I/O.
struct MagazineLeaseStop {
    hand::FrameKey source{},decision_key{};
    hand::FrameReadObservation read{},withdrawal{};
    hand::Lease lease{},incumbents[2]{};
    std::uint64_t token{},decision_token{},pin_epoch{},pin_lifecycle{};
    std::int64_t age{};
    unsigned status{},required{},wanted{},control{},pressed{},released{},known{},down{},tracked{};
    bool captured{};
};
MagazineLeaseStop magazine_lease_stop;
void clear_magazine_input_pin() noexcept {
    magazine_parent={};magazine_lifetime={};magazine_input.reset();
    magazine_hold=0;magazine_pin_known=false;magazine_pose_gap_checks=0;
}
// Bounded game-thread observations, drained only at a transition/cleanup.
// This captures what the stroke actually consumed, not requested controller
// targets, without adding I/O inside the animation/pose callback.
struct ExtractionObservation {
    std::uint64_t sequence{},ms{};
    double depth{},lateral{};
    unsigned failure{};
    bool active{},complete{};
};
std::array<ExtractionObservation,32> extraction_observations{};
unsigned extraction_observation_count{};
void report_extraction() noexcept {
    if(magazine_lease_stop.captured&&log_file){
        const auto& s=magazine_lease_stop;
        fwprintf(log_file,L"lancer_reload_lease_stop frame=%llu lifecycle=%llu token=%llu decision_frame=%llu decision_lifecycle=%llu decision_token=%llu status=%u release_required=%u wanted=%u lease=%u lease_id=%llu lease_lifecycle=%llu control=%u pressed=%u released=%u known=%u down=%u tracked=%u age_ns=%lld pin_epoch=%llu pin_lifecycle=%llu read_failure=%u read_difference=%u withdrawal_frame=%llu withdrawal_sequence=%llu withdrawal_failure=%u withdrawal_difference=%u withdrawal_age_ns=%lld incumbent0=%u,%llu,%llu incumbent1=%u,%llu,%llu\n",
            s.source.frame,s.source.lifecycle,s.token,s.decision_key.frame,s.decision_key.lifecycle,s.decision_token,
            s.status,s.required,s.wanted,unsigned(s.lease.kind),s.lease.id,s.lease.lifecycle,s.control,s.pressed,s.released,s.known,s.down,s.tracked,s.age,
            s.pin_epoch,s.pin_lifecycle,unsigned(s.read.failure),s.read.game_difference,s.withdrawal.frame,s.withdrawal.sequence,
            unsigned(s.withdrawal.failure),s.withdrawal.game_difference,s.withdrawal.input_age_ns,
            unsigned(s.incumbents[0].kind),s.incumbents[0].id,s.incumbents[0].lifecycle,
            unsigned(s.incumbents[1].kind),s.incumbents[1].id,s.incumbents[1].lifecycle);fflush(log_file);
    }
    magazine_lease_stop={};
    if(log_file&&magazine_pose_gap_checks){
        fwprintf(log_file,L"lancer_reload_magazine_gap_kept verified_queries=%u pose_advanced=0 inventory_written=0\n",magazine_pose_gap_checks);
        fflush(log_file);
    }
    if(log_file&&extraction_observation_count){
        const auto begin=extraction_observation_count>32?extraction_observation_count-32:0;
        for(auto i=begin;i<extraction_observation_count;++i){const auto& o=extraction_observations[i%32];
            fwprintf(log_file,L"lancer_reload_extraction_observed index=%u sequence=%llu ms=%llu depth=%.6g lateral=%.6g failure=%u active=%u complete=%u\n",
                i,o.sequence,o.ms,o.depth,o.lateral,o.failure,unsigned(o.active),unsigned(o.complete));}
        fflush(log_file);
    }
    extraction_observation_count=0;
}
struct InsertionObservation {
    std::uint64_t sequence{},ms{};double depth{},lateral{};unsigned result{};
};
std::array<InsertionObservation,32> insertion_observations{};unsigned insertion_observation_count{};
void report_insertion() noexcept {
    if(log_file&&insertion_observation_count){
        const auto begin=insertion_observation_count>32?insertion_observation_count-32:0;
        for(auto i=begin;i<insertion_observation_count;++i){const auto& o=insertion_observations[i%32];
            fwprintf(log_file,L"lancer_reload_insertion_observed index=%u sequence=%llu ms=%llu depth=%.6g lateral=%.6g result=%u\n",
                i,o.sequence,o.ms,o.depth,o.lateral,o.result);}
        fflush(log_file);
    }
    insertion_observation_count=0;
}
std::uint64_t original_item{},reload_serial{},replacement_item{};
std::uint64_t resumed_grab{};
inv::InsertionWriter writer;
inv::Request completion;
inv::Inventory opening_inventory;
lancerreload_state::RewardReceipt reward_receipt;
lancerreload_state::Lease parked_lease;
std::int64_t perfect_window_ms{};
bool completion_exited{};
bool standalone_charging{},parked_charging{};
bool park_pending{},park_preserve{};
std::int64_t charging_closed_tick{},charging_closed_window{};
inv::Identity standalone_identity{};
bool tracking_suspended{},resume_release_seen{};
hand::FrameReadObservation last_frame_read_observation{};
hand::FrameReadObservation first_charging_gap{};
bool charging_gap_known{};
std::int64_t charging_gap_began{};
std::int64_t last_input_sync_ns{};
hand::Frame charging_parent{};hand::ExistingActionLifetime charging_lifetime{};
gears_lancer_saw_input::HoldInput charging_input;
std::uint64_t charging_hold{};bool charging_pin_known{};
void clear_charging_pin() noexcept {
    charging_parent={};charging_lifetime={};charging_input.reset();charging_hold=0;charging_pin_known=false;
}
bool retain_charging_during_pose_gap(uptr pc) noexcept {
    // Retain only an already-acquired E pinch, never a pose/part movement or
    // ammunition completion. The independent raw publisher must STILL be
    // fresh, tracked and on the exact same held trigger with no hidden edges.
    if(stage!=Stage::charging||!charging_pin_known||!lancer_charging::armed||
       !lancer_charging::ownership.active||lancer_charging::ownership.retiring||
       pc!=charging_parent.controller.address||!nativeequipment::domain_ready()||
       !weaponhand::current_gt(nullptr)||!nativeequipment::restoration_quiet()||
       charging_hold!=vrequip::model.hold_generation()||
       !vrequip::model.held_for(charging_parent.world.address,charging_parent.pawn.address,charging_parent.weapon.address)||
       !weaponhand::client.existing_action_lifetime_on_gt(charging_lifetime))return false;
    const auto begin=weaponhand::clock_ns(nullptr);weaponhand::HoldSourceObservation game{};
    if(begin<=0||!weaponhand::observe_hold_source(nullptr,charging_parent,begin,game)||
       !game.known||!game.main_hand_held||game.busy||game.animation_owns_hands)return false;
    gears_vr::StereoSample stereo{};gears_vr::ControllerInputFrame input{};
    gears_hand_continuity::TrackedInput tracked{};gears_hand_continuity::Publisher publisher{};
    const bool sim=charging_parent.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated;
    if(nativeequipment::input_continuity_reader.read(GetCurrentProcessId(),[](){return weaponhand::clock_ns(nullptr);},
       sim,stereo,input,tracked,publisher)!=gears_current_tracking::Failure::none)return false;
    const auto end=weaponhand::clock_ns(nullptr);
    return end>=begin&&end-begin<=50000000&&charging_input.refresh(stereo,tracked,publisher,end,sim)&&
        weaponhand::client.existing_action_lifetime_on_gt(charging_lifetime);
}
bool equipment_switch_dispatched{};
bool pin_existing_magazine_grasp(hand::Frame const& f) noexcept {
    if(magazine_pin_known&&magazine_parent.press_token==f.press_token&&
       hand::same(magazine_parent.weapon,f.weapon)&&magazine_hold==vrequip::model.hold_generation()&&
       weaponhand::client.existing_action_lifetime_on_gt(magazine_lifetime))return true;
    clear_magazine_input_pin();
    if(!f.left_tracked||!f.left_grip_known||!f.left_grip_down||!nativeequipment::current(nullptr,f))return false;
    gears_hand_continuity::TrackedInput tracked{};gears_hand_continuity::Publisher publisher{};
    if(!nativeequipment::read_input_continuity(f,tracked,publisher)||
       !magazine_input.bind(f.stereo,tracked,publisher,weaponhand::clock_ns(nullptr),
           f.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated,hand::left_control(f))||
       !weaponhand::client.capture_existing_action_lifetime_on_gt(f.key,magazine_lifetime))return false;
    magazine_parent=f;magazine_hold=vrequip::model.hold_generation();magazine_pin_known=true;return true;
}
bool retain_magazine_during_pose_gap(uptr pc) noexcept {
    // The same genuine held input protects an extracted original/replacement
    // too. Missing action geometry alone is not a physical release. This
    // retains logical ownership only; fresh geometry is still required below
    // for every visual move, insertion sample and ammunition transition.
    const bool held_stage=(stage==Stage::extracting&&extraction.active())||
        stage==Stage::holding_original||stage==Stage::holding_new;
    if(!held_stage||!magazine_pin_known||equipment_switch_dispatched||
       pc!=magazine_parent.controller.address||!nativeequipment::domain_ready()||
       !weaponhand::current_gt(nullptr)||!nativeequipment::restoration_quiet()||
       magazine_hold!=vrequip::model.hold_generation()||!lancerreload_visual::alive()||
       !vrequip::model.held_for(magazine_parent.world.address,magazine_parent.pawn.address,magazine_parent.weapon.address)||
       !weaponhand::client.existing_action_lifetime_on_gt(magazine_lifetime))return false;
    const auto begin=weaponhand::clock_ns(nullptr);weaponhand::HoldSourceObservation game{};
    if(begin<=0||!weaponhand::observe_hold_source(nullptr,magazine_parent,begin,game)||
       !game.known||!game.main_hand_held||game.busy||game.animation_owns_hands)return false;
    gears_vr::StereoSample stereo{};gears_vr::ControllerInputFrame input{};
    gears_hand_continuity::TrackedInput tracked{};gears_hand_continuity::Publisher publisher{};
    const bool sim=magazine_parent.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated;
    if(nativeequipment::input_continuity_reader.read(GetCurrentProcessId(),[](){return weaponhand::clock_ns(nullptr);},
       sim,stereo,input,tracked,publisher)!=gears_current_tracking::Failure::none)return false;
    const auto end=weaponhand::clock_ns(nullptr);
    const bool kept=end>=begin&&end-begin<=50000000&&magazine_input.refresh(stereo,tracked,publisher,end,sim)&&
        weaponhand::client.existing_action_lifetime_on_gt(magazine_lifetime);
    if(kept){
        ++magazine_pose_gap_checks;
        if(magazine_pose_gap_checks==1&&log_file){fwprintf(log_file,
            L"lancer_magazine_pose_gap retained=1 stage=%u token=%llu physical_input_current=1 no_pose_or_inventory_progress=1\n",
            unsigned(stage),magazine_parent.press_token);fflush(log_file);}
    }
    return kept; // No source timestamp, geometry, gesture or inventory changes.
}
std::atomic<bool> stop_requested{},outstanding{};
bool needs_priority() noexcept {return outstanding.load(std::memory_order_acquire);}
std::uint64_t logged_press{};
// Software preparation only. No component/item/ammo ownership, old pose or
// timestamp is carried forward. A retained CURRENT lease may finish a start
// on a later fresh publication while the hand remains at actual contact.
struct PendingStart {
    inv::Identity identity{};hand::Lease lease{};
    std::uint64_t frame{},attempt_frame{};std::int64_t until_ns{};
};
PendingStart pending_start{};
void cancel_preparation() noexcept {
    if(stage!=Stage::idle||lancerreload_visual::active||lancerreload_state::lease.acquired||transaction.pending())return;
    pending_start={};observer={};transaction={};extraction.reset();
    clear_magazine_input_pin();
    if(lancerreload_contact::preparing)lancerreload_contact::release();
    outstanding.store(false,std::memory_order_release);
}
hand::ObjectIdentity last_visual_weapon{};
unsigned notify_observations{};
void observe_magazine_notify(uptr object,uptr function) noexcept {
    if(object!=last_visual_weapon.address||notify_observations>=24||
       GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return;
    wchar_t label[96]{};name(function,label,96);
    if(wcscmp(label,L"Notify_AmmoGrab")&&wcscmp(label,L"Notify_AmmoRelease")&&
       wcscmp(label,L"Notify_AmmoReload"))return;
    hand::ObjectIdentity actual{};
    if(!nativeequipment::identify(nullptr,object,actual)||!hand::same(actual,last_visual_weapon))return;
    ++notify_observations;
    if(log_file){fwprintf(log_file,L"lancer_reload_notify name=%ls stage=%u lease=%u visual=%u qpc_ns=%lld\n",
        label,unsigned(stage),unsigned(lancerreload_state::lease.acquired),unsigned(lancerreload_visual::active),
        weaponhand::clock_ns(nullptr));fflush(log_file);}
}
void switch_dispatched(hand::Frame const& source) noexcept {
    gnasher_state::switch_dispatched(source);
    hammerburst_runtime::switch_dispatched(source);
    snub_runtime::switch_dispatched(source);
    longshot_runtime::switch_dispatched(source);
    boltok_runtime::switch_dispatched(source);
    // The equipment owner already authenticated and dispatched this call.
    // Record its outcome before it invalidates the next hand publication.
    if(GetCurrentThreadId()==game_thread.load(std::memory_order_acquire)&&stage!=Stage::idle&&
       inv::identity(source)==transaction.inventory().identity)equipment_switch_dispatched=true;
}
bool weapon_retired(hand::ObjectIdentity const& expected) noexcept {
    if(!hand::identity_valid(expected)||GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return false;
    namespace meta=weaponhand::meta;
    meta::Table objects{},names{},after{},after_names{};uptr first{},second{};
    if(!meta::tables(base,objects,names)||expected.registry_index>=unsigned(objects.count)||
       !copy(&first,objects.data+size_t(expected.registry_index)*sizeof(uptr),sizeof first)||
       !meta::tables(base,after,after_names)||!meta::same_table(objects,after)||
       !copy(&second,after.data+size_t(expected.registry_index)*sizeof(uptr),sizeof second)||first!=second)return false;
    if(first!=expected.address)return true; // Stable registry removal/replacement.
    hand::ObjectIdentity current{};
    return nativeequipment::identify(nullptr,expected.address,current)&&!hand::same(expected,current);
}
void reset_bookkeeping(bool keep_seated_grasp=false) noexcept {
    parked_display_hold=0;
    lancer_charging::enabled=false;lancer_charging::release_seen=false;
    standalone_charging=parked_charging=false;standalone_identity={};
    park_pending=park_preserve=false;charging_closed_tick=charging_closed_window=0;
    lancerreload_visual::set_insertion_assist(false);
    lancerreload_visual::extraction_display_enabled=false;
    clear_magazine_input_pin();
    opening_policy={};ui_config_pending=false;
    pending_start={};
    lancerreload_contact::waiting_ammo=false;lancerreload_contact::resume_original=false;
    if(!keep_seated_grasp)lancerreload_contact::release();
    observer={};transaction={};extraction.reset();insertion.reset();motion_samples.clear();
    writer={};completion={};opening_inventory={};reward_receipt={};parked_lease={};perfect_window_ms=0;
    completion_exited=completing_magazine_seat=false;tracking_suspended=resume_release_seen=false;last_input_sync_ns=0;
    charging_gap_known=false;charging_gap_began=0;first_charging_gap={};
    clear_charging_pin();
    equipment_switch_dispatched=false;
    extraction_observation_count=insertion_observation_count=0;
    original_item=reload_serial=replacement_item=resumed_grab=0;stage=Stage::idle;
    outstanding.store(false,std::memory_order_release);
}
bool retire_destroyed_weapon() noexcept {
    const auto weapon=lancer_charging::ownership.active?lancer_charging::ownership.identity.weapon:
        lancerreload_visual::active?lancerreload_visual::saved.weapon:
        transaction.inventory().identity.weapon;
    if(!weapon_retired(weapon))return false;
    if(!lancer_magazine_appearance::retired_weapon(weapon,
        weapon_retired(lancerreload_visual::saved.component)))return false;
    // The old registry identity is proven gone. Never restore into its reused
    // address or acknowledge an old inventory request against the new actor.
    if(log_file){fwprintf(log_file,L"lancer_reload_retired weapon=%llx index=%u pending=%u\n",
        weapon.address,weapon.registry_index,unsigned(transaction.pending()));fflush(log_file);}
    lancerreload_visual::active=false;lancerreload_visual::saved={};lancerreload_visual::last_grab_known=false;
    lancerreload_state::lease={};lancer_charging::ownership={};lancer_charging::geometry={};
    lancer_charging::stroke.reset();lancer_charging::held={};lancer_charging::armed=false;
    reset_bookkeeping();return true;
}
bool reconcile_after_exit() noexcept {
    if(!transaction.pending())return true;
    inv::Request request{};inv::Inventory reconciled_counts{};
    if(!transaction.dispatched_request(request)||
       !observer.sample_owned(request.identity,reconciled_counts))return false;
    using Call=gears_weapon::ReloadTxnCall;
    using Result=gears_weapon::ReloadTxnResult;
    // Recovery never retries setters. Exact before/after counts can resolve a
    // lost receipt; an intermediate or unexpected inventory stays unresolved.
    bool accepted{};Result applied=Result::none;
    if(request.call==Call::insert){
        if(inv::counts_equal(reconciled_counts,request.expected))accepted=true;
        else if(!inv::counts_equal(reconciled_counts,request.before))return false;
        else if(!opening_inventory.loaded)lancer_charging::clear_uncommitted(request.identity);
    }else if(request.call==Call::exit_without_fill||request.call==Call::finish){
        if(!inv::counts_equal(reconciled_counts,request.expected))return false;
        if(request.result==Result::perfect){if(!reward_receipt.applied)return false;}
        else if(request.result!=Result::ordinary&&request.result!=Result::inspection)return false;
        accepted=true;
        applied=request.result==Result::inspection?Result::none:request.result;
    }else return false;
    transaction.interrupt();
    return transaction.acknowledge({request.token,accepted,reconciled_counts,applied});
}
bool cleanup(unsigned reason,bool keep_seated_grasp=false) noexcept {
    if(retire_destroyed_weapon())return true;
    report_extraction();report_insertion();
    // Keep retained ownership if restoration failed. Do not let a new action
    // silently overwrite the unresolved original component/reload lease.
    if(stage!=Stage::recovering&&log_file){fwprintf(log_file,L"lancer_reload_cleanup line=%u stage=%u ledger=%u result=%u pending=%u\n",reason,unsigned(stage),unsigned(transaction.stage()),unsigned(transaction.result()),unsigned(transaction.pending()));fflush(log_file);}
    stage=Stage::recovering;
    const bool charging=lancer_charging::restore();
    const bool visual=lancerreload_visual::restore();
    inv::Request pending_request{};
    if(transaction.dispatched_request(pending_request)&&
       !lancerreload_state::restore_partial_insert(pending_request,observer))return false;
    const bool state=lancerreload_state::recover_serial(reload_serial);
    if(!charging||!visual||!state||!reconcile_after_exit())return false;
    const bool seated=keep_seated_grasp&&lancerreload_contact::held_after_frame&&
        lancerreload_contact::held.id&&lancerreload_contact::held_identity.valid();
    if(seated)lancerreload_contact::seated_hold=true;
    reset_bookkeeping(seated);return true;
}
// An uncommitted replacement is returned to the supply on tracking loss.
// Keep the open gun, original reload clock and inventory ledger; no native
// exit/refill and no new gesture is inferred from tracking returning.
bool suspend_replacement(unsigned reason) noexcept {
    if(stage==Stage::holding_original){
        if(tracking_suspended)return true;
        if(transaction.pending()||!lancerreload_visual::alive()||
           !lancerreload_visual::last_grab_known||
           (reload_serial&&!lancerreload_state::owned_state()))return false;
        // The absolute visual stays at the last valid position. Old rounds,
        // item identity and the accepted reload remain unchanged.
        insertion.reset();motion_samples.clear();lancerreload_contact::release();
        lancerreload_contact::resume_original=true;
        tracking_suspended=true;resume_release_seen=false;
        if(log_file){fwprintf(log_file,L"lancer_reload_suspend stage=%u reason=%u interaction_gate=%u geometry_age_ns=%lld frame_read_failure=%u game_difference=%u\n",unsigned(stage),reason,lancerreload_visual::interaction_failure,
            weaponhand::clock_ns(nullptr)-lancerreload_contact::last_geometry_ns,unsigned(last_frame_read_observation.failure),last_frame_read_observation.game_difference);fflush(log_file);}return true;
    }
    if(stage!=Stage::holding_new&&stage!=Stage::waiting_ammo)return false;
    if(tracking_suspended)return true;
    if(transaction.pending()||(reload_serial?!lancerreload_state::owned_state():
        lancerreload_state::lease.acquired||transaction.target()||!lancerreload_visual::alive()))return false;
    if(!lancerreload_visual::stow())return false;
    if(stage==Stage::holding_new&&!transaction.release_item(replacement_item))return false;
    replacement_item=0;insertion.reset();motion_samples.clear();lancerreload_contact::release();
    lancerreload_contact::waiting_ammo=true;stage=Stage::waiting_ammo;
    tracking_suspended=true;resume_release_seen=false;
    if(log_file){fwprintf(log_file,L"lancer_reload_suspend stage=%u reason=%u interaction_gate=%u geometry_age_ns=%lld frame_read_failure=%u game_difference=%u\n",unsigned(stage),reason,lancerreload_visual::interaction_failure,
        weaponhand::clock_ns(nullptr)-lancerreload_contact::last_geometry_ns,unsigned(last_frame_read_observation.failure),last_frame_read_observation.game_difference);fflush(log_file);}return true;
}
bool park_owned_reload(bool preserve_charging_state=false) noexcept {
    if(stage==Stage::charging){
        if(!lancer_charging::restore()){
            // Retire only the cosmetic action; preserve this gun's committed
            // ammo, charging debt and original reward clock while its render
            // FIFO drains. A cleanup fallback would otherwise lose the park.
            park_preserve=park_pending?(park_preserve&&preserve_charging_state):preserve_charging_state;
            park_pending=true;return true;
        }
        park_pending=false;
        if(standalone_charging){
            if(preserve_charging_state){lancer_charging::release_seen=false;return true;}
            reset_bookkeeping();return true;
        }
        if(transaction.pending()||!transaction.interrupt())return false;
        if(lancerreload_state::lease.acquired){
            parked_lease=lancerreload_state::lease;
            // A lost hand publication revokes the gesture, not the accepted
            // reload clock/HUD or committed ammunition. Keep that original
            // ownership until a fresh release/grasp resumes this same hold.
            // Equipment switching still performs the original normal exit.
            if(preserve_charging_state){if(!lancerreload_state::owned_state())return false;}
            else if(!lancerreload_state::recover_serial(reload_serial))return false;
        }
        completion_exited=!lancerreload_state::lease.acquired;
        parked_charging=true;lancerreload_contact::release();clear_charging_pin();
        resume_release_seen=false;stage=Stage::parked;return true;
    }
    if(stage!=Stage::holding_original&&stage!=Stage::holding_new&&stage!=Stage::waiting_ammo)return false;
    if(!reload_serial||transaction.pending()||!lancerreload_state::lease.acquired)return false;
    const auto saved=lancerreload_state::lease;
    if(!lancerreload_visual::stow())return false;
    if(stage==Stage::holding_original&&!transaction.stow_original(original_item))return false;
    if(stage==Stage::holding_new&&!transaction.release_item(replacement_item))return false;
    if(!lancerreload_state::recover_serial(reload_serial)||!transaction.interrupt())return false;
    bool display_changed{};
    if(!lancerreload_visual::hide_parked(display_changed))return false;
    parked_display_hold=saved.identity.hold_generation;
    parked_lease=saved;replacement_item=0;insertion.reset();motion_samples.clear();
    lancerreload_contact::release();lancerreload_contact::resume_original=false;
    lancerreload_contact::waiting_ammo=false;tracking_suspended=resume_release_seen=false;
    stage=Stage::parked;return true;
}
bool blocks_fire(hand::ObjectIdentity const& weapon) noexcept {
    if(gnasher_fire::blocks_fire(weapon))return true;
      return lancer_charging::blocks_fire(weapon)||longshot_runtime::blocks_fire(weapon)||hammerburst_runtime::blocks_fire(weapon)||snub_runtime::blocks_fire(weapon)||boltok_runtime::blocks_fire(weapon)||
          lancersaw_starter::blocks_fire(weapon)||
        (stage!=Stage::idle&&hand::same(weapon,transaction.inventory().identity.weapon));
}
bool stow_ready(hand::Frame const& f) noexcept {
    // Committed ammunition and the uncharged debt survive parking. An old
    // hand lease, running animation or unresolved inventory write does not.
    if(stage!=Stage::parked||transaction.pending()||lancerreload_state::lease.acquired||
       lancer_charging::ownership.active)return false;
    if(parked_charging)return lancer_charging::required(f)&&!lancerreload_visual::active&&
        hand::same(f.weapon,transaction.inventory().identity.weapon);
    // Ordinary magazine parking keeps the original leased display component
    // hidden for later recovery. It is deliberately still active; charging's
    // no-magazine-display predicate would reject every open-magazine stow.
    return reload_serial&&transaction.stage()==gears_weapon::ReloadTxnStage::interrupted&&
        parked_lease.identity==transaction.inventory().identity&&
        inv::same_instance_across_hold(parked_lease.identity,inv::identity(f))&&
        lancerreload_visual::active&&lancerreload_visual::alive();
}
bool prepare_stow(hand::Frame const& f,bool& changed) noexcept {
    if(hammerburst_runtime::park_control::owns_stow(f))return hammerburst_runtime::park_control::prepare_stow(f,changed);
    changed=false;if(!needs_priority()||stow_ready(f))return true;
    if(stage==Stage::holding_original||stage==Stage::holding_new||stage==Stage::waiting_ammo){
        // A genuine shoulder press may suspend an accepted magazine reload,
        // just as the existing weapon-change path already does. Parking
        // releases item reservations and exits retail Reloading without
        // refilling, rewarding, or restarting the accepted transaction clock.
        if(!reload_serial||standalone_charging||transaction.pending()||
           !lancerreload_state::lease.acquired||!lancerreload_visual::active||
           lancer_charging::ownership.active||
           !(inv::identity(f)==transaction.inventory().identity)||
           !lancerreload_state::owns(f,reload_serial)||!park_owned_reload())return false;
        changed=true;return true; // Equipment must reacquire its NEW frame.
    }
    const bool retained_charging=stage==Stage::parked&&parked_charging;
    if((stage!=Stage::charging&&!retained_charging)||standalone_charging||!lancer_charging::required(f)||
       transaction.pending()||!lancerreload_state::lease.acquired||lancerreload_visual::active||
       lancer_charging::ownership.active||!(inv::identity(f)==transaction.inventory().identity)||
       !nativeequipment::current(nullptr,f))return false;
    if(retained_charging){
        // A brief input gap may park charging while retaining its original
        // reload/UI lease. A NEW authenticated shoulder press must be able to
        // close that lease just like the ordinary charging path, rather than
        // reject forever because it is already parked. The existing equipment
        // handoff reacquires a fresh Frame after changed=true. This neither
        // charges the gun nor changes ammo, identity or the reward clock.
        if(!lancerreload_state::owned_state()||!lancerreload_state::recover_serial(reload_serial))return false;
        completion_exited=!lancerreload_state::lease.acquired;
        if(!completion_exited)return false;
        changed=true;return true;
    }
    if(!park_owned_reload())return false;
    changed=true;return true;
}
void report_charging_gate(unsigned gate,hand::Frame const& source) noexcept {
    for(unsigned i=0;i<lancer_charging::contact_observation_count&&log_file;++i){
        const auto& c=lancer_charging::contact_observations[i];
        fwprintf(log_file,L"lancer_charging_contact frame=%llu token=%llu held=%llu flags=%u gate=%u\n",c.frame,c.token,c.held_id,c.flags,c.gate);
    }
    lancer_charging::contact_observation_count=0;
    static std::uint64_t previous_gate=~0ull;static unsigned gate_rows{};
    const auto& d=lancer_charging::last_decision;
    const auto key=std::uint64_t(gate)|(std::uint64_t(lancer_charging::contact_gate)<<12)|
        (std::uint64_t(lancer_charging::local_gate)<<16)|(std::uint64_t(d.status)<<20)|
        (std::uint64_t(d.new_grant)<<28)|(std::uint64_t(d.release_required)<<29);
    if(key!=previous_gate&&gate_rows<96&&log_file){
        previous_gate=key;++gate_rows;
        fwprintf(log_file,L"lancer_charging_gate gate=%u phase=%u armed=%u owned=%u frame=%llu contact_gate=%u local_gate=%u distance_m=%.9g contact_sequence=%llu interaction=%u decision=%u new_grant=%u release=%u kind=%u token=%llu lease=%llu lifecycle=%llu pending_frame=%llu pending_remaining_ns=%lld resume_release=%u ledger=%u capture_gate=%u capture_node=%llx capture_name=%ls\n",
            gate,unsigned(lancer_charging::stroke.phase()),unsigned(lancer_charging::armed),
            unsigned(lancer_charging::ownership.active),source.key.frame,lancer_charging::contact_gate,lancer_charging::local_gate,
            lancer_charging::contact_distance,lancer_charging::contact_sequence,lancerreload_visual::interaction_failure,
            unsigned(d.status),d.new_grant,d.release_required,unsigned(d.lease.kind),source.press_token,d.lease.id,source.key.lifecycle,
            lancer_charging::pending_frame,lancer_charging::pending_until-weaponhand::clock_ns(nullptr),
            unsigned(resume_release_seen),unsigned(transaction.stage()),lancer_charging::capture_gate,lancer_charging::capture_node,lancer_charging::capture_name.data());fflush(log_file);
    }
}
void charging_step(hand::Frame const& source,hand::Decision const& decision) noexcept {
    lancer_charging::enabled=true;
    const auto result=lancer_charging::step(source,decision);
    if(lancer_charging::armed&&!charging_pin_known&&nativeequipment::current(nullptr,source)){
        gears_hand_continuity::TrackedInput tracked{};gears_hand_continuity::Publisher publisher{};
        if(nativeequipment::read_input_continuity(source,tracked,publisher)&&
           charging_input.bind(source.stereo,tracked,publisher,weaponhand::clock_ns(nullptr),
               source.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated,hand::left_control(source))&&
           weaponhand::client.capture_existing_action_lifetime_on_gt(source.key,charging_lifetime)){
            charging_parent=source;charging_hold=vrequip::model.hold_generation();charging_pin_known=true;
        }
    }else if(!lancer_charging::armed)clear_charging_pin();
    report_charging_gate(lancer_charging::failure,source);
    if(result==lancer_charging::Result::recovery_required){cleanup(__LINE__);return;}
    if(result!=lancer_charging::Result::complete)return;
    // Actual FRONT handle returned closed after a real full hand pull. Capture
    // this endpoint once; a UI deadline or inventory insertion cannot do it.
    if(standalone_charging){
        if(!lancer_charging::restore())return;
        if(log_file){fwprintf(log_file,L"lancer_charging_complete standalone=1 weapon=%llx reward=none\n",source.weapon.address);fflush(log_file);}
        lancer_charging::clear(source);reset_bookkeeping();return;
    }
    std::int64_t closed_tick{};std::int64_t window=0;
    if(lancerreload_state::lease.acquired){
        if(lancerreload_state::closure_time(source,reload_serial,closed_tick))window=perfect_window_ms;
    }else{
        float now{};
        if(nativeequipment::current(nullptr,source)&&inv::identity(source)==parked_lease.identity&&
           lancerreload_state::time_field(source.world.address,L"TimeSeconds",now)&&now>=parked_lease.reward_start){
            const double elapsed=std::ceil((double(now)-parked_lease.reward_start)*1000.);
            if(elapsed>=0&&elapsed<double(INT64_MAX-parked_lease.accepted_begin_ms)){
                closed_tick=parked_lease.accepted_begin_ms+std::int64_t(elapsed);window=perfect_window_ms;
            }
        }
    }
    if(!closed_tick)closed_tick=source.controllers.sync_ns/1000000;
    if(!charging_closed_tick){charging_closed_tick=closed_tick;charging_closed_window=window;}
    if(!lancer_charging::restore())return;
    inv::Request request{};
    if(!transaction.confirm_closed(charging_closed_tick,charging_closed_window)||!transaction.take_request(request)||
       request.call!=gears_weapon::ReloadTxnCall::finish){cleanup(__LINE__);return;}
    lancer_charging::clear(source);completion=request;stage=Stage::completing;
    if(log_file){fwprintf(log_file,L"lancer_charging_complete serial=%llu tick=%lld window=%lld result=%u\n",
        reload_serial,closed_tick,window,unsigned(request.result));fflush(log_file);}
}
void step(uptr pc) noexcept {
    lancerreload_ui::service();
    if(!lancerreload_contact::dispatch_enabled)return;
    if(lancerreload_state::lease.acquired&&!lancerreload_state::tick_ui_timing()){
        cleanup(__LINE__);return;
    }
    if(stage==Stage::completing){
        inv::Inventory after{};
        // Closure was authenticated before dispatch. Finishing that existing
        // action must not wait forever for a new tracked frame after release,
        // tracking loss, or a stop request. No new hand action is admitted here.
        if(!completion_exited){
            const auto& owned=lancerreload_state::lease;
            // Save read-only drawing metadata before the physical Reloading
            // lease is restored. Only the confirmed original reward below can
            // activate its short result display; this is no action authority.
            lancerreload_ui::stage_completion();
            if(!owned.acquired||owned.serial!=reload_serial||!(owned.identity==completion.identity)||
               !lancerreload_state::recover_serial(reload_serial)){
                transaction.require_reconciliation();cleanup(__LINE__);return;
            }
            completion_exited=true;return;
        }
        if(!observer.sample_owned(completion.identity,after)){retire_destroyed_weapon();return;}
        if(completion.result==gears_weapon::ReloadTxnResult::perfect&&
           !lancerreload_state::perfect_reward(completion,opening_inventory,observer,reward_receipt)){
            // The original reward already succeeded. Keep its receipt and
            // retry only authenticated hand-back/observation, never the score
            // callback or ammo setters. Cleanup cannot acknowledge it yet.
            if(reward_receipt.dispatched&&reward_receipt.call_confirmed&&!reward_receipt.failed)return;
            transaction.require_reconciliation();cleanup(__LINE__);return;
        }
        if(!observer.sample_owned(completion.identity,after)){retire_destroyed_weapon();return;}
        const auto applied=completion.result==gears_weapon::ReloadTxnResult::inspection?
            gears_weapon::ReloadTxnResult::none:completion.result;
        const bool confirmed=transaction.acknowledge({completion.token,true,after,applied});
        if(confirmed&&completion.result==gears_weapon::ReloadTxnResult::perfect&&reward_receipt.applied)
            lancerreload_ui::confirm_completion(completion.identity,reload_serial);
        if(!confirmed)transaction.require_reconciliation();
        cleanup(__LINE__,confirmed&&completing_magazine_seat);return;
    }
    if(stop_requested.load(std::memory_order_acquire)){
        if(stage!=Stage::idle||pending_start.frame)cleanup(__LINE__);return;
    }
    if(stage==Stage::idle&&pending_start.frame){
        const auto now=weaponhand::clock_ns(nullptr);
        if(now<=0||now>pending_start.until_ns)cancel_preparation();
    }
    if(stage==Stage::recovering){cleanup(__LINE__);return;}
    if(park_pending){if(!park_owned_reload(park_preserve))cleanup(__LINE__);return;}
    if(equipment_switch_dispatched){
        equipment_switch_dispatched=false;
        if(stage!=Stage::parked&&!park_owned_reload())cleanup(__LINE__);
        return;
    }
    hand::Frame source{};hand::GripGameState game{};hand::Decision decision{};
    hand::FrameReadObservation frame_read{};
    const bool frame_ready=weaponhand::client.copy_frame_on_gt(source,game,decision,&frame_read);
    last_frame_read_observation=frame_read;
    if(stage==Stage::charging&&(!frame_ready||!source.left_tracked)&&!charging_gap_known){
        // Preserve the FIRST existing failed check. Later unarmed reads are
        // consequences and must not conceal whether this began with stale
        // input, an actor/game change, or an unavailable publication.
        first_charging_gap=frame_read;charging_gap_began=weaponhand::clock_ns(nullptr);charging_gap_known=true;
    }
    if(frame_ready&&source.left_tracked){charging_gap_known=false;charging_gap_began=0;}
    if(!frame_ready&&stage==Stage::idle&&pending_start.frame){
        // A newly arbitrated grip can precede its matching actual hand result.
        // This software-only wait owns no component/pose/ammunition. Its
        // ORIGINAL250ms deadline never becomes a substitute action grant.
        const auto now=weaponhand::clock_ns(nullptr);
        if(now>0&&now<=pending_start.until_ns)return;
        cancel_preparation();
    }
    // Keep the authenticated INPUT's original clock, not a geometry clock.
    // Waiting at the shoulder need not produce a magazine-contact sample.
    // A self-state change after a fresh grip grant can revoke one publication;
    // old contact geometry must not turn that brief gap into tracking loss.
    if(frame_ready&&source.left_tracked)last_input_sync_ns=source.controllers.sync_ns;
    if(!frame_ready&&stage!=Stage::idle&&retire_destroyed_weapon())return;
    if(stage==Stage::parked){
        if(!frame_ready){if(parked_charging)report_charging_gate(100+unsigned(frame_read.failure),source);retire_destroyed_weapon();return;}
        const auto old=transaction.inventory().identity;
        if(!hand::same(source.world,old.world)||!hand::same(source.pawn,old.pawn)||
           source.key.lifecycle<old.lifecycle||source.key.game_start!=old.game_start||
           source.key.native_started_qpc!=old.native_start||source.key.registration_cookie!=old.registration||
           source.key.world_generation!=old.world_generation||source.stereo.session!=old.session||
           source.stereo.space_epoch!=old.space||source.controllers.input_epoch!=old.input){cleanup(__LINE__);return;}
        if(!hand::same(source.weapon,old.weapon)||!game.main_hand_held||game.busy||
           game.animation_owns_hands||!source.left_tracked){if(parked_charging)report_charging_gate(120,source);return;}
        if(!parked_charging&&parked_display_hold!=vrequip::model.hold_generation()){
            // Reapply the same parked cosmetic ownership once after a real
            // new equipment hold. Do not poll setters every tick or resume a
            // transaction/action from the old hand publication.
            bool changed{};
            if(!inv::same_instance_across_hold(old,inv::identity(source))||
               !hand::same(lancerreload_visual::saved.weapon,old.weapon)||
               !nativeequipment::current(nullptr,source))return;
            if(!lancerreload_visual::hide_parked(changed)){cleanup(__LINE__);return;}
            parked_display_hold=vrequip::model.hold_generation();
            if(changed)return; // Original setter retires this input frame.
        }
        // Prepare actual mechanism contact BEFORE the engine inventory
        // queries. Repeating those while idle can retire every publication
        // before geometry/release is seen, so no charging grant can form.
        // Reconciliation below runs only for a fresh granted grasp, bounded
        // by its original pending deadline if its own query retires a frame.
        if(parked_charging){
            lancer_charging::enabled=true;
            if(!lancer_charging::current_geometry(source)){lancer_charging::discover(source);report_charging_gate(200+lancer_charging::failure,source);return;}
            if(!source.left_grip_known){report_charging_gate(121,source);return;}
            if(!source.left_grip_down){resume_release_seen=lancer_charging::release_seen=true;report_charging_gate(122,source);return;}
            if(!resume_release_seen||!lancer_charging::fresh_grant(source,decision)){report_charging_gate(123,source);return;}
        }else{
            // Publish the shoulder candidate and observe release before
            // querying the inventory, just as the mechanism path does.
            // A lifecycle interruption must not leave the shoulder invisible
            // until a preflight has already consumed its grab publication.
            lancerreload_contact::waiting_ammo=true;
            if(!source.left_grip_known)return;
            if(!source.left_grip_down){resume_release_seen=true;return;}
            if(!resume_release_seen||!decision.new_grant||decision.release_required||
               decision.lease.kind!=hand::LeaseKind::ammo)return;
        }
        auto resumed_observer=observer;inv::Inventory resumed{};
        if(!resumed_observer.rebind_hold(source,old,resumed)){if(parked_charging)report_charging_gate(124,source);return;}
        auto resumed_transaction=transaction;
        if(parked_charging){
            const bool retained_state=lancerreload_state::lease.acquired;
            if(retained_state&&(!(lancerreload_state::lease.identity==old)||
               old.hold_generation!=resumed.identity.hold_generation||
               !lancerreload_state::owned_state())){cleanup(__LINE__);return;}
            if(!resumed_transaction.rebind_suspended(old,resumed)){report_charging_gate(125,source);return;}
            if(!resumed_transaction.resume(resumed,source.press_token,reload_serial)){report_charging_gate(126,source);return;}
            if(!nativeequipment::current(nullptr,source)){report_charging_gate(127,source);return;}
            if(retained_state)lancerreload_state::lease.identity=resumed.identity;
            observer=resumed_observer;transaction=resumed_transaction;opening_inventory.identity=resumed.identity;
            parked_lease.identity=resumed.identity;parked_charging=false;stage=Stage::charging;
            charging_step(source,decision);return;
        }
        // Resume intent is the first fresh hand action, not the equipment
        // switch itself. Keep the parked state until that explicit release/grab.
        if(!resumed_transaction.rebind_suspended(old,resumed)||
           !resumed_transaction.resume(resumed,source.press_token,reload_serial))return;
        // Native reacquisition invalidates the publication. Retain this
        // authenticated new ammo grant first so the next arbitration can keep
        // it after the hand leaves the shoulder, just as normal pickup does.
        if(!lancerreload_contact::retain(source,decision)){cleanup(__LINE__);return;}
        std::uint64_t serial{};
        if(lancerreload_state::begin(source,serial,&parked_lease)!=lancerreload_state::BeginResult::acquired){
            cleanup(__LINE__);return;
        }
        observer=resumed_observer;transaction=resumed_transaction;
        opening_inventory.identity=resumed.identity;
        stage=Stage::waiting_ammo;tracking_suspended=false;resume_release_seen=false;
        ui_config_pending=reload_serial!=0;
        resumed_grab=source.press_token;
        return; // Native state acquisition may retire this input publication.
    }
    if(frame_ready&&stage!=Stage::idle&&
       (!hand::same(source.weapon,standalone_charging?standalone_identity.weapon:transaction.inventory().identity.weapon)||
        (game.known&&!game.main_hand_held))){
        if(!park_owned_reload())cleanup(__LINE__);
        return;
    }
    // Native reload calls retire the action publication. A brief missing
    // publication is not a tracking-loss edge. Await a NEW authorized frame;
    // do not perform any gesture, setter or inventory work from the old one.
    // The last authenticated input's ORIGINAL deadline bounds this wait.
    // This timestamp never grants a gesture or substitutes for actual geometry.
    if(!frame_ready&&stage!=Stage::idle){
        if(retain_magazine_during_pose_gap(pc))return;
        const auto age=weaponhand::clock_ns(nullptr)-last_input_sync_ns;
        if(last_input_sync_ns>0&&age>=0&&age<=100000000)return;
    }
    if((!frame_ready||!source.left_tracked)&&stage==Stage::charging){
        if(!frame_ready&&retain_charging_during_pose_gap(pc))return;
        if(log_file){fwprintf(log_file,L"lancer_charging_park first_failure=%u first_frame=%llu first_sequence=%llu first_age_ns=%lld first_difference=%u last_failure=%u last_input_age_ns=%lld gap_ns=%lld tracked=%u\n",
            unsigned(first_charging_gap.failure),first_charging_gap.frame,first_charging_gap.sequence,
            first_charging_gap.input_age_ns,first_charging_gap.game_difference,unsigned(frame_read.failure),
            weaponhand::clock_ns(nullptr)-last_input_sync_ns,weaponhand::clock_ns(nullptr)-charging_gap_began,unsigned(source.left_tracked));fflush(log_file);}
        if(park_owned_reload(true))return;
    }
    if((!frame_ready||!source.left_tracked)&&suspend_replacement(frame_ready?2u:1u))return;
    if(!frame_ready||source.controller.address!=pc||
       !game.known||!game.main_hand_held||game.busy||game.animation_owns_hands||
       !source.left_tracked){
        if(stage==Stage::idle)cancel_preparation();
        if(stage!=Stage::idle){
            if(log_file){fwprintf(log_file,L"lancer_reload_input_stop frame_ready=%u controller_match=%u known=%u held=%u busy=%u animation=%u left_tracked=%u read_failure=%u game_difference=%u\n",
                unsigned(frame_ready),unsigned(source.controller.address==pc),unsigned(game.known),unsigned(game.main_hand_held),
                unsigned(game.busy),unsigned(game.animation_owns_hands),unsigned(source.left_tracked),
                unsigned(frame_read.failure),frame_read.game_difference);fflush(log_file);}
            cleanup(__LINE__);
        }return;
    }
    if(stage==Stage::idle&&lancer_charging::required(source)){
        if(!lancerreload_visual::active&&!lancerreload_state::lease.acquired){
            standalone_charging=true;standalone_identity=inv::identity(source);
            stage=Stage::charging;lancer_charging::enabled=true;
            outstanding.store(true,std::memory_order_release);
        }
    }
    if(stage==Stage::charging&&standalone_charging){charging_step(source,decision);return;}
    if(stage!=Stage::idle&&!(inv::identity(source)==transaction.inventory().identity)){
        const auto before=transaction.inventory().identity,after=inv::identity(source);
        // Losing an action publication advances the hand lifecycle even when
        // the world, pawn, weapon, hold and tracking session are unchanged.
        // Preserve the already-owned reload just as for an equipment switch;
        // never use this new frame to revive its old hand grant or insertion.
        // The parked path requires a fresh release/grab, exact inventory read
        // and coordinated observer/ledger rebind before it can resume.
        if(after.lifecycle>before.lifecycle&&after.hold_generation==before.hold_generation&&
           inv::same_instance_across_hold(before,after)&&stage==Stage::holding_original&&tracking_suspended){
            // A head-tracking interruption already froze this actual original
            // magazine. Do not turn a new hand lifecycle into equipment stow:
            // that would hide the retained item and require unrelated ammo.
            // Reconcile private identity/count bookkeeping from THIS fresh
            // source only. The suspended branch below still requires a real
            // tracked release/new magazine grant before any pose or insertion.
            // A full/no-reserve inspection deliberately has no ForceReload
            // lease or reward clock. It still owns its actual old magazine.
            // Reject a missing required lease OR an unrelated acquired one;
            // never invent native reload ownership for an inspection.
            const bool has_reload=reload_serial!=0;
            if(!lancerreload_visual::alive()||!lancerreload_visual::last_grab_known||
               lancerreload_state::lease.acquired!=has_reload||
               (has_reload&&(!(lancerreload_state::lease.identity==before)||!lancerreload_state::owned_state()))||
               (!has_reload&&transaction.target()!=0)){cleanup(__LINE__);return;}
            auto rebound_observer=observer;inv::Inventory rebound{};
            if(!rebound_observer.rebind_hold(source,before,rebound))return;
            auto rebound_transaction=transaction;
            if(!rebound_transaction.rebind_retained_original(before,rebound,original_item)){
                cleanup(__LINE__);return;
            }
            if(!nativeequipment::current(nullptr,source)||(has_reload&&!lancerreload_state::owned_state()))return;
            observer=rebound_observer;transaction=rebound_transaction;
            opening_inventory.identity=rebound.identity;
            if(has_reload)lancerreload_state::lease.identity=rebound.identity;
            clear_magazine_input_pin();resumed_grab=0;resume_release_seen=false;
            if(log_file){fwprintf(log_file,L"lancer_reload_original_input_rebound previous=%llu current=%llu serial=%llu loaded=%u reserve=%u old_item=%llu insertion_granted=0\n",
                before.lifecycle,after.lifecycle,reload_serial,rebound.loaded,rebound.reserve,original_item);fflush(log_file);}
            return;
        }
        if(after.lifecycle>before.lifecycle&&after.hold_generation==before.hold_generation&&
           inv::same_instance_across_hold(before,after)&&park_owned_reload(stage==Stage::charging)){
            if(log_file){fwprintf(log_file,L"lancer_reload_park_input_boundary previous=%llu current=%llu serial=%llu loaded=%u reserve=%u\n",
                before.lifecycle,after.lifecycle,reload_serial,transaction.inventory().loaded,transaction.inventory().reserve);fflush(log_file);}
            return;
        }
        cleanup(__LINE__);return;
    }
    if(ui_config_pending&&reload_serial){
        // ForceReload/reacquisition can revoke its input publication. Configure
        // UI only on this later authenticated frame, and consume no hand
        // gesture from a publication changed by the getter/property work.
        if(!lancerreload_state::set_ui_timing(source,reload_serial,
            opening_inventory.loaded?opening_policy.tactical_ms:opening_policy.empty_ms,perfect_window_ms,
            opening_policy.mode!=gears_weapon::LancerPerfectMode::original)){
            if(log_file){fwprintf(log_file,L"lancer_reload_ui_setup_stop serial=%llu gate=%u\n",reload_serial,lancerreload_state::ui_setup_gate);fflush(log_file);}
            // Display admission is independent of accepted physical loading.
            // Once the scoped weapon timing and no-fumble bit are configured,
            // an unavailable/unrecognized HUD must not cancel the magazine or
            // refund committed ammunition. Keep all typed HUD guards and its
            // partial journal for normal cleanup; actual display stays pending.
            const auto& ui=lancerreload_state::lease;
            if(ui.acquired&&ui.serial==reload_serial&&ui.ui_owned&&ui.attempts_owned&&
               lancerreload_state::ui_setup_gate>=6){
                ui_config_pending=false;
                if(log_file){fwprintf(log_file,L"lancer_reload_ui_display_pending serial=%llu policy_configured=1 actual_draw_unverified=1\n",reload_serial);fflush(log_file);}
                return;
            }
            cleanup(__LINE__);return;
        }
        ui_config_pending=false;return;
    }
    if(stage==Stage::charging){charging_step(source,decision);return;}
    if(tracking_suspended){
        // Require a tracked release and then a newly arbitrated shoulder grab.
        // A held button on the first recovered pose cannot consume ammunition.
        if(!source.left_grip_known)return;
        if(!source.left_grip_down){resume_release_seen=true;return;}
        const auto resume_kind=stage==Stage::holding_original?hand::LeaseKind::magazine:hand::LeaseKind::ammo;
        if(!resume_release_seen||decision.status!=hand::DecisionStatus::granted||
           !decision.new_grant||decision.release_required||decision.lease.kind!=resume_kind)return;
        if(stage==Stage::holding_original){
            gears_weapon::LancerInsertionSample resumed{};
            if(!lancerreload_visual::regrasp_original(source)||
               !lancerreload_contact::sample(source,resumed)||!insertion.arm(resumed)||
               !lancerreload_contact::retain(source,decision)){cleanup(__LINE__);return;}
            // This is a newly authenticated real grasp, so its existing-input
            // lifetime must replace the pre-interruption pin as well. A skin
            // reattachment alone cannot retain it through the next pose gap.
            pin_existing_magazine_grasp(source);
            insertion.observe(resumed);
            motion_samples.begin(unsigned(stage),source.press_token,resumed.sequence);
            lancerreload_contact::resume_original=false;
            tracking_suspended=false;resume_release_seen=false;
            return; // A regrab sample cannot also close the gun.
        }
        tracking_suspended=false;resume_release_seen=false;
    }
    if(!source.left_grip_down){
        resumed_grab=0;
        if(stage==Stage::waiting_ammo)lancerreload_contact::release();
        if(stage==Stage::holding_original){
            if(!transaction.stow_original(original_item)||!lancerreload_visual::stow()){cleanup(__LINE__);return;}
            lancerreload_contact::release();lancerreload_contact::waiting_ammo=true;
            stage=Stage::waiting_ammo;return;
        }
        if(stage==Stage::holding_new){
            if(!transaction.release_item(replacement_item)||!lancerreload_visual::stow()){cleanup(__LINE__);return;}
            lancerreload_contact::release();lancerreload_contact::waiting_ammo=true;
            stage=Stage::waiting_ammo;return;
        }
        if(stage!=Stage::idle&&stage!=Stage::waiting_ammo)cleanup(__LINE__);return;
    }
    const auto wanted=(stage==Stage::waiting_ammo||stage==Stage::holding_new)?
        hand::LeaseKind::ammo:hand::LeaseKind::magazine;
    const auto observe_wait=[&](unsigned phase)noexcept {
        if(stage!=Stage::waiting_ammo||!log_file)return;
        static std::uint64_t last=~0ull;static unsigned rows{};
        const auto key=std::uint64_t(phase)|(std::uint64_t(decision.status)<<8)|
            (std::uint64_t(decision.lease.kind)<<16)|(std::uint64_t(decision.release_required)<<24)|
            (std::uint64_t(lancerreload_contact::sample_failure)<<32);
        if(key==last||rows>=24)return;last=key;++rows;
        fwprintf(log_file,L"lancer_reload_wait phase=%u frame=%llu token=%llu resumed=%llu decision=%u kind=%u release=%u held=%u incumbent=%u,%u sample=%u\n",
            phase,source.key.frame,source.press_token,resumed_grab,unsigned(decision.status),unsigned(decision.lease.kind),
            decision.release_required,unsigned(lancerreload_contact::held.kind),unsigned(game.object_incumbents[0].kind),
            unsigned(game.object_incumbents[1].kind),lancerreload_contact::sample_failure);fflush(log_file);
    };
    observe_wait(1);
    if(source.press_token!=logged_press&&log_file){
        logged_press=source.press_token;const auto& g=lancerreload_contact::last_geometry;
        const auto& cohort=lancerreload_contact::cohort_failure;
        // This exact already-authenticated observation is also the committed
        // no-candidate observation (copy_frame compares it before returning).
        // Pure geometry/POD only, once per existing press row: distinguish a
        // known support miss from an unavailable competing part. No new game
        // read, sample, input edge or permission is introduced for this row.
        double support_distance=-1;
        const bool support_contact=hand::support_contact(source,game,false,&support_distance);
        fwprintf(log_file,L"lancer_reload_grip token=%llu decision=%u lease=%u sample_failure=%u match=%u depth=%.6g lateral=%.6g axis=%.6g clock=%.6g frame=%llu source_sequence=%llu source_age_ns=%lld support_contact=%u support_distance_m=%.9g mount_known=%u mount_valid=%u magazine_pending=%u magazine_candidate=%u ammo_candidate=%u control=%u cohort=%u pair_sequence=%llu request_sequence=%llu pair_sync_ns=%lld request_sync_ns=%lld pair_control=%u pair_active=%u request_active=%u pair_press=%u request_press=%u pair_release=%u request_release=%u pair_down=%u request_down=%u\n",
            logged_press,unsigned(decision.status),unsigned(decision.lease.kind),lancerreload_contact::sample_failure,
            lancerreload_contact::match_status,g.depth_raw,g.lateral_raw,g.axis_alignment,g.clock_alignment,
            source.key.frame,source.controllers.sequence,weaponhand::clock_ns(nullptr)-source.controllers.sync_ns,
            unsigned(support_contact),support_distance,unsigned(game.mount_known),unsigned(game.mount.valid()),
            unsigned(game.magazine_observation_pending),unsigned(game.request_magazine),unsigned(game.request_ammo),
            source.interaction_control,cohort.why,cohort.old_sequence,cohort.new_sequence,
            cohort.old_sync,cohort.new_sync,cohort.control,cohort.old_active,cohort.new_active,
            cohort.old_press,cohort.new_press,cohort.old_release,cohort.new_release,cohort.old_down,cohort.new_down);fflush(log_file);
        if(!lancerreload_contact::sample_failure){
            const auto& a=lancerreload_contact::last_actual;const auto p=a.left_grip.position,w=a.left_wrist.position;
            const auto r=a.right.right_grip.position,m=lancerreload_contact::last_magazine_position;
            fwprintf(log_file,L"lancer_reload_grip_geometry token=%llu serial=%llu scale=%.9g left=%.9g,%.9g,%.9g wrist=%.9g,%.9g,%.9g right=%.9g,%.9g,%.9g magazine=%.9g,%.9g,%.9g\n",
                logged_press,a.right.serial,a.model_scale,p.x,p.y,p.z,w.x,w.y,w.z,r.x,r.y,r.z,m.x,m.y,m.z);fflush(log_file);
        }
    }
    if(decision.lease.kind!=wanted||decision.release_required){
        if(magazine_pin_known&&!magazine_lease_stop.captured&&
           (stage==Stage::extracting||stage==Stage::holding_original||stage==Stage::holding_new)){
            const auto control=hand::left_control(source);const auto& levels=source.controllers.hands[0];
            magazine_lease_stop={source.key,decision.key,frame_read,weaponhand::client.withdrawal_on_gt(),
                decision.lease,{game.object_incumbents[0],game.object_incumbents[1]},source.press_token,decision.press_token,
                magazine_lifetime.epoch,magazine_lifetime.parent.key.lifecycle,
                weaponhand::clock_ns(nullptr)-source.controllers.sync_ns,unsigned(decision.status),decision.release_required,
                unsigned(wanted),control,levels.pressed[control],levels.released[control],
                unsigned(source.left_grip_known),unsigned(source.left_grip_down),unsigned(source.left_tracked),true};
        }
        if(stage==Stage::idle)cancel_preparation();
        // A revoked action lease is not evidence that the same old magazine
        // was discarded. Freeze it and require tracked release/new contact;
        // never move/insert it using this rejected decision.
        if(stage==Stage::holding_original&&suspend_replacement(4))return;
        if(stage!=Stage::idle&&stage!=Stage::waiting_ammo)cleanup(__LINE__);return;
    }
    if(stage==Stage::idle){
        const auto now=weaponhand::clock_ns(nullptr);
        if(pending_start.frame&&(!(pending_start.identity==inv::identity(source))||
           !hand::same(pending_start.lease,decision.lease)||source.press_token!=pending_start.lease.id||
           now<=0||now>pending_start.until_ns))cancel_preparation();
        // Retain the fresh accepted input association BEFORE asking for the
        // delayed actual geometry. Previously a sample_failure2 consumed the
        // only new_grant publication, so later valid retained frames could
        // never start. Every later action still requires its CURRENT sample.
        if(!pending_start.frame){
            if(!lancerreload_contact::mechanism_route.grants(source,decision,gears_weapon::LancerContact::magazine)||
               source.controllers.sync_ns<=0||source.controllers.sync_ns>INT64_MAX-250000000)return;
            pending_start={inv::identity(source),decision.lease,source.key.frame,0,source.controllers.sync_ns+250000000};
        }
    }
    if(stage==Stage::waiting_ammo&&!resumed_grab&&
       decision.status==hand::DecisionStatus::granted&&decision.new_grant){
        // The fresh shoulder grant exists for one publication. Retain that
        // exact grant before waiting for a new actual wrist result, just as
        // reacquisition does. No item reservation, pose write or ammo change
        // is performed from unavailable geometry; release above cancels it.
        if(!lancerreload_contact::retain(source,decision)){cleanup(__LINE__);return;}
        resumed_grab=source.press_token;return;
    }
    gears_weapon::LancerInsertionSample sample{};
    if(!lancerreload_contact::sample(source,sample)){
        observe_wait(2);
        // The input above is live and tracked. A revoked render result is not
        // tracking loss: wait for a NEW actual result without advancing a
        // gesture, moving the magazine or touching inventory. Release and real
        // source/tracking loss still follow their independent paths above.
        // Never reuse a cached pose or extend its acquisition deadline.
        if(lancerreload_contact::sample_failure==2)return;
        if(stage!=Stage::idle&&!suspend_replacement(3))cleanup(__LINE__);return;
    }
    if(stage==Stage::idle){
        if(!pending_start.frame)return;
        if(source.key.frame<pending_start.frame||pending_start.attempt_frame==source.key.frame||
           (decision.status!=hand::DecisionStatus::granted&&decision.status!=hand::DecisionStatus::retained)||
           !extraction.begin(sample,true))return;
        pending_start.attempt_frame=source.key.frame;
        if(!pin_existing_magazine_grasp(source))return;
        outstanding.store(true,std::memory_order_release);
        inv::Inventory inventory{};
        const bool inventory_ready=observer.sample(source,inventory)&&transaction.bind(inventory);
        const bool visual_ready=inventory_ready&&lancerreload_visual::begin(source,lancerreload_visual::Profile::lancer,
            &game,&decision,&lancerreload_contact::last_actual);
        if(!visual_ready){
            // A pre-write failure must not consume a held grab permanently.
            // Retry no original call from this frame; await a NEW publication
            // with the SAME actual current lease, identity and contact. Any
            // attempted mutation instead follows normal owned cleanup.
            if(!lancerreload_visual::active&&!lancerreload_state::lease.acquired&&
               !transaction.pending()&&(!inventory_ready||lancerreload_visual::begin_retryable)){
                observer={};transaction={};extraction.reset();return;
            }
            cleanup(__LINE__);return;
        }
        pending_start={};
        original_item=decision.lease.id;
        if(!hand::same(last_visual_weapon,source.weapon)){last_visual_weapon=source.weapon;notify_observations=0;}
        if(!lancerreload_contact::retain(source,decision)){cleanup(__LINE__);return;}
        stage=Stage::extracting;
        motion_samples.begin(unsigned(stage),source.press_token,sample.sequence);
        // Retaining changes the observed private game state. Consume the next
        // committed Native frame instead of reusing this frame as permission.
        return;
    }
    if(stage==Stage::waiting_ammo){
        observe_wait(3);
        const bool resumed=(resumed_grab&&source.press_token==resumed_grab&&
            (decision.status==hand::DecisionStatus::granted||decision.status==hand::DecisionStatus::retained))||
            lancerreload_contact::retained(source,decision);
        if(!resumed&&(decision.status!=hand::DecisionStatus::granted||!decision.new_grant))return;
        // With no spare delta there is no original Reloading lease. Re-read
        // the actual inventory before reusing its L rounds; never turn a stale
        // opening observation into a replacement or an empty-supply cue.
        if(!reload_serial){
            inv::Inventory current{};const auto expected=transaction.inventory();
            if(!observer.sample(source,current))return;
            if(!(current.identity==expected.identity)||current.capacity!=expected.capacity||
               current.loaded!=expected.loaded||current.reserve!=expected.reserve||
               current.infinite_reserve!=expected.infinite_reserve){cleanup(__LINE__);return;}
            if(!current.loaded&&!current.reserve&&!current.infinite_reserve){
                if(!nativeequipment::current(nullptr,source))return;
                const bool first=lancerreload_contact::empty_supply.reject(source.key.lifecycle,source.press_token);
                lancerreload_contact::release();resumed_grab=0;replacement_item=0;
                if(first){
                    gears_weapon::EquipmentSnapshot cue{};
                    cue.world=source.world.address;cue.pawn=source.pawn.address;cue.current_weapon=source.weapon.address;
                    bool played{};try{played=vrequip::play_empty_slot_sound(cue,4);}catch(...){}
                    if(log_file){fwprintf(log_file,L"lancer_reload_empty_supply token=%llu loaded=0 reserve=0 sound=%u replacement_created=0\n",
                        source.press_token,unsigned(played));fflush(log_file);}
                }
                return;
            }
        }
        replacement_item=decision.lease.id;
        const auto acquire=[&]()noexcept {
            if(reload_serial?!lancerreload_state::owns(source,reload_serial):
               lancerreload_state::lease.acquired||transaction.target()||
               !(transaction.inventory().identity==inv::identity(source))||!nativeequipment::current(nullptr,source))return 1u;
            if(!transaction.reserve_magazine_reusing_loaded(replacement_item))return 2u;
            if(!lancerreload_visual::take_replacement(source))return 3u;
            if(!insertion.arm(sample))return 4u;
            if(!lancerreload_contact::retain(source,decision))return 5u;
            return 0u;
        };
        if(const auto failure=acquire()){
            if(log_file){fwprintf(log_file,L"lancer_reload_replacement_stop phase=%u visual=%u age_ns=%lld\n",
                failure,lancerreload_visual::replacement_failure,
                weaponhand::clock_ns(nullptr)-source.controllers.sync_ns);fflush(log_file);}
            cleanup(__LINE__);return;
        }
        if(log_file){fwprintf(log_file,L"lancer_reload_supply token=%llu rounds=%u spare_reserved=%u loaded=%u reserve=%u\n",
            source.press_token,transaction.held_magazine_rounds(),transaction.reserved(),
            transaction.inventory().loaded,transaction.inventory().reserve);fflush(log_file);}
        resumed_grab=0;lancerreload_contact::waiting_ammo=false;insertion.observe(sample);stage=Stage::holding_new;
        pin_existing_magazine_grasp(source);
        motion_samples.begin(unsigned(stage),source.press_token,sample.sequence);return;
    }
    if((stage==Stage::holding_original||stage==Stage::holding_new)&&source.left_grip_down)
        pin_existing_magazine_grasp(source);
    if(!lancerreload_visual::move(source)){
        if((lancerreload_visual::move_failure==2||(lancerreload_visual::move_failure==3&&
            (lancerreload_visual::interaction_failure==2||lancerreload_visual::interaction_failure==3)))&&
           retain_magazine_during_pose_gap(pc))return;
        // The first contact read above can succeed and the later visual read
        // be revoked by an original animation/publication boundary. No setter
        // has run at gate3. Await a NEW current result, just as sample() does;
        // do not cancel the held item or consume queued travel from this one.
        // Every following tick still checks live input, lease and identity.
        if(lancerreload_visual::move_failure==3&&
           (lancerreload_visual::interaction_failure==2||lancerreload_visual::interaction_failure==3))return;
        if(log_file){fwprintf(log_file,L"lancer_reload_move_stop gate=%u frame=%llu age_ns=%lld\n",
            lancerreload_visual::move_failure,source.key.frame,
            weaponhand::clock_ns(nullptr)-source.controllers.sync_ns);fflush(log_file);}
        cleanup(__LINE__);return;
    }
    if(stage==Stage::extracting){
        bool extracted{};
        if(!motion_samples.consume(unsigned(stage),source.press_token,sample,[&](auto const& observed){
            if(!extracted&&extraction.active()){
                extracted=extraction.observe(observed);
                gears_weapon::LancerInsertionGeometry g{};
                gears_weapon::lancer_insertion_geometry(observed,g);
                extraction_observations[extraction_observation_count++%32]={observed.sequence,observed.created_ms,
                    g.depth_raw,g.lateral_raw,extraction.failure().reason,extraction.active(),extracted};
            }
        })){cleanup(__LINE__);return;}
        if(!extracted){
            if(!extraction.active()){
                const auto& f=extraction.failure();
                if(log_file){fwprintf(log_file,L"lancer_reload_extraction_stop reason=%u previous_sequence=%llu sequence=%llu previous_ms=%llu ms=%llu depth=%.6g lateral=%.6g axis=%.6g clock=%.6g travel=%.6g\n",
                    f.reason,f.previous_sequence,f.sequence,f.previous_ms,f.ms,f.depth,f.lateral,f.axis,f.clock,f.travel);fflush(log_file);}
                cleanup(__LINE__);
            }
            return;
        }
        gears_weapon::LancerInsertionGeometry current_geometry{};
        report_extraction();
        clear_magazine_input_pin();
        if(!gears_weapon::lancer_insertion_geometry(sample,current_geometry)||
           current_geometry.depth_raw<4.5||current_geometry.depth_raw>25||
           !gears_weapon::lancer_extraction_alignment(current_geometry,sample.units_per_metre)){
            cleanup(__LINE__);return;
        }
        inv::Inventory before{};
        if(!observer.sample(source,before)){cleanup(__LINE__);return;}
        const bool needs_reload=before.loaded<before.capacity&&(before.infinite_reserve||before.reserve);
        if(needs_reload&&!lancerreload_state::capture_reward_eligibility(before,reward_receipt)){
            cleanup(__LINE__);return;
        }
        if(needs_reload&&lancerreload_state::begin(source,reload_serial)!=lancerreload_state::BeginResult::acquired){
            cleanup(__LINE__);return;
        }
        opening_inventory=before;
        gears_weapon::LancerReloadPolicy policy{};
        if(!gears_weapon::LancerReloadPolicy::unpack(configured_policy.load(std::memory_order_acquire),policy)){
            cleanup(__LINE__);return;
        }
        perfect_window_ms=reload_serial?policy.deadline(before.loaded==0,lancerreload_state::lease.perfect_end_ms):0;
        opening_policy=policy;ui_config_pending=reload_serial!=0;
        if(!transaction.begin(before,source.press_token,gears_weapon::ReloadIntentSource::magazine_pull,
            reload_serial,gears_weapon::ReloadTxnMode::magazine,before.capacity,
            reload_serial?lancerreload_state::lease.accepted_begin_ms:std::int64_t(sample.created_ms),original_item)||!insertion.arm(sample)){
            cleanup(__LINE__);return;
        }
        insertion.observe(sample);stage=Stage::holding_original;
        motion_samples.begin(unsigned(stage),source.press_token,sample.sequence);return;
    }
    auto insertion_result=gears_weapon::LancerInsertResult::rejected;
    if(!motion_samples.consume(unsigned(stage),source.press_token,sample,[&](auto const& observed){
        if(insertion_result!=gears_weapon::LancerInsertResult::inserted){
            insertion_result=insertion.observe(observed);
            gears_weapon::LancerInsertionGeometry geometry{};
            gears_weapon::lancer_insertion_geometry(observed,geometry);
            insertion_observations[insertion_observation_count++%32]={observed.sequence,observed.created_ms,
                geometry.depth_raw,geometry.lateral_raw,unsigned(insertion_result)};
        }
    })){cleanup(__LINE__);return;}
    if(insertion_result==gears_weapon::LancerInsertResult::inserted){
        gears_weapon::LancerInsertionGeometry current_geometry{};
        if(!gears_weapon::lancer_insertion_geometry(sample,current_geometry)||current_geometry.depth_raw<-.02*sample.units_per_metre||
           current_geometry.depth_raw>2.5||current_geometry.lateral_raw>4||
           current_geometry.axis_alignment<.85||current_geometry.clock_alignment<.8){cleanup(__LINE__);return;}
    }
    if(log_file){
        static std::uint64_t traced_serial{},traced_sequence{};static unsigned traced_rows{};
        if(traced_serial!=reload_serial){traced_serial=reload_serial;traced_sequence=0;traced_rows=0;}
        gears_weapon::LancerInsertionGeometry geometry{};
        if(traced_sequence!=sample.sequence&&traced_rows<64&&gears_weapon::lancer_insertion_geometry(sample,geometry)){
            fwprintf(log_file,L"lancer_reload_motion stage=%u serial=%llu sequence=%llu created_ms=%llu result=%u depth=%.9g lateral=%.9g axis=%.9g clock=%.9g\n",
                unsigned(stage),reload_serial,sample.sequence,sample.created_ms,unsigned(insertion_result),
                geometry.depth_raw,geometry.lateral_raw,geometry.axis_alignment,geometry.clock_alignment);fflush(log_file);
            traced_sequence=sample.sequence;++traced_rows;
        }
    }
    if(stage==Stage::holding_original&&insertion_result==gears_weapon::LancerInsertResult::inserted){
        if(!transaction.restore_original(original_item)||!transaction.confirm_closed(std::int64_t(sample.created_ms))){
            cleanup(__LINE__);return;
        }
        if(reload_serial){
            inv::Request request{};
            if(!transaction.take_request(request)||request.call!=gears_weapon::ReloadTxnCall::exit_without_fill){cleanup(__LINE__);return;}
            completing_magazine_seat=true;completion=request;stage=Stage::completing;return;
        }
        // This is original-magazine cancellation, never a refill/reward.
        cleanup(__LINE__,true);
    }
    if(stage==Stage::holding_new&&insertion_result==gears_weapon::LancerInsertResult::inserted){
        if(!reload_serial){
            inv::Inventory current{};const auto expected=transaction.inventory();
            if(!observer.sample(source,current)||!(current.identity==expected.identity)||
               current.capacity!=expected.capacity||current.loaded!=expected.loaded||
               current.reserve!=expected.reserve||current.infinite_reserve!=expected.infinite_reserve||
               !nativeequipment::current(nullptr,source)||!transaction.seat_reused_rounds(replacement_item)||
               !transaction.confirm_closed(std::int64_t(sample.created_ms))){cleanup(__LINE__);return;}
            // A new physical magazine containing the conserved old rounds:
            // no native insertion setter, ForceReload, reward or charging debt.
            if(log_file){fwprintf(log_file,L"lancer_reload_supply_seated token=%llu rounds=%u loaded=%u reserve=%u inventory_changed=0\n",
                source.press_token,current.loaded,current.loaded,current.reserve);fflush(log_file);}
            cleanup(__LINE__,true);return;
        }
        // Unknown game time permits ordinary completion, never an invented
        // perfect reward. Preserve this closure timestamp across setter work.
        std::int64_t closed_tick=std::int64_t(sample.created_ms);
        const auto closing_window=lancerreload_state::closure_time(source,reload_serial,closed_tick)?perfect_window_ms:0;
        const bool empty=opening_inventory.loaded==0;
        if(empty&&!lancer_charging::can_mark(source)){cleanup(__LINE__);return;}
        inv::Request request{};inv::Receipt receipt{};
        if(!transaction.request_insertion(replacement_item)||!transaction.take_request(request)){cleanup(__LINE__);return;}
        const auto result=writer.commit(source,request,observer,lancerreload_state::owns,receipt);
        if(result!=inv::CommitResult::committed&&log_file){
            fwprintf(log_file,L"lancer_reload_insert_stop phase=%u result=%u receipt=%llu sync_age_ns=%lld\n",
                writer.phase,unsigned(result),receipt.token,weaponhand::clock_ns(nullptr)-source.controllers.sync_ns);fflush(log_file);
        }
        // Set the restriction even for an ambiguous setter receipt. Recovery
        // may prove that ammunition committed after the action frame retired.
        // This never grants firing or a reward and does not repeat a setter.
        if(empty&&(result==inv::CommitResult::committed||result==inv::CommitResult::reconciliation_required))
            lancer_charging::mark(source);
        if(result==inv::CommitResult::reconciliation_required){transaction.require_reconciliation();cleanup(__LINE__);return;}
        // A pre-dispatch rejection may not have filled a receipt. Authenticate
        // unchanged counts before retiring that request; ambiguity remains held.
        if(!receipt.token){receipt.token=request.token;observer.sample(source,receipt.observed);}
        if(!transaction.acknowledge(receipt)||result!=inv::CommitResult::committed){cleanup(__LINE__);return;}
        if(empty){
            if(!lancerreload_visual::restore()){cleanup(__LINE__);return;}
            lancerreload_contact::seated_hold=true;replacement_item=0;insertion.reset();motion_samples.clear();
            lancer_charging::release_seen=false;lancer_charging::enabled=true;
            stage=Stage::charging;return;
        }
        if(!transaction.confirm_closed(closed_tick,closing_window)||!transaction.take_request(request)){
            cleanup(__LINE__);return;
        }
        if(request.call!=gears_weapon::ReloadTxnCall::finish||
           (request.result!=gears_weapon::ReloadTxnResult::ordinary&&request.result!=gears_weapon::ReloadTxnResult::perfect)){
            transaction.require_reconciliation();cleanup(__LINE__);return;
        }
        completing_magazine_seat=true;completion=request;stage=Stage::completing;
        if(log_file){fwprintf(log_file,L"lancer_reload_closed serial=%llu closed_tick=%lld perfect_window_ms=%lld result=%u\n",
            reload_serial,closed_tick,closing_window,unsigned(request.result));fflush(log_file);}
    }
}
bool idle_support_only(hand::Frame const& source,hand::GripGameState const& game,
    hand::Decision const& decision) noexcept {
    // A current exclusive support grant cannot start a magazine transaction.
    // Do not reopen the full action/game observer just to discover that fact.
    // Any owned work, pending start, empty-chamber debt or cleanup stays on the
    // original tick path. This does not retain/refresh an input or pose permit.
    if(stage!=Stage::idle||pending_start.frame||needs_priority()||stop_requested.load(std::memory_order_acquire)||
       park_pending||equipment_switch_dispatched||tracking_suspended||standalone_charging||parked_charging||
       magazine_pin_known||charging_pin_known||completing_magazine_seat||transaction.pending()||
       lancerreload_visual::active||lancerreload_state::lease.acquired||lancer_charging::required(source)||
       !nativelancerfollow::bound_is_lancer||!hand::same(nativelancerfollow::bound_key.weapon,source.weapon)||
       !game.known||!game.main_hand_held||game.busy||game.animation_owns_hands||
       !source.left_tracked||!source.left_grip_known||!source.left_grip_down||decision.release_required||
       (decision.status!=hand::DecisionStatus::granted&&decision.status!=hand::DecisionStatus::retained)||
       decision.lease.kind!=hand::LeaseKind::support||!hand::lease_valid(decision.lease,source.key.lifecycle)||
       decision.lease.id!=source.press_token||!nativeequipment::current(nullptr,source))return false;
    // Completed-result presentation has its own expiry; it still runs even
    // when there is no physical reload work to perform this frame.
    lancerreload_ui::service();
    return nativeequipment::current(nullptr,source);
}
void tick_selected(uptr pc) noexcept {
    const auto before=stage;
    step(pc);
    lancerreload_visual::extraction_display_enabled=!tracking_suspended&&!equipment_switch_dispatched&&stage==Stage::extracting;
    lancerreload_visual::set_insertion_assist(gears_lancer_workbench_profile::allow_insertion_assist()&&
        !tracking_suspended&&!equipment_switch_dispatched&&
        (stage==Stage::holding_original||stage==Stage::holding_new));
    if(before!=stage&&log_file){
        report_insertion();
        const auto counts=transaction.inventory();
        fwprintf(log_file,L"lancer_reload_transition from=%u to=%u serial=%llu loaded=%u reserve=%u pending=%u visual=%u infinite=%u\n",
            unsigned(before),unsigned(stage),reload_serial,counts.loaded,counts.reserve,
            unsigned(transaction.pending()),unsigned(lancerreload_visual::active),unsigned(counts.infinite_reserve));
        fflush(log_file);
    }
}
bool pending_work() noexcept {return stage!=Stage::idle||pending_start.frame||park_pending;}
// Existing original-state obligations are retired independently of selection.
void service_unselected(uptr pc,uptr selected) noexcept {
    const auto owner=transaction.inventory().identity.weapon.address;
    if((stage!=Stage::idle||pending_start.frame)&&(!owner||owner!=selected))tick_selected(pc);
}
void tick(uptr pc) noexcept; // Composition root, after concrete consumers.
bool prepare_render_result(gears_committed_hand::Frame const& result,hand::Frame& source) noexcept {
    // Collect the actual completed pair BEFORE gun component setters spend its
    // original acquisition deadline. This only queues observations for the
    // existing normal-tick gesture consumer; it grants no transition/writer.
    source={};
    if(tracking_suspended||equipment_switch_dispatched||
       (stage!=Stage::extracting&&stage!=Stage::holding_original&&stage!=Stage::holding_new)||
       !lancerreload_visual::active||!nativelancerfollow::bound_is_lancer)return false;
    if(!weaponhand::current_gt(nullptr))return false;
    // Record each current completed pair before the full action/game query.
    // The exact retained input association is only ancestry/counters: it does
    // not return a Frame, refresh its age, or authorize visual/inventory writes.
    gears_committed_interaction::Frame completed{};hand::RetainedItemInput associated{};
    if(handrig::read_interaction(completed)&&completed.right.serial==result.serial&&
       completed.right.compose==result.compose&&completed.right.tick==result.tick&&
       weaponhand::client.copy_retained_item_input_on_gt(gears_committed_interaction::equipment_identity(completed),associated)&&
       lancerreload_contact::retained_observation(completed,associated,transaction.inventory().identity)){
        gears_weapon::LancerInsertionSample sample{};
        if(lancerreload_contact::completed_sample(completed,true,sample))
            motion_samples.push(unsigned(stage),associated.press_token,sample);
    }
    hand::GripGameState game{};hand::Decision decision{};
    if(!weaponhand::current_gt(nullptr)||!weaponhand::client.copy_frame_on_gt(source,game,decision)||
       !source.left_tracked||!source.left_grip_known||!source.left_grip_down)return false;
    gears_committed_interaction::Frame actual{};
    if(!lancerreload_visual::interaction(source,actual)||actual.right.serial!=result.serial||
       actual.right.compose!=result.compose||actual.right.tick!=result.tick)return false;
    // Observe while the actual pose is still published, before original
    // animation invalidates it. Never dispatch transitions from this callback.
    if(game.known&&game.main_hand_held&&!game.busy&&!game.animation_owns_hands&&
       inv::identity(source)==transaction.inventory().identity&&
       lancerreload_contact::retained(source,decision)){
        gears_weapon::LancerInsertionSample sample{};
        if(lancerreload_contact::sample(source,sample))
            motion_samples.push(unsigned(stage),source.press_token,sample);
    }
    return true;
}
bool on_render_result(gears_committed_hand::Frame const& result,hand::Frame const& source) noexcept {
    // The source retains its original clocks and key. move() independently
    // rechecks the current owner, actual pair/geometry and component identity
    // after the gun update. A recursive boundary cannot revive this frame.
    if(tracking_suspended||equipment_switch_dispatched||
       (stage!=Stage::extracting&&stage!=Stage::holding_original&&stage!=Stage::holding_new)||
       !lancerreload_visual::active||!nativelancerfollow::bound_is_lancer||
       result.purpose!=gears_committed_hand::Purpose::held_lancer||
       result.parent.registration_cookie!=source.key.registration_cookie||
       result.parent.native_started_qpc!=source.key.native_started_qpc)return false;
    return lancerreload_visual::move(source);
}
bool pre_tick(uptr pc) noexcept {
    // This is a scheduling opportunity, not a retained geometry permission.
    // Re-read the current authenticated frame and actual bones; the normal
    // consumer still performs all lease, identity and inventory checks.
    // The first grant remains post-equipment. A read-only visual preparation
    // which already failed there may retry from its SAME admitted pending
    // grant on a NEW publication here, before PlayerTick ages/revokes the
    // actual contact. It retains the original250ms edge deadline. No cold
    // acquisition or new equipment/grip action is introduced by this path.
    // Once acquired, extraction
    // also needs the completed previous hand pose before animation revokes it;
    // postponing that stroke until post-tick can skip its actual motion samples.
    const bool initial_retry=stage==Stage::idle&&pending_start.frame;
    const bool charging_pre=stage==Stage::charging||(stage==Stage::parked&&parked_charging);
    if(!initial_retry&&!charging_pre&&stage!=Stage::extracting&&stage!=Stage::holding_original&&stage!=Stage::waiting_ammo&&stage!=Stage::holding_new)return false;
    const auto gate=[](unsigned reason,hand::FrameReadObservation const* observation=nullptr)noexcept {
        const unsigned failure=observation?unsigned(observation->failure):0,difference=observation?observation->game_difference:0;
        const unsigned signature=reason|(failure<<8)|(difference<<16);
        static unsigned last=~0u,rows{};
        if(last!=signature&&rows<24&&log_file){
            fwprintf(log_file,L"lancer_reload_pre_phase stage=%u gate=%u interaction=%u read_failure=%u game_difference=%u\n",
                unsigned(stage),reason,lancerreload_visual::interaction_failure,failure,difference);fflush(log_file);++rows;
        }
        last=signature;return reason==0;
    };
    if(!nativeequipment::domain_ready()||!nativelancerfollow::bound_is_lancer)return gate(1);
    hand::Frame source{};hand::GripGameState game{};hand::Decision decision{};hand::FrameReadObservation observation{};
    if(!weaponhand::current_gt(nullptr)||!weaponhand::client.copy_frame_on_gt(source,game,decision,&observation))return gate(2,&observation);
    if(source.controller.address!=pc||(!source.left_grip_down&&!needs_priority()))return gate(3);
    if(charging_pre){
        //45172: the actual front-handle grant was fresh, but postponing the
        // consumer until AFTER original PlayerTick aged/revoked it. The gap
        // then parked an untouched stroke and unnecessarily required regrasp.
        // Like extraction, service this existing charging transaction while
        // its current input is still fresh. tick/step retain all identity,
        // grant, original250ms deadline, <=100ms source and actual-upload
        // stroke/release/return checks. No cached pose or new edge is supplied.
        // Charging deliberately uses the authenticated physical grips rather
        // than a cosmetic C0 hand pair, exactly as its normal step does.
        if(!source.left_tracked||!source.left_grip_known||!game.known||!game.main_hand_held||
           game.busy||game.animation_owns_hands||!lancer_charging::required(source))return gate(6);
        gate(0);tick(pc);return true;
    }
    if(initial_retry){
        const auto now=weaponhand::clock_ns(nullptr);
        if(!source.left_tracked||!source.left_grip_down||!game.known||!game.main_hand_held||
           game.busy||game.animation_owns_hands||source.key.frame<=pending_start.attempt_frame||
           !(pending_start.identity==inv::identity(source))||!hand::same(pending_start.lease,decision.lease)||
           source.press_token!=pending_start.lease.id||decision.release_required||
           decision.lease.kind!=hand::LeaseKind::magazine||
           (decision.status!=hand::DecisionStatus::granted&&decision.status!=hand::DecisionStatus::retained)||
           now<=0||now>pending_start.until_ns)return gate(5);
    }
    gears_committed_interaction::Frame actual{};
    if(!lancerreload_visual::interaction(source,actual))return gate(4);
    if(stage!=Stage::waiting_ammo)lancerreload_visual::observe_actual_pair(actual);
    gate(0);
    tick(pc);
    return true;
}
}
