// Bow-only original reload journal and inventory bridge. The single existing
// interaction model issues the closure request; this is not a second ledger,
// input owner, clock, geometry recognizer or route to timed automatic refill.
namespace torquebow_reload_live {
namespace hand=gears_weapon_hand;namespace inv=lancerreload_inventory;namespace w=gears_weapon;
enum class Phase {idle,reserved,opening,holding,committing,exiting,rewarding,receipt,recovering,conflict};
enum class Begin {rejected,accepted,unresolved};
struct Owner {
    Phase phase{};hand::Frame source{};hand::Lease hand_lease{};
    std::uint64_t generation{},original_serial{};std::int64_t original_start_tick{-1};
    torquebow_cleanup::Anchor anchor{};inv::ObservedInventory observer{};inv::InsertionWriter writer{};
    inv::Inventory opening{};lancerreload_state::RewardReceipt reward{};
    w::TorqueBowInventory model_before{};w::TorqueBowRequest request{};inv::Request insert{};
    bool start_dispatched{},insert_attempted{},committed{},exit_attempted{},original_exited{};
};
Owner owner;std::uint64_t next_generation{1};std::atomic<bool> outstanding{};
inv::Identity last_intent_owner{};std::uint64_t last_intent{};
bool pending() noexcept {return outstanding.load(std::memory_order_acquire);}
bool owns_anchor(hand::Frame const& f,std::uint64_t generation) noexcept {
    return generation&&owner.generation==generation&&owner.phase!=Phase::idle&&
        hand::same_frame(owner.source,f);
}
bool context() noexcept {
    return pending()&&owner.anchor.current(owner.source,owner.generation,owns_anchor);
}
bool matches(hand::Frame const& f) noexcept {
    return context()&&owner.opening.identity==inv::identity(f)&&
        nativeequipment::current(nullptr,f)&&vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address);
}
bool owns_reload(hand::Frame const& f,std::uint64_t serial) noexcept {
    return serial&&serial==owner.original_serial&&matches(f)&&lancerreload_state::owns(f,serial);
}
bool ammo_grant(hand::Frame const& f,hand::Decision const& d) noexcept {
    return !hand::index_pinch(f)&&hand::same(f.key,d.key)&&f.press_token&&d.press_token==f.press_token&&
        !d.release_required&&d.lease.kind==hand::LeaseKind::ammo&&d.lease.id==f.press_token&&
        hand::lease_valid(d.lease,f.key.lifecycle)&&f.left_tracked&&f.left_grip_known&&f.left_grip_down&&
        (d.status==hand::DecisionStatus::granted||d.status==hand::DecisionStatus::retained);
}
bool forget(bool no_original_debt) noexcept {
    if(!no_original_debt)return false;
    if(owner.anchor.serial()&&!owner.anchor.retire(owner.generation,true))return false;
    owner={};outstanding.store(false,std::memory_order_release);return true;
}
// This is called only AFTER the Bow caller has certified a fresh left-shoulder
// proposal and its real Native ammo grant. Reserving neither spends stock nor
// starts an original action. No normal dispatcher calls this API yet.
bool reserve_before_begin(hand::Frame const& f,hand::Decision const& d,w::TorqueBowInventory const& model) noexcept {
    if(pending()||next_generation==UINT64_MAX||!next_generation||!weaponhand::current_gt(nullptr)||
       !nativeequipment::current(nullptr,f)||!playground::instance(f.weapon.address,L"Weap_Bow_base")||
       !ammo_grant(f,d)||!d.new_grant||!model.revision||model.revision==UINT64_MAX||model.loaded||!model.reserve||
       (last_intent_owner==inv::identity(f)&&f.press_token<=last_intent)||
       !vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address))return false;
    Owner candidate{};candidate.source=f;candidate.hand_lease=d.lease;candidate.generation=next_generation++;
    candidate.phase=Phase::reserved;candidate.model_before=model;
    if(!candidate.observer.sample(f,candidate.opening)||candidate.opening.capacity!=1||candidate.opening.loaded||
       candidate.opening.infinite_reserve||candidate.opening.reserve!=model.reserve||
       !lancerreload_state::capture_reward_eligibility(candidate.opening,candidate.reward))return false;
    owner=candidate;outstanding.store(true,std::memory_order_release);
    if(!owner.anchor.capture_before_press(f,owner.generation,owns_anchor)){
        forget(true);return false;
    }
    last_intent_owner=owner.opening.identity;last_intent=f.press_token;
    return true;
}
Begin begin_original(hand::Frame const& f,hand::Decision const& d) noexcept {
    if(owner.phase!=Phase::reserved||owner.start_dispatched||!matches(f)||!ammo_grant(f,d)||
       !hand::same(owner.hand_lease,d.lease))return Begin::rejected;
    owner.phase=Phase::opening;
    const auto result=lancerreload_state::begin_for_part(f,owner.original_serial,hand::LeaseKind::ammo,
                                                       nullptr,&owner.start_dispatched);
    if(result==lancerreload_state::BeginResult::rejected&&!owner.start_dispatched){forget(true);return Begin::rejected;}
    // A dispatched ForceReload is NEVER called again, even if its normal
    // return/state receipt is unknown. Keep that obligation, including serial0.
    if(result!=lancerreload_state::BeginResult::acquired||!context()){
        owner.phase=Phase::recovering;return Begin::unresolved;
    }
    float started{};
    if(!lancerreload_state::accepted_game_start(owner.original_serial,started)||
       !std::isfinite(started)||started<0||double(started)*1000000.>9e15){
        owner.phase=Phase::recovering;return Begin::unresolved;
    }
    owner.original_start_tick=static_cast<std::int64_t>(double(started)*1000000.);
    owner.phase=Phase::holding;return Begin::accepted;
}
bool accepted(std::uint64_t& serial,std::int64_t& original_start_tick) noexcept {
    serial=0;original_start_tick=-1;
    float actual_start{};
    if(owner.phase!=Phase::holding||!context()||
       !lancerreload_state::accepted_game_start(owner.original_serial,actual_start)||
       !std::isfinite(actual_start)||actual_start<0||double(actual_start)*1000000.>9e15||
       static_cast<std::int64_t>(double(actual_start)*1000000.)!=owner.original_start_tick)return false;
    serial=owner.original_serial;original_start_tick=owner.original_start_tick;return true;
}
bool same_model(w::TorqueBowIdentity const& id) noexcept {
    const auto& f=owner.source;
    return id==w::TorqueBowIdentity{f.world.address,f.pawn.address,f.controller.address,f.weapon.address,
        f.key.world_generation,f.controllers.session,f.controllers.space_epoch,f.controllers.input_epoch};
}
bool request_matches(w::TorqueBowRequest const& request) noexcept {
    return request.token&&same_model(request.identity)&&request.action==w::TorqueBowAction::complete_original_reload&&
       request.original_reload_serial==owner.original_serial&&request.arrow_serial&&request.authored_ns>0&&
       request.before.revision==owner.model_before.revision&&!request.before.loaded&&
       request.before.reserve==owner.opening.reserve&&request.after.loaded==1&&
       request.after.reserve==request.before.reserve-1&&request.finished_game_tick>=owner.original_start_tick&&
       request.outcome==(request.finished_game_tick-owner.original_start_tick<=4000000?
           w::TorqueBowOutcome::perfect:w::TorqueBowOutcome::ordinary);
}
void record_request(w::TorqueBowRequest const& request) noexcept {
    owner.request=request;owner.insert={};owner.insert.identity=owner.opening.identity;
    owner.insert.token=request.token;owner.insert.reload_serial=owner.original_serial;owner.insert.item_serial=request.arrow_serial;
    owner.insert.call=w::ReloadTxnCall::insert;owner.insert.before=owner.opening;owner.insert.expected=owner.opening;
    owner.insert.expected.revision=0;owner.insert.expected.loaded=1;--owner.insert.expected.reserve;
    owner.insert.result=request.outcome==w::TorqueBowOutcome::perfect?w::ReloadTxnResult::perfect:w::ReloadTxnResult::ordinary;
}
bool begin_completion(hand::Frame const& f,hand::Decision const& d,w::TorqueBowRequest const& request) noexcept {
    const auto now=weaponhand::clock_ns(nullptr);
    if(owner.phase!=Phase::holding||!owns_reload(f,owner.original_serial)||!ammo_grant(f,d)||!request_matches(request)||
       now<request.authored_ns||now>request.valid_until_ns)return false;
    // This copies the one REAL model's complete continuous-stroke request. A
    // touch, elapsed animation, hidden warm component or clock never makes it.
    record_request(request);owner.phase=Phase::committing;return true;
}
// A model closure can lose tracking before any setter. Retain its token so
// the SAME model receives an actual unchanged-stock/exit rejection, without
// demanding a new pose or sending that stale closure to the inventory writer.
bool reject_completion(w::TorqueBowRequest const& request) noexcept {
    if((owner.phase!=Phase::holding&&!(owner.phase==Phase::committing&&!owner.insert_attempted))||
       !request_matches(request))return false;
    record_request(request);owner.phase=Phase::exiting;return true;
}
bool observe_counts(inv::Inventory& counts) noexcept {
    return context()&&owner.observer.sample_owned(owner.opening.identity,counts);
}
bool observe_exited() noexcept {
    if(!context()||!owner.original_serial)return false;
    bool reloading{};
    if(!lancerreload_state::bool_call(owner.source.weapon.address,L"IsReloading",reloading)||reloading)return false;
    // recover_serial cannot invoke Abort when the preceding observation says
    // the single-threaded original state has already exited. Its timer/UI
    // checks still have to pass; a still-running completion timer is debt.
    return lancerreload_state::recover_serial(owner.original_serial);
}
bool service(hand::Frame const* current=nullptr) noexcept {
    if(!pending())return true;
    if(!context())return false;
    if(owner.phase==Phase::committing){
        if(!owner.insert_attempted){
            if(!current||!owns_reload(*current,owner.original_serial))return false;
            owner.insert_attempted=true;inv::Receipt receipt{};
            const auto result=owner.writer.commit(*current,owner.insert,owner.observer,owns_reload,receipt);
            if(result==inv::CommitResult::committed)owner.committed=true;
        }
        inv::Inventory counts{};if(!observe_counts(counts))return false;
        if(inv::counts_equal(counts,owner.insert.expected))owner.committed=true;
        else if(owner.committed){owner.phase=Phase::conflict;return false;}
        else if(!inv::counts_equal(counts,owner.insert.before)){
            // Compensate only this journal's exact reserve-first half-write.
            // A complete insert is conserved even after tracking/switch loss.
            if(!lancerreload_state::restore_partial_insert(owner.insert,owner.observer)||
               !observe_counts(counts)||!inv::counts_equal(counts,owner.insert.before))return false;
        }
        owner.phase=Phase::exiting;
    }
    if(owner.phase==Phase::exiting||owner.phase==Phase::recovering){
        if(!owner.original_serial)return false; // dispatched unknown begin is not "nothing happened"
        if(!owner.exit_attempted){
            // Before invoking once, retain both the exact start and the
            // original-call obligation. Unknown Abort returns are not replayed.
            if(!lancerreload_state::retains_serial(owner.original_serial)){
                if(!observe_exited())return false;
            }else{
                owner.exit_attempted=true;
                if(!lancerreload_state::recover_serial(owner.original_serial))return false;
            }
        }else if(!observe_exited())return false;
        owner.original_exited=true;
        inv::Inventory counts{};if(!observe_counts(counts))return false;
        const auto& wanted=owner.committed?owner.insert.expected:owner.opening;
        if(!inv::counts_equal(counts,wanted)){owner.phase=Phase::conflict;return false;}
        if(!owner.request.token)return forget(true);
        owner.phase=owner.committed&&owner.request.outcome==w::TorqueBowOutcome::perfect?Phase::rewarding:Phase::receipt;
    }
    if(owner.phase==Phase::rewarding){
        if(!lancerreload_state::perfect_reward(owner.insert,owner.opening,owner.observer,owner.reward))return false;
        owner.phase=Phase::receipt;
    }
    return owner.phase==Phase::receipt;
}
bool stop() noexcept {
    if(!pending())return true;
    if(owner.phase==Phase::reserved)return forget(true); // no original call was dispatched
    if(owner.phase==Phase::holding||owner.phase==Phase::opening)owner.phase=Phase::recovering;
    if(owner.phase==Phase::committing&&!owner.insert_attempted)owner.phase=Phase::exiting;
    return service(); // committed insert/reward is preserved, never refunded
}
// The caller supplies its Driver-owned monotonic stamp and event serial only
// after this returns a real current neutral observation. It then acknowledges
// that SAME model request before retiring this journal.
bool receipt_data(torquebow_state::Observed const& actual,w::TorqueBowActionReceipt& out) noexcept {
    inv::Inventory counts{};
    if(!actual.stopped()||!observe_counts(counts)||
       !inv::counts_equal(counts,owner.committed?owner.insert.expected:owner.opening)||actual.inventory.capacity!=1||
       actual.inventory.used!=int(1-counts.loaded)||actual.inventory.reserve!=int(counts.reserve)||actual.inventory.infinite)return false;
    out.request_token=owner.request.token;out.accepted=owner.committed;
    // Both model and observer revisions are local observation counters, not a
    // fabricated game field. Advance the MODEL counter only after real counts.
    out.inventory={owner.model_before.revision+(owner.committed?1u:0u),counts.loaded,counts.reserve};
    out.original={actual.active,false,false,false,false,false,false};
    out.reload_result_applied=owner.committed?owner.request.outcome:w::TorqueBowOutcome::none;return true;
}
bool receipt(hand::Frame const& f,w::TorqueBowActionReceipt& out) noexcept {
    out={};if(owner.phase!=Phase::receipt||!matches(f)||!owner.original_exited)return false;
    torquebow_state::Observed actual{};
    return torquebow_state::read(f,actual)&&actual.active&&receipt_data(actual,out);
}
bool receipt_owned(w::TorqueBowActionReceipt& out) noexcept {
    out={};if(owner.phase!=Phase::receipt||!context()||!owner.original_exited)return false;
    torquebow_state::Observed actual{};
    return torquebow_state::read_owned(owner.source,actual,owner.anchor,owner.generation,owns_anchor)&&receipt_data(actual,out);
}
bool retire_acknowledged(std::uint64_t token,w::TorqueBowInteraction const& model) noexcept {
    const auto counts=model.inventory();
    return owner.phase==Phase::receipt&&token&&token==owner.request.token&&owner.original_exited&&same_model(model.identity())&&
        !model.has_pending_request()&&!model.has_fire_attempt()&&
        (model.stage()==w::TorqueBowStage::ready||model.stage()==w::TorqueBowStage::interrupted)&&
        counts.revision==owner.model_before.revision+(owner.committed?1u:0u)&&
        counts.loaded==(owner.committed?1u:owner.opening.loaded)&&
        counts.reserve==(owner.committed?owner.insert.expected.reserve:owner.opening.reserve)&&forget(true);
}
}
