#if defined(GEARS_BODY_MAIN_MOUNT_SOURCE) && GEARS_BODY_MAIN_MOUNT_SOURCE
namespace body_tick_source_live {
namespace source=::gears_body_tick_source;namespace wire=::gears_body_main_mount_provider;
using Parent=::body_tick_source_live::Parent;
namespace input=gears_body_tick_input;namespace rig=rig_tracking_live;namespace api=gears_rig_tracking;
bool enabled{};source::Input latch{},working{},last{};
bool active{},input_attempted{},acquired{},write_valid{},hard_failed{},age_burned{},have_last{};
std::uint64_t attempted_tick{},attempted_native{};
std::atomic<std::uint64_t> owner_thread{};
std::atomic<std::uint64_t> attempts{},acquisitions{},unavailable{},age_rejected{},hard_rejected{},ends{};
std::atomic<std::uint64_t> partial_scene_continuity{},partial_scene_controllers{};
// Bounded opt-in observations of the actual rejecting calls. These records
// never reacquire tracking, change the latch, or supply a writer permit.
struct RejectedSource {
    int64_t at_ns{},capture_begin{},metadata_begin{},tracking_begin{},tracking_end{},stereo_received{},controller_received{},sync{};
    uint64_t tick{};unsigned stage{},reason{},verdict{};
};
std::array<RejectedSource,64> rejected_sources{};unsigned rejected_source_count{};
void note_rejected(source::Input const& s,int64_t now,int64_t capture,unsigned stage,unsigned reason,input::Verdict verdict)noexcept {
    if(!rig::metadata_reject_trace||GetCurrentThreadId()!=owner_thread.load()||rejected_source_count>=rejected_sources.size())return;
    auto const& v=s.observed;
    // Retained-pose checks can observe the same already rejected Tick many
    // times. Keep its first actual rejection, not 64 copies of one event.
    for(unsigned i=0;i<rejected_source_count;++i){auto const& r=rejected_sources[i];
        if(r.tick==v.tick&&r.stage==stage&&r.reason==reason&&r.verdict==unsigned(verdict))return;}
    rejected_sources[rejected_source_count++]={now,capture,s.metadata_begin_ns,v.begin_ns,v.end_ns,
        v.stereo.received_ns,v.controllers.received_ns,v.controllers.sync_ns,v.tick,stage,reason,unsigned(verdict)};
}
bool same_reference(api::NativeReference const&a,api::NativeReference const&b)noexcept {
    return a.epoch==b.epoch&&a.flags==b.flags&&!a.reserved&&!b.reserved;
}
void begin(std::uint64_t tick,std::uint64_t thread,gears_native_rig_tracking::CoreSnapshot const& original,std::uint64_t revision,std::int64_t metadata_begin)noexcept {
    if(!enabled||!tick||!revision||GetCurrentThreadId()!=thread||original.native.game_thread!=thread||
       weapon_hand_scene_binding::game_thread.load()!=thread)return;
    owner_thread.store(thread,std::memory_order_release);
    if(attempted_tick==tick&&attempted_native==original.native.native_started_qpc)return;
    attempted_tick=tick;attempted_native=original.native.native_started_qpc;
    active=true;input_attempted=acquired=write_valid=hard_failed=age_burned=false;++attempts;
    working={};working.scene_cache_revision=revision;
    working.metadata_begin_ns=metadata_begin;working.metadata_end_ns=rig::clock_ns();auto& v=working.observed;
    std::memcpy(&v.parent_scene,&original,sizeof original);v.tick=tick;v.thread=thread;
    // Preserve the original metadata observation. The sole coherent tracking
    // read is deferred to the canonical Compose that will consume it. Do not
    // spend the tracking deadline on the game's earlier animation work.
}
void acquire_for_compose(std::uint64_t tick,std::uint64_t thread)noexcept {
    if(!enabled||!active||input_attempted||hard_failed||tick!=attempted_tick||
       GetCurrentThreadId()!=thread||thread!=owner_thread.load(std::memory_order_acquire)||
       working.observed.tick!=tick||working.observed.thread!=thread)return;
    input_attempted=true; // A failed acquisition is never retried this Tick.
    auto& v=working.observed;auto const& original=v.parent_scene;
    auto* broker=weapon_hand_runtime::current();v.parent_cookie=broker?broker->cookie():0;v.begin_ns=rig::clock_ns();
    try{
        // One actual acquisition, shared with the existing producer primitive;
        // old body_tick_input_live::begin is NOT called in this mode.
        if(!rig::metadata_copy_current(original,working.scene_cache_revision)){
            note_rejected(working,rig::clock_ns(),0,5,1,input::Verdict::hard);++unavailable;return;}
        body_tick_input_live::AcquireFailure acquisition{};
        if(!body_tick_input_live::acquire(v,&acquisition,true)){
            note_rejected(working,rig::clock_ns(),0,4,unsigned(acquisition),input::Verdict::hard);++unavailable;return;}
        v.end_ns=rig::clock_ns();
        const auto verdict=source::classify(working,original,v.end_ns,simulator_operator);
        if(verdict!=input::Verdict::okay){
            const unsigned detail=rig::metadata_reject_trace&&verdict==input::Verdict::hard?
                source::rejection_detail(working,original,v.end_ns,simulator_operator):0;
            note_rejected(working,v.end_ns,0,1,detail,verdict);
            if(verdict==input::Verdict::source_age_only)++age_rejected;else ++hard_rejected;return;}
        if(!(original.flags&gears_rig_tracking::controllers_matched)&&
           (!have_last||!source::independent_domain_matches(working,last))){
            note_rejected(working,v.end_ns,0,6,have_last?2:1,input::Verdict::hard);++hard_rejected;return;}
        if(have_last){auto const& p=last.observed;
            if(p.parent_scene.native.native_started_qpc==original.native.native_started_qpc&&p.parent_scene.world_generation==original.world_generation&&
               p.reference_revision==v.reference_revision&&p.stereo.source==v.stereo.source&&p.stereo.session==v.stereo.session&&p.stereo.space_epoch==v.stereo.space_epoch&&
               p.publisher_pid==v.publisher_pid&&p.publisher_start==v.publisher_start){
                if(v.stereo.sequence<p.stereo.sequence||v.stereo.received_ns<p.stereo.received_ns||v.stereo.predicted_time<p.stereo.predicted_time||
                   v.controllers.received_ns<p.controllers.received_ns||v.controllers.sync_ns<p.controllers.sync_ns||
                   !gears_current_continuity::not_regressed(v.continuity,p.continuity)||
                   (v.stereo.sequence==p.stereo.sequence&&(std::memcmp(&v.stereo,&p.stereo,sizeof v.stereo)||!gears_hand_continuity::same_input(v.controllers,p.controllers)))){++hard_rejected;return;}
            }
        }
        if(!rig::metadata_copy_current(original,working.scene_cache_revision)){
            note_rejected(working,rig::clock_ns(),0,5,2,input::Verdict::hard);++unavailable;return;}
        std::memcpy(&latch,&working,sizeof latch);std::memcpy(&last,&working,sizeof last);
        if(!(original.flags&gears_rig_tracking::producer_continuity_available))++partial_scene_continuity;
        if(!(original.flags&gears_rig_tracking::controllers_matched))++partial_scene_controllers;
        have_last=acquired=write_valid=true;++acquisitions;
    }catch(...){++unavailable;}
}
void end(std::uint64_t tick)noexcept {
    if(enabled&&GetCurrentThreadId()==owner_thread.load(std::memory_order_acquire)&&active&&tick==attempted_tick){active=acquired=write_valid=false;++ends;}
}
bool copy(std::uint64_t tick,std::uint64_t thread,gears_native_rig_tracking::CoreSnapshot const& original,source::Input& out)noexcept {
    if(!enabled||GetCurrentThreadId()!=owner_thread.load(std::memory_order_acquire)||GetCurrentThreadId()!=thread)return false;
    if(!active||!acquired||!write_valid||hard_failed||tick!=attempted_tick||
       latch.observed.tick!=tick||latch.observed.thread!=thread||std::memcmp(&latch.observed.parent_scene,&original,sizeof original))return false;
    std::memcpy(&out,&latch,sizeof out);return true;
}
input::Verdict observe(source::Input const& saved,gears_native_rig_tracking::CoreSnapshot const& original,
    std::uint64_t tick,std::uint64_t thread)noexcept {
    const auto reject=[&](unsigned reason){note_rejected(saved,rig::clock_ns(),0,2,reason,input::Verdict::hard);return input::Verdict::hard;};
    if(!enabled||GetCurrentThreadId()!=owner_thread.load(std::memory_order_acquire)||GetCurrentThreadId()!=thread)return input::Verdict::hard;
    if(!active||!acquired||hard_failed||tick!=attempted_tick||saved.observed.tick!=tick||saved.observed.thread!=thread||
       std::memcmp(&saved,&latch,sizeof saved)||std::memcmp(&original,&saved.observed.parent_scene,sizeof original))return reject(1);
    // Cookie0 at acquisition is the independent freehand bootstrap. Once a
    // real nonzero parent was observed, loss/replacement before mount select
    // is a hard source change, never a fallback permitting a freehand write.
    // This checks identity only, so a query's own pending callback is valid.
    if(saved.observed.parent_cookie){auto* broker=weapon_hand_runtime::current();
        if(!broker||!broker->registration_current(saved.observed.parent_cookie,original.native.native_started_qpc))return reject(2);}
    api::NativeReference reference{};std::uint64_t revision{};
    if(!rig::reference_snapshot(reference,revision)||revision!=saved.observed.reference_revision||!same_reference(reference,saved.observed.native_reference))return reject(3);
    // Every dependency/cache/reference/ticket is still checked. Only the old
    // render Scene's elapsed age is distinct from the actual current-Tick
    // metadata receipt. The live C0 context also rereads actor + model before
    // every write; an old receipt is bounded from its first read, never end.
    rig::TickFailure why{};const bool legacy=rig::current_copy(original,saved.scene_cache_revision,50000000,&why);
    if(!legacy&&why!=rig::TickFailure::source_age_only)return reject(4);
    const auto observed_now=rig::clock_ns();const auto verdict=source::classify(saved,original,observed_now,simulator_operator);
    if(verdict!=input::Verdict::okay)note_rejected(saved,observed_now,0,2,5,verdict);
    if(verdict==input::Verdict::hard)return verdict;
    return verdict;
}
input::Verdict current(source::Input const& saved,gears_native_rig_tracking::CoreSnapshot const& original,
    std::uint64_t tick,std::uint64_t thread,std::int64_t capture_begin)noexcept {
    // Recheck hard state even after a known age burn. Never revive write_valid.
    if(GetCurrentThreadId()!=owner_thread.load(std::memory_order_acquire)||GetCurrentThreadId()!=thread)return input::Verdict::hard;
    const auto verdict=observe(saved,original,tick,thread);
    if(verdict==input::Verdict::hard){hard_failed=true;write_valid=false;++hard_rejected;return verdict;}
    const auto now=rig::clock_ns();
    if(capture_begin<0||(capture_begin&&(capture_begin>now||capture_begin<saved.observed.end_ns))){note_rejected(saved,now,capture_begin,3,1,input::Verdict::hard);hard_failed=true;write_valid=false;++hard_rejected;return input::Verdict::hard;}
    if(age_burned||verdict==input::Verdict::source_age_only||(capture_begin&&std::uint64_t(now-capture_begin)>wire::tracking_age_ns)){
        note_rejected(saved,now,capture_begin,3,unsigned(age_burned)|unsigned(verdict==input::Verdict::source_age_only)*2|unsigned(capture_begin&&std::uint64_t(now-capture_begin)>wire::tracking_age_ns)*4,input::Verdict::source_age_only);
        age_burned=true;write_valid=false;++age_rejected;return input::Verdict::source_age_only;
    }
    return write_valid?input::Verdict::okay:input::Verdict::hard;
}
bool retained_current(source::Input const& saved,gears_native_rig_tracking::CoreSnapshot const& original,
    std::uint64_t tick,std::uint64_t thread)noexcept {
    // Caller must authenticate the exact already-consumed Kernel lease. This
    // observes only hard continuity and cannot grant a new pose write.
    if(GetCurrentThreadId()!=owner_thread.load(std::memory_order_acquire)||GetCurrentThreadId()!=thread)return false;
    const auto verdict=observe(saved,original,tick,thread);
    if(verdict==input::Verdict::hard){hard_failed=true;write_valid=false;return false;}
    return true;
}
bool parent_idle_current(Parent const& p)noexcept {
    if(!enabled||!p.cookie||!p.native_started||!p.revision||!p.generation||!p.tick||!p.compose)return false;
    auto* b=weapon_hand_runtime::current();if(!b)return false;
    // This is the NORMAL idle check for use after callback and before writes.
    // The broker's own reserved query must not call this during its callback.
    return b->registration_current(p.cookie,p.native_started)&&b->lifecycle_revision()==p.revision&&b->action_generation()==p.generation&&
        b->availability()==gears_weapon_hand::Availability::ready&&!b->action_barrier()&&!b->pending_action_invalidation()&&
        !b->pending_callbacks()&&!b->pending_transactions()&&!b->pending_aux_scopes()&&!b->aux_debt()&&
        b->registration_current(p.cookie,p.native_started)&&b->lifecycle_revision()==p.revision&&b->action_generation()==p.generation;
}
bool parent_current(Parent const& p)noexcept {
    if(!p.query||p.equipment_begin_ns<=0||p.equipment_end_ns<p.equipment_begin_ns)return false;
    const auto now=rig::clock_ns();if(now<p.equipment_end_ns||std::uint64_t(now-p.equipment_begin_ns)>wire::equipment_age_ns)return false;
    return body_tick_source_live::parent_idle_current(p);
}
bool configure(wchar_t const* ini,FILE* f,bool consumer){const auto request=GetPrivateProfileIntW(L"BodyMainMountSource",L"enabled",0,ini);
    if(request!=0&&request!=1)return false;enabled=request==1;
    if(enabled&&(!consumer||!body_tick_input_live::enabled||!rig::enabled||!weapon_hand_runtime::enabled||body_tick_tracking_probe_live::enabled))return false;
    if(f)fprintf(f,"body_mount_source_compiled=1 body_mount_source_enabled=%u distinct_tracking40_metadata50_equipment50=1 body_source_abi=3 scene_association_not_tick_metadata=1 legacy_V1_unchanged=1\n",unsigned(enabled));return true;
}
void summary(FILE* f)noexcept {if(f)fprintf(f,"body_mount_source_enabled=%u body_mount_source_attempts=%llu body_mount_source_acquired=%llu body_mount_source_unavailable=%llu body_mount_source_age_rejected=%llu body_mount_source_hard_rejected=%llu body_mount_source_ends=%llu body_mount_source_partial_scene_continuity=%llu ",unsigned(enabled),attempts.load(),acquisitions.load(),unavailable.load(),age_rejected.load(),hard_rejected.load(),ends.load(),partial_scene_continuity.load());if(f)fprintf(f,"body_mount_source_partial_scene_controllers=%llu ",partial_scene_controllers.load());}
}
#endif
