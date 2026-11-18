#if !defined(GEARS_PLAYGROUND_NATIVE_LANCER_R2_TRIAL)
namespace hod_input {
bool owed() noexcept {return false;}void service(bool=false) noexcept {}void cancel() noexcept {}
bool yield_for_equipment(gears_weapon_hand::Frame const&) noexcept {return true;}
void advance(unified_weapon_fire::MuzzleSample const&,hod_state::Observed const&) noexcept {}
}
#else
// Exact-owner original R2 + effect lifetime. No new input source, timer,
// damage/effect writer or reload. The common R2 ledger handles edge replay.
namespace vrfire {extern Name r2;bool name_current(Name const&,wchar_t) noexcept;}
namespace hod_input {
namespace hand=gears_weapon_hand;namespace api=hod_original;
using Action=gears_weapon::OriginalR2Action;using Execution=gears_weapon::OriginalR2Execution;
using Identity=gears_weapon::HammerOfDawnIdentity;
gears_weapon::OriginalR2Fire input;
gears_weapon::HammerOfDawnUseOwner debt;
struct Journal {
    hand::Frame source{};hand::ExistingActionLifetime lifetime{};
    api::Entry release{},end{};hand::ObjectIdentity manager{};Name r2{};
    std::uint64_t hold{},serial{};bool end_unknown{},equipment_yielded{};
};
Journal journal;std::uint64_t next_serial{},read_serial{};
std::atomic<bool> retained{};bool cancelling{},faulted{};unsigned logs{};
bool owed() noexcept {return retained.load(std::memory_order_acquire);}
Identity identity(hand::Frame const& f,std::uint64_t hold) noexcept {
    return {f.world.address,f.key.world_generation,f.pawn.address,f.controller.address,
        f.weapon.address,hold,f.controllers.session,f.controllers.space_epoch,f.controllers.input_epoch};
}
void report(const wchar_t* event,unsigned status) noexcept {
    if(log_file&&logs++<24){hod_trace::UseCounters counters{};
        const bool known=hod_trace::use_counters(journal.serial,counters);
        fwprintf(log_file,L"hod_input event=%ls serial=%llu status=%u owed=%u key_owned=%u press_unknown=%u ray_use_known=%u now_ns=%lld use_query_returns=%llu use_query_ray_commits=%llu use_fx_returns=%llu use_fx_origin_bodies=%llu use_fx_aim_bodies=%llu use_fx_paired_bodies=%llu\n",
        event,journal.serial,status,unsigned(owed()),unsigned(input.owns_press()),unsigned(debt.call_unknown()),unsigned(known),weaponhand::clock_ns(nullptr),
        counters.query_returns,counters.query_ray_commits,counters.fx_returns,counters.fx_origin_bodies,counters.fx_aim_bodies,counters.fx_paired_bodies);
        fflush(log_file);}
}
bool state(hod_state::Observed& out) noexcept {
    return debt.retained()&&journal.serial==debt.serial()&&hod_state::read(journal.source,out)&&
        hand::same(out.manager,journal.manager);
}
Execution release_original() noexcept {
    if(!debt.retained()||!debt.press_invoked())return Execution::failed;
    if(debt.release_invoked())return Execution::executed; // Readback polling never repeats a release.
    hod_state::Observed before{};
    if(!state(before)||!debt.begin_release(journal.serial,identity(journal.source,journal.hold)))return Execution::not_ready;
    bool sent{};const bool returned=api::execute(journal.source,journal.release,journal.r2,sent,&journal.lifetime);
    if(!debt.released(journal.serial,sent,returned)){faulted=true;return Execution::failed;}
    if(sent)report(L"release_return",unsigned(returned));
    return returned?Execution::executed:sent?Execution::failed:Execution::not_ready;
}
void observe_stop() noexcept {
    if(!debt.retained()||!debt.release_invoked())return;
    hod_state::Observed actual{};if(!state(actual))return;
    const bool needs_end=actual.laser||actual.beam_firing||actual.pending_beam||actual.target_live;
    if(needs_end&&debt.begin_fallback(journal.serial,identity(journal.source,journal.hold),true)){
        bool sent{};const bool returned=api::execute(journal.source,journal.end,journal.r2,sent,&journal.lifetime);
        if(!debt.fallback(journal.serial,sent)){faulted=true;return;}
        if(sent){journal.end_unknown=!returned;report(L"end_return",unsigned(returned));}
        if(!returned||!state(actual))return;
    }
    if(!actual.stopped())return;
    if(read_serial==UINT64_MAX){faulted=true;return;}
    const auto id=debt.identity();
    gears_weapon::HammerOfDawnStopReadback proof{id.weapon,id.weapon_generation,journal.serial,++read_serial,
        true,!actual.target_live,!actual.beam_firing&&!actual.pending_beam,!actual.laser,
        !actual.valid_emitter,!actual.loop_playing&&!actual.beam_audio,!actual.pending_beam};
    if(debt.stopped(journal.serial,proof)){
        report(L"effects_stopped",0);hod_trace::end_use_observation(journal.serial);
        journal={};retained.store(false,std::memory_order_release);
    }
}
// force seals the common ledger before dispatch. Cleanup can only target the
// exact journaled original owner and still-selected weapon; it cannot migrate.
void service(bool force=false) noexcept {
    if(!weaponhand::current_gt(nullptr))return;
    if(force)cancelling=true;
    if(!debt.retained())return;
    if(cancelling){
        hod_state::Observed actual{};
        if(state(actual)){
            gears_weapon::OriginalR2Context context{journal.source.world.address,journal.source.controller.address,
                journal.source.weapon.address,true,actual.input_down,false,true};
            const auto result=input.yield(context,[&](Action action,std::uint64_t controller) noexcept {
                if(action!=Action::release||controller!=journal.source.controller.address)return Execution::failed;
                return release_original();
            });
            if(result==gears_weapon::OriginalR2Status::abort_required)faulted=true;
        }
        // An unknown press may never have reached the common ledger's owned
        // state. Its durable effect journal still requires one original OFF.
        if(!debt.release_invoked())(void)release_original();
    }
    observe_stop();
}
void cancel() noexcept {service(true);}
bool prepare_equipment_stow(hand::Frame const& f,bool& stop_issued) noexcept {
    stop_issued=false;
    if(!owed())return true;
    // A different weapon neither inherits nor clears this HOD journal.
    if(!hand::same(f.weapon,journal.source.weapon))return true;
    service(true);
    if(!nativeequipment::current(nullptr,f))return false;
    if(!owed())return true;
    // Original EndFire aborts the beam immediately, while its cooldown/audio
    // can fade afterwards. Preserve that exact owner's debt and continue
    // observing it; an audio tail must not consume a valid equipment switch.
    // All original OFF calls have already completed before changing weapons.
    hod_state::Observed actual{};
    stop_issued=!faulted&&!journal.end_unknown&&!debt.call_unknown()&&debt.release_invoked()&&
        !input.owns_press()&&journal.hold==weaponhand::held_binding.generation&&
        hand::same(f.world,journal.source.world)&&hand::same(f.pawn,journal.source.pawn)&&
        hand::same(f.controller,journal.source.controller)&&
        identity(f,journal.hold)==debt.identity();
    const bool ready=stop_issued&&state(actual)&&!actual.input_down&&!actual.laser&&!actual.valid_emitter&&
        !actual.beam_firing&&!actual.pending_beam&&!actual.target_live;
    if(ready&&!journal.equipment_yielded){journal.equipment_yielded=true;report(L"equipment_visuals_stopped",0);}
    return ready;
}
bool yield_for_equipment(hand::Frame const& f) noexcept {
    bool stop_issued{};
    return prepare_equipment_stow(f,stop_issued);
}
Execution press(unified_weapon_fire::MuzzleSample const& sample,hod_state::Observed const& actual) noexcept {
    const auto& f=sample.source;
    if(faulted||owed()||!actual.stopped()||!hod_trace::live(sample)||!nativeequipment::current(nullptr,f)||
       sample.hold_generation!=weaponhand::held_binding.generation||next_serial==UINT64_MAX)return Execution::not_ready;
    Journal next{};api::Entry entry{};next.r2=vrfire::r2;
    if(!vrfire::name_current(next.r2,L'R')||!api::bind(f.controller,api::Method::press,entry)||
       !api::bind(f.controller,api::Method::release,next.release)||!api::bind(f.weapon,api::Method::end_fire,next.end)||
       !weaponhand::client.capture_existing_action_lifetime_on_gt(f.key,next.lifetime))return Execution::not_ready;
    next.source=f;next.manager=actual.manager;next.hold=sample.hold_generation;next.serial=++next_serial;
    const auto now=weaponhand::clock_ns(nullptr);
    // All reflective work stays inside the original 100ms source lease.
    if(now>INT64_MAX-20000000||!unified_weapon_fire::live_sample(sample,now+20000000)||
       !nativeequipment::current(nullptr,f)||!debt.reserve(identity(f,next.hold),next.serial))return Execution::not_ready;
    journal=next;retained.store(true,std::memory_order_release);cancelling=false;
    hod_trace::begin_use_observation(sample,next.serial);
    bool sent{};const bool returned=api::execute(f,entry,next.r2,sent);
    if(!debt.pressed(next.serial,sent,returned)){faulted=true;return Execution::failed;}
    if(!sent){hod_trace::end_use_observation(journal.serial);journal={};retained.store(false,std::memory_order_release);return Execution::not_ready;}
    report(L"press_return",unsigned(returned));
    if(!returned){cancelling=faulted=true;return Execution::failed;}
    return Execution::executed;
}
void advance(unified_weapon_fire::MuzzleSample const& sample,hod_state::Observed const& actual) noexcept {
    const auto& f=sample.source;
    if(faulted){service(true);return;}
    // While fading or otherwise awaiting a full stop, no new press can enter.
    const bool stopping=debt.retained()&&debt.release_invoked();
    const bool allow=!cancelling&&!stopping&&hod_trace::live(sample)&&nativeequipment::current(nullptr,f);
    gears_weapon::OriginalR2Context context{f.world.address,f.controller.address,f.weapon.address,
        true,actual.input_down,allow,true};
    const auto result=input.advance(f.controllers,f.stereo,context,weaponhand::clock_ns(nullptr),
        [&](Action action,std::uint64_t controller) noexcept {
            if(controller!=f.controller.address)return Execution::failed;
            return action==Action::press?press(sample,actual):release_original();
        },f.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated);
    if(result==gears_weapon::OriginalR2Status::abort_required){faulted=true;cancelling=true;}
    if(result==gears_weapon::OriginalR2Status::pressed||result==gears_weapon::OriginalR2Status::released||
       result==gears_weapon::OriginalR2Status::tapped||result==gears_weapon::OriginalR2Status::abort_required)
        report(L"edge",unsigned(result));
    if(!debt.retained()&&cancelling&&!input.owns_press()){
        // The shared ledger still requires its fresh release baseline. Do not
        // reset it to manufacture a new edge after an interruption.
        cancelling=false;
    }
    service();
}
} // namespace hod_input

#endif
