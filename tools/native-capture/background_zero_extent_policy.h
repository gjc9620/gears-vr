#pragma once
#include <cstdint>

// Experimental decision only: no API calls, stored COM identities, or HWND
// mutations. The integrating control is disabled by default. Supply observations from this resize call,
// never a pointer-keyed history that may outlive the swapchain.
namespace gears_background_zero_extent {
struct Evidence {
    bool trial_enabled{}, outermost{}, owned_hwnd{}, windowed{};
    bool background_verified{}, iconic{}, client_valid{};
    std::uint32_t client_width{}, client_height{};
    // Fresh pre-call GetDesc and D3D12 GetBuffer descriptions must agree.
    // The adapter must release all queried buffer references before forwarding.
    bool physical_verified{};
    std::uint32_t physical_width{}, physical_height{};
};
struct Decision {
    std::uint32_t width{}, height{};
    bool substituted{};
};
constexpr Decision resolve(std::uint32_t width, std::uint32_t height,
                           Evidence const& e) {
    const Decision unchanged{width,height,false};
    if ((width && height) || !e.trial_enabled || !e.outermost ||
        !e.owned_hwnd || !e.windowed || !e.background_verified ||
        !e.physical_verified || e.physical_width<=8 || e.physical_height<=8)
        return unchanged;
    if (!e.iconic && !(e.client_valid && e.client_width<=8 && e.client_height<=8))
        return unchanged;
    return {width ? width : e.physical_width,
            height ? height : e.physical_height,true};
}
}
