#include "menu_swapchain_format.h"
#include <cassert>
#include <cstdio>

int main(){
    using namespace menu_swapchain_format;
    // Restore original VDXR colors without losing SteamVR sRGB support.
    const std::vector<std::int64_t> steamvr{29,91,2,10,24,40,55,45,20};
    const std::vector<std::int64_t> vdxr{29,91,28,87,93,88,10,40,20,45,55};
    assert(choose(steamvr)==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);
    assert(choose_gpu(steamvr,DXGI_FORMAT_R8G8B8A8_UNORM)==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
    for(auto const& formats:{vdxr,std::vector<std::int64_t>{28,87,91,29}}){
        assert(choose(formats)==DXGI_FORMAT_B8G8R8A8_UNORM);
        assert(choose_gpu(formats,DXGI_FORMAT_R8G8B8A8_UNORM)==DXGI_FORMAT_R8G8B8A8_UNORM);
        assert(choose_gpu(formats,DXGI_FORMAT_B8G8R8A8_UNORM)==DXGI_FORMAT_B8G8R8A8_UNORM);
    }
    for(auto format:{DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_B8G8R8A8_UNORM,
                     DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,DXGI_FORMAT_B8G8R8A8_UNORM_SRGB}){
        assert(choose({std::int64_t(format)})==format);
        assert(choose_gpu({std::int64_t(format)},DXGI_FORMAT_R8G8B8A8_UNORM)==format);
        assert(rgba(format)!=bgra(format));
    }
    assert(choose({29,28})==DXGI_FORMAT_R8G8B8A8_UNORM);
    assert(choose_gpu({91,87},DXGI_FORMAT_B8G8R8A8_UNORM)==DXGI_FORMAT_B8G8R8A8_UNORM);
    assert(choose_gpu({24,29},DXGI_FORMAT_R10G10B10A2_UNORM)==DXGI_FORMAT_R10G10B10A2_UNORM);
    assert(choose_gpu({29,91},DXGI_FORMAT_R10G10B10A2_UNORM)==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
    assert(choose({})==DXGI_FORMAT_UNKNOWN);
    assert(choose_gpu({},DXGI_FORMAT_R8G8B8A8_UNORM)==DXGI_FORMAT_UNKNOWN);
    assert(choose({27,90,24,10,40})==DXGI_FORMAT_UNKNOWN);
    assert(choose_gpu({27,90,10,40},DXGI_FORMAT_R8G8B8A8_UNORM)==DXGI_FORMAT_UNKNOWN);
    assert(!rgba(DXGI_FORMAT_R8G8B8A8_TYPELESS));
    assert(!bgra(DXGI_FORMAT_B8G8R8A8_TYPELESS));
    assert(srgb(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)&&srgb(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB));
    assert(!srgb(DXGI_FORMAT_R8G8B8A8_UNORM)&&!srgb(DXGI_FORMAT_B8G8R8A8_UNORM));
    std::puts("original color format tests passed; VDXR UNORM restored; SteamVR sRGB retained");
}
