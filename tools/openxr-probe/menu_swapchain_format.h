#pragma once
#include <dxgiformat.h>
#include <algorithm>
#include <cstdint>
#include <vector>

namespace menu_swapchain_format {
inline bool rgba(DXGI_FORMAT value) noexcept {
    return value==DXGI_FORMAT_R8G8B8A8_UNORM||value==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
}
inline bool bgra(DXGI_FORMAT value) noexcept {
    return value==DXGI_FORMAT_B8G8R8A8_UNORM||value==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
}
inline bool srgb(DXGI_FORMAT value) noexcept {
    return value==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB||value==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
}
inline DXGI_FORMAT choose(std::vector<std::int64_t> const& formats) noexcept {
    // Restore the original menu colors where UNORM is available, while
    // retaining SteamVR's required sRGB fallback and exact encoded upload.
    for(auto candidate:{DXGI_FORMAT_B8G8R8A8_UNORM,DXGI_FORMAT_R8G8B8A8_UNORM,
                        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,DXGI_FORMAT_R8G8B8A8_UNORM_SRGB})
        if(std::find(formats.begin(),formats.end(),std::int64_t(candidate))!=formats.end())return candidate;
    return DXGI_FORMAT_UNKNOWN;
}
inline DXGI_FORMAT choose_gpu(std::vector<std::int64_t> const& formats,DXGI_FORMAT source) noexcept {
    // Match the original source-first path. VDXR can copy UNORM directly;
    // SteamVR needs the established conversion when it omits that format.
    if(source!=DXGI_FORMAT_UNKNOWN&&std::find(formats.begin(),formats.end(),std::int64_t(source))!=formats.end())return source;
    for(auto candidate:{DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
                        DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_B8G8R8A8_UNORM})
        if(std::find(formats.begin(),formats.end(),std::int64_t(candidate))!=formats.end())return candidate;
    return DXGI_FORMAT_UNKNOWN;
}
}
