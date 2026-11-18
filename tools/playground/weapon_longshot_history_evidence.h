#pragma once
#include "../vr-common/controller_relative_motion.h"
#include <algorithm>
#include <cstdio>
#include <type_traits>

// Offline replay evidence only. These are copies of arguments already read by
// the bolt adapter, never an input source or a replacement action receipt.
namespace gears_longshot_history_evidence {
struct Record {
    std::uint32_t magic{0x4c534831},version{1},bytes{},tracked_bytes{},window_bytes{},control{};
    std::uint64_t game_pid{},game_start{},frame{},serial{},token{};
    gears_hand_continuity::TrackedInput previous{},current{};
    gears_hand_continuity::MotionHistory window{};
    gears_vr::Pose fixed_part_in_right_m{};
    double scale{};
};
static_assert(std::is_standard_layout_v<Record> && std::is_trivially_copyable_v<Record>);
inline bool compatible(Record const& r) noexcept {
    return r.magic==0x4c534831 && r.version==1 && r.bytes==sizeof(Record) &&
        r.tracked_bytes==sizeof(r.previous) && r.window_bytes==sizeof(r.window);
}
inline bool write(FILE* log,Record value) noexcept {
    if(!log)return false;
    value.bytes=sizeof(value);value.tracked_bytes=sizeof(value.previous);value.window_bytes=sizeof(value.window);
    const auto* raw=reinterpret_cast<unsigned char const*>(&value);
    constexpr std::size_t chunk=64;
    constexpr wchar_t digits[]=L"0123456789abcdef";
    for(std::size_t offset=0;offset<sizeof(value);offset+=chunk){
        const auto count=(std::min)(chunk,sizeof(value)-offset);wchar_t hex[chunk*2+1]{};
        for(std::size_t i=0;i<count;++i){hex[i*2]=digits[raw[offset+i]>>4];hex[i*2+1]=digits[raw[offset+i]&15];}
        if(fwprintf(log,L"longshot_bolt_history_raw_v1 serial=%llu frame=%llu bytes=%zu offset=%zu hex=%ls\n",
            value.serial,value.frame,sizeof(value),offset,hex)<0)return false;
    }
    return fflush(log)==0;
}
}
