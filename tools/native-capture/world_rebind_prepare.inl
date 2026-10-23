// Worker-side preparation only. No live anchor/config, renderer lifetime, GPU
// epoch or OpenXR session is changed by accepting an immutable pending bundle.
struct PreparedWorld {
    uint64_t request{};
    gears_world_rebind::Metadata metadata{};
    gears_anchor::Config anchor{};
    gears_ui::Config ui{};
#if defined(GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER) && GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER
    std::optional<gears_crosshair_assignment::OwnedConfig> crosshair;
#endif
};
std::unique_ptr<PreparedWorld> pending_world;
bool same_world_object(gears_anchor::Identity const& a,gears_anchor::Identity const& b){
    return a.address==b.address&&a.index==b.index&&a.name_index==b.name_index&&a.number==b.number&&
        a.cls==b.cls&&a.outer==b.outer&&a.name==b.name;
}
bool prepare_world_candidate(const wchar_t* ini,PreparedWorld& output,bool& timestamp_rejected,gears_profile::Snapshot const* profile=nullptr){
    timestamp_rejected=false;
    if(!world_handoff_supported||!hook_entry_manifest_ready.load()||!anchor_observer_enabled||!ui_observer_enabled||submitted_lifetime.stopping())return false;
    FILETIME created{},exited{},kernel{},user{};
    if(!GetProcessTimes(GetCurrentProcess(),&created,&exited,&kernel,&user))return false;
    const gears_world_rebind::Expected expected{GetCurrentProcessId(),session_owner_pid,
        gears_session::creation_time(created),session_owner_creation_time,frame_link.world_epoch(),base,expected_local};
    auto text=[&](const wchar_t* section,const wchar_t* key,wchar_t* buffer,unsigned capacity){
        return gears_profile::get(profile,section,key,buffer,capacity,ini);};
    auto candidate=std::make_unique<PreparedWorld>();
    if(!gears_world_rebind::parse(text,expected,candidate->metadata))return false;
    auto const& m=candidate->metadata;auto& c=candidate->anchor;
    if(!gears_anchor::parse_config([&](const wchar_t* key,wchar_t* buffer,unsigned capacity){
           return text(L"AnchorObserver",key,buffer,capacity);},base,c)||!c.cinematic_enabled||
       c.role_local>=c.identity_count||c.role_controller>=c.identity_count||c.role_camera>=c.identity_count||
       anchor_config.role_local>=anchor_config.identity_count||
       !same_world_object(c.identities[c.role_local],anchor_config.identities[anchor_config.role_local])||
       c.identities[c.role_local].address!=m.local_player||c.identities[c.role_controller].address!=m.controller||
       c.identities[c.role_camera].address!=m.camera||c.camera_timestamp>UINTPTR_MAX-m.camera||
       m.camera+c.camera_timestamp!=m.timestamp||expected_camera>camera_yaw||m.camera>m.camera_yaw||
       m.camera_yaw-m.camera!=camera_yaw-expected_camera||c.camera_pov!=anchor_config.camera_pov)return false;
    configure_unpossessed_camera(c);
    gears_anchor::Anchor live{};std::uint32_t stamp{};
    auto guard=[&](){
        gears_anchor::BoundedGuardDiagnostic diagnostic{};
        const bool accepted=gears_anchor::bounded_guard(c,safe_copy,[](){return std::uint64_t(GetTickCount64());},
            [](unsigned milliseconds){Sleep(milliseconds);},live,stamp,&diagnostic);
        if(!accepted)timestamp_rejected=gears_anchor::bounded_timestamp_retryable(diagnostic);
        return accepted;
    };
    if(!guard()||!parse_ui_config(ini,candidate->ui,m.controller,m.camera,true,profile))return false;
    auto const& ui=candidate->ui;
    if(ui.role_controller>=ui.identity_count||ui.role_camera>=ui.identity_count||ui.role_pawn>=ui.identity_count||
       c.role_pawn>=c.identity_count||!same_world_object(ui.identities[ui.role_controller],c.identities[c.role_controller])||
       !same_world_object(ui.identities[ui.role_camera],c.identities[c.role_camera])||
       ui.mounted!=c.mounted_enabled||(c.mounted_enabled&&c.mounted_role_driver>=c.identity_count)||
       !same_world_object(ui.identities[ui.role_pawn],c.identities[c.mounted_enabled?c.mounted_role_driver:c.role_pawn])||!guard())return false;
#if defined(GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER) && GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER
    if(crosshair_live::enabled){candidate->crosshair.emplace();
        gears_anchor::GuardDiagnostic diagnostic{};
        if(!crosshair_live::prepare_recovery(ini,c,ui,*candidate->crosshair,&diagnostic,profile)){
            timestamp_rejected=gears_anchor::timestamp_retryable(diagnostic);return false;
        }
        if(!guard())return false;
    }
#endif
    output=*candidate;return true;
}
class WorldRebindPrepareRequest {
    HANDLE event_{},commit_event_{};
    gears_world::ControlWake wake_;
    gears_world::ControlCadence cadence_;
public:
    WorldRebindPrepareRequest(){
        if(!continuous_session||!hook_entry_manifest_ready.load())return;
        wchar_t name[128]{};swprintf_s(name,L"Local\\GearsVR.Capture.WorldRebindPrepare.%lu",GetCurrentProcessId());
        event_=CreateEventW(nullptr,FALSE,FALSE,name);
        swprintf_s(name,L"Local\\GearsVR.Capture.WorldRebindCommit.%lu",GetCurrentProcessId());
        commit_event_=CreateEventW(nullptr,FALSE,FALSE,name);
    }
    ~WorldRebindPrepareRequest(){if(event_)CloseHandle(event_);if(commit_event_)CloseHandle(commit_event_);}
    bool wait(HANDLE stop){
        const bool stopping=submitted_lifetime.stopping();
        const auto request=pending_world&&world_commit_request.load()==pending_world->request?pending_world->request:0;
        const auto interval=cadence_.interval(request,GetTickCount64(),stopping);
        // The manual stop event stays signalled throughout draining. Retain the
        // original cleanup sleep, avoiding an immediate-wake cleanup spin.
        if(stopping){Sleep(interval);return !wake_.failed();}
        return wake_.wait(stop,event_,commit_event_,interval)!=gears_world::ControlWakeResult::failed;
    }
    void print(FILE* status)const{
        fprintf(status,"world_control_wait world_wait_prepare_wakes=%llu world_wait_commit_wakes=%llu world_wait_stop_wakes=%llu world_wait_timeouts=%llu world_wait_failed=%u world_wait_idle_ms=20 world_wait_fast_ms=1 world_wait_burst_ms=250\n",
            static_cast<unsigned long long>(wake_.prepare_wakes()),static_cast<unsigned long long>(wake_.commit_wakes()),
            static_cast<unsigned long long>(wake_.stop_wakes()),static_cast<unsigned long long>(wake_.timeouts()),wake_.failed()?1u:0u);
    }
    void poll(FILE* status){
        if(wake_.take_commit(commit_event_)&&pending_world&&!submitted_lifetime.stopping()){
            world_commit_request=pending_world->request;world_handoff_phase=world_quiesce_requested.load()?2u:1u;
        }
        if(!wake_.take_prepare(event_))return;
        ++world_prepare_requests;
        world_prepare_timestamp_request=0;
        const auto generation=frame_link.world_epoch();
        if(generation==UINT64_MAX||pending_world||submitted_lifetime.stopping()){
            ++world_prepare_rejected;
            fprintf(status,"world_rebind_prepare accepted=0 pending=%u stopping=%u generation=%llu\n",pending_world?1:0,
                submitted_lifetime.stopping()?1:0,static_cast<unsigned long long>(generation));fflush(status);return;}
        FILETIME created{},exited{},kernel{},user{};
        if(!GetProcessTimes(GetCurrentProcess(),&created,&exited,&kernel,&user)){++world_prepare_rejected;return;}
        wchar_t filename[112]{},path[MAX_PATH]{};swprintf_s(filename,L"world-rebind-%llu-%llu-%llu.ini",
            static_cast<unsigned long long>(gears_session::creation_time(created)),static_cast<unsigned long long>(generation+1),
            static_cast<unsigned long long>(world_prepare_requests.load()));path_for(path,filename);
        // Retain a read handle denying writes/deletion across all INI reads.
        HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file==INVALID_HANDLE_VALUE&&GetLastError()==ERROR_FILE_NOT_FOUND&&world_prepare_requests.load()==1){
            swprintf_s(filename,L"world-rebind-%llu.ini",static_cast<unsigned long long>(generation+1));path_for(path,filename);
            file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        }
        bool accepted=false,timestamp_rejected=false;LARGE_INTEGER size{};
        if(file!=INVALID_HANDLE_VALUE){
            unsigned char bom[2]{};DWORD count{};
            if(GetFileSizeEx(file,&size)&&size.QuadPart>2&&size.QuadPart<=2*1024*1024&&
               ReadFile(file,bom,2,&count,nullptr)&&count==2&&bom[0]==0xff&&bom[1]==0xfe){
                gears_profile::Snapshot profile;profile.load(file);
                auto candidate=std::make_unique<PreparedWorld>();accepted=prepare_world_candidate(path,*candidate,timestamp_rejected,&profile);
                if(accepted){candidate->request=world_prepare_requests.load();pending_world=std::move(candidate);world_candidate_generation=pending_world->metadata.generation;}
            }
            CloseHandle(file);
        }
        if(!accepted){
            if(timestamp_rejected)world_prepare_timestamp_request=world_prepare_requests.load();
            ++world_prepare_rejected;
        }
        fprintf(status,"world_rebind_prepare accepted=%u pending=%u generation=%llu next_generation=%llu timestamp_rejected=%u committed=0 live_config_changed=0\n",
            accepted?1:0,pending_world?1:0,static_cast<unsigned long long>(generation),static_cast<unsigned long long>(generation+1),timestamp_rejected?1u:0u);fflush(status);
    }
};
