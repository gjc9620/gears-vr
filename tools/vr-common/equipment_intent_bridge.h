#pragma once
#include "controller_input_bridge.h"
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace gears_weapon {
struct EquipmentIntentEvent {
    std::uint64_t serial{},created_ms{},input_session{},input_sequence{},space_epoch{},world_generation{};
    std::uintptr_t controller{},pawn{},weapon{};
    std::uint8_t inventory_slot{4};
};
struct EquipmentIntentPacket {
    static constexpr std::uint32_t magic_value=0x47564531; // GVE1
    static constexpr std::uint32_t version_value=1;
    static constexpr unsigned capacity=8;
    std::uint32_t magic{magic_value},version{version_value},size{sizeof(EquipmentIntentPacket)},game_pid{};
    std::uint64_t game_start{},publisher_pid{},publisher_start{},latest_serial{};
    EquipmentIntentEvent events[capacity]{};
};
static_assert(std::is_standard_layout_v<EquipmentIntentPacket> &&
              std::is_trivially_copyable_v<EquipmentIntentPacket>);

// Native's certified game-thread input router publishes a slot number and its
// original game identities. The receiving game-thread adapter must still read
// the inventory and revalidate every identity before calling the game.
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
    static void name(wchar_t (&out)[128],DWORD pid,std::uint64_t start,bool lock) noexcept {
        swprintf_s(out,lock?L"Local\\GearsVR.EquipmentIntentLock.v1.%lu.%llu":
                           L"Local\\GearsVR.EquipmentIntent.v1.%lu.%llu",
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
        if(!producer_||!data_||event.inventory_slot>=4||!event.controller||!event.pawn||
           !event.weapon||!event.world_generation||!event.input_session||!event.input_sequence||
           !lock_now())return false;
        const bool ok=valid(*data_)&&data_->latest_serial!=UINT64_MAX;
        if(ok){
            event.serial=++data_->latest_serial;event.created_ms=GetTickCount64();
            data_->events[event.serial%EquipmentIntentPacket::capacity]=event;
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
        if(event.serial!=serial||event.inventory_slot>=4||!event.controller||!event.pawn||
           !event.weapon||!event.world_generation||!event.input_session||!event.input_sequence||
           now<event.created_ms||now-event.created_ms>250)return false;
        out=event;return true;
    }
    void close() noexcept {
        if(data_)UnmapViewOfFile(data_);data_=nullptr;
        if(mapping_)CloseHandle(mapping_);mapping_=nullptr;
        if(mutex_)CloseHandle(mutex_);mutex_=nullptr;
        pid_=0;game_start_=0;producer_=false;
    }
};
}
