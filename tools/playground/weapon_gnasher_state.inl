// Normal-GT original Reloading owner. The physical dispatcher supplies a real
// fresh ammo grant; this adapter never creates input or a second native lease.
namespace gnasher_state {
namespace hand=gears_weapon_hand;namespace inv=lancerreload_inventory;
namespace mechanism=gears_weapon;
enum class Stage {idle,starting,feeding,exiting,awaiting_cycle,finishing,recovery_required,recovering,start_settlement,parking,parked,resuming,empty_exiting,empty_settlement};
struct Record {
    inv::Identity identity{};inv::Inventory opening{};inv::Request completion{};
    lancerreload_state::RewardReceipt reward{};
    lancerreload_state::Lease parked_lease{};inv::Inventory park_counts{},recovery_counts{};
    hand::Frame source{};
    Stage stage{};std::uint64_t serial{},last_intent{},exit_frame{},cycle_token{};
    bool known{},start_dispatched{},ui_pending{},parked_reload{},mechanical_parked{},native_exited{},recover_to_park{};
};
std::array<Record,16> records{};
Record* record(hand::Frame const& f,bool create=false) noexcept {
    const auto id=inv::identity(f);
    for(auto& r:records)if(r.known&&(r.identity==id||inv::same_instance_across_hold(r.identity,id)))return &r;
    if(create)for(auto& r:records)if(!r.known){r.identity=id;r.known=true;return &r;}
    return nullptr;
}
bool pending_work() noexcept {
    if(gnasher_animation::pending_work())return true;
    for(auto const& r:records)if(r.known&&r.stage!=Stage::idle)return true;return false;
}
bool blocks_fire(hand::ObjectIdentity const& weapon) noexcept {
    if(gnasher_animation::owns_weapon(weapon))return true;
    for(auto const& r:records)if(r.known&&r.stage!=Stage::idle&&hand::same(r.identity.weapon,weapon))return true;
    return false;
}
bool fresh_part_grant(hand::Frame const& f,hand::Decision const& d,hand::LeaseKind kind) noexcept {
    return d.new_grant&&d.status==hand::DecisionStatus::granted&&!d.release_required&&
        hand::same(d.key,f.key)&&d.press_token==f.press_token&&f.press_token&&
        d.lease.kind==kind&&d.lease.id==f.press_token&&
        hand::lease_valid(d.lease,f.key.lifecycle)&&f.left_tracked&&f.left_grip_known&&f.left_grip_down;
}
bool fresh_ammo_grant(hand::Frame const& f,hand::Decision const& d) noexcept {
    return fresh_part_grant(f,d,hand::LeaseKind::ammo);
}
bool begin(hand::Frame const& f,hand::Decision const& d) noexcept {
    if(lancerreload_runtime::stop_requested.load(std::memory_order_acquire)||
       !weaponhand::current_gt(nullptr)||!playground::instance(f.weapon.address,L"Weap_Shotgun")||
       !fresh_ammo_grant(f,d)||!nativeequipment::current(nullptr,f)||
       gnasher_inventory::blocks_fire(f.weapon))return false;
    auto* e=gnasher_fire::entry(f,false);auto* r=record(f,true);
    if(!e||!r||r->stage!=Stage::idle||f.press_token<=r->last_intent||
       !(e->identity==inv::identity(f))||e->cycle.pending()||e->cycle.faulted()||
       e->mechanical.game_ticks_per_second()!=1000000)return false;
    inv::Inventory opening{};
    if(!e->observer.sample(f,opening)||!opening.valid()||opening.infinite_reserve||
       opening.capacity!=e->mechanical.capacity())return false;
    const auto fire_before=e->cycle.inventory();
    if(!(fire_before.identity==gnasher_fire::mechanical_identity(f))||fire_before.revision!=opening.revision||
       fire_before.loaded!=opening.loaded||fire_before.reserve!=opening.reserve)return false;
    const auto now=weaponhand::clock_ns(nullptr);
    mechanism::GnasherReloadAcceptance acceptance{gnasher_fire::mechanical_identity(f),f.controllers.sequence,
        f.controllers.sync_ns,now,0,gnasher_fire::counts(f,opening),true};
    float game_seconds{};
    if(!lancerreload_state::time_field(f.world.address,L"TimeSeconds",game_seconds)||
       !std::isfinite(game_seconds)||game_seconds<0||double(game_seconds)*1000000.>=double(INT64_MAX))return false;
    acceptance.original_start_game_tick=std::int64_t(std::floor(double(game_seconds)*1000000.));
    auto proposed=e->mechanical;
    if(proposed.accept_original_reload(acceptance).event!=mechanism::GnasherMechanicalEvent::reload_started)return false;
    lancerreload_state::RewardReceipt reward{};
    if(!lancerreload_state::capture_reward_eligibility(opening,reward)||!nativeequipment::current(nullptr,f))return false;
    // Publish responsibility BEFORE entering ForceReload. A rejected/unknown
    // return after dispatch cannot authorize another ForceReload for this press.
    r->identity=e->identity;r->opening=opening;r->reward=reward;r->completion={};
    r->source=f;r->recovery_counts=opening;r->recover_to_park=false;
    r->last_intent=f.press_token;r->serial=0;r->cycle_token=0;r->exit_frame=0;
    r->start_dispatched=false;r->ui_pending=false;r->stage=Stage::starting;
    const auto result=lancerreload_state::begin_for_part(f,r->serial,hand::LeaseKind::ammo,nullptr,&r->start_dispatched);
    if(result!=lancerreload_state::BeginResult::acquired){
        r->stage=(r->start_dispatched||r->serial)?Stage::recovery_required:Stage::idle;return false;
    }
    const auto& lease=lancerreload_state::lease;
    const double original_tick=double(lease.original_start)*1000000.;
    if(!lease.acquired||lease.serial!=r->serial||!(lease.identity==r->identity)||
       !std::isfinite(original_tick)||original_tick<0||original_tick>=double(INT64_MAX)){
        r->stage=Stage::recovery_required;return false;
    }
    // Use the actual original state timestamp, never the elapsed dispatcher
    // time, the stock sweet-spot window or a renewed controller timestamp.
    acceptance.original_start_game_tick=std::int64_t(std::floor(original_tick));
    if(e->mechanical.accept_original_reload(acceptance).event!=mechanism::GnasherMechanicalEvent::reload_started){
        r->stage=Stage::recovery_required;return false;
    }
    r->stage=Stage::feeding;r->ui_pending=true;return true;
}
// The mechanical consumer has accepted a NEW support grasp and reported a
// zero-insert cancellation. No closure ACK/reward or automatic refill belongs
// to this path. A previous unpaid shot instead stays on the ordinary lever path.
bool close_empty(hand::Frame const& f,hand::Decision const& d) noexcept {
    auto* r=record(f);auto* e=gnasher_fire::entry(f,false);
    if(!r||!e||r->stage!=Stage::feeding||!(r->identity==inv::identity(f))||
       !fresh_part_grant(f,d,hand::LeaseKind::support)||
       e->mechanical.reload_active()||e->mechanical.inserted()||e->mechanical.held_round_serial()||
       e->mechanical.mechanism_pending()||e->mechanical.inventory_request().token||
       e->mechanical.cycle_request().token||e->cycle.blocked()||
       gnasher_inventory::blocks_fire(f.weapon)||!lancerreload_state::owns(f,r->serial))return false;
    inv::Inventory actual{};
    if(!e->observer.sample(f,actual)||!inv::counts_equal(actual,r->opening)||!nativeequipment::current(nullptr,f))return false;
    r->source=f;r->exit_frame=f.key.frame;r->stage=Stage::empty_exiting;r->native_exited=false;
    if(lancerreload_state::exit_without_fill(f,r->serial)){
        r->native_exited=true;r->stage=Stage::empty_settlement;
    }
    return r->native_exited;
}
void settle_empty(Record& r,gnasher_fire::Entry& e,hand::Frame const* current=nullptr) noexcept {
    if(r.stage==Stage::empty_exiting){
        const auto& lease=lancerreload_state::lease;
        if(!lease.acquired||lease.serial!=r.serial||!(lease.identity==r.identity)||
           !lancerreload_state::recover_serial(r.serial))return;
        r.native_exited=true;r.stage=Stage::empty_settlement;
        if(current){r.exit_frame=current->key.frame;return;}
    }
    if(r.stage!=Stage::empty_settlement||!r.native_exited||
       (current&&current->key.frame<=r.exit_frame))return;
    inv::Inventory actual{};bool firing{},reloading{};
    if(!e.observer.sample_owned(r.identity,actual)||!inv::counts_equal(actual,r.opening)||
       !lancerreload_state::bool_call(r.identity.weapon.address,L"IsFiring",firing)||firing||
       !lancerreload_state::bool_call(r.identity.weapon.address,L"IsReloading",reloading)||reloading)return;
    r.stage=Stage::idle; // Existing inventory/shot ledger unchanged; never reward.
}
bool owns_stow(hand::Frame const& f) noexcept {
    return playground::instance(f.weapon.address,L"Weap_Shotgun")&&gnasher_fire::entry(f,false);
}
bool stow_ready(hand::Frame const& f) noexcept {
    auto* r=record(f);auto* e=gnasher_fire::entry(f,false);
    if(!r||!e||r->stage!=Stage::parked||!r->native_exited||
       !inv::same_instance_across_hold(r->identity,inv::identity(f))||
       gnasher_inventory::blocks_fire(f.weapon)||gnasher_animation::owns_weapon(f.weapon)||
       !nativeequipment::current(nullptr,f))return false;
    auto observer=e->observer;inv::Inventory actual{};
    if(!(e->identity==inv::identity(f))){if(!observer.rebind_hold(f,e->identity,actual))return false;}
    else if(!observer.sample(f,actual))return false;
    return actual.loaded==r->park_counts.loaded&&actual.reserve==r->park_counts.reserve&&
        actual.capacity==r->park_counts.capacity&&actual.infinite_reserve==r->park_counts.infinite_reserve&&
        actual.revision>=r->park_counts.revision&&nativeequipment::current(nullptr,f);
}
bool settle_park(Record& r,gnasher_fire::Entry& e) noexcept {
    if(r.stage!=Stage::parking)return r.stage==Stage::parked;
    if(!r.native_exited){
        auto const& lease=lancerreload_state::lease;
        if(!lease.acquired||lease.serial!=r.serial||!(lease.identity==r.identity)||
           !lancerreload_state::recover_serial(r.serial))return false;
        r.native_exited=true;
    }
    inv::Inventory actual{};
    if(!e.observer.sample_owned(r.identity,actual)||!inv::counts_equal(actual,r.park_counts)||
       !gnasher_animation::release_owned_after_exit(r.identity))return false;
    r.stage=Stage::parked;return true;
}
bool prepare_stow(hand::Frame const& f,bool& changed) noexcept {
    changed=false;if(!owns_stow(f))return true;
    if(lancerreload_runtime::stop_requested.load(std::memory_order_acquire))return false;
    auto* e=gnasher_fire::entry(f,false);auto* r=record(f,true);
    if(!e||!r||!(e->identity==inv::identity(f))||!nativeequipment::current(nullptr,f))return false;
    if(r->stage==Stage::parked)return stow_ready(f);
    if(r->stage==Stage::parking){changed=true;return settle_park(*r,*e);}
    if((r->stage!=Stage::idle&&r->stage!=Stage::feeding)||e->cycle.pending()||e->cycle.faulted())return false;
    if(gnasher_inventory::blocks_fire(f.weapon)){
        (void)gnasher_inventory::recover_round(f);changed=true;return false;
    }
    // Stowing may cancel a physical proposal only if it never entered the
    // original writer and the exact original before-counts still hold. A
    // dispatched/uncertain journal above remains responsible for settlement.
    if(e->mechanical.inventory_request().token){
        changed=true;(void)gnasher_inventory::reject_undispatched_round(f);
        if(e->mechanical.inventory_request().token)return false;
    }
    inv::Inventory actual{};
    if(!e->observer.sample(f,actual)||!actual.valid())return false;
    const auto fire=e->cycle.inventory();
    if(fire.loaded!=actual.loaded||fire.reserve!=actual.reserve||fire.revision>actual.revision)return false;
    auto suspended=e->mechanical;
    const bool bound=suspended.stage()!=mechanism::GnasherMechanicalStage::unbound;
    if(bound&&(suspended.inventory_request().token||suspended.cycle_request().token))return false;
    const auto held_round=bound?suspended.held_round_serial():0;
    if(held_round){
        changed=true;
        if(!gnasher_round::release_for_stow(e->identity,held_round)||
           suspended.release_round(gnasher_fire::mechanical_identity(f),held_round).event!=
               mechanism::GnasherMechanicalEvent::round_released)return false;
    }
    if(bound&&(suspended.inventory().loaded!=actual.loaded||suspended.inventory().reserve!=actual.reserve||
       !suspended.suspend_hold()))return false;
    const bool reload=r->stage==Stage::feeding;
    if(reload&&!lancerreload_state::owns(f,r->serial))return false;
    r->identity=e->identity;r->source=f;r->park_counts=actual;r->parked_reload=reload;
    r->mechanical_parked=bound;r->native_exited=!reload;r->recover_to_park=false;
    r->parked_lease=reload?lancerreload_state::lease:lancerreload_state::Lease{};
    e->mechanical=suspended;e->prepared=false;r->stage=Stage::parking;
    changed=changed||reload||gnasher_animation::owns_weapon(f.weapon);
    return settle_park(*r,*e);
}
// Fresh hold-count rebind is separate from a new physical part grant. A pure
// shot debt needs no new Reloading action; an interrupted reload reacquires
// native state only from a NEW ammo/support grant for its current step.
bool resume(hand::Frame const& f,hand::Decision const* grant=nullptr) noexcept {
    if(lancerreload_runtime::stop_requested.load(std::memory_order_acquire))return false;
    auto* r=record(f);auto* e=gnasher_fire::entry(f,false);
    if(!r||!e||r->stage!=Stage::parked||!(r->identity==e->identity)||
       !inv::same_instance_across_hold(r->identity,inv::identity(f))||
       !nativeequipment::current(nullptr,f)||gnasher_inventory::blocks_fire(f.weapon))return false;
    if(r->parked_reload){
        if(!grant)return false;
        const auto wanted=e->mechanical.stage()==mechanism::GnasherMechanicalStage::feed_port?
            hand::LeaseKind::ammo:hand::LeaseKind::support;
        if(!fresh_part_grant(f,*grant,wanted)||f.press_token<=r->last_intent)return false;
    }
    auto observer=e->observer;auto mechanical=e->mechanical;auto fire=e->cycle;inv::Inventory actual{};
    if(!observer.rebind_hold(f,e->identity,actual)||actual.loaded!=r->park_counts.loaded||
       actual.reserve!=r->park_counts.reserve||actual.capacity!=r->park_counts.capacity||
       actual.infinite_reserve!=r->park_counts.infinite_reserve)return false;
    const auto counts=gnasher_fire::counts(f,actual);
    if(r->mechanical_parked){if(!mechanism::gnasher_resume_held_consumers(mechanical,fire,counts))return false;}
    else if(!fire.rebind_observation(counts))return false;
    const auto old_lease=r->parked_lease;
    // No input is revived by these exact-count observation rebinds. During
    // synchronous entry, the journal continues blocking this actor's fire.
    r->identity=actual.identity;r->source=f;r->park_counts=actual;r->opening.identity=actual.identity;
    r->recovery_counts=actual;r->recover_to_park=true;r->stage=Stage::resuming;
    e->identity=actual.identity;e->observer=observer;e->cycle=fire;e->prepared=false;
    if(r->parked_reload){
        r->last_intent=f.press_token;r->start_dispatched=false;
        // This resumes the original accepted AMMO reload. The current grant
        // remains its real support/ammo kind above; suspended native re-entry
        // has no new item grant and preserves the original transaction serial.
        const auto result=lancerreload_state::begin_for_part(f,r->serial,hand::LeaseKind::ammo,&old_lease,&r->start_dispatched);
        if(result!=lancerreload_state::BeginResult::acquired){
            r->stage=(r->start_dispatched||r->serial)?Stage::recovery_required:Stage::parked;return false;
        }
        const auto& lease=lancerreload_state::lease;
        if(!lease.acquired||lease.serial!=old_lease.serial||!(lease.identity==r->identity)||
           lease.reward_start!=old_lease.reward_start){r->stage=Stage::recovery_required;return false;}
        r->stage=Stage::feeding;r->ui_pending=true;
    }else r->stage=Stage::idle;
    e->mechanical=mechanical;r->mechanical_parked=false;r->recover_to_park=false;return true;
}
void switch_dispatched(hand::Frame const& f) noexcept {
    auto* r=record(f);if(r&&r->stage!=Stage::idle&&r->identity==inv::identity(f))r->source=f;
    // No old gesture is replayed. The independent normal-GT service below
    // observes an actual selection change and drains only this original owner.
}
void settle_start_recovery(Record& r,gnasher_fire::Entry& e) noexcept {
    inv::Inventory actual{};
    if(r.stage==Stage::recovery_required&&r.serial){
        const auto& lease=lancerreload_state::lease;
        if(!lease.acquired||lease.serial!=r.serial||!(lease.identity==r.identity)||
           !e.observer.sample_owned(r.identity,actual)||!inv::counts_equal(actual,r.recovery_counts))return;
        r.stage=Stage::recovering;
        if(!lancerreload_state::recover_serial(r.serial)){r.stage=Stage::recovery_required;return;}
        r.stage=Stage::start_settlement;
    }
    if(r.stage==Stage::start_settlement&&e.observer.sample_owned(r.identity,actual)&&
       inv::counts_equal(actual,r.recovery_counts))r.stage=r.recover_to_park?Stage::parked:Stage::idle;
}
void settle_finish(Record& r,gnasher_fire::Entry& e) noexcept {
    inv::Inventory actual{};
    if(r.stage!=Stage::finishing||!e.observer.sample_owned(r.identity,actual)||
       !inv::counts_equal(actual,r.completion.expected))return;
    if(r.completion.result==mechanism::ReloadTxnResult::perfect&&
       !lancerreload_state::perfect_reward(r.completion,r.opening,e.observer,r.reward))return;
    r.stage=Stage::idle;
}
// No selected hand Frame is needed for old original-state cleanup. This is a
// receipt/recovery service, never a current candidate or new action dispatcher.
void service_unselected(uptr pc,uptr selected,bool stopping) noexcept {
    if(!weaponhand::current_gt(nullptr))return;
    if(gnasher_animation::owns_controller(pc)){
        (void)gnasher_animation::service_owned();
        const auto identity=gnasher_animation::owned_identity();
        bool original_pending=lancerreload_state::lease.acquired&&lancerreload_state::lease.identity==identity;
        for(auto const& r:records)if(r.known&&r.identity==identity&&r.stage!=Stage::idle)original_pending=true;
        // Pure per-shot animation ownership may have no Reloading record.
        // An unselected/stop cleanup still retires it only after original
        // !IsFiring/!IsReloading and the actual stopped/replaced-node receipt.
        if((stopping||identity.weapon.address!=selected)&&!original_pending&&
           !gnasher_inventory::blocks_fire(identity.weapon))
            (void)gnasher_animation::release_owned_after_exit(identity);
    }
    for(auto& r:records){
        if(!r.known||r.stage==Stage::idle||r.source.controller.address!=pc||
           (!stopping&&r.identity.weapon.address==selected))continue;
        if(!weaponhand::client.registration_alive(r.source.key.registration_cookie,r.source.key.native_started_qpc))continue;
        gnasher_fire::Entry* e{};
        for(auto& candidate:gnasher_fire::entries)if(candidate.known&&candidate.identity==r.identity){e=&candidate;break;}
        if(!e)continue;
        if(gnasher_inventory::blocks_fire(r.identity.weapon)){
            (void)gnasher_inventory::recover_round(r.source);
            if(gnasher_inventory::blocks_fire(r.identity.weapon))continue;
        }
        if(r.stage==Stage::feeding){
            if(e->mechanical.inventory_request().token)
                (void)gnasher_inventory::reject_undispatched_round(r.source);
            auto const& lease=lancerreload_state::lease;inv::Inventory actual{};auto suspended=e->mechanical;
            if(suspended.inventory_request().token||suspended.cycle_request().token)continue;
            const auto item=suspended.held_round_serial();
            if(item&&(!gnasher_round::release_for_stow(r.identity,item)||
               suspended.release_round(suspended.inventory().identity,item).event!=
                    mechanism::GnasherMechanicalEvent::round_released))continue;
            if(!lease.acquired||lease.serial!=r.serial||!(lease.identity==r.identity)||
               !e->observer.sample_owned(r.identity,actual)||
               actual.loaded!=suspended.inventory().loaded||actual.reserve!=suspended.inventory().reserve||
               !suspended.suspend_hold())continue;
            r.parked_lease=lease;r.park_counts=actual;r.parked_reload=r.mechanical_parked=true;
            r.native_exited=false;e->prepared=false;e->mechanical=suspended;r.stage=Stage::parking;
        }
        if(r.stage==Stage::parking)(void)settle_park(r,*e);
        if(r.stage==Stage::empty_exiting||r.stage==Stage::empty_settlement)settle_empty(r,*e);
        if(r.stage==Stage::recovery_required||r.stage==Stage::start_settlement)settle_start_recovery(r,*e);
        if(r.stage==Stage::finishing)settle_finish(r,*e);
        if(stopping&&r.stage==Stage::parked&&!gnasher_animation::owns_weapon(r.identity.weapon))
            r.stage=Stage::idle; // No ACK/refund/reward: external lease/clip already retired.
    }
}
// False means no cycle ACK this frame. In particular, the synchronous original
// state exit retires its source and MUST be followed by a new original readback.
bool prepare_cycle(hand::Frame const& f,mechanism::GnasherCycleRequest const& request,
    mechanism::GnasherCycleReadbackGate const& gate,mechanism::GnasherCycleReadback const& value) noexcept {
    auto* r=record(f);if(!r||r->stage==Stage::idle)return true;
    if(!(r->identity==inv::identity(f))||!nativeequipment::current(nullptr,f))return false;
    if(r->stage==Stage::awaiting_cycle)return r->cycle_token==request.token&&f.key.frame>r->exit_frame;
    if(r->stage!=Stage::feeding||!gate.endpoint_current(value,weaponhand::clock_ns(nullptr))||
       !(request.identity==gnasher_fire::mechanical_identity(f))||!request.token||
       !lancerreload_state::owns(f,r->serial)||gnasher_inventory::blocks_fire(f.weapon))return false;
    auto* e=gnasher_fire::entry(f,false);inv::Inventory actual{};
    if(!e||!(e->identity==r->identity))return false;
    const auto physical=e->mechanical.cycle_request();
    if(physical.token!=request.token||physical.completion!=request.completion||
       physical.action!=request.action||physical.input_sequence!=request.input_sequence||
       physical.physical_completion_ns!=request.physical_completion_ns||
       !(physical.identity==request.identity)||!e->observer.sample(f,actual)||
       actual.loaded!=e->mechanical.inventory().loaded||actual.reserve!=e->mechanical.inventory().reserve||
       actual.loaded<r->opening.loaded||actual.loaded-r->opening.loaded!=e->mechanical.inserted())return false;
    r->completion={};r->completion.identity=r->identity;r->completion.token=request.token;
    r->completion.reload_serial=r->serial;r->completion.call=mechanism::ReloadTxnCall::finish;
    r->completion.before=r->completion.expected=actual;
    r->completion.result=request.completion==mechanism::ReloadEvent::perfect?
        mechanism::ReloadTxnResult::perfect:mechanism::ReloadTxnResult::ordinary;
    r->cycle_token=request.token;r->exit_frame=f.key.frame;r->stage=Stage::exiting;
    if(lancerreload_state::exit_without_fill(f,r->serial))r->stage=Stage::awaiting_cycle;
    return false;
}
void cycle_accepted(hand::Frame const& f,mechanism::GnasherCycleReceipt const& receipt) noexcept {
    auto* r=record(f);if(!r||r->stage!=Stage::awaiting_cycle||!receipt.accepted||
       !(r->identity==inv::identity(f))||r->cycle_token!=receipt.request_token||
       !(receipt.identity==gnasher_fire::mechanical_identity(f))||f.key.frame<=r->exit_frame)return;
    // Called only AFTER the paired mechanical/fire publication. No reward is
    // allowed merely because the original clip reached its final pose.
    auto* e=gnasher_fire::entry(f,false);
    if(!e||e->mechanical.reload_active()||e->mechanical.cycle_request().token||e->cycle.blocked())return;
    r->stage=Stage::finishing;
}
void service(hand::Frame const& f,bool equipment_pending=true,bool main_hand_held=true) noexcept {
    if(!weaponhand::current_gt(nullptr))return;
    if(gnasher_animation::owns_controller(f.controller.address))(void)gnasher_animation::service_owned();
    if(lancerreload_runtime::stop_requested.load(std::memory_order_acquire))return;
    auto* r=record(f);
    if(!r||r->stage==Stage::idle){
        auto* e=gnasher_fire::entry(f,false);
        if(e&&e->identity==inv::identity(f)&&gnasher_animation::owns_weapon(f.weapon)&&
           nativeequipment::current(nullptr,f)&&!e->cycle.blocked()&&
           e->mechanical.can_request_fire(weaponhand::clock_ns(nullptr))&&
           !gnasher_inventory::blocks_fire(f.weapon)&&
           !(lancerreload_state::lease.acquired&&lancerreload_state::lease.identity==e->identity))
            (void)gnasher_animation::release_owned_after_exit(e->identity);
        return;
    }
    if(r->stage==Stage::parked&&!r->parked_reload&&main_hand_held&&
       (inv::identity(f).hold_generation>r->identity.hold_generation||
        (!equipment_pending&&f.key.frame>r->source.key.frame))){
        (void)resume(f);return;
    }
    if(!(r->identity==inv::identity(f)))return;
    if(r->stage==Stage::empty_exiting||r->stage==Stage::empty_settlement){
        if(auto* e=gnasher_fire::entry(f,false))settle_empty(*r,*e,&f);return;
    }
    if(r->stage==Stage::parking){if(auto* e=gnasher_fire::entry(f,false))(void)settle_park(*r,*e);return;}
    if(r->stage==Stage::feeding&&nativeequipment::current(nullptr,f)){
        if(r->ui_pending){
            if(!lancerreload_state::set_ui_timing(f,r->serial,4000,4000,true))return;
            r->ui_pending=false;
        }
        (void)lancerreload_state::tick_ui_timing();return;
    }
    if(r->stage==Stage::exiting){
        if(lancerreload_state::recover_serial(r->serial)){
            r->exit_frame=f.key.frame;r->stage=Stage::awaiting_cycle;
        }
        return;
    }
    if(r->stage==Stage::recovery_required||r->stage==Stage::start_settlement){
        if(auto* e=gnasher_fire::entry(f,false))settle_start_recovery(*r,*e);return;
    }
    if(r->stage!=Stage::finishing||!nativeequipment::current(nullptr,f))return;
    if(auto* e=gnasher_fire::entry(f,false))settle_finish(*r,*e);
}
} // namespace gnasher_state
