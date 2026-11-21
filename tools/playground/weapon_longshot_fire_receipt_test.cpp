#include "../vr-common/weapon_longshot_reload.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cwchar>
#include <cstdint>
// Inject only the read/clock boundary. The actual receipt implementation below
// runs unchanged, without a game process, engine dispatcher or pose writer.
using uptr=std::uintptr_t;
FILE* log_file{};
bool fresh=true,registered=true,counts_available=true,field_available=true;
float game_seconds=12.5f;
int used_count=1,capacity=1;
namespace gears_weapon_hand {
struct ObjectIdentity {uptr address{};};
struct Frame {
    struct {std::uint64_t registration_cookie{1},native_started_qpc{2},world_generation{3};} key;
    struct {std::uint64_t session{4},space_epoch{5};} stereo;
    struct {std::uint64_t input_epoch{6},profile_generation{7};} controllers;
    ObjectIdentity world{100},pawn{200},weapon{300};
};
bool same(ObjectIdentity a,ObjectIdentity b){return a.address==b.address;}
bool frame_active(Frame const&,std::int64_t){return fresh;}
}
namespace weaponhand {
unsigned current_gt(void*){return 1;}
std::int64_t clock_ns(void*){return 1;}
struct Client {bool registration_alive(std::uint64_t,std::uint64_t){return registered;}} client;
}
namespace vrequip {
struct Model {std::uint64_t hold_generation(){return 8;}} model;
}
namespace nativelancerfollow {
bool bound_is_longshot=true,bound_key_known=true;
struct {gears_weapon_hand::ObjectIdentity weapon{300};} bound_key;
}
namespace lancerreload_inventory {
struct Identity {
    uptr weapon{};
    bool operator==(Identity const& b)const{return weapon==b.weapon;}
};
Identity identity(gears_weapon_hand::Frame const& f){return {f.weapon.address};}
struct Counts {int capacity{},used{};};
bool integer(uptr,const wchar_t*,int& out){out=used_count;return counts_available;}
bool read(gears_weapon_hand::Frame const&,Counts& out){out={capacity,used_count};return counts_available;}
}
namespace lancerreload_ui {
bool object(gears_weapon_hand::ObjectIdentity const&){return registered;}
}
namespace lancerreload_state {
bool time_field(uptr,const wchar_t*,float& out){out=game_seconds;return field_available;}
}
namespace nativeequipment {
bool current(void*,gears_weapon_hand::Frame const&){return fresh&&registered;}
}
#include "weapon_longshot_fire_receipt.inl"
int main(){
    gears_weapon_hand::Frame f{};
    assert(tmpfile_s(&log_file)==0&&log_file);
    longshot_fire::trace(f,1);
    assert(longshot_fire::pending.active&&!longshot_fire::confirmed.active);
    used_count=0;longshot_fire::service(f);
    assert(longshot_fire::pending.active&&!longshot_fire::confirmed.active);
    used_count=1;longshot_fire::service(f);
    assert(!longshot_fire::pending.active&&longshot_fire::confirmed.shot.confirmed);
    assert(longshot_fire::confirmed.shot.game_tick==12500000);
    rewind(log_file);wchar_t line[512]{};assert(fgetws(line,512,log_file));
    assert(std::wcsstr(line,L"shot=1 game_tick_us=12500000 used=1 capacity=1"));
    assert(std::wcsstr(line,L"weapon=12c world=64 pawn=c8 hold=8"));
    assert(fseek(log_file,0,SEEK_END)==0);
    game_seconds=20;longshot_fire::trace(f,1);
    assert(!longshot_fire::pending.active&&longshot_fire::confirmed.shot.game_tick==12500000);
    longshot_fire::trace(f,2);fresh=false;longshot_fire::service(f);
    assert(longshot_fire::pending.active&&longshot_fire::confirmed.shot.original_event_sequence==1);
    fresh=true;capacity=2;longshot_fire::service(f);
    assert(longshot_fire::pending.active);
    capacity=1;f.weapon.address=301;longshot_fire::service(f);
    assert(!longshot_fire::pending.active);
    f.weapon.address=300;registered=false;longshot_fire::trace(f,3);
    assert(!longshot_fire::pending.active);
    registered=true;field_available=false;longshot_fire::trace(f,3);
    assert(!longshot_fire::pending.active);
    field_available=true;game_seconds=-1;longshot_fire::trace(f,3);
    assert(!longshot_fire::pending.active);
    game_seconds=30;longshot_fire::trace(f,3);longshot_fire::service(f);
    assert(longshot_fire::confirmed.shot.original_event_sequence==3&&longshot_fire::confirmed.shot.game_tick==30000000);
    fclose(log_file);log_file=nullptr;
    std::puts("Longshot production fire receipt: ammo confirmation, clock, freshness, identity and replay checks passed");
}
