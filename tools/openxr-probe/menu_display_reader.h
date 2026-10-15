#pragma once
#include <filesystem>
#include "../vr-common/menu_display_state.h"

class MenuDisplayReader {
    std::filesystem::path path_;
    gears_menu::Validator validator_;
    std::uint64_t next_{};
    gears_menu::Packet cached_{};
public:
    MenuDisplayReader(const char* path,DWORD game,std::uint64_t creation,DWORD owner,std::uint64_t owner_creation)
        :path_(path?path:""),validator_(game,creation,owner,owner_creation){
        if(!path_.empty()){
            HANDLE target=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|SYNCHRONIZE,FALSE,game);
            const bool matches=target&&WaitForSingleObject(target,0)==WAIT_TIMEOUT&&
                ContinuousSessionControl::creation_time(target)==creation;
            if(target)CloseHandle(target);
            if(!matches)throw std::runtime_error("Menu display game identity changed");
        }
    }
    gears_menu::Mode poll(std::uint64_t now){
        if(path_.empty())return gears_menu::Mode::unknown;
        if(now>=next_){
            next_=now+100000000;cached_={};
            HANDLE file=CreateFileW(path_.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
            if(file!=INVALID_HANDLE_VALUE){
                LARGE_INTEGER size{};DWORD read{};gears_menu::Packet p{};
                if(GetFileSizeEx(file,&size)&&size.QuadPart==sizeof(p)&&ReadFile(file,&p,sizeof(p),&read,nullptr)&&read==sizeof(p))cached_=p;
                CloseHandle(file);
            }
        }
        return validator_.accept(cached_,now);
    }
};
