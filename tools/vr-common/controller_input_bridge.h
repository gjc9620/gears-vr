#pragma once
#include "controller_input_frame.h"
#include <windows.h>
#include <cstddef>
#include <cstring>
#include <type_traits>

namespace gears_vr {
inline std::uint64_t controller_process_start(HANDLE process) noexcept {
    FILETIME created{},exited{},kernel{},user{};
    if(!GetProcessTimes(process,&created,&exited,&kernel,&user))return 0;
    return (std::uint64_t(created.dwHighDateTime)<<32)|created.dwLowDateTime;
}
struct ControllerInputPacket {
    static constexpr std::uint32_t magic_value=0x47564332; // GVC2
    static constexpr std::uint32_t version_value=2;
    static constexpr unsigned capacity=8;
    std::uint32_t magic{magic_value},version{version_value},size{sizeof(ControllerInputPacket)},game_pid{};
    std::uint64_t game_start{},publisher_pid{},publisher_start{};
    std::int64_t qpc_frequency{};
    std::uint64_t latest_sequence{};
    ControllerInputFrame frames[capacity]{};
};
static_assert(std::is_standard_layout_v<ControllerInputPacket> &&
              std::is_trivially_copyable_v<ControllerInputPacket>);
static_assert(offsetof(ControllerInputPacket,frames)%alignof(ControllerInputFrame)==0);

// Independent from StereoBridge v4. A receiver asks for the exact frame key
// frozen by its stereo consumer; a newer hand sample can never be substituted.
class ControllerInputBridge {
    HANDLE mapping_{},mutex_{},publisher_process_{};
    ControllerInputPacket* data_{};
    DWORD game_pid_{};
    std::uint64_t game_start_{};
    bool producer_{};
    bool header_valid(ControllerInputPacket const& packet) const noexcept {
        LARGE_INTEGER frequency{};
        return QueryPerformanceFrequency(&frequency)&&frequency.QuadPart>0&&
            packet.magic==ControllerInputPacket::magic_value&&
            packet.version==ControllerInputPacket::version_value&&
            packet.size==sizeof(ControllerInputPacket)&&
            packet.game_pid==game_pid_&&packet.game_start==game_start_&&
            packet.publisher_pid>0&&packet.publisher_pid<=MAXDWORD&&packet.publisher_start&&
            packet.qpc_frequency==frequency.QuadPart;
    }
    static void name(wchar_t (&out)[128],DWORD pid,std::uint64_t created,bool lock) noexcept {
        swprintf_s(out,lock?L"Local\\GearsVR.ControllerInputLock.v2.%lu.%llu":
                           L"Local\\GearsVR.ControllerInput.v2.%lu.%llu",
                   pid,static_cast<unsigned long long>(created));
    }
    static bool lock_now(HANDLE mutex) noexcept {
        return mutex&&WaitForSingleObject(mutex,0)==WAIT_OBJECT_0;
    }
public:
    ControllerInputBridge()=default;
    ControllerInputBridge(ControllerInputBridge const&)=delete;
    ControllerInputBridge& operator=(ControllerInputBridge const&)=delete;
    ~ControllerInputBridge(){close();}
    bool open(DWORD pid,bool producer) noexcept {
        close();if(!pid||(!producer&&pid!=GetCurrentProcessId()))return false;
        HANDLE game=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);
        if(!game)return false;
        const auto created=controller_process_start(game);CloseHandle(game);
        if(!created)return false;
        wchar_t map_name[128]{},lock_name[128]{};
        name(map_name,pid,created,false);name(lock_name,pid,created,true);
        if(producer){
            SetLastError(0);mutex_=CreateMutexW(nullptr,FALSE,lock_name);
            if(!mutex_||GetLastError()==ERROR_ALREADY_EXISTS){close();return false;}
            SetLastError(0);mapping_=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,
                0,sizeof(ControllerInputPacket),map_name);
            if(!mapping_||GetLastError()==ERROR_ALREADY_EXISTS){close();return false;}
        }else{
            mutex_=OpenMutexW(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,lock_name);
            mapping_=OpenFileMappingW(FILE_MAP_READ,FALSE,map_name);
        }
        if(!mutex_||!mapping_){close();return false;}
        data_=static_cast<ControllerInputPacket*>(MapViewOfFile(mapping_,
            producer?FILE_MAP_WRITE:FILE_MAP_READ,0,0,sizeof(ControllerInputPacket)));
        if(!data_){close();return false;}
        game_pid_=pid;game_start_=created;producer_=producer;
        if(producer){
            LARGE_INTEGER frequency{};
            ControllerInputPacket initial{};
            initial.game_pid=pid;initial.game_start=created;
            initial.publisher_pid=GetCurrentProcessId();
            initial.publisher_start=controller_process_start(GetCurrentProcess());
            if(!initial.publisher_start||!QueryPerformanceFrequency(&frequency)||frequency.QuadPart<=0){close();return false;}
            initial.qpc_frequency=frequency.QuadPart;
            if(!lock_now(mutex_)){close();return false;}
            std::memcpy(data_,&initial,sizeof(initial));ReleaseMutex(mutex_);
        }else{
            ControllerInputPacket header{};
            if(!lock_now(mutex_)){close();return false;}
            std::memcpy(&header,data_,sizeof(header));ReleaseMutex(mutex_);
            if(!header_valid(header)){close();return false;}
            publisher_process_=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,
                static_cast<DWORD>(header.publisher_pid));
            if(!publisher_process_||controller_process_start(publisher_process_)!=header.publisher_start||
               WaitForSingleObject(publisher_process_,0)!=WAIT_TIMEOUT){close();return false;}
        }
        return true;
    }
    bool publish(ControllerInputFrame const& frame) noexcept {
        if(!producer_||!data_||!controller_frame_shape_valid(frame)||!lock_now(mutex_))return false;
        bool result=header_valid(*data_)&&frame.sequence>data_->latest_sequence;
        if(result){
            data_->frames[frame.sequence%ControllerInputPacket::capacity]=frame;
            data_->latest_sequence=frame.sequence;
        }
        ReleaseMutex(mutex_);return result;
    }
    bool read_matching(StereoSample const& stereo,std::int64_t now_ns,
                       ControllerInputFrame& out,bool allow_sim=false) noexcept {
        if(producer_||!data_||!publisher_process_||
           WaitForSingleObject(publisher_process_,0)!=WAIT_TIMEOUT||!lock_now(mutex_))return false;
        ControllerInputPacket packet{};
        std::memcpy(&packet,data_,sizeof(packet));ReleaseMutex(mutex_);
        if(!header_valid(packet)||controller_process_start(publisher_process_)!=packet.publisher_start||
           stereo.sequence>packet.latest_sequence||
           packet.latest_sequence-stereo.sequence>=ControllerInputPacket::capacity)return false;
        auto const& selected=packet.frames[stereo.sequence%ControllerInputPacket::capacity];
        if(!controller_frame_matches(selected,stereo,now_ns,allow_sim))return false;
        out=selected;return true;
    }
    void close() noexcept {
        if(data_)UnmapViewOfFile(data_);data_=nullptr;
        if(mapping_)CloseHandle(mapping_);mapping_=nullptr;
        if(mutex_)CloseHandle(mutex_);mutex_=nullptr;
        if(publisher_process_)CloseHandle(publisher_process_);publisher_process_=nullptr;
        game_pid_=0;game_start_=0;producer_=false;
    }
};
}
