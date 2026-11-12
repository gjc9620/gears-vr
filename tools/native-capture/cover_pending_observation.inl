// Included after PendingFrame and the pure JSON formatter, inside Native.
// Bounded GT observations become immutable before any collector can read them.
// No source, write permit, retirement receipt, or recovery state is changed.
namespace cover_pending_observation {
constexpr int64_t sample_gap_ns=20000000;
struct Record {
    uint64_t serial{};unsigned count{};
    std::array<uint64_t,32> attempted_compose{};
    std::array<PendingFrame,32> frames{};
} first;
std::atomic<bool> ready{},export_attempted{},export_written{};
std::atomic<unsigned> export_error{};
void capture(PendingObservation const& pending,uint64_t attempted) noexcept {
    if(!diagnostic_first_failure||ready.load(std::memory_order_acquire)||GetCurrentThreadId()!=owner_thread.load()||
       !attempted||!pending.fields||!(state.context.source.flags&gears_rig_tracking::cover_key_known)||
       !state.context.source.cover_type||!state.context.source.cover_link.address||state.context.source.cover_slot<0)return;
    const auto serial=state.kernel.serial();const auto stamp=rig_tracking_live::clock_ns();
    if(!serial||stamp<=0)return;
    if(first.serial!=serial){first={};first.serial=serial;}
    if(first.count&&stamp-first.frames[first.count-1].at_ns<sample_gap_ns)return;
    auto& row=first.frames[first.count];row.at_ns=stamp;row.observed=pending;
    row.qpc=uint64_t(phase::qpc());row.branch_count=fresh_pose_producer_live::pending_branches(pending.mesh,row.branches);
    first.attempted_compose[first.count]=attempted;++first.count;
    if(first.count==first.frames.size())ready.store(true,std::memory_order_release);
}
void export_record() noexcept {
    if(!ready.load(std::memory_order_acquire)||GetCurrentThreadId()==owner_thread.load()||export_attempted.exchange(true))return;
    wchar_t path[MAX_PATH]{};
    if(!directory[0]||swprintf_s(path,L"%s\\cover-pending-observation.json",directory)<=0){export_error=ERROR_INVALID_NAME;return;}
    if(GetFileAttributesW(path)!=INVALID_FILE_ATTRIBUTES){export_error=ERROR_FILE_EXISTS;return;}
    FILE* out{};if(_wfopen_s(&out,path,L"wb")||!out){export_error=ERROR_OPEN_FAILED;return;}
    fprintf(out,"{\"schema\":1,\"serial\":%llu,\"sample_count\":%u,\"attempted_compose\":[",first.serial,first.count);
    for(unsigned i=0;i<first.count;++i)fprintf(out,"%s%llu",i?",":"",first.attempted_compose[i]);
    fputs("],\"last\":",out);write_pending_json(out,first.frames[first.count-1].observed,first.frames.data(),first.count);
    fputs("}\n",out);const bool okay=ferror(out)==0;const bool closed=fclose(out)==0;
    if(!okay||!closed)export_error=ERROR_WRITE_FAULT;else export_written=true;
}
void summary(FILE* file) noexcept {
    const bool published=ready.load(std::memory_order_acquire);
    if(file)fprintf(file,"cover_pending_ready=%u cover_pending_serial=%llu cover_pending_samples=%u cover_pending_export_written=%u cover_pending_export_error=%u ",
        unsigned(published),published?first.serial:0,published?first.count:0,unsigned(export_written.load()),export_error.load());
}
}
