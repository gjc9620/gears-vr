#if defined(GEARS_BODY_TICK_INPUT) && GEARS_BODY_TICK_INPUT
namespace body_tick_input_live {
namespace input=::gears_body_tick_input;using Parent=::body_tick_input_live::Parent;
namespace api=gears_rig_tracking;namespace rig=rig_tracking_live;
bool enabled{};input::Input latch{},working{},last{};bool have_last{},valid{},active{};
std::uint64_t attempted_tick{},attempted_native{};
std::atomic<std::uint64_t> attempts{},acquired{},unavailable{},age_rejected{},hard_rejected{},ends{};
bool reference_equal(api::NativeReference const&a,api::NativeReference const&b)noexcept{return a.epoch==b.epoch&&a.flags==b.flags&&!a.reserved&&!b.reserved;}
bool acquire(input::Input& out) {
    auto const& old=out.parent_scene;const auto now=rig::clock_ns();
    if(!gears_native_context::engine_context_clear(gears_native_context::thread_ledger,true))return false;
    std::unique_lock<decltype(stereo_mutex)> lock(stereo_mutex,std::try_to_lock);if(!lock.owns_lock())return false;
    if(!rig::reference_snapshot(out.native_reference,out.reference_revision)||out.reference_revision!=old.native_reference_revision||
       !reference_equal(out.native_reference,old.native_reference))return false;
    if(!stereo_bridge.read(out.stereo)||!gears_vr::stereo_geometry_valid(out.stereo)||!gears_vr::stereo_render_active(out.stereo)||
       out.stereo.focused!=1||!gears_vr::stereo_tracking_valid(out.stereo,simulator_operator))return false;
    if(!cover_input_live::bridge_open||!cover_input_live::controllers.read_matching(out.stereo,now,out.controllers,simulator_operator)||
       !cover_input_live::controllers.publisher_identity(out.publisher_pid,out.publisher_start))return false;
    gears_hand_continuity::Publisher publisher{};
    if(!cover_shared_input_live::continuity_open||!cover_shared_input_live::continuity_bridge.read_matching(out.stereo,now,out.continuity,publisher,simulator_operator)||
       publisher.pid!=out.publisher_pid||publisher.start!=out.publisher_start)return false;
    api::NativeReference again{};std::uint64_t revision{};
    if(!rig::reference_snapshot(again,revision)||revision!=out.reference_revision||!reference_equal(again,out.native_reference))return false;
    return true;
}
void begin(std::uint64_t tick,std::uint64_t thread,gears_native_rig_tracking::CoreSnapshot const& source)noexcept {
    if(!enabled||!tick||GetCurrentThreadId()!=thread||source.native.game_thread!=thread)return;
    if(attempted_tick==tick&&attempted_native==source.native.native_started_qpc)return;
    attempted_tick=tick;attempted_native=source.native.native_started_qpc;active=true;valid=false;++attempts;
    working={};std::memcpy(&working.parent_scene,&source,sizeof source);working.tick=tick;working.thread=thread;
    auto* broker=weapon_hand_runtime::current();working.parent_cookie=broker?broker->cookie():0;working.begin_ns=rig::clock_ns();
    try{if(!acquire(working)){++unavailable;return;}}catch(...){++unavailable;return;}
    working.end_ns=rig::clock_ns();const auto verdict=input::classify(working,source,working.end_ns,simulator_operator);
    if(verdict!=input::Verdict::okay){if(verdict==input::Verdict::source_age_only)++age_rejected;else ++hard_rejected;return;}
    if(have_last&&last.parent_scene.native.native_started_qpc==source.native.native_started_qpc&&
       last.parent_scene.world_generation==source.world_generation&&last.reference_revision==working.reference_revision&&
       last.stereo.source==working.stereo.source&&last.stereo.session==working.stereo.session&&last.stereo.space_epoch==working.stereo.space_epoch&&
       last.publisher_pid==working.publisher_pid&&last.publisher_start==working.publisher_start){
        auto const&a=working.stereo;auto const&b=last.stereo;
        if(a.sequence<b.sequence||a.received_ns<b.received_ns||a.predicted_time<b.predicted_time||
           working.controllers.received_ns<last.controllers.received_ns||working.controllers.sync_ns<last.controllers.sync_ns||
           (a.sequence==b.sequence&&(std::memcmp(&a,&b,sizeof a)||!gears_hand_continuity::same_input(working.controllers,last.controllers)))){++hard_rejected;return;}
    }
    std::memcpy(&latch,&working,sizeof latch);std::memcpy(&last,&working,sizeof last);have_last=true;valid=true;++acquired;
}
void end(std::uint64_t tick)noexcept {if(enabled&&active&&tick==attempted_tick){active=valid=false;++ends;}}
bool copy(std::uint64_t tick,std::uint64_t thread,gears_native_rig_tracking::CoreSnapshot const& source,input::Input& out)noexcept {
    if(!enabled||!active||!valid||tick!=attempted_tick||thread!=GetCurrentThreadId()||latch.thread!=thread||
       latch.tick!=tick||std::memcmp(&latch.parent_scene,&source,sizeof source))return false;
    std::memcpy(&out,&latch,sizeof out);return true;
}
input::Verdict current(input::Input const& saved,gears_native_rig_tracking::CoreSnapshot const& source,
    std::uint64_t tick,std::uint64_t thread,std::int64_t basis_ns)noexcept {
    if(!enabled||!active||!valid||GetCurrentThreadId()!=thread||latch.thread!=thread||saved.tick!=tick||tick!=attempted_tick||
       std::memcmp(&saved,&latch,sizeof saved))return input::Verdict::hard;
    api::NativeReference ref{};std::uint64_t revision{};
    if(!rig::reference_snapshot(ref,revision)||revision!=saved.reference_revision||!reference_equal(ref,saved.native_reference))return input::Verdict::hard;
    const auto now=rig::clock_ns();const auto verdict=input::classify(saved,source,now,simulator_operator);
    if(verdict==input::Verdict::hard||basis_ns<0||(basis_ns&&(basis_ns>now||basis_ns<saved.begin_ns)))return input::Verdict::hard;
    if(verdict==input::Verdict::source_age_only||(basis_ns&&std::uint64_t(now-basis_ns)>40000000))return input::Verdict::source_age_only;
    return input::Verdict::okay;
}
bool parent_current(Parent const& p)noexcept {
    if(!enabled||!p.cookie||!p.native_started||!p.revision||!p.generation||!p.tick||!p.compose)return false;
    auto* b=weapon_hand_runtime::current();if(!b)return false;
    return b->registration_current(p.cookie,p.native_started)&&b->lifecycle_revision()==p.revision&&b->action_generation()==p.generation&&
        b->availability()==gears_weapon_hand::Availability::ready&&!b->action_barrier()&&!b->pending_action_invalidation()&&
        !b->pending_callbacks()&&!b->pending_transactions()&&!b->pending_aux_scopes()&&!b->aux_debt()&&
        b->registration_current(p.cookie,p.native_started)&&b->lifecycle_revision()==p.revision&&b->action_generation()==p.generation;
}
bool parent_snapshot(input::Input const& i,std::uint64_t compose,Parent& out)noexcept {
    auto* b=weapon_hand_runtime::current();if(!enabled||!b||!i.parent_cookie)return false;
    Parent p{};p.cookie=i.parent_cookie;p.native_started=i.parent_scene.native.native_started_qpc;p.tick=i.tick;p.compose=compose;
    p.revision=b->lifecycle_revision();p.generation=b->action_generation();if(!body_tick_input_live::parent_current(p))return false;out=p;return true;
}
bool configure(wchar_t const* ini,FILE* f,bool consumer){const auto request=GetPrivateProfileIntW(L"BodyTickInput",L"enabled",0,ini);
    if(request!=0&&request!=1)return false;enabled=request==1;
    if(enabled&&(!consumer||!rig::enabled||!weapon_hand_runtime::enabled||body_tick_tracking_probe_live::enabled||
       GetPrivateProfileIntW(L"BodyC0GoalPolicy",L"enabled",0,ini)!=1))return false;
    if(f)fprintf(f,"body_tick_input_compiled=1 body_tick_input_enabled=%u legacy_Scene_unchanged=1 body40_scene50=1\n",unsigned(enabled));return true;
}
void summary(FILE* f)noexcept{if(f)fprintf(f,"body_tick_input_enabled=%u body_tick_input_attempts=%llu body_tick_input_acquired=%llu body_tick_input_unavailable=%llu body_tick_input_age_rejected=%llu body_tick_input_hard_rejected=%llu body_tick_input_ends=%llu ",unsigned(enabled),attempts.load(),acquired.load(),unavailable.load(),age_rejected.load(),hard_rejected.load(),ends.load());}
}
#endif
