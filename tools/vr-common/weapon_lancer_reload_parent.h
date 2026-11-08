#pragma once
#include <array>
#include <cstdint>
#include <cstring>

namespace gears_lancer_reload_parent {
// Exact first IsReloading expression in shipped WarHUD.DrawWeaponInfo.
// Offline WarfareGame.u SHA91c0f56e...: memory1dc..216, storage144..16f.
// The later ammunition-order IsReloading query has no matching branch/suffix.
inline constexpr unsigned prefix=26, span=59;
// Shipped WarWeapon.IsReloading.ReturnValue is Parm|OutParm|ReturnParm
// (0x580), NOT Parm|ReturnParm (0x480). Keep the exact reflected ABI.
inline bool query_return_layout(unsigned dimension,unsigned size,std::uint64_t flags,
    unsigned offset,unsigned bool_mask) noexcept {
    return dimension==1&&size==4&&(flags&0x580)==0x580&&offset==0&&bool_mask==1;
}
inline bool direct_bar_gate(std::array<unsigned char,span> const& b,
    unsigned begin,unsigned script_size,std::uintptr_t weapon_property,
    std::uintptr_t reload_function,std::uintptr_t draw_ar_property,
    std::uintptr_t draw_function) noexcept {
    if(!weapon_property||!reload_function||!draw_ar_property||!draw_function||
       begin>script_size||script_size-begin<=225)return false;
    auto pointer=[&](unsigned at,std::uintptr_t expected){
        std::uint64_t value{};std::memcpy(&value,b.data()+at,8);return value==expected;};
    std::uint16_t jump{},context{},skip{};
    std::memcpy(&jump,b.data()+1,2);std::memcpy(&context,b.data()+14,2);
    std::memcpy(&skip,b.data()+28,2);
    return b[0]==0x07&&jump==begin+225&&b[3]==0x82&&b[4]==0x19&&b[5]==0x00&&
        pointer(6,weapon_property)&&context==10&&b[16]==0x04&&b[17]==0x1c&&
        pointer(18,reload_function)&&b[26]==0x16&&b[27]==0x18&&skip==11&&
        b[30]==0x2d&&b[31]==0x00&&pointer(32,draw_ar_property)&&b[40]==0x16&&
        b[41]==0x1c&&pointer(42,draw_function)&&b[50]==0x00&&pointer(51,weapon_property);
}
}
