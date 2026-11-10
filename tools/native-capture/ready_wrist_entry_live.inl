// Included inside full_body_c0_native_live, after State and the old observer.
// No game writer/solver/guard is replaced. Default OFF has no new sampling.
#include "cover_query_source_cause.h"
namespace ready_wrist {
namespace cause=gears_cover_query_source_cause;
namespace wire=::gears_ready_wrist_entry;
wire::Recorder recorder;
std::atomic<bool> attempted{},written{};std::atomic<unsigned> error{};
void begin_unknown(Entry const& e) noexcept {
    if(!recorder.enabled||!recorder.window)return;
    wire::Key key{};auto const& owner=hand::slot.data.owner.key;auto const& n=hand::slot.key.request.native;
    key.entry=reinterpret_cast<uintptr_t>(&e);key.pid=n.game_pid;key.game_start=n.game_start;key.native_started=n.native_started_qpc;
    key.thread=e.thread;key.tick=hand::slot.tick;key.compose=e.invocation;key.mesh=owner.mesh.address;
    auto* broker=weapon_hand_runtime::current();key.cookie=broker?broker->cookie():0;
    if(recorder.begin(key,{}, {},0,0,false))recorder.current->key.begin_qpc=uint64_t(phase::qpc());
}
wire::Record* current(Entry const& e) noexcept {
    if(!recorder.enabled||!recorder.exact(reinterpret_cast<uintptr_t>(&e),e.invocation,e.thread)||
       GetCurrentThreadId()!=e.thread)return nullptr;
    return recorder.current;
}
void source_rejected(Entry const* e,tx::Stage stage,tx::Verdict verdict,unsigned causes) noexcept {
    if(e&&cause::armed())cause::note({reinterpret_cast<uintptr_t>(e),e->invocation,unsigned(e->thread),unsigned(stage)},unsigned(verdict),causes);
    if(!recorder.enabled||!e||stage!=tx::Stage::commit)return;
    if(current(*e))recorder.note_commit_source(reinterpret_cast<uintptr_t>(e),e->invocation,e->thread,unsigned(verdict),causes);
}
void begin(Entry const& e,bool copied) noexcept {
    if(!recorder.enabled)return;
    auto const& c=state.context;auto const& s=c.source;wire::Key key{};
    key.entry=reinterpret_cast<uintptr_t>(&e);key.pid=s.native.game_pid;key.game_start=s.native.game_start;
    key.native_started=s.native.native_started_qpc;key.thread=e.thread;key.tick=c.tick;key.compose=e.invocation;
    key.mesh=c.owner.mesh.address;key.publication=s.publication_sequence;key.scene=s.scene_invocation;
    auto* broker=weapon_hand_runtime::current();key.cookie=broker?broker->cookie():0;
    auto const& input=c.body_source.observed;auto const& h=input.controllers.hands[1];
    if(copied){key.input_session=input.stereo.session;key.stereo_sequence=input.stereo.sequence;
        key.controller_sequence=input.controllers.sequence;key.space_epoch=input.stereo.space_epoch;
        key.profile_generation=input.controllers.profile_generation;key.input_epoch=input.controllers.input_epoch;
        key.reference_revision=input.reference_revision;key.publisher_pid=input.publisher_pid;key.publisher_start=input.publisher_start;
        key.stereo_received_ns=uint64_t(input.stereo.received_ns);key.controllers_received_ns=uint64_t(input.controllers.received_ns);
        key.sync_ns=uint64_t(input.controllers.sync_ns);key.tracking_begin_ns=uint64_t(input.begin_ns);key.tracking_end_ns=uint64_t(input.end_ns);
        key.input_source=uint64_t(input.stereo.source);key.grip_flags=h.grip_flags;key.grip_active=h.grip_active;}
    const bool fixture=copied&&input.parent_cookie==key.cookie&&key.cookie&&
        input.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated&&input.controllers.source==input.stereo.source&&
        input.stereo.head_flags==15&&input.stereo.view_flags==15&&h.grip_active==1&&h.grip_flags==15;
    const bool reentry=recorder.saw_reentry&&fixture&&recorder.select_saw_reentry(key,c.actor.weapon.address,c.actor.special_move,
        c.actor.special_move==33&&binding::current_lancer_animation(c.actor));
    if(recorder.begin(key,input.stereo.head,h.grip,h.squeeze,h.trigger,fixture,true,reentry)){
        auto& row=*recorder.current;if(!row.key.begin_qpc)row.key.begin_qpc=uint64_t(phase::qpc());
        if(row.valid&wire::Valid::source){row.xr_reference=input.parent_scene.reference;
            row.native_reference_epoch=input.native_reference.epoch;row.native_reference_flags=input.native_reference.flags;}}
}
void native_result(Entry const& e,held_outcome::NativeBegin why) noexcept {
    if(auto* row=current(e))row->native_begin=uint64_t(why);
}
void captured(Entry const& e,native::CaptureObservation const& observation,bool okay) noexcept {
    if(auto* row=current(e)){row->key.capture_ns=uint64_t(state.context.basis_ns);
        row->capture_reason=observation.reason;row->capture_context_verdict=observation.context_verdict;
        if(okay){row->valid|=wire::Valid::capture;row->capture_epoch=state.backend.installation_epoch();
            row->avatar_asset=state.context.asset.address;row->units_per_metre=state.source.units_per_metre;
            std::memcpy(&row->capture_component,&state.source.component_to_world,64);}}
}
void body_plan(Entry const& e,unsigned status) noexcept {if(auto* row=current(e))row->body_plan_status=status;}
void mount(Entry const& e,unsigned kind) noexcept {if(auto* row=current(e))row->actual_mount=kind;}
void target(Entry const& e,hands::Result const& result,gears_vr::StereoMatrix const* view) noexcept {
    if(auto* row=current(e)){row->hands_status=unsigned(result.status);
        if(result.status!=hands::Status::okay)return;
        static_assert(sizeof(row->target_wrist_world)==sizeof(state.hand_work.computed_wrist[1]));
        std::memcpy(&row->target_wrist_world,&state.hand_work.computed_wrist[1],sizeof(row->target_wrist_world));
        row->valid|=wire::Valid::target;
        for(unsigned i=0;i<3;++i){row->before[i]=state.source.local[wire::bones[i]];row->planned_local[i]=state.output.local[wire::bones[i]];}
        row->valid|=wire::Valid::planned;
        if(view){std::memcpy(&row->base_view,view,64);row->valid|=wire::Valid::basis;}}
}
void observe(Entry const& e,wire::Stage& stage,bool space) noexcept {
    stage.begin_qpc=uint64_t(phase::qpc());
    const auto done=[&](wire::ReadStatus status){stage.status=uint64_t(status);stage.end_qpc=uint64_t(phase::qpc());};
    try {
        auto* row=current(e);
        if(!row||!state.captured||state.context.entry!=&e||!same_entry(e)||
           row->key.mesh!=state.context.owner.mesh.address||!state.context.retained_current()){
            done(wire::ReadStatus::state);return;}
        namespace p=gears_cover_pose_phase;p::Request request{};request.owner=state.context.owner;
        auto reader=[](void* to,void const* at,size_t bytes){return native::read_memory(to,reinterpret_cast<uintptr_t>(at),bytes);};
        p::Metadata before{},again{};
        if(!p::metadata(reader,request,before)){done(wire::ReadStatus::metadata);return;}
        // Only pose epoch may advance across the original call. All storage,
        // identity, component, tick and flags still match this exact capture.
        auto comparable=before;comparable.pose_epoch=state.backend.metadata().pose_epoch;
        if(!p::metadata_equal(comparable,state.backend.metadata())){done(wire::ReadStatus::storage);return;}
        for(unsigned i=0;i<3;++i)if(!native::read_memory(&stage.local[i],before.local.data+wire::bones[i]*32,32)){
            done(wire::ReadStatus::local_read);return;}
        if(space&&(!native::read_memory(&stage.space35,before.space.data+35*64,64)||
                   !native::read_memory(&stage.space52,before.space.data+52*64,64))){done(wire::ReadStatus::space_read);return;}
        body::Atom local{};body::Matrix64 matrix{};
        for(unsigned i=0;i<3;++i)if(!native::read_memory(&local,before.local.data+wire::bones[i]*32,32)||
            std::memcmp(&local,&stage.local[i],32)){done(wire::ReadStatus::local_changed);return;}
        if(space&&(!native::read_memory(&matrix,before.space.data+35*64,64)||std::memcmp(&matrix,&stage.space35,64)||
                   !native::read_memory(&matrix,before.space.data+52*64,64)||std::memcmp(&matrix,&stage.space52,64))){
            done(wire::ReadStatus::space_changed);return;}
        if(!p::metadata(reader,request,again)||!p::metadata_equal(before,again)){
            done(wire::ReadStatus::metadata_changed);return;}
        stage.pose_epoch=before.pose_epoch;std::memcpy(&stage.component,&before.component,64);stage.space_valid=uint64_t(space);
        done(wire::ReadStatus::okay);
    }catch(...){done(wire::ReadStatus::exception);}
}
void prepared(Entry const& e,tx::Result const& result) noexcept {
    if(auto* row=current(e)){row->prepare_code=unsigned(result.code);row->prepare_unit=result.unit;}
}
void committed(Entry const& e,tx::Result const& result) noexcept {
    if(auto* row=current(e)){row->commit_code=unsigned(result.code);row->commit_unit=result.unit;
        if(result.code==tx::Code::committed)observe(e,row->commit,false);}
}
void consumer_pre(Entry const& e) noexcept {
    if(auto* row=current(e))observe(e,row->consumer_pre,false);
}
void alpha_event(gears_body_c0_arm_alpha::Diagnostic const& event) noexcept {
    if(!recorder.enabled||!state.context.entry)return;
    auto* row=current(*state.context.entry);
    if(!row||event.compose!=row->key.compose||event.outer_tick!=row->key.tick||event.thread!=row->key.thread)return;
    if(row->alpha_count<wire::alpha_capacity)row->alpha[row->alpha_count++]=event;else ++row->alpha_dropped;
}
void weight_return(Entry const& e,gears_body_c0_arm_alpha::WeightReturn const& input) noexcept {
    auto* row=current(e);
    if(!row||input.compose!=row->key.compose||input.outer_tick!=row->key.tick||input.thread!=row->key.thread||input.mesh!=row->key.mesh)return;
    if(row->weight_count>=wire::weight_capacity){++row->weight_dropped;return;}
    auto& sample=row->weight[row->weight_count++];sample.input=input;
    if(!(input.flags&1u))return; // No normal getter return, sampling remains UNKNOWN.
    observe(e,sample.snapshot,false);
    uint32_t bits{};
    if(native::read_memory(&bits,base+0x17070d4,4)){sample.full_threshold_bits=bits;sample.constant_read_mask|=1u;}
    if(native::read_memory(&bits,base+0x17070d8,4)){sample.zero_threshold_bits=bits;sample.constant_read_mask|=2u;}
}
void raw_return(Entry const& e) noexcept {
    if(auto* row=current(e))if(row->raw_return.status==wire::unknown)observe(e,row->raw_return,true);
}
void returned_end(Entry const& e,bool returned,bool called) noexcept {
    if(auto* row=current(e))if(returned&&called)observe(e,row->returned_end,true);
}
#include "ready_wrist_outer_tick_live.inl"
void closed(Entry const& e,tx::Result const& result) noexcept {
    if(auto* row=current(e)){row->finish_code=unsigned(result.code);
        if(result.code==tx::Code::scope_closed&&!state.kernel.failed()&&!state.kernel.protocol_debt()&&!state.kernel.original_exception())
            row->valid|=wire::Valid::normal_scope_close;
        hold_outer(e);}
}
struct Finish {
    Entry const& entry;bool returned{},called{};
    ~Finish(){if(current(entry))recorder.finish(reinterpret_cast<uintptr_t>(&entry),entry.invocation,entry.thread,called,returned);}
};
void export_records() noexcept {
    if(!recorder.enabled||!stopping.load()||callbacks.load()||obligations.load()||!hand::stopping.load()||
       hand::callbacks.load()||hand::live_frames.load()||recorder.active.load(std::memory_order_acquire)||
       GetCurrentThreadId()==owner_thread.load()||attempted.exchange(true))return;
    wchar_t path[MAX_PATH]{};if(!directory[0]||swprintf_s(path,L"%s\\body-ready-wrist.bin",directory)<=0){error=ERROR_INVALID_NAME;return;}
    LARGE_INTEGER frequency{};if(!QueryPerformanceFrequency(&frequency)||frequency.QuadPart<=0){error=ERROR_INVALID_DATA;return;}
    wire::Header h{};h.count=recorder.saved.load(std::memory_order_acquire);h.pid=GetCurrentProcessId();
    h.native_started=uint64_t(session_started_qpc);h.qpc_frequency=uint64_t(frequency.QuadPart);
    h.selected=recorder.selected;h.dropped_capacity=recorder.dropped_capacity;h.nested=recorder.nested;h.unmatched_end=recorder.unmatched_end;
    h.nonmatching_input=recorder.nonmatching_input;h.unavailable_input=recorder.unavailable_input;h.window_cookie=recorder.window_cookie;
    if(h.count>wire::capacity){error=ERROR_INVALID_DATA;return;}
    HANDLE file=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){error=GetLastError();return;}
    const auto write=[&](void const* bytes,DWORD count){DWORD actual{};const bool okay=WriteFile(file,bytes,count,&actual,nullptr)&&actual==count;
        if(!okay)error=GetLastError();return okay;};
    bool okay=write(&h,sizeof h);if(okay&&h.count)okay=write(recorder.rows.data(),DWORD(h.count*sizeof(wire::Record)));
    if(!CloseHandle(file)){error=GetLastError();okay=false;}written=okay;
}
} // namespace ready_wrist
