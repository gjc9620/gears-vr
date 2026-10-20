#pragma once
#include <windows.h>
#include <cstdint>
#include <limits>

namespace gears_session {
inline bool decimal(const wchar_t* text,std::uint64_t maximum,std::uint64_t& value) {
    value=0;if(!text||!*text)return false;
    for(;*text;++text){if(*text<L'0'||*text>L'9')return false;
        auto digit=std::uint64_t(*text-L'0');
        if(value>maximum/10||(value==maximum/10&&digit>maximum%10))return false;
        value=value*10+digit;
    }
    return value!=0;
}
enum class OwnerState {alive,exited,error};
inline OwnerState owner_wait_state(DWORD result) {
    return result==WAIT_TIMEOUT?OwnerState::alive:result==WAIT_OBJECT_0?OwnerState::exited:OwnerState::error;
}
inline std::uint64_t creation_time(const FILETIME& value) {
    return (std::uint64_t(value.dwHighDateTime)<<32)|value.dwLowDateTime;
}
struct Owner {
    HANDLE handle{};
    Owner()=default;Owner(const Owner&)=delete;Owner& operator=(const Owner&)=delete;
    ~Owner(){if(handle)CloseHandle(handle);}
    bool open(DWORD pid,std::uint64_t expected) {
        if(handle||!pid||!expected)return false;
        HANDLE candidate=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);
        if(!candidate)return false;
        FILETIME created{},exited{},kernel{},user{};
        bool valid=GetProcessTimes(candidate,&created,&exited,&kernel,&user)&&creation_time(created)==expected&&
            owner_wait_state(WaitForSingleObject(candidate,0))==OwnerState::alive;
        if(!valid){CloseHandle(candidate);return false;}
        handle=candidate;return true;
    }
    OwnerState state()const {return handle?owner_wait_state(WaitForSingleObject(handle,0)):OwnerState::error;}
};
}
