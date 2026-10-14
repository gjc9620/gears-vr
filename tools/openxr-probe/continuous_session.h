#pragma once
#include <windows.h>
#include <string>
#include <stdexcept>
#include <cstdint>

// One XR owner per game process. Stop requests do not create an XR instance.
struct ContinuousSessionControl {
    HANDLE stop{}, game{}, owner{};
    DWORD owner_pid{};
    std::uint64_t owner_creation{};
    static std::wstring name(DWORD pid) { return L"Local\\GearsVR.XR.Stop."+std::to_wstring(pid); }
    ContinuousSessionControl()=default;
    ContinuousSessionControl(const ContinuousSessionControl&)=delete;
    ~ContinuousSessionControl(){if(stop)CloseHandle(stop);if(game)CloseHandle(game);if(owner)CloseHandle(owner);}
    static std::uint64_t creation_time(HANDLE process) {
        FILETIME created{},exited{},kernel{},user{};
        if(!GetProcessTimes(process,&created,&exited,&kernel,&user))return 0;
        return (std::uint64_t(created.dwHighDateTime)<<32)|created.dwLowDateTime;
    }
    void open(DWORD pid,DWORD expected_owner,std::uint64_t expected_creation) {
        if(stop||game||owner||!expected_owner||!expected_creation)throw std::runtime_error("Continuous owner identity required");
        owner=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,expected_owner);
        if(!owner||creation_time(owner)!=expected_creation||WaitForSingleObject(owner,0)!=WAIT_TIMEOUT)
            throw std::runtime_error("Continuous owner process identity rejected");
        owner_pid=expected_owner;owner_creation=expected_creation;
        game=OpenProcess(SYNCHRONIZE,FALSE,pid);
        if(!game||WaitForSingleObject(game,0)!=WAIT_TIMEOUT)throw std::runtime_error("Continuous game process is unavailable");
        stop=CreateEventW(nullptr,TRUE,FALSE,name(pid).c_str());
        const auto error=GetLastError();
        if(!stop||error==ERROR_ALREADY_EXISTS)throw std::runtime_error("Continuous XR stop event unavailable or session already owned");
    }
    static const char* owner_wait_reason(DWORD result) {
        return result==WAIT_TIMEOUT?nullptr:result==WAIT_OBJECT_0?"owner_exited":"owner_error";
    }
    const char* owner_reason() const {return owner_wait_reason(owner?WaitForSingleObject(owner,0):WAIT_FAILED);}
    const char* requested() const {
        // Check the retained handle, never reopen a PID that could be reused.
        if(const auto* reason=owner_reason())return reason;
        auto result=WaitForSingleObject(stop,0);
        if(result==WAIT_OBJECT_0)return "stop_event";
        if(result!=WAIT_TIMEOUT)return "control_error";
        result=WaitForSingleObject(game,0);
        if(result==WAIT_OBJECT_0)return "game_exited";
        return result==WAIT_TIMEOUT?nullptr:"control_error";
    }
    static bool request(DWORD pid) {
        HANDLE event=OpenEventW(EVENT_MODIFY_STATE,FALSE,name(pid).c_str());
        if(!event)return false;
        const bool signalled=SetEvent(event)!=FALSE;CloseHandle(event);return signalled;
    }
};

inline bool continuous_orderly_reason(const std::string& reason) {
    return reason=="stop_event"||reason=="producer_stopped"||reason=="runtime_stopping"||reason=="runtime_exiting"||reason=="game_exited";
}
