// Include after capture's safe_copy/base/session/world globals and metadata helper.
#include "path_choice_position_trial.h"
#include "path_choice_position_code_contract.h"
#include <intrin.h>
namespace path_choice_position_live {
#if defined(GEARS_PATH_CHOICE_POSITION_TRIAL) && GEARS_PATH_CHOICE_POSITION_TRIAL
namespace trial=gears_path_choice_position;
struct Request {trial::Config config;ULONGLONG expires{};
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
    std::string wire;ULONGLONG first_tick{},accepted_tick{};uint64_t renewals{};
#endif
};
// A previously accepted immutable renewal whose exact owner lease completed
// normally while the worker was arriving/certifying. Never a new admission.
struct CompletedRenewal {uint64_t nonce{};char const* stage{};};
std::shared_ptr<Request const> selected;
std::atomic<uint64_t> loaded_nonce{},applied{},restored{},rejected{},identity_lost{},ownership_lost{},setter_failed{};
std::atomic<bool> pending_restore{},ever_modified{},drain_requested{},unresolved{},terminal{};
std::atomic<ULONGLONG> drain_started{};
std::atomic<DWORD> owner_thread{};std::atomic<unsigned> in_flight{},restore_retries{};
// Admission handshake: 0=idle,1=owner invocation,2=terminal. A timeout
// cannot race an owner entering between its idle check and terminal publish.
std::atomic<unsigned> owner_activity{};
inline constexpr unsigned retry_limit=3;
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
// Worker reads these acknowledgments only; it never inspects owner State.
std::atomic<uint64_t> active_nonce{},consumed_nonce_ack{},last_restored_nonce{},service_renewed{};
std::atomic<ULONGLONG> last_restored_tick{};
std::shared_ptr<Request const> last_request_receipt;
#endif
// First ownership loss only. The owner writes once, then publishes immutable
// fields; worker progress reads them only after an acquire of ready.
struct OwnershipLossReceipt {
    std::array<std::uint32_t,2> original{},owned{},observed{};
    std::uint32_t units{};unsigned read_mask{};bool positions_complete{},units_readable{};
    std::uint64_t scene{},panel{},world{},current_world{},tick{};
};
OwnershipLossReceipt first_loss;
std::atomic<bool> loss_claimed{},loss_ready{};
trial::State state; // Only the bound scene-root owner thread accesses this.
std::uint64_t successful_nonce{}; // Same owner only; a later partial apply is not certified by an older count.
enum class AuthStage:unsigned {hold=1,restore=2,apply=3};
enum class AuthKind:unsigned {check,context,vtable,invalidate,notify,units};
struct AuthFailureReceipt {
    std::uint64_t nonce{},panel{},scene{},world{},current_world{};
    uintptr_t caller_hint{},address{};long long qpc{};DWORD thread{};
    AuthStage stage{};AuthKind kind{};unsigned check_index=UINT_MAX,context_field=UINT_MAX;
    unsigned length{},first_mismatch=UINT_MAX,window_offset{},window_size{};bool readable{};
    std::array<unsigned char,16> expected{},observed{};
};
AuthFailureReceipt first_auth_failure;
std::atomic<bool> auth_failure_claimed{},auth_failure_ready{};
struct RegistryAcceptedReceipt {
    std::uint64_t nonce{},world{},address{};unsigned check_index{},header_index{};
    trial::RegistryHeader initial{},observed{};AuthStage stage{};uintptr_t caller_hint{};long long qpc{};DWORD thread{};
};
std::array<RegistryAcceptedReceipt,2> registry_accepted_receipts{};
std::array<std::atomic<bool>,2> registry_accepted_claimed{},registry_accepted_ready{};
std::array<std::atomic<uint64_t>,2> registry_accepted_by_header{};std::atomic<uint64_t> registry_accepted_reads{};
void observe_registry_accepted(trial::Config const& c,trial::Context const& ctx,AuthStage stage,uintptr_t hint,unsigned check_index,
    trial::Check const& check,unsigned char const* observed){
    if(owner_thread.load(std::memory_order_acquire)!=GetCurrentThreadId())return;
    int index=trial::registry_header_index(c,check.address,check.bytes.size());if(index<0)return;
    const auto header=static_cast<unsigned>(index);++registry_accepted_reads;++registry_accepted_by_header[header];
    bool empty=false;if(!registry_accepted_claimed[header].compare_exchange_strong(empty,true))return;
    RegistryAcceptedReceipt r{};r.nonce=c.nonce;r.world=ctx.world_generation;r.address=check.address;r.check_index=check_index;r.header_index=header;
    r.initial=trial::registry_header(check.bytes.data());r.observed=trial::registry_header(observed);r.stage=stage;r.caller_hint=hint;r.thread=GetCurrentThreadId();
    LARGE_INTEGER now{};QueryPerformanceCounter(&now);r.qpc=now.QuadPart;
    registry_accepted_receipts[header]=r;registry_accepted_ready[header].store(true,std::memory_order_release);
}
bool observe_auth_allowed(trial::Config const& c){
    // Must precede successful_nonce (owner-only) and every State access.
    return owner_thread.load(std::memory_order_acquire)==GetCurrentThreadId()&&applied.load()&&
        c.nonce&&successful_nonce==c.nonce;
}
void publish_auth_failure(trial::Config const& c,trial::Context const& ctx,AuthStage stage,AuthKind kind,
    uintptr_t hint,unsigned check_index,unsigned context_field,uintptr_t address,unsigned length,
    unsigned char const* expected,unsigned char const* observed,bool readable){
    if(!observe_auth_allowed(c))return;
    bool empty=false;if(!auth_failure_claimed.compare_exchange_strong(empty,true))return;
    AuthFailureReceipt r{};r.nonce=c.nonce;r.panel=c.panel;r.scene=c.scene;r.world=c.world_generation;r.current_world=ctx.world_generation;
    r.caller_hint=hint;r.address=address;r.thread=GetCurrentThreadId();r.stage=stage;r.kind=kind;
    r.check_index=check_index;r.context_field=context_field;r.length=length;r.readable=readable;
    if(readable){for(unsigned i=0;i<length;++i)if(expected[i]!=observed[i]){r.first_mismatch=i;break;}}
    r.window_offset=readable&&r.first_mismatch!=UINT_MAX&&r.first_mismatch>8?r.first_mismatch-8:0;
    r.window_size=(std::min)(16u,length-r.window_offset);
    for(unsigned i=0;i<r.window_size;++i){r.expected[i]=expected[r.window_offset+i];
        if(readable)r.observed[i]=observed[r.window_offset+i];}
    LARGE_INTEGER now{};QueryPerformanceCounter(&now);r.qpc=now.QuadPart;
    first_auth_failure=r;auth_failure_ready.store(true,std::memory_order_release);
}
// Uses only already-available context values; it performs no game reads.
void observe_context_failure(trial::Config const& c,trial::Context const& ctx,AuthStage stage,uintptr_t hint){
    if(!observe_auth_allowed(c)||trial::same(c,ctx))return;
    std::array<std::uint64_t,7> expected{c.pid,c.creation_time,c.started_qpc,c.world_generation,c.base,c.client,c.scene};
    std::array<std::uint64_t,7> actual{ctx.pid,ctx.creation_time,ctx.started_qpc,ctx.world_generation,ctx.base,ctx.client,ctx.scene};
    for(unsigned i=0;i<expected.size();++i)if(expected[i]!=actual[i]){
        publish_auth_failure(c,ctx,stage,AuthKind::context,hint,UINT_MAX,i,0,8,
            reinterpret_cast<unsigned char const*>(&expected[i]),reinterpret_cast<unsigned char const*>(&actual[i]),true);return;}
}
template<class Read> struct AuthRead {
    trial::Config const& config;trial::Context const& ctx;AuthStage stage;uintptr_t hint;Read read;unsigned next_check{};
    bool operator()(void* out,void const* from,std::size_t size){
        const bool ok=read(out,from,size); // Forward exactly once, with the original arguments and result.
        if(owner_thread.load(std::memory_order_acquire)!=GetCurrentThreadId())return ok;
        const auto at=reinterpret_cast<uintptr_t>(from);
        auto compare=[&](AuthKind kind,unsigned index,unsigned char const* expected,unsigned length){
            auto observed=static_cast<unsigned char const*>(out);
            if(!ok||!std::equal(expected,expected+length,observed))
                publish_auth_failure(config,ctx,stage,kind,hint,index,UINT_MAX,at,length,expected,observed,ok);
        };
        // authenticate receives its Reader by value. Each authentication starts
        // at check zero, including duplicate address/length checks in the config.
        if(next_check<config.checks.size()){
            auto const& check=config.checks[next_check];
            if(at==check.address&&size==check.bytes.size()){
                const auto index=next_check++;
                const auto match=ok?trial::match_check(config,check,static_cast<unsigned char const*>(out),size):trial::CheckMatch::rejected;
                if(match==trial::CheckMatch::accepted_registry_suffix)
                    observe_registry_accepted(config,ctx,stage,hint,index,check,static_cast<unsigned char const*>(out));
                else if(match==trial::CheckMatch::rejected)
                    publish_auth_failure(config,ctx,stage,AuthKind::check,hint,index,UINT_MAX,at,static_cast<unsigned>(size),check.bytes.data(),static_cast<unsigned char const*>(out),ok);
                return ok;}
        }
        auto scalar=[&](AuthKind kind,auto expected){compare(kind,UINT_MAX,reinterpret_cast<unsigned char const*>(&expected),static_cast<unsigned>(sizeof(expected)));};
        if(size==8&&at==config.panel)scalar(AuthKind::vtable,uintptr_t(config.panel_vtable));
        else if(size==8&&at==config.panel_vtable+0x3d8)scalar(AuthKind::invalidate,uintptr_t(config.base+0x2988d8));
        else if(size==8&&at==config.panel_vtable+0x3b8)scalar(AuthKind::notify,uintptr_t(config.base+0x297338));
        else if(size==4&&at==config.panel+0x70)scalar(AuthKind::units,std::uint32_t(0x04040404));
        return ok;
    }
};
template<class Read> auto auth_reader(trial::Config const& c,trial::Context const& ctx,AuthStage stage,uintptr_t hint,Read read){
    observe_context_failure(c,ctx,stage,hint);return AuthRead<Read>{c,ctx,stage,hint,read};
}
template<class Read> void publish_loss(trial::Config const& c,std::uint64_t current_world,
    std::array<std::uint32_t,2> observed,unsigned read_mask,bool complete,Read read){
    bool empty=false;if(!loss_claimed.compare_exchange_strong(empty,true))return;
    OwnershipLossReceipt receipt;
    receipt.original={c.top_bits,c.bottom_bits};receipt.owned=state.lease.owned;
    receipt.read_mask=read_mask;receipt.positions_complete=complete;
    for(unsigned i=0;i<2;++i)if(read_mask&(1u<<i))receipt.observed[i]=observed[i];
    std::uint32_t units{};receipt.units_readable=read(&units,reinterpret_cast<void const*>(c.panel+0x70),4);
    if(receipt.units_readable)receipt.units=units;
    receipt.scene=c.scene;receipt.panel=c.panel;receipt.world=c.world_generation;
    receipt.current_world=current_world;receipt.tick=GetTickCount64();
    first_loss=receipt;loss_ready.store(true,std::memory_order_release);
}
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
void isolate_ui_failure(){
    unresolved=true;terminal=true;std::atomic_store(&selected,std::shared_ptr<Request const>{});
}
void publish_owner_ack(trial::Result result){
    if(owner_thread.load(std::memory_order_acquire)!=GetCurrentThreadId())return;
    consumed_nonce_ack.store(state.consumed_nonce,std::memory_order_release);
    active_nonce.store(state.lease.active?state.lease.config.nonce:0,std::memory_order_release);
    if(result==trial::Result::restored&&!state.lease.active&&!state.lease.touched[0]&&!state.lease.touched[1]){
        last_restored_tick.store(GetTickCount64(),std::memory_order_relaxed);
        last_restored_nonce.store(state.lease.config.nonce,std::memory_order_release);
    }
}
#endif
ULONGLONG last_poll{};FILETIME seen_file{};thread_local bool inside{};
uint64_t creation_time(){FILETIME c{},e{},k{},u{};if(!GetProcessTimes(GetCurrentProcess(),&c,&e,&k,&u))return 0;
    return (uint64_t(c.dwHighDateTime)<<32)|c.dwLowDateTime;}
trial::Context context(uintptr_t client,uintptr_t scene){return {GetCurrentProcessId(),creation_time(),
    static_cast<uint64_t>(session_started_qpc),committed_world_generation.load(),base,client,scene};}
bool certify_code(){std::array<unsigned char,1024> bytes{},again{};
    for(auto const& span:gears_path_position_code::spans)if(span.size>bytes.size()||base>UINTPTR_MAX-span.rva-span.size||
        !safe_copy(bytes.data(),reinterpret_cast<void const*>(base+span.rva),span.size)||
        !safe_copy(again.data(),reinterpret_cast<void const*>(base+span.rva),span.size)||
        !std::equal(bytes.begin(),bytes.begin()+span.size,again.begin())||
        !gears_native_metadata::matches(bytes.data(),span.size,span.sha256))return false;return true;}
void disable(){std::atomic_store(&selected,std::shared_ptr<Request const>{});}
bool modified(){return ever_modified.load();}
// Call BEFORE closing hook admission. In-flight owner work keeps draining true.
void request_drain(){ULONGLONG empty=0;drain_started.compare_exchange_strong(empty,GetTickCount64());drain_requested=true;disable();}
bool draining(){
    if(owner_activity.load()==1||in_flight.load()!=0)return true;
    if(drain_requested.load()&&!terminal.load()&&pending_restore.load()&&GetTickCount64()-drain_started.load()>=2000){
        // No matching owner entry arrived in the bounded drain window. State
        // remains untouched and pending_restore stays visible in final status.
        unsigned idle=0;if(owner_activity.compare_exchange_strong(idle,2)){unresolved=true;terminal=true;disable();}
    }
    return owner_activity.load()==1||in_flight.load()!=0||(!terminal.load()&&pending_restore.load());
}
// External bounded-drain deadline only. Preserve pending_restore as an explicit
// unresolved receipt, prohibit subsequent writes, and wait for in_flight==0.
void abandon_drain(){drain_requested=true;unresolved=true;terminal=true;unsigned idle=0;owner_activity.compare_exchange_strong(idle,2);disable();}
template<class Read,class Certify>
std::shared_ptr<Request const> load_request(std::string const& text,ULONGLONG tick,Read read,Certify certify,
    char const** rejection_stage=nullptr){
    auto stage=[&](char const* value){if(rejection_stage)*rejection_stage=value;};
    stage("parse");trial::Config c;if(!trial::parse(text,c))throw 2;
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
    const auto previous=std::atomic_load(&selected);const auto loaded=loaded_nonce.load();const bool renew=c.nonce==loaded;
    auto held=[&]{return !terminal.load()&&!drain_requested.load()&&active_nonce.load(std::memory_order_acquire)==c.nonce&&
        consumed_nonce_ack.load(std::memory_order_acquire)==c.nonce&&last_restored_nonce.load(std::memory_order_acquire)<c.nonce;};
    auto completed=[&](trial::Config const& current){
        // Only owner-published atomics; never inspect owner-thread State here.
        return renew&&previous&&previous->config.nonce==current.nonce&&previous->wire==text&&
            last_restored_nonce.load(std::memory_order_acquire)==current.nonce&&
            active_nonce.load(std::memory_order_acquire)==0&&consumed_nonce_ack.load(std::memory_order_acquire)==current.nonce&&
            last_restored_tick.load()>0&&!pending_restore.load()&&!terminal.load()&&!drain_requested.load()&&
            trial::same(current,context(current.client,current.scene));
    };
    stage("terminal");if(terminal.load())throw 2;
    stage("draining");if(drain_requested.load())throw 2;
    stage("older_nonce");if(c.nonce<loaded)throw 2;
    if(renew){
        stage("renewal_missing_previous");if(!previous)throw 2;
        stage("renewal_wire_changed");if(previous->wire!=text)throw 2;
        if(completed(c))throw CompletedRenewal{c.nonce,"completed_before_auth"};
        stage("renewal_expired");if(tick>=previous->expires)throw 2;
        stage("renewal_counter_exhausted");if(previous->renewals==UINT64_MAX)throw 2;
        stage("renewal_not_held");if(!held()){
            if(completed(c))throw CompletedRenewal{c.nonce,"completed_before_auth"};
            throw 2;
        }
    }else{stage("first_admission");if(active_nonce.load(std::memory_order_acquire)||c.nonce<=consumed_nonce_ack.load(std::memory_order_acquire))throw 2;}
#else
    stage("nonce_consumed");if(c.nonce<=loaded_nonce.load())throw 2;
#endif
    stage("authenticate");if(!trial::authenticate(c,context(c.client,c.scene),read)){
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
        // Authentication did not succeed. The independent owner receipt can
        // nevertheless prove that this immutable old renewal is now retired.
        if(completed(c))throw CompletedRenewal{c.nonce,"completed_during_auth"};
#endif
        throw 2;
    }
    stage("certify");if(!certify()){
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
        if(completed(c))throw CompletedRenewal{c.nonce,"completed_during_cert"};
#endif
        throw 2;
    }
    stage("tick_overflow");if(tick>UINT64_MAX-c.duration_ms)throw 2;
    stage("allocate");auto request=std::make_shared<Request>();request->config=std::move(c);request->expires=tick+request->config.duration_ms;
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
    request->wire=text;const auto accepted=GetTickCount64();
    request->first_tick=renew?previous->first_tick:accepted;request->accepted_tick=accepted;request->expires=accepted+request->config.duration_ms;
    request->renewals=renew?previous->renewals+1:0;
    stage("post_context");if(!trial::same(request->config,context(request->config.client,request->config.scene)))throw 2;
    if(renew){
        if(completed(request->config))throw CompletedRenewal{request->config.nonce,"completed_after_auth"};
        stage("post_renewal_not_held");if(!held()){
            if(completed(request->config))throw CompletedRenewal{request->config.nonce,"completed_after_auth"};
            throw 2;
        }
        stage("post_renewal_expired");if(accepted>=previous->expires)throw 2;
    }else{stage("post_first_admission");if(terminal.load()||drain_requested.load()||active_nonce.load()||request->config.nonce<=consumed_nonce_ack.load())throw 2;}
    if(renew)++service_renewed;
    std::atomic_store(&last_request_receipt,std::shared_ptr<Request const>(request));
#endif
    loaded_nonce=request->config.nonce;std::atomic_store(&selected,std::shared_ptr<Request const>(request));return request;
}

template<class Read,class Certify> void poll_with(wchar_t const* path,FILE* log,Read read,Certify certify){
    if(drain_requested.load()||terminal.load())return;
    auto tick=GetTickCount64();if(tick-last_poll<200)return;last_poll=tick;WIN32_FILE_ATTRIBUTE_DATA file{};
    if(!GetFileAttributesExW(path,GetFileExInfoStandard,&file)){if(GetLastError()==ERROR_FILE_NOT_FOUND){disable();seen_file={};}return;}
    if(file.ftLastWriteTime.dwLowDateTime==seen_file.dwLowDateTime&&file.ftLastWriteTime.dwHighDateTime==seen_file.dwHighDateTime)return;
    auto previous=seen_file;seen_file=file.ftLastWriteTime;
    char const* rejection_stage="file_validation";
    try{if((file.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||file.nFileSizeHigh||!file.nFileSizeLow||file.nFileSizeLow>262144)throw 1;
        FILE* input{};if(_wfopen_s(&input,path,L"rb")||!input){seen_file=previous;return;}
        struct Close{FILE* f;~Close(){fclose(f);}} close{input};std::string text(file.nFileSizeLow,'\0');
        if(fread(text.data(),1,text.size(),input)!=text.size()||fgetc(input)!=EOF){seen_file=previous;return;}
        auto req=load_request(text,tick,read,certify,&rejection_stage);
        const auto& c=req->config;
        const double pixels=c.group_fit.enabled?double(trial::delta_for(c))*1080:c.vertical_offset_reference_pixels;
        fprintf(log,"path_position_loaded nonce=%llu duration_ms=%u delta_y_pixels=%.9g group_fit=%u code_attested=1\n",c.nonce,c.duration_ms,pixels,c.group_fit.enabled?1u:0u);fflush(log);
    }catch(CompletedRenewal const& done){
        // No rejected/unresolved transition, sticky clearing, retry or publish.
        fprintf(log,"path_position_completed_renewal_discarded nonce=%llu stage=%s\n",done.nonce,done.stage);fflush(log);
    }catch(...){++rejected;disable();
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
        isolate_ui_failure();
#endif
        fprintf(log,"path_position_load_rejected count=%llu stage=%s\n",rejected.load(),rejection_stage);fflush(log);}}
void poll(wchar_t const* path,FILE* log){poll_with(path,log,safe_copy,certify_code);}
using Setter=void(__fastcall*)(void*,float,unsigned char,unsigned char,int);
bool set(uintptr_t panel,float v,unsigned face,unsigned input,int origin){
    ever_modified=true;reinterpret_cast<Setter>(base+0x298760)(reinterpret_cast<void*>(panel),v,static_cast<unsigned char>(face),static_cast<unsigned char>(input),origin);return true;}
// Owner thread only: callers bind owner_thread before inspecting State.
void count(trial::Result r){switch(r){case trial::Result::applied:successful_nonce=state.lease.config.nonce;++applied;ever_modified=true;break;
    case trial::Result::restored:++restored;break;case trial::Result::rejected:++rejected;break;
    case trial::Result::identity_lost:++identity_lost;break;case trial::Result::ownership_lost:++ownership_lost;break;
    case trial::Result::setter_failed:++setter_failed;break;default:break;}
    pending_restore=state.lease.active;
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
    publish_owner_ack(r);
    if(r==trial::Result::rejected||r==trial::Result::identity_lost||r==trial::Result::ownership_lost||r==trial::Result::setter_failed){
        isolate_ui_failure(); // Optional UI control cannot stop the stereo session.
    }
#else
    if(r==trial::Result::setter_failed){
        // A failed rollback still owns known values: retain the hook and retry
        // on later owner-thread scene entries, never during the same frame.
        disable();if(state.lease.active&&restore_retries.load()<retry_limit)return;
    }
    if(r==trial::Result::identity_lost||r==trial::Result::ownership_lost||r==trial::Result::setter_failed){
        unresolved=state.lease.touched[0]||state.lease.touched[1];terminal=true;
        disable();request_session_stop(SessionStop::proof_failure);
    }
#endif
}
#if defined(GEARS_PATH_CHOICE_CLOSE_RESTORE) && GEARS_PATH_CHOICE_CLOSE_RESTORE
// Called only by the separately authenticated CloseScene boundary, before its
// original. No fabricated root/FCanvas candidate and no inactive-widget writes.
enum class CloseRestoreStatus:unsigned {bypass,rejected,restored,setter_failed,identity_lost,ownership_lost,exception};
struct CloseRestoreCompletion {
    CloseRestoreStatus status=CloseRestoreStatus::bypass;bool borrowed{},attempted{};
    unsigned setter_calls{},read_mask{};std::array<std::uint32_t,2> entry_owned{},observed{};
};
template<class Read,class Set,class Certify>
CloseRestoreCompletion restore_before_close(trial::Config const& expected,Read read,Set setter,Certify certify)noexcept{
    CloseRestoreCompletion out;
    // In particular, a foreign caller cannot inspect inside or any lease State.
    if(owner_thread.load(std::memory_order_acquire)!=GetCurrentThreadId())return out;
    bool owned_admission=false;
    if(owner_activity.load()==1){
        if(!inside||!in_flight.load())return out;out.borrowed=true;
    }else{
        if(inside)return out;unsigned idle=0;if(!owner_activity.compare_exchange_strong(idle,1))return out;
        ++in_flight;inside=true;owned_admission=true;
    }
    struct Release {bool owned;~Release(){if(owned){inside=false;--in_flight;owner_activity=terminal.load()?2u:0u;}}} release{owned_admission};
    if(terminal.load()||restore_retries.load())return out;
    try{
        auto const& l=state.lease;auto const& c=l.config;
        if(!l.active||!l.touched[0]||!l.touched[1]||!applied.load()||successful_nonce!=c.nonce||
           c.nonce!=expected.nonce||c.client!=expected.client||c.scene!=expected.scene||c.panel!=expected.panel||
           c.panel_vtable!=expected.panel_vtable||c.top_bits!=expected.top_bits||c.bottom_bits!=expected.bottom_bits||
           !trial::same_offset(c,expected))return out;
        const auto ctx=context(c.client,c.scene);const auto hint=reinterpret_cast<uintptr_t>(_ReturnAddress());
        auto observed_read=[&](void* to,void const* from,std::size_t size){
            const auto at=reinterpret_cast<uintptr_t>(from);if(size==4&&at==c.panel+0x64)out.read_mask=0;
            const bool ok=read(to,from,size);
            if(ok&&size==4){if(at==c.panel+0x64){std::memcpy(&out.observed[0],to,4);out.read_mask|=1;}
                if(at==c.panel+0x6c){std::memcpy(&out.observed[1],to,4);out.read_mask|=2;}}return ok;};
        if(!trial::same(expected,ctx)||!certify()||
           !trial::authenticate(c,ctx,auth_reader(c,ctx,AuthStage::restore,hint,observed_read))||
           !trial::positions(c,observed_read,out.entry_owned)||out.entry_owned!=l.owned){out.status=CloseRestoreStatus::rejected;
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
            isolate_ui_failure();
#endif
            return out;}
        // Suppress reflow control and nested root mutation BEFORE either Setter.
        ++restore_retries;out.attempted=true;
        auto set_owned=[&](auto panel,float v,unsigned face,unsigned input,int origin){
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
            if(terminal.load())return false;
#endif
            ever_modified=true;++out.setter_calls;return setter(panel,v,face,input,origin);};
        const auto result=trial::restore(state,ctx,auth_reader(c,ctx,AuthStage::restore,hint,observed_read),set_owned);
        count(result);
        switch(result){case trial::Result::restored:out.status=CloseRestoreStatus::restored;break;
            case trial::Result::setter_failed:out.status=CloseRestoreStatus::setter_failed;break;
            case trial::Result::ownership_lost:out.status=CloseRestoreStatus::ownership_lost;break;
            default:out.status=CloseRestoreStatus::identity_lost;break;}
    }catch(...){
        out.status=CloseRestoreStatus::exception;
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
        isolate_ui_failure();
#endif
        if(out.setter_calls){
            // A self-exception after a native write leaves uncertain ownership.
            // Preserve the lease receipt; never guess another rollback value.
            unresolved=true;terminal=true;pending_restore=state.lease.active;disable();
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
            isolate_ui_failure();publish_owner_ack(trial::Result::idle);
#else
            request_session_stop(SessionStop::proof_failure);
#endif
        }
    }
    return out;
}
#endif
template<class Candidate,class Original,class Read,class Set,class Certify>
uintptr_t invoke_with(Candidate const& candidate,Original original,Read read,Set setter,Certify certify){
    // Fixed thread ownership is claimed even before a request exists. Foreign
    // invocations never inspect State, run authentication, or call setters.
    DWORD thread=GetCurrentThreadId(),expected=0;
    owner_thread.compare_exchange_strong(expected,thread);
    if(owner_thread.load()!=thread||inside)return original();
    unsigned idle=0;if(!owner_activity.compare_exchange_strong(idle,1))return original();
    ++in_flight;inside=true;
    struct Reset{~Reset(){inside=false;--in_flight;owner_activity=terminal.load()?2u:0u;}} reset;
    if(terminal.load())return original();
    auto req=std::atomic_load(&selected);auto ctx=context(candidate.client,candidate.scene);
    const auto caller_hint=reinterpret_cast<uintptr_t>(_ReturnAddress());
    auto invoke_set=[&](auto panel,float v,unsigned face,unsigned input,int origin){
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
        if(terminal.load())return false;
#endif
        ever_modified=true;return setter(panel,v,face,input,origin);};
    if(state.lease.active){auto const& c=state.lease.config;
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
        if(active_nonce.load(std::memory_order_acquire)!=c.nonce||consumed_nonce_ack.load(std::memory_order_acquire)!=c.nonce||last_restored_nonce.load(std::memory_order_acquire)>=c.nonce){
            isolate_ui_failure();return original();}
#endif
        if(c.world_generation!=ctx.world_generation||c.started_qpc!=ctx.started_qpc||c.pid!=ctx.pid||c.creation_time!=ctx.creation_time||c.base!=ctx.base){
            observe_context_failure(c,ctx,AuthStage::hold,caller_hint);
            state.lease.active=false;count(trial::Result::identity_lost);return original();}
        if(c.client!=candidate.client||c.scene!=candidate.scene)return original();
        if(!trial::authenticate(c,ctx,auth_reader(c,ctx,AuthStage::hold,caller_hint,read))){state.lease.active=false;count(trial::Result::identity_lost);return original();}
        std::array<std::uint32_t,2> p{};unsigned read_mask=0;
        auto observed_read=[&](void* out,void const* from,std::size_t size){
            bool ok=read(out,from,size);auto address=reinterpret_cast<uintptr_t>(from);
            if(ok&&size==4){if(address==c.panel+0x64)read_mask|=1;if(address==c.panel+0x6c)read_mask|=2;}
            return ok;
        };
        const bool complete=trial::positions(c,observed_read,p);
        if(!complete||p!=state.lease.owned){
            publish_loss(c,ctx.world_generation,p,read_mask,complete,read);
            state.lease.active=false;count(trial::Result::ownership_lost);return original();}
        if(drain_requested.load()||!req||req->config.nonce!=c.nonce||
           !trial::same_offset(req->config,c)||GetTickCount64()>=req->expires){
            if(!certify()){state.lease.active=false;count(trial::Result::identity_lost);return original();}
            ++restore_retries;count(trial::restore(state,ctx,auth_reader(c,ctx,AuthStage::restore,caller_hint,read),invoke_set));return original();}
        return original();
    }
    if(!drain_requested.load()&&req&&GetTickCount64()<req->expires&&req->config.nonce>state.consumed_nonce&&
        req->config.client==candidate.client&&req->config.scene==candidate.scene){
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
        if(active_nonce.load()||req->config.nonce<=consumed_nonce_ack.load()||req->config.nonce<=last_restored_nonce.load())return original();
#endif
        if(!certify()){count(trial::Result::rejected);disable();return original();}
        restore_retries=0;count(trial::apply(state,req->config,ctx,auth_reader(req->config,ctx,AuthStage::apply,caller_hint,read),invoke_set));}
    return original();
}
template<class Candidate,class Original> uintptr_t invoke(Candidate const& candidate,Original original){
    return invoke_with(candidate,original,safe_copy,set,certify_code);
}
void progress(FILE* f){fprintf(f,"path_position_compiled=1 path_position_nonce=%llu path_position_applied=%llu path_position_restored=%llu path_position_rejected=%llu path_position_identity_lost=%llu path_position_ownership_lost=%llu path_position_setter_failed=%llu path_position_pending_restore=%u path_position_owner_thread=%lu path_position_in_flight=%u path_position_restore_retries=%u path_position_draining=%u path_position_unresolved=%u ",
    loaded_nonce.load(),applied.load(),restored.load(),rejected.load(),identity_lost.load(),ownership_lost.load(),setter_failed.load(),pending_restore.load()?1u:0u,owner_thread.load(),in_flight.load(),restore_retries.load(),draining()?1u:0u,unresolved.load()?1u:0u);
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
    fprintf(f,"path_position_service_compiled=1 path_position_active_nonce=%llu path_position_consumed_nonce=%llu path_position_last_restored_nonce=%llu path_position_last_restored_tick=%llu path_position_service_renewed=%llu path_position_service_terminal=%u path_position_request_tuple=nonce,first_tick,accepted_tick,expires_tick,renewals ",
        active_nonce.load(std::memory_order_acquire),consumed_nonce_ack.load(std::memory_order_acquire),last_restored_nonce.load(std::memory_order_acquire),
        last_restored_tick.load(),service_renewed.load(),terminal.load()?1u:0u);
    auto request=std::atomic_load(&last_request_receipt);
    fprintf(f,"path_position_first_tick=%llu path_position_accepted_tick=%llu path_position_expires_tick=%llu path_position_renewals=%llu ",
        request?request->first_tick:0ull,request?request->accepted_tick:0ull,request?request->expires:0ull,request?request->renewals:0ull);
    if(request)fprintf(f,"path_position_request=%llu,%llu,%llu,%llu,%llu ",request->config.nonce,
        request->first_tick,request->accepted_tick,request->expires,request->renewals);
#endif
    const bool ready=loss_ready.load(std::memory_order_acquire);
    fprintf(f,"path_position_loss_ready=%u ",ready?1u:0u);
    if(ready){auto const& r=first_loss;
        fprintf(f,"path_position_loss_read_mask=%u path_position_loss_complete=%u path_position_loss_original_top=%u path_position_loss_original_bottom=%u path_position_loss_owned_top=%u path_position_loss_owned_bottom=%u path_position_loss_observed_top=%u path_position_loss_observed_bottom=%u path_position_loss_units_readable=%u path_position_loss_units=%u path_position_loss_scene=%llu path_position_loss_panel=%llu path_position_loss_world=%llu path_position_loss_current_world=%llu path_position_loss_tick=%llu ",
            r.read_mask,r.positions_complete?1u:0u,r.original[0],r.original[1],r.owned[0],r.owned[1],r.observed[0],r.observed[1],
            r.units_readable?1u:0u,r.units,r.scene,r.panel,r.world,r.current_world,r.tick);
    }
    const bool auth_ready=auth_failure_ready.load(std::memory_order_acquire);
    fprintf(f,"path_position_auth_fail_ready=%u ",auth_ready?1u:0u);
    if(auth_ready){auto const& r=first_auth_failure;
        fprintf(f,"path_position_auth_fail_nonce=%llu path_position_auth_fail_panel=%llu path_position_auth_fail_scene=%llu path_position_auth_fail_world=%llu path_position_auth_fail_current_world=%llu path_position_auth_fail_stage=%u path_position_auth_fail_stage_tuple=hold:1,restore:2,apply:3 path_position_auth_fail_kind=%u path_position_auth_fail_kind_tuple=check:0,context:1,vtable:2,invalidate:3,notify:4,units:5 path_position_auth_fail_context_tuple=pid,creation_time,started_qpc,world,base,client,scene path_position_auth_fail_context_field=%u path_position_auth_fail_index=%u path_position_auth_fail_address=%llu path_position_auth_fail_length=%u path_position_auth_fail_readable=%u path_position_auth_fail_first_mismatch=%u path_position_auth_fail_window_offset=%u path_position_auth_fail_window_size=%u path_position_auth_fail_qpc=%lld path_position_auth_fail_thread=%lu path_position_auth_fail_caller_hint=%llu path_position_auth_fail_expected=",
            r.nonce,r.panel,r.scene,r.world,r.current_world,unsigned(r.stage),unsigned(r.kind),r.context_field,r.check_index,
            static_cast<unsigned long long>(r.address),r.length,r.readable?1u:0u,r.first_mismatch,r.window_offset,r.window_size,r.qpc,r.thread,static_cast<unsigned long long>(r.caller_hint));
        for(unsigned i=0;i<r.window_size;++i)fprintf(f,"%02x",unsigned(r.expected[i]));
        fprintf(f," path_position_auth_fail_observed=");for(unsigned i=0;i<r.window_size;++i)fprintf(f,"%02x",unsigned(r.observed[i]));fprintf(f," ");
    }
    fprintf(f,"path_position_registry_accepted_reads=%llu path_position_registry_accepted_by_header=%llu,%llu path_position_registry_policy_supported=fixed_prefix_v1 path_position_registry_no_high_water=1 path_position_registry_header_tuple=objects:0,names:1 path_position_registry_accepted_stage_tuple=hold:1,restore:2,apply:3 path_position_registry_accepted_tuple=nonce,world,address,check_index,initial_data,initial_num,initial_max,observed_data,observed_num,observed_max,stage,caller_hint,qpc,thread ",
        registry_accepted_reads.load(),registry_accepted_by_header[0].load(),registry_accepted_by_header[1].load());
    for(unsigned i=0;i<2;++i){bool registry_ready=registry_accepted_ready[i].load(std::memory_order_acquire);
        fprintf(f,"path_position_registry_accepted_%u_ready=%u ",i,registry_ready?1u:0u);if(!registry_ready)continue;auto const& r=registry_accepted_receipts[i];
        fprintf(f,"path_position_registry_accepted_%u=%llu,%llu,%llu,%u,%llu,%d,%d,%llu,%d,%d,%u,%llu,%lld,%lu ",i,
            r.nonce,r.world,r.address,r.check_index,r.initial.data,r.initial.num,r.initial.max,r.observed.data,r.observed.num,r.observed.max,unsigned(r.stage),
            static_cast<unsigned long long>(r.caller_hint),r.qpc,r.thread);}
}
#else
void request_drain(){}
bool draining(){return false;}
void abandon_drain(){}
void disable(){}
bool modified(){return false;}
void poll(wchar_t const*,FILE*){}
void progress(FILE* f){fprintf(f,"path_position_compiled=0 ");}
template<class Candidate,class Original> uintptr_t invoke(Candidate const&,Original original){return original();}
#endif
}
