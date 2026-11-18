// Included after safe_copy. Only the existing collection worker touches disk.
#include "../vr-common/pose_editor.h"
namespace pose_editor_live {
namespace api=gears_pose_editor;
SRWLOCK config_lock=SRWLOCK_INIT;
api::Config config{};api::Config bank[5]{};
bool enabled{};std::wstring channel;
ULONGLONG next_poll{};
std::atomic<std::uint64_t> finger_revision{},finger_compose{},mount_revision{},mount_frame{},preview_revision{},preview_frame{};
std::uint64_t rejected{};char const* input_status="waiting";
bool snapshot(api::Config& out,unsigned part=0) noexcept {
    if(!enabled||!TryAcquireSRWLockShared(&config_lock))return false;
    out=part<5?bank[part]:config;ReleaseSRWLockShared(&config_lock);return out.revision!=0;
}
bool read(unsigned part,api::Config* out) noexcept {
    api::Config c{};return out&&snapshot(c,part)&&safe_copy(out,&c,sizeof c);
}
void receipt(std::uint64_t revision,unsigned kind,std::uint64_t frame) noexcept {
    if(!enabled||!revision||!frame)return;
    if(kind==1&&revision>=finger_revision.load()){finger_compose.store(frame);finger_revision.store(revision,std::memory_order_release);}
    if(kind==3&&revision>=preview_revision.load()){preview_frame.store(frame);preview_revision.store(revision,std::memory_order_release);}
    if(kind==2&&revision>=mount_revision.load()){mount_frame.store(frame);mount_revision.store(revision,std::memory_order_release);}
}
void configure(wchar_t const* ini,FILE* log) {
    enabled=GetPrivateProfileIntW(L"PoseEditor",L"enabled",0,ini)==1;
    wchar_t environment[1024]{};
    const auto environment_size=GetEnvironmentVariableW(L"GEARSVR_POSE_EDITOR_CHANNEL",environment,1024);
    if(!enabled&&environment_size>0&&environment_size<1024)enabled=true;
    if(!enabled)return;
    wchar_t path[1024]{};GetPrivateProfileStringW(L"PoseEditor",L"channel",environment,path,1024,ini);
    // A separate absolute, existing directory per native session. Never create
    // game files or fall back to the working directory on malformed settings.
    const DWORD attr=GetFileAttributesW(path);
    if(!(path[0]&&path[1]==L':'&&(path[2]==L'\\'||path[2]==L'/'))||
       attr==INVALID_FILE_ATTRIBUTES||!(attr&FILE_ATTRIBUTE_DIRECTORY))enabled=false;
    if(enabled)channel=path;
    if(log)std::fprintf(log,"pose_editor configured=%u default_off=1\n",unsigned(enabled));
}
void worker_tick(bool stopping=false) noexcept {
    if(!enabled)return;
    const auto now=GetTickCount64();if(now<next_poll)return;next_poll=now+100;
    try {
        if(!stopping){
            const auto path=channel+L"\\preview.bin";
            HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
            if(file!=INVALID_HANDLE_VALUE){
                api::Wire wire{};DWORD got{};LARGE_INTEGER size{};
                const bool okay=GetFileSizeEx(file,&size)&&size.QuadPart==api::wire_size&&
                    ReadFile(file,wire.data(),DWORD(wire.size()),&got,nullptr)&&got==wire.size();CloseHandle(file);
                api::Config candidate{};
                if(!okay||!api::decode(wire,candidate)){++rejected;input_status="invalid_retained_previous";}
                else if(candidate.revision<config.revision){++rejected;input_status="older_retained_previous";}
                else if(candidate.revision==config.revision&&std::memcmp(&candidate,&config,sizeof config)){
                    ++rejected;input_status="revision_reused_retained_previous";
                }else{
                    AcquireSRWLockExclusive(&config_lock);
                    if(candidate.enabled)bank[api::part(candidate)]=candidate;
                    else for(auto& saved:bank)saved=candidate;
                    config=candidate;ReleaseSRWLockExclusive(&config_lock);
                    input_status="accepted";
                }
            }else input_status="file_missing_retained_previous";
        }
        const auto path=channel+L"\\status.json";
        gears_status::AtomicFile status(path.c_str());auto* f=status.get();
        if(f){
            FILETIME a{},b{},c{},d{};GetProcessTimes(GetCurrentProcess(),&a,&b,&c,&d);
            const auto creation=(std::uint64_t(a.dwHighDateTime)<<32)|a.dwLowDateTime;
            std::fprintf(f,"{\"schema\":1,\"pid\":%lu,\"creation\":%llu,\"running\":%s,\"received_revision\":%llu,\"enabled\":%u,\"input_status\":\"%s\",\"rejected\":%llu,\"finger_revision\":%llu,\"finger_compose\":%llu,\"mount_revision\":%llu,\"mount_frame\":%llu,\"preview_revision\":%llu,\"preview_frame\":%llu,\"part\":%u,\"evidence\":\"cpu_transaction_only\"}\n",
                GetCurrentProcessId(),creation,stopping?"false":"true",config.revision,config.enabled,input_status,rejected,
                finger_revision.load(),finger_compose.load(),mount_revision.load(),mount_frame.load(),preview_revision.load(),preview_frame.load(),api::part(config));status.commit();
        }
    }catch(...){/* Optional editor I/O never changes gameplay admission. */}
}
}
