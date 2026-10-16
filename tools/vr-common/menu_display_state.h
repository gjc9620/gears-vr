#pragma once
#include <cstdint>
#include <cstring>

namespace gears_menu {
// main_menu is an explicit whole-window 2D mirror choice. It does not certify
// mono source pixels or authorize projection without a stereo ticket.
enum class Mode : std::uint32_t { unknown, gameplay, death_menu, pause_menu, main_menu=4, loading_display=5, acquisition_wait=6, movie_display=7 };
// Atomic file replacement by the session owner. This authorizes a display
// choice only; it never authorizes native scene mutation or stale pose reuse.
struct Packet {
    std::uint32_t magic{0x47564d31},version{1},size{64},mode{};
    std::uint64_t game_pid{},game_creation{},owner_pid{},owner_creation{},sequence{},observed_ns{};
};
static_assert(sizeof(Packet)==64);
class Validator {
    Packet identity_{},last_{};
    bool movie_display_{};
public:
    Validator(std::uint64_t game,std::uint64_t creation,std::uint64_t owner,std::uint64_t owner_creation,bool movie_display=false):movie_display_(movie_display){
        identity_.game_pid=game;identity_.game_creation=creation;identity_.owner_pid=owner;identity_.owner_creation=owner_creation;
    }
    Mode accept(Packet const& p,std::uint64_t now){
        if(p.magic!=0x47564d31||p.version!=1||p.size!=sizeof(Packet)||p.mode>(movie_display_?7u:6u)||
           !identity_.game_pid||!identity_.game_creation||!identity_.owner_pid||!identity_.owner_creation||
           p.game_pid!=identity_.game_pid||p.game_creation!=identity_.game_creation||
           p.owner_pid!=identity_.owner_pid||p.owner_creation!=identity_.owner_creation||
           !p.sequence||!p.observed_ns||now<p.observed_ns||now-p.observed_ns>(p.mode==7?250000000ull:1500000000ull)||
           p.sequence<last_.sequence||(p.sequence==last_.sequence&&std::memcmp(&p,&last_,sizeof(p))))return Mode::unknown;
        last_=p;return static_cast<Mode>(p.mode);
    }
};
}
