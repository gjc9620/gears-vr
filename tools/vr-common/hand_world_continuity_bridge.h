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

#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL) && defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
// Private consumer diagnostics. Bits only describe predicates actually run;
// helper bits do not claim evaluation of their short-circuited subexpressions.
enum class WorldRejectBranch:unsigned {none,consumer_or_mapping,lock,header,serial_regressed,frame};
enum class WorldRejectCheck:unsigned {
    consumer,mapped,locked,frequency_query,frequency_positive,magic,version,size,pid,game_start,
    native_present,native_matches,frequency_matches,serial_available,serial_not_regressed,
    frame_serial,frame_shape,frame_native,created_not_future,pose_recent,certificate_recent,count
};
// read_frequency means the original QPF call ran, not that its output is valid.
enum WorldReadAvailable:unsigned {read_lock=1,read_packet=2,read_frequency=4,read_now=8,read_now_ns=16};
struct WorldReadObservation {
    unsigned evaluated{},passed{},available{};
    DWORD lock_result{},expected_pid{};
    BOOL frequency_result{};
    std::int64_t frequency{},now_ns{};
    std::uint64_t cursor_before{},cursor_after{},expected_game_start{},expected_native_started_qpc{},now{};
};
// Success-only evidence from the original post-copy clocks. No new clock or mapping read.
struct WorldReadSuccess {
    unsigned known{};
    std::uint64_t now{};
    std::int64_t now_ns{};
};
static_assert(std::is_standard_layout_v<WorldReadSuccess> && std::is_trivially_copyable_v<WorldReadSuccess>);
struct WorldReadRejection {
    static constexpr unsigned schema=1;
    unsigned size{sizeof(WorldReadRejection)},known{};
    WorldRejectBranch branch{};
    WorldReadObservation observation{};
    WorldPacket packet{}; // Exact local copy, including padding; read_packet marks availability.
};
static_assert(std::is_standard_layout_v<WorldReadRejection> && std::is_trivially_copyable_v<WorldReadRejection>);
static_assert(unsigned(WorldRejectCheck::count)<=32);
#endif

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

#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL) && defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    WorldReadRejection first_read_rejection_{}; // No mapping/producer ABI or packet change.
    static bool observe(WorldReadObservation& o,WorldRejectCheck check,bool value,bool rejecting=false) noexcept {
        const auto bit=1u<<unsigned(check);o.evaluated|=bit;
        if(value!=rejecting)o.passed|=bit;
        return value;
    }
    bool read_header_valid(WorldPacket const& p,WorldReadObservation& o) const noexcept {
        LARGE_INTEGER frequency{};o.available|=read_frequency;
        const bool valid=observe(o,WorldRejectCheck::frequency_query,
                (o.frequency_result=QueryPerformanceFrequency(&frequency))!=FALSE)&&
            observe(o,WorldRejectCheck::frequency_positive,frequency.QuadPart>0)&&
            observe(o,WorldRejectCheck::magic,p.magic==WorldPacket::magic_value)&&
            observe(o,WorldRejectCheck::version,p.version==WorldPacket::version_value)&&
            observe(o,WorldRejectCheck::size,p.size==sizeof(WorldPacket))&&
            observe(o,WorldRejectCheck::pid,p.game_pid==pid_)&&
            observe(o,WorldRejectCheck::game_start,p.game_start==game_start_)&&
            observe(o,WorldRejectCheck::native_present,p.native_started_qpc!=0)&&
            observe(o,WorldRejectCheck::native_matches,p.native_started_qpc==native_started_qpc_)&&
            observe(o,WorldRejectCheck::frequency_matches,p.qpc_frequency==frequency.QuadPart)&&
            observe(o,WorldRejectCheck::serial_available,p.latest_serial!=(std::numeric_limits<std::uint64_t>::max)());
        o.frequency=frequency.QuadPart;return valid;
    }
    WorldRead read_rejected(WorldRejectBranch branch,WorldReadObservation o,std::uint64_t cursor,
            WorldPacket const* packet=nullptr) noexcept {
        if(!first_read_rejection_.known){
            o.cursor_after=cursor;first_read_rejection_.branch=branch;first_read_rejection_.observation=o;
            if(packet)std::memcpy(&first_read_rejection_.packet,packet,sizeof(*packet));
            first_read_rejection_.known=1;
        }
        return WorldRead::rejected;
    }
#endif
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
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL) && defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    WorldRead read(std::uint64_t& cursor,WorldFrame& out,WorldReadSuccess* success=nullptr) noexcept {
        out={};if(success)*success={};
        WorldReadObservation o{};o.cursor_before=o.cursor_after=cursor;o.expected_pid=pid_;
        o.expected_game_start=game_start_;o.expected_native_started_qpc=native_started_qpc_;
        if(observe(o,WorldRejectCheck::consumer,producer_,true)||
           observe(o,WorldRejectCheck::mapped,!data_,true))
            return read_rejected(WorldRejectBranch::consumer_or_mapping,o,cursor);
        const auto locked=lock_now();o.available|=read_lock;o.lock_result=locked;
        if(observe(o,WorldRejectCheck::locked,locked!=WAIT_OBJECT_0,true))
            return locked==WAIT_TIMEOUT?WorldRead::none:read_rejected(WorldRejectBranch::lock,o,cursor);
        WorldPacket packet{};std::memcpy(&packet,data_,sizeof(packet));ReleaseMutex(mutex_);
        o.available|=read_packet;
        if(!read_header_valid(packet,o))return read_rejected(WorldRejectBranch::header,o,cursor,&packet);
        if(cursor==(std::numeric_limits<std::uint64_t>::max)()){
            cursor=packet.latest_serial;return WorldRead::baseline;
        }
        if(observe(o,WorldRejectCheck::serial_not_regressed,packet.latest_serial<cursor,true))
            return read_rejected(WorldRejectBranch::serial_regressed,o,cursor,&packet);
        if(packet.latest_serial==cursor)return WorldRead::none;
        cursor=packet.latest_serial;
        auto const& frame=packet.frame;const auto now=GetTickCount64();
        const auto now_ns=gears_vr::monotonic_ns(); // After copying, never a caller's pre-read stamp.
        o.available|=read_now|read_now_ns;o.now=now;o.now_ns=now_ns;
        if(observe(o,WorldRejectCheck::frame_serial,frame.pose.serial!=cursor,true)||
           observe(o,WorldRejectCheck::frame_shape,!world_frame_shape_valid(frame),true)||
           observe(o,WorldRejectCheck::frame_native,frame.certificate.native_started_qpc!=native_started_qpc_,true)||
           observe(o,WorldRejectCheck::created_not_future,now<frame.pose.created_ms,true)||
           observe(o,WorldRejectCheck::pose_recent,now-frame.pose.created_ms>100,true)||
           observe(o,WorldRejectCheck::certificate_recent,!certificate_recent(frame.certificate,now_ns),true))
            return read_rejected(WorldRejectBranch::frame,o,cursor,&packet);
        out=frame;
        if(success)*success={1,now,now_ns};
        return WorldRead::received;
    }
    WorldReadRejection const& first_read_rejection() const noexcept {return first_read_rejection_;}
    // Explicit trial lifecycle reset; close/open do not erase a pending first rejection.
    void reset_read_rejection() noexcept {first_read_rejection_={};}
#else
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
#endif
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
