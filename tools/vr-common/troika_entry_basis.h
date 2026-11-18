#pragma once
#include "weapon_troika_mounted_space.h"
namespace gears_troika_basis {
// Geometry retention on the SAME registered Native incarnation. The Native
// side freezes its own exact current rig publication; no matrix is supplied
// by PG. Neither retention nor cancellation grants input, aim, fire or C0.
constexpr std::uint32_t version=1;
struct Request {
    std::uint32_t size{sizeof(Request)},abi_version{version};
    gears_weapon::TroikaMountKey owner{};
    std::uint64_t entry_publication{};
};
inline bool valid(Request const& q) noexcept {
    namespace h=gears_weapon_hand;
    return q.size==sizeof(q)&&q.abi_version==version&&q.entry_publication&&q.owner.binding&&q.owner.token&&
        q.owner.native.game_pid&&q.owner.native.game_thread&&q.owner.native.game_start&&q.owner.native.native_started_qpc&&q.owner.native.registration_cookie&&
        h::identity_valid(q.owner.world)&&h::identity_valid(q.owner.controller)&&h::identity_valid(q.owner.driver)&&
        h::identity_valid(q.owner.turret)&&h::identity_valid(q.owner.weapon)&&q.owner.driver.address!=q.owner.turret.address;
}
inline bool same(Request const& a,Request const& b) noexcept {
    return valid(a)&&valid(b)&&gears_weapon::troika_same_mount(a.owner,b.owner)&&a.entry_publication==b.entry_publication;
}
#if defined(_WIN32)
using Submit=gears_rig_tracking::Reply(__cdecl*)(Request const*) noexcept;
#else
using Submit=gears_rig_tracking::Reply(*)(Request const*) noexcept;
#endif
inline constexpr char retain_export[]="GearsVrRetainTroikaEntryBasisV1";
inline constexpr char cancel_export[]="GearsVrCancelTroikaEntryBasisV1";
static_assert(sizeof(Request)==264&&alignof(Request)==8);
static_assert(std::is_standard_layout_v<Request>&&std::is_trivially_copyable_v<Request>);
} // namespace gears_troika_basis
