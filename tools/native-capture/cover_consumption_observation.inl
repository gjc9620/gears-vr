// Included inside full_body_c0_native_live after persistent State.
// Immutable first-cover observation, never a writer/consumer/retirement permit.
namespace cover_consumption_observation {
struct Record {
    uint64_t tick{},compose{},serial{};int64_t at_ns{},metadata_end_ns{},tracking_received_ns{};
    unsigned finish_code{},read_code{},kernel_failure{},pending{},fields{},protocol{},installation_epoch{};
    bool returned{},original_called{},same_entry{},notified{},consumed{},atom_installed{},journal_known{},source_v3{};
    tx::Stamp stamp{};gears_atom_overwrite::Diagnostic journal{};tx::Mask required{};
    body::Frame source{};body::Output planned{};native::ConsumedPose observed{};
    gears_native_rig_tracking::CoreSnapshot core{};
} first;
std::atomic<bool> ready{},export_attempted{},export_written{};std::atomic<unsigned> export_error{};
void capture(unsigned finish_code,bool returned,bool original_called,bool exact)noexcept {
    if(ready.load(std::memory_order_acquire)||GetCurrentThreadId()!=owner_thread.load())return;
    auto& r=first;r.tick=state.context.tick;r.compose=state.context.copy.invocation;r.serial=state.kernel.serial();
    r.at_ns=rig_tracking_live::clock_ns();r.finish_code=finish_code;r.read_code=unsigned(state.cover_completed_code);
    r.kernel_failure=unsigned(state.kernel.failure());r.pending=state.kernel.pending();r.fields=state.kernel.outstanding_fields();
    r.protocol=state.kernel.protocol_debt();r.installation_epoch=state.backend.installation_epoch();
    r.returned=returned;r.original_called=original_called;r.same_entry=exact;r.notified=state.notified;
    r.consumed=state.kernel.consumed();r.atom_installed=state.atom_installed;r.required=state.kernel.outstanding_mask();
    if(auto* journal=native_atom_overwrite_live::receipts())r.journal_known=journal->diagnostic(r.journal);
    r.source_v3=body_tick_source_live::enabled;
    if(r.source_v3){r.metadata_end_ns=state.context.body_source.metadata_end_ns;r.tracking_received_ns=state.context.body_source.observed.stereo.received_ns;}
    else r.tracking_received_ns=state.context.source.stereo.received_ns;
    r.stamp=state.stamp;r.source=state.source;r.planned=state.output;r.core=state.context.source;
    r.observed=state.backend.unvalidated_consumed_observation();
    ready.store(true,std::memory_order_release);
}
void export_record()noexcept {
    if(!ready.load(std::memory_order_acquire)||GetCurrentThreadId()==owner_thread.load()||export_attempted.exchange(true))return;
    auto write=[](wchar_t const* name,void const* data,size_t bytes){
        wchar_t path[MAX_PATH]{};if(!directory[0]||swprintf_s(path,L"%s\\%s",directory,name)<=0){export_error=ERROR_INVALID_NAME;return false;}
        if(GetFileAttributesW(path)!=INVALID_FILE_ATTRIBUTES){export_error=ERROR_FILE_EXISTS;return false;}
        FILE* out{};if(_wfopen_s(&out,path,L"wb")||!out){export_error=ERROR_OPEN_FAILED;return false;}
        const bool written=fwrite(data,1,bytes,out)==bytes;const bool closed=fclose(out)==0;
        if(!written||!closed)export_error=ERROR_WRITE_FAULT;return written&&closed;
    };
    auto const& r=first;bool ok=write(L"cover-first-consumption-source.bin",&r.source,sizeof r.source);
    ok=write(L"cover-first-consumption-output.bin",&r.planned,sizeof r.planned)&&ok;
    ok=write(L"cover-first-consumption-observed.bin",&r.observed,sizeof r.observed)&&ok;
    ok=write(L"cover-first-consumption-core.bin",&r.core,sizeof r.core)&&ok;
    ok=write(L"cover-first-consumption-stamp.bin",&r.stamp,sizeof r.stamp)&&ok;
    export_written.store(ok,std::memory_order_release);
}
void summary(FILE* f)noexcept {
    export_record();if(!f)return;
    fprintf(f,"cover_consume_observation_ready=%u cover_consume_export_written=%u cover_consume_export_error=%u ",
        unsigned(ready.load(std::memory_order_acquire)),unsigned(export_written.load()),export_error.load());
    if(!ready.load(std::memory_order_acquire))return;auto const& r=first;auto const& j=r.journal;
    fprintf(f,"cover_consume_identity=%llu,%llu,%llu cover_consume_times=%lld,%lld,%lld cover_consume_finish=%u cover_consume_read=%u cover_consume_failure=%u cover_consume_pending=%u cover_consume_fields=%u cover_consume_protocol=%u cover_consume_installation_epoch=%u ",
        r.tick,r.compose,r.serial,r.at_ns,r.metadata_end_ns,r.tracking_received_ns,r.finish_code,r.read_code,r.kernel_failure,r.pending,r.fields,r.protocol,r.installation_epoch);
    fprintf(f,"cover_consume_flags=%u,%u,%u,%u,%u,%u,%u cover_consume_journal=%u,%u,%llu,%llu,%u,%u,%u,%u cover_consume_required=%llu,%llu,%llu,%llu cover_consume_covered=%llu,%llu,%llu,%llu cover_consume_bytes=%zu,%zu,%zu,%zu,%zu ",
        unsigned(r.returned),unsigned(r.original_called),unsigned(r.same_entry),unsigned(r.notified),unsigned(r.consumed),unsigned(r.atom_installed),unsigned(r.source_v3),
        unsigned(r.journal_known),unsigned(j.bound),j.version,j.event,j.installed_epoch,j.coverage_epoch,unsigned(j.active),unsigned(export_written.load()),
        r.required.words[0],r.required.words[1],r.required.words[2],r.required.words[3],j.covered.words[0],j.covered.words[1],j.covered.words[2],j.covered.words[3],
        sizeof r.source,sizeof r.planned,sizeof r.observed,sizeof r.core,sizeof r.stamp);
}
}
