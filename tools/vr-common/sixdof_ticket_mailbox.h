#pragma once
#include <windows.h>
#include "sixdof_ticket_diagnostics.h"

// Diagnostic-only append-only requests. Name binds PID plus process creation
// FILETIME, preventing PID reuse. Each payload also binds XR process, session,
// world and immutable image ticket. Neither side writes game/runtime state.
namespace gears_sixdof {
inline std::uint64_t process_start(HANDLE process) noexcept {
    FILETIME creation{},exit{},kernel{},user{};
    if(!GetProcessTimes(process,&creation,&exit,&kernel,&user))return 0;
    return (std::uint64_t(creation.dwHighDateTime)<<32)|creation.dwLowDateTime;
}
struct PinRequest {
    alignas(8) volatile LONG64 committed{};
    Key key{};std::uint64_t xr_pid{},xr_start{},serial{};unsigned snapshot_index{};
};
struct PinPacket {
    std::uint64_t magic{},version{},pid{},start{};
    std::array<PinRequest,8> slots{};
};
class PinMailbox {
    HANDLE mapping_{};PinPacket* packet_{};
    std::array<bool,8> serviced_{};
    static constexpr std::uint64_t magic=0x534958444F465031ull;
    static void name(wchar_t (&out)[128],std::uint64_t pid,std::uint64_t start) noexcept {
        swprintf_s(out,L"Local\\GearsVR.SixDof.Pin.%llu.%llu",(unsigned long long)pid,(unsigned long long)start);
    }
    bool attach(HANDLE mapping) noexcept {
        if(!mapping)return false;
        auto* view=static_cast<PinPacket*>(MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(PinPacket)));
        if(!view){CloseHandle(mapping);return false;}mapping_=mapping;packet_=view;return true;
    }
public:
    std::uint64_t pid{},start{},unavailable{},rejected{};
    ~PinMailbox(){if(packet_)UnmapViewOfFile(packet_);if(mapping_)CloseHandle(mapping_);}
    bool create() noexcept {
        if(packet_)return true;pid=GetCurrentProcessId();start=process_start(GetCurrentProcess());
        if(!start){++unavailable;return false;}wchar_t path[128]{};name(path,pid,start);
        HANDLE mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(PinPacket),path);
        if(!mapping){++unavailable;return false;}
        if(GetLastError()==ERROR_ALREADY_EXISTS){CloseHandle(mapping);++rejected;return false;}
        if(!attach(mapping)){++unavailable;return false;}
        // Fresh page-file mapping is zero initialized. Publish identity last.
        packet_->version=1;packet_->pid=pid;packet_->start=start;MemoryBarrier();packet_->magic=magic;return true;
    }
    bool open(DWORD game_pid) noexcept {
        if(packet_)return pid==game_pid;
        HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,game_pid);
        if(!process){++unavailable;return false;}auto creation=process_start(process);CloseHandle(process);
        if(!creation){++unavailable;return false;}wchar_t path[128]{};name(path,game_pid,creation);
        if(!attach(OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,path))){++unavailable;return false;}
        MemoryBarrier();
        if(packet_->magic!=magic||packet_->version!=1||packet_->pid!=game_pid||packet_->start!=creation){
            UnmapViewOfFile(packet_);CloseHandle(mapping_);packet_=nullptr;mapping_=nullptr;++rejected;return false;}
        pid=game_pid;start=creation;return true;
    }
    bool request(Key const& key,unsigned index,std::uint64_t serial) noexcept {
        if(!packet_||key.pid!=pid||key.start!=start||!valid(key)||!serial){++rejected;return false;}
        auto xr_start=process_start(GetCurrentProcess());if(!xr_start){++rejected;return false;}
        for(unsigned i=0;i<packet_->slots.size();++i){auto& slot=packet_->slots[i];
            if(InterlockedCompareExchange64(&slot.committed,-1,0)!=0)continue;
            slot.key=key;slot.xr_pid=GetCurrentProcessId();slot.xr_start=xr_start;slot.serial=serial;slot.snapshot_index=index;
            InterlockedExchange64(&slot.committed,LONG64(i+1));return true;}
        ++rejected;return false; // Never overwrite an occupied or unknown request.
    }
    void service(NativeRecorder& recorder) noexcept {
        if(!packet_)return;
        for(unsigned i=0;i<packet_->slots.size();++i){if(serviced_[i])continue;auto& slot=packet_->slots[i];
            auto commit=InterlockedCompareExchange64(&slot.committed,0,0);if(commit<=0)continue;
            // Once committed, writer never reuses this slot for any reason.
            const auto key=slot.key;const auto serial=slot.serial;const auto index=slot.snapshot_index;
            const auto xr_pid=slot.xr_pid,xr_start=slot.xr_start;
            MemoryBarrier();if(InterlockedCompareExchange64(&slot.committed,0,0)!=commit)continue;
            serviced_[i]=true;
            if(commit!=LONG64(i+1)||key.pid!=pid||key.start!=start||!xr_pid||!xr_start){++rejected;continue;}
            HANDLE xr=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,DWORD(xr_pid));
            const bool identity=xr&&process_start(xr)==xr_start;if(xr)CloseHandle(xr);
            if(!identity){++rejected;continue;}
            recorder.pin(i,key,index,serial);
        }
    }
    void dump(FILE* f) const noexcept {
        fprintf(f,"{\"sixdof_pin_mailbox\":true,\"game_pid\":%llu,\"game_start\":%llu,\"available\":%s,\"unavailable\":%llu,\"rejected\":%llu}\n",
            (unsigned long long)pid,(unsigned long long)start,packet_?"true":"false",(unsigned long long)unavailable,(unsigned long long)rejected);
        if(packet_)for(unsigned i=0;i<packet_->slots.size();++i){auto const& r=packet_->slots[i];
            if(InterlockedCompareExchange64(const_cast<volatile LONG64*>(&r.committed),0,0)!=LONG64(i+1))continue;
            fprintf(f,"{\"sixdof_pin_request\":true,\"slot\":%u,",i);print_key(f,r.key);
            fprintf(f,",\"xr_pid\":%llu,\"xr_start\":%llu,\"source_serial\":%llu,\"snapshot_index\":%u,\"serviced_here\":%s}\n",
                (unsigned long long)r.xr_pid,(unsigned long long)r.xr_start,(unsigned long long)r.serial,r.snapshot_index,serviced_[i]?"true":"false");}
    }
};
}
