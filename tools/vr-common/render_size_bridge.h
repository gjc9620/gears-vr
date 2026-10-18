#pragma once
#include "render_size_plan.h"
#include <windows.h>
#include <cstring>

namespace gears_vr {
struct RenderSizePacket {
    unsigned magic{0x47565231},version{1},size{sizeof(RenderSizePacket)},target_pid{};
    std::uint64_t game_creation{},session{};
    RenderSizePlan plan{};
};
inline std::uint64_t render_size_process_creation(HANDLE process) {
    FILETIME a{},b{},c{},d{};if(!GetProcessTimes(process,&a,&b,&c,&d))return 0;
    return (std::uint64_t(a.dwHighDateTime)<<32)|a.dwLowDateTime;
}
// A separate mapping avoids changing the pose/image-ticket ABI. Its producer
// retains the mapping until XR shutdown; native freezes a validated copy.
class RenderSizeBridge {
    HANDLE mapping_{};RenderSizePacket const* view_{};
public:
    ~RenderSizeBridge(){close();}
    RenderSizeBridge()=default;RenderSizeBridge(RenderSizeBridge const&)=delete;
    RenderSizeBridge& operator=(RenderSizeBridge const&)=delete;
    bool publish(DWORD pid,std::uint64_t session,RenderSizePlan const& plan) {
        if(mapping_||!pid||!session||!render_size_valid(plan))return false;
        HANDLE game=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);if(!game)return false;
        const auto creation=render_size_process_creation(game);CloseHandle(game);if(!creation)return false;
        wchar_t name[96]{};swprintf_s(name,L"Local\\GearsVR.RenderSize.%lu",pid);
        SetLastError(0);mapping_=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(RenderSizePacket),name);
        if(!mapping_||GetLastError()==ERROR_ALREADY_EXISTS){close();return false;}
        auto* value=static_cast<RenderSizePacket*>(MapViewOfFile(mapping_,FILE_MAP_WRITE,0,0,sizeof(RenderSizePacket)));
        if(!value){close();return false;}
        RenderSizePacket packet{};packet.target_pid=pid;packet.game_creation=creation;packet.session=session;packet.plan=plan;
        // Readiness is published last; a transient/torn read fails validation.
        auto pending=packet;pending.magic=0;std::memcpy(value,&pending,sizeof(pending));
        MemoryBarrier();InterlockedExchange(reinterpret_cast<volatile LONG*>(&value->magic),LONG(packet.magic));
        UnmapViewOfFile(value);return true;
    }
    bool read_current(RenderSizePacket& out) {
        if(!mapping_){wchar_t name[96]{};swprintf_s(name,L"Local\\GearsVR.RenderSize.%lu",GetCurrentProcessId());
            mapping_=OpenFileMappingW(FILE_MAP_READ,FALSE,name);if(!mapping_)return false;
            view_=static_cast<RenderSizePacket const*>(MapViewOfFile(mapping_,FILE_MAP_READ,0,0,sizeof(RenderSizePacket)));
            if(!view_){close();return false;}}
        RenderSizePacket a{},b{};std::memcpy(&a,view_,sizeof(a));MemoryBarrier();std::memcpy(&b,view_,sizeof(b));
        if(std::memcmp(&a,&b,sizeof(a))||a.magic!=0x47565231||a.version!=1||a.size!=sizeof(a)||
           a.target_pid!=GetCurrentProcessId()||!a.session||!a.game_creation||
           a.game_creation!=render_size_process_creation(GetCurrentProcess())||!render_size_valid(a.plan))return false;
        out=a;return true;
    }
    void close(){if(view_)UnmapViewOfFile(view_);view_=nullptr;if(mapping_)CloseHandle(mapping_);mapping_=nullptr;}
};
} // namespace gears_vr
