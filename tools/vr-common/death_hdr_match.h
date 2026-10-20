#pragma once
#include <cstddef>
#include <cstdint>
#ifdef _WIN32
#include <windows.h>
#include <cstdio>
#endif

// Independent diagnostic mapping. No existing pose/image/GPU bridge ABI changes.
namespace gears_death_hdr_match {
inline constexpr std::uint32_t magic=0x47564854,version=1,packet_bytes=40;
struct alignas(8) Packet {
    std::uint32_t magic{},version{},size{},game_pid{};
    std::uint64_t world{},copy_seq{};
    alignas(8) volatile std::int64_t ticket{}; // One-shot commit marker, published LAST.
};
static_assert(sizeof(Packet)==packet_bytes&&offsetof(Packet,ticket)==32);
struct Snapshot {
    std::uint32_t magic{},version{},size{},game_pid{};
    std::uint64_t world{},copy_seq{},ticket{};
    bool operator==(Snapshot const& b)const{return magic==b.magic&&version==b.version&&size==b.size&&game_pid==b.game_pid&&
        world==b.world&&copy_seq==b.copy_seq&&ticket==b.ticket;}
};
inline bool stable_valid(Snapshot const& a,Snapshot const& b,std::uint32_t pid){
    return a==b&&pid&&a.magic==magic&&a.version==version&&a.size==packet_bytes&&a.game_pid==pid&&a.ticket&&a.world&&a.copy_seq;
}
enum class Decision {waiting,before,matched,missed_after,missed_world,missed_capacity,inactive};
struct Selector {
    Snapshot selected{};bool has_selection{},finished{};
    bool select(Snapshot const& a,Snapshot const& b,std::uint32_t pid){
        if(!stable_valid(a,b,pid))return false;
        if(has_selection)return selected==a; // A selected identity can never change.
        selected=a;has_selection=true;return true;
    }
    Decision decide(std::uint64_t ticket,std::uint64_t world,bool world_available,bool room){
        if(finished)return Decision::inactive;if(!has_selection)return Decision::waiting;
        if(!ticket||ticket<selected.ticket)return Decision::before;
        finished=true;
        if(ticket>selected.ticket)return Decision::missed_after;
        if(world_available&&world!=selected.world)return Decision::missed_world;
        return room?Decision::matched:Decision::missed_capacity;
    }
};
#ifdef _WIN32
inline void mapping_name(wchar_t (&name)[96],std::uint32_t pid){swprintf_s(name,L"Local\\GearsVR.DeathHdrTicket.%lu",static_cast<unsigned long>(pid));}
class Publisher {
    HANDLE mapping_{};volatile Packet* packet_{};bool published_{};
public:
    Publisher()=default;Publisher(Publisher const&)=delete;Publisher& operator=(Publisher const&)=delete;
    ~Publisher(){close();}
    void close(){if(packet_){UnmapViewOfFile(const_cast<Packet*>(packet_));packet_=nullptr;}if(mapping_){CloseHandle(mapping_);mapping_=nullptr;}}
    bool create(std::uint32_t pid){
        if(mapping_||!pid)return false;wchar_t name[96]{};mapping_name(name,pid);SetLastError(0);
        mapping_=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(Packet),name);
        const auto error=GetLastError();if(!mapping_||error==ERROR_ALREADY_EXISTS){close();return false;}
        packet_=static_cast<volatile Packet*>(MapViewOfFile(mapping_,FILE_MAP_ALL_ACCESS,0,0,sizeof(Packet)));
        if(!packet_){close();return false;}
        packet_->magic=magic;packet_->version=version;packet_->size=packet_bytes;packet_->game_pid=pid;
        packet_->world=0;packet_->copy_seq=0;packet_->ticket=0;MemoryBarrier();return true;
    }
    bool publish(std::uint64_t ticket,std::uint64_t world,std::uint64_t copy_seq){
        if(!packet_||published_||!ticket||!world||!copy_seq)return false;
        packet_->world=world;packet_->copy_seq=copy_seq;
        InterlockedExchange64(reinterpret_cast<volatile LONG64*>(&packet_->ticket),static_cast<LONG64>(ticket));
        published_=true;return true;
    }
    bool published()const{return published_;}
};
class Reader {
    HANDLE mapping_{};volatile Packet const* packet_{};ULONGLONG next_open_{};
    static Snapshot load(volatile Packet const* p){
        // Aligned 64-bit load is atomic on the supported Win64 builds. Do not
        // use an interlocked RMW on a FILE_MAP_READ view.
        Snapshot out{};out.ticket=static_cast<std::uint64_t>(p->ticket);MemoryBarrier();
        out.magic=p->magic;out.version=p->version;out.size=p->size;out.game_pid=p->game_pid;
        out.world=p->world;out.copy_seq=p->copy_seq;MemoryBarrier();return out;
    }
public:
    Reader()=default;Reader(Reader const&)=delete;Reader& operator=(Reader const&)=delete;
    ~Reader(){close();}
    void close(){if(packet_){UnmapViewOfFile(const_cast<Packet const*>(packet_));packet_=nullptr;}if(mapping_){CloseHandle(mapping_);mapping_=nullptr;}}
    bool read(std::uint32_t pid,Snapshot& first,Snapshot& second){
        if(!packet_){const auto now=GetTickCount64();if(now<next_open_)return false;next_open_=now+100;
            wchar_t name[96]{};mapping_name(name,pid);mapping_=OpenFileMappingW(FILE_MAP_READ,FALSE,name);if(!mapping_)return false;
            packet_=static_cast<volatile Packet const*>(MapViewOfFile(mapping_,FILE_MAP_READ,0,0,sizeof(Packet)));
            if(!packet_){close();return false;}}
        first=load(packet_);second=load(packet_);return true;
    }
};
#endif
}
