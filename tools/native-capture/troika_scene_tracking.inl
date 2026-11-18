// Include after the original shared selector AND rig_tracking_live. The only
// input selection below uses cover_input_live::matching_input, exactly once
// per attempted mounted Scene. No input bridge, arbiter or pose writer added.
namespace troika_scene_tracking {
namespace api=gears_troika_source;namespace hand=gears_weapon_hand;
namespace mounted=troika_scene_binding;namespace rig=rig_tracking_live;
std::atomic<bool> stopped{};
#if defined(GEARS_TROIKA_MOUNTED_SOURCE) && GEARS_TROIKA_MOUNTED_SOURCE
std::mutex mutex;
api::Frame value{};gears_anchor::Receipt receipt{};
std::atomic<uint64_t> revision{1};uint64_t publication{},attempted_scene{};bool active{};
gears_weapon::TroikaMountedSpace entry_space;
gears_troika_basis::Request retained_basis{};
gears_weapon::TroikaMountedMapping mapped_value{};
bool basis_retained{},mapping_ready{};uint64_t basis_cookie{},last_basis_token{};
gears_troika_geometry::Response geometry_value{};bool geometry_ready{};uint64_t control_query_publication{};
bool geometry_current(gears_troika_geometry::Source const&) noexcept;
struct Selection {
    gears_vr::StereoResult stereo{};gears_vr::ControllerInputFrame hands{};
    gears_hand_continuity::TrackedInput continuity{};
    uint64_t publisher_pid{},publisher_start{};bool matched{},available{},continuity_known{};
};
bool bump(){const auto previous=revision.load();if(previous==UINT64_MAX){stopped=true;active=false;value={};return false;}
    revision.store(previous+1);return true;}
void clear() noexcept {try{std::lock_guard<std::mutex> lock(mutex);mapping_ready=false;entry_space.interrupt();
    geometry_ready=false;geometry_value={};
    if(active){active=false;value={};bump();}}catch(...){stopped=true;}}
hand::ObjectIdentity object(gears_anchor::Identity const& id){return rig::object(id);}
bool control_mapping_ready() noexcept {try{std::lock_guard<std::mutex> lock(mutex);return !stopped.load()&&active&&basis_retained&&mapping_ready;}catch(...){return false;}}
bool prepare(gears_anchor::Receipt const* scene,gears_vr::StereoResult const& stereo) noexcept {
    try{
        if(stopped.load()||!rig::enabled||!scene||!stereo||scene->thread!=GetCurrentThreadId()||!scene->invocation){clear();return false;}
        auto* broker=weapon_hand_runtime::current();const auto session=weapon_hand_runtime::session();
        const auto cookie=broker?broker->cookie():0;
        if(!rig::session_current(broker,session,cookie)){clear();return false;}
        mounted::Binding first{},closing{};
        if(!mounted::sample(scene,first)){clear();return false;}
        if(!vr_gamepad_live::arm_mounted_look()){clear();return false;}
        auto stamp=rig::clock_ns();
        gears_rig_tracking::NativeReference observed_reference{};uint64_t observed_revision{};
        if(!rig::reference_snapshot(observed_reference,observed_revision)){clear();return false;}
        {
            std::lock_guard<std::mutex> lock(mutex);
            if(scene->invocation<=attempted_scene){
                // Repeated views only reuse the original raw sample; they do
                // not reselect, republish, rearm or refresh any timestamp.
                return active&&value.native.registration_cookie==cookie&&value.native.game_start==session.game_start&&
                    value.native.native_started_qpc==session.native_started_qpc&&value.native_reference_revision==observed_revision&&
                    value.native_reference.epoch==observed_reference.epoch&&scene->invocation==value.scene_invocation&&api::frame_valid(value,stamp,gears_rig_tracking::maximum_age_ns,simulator_operator)&&
                    gears_vr::stereo_same_pose(value.reference,stereo.reference)&&value.reference_generation==stereo.reference_generation&&
                    !std::memcmp(&value.stereo,&stereo.sample,sizeof value.stereo)&&!std::memcmp(&receipt,scene,sizeof receipt);
            }
            attempted_scene=scene->invocation;active=false;value={};mapping_ready=false;if(!bump())return false;
        }
        api::Frame next{};next.native={session.game_pid,scene->thread,session.game_start,session.native_started_qpc,cookie};
        next.world=object(first.world);next.controller=object(first.controller);next.turret=object(first.turret);next.driver=object(first.driver);
        next.turret_mesh=object(first.turret_mesh);next.driver_mesh=object(first.driver_mesh);next.weapon=object(first.weapon);
        next.world_generation=first.world_generation;next.scene_invocation=scene->invocation;next.scene_qpc=scene->scene_qpc;next.qpc_frequency=qpc_frequency;
        next.reference=stereo.reference;next.reference_generation=stereo.reference_generation;next.stereo=stereo.sample;
        if(!rig::reference_snapshot(next.native_reference,next.native_reference_revision)){clear();return false;}
        Selection selected{};std::memcpy(&selected.stereo,&stereo,sizeof stereo);
        selected.matched=cover_input_live::matching_input(stereo,stamp,selected.hands);
        selected.available=selected.matched&&cover_input_live::controllers.publisher_identity(selected.publisher_pid,selected.publisher_start)&&
            selected.publisher_pid<=UINT32_MAX;
        cover_shared_input_live::select_continuity(selected,stamp);
        if(!selected.available||!selected.continuity_known){clear();return false;}
        next.controllers=selected.hands;next.publisher_pid=uint32_t(selected.publisher_pid);next.publisher_start=selected.publisher_start;
        next.control_policy=api::mounted_look_suppressed;
        next.producer={selected.continuity.stream_epoch,{selected.continuity.tracking_epoch[0],selected.continuity.tracking_epoch[1]}};
        if(!mounted::sample(scene,closing)||!mounted::same(first,closing)||!rig::session_current(broker,session,cookie)||!vr_gamepad_live::mounted_look_ready()){clear();return false;}
        gears_rig_tracking::NativeReference reference{};uint64_t reference_revision{};
        if(!rig::reference_snapshot(reference,reference_revision)||reference_revision!=next.native_reference_revision||
           reference.epoch!=next.native_reference.epoch){clear();return false;}
        std::lock_guard<std::mutex> lock(mutex);
        if(stopped.load()||!rig::session_current(broker,session,cookie)||publication==UINT64_MAX)return false;
        next.publication_sequence=publication+1;next.source_ticket=revision.load();next.published_ns=rig::clock_ns();
        if(!api::frame_valid(next,next.published_ns,gears_rig_tracking::maximum_age_ns,simulator_operator))return false;
        value=next;std::memcpy(&receipt,scene,sizeof receipt);publication=next.publication_sequence;active=true;
        mapping_ready=basis_retained&&entry_space.map(retained_basis.owner,next,next.published_ns,mapped_value);
        geometry_ready=false;geometry_value={};
        return true;
    }catch(...){clear();return false;}
}
// Called after the existing stereo lock has ended, on the admitted Scene GT.
void query_geometry(gears_anchor::Receipt const* scene) noexcept {
    weapon_hand_runtime::detail::Call call;
    gears_troika_geometry::Source source{};
    try{
        {std::lock_guard<std::mutex> lock(mutex);
            if(stopped.load()||!active||!mapping_ready||!basis_retained||!scene||
               std::memcmp(scene,&receipt,sizeof receipt)||scene->thread!=GetCurrentThreadId())return;
            if(value.publication_sequence<=control_query_publication)return;
            control_query_publication=value.publication_sequence; // Failed/duplicate views never arbitrate again.
            source.frame=value;source.entry_binding=retained_basis.owner.binding;source.entry_token=retained_basis.owner.token;}
        gears_troika_geometry::Response proposal{};
        gears_troika_control::Decision decision{};
        const auto queried=weapon_hand_runtime::transact_troika_control(source,decision,proposal);
        const bool current=queried.code==decltype(queried)::Code::ready&&geometry_current(source);
        if(!current){std::lock_guard<decltype(stereo_mutex)> lock(stereo_mutex);troika_control_live::interrupt_locked();}
        std::lock_guard<std::mutex> lock(mutex);
        if(current&&active&&value.publication_sequence==source.frame.publication_sequence&&value.source_ticket==source.frame.source_ticket){
            geometry_value=proposal;geometry_ready=true;}
    }catch(...){clear();}
}
gears_rig_tracking::Reply retain_basis(gears_troika_basis::Request const* input) noexcept {
    weapon_hand_runtime::detail::Call call;gears_troika_basis::Request query{};
    if(!input||!hand::weapon_broker_win32::copy(&query,input,sizeof query)||!gears_troika_basis::valid(query))return gears_rig_tracking::Reply::invalid;
    if(stopped.load())return gears_rig_tracking::Reply::stopping;
    if(!rig::enabled||GetCurrentThreadId()!=query.owner.native.game_thread)return gears_rig_tracking::Reply::unavailable;
    // Freeze the exact authentic Native cache, never a PG-provided matrix.
    gears_rig_tracking::ReadRequest request{};request.registration_cookie=query.owner.native.registration_cookie;
    request.native_started_qpc=query.owner.native.native_started_qpc;request.minimum_publication_sequence=query.entry_publication;
    request.maximum_source_age_ns=100000000;gears_rig_tracking::Frame original{};
    const auto reply=rig::read(&request,&original);if(reply!=gears_rig_tracking::Reply::accepted)return reply;
    if(original.publication_sequence!=query.entry_publication||!gears_rig_tracking::same(original.native,query.owner.native))return gears_rig_tracking::Reply::unavailable;
    gears_weapon::TroikaMountedSpace prepared;if(!prepared.capture(query.owner,original,rig::clock_ns()))return gears_rig_tracking::Reply::unavailable;
    auto* broker=weapon_hand_runtime::current();const auto session=weapon_hand_runtime::session();
    try{std::lock_guard<std::mutex> lock(mutex);
        if(stopped.load()||!rig::session_current(broker,session,query.owner.native.registration_cookie))return gears_rig_tracking::Reply::stopping;
        if(basis_cookie!=query.owner.native.registration_cookie){basis_retained=false;mapping_ready=false;entry_space.reset();
            basis_cookie=query.owner.native.registration_cookie;last_basis_token=0;}
        if(basis_retained)return gears_troika_basis::same(retained_basis,query)?gears_rig_tracking::Reply::accepted:gears_rig_tracking::Reply::unavailable;
        if(query.owner.token<=last_basis_token)return gears_rig_tracking::Reply::unavailable;
        retained_basis=query;entry_space=prepared;basis_retained=true;mapping_ready=false;last_basis_token=query.owner.token;
        return gears_rig_tracking::Reply::accepted;
    }catch(...){return gears_rig_tracking::Reply::unavailable;}
}
gears_rig_tracking::Reply cancel_basis(gears_troika_basis::Request const* input) noexcept {
    weapon_hand_runtime::detail::Call call;gears_troika_basis::Request query{};
    if(!input||!hand::weapon_broker_win32::copy(&query,input,sizeof query)||!gears_troika_basis::valid(query))return gears_rig_tracking::Reply::invalid;
    if(stopped.load())return gears_rig_tracking::Reply::stopping;
    if(GetCurrentThreadId()!=query.owner.native.game_thread)return gears_rig_tracking::Reply::unavailable;
    auto* broker=weapon_hand_runtime::current();const auto session=weapon_hand_runtime::session();
    if(!rig::session_current(broker,session,query.owner.native.registration_cookie))return gears_rig_tracking::Reply::wrong_cookie;
    try{std::lock_guard<std::mutex> lock(mutex);
        if(!basis_retained||!gears_troika_basis::same(retained_basis,query))return gears_rig_tracking::Reply::unavailable;
        basis_retained=false;mapping_ready=false;entry_space.reset();retained_basis={};return gears_rig_tracking::Reply::accepted;
    }catch(...){return gears_rig_tracking::Reply::unavailable;}
}
// Internal data-only consumption by the existing Native view/C0 integration.
// The consumer must authenticate its mounted Driver/Scene separately.
bool mapped_for(api::Frame const& expected,gears_weapon::TroikaMountedMapping& output) noexcept {
    output={};try{std::lock_guard<std::mutex> lock(mutex);
        auto* broker=weapon_hand_runtime::current();const auto session=weapon_hand_runtime::session();
        gears_rig_tracking::NativeReference reference{};uint64_t reference_revision{};
        if(stopped.load()||!active||!mapping_ready||!vr_gamepad_live::mounted_look_ready()||expected.publication_sequence!=value.publication_sequence||
           expected.source_ticket!=value.source_ticket||revision.load()!=value.source_ticket||
           !gears_rig_tracking::same(expected.native,value.native)||expected.world_generation!=value.world_generation||
           expected.scene_invocation!=value.scene_invocation||!hand::same(expected.world,value.world)||
           !hand::same(expected.controller,value.controller)||!hand::same(expected.turret,value.turret)||!hand::same(expected.driver,value.driver)||
           !hand::same(expected.driver_mesh,value.driver_mesh)||!hand::same(expected.turret_mesh,value.turret_mesh)||!hand::same(expected.weapon,value.weapon)||
           expected.reference_generation!=value.reference_generation||expected.stereo.session!=value.stereo.session||expected.stereo.space_epoch!=value.stereo.space_epoch||
           expected.controllers.sequence!=value.controllers.sequence||expected.controllers.sync_ns!=value.controllers.sync_ns||
           expected.controllers.input_epoch!=value.controllers.input_epoch||expected.controllers.profile_generation!=value.controllers.profile_generation||
           !rig::session_current(broker,session,value.native.registration_cookie)||
           !rig::reference_snapshot(reference,reference_revision)||reference_revision!=value.native_reference_revision||reference.epoch!=value.native_reference.epoch||
           !api::frame_valid(value,rig::clock_ns(),100000000,simulator_operator))return false;
        output=mapped_value;return true;
    }catch(...){return false;}
}
// Native-fixed live validator. Runs outside the broker mutex with its pin
// reservation retained. PG cannot replace this function or supply a reader.
bool geometry_current(gears_troika_geometry::Source const& expected) noexcept {
    gears_weapon::TroikaMountedMapping mapping{};
    if(!mapped_for(expected.frame,mapping))return false;
    gears_anchor::Receipt scene{};
    try{std::lock_guard<std::mutex> lock(mutex);
        if(!basis_retained||expected.entry_binding!=retained_basis.owner.binding||expected.entry_token!=retained_basis.owner.token||
           std::memcmp(&expected.frame,&value,sizeof value))return false;
        scene=receipt;
    }catch(...){return false;}
    mounted::Binding observed{};
    if(!mounted::sample(&scene,observed))return false;
    return expected.frame.world_generation==observed.world_generation&&
        hand::same(expected.frame.world,object(observed.world))&&hand::same(expected.frame.controller,object(observed.controller))&&
        hand::same(expected.frame.turret,object(observed.turret))&&hand::same(expected.frame.driver,object(observed.driver))&&
        hand::same(expected.frame.turret_mesh,object(observed.turret_mesh))&&hand::same(expected.frame.driver_mesh,object(observed.driver_mesh))&&
        hand::same(expected.frame.weapon,object(observed.weapon))&&mapped_for(expected.frame,mapping);
}
gears_rig_tracking::Reply read(api::Request const* request,api::Frame* output) noexcept {
    weapon_hand_runtime::detail::Call call;
    api::Frame empty{};
    if(!output||!hand::weapon_broker_win32::copy(output,&empty,sizeof empty))return gears_rig_tracking::Reply::invalid;
    try{
        api::Request query{};if(!request||!hand::weapon_broker_win32::copy(&query,request,sizeof query)||!api::request_valid(query))return gears_rig_tracking::Reply::invalid;
        if(stopped.load())return gears_rig_tracking::Reply::stopping;
        if(GetCurrentThreadId()!=query.native.game_thread)return gears_rig_tracking::Reply::unavailable;
        auto* broker=weapon_hand_runtime::current();const auto session=weapon_hand_runtime::session();
        if(!rig::session_current(broker,session,query.native.registration_cookie)||query.native.game_pid!=session.game_pid||
           query.native.game_start!=session.game_start||query.native.native_started_qpc!=session.native_started_qpc)return gears_rig_tracking::Reply::wrong_cookie;
        api::Frame copy{};uint64_t ticket{};
        {std::lock_guard<std::mutex> lock(mutex);if(!active)return gears_rig_tracking::Reply::unavailable;
            copy=value;ticket=revision.load();}
        gears_rig_tracking::NativeReference reference{};uint64_t reference_revision{};
        const auto valid=[&](){return !stopped.load()&&vr_gamepad_live::mounted_look_ready()&&ticket==revision.load()&&ticket==copy.source_ticket&&
            rig::session_current(broker,session,query.native.registration_cookie)&&
            rig::reference_snapshot(reference,reference_revision)&&reference_revision==copy.native_reference_revision&&reference.epoch==copy.native_reference.epoch&&
            api::matches(query,copy,rig::clock_ns(),simulator_operator);};
        // Data-only read: no Scene/actor/controller memory read and no stereo
        // lock or module query here. PG must authenticate its current original
        // mounted owner independently before and after using this raw data.
        if(!valid())return gears_rig_tracking::Reply::unavailable;
        if(!hand::weapon_broker_win32::copy(output,&copy,sizeof copy))return gears_rig_tracking::Reply::invalid;
        if(!valid()){hand::weapon_broker_win32::copy(output,&empty,sizeof empty);return gears_rig_tracking::Reply::unavailable;}
        return gears_rig_tracking::Reply::accepted;
    }catch(...){hand::weapon_broker_win32::copy(output,&empty,sizeof empty);return gears_rig_tracking::Reply::unavailable;}
}
#else
void clear() noexcept {}
bool prepare(gears_anchor::Receipt const*,gears_vr::StereoResult const&) noexcept {return false;}
bool control_mapping_ready() noexcept {return false;}
gears_rig_tracking::Reply read(api::Request const*,api::Frame* output) noexcept {
    api::Frame empty{};return hand::weapon_broker_win32::copy(output,&empty,sizeof empty)?
        gears_rig_tracking::Reply::unavailable:gears_rig_tracking::Reply::invalid;
}
gears_rig_tracking::Reply retain_basis(gears_troika_basis::Request const*) noexcept {return gears_rig_tracking::Reply::unavailable;}
gears_rig_tracking::Reply cancel_basis(gears_troika_basis::Request const*) noexcept {return gears_rig_tracking::Reply::unavailable;}
bool mapped_for(api::Frame const&,gears_weapon::TroikaMountedMapping& output) noexcept {output={};return false;}
bool geometry_current(gears_troika_geometry::Source const&) noexcept {return false;}
void query_geometry(gears_anchor::Receipt const*) noexcept {}
#endif
void stop() noexcept {stopped.store(true);clear();
#if defined(GEARS_TROIKA_MOUNTED_SOURCE) && GEARS_TROIKA_MOUNTED_SOURCE
    try{std::lock_guard<std::mutex> lock(mutex);basis_retained=false;mapping_ready=false;entry_space.reset();retained_basis={};}catch(...){}
#endif
}
} // namespace troika_scene_tracking
