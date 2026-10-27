// Included inside CCTRL after post_weight_scale. No native output writes.
namespace future_g {
bool enabled{};void* target{};void* original{};
bool own_entry(); // Root's sealed entry manifest, exact getter original prefix.
#if defined(GEARS_COVER_FUTURE_G_OBSERVER) && GEARS_COVER_FUTURE_G_OBSERVER
namespace cert=gears_cover_future_g_certificate;
constexpr unsigned capacity=4;
enum class Reject:unsigned {waiting,observed,source,entry,identity,changed,original_exception,stopped,duplicate,missing,compose_failed,protocol,nonfinite};
// Deliberately excludes the parameter block: no additional +AC read. The old
// post-weight Record remains an exact prior sample; only original XMM0 is g.
struct CurrentState {pose::Identity node{},root{};uintptr_t tree{},vtable{};pose::Metadata metadata{};std::array<Method,5> methods{};};
struct Record {
    post_weight_scale::Record post{};
    pose::CallPacket input{},output{};CurrentState before{},after{};
    uint64_t serial{};uintptr_t caller{},entry_rsp{};LONGLONG begin_qpc{},end_qpc{};
    unsigned post_index{},thread{},frame{};uint32_t g_bits{};float g{};Reject reject{Reject::waiting};
    bool entered{},returned{},exception{},before_valid{},after_valid{},entry_before{},entry_after{},caller_before{},caller_after{},
        packet_unchanged{},g_finite{},g_is_one{},closed{},compose_returned{},nested{},concurrent{},end_known{};
};
struct Header {char magic[8]{};unsigned version{},header_size{},row_size{},count{},capacity{};uint64_t session{},module_base{},compose{};
    unsigned pid{},read_only{},writer{},global_invariance_proved{},fence_proved{},old_post_row_size{},old_cctrl_row_size{};};
static_assert(std::is_trivially_copyable_v<Record>&&std::is_trivially_copyable_v<Header>);
std::mutex record_mutex;std::array<Record,capacity> records{};unsigned count{};std::atomic<unsigned> ignored{},duplicates{};
std::atomic<unsigned> in_flight{},awaiting_close{},protocol_debt{};std::atomic<uint64_t> calls{},next_serial{};
std::atomic<bool> stopping{},failed{},file_attempted{},file_written{};std::atomic<DWORD> file_error{};
bool certified{},reserved{};std::array<unsigned char,cert::call_size> caller_bytes{};int reserve_status{};
struct Call {Call* previous{};uint64_t serial{};unsigned thread{},slot{capacity};bool linked{};
    Call const* counter_self{};unsigned counter_thread{};bool counter_owned{};};
struct Thread {Call* call{};unsigned depth{};Record work{};};inline thread_local Thread local{};
extern "C" void GearsCoverFutureGHook();
unsigned pending_count(){const uint64_t sum=uint64_t(in_flight.load())+awaiting_close.load()+protocol_debt.load();return sum>UINT_MAX?UINT_MAX:unsigned(sum);}
void note_protocol() noexcept {auto debt=protocol_debt.load();while(debt!=UINT_MAX&&!protocol_debt.compare_exchange_weak(debt,debt+1)){}failed=true;stopping=true;}
// Count ownership is independent from begin/identity/TLS success. Copies retain
// the real address and cannot consume it; duplicate release sees owned=false.
void acquire(Call& call) noexcept {call.counter_self=&call;call.counter_thread=GetCurrentThreadId();call.counter_owned=true;++active;++in_flight;++calls;}
template<class A,class B>bool same_current(A const& a,B const& b){
    if(!phase::same(a.node,b.node)||!phase::same(a.root,b.root)||a.tree!=b.tree||a.vtable!=b.vtable||!same_metadata(a.metadata,b.metadata))return false;
    for(unsigned i=0;i<5;++i)if(a.methods[i].target!=b.methods[i].target||a.methods[i].current_prefix!=b.methods[i].current_prefix)return false;return true;
}
bool current_state(Compose const& frame,uintptr_t address,CurrentState& state){
    if(!current(frame))return false;std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);if(!lock.owns_lock()||!binding::metadata_current())return false;
    gears_anchor::Identity node_id{},root_id{},again_id{};uintptr_t tree_again{},tree_field{};
    if(!pose::add(frame.scope.owner.key.mesh.address,0x230,tree_field)||!read(state.tree,tree_field)||!state.tree||
        !binding::identity(state.tree,root_id,true,true)||!binding::identity(address,node_id,true,true)||node_id.outer!=state.tree||
        !read(state.vtable,address)||!state.vtable)return false;
    pose::Request request{};request.owner=frame.scope.owner.key;auto reader=[](void* out,void const* in,size_t n){return safe_copy(out,in,n);};
    if(!pose::metadata(reader,request,state.metadata)||!pose::header_valid(state.metadata.reference,68)||!pose::header_valid(state.metadata.local,32)||
        !pose::header_valid(state.metadata.space,64)||!pose::header_valid(state.metadata.desired,1,true)||
        state.metadata.reference.count!=state.metadata.local.count||state.metadata.reference.count!=state.metadata.space.count||!pose::finite(state.metadata.component))return false;
    constexpr unsigned slots[]={0x230,0x238,0x240,0x248,0x278};
    for(unsigned i=0;i<5;++i){uintptr_t slot{};if(!pose::add(state.vtable,slots[i],slot)||!read(state.methods[i].target,slot)||!pose::span(state.methods[i].target,32)||
        !safe_copy(state.methods[i].current_prefix.data(),reinterpret_cast<void const*>(state.methods[i].target),32))return false;}
    if(!binding::identity(address,again_id,true,true)||!binding::same(node_id,again_id)||!read(tree_again,tree_field)||tree_again!=state.tree||
        !binding::identity(state.tree,again_id,true,true)||!binding::same(root_id,again_id)||!binding::metadata_current())return false;
    state.node=phase::compact(node_id);state.root=phase::compact(root_id);return true;
}
bool caller_current(){std::array<unsigned char,cert::call_size> now{};return certified&&
    safe_copy(now.data(),reinterpret_cast<void const*>(base+cert::call_rva),now.size())&&now==caller_bytes;}
bool frame_current(Record const& r){
    const auto& p=r.post;if(thread.depth!=1||thread.overflow||thread.call||r.frame!=0||local.depth!=1)return false;
    const auto& f=thread.frames[0];return f.admitted&&!f.nested&&f.last_valid&&current(f)&&
        reinterpret_cast<uintptr_t>(f.address)==p.scope_address&&phase::same(f.scope.owner.key,p.scope_owner)&&
        f.scope.invocation==p.scope_compose&&f.outer_tick==p.scope_outer_tick&&f.scope.episode==p.scope_episode&&
        f.scope.owner.thread==GetCurrentThreadId()&&r.thread==GetCurrentThreadId()&&
        f.ordinal==p.weight.ordinal&&f.last_ordinal==p.compute.ordinal&&f.last_row==p.weight.linked_row&&
        f.last_node==p.compute.node_argument&&phase::same(f.last_identity,p.compute.after.node);
}
void arm(cover_control_observer_live::Call const& call) noexcept {
    if(!enabled||stopping.load()||call.row.kind!=2||!call.index)return;
    try{auto& work=local.work;bool found{};
        {std::lock_guard<std::mutex> lock(post_weight_scale::record_mutex);for(unsigned i=0;i<post_weight_scale::count;++i){const auto& p=post_weight_scale::records[i];
            if(p.call_index==call.index&&p.weight.ordinal==call.row.ordinal&&p.scope_compose==call.row.compose){work={};work.post=p;work.post_index=i+1;found=true;break;}}}
        if(!found||work.post.reject!=post_weight_scale::Reject::okay||!work.post.scope_open||!work.post.packet_unchanged||
            work.post.compose_closed||work.post.before.read_mask!=127||work.post.after.read_mask!=127)return;
        std::lock_guard<std::mutex> lock(record_mutex);for(unsigned i=0;i<count;++i)if(records[i].post_index==work.post_index){failed=true;return;}
        if(count==capacity){failed=true;return;}records[count++]=work;++awaiting_close;
    }catch(...){note_protocol();}
}
void begin(pose::CallPacket const& packet,uintptr_t ra,uintptr_t rsp,Call& call) {
    call.previous=local.call;call.serial=++next_serial;call.thread=GetCurrentThreadId();call.linked=true;local.call=&call;++local.depth;
    if(!enabled||stopping.load()||ra!=base+cert::return_rva||local.depth!=1||thread.depth!=1||thread.overflow||thread.call){++ignored;return;}
    auto& work=local.work;unsigned selected=capacity;
    {std::lock_guard<std::mutex> lock(record_mutex);for(unsigned i=0;i<count;++i){const auto& p=records[i].post;
        if(records[i].closed||p.scope_compose!=thread.frames[0].scope.invocation||p.scope_thread!=call.thread||
            p.compute.node_argument!=packet.gpr[1]||p.compute.raw_rdx!=packet.gpr[2]||p.compute.mesh_argument!=packet.gpr[3]||
            p.weight.ordinal!=thread.frames[0].ordinal||p.weight.entry_rsp!=rsp)continue;
        if(records[i].entered){records[i].reject=Reject::duplicate;++duplicates;failed=true;return;}
        selected=i;work=records[i];break;}}
    if(selected==capacity){++ignored;return;}
    work.serial=call.serial;work.thread=call.thread;work.frame=0;work.caller=ra;work.entry_rsp=rsp;work.input=packet;work.begin_qpc=phase::qpc();work.entered=true;
    if(!frame_current(work)||(rsp&15)!=8||!stack_range(rsp,8))work.reject=Reject::source;
    else {uintptr_t actual_ra{};work.entry_before=own_entry();work.caller_before=caller_current();
        if(!read(actual_ra,rsp)||actual_ra!=ra||!work.entry_before||!work.caller_before)work.reject=Reject::entry;
        else {work.before_valid=current_state(thread.frames[0],packet.gpr[1],work.before);
            if(!work.before_valid||!same_current(work.post.node_after,work.before)||
                !pose::metadata_equal(work.post.node_after.metadata,work.before.metadata)||
                work.before.methods[3].target!=reinterpret_cast<uintptr_t>(target))work.reject=Reject::identity;
            else work.reject=Reject::waiting;}}
    {std::lock_guard<std::mutex> lock(record_mutex);records[selected]=work;}call.slot=selected;
}
void finish(Call const& call,pose::CallPacket const& packet,bool returned){
    if(call.slot==capacity)return;auto& work=local.work;
    {std::lock_guard<std::mutex> lock(record_mutex);work=records[call.slot];}
    work.output=packet;work.end_qpc=phase::qpc();work.returned=returned;work.exception=!returned;
    memcpy(&work.g_bits,packet.xmm[0].data(),4);memcpy(&work.g,&work.g_bits,4);work.g_finite=returned&&std::isfinite(work.g);work.g_is_one=returned&&work.g_bits==0x3f800000u;
    if(work.reject==Reject::protocol){} // Preserve an already rejected end.
    else if(!returned)work.reject=Reject::original_exception;
    else if(stopping.load())work.reject=Reject::stopped;
    else if(work.reject==Reject::waiting){work.entry_after=own_entry();work.caller_after=caller_current();
        if(!frame_current(work)||!work.entry_after||!work.caller_after)work.reject=Reject::entry;
        else {work.after_valid=current_state(thread.frames[0],work.input.gpr[1],work.after);
            if(!work.after_valid||!same_current(work.before,work.after)||!pose::metadata_equal(work.before.metadata,work.after.metadata))work.reject=Reject::identity;
            else work.reject=work.g_finite?Reject::observed:Reject::nonfinite;}}
    work.packet_unchanged=!memcmp(&packet,&work.output,sizeof(packet));
    {std::lock_guard<std::mutex> lock(record_mutex);if(records[call.slot].serial!=call.serial||records[call.slot].closed){note_protocol();return;}records[call.slot]=work;}
}
void release(Call& call) noexcept {
    const bool counted=call.counter_self==&call&&call.counter_owned&&call.counter_thread==GetCurrentThreadId();
    const bool known=counted&&call.linked&&call.serial&&call.thread==GetCurrentThreadId()&&local.call==&call&&local.depth;
    if(known){local.call=call.previous;--local.depth;}else note_protocol();
    if(counted)call.counter_owned=false;
    try{if(call.slot<capacity){std::lock_guard<std::mutex> lock(record_mutex);auto& r=records[call.slot];
        if(r.serial==call.serial){r.end_known=known;if(!known)r.reject=Reject::protocol;}
        else note_protocol();}}catch(...){note_protocol();}
    if(counted){--in_flight;--active;} // Unknown linkage retains debt, but only the exact owner consumes callback counts.
}
void begin_guarded(pose::CallPacket const& p,uintptr_t ra,uintptr_t rsp,Call& call) noexcept {try{begin(p,ra,rsp,call);}catch(...){note_protocol();}}
void finish_guarded(Call const& call,pose::CallPacket const& p,bool returned) noexcept {try{finish(call,p,returned);}catch(...){note_protocol();}}
extern "C" __declspec(noinline) void GearsCoverFutureGInvokeProtected(pose::CallPacket* packet,unsigned,uintptr_t ra,uintptr_t rsp) noexcept(false){
    Call call{};bool returned{};acquire(call);
    __try{begin_guarded(*packet,ra,rsp,call);
        // reserve/install must establish the immutable nonnull trampoline.
        // No catch surrounds the original: SEH and C++ exceptions propagate.
        cover_body_live::GearsCoverRawInvoke(reinterpret_cast<cover_body_live::CallPacket*>(packet),original);returned=true;
    }__finally{__try{finish_guarded(call,*packet,returned&&!AbnormalTermination());}__finally{release(call);}}
}
void on_compose_end(uint64_t invocation,bool returned,bool nested,bool concurrent) noexcept {
    if(!enabled)return;try{std::lock_guard<std::mutex> lock(record_mutex);for(unsigned i=0;i<count;++i){auto& r=records[i];
        if(r.post.scope_compose!=invocation||r.closed)continue;r.closed=true;r.compose_returned=returned;r.nested=nested;r.concurrent=concurrent;
        if(!returned||nested||concurrent)r.reject=Reject::compose_failed;else if(!r.entered)r.reject=Reject::missing;
        --awaiting_close;
    }}catch(...){note_protocol();}
}
bool prepare(){if(!enabled)return true;if(certified)return true;
    if(!phase::digest(cert::getter)||!phase::digest(cert::caller)||
        !safe_copy(caller_bytes.data(),reinterpret_cast<void const*>(base+cert::call_rva),caller_bytes.size())||
        !phase::digest(cert::getter)||!phase::digest(cert::caller))return false;
    certified=true;return true;
}
bool reserve(){if(!enabled)return true;if(reserved||!prepare())return false;target=reinterpret_cast<void*>(base+cert::getter.rva);
    std::array<unsigned char,9> before{},after{};if(!safe_copy(before.data(),target,9)||before!=cert::prefix)return false;
    reserve_status=int(MH_CreateHook(target,reinterpret_cast<void*>(GearsCoverFutureGHook),&original));
    if(reserve_status!=int(MH_OK)||!original)return false;reserved=true;
    return safe_copy(after.data(),target,9)&&after==before;
}
bool reserved_target(void* p){return reserved&&p==target;}
bool configure(wchar_t const* ini,FILE* file){wchar_t value[16]{};GetPrivateProfileStringW(L"CoverFutureGObserver",L"enabled",L"0",value,16,ini);
    if(wcscmp(value,L"0")&&wcscmp(value,L"1"))return false;if(!wcscmp(value,L"1")){if(enabled||!cover_control_observer_live::enabled||!post_weight_scale::enabled)return false;enabled=true;}
    if(file)fprintf(file,"future_g_compiled=1 future_g_enabled=%u writer=0 global_invariance_proved=0\n",unsigned(enabled));return true;}
void request_drain(){stopping=true;}
void summary(FILE* file){if(file)fprintf(file,"future_g_enabled=%u future_g_pending=%u future_g_inflight=%u future_g_awaiting_close=%u future_g_protocol_debt=%u future_g_calls=%llu future_g_failed=%u future_g_file_written=%u ",unsigned(enabled),pending_count(),in_flight.load(),awaiting_close.load(),protocol_debt.load(),calls.load(),unsigned(failed.load()),unsigned(file_written.load()));}
void print(FILE* file,bool disabled){if(enabled&&disabled&&stopping.load()&&!pending_count()&&!cover_control_observer_live::in_flight.load()&&!compose_in_flight.load()&&!file_attempted.exchange(true)){
    Header h{};memcpy(h.magic,"CFUTG001",8);h.version=1;h.header_size=sizeof h;h.row_size=sizeof(Record);h.count=count;h.capacity=capacity;h.session=selected_session;h.module_base=base;h.compose=selected_compose;
    h.pid=GetCurrentProcessId();h.read_only=1;h.old_post_row_size=sizeof(post_weight_scale::Record);h.old_cctrl_row_size=sizeof(Row);
    wchar_t path[MAX_PATH]{};if(directory[0]&&swprintf_s(path,L"%s\\control-future-g.bin",directory)>0){HANDLE out=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(out!=INVALID_HANDLE_VALUE){DWORD bytes{};bool okay=WriteFile(out,&h,sizeof h,&bytes,nullptr)&&bytes==sizeof h;const auto n=DWORD(count*sizeof(Record));if(okay&&n)okay=WriteFile(out,records.data(),n,&bytes,nullptr)&&bytes==n;
            if(!okay)file_error=GetLastError();if(!CloseHandle(out)){file_error=GetLastError();okay=false;}file_written=okay;}else file_error=GetLastError();}else file_error=ERROR_INVALID_NAME;
    }summary(file);}
#else
bool configure(wchar_t const* ini,FILE*){wchar_t value[16]{};GetPrivateProfileStringW(L"CoverFutureGObserver",L"enabled",L"0",value,16,ini);return !wcscmp(value,L"0");}
bool reserve(){return true;}bool reserved_target(void*){return false;}unsigned pending_count(){return 0;}void request_drain(){}
void arm(cover_control_observer_live::Call const&) noexcept {}void on_compose_end(uint64_t,bool,bool,bool) noexcept {}
void summary(FILE* f){if(f)fprintf(f,"future_g_compiled=0 ");}void print(FILE* f,bool){summary(f);}
#endif
} // namespace future_g
