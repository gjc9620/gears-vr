// Normal-GT count observations plus receipt-only CustomFire normal return.
// No engine calls, input writes or inventory writes inside the subscriber.
namespace gnasher_fire {
namespace hand=gears_weapon_hand;namespace inv=lancerreload_inventory;
namespace shot=gears_weapon_shot;namespace abi=gears_weapon_call;
struct Entry {
    inv::Identity identity{};inv::ObservedInventory observer{};
    gears_weapon::GnasherFireCycle cycle{};hand::Frame source{};
    gears_weapon::GnasherMechanicalInteraction mechanical{};
    std::uint64_t profile{},reference{},reported_shot{},mechanical_shot{},accepted_call{};std::int64_t prepared_tick{};
    gears_weapon::GnasherShotAcceptance admitted_shot{};
    bool known{},prepared{};
    unsigned preparation_stage{},preparation_flags{},preparation_rejection{}; // Last normal-GT admission result; observation only.
};
std::array<Entry,16> entries{};
gears_weapon::GnasherIdentity mechanical_identity(hand::Frame const& f) noexcept {
    return {f.world.address,f.pawn.address,f.weapon.address,f.key.world_generation,
        f.stereo.session,f.stereo.space_epoch,f.controllers.input_epoch,f.controllers.profile_generation};
}
gears_weapon::GnasherInventory counts(hand::Frame const& f,inv::Inventory const& i) noexcept {
    return {mechanical_identity(f),i.revision,i.loaded,i.reserve};
}
bool same_actor(Entry const& e,hand::Frame const& f) noexcept {
    const auto next=inv::identity(f);
    return e.known&&(e.identity==next||inv::same_instance_across_hold(e.identity,next));
}
bool observation_only(Entry const& e) noexcept {
    return e.known&&!e.prepared&&!e.profile&&!e.reference&&!e.reported_shot&&!e.mechanical_shot&&!e.accepted_call&&
        e.mechanical.stage()==gears_weapon::GnasherMechanicalStage::unbound&&
        !e.cycle.inventory().revision&&!e.cycle.pending()&&!e.cycle.confirmed()&&!e.cycle.blocked()&&!e.cycle.faulted();
}
Entry* entry(hand::Frame const& f,bool create) noexcept {
    const auto next=inv::identity(f);
    for(auto& e:entries)if(e.known&&e.identity==next)return &e;
    // An existing actual action/mechanical responsibility takes precedence.
    for(auto& e:entries)if(!observation_only(e)&&same_actor(e,f))return &e;
    if(create)for(auto& e:entries)if(observation_only(e)&&
        next.hold_generation>=e.identity.hold_generation&&next.lifecycle>=e.identity.lifecycle){
        auto normalized=next;normalized.hold_generation=e.identity.hold_generation;normalized.lifecycle=e.identity.lifecycle;
        if(!(normalized==e.identity))continue;
        // Before the first real hold, retain ONE known unbound veto even if
        // another equipment hold advances the counter. No action was admitted.
        if(next.valid()&&vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)){
            const auto old_hold=e.identity.hold_generation,old_lifecycle=e.identity.lifecycle;
            e=Entry{};e.identity=next;e.known=true;
            static unsigned cold_bind_rows{};
            if(log_file&&cold_bind_rows++<8){
                fwprintf(log_file,L"gnasher_cold_entry_bound weapon=%llx old_hold=%llu hold=%llu old_lifecycle=%llu lifecycle=%llu frame=%llu no_original_responsibility=1\n",
                    f.weapon.address,old_hold,next.hold_generation,old_lifecycle,next.lifecycle,f.key.frame);fflush(log_file);
            }
        }
        return &e;
    }
    for(auto& e:entries)if(same_actor(e,f))return &e;
    if(create)for(auto& e:entries)if(!e.known){e.identity=next;e.known=true;return &e;}
    return nullptr;
}
bool blocks_fire(hand::ObjectIdentity const& weapon) noexcept {
    for(auto const& e:entries)if(e.known&&hand::same(weapon,e.identity.weapon)&&
        (e.cycle.blocked()||e.mechanical.stage()==gears_weapon::GnasherMechanicalStage::unbound||
         !e.mechanical.can_request_fire(weaponhand::clock_ns(nullptr))))return true;
    return gnasher_inventory::blocks_fire(weapon)||gnasher_state::blocks_fire(weapon);
}
bool rebind_observation(Entry& e,hand::Frame const& f) noexcept {
    e.prepared=false;
    if(e.cycle.pending()){
        inv::Inventory prior{};
        if(!e.observer.sample_owned(e.identity,prior))return false;
        auto counts_before=e.cycle.inventory();counts_before.revision=prior.revision;
        counts_before.loaded=prior.loaded;counts_before.reserve=prior.reserve;
        if(!e.cycle.confirm(counts_before))return false;
    }
    auto next_observer=e.observer;auto next_cycle=e.cycle;inv::Inventory observed_inventory{};
    const auto fresh_identity=inv::identity(f);
    if(!(e.identity==fresh_identity)){
        if(!next_observer.rebind_hold(f,e.identity,observed_inventory))return false;
    }else if(!next_observer.sample(f,observed_inventory))return false;
    if(!next_cycle.rebind_observation(counts(f,observed_inventory)))return false;
    e.identity=fresh_identity;e.observer=next_observer;e.cycle=next_cycle;return true;
}
void service(hand::Frame const& f) noexcept {
    if(!unified_weapon_fire::manual_profile()||!weaponhand::current_gt(nullptr)||
       !playground::instance(f.weapon.address,L"Weap_Shotgun"))return;
    auto* e=entry(f,true);if(!e)return;
    e->preparation_stage=1;e->preparation_flags=0;e->preparation_rejection=0;
    e->prepared=false; // A failed current read cannot retain an earlier shot admission.
    // An original insertion's receipt belongs to its OLD full inventory
    // identity. The receipt service resolves it before any hold rebind.
    if(gnasher_inventory::blocks_fire(f.weapon)||gnasher_state::blocks_fire(f.weapon)){e->preparation_stage=2;e->prepared=false;return;}
    // A new hold never borrows the old prepared source. Retained shot/cycle
    // obligations stay on the exact original actor until explicit completion.
    if(e->cycle.faulted()){e->preparation_stage=3;e->prepared=false;return;}
    const auto prior_identity=e->cycle.inventory().identity;
    if(e->cycle.inventory().revision&&(!(e->identity==inv::identity(f))||
       !(prior_identity==mechanical_identity(f)))){
        if(!rebind_observation(*e,f)){e->preparation_stage=4;return;}
    }
    // Keep the known unbound entry (and therefore the fire veto), but do not
    // read ammunition before the VR equipment model actually holds this gun.
    // Existing original-shot/count responsibilities retain the normal path.
    if(e->mechanical.stage()==gears_weapon::GnasherMechanicalStage::unbound&&
       !e->cycle.inventory().revision&&!e->cycle.pending()&&!e->cycle.confirmed()&&!e->cycle.blocked()&&
       !vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)){
        e->preparation_stage=15;return;
    }
    // The pre-mechanics call cannot prepare an initial idle shot from the
    // previous controller sample. Reject that already-invalid source before
    // the expensive inventory read. After raw mechanics consumes this source,
    // the normal call still reads actual inventory before any fire admission.
    // Pending/confirmed shots, reloads and mechanical responsibilities retain
    // their original receipt/readback path even across a source mismatch.
    if(f.left_grip_known&&!f.left_grip_down&&!e->cycle.pending()&&!e->cycle.confirmed()&&!e->cycle.blocked()&&
       e->mechanical.stage()==gears_weapon::GnasherMechanicalStage::ready&&
       !e->mechanical.reload_active()&&!e->mechanical.mechanism_pending()&&
       !e->mechanical.hold_suspended()&&!e->mechanical.animation_owns_hands()&&
       !e->mechanical.source_matches(f.controllers.sequence,f.controllers.sync_ns)){
        e->preparation_stage=9;return;
    }
    inv::Inventory inventory_observed{};
    if(!e->observer.sample(f,inventory_observed)){e->preparation_stage=5;return;}
    const auto value=counts(f,inventory_observed);
    if(e->cycle.pending())e->cycle.confirm(value);
    if(e->cycle.confirmed()){
        const auto receipt=e->cycle.receipt();
        if(e->mechanical.stage()!=gears_weapon::GnasherMechanicalStage::unbound&&e->mechanical_shot!=receipt.original_event_sequence){
            if(e->mechanical.accept_original_shot(receipt,e->admitted_shot).event!=
               gears_weapon::GnasherMechanicalEvent::shot_recorded){e->preparation_stage=6;e->prepared=false;return;}
            e->mechanical_shot=receipt.original_event_sequence;
        }
        if(e->reported_shot!=receipt.original_event_sequence){
            e->reported_shot=receipt.original_event_sequence;
            if(log_file){fwprintf(log_file,L"gnasher_original_shot_confirmed shot=%llu weapon=%llx loaded=%u reserve=%u game_tick_us=%lld physical_cycle_pending=1\n",
                receipt.original_event_sequence,f.weapon.address,receipt.observed.loaded,receipt.observed.reserve,receipt.game_tick);fflush(log_file);}
        }
    }
    if(e->cycle.blocked()){e->preparation_stage=7;e->prepared=false;return;}
    // Do not spend a legacy first shot before this actual instance has its
    // physical controller. Installing after that debt exists is forbidden.
    if(e->mechanical.stage()==gears_weapon::GnasherMechanicalStage::unbound){e->preparation_stage=8;return;}
    const auto prior=e->cycle.inventory();
    if(e->mechanical.stage()!=gears_weapon::GnasherMechanicalStage::unbound){
        if(!e->mechanical.source_matches(f.controllers.sequence,f.controllers.sync_ns)){e->preparation_stage=9;e->prepared=false;return;}
        const auto fire_now=weaponhand::clock_ns(nullptr);
        if(!e->mechanical.can_request_fire(fire_now,value)){e->preparation_stage=10;e->preparation_flags=e->mechanical.fire_readiness_mask(fire_now,value);e->preparation_rejection=e->mechanical.observation_rejection();e->prepared=false;return;}
    }
    if(!prior.revision){if(!e->cycle.bind(value)){e->preparation_stage=11;return;}}
    else if(!e->cycle.prepare(value)){e->preparation_stage=12;e->prepared=false;return;}
    float seconds{};const double scale=1000000.;
    if(!lancerreload_state::time_field(f.world.address,L"TimeSeconds",seconds)||!std::isfinite(seconds)||seconds<0||
       double(seconds)*scale>9e15||!nativeequipment::current(nullptr,f)){e->preparation_stage=13;e->prepared=false;return;}
    e->source=f;e->profile=f.controllers.profile_generation;e->reference=f.reference_generation;
    e->prepared_tick=static_cast<std::int64_t>(double(seconds)*scale);e->prepared=true;e->preparation_stage=14;
}
bool cohort_matches(Entry const& e,hand::GripSnapshot const& g) noexcept {
    const auto& f=e.source;
    return hand::same(g.key,f.key)&&hand::same(g.world,f.world)&&hand::same(g.pawn,f.pawn)&&
        hand::same(g.controller,f.controller)&&hand::same(g.weapon,f.weapon)&&hand::same(g.mesh,f.mesh)&&
        g.input_sequence==f.controllers.sequence&&g.press_token==f.press_token&&
        g.input_session==f.stereo.session&&g.space_epoch==f.stereo.space_epoch&&
        g.input_epoch==f.controllers.input_epoch&&g.profile_generation==e.profile&&
        g.profile_generation==f.controllers.profile_generation&&g.reference_generation==e.reference&&
        g.reference_generation==f.reference_generation;
}
// Existing CallFunction subscriber, root only; no engine/getter/input calls.
// Preserve the actual admission time separately from a delayed return receipt.
void begin(abi::Invocation const& call,shot::Token const& token) noexcept {
    if(!token.entered||token.previous_depth||!abi::invocation_valid(call)||!weaponhand::current_gt(nullptr)||
       call.thread_id!=GetCurrentThreadId()||!playground::named(call.callee,L"CustomFire"))return;
    for(auto& e:entries){
        if(!e.known||!e.prepared||call.self!=e.identity.weapon.address||
           call.hand_cookie!=e.identity.registration||call.native_started_qpc!=e.identity.native_start||
           e.mechanical.stage()==gears_weapon::GnasherMechanicalStage::unbound)continue;
        shot::Packet packet{};
        if(!weaponshot::stack.invocation_packet(token,call.callee,e.identity.weapon,packet)||
           packet.root!=shot::Role::shotgun_custom||packet.mode!=shot::Role::shotgun_custom||
           !cohort_matches(e,packet.grip))return;
        if(e.admitted_shot.accepted&&e.admitted_shot.original_event_sequence==packet.shot)return;
        const auto now=weaponhand::clock_ns(nullptr);const auto& f=e.source;
        const auto before=e.mechanical.inventory();
        if(!e.mechanical.source_matches(f.controllers.sequence,f.controllers.sync_ns)||
           !e.mechanical.can_request_fire(now,e.cycle.inventory()))return;
        e.admitted_shot={mechanical_identity(f),packet.shot,f.controllers.sequence,
            f.controllers.sync_ns,now,before,true};e.accepted_call=call.call_id;return;
    }
}
void returned(abi::Invocation const& call,shot::Token const& token) noexcept {
    if(!token.entered||token.previous_depth||!weaponhand::current_gt(nullptr)||call.thread_id!=GetCurrentThreadId()||
       !playground::named(call.callee,L"CustomFire"))return;
    for(auto& e:entries){
        if(!e.known||!e.prepared||call.self!=e.identity.weapon.address||
           call.hand_cookie!=e.identity.registration||call.native_started_qpc!=e.identity.native_start)continue;
        shot::Packet packet{};
        if(!weaponshot::stack.invocation_packet(token,call.callee,e.identity.weapon,packet)||
           packet.root!=shot::Role::shotgun_custom||packet.mode!=shot::Role::shotgun_custom)continue;
        if(!cohort_matches(e,packet.grip)||
           (e.mechanical.stage()!=gears_weapon::GnasherMechanicalStage::unbound&&
            (!e.admitted_shot.accepted||e.admitted_shot.original_event_sequence!=packet.shot||e.accepted_call!=call.call_id))){
            // This is the exact OWN original root return, not a malformed
            // token/foreign call. A missing cohort must never leave the
            // stock cocking timer free to refire on held R2.
            (void)e.cycle.unresolved_return(packet.shot);e.prepared=false;return;
        }
        // Use the accepted source's GAME clock. This records an observation,
        // never renews source freshness or authorizes another shot.
        (void)e.cycle.returned(packet.shot,weaponhand::clock_ns(nullptr),e.prepared_tick);
        e.prepared=false;return;
    }
}
// Called before the exact original root scope leaves on an unknown/unwound
// outcome. This denies refire; it creates no confirmed shot/count receipt.
void abandoned(abi::Invocation const& call,shot::Token const& token) noexcept {
    if(token.previous_depth||!weaponhand::current_gt(nullptr)||call.thread_id!=GetCurrentThreadId())return;
    for(auto& e:entries){
        if(!e.known||call.self!=e.identity.weapon.address||call.hand_cookie!=e.identity.registration||
           call.native_started_qpc!=e.identity.native_start)continue;
        shot::Packet packet{};
        if(!weaponshot::stack.invocation_packet(token,call.callee,e.identity.weapon,packet)||
           packet.root!=shot::Role::shotgun_custom||packet.mode!=shot::Role::shotgun_custom)return;
        (void)e.cycle.unresolved_return(packet.shot);e.prepared=false;return;
    }
}
} // namespace gnasher_fire
