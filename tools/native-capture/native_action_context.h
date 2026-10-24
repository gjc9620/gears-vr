#pragma once
#include <array>
#include <cstdint>
#include <limits>
#include <mutex>
#include <type_traits>

namespace gears_native_context {
// Internal Native bookkeeping only. This ledger grants no engine operation or
// game-field rights. Permission also needs the broker's current authenticated GT.
enum class Kind:unsigned {scene,stereo,action,restore,callback,count};
struct Ticket;
struct Ledger {
    std::array<unsigned,static_cast<unsigned>(Kind::count)> depth{};
    Ticket* top{};
    std::uint64_t next{1};
    bool poisoned{};
};
struct Ticket {
    Ledger* owner{};Ticket* previous{};Ticket* address{};
    std::uint64_t serial{};Kind kind{};bool active{};
};
static_assert(std::is_trivially_copyable_v<Ticket>&&std::is_trivially_destructible_v<Ticket>);
inline thread_local Ledger thread_ledger{};
inline bool enter(Ledger& ledger,Ticket& ticket,Kind kind) noexcept {
    const auto index=static_cast<unsigned>(kind);
    if(ticket.owner||ticket.active||ticket.serial||index>=ledger.depth.size()||
       ledger.next==(std::numeric_limits<std::uint64_t>::max)()||
       (index<ledger.depth.size()&&ledger.depth[index]==(std::numeric_limits<unsigned>::max)())){
        ledger.poisoned=true;return false;
    }
    ticket.owner=&ledger;ticket.previous=ledger.top;ticket.address=&ticket;
    ticket.serial=ledger.next++;ticket.kind=kind;ticket.active=true;
    ledger.top=&ticket;++ledger.depth[index];return true;
}
inline bool leave(Ledger& ledger,Ticket& ticket) noexcept {
    const auto index=static_cast<unsigned>(ticket.kind);
    if(!ticket.active||ticket.owner!=&ledger||ticket.address!=&ticket||!ticket.serial||
       ledger.top!=&ticket||index>=ledger.depth.size()||!ledger.depth[index]){
        ledger.poisoned=true;return false;
    }
    ledger.top=ticket.previous;--ledger.depth[index];ticket.active=false;return true;
}
inline bool engine_context_clear(Ledger const& ledger,bool authenticated_game_thread) noexcept {
    if(!authenticated_game_thread||ledger.poisoned||ledger.top)return false;
    for(auto value:ledger.depth)if(value)return false;
    return true;
}

// Drop-in BasicLockable/Lockable. The token lives in the mutex, through the
// whole ownership interval; another thread cannot access it before acquisition.
// Tracking failure poisons permission but does not change mutex acquisition.
class StereoMutex {
    std::mutex mutex_;Ticket ticket_{};bool tracked_{};
    void acquired() noexcept {
        if(ticket_.active){thread_ledger.poisoned=true;tracked_=false;return;}
        ticket_={};tracked_=enter(thread_ledger,ticket_,Kind::stereo);
    }
public:
    StereoMutex()=default;
    StereoMutex(StereoMutex const&)=delete;
    StereoMutex& operator=(StereoMutex const&)=delete;
    void lock(){mutex_.lock();acquired();}
    bool try_lock(){if(!mutex_.try_lock())return false;acquired();return true;}
    void unlock() noexcept {
        if(tracked_)(void)leave(thread_ledger,ticket_);
        tracked_=false;mutex_.unlock();
    }
};
} // namespace gears_native_context
