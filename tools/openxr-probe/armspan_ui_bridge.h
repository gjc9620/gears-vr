#pragma once
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <string>
#include "../vr-common/armspan_ui_state.h"

namespace armspan_ui_bridge {
namespace state=gears_armspan_ui;
enum class Read {current,unavailable,stale,invalid,publisher_stopped};
inline std::uint64_t creation(HANDLE process) noexcept {
    FILETIME born{},exited{},kernel{},user{};
    if(!process||!GetProcessTimes(process,&born,&exited,&kernel,&user))return 0;
    return (std::uint64_t(born.dwHighDateTime)<<32)|born.dwLowDateTime;
}
inline std::wstring name(state::Nonce const& nonce){
    static constexpr wchar_t hex[]=L"0123456789abcdef";
    std::wstring value=L"Local\\GearsVR.ArmspanUI.v1.";
    for(auto byte:nonce){value+=hex[byte>>4];value+=hex[byte&15];}
    return value;
}
inline bool alive_identity(std::uint64_t pid,std::uint64_t born) noexcept {
    if(!state::valid_pid(pid)||!born)return false;
    const auto process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|SYNCHRONIZE,FALSE,DWORD(pid));
    if(!process)return false;
    const bool matches=creation(process)==born&&WaitForSingleObject(process,0)==WAIT_TIMEOUT;
    CloseHandle(process);return matches;
}

// Read-only mapping view. Its process handle pins the publisher identity, not
// controller tracking or any game-image/Native authorization.
class Consumer {
    HANDLE mapping_{},mutex_{},publisher_{};
    state::Packet const* view_{};
    state::Binding binding_{};
    state::Packet latest_{};
    std::uint64_t publisher_pid_{},publisher_created_{};
    bool accepted_{},invalid_{};
    bool copy(state::Packet& value) noexcept {
        const auto waited=WaitForSingleObject(mutex_,0);
        if(waited!=WAIT_OBJECT_0){
            if(waited==WAIT_ABANDONED){ReleaseMutex(mutex_);invalid_=true;}
            else if(waited==WAIT_FAILED)invalid_=true;
            return false;
        }
        std::memcpy(&value,view_,sizeof(value));
        if(!ReleaseMutex(mutex_)){invalid_=true;return false;}
        return true;
    }
    bool publisher_alive()const noexcept {
        return publisher_&&creation(publisher_)==publisher_created_&&WaitForSingleObject(publisher_,0)==WAIT_TIMEOUT;
    }
public:
    Consumer()=default;Consumer(Consumer const&)=delete;Consumer& operator=(Consumer const&)=delete;
    ~Consumer(){close();}
    void close() noexcept {
        if(view_)UnmapViewOfFile(view_);view_=nullptr;
        for(auto handle:{publisher_,mapping_,mutex_})if(handle)CloseHandle(handle);
        publisher_=mapping_=mutex_=nullptr;accepted_=invalid_=false;publisher_pid_=publisher_created_=0;latest_={};binding_={};
    }
    bool open(state::Binding const& binding) noexcept {
        if(mapping_||mutex_||publisher_||!state::binding_valid(binding)||
           !alive_identity(binding.game_pid,binding.game_creation)||!alive_identity(binding.owner_pid,binding.owner_creation))return false;
        try{
            const auto mapping_name=name(binding.nonce),mutex_name=mapping_name+L".Lock";
            mutex_=OpenMutexW(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,mutex_name.c_str());
            mapping_=OpenFileMappingW(FILE_MAP_READ,FALSE,mapping_name.c_str());
            if(!mutex_||!mapping_){close();return false;}
            view_=static_cast<state::Packet const*>(MapViewOfFile(mapping_,FILE_MAP_READ,0,0,sizeof(state::Packet)));
            if(!view_){close();return false;}
            state::Packet first{};
            if(!copy(first)||!state::shape(first,binding)){close();return false;}
            publisher_pid_=first.publisher_pid;publisher_created_=first.publisher_creation;
            publisher_=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|SYNCHRONIZE,FALSE,DWORD(publisher_pid_));
            if(!publisher_alive()){close();return false;}
            binding_=binding;latest_=first;accepted_=true;return true;
        }catch(...){close();return false;}
    }
    state::Language language()const noexcept {return latest_.content.language;}
    Read read(std::int64_t now_ns,state::Packet& output) noexcept {
        if(!mapping_||!view_||!publisher_)return Read::unavailable;
        if(invalid_)return Read::invalid;
        if(!publisher_alive())return Read::publisher_stopped;
        state::Packet value{};
        if(!copy(value)){
            if(invalid_)return Read::invalid;
            if(accepted_&&state::fresh(latest_,now_ns)){output=latest_;return Read::current;}
            return Read::stale;
        }
        if(!state::shape(value,binding_)||value.publisher_pid!=publisher_pid_||value.publisher_creation!=publisher_created_||
           (accepted_&&(value.sequence<latest_.sequence||value.updated_ns<latest_.updated_ns||
             (value.sequence==latest_.sequence&&!state::same_publication(value,latest_))))){invalid_=true;return Read::invalid;}
        if(!publisher_alive())return Read::publisher_stopped;
        latest_=value;accepted_=true;
        if(!state::fresh(value,now_ns))return Read::stale;
        output=value;return Read::current;
    }
};
} // namespace armspan_ui_bridge
