// Display-only consumer of the existing session owner's authenticated packet.
// No render-thread file IO and no pose, input, equipment or firing authority.
namespace native_menu_display {
wchar_t packet_path[MAX_PATH]{};
std::unique_ptr<gears_menu::Validator> validator;
std::atomic<uint64_t> lease{},accepted{},rejected{},flat_frames{};
ULONGLONG next_poll{};
bool configure(wchar_t const* ini,FILE* log){
    lease=0;validator.reset();
    const auto length=GetPrivateProfileStringW(L"Capture",L"menu_display_file",L"",packet_path,MAX_PATH,ini);
    if(!length)return true;
    if(length>=MAX_PATH-1||!continuous_session||!session_owner_pid||!session_owner_creation_time)return false;
    FILETIME created{},exited{},kernel{},user{};
    if(!GetProcessTimes(GetCurrentProcess(),&created,&exited,&kernel,&user))return false;
    validator=std::make_unique<gears_menu::Validator>(GetCurrentProcessId(),gears_session::creation_time(created),
        session_owner_pid,session_owner_creation_time,true);
    if(log)fprintf(log,"native_menu_display configured=1 display_only=1 action_authority=0 frozen_per_ticket=1\n");
    return true;
}
void worker_tick(){
    if(!validator)return;
    const auto tick=GetTickCount64();if(tick<next_poll)return;next_poll=tick+50;
    gears_menu::Packet packet{};DWORD read{};LARGE_INTEGER size{};BY_HANDLE_FILE_INFORMATION info{};
    HANDLE handle=CreateFileW(packet_path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    bool valid=false;
    if(handle!=INVALID_HANDLE_VALUE){
        valid=GetFileInformationByHandle(handle,&info)&&
            !(info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))&&
            GetFileSizeEx(handle,&size)&&size.QuadPart==sizeof packet&&
            ReadFile(handle,&packet,sizeof packet,&read,nullptr)&&read==sizeof packet;
        CloseHandle(handle);
    }
    const auto now=uint64_t(gears_vr::monotonic_ns());
    const auto mode=valid?validator->accept(packet,now):gears_menu::Mode::unknown;
    if(mode==gears_menu::Mode::unknown){lease.store(0,std::memory_order_release);++rejected;return;}
    const auto ttl=mode==gears_menu::Mode::movie_display?250000000ull:1500000000ull;
    if(packet.observed_ns>((UINT64_MAX>>3)-ttl)){lease=0;++rejected;return;}
    // Original observation deadline; rereading an identical packet never
    // renews it. Mode and deadline are acquired as one coherent value.
    lease.store(((packet.observed_ns+ttl)<<3)|unsigned(mode),std::memory_order_release);++accepted;
}
gears_menu::Mode current_mode(){
    const auto value=lease.load(std::memory_order_acquire),deadline=value>>3;
    const auto mode=gears_menu::Mode(value&7);
    return deadline&&uint64_t(gears_vr::monotonic_ns())<=deadline?mode:gears_menu::Mode::unknown;
}
bool flat(){
    const auto mode=current_mode();
    const bool result=mode==gears_menu::Mode::pause_menu||mode==gears_menu::Mode::death_menu||mode==gears_menu::Mode::main_menu;
    if(result)++flat_frames;return result;
}
void stop(){lease.store(0,std::memory_order_release);}
void summary(FILE* log){if(log)fprintf(log,
    "native_menu_display_configured=%u native_menu_display_accepted=%llu native_menu_display_rejected=%llu native_menu_flat_frames=%llu native_menu_display_action_authority=0\n",
    unsigned(bool(validator)),accepted.load(),rejected.load(),flat_frames.load());}
}
