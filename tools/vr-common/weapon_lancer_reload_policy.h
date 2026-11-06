#pragma once
#include <cstdint>

namespace gears_weapon {
enum class LancerPerfectMode : unsigned {off=0,original=1,vr=2};
struct LancerReloadPolicy {
    LancerPerfectMode mode{LancerPerfectMode::vr};
    std::uint32_t tactical_ms{4000},empty_ms{4000};
    constexpr bool valid() const noexcept {
        return unsigned(mode)<=2&&tactical_ms>=100&&tactical_ms<=60000&&empty_ms>=100&&empty_ms<=60000;
    }
    constexpr std::uint64_t packed() const noexcept {
        return std::uint64_t(tactical_ms)|(std::uint64_t(empty_ms)<<16)|(std::uint64_t(mode)<<32);
    }
    static constexpr bool unpack(std::uint64_t value,LancerReloadPolicy& out) noexcept {
        if(value>>34)return false;
        const LancerReloadPolicy candidate{LancerPerfectMode(value>>32),unsigned(value&65535),unsigned((value>>16)&65535)};
        if(!candidate.valid())return false;
        out=candidate;return true;
    }
    constexpr std::int64_t deadline(bool empty,std::int64_t original) const noexcept {
        return mode==LancerPerfectMode::off?0:mode==LancerPerfectMode::original?original:empty?empty_ms:tactical_ms;
    }
};
}
