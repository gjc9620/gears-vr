#pragma once
#include "controller_input_bridge.h"
#include "pose_math.h"
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace gears_weapon {
// index_acquire_slot is a PG-local intent derived from its authenticated
// current Native Frame. The legacy Native bridge deliberately does NOT accept
// or publish it; this adds no wire fields or permission to reinterpret Grip.
enum class EquipmentIntentKind : std::uint8_t { slot,grab_ammo,reload,index_acquire_slot };
struct EquipmentIntentEvent {
    std::uint64_t serial{},created_ms{},input_session{},input_sequence{},space_epoch{},input_epoch{},world_generation{};
    std::uintptr_t controller{},pawn{},weapon{};
    std::uint8_t inventory_slot{4};
    EquipmentIntentKind kind{EquipmentIntentKind::slot};
};
struct HandWorldFrame {
    std::uint64_t serial{},created_ms{},session{},sequence{},space_epoch{},input_epoch{},world_generation{};
    // weapon==0 is an authenticated unarmed pose: read-only hand targets only.
    // It cannot bind an equipment intent. Packet layout and GVE6 stay unchanged.
    std::uintptr_t controller{},pawn{},weapon{};
    double units_per_metre{}; // World-pose unit scale frozen with this XR ticket.
    gears_vr::Viewpoint left{},right{};
    std::uint8_t valid_mask{}; // bit 0 left, bit 1 right; zero explicitly clears tracking.
    std::uint8_t grip_down_mask{}; // Same XR ticket; only tracked hands may be down.
};

// A consumer must bind the queued edge to a recent pose from the same XR and
// game generation before changing inventory or creating a hand-held object.
// The pose may be from a later frame, but never an earlier input ticket.
inline bool equipment_intent_matches_pose(EquipmentIntentEvent const& event,
        HandWorldFrame const& pose,std::uint64_t now,
        std::uintptr_t controller,std::uintptr_t pawn,std::uintptr_t weapon) noexcept {
    const auto hand=event.kind==EquipmentIntentKind::grab_ammo?1u:2u;
    return controller&&pawn&&weapon&&
        event.controller==controller&&event.pawn==pawn&&event.weapon==weapon&&
        pose.controller==controller&&pose.pawn==pawn&&pose.weapon==weapon&&
        std::isfinite(pose.units_per_metre)&&pose.units_per_metre>=10&&
        pose.units_per_metre<=500&&
        event.world_generation&&event.world_generation==pose.world_generation&&
        event.input_session&&event.input_session==pose.session&&
        event.space_epoch&&event.space_epoch==pose.space_epoch&&
        event.input_epoch&&event.input_epoch==pose.input_epoch&&
        event.input_sequence&&event.input_sequence<=pose.sequence&&
        (pose.valid_mask&hand)!=0&&
        now>=event.created_ms&&now-event.created_ms<=250&&
        now>=pose.created_ms&&now-pose.created_ms<=100;
}
struct EquipmentIntentPacket {
    static constexpr std::uint32_t magic_value=0x47564536; // GVE6
    static constexpr std::uint32_t version_value=6;
    static constexpr unsigned capacity=8;
    std::uint32_t magic{magic_value},version{version_value},size{sizeof(EquipmentIntentPacket)},game_pid{};
    std::uint64_t game_start{},publisher_pid{},publisher_start{},latest_serial{},latest_pose_serial{};
    EquipmentIntentEvent events[capacity]{};
    HandWorldFrame pose{};
};
static_assert(std::is_standard_layout_v<EquipmentIntentPacket> &&
              std::is_trivially_copyable_v<EquipmentIntentPacket>);

// Native's certified game-thread input router publishes a body-slot, left
// shoulder ammo-grab, or R3 reload intent with original game identities. The receiving
// adapter must still read inventory and revalidate identity before acting.
class EquipmentIntentBridge {
    HANDLE mapping_{},mutex_{};
    EquipmentIntentPacket* data_{};
    DWORD pid_{};
    std::uint64_t game_start_{};
    bool producer_{};
    bool valid(EquipmentIntentPacket const& packet) const noexcept {
        return packet.magic==EquipmentIntentPacket::magic_value&&
            packet.version==EquipmentIntentPacket::version_value&&
            packet.size==sizeof(EquipmentIntentPacket)&&packet.game_pid==pid_&&
            packet.game_start==game_start_&&packet.publisher_pid==pid_&&
            packet.publisher_start==game_start_;
    }
    static bool event_valid(EquipmentIntentEvent const& event) noexcept {
        return ((event.kind==EquipmentIntentKind::slot&&event.inventory_slot<4)||
                (event.kind==EquipmentIntentKind::grab_ammo&&event.inventory_slot==4)||
                (event.kind==EquipmentIntentKind::reload&&event.inventory_slot==4))&&
            event.controller&&event.pawn&&event.weapon&&event.world_generation&&
            event.input_session&&event.input_sequence&&event.space_epoch&&event.input_epoch;
    }
    static bool pose_valid(HandWorldFrame const& pose) noexcept {
        auto viewpoint=[](gears_vr::Viewpoint const& v){
            return gears_vr::finite(v.position)&&std::isfinite(v.rotation.pitch)&&
                std::isfinite(v.rotation.yaw)&&std::isfinite(v.rotation.roll);
        };
        return pose.session&&pose.sequence&&pose.space_epoch&&pose.input_epoch&&pose.world_generation&&
            std::isfinite(pose.units_per_metre)&&pose.units_per_metre>=10&&
            pose.units_per_metre<=500&&
            pose.controller&&pose.pawn&&!(pose.valid_mask&~3u)&&
            !(pose.grip_down_mask&~pose.valid_mask)&&
            (!(pose.valid_mask&1)||viewpoint(pose.left))&&
            (!(pose.valid_mask&2)||viewpoint(pose.right));
    }
    static void name(wchar_t (&out)[128],DWORD pid,std::uint64_t start,bool lock) noexcept {
        swprintf_s(out,lock?L"Local\\GearsVR.EquipmentIntentLock.v6.%lu.%llu":
                           L"Local\\GearsVR.EquipmentIntent.v6.%lu.%llu",
                   pid,static_cast<unsigned long long>(start));
    }
    bool lock_now() const noexcept {return mutex_&&WaitForSingleObject(mutex_,0)==WAIT_OBJECT_0;}
public:
    EquipmentIntentBridge()=default;
    EquipmentIntentBridge(EquipmentIntentBridge const&)=delete;
    EquipmentIntentBridge& operator=(EquipmentIntentBridge const&)=delete;
    ~EquipmentIntentBridge(){close();}
    bool open(bool producer) noexcept {
        close();
        pid_=GetCurrentProcessId();game_start_=gears_vr::controller_process_start(GetCurrentProcess());
        if(!pid_||!game_start_)return false;
        wchar_t map_name[128]{},lock_name[128]{};
        name(map_name,pid_,game_start_,false);name(lock_name,pid_,game_start_,true);
        if(producer){
            SetLastError(0);mutex_=CreateMutexW(nullptr,FALSE,lock_name);
            if(!mutex_||GetLastError()==ERROR_ALREADY_EXISTS){close();return false;}
            SetLastError(0);mapping_=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,
                0,sizeof(EquipmentIntentPacket),map_name);
            if(!mapping_||GetLastError()==ERROR_ALREADY_EXISTS){close();return false;}
        }else{
            mutex_=OpenMutexW(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,lock_name);
            mapping_=OpenFileMappingW(FILE_MAP_READ,FALSE,map_name);
        }
        if(!mutex_||!mapping_){close();return false;}
        data_=static_cast<EquipmentIntentPacket*>(MapViewOfFile(mapping_,
            producer?FILE_MAP_WRITE:FILE_MAP_READ,0,0,sizeof(EquipmentIntentPacket)));
        if(!data_){close();return false;}
        producer_=producer;
        if(!lock_now()){close();return false;}
        if(producer){
            EquipmentIntentPacket packet{};
            packet.game_pid=pid_;packet.game_start=game_start_;
            packet.publisher_pid=pid_;packet.publisher_start=game_start_;
            std::memcpy(data_,&packet,sizeof(packet));
        }
        const bool ok=valid(*data_);
        ReleaseMutex(mutex_);
        if(!ok){close();return false;}
        return true;
    }
    bool publish(EquipmentIntentEvent event) noexcept {
        if(!producer_||!data_||!event_valid(event)||
           !lock_now())return false;
        const bool ok=valid(*data_)&&data_->latest_serial!=UINT64_MAX;
        if(ok){
            event.serial=++data_->latest_serial;event.created_ms=GetTickCount64();
            data_->events[event.serial%EquipmentIntentPacket::capacity]=event;
        }
        ReleaseMutex(mutex_);return ok;
    }
    bool publish_pose(HandWorldFrame pose) noexcept {
        if(!producer_||!data_||!pose_valid(pose)||!lock_now())return false;
        const bool ok=valid(*data_)&&data_->latest_pose_serial!=UINT64_MAX;
        if(ok){
            pose.serial=++data_->latest_pose_serial;pose.created_ms=GetTickCount64();
            data_->pose=pose;
        }
        ReleaseMutex(mutex_);return ok;
    }
    // Caller starts its cursor at UINT64_MAX. Its first read discards all older
    // events, so a newly attached adapter cannot replay a pre-attachment grip.
    bool read_next(std::uint64_t& cursor,EquipmentIntentEvent& out) noexcept {
        if(producer_||!data_||!lock_now())return false;
        EquipmentIntentPacket packet{};
        std::memcpy(&packet,data_,sizeof(packet));ReleaseMutex(mutex_);
        if(!valid(packet))return false;
        if(cursor==UINT64_MAX){cursor=packet.latest_serial;return false;}
        if(packet.latest_serial<=cursor)return false;
        if(packet.latest_serial-cursor>EquipmentIntentPacket::capacity){
            cursor=packet.latest_serial;return false;
        }
        const auto serial=cursor+1;
        const auto event=packet.events[serial%EquipmentIntentPacket::capacity];
        cursor=serial;
        const auto now=GetTickCount64();
        if(event.serial!=serial||!event_valid(event)||
           now<event.created_ms||now-event.created_ms>250)return false;
        out=event;return true;
    }
    bool read_pose(std::uint64_t& cursor,HandWorldFrame& out) noexcept {
        if(producer_||!data_||!lock_now())return false;
        EquipmentIntentPacket packet{};
        std::memcpy(&packet,data_,sizeof(packet));ReleaseMutex(mutex_);
        if(!valid(packet))return false;
        if(cursor==UINT64_MAX){cursor=packet.latest_pose_serial;return false;}
        if(packet.latest_pose_serial<=cursor)return false;
        cursor=packet.latest_pose_serial;
        const auto now=GetTickCount64();
        if(packet.pose.serial!=cursor||!pose_valid(packet.pose)||
           now<packet.pose.created_ms||now-packet.pose.created_ms>100)return false;
        out=packet.pose;return true;
    }
    void close() noexcept {
        if(data_)UnmapViewOfFile(data_);data_=nullptr;
        if(mapping_)CloseHandle(mapping_);mapping_=nullptr;
        if(mutex_)CloseHandle(mutex_);mutex_=nullptr;
        pid_=0;game_start_=0;producer_=false;
    }
};
}
