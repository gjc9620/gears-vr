// Include after Native backend and its real receipt implementation. The pure
// body_c0_neutral.h is included globally before capture's anonymous namespace.
namespace body_c0_goal_policy_live {
namespace budget=::gears_body_c0_budget;
namespace n=full_body_c0_native_live;namespace b=gears_full_body_c0;namespace a=gears_body_goal_view;
namespace neutral=gears_body_c0_neutral;namespace bind=weapon_hand_scene_binding;
namespace rig=rig_tracking_live;namespace pose=gears_cover_pose_phase;namespace core=gears_native_rig_tracking;
using Entry=gears_continuous_hand_entry::Entry;
bool enabled{};
#if defined(GEARS_BODY_C0_GOAL_POLICY) && GEARS_BODY_C0_GOAL_POLICY
std::atomic<bool> stopping{},view_valid{};std::atomic<unsigned> callbacks{},open_ends{};
std::atomic<uint64_t> attempts{},plans{},applied{},published{},rejected{},view_rejected{},view_used{},revision{1};
std::atomic<uint64_t> view_held{};
std::atomic<uint64_t> neutral_rebind_attempts{},neutral_rebind_deferred{},neutral_rebinds{};
struct Call {Call(){++callbacks;}~Call(){--callbacks;}};
struct Key {bind::Binding actor{};gears_anchor::Identity asset{};core::CoreSnapshot source{};
    pose::ArrayHeader reference{};std::array<b::rig::ReferenceRecord68,b::bones> ref{};double units{},model_scale{};};
struct NeutralState {Key key{};neutral::Stored stored{};a::marker::Marker marker{};a::marker::Workspace work{};
    bool initialized{},poisoned{};} origin;
enum class RejectReason : unsigned {wrong_thread=1,stopping,pending,uncaptured,wrong_entry,same_entry,
    current_prepare,standing,cover,key_mismatch,poisoned,initialize,neutral_move,adapter,
    receipt,current_final,standing_final,apply,exception};
struct RejectSource {uint64_t world{},xr_session{},space{},reference{},native_revision{},native_epoch{},
    stream{},observed_reference{},head{};double units{};unsigned native_flags{};};
struct FirstReject {uint64_t epoch{},tick{},compose{},qpc{},qpc_frequency{},game_start{},native_started_qpc{},key_bits{};
    uint32_t pid{},thread{},owner_thread{};unsigned reason{},adapter_status{};
    bool source_available{},key_available{},origin_initialized{},origin_poisoned{};
    RejectSource old_source{},current_source{};};
std::atomic<unsigned> first_reject_state{};FirstReject first_reject{};
RejectSource reject_source(core::CoreSnapshot const& s) noexcept {return {
    s.world_generation,s.stereo.session,s.stereo.space_epoch,s.reference_generation,
    s.native_reference_revision,s.native_reference.epoch,s.observed.stream,s.observed.reference,
    s.observed.head_tracking,s.units_per_metre,s.native_reference.flags};}
void note_held_reject(RejectReason reason,Entry const& e,bool source_available,unsigned adapter_status) noexcept {
    auto& journal=::gears_body_held_outcome::recorder;
    const auto entry=reinterpret_cast<uintptr_t>(&e);
    if(!journal.exact(entry,e.invocation))return;
    const bool captured=source_available&&n::state.captured;
    const bool key=captured&&origin.initialized;
    auto result=::gears_body_held_outcome::unknown;
    if(key){auto const& k=origin.key;auto const& s=n::state;result=0;
            const bool bits[]={bind::same_key(k.actor,s.context.actor),bind::same(k.asset,s.context.asset),
                core::same(k.source.native,s.context.source.native),k.source.world_generation==s.context.source.world_generation,
                k.source.stereo.source==s.context.source.stereo.source,k.source.stereo.session==s.context.source.stereo.session,
                k.source.stereo.space_epoch==s.context.source.stereo.space_epoch,
                k.source.reference_generation==s.context.source.reference_generation,
                k.source.native_reference_revision==s.context.source.native_reference_revision,
                k.source.native_reference.epoch==s.context.source.native_reference.epoch&&
                    k.source.native_reference.flags==s.context.source.native_reference.flags,
                k.source.observed.stream==s.context.source.observed.stream,
                k.source.observed.reference==s.context.source.observed.reference,
                k.source.observed.head_tracking==s.context.source.observed.head_tracking,
                k.source.units_per_metre==s.context.source.units_per_metre,
                gears_vr::stereo_same_pose(k.source.reference,s.context.source.reference),
                pose::header_equal(k.reference,s.backend.metadata().reference),
                k.units==s.source.units_per_metre,k.model_scale==s.source.model_scale,
                !std::memcmp(k.ref.data(),s.source.reference.data(),sizeof k.ref)};
            for(unsigned i=0;i<sizeof bits/sizeof bits[0];++i)if(bits[i])result|=uint64_t(1)<<i;
    }
    journal.goal_result(entry,e.invocation,unsigned(reason),captured,key,origin.initialized,origin.poisoned,result,adapter_status);
}
void note_reject(RejectReason reason,Entry const& e,bool source_available=true,unsigned adapter_status=0) noexcept {
    note_held_reject(reason,e,source_available,adapter_status);
    const auto epoch=cover_shared_input_live::goal_absent_epoch.load(std::memory_order_acquire);
    if(!epoch)return;
    unsigned expected=0;if(!first_reject_state.compare_exchange_strong(expected,1,std::memory_order_acq_rel))return;
    try{
    FirstReject row{};row.epoch=epoch;row.compose=e.invocation;LARGE_INTEGER q{},frequency{};
    if(QueryPerformanceCounter(&q)&&q.QuadPart>0)row.qpc=uint64_t(q.QuadPart);
    if(QueryPerformanceFrequency(&frequency)&&frequency.QuadPart>0)row.qpc_frequency=uint64_t(frequency.QuadPart);
    row.pid=GetCurrentProcessId();row.thread=GetCurrentThreadId();row.owner_thread=n::owner_thread.load();
    row.reason=unsigned(reason);row.adapter_status=adapter_status;
    if(source_available&&n::state.captured){auto const& s=n::state;row.source_available=true;row.tick=s.context.tick;
        row.game_start=s.context.source.native.game_start;row.native_started_qpc=s.context.source.native.native_started_qpc;
        row.origin_initialized=origin.initialized;row.origin_poisoned=origin.poisoned;
        row.current_source=reject_source(s.context.source);row.key_available=origin.initialized;
        if(row.key_available){auto const& k=origin.key;row.old_source=reject_source(k.source);
            const bool bits[]={bind::same_key(k.actor,s.context.actor),bind::same(k.asset,s.context.asset),
                core::same(k.source.native,s.context.source.native),k.source.world_generation==s.context.source.world_generation,
                k.source.stereo.source==s.context.source.stereo.source,k.source.stereo.session==s.context.source.stereo.session,
                k.source.stereo.space_epoch==s.context.source.stereo.space_epoch,
                k.source.reference_generation==s.context.source.reference_generation,
                k.source.native_reference_revision==s.context.source.native_reference_revision,
                k.source.native_reference.epoch==s.context.source.native_reference.epoch&&
                    k.source.native_reference.flags==s.context.source.native_reference.flags,
                k.source.observed.stream==s.context.source.observed.stream,
                k.source.observed.reference==s.context.source.observed.reference,
                k.source.observed.head_tracking==s.context.source.observed.head_tracking,
                k.source.units_per_metre==s.context.source.units_per_metre,
                gears_vr::stereo_same_pose(k.source.reference,s.context.source.reference),
                pose::header_equal(k.reference,s.backend.metadata().reference),
                k.units==s.source.units_per_metre,k.model_scale==s.source.model_scale,
                !std::memcmp(k.ref.data(),s.source.reference.data(),sizeof k.ref)};
            for(unsigned i=0;i<sizeof bits/sizeof bits[0];++i)if(bits[i])row.key_bits|=uint64_t(1)<<i;
        }
    }
    first_reject=row;first_reject_state.store(2,std::memory_order_release);
    }catch(...){first_reject_state.store(3,std::memory_order_release);}
}
struct Publication {Key key{};gears_vr::CoverViewResult render{};b::Matrix64 component{};
    neutral::Stored stored{};
    uint64_t version{},tick{},compose{},revision{};int64_t ended_ns{};};
std::mutex view_mutex;Publication view{};uint64_t publication_sequence{};
struct Pending {Entry const* entry{};Entry copy{};Publication next{};uint64_t revision{};bool committed{};} pending;
bind::Proof standing_proof{};std::array<unsigned,2> crouch_masks{};
std::array<unsigned char,sizeof(gears_anchor::Config)> metadata_bytes{};uint64_t metadata_generation{};

void invalidate() noexcept {view_valid=false;auto old=revision.load();while(old!=UINT64_MAX&&!revision.compare_exchange_weak(old,old+1)){}
    if(old==UINT64_MAX)stopping=true;}
bool receipt_ready() noexcept {
#if defined(GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT) && GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT
    return native_atom_overwrite_live::ready_for_commit();
#else
    return false; // No receipt implementation means no real write permission.
#endif
}
bool standing(bind::Binding const& actor){budget::Span measured(budget::Op::standing);
    if(actor.special_move||!bind::current_actor(actor))return false;
    {std::unique_lock<std::mutex> lock(bind::mutex,std::try_to_lock);if(!lock.owns_lock()||!bind::metadata_current())return false;
        if(metadata_generation!=bind::metadata_generation||metadata_bytes!=bind::metadata_bytes){standing_proof={};crouch_masks={};metadata_generation=bind::metadata_generation;metadata_bytes=bind::metadata_bytes;}
        gears_anchor::Identity cls{};if(!bind::identity(actor.pawn.cls,cls))return false;bind::keys(standing_proof,cls);
        if(!bind::begin_prepare(standing_proof))return false;
        if(!standing_proof.ready){const wchar_t* names[]={L"CoverType",L"bIsCrouched",L"bWantsToCrouch"};crouch_masks={};
            for(unsigned i=0;i<3;++i){auto& field=standing_proof.fields[i];
                if(!bind::property(standing_proof,cls.address,names[i],i?L"BoolProperty":L"ByteProperty",i?4u:1u,nullptr,field))return false;
                if(i&&(!bind::source_field(standing_proof,crouch_masks[i-1],field.id.address,0xd0)||!crouch_masks[i-1]||(crouch_masks[i-1]&(crouch_masks[i-1]-1))))return false;}
            if(!bind::current(standing_proof))return false;standing_proof.ready=true;}
        unsigned char ct[2]{};unsigned words[2][2]{};
        for(unsigned repeat=0;repeat<2;++repeat){if(!bind::field(ct[repeat],actor.pawn.address,standing_proof.fields[0].offset))return false;
            for(unsigned i=0;i<2;++i)if(!bind::field(words[repeat][i],actor.pawn.address,standing_proof.fields[i+1].offset))return false;}
        if(ct[0]||ct[1]||std::memcmp(words[0],words[1],sizeof words[0])||
           (words[0][0]&crouch_masks[0])||(words[0][1]&crouch_masks[1])||!bind::current(standing_proof)||!bind::metadata_current())return false;
    }
    return !actor.special_move&&bind::current_actor(actor);
}
bool no_cover(core::CoreSnapshot const& s){return !(s.flags&core::rig::cover_key_known)&&!s.cover_type&&!s.cover_link.address&&s.cover_slot==-1;}
bool same_hard_source_basis(core::CoreSnapshot const& old,core::CoreSnapshot const& s){
    return core::same(old.native,s.native)&&old.world_generation==s.world_generation&&
        old.stereo.source==s.stereo.source&&old.stereo.session==s.stereo.session&&old.stereo.space_epoch==s.stereo.space_epoch&&
        old.reference_generation==s.reference_generation&&old.native_reference_revision==s.native_reference_revision&&
        old.native_reference.epoch==s.native_reference.epoch&&old.native_reference.flags==s.native_reference.flags&&
        gears_vr::stereo_same_pose(old.reference,s.reference)&&old.units_per_metre==s.units_per_metre;
}
bool same_observed_basis(core::CoreSnapshot const& old,core::CoreSnapshot const& s){
    return old.observed.stream==s.observed.stream&&old.observed.reference==s.observed.reference&&
        old.observed.head_tracking==s.observed.head_tracking;
}
bool same_source_basis(core::CoreSnapshot const& old,core::CoreSnapshot const& s){
    return same_hard_source_basis(old,s)&&same_observed_basis(old,s);
}
bool same_hard_key(Key const& key){auto const& s=n::state;return bind::same_key(key.actor,s.context.actor)&&bind::same(key.asset,s.context.asset)&&
    same_hard_source_basis(key.source,s.context.source)&&pose::header_equal(key.reference,s.backend.metadata().reference)&&
    key.units==s.source.units_per_metre&&key.model_scale==s.source.model_scale&&
    !std::memcmp(key.ref.data(),s.source.reference.data(),sizeof key.ref);}
bool same_key(Key const& key){return same_hard_key(key)&&same_observed_basis(key.source,n::state.context.source);}
// Observed counters also advance after a rejected publication. They are not
// the independent hard reference epoch. Reuse only immutable mesh-local
// calibration, never an old tracking sample or a newly animated Local pose.
bool observed_rebind_candidate(Key const& key){
    if(!same_hard_key(key))return false;
    auto const& a=key.source.observed;auto const& b=n::state.context.source.observed;
    return a.stream&&a.reference&&a.head_tracking&&b.stream&&b.reference&&b.head_tracking&&
        b.stream>=a.stream&&b.reference>=a.reference&&b.head_tracking>=a.head_tracking&&
        (b.stream>a.stream||b.reference>a.reference||b.head_tracking>a.head_tracking);
}
bool observed_rebind_idle(){auto const& s=n::state;return !pending.entry&&!open_ends.load()&&
    !s.prepared&&!s.notified&&!s.atom_installed&&!s.recovery.active&&
    !s.kernel.pending()&&!s.kernel.outstanding_fields()&&!s.kernel.protocol_debt()&&
    !s.kernel.failed()&&!s.kernel.stopping()&&!s.kernel.original_exception();}
bool initialize(){auto& s=n::state;
    if(origin.initialized)return same_key(origin.key);if(origin.poisoned)return false;
    if(n::commits.load()||s.kernel.pending()||s.prepared||s.notified||!s.captured||s.source.units_per_metre!=s.context.source.units_per_metre)return false;
    a::marker::Marker marker{};neutral::Stored stored{};
    if(a::marker::prepare(s.model,s.source,a::marker::Basis::marcus_mesh_face_forward_y,origin.work,marker)!=a::marker::Status::okay||
       !neutral::capture(s.model,s.source,marker,s.workspace,stored)||!s.context.current(n::tx::Stage::prepare)||!standing(s.context.actor))return false;
    auto& k=origin.key;k.actor=s.context.actor;k.asset=s.context.asset;k.source=s.context.source;k.reference=s.backend.metadata().reference;
    k.ref=s.source.reference;k.units=s.source.units_per_metre;k.model_scale=s.source.model_scale;
    origin.marker=marker;origin.stored=stored;origin.initialized=true;return true;
}
void pose_begin(Entry const& e) noexcept {Call call;if(e.kind==0)budget::recorder.mark_entry(budget::Stage::goal_entry,reinterpret_cast<uintptr_t>(&e),e.invocation);if(!enabled||e.kind!=0||!e.selected)return;++attempts;
    if(first_person_live::body_basis&&!first_person_live::toggle.requested()){invalidate();return;}
    if(GetCurrentThreadId()!=n::owner_thread.load()){note_reject(RejectReason::wrong_thread,e,false);++rejected;return;}
    try{auto& s=n::state;
        if(stopping.load()){note_reject(RejectReason::stopping,e);++rejected;return;}
        if(pending.entry){note_reject(RejectReason::pending,e);++rejected;return;}
        if(!s.captured){note_reject(RejectReason::uncaptured,e);++rejected;return;}
        if(s.context.entry!=&e){note_reject(RejectReason::wrong_entry,e);++rejected;return;}
        if(!n::same_entry(e)){note_reject(RejectReason::same_entry,e);++rejected;return;}
        if(!s.context.current(n::tx::Stage::prepare)){note_reject(RejectReason::current_prepare,e);++rejected;return;}
        if(!standing(s.context.actor)){note_reject(RejectReason::standing,e);++rejected;return;}
        if(!no_cover(s.context.source)){note_reject(RejectReason::cover,e);++rejected;return;}
        budget::recorder.mark(budget::Stage::goal_initial_end);bool rebind_observed=false;
        if(origin.initialized&&!same_key(origin.key)){
            if(origin.poisoned||!observed_rebind_candidate(origin.key)){
                origin.poisoned=true;note_reject(RejectReason::key_mismatch,e);++rejected;return;}
            ++neutral_rebind_attempts;rebind_observed=true;
            if(!observed_rebind_idle()){
                ++neutral_rebind_deferred;note_reject(RejectReason::key_mismatch,e);++rejected;return;}}
        if(origin.poisoned){note_reject(RejectReason::poisoned,e);++rejected;return;}
        if(!rebind_observed&&!initialize()){note_reject(RejectReason::initialize,e);++rejected;return;}
        a::Neutral moved{};a::Output prepared{};a::Policy policy{};policy.scope=b::Scope::stationary;policy.posture=b::Posture::reference_neutral;
        policy.now_ns=rig::clock_ns();policy.allow_sim=simulator_operator;
        // Ordinary SM0 prototype has no Cover drag owner. Its committed virtual
        // offset is explicitly zero. Cover interaction remains a later policy.
        const gears_vr::CoverOffset committed{};
        if(!neutral::move(origin.stored,s.source.component_to_world,moved)){note_reject(RejectReason::neutral_move,e);++rejected;return;}
        const auto prepared_status=body_tick_source_live::enabled?
            a::prepare(moved,s.context.body_source,s.model,s.source,origin.marker,committed,policy,prepared):body_tick_input_live::enabled?
            a::prepare(moved,s.context.body_input,s.model,s.source,origin.marker,committed,policy,prepared):
            a::prepare(moved,s.context.source,s.model,s.source,origin.marker,committed,policy,prepared);
        if(prepared_status!=a::Status::okay){note_reject(RejectReason::adapter,e,true,unsigned(prepared_status));++rejected;return;}++plans;
        body_tick_tracking_probe_live::note_basis(s.context.tick,e.invocation,s.source.component_to_world,prepared.render.base_view,s.model.signature);
        budget::recorder.mark(budget::Stage::goal_adapter_end);
        if(!receipt_ready()){note_reject(RejectReason::receipt,e);++rejected;return;}
        if(!s.context.current(n::tx::Stage::prepare)){note_reject(RejectReason::current_final,e);++rejected;return;}
        if(!standing(s.context.actor)){note_reject(RejectReason::standing_final,e);++rejected;return;}
        if(rebind_observed){
            // Commit only after the ordinary current-source, marker/adapter,
            // receipt and final guards. A deferred attempt leaves the old key.
            if(!observed_rebind_candidate(origin.key)||!observed_rebind_idle()){
                ++neutral_rebind_deferred;note_reject(RejectReason::key_mismatch,e);++rejected;return;}
            invalidate();
            if(stopping.load()){note_reject(RejectReason::stopping,e);++rejected;return;}
            origin.key.source.observed.stream=s.context.source.observed.stream;
            origin.key.source.observed.reference=s.context.source.observed.reference;
            origin.key.source.observed.head_tracking=s.context.source.observed.head_tracking;
            ++neutral_rebinds;
        }
        budget::recorder.mark(budget::Stage::goal_final_end);pending={};pending.entry=&e;pending.copy=e;pending.revision=revision.load();open_ends=1;
        pending.next.key=origin.key;pending.next.key.actor=s.context.actor;pending.next.key.source=s.context.source;
        pending.next.render=prepared.render;pending.next.component=s.source.component_to_world;
        pending.next.stored=origin.stored;
        pending.next.tick=s.context.tick;pending.next.compose=e.invocation;pending.next.revision=pending.revision;
        pending.committed=n::apply_with_view_marker(e,prepared.goal,s.context.source,prepared.render.base_view,origin.marker,&prepared.desired_eye);if(pending.committed)++applied;
        else {note_reject(RejectReason::apply,e);++rejected;}
    }catch(...){note_reject(RejectReason::exception,e);++rejected;invalidate();}
}
void pose_end(Entry const& e,bool returned,bool original_called) noexcept {Call call;
    if(GetCurrentThreadId()!=n::owner_thread.load()){n::pose_end(e,returned,original_called);return;}
    bool exact{},publish{};try{exact=pending.entry==&e&&!std::memcmp(&pending.copy,&e,sizeof e);
        publish=exact&&pending.committed&&returned&&original_called&&enabled&&!stopping.load()&&pending.revision==revision.load()&&
            n::state.captured&&n::same_entry(e)&&n::state.notified&&n::state.kernel.consumed()&&!n::state.kernel.failed()&&
            !n::state.kernel.protocol_debt()&&n::state.context.current(n::tx::Stage::scope_end)&&standing(n::state.context.actor)&&same_key(origin.key);
    }catch(...){publish=false;}
    n::pose_end(e,returned,original_called); // Real finally, even OFF or rejected.
    try{if(exact){if(publish&&!n::state.kernel.failed()&&!n::state.kernel.protocol_debt()&&!n::state.kernel.original_exception()&&
           !stopping.load()&&pending.revision==revision.load()){
            std::lock_guard<std::mutex> lock(view_mutex);if(publication_sequence==UINT64_MAX){stopping=true;invalidate();}
            else if(!stopping.load()&&pending.revision==revision.load()) {pending.next.version=++publication_sequence;pending.next.ended_ns=rig::clock_ns();view=pending.next;view_valid=true;++published;}}
        else if(!returned||!original_called||n::state.kernel.protocol_debt()||n::state.kernel.original_exception())invalidate();
        pending={};open_ends=0;}
    }catch(...){invalidate();if(exact){pending={};open_ends=0;}}
}
bool current_geometry(Publication const& p,b::Matrix64& component){
    auto reader=[](void* o,void const* i,size_t bytes){return safe_copy(o,i,bytes);};
    pose::Request request{};request.owner.mesh=cover_pose_phase_live::compact(p.key.actor.mesh);
    pose::Metadata before{},after{};gears_anchor::Identity asset{};
    std::unique_lock<std::mutex> lock(bind::mutex,std::try_to_lock);
    const bool current=lock.owns_lock()&&bind::metadata_current()&&pose::metadata(reader,request,before)&&!before.master&&
        before.asset==p.key.asset.address&&pose::header_equal(before.reference,p.key.reference)&&pose::header_valid(before.reference,68)&&
        before.reference.count==115&&
        bind::identity(before.asset,asset,true)&&bind::same(asset,p.key.asset)&&
        pose::repeat(reader,p.key.ref.data(),before.reference.data,sizeof p.key.ref)==pose::Reject::none&&
        pose::metadata(reader,request,after)&&pose::metadata_equal(before,after)&&bind::metadata_current();
    if(current)std::memcpy(&component,&before.component,sizeof component);return current;
}
bool read_view(gears_anchor::Receipt const* scene,gears_vr::StereoResult const& stereo,double units,gears_vr::CoverViewResult& output) noexcept {Call call;
    auto deny=[](bool clear=true){++view_rejected;if(clear)invalidate();return false;};
    try{if(!enabled||stopping.load()||!recording.load()||!scene||scene->thread!=GetCurrentThreadId()||!view_valid.load()||!stereo||
        (first_person_live::body_basis&&!first_person_live::toggle.requested()))return deny();
        Publication copy{};{std::lock_guard<std::mutex> lock(view_mutex);if(!view_valid.load())return deny();copy=view;}
        auto const& s=copy.key.source;const auto now=rig::clock_ns();core::rig::NativeReference reference{};uint64_t ref_revision{};
        const auto session=weapon_hand_runtime::session();bind::Binding actor{};
        if(!copy.version||copy.revision!=revision.load()||units!=copy.key.units||copy.key.model_scale!=1||!no_cover(s)||
           !rig::native_session_current(weapon_hand_runtime::current(),session)||session.game_pid!=s.native.game_pid||session.game_start!=s.native.game_start||session.native_started_qpc!=s.native.native_started_qpc||
           s.native.game_thread!=scene->thread||s.world_generation!=committed_world_generation.load()||
           !rig::reference_snapshot(reference,ref_revision)||ref_revision!=s.native_reference_revision||reference.epoch!=s.native_reference.epoch||reference.flags!=s.native_reference.flags||
           stereo.reference_generation!=s.reference_generation||!gears_vr::stereo_same_pose(stereo.reference,s.reference)||
           stereo.sample.source!=s.stereo.source||stereo.sample.session!=s.stereo.session||stereo.sample.space_epoch!=s.stereo.space_epoch)return deny();
        if(
           !gears_vr::stereo_tracking_valid(stereo.sample,simulator_operator)||!gears_vr::stereo_render_active(stereo.sample)||stereo.sample.focused!=1||
           !rig::source_fresh(stereo.sample.received_ns,now,a::maximum_age_ns))return deny(false);
        b::Matrix64 component{};a::Neutral moved{};gears_vr::CoverViewResult render{};
        if(!bind::sample(scene,actor)||!bind::same_key(actor,copy.key.actor)||!standing(actor)||!current_geometry(copy,component)||
           !neutral::move(copy.stored,component,moved)||!gears_vr::prepare_cover_view(moved.eye_anchor,{},units,render)||
           !bind::current_actor(actor)||!rig::reference_snapshot(reference,ref_revision)||ref_revision!=s.native_reference_revision)return deny();
        std::lock_guard<std::mutex> lock(view_mutex);
        if(stopping.load()||!view_valid.load()||copy.version!=view.version||copy.revision!=revision.load())return deny();
        // Only the immutable neutral basis is held. Current Scene/geometry and
        // current Stereo were revalidated above; no old tracking time or body
        // commit is refreshed. This standing policy has zero virtual offset.
        output=render;++view_used;if(!rig::source_fresh(copy.ended_ns,now,a::maximum_age_ns))++view_held;return true;
    }catch(...){return deny();}
}
bool configure(wchar_t const* ini,FILE* file){wchar_t value[8]{};GetPrivateProfileStringW(L"BodyC0GoalPolicy",L"enabled",L"0",value,8,ini);
    if(wcscmp(value,L"0")&&wcscmp(value,L"1"))return false;enabled=!wcscmp(value,L"1");if(enabled&&!n::enabled)return false;
    if(file)fprintf(file,"body_goal_policy_compiled=1 body_goal_policy_enabled=%u body_goal_scope=standing_SM0_CT0_not_crouched body_goal_scene_view_wired=1 body_goal_virtual_offset=0\n",unsigned(enabled));return true;}
void request_drain(){stopping=true;invalidate();}unsigned pending_count(){return callbacks.load()+open_ends.load();}
void summary(FILE* f){if(!f)return;
    fprintf(f,"body_goal_enabled=%u body_goal_attempts=%llu body_goal_plans=%llu body_goal_applied=%llu body_goal_published=%llu body_goal_rejected=%llu body_goal_view_rejected=%llu body_goal_view_used=%llu body_goal_pending=%u body_goal_view_available=%u body_goal_view_held=%llu ",unsigned(enabled),attempts.load(),plans.load(),applied.load(),published.load(),rejected.load(),view_rejected.load(),view_used.load(),pending_count(),unsigned(view_valid.load()),view_held.load());
    fprintf(f,"body_goal_neutral_rebind_attempts=%llu body_goal_neutral_rebind_deferred=%llu body_goal_neutral_rebinds=%llu ",neutral_rebind_attempts.load(),neutral_rebind_deferred.load(),neutral_rebinds.load());
    fprintf(f,"body_goal_absent_epoch=%llu ",static_cast<unsigned long long>(cover_shared_input_live::goal_absent_epoch.load(std::memory_order_acquire)));
    const auto state=first_reject_state.load(std::memory_order_acquire);
    if(state==2){auto const& r=first_reject;
        fprintf(f,"body_goal_first_epoch=%llu body_goal_first_reason=%u body_goal_first_adapter=%u body_goal_first_pid=%u body_goal_first_thread=%u body_goal_first_owner_thread=%u body_goal_first_tick=%llu body_goal_first_compose=%llu body_goal_first_qpc=%llu body_goal_first_qpc_frequency=%llu body_goal_first_game_start=%llu body_goal_first_native_started=%llu body_goal_first_source=%u body_goal_first_key=%u body_goal_first_initialized=%u body_goal_first_poisoned=%u body_goal_first_bits=%llx body_goal_first_old_world=%llu body_goal_first_new_world=%llu body_goal_first_old_stream=%llu body_goal_first_new_stream=%llu body_goal_first_old_observed_ref=%llu body_goal_first_new_observed_ref=%llu body_goal_first_old_head=%llu body_goal_first_new_head=%llu body_goal_first_old_xr_session=%llu body_goal_first_new_xr_session=%llu body_goal_first_old_space=%llu body_goal_first_new_space=%llu body_goal_first_old_ref=%llu body_goal_first_new_ref=%llu body_goal_first_old_native_revision=%llu body_goal_first_new_native_revision=%llu body_goal_first_old_native_epoch=%llu body_goal_first_new_native_epoch=%llu body_goal_first_old_units=%.9g body_goal_first_new_units=%.9g body_goal_first_old_native_flags=%u body_goal_first_new_native_flags=%u ",
            static_cast<unsigned long long>(r.epoch),r.reason,r.adapter_status,r.pid,r.thread,r.owner_thread,
            static_cast<unsigned long long>(r.tick),static_cast<unsigned long long>(r.compose),static_cast<unsigned long long>(r.qpc),
            static_cast<unsigned long long>(r.qpc_frequency),
            static_cast<unsigned long long>(r.game_start),static_cast<unsigned long long>(r.native_started_qpc),
            unsigned(r.source_available),unsigned(r.key_available),unsigned(r.origin_initialized),unsigned(r.origin_poisoned),
            static_cast<unsigned long long>(r.key_bits),
            static_cast<unsigned long long>(r.old_source.world),static_cast<unsigned long long>(r.current_source.world),
            static_cast<unsigned long long>(r.old_source.stream),static_cast<unsigned long long>(r.current_source.stream),
            static_cast<unsigned long long>(r.old_source.observed_reference),static_cast<unsigned long long>(r.current_source.observed_reference),
            static_cast<unsigned long long>(r.old_source.head),static_cast<unsigned long long>(r.current_source.head),
            static_cast<unsigned long long>(r.old_source.xr_session),static_cast<unsigned long long>(r.current_source.xr_session),
            static_cast<unsigned long long>(r.old_source.space),static_cast<unsigned long long>(r.current_source.space),
            static_cast<unsigned long long>(r.old_source.reference),static_cast<unsigned long long>(r.current_source.reference),
            static_cast<unsigned long long>(r.old_source.native_revision),static_cast<unsigned long long>(r.current_source.native_revision),
            static_cast<unsigned long long>(r.old_source.native_epoch),static_cast<unsigned long long>(r.current_source.native_epoch),
            r.old_source.units,r.current_source.units,
            r.old_source.native_flags,r.current_source.native_flags);
    }else fprintf(f,"body_goal_first_state=%u ",state);
}
#else
bool configure(wchar_t const* ini,FILE*){return GetPrivateProfileIntW(L"BodyC0GoalPolicy",L"enabled",0,ini)==0;}
void pose_begin(Entry const&) noexcept {}void pose_end(Entry const& e,bool returned,bool called) noexcept {n::pose_end(e,returned,called);}
bool read_view(gears_anchor::Receipt const*,gears_vr::StereoResult const&,double,gears_vr::CoverViewResult&) noexcept {return false;}
void request_drain(){}unsigned pending_count(){return 0;}void summary(FILE*){}
#endif
}
