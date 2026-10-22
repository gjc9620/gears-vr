#pragma once
#include "equipment_intent_bridge.h"
#include "hand_continuity_certificate.h"
#include "pose_bridge.h"
#include <limits>

namespace gears_hand_continuity {
struct WorldFrame {
    gears_weapon::HandWorldFrame pose{};
    Certificate certificate{};
};
inline bool world_frame_shape_valid(WorldFrame const& frame) noexcept {
    auto const& p=frame.pose;
    auto valid=[](gears_vr::Viewpoint const& v) noexcept {
        return gears_vr::finite(v.position)&&std::isfinite(v.rotation.pitch)&&
            std::isfinite(v.rotation.yaw)&&std::isfinite(v.rotation.roll);
    };
    return certificate_shape_valid(frame.certificate)&&p.session&&p.sequence&&p.space_epoch&&
        p.input_epoch&&p.world_generation&&p.controller&&p.pawn&&
        std::isfinite(p.units_per_metre)&&p.units_per_metre>=10&&p.units_per_metre<=500&&
        !(p.valid_mask&~3u)&&!(p.grip_down_mask&~p.valid_mask)&&
        (!(p.valid_mask&1)||valid(p.left))&&(!(p.valid_mask&2)||valid(p.right));
}
struct WorldPacket {
    static constexpr std::uint32_t magic_value=0x47485731; // GHW1, separate from GVE6.
    static constexpr std::uint32_t version_value=1;
    std::uint32_t magic{magic_value},version{version_value},size{sizeof(WorldPacket)},game_pid{};
    std::uint64_t game_start{},native_started_qpc{},latest_serial{};
    std::int64_t qpc_frequency{};
    WorldFrame frame{};
};
static_assert(std::is_standard_layout_v<WorldPacket> && std::is_trivially_copyable_v<WorldPacket>);
enum class WorldRead {none,baseline,received,rejected};

// Native and Playground live inside the same game process. This lane commits
// its own serial/time with pose+certificate; it never reads GVE6 back to join
// metadata, nor silently filters a consumed invalid packet into "no receipt".
class WorldBridge {
    HANDLE mapping_{},mutex_{};
    WorldPacket* data_{};
    DWORD pid_{};
    std::uint64_t game_start_{},native_started_qpc_{};
    bool producer_{};
    bool header_valid(WorldPacket const& p) const noexcept {
        LARGE_INTEGER frequency{};
        return QueryPerformanceFrequency(&frequency)&&frequency.QuadPart>0&&
            p.magic==WorldPacket::magic_value&&p.version==WorldPacket::version_value&&
            p.size==sizeof(WorldPacket)&&p.game_pid==pid_&&p.game_start==game_start_&&
            p.native_started_qpc&&p.native_started_qpc==native_started_qpc_&&
            p.qpc_frequency==frequency.QuadPart&&p.latest_serial!=(std::numeric_limits<std::uint64_t>::max)();
    }
    DWORD lock_now() const noexcept {
        if(!mutex_)return WAIT_FAILED;
        const auto state=WaitForSingleObject(mutex_,0);
        if(state==WAIT_ABANDONED)ReleaseMutex(mutex_);
        return state;
    }
    static void name(wchar_t (&out)[128],DWORD pid,std::uint64_t start,bool lock) noexcept {
        swprintf_s(out,lock?L"Local\\GearsVR.WorldHandContinuityLock.v1.%lu.%llu":
                           L"Local\\GearsVR.WorldHandContinuity.v1.%lu.%llu",
                   pid,static_cast<unsigned long long>(start));
    }
public:
    WorldBridge()=default;
    WorldBridge(WorldBridge const&)=delete;
    WorldBridge& operator=(WorldBridge const&)=delete;
    ~WorldBridge(){close();}
    bool open(bool producer,std::uint64_t native_started_qpc) noexcept {
        close();pid_=GetCurrentProcessId();
        game_start_=gears_vr::controller_process_start(GetCurrentProcess());
        if(!pid_||!game_start_||!native_started_qpc)return false;
        native_started_qpc_=native_started_qpc;
        wchar_t map_name[128]{},lock_name[128]{};
        name(map_name,pid_,game_start_,false);name(lock_name,pid_,game_start_,true);
        if(producer){
            SetLastError(0);mutex_=CreateMutexW(nullptr,FALSE,lock_name);
            if(!mutex_||GetLastError()==ERROR_ALREADY_EXISTS){close();return false;}
            SetLastError(0);mapping_=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,
                0,sizeof(WorldPacket),map_name);
            if(!mapping_||GetLastError()==ERROR_ALREADY_EXISTS){close();return false;}
        }else{
            mutex_=OpenMutexW(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,lock_name);
            mapping_=OpenFileMappingW(FILE_MAP_READ,FALSE,map_name);
        }
        if(!mutex_||!mapping_){close();return false;}
        data_=static_cast<WorldPacket*>(MapViewOfFile(mapping_,producer?FILE_MAP_WRITE:FILE_MAP_READ,
            0,0,sizeof(WorldPacket)));
        if(!data_||lock_now()!=WAIT_OBJECT_0){close();return false;}
        producer_=producer;
        if(producer){
            LARGE_INTEGER frequency{};
            if(!QueryPerformanceFrequency(&frequency)||frequency.QuadPart<=0){
                ReleaseMutex(mutex_);close();return false;
            }
            WorldPacket initial{};initial.game_pid=pid_;initial.game_start=game_start_;
            initial.native_started_qpc=native_started_qpc;initial.qpc_frequency=frequency.QuadPart;
            std::memcpy(data_,&initial,sizeof(initial));
        }
        const bool ok=header_valid(*data_);ReleaseMutex(mutex_);
        if(!ok){close();return false;}
        return true;
    }
    std::uint64_t native_started_qpc() const noexcept {return native_started_qpc_;}
    bool publish(WorldFrame frame) noexcept {
        if(!producer_||!data_||!world_frame_shape_valid(frame)||
           frame.certificate.native_started_qpc!=native_started_qpc_||
           lock_now()!=WAIT_OBJECT_0)return false;
        // UINT64_MAX remains the initial-attachment cursor sentinel.
        auto const& previous=data_->frame;
        const bool duplicate_source=data_->latest_serial&&
            frame.certificate.xr_publisher_pid==previous.certificate.xr_publisher_pid&&
            frame.certificate.xr_publisher_start==previous.certificate.xr_publisher_start&&
            frame.certificate.source==previous.certificate.source&&frame.pose.session==previous.pose.session&&
            frame.pose.sequence<=previous.pose.sequence;
        const bool ok=header_valid(*data_)&&data_->latest_serial<(std::numeric_limits<std::uint64_t>::max)()-1&&
            !duplicate_source&&certificate_recent(frame.certificate,gears_vr::monotonic_ns());
        if(ok){
            frame.pose.serial=data_->latest_serial+1;frame.pose.created_ms=GetTickCount64();
            data_->frame=frame;data_->latest_serial=frame.pose.serial;
        }
        ReleaseMutex(mutex_);return ok;
    }
    WorldRead read(std::uint64_t& cursor,WorldFrame& out) noexcept {
        out={};
        if(producer_||!data_)return WorldRead::rejected;
        const auto locked=lock_now();
        if(locked!=WAIT_OBJECT_0)return locked==WAIT_TIMEOUT?WorldRead::none:WorldRead::rejected;
        WorldPacket packet{};std::memcpy(&packet,data_,sizeof(packet));ReleaseMutex(mutex_);
        if(!header_valid(packet))return WorldRead::rejected;
        if(cursor==(std::numeric_limits<std::uint64_t>::max)()){
            cursor=packet.latest_serial;return WorldRead::baseline;
        }
        if(packet.latest_serial<cursor)return WorldRead::rejected;
        if(packet.latest_serial==cursor)return WorldRead::none;
        cursor=packet.latest_serial;
        auto const& frame=packet.frame;const auto now=GetTickCount64();
        const auto now_ns=gears_vr::monotonic_ns(); // After copying, never a caller's pre-read stamp.
        if(frame.pose.serial!=cursor||!world_frame_shape_valid(frame)||
           frame.certificate.native_started_qpc!=native_started_qpc_||
           now<frame.pose.created_ms||now-frame.pose.created_ms>100||
           !certificate_recent(frame.certificate,now_ns))return WorldRead::rejected;
        out=frame;return WorldRead::received;
    }
    void close() noexcept {
        if(data_)UnmapViewOfFile(data_);
        data_=nullptr;
        if(mapping_)CloseHandle(mapping_);
        mapping_=nullptr;
        if(mutex_)CloseHandle(mutex_);
        mutex_=nullptr;pid_=0;game_start_=native_started_qpc_=0;producer_=false;
    }
};
} // namespace gears_hand_continuity
