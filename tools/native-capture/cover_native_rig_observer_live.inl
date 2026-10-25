// Include after Pose, CCTRL, Runtime and Rig, with native_rig_tracking_frame.h
// and <new> already included globally. Root calls tick_end BEFORE Pose release.
// Read-only first-event diagnostic: no engine, bridge, input or pose writes.
namespace cover_pose_phase_live {struct Scope;}
namespace cover_control_observer_live {struct Row;}
namespace cover_native_rig_observer_live {
namespace phase=cover_pose_phase_live;namespace control_live=cover_control_observer_live;
bool enabled{}; // Startup-only; default OFF.
#if defined(GEARS_COVER_NATIVE_RIG_OBSERVER) && GEARS_COVER_NATIVE_RIG_OBSERVER && defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE && defined(GEARS_COVER_CONTROL_OBSERVER) && GEARS_COVER_CONTROL_OBSERVER
namespace rig=rig_tracking_live;namespace rt=weapon_hand_runtime;namespace core=gears_native_rig_tracking;
constexpr unsigned capacity=4;
enum Flag:unsigned {selected_tick=1,world_lease=2,latch_begun=4,tick_ended=8,original_returned=16,sticky_rejection=32,observation_failed=64,drained=128,no_tick=256};
enum class Failure:unsigned {none,owner,lease,latch,control,end,exception,protocol};
struct Header {
    char magic[8]{};unsigned version{},header_size{},record_size{},count{},capacity{},read_only{},writer{},atomic_verified{};
    core::NativeSessionKey native{};uint64_t outer_tick{},scene{},world_generation{};int64_t scene_qpc{};
    unsigned episode{},flags{};Failure failure{};unsigned reserved{};
};
struct Record {
    uint64_t outer_tick{},compose{},ordinal{},compute_ordinal{},node{};
    int64_t before_qpc{},after_qpc{};uint64_t result_rax{};
    int32_t end_bone{};unsigned kind{},thread{},native_failure{},flags{},reserved{};
    core::CoreSnapshot snapshot{};
};
static_assert(std::is_standard_layout_v<Header>&&std::is_trivially_copyable_v<Header>&&std::is_standard_layout_v<Record>&&std::is_trivially_copyable_v<Record>);
std::mutex mutex;Header header{};std::array<Record,capacity> records{};unsigned record_count{};
std::atomic<bool> selected{},stopping{},failed{},file_attempted{},file_written{};
std::atomic<unsigned> live_frames{},in_flight{};std::atomic<DWORD> file_error{};
std::atomic<uint64_t> ticks{},latch_attempts{},read_ok{},read_reject{},ended{},dropped{};
struct Frame {
    phase::Scope const* address{};phase::Owner owner{};uint64_t invocation{},compose{},ordinal{};unsigned episode{};
    rig::TickKey key{};rig::TickLatch latch{};
    alignas(::Scope) unsigned char lease_storage[sizeof(::Scope)]{};::Scope* lease{};
    bool active{},attempted{},sticky{};
    Frame()=default;Frame(Frame const&)=delete;Frame& operator=(Frame const&)=delete;
};
inline thread_local Frame frame{};
struct Call {Call(){++in_flight;}~Call(){--in_flight;}Call(Call const&)=delete;Call& operator=(Call const&)=delete;};
void reject(Failure reason){failed=true;frame.sticky=true;std::lock_guard<std::mutex> lock(mutex);
    header.flags|=sticky_rejection|observation_failed;if(header.failure==Failure::none)header.failure=reason;}
bool current_owner(phase::Owner const& owner){
    const auto session=rt::session();auto const& b=owner.binding;auto const& k=owner.key;
    return !stopping.load()&&recording.load()&&owner.valid&&owner.thread==GetCurrentThreadId()&&phase::thread.ticks==1&&phase::thread.episode==frame.episode&&
        owner.thread==frame.key.request.native.game_thread&&rig::native_session_current(rt::current(),session)&&
        session.game_pid==GetCurrentProcessId()&&session.game_start==frame.key.request.native.game_start&&
        session.native_started_qpc==frame.key.request.native.native_started_qpc&&session.native_started_qpc==uint64_t(session_started_qpc)&&
        k.session==session.native_started_qpc&&k.world_generation==committed_world_generation.load()&&k.world_generation==b.world_generation&&
        k.scene&&k.scene==b.scene_invocation&&k.scene_qpc>0&&b.context_valid&&
        phase::same(k.world,phase::compact(b.world))&&phase::same(k.pawn,phase::compact(b.pawn))&&phase::same(k.mesh,phase::compact(b.mesh))&&phase::current(owner);
}
void tick_begin_impl(phase::Scope const& scope){
    if(!enabled||stopping.load()||scope.kind!=2)return;
    if(frame.active){reject(Failure::protocol);return;} // Never overwrite an active latch, including nested/NPC ticks.
    if(!scope.started||!scope.tick||!scope.matching||!scope.episode||!scope.invocation)return;
    bool expected=false;if(!selected.compare_exchange_strong(expected,true))return;
    frame.active=true;frame.address=&scope;frame.owner=scope.owner;frame.invocation=scope.invocation;frame.episode=scope.episode;++live_frames;++ticks;
    const auto session=rt::session();auto& request=frame.key.request;frame.key.outer_tick=scope.invocation;
    request.native={session.game_pid,scope.owner.thread,session.game_start,session.native_started_qpc};
    request.world=rig::object(scope.owner.binding.world);request.pawn=rig::object(scope.owner.binding.pawn);request.mesh=rig::object(scope.owner.binding.mesh);
    request.world_generation=scope.owner.key.world_generation;request.scene_invocation=scope.owner.key.scene;request.scene_qpc=scope.owner.key.scene_qpc;
    {std::lock_guard<std::mutex> lock(mutex);header.native=request.native;header.outer_tick=scope.invocation;header.scene=request.scene_invocation;
        header.scene_qpc=request.scene_qpc;header.world_generation=request.world_generation;header.episode=scope.episode;header.flags=selected_tick;}
    frame.lease=::new(static_cast<void*>(frame.lease_storage)) ::Scope;
    if(!frame.lease->world()){reject(Failure::lease);return;}
    {std::lock_guard<std::mutex> lock(mutex);header.flags|=world_lease;}
    if(!current_owner(scope.owner)){reject(Failure::owner);return;}
    frame.attempted=true;++latch_attempts;
    if(!rig::begin_tick(frame.key,frame.latch)||!current_owner(scope.owner)){reject(Failure::latch);return;}
    std::lock_guard<std::mutex> lock(mutex);header.flags|=latch_begun;
}
void tick_begin(phase::Scope const& scope) noexcept {Call call;try{tick_begin_impl(scope);}catch(...){try{reject(Failure::exception);}catch(...){failed=true;frame.sticky=true;}}}
void control_impl(control_live::Row const& row){
    if(!enabled||!frame.active||row.kind>1)return;
    if(frame.sticky||stopping.load()){++read_reject;return;}
    const auto valid=control_live::thread.depth==1&&!control_live::thread.overflow;
    auto const* compose=valid?&control_live::thread.frames[0]:nullptr;
    if(!compose||!compose->admitted||compose->nested||!control_live::current(*compose)||
        compose->scope.kind!=0||!compose->scope.arguments||compose->scope.invocation!=row.compose||compose->outer_tick!=frame.invocation||
        compose->scope.episode!=frame.episode||!phase::same(compose->scope.owner.key,frame.owner.key)||
        row.failure!=control_live::Failure::okay||!row.before_valid||!row.after_valid||!row.returned||row.exception||row.nested||row.concurrent||
        row.compose_exception||!row.compose||!row.ordinal||row.ordinal<=frame.ordinal||row.compute_ordinal!=row.ordinal||
        (frame.compose&&frame.compose!=row.compose)||row.episode!=frame.episode||row.outer_tick!=frame.invocation||
        row.thread!=GetCurrentThreadId()||row.thread!=frame.owner.thread||row.mesh_argument!=frame.owner.key.mesh.address||
        row.node_argument!=row.before.node.address||row.node_argument!=row.after.node.address||!phase::same(row.before.node,row.after.node)||
        !phase::same(row.owner,frame.owner.key)||!current_owner(frame.owner)){
        ++read_reject;reject(Failure::control);return;}
    {std::lock_guard<std::mutex> lock(mutex);if(record_count==capacity){++dropped;return;}}
    frame.compose=row.compose;frame.ordinal=row.ordinal;Record record{};
    record.outer_tick=row.outer_tick;record.compose=row.compose;record.ordinal=row.ordinal;record.compute_ordinal=row.compute_ordinal;
    record.node=row.node_argument;record.before_qpc=row.before_qpc;record.after_qpc=row.after_qpc;record.result_rax=row.result_rax;
    record.end_bone=static_cast<int32_t>(static_cast<uint32_t>(row.raw_rdx));record.kind=row.kind;record.thread=row.thread;record.native_failure=unsigned(row.failure);
    // Compose has not returned here. Only the OUTER TICK outcome is recorded at
    // tick_end; these records do not certify a later Compose outcome.
    const bool okay=frame.attempted&&rig::read_tick(frame.key,frame.latch,record.snapshot)&&current_owner(frame.owner);
    if(okay){record.flags=1;++read_ok;}else{record.snapshot={};++read_reject;reject(Failure::latch);}
#if defined(GEARS_HAND_GOAL_OBSERVER) && GEARS_HAND_GOAL_OBSERVER
    if(okay)hand_goal_observer_live::observe(row,record.snapshot);
#endif
    std::lock_guard<std::mutex> lock(mutex);records[record_count++]=record;
}
void control(control_live::Row const& row) noexcept {Call call;try{control_impl(row);}catch(...){try{reject(Failure::exception);}catch(...){failed=true;frame.sticky=true;}}}
void tick_end_impl(phase::Scope const& scope,bool returned){
    if(!enabled||scope.kind!=2||!frame.active)return;
    if(frame.address!=&scope||scope.invocation!=frame.invocation||scope.episode!=frame.episode||
        !phase::same(scope.owner.key,frame.owner.key)||GetCurrentThreadId()!=frame.owner.thread||phase::thread.ticks!=1){reject(Failure::end);return;}
    if(!returned)reject(Failure::exception);
    if(frame.attempted&&!rig::end_tick(frame.key,frame.latch)){reject(Failure::end);return;} // Unknown latch responsibility retains the world lease.
    if(frame.lease){frame.lease->~Scope();frame.lease=nullptr;}
    {std::lock_guard<std::mutex> lock(mutex);header.flags|=tick_ended;if(returned)header.flags|=original_returned;}
    frame.active=false;--live_frames;++ended;
}
void tick_end(phase::Scope const& scope,bool returned) noexcept {Call call;try{tick_end_impl(scope,returned);}catch(...){try{reject(Failure::exception);}catch(...){failed=true;frame.sticky=true;}}}
bool configure(wchar_t const* ini,FILE* file){wchar_t value[16]{};GetPrivateProfileStringW(L"CoverNativeRigObserver",L"enabled",L"0",value,16,ini);
    if(wcscmp(value,L"0")&&wcscmp(value,L"1"))return false;
    if(!wcscmp(value,L"1")){if(enabled||stopping.load()||!phase::enabled||!control_live::enabled||!rig::enabled||!rt::enabled||!continuous_session||!stereo_xr||!anchor_observer_enabled)return false;enabled=true;}
    if(file)fprintf(file,"cover_native_rig_enabled=%u read_only=1 writer=0 capacity=4 first_event_only=1\n",unsigned(enabled));return true;
}
void request_drain(){stopping=true;}
unsigned pending_count(){return live_frames.load()+in_flight.load();}
void summary(FILE* file){if(file){unsigned count{};{std::lock_guard<std::mutex> lock(mutex);count=record_count;}
    fprintf(file,"cover_native_rig_enabled=%u cover_native_rig_ticks=%llu cover_native_rig_latch_attempts=%llu cover_native_rig_read_ok=%llu cover_native_rig_read_reject=%llu cover_native_rig_records=%u cover_native_rig_ended=%llu cover_native_rig_failed=%u cover_native_rig_pending=%u cover_native_rig_dropped=%llu cover_native_rig_file_written=%u cover_native_rig_file_error=%lu ",
        unsigned(enabled),static_cast<unsigned long long>(ticks.load()),static_cast<unsigned long long>(latch_attempts.load()),static_cast<unsigned long long>(read_ok.load()),static_cast<unsigned long long>(read_reject.load()),count,
        static_cast<unsigned long long>(ended.load()),unsigned(failed.load()),pending_count(),static_cast<unsigned long long>(dropped.load()),unsigned(file_written.load()),file_error.load());}}
void print(FILE* file,bool disabled){
    if(enabled&&disabled&&stopping.load()&&!pending_count()&&!file_attempted.exchange(true)){
        wchar_t path[MAX_PATH]{};std::lock_guard<std::mutex> lock(mutex);memcpy(header.magic,"CRIG0001",8);header.version=1;header.header_size=sizeof(Header);
        header.record_size=sizeof(Record);header.count=record_count;header.capacity=capacity;header.read_only=1;header.flags|=drained;
        if(!selected.load()){header.flags|=no_tick;const auto session=rt::session();header.native={session.game_pid,0,session.game_start,session.native_started_qpc};}
        if(directory[0]&&swprintf_s(path,L"%s\\core-tick-observation.bin",directory)>0){
            HANDLE out=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
            if(out==INVALID_HANDLE_VALUE)file_error=GetLastError();else{DWORD written{};bool okay=WriteFile(out,&header,sizeof header,&written,nullptr)&&written==sizeof header;
                if(!okay)file_error=GetLastError()?GetLastError():ERROR_WRITE_FAULT;const auto bytes=DWORD(record_count*sizeof(Record));
                if(okay&&bytes){okay=WriteFile(out,records.data(),bytes,&written,nullptr)&&written==bytes;if(!okay)file_error=GetLastError()?GetLastError():ERROR_WRITE_FAULT;}
                if(!CloseHandle(out)){file_error=GetLastError();okay=false;}file_written=okay;}
        }else file_error=ERROR_INVALID_NAME;
    }
    summary(file);if(file)fprintf(file,"read_only=1 writer=0 atomic_verified=0\n");
}
#else
bool configure(wchar_t const* ini,FILE*){wchar_t value[16]{};GetPrivateProfileStringW(L"CoverNativeRigObserver",L"enabled",L"0",value,16,ini);return !wcscmp(value,L"0");}
void tick_begin(phase::Scope const&) noexcept {}void tick_end(phase::Scope const&,bool) noexcept {}void control(control_live::Row const&) noexcept {}
void request_drain(){}unsigned pending_count(){return 0;}void summary(FILE* file){if(file)fprintf(file,"cover_native_rig_enabled=0 cover_native_rig_compiled=0 ");}void print(FILE* file,bool){summary(file);}
#endif
} // namespace cover_native_rig_observer_live
