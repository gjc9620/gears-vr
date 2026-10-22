#pragma once
#include "controller_continuity_frame.h"
#include <windows.h>
#include <cstddef>
#include <cstring>

namespace gears_hand_continuity {
struct ContinuityPacket {
    static constexpr std::uint32_t magic_value=0x47484331; // GHC1
    static constexpr std::uint32_t version_value=1;
    static constexpr unsigned capacity=8;
    std::uint32_t magic{magic_value},version{version_value},size{sizeof(ContinuityPacket)},game_pid{};
    std::uint64_t game_start{},publisher_pid{},publisher_start{};
    std::int64_t qpc_frequency{};
    std::uint64_t latest_stream_epoch{},latest_sequence{};
    TrackedInput frames[capacity]{};
};
static_assert(std::is_standard_layout_v<ContinuityPacket> &&
              std::is_trivially_copyable_v<ContinuityPacket>);
static_assert(offsetof(ContinuityPacket,frames)%alignof(TrackedInput)==0);

// Separate mapping from GVC3. The Native consumer must also compare this
// publisher and the complete input with its independently read GVC3 sample.
class InputBridge {
    HANDLE mapping_{},mutex_{},game_process_{},publisher_process_{};
    ContinuityPacket* data_{};
    DWORD game_pid_{};
    std::uint64_t game_start_{};
    Publisher publisher_{};
    TrackedInput last_published_{};
    bool producer_{},published_{};
    static std::uint64_t process_start(HANDLE process) noexcept {
        FILETIME created{},exited{},kernel{},user{};
        if(!GetProcessTimes(process,&created,&exited,&kernel,&user))return 0;
        return (std::uint64_t(created.dwHighDateTime)<<32)|created.dwLowDateTime;
    }
    static bool process_alive(HANDLE process,std::uint64_t pid,std::uint64_t start) noexcept {
        return process&&pid&&pid<=MAXDWORD&&start&&GetProcessId(process)==pid&&
            WaitForSingleObject(process,0)==WAIT_TIMEOUT&&process_start(process)==start;
    }
    bool header_valid(ContinuityPacket const& packet) const noexcept {
        LARGE_INTEGER frequency{};
        return QueryPerformanceFrequency(&frequency)&&frequency.QuadPart>0&&
            packet.magic==ContinuityPacket::magic_value&&packet.version==ContinuityPacket::version_value&&
            packet.size==sizeof(ContinuityPacket)&&packet.game_pid==game_pid_&&packet.game_start==game_start_&&
            publisher_.pid&&publisher_.pid<=MAXDWORD&&publisher_.start&&
            packet.publisher_pid==publisher_.pid&&packet.publisher_start==publisher_.start&&
            packet.qpc_frequency==frequency.QuadPart&&
            ((packet.latest_stream_epoch==0)==(packet.latest_sequence==0))&&
            (!packet.latest_sequence||
                (packet.frames[packet.latest_sequence%ContinuityPacket::capacity].stream_epoch==packet.latest_stream_epoch&&
                 packet.frames[packet.latest_sequence%ContinuityPacket::capacity].input.sequence==packet.latest_sequence&&
                 tracked_input_shape_valid(packet.frames[packet.latest_sequence%ContinuityPacket::capacity])));
    }
    static void name(wchar_t (&out)[128],DWORD pid,std::uint64_t created,bool lock) noexcept {
        swprintf_s(out,lock?L"Local\\GearsVR.ControllerContinuityLock.v1.%lu.%llu":
                           L"Local\\GearsVR.ControllerContinuity.v1.%lu.%llu",
                   pid,static_cast<unsigned long long>(created));
    }
    static bool lock_now(HANDLE mutex) noexcept {
        if(!mutex)return false;
        const auto result=WaitForSingleObject(mutex,0);
        if(result==WAIT_ABANDONED)ReleaseMutex(mutex);
        return result==WAIT_OBJECT_0;
    }
    bool open_publisher() noexcept {
        if(!publisher_.pid||publisher_.pid>MAXDWORD||!publisher_.start)return false;
        publisher_process_=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,
            static_cast<DWORD>(publisher_.pid));
        return process_alive(publisher_process_,publisher_.pid,publisher_.start);
    }
    bool publication_advances(TrackedInput const& next) const noexcept {
        if(!published_)return true;
        auto const& old=last_published_;
        if(next.stream_epoch<old.stream_epoch||next.tracking_epoch[0]<old.tracking_epoch[0]||
           next.tracking_epoch[1]<old.tracking_epoch[1]||detail::raw_regressed(old.input,next.input))return false;
        if(next.stream_epoch==old.stream_epoch)
            return detail::same_key(old.input,next.input)&&next.input.sequence>old.input.sequence;
        return true;
    }
public:
    InputBridge()=default;
    InputBridge(InputBridge const&)=delete;
    InputBridge& operator=(InputBridge const&)=delete;
    ~InputBridge(){close();}
    bool open(DWORD targetGamePid,bool producer) noexcept {
        close();
        if(!targetGamePid||(!producer&&targetGamePid!=GetCurrentProcessId()))return false;
        game_process_=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,targetGamePid);
        game_pid_=targetGamePid;game_start_=process_start(game_process_);
        if(!process_alive(game_process_,game_pid_,game_start_)){close();return false;}
        wchar_t map_name[128]{},lock_name[128]{};
        name(map_name,game_pid_,game_start_,false);name(lock_name,game_pid_,game_start_,true);
        if(producer){
            SetLastError(0);mutex_=CreateMutexW(nullptr,FALSE,lock_name);
            if(!mutex_||GetLastError()==ERROR_ALREADY_EXISTS){close();return false;}
            SetLastError(0);mapping_=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,
                0,sizeof(ContinuityPacket),map_name);
            if(!mapping_||GetLastError()==ERROR_ALREADY_EXISTS){close();return false;}
        }else{
            mutex_=OpenMutexW(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,lock_name);
            mapping_=OpenFileMappingW(FILE_MAP_READ,FALSE,map_name);
        }
        if(!mutex_||!mapping_){close();return false;}
        data_=static_cast<ContinuityPacket*>(MapViewOfFile(mapping_,
            producer?FILE_MAP_WRITE:FILE_MAP_READ,0,0,sizeof(ContinuityPacket)));
        if(!data_){close();return false;}
        producer_=producer;
        if(producer){
            LARGE_INTEGER frequency{};
            publisher_={GetCurrentProcessId(),process_start(GetCurrentProcess())};
            if(!open_publisher()||!QueryPerformanceFrequency(&frequency)||frequency.QuadPart<=0){close();return false;}
            ContinuityPacket initial{};
            initial.game_pid=game_pid_;initial.game_start=game_start_;
            initial.publisher_pid=publisher_.pid;initial.publisher_start=publisher_.start;
            initial.qpc_frequency=frequency.QuadPart;
            if(!lock_now(mutex_)){close();return false;}
            std::memcpy(data_,&initial,sizeof(initial));ReleaseMutex(mutex_);
        }else{
            ContinuityPacket header{};
            if(!lock_now(mutex_)){close();return false;}
            std::memcpy(&header,data_,sizeof(header));ReleaseMutex(mutex_);
            publisher_={header.publisher_pid,header.publisher_start};
            if(!header_valid(header)||!open_publisher()){close();return false;}
        }
        return true;
    }
    bool publish(TrackedInput const& frame) noexcept {
        if(!producer_||!data_||!tracked_input_shape_valid(frame)||!publication_advances(frame)||
           !process_alive(game_process_,game_pid_,game_start_)||
           !process_alive(publisher_process_,publisher_.pid,publisher_.start)||!lock_now(mutex_))return false;
        const bool result=header_valid(*data_)&&
            data_->latest_stream_epoch==(published_?last_published_.stream_epoch:0)&&
            data_->latest_sequence==(published_?last_published_.input.sequence:0);
        if(result){
            if(frame.stream_epoch!=data_->latest_stream_epoch)
                for(auto& entry:data_->frames)entry={};
            data_->frames[frame.input.sequence%ContinuityPacket::capacity]=frame;
            data_->latest_stream_epoch=frame.stream_epoch;data_->latest_sequence=frame.input.sequence;
            last_published_=frame;published_=true;
        }
        ReleaseMutex(mutex_);return result;
    }
    bool read_matching(gears_vr::StereoSample const& stereo,std::int64_t now_ns,
                       TrackedInput& out,Publisher& publisher,bool allow_sim=false) noexcept {
        out={};publisher={};
        if(producer_||!data_||!process_alive(game_process_,game_pid_,game_start_)||
           !process_alive(publisher_process_,publisher_.pid,publisher_.start)||!lock_now(mutex_))return false;
        ContinuityPacket packet{};
        std::memcpy(&packet,data_,sizeof(packet));ReleaseMutex(mutex_);
        if(!header_valid(packet)||!process_alive(game_process_,game_pid_,game_start_)||
           !process_alive(publisher_process_,publisher_.pid,publisher_.start)||
           stereo.sequence>packet.latest_sequence||
           packet.latest_sequence-stereo.sequence>=ContinuityPacket::capacity)return false;
        auto const& selected=packet.frames[stereo.sequence%ContinuityPacket::capacity];
        if(selected.stream_epoch!=packet.latest_stream_epoch||!tracked_input_shape_valid(selected)||
           !gears_vr::controller_frame_matches(selected.input,stereo,now_ns,allow_sim))return false;
        out=selected;publisher={packet.publisher_pid,packet.publisher_start};return true;
    }
    void close() noexcept {
        if(data_)UnmapViewOfFile(data_);data_=nullptr;
        if(mapping_)CloseHandle(mapping_);mapping_=nullptr;
        if(mutex_)CloseHandle(mutex_);mutex_=nullptr;
        if(publisher_process_)CloseHandle(publisher_process_);publisher_process_=nullptr;
        if(game_process_)CloseHandle(game_process_);game_process_=nullptr;
        game_pid_=0;game_start_=0;publisher_={};last_published_={};producer_=false;published_=false;
    }
};
}
