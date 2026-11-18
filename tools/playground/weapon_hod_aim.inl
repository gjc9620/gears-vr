// HOD original hold-to-aim. Separate from R2/beam effect debt and left
// support ownership. Head pose never supplies the gun's query direction.
#if !defined(GEARS_PLAYGROUND_NATIVE_LANCER_L2_TRIAL)
namespace hod_aim {
bool owed() noexcept {return false;}void service(bool=false) noexcept {}void cancel() noexcept {}
bool yield_for_equipment(gears_weapon_hand::Frame const&) noexcept {return true;}
void advance(unified_weapon_fire::MuzzleSample const&,hod_state::Observed const&) noexcept {}
}
#else
namespace vrfire {extern Name l2;bool name_current(Name const&,wchar_t) noexcept;}
namespace hod_aim {
namespace hand=gears_weapon_hand;namespace api=hod_original;
using Action=gears_weapon::OriginalL2Action;
gears_weapon::OriginalL2Aim input;
struct Journal {
    hand::Frame source{};hand::ExistingActionLifetime lifetime{};api::Entry release{};Name l2{};
    bool dispatched{},release_sent{};
};
Journal journal;std::atomic<bool> retained{};bool faulted{},cancelling{};unsigned logs{};
bool owed() noexcept {return retained.load(std::memory_order_acquire);}
void report(const wchar_t* event,bool returned) noexcept {
    if(log_file&&logs++<12){fwprintf(log_file,L"hod_aim event=%ls returned=%u owed=%u\n",event,unsigned(returned),unsigned(owed()));fflush(log_file);}
}
void observe_stop() noexcept {
    if(!owed()||!journal.release_sent)return;
    hod_state::Observed actual{};
    if(hod_state::read(journal.source,actual)&&actual.aim_stopped()){
        report(L"stopped",true);journal={};retained.store(false,std::memory_order_release);
    }
}
bool release_original() noexcept {
    if(!owed()||!journal.dispatched)return false;
    if(journal.release_sent)return true;
    bool sent{};const bool returned=api::execute(journal.source,journal.release,journal.l2,sent,
        &journal.lifetime,api::Button::aim);
    journal.release_sent=sent;
    if(sent)report(L"release_return",returned);
    return returned;
}
void service(bool force=false) noexcept {
    if(!weaponhand::current_gt(nullptr))return;
    if(force)cancelling=true;
    if(!owed())return;
    if(cancelling){
        hod_state::Observed actual{};
        if(hod_state::read(journal.source,actual)){
            gears_weapon::OriginalL2Context context{journal.source.world.address,journal.source.controller.address,
                journal.source.weapon.address,true,actual.aim_key,actual.toggle_targeting,false};
            const auto result=input.yield(context,[&](Action action,std::uint64_t controller) noexcept {
                return action==Action::release&&controller==journal.source.controller.address&&release_original();
            });
            if(result==gears_weapon::OriginalL2Status::abort_required)faulted=true;
        }
        // Unknown original press still owes one same-selected-owner release.
        if(!journal.release_sent)(void)release_original();
    }
    observe_stop();
}
void cancel() noexcept {service(true);}
bool yield_for_equipment(hand::Frame const& f) noexcept {
    if(!owed()||!hand::same(f.weapon,journal.source.weapon))return true;
    service(true);return !owed()&&nativeequipment::current(nullptr,f);
}
bool press(unified_weapon_fire::MuzzleSample const& sample,hod_state::Observed const& actual) noexcept {
    const auto& f=sample.source;
    if(owed()||faulted||actual.toggle_targeting||!actual.aim_stopped()||!hod_trace::live(sample)||
       !nativeequipment::current(nullptr,f))return false;
    Journal next{};api::Entry entry{};next.l2=vrfire::l2;
    if(!vrfire::name_current(next.l2,L'L')||!api::bind(f.controller,api::Method::press,entry)||
       !api::bind(f.controller,api::Method::release,next.release)||
       !weaponhand::client.capture_existing_action_lifetime_on_gt(f.key,next.lifetime))return false;
    const auto now=weaponhand::clock_ns(nullptr);
    if(now>INT64_MAX-20000000||!unified_weapon_fire::live_sample(sample,now+20000000)||
       !nativeequipment::current(nullptr,f))return false;
    next.source=f;journal=next;retained.store(true,std::memory_order_release);cancelling=false;
    bool sent{};const bool returned=api::execute(f,entry,next.l2,sent,nullptr,api::Button::aim);
    journal.dispatched=sent;
    if(!sent){journal={};retained.store(false,std::memory_order_release);return false;}
    report(L"press_return",returned);
    if(!returned)faulted=cancelling=true;
    return returned;
}
void advance(unified_weapon_fire::MuzzleSample const& sample,hod_state::Observed const& actual) noexcept {
    if(faulted){service(true);return;}
    const auto& f=sample.source;
    const bool allow=!cancelling&&!(owed()&&journal.release_sent)&&hod_trace::live(sample)&&
        nativeequipment::current(nullptr,f);
    gears_weapon::OriginalL2Context context{f.world.address,f.controller.address,f.weapon.address,
        true,actual.aim_key,actual.toggle_targeting,allow};
    const auto result=input.advance(f.controllers,f.stereo,context,weaponhand::clock_ns(nullptr),
        [&](Action action,std::uint64_t controller) noexcept {
            return controller==f.controller.address&&(action==Action::press?press(sample,actual):release_original());
        },f.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated);
    if(result==gears_weapon::OriginalL2Status::abort_required)faulted=cancelling=true;
    if(!owed()&&cancelling&&!input.owns_press())cancelling=false;
    service();
}
}
#endif
