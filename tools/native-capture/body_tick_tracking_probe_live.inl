#if defined(GEARS_BODY_TICK_TRACKING_PROBE) && GEARS_BODY_TICK_TRACKING_PROBE
namespace body_tick_tracking_probe_live {
namespace p=::gears_body_tick_probe;namespace rig=rig_tracking_live;namespace api=gears_rig_tracking;
bool enabled{};std::atomic<unsigned> count{};std::atomic<bool> attempted{},written{};std::atomic<unsigned> error{};
std::array<p::Record,32> records{};p::Record working{},last{};bool have_last{};
inline thread_local unsigned selected_index{32};inline thread_local std::uint64_t selected_tick{};
std::uint64_t last_attempt_tick{},last_attempt_native{};
bool fresh(std::int64_t stamp,std::int64_t now)noexcept{return stamp>0&&now>=stamp&&std::uint64_t(now-stamp)<=40000000;}
bool ref_equal(api::NativeReference const&a,api::NativeReference const&b)noexcept{return a.epoch==b.epoch&&a.flags==b.flags&&!a.reserved&&!b.reserved;}
p::Failure acquire(p::Record& out) {
    using F=p::Failure;auto const& old=out.original_scene;auto const now=rig::clock_ns();
    if(!gears_native_context::engine_context_clear(gears_native_context::thread_ledger,true))return F::context;
    std::unique_lock<decltype(stereo_mutex)> lock(stereo_mutex,std::try_to_lock);if(!lock.owns_lock())return F::lock;
    if(!rig::reference_snapshot(out.reference,out.reference_revision)||out.reference_revision!=old.native_reference_revision||
       !ref_equal(out.reference,old.native_reference)||out.reference.flags!=api::reference_stable)return F::reference;
    if(!stereo_bridge.read(out.sampled)||!gears_vr::stereo_render_active(out.sampled)||out.sampled.focused!=1||
       !gears_vr::stereo_tracking_valid(out.sampled,simulator_operator)||!gears_vr::stereo_geometry_valid(out.sampled))return F::stereo;
    if(!cover_input_live::bridge_open||!cover_input_live::controllers.read_matching(out.sampled,now,out.hands,simulator_operator))return F::hands;
    if(!cover_input_live::controllers.publisher_identity(out.publisher_pid,out.publisher_start)||
       out.publisher_pid!=old.controller_publisher_pid||out.publisher_start!=old.controller_publisher_start)return F::publisher;
    gears_hand_continuity::Publisher publisher{};
    if(!(old.flags&api::producer_continuity_available)||!cover_shared_input_live::continuity_open||
       !cover_shared_input_live::continuity_bridge.read_matching(out.sampled,now,out.continuity,publisher,simulator_operator)||
       publisher.pid!=out.publisher_pid||publisher.start!=out.publisher_start||!gears_hand_continuity::same_input(out.continuity.input,out.hands))return F::continuity;
    auto const& a=out.sampled;auto const& b=old.stereo;
    if(a.source!=b.source||a.session!=b.session||a.space_epoch!=b.space_epoch||a.sequence<b.sequence||
       out.hands.profile_generation!=old.controllers.profile_generation||out.hands.input_epoch!=old.controllers.input_epoch||
       out.continuity.stream_epoch!=old.producer.stream||out.continuity.tracking_epoch[0]!=old.producer.tracking[0]||
       out.continuity.tracking_epoch[1]!=old.producer.tracking[1])return F::source;
    if(a.received_ns<b.received_ns||a.predicted_time<b.predicted_time||out.hands.received_ns<old.controllers.received_ns||
       out.hands.sync_ns<old.controllers.sync_ns)return F::replay;
    if(a.sequence==b.sequence&&(std::memcmp(&a,&b,sizeof a)||!gears_hand_continuity::same_input(out.hands,old.controllers)))return F::replay;
    api::NativeReference again{};std::uint64_t revision{};
    if(!rig::reference_snapshot(again,revision)||revision!=out.reference_revision||!ref_equal(again,out.reference))return F::reference;
    const auto end=rig::clock_ns();
    if(!fresh(out.sampled.received_ns,end)||!fresh(out.hands.received_ns,end)||!fresh(out.hands.sync_ns,end)||!fresh(out.begin_ns,end))return F::age;
    if(have_last&&last.original_scene.native.native_started_qpc==old.native.native_started_qpc&&
       last.original_scene.world_generation==old.world_generation&&last.reference_revision==out.reference_revision&&
       last.sampled.session==a.session&&last.sampled.space_epoch==a.space_epoch&&last.sampled.source==a.source){
        if(a.sequence<last.sampled.sequence||a.received_ns<last.sampled.received_ns||a.predicted_time<last.sampled.predicted_time||
           out.hands.received_ns<last.hands.received_ns||out.hands.sync_ns<last.hands.sync_ns)return F::replay;
        if(a.sequence==last.sampled.sequence&&(std::memcmp(&a,&last.sampled,sizeof a)||!gears_hand_continuity::same_input(out.hands,last.hands)))return F::replay;
    }
    out.newer=unsigned(a.sequence>b.sequence);return F::okay;
}
void sample(std::uint64_t tick,std::uint64_t thread,gears_native_rig_tracking::CoreSnapshot const& original)noexcept {
    if(!enabled||!tick||GetCurrentThreadId()!=thread||thread!=original.native.game_thread)return;
    if(last_attempt_tick==tick&&last_attempt_native==original.native.native_started_qpc)return;
    selected_index=32;selected_tick=0;
    auto* broker=weapon_hand_runtime::current();const auto cookie=broker?broker->cookie():0;if(!cookie)return;
    const auto index=count.load();if(index>=records.size())return;
    last_attempt_tick=tick;last_attempt_native=original.native.native_started_qpc;
    working={};working.tick=tick;working.thread=thread;working.cookie=cookie;working.original_scene=original;working.begin_ns=rig::clock_ns();
    try{working.failure=acquire(working);}catch(...){working.failure=p::Failure::exception;}
    working.end_ns=rig::clock_ns();working.valid=unsigned(working.failure==p::Failure::okay);
    if(working.valid){last=working;have_last=true;}
    records[index]=working;selected_index=index;selected_tick=tick;count.store(index+1,std::memory_order_release);
}
void end_tick(std::uint64_t tick)noexcept{if(selected_tick==tick){selected_tick=0;selected_index=32;}}
void note_basis(std::uint64_t tick,std::uint64_t compose,gears_full_body_c0::Matrix64 const& component,
    gears_vr::StereoMatrix const& view,gears_full_body_c0::rig::Signature const& geometry)noexcept {
    if(!enabled||selected_index>=records.size()||selected_tick!=tick)return;auto& r=records[selected_index];
    if(r.tick!=tick||r.thread!=GetCurrentThreadId()||r.basis_available)return;
    r.component=component;r.base_view=view;r.geometry=geometry;r.compose=compose;r.basis_observed_ns=rig::clock_ns();r.basis_available=1;
}
bool configure(wchar_t const* ini,FILE* f){const auto request=GetPrivateProfileIntW(L"BodyTickTrackingProbe",L"enabled",0,ini);
    if(request!=0&&request!=1)return false;enabled=request==1;
    if(enabled&&(!rig::enabled||!weapon_hand_runtime::enabled))return false;
    if(f)fprintf(f,"body_tick_probe_compiled=1 body_tick_probe_enabled=%u read_only=1 original_Scene_and_body_consumer_unchanged=1\n",unsigned(enabled));return true;
}
void summary(FILE* f,bool drained)noexcept {
    if(enabled&&drained&&!attempted.exchange(true)){
        wchar_t path[MAX_PATH]{};if(!directory[0]||swprintf_s(path,L"%s\\body-tick-probe.bin",directory)<=0)error=ERROR_INVALID_NAME;
        else {HANDLE file=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
            if(file==INVALID_HANDLE_VALUE)error=GetLastError();else{
                p::Header h{};h.count=count.load(std::memory_order_acquire);h.pid=GetCurrentProcessId();h.native_started=std::uint64_t(session_started_qpc);
                auto write=[&](void const* data,DWORD size){DWORD bytes{};const bool ok=WriteFile(file,data,size,&bytes,nullptr)&&bytes==size;if(!ok)error=GetLastError();return ok;};
                bool ok=h.count<=32&&write(&h,sizeof h);if(ok&&h.count)ok=write(records.data(),DWORD(h.count*sizeof(p::Record)));
                if(!CloseHandle(file)){error=GetLastError();ok=false;}written=ok;
            }}
    }
    if(f)fprintf(f,"body_tick_probe_enabled=%u body_tick_probe_records=%u body_tick_probe_written=%u body_tick_probe_error=%u ",unsigned(enabled),count.load(),unsigned(written.load()),error.load());
}
}
#endif
