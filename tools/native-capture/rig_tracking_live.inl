// Global include native_rig_tracking_frame.h first; include after Runtime/Shared.
// publish owns stereo_mutex/admitted ordinary Scene GT. read/stop never take
// stereo_mutex, query modules, invoke PG or read controller/game memory.
namespace rig_tracking_live {
namespace api=gears_rig_tracking;namespace rt=weapon_hand_runtime;namespace shared=cover_shared_input_live;
namespace binding=weapon_hand_scene_binding;namespace hand=gears_weapon_hand;
namespace core=::gears_native_rig_tracking;
bool enabled{};std::atomic<bool> stopped{};std::mutex mutex;
binding::Binding scene_owner_binding{};
core::CoreSnapshot value{},previous_payload{};api::ObservedContinuity observed{};
uint64_t value_version{1}; // Cache invalidation only; not a source/Scene epoch.
uint64_t sequence{},binding_generation{},source_ticket{};bool have_previous{},loss_seen{};
std::atomic<uint64_t> reference_revision{},reference_epoch{};std::atomic<unsigned> reference_flags{};std::atomic<bool> reference_failed{};
uint64_t value_reference_revision{};
void reference_changed(gears_vr::ReferenceContinuity snapshot) noexcept {
    const unsigned flags=(snapshot.stable?api::reference_stable:0u)|(snapshot.pending?api::reference_pending:0u)|(snapshot.exhausted?api::reference_exhausted:0u);
    if(!snapshot.epoch||(snapshot.stable&&(snapshot.pending||snapshot.exhausted))||snapshot.epoch<reference_epoch.load()){reference_failed=true;return;}
    if(snapshot.exhausted)reference_failed=true;
    if(reference_epoch.load()==snapshot.epoch&&reference_flags.load()==flags)return;
    auto revision=reference_revision.load();if((revision&1)||revision>UINT64_MAX-2||!reference_revision.compare_exchange_strong(revision,revision+1)){reference_failed=true;return;}
    reference_epoch=snapshot.epoch;reference_flags=flags;reference_revision=revision+2;
}
void reference_observer(void*,gears_vr::ReferenceContinuity snapshot) noexcept {reference_changed(snapshot);}
bool reference_snapshot(api::NativeReference& out,uint64_t& revision){
    revision=reference_revision.load();if(!revision||(revision&1)||reference_failed.load())return false;
    out={reference_epoch.load(),reference_flags.load(),0};return revision==reference_revision.load()&&out.epoch&&out.flags==api::reference_stable&&!reference_failed.load();
}
bool reference_matches(gears_vr::ReferenceContinuity const& frozen,uint64_t revision){api::NativeReference current{};uint64_t stamp{};
    return frozen.epoch&&frozen.stable&&!frozen.pending&&!frozen.exhausted&&reference_snapshot(current,stamp)&&stamp==revision&&current.epoch==frozen.epoch;}
unsigned long long attempts{},published{},rejected{},duplicates{},reads{},read_rejected{};
unsigned long long core_reads{},core_read_rejected{};
int64_t (*clock_ns)()=gears_vr::monotonic_ns;
bool changed(){if(value_version==UINT64_MAX){stopped=true;value={};scene_owner_binding={};return false;}++value_version;return true;}
bool increment(uint64_t& v){if(v==UINT64_MAX){stopped=true;value={};scene_owner_binding={};changed();return false;}++v;return true;}
bool bump(api::ObservedContinuity& e){return increment(e.stream)&&increment(e.reference)&&increment(e.head_tracking)&&increment(e.hand_tracking[0])&&increment(e.hand_tracking[1]);}
bool all_epochs(){return bump(observed);}
bool fail(){std::lock_guard<std::mutex> lock(mutex);++rejected;value={};scene_owner_binding={};changed();
    if(have_previous&&!loss_seen){loss_seen=true;all_epochs();}return false;}
hand::ObjectIdentity object(gears_anchor::Identity const& id){return id.address?hand::ObjectIdentity{id.address,id.outer,id.cls,id.index,id.name_index,id.number,1}:hand::ObjectIdentity{};}
bool actor_same(binding::Binding const& a,binding::Binding const& b){return binding::same_key(a,b)&&a.special_move==b.special_move&&a.scene_invocation==b.scene_invocation;}
bool source_fresh(int64_t stamp,int64_t now,uint64_t age){return stamp>0&&now>=stamp&&uint64_t(now-stamp)<=age;}
bool scene_fresh(int64_t ticks,int64_t frequency,int64_t now,uint64_t age){
    if(ticks<=0||frequency<=0)return false;const auto t=uint64_t(ticks),f=uint64_t(frequency),seconds=t/f,remainder=t%f;
    if(seconds>uint64_t(INT64_MAX)/1000000000||remainder>uint64_t(INT64_MAX)/1000000000)return false;
    const auto ns=seconds*1000000000+remainder*1000000000/f;return ns<=uint64_t(INT64_MAX)&&source_fresh(int64_t(ns),now,age);
}
bool native_session_current(hand::Broker* broker,hand::BrokerSession const& session){
    const auto current=rt::session();return rt::enabled&&broker&&rt::current()==broker&&
        !rt::detail::admission_closed.load()&&!rt::detail::stop_requested.load()&&!rt::detail::unknown_fault.load()&&
        current.game_pid==session.game_pid&&current.game_start==session.game_start&&current.native_started_qpc==session.native_started_qpc;
}
bool session_current(hand::Broker* broker,hand::BrokerSession const& session,uint64_t registration_cookie){
    return registration_cookie&&native_session_current(broker,session)&&broker->registration_current(registration_cookie,session.native_started_qpc)&&!broker->draining();
}
bool binding_key(core::CoreSnapshot const& a,core::CoreSnapshot const& b){return core::same(a.native,b.native)&&a.world_generation==b.world_generation&&
    hand::same(a.world,b.world)&&hand::same(a.controller,b.controller)&&hand::same(a.pawn,b.pawn)&&hand::same(a.mesh,b.mesh)&&
    a.view_policy==b.view_policy&&a.reference_generation==b.reference_generation&&gears_vr::stereo_same_pose(a.reference,b.reference)&&
    a.units_per_metre==b.units_per_metre&&a.native_reference.epoch==b.native_reference.epoch&&((a.flags^b.flags)&api::cover_key_known)==0&&
    hand::same(a.cover_link,b.cover_link)&&a.cover_slot==b.cover_slot&&a.cover_type==b.cover_type;}
bool stream_key(core::CoreSnapshot const& a,core::CoreSnapshot const& b){return core::same(a.native,b.native)&&a.stereo.source==b.stereo.source&&
    a.stereo.session==b.stereo.session&&a.stereo.space_epoch==b.stereo.space_epoch&&
    a.controller_publisher_pid==b.controller_publisher_pid&&a.controller_publisher_start==b.controller_publisher_start&&
    ((a.flags^b.flags)&(api::controllers_matched|api::producer_continuity_available))==0&&
    a.producer.stream==b.producer.stream&&a.producer.tracking[0]==b.producer.tracking[0]&&a.producer.tracking[1]==b.producer.tracking[1]&&
    a.controllers.profile_generation==b.controllers.profile_generation&&a.controllers.input_epoch==b.controllers.input_epoch;}
bool publish(gears_anchor::Receipt const* scene_receipt,gears_vr::StereoMatrix const& source_view,
             gears_vr::StereoMatrix const& base_view,gears_vr::StereoResult const& stereo_result,uint32_t view_policy,gears_vr::ReferenceContinuity const& frozen){
    if(!enabled)return false;try{
        {std::lock_guard<std::mutex> lock(mutex);++attempts;}
        const auto stamp=clock_ns();auto* broker=rt::current();const auto session=rt::session();
        shared::TrackingBundle selected{};binding::Binding first{},last{};api::NativeReference reference{};uint64_t reference_stamp{};
        if(stopped.load()||!scene_receipt||scene_receipt->thread!=GetCurrentThreadId()||!scene_receipt->invocation||!stereo_result||
           !native_session_current(broker,session)||!reference_snapshot(reference,reference_stamp)||!reference_matches(frozen,reference_stamp)||
           !gears_vr::stereo_render_active(stereo_result.sample)||stereo_result.sample.focused!=1||
           !gears_vr::stereo_tracking_valid(stereo_result.sample,simulator_operator)||!source_fresh(stereo_result.sample.received_ns,stamp,api::maximum_age_ns)||
           !scene_fresh(scene_receipt->scene_qpc,qpc_frequency,stamp,api::maximum_age_ns)||!shared::tracking_for(scene_receipt,stereo_result,selected)||
           !binding::sample(scene_receipt,first)||!actor_same(first,selected.actor))return fail();
        std::array<unsigned char,sizeof(gears_anchor::Receipt)> receipt{};memcpy(receipt.data(),scene_receipt,receipt.size());
        core::CoreSnapshot next{};next.status=api::Status::ready;next.native={session.game_pid,scene_receipt->thread,session.game_start,session.native_started_qpc};
        next.source_ticket=selected.ticket;next.native_reference_revision=reference_stamp;
        next.scene_invocation=scene_receipt->invocation;next.world_generation=first.world_generation;next.reference_generation=stereo_result.reference_generation;
        next.scene_qpc=scene_receipt->scene_qpc;next.qpc_frequency=qpc_frequency;next.view_policy=view_policy;next.units_per_metre=stereo_units_per_metre;next.native_reference=reference;
        next.world=object(first.world);next.controller=object(first.controller);next.pawn=object(first.pawn);next.mesh=object(first.mesh);next.weapon=object(first.weapon);
        next.source_view=source_view;next.base_view=base_view;next.reference=stereo_result.reference;next.stereo=stereo_result.sample;
        if(selected.matched){next.flags|=api::controllers_matched;next.controllers=selected.hands;
            next.controller_publisher_pid=uint32_t(selected.publisher_pid);next.controller_publisher_start=selected.publisher_start;}
        if(selected.continuity_known){next.flags|=api::producer_continuity_available;next.producer={selected.continuity.stream_epoch,{selected.continuity.tracking_epoch[0],selected.continuity.tracking_epoch[1]}};}
        if(selected.cover_known&&selected.eye.native_ct&&selected.eye.link.address&&selected.eye.slot>=0){
            next.flags|=api::cover_key_known;next.cover_link=object(selected.eye.link);next.cover_slot=selected.eye.slot;next.cover_type=selected.eye.native_ct;}
        if(!gears_vr::rig_tracking::pose(base_view,stereo_result.reference,stereo_result.sample.head,stereo_units_per_metre,next.world_head))return fail();
        next.world_pose_mask=api::head_pose;
        for(unsigned i=0;i<2;++i)if(selected.matched&&gears_vr::controller_pose_usable(selected.hands.hands[i].grip,selected.hands.hands[i].grip_flags,selected.hands.hands[i].grip_active)){
            if(!gears_vr::rig_tracking::pose(base_view,stereo_result.reference,selected.hands.hands[i].grip,stereo_units_per_metre,next.world_grip[i]))return fail();
            next.world_pose_mask|=api::left_grip_pose<<i;}
        if(!binding::sample(scene_receipt,last)||!actor_same(first,last)||memcmp(receipt.data(),scene_receipt,receipt.size())||!recording.load()||
           committed_world_generation.load()!=first.world_generation||scene_receipt->thread!=GetCurrentThreadId()||
           !native_session_current(broker,session)||!reference_matches(frozen,reference_stamp)||!shared::tracking_ticket_current(selected.ticket))return fail();
        std::lock_guard<std::mutex> lock(mutex);if(stopped.load()||!native_session_current(broker,session)||!reference_matches(frozen,reference_stamp))return false;
        if(have_previous&&next.scene_invocation==previous_payload.scene_invocation&&core::same(next.native,previous_payload.native)){
            if(value.status!=api::Status::ready||loss_seen||memcmp(&next,&previous_payload,sizeof next)||!shared::tracking_ticket_current(source_ticket)){
                ++rejected;value={};scene_owner_binding={};changed();if(!loss_seen){loss_seen=true;all_epochs();}return false;}
            ++duplicates;return true; // No publication, epoch or timestamp refresh.
        }
        auto epochs=observed;auto generation=binding_generation;auto publication=sequence;
        if(!have_previous){if(!bump(epochs)||!increment(generation))return false;}
        else{
            if(!binding_key(next,previous_payload)&&!increment(generation))return false;
            if(!stream_key(next,previous_payload)||reference_stamp!=value_reference_revision){if(!bump(epochs))return false;}
            else{
                if(next.reference_generation!=previous_payload.reference_generation||next.native_reference.epoch!=previous_payload.native_reference.epoch||!gears_vr::stereo_same_pose(next.reference,previous_payload.reference))if(!increment(epochs.reference))return false;
                for(unsigned i=0;i<2;++i)if(((next.world_pose_mask^previous_payload.world_pose_mask)&(api::left_grip_pose<<i))&&!increment(epochs.hand_tracking[i]))return false;
            }
        }
        if(!increment(publication))return false;const auto payload=next;
        next.publication_sequence=publication;next.binding_generation=generation;next.observed=epochs;next.published_ns=uint64_t(clock_ns());
        if(!core::core_snapshot_valid(next,clock_ns(),api::maximum_age_ns,simulator_operator)||
           (selected.matched&&!source_fresh(next.controllers.received_ns,clock_ns(),api::maximum_age_ns))||!reference_matches(frozen,reference_stamp)||!shared::tracking_ticket_current(selected.ticket)){
            ++rejected;value={};scene_owner_binding={};changed();if(have_previous&&!loss_seen){loss_seen=true;all_epochs();}return false;}
        sequence=publication;binding_generation=generation;observed=epochs;previous_payload=payload;have_previous=true;loss_seen=false;source_ticket=selected.ticket;value_reference_revision=reference_stamp;
        value=next;if(!changed())return false;if(!reference_matches(frozen,reference_stamp)){value={};scene_owner_binding={};changed();++rejected;return false;}scene_owner_binding=last;++published;return true;
    }catch(...){return fail();}
}
// Internal C++ data request, not a public wire type or owner authentication.
// The Native caller supplies its separately checked, current body/Scene cohort.
struct CoreReadRequest {
    core::NativeSessionKey native{};
    hand::ObjectIdentity world{},pawn{},mesh{};
    uint64_t world_generation{},scene_invocation{};
    int64_t scene_qpc{};
    uint64_t minimum_publication_sequence{},maximum_source_age_ns{50000000};
};
bool request_shape(CoreReadRequest const& r){return r.native.game_pid&&r.native.game_thread&&r.native.game_start&&r.native.native_started_qpc&&
    hand::identity_valid(r.world)&&hand::identity_valid(r.pawn)&&hand::identity_valid(r.mesh)&&r.world_generation&&r.scene_invocation&&r.scene_qpc>0&&
    r.maximum_source_age_ns&&r.maximum_source_age_ns<=api::maximum_age_ns;}
bool owner_matches(CoreReadRequest const& r,core::CoreSnapshot const& f){return core::same(r.native,f.native)&&
    hand::same(r.world,f.world)&&hand::same(r.pawn,f.pawn)&&hand::same(r.mesh,f.mesh)&&r.world_generation==f.world_generation&&
    r.scene_invocation==f.scene_invocation&&r.scene_qpc==f.scene_qpc&&f.publication_sequence>=r.minimum_publication_sequence;}
bool same_request(CoreReadRequest const& a,CoreReadRequest const& b){return core::same(a.native,b.native)&&
    hand::same(a.world,b.world)&&hand::same(a.pawn,b.pawn)&&hand::same(a.mesh,b.mesh)&&a.world_generation==b.world_generation&&
    a.scene_invocation==b.scene_invocation&&a.scene_qpc==b.scene_qpc&&a.minimum_publication_sequence==b.minimum_publication_sequence&&
    a.maximum_source_age_ns==b.maximum_source_age_ns;}
// Does not read an actor, bridge, module or current game pose. The version is a
// private cache revision; unchanged timestamps/Scene/source epochs remain raw.
enum class TickFailure {none,source_age_only,hard};
// Hard continuity only for the SAME already-consumed C0. This is neither a
// readable TickLatch nor a new source permit; elapsed age is never renewed.
bool retained_core_identity(core::CoreSnapshot const& copy)noexcept {try{
    const auto stamp=clock_ns();const auto session=rt::session();auto* broker=rt::current();
    api::NativeReference reference{};uint64_t revision{};
    if(!enabled||stopped.load()||!recording.load()||!native_session_current(broker,session)||
       copy.native.game_pid!=session.game_pid||copy.native.game_start!=session.game_start||copy.native.native_started_qpc!=session.native_started_qpc||
       committed_world_generation.load()!=copy.world_generation||!shared::tracking_ticket_current(copy.source_ticket)||
       !core::core_snapshot_non_age_valid(copy,stamp,50000000,simulator_operator)||!scene_fresh(copy.scene_qpc,copy.qpc_frequency,stamp,UINT64_MAX)||
       !reference_snapshot(reference,revision)||revision!=copy.native_reference_revision||reference.epoch!=copy.native_reference.epoch||reference.flags!=copy.native_reference.flags)return false;
    std::unique_lock<std::mutex> lock(mutex,std::try_to_lock);if(!lock.owns_lock())return false;
    const auto final_stamp=clock_ns();
    return !stopped.load()&&recording.load()&&native_session_current(broker,session)&&committed_world_generation.load()==copy.world_generation&&
        value.status==api::Status::ready&&source_ticket==copy.source_ticket&&value_reference_revision==copy.native_reference_revision&&
        !memcmp(&value,&copy,sizeof copy)&&shared::tracking_ticket_current(copy.source_ticket)&&
        reference_snapshot(reference,revision)&&revision==copy.native_reference_revision&&reference.epoch==copy.native_reference.epoch&&reference.flags==copy.native_reference.flags&&
        core::core_snapshot_non_age_valid(copy,final_stamp,50000000,simulator_operator)&&scene_fresh(copy.scene_qpc,copy.qpc_frequency,final_stamp,UINT64_MAX);
}catch(...){return false;}}
// A classified failure is observed at these same guards, never inferred later.
bool current_copy(core::CoreSnapshot const& copy,uint64_t revision,uint64_t age,TickFailure* failure=nullptr){
    if(failure)*failure=TickFailure::hard;
    const auto stamp=clock_ns();const auto session=rt::session();auto* broker=rt::current();
    api::NativeReference reference{};uint64_t reference_stamp{};
    if(!enabled||stopped.load()||!recording.load()||!native_session_current(broker,session)||
       copy.native.game_pid!=session.game_pid||copy.native.game_start!=session.game_start||copy.native.native_started_qpc!=session.native_started_qpc||
       committed_world_generation.load()!=copy.world_generation||!reference_snapshot(reference,reference_stamp)||reference_stamp!=copy.native_reference_revision||
       reference.epoch!=copy.native_reference.epoch||reference.flags!=copy.native_reference.flags||!shared::tracking_ticket_current(copy.source_ticket)||
       !core::core_snapshot_non_age_valid(copy,stamp,age,simulator_operator)||!scene_fresh(copy.scene_qpc,copy.qpc_frequency,stamp,UINT64_MAX)||
       ((copy.flags&api::controllers_matched)&&!source_fresh(copy.controllers.received_ns,stamp,UINT64_MAX)))return false;
    const bool first_fresh=core::core_snapshot_valid(copy,stamp,age,simulator_operator)&&
        scene_fresh(copy.scene_qpc,copy.qpc_frequency,stamp,age)&&
        (!(copy.flags&api::controllers_matched)||source_fresh(copy.controllers.received_ns,stamp,age));
    std::lock_guard<std::mutex> lock(mutex);
    const auto final_stamp=clock_ns();
    const bool hard_valid=!stopped.load()&&recording.load()&&native_session_current(broker,session)&&committed_world_generation.load()==copy.world_generation&&
        revision==value_version&&value.status==api::Status::ready&&value.publication_sequence==copy.publication_sequence&&source_ticket==copy.source_ticket&&
        value_reference_revision==copy.native_reference_revision&&shared::tracking_ticket_current(copy.source_ticket)&&
        reference_snapshot(reference,reference_stamp)&&reference_stamp==copy.native_reference_revision&&
        reference.epoch==copy.native_reference.epoch&&reference.flags==copy.native_reference.flags&&
        core::core_snapshot_non_age_valid(copy,final_stamp,age,simulator_operator)&&scene_fresh(copy.scene_qpc,copy.qpc_frequency,final_stamp,UINT64_MAX)&&
        (!(copy.flags&api::controllers_matched)||source_fresh(copy.controllers.received_ns,final_stamp,UINT64_MAX));
    if(!hard_valid)return false;
    const bool fresh=first_fresh&&core::core_snapshot_valid(copy,final_stamp,age,simulator_operator)&&
        scene_fresh(copy.scene_qpc,copy.qpc_frequency,final_stamp,age)&&
        (!(copy.flags&api::controllers_matched)||source_fresh(copy.controllers.received_ns,final_stamp,age));
    if(failure)*failure=fresh?TickFailure::none:TickFailure::source_age_only;
    return fresh;
}
// Distinct Scene-observation policy, not a successful V1 tracking read.
// The original current_copy still returns false for expired old tracking. Its
// classified result supplies the complete hard/cache/reference checks; this
// new contract separately bounds ONLY the original Scene/publication data.
bool metadata_copy_current(core::CoreSnapshot const& copy,uint64_t revision,TickFailure* failure=nullptr){
    if(failure)*failure=TickFailure::hard;
    TickFailure first{};const bool old_first=current_copy(copy,revision,50000000,&first);
    if(!old_first&&first!=TickFailure::source_age_only)return false;
    const auto at=clock_ns();
    const bool fresh=copy.published_ns<=uint64_t(INT64_MAX)&&source_fresh(int64_t(copy.published_ns),at,50000000)&&
        scene_fresh(copy.scene_qpc,copy.qpc_frequency,at,50000000);
    TickFailure last{};const bool old_last=current_copy(copy,revision,50000000,&last);
    if(!old_last&&last!=TickFailure::source_age_only)return false;
    const auto final=clock_ns();
    const bool result=fresh&&source_fresh(int64_t(copy.published_ns),final,50000000)&&scene_fresh(copy.scene_qpc,copy.qpc_frequency,final,50000000);
    if(failure)*failure=result?TickFailure::none:TickFailure::source_age_only;
    return result;
}
bool read_metadata_core_revision(CoreReadRequest const& request,core::CoreSnapshot& output,uint64_t& revision) noexcept {
    rt::detail::Call call;output={};revision=0;
    try{
        core::CoreSnapshot copy{};uint64_t captured{};{std::lock_guard<std::mutex> lock(mutex);++core_reads;copy=value;captured=value_version;}
        if(!request_shape(request)||!owner_matches(request,copy)||!metadata_copy_current(copy,captured)){
            std::lock_guard<std::mutex> lock(mutex);++core_read_rejected;return false;}
        std::memcpy(&output,&copy,sizeof output);
        if(!owner_matches(request,copy)||!metadata_copy_current(copy,captured)){
            output={};std::lock_guard<std::mutex> lock(mutex);++core_read_rejected;return false;}
        revision=captured;return true;
    }catch(...){output={};revision=0;return false;}
}
bool copy_scene_metadata_owner(binding::Binding& actor,CoreReadRequest& request) noexcept {
    actor={};request={};try {
        core::CoreSnapshot copy{};binding::Binding owner{};uint64_t revision{};
        {std::lock_guard<std::mutex> lock(mutex);copy=value;owner=scene_owner_binding;revision=value_version;}
        if(copy.status!=api::Status::ready||!owner.context_valid||owner.scene_invocation!=copy.scene_invocation||
           owner.world_generation!=copy.world_generation||!hand::same(object(owner.world),copy.world)||
           !hand::same(object(owner.pawn),copy.pawn)||!hand::same(object(owner.mesh),copy.mesh)||
           !metadata_copy_current(copy,revision))return false;
        CoreReadRequest next{};next.native=copy.native;next.world=copy.world;next.pawn=copy.pawn;next.mesh=copy.mesh;
        next.world_generation=copy.world_generation;next.scene_invocation=copy.scene_invocation;next.scene_qpc=copy.scene_qpc;
        next.minimum_publication_sequence=copy.publication_sequence;
        if(!request_shape(next)||!metadata_copy_current(copy,revision))return false;
        actor=owner;request=next;return true;
    }catch(...){return false;}
}
bool read_core_revision(CoreReadRequest const& request,core::CoreSnapshot& output,uint64_t& revision) noexcept {
    rt::detail::Call call;output={};revision=0;
    try{
        core::CoreSnapshot copy{};uint64_t captured{};{std::lock_guard<std::mutex> lock(mutex);++core_reads;copy=value;captured=value_version;}
        if(!request_shape(request)||!owner_matches(request,copy)||!current_copy(copy,captured,request.maximum_source_age_ns)){
            std::lock_guard<std::mutex> lock(mutex);++core_read_rejected;return false;}
        std::memcpy(&output,&copy,sizeof output);
        if(!owner_matches(request,copy)||!current_copy(copy,captured,request.maximum_source_age_ns)){
            output={};std::lock_guard<std::mutex> lock(mutex);++core_read_rejected;return false;}
        revision=captured;return true;
    }catch(...){output={};revision=0;return false;}
}
// Copy the owner captured by the SAME successful admitted Scene publication.
// No actor reads, no new Scene, and no admission/permission is granted here.
bool copy_scene_owner(binding::Binding& actor,CoreReadRequest& request) noexcept {
    actor={};request={};try {
        core::CoreSnapshot copy{};binding::Binding owner{};uint64_t revision{};
        {std::lock_guard<std::mutex> lock(mutex);copy=value;owner=scene_owner_binding;revision=value_version;}
        if(copy.status!=api::Status::ready||!owner.context_valid||owner.scene_invocation!=copy.scene_invocation||
           owner.world_generation!=copy.world_generation||!hand::same(object(owner.world),copy.world)||
           !hand::same(object(owner.pawn),copy.pawn)||!hand::same(object(owner.mesh),copy.mesh)||
           !current_copy(copy,revision,50000000))return false;
        CoreReadRequest next{};next.native=copy.native;next.world=copy.world;next.pawn=copy.pawn;next.mesh=copy.mesh;
        next.world_generation=copy.world_generation;next.scene_invocation=copy.scene_invocation;next.scene_qpc=copy.scene_qpc;
        next.minimum_publication_sequence=copy.publication_sequence;
        if(!request_shape(next)||!current_copy(copy,revision,next.maximum_source_age_ns))return false;
        actor=owner;request=next;return true;
    }catch(...){return false;}
}
// Thread-safe cached data only. A successful read grants no body/hand writer.
bool read_core(CoreReadRequest const& request,core::CoreSnapshot& output) noexcept {
    uint64_t revision{};return read_core_revision(request,output,revision);
}
struct TickKey {CoreReadRequest request{};uint64_t outer_tick{};};
class TickLatch {
    TickKey key_{};core::CoreSnapshot frame_{};uint64_t revision_{};bool active_{},valid_{};
    friend bool begin_tick(TickKey const&,TickLatch&) noexcept;
    friend bool read_tick(TickKey const&,TickLatch&,core::CoreSnapshot&,TickFailure*) noexcept;
    friend bool end_tick(TickKey const&,TickLatch&) noexcept;
public:
    TickLatch()=default;TickLatch(TickLatch const&)=delete;TickLatch& operator=(TickLatch const&)=delete;
};
bool same_tick(TickKey const& a,TickKey const& b){return a.outer_tick==b.outer_tick&&same_request(a.request,b.request);}
// Native integration owns one thread-confined latch on the real outer meshTick
// stack and passes this same object/key to all four controls. Scene is retained.
bool begin_tick(TickKey const& key,TickLatch& latch) noexcept {
    if(GetCurrentThreadId()!=key.request.native.game_thread)return false;
    if(latch.active_){latch.valid_=false;latch.frame_={};return false;}
    latch.active_=true;latch.key_=key;
    if(!key.outer_tick||!read_core_revision(key.request,latch.frame_,latch.revision_))return false;
    latch.valid_=true;return true;
}
// Explicit classification may return the original immutable copy on age-only
// failure for cohort diagnostics. It remains false, grants no writer, and burns
// the latch just like every original failed same-thread read. Untyped failures
// preserve the original empty-output contract.
bool read_tick(TickKey const& key,TickLatch& latch,core::CoreSnapshot& output,TickFailure* failure=nullptr) noexcept {
    output={};if(failure)*failure=TickFailure::hard;
    if(GetCurrentThreadId()!=key.request.native.game_thread)return false;
    try{
        if(!latch.active_||!latch.valid_)return false;
        if(!same_tick(key,latch.key_)||!owner_matches(key.request,latch.frame_)){
            latch.valid_=false;latch.frame_={};return false;}
        TickFailure why{};
        bool fresh=current_copy(latch.frame_,latch.revision_,key.request.maximum_source_age_ns,&why);
        if(fresh){
            std::memcpy(&output,&latch.frame_,sizeof output);
            fresh=current_copy(latch.frame_,latch.revision_,key.request.maximum_source_age_ns,&why);
        }
        if(!fresh){
            if(failure){*failure=why;if(why==TickFailure::source_age_only)std::memcpy(&output,&latch.frame_,sizeof output);else output={};}
            else output={};
            latch.valid_=false;latch.frame_={};return false;
        }
        if(failure)*failure=TickFailure::none;
        return true;
    }catch(...){output={};if(failure)*failure=TickFailure::hard;latch.valid_=false;latch.frame_={};return false;}
}
bool end_tick(TickKey const& key,TickLatch& latch) noexcept {
    if(GetCurrentThreadId()!=key.request.native.game_thread)return false;
    if(!latch.active_)return false;
    if(!same_tick(key,latch.key_)){latch.frame_={};latch.valid_=false;return false;}
    latch.key_={};latch.frame_={};latch.revision_=0;latch.active_=latch.valid_=false;return true;
}
// Separate lifetime for original Scene metadata. This never grants old
// tracking rights, never becomes a V1 TickLatch and never refreshes a stamp.
class SceneMetadataLatch {
    TickKey key_{};core::CoreSnapshot frame_{};uint64_t revision_{};bool active_{},valid_{};
    friend bool begin_scene_metadata_tick(TickKey const&,SceneMetadataLatch&) noexcept;
    friend bool read_scene_metadata_tick(TickKey const&,SceneMetadataLatch&,core::CoreSnapshot&,uint64_t&,TickFailure*) noexcept;
    friend bool end_scene_metadata_tick(TickKey const&,SceneMetadataLatch&) noexcept;
public:
    SceneMetadataLatch()=default;SceneMetadataLatch(SceneMetadataLatch const&)=delete;SceneMetadataLatch& operator=(SceneMetadataLatch const&)=delete;
};
bool begin_scene_metadata_tick(TickKey const& key,SceneMetadataLatch& latch) noexcept {
    if(GetCurrentThreadId()!=key.request.native.game_thread)return false;
    if(latch.active_){latch.valid_=false;latch.frame_={};return false;}
    latch.active_=true;latch.key_=key;
    if(!key.outer_tick||!read_metadata_core_revision(key.request,latch.frame_,latch.revision_))return false;
    latch.valid_=true;return true;
}
bool read_scene_metadata_tick(TickKey const& key,SceneMetadataLatch& latch,core::CoreSnapshot& output,uint64_t& revision,TickFailure* failure=nullptr) noexcept {
    output={};revision=0;if(failure)*failure=TickFailure::hard;
    if(GetCurrentThreadId()!=key.request.native.game_thread)return false;
    try{
        if(!latch.active_||!latch.valid_)return false;
        if(!same_tick(key,latch.key_)||!owner_matches(key.request,latch.frame_)){
            latch.valid_=false;latch.frame_={};return false;}
        TickFailure why{};bool fresh=metadata_copy_current(latch.frame_,latch.revision_,&why);
        if(fresh){std::memcpy(&output,&latch.frame_,sizeof output);fresh=metadata_copy_current(latch.frame_,latch.revision_,&why);}
        if(!fresh){
            if(failure){*failure=why;if(why==TickFailure::source_age_only)std::memcpy(&output,&latch.frame_,sizeof output);else output={};}
            else output={};latch.valid_=false;latch.frame_={};return false;
        }
        revision=latch.revision_;if(failure)*failure=TickFailure::none;return true;
    }catch(...){output={};revision=0;if(failure)*failure=TickFailure::hard;latch.valid_=false;latch.frame_={};return false;}
}
bool end_scene_metadata_tick(TickKey const& key,SceneMetadataLatch& latch) noexcept {
    if(GetCurrentThreadId()!=key.request.native.game_thread)return false;
    if(!latch.active_)return false;
    if(!same_tick(key,latch.key_)){latch.frame_={};latch.valid_=false;return false;}
    latch.key_={};latch.frame_={};latch.revision_=0;latch.active_=latch.valid_=false;return true;
}
api::Reply read(api::ReadRequest const* input,api::Frame* output) noexcept {
    rt::detail::Call call;api::Frame empty{};api::ReadRequest request{};
    if(!hand::weapon_broker_win32::copy(output,&empty,sizeof empty)||!hand::weapon_broker_win32::copy(&request,input,sizeof request)||!api::request_valid(request))return api::Reply::invalid;
    try{
        core::CoreSnapshot copy{};uint64_t revision{};{std::lock_guard<std::mutex> lock(mutex);++reads;copy=value;revision=value_version;}
        const auto session=rt::session();auto* broker=rt::current();const auto registration_cookie=broker?broker->cookie():0;
        if(stopped.load()||rt::detail::stop_requested.load())return api::Reply::stopping;
        if(request.registration_cookie!=registration_cookie||request.native_started_qpc!=session.native_started_qpc)return api::Reply::wrong_cookie;
        api::Frame projected{};
        const api::NativeKey authenticated{session.game_pid,copy.native.game_thread,session.game_start,session.native_started_qpc,registration_cookie};
        if(!session_current(broker,session,registration_cookie)||copy.publication_sequence<request.minimum_publication_sequence||
           !current_copy(copy,revision,request.maximum_source_age_ns)||!session_current(broker,session,registration_cookie)||
           !core::project_for_authenticated_registration(copy,authenticated,clock_ns(),request.maximum_source_age_ns,projected,simulator_operator)){
            std::lock_guard<std::mutex> lock(mutex);++read_rejected;return api::Reply::unavailable;}
        if(!hand::weapon_broker_win32::copy(output,&projected,sizeof projected))return api::Reply::invalid;
        if(!session_current(broker,session,registration_cookie)||!current_copy(copy,revision,request.maximum_source_age_ns)||!session_current(broker,session,registration_cookie)){
            hand::weapon_broker_win32::copy(output,&empty,sizeof empty);return api::Reply::unavailable;}
        return api::Reply::accepted;
    }catch(...){hand::weapon_broker_win32::copy(output,&empty,sizeof empty);return api::Reply::unavailable;}
}
void stop() noexcept {stopped.store(true);try{std::lock_guard<std::mutex> lock(mutex);value={};scene_owner_binding={};changed();}catch(...){}}
bool configure(const wchar_t* ini,FILE* file){const auto request=GetPrivateProfileIntW(L"RigTracking",L"enabled",0,ini);if(!request)return !enabled;
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
    if(request!=1||enabled||stopped.load()||!rt::enabled||!continuous_session||!stereo_xr||!anchor_observer_enabled)return false;
    enabled=true;shared::continuity_enabled=true;if(file)fprintf(file,"rig_tracking_config enabled=1 data_only=1 native_core_without_registration=1 producer_companion_optional=1 native_reference_mirror=1 setters_granted=0 ghw1_certificate=0\n");return true;
#else
    (void)file;return false;
#endif
}
void summary(FILE* file){std::lock_guard<std::mutex> lock(mutex);fprintf(file,"rig_tracking_enabled=%u rig_tracking_stopped=%u rig_tracking_attempts=%llu rig_tracking_published=%llu rig_tracking_rejected=%llu rig_tracking_duplicates=%llu rig_tracking_reads=%llu rig_tracking_read_rejected=%llu rig_tracking_sequence=%llu rig_tracking_binding_generation=%llu rig_tracking_observed_stream=%llu rig_tracking_observed_reference=%llu rig_tracking_observed_head=%llu rig_tracking_observed_left=%llu rig_tracking_observed_right=%llu rig_tracking_producer_continuity=%u rig_tracking_native_reference_epoch=%llu rig_tracking_native_reference_flags=%u rig_tracking_reference_failed=%u rig_tracking_setters_granted=0 ",unsigned(enabled),unsigned(stopped.load()),attempts,published,rejected,duplicates,reads,read_rejected,static_cast<unsigned long long>(sequence),static_cast<unsigned long long>(binding_generation),static_cast<unsigned long long>(observed.stream),static_cast<unsigned long long>(observed.reference),static_cast<unsigned long long>(observed.head_tracking),static_cast<unsigned long long>(observed.hand_tracking[0]),static_cast<unsigned long long>(observed.hand_tracking[1]),unsigned((value.flags&api::producer_continuity_available)!=0),static_cast<unsigned long long>(reference_epoch.load()),reference_flags.load(),unsigned(reference_failed.load()));fprintf(file,"rig_tracking_core_reads=%llu rig_tracking_core_read_rejected=%llu rig_tracking_cache_revision=%llu ",core_reads,core_read_rejected,static_cast<unsigned long long>(value_version));}
} // namespace rig_tracking_live
