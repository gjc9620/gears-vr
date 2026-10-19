#include "swapchain_resize_mode.h"
#include <cstdio>

static_assert(GEARS_DXGI_RESIZE_HOOKS == EXPECT_RESIZE_HOOKS,
              "Resize wrapper availability must match the selected mode");
static_assert(GEARS_DXGI_RESIZE_TRANSPORT == EXPECT_RESIZE_TRANSPORT,
              "Runtime render size must not acquire game-resolution transport");

int main() {
    std::puts("DXGI resize mode gate PASS");
}
