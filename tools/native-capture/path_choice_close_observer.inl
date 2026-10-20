// Included after position live. Read-only by default; private close restore is separately gated.
#include "path_choice_close_observer.h"
namespace path_choice_close_observer {
#if defined(GEARS_PATH_CHOICE_CLOSE_OBSERVER) && GEARS_PATH_CHOICE_CLOSE_OBSERVER
#if !defined(GEARS_PATH_CHOICE_POSITION_TRIAL) || !GEARS_PATH_CHOICE_POSITION_TRIAL || !defined(GEARS_UI_SCENE_ROOT_OBSERVER) || !GEARS_UI_SCENE_ROOT_OBSERVER
#error CloseScene observation requires the position trial and scene root observer
#endif
namespace core=gears_path_choice_close;namespace position=path_choice_position_live;
using Exec=uintptr_t(__fastcall*)(void*,void*);
Exec original{};void* target{};
bool read_own_entry(void*,void const*,std::size_t); // Exact sealed target AND this detour; no fallback.
struct Active {core::Request request;ULONGLONG expires{};
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
    std::string wire;ULONGLONG first_tick{},accepted_tick{};uint64_t renewals{};
#endif
};
std::shared_ptr<Active const> selected;
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
std::shared_ptr<Active const> last_request_receipt;
#endif
std::atomic<uint64_t> entries{},loaded_nonce{},load_rejected{};
enum class Bypass:unsigned {stereo,disabled,world,not_recording,foreign_thread,nested,no_request,no_lease,wrong_object,identity,quota,count};
std::array<std::atomic<uint64_t>,unsigned(Bypass::count)> bypass{};
inline constexpr unsigned row_limit=2;
#if defined(GEARS_PATH_CHOICE_CLOSE_RESTORE) && GEARS_PATH_CHOICE_CLOSE_RESTORE
inline constexpr unsigned observation_only=0;
struct RestoreReceipt {uint64_t nonce{},world{};uintptr_t client{},scene{},panel{};DWORD thread{};
    long long enter{},completed{},original_enter{};position::CloseRestoreCompletion completion;};
std::array<RestoreReceipt,2> restore_rows{};std::array<std::atomic<bool>,2> restore_ready{};
std::atomic<unsigned> restore_slots{};std::atomic<uint64_t> restore_attempted{},restore_completed{},restore_failed{};
#else
inline constexpr unsigned observation_only=1;
#endif
struct Row {
    uintptr_t client{},scene{},panel{},caller{},rax{};uint64_t nonce{},world{},after_world{};DWORD thread{};
    long long enter{},exit{};bool normal{},activation_before{},activation_after{},owned_before{};
    std::array<std::uint32_t,2> owned{};core::Snapshot before{},after{};
};
std::array<Row,row_limit> rows{};std::array<std::atomic<bool>,row_limit> ready{};
std::atomic<unsigned> slots{};thread_local unsigned depth{};
ULONGLONG last_poll{};FILETIME seen_file{};
long long qpc(){LARGE_INTEGER value{};QueryPerformanceCounter(&value);return value.QuadPart;}
void disable(){std::atomic_store(&selected,std::shared_ptr<Active const>{});}
bool certify_live(){return core::certify(base,safe_copy,read_own_entry,gears_native_metadata::matches);}
bool enable(FILE* log){
    const bool ok=core::certify(base,safe_copy,safe_copy,gears_native_metadata::matches);
    if(ok)target=reinterpret_cast<void*>(base+core::close_rva);
    fprintf(log,"path_close_attested=%u module_base=%llu rva=0x346574 complete_bytes=62 code_sha256=%s exec_rva=0x34698c exec_chunks=0x34698c:128,0x90dbe6:32 exec_complete_bytes=160 exec_sha256=%s vtable_rva=0x1429d58 slot=0x228 source_snapshot_sha256=911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6 historical_module_base=0x140000000 slot330_certified=0 observation_only=%u\n",
        ok?1u:0u,static_cast<unsigned long long>(base),core::close_sha,core::exec_sha,observation_only);return ok;
}
bool restored(){return !target||core::certify(base,safe_copy,safe_copy,gears_native_metadata::matches);}
template<class Read,class Certify>
std::shared_ptr<Active const> load_request(std::string const& text,ULONGLONG tick,Read read,Certify certify,
    char const** rejection_stage=nullptr){
    auto stage=[&](char const* value){if(rejection_stage)*rejection_stage=value;};
    stage("parse");core::Request r;if(!core::parse(text,r))throw 2;
    auto same_session=[&]{return r.base==base&&r.pid==GetCurrentProcessId()&&r.creation_time==position::creation_time()&&
        r.started_qpc==uint64_t(session_started_qpc)&&r.world_generation==committed_world_generation.load();};
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
    const auto previous=std::atomic_load(&selected);const auto loaded=loaded_nonce.load();const bool renew=r.nonce==loaded;
    std::shared_ptr<position::Request const> position_request;
    auto held=[&]{return !position::terminal.load()&&!position::drain_requested.load()&&position::active_nonce.load(std::memory_order_acquire)==r.nonce&&
        position::consumed_nonce_ack.load(std::memory_order_acquire)==r.nonce&&position::last_restored_nonce.load(std::memory_order_acquire)<r.nonce;};
    auto completed=[&]{
        // Authenticate this close worker's own immutable accepted request, then
        // bind it to the immutable position receipt completed by the owner.
        const auto completed_position=std::atomic_load(&position::last_request_receipt);
        return renew&&previous&&previous->request.nonce==r.nonce&&previous->wire==text&&
            completed_position&&core::same(r,completed_position->config)&&
            position::last_restored_nonce.load(std::memory_order_acquire)==r.nonce&&
            position::active_nonce.load(std::memory_order_acquire)==0&&position::consumed_nonce_ack.load(std::memory_order_acquire)==r.nonce&&
            position::last_restored_tick.load()>0&&!position::pending_restore.load()&&
            !position::terminal.load()&&!position::drain_requested.load()&&same_session();
    };
    auto failed=[&](char const* value){if(completed())throw position::CompletedRenewal{r.nonce,value};throw 2;};
    stage("terminal");if(position::terminal.load())throw 2;
    stage("draining");if(position::drain_requested.load())throw 2;
    stage("older_nonce");if(r.nonce<loaded)throw 2;
    if(renew){
        stage("renewal_missing_previous");if(!previous)throw 2;
        stage("renewal_wire_changed");if(previous->wire!=text)throw 2;
        if(completed())throw position::CompletedRenewal{r.nonce,"close_completed_before_auth"};
        stage("renewal_expired");if(tick>=previous->expires)throw 2;
        stage("renewal_counter_exhausted");if(previous->renewals==UINT64_MAX)throw 2;
        stage("renewal_not_held");if(!held())failed("close_completed_before_auth");
    }else{stage("first_admission");if((position::active_nonce.load()&&position::active_nonce.load()!=r.nonce)||r.nonce<=position::last_restored_nonce.load())throw 2;}
#else
    stage("nonce_consumed");if(r.nonce<=loaded_nonce.load())throw 2;
#endif
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
    if(renew){position_request=std::atomic_load(&position::selected);
        stage("position_binding");if(!position_request||tick>=position_request->expires||!core::same(r,position_request->config))failed("close_completed_during_position_binding");
        stage("position_authenticate");if(!position::trial::authenticate(position_request->config,position::context(r.client,r.scene),read))failed("close_completed_during_position_auth");
        stage("function_identity");if(!core::function_identity(position_request->config,r,read))failed("close_completed_during_function_auth");}
#endif
    stage("session");if(!same_session())throw 2;
    stage("certify");if(!certify()){
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
        failed("close_completed_during_cert");
#else
        throw 2;
#endif
    }
    stage("native_pointer");if(!core::native_pointer(r,read)){
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
        failed("close_completed_during_native_pointer");
#else
        throw 2;
#endif
    }
    stage("tick_overflow");if(tick>UINT64_MAX-r.duration_ms)throw 2;
    stage("allocate");auto request=std::make_shared<Active>();request->request=r;request->expires=tick+r.duration_ms;
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
    request->wire=text;const auto accepted=GetTickCount64();
    stage("post_session");if(!same_session())throw 2;
    stage("post_terminal");if(position::terminal.load())throw 2;
    stage("post_draining");if(position::drain_requested.load())throw 2;
    if(renew){
        if(completed())throw position::CompletedRenewal{r.nonce,"close_completed_after_auth"};
        stage("post_renewal_not_held");if(!held())failed("close_completed_after_auth");
        stage("post_renewal_expired");if(accepted>=previous->expires||accepted>=position_request->expires)throw 2;
    }
    request->first_tick=renew?previous->first_tick:accepted;request->accepted_tick=accepted;
    request->renewals=renew?previous->renewals+1:0;request->expires=accepted+r.duration_ms;
    std::atomic_store(&last_request_receipt,std::shared_ptr<Active const>(request));
#endif
    loaded_nonce=r.nonce;std::atomic_store(&selected,std::shared_ptr<Active const>(request));return request;
}

template<class Read,class Certify> void poll_with(wchar_t const* path,FILE* log,Read read,Certify certify){
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
    if(position::terminal.load()||position::drain_requested.load())return;
#endif
    const auto tick=GetTickCount64();if(tick-last_poll<200)return;last_poll=tick;WIN32_FILE_ATTRIBUTE_DATA file{};
    if(!GetFileAttributesExW(path,GetFileExInfoStandard,&file)){
        if(GetLastError()==ERROR_FILE_NOT_FOUND){disable();seen_file={};}return;}
    if(file.ftLastWriteTime.dwLowDateTime==seen_file.dwLowDateTime&&file.ftLastWriteTime.dwHighDateTime==seen_file.dwHighDateTime)return;
    auto previous=seen_file;seen_file=file.ftLastWriteTime;
    char const* rejection_stage="file_validation";
    try{
        if((file.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||file.nFileSizeHigh||!file.nFileSizeLow||file.nFileSizeLow>32768)throw 1;
        FILE* input{};if(_wfopen_s(&input,path,L"rb")||!input){seen_file=previous;return;}
        struct Close{FILE* file;~Close(){fclose(file);}} close{input};std::string text(file.nFileSizeLow,'\0');
        if(fread(text.data(),1,text.size(),input)!=text.size()||fgetc(input)!=EOF){seen_file=previous;return;}
        auto request=load_request(text,tick,read,certify,&rejection_stage);
        fprintf(log,"path_close_loaded nonce=%llu duration_ms=%u native_pointer=%llu own_prefix_certified=1 observation_only=%u\n",
            request->request.nonce,request->request.duration_ms,static_cast<unsigned long long>(request->request.function+0x114),observation_only);fflush(log);
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
    }catch(position::CompletedRenewal const& done){
        fprintf(log,"path_close_completed_renewal_discarded nonce=%llu stage=%s\n",done.nonce,done.stage);fflush(log);
#endif
    }catch(...){++load_rejected;disable();
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
        position::isolate_ui_failure();
#endif
        fprintf(log,"path_close_load_rejected count=%llu observation_only=%u stage=%s\n",load_rejected.load(),observation_only,rejection_stage);fflush(log);}
}
void poll(wchar_t const* path,FILE* log){
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
    poll_with(path,log,safe_copy,[]{return certify_live()&&position::certify_code();});
#else
    poll_with(path,log,safe_copy,certify_live);
#endif
}

template<class Original,class Read,class Certify,class Restore>
uintptr_t invoke_with(void* client,void* scene,uintptr_t caller,Original call,Read read,Certify certify,Restore restore){
    (void)restore;
    ++entries;auto pass=[&](Bypass why){++bypass[unsigned(why)];return call(client,scene);};
    if(stereo_publish_bypass)return pass(Bypass::stereo);
    Scope scope;if(!ui_observer_enabled)return pass(Bypass::disabled);
    if(!scope.world())return pass(Bypass::world);if(!recording.load())return pass(Bypass::not_recording);
    if(position::owner_thread.load(std::memory_order_acquire)!=GetCurrentThreadId())return pass(Bypass::foreign_thread);
    if(depth)return pass(Bypass::nested);
    ++depth;struct Reset{~Reset(){--depth;}} reset;
    auto request=std::atomic_load(&selected);
    if(!request||GetTickCount64()>=request->expires)return pass(Bypass::no_request);
    // No State read precedes the fixed-owner atomic gate above.
    auto const& lease=position::state.lease;auto const& live=lease.config;auto const& r=request->request;
    if(!lease.active||!lease.touched[0]||!lease.touched[1]||position::terminal.load()||
       !position::applied.load()||!live.nonce||position::successful_nonce!=live.nonce||!core::same(r,live))return pass(Bypass::no_lease);
    position::trial::Config frozen;try{frozen=live;}catch(...){return pass(Bypass::identity);}auto const& c=frozen;
    if(reinterpret_cast<uintptr_t>(client)!=c.client||reinterpret_cast<uintptr_t>(scene)!=c.scene)return pass(Bypass::wrong_object);
    if(c.world_generation!=committed_world_generation.load())return pass(Bypass::world);
#if !defined(GEARS_PATH_CHOICE_CLOSE_RESTORE) || !GEARS_PATH_CHOICE_CLOSE_RESTORE
    if(slots.load()>=row_limit)return pass(Bypass::quota);
#endif
    Row row{};row.client=c.client;row.scene=c.scene;row.panel=c.panel;row.nonce=c.nonce;row.world=c.world_generation;
    row.caller=caller;row.thread=GetCurrentThreadId();row.enter=qpc();row.owned=lease.owned;
    bool proof_ok=false;
    try{
        uintptr_t vt{},slot{};
        row.activation_before=position::trial::authenticate(c,position::context(c.client,c.scene),read);
        row.before=core::snapshot(c,r,read);
        row.owned_before=(row.before.mask&63)==63&&row.before.vtable==c.panel_vtable&&row.before.units==0x04040404&&
            row.before.position[1]==row.owned[0]&&row.before.position[3]==row.owned[1];
        proof_ok=row.activation_before&&row.owned_before&&(row.before.mask&64)&&row.before.occurrences==1&&core::descriptor(c,r)&&
            read(&vt,client,8)&&vt==base+core::client_table_rva&&
            read(&slot,reinterpret_cast<void const*>(vt+core::close_slot),8)&&slot==base+core::close_rva&&
            core::function_identity(c,r,read)&&core::native_pointer(r,read)&&certify();
    }catch(...){proof_ok=false;}
    if(!proof_ok){
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
        position::isolate_ui_failure();
#endif
        return pass(Bypass::identity);
    }
    unsigned slot=slots.load();while(slot<row_limit&&!slots.compare_exchange_weak(slot,slot+1)){}
#if !defined(GEARS_PATH_CHOICE_CLOSE_RESTORE) || !GEARS_PATH_CHOICE_CLOSE_RESTORE
    if(slot>=row_limit)return pass(Bypass::quota);
#else
    if(slot>=row_limit)++bypass[unsigned(Bypass::quota)]; // Diagnostics never authorize or cap restoration.
    RestoreReceipt completion{};completion.nonce=c.nonce;completion.world=c.world_generation;
    completion.client=c.client;completion.scene=c.scene;completion.panel=c.panel;completion.thread=GetCurrentThreadId();completion.enter=qpc();
    // Production restore is noexcept; contain a test/injected prelude exception as well.
    try{completion.completion=restore(c);}catch(...){completion.completion.status=position::CloseRestoreStatus::exception;
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
        position::isolate_ui_failure();
#endif
    }
    completion.completed=qpc();
    if(completion.completion.attempted)++restore_attempted;
    if(completion.completion.status==position::CloseRestoreStatus::restored)++restore_completed;
    else if(completion.completion.status!=position::CloseRestoreStatus::bypass)++restore_failed;
    unsigned receipt_slot=restore_slots.load();while(receipt_slot<restore_rows.size()&&!restore_slots.compare_exchange_weak(receipt_slot,receipt_slot+1)){}
    completion.original_enter=qpc();
    if(receipt_slot<restore_rows.size()){restore_rows[receipt_slot]=completion;restore_ready[receipt_slot].store(true,std::memory_order_release);}
#endif
    // All row state is local until the normal or exceptional original completes.
    auto finish=[&](bool normal)noexcept{
        if(slot>=row_limit)return;row.normal=normal;row.after=core::snapshot(c,r,read);
        row.after_world=committed_world_generation.load();if(row.after_world!=row.world)row.after.mask&=~71u;
        try{row.activation_after=position::trial::authenticate(c,position::context(c.client,c.scene),read);}catch(...){row.activation_after=false;}
        row.exit=qpc();rows[slot]=row;ready[slot].store(true,std::memory_order_release);
    };
    uintptr_t result{};try{result=call(client,scene);row.rax=result;}catch(...){finish(false);throw;}
    finish(true);return result;
}
template<class Original,class Read,class Certify>
uintptr_t invoke_with(void* client,void* scene,uintptr_t caller,Original call,Read read,Certify certify){
#if defined(GEARS_PATH_CHOICE_CLOSE_RESTORE) && GEARS_PATH_CHOICE_CLOSE_RESTORE
    return invoke_with(client,scene,caller,call,read,certify,[&](position::trial::Config const& c){
        return position::restore_before_close(c,read,position::set,position::certify_code);});
#else
    return invoke_with(client,scene,caller,call,read,certify,[]{});
#endif
}
uintptr_t __fastcall hook(void* client,void* scene){return invoke_with(client,scene,reinterpret_cast<uintptr_t>(_ReturnAddress()),original,safe_copy,certify_live);}
void progress(FILE* f){
    fprintf(f,"path_close_compiled=1 path_close_entries=%llu path_close_nonce=%llu path_close_load_rejected=%llu path_close_rows=%u path_close_row_limit=2 path_close_observation_only=%u path_close_bypass_tuple=stereo,disabled,world,not_recording,foreign_thread,nested,no_request,no_lease,wrong_object,identity,quota path_close_bypass=",
        entries.load(),loaded_nonce.load(),load_rejected.load(),slots.load(),observation_only);
    for(unsigned i=0;i<unsigned(Bypass::count);++i)fprintf(f,"%s%llu",i?",":"",bypass[i].load());
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
    auto request=std::atomic_load(&last_request_receipt);
    fprintf(f," path_close_first_tick=%llu path_close_accepted_tick=%llu path_close_expires_tick=%llu path_close_renewals=%llu ",
        request?request->first_tick:0ull,request?request->accepted_tick:0ull,request?request->expires:0ull,request?request->renewals:0ull);
    if(request)fprintf(f,"path_close_request_tuple=nonce,first_tick,accepted_tick,expires_tick,renewals path_close_request=%llu,%llu,%llu,%llu,%llu ",
        request->request.nonce,request->first_tick,request->accepted_tick,request->expires,request->renewals);
#endif
    fprintf(f," path_close_row_tuple=nonce,client,scene,panel,world,caller,thread,enter,exit,normal,rax,activation_before,activation_after,owned_before,owned_top,owned_bottom,after_world path_close_snapshot_identity_is_raw=1 path_close_liveness_proven=0 path_close_snapshot_tuple=mask,read_exception,vtable,left,top,right,bottom,units,active_data,active_num,active_max,scene_occurrences ");
#if defined(GEARS_PATH_CHOICE_CLOSE_RESTORE) && GEARS_PATH_CHOICE_CLOSE_RESTORE
    fprintf(f,"path_close_restore_compiled=1 path_close_restore_attempted=%llu path_close_restore_completed=%llu path_close_restore_failed=%llu path_close_restore_rows=%u path_close_restore_limit=2 path_close_restore_status_tuple=bypass:0,rejected:1,restored:2,setter_failed:3,identity_lost:4,ownership_lost:5,exception:6 path_close_restore_tuple=nonce,client,scene,panel,world,thread,enter,completed,original_enter,status,borrowed,attempted,setter_calls,read_mask,entry_owned_top,entry_owned_bottom,observed_top,observed_bottom ",
        restore_attempted.load(),restore_completed.load(),restore_failed.load(),restore_slots.load());
    for(unsigned i=0;i<restore_rows.size();++i)if(restore_ready[i].load(std::memory_order_acquire)){
        auto const& r=restore_rows[i];auto const& c=r.completion;
        fprintf(f,"path_close_restore_%u=%llu,%llu,%llu,%llu,%llu,%lu,%lld,%lld,%lld,%u,%u,%u,%u,%u,%u,%u,%u,%u ",i,r.nonce,
            static_cast<unsigned long long>(r.client),static_cast<unsigned long long>(r.scene),static_cast<unsigned long long>(r.panel),r.world,r.thread,
            r.enter,r.completed,r.original_enter,unsigned(c.status),c.borrowed?1u:0u,c.attempted?1u:0u,c.setter_calls,c.read_mask,
            c.entry_owned[0],c.entry_owned[1],c.observed[0],c.observed[1]);}
#endif
    for(unsigned i=0;i<row_limit;++i){if(!ready[i].load(std::memory_order_acquire))continue;auto const& r=rows[i];
        fprintf(f,"path_close_%u=%llu,%llu,%llu,%llu,%llu,%llu,%lu,%lld,%lld,%u,%llu,%u,%u,%u,%u,%u,%llu ",i,r.nonce,
            static_cast<unsigned long long>(r.client),static_cast<unsigned long long>(r.scene),static_cast<unsigned long long>(r.panel),r.world,
            static_cast<unsigned long long>(r.caller),r.thread,r.enter,r.exit,r.normal?1u:0u,static_cast<unsigned long long>(r.rax),
            r.activation_before?1u:0u,r.activation_after?1u:0u,r.owned_before?1u:0u,r.owned[0],r.owned[1],r.after_world);
        for(unsigned side=0;side<2;++side){auto const& s=side?r.after:r.before;
            fprintf(f,"path_close_%u_%s=%u,%u,%llu,%u,%u,%u,%u,%u,%llu,%d,%d,%u ",i,side?"after":"before",s.mask,s.read_exception?1u:0u,
                static_cast<unsigned long long>(s.vtable),s.position[0],s.position[1],s.position[2],s.position[3],s.units,
                static_cast<unsigned long long>(s.active.data),s.active.num,s.active.max,s.occurrences);
            if(s.mask&64){fprintf(f,"path_close_%u_%s_members=",i,side?"after":"before");
                if(!s.active.num)fprintf(f,"empty");for(int j=0;j<s.active.num;++j)fprintf(f,"%s%llu",j?",":"",static_cast<unsigned long long>(s.members[unsigned(j)]));fprintf(f," ");}}
    }
}
#else
void progress(FILE* f){fprintf(f,"path_close_compiled=0 ");}
void poll(wchar_t const*,FILE*){}
#endif
}
